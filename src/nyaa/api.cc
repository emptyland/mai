#include "nyaa/nyaa-core.h"
#include "nyaa/thread.h"
#include "nyaa/object-factory.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/thread.h"
#include "base/slice.h"
#include "mai-lang/nyaa.h"
#include <shared_mutex>

namespace mai {
    
namespace nyaa {
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Isolate:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
static Isolate *top = nullptr;
static std::shared_mutex is_mutex;

void Isolate::Enter() {
    std::unique_lock<std::shared_mutex> lock(is_mutex);
    prev_ = top;
    DCHECK_NE(top, this);
    top = this;
}

void Isolate::Exit() {
    std::unique_lock<std::shared_mutex> lock(is_mutex);
    DCHECK_EQ(top, this);
    top = prev_;
}

void Isolate::Dispose() { delete this; }

/*static*/ Isolate *Isolate::Current() {
    std::shared_lock<std::shared_mutex> lock(is_mutex);
    return DCHECK_NOTNULL(top);
}

/*static*/ Isolate *Isolate::New(Env *env) {
    auto is = new Isolate(env);
    if (!is->Init()) {
        is->Dispose();
        return nullptr;
    }
    return is;
}

Isolate::Isolate(Env *env) : env_(DCHECK_NOTNULL(env)) {}

Isolate::~Isolate() {}

bool Isolate::Init() {
    auto rs = env_->NewThreadLocalSlot("nyaa", nullptr, &ctx_tls_);
    if (!rs) {
        return false;
    }
    
    // TODO:
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Nyaa:
////////////////////////////////////////////////////////////////////////////////////////////////////

Nyaa::Nyaa(const Options &opt, Isolate *is)
    : isolate_(DCHECK_NOTNULL(is))
    , major_area_initial_size_(opt.major_area_initial_size)
    , major_area_max_size_(opt.major_area_max_size)
    , init_thread_stack_size_(opt.init_thread_stack_size)
    , minor_area_initial_size_(opt.minor_area_initial_size)
    , minor_area_max_size_(opt.minor_area_max_size)
    , nogc_(opt.nogc)
    , use_string_pool_(opt.use_string_pool)
    , core_(new NyaaCore(this)) {
    prev_ = isolate_->GetNyaa();
    DCHECK_NE(prev_, this);
    isolate_->SetNyaa(this);
}

Nyaa::~Nyaa() {
    DCHECK_EQ(this, isolate_->GetNyaa());
    isolate_->SetNyaa(prev_);
    prev_ = nullptr;
}

void Nyaa::SetGlobal(Handle<String> name, Handle<Value> value) {
    core_->SetGlobal(reinterpret_cast<NyString *>(*name),
                     reinterpret_cast<Object *>(*value));
}

Handle<Value> Nyaa::GetGlobal(Handle<String> name) {
    Object *val = core_->g()->RawGet(reinterpret_cast<NyString *>(*name), core_.get());
    return reinterpret_cast<Value *>(val);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Values:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
bool Value::IsObject() const {
    auto o = reinterpret_cast<const Object *>(this);
    return o->IsObject();
}

bool Value::IsNumber() const {
    auto o = reinterpret_cast<const Object *>(this);
    return o->IsSmi() || o->ToHeapObject()->IsNumeric();
}

bool Value::IsString() const {
    auto o = reinterpret_cast<const Object *>(this);
    return o->IsObject() && o->ToHeapObject()->IsString();
}
    
bool Value::IsFunction() const {
    auto o = reinterpret_cast<const Object *>(this);
    return o->IsObject() && (o->ToHeapObject()->IsRunnable());
}

bool Value::IsScript() const {
    auto o = reinterpret_cast<const Object *>(this);
    return o->IsObject() && o->ToHeapObject()->IsFunction();
}

/*static*/ Handle<Number> Number::NewI64(int64_t val) {
    NyaaCore *N = NyaaCore::Current();
    if (val > NySmi::kMaxValue || val < NySmi::kMinValue) {
        return Handle<Number>(reinterpret_cast<Number *>(NyInt::NewI64(val, N->factory())));
    }
    return Handle<Number>(reinterpret_cast<Number *>(NySmi::New(val)));
}
    
/*static*/ Handle<Number> Number::NewF64(double val) {
    NyaaCore *N = NyaaCore::Current();
    return Handle<Number>(reinterpret_cast<Number *>(N->factory()->NewFloat64(val)));
}
    
int64_t Number::I64Value() const {
    const Object *maybe = reinterpret_cast<const Object *>(this);
    switch (maybe->GetType()) {
        case kTypeSmi:
            return maybe->ToSmi();
        case kTypeInt:
            return NyInt::Cast(maybe)->ToI64();
        case kTypeFloat64:
            return static_cast<int64_t>(NyFloat64::Cast(maybe)->value());
        default:
            break;
    }
    return 0;
}

double Number::F64Value() const {
    const Object *maybe = reinterpret_cast<const Object *>(this);
    switch (maybe->GetType()) {
        case kTypeSmi:
            return maybe->ToSmi();
        case kTypeInt:
            return NyInt::Cast(maybe)->ToF64();
        case kTypeFloat64:
            return NyFloat64::Cast(maybe)->value();
        default:
            break;
    }
    return 0;
}

/*static*/ Handle<String> String::New(const char *s, size_t n) {
    NyaaCore *N = NyaaCore::Current();
    return reinterpret_cast<String *>(N->factory()->NewString(s, n, false));
}

uint32_t String::HashVal() const {
    return reinterpret_cast<const NyString *>(this)->hash_val();
}

const char *String::Bytes() const {
    return reinterpret_cast<const NyString *>(this)->bytes();
}

size_t String::Length() const {
    return reinterpret_cast<const NyString *>(this)->size();
}
    
/*static*/ Handle<Function> Function::New(Function::InvocationCallback callback,
                                          size_t nupvals) {
    NyaaCore *N = NyaaCore::Current();
    NyDelegated *fn = N->factory()->NewDelegated(callback, nupvals);
    return Handle<Function>(reinterpret_cast<Function *>(fn));
}
    
void Function::Bind(int i, Handle<Value> val) {
    DCHECK(IsFunction());
    Object *maybe = reinterpret_cast<Object *>(this);
    if (NyClosure *fn = NyClosure::Cast(maybe)) {
        fn->Bind(i, reinterpret_cast<Object *>(*val), NyaaCore::Current());
    } else if (NyDelegated *fn = NyDelegated::Cast(maybe)) {
        fn->Bind(i, reinterpret_cast<Object *>(*val), NyaaCore::Current());
    }
}

Handle<Value> Function::GetUpVal(int i) {
    DCHECK(IsFunction());
    Object *maybe = reinterpret_cast<Object *>(this);
    if (NyClosure *fn = NyClosure::Cast(maybe)) {
        return reinterpret_cast<Value *>(fn->upval(i));
    } else if (NyDelegated *fn = NyDelegated::Cast(maybe)) {
        return reinterpret_cast<Value *>(fn->upval(i));
    }
    return Handle<Value>::Empty();
}
    
int Function::Call(Handle<Value> argv[], int argc, Handle<Value> rets[], int wanted) {
    DCHECK(IsFunction());
    NyaaCore *N = NyaaCore::Current();
    NyRunnable *self = NyRunnable::Cast(reinterpret_cast<Object *>(this));

    base::ScopedMemoryTemplate<Object *, 8> tmp;
    Object **input = tmp.New(argc);
    for (int i = 0; i < argc; ++i) {
        input[i] = reinterpret_cast<Object *>(*argv[i]);
    }
    int rv = self->Apply(input, argc, wanted, N);
    if (rv >= 0) {
        DCHECK(wanted < 0 || rv == wanted) << rv << " vs " << wanted;
        int n = wanted < 0 ? rv : wanted;
        for (int i = 0; i < n; ++i) {
            rets[i] = reinterpret_cast<Value *>(N->Get(-(n - i)));
        }
    }
    return rv;
}
    
int Function::Call(Handle<Value> argv[], int argc, std::vector<Handle<Value>> *rets, int wanted) {
    DCHECK(IsFunction());
    NyaaCore *N = NyaaCore::Current();
    NyRunnable *self = NyRunnable::Cast(reinterpret_cast<Object *>(this));
    
    base::ScopedMemoryTemplate<Object *, 8> tmp;
    Object **input = tmp.New(argc);
    for (int i = 0; i < argc; ++i) {
        input[i] = reinterpret_cast<Object *>(*argv[i]);
    }
    int rv = self->Apply(input, argc, wanted, N);
    if (rv >= 0) {
        rets->clear();
        DCHECK(wanted < 0 || rv == wanted) << rv << " vs " << wanted;
        int n = wanted < 0 ? rv : wanted;
        for (int i = 0; i < n; ++i) {
            rets->push_back(reinterpret_cast<Value *>(N->Get(-(n - i))));
        }
    }
    return rv;
}
    
/*static*/ Handle<Script> Script::Compile(Handle<String> source) {
    NyaaCore *N = NyaaCore::Current();
    Handle<NyClosure> script = NyClosure::Compile(source->Bytes(), source->Length(), N);
    if (script.is_empty()) {
        return Handle<Script>();
    }
    return Handle<Script>(reinterpret_cast<Script *>(*script));
}

/*static*/ Handle<Script> Script::Compile(const char *file_name, FILE *fp) {
    NyaaCore *N = NyaaCore::Current();
    Handle<NyClosure> script = NyClosure::Compile(file_name, fp, N);
    if (script.is_empty()) {
        return Handle<Script>();
    }
    return Handle<Script>(reinterpret_cast<Script *>(*script));
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Handles:
////////////////////////////////////////////////////////////////////////////////////////////////////
HandleScope::HandleScope() : HandleScope(Isolate::Current()->GetNyaa()) {}

HandleScope::HandleScope(Nyaa *N)
    : N_(DCHECK_NOTNULL(N)) {
    N_->core()->EnterHandleScope(this);
}

HandleScope::~HandleScope() {
    N_->core()->ExitHandleScope();
}

void **HandleScope::NewHandle(void *value) {
    void **space = reinterpret_cast<void **>(N_->core()->AdvanceHandleSlots(1));
    *space = value;
    return space;
}
    
HandleScope *HandleScope::Prev() {
    auto core = N_->core();
    auto slot = DCHECK_NOTNULL(core->current_handle_scope_slot());
    return DCHECK_NOTNULL(slot->prev)->scope;
}

/*static*/ HandleScope *HandleScope::Current() {
    return Current(Isolate::Current()->GetNyaa());
}

/*static*/ HandleScope *HandleScope::Current(Nyaa *N) {
    return DCHECK_NOTNULL(N)->core()->current_handle_scope();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Arguments:
////////////////////////////////////////////////////////////////////////////////////////////////////
void FunctionCallbackBase::Raisef(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    N_->core()->Vraisef(fmt, ap);
    va_end(ap);
}

FunctionCallbackBase::FunctionCallbackBase(void *address, size_t length, Nyaa *N)
    : address_(DCHECK_NOTNULL(address))
    , length_(length)
    , N_(DCHECK_NOTNULL(N)) {
}

FunctionCallbackBase::FunctionCallbackBase(size_t length, Nyaa *N)
    : address_(N->core()->AdvanceHandleSlots(static_cast<int>(length) + 1))
    , length_(length)
    , N_(DCHECK_NOTNULL(N)) {
    static_cast<void **>(address_)[0] = nullptr;
}
    
NyaaCore *FunctionCallbackBase::Core() const { return N_->core(); }
    
Object *FunctionCallbackBase::CurrentEnv() const {
    auto ci = N_->core()->curr_thd()->call_info();
    return ci->env();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns:
////////////////////////////////////////////////////////////////////////////////////////////////////
void ErrorsBase::Raisef(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    N_->core()->Vraisef(fmt, ap);
    va_end(ap);
}

void ErrorsBase::Raise(const char *z, size_t n, void *ex) {
    auto core = N_->core();
    core->curr_thd()->Raise(core->factory()->NewString(z, n),
                            static_cast<Object *>(ex));
}
    
ReturnValuesBase::ReturnValuesBase(Nyaa *N)
    : N_(DCHECK_NOTNULL(N)) {
}
    
void ReturnValuesBase::Add(int64_t val) {
    if (val > NySmi::kMaxValue || val < NySmi::kMinValue) {
        AddInternal(NyInt::NewI64(val, N_->core()->factory()));
    } else {
        AddInternal(NySmi::New(val));
    }
}

void ReturnValuesBase::AddF64(double val) {
    AddInternal(N_->core()->factory()->NewFloat64(val));
}

void ReturnValuesBase::Add(const char *z, size_t n) {
    AddInternal(N_->core()->factory()->NewString(z, n));
}

void ReturnValuesBase::AddInternal(Object *val) {
    auto ci = N_->core()->curr_thd()->call_info();
    ci->set_nrets(ci->nrets() + 1);
    N_->core()->main_thd()->Push(val);
}

void ReturnValuesBase::AddInternalSome(int nrets, ...) {
    va_list ap;
    va_start(ap, nrets);
    for (int i = 0; i < nrets; ++i) {
        AddInternal(va_arg(ap, Object *));
    }
    va_end(ap);
}
    
void ReturnValuesBase::Set(int nrets) {
    auto ci = N_->core()->curr_thd()->call_info();
    ci->set_nrets(nrets);
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// TryCatch:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
int Raisef(const char *fmt, ...) {
    Nyaa *N = Isolate::Current()->GetNyaa();

    va_list ap;
    va_start(ap, fmt);
    N->core()->Vraisef(fmt, ap);
    va_end(ap);
    return -1;
}

TryCatch::TryCatch(Nyaa *N)
    : N_(N)
    , catch_point_(new TryCatchCore(N->core())) {
}

TryCatch::~TryCatch() {
}

Handle<Value> TryCatch::Exception() const {
    return Handle<Value>(reinterpret_cast<Value *>(catch_point_->exception()));
}
    
bool TryCatch::HasCaught() const {
    return catch_point_->has_caught();
}

Handle<String> TryCatch::Message() const {
    return Handle<String>(reinterpret_cast<String *>(catch_point_->message()));
}
    
Handle<Value> TryCatch::StackTrace() const {
    return Handle<Value>(reinterpret_cast<Value *>(catch_point_->stack_trace()));
}

} // namespace nyaa
    
} // namespace mai
