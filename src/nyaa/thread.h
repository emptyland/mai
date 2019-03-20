#ifndef MAI_NYAA_THREAD_H_
#define MAI_NYAA_THREAD_H_

#include "nyaa/nyaa-values.h"
#include "mai/error.h"

namespace mai {
    
namespace nyaa {
    
class TryCatch;
class NyaaCore;
    
class NyThread : public NyUDO {
public:
    NyThread(NyaaCore *owns);
    ~NyThread();
    
    Error Init();
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    friend class NyaaCore;
private:
    NyaaCore *const owns_;
    TryCatch *catch_point_ = nullptr;
    Object *ra_ = nullptr; // register A
    Object **stack_ = nullptr;
    Object **stack_last_ = nullptr;
    size_t stack_size_ = 0;
}; // class NyThread
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_THREAD_H_
