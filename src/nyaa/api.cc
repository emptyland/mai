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

bool Value::IsScript() const {
    auto o = reinterpret_cast<const Object *>(this);
    return o->IsObject() && o->ToHeapObject()->IsScript();
}

int64_t Value::AsInt() const {
    auto o = reinterpret_cast<const Object *>(this);
    return o->IsSmi() ? o->ToSmi() : 0;
}

/*static*/ Handle<Value> Integral::New(Nyaa *N, int64_t val) {
    // TODO:
    uintptr_t word = (val << 1) | 1u;
    return Handle<Value>(reinterpret_cast<Value *>(word));
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
    
/*static*/ Handle<Script> Script::Compile(Nyaa *N, Handle<String> source) {
    // TODO:
    return Handle<Script>::Null();
}

/*static*/ Handle<Script> Script::Compile(Nyaa *N, FILE *fp) {
    // TODO:
    return Handle<Script>::Null();
}

int Script::Run(Nyaa *N) {
    // TODO:
    return -1;
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Handles:
////////////////////////////////////////////////////////////////////////////////////////////////////
HandleScope::HandleScope(Isolate *isolate)
    : isolate_(DCHECK_NOTNULL(isolate)) {
    //NyaaCore::Current()->EnterHandleScope(this);
    isolate_->GetNyaa()->core()->EnterHandleScope(this);
}

HandleScope::~HandleScope() {
    isolate_->GetNyaa()->core()->ExitHandleScope();
}

void **HandleScope::NewHandle(void *value) {
    auto core = isolate_->GetNyaa()->core();
    void **space = reinterpret_cast<void **>(core->AdvanceHandleSlots(1));
    *space = value;
    return space;
}

/*static*/ HandleScope *HandleScope::Current() {
    return Current(Isolate::Current());
}

/*static*/ HandleScope *HandleScope::Current(Isolate *isolate) {
    return DCHECK_NOTNULL(isolate)->GetNyaa()->core()->current_handle_scope();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Arguments:
////////////////////////////////////////////////////////////////////////////////////////////////////
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
    return N->core()->Raised() ? -1 : nrets;
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// TryCatch:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
int Raisef(Nyaa *N, const char *fmt, ...) {
    N = !N ? Isolate::Current()->GetNyaa() : N;

    va_list ap;
    va_start(ap, fmt);
    N->core()->Vraisef(fmt, ap);
    va_end(ap);
    return -1;
}

TryCatch::TryCatch(Isolate *isolate)
    : isolate_(isolate) {
    //isolate_->GetNyaa()->core()->curr_thd()
}

TryCatch::~TryCatch() {
    
}

Handle<Value> TryCatch::Exception() const {
    // TODO:
    return Local<Value>();
}
    
bool TryCatch::HasCaught() const {
    // TODO:
    return false;
}

Handle<String> TryCatch::Message() const {
    // TODO:
    return Local<String>();
}

} // namespace nyaa
    
} // namespace mai
