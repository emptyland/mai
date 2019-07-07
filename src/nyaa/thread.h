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
    static const int32_t kOffsetEntry;
    static const int32_t kOffsetPC;
    
    explicit CallFrame(int trace_id = -1) : trace_id_(trace_id) {}
    ~CallFrame() {}
    
    void Enter(NyThread *owns, NyRunnable *callee, NyObject *exec, NyArray *kpool, int wanted,
               size_t bp, size_t tp, NyMap *env);
    void Exit(NyThread *owns);

    DEF_VAL_GETTER(int, trace_id);
    DEF_PTR_GETTER(NyRunnable, callee);
    DEF_PTR_GETTER(NyMap, env);
    DEF_PTR_GETTER(NyBytecodeArray, bcbuf);
    DEF_PTR_GETTER(NyCode, code);
    DEF_PTR_GETTER(NyArray, const_pool);
    DEF_PTR_GETTER(CallFrame, prev);
    DEF_VAL_GETTER(int, level);
    DEF_VAL_GETTER(int, pc);
    DEF_VAL_PROP_RW(int, nrets);
    DEF_VAL_GETTER(int, wanted);
    DEF_VAL_GETTER(size_t, stack_be);
    DEF_VAL_GETTER(size_t, stack_bp);
    DEF_VAL_GETTER(size_t, stack_tp);

    std::tuple<NyString *, NyInt32Array *> SourceLines() const;
    
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
        DCHECK_LE(pc_ + delta, bcbuf_->bytecode_bytes_size());
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
        NyString *file_name; NyInt32Array *source_lines;
        std::tie(file_name, source_lines) = SourceLines();
        if (!source_lines) {
            return {nullptr, nullptr, 0};
        } else {
            return {file_name, proto()->name(), GetFileLine(source_lines)};
        }
    }
    
    Byte BC() const { return bcbuf_->byte(pc_); }
    
    void IterateRoot(RootVisitor *visitor);

    friend class NyThread;
    DISALLOW_IMPLICIT_CONSTRUCTORS(CallFrame);
private:
    int GetFileLine(const NyInt32Array *line_info);

    int trace_id_; // calling trace-id.
    NyRunnable *callee_; // [strong ref]
    NyMap *env_; // [strong ref]
    union {
        NyBytecodeArray *bcbuf_; // [strong ref]
        NyCode *code_; // [strong ref]
        Object *exec_ = nullptr;;
    };
    NyArray *const_pool_ = nullptr; // [strong ref]
    CallFrame *prev_;
    int level_ = 0;
    intptr_t entry_;
    int pc_ = 0;
    size_t stack_be_ = 0;
    size_t stack_bp_ = 0;
    size_t stack_tp_ = 0;
    //ExceptionId eid_ = kNormal;
    int nrets_ = 0; // record real return results.
    int wanted_ = 0; // return wanted results.
    bool compact_file_info_ = false;
};
    
class TryCatchCore {
public:
    static constexpr int kThread = 0;
    static constexpr int kMessage = 1;
    static constexpr int kException = 2;
    static constexpr int kStackTrace = 3;
    static constexpr int kSlotSize = 4;
    
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
    
    NyString *GetFileLineMessage() const;
    std::string ToString() const;

    DISALLOW_IMPLICIT_CONSTRUCTORS(TryCatchCore);
private:
    NyaaCore *const core_;
    bool has_caught_ = false;
    TryCatchCore *prev_ = nullptr;
    Object **obs_ = nullptr;
};
    
class CodeContextBundle : public arch::RegisterContext {
public:
    using Template = arch::ObjectTemplate<CodeContextBundle, int32_t>;
    
    static const int32_t kOffsetNaStTP;
    static const int32_t kOffsetNaStBP;
    
    inline CodeContextBundle(NyThread *owns);
    inline ~CodeContextBundle();
    
    inline size_t nast_size() const { return nast_bp_ - nast_tp_; }
    DEF_PTR_PROP_RW(Byte, nast_tp);
    DEF_PTR_PROP_RW(Byte, nast_bp);
    DEF_PTR_GETTER(CodeContextBundle, prev);
private:
    NyThread *const owns_;
    CodeContextBundle *prev_;
    Address nast_tp_ = nullptr; // saved native stack pointer
    Address nast_bp_ = nullptr; // bk size = nast_bp_ - nast_tp_
}; // class CodeContextBundle
    
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
    static const int32_t kOffsetStackTP;
    static const int32_t kOffsetState;
    static const int32_t kOffsetNaStBK;
    static const int32_t kOffsetNaStBKSize;
    
    NyThread(NyaaCore *owns);
    ~NyThread();
    
    Error Init();
    
    DEF_PTR_PROP_RW(NyRunnable, entry);
    DEF_PTR_PROP_RW(NyThread, save);
    DEF_VAL_PROP_RW(State, state);
    DEF_PTR_GETTER(Object*, stack);
    DEF_PTR_PROP_RW(Object*, stack_tp);
    CallFrame *call_info() const { return DCHECK_NOTNULL(frame_); }
    NyMap *CurrentEnv() const;
    
    size_t frame_size() const {
        DCHECK_GE(stack_tp_, stack_);
        DCHECK_LT(stack_tp_, stack_last_);
        return stack_tp_ - stack_;
    }
    
    int TryRun(NyRunnable *fn, Object *argv[], int argc, int wanted = 0, NyMap *env = nullptr);
    int Run(NyRunnable *fn, Object *argv[], int argc, int wanted = 0, NyMap *env = nullptr);
    int Resume(Object *argv[], int argc, int wanted, NyMap *env);
    void Yield();
    
    void Raisef(const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        Vraisef(fmt, ap);
        va_end(ap);
    }
    
    void Vraisef(const char *fmt, va_list ap);
    void Raise(NyString *msg, Object *ex);

    inline void Push(Object *value, size_t n = 1);
    inline Object *Get(int i);

    void Set(int i, Object *value) {
        if (i < 0) {
            DCHECK_GE(frame_tp() + i, frame_bp());
            *(frame_tp() + i) = value;
        } else {
            DCHECK_LT(frame_bp() + i, frame_tp());
            frame_bp()[i] = value;
        }
    }
    
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
    friend class CodeContextBundle;
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

    void Unwind(CallFrame *ci) {
        while (frame_ != ci) {
            CallFrame *frame = frame_;
            frame->Exit(this);
            delete frame;
        }
    }

    int Run();

    int InternalCall(Object **base, int32_t nargs, int wanted) {
        size_t base_p = base - stack_;
        nargs = PrepareCall(base, nargs, wanted, -1/*trace_id*/);
        base = stack_ + base_p;
        return FinializeCall(base, nargs, wanted);
    }
    
    void RuntimeNewMap(int32_t map, int32_t n, int32_t p, uint32_t seed);
    NyUDO *RuntimePrepareNew(int32_t base, int32_t ob, int32_t *nargs, NyRunnable **init);
    int RuntimePrepareCall(int32_t callee, int32_t nargs, int wanted, int trace_id);
    int RuntimeFinializeCall(int32_t callee, int32_t nargs, int wanted);

    int RuntimeRet(int32_t base, int32_t nrets);
    
    void RuntimeSaveNativeStack(Address nast_tp);
    
    void RuntimeExpandVArgs(int32_t ra, int wanted);
    void RuntimeConcat(int32_t ret, int32_t base, int32_t n);

    int PrepareCall(Object **base, int32_t nargs, int32_t wanted, int trace_id);
    int FinializeCall(Object **base, int32_t nargs, int32_t wanted);

    int CopyArgs(Object **args, int nargs, int nparams, bool vargs);
    
    int CopyResult(Object **ret, int nrets, int wanted);
    inline void CopyResult(Object **ret, Object **argv, int argc, int wanted);
    
    void ProcessClass(NyMap *clazz);
    
    int ParseBytecodeInt32Params(int offset, int scale, int n, ...);
    int ParseBytecodeSize(int offset);
    
    void Bind(int i, NyClosure *closure, const NyFunction::UpvalDesc &desc) {
        CallFrame *up = frame_;
        Object *val = desc.in_stack ? stack_[up->stack_bp() + desc.index] :
                      static_cast<NyClosure *>(up->callee())->upval(desc.index);
        closure->Bind(i, val, owns_);
    }
    
    int GetLine(const NyInt32Array *line_info, int pc) const;
    
    NyaaCore *const owns_;
    TryCatchCore *catch_point_ = nullptr;
    CallFrame::ExceptionId interruption_pending_ = CallFrame::kNormal;
    CodeContextBundle *save_point_ = nullptr;
    State state_ = kSuspended;
    Address nast_bk_ = nullptr; // backup native stack pointer
    size_t nast_bk_size_ = 0; // size of backup native stack
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
    
inline CodeContextBundle::CodeContextBundle(NyThread *owns)
    : owns_(owns)
    , prev_(owns_->save_point_) {
    DCHECK_NE(owns_->save_point_, this);
    owns_->save_point_ = this;
    
    static_assert(std::is_base_of<arch::RegisterContext, CodeContextBundle>::value, "error");
    if (std::is_base_of<arch::RegisterContext, CodeContextBundle>::value) {
        ::memset(this, 0, sizeof(arch::RegisterContext));
    }
}

inline CodeContextBundle::~CodeContextBundle() {
    //DCHECK_EQ(owns_->save_point_, this);
    owns_->save_point_ = prev_;
}


inline NyThread *TryCatchCore::thread() const { return static_cast<NyThread *>(obs_[kThread]); }
    
inline void NyThread::Push(Object *value, size_t n) {
    CheckStackAdd(n);
    if (stack_tp_ + n > stack_last_) {
        Raisef("stack overflow!");
        return;
    }
    for (int i = 0; i < n; ++i) {
        stack_tp_[i] = value;
    }
    stack_tp_ += n;
}
    
inline Object *NyThread::Get(int i) {
    if (i < 0) {
        return *(stack_tp_ + i);
    } else {
        DCHECK_LT(frame_bp() + i, stack_last_);
        return frame_bp()[i];
    }
}
    
inline void NyThread::CopyResult(Object **ret, Object **argv, int argc, int wanted) {
    if (wanted < 0) {
        wanted = argc;
    }
    for (int i = 0; i < argc && i < wanted; ++i) {
        ret[i] = argv[i];
    }
    for (int i = argc; i < wanted; ++i) {
        ret[i] = Object::kNil;
    }
    stack_tp_ = ret + wanted;
}
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_THREAD_H_
