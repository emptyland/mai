#ifndef MAI_NYAA_RUNTIME_H_
#define MAI_NYAA_RUNTIME_H_

#include "nyaa/builtin.h"
#if defined(MAI_ARCH_X64)
#include "asm/x64/asm-x64.h"
#endif
#include "base/base.h"


namespace mai {
    
namespace nyaa {

class NyThread;
class Object;
class NyaaCore;
class NyRunnable;

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
        kThread_NewMap,
        kThread_PrepareNew,
        kThread_SaveNativeStack,
        
        kNyaaCore_GetRecoverPoint,
        kNyaaCore_TryMetaFunction,
        
        kObject_IsFalse,
        kObject_IsTrue,
        kObject_Add,
        kObject_Sub,
        kObject_Mul,
        kObject_Div,
        kObject_Mod,
        kObject_EQ,
        kObject_NE,
        kObject_LT,
        kObject_LE,
        kObject_GT,
        kObject_GE,
        kObject_Get,
        kObject_Put,
        
        kMap_RawGet,
        kMap_RawPut,
        
        kTest_PrintNaSt,
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
    static int Object_IsFalseWarp(Object *ob);
    static int Object_IsTrueWarp(Object *ob);
    static Object *Object_EQWarp(Object *lhs, Object *rhs, NyaaCore *N);
    static Object *Object_NEWarp(Object *lhs, Object *rhs, NyaaCore *N);
    static Object *Object_LTWarp(Object *lhs, Object *rhs, NyaaCore *N);
    static Object *Object_LEWarp(Object *lhs, Object *rhs, NyaaCore *N);
    static Object *Object_GTWarp(Object *lhs, Object *rhs, NyaaCore *N);
    static Object *Object_GEWarp(Object *lhs, Object *rhs, NyaaCore *N);
    
    static Object *Thread_GetUpVal(NyThread *thd, int slot);
    static void Thread_SetUpVal(NyThread *thd, Object *val, int up);
    static Object *Thread_GetProto(NyThread *thd, int slot);
    static Object *Thread_Closure(NyThread *thd, int slot);
    static Address NyaaCore_GetRecoverPoint(NyaaCore *N);
    static NyRunnable *NyaaCore_TryMetaFunction(NyaaCore *N, Object *ob, Operator::ID op);
    
    static void Test_PrintNaSt(Address tp, Address bp);
}; // class Runtime

} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_RUNTIME_H_
