#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "nyaa/nyaa-values.h"
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

/*static*/ Isolate *Isolate::New(const Options &opts) {
    auto is = new Isolate(opts);
    if (!is->Init()) {
        is->Dispose();
        return nullptr;
    }
    return is;
}

Isolate::Isolate(const Options &opts)
    : env_(DCHECK_NOTNULL(opts.env))
    , major_area_initial_size_(opts.major_area_max_size)
    , major_area_max_size_(opts.major_area_max_size)
    , init_thread_stack_size_(opts.init_thread_stack_size) {
}

Isolate::~Isolate() {
    
}

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

Nyaa::Nyaa(Isolate *is)
    : isolate_(DCHECK_NOTNULL(is))
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

/*static*/ Handle<Value> Integral::New(Nyaa *N, int64_t val) {
    // TODO:
    return Handle<Value>::Null();
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

Handle<Value> Script::Run(Nyaa *N) {
    // TODO:
    return Handle<Value>::Null();
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
    
    
} // namespace nyaa
    
} // namespace mai
