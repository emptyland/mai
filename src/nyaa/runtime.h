#ifndef MAI_NYAA_RUNTIME_H_
#define MAI_NYAA_RUNTIME_H_

#include "base/base.h"
#if defined(MAI_ARCH_X64)
#include "asm/x64/asm-x64.h"
#endif

namespace mai {
    
namespace nyaa {

class NyThread;
class Object;
class NyaaCore;

class Runtime {
public:
    enum ExternalLink {
        kThread_Set,
        kThread_Get,
        kThread_GetUpVal,
        kThread_SetUpVal,
        kThread_GetProto,
        kThread_Closure,
        kThread_PrepareCall,
        kThread_FinalizeCall,
        kThread_Ret,
        
        kNyaaCore_GetSuspendPoint, //SUSPEND_POINT
        
        kMap_RawGet,
        kMap_RawPut,
        kMaxLinks,
    };

#if defined(MAI_ARCH_X64)
    static constexpr x64::Register kScratch = x64::r10;
    static constexpr x64::Register kThread = x64::r11;
    static constexpr x64::Register kCore = x64::r12;
    static constexpr x64::Register kBP = x64::r13;
#endif // defined(MAI_ARCH_X64)

    static Address kExternalLinks[kMaxLinks];
private:
    static Object *Thread_GetUpVal(NyThread *thd, int slot);
    static void Thread_SetUpVal(NyThread *thd, Object *val, int up);
    static Object *Thread_GetProto(NyThread *thd, int slot);
    static Object *Thread_Closure(NyThread *thd, int slot);
    static Address NyaaCore_GetSuspendPoint(NyaaCore *N);
}; // class Runtime

} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_RUNTIME_H_
