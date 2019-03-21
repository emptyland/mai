#ifndef MAI_NYAA_THREAD_H_
#define MAI_NYAA_THREAD_H_

#include "nyaa/nyaa-values.h"
#include "mai/error.h"

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
    static constexpr const int kBCBufOffset = 1;
    static constexpr const int kPCOffset = 2;
    static constexpr const int kBSOffset = 3;
    static constexpr const int kBPOffset = kBSOffset + 1;
    
    NyThread(NyaaCore *owns);
    ~NyThread();
    
    Error Init();

    int Run(NyScript *entry, NyTable *env = nullptr);

    void Push(Object *value, int n = 1);
    
    Object *Get(int i);
    
    //DEF_PTR_GETTER(Object, ra);
    
    uint32_t pc() const { return *pc_ptr(); }
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    friend class NyaaCore;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyThread);
private:
    int Run(const NyByteArray *bcbuf);
    
    int CallDelegated(NyDelegated *fn, ptrdiff_t saved, int32_t n_params, int32_t n_accept);
    
    Object *NewPC(uint32_t pc) {
        uint64_t word = (static_cast<uint64_t>(pc) << 32) | 0x1u;
        return reinterpret_cast<Object *>(word);
    }
    
    Object **stack_bp() const { return stack_fp_ + kBPOffset; }
    
    uint32_t *pc_ptr() const { return reinterpret_cast<uint32_t *>(stack_fp_ + kPCOffset) + 1; }
    
    NyRunnable *Current() const;
    
    const NyArray *ConstPool() const;
    
    int32_t ParseParam(const NyByteArray *bcbuf, uint32_t offset, int scale, bool *ok);
    
    NyaaCore *const owns_;
    TryCatch *catch_point_ = nullptr;
    //Object *ra_ = nullptr; // [strong ref] register A
    Object **stack_ = nullptr; // [strong ref]
    Object **stack_fp_ = nullptr;
    Object **stack_tp_ = nullptr;
    Object **stack_last_ = nullptr;
    NyScript *entry_ = nullptr; // [strong ref] The entry script.
    NyRunnable *current_ = nullptr; // [strong ref] The current runnable object.
    size_t stack_size_ = 0;
    NyThread *next_ = nullptr; // [strong ref]
    NyTable *env_ = nullptr; // [strong ref] local global
    bool has_raised_ = false;
}; // class NyThread
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_THREAD_H_
