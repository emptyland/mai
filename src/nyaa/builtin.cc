#include "nyaa/builtin.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/thread.h"
#include "nyaa/object-factory.h"
#include "nyaa/heap.h"
#include "base/allocators.h"
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
    "code",
    "closure",
    "thread",
    "array[byte]",
    "array[int32]",
    "array",
};
    
const size_t kRawBuiltinKzsSize = arraysize(kRawBuiltinKzs);
const size_t kRawBuiltinkmtSize = sizeof(BuiltinMetatablePool) / sizeof(NyMap *);
const size_t kRawBuiltinCodeSize = sizeof(BuiltinCodePool) / sizeof(NyCode *);
    
static_assert(sizeof(BuiltinStrPool) / kPointerSize == arraysize(kRawBuiltinKzs),
              "Incorrect size of kRawBuiltinKzs");
    
const char *kBuiltinTypeName[] = {
//    kTypeNil,
//    kTypeSmi,
//    kTypeUdo,
//#define DEFINE_TYPE(type) kType##type,
//    DECL_BUILTIN_TYPES(DEFINE_TYPE)
//#undef DEFINE_TYPE
    "Nil", "Smi", "Udo",
#define DEFINE_TYPE(type) #type,
    DECL_BUILTIN_TYPES(DEFINE_TYPE)
#undef DEFINE_TYPE
};
    
static void BuiltinPrint(const FunctionCallbackInfo<Object> &info) {
    HandleScope handle_scope(info.VM());
    
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
    HandleScope handle_scope(info.VM());
    
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
    HandleScope handle_scope(info.VM());
    
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
    HandleScope handle_scope(info.VM());
    
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
    HandleScope handle_scope(info.VM());
    
    if (info.Length() < 1) {
        info.GetErrors().Raisef("incorrect argument length, expected: %zd, need: 1", info.Length());
    }
    if (NyMap *map = NyMap::Cast(*info[0])) {
        Handle<NyDelegated> iter = info.Core()->factory()->NewDelegated(BuiltinMapIter, 2);
        iter->Bind(0, map, info.Core());
        iter->Bind(1, Object::kNil, info.Core());
        info.GetReturnValues().Add(iter);
    } else if (NyUDO *uod = NyUDO::Cast(*info[0])) {
        // TODO:
        info.GetErrors().Raisef("TODO:");
    } else {
        info.GetErrors().Raisef("incorrect type: no pairs.");
    }
}
    
static void BuiltinRangeIter(const FunctionCallbackInfo<Object> &info) {
    HandleScope handle_scope(info.VM());
    
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
    HandleScope handle_scope(info.VM());
    
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
    HandleScope handle_scope(info.VM());
    
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
    HandleScope handle_scope(info.VM());
    
    Handle<Object> ob = info[0];
    if (ob.is_not_valid()) {
        return;
    }
    if (NyMap *map = NyMap::Cast(*ob)) {
        info.GetReturnValues().Add(map->GetMetatable());
    }
}
    
static void BuiltinLog(const FunctionCallbackInfo<Object> &info) {
    HandleScope handle_scope(info.VM());
    
    if (info.Length() == 0) {
        return;
    }
    Handle<NyString> buf;
    switch (info.Length()) {
        case 0:
            buf = info.Core()->bkz_pool()->kEmpty;
            break;
        case 1:
            buf = info[0]->ToString(info.Core());
            break;
        default: {
            buf = info.Core()->factory()->NewUninitializedString(64);
            for (size_t i = 0; i < info.Length(); ++i) {
                if (i > 0) {
                    buf = buf->Add("|", 1, info.Core());
                }
                buf = buf->Add(info[i]->ToString(info.Core()), info.Core());
            }
            buf = buf->Done(info.Core());
        } break;
    }

    auto logger = info.Core()->logger();
    auto ci = info.Core()->curr_thd()->call_info();
    auto [file_name, name, line] = ci->prev()->GetCurrentSourceInfo();
    if (file_name) {
        const char *b = file_name->bytes();
        const char *p = b + file_name->size();
        int sp = 0;
        while (p > b) {
            if (*p == '\\' || *p == '/') {
                if (sp++ > 0) {
                    p++;
                    break;
                }
            }
            --p;
        }
        ::fprintf(logger, "LOG %s:%d %s() ", p, line, !name ? "" : name->bytes());
    }
    ::fputs(buf->bytes(), logger);
    ::fputc('\n', logger);
    ::fflush(logger);
}

static void BuiltinRequire(const FunctionCallbackInfo<Object> &info) {
    HandleScope handle_scope(info.VM());

    if (info.Length() < 1) {
        info.GetErrors().Raisef("incorrect arguments length, required: >1");
        return;
    }
    
    NyaaCore *N = info.Core();
    Handle<NyString> arg0 = info[0]->ToString(N);
    Env *paltform = Isolate::Current()->env();

    std::string file_name(arg0->bytes(), arg0->size());
    auto rs = paltform->FileExists(file_name);
    if (!rs) {
        for (const auto &path : info.VM()->find_paths()) {
            file_name = path;
            file_name.append("/");
            file_name.append(arg0->bytes(), arg0->size());
            rs = paltform->FileExists(file_name);
            if (rs.ok()) {
                break;
            }
        }
        if (!rs) {
            info.GetErrors().Raisef("file not found: %s", arg0->bytes());
            return;
        }
    }
    Handle<NyString> key = N->factory()->NewString(file_name.data(), file_name.size());
    if (N->loads()->RawGet(*key, N)) {
        return;
    }
    N->loads()->RawPut(*key, NySmi::New(0), N);
    
    Handle<NyMap> env = Handle<NyMap>::Null();
    if (info.Length() > 1) {
        env = NyMap::Cast(*info[1]);
    }
    env = info.Core()->NewEnv(*env);
    
    FILE *fp = ::fopen(file_name.c_str(), "r");
    if (!fp) {
        info.GetErrors().Raisef("can not open file: %s", arg0->bytes());
        return;
    }
    Handle<NyFunction> script = NyFunction::Compile(file_name.c_str(), fp, N);
    ::fclose(fp);
    Handle<NyClosure> fn = N->factory()->NewClosure(*script);
    int rv = N->curr_thd()->Run(*fn, nullptr/*argv*/, 0/*argc*/, -1/*wanted*/, *env);
    if (rv > 0) {
        info.GetReturnValues().Set(rv);
    }
}
    
static void BuiltinLoadFile(const FunctionCallbackInfo<Object> &info) {
    HandleScope handle_scope(info.VM());

    if (info.Length() < 1) {
        info.GetErrors().Raisef("incorrect arguments length, required: >1");
        return;
    }
    
    NyaaCore *N = info.Core();
    Handle<NyString> arg0 = info[0]->ToString(N);
    Env *paltform = Isolate::Current()->env();
    
    std::string file_name(arg0->bytes(), arg0->size());
    auto rs = paltform->FileExists(file_name);
    if (!rs) {
        info.GetErrors().Raisef("file not found: %s", arg0->bytes());
        return;
    }
    FILE *fp = ::fopen(file_name.c_str(), "r");
    if (!fp) {
        info.GetErrors().Raisef("can not open file: %s", arg0->bytes());
        return;
    }
    TryCatchCore try_catch(info.Core());
    Handle<NyClosure> fn = NyClosure::Compile(file_name.c_str(), fp, info.Core());
    ::fclose(fp);
    if (try_catch.has_caught()) {
        info.GetReturnValues().AddNil().Add(try_catch.message());
    } else {
        info.GetReturnValues().Add(fn);
    }
}
    
static void BuiltinLoadString(const FunctionCallbackInfo<Object> &info) {
    HandleScope handle_scope(info.VM());

    if (info.Length() < 1) {
        info.GetErrors().Raisef("incorrect arguments length, required: >1");
        return;
    }
    
    NyaaCore *N = info.Core();
    Handle<NyString> source = info[0]->ToString(N);
    TryCatchCore try_catch(info.Core());
    Handle<NyClosure> fn = NyClosure::Compile(source->bytes(), source->size(), N);
    if (try_catch.has_caught()) {
        info.GetReturnValues().AddNil().Add(try_catch.message());
    } else {
        info.GetReturnValues().Add(fn);
    }
}
    
static void BuiltinPCall(const FunctionCallbackInfo<Object> &info) {
    if (info.Length() < 1) {
        info.GetErrors().Raisef("incorrect arguments length, required: >1");
        return;
    }
    
    NyaaCore *N = info.Core();
    NyRunnable *fn;
    base::ScopedMemoryTemplate<Object *, 8> scoped;
    int argc = static_cast<int>(info.Length() - 1);
    Object **argv = scoped.New(argc);
    {
        HandleScope handle_scope(info.VM());
        fn = NyRunnable::Cast(*info[0]);
        for (int i = 0; i < argc; i++) {
            argv[i] = *info[i + 1];
        }
    }
    TryCatchCore try_catch(info.Core());
    int nrets = N->curr_thd()->TryRun(fn, argv, argc);
    if (try_catch.has_caught()) {
        info.GetReturnValues().AddNil().Add(try_catch.message());
    } else {
        info.GetReturnValues().Set(nrets);
    }
}

static void BuiltinGarbageCollect(const FunctionCallbackInfo<Object> &info) {
    if (info.Length() < 1) {
        info.GetErrors().Raisef("incorrect arguments length, required: >1");
        return;
    }

    GarbageCollectionHistogram histogram;
    HandleScope handle_scope(info.VM());
    NyaaCore *N = info.Core();
    if (*info[0] == N->factory()->NewString("major")) {
        N->GarbageCollect(kMajorGC, &histogram);
    } else if (*info[0] == N->factory()->NewString("minor")) {
        N->GarbageCollect(kMinorGC, &histogram);
    } else if (*info[0] == N->factory()->NewString("full")) {
        N->GarbageCollect(kFullGC, &histogram);
    } else {
        info.GetReturnValues().Add("gc not run");
        return;
    }
    info.GetReturnValues()
        .Add(histogram.collected_bytes)
        .Add(histogram.collected_objs)
        .AddF64(histogram.time_cost);
}

static void BuiltinAssert(const FunctionCallbackInfo<Object> &info) {
    HandleScope handle_scope(info.VM());
    NyaaCore *N = info.Core();
    Handle<NyString> message;
    if (info.Length() >= 2) {
        message = info[1]->ToString(info.Core());
    }
    if (!message) {
        message = N->factory()->NewString("assert fail");
    }

    if (info.Length() >= 1) {
        if (info[0]->IsFalse()) {
            info.GetErrors().Raise(message->bytes(), info[0]);
        } else {
            info.GetReturnValues().Add(info[0]);
        }
    }
}
    
static void ThreadInit(const FunctionCallbackInfo<Object> &info) {
    if (info.Length() < 2) {
        info.GetErrors().Raisef("incorrect arguments length, required: >1");
        return;
    }
    
    HandleScope handle_scope(info.VM());
    NyaaCore *N = info.Core();
    Handle<NyUDO> udo = NyUDO::Cast(*info[0]);
    if (udo.is_not_valid()) {
        info.GetErrors().Raisef("incorrect constructor calling.");
        return;
    }
    
    Handle<NyRunnable> entry = NyRunnable::Cast(*info[1]);
    if (entry.is_not_valid()) {
        info.GetErrors().Raisef("incorrect argument[1] type, required: function");
        return;
    }
    
    Handle<NyThread> thread = new (*udo) NyThread(N);
    auto rs = thread->Init();
    if (!rs) {
        info.GetErrors().Raisef("thread initialize fail: %s", rs.ToString().c_str());
        return;
    }
    thread->set_entry(*entry);
    thread->SetMetatable(N->kmt_pool()->kThread, N);
    thread->SetType(kTypeThread); // FIXME
    thread->SetColor(N->heap()->initial_color());
    N->InsertThread(*thread);
}

static void ThreadIndex(const FunctionCallbackInfo<Object> &info) {
    if (info.Length() < 2) {
        info.GetErrors().Raisef("incorrect arguments length, required: >1");
        return;
    }

    HandleScope handle_scope(info.VM());
    NyaaCore *N = info.Core();
    Handle<NyThread> thread = NyThread::Cast(*info[0]);
    if (thread.is_not_valid()) {
        info.GetErrors().Raisef("incorrect argument[0] type, required: thread");
        return;
    }
    
    Handle<NyString> key = info[1]->ToString(info.Core());
    if (*key == N->factory()->NewString("status")) {
        switch (thread->state()) {
            case NyThread::kDead:
                info.GetReturnValues().Add(N->bkz_pool()->kDead);
                break;
            case NyThread::kRunning:
                info.GetReturnValues().Add(N->bkz_pool()->kRunning);
                break;
            case NyThread::kSuspended:
                info.GetReturnValues().Add(N->bkz_pool()->kSuspended);
                break;
            default:
                break;
        }
    } else {
        info.GetReturnValues().Add(thread->GetMetatable()->RawGet(*key, N));
    }
}
    
static void ThreadNewindex(const FunctionCallbackInfo<Object> &info) {
    HandleScope handle_scope(info.VM());
    // TODO:
    info.GetErrors().Raisef("TODO:");
}

static void BuiltinYield(const FunctionCallbackInfo<Object> &info) {
    info.GetReturnValues().Set(static_cast<int>(info.Length()));
    info.GetReturnValues().Yield();
}
    
static void ThreadResume(const FunctionCallbackInfo<Object> &info) {
    if (info.Length() < 1) {
        info.GetErrors().Raisef("incorrect arguments length, required: >1");
        return;
    }
    
    NyaaCore *N = info.Core();
    NyThread *thread;
    base::ScopedMemoryTemplate<Object *, 8> scoped;
    int argc = static_cast<int>(info.Length() - 1);
    Object **argv = scoped.New(argc);
    {
        HandleScope handle_scope(info.VM());
        thread = NyThread::Cast(*info[0]);
        if (!thread) {
            info.GetErrors().Raisef("incorrect argument[0] type, unexpected: %d, requried: thread",
                                    info[0]->GetType());
            return;
        }
        for (int i = 0; i < argc; i++) {
            argv[i] = *info[i + 1];
        }
    }

    TryCatchCore try_catch(N, thread);
    int nrets = thread->Resume(argv, argc, -1, nullptr); // TODO: env

    if (try_catch.has_caught()) {
        info.GetReturnValues().AddNil().Add(try_catch.message());
    } else {
        for (int i = 0; i <  nrets; ++i) {
            info.GetReturnValues().Add(thread->Get(-(nrets - i)));
        }
    }
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
    //kDelegated->RawPut(kzs->kInnerCall, factory->NewDelegated(DelegatedCall, 0, true), N);

    //----------------------------------------------------------------------------------------------
    // NyFunction
    kFunction = NEW_METATABLE(kTypeFunction);
    kFunction->RawPut(kzs->kInnerType, kzs->kFunction, N);
    
    //----------------------------------------------------------------------------------------------
    // NyCode
    kCode = NEW_METATABLE(kTypeCode);
    kCode->RawPut(kzs->kInnerType, kzs->kCode, N);
    
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
    {"log", BuiltinLog},
    {"print", BuiltinPrint},
    {"yield", BuiltinYield},
    {"raise", BuiltinRaise},
    {"pairs", BuiltinPairs},
    {"range", BuiltinRange},
    {"pcall", BuiltinPCall},
    {"assert", BuiltinAssert},
    {"require", BuiltinRequire},
    {"loadfile", BuiltinLoadFile},
    {"loadstring", BuiltinLoadString},
    {"getmetatable", BuiltinGetMetatable},
    {"setmetatable", BuiltinSetMetatable},
    {"garbagecollect", BuiltinGarbageCollect},
    {nullptr, nullptr},
};

} // namespace nyaa
    
} // namespace mai
