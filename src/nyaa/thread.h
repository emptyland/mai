#ifndef MAI_NYAA_THREAD_H_
#define MAI_NYAA_THREAD_H_

#include "nyaa/nyaa-values.h"

namespace mai {
    
namespace nyaa {
    
class TryCatch;
class NyaaCore;
    
class NyThread : public NyUDO {
public:
    NyThread(NyaaCore *owns);
    ~NyThread();
    
    friend class NyaaCore;
private:
    NyaaCore *const owns_;
    TryCatch *catch_point_;
    Object *ra_ = nullptr; // register A
    Object **stack_ = nullptr;
    Object **stack_last_ = nullptr;
}; // class NyThread
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_THREAD_H_
