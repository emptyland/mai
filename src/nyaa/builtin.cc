#include "nyaa/builtin.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/thread.h"
#include "nyaa/object-factory.h"
#include "mai-lang/nyaa.h"

namespace mai {
    
namespace nyaa {
        
const char *kRawBuiltinKzs[] = {
    "__init__",
    "__index__",
    "__newindex__",
    "__call__",
    "__str__",
    "__add__",
    "__sub__",
    "__mul__",
    "__div__",
    "__mod__",
    "__unm__",
    "__concat__",
    "__eq__",
    "__lt__",
    "__le__",
    "__gc__",
    "__id__",
    "__size__",
    "__base__",
    "__weak__",
    "nil",
    "true",
    "false",
    "",
};
    
const size_t kRawBuiltinKzsSize = arraysize(kRawBuiltinKzs);
    
static_assert(sizeof(BuiltinStrPool) / kPointerSize == arraysize(kRawBuiltinKzs),
              "Incorrect size of kRawBuiltinKzs");
    
static int BuiltinPrint(Arguments *args, Nyaa *N) {
    for (size_t i = 0; i < args->Length(); ++i) {
        
    }

    return 0;
}
    
static int DelegatedCall(Arguments *args, Nyaa *N) {
    HandleScope scope(N->isolate());
    
    auto callee = ApiWarp<NyDelegated>(args->Callee(), N->core());
    if (callee.is_empty()) {
        return -1;
    }
    return callee->Call(args, N->core());
}
    
static int ThreadInit(Arguments *args, Nyaa *N) {
    HandleScope scope(N->isolate());
    
    auto thrd = ApiWarp<NyThread>(args->Get(0), N->core());
    if (thrd.is_empty()) {
        return -1;
    }

    auto rs = thrd->Init();
    if (!rs) {
        N->core()->Raisef("Thread init fail, cause: %s", rs.ToString().c_str());
        return -1;
    }
    return 0;
}
    
Error BuiltinMetatablePool::Boot(NyaaCore *N) {
    auto kzs = N->bkz_pool();
    auto factory = N->factory();
    //Object *val;
    
    //----------------------------------------------------------------------------------------------
    // NyString
    kString = factory->NewTable(16, 0);
    // Set Right Metatable
    NyString **pool_a = &kzs->kInnerInit;
    for (size_t i = 0; i < kRawBuiltinKzsSize; ++i) {
        pool_a[i]->SetMetatable(kString, N);
    }
    kString->Put(kzs->kInnerID, factory->NewString("string"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyDelegated
    kDelegated = factory->NewTable(16, 0);
    kDelegated->Put(kzs->kInnerID, factory->NewString("delegated"), N);
    kDelegated->Put(kzs->kInnerCall, factory->NewDelegated(DelegatedCall), N);
    
    //----------------------------------------------------------------------------------------------
    // NyFunction
    kFunction = factory->NewTable(16, 0);
    kFunction->Put(kzs->kInnerID, factory->NewString("function"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyScript
    kScript = factory->NewTable(16, 0);
    kScript->Put(kzs->kInnerID, factory->NewString("script"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyThread
    kThread = factory->NewTable(16, 0);
    kThread->Put(kzs->kInnerID, factory->NewString("thread"), N);
    kThread->Put(kzs->kInnerInit, factory->NewDelegated(ThreadInit), N);
    kThread->Put(kzs->kInnerSize, NyInt64::New(sizeof(NyThread)), N);

    //----------------------------------------------------------------------------------------------
    // NyByteArray
    kByteArray = factory->NewTable(16, 0);
    kByteArray->Put(kzs->kInnerID, factory->NewString("array[byte]"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyInt32Array
    kInt32Array = factory->NewTable(16, 0);
    kInt32Array->Put(kzs->kInnerID, factory->NewString("array[int32]"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyArray
    kArray = factory->NewTable(16, 0);
    kArray->Put(kzs->kInnerID, factory->NewString("array"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyMap
    kMap = factory->NewTable(16, 0);
    kMap->Put(kzs->kInnerID, factory->NewString("map"), N);
    return Error::OK();
}
    
const NyaaNaFnEntry kBuiltinFnEntries[] = {
    {"print", BuiltinPrint},
    {nullptr, nullptr},
};

} // namespace nyaa
    
} // namespace mai
