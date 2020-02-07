#pragma once
#ifndef MAI_LANG_MACHINE_H_
#define MAI_LANG_MACHINE_H_

#include "lang/isolate-inl.h"
#include "lang/value-inl.h"
#include "lang/mm.h"
#include "base/base.h"
#include "glog/logging.h"
#include <thread>
#include <atomic>

namespace mai {

namespace lang {

class Scheduler;
class HandleScope;
class Coroutine;

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
    };
    
    static constexpr int kMaxFreeCoroutines = 10;
    
    Machine(int id, Scheduler *owner);
    ~Machine();
    
    DEF_VAL_GETTER(int, id);
    DEF_VAL_GETTER(int, n_free);
    DEF_VAL_GETTER(int, n_runnable);
    DEF_PTR_GETTER(Coroutine, running);

    // Get current thread machine object
    static Machine *This() { return TLS_STORAGE->machine; }

    // Value's factory
    AbstractValue *ValueOfNumber(BuiltinType primitive_type, const void *value, size_t n);
    
    // New a box-in number object
    AbstractValue *NewNumber(BuiltinType primitive_type, const void *value, size_t n, uint32_t flags);
    
    // New a UTF-8 encoding string
    String *NewUtf8String(const char *utf8_string, size_t n, uint32_t flags);
    
    // New (immutable) array slowly
    AbstractArray *NewArraySlow(BuiltinType type, size_t length, uint32_t flags);
    
    AbstractArray *NewArrayCopiedSlow(const AbstractArray *origin, size_t increment, uint32_t flags);
    
    // New native closure
    Closure *NewClosure(Code *code, size_t captured_var_size, uint32_t flags);
    
    // New bytecode closure
    Closure *NewClosure(Function *func, size_t captured_var_size, uint32_t flags);
    
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
    int UpdateRememberRecords(Any *host, Any **address, size_t n) {
        // TODO:
        DLOG(ERROR) << "TODO: batch write barrier : " << n;
        return 0;
    }

    DEF_PTR_GETTER(HandleScopeSlot, top_slot);

    friend class Isolate;
    friend class Scheduler;
    friend class MachineScope;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Machine);
private:
    void Entry() {
        // TODO:
        TODO();
    }

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
    
    void InsertFreeCoroutine(Coroutine *co);
    
    Coroutine *TakeFreeCoroutine();

    const int id_; // Machine id
    Scheduler *const owner_; // Scheduler
    HandleScopeSlot *top_slot_ = nullptr; // Top of handle-scope slot pointer
    std::atomic<State> state_ = kDead; // Current machine state
    uint64_t exclusion_ = 0; // Exclusion counter if > 0 can not be preempted
    Coroutine *free_dummy_; // Local free coroutines(coroutine pool)
    Coroutine *runnable_dummy_; // Waiting for running coroutines
    int n_free_ = 0; // Number of free coroutine
    int n_runnable_ = 0; // Number of runnable coroutine
    Coroutine *running_ = nullptr; // Current running coroutine
    std::thread thread_; // Thread object
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
