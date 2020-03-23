#pragma once
#ifndef MAI_LANG_MACHINE_H_
#define MAI_LANG_MACHINE_H_

#include "lang/isolate-inl.h"
#include "lang/value-inl.h"
#include "lang/heap.h"
#include "base/base.h"
#include "glog/logging.h"
#include <thread>
#include <atomic>

namespace mai {

namespace lang {

class Scheduler;
class HandleScope;
class Coroutine;
class RootVisitor;

struct HandleScopeSlot {
    HandleScope     *scope;
    HandleScopeSlot *prev;
    Address          base;
    Address          end;
    Address          limit;
};

class Machine {
public:
    enum State: int {
        kDead,
        kIdle,
        kRunning,
        kPanic,
    };
    using TaskFunc = void (*)();
    
    static constexpr int kMaxFreeCoroutines = 10;
    
    Machine(int id, Scheduler *owner);
    ~Machine();
    
    DEF_VAL_GETTER(int, id);
    DEF_VAL_GETTER(int, n_free);
    DEF_PTR_GETTER(Coroutine, running);
    DEF_VAL_GETTER(uint64_t, user_time);

    // Get machine state
    State state() const { return state_.load(std::memory_order_acquire); }

    // Set machine state
    void set_state(State state) { state_.store(state, std::memory_order_release); }

    // Add counter when some exception uncaught.
    void IncrmentUncaughtCount() { uncaught_count_++; }
    
    DEF_VAL_GETTER(int, uncaught_count);
    
    int GetNumberOfRunnable() const {
        std::lock_guard<std::mutex> lock(runnable_mutex_);
        return n_runnable_;
    }
    
    // Start Machine
    void Start();
    
    // Post coroutine to runnable
    void PostRunnable(Coroutine *co, bool now = true);
    
    // Post coroutine to waitting list
    void PostWaitting(Coroutine *co);

    // Get current thread machine object
    static Machine *This() { return DCHECK_NOTNULL(TLS_STORAGE->machine); }

    // New object by type information
    // NOTICE: Just allocation object and initialize Any class
    Any *NewObject(const Class *clazz, uint32_t flags);

    // Value's factory
    AbstractValue *ValueOfNumber(BuiltinType primitive_type, const void *value, size_t n);

    // New a box-in number object
    AbstractValue *NewNumber(BuiltinType primitive_type, const void *value, size_t n, uint32_t flags);
    
    // Create a captured value
    CapturedValue *NewCapturedValue(const Class *clazz, const void *value, size_t n, uint32_t flags);

    // New a UTF-8 encoding string
    String *NewUtf8String(const char *utf8_string, size_t n, uint32_t flags);

    __attribute__ (( __format__ (__printf__, 3, 4)))
    String *NewUtf8StringWithFormat(uint32_t flags, const char *fmt, ...);

    String *NewUtf8StringWithFormatV(uint32_t flags, const char *fmt, va_list ap);

    // New (immutable/mutable) array slowly
    AbstractArray *NewArray(BuiltinType type, size_t length, size_t capacity, uint32_t flags);

    AbstractArray *NewArrayCopied(const AbstractArray *origin, size_t increment, uint32_t flags);
    
    // New (mutable) array
    AbstractArray *NewMutableArray(BuiltinType type, size_t length, size_t capacity, uint32_t flags);
    
    AbstractArray *NewMutableArray8(const void *init_data, size_t length, size_t capacity, uint32_t flags);
    
    AbstractArray *ResizeMutableArray(AbstractArray *origin, size_t new_size);
    
    String *Array8ToString(AbstractArray *from);

    // New native closure
    Closure *NewClosure(Code *code, size_t captured_var_size, uint32_t flags);
    
    // New bytecode closure
    Closure *NewClosure(Function *func, size_t captured_var_size, uint32_t flags);
    
    // Close function and make a closure
    Closure *CloseFunction(Function *func, uint32_t flags);
    
    // New Panic error object
    Throwable *NewPanic(Panic::Level code, String *message, uint32_t flags);
    
    // New base of Exception object
    Exception *NewException(uint32_t type, String *message, Exception *cause, uint32_t flags);
    
    // New channel
    Channel *NewChannel(uint32_t data_type, size_t capacity, uint32_t flags);
    
    // Throw a panic
    void ThrowPanic(Panic::Level level, String *message);
    
    // Handle scope enter
    void EnterHandleScope(HandleScope *handle_scope) {
        auto prev_slot = top_slot_;
        auto slot = new HandleScopeSlot{DCHECK_NOTNULL(handle_scope), prev_slot};
        slot->base  = prev_slot->end;
        slot->end   = slot->base;
        slot->limit = prev_slot->limit;
        top_slot_ = slot;
    }

    // Handle scope exit
    void ExitHandleScope();
    
    // Allocate 'n_slots' slots for handle
    Address AdvanceHandleSlots(int n_slots);

    HandleScope *top_handle_scope() const { return !top_slot_ ? nullptr : top_slot_->scope; }
    
    // Update local remember-set record
    ALWAYS_INLINE int UpdateRememberRecords(Any *host, Any **address, size_t n) {
        int count = 0;
        for (size_t i = 0; i < n; i++) {
            if (STATE->heap()->InOldArea(host)) {
                if (*address == nullptr) {
                    remember_set_.erase(address);
                } else if (STATE->heap()->InNewArea(*address)) {
                    AddRememberRecord(host, address);
                }
            }
        }
        return count;
    }
    
    ALWAYS_INLINE static bool ShouldRemember(Any *host, Any *val) {
        return STATE->heap()->InOldArea(host) && val != nullptr && STATE->heap()->InNewArea(val);
    }

    DEF_PTR_GETTER(HandleScopeSlot, top_slot);
    
    void TakeWaittingCoroutine(Coroutine *co);
    
    void VisitRoot(RootVisitor *visitor);

    friend class Isolate;
    friend class Scheduler;
    friend class MachineScope;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Machine);
private:
    using RememberSet = std::map<void *, RememberRecord>;
    
    ALWAYS_INLINE void AddRememberRecord(Any *host, Any **address) {
        DCHECK(ShouldRemember(host, *address));
        RememberRecord rd {
            STATE->heap()->NextRememberRecordSequanceNumber(),
            host,
            address,
        };
        remember_set_[address] = rd;
    }
    
    void Entry(); // Machine entry point

    void Enter() {
        DCHECK(__tls_storage == nullptr);
        TLSStorage *tlss = new TLSStorage(this);
        tlss->mid = id_;
        __tls_storage = tlss;
    }

    void Exit() {
        DCHECK_EQ(this, TLS_STORAGE->machine);
        delete __tls_storage;
        __tls_storage = nullptr;
    }
    
    void AllocationPanic(AllocationResult result) { AllocationPanic(result.result()); }
    
    void AllocationPanic(AllocationResult::Result result);
    
    void PrintStacktrace(const char *message);
    
    static Address GetPrevStackFrame(Address frame_bp, Address stack_hi);
    
    void InsertFreeCoroutine(Coroutine *co);
    
    Coroutine *TakeFreeCoroutine();

    const int id_; // Machine id
    Scheduler *const owner_; // Scheduler
    HandleScopeSlot *top_slot_ = nullptr; // [nested strong ref] Top of handle-scope slot pointer
    std::atomic<State> state_ = kDead; // Current machine state
    int uncaught_count_ = 0; // Counter of uncaught
    uint64_t exclusion_ = 0; // Exclusion counter if > 0 can not be preempted
    Coroutine *free_dummy_; // Local free coroutines(coroutine pool)
    Coroutine *runnable_dummy_; // [nested strong ref] Waiting for running coroutines
    Coroutine *waitting_dummy_; // [nested strong ref] Waiting for wakeup coroutines
    mutable std::mutex runnable_mutex_; // Mutex for runnable_dummy_
    mutable std::mutex waitting_mutex_; // Mutex for waitting_dummy_
    int n_free_ = 0; // Number of free coroutine
    int n_runnable_ = 0; // Number of runnable coroutines
    int n_waitting_ = 0; // Number of waitting coroutines
    Coroutine *running_ = nullptr; // [nested strong ref] Current running coroutine
    uint64_t user_time_ = 0; // User time for realy code execution
    std::condition_variable cond_var_; // Condition variable for scheduling
    std::mutex mutex_; // Total mutex
    std::thread thread_; // Thread object
    RememberSet remember_set_; // elements [weak ref] Local remember set
}; // class Machine


class MachineScope final {
public:
    MachineScope(Machine *owns) : owns_(owns) { owns_->Enter(); }
    ~MachineScope() { owns_->Exit(); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(MachineScope);
private:
    Machine *const owns_;
}; // class MachineScope

} // namespace lang

} // namespace mai


#endif // MAI_LANG_MACHINE_H_
