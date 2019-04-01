#ifndef MAI_NYAA_THREAD_H_
#define MAI_NYAA_THREAD_H_

#include "nyaa/nyaa-values.h"
#include "mai/error.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
class TryCatch;
class NyaaCore;
class NyScript;
class NyRunnable;
class NyByteArray;
class RootVisitor;
class NyThread;
    
class CallFrame {
public:
    CallFrame() {}
    ~CallFrame() {}
    
    void Enter(NyThread *owns, NyRunnable *callee, NyByteArray *bcbuf, NyArray *kpool, int wanted,
               Object **bp, Object **tp, NyMap *env);
    void Exit(NyThread *owns);

    DEF_PTR_GETTER(NyRunnable, callee);
    DEF_PTR_GETTER(NyMap, env);
    DEF_PTR_GETTER(NyByteArray, bcbuf);
    DEF_PTR_GETTER(NyArray, const_poll);
    DEF_PTR_GETTER(CallFrame, prev);
    DEF_VAL_GETTER(int, level);
    DEF_VAL_GETTER(int, pc);
    DEF_VAL_GETTER(int, wanted);
    DEF_VAL_GETTER(size_t, stack_bp);
    DEF_VAL_GETTER(size_t, stack_tp);
    
    void AddPC(int delta) {
        DCHECK_GE(pc_ + delta, 0);
        DCHECK_LE(pc_ + delta, bcbuf_->size());
        pc_ += delta;
    }
    
    Byte BC() const { return bcbuf_->Get(pc_); }

    friend class NyThread;
    DISALLOW_IMPLICIT_CONSTRUCTORS(CallFrame);
private:
    NyRunnable *callee_; // [strong ref]
    NyMap *env_; // [strong ref]
    NyByteArray *bcbuf_ = nullptr; // [strong ref]
    NyArray *const_poll_ = nullptr; // [strong ref]
    CallFrame *prev_;
    int level_ = 0;
    int pc_ = 0;
    size_t stack_bp_ = 0;
    size_t stack_tp_ = 0;
    int wanted_ = 0; // return wanted results.
};
    
class NyThread : public NyUDO {
public:
    enum State {
        kSuspended,
        kRunning,
        kDead,
    };
    
    NyThread(NyaaCore *owns);
    ~NyThread();
    
    Error Init();
    
    DEF_PTR_PROP_RW(NyRunnable, entry);
    DEF_PTR_PROP_RW(NyThread, save);
    DEF_VAL_PROP_RW(State, state);
    
    size_t frame_size() const {
        DCHECK_GE(stack_tp_, stack_);
        DCHECK_LT(stack_tp_, stack_last_);
        return stack_tp_ - stack_;
    }

    int Run(NyScript *entry, int n_result, NyMap *env = nullptr);
    
    int Run(NyFunction *fn, Arguments *args, int n_result, NyMap *env = nullptr);
    
    int Resume(Arguments *args, NyThread *save, NyMap *env = nullptr);
    
    //int Yield(Arguments *args);

    void Push(Object *value, size_t n = 1);
    
    Object *Get(int i);
    
    void Set(int i, Object *value);
    
    void Pop(int n) {
        DCHECK_GE(stack_tp_ - n, frame_bp());
        stack_tp_ -= n;
    }
    
    void IterateRoot(RootVisitor *visitor);
    
    constexpr size_t PlacedSize() const { return sizeof(*this); }
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    friend class NyaaCore;
    
    friend class CallFrame;
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyThread);
private:
    Object **frame_bp() const { return !frame_ ? stack_ : stack_ + frame_->stack_bp(); }
    Object **frame_tp() const { return !frame_ ? stack_tp_ : stack_ + frame_->stack_tp(); }
    
    int Run();

    int InternalCall(Object **func, int32_t n_args, int wanted);
    
    void CopyArgs(Arguments *args, int n_params, bool vargs);
    
    void CopyResult(Object **ret, int n_rets, int wanted);
    
    int ParseBytecodeInt32Params(int offset, int scale, int n, ...);
    
    int32_t ParseInt32(const NyByteArray *bcbuf, int offset, int scale, bool *ok);
    
    NyaaCore *const owns_;
    TryCatch *catch_point_ = nullptr;
    State state_ = kSuspended;
    Object **stack_ = nullptr; // [strong ref]
    Object **stack_tp_ = nullptr;
    Object **stack_last_ = nullptr;
    NyRunnable *entry_ = nullptr; // [strong ref] The entry script.
    size_t stack_size_ = 0;
    bool has_raised_ = false;
    CallFrame *frame_ = nullptr;
    NyThread *save_ = nullptr; // [strong ref]
    NyThread *prev_ = this; // [strong ref]
    NyThread *next_ = this; // [strong ref]
}; // class NyThread
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_THREAD_H_
