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
    "resume",
    "coroutine",
    "string",
    "float",
    "int",
    "map",
    "table",
    "delegated",
    "function",
    "closure",
    "thread",
    "array[byte]",
    "array[int32]",
    "array",
};
    
const size_t kRawBuiltinKzsSize = arraysize(kRawBuiltinKzs);
const size_t kRawBuiltinkmtSize = sizeof(BuiltinMetatablePool) / sizeof(NyMap *);
    
static_assert(sizeof(BuiltinStrPool) / kPointerSize == arraysize(kRawBuiltinKzs),
              "Incorrect size of kRawBuiltinKzs");
    
static int BuiltinPrint(Arguments *args, Nyaa *N) {
    for (size_t i = 0; i < args->Length(); ++i) {
        auto arg = ApiWarpNoCheck<Object>(args->Get(i), N->core());
        Handle<NyString> str(arg->ToString(N->core()));
        if (i > 0) {
            putchar('\t');
        }
        fwrite(str->bytes(), 1, str->size(), stdout);
    }
    putchar('\n');
    return 0;
}
    
static int BuiltinRaise(Arguments *args, Nyaa *N) {
    auto msg = Handle<NyString>::Null();
    if (args->Length() >= 1) {
        auto ob = ApiWarpNoCheck<Object>(args->Get(0), N->core());
        msg = ob->ToString(N->core());
    }
    
    auto ex = Handle<Object>::Null();
    if (args->Length() >= 2) {
        ex = ApiWarpNoCheck<Object>(args->Get(1), N->core());
    }
    N->core()->curr_thd()->Raise(*msg, *ex);
    return -1;
}
    
static int DelegatedCall(Arguments *args, Nyaa *N) {
    auto callee = ApiWarp<NyDelegated>(args->Callee(), N->core());
    if (callee.is_empty()) {
        return -1;
    }
    return callee->RawCall(args, N->core());
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
    N->core()->InsertThread(*thd);
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
    
    //printf("%s\n", name->bytes());
    if (::strncmp(name->bytes(), "status", name->size()) == 0) {
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

// coroutine.yield(a, b, c)
// yield(a,b,c)
static int BuiltinYield(Arguments *args, Nyaa *N) {
    Handle<NyThread> curr_thd(N->core()->curr_thd());
    
    if (!curr_thd->save()) {
        return Raisef(N, "no restore coroutine.");
    }
    Handle<NyThread> restore_thd(curr_thd->save());
    curr_thd->set_save(nullptr);
    curr_thd->set_state(NyThread::kSuspended);
    N->core()->set_curr_thd(*restore_thd);
    
    for (size_t i = 0; i < args->Length(); ++i) {
        restore_thd->Push(*ApiWarpNoCheck<Object>(args->Get(i), N->core()));
    }
    return 0;
}
    
static int ThreadResume(Arguments *args, Nyaa *N) {
    auto thd = ApiWarp<NyThread>(args->Get(0), N->core());
    if (!thd) {
        return -1;
    }
    if (thd->state() == NyThread::kDead) {
        return Return(Handle<Value>(Object::kNil), String::New(N, "coroutine is dead."));
    }
    
    Arguments params(args->Length() - 1);
    for (size_t i = 1; i < args->Length(); ++i) {
        params.Set(i, args->Get(i));
    }
    
    Handle<NyThread> saved_thd(N->core()->curr_thd());
    N->core()->set_curr_thd(*thd);
    TryCatch try_catch(N->isolate());
    int nret = thd->Resume(&params, *saved_thd);
    N->core()->set_curr_thd(*saved_thd);
    if (try_catch.HasCaught()) {
        return Return(Handle<Value>(Object::kNil), try_catch.Message());
    }
    
    saved_thd->Push(N->core()->factory()->NewString("ok"));
    for (int i = 0; i < nret; ++i) {
        saved_thd->Push(thd->Get(i - nret - 1));
    }
    thd->Pop(nret);
    
    if (thd->state() == NyThread::kDead) {
        N->core()->RemoveThread(*thd);
    }
    return 1 + nret;
}
    

Error BuiltinMetatablePool::Boot(NyaaCore *N) {
    auto kzs = N->bkz_pool();
    auto factory = N->factory();
    //Object *val;
    
#define NEW_METATABLE(kind) factory->NewMap(16, 0, kind, false, true)
    
    //----------------------------------------------------------------------------------------------
    // Initialize
    kMap = NEW_METATABLE(kTypeMap);
    kTable = NEW_METATABLE(kTypeTable);
    kTable->SetMetatable(kMap, N);
    kTable->generic()->SetMetatable(kTable, N);
    kMap->SetMetatable(kMap, N);
    kMap->generic()->SetMetatable(kTable, N);

    //----------------------------------------------------------------------------------------------
    // NyString
    kString = NEW_METATABLE(kTypeString);
    // Set Right Metatable
    NyString **pool_a = reinterpret_cast<NyString **>(kzs);
    for (size_t i = 0; i < kRawBuiltinKzsSize; ++i) {
        pool_a[i]->SetMetatable(kString, N);
    }
    kString->RawPut(kzs->kInnerType, kzs->kString, N);

    //----------------------------------------------------------------------------------------------
    // NyFloat64
    kFloat64 = NEW_METATABLE(kTypeFloat64);
    kFloat64->RawPut(kzs->kInnerType, kzs->kFloat, N);
    
    //----------------------------------------------------------------------------------------------
    // NyLong
    kInt = NEW_METATABLE(kTypeInt);
    kInt->RawPut(kzs->kInnerType, kzs->kInt, N);
    
    //----------------------------------------------------------------------------------------------
    // NyMap
    kMap->RawPut(kzs->kInnerType, kzs->kMap, N);
    
    //----------------------------------------------------------------------------------------------
    // NyTable
    kTable->RawPut(kzs->kInnerType, kzs->kMap, N);

    //----------------------------------------------------------------------------------------------
    // NyDelegated
    kDelegated = NEW_METATABLE(kTypeDelegated);
    kDelegated->RawPut(kzs->kInnerType, kzs->kDelegated, N);
    kDelegated->RawPut(kzs->kInnerCall, factory->NewDelegated(DelegatedCall, 0, true), N);

    //----------------------------------------------------------------------------------------------
    // NyFunction
    kFunction = NEW_METATABLE(kTypeFunction);
    kFunction->RawPut(kzs->kInnerType, kzs->kFunction, N);
    
    //----------------------------------------------------------------------------------------------
    // NyClosure
    kClosure = NEW_METATABLE(kTypeClosure);
    kClosure->RawPut(kzs->kInnerType, kzs->kClosure, N);

    //----------------------------------------------------------------------------------------------
    // NyThread
    kThread = NEW_METATABLE(kTypeThread);
    kThread->RawPut(kzs->kInnerType, kzs->kThread, N);
    kThread->RawPut(kzs->kInnerInit, factory->NewDelegated(ThreadInit, 0, true), N);
    kThread->RawPut(kzs->kInnerIndex, factory->NewDelegated(ThreadIndex, 0, true), N);
    kThread->RawPut(kzs->kInnerNewindex, factory->NewDelegated(ThreadNewindex, 0, true), N);
    kThread->RawPut(kzs->kResume, factory->NewDelegated(ThreadResume, 0, true), N);
    kThread->RawPut(kzs->kInnerSize, NySmi::New(sizeof(NyThread)), N);

    //----------------------------------------------------------------------------------------------
    // NyByteArray
    kByteArray = NEW_METATABLE(kTypeByteArray);
    kByteArray->RawPut(kzs->kInnerType, kzs->kByteArray, N);
    
    //----------------------------------------------------------------------------------------------
    // NyInt32Array
    kInt32Array = NEW_METATABLE(kTypeInt32Array);
    kInt32Array->RawPut(kzs->kInnerType, kzs->kInt32Array, N);
    
    //----------------------------------------------------------------------------------------------
    // NyArray
    kArray = NEW_METATABLE(kTypeArray);
    kArray->RawPut(kzs->kInnerType, kzs->kArray, N);
//#endif
    return Error::OK();
}
    
const NyaaNaFnEntry kBuiltinFnEntries[] = {
    {"print", BuiltinPrint},
    {"yield", BuiltinYield},
    {"raise", BuiltinRaise},
    {nullptr, nullptr},
};

} // namespace nyaa
    
} // namespace mai
