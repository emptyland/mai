#include "mai-lang/isolate.h"
#include "glog/logging.h"
#include <shared_mutex>

namespace mai {

namespace nyaa {
    
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
    , major_area_max_size_(opts.major_area_max_size) {
    
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
    
} // namespace nyaa

} // namespace mai
