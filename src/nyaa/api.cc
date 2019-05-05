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
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Values:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
bool Value::IsObject() const {
    auto o = reinterpret_cast<const Object *>(this);
    return o->IsObject();
}

bool Value::IsInt() const { return reinterpret_cast<const Object *>(this)->IsSmi(); }

bool Value::IsLong() const { /* TODO */ return false; }

bool Value::IsString() const {
    auto o = reinterpret_cast<const Object *>(this);
    return o->IsObject() && o->ToHeapObject()->IsString();
}
    
bool Value::IsFunction() const {
    auto o = reinterpret_cast<const Object *>(this);
    return o->IsObject() && (o->ToHeapObject()->IsFunction() || o->ToHeapObject()->IsDelegated());
}

bool Value::IsScript() const {
    auto o = reinterpret_cast<const Object *>(this);
    return o->IsObject() && o->ToHeapObject()->IsFunction();
}

int64_t Value::AsInt() const {
    auto o = reinterpret_cast<const Object *>(this);
    return o->IsSmi() ? o->ToSmi() : 0;
}

/*static*/ Handle<Value> Integer::New(Nyaa *N, int64_t val) {
    return Handle<Value>(reinterpret_cast<Value *>(NySmi::New(val)));
}

/*static*/ Handle<String> String::New(Nyaa *N, const char *s, size_t n) {
    auto factory = N->core()->factory();
    return factory->NewString(s, n, false);
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
    
Handle<Value> Result::Get(size_t i) const {
    auto core = reinterpret_cast<const NyArray *>(this);
    return i >= core->size() ? Handle<Value>() : core->Get(i);
}

size_t Result::Length() const {
    return reinterpret_cast<const NyArray *>(this)->size();
}
    
void Function::Bind(int i, Handle<Value> val) {
    if (!IsFunction()) {
        return;
    }
    if (NyClosure *fn = reinterpret_cast<NyObject *>(this)->ToClosure()) {
        fn->Bind(i, reinterpret_cast<Object *>(*val), NyaaCore::Current());
    } else if (NyDelegated *fn = reinterpret_cast<NyObject *>(this)->ToDelegated()) {
        fn->Bind(i, reinterpret_cast<Object *>(*val), NyaaCore::Current());
    }
}

Handle<Value> Function::GetUpVal(int i) {
    if (!IsFunction()) {
        return Handle<Value>();
    }
    if (NyClosure *fn = reinterpret_cast<NyObject *>(this)->ToClosure()) {
        return fn->upval(i);
    } else if (NyDelegated *fn = reinterpret_cast<NyObject *>(this)->ToDelegated()) {
        return fn->upval(i);
    }
    return Handle<Value>();
}
    
/*static*/ Handle<Script> Script::Compile(Nyaa *N, Handle<String> source) {
    // TODO:
    return Handle<Script>::Null();
}

/*static*/ Handle<Script> Script::Compile(Nyaa *N, FILE *fp) {
    // TODO:
    return Handle<Script>::Null();
}

Handle<Result> Script::Run(Nyaa *N) {
    // TODO:
    return Local<Result>();
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

Arguments::Arguments(Value **address, size_t length)
    : length_(length)
    , address_(address) {
}

Arguments::Arguments(size_t length)
    : length_(length) {
    auto addr = NyaaCore::Current()->AdvanceHandleSlots(static_cast<int>(length));
    address_ = reinterpret_cast<Value **>(addr);
    int64_t i = length;
    while (i-- > 0) {
        address_[i] = nullptr;
    }
}

void Arguments::SetCallee(Local<Value> callee) {
    if (!callee_) {
        callee_ = reinterpret_cast<Value **>(HandleScope::Current()->NewHandle(nullptr));
    }
    *callee_ = *callee;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns:
////////////////////////////////////////////////////////////////////////////////////////////////////
int Returnf(Nyaa *N, int nrets, ...) {
    N = !N ? Isolate::Current()->GetNyaa() : N;

    va_list ap;
    va_start(ap, nrets);
    for (int i = 0; i < nrets; ++i) {
        Object *ret = va_arg(ap, Object *);
        N->core()->main_thd()->Push(ret);
    }
    va_end(ap);
    return nrets;
}
    
void ReturnValuesBase::Add(int64_t val) {
    if (val > NySmi::kMaxValue || val < NySmi::kMinValue) {
        AddInternal(NyInt::NewI64(val, N_->core()->factory()));
    } else {
        AddInternal(NySmi::New(val));
    }
}

void ReturnValuesBase::Add(double val) {
    AddInternal(N_->core()->factory()->NewFloat64(val));
}

void ReturnValuesBase::Add(const char *z, size_t n) {
    AddInternal(N_->core()->factory()->NewString(z, n));
}

void ReturnValuesBase::AddInternal(Object *val) {
    N_->core()->main_thd()->Push(val);
}

void ReturnValuesBase::AddInternalSome(int nrets, ...) {
    va_list ap;
    va_start(ap, nrets);
    for (int i = 0; i < nrets; ++i) {
        Object *ret = va_arg(ap, Object *);
        N_->core()->main_thd()->Push(ret);
    }
    va_end(ap);
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
    return Handle<Value>(catch_point_->exception());
}
    
bool TryCatch::HasCaught() const {
    return catch_point_->has_caught();
}

Handle<String> TryCatch::Message() const {
    return Handle<String>(catch_point_->message());
}
    
Handle<Value> TryCatch::StackTrace() const {
    return Handle<Value>(catch_point_->stack_trace());
}

} // namespace nyaa
    
} // namespace mai
