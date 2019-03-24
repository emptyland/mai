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
    
class NyThread : public NyUDO {
public:
    static constexpr const int kCalleeOffset = 0;
    static constexpr const int kEnvOffset = 1;
    static constexpr const int kAcceptsOffset = 2;
    static constexpr const int kPCOffset = 3;
    static constexpr const int kRBPOffset = 4;
    static constexpr const int kRBSOffset = 5;
    static constexpr const int kBPSize = kRBSOffset + 1;
    
    enum State {
        kSuspended,
        kRunning,
        kDead,
    };
    
    NyThread(NyaaCore *owns);
    ~NyThread();
    
    Error Init();
    
    DEF_PTR_PROP_RW(NyRunnable, entry);
    DEF_VAL_GETTER(State, state);
    
    size_t frame_size() const {
        DCHECK_GE(stack_tp_, stack_bp());
        return stack_tp_ - stack_bp();
    }
    
    uint32_t pc() const { return *pc_ptr(); }

    int Run(NyScript *entry, NyMap *env = nullptr);
    
    int Run(NyFunction *fn, Arguments *args, NyMap *env = nullptr);
    
    int Resume(Arguments *args, NyThread *prev);
    
    //int Yield(Arguments *args);

    void Push(Object *value, int n = 1);
    
    Object *Get(int i);
    
    void Set(int i, Object *value);
    
    void Pop(int n) {
        DCHECK_GE(stack_tp_ - n, stack_bp());
        stack_tp_ -= n;
    }
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    friend class NyaaCore;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyThread);
private:
    Object **stack_bp() const {
        DCHECK_LT(stack_fp_ + kBPSize, stack_tp_);
        return stack_fp_ + kBPSize;
    }
    
    uint32_t *pc_ptr() const {
        DCHECK_LT(stack_fp_ + kPCOffset, stack_last_);
        return reinterpret_cast<uint32_t *>(stack_fp_ + kPCOffset) + 1;
    }
    
    size_t rbp() const {
        DCHECK_LT(stack_fp_ + kRBPOffset, stack_last_);
        return stack_fp_[kRBPOffset]->ToSmi();
    }
    
    size_t rbs() const {
        DCHECK_LT(stack_fp_ + kRBSOffset, stack_last_);
        return stack_fp_[kRBSOffset]->ToSmi();
    }
    
    int32_t n_accepts() const {
        DCHECK_LT(stack_fp_ + kAcceptsOffset, stack_last_);
        return static_cast<int32_t>(stack_fp_[kAcceptsOffset]->ToSmi());
    }
    
    int Run(const NyByteArray *bcbuf);
    
    int CallDelegated(NyDelegated *fn, Object **base, int32_t n_params, int32_t n_accept);
    int CallFunction(NyFunction *fn, Object **base, int32_t n_params, int32_t n_accept);
    
    void InitStack(NyRunnable *callee, Object **bp, int n_accepts, NyMap *env);
    
    Object *NewPC(uint32_t pc) {
        uint64_t word = (static_cast<uint64_t>(pc) << 32) | 0x1u;
        return reinterpret_cast<Object *>(word);
    }

    NyRunnable *Current() const;
    
    const NyByteArray *CurrentBC() const;
    
    NyMap *Env() const;
    
    const NyArray *ConstPool() const;
    
    int32_t ParseParam(const NyByteArray *bcbuf, uint32_t offset, int scale, bool *ok);
    
    NyaaCore *const owns_;
    TryCatch *catch_point_ = nullptr;
    State state_ = kSuspended;
    Object **stack_ = nullptr; // [strong ref]
    Object **stack_fp_ = nullptr;
    Object **stack_tp_ = nullptr;
    Object **stack_last_ = nullptr;
    NyRunnable *entry_ = nullptr; // [strong ref] The entry script.
    size_t stack_size_ = 0;
    bool has_raised_ = false;
    NyThread *prev_ = nullptr; // [strong ref]
    NyThread *next_ = nullptr; // [strong ref]
}; // class NyThread
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_THREAD_H_
