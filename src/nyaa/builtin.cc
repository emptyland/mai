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
    "__type__",
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
    // Initialize
    kTable = factory->NewMap(16, 0, kTypeTable);
    kMap = factory->NewMap(16, 0, kTypeMap);
    kTable->SetMetatable(kTable, N);
    kMap->SetMetatable(kMap, N);

    //----------------------------------------------------------------------------------------------
    // NyString
    kString = factory->NewMap(16, 0, kTypeString);
    // Set Right Metatable
    NyString **pool_a = &kzs->kInnerInit;
    for (size_t i = 0; i < kRawBuiltinKzsSize; ++i) {
        pool_a[i]->SetMetatable(kString, N);
    }
    kString->Put(kzs->kInnerType, factory->NewString("string"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyMap
    kMap->Put(kzs->kInnerType, factory->NewString("map"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyTable
    kTable->Put(kzs->kInnerType, factory->NewString("table"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyDelegated
    kDelegated = factory->NewMap(16, 0, kTypeDelegated);
    kDelegated->Put(kzs->kInnerType, factory->NewString("delegated"), N);
    kDelegated->Put(kzs->kInnerCall, factory->NewDelegated(DelegatedCall), N);
    
    //----------------------------------------------------------------------------------------------
    // NyFunction
    kFunction = factory->NewMap(16, 0, kTypeFunction);
    kFunction->Put(kzs->kInnerType, factory->NewString("function"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyScript
    kScript = factory->NewMap(16, 0, kTypeScript);
    kScript->Put(kzs->kInnerType, factory->NewString("script"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyThread
    kThread = factory->NewMap(16, 0, kTypeThread);
    kThread->Put(kzs->kInnerType, factory->NewString("thread"), N);
    kThread->Put(kzs->kInnerInit, factory->NewDelegated(ThreadInit), N);
    kThread->Put(kzs->kInnerSize, NySmi::New(sizeof(NyThread)), N);

    //----------------------------------------------------------------------------------------------
    // NyByteArray
    kByteArray = factory->NewMap(16, 0, kTypeByteArray);
    kByteArray->Put(kzs->kInnerType, factory->NewString("array[byte]"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyInt32Array
    kInt32Array = factory->NewMap(16, 0, kTypeInt32Array);
    kInt32Array->Put(kzs->kInnerType, factory->NewString("array[int32]"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyArray
    kArray = factory->NewMap(16, 0, kTypeArray);
    kArray->Put(kzs->kInnerType, factory->NewString("array"), N);

    return Error::OK();
}
    
const NyaaNaFnEntry kBuiltinFnEntries[] = {
    {"print", BuiltinPrint},
    {nullptr, nullptr},
};

} // namespace nyaa
    
} // namespace mai
