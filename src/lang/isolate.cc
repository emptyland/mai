#include "lang/isolate-inl.h"
#include "lang/heap.h"
#include "lang/metadata-space.h"
#include "glog/logging.h"
#include <mutex>

namespace mai {

namespace lang {

static std::once_flag init_flag;

Isolate *__isolate = nullptr; // Global isolate object

/*static*/ Isolate *Isolate::New(const Options &opts) {
    return new Isolate(opts);
}

void Isolate::Dispose() {
    delete this;
}

Error Isolate::Initialize() {
    // TODO:
    return Error::OK();
}

Isolate::Isolate(const Options &opts)
    : env_(opts.env) {
    // TODO:
}

Isolate::~Isolate() {
    
}

void Isolate::Enter() {
    std::call_once(init_flag, [this]() {
        DCHECK(__isolate == nullptr) << "Reinitialize isolate";
        __isolate = this;
    });
    DCHECK_EQ(this, __isolate);
}

void Isolate::Leave() {
    DCHECK_EQ(this, __isolate);
    __isolate = nullptr;
}

} // namespace lang

} // namespace mai
