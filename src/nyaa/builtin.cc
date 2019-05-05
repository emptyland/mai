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
    
static void BuiltinPrint(const FunctionCallbackInfo<Object> &info) {
    for (size_t i = 0; i < info.Length(); ++i) {
        Handle<NyString> str(info[i]->ToString(info.Core()));
        if (i > 0) {
            putchar('\t');
        }
        fwrite(str->bytes(), 1, str->size(), stdout);
    }
    putchar('\n');
    info.GetReturnValues().Add(info.Length());
}
    
static void BuiltinStr(const FunctionCallbackInfo<Object> &info) {
    switch (info.Length()) {
        case 0:
            info.GetReturnValues().Add("");
            break;
        case 1:
            info.GetReturnValues().Add(info[0]->ToString(info.Core()));
            break;
        default: {
            Handle<NyString> buf = info.Core()->factory()->NewUninitializedString(64);
            for (size_t i = 0; i < info.Length(); ++i) {
                buf = buf->Add(info[i]->ToString(info.Core()), info.Core());
            }
            info.GetReturnValues().Add(buf->Done(info.Core()));
        } break;
    }
}
    
static void BuiltinRaise(const FunctionCallbackInfo<Object> &info) {
    auto msg = Handle<NyString>::Null();
    if (info.Length() >= 1) {
        msg = info[0]->ToString(info.Core());
    }
    
    auto ex = Handle<Object>::Null();
    if (info.Length() >= 2) {
        ex = info[1];
    }
    info.Core()->curr_thd()->Raise(*msg, *ex);
}
    
static void BuiltinMapIter(const FunctionCallbackInfo<Object> &info) {
    Handle<NyDelegated> callee = NyDelegated::Cast(*info.Callee());
    DCHECK(callee.is_valid());
    Handle<NyMap> map = NyMap::Cast(callee->upval(0));
    DCHECK(map.is_valid());
    Handle<Object> key = callee->upval(1);

    Object *next, *value;
    std::tie(next, value) = map->GetNextPair(*key, info.Core());
    callee->Bind(1, next, info.Core());
    info.GetReturnValues().Add(next).Add(value);
}
    
static void BuiltinPairs(const FunctionCallbackInfo<Object> &info) {
    if (info.Length() < 1) {
        info.GetErrors().Raisef("incorrect argument length, expected: %zd, need: 1", info.Length());
    }
    if (NyMap *map = NyMap::Cast(*info[0])) {
        Handle<NyDelegated> iter = info.Core()->factory()->NewDelegated(BuiltinMapIter, 2);
        iter->Bind(0, map, info.Core());
        iter->Bind(1, Object::kNil, info.Core());
        //return Return(iter);
        info.GetReturnValues().Add(iter);
    } else if (NyUDO *uod = NyUDO::Cast(*info[0])) {
        // TODO:
        info.GetErrors().Raisef("TODO:");
    } else {
        info.GetErrors().Raisef("incorrect type: no pairs.");
    }
}
    
static void BuiltinRangeIter(const FunctionCallbackInfo<Object> &info) {
    Handle<NyDelegated> callee = NyDelegated::Cast(*info.Callee());
    DCHECK(callee.is_valid());
    
    int64_t curr = callee->upval(0)->ToSmi();
    int64_t end  = callee->upval(1)->ToSmi();
    int64_t step = callee->upval(2)->ToSmi();
    if (step > 0) {
        if (curr >= end) {
            info.GetReturnValues().AddNil();
            return;
        }
    } else {
        if (curr <= end) {
            info.GetReturnValues().AddNil();
            return;
        }
    }
    curr += step;
    Handle<Object> rv = NySmi::New(curr);
    callee->Bind(0, *rv, info.Core());
    info.GetReturnValues().Add(rv);
}

static void BuiltinRange(const FunctionCallbackInfo<Object> &info) {
    int64_t begin = 0, end = 0, step = 1;
    switch (info.Length()) {
        case 3: {
            Handle<Object> ob = info[2];
            if (!ob->IsSmi()) {
                info.GetErrors().Raisef("incorrect type: need integer.");
            }
            step = ob->ToSmi();
        } // though
        case 2: {
            Handle<Object> ob = info[1];
            if (!ob->IsSmi()) {
                info.GetErrors().Raisef("incorrect type: need integer.");
            }
            end = ob->ToSmi();
            ob = info[0];
            if (!ob->IsSmi()) {
                info.GetErrors().Raisef("incorrect type: need integer.");
            }
            begin = ob->ToSmi();
        } break;

        default:
            info.GetErrors().Raisef("incorrect number of arguments. need [2, 3]");
            return;
    }
    Handle<NyDelegated> iter = info.Core()->factory()->NewDelegated(BuiltinRangeIter, 3);
    iter->Bind(0, NySmi::New(begin), info.Core());
    iter->Bind(1, NySmi::New(end), info.Core());
    iter->Bind(2, NySmi::New(step), info.Core());
    info.GetReturnValues().Add(iter);
}
    
static void BuiltinSetMetatable(const FunctionCallbackInfo<Object> &info) {
    Handle<Object> ob = info[0];
    if (ob.is_not_valid()) {
        return;
    }
    Handle<NyMap> a1 = NyMap::Cast(*info[1]);
    if (a1.is_not_valid()) {
        return;
    }
    if (NyMap *map = NyMap::Cast(*ob)) {
        a1->set_kid(kTypeMap);
        map->SetMetatable(*a1, info.Core());
        info.GetReturnValues().Add(info[0]);
    }
}
    
static void BuiltinGetMetatable(const FunctionCallbackInfo<Object> &info) {
    Handle<Object> ob = info[0];
    if (ob.is_not_valid()) {
        return;
    }
    if (NyMap *map = NyMap::Cast(*ob)) {
        info.GetReturnValues().Add(map->GetMetatable());
    }
}

static void DelegatedCall(const FunctionCallbackInfo<Object> &info) {
    // TODO:
    info.GetErrors().Raisef("TODO:");
}
    
static void ThreadInit(const FunctionCallbackInfo<Object> &info) {
    // TODO:
    info.GetErrors().Raisef("TODO:");
}

static void ThreadIndex(const FunctionCallbackInfo<Object> &info) {
    // TODO:
    info.GetErrors().Raisef("TODO:");
}
    
static void ThreadNewindex(const FunctionCallbackInfo<Object> &info) {
    info.GetErrors().Raisef("TODO:");
}

// coroutine.yield(a, b, c)
// yield(a,b,c)
static void BuiltinYield(const FunctionCallbackInfo<Object> &info) {
    info.GetErrors().Raisef("TODO:");
}
    
static void ThreadResume(const FunctionCallbackInfo<Object> &info) {
    info.GetErrors().Raisef("TODO:");
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
