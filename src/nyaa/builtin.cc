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
    "running",
    "suspended",
    "dead",
};
    
const size_t kRawBuiltinKzsSize = arraysize(kRawBuiltinKzs);
    
static_assert(sizeof(BuiltinStrPool) / kPointerSize == arraysize(kRawBuiltinKzs),
              "Incorrect size of kRawBuiltinKzs");
    
static int BuiltinPrint(Arguments *args, Nyaa *N) {
    for (size_t i = 0; i < args->Length(); ++i) {
        auto arg = ApiWarpNoCheck<Object>(args->Get(i), N->core());
        Handle<NyString> str(arg->ToString(N->core()));
        if (i > 0) {
            puts("\t");
        }
        puts(str->bytes());
    }
    puts("\n");
    return 0;
}
    
static int BuiltinYield(Arguments *args, Nyaa *N) {
    // TODO:
    return 0;
}
    
static int DelegatedCall(Arguments *args, Nyaa *N) {
    auto callee = ApiWarp<NyDelegated>(args->Callee(), N->core());
    if (callee.is_empty()) {
        return -1;
    }
    return callee->Call(args, N->core());
}
    
static int ThreadInit(Local<Value> arg0, Local<Value> arg1, Nyaa *N) {
    auto udo = ApiWarpNoCheck<NyUDO>(arg0, N->core());
    if (!udo) {
        return -1;
    }
    DCHECK_EQ(udo->GetMetatable(), N->core()->kmt_pool()->kThread);
    
    auto entry = ApiWarpNoCheck<NyRunnable>(arg1, N->core());
    if (entry->IsNil() || entry->IsSmi() || !entry->IsRunnable()) {
        return Raisef(N, "incorrect entry type.");
    }

    Handle<NyThread> thd(new (*udo) NyThread(N->core()));
    thd->set_entry(*entry);
    thd->SetMetatable(N->core()->kmt_pool()->kThread, N->core());
    auto rs = thd->Init();
    if (!rs) {
        return Raisef(N, "coroutine init fail, cause: %s", rs.ToString().c_str());
    }
    return Return(thd);
}

static int ThreadIndex(Local<Value> arg0, Local<Value> arg1, Nyaa *N) {
    auto thd = ApiWarp<NyThread>(arg0, N->core());
    if (!thd) {
        return -1;
    }
    auto name = ApiWarp<NyString>(arg1, N->core());
    if (!name) {
        return -1;
    }
    
    if (::strncmp(name->bytes(), "status", name->size())) {
        Handle<NyString> rv;
        switch (thd->state()) {
            case NyThread::kRunning:
                rv = N->core()->bkz_pool()->kRunning;
                break;
                
            case NyThread::kSuspended:
                rv = N->core()->bkz_pool()->kSuspended;
                break;
                
            case NyThread::kDead:
                rv = N->core()->bkz_pool()->kDead;
                break;
            default:
                break;
        }
        return Return(rv);
    }
    
    Handle<NyMap> mt(thd->GetMetatable());
    Handle<Object> rv(mt->RawGet(*name, N->core()));
    return Return(rv);
}
    
static int ThreadNewindex(Local<Value>, Local<Value>, Local<Value>, Nyaa *N) {
    return Raisef(N, "coroutine is readonly");
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
    kString->RawPut(kzs->kInnerType, factory->NewString("string"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyFloat64
    kFloat64 = factory->NewMap(16, 0, kTypeFloat64);
    kFloat64->RawPut(kzs->kInnerType, factory->NewString("float"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyLong
    kLong = factory->NewMap(16, 0, kTypeLong);
    kLong->RawPut(kzs->kInnerType, factory->NewString("int"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyMap
    kMap->RawPut(kzs->kInnerType, factory->NewString("map"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyTable
    kTable->RawPut(kzs->kInnerType, factory->NewString("table"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyDelegated
    kDelegated = factory->NewMap(16, 0, kTypeDelegated);
    kDelegated->RawPut(kzs->kInnerType, factory->NewString("delegated"), N);
    kDelegated->RawPut(kzs->kInnerCall, factory->NewDelegated(DelegatedCall), N);
    
    //----------------------------------------------------------------------------------------------
    // NyFunction
    kFunction = factory->NewMap(16, 0, kTypeFunction);
    kFunction->RawPut(kzs->kInnerType, factory->NewString("function"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyScript
    kScript = factory->NewMap(16, 0, kTypeScript);
    kScript->RawPut(kzs->kInnerType, factory->NewString("script"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyThread
    kThread = factory->NewMap(16, 0, kTypeThread);
    kThread->RawPut(kzs->kInnerType, factory->NewString("thread"), N);
    kThread->RawPut(kzs->kInnerInit, factory->NewDelegated(ThreadInit), N);
    kThread->RawPut(kzs->kInnerIndex, factory->NewDelegated(ThreadIndex), N);
    kThread->RawPut(kzs->kInnerNewindex, factory->NewDelegated(ThreadNewindex), N);
    kThread->RawPut(kzs->kInnerSize, NySmi::New(sizeof(NyThread)), N);

    //----------------------------------------------------------------------------------------------
    // NyByteArray
    kByteArray = factory->NewMap(16, 0, kTypeByteArray);
    kByteArray->RawPut(kzs->kInnerType, factory->NewString("array[byte]"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyInt32Array
    kInt32Array = factory->NewMap(16, 0, kTypeInt32Array);
    kInt32Array->RawPut(kzs->kInnerType, factory->NewString("array[int32]"), N);
    
    //----------------------------------------------------------------------------------------------
    // NyArray
    kArray = factory->NewMap(16, 0, kTypeArray);
    kArray->RawPut(kzs->kInnerType, factory->NewString("array"), N);

    return Error::OK();
}
    
const NyaaNaFnEntry kBuiltinFnEntries[] = {
    {"print", BuiltinPrint},
    {nullptr, nullptr},
};

} // namespace nyaa
    
} // namespace mai
