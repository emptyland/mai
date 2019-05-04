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
    "__offset__",
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
    
static int BuiltinStr(Arguments *args, Nyaa *N) {
    switch (args->Length()) {
        case 0:
            return Return(Local<NyString>::New(N->core()->bkz_pool()->kEmpty));
        case 1: {
            Handle<Object> a0 = ApiWarpNoCheck<Object>(args->Get(0), N->core());
            DCHECK(a0.is_not_empty());
            return Return(Local<NyString>::New(a0->ToString(N->core())));
        } break;
        default: {
            Handle<NyString> buf = N->core()->factory()->NewUninitializedString(64);
            for (size_t i = 0; i < args->Length(); ++i) {
                Handle<Object> a = ApiWarpNoCheck<Object>(args->Get(i), N->core());
                DCHECK(a.is_not_empty());
                buf = buf->Add(a->ToString(N->core()), N->core());
            }
            return Return(Local<NyString>::New(buf->Done(N->core())));
        } break;
    }
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
    
static int BuiltinMapIter(Arguments *args, Nyaa *N) {
    Handle<NyDelegated> callee = ApiWarpNoCheck<NyDelegated>(args->Callee(), N->core());
    DCHECK(callee.is_valid());
    Handle<NyMap> map = NyMap::Cast(callee->upval(0));
    DCHECK(map.is_valid());
    Handle<Object> key = callee->upval(1);

    Object *next, *value;
    std::tie(next, value) = map->GetNextPair(*key, N->core());
    callee->Bind(1, next, N->core());
    return Return(Local<Object>::New(next), Local<Object>::New(value));
}
    
static int BuiltinPairs(Arguments *args, Nyaa *N) {
    if (args->Length() < 1) {
        return Raisef("incorrect argument length, expected: %zd, need: 1", args->Length());
    }
    auto arg0 = ApiWarpNoCheck<Object>(args->Get(0), N->core());
    if (NyMap *map = NyMap::Cast(*arg0)) {
        Handle<NyDelegated> iter = N->core()->factory()->NewDelegated(BuiltinMapIter, 2);
        iter->Bind(0, map, N->core());
        iter->Bind(1, Object::kNil, N->core());
        return Return(iter);
    } else if (NyUDO *uod = NyUDO::Cast(*arg0)) {
        // TODO:
        return Raisef("TODO:");
    } else {
        return Raisef("incorrect type: no pairs.");
    }
}
    
static int BuiltinRangeIter(Arguments *args, Nyaa *N) {
    Handle<NyDelegated> callee = ApiWarpNoCheck<NyDelegated>(args->Callee(), N->core());
    DCHECK(callee.is_valid());
    
    int64_t curr = callee->upval(0)->ToSmi();
    int64_t end  = callee->upval(1)->ToSmi();
    int64_t step = callee->upval(2)->ToSmi();
    if (step > 0) {
        if (curr >= end) {
            return Return(Local<Object>::New(Object::kNil));
        }
    } else {
        if (curr <= end) {
            return Return(Local<Object>::New(Object::kNil));
        }
    }
    curr += step;
    Handle<Object> rv = NySmi::New(curr);
    callee->Bind(0, *rv, N->core());
    return Return(rv);
}

static int BuiltinRange(Arguments *args, Nyaa *N) {
    int64_t begin = 0, end = 0, step = 1;
    switch (args->Length()) {
        case 3: {
            Handle<Object> ob = ApiWarpNoCheck<Object>(args->Get(2), N->core());
            if (!ob->IsSmi()) {
                return Raisef("incorrect type: need integer.");
            }
            step = ob->ToSmi();
        } // though
        case 2: {
            Handle<Object> ob = ApiWarpNoCheck<Object>(args->Get(1), N->core());
            if (!ob->IsSmi()) {
                return Raisef("incorrect type: need integer.");
            }
            end = ob->ToSmi();
            ob = ApiWarpNoCheck<Object>(args->Get(0), N->core());
            if (!ob->IsSmi()) {
                return Raisef("incorrect type: need integer.");
            }
            begin = ob->ToSmi();
        } break;

        default:
            return Raisef("incorrect number of arguments. need [2, 3]");
    }
    Handle<NyDelegated> iter = N->core()->factory()->NewDelegated(BuiltinRangeIter, 3);
    iter->Bind(0, NySmi::New(begin), N->core());
    iter->Bind(1, NySmi::New(end), N->core());
    iter->Bind(2, NySmi::New(step), N->core());
    return Return(iter);
}
    
static int BuiltinSetMetatable(Arguments *args, Nyaa *N) {
    Handle<Object> ob = ApiWarpNoCheck<Object>(args->Get(0), N->core());
    if (ob.is_not_valid()) {
        return Return();
    }
    Handle<NyMap> a0 = ApiWarp<NyMap>(args->Get(1), N->core());
    if (a0.is_not_valid()) {
        return Return();
    }
    if (NyMap *map = NyMap::Cast(*ob)) {
        a0->set_kid(kTypeMap);
        map->SetMetatable(*a0, N->core());
        return Return(args->Get(0));
    }
    return Return();
}
    
static int BuiltinGetMetatable(Arguments *args, Nyaa *N) {
    Handle<Object> ob = ApiWarpNoCheck<Object>(args->Get(0), N->core());
    if (ob.is_not_valid()) {
        return Return();
    }
    if (NyMap *map = NyMap::Cast(*ob)) {
        return Return(Local<NyMap>::New(map->GetMetatable()));
    }
    return Return();
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
        return Raisef("incorrect entry type.");
    }

    Handle<NyThread> thd(new (*udo) NyThread(N->core()));
    thd->set_entry(*entry);
    thd->SetMetatable(N->core()->kmt_pool()->kThread, N->core());
    auto rs = thd->Init();
    if (!rs) {
        return Raisef("coroutine init fail, cause: %s", rs.ToString().c_str());
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
    return Raisef("coroutine is readonly");
}

// coroutine.yield(a, b, c)
// yield(a,b,c)
static int BuiltinYield(Arguments *args, Nyaa *N) {
    return Raisef("TODO:");
}
    
static int ThreadResume(Arguments *args, Nyaa *N) {
    return Raisef("TODO:");
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
    {"str", BuiltinStr},
    {"print", BuiltinPrint},
    {"yield", BuiltinYield},
    {"raise", BuiltinRaise},
    {"pairs", BuiltinPairs},
    {"range", BuiltinRange},
    {"getmetatable", BuiltinGetMetatable},
    {"setmetatable", BuiltinSetMetatable},
    {nullptr, nullptr},
};

} // namespace nyaa
    
} // namespace mai
