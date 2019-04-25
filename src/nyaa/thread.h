#ifndef MAI_NYAA_THREAD_H_
#define MAI_NYAA_THREAD_H_

#include "nyaa/nyaa-values.h"
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
    CallFrame() {}
    ~CallFrame() {}
    
    void Enter(NyThread *owns, NyRunnable *callee, NyByteArray *bcbuf, NyArray *kpool, int wanted,
               size_t bp, size_t tp, NyMap *env);
    void Exit(NyThread *owns);

    DEF_PTR_GETTER(NyRunnable, callee);
    DEF_PTR_GETTER(NyMap, env);
    DEF_PTR_GETTER(NyByteArray, bcbuf);
    DEF_PTR_GETTER(NyArray, const_poll);
    DEF_PTR_GETTER(CallFrame, prev);
    DEF_VAL_GETTER(int, level);
    DEF_VAL_GETTER(int, pc);
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
    
    Byte BC() const { return bcbuf_->Get(pc_); }
    
    void IterateRoot(RootVisitor *visitor);

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
    size_t stack_be_ = 0;
    size_t stack_bp_ = 0;
    size_t stack_tp_ = 0;
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
    
    int TryRun(NyRunnable *fn, Arguments *args, int nrets = 0, NyMap *env = nullptr);

    int Run(NyRunnable *fn, Arguments *args, int nrets = 0, NyMap *env = nullptr);
    
    int Run(NyRunnable *fn, Object**args, int nargs, int nrets = 0, NyMap *env = nullptr);
    
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
    
    void IterateRoot(RootVisitor *visitor);
    
    void Iterate(ObjectVisitor *) {}
    
    constexpr size_t PlacedSize() const { return sizeof(*this); }

    enum CatchId {
        kException,
    };
    
    friend class NyaaCore;
    friend class TryCatchCore;
    friend class CallFrame;
    DEF_HEAP_OBJECT(Thread);
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyThread);
private:
    
    
    Object **frame_bp() const { return !frame_ ? stack_ : stack_ + frame_->stack_bp(); }
    Object **frame_tp() const { return !frame_ ? stack_tp_ : stack_ + frame_->stack_tp(); }
    Object **frame_be() const { return !frame_ ? stack_ : stack_ + frame_->stack_be(); }
    
    void CheckStackAdd(size_t add) {
        DCHECK_GE(stack_tp_, stack_);
        CheckStack(stack_tp_ - stack_ + add);
    }
    void CheckStack(size_t size);
    void Rewind();
    int Run();

    Object *InternalGetField(Object *mm, Object *key);
    int InternalSetField(Object *mm, Object *key, Object *value);
    int InternalCall(Object **func, int32_t n_args, int wanted);
    int InternalNewUdo(Object **udo, int32_t n_args, size_t size, NyMap *clazz);
    
    int CopyArgs(Object **args, int n_args, int n_params, bool vargs);
    
    void CopyResult(Object **ret, int n_rets, int wanted);
    
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
    TryCatchCore *catch_point_ = nullptr; // elements [strong ref]
    State state_ = kSuspended;
    Object **stack_ = nullptr; // [strong ref]
    Object **stack_tp_ = nullptr;
    Object **stack_last_ = nullptr;
    NyRunnable *entry_ = nullptr; // [strong ref] The entry script.
    size_t stack_size_ = 0;
    CallFrame *frame_ = nullptr;
    NyThread *save_ = nullptr; // [strong ref]
    NyThread *prev_ = this; // [strong ref]
    NyThread *next_ = this; // [strong ref]
}; // class NyThread
    
inline NyThread *TryCatchCore::thread() const { return static_cast<NyThread *>(obs_[kThread]); }
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_THREAD_H_
