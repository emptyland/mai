#ifndef MAI_NYAA_THREAD_H_
#define MAI_NYAA_THREAD_H_

#include "nyaa/nyaa-values.h"
#include "nyaa/function.h"
#include "asm/utils.h"
#include "mai/error.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
class TryCatch;
class NyaaCore;
class NyRunnable;
class NyByteArray;
class RootVisitor;
class NyThread;
    
class CallFrame {
public:
    enum ExceptionId {
        kNormal,
        kException,
        kYield,
    };

    using Template = arch::ObjectTemplate<CallFrame, int32_t>;

    static const int32_t kOffsetCallee;
    static const int32_t kOffsetEnv;
    static const int32_t kOffsetConstPool;
    static const int32_t kOffsetStackBE;
    static const int32_t kOffsetStackBP;
    static const int32_t kOffsetStackTP;
    
    CallFrame() {}
    ~CallFrame() {}
    
    void Enter(NyThread *owns, NyRunnable *callee, NyByteArray *bcbuf, NyArray *kpool, int wanted,
               size_t bp, size_t tp, NyMap *env);
    void Exit(NyThread *owns);

    DEF_PTR_GETTER(NyRunnable, callee);
    DEF_PTR_GETTER(NyMap, env);
    DEF_PTR_GETTER(NyByteArray, bcbuf);
    DEF_PTR_GETTER(NyArray, const_pool);
    DEF_PTR_GETTER(CallFrame, prev);
    DEF_VAL_GETTER(int, level);
    DEF_VAL_GETTER(int, pc);
    DEF_VAL_PROP_RW(int, nrets);
    DEF_VAL_GETTER(int, wanted);
    DEF_VAL_GETTER(size_t, stack_be);
    DEF_VAL_GETTER(size_t, stack_bp);
    DEF_VAL_GETTER(size_t, stack_tp);
    
    std::tuple<NyString *, NyInt32Array *> FileInfo() const;
    
    NyFunction *proto() const {
        NyClosure *ob = DCHECK_NOTNULL(callee_->ToClosure());
        return ob->proto();
    }
    
    Object *upval(int32_t index) const {
        NyClosure *ob = DCHECK_NOTNULL(callee_->ToClosure());
        return ob->upval(index);
    }
    
    void SetUpval(int32_t index, Object *value, NyaaCore *core) {
        NyClosure *ob = DCHECK_NOTNULL(callee_->ToClosure());
        ob->Bind(index, value, core);
    }
    
    void AddPC(int delta) {
        DCHECK_GE(pc_ + delta, 0);
        DCHECK_LE(pc_ + delta, bcbuf_->size());
        pc_ += delta;
    }
    
    void AdjustBP(int delta) {
        stack_bp_ += delta;
        stack_tp_ += delta;
    }
    
    size_t GetNVargs() const {
        DCHECK_GE(stack_bp_, stack_be_);
        return stack_bp_ - stack_be_;
    }
    
    std::tuple<NyString *, NyString *, int> GetCurrentSourceInfo() {
        if (NyClosure *fn = NyClosure::Cast(callee_)) {
            int line = static_cast<int>(proto()->file_info()->Get(pc()));
            return {proto()->file_name(), proto()->name(), line};
        }
        return {nullptr, nullptr, 0};
    }
    
    Byte BC() const { return bcbuf_->Get(pc_); }
    
    void IterateRoot(RootVisitor *visitor);

    friend class NyThread;
    DISALLOW_IMPLICIT_CONSTRUCTORS(CallFrame);
private:
    NyRunnable *callee_; // [strong ref]
    NyMap *env_; // [strong ref]
    NyByteArray *bcbuf_ = nullptr; // [strong ref]
    NyArray *const_pool_ = nullptr; // [strong ref]
    CallFrame *prev_;
    int level_ = 0;
    int pc_ = 0;
    size_t stack_be_ = 0;
    size_t stack_bp_ = 0;
    size_t stack_tp_ = 0;
    //ExceptionId eid_ = kNormal;
    int nrets_ = 0; // record real return results.
    int wanted_ = 0; // return wanted results.
};
    
class TryCatchCore {
public:
    static constexpr const int kThread = 0;
    static constexpr const int kMessage = 1;
    static constexpr const int kException = 2;
    static constexpr const int kStackTrace = 3;
    static constexpr const int kSlotSize = 4;
    
    TryCatchCore(NyaaCore *core);
    TryCatchCore(NyaaCore *core, NyThread *thread);
    ~TryCatchCore();
    
    void Catch(NyString *message, Object *exception, NyArray *stack_trace);
    void Reset() {
        has_caught_  = false;
        obs_[kMessage] = nullptr;
        obs_[kException] = nullptr;
        obs_[kStackTrace] = nullptr;
    }
    
    DEF_VAL_GETTER(bool, has_caught);
    DEF_PTR_GETTER(TryCatchCore, prev);
    inline NyThread *thread() const;
    NyString *message() const { return static_cast<NyString *>(obs_[kMessage]); }
    Object *exception() const { return obs_[kException]; }
    NyArray *stack_trace() const { return static_cast<NyArray *>(obs_[kStackTrace]); }
    
    std::string ToString() const;
    
    //friend class TryCatch;
    DISALLOW_IMPLICIT_CONSTRUCTORS(TryCatchCore);
private:
    NyaaCore *const core_;
    bool has_caught_ = false;
    TryCatchCore *prev_ = nullptr;
    Object **obs_ = nullptr;
};
    
class NyThread : public NyUDO {
public:
    enum State {
        kSuspended,
        kRunning,
        kDead,
    };
    
    using Template = arch::ObjectTemplate<NyThread, int32_t>;
    
    static const int32_t kOffsetOwns;
    static const int32_t kOffsetInterruptionPending;
    static const int32_t kOffsetSavePoint;
    static const int32_t kOffsetFrame;
    static const int32_t kOffsetStack;
    
    NyThread(NyaaCore *owns);
    ~NyThread();
    
    Error Init();
    
    DEF_PTR_PROP_RW(NyRunnable, entry);
    DEF_PTR_PROP_RW(NyThread, save);
    DEF_VAL_PROP_RW(State, state);
    CallFrame *call_info() const { return DCHECK_NOTNULL(frame_); }
    
    size_t frame_size() const {
        DCHECK_GE(stack_tp_, stack_);
        DCHECK_LT(stack_tp_, stack_last_);
        return stack_tp_ - stack_;
    }
    
    int TryRun(NyRunnable *fn, Object *argv[], int argc, int nrets = 0, NyMap *env = nullptr);
    int Run(NyRunnable *fn, Object *argv[], int argc, int nrets = 0, NyMap *env = nullptr);
    int Resume(Object *argv[], int argc, int wanted, NyMap *env);
    void Yield();
    
    void Raisef(const char *fmt, ...);
    void Vraisef(const char *fmt, va_list ap);
    void Raise(NyString *msg, Object *ex);

    void Push(Object *value, size_t n = 1);
    Object *Get(int i);
    void Set(int i, Object *value);
    
    void Pop(int n) {
        DCHECK_GE(stack_tp_ - n, frame_bp());
        stack_tp_ -= n;
    }
    
    void PrintStack();
    
    void IterateRoot(RootVisitor *visitor);
    
    void Iterate(ObjectVisitor *) {}
    
    constexpr size_t PlacedSize() const { return sizeof(*this); }
    
    friend class NyaaCore;
    friend class TryCatchCore;
    friend class CallFrame;
    friend class Runtime;
    DEF_HEAP_OBJECT(Thread);
private:

    Object **frame_bp() const { return !frame_ ? stack_ : stack_ + frame_->stack_bp(); }
    Object **frame_tp() const { return !frame_ ? stack_tp_ : stack_ + frame_->stack_tp(); }
    Object **frame_be() const { return !frame_ ? stack_ : stack_ + frame_->stack_be(); }
    
    void CheckStackAdd(size_t add) {
        DCHECK_GE(stack_tp_, stack_);
        CheckStack(stack_tp_ - stack_ + add);
    }
    void CheckStack(size_t size);
    void Unwind(CallFrame *ci);
    int Run();

    Object *InternalGetField(Object **base, Object *mm, Object *key);
    int InternalSetField(Object **base, Object *key, Object *value);
    NyUDO *InternalNewUdo(Object **args, int32_t n_args, size_t size, NyMap *clazz);
    
    int InternalCallMetaFunction(Object **base, Object *a1, Object *a2, int wanted,
                                 NyString *name, bool *has);
    bool InternalCallMetaFunction(Object **base, NyString *name, int wanted, Object *a1,
                                  int n, ...);
    int InternalCall(Object **func, int32_t n_args, int wanted);
    
    int RuntimeCall(int32_t callee, int32_t nargs, int wanted);
    int RuntimeRet(int32_t base, int32_t nrets);

    int CopyArgs(Object **args, int n_args, int n_params, bool vargs);
    
    void CopyResult(Object **ret, int n_rets, int wanted);
    void CopyResult(Object **ret, Object **argv, int argc, int wanted);
    
    void ProcessClass(NyMap *clazz);
    
    int ParseBytecodeInt32Params(int offset, int scale, int n, ...);
    int ParseBytecodeSize(int offset);
    
    void Bind(int i, NyClosure *closure, const NyFunction::UpvalDesc &desc) {
        CallFrame *up = frame_;
        Object *val = desc.in_stack ? stack_[up->stack_bp() + desc.index] :
                      static_cast<NyClosure *>(up->callee())->upval(desc.index);
        closure->Bind(i, val, owns_);
    }
    
    NyaaCore *const owns_;
    TryCatchCore *catch_point_ = nullptr;
    CallFrame::ExceptionId interruption_pending_ = CallFrame::kNormal;
    arch::RegisterContext save_point_;
    State state_ = kSuspended;
    Object **stack_ = nullptr; // elements [strong ref]
    Object **stack_tp_ = nullptr;
    Object **stack_last_ = nullptr;
    NyRunnable *entry_ = nullptr; // [strong ref] The entry script.
    size_t stack_size_ = 0;
    CallFrame *frame_ = nullptr; // elements [strong ref]
    NyThread *save_ = nullptr; // [strong ref]
    NyThread *prev_ = this; // [strong ref]
    NyThread *next_ = this; // [strong ref]
}; // class NyThread

inline NyThread *TryCatchCore::thread() const { return static_cast<NyThread *>(obs_[kThread]); }
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_THREAD_H_
