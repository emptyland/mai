#include "lang/isolate-inl.h"
#include "lang/scheduler.h"
#include "lang/machine.h"
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
    if (new_space_initial_size_ < 10 * base::kMB) {
        return MAI_CORRUPTION("new_space_initial_size too small(least than 10MB)");
    }
    
    if (auto err = env_->NewThreadLocalSlot("isolate.tls", &TLSStorage::Dtor, &tls_); err.fail()) {
        return err;
    }
    
    if (auto err = heap_->Initialize(new_space_initial_size_); err.fail()) {
        return err;
    }

    if (auto err = metadata_space_->Initialize(); err.fail()) {
        return err;
    }
    return Error::OK();
}

Isolate::Isolate(const Options &opts)
    : new_space_initial_size_(opts.new_space_initial_size)
    , env_(opts.env)
    , heap_(new Heap(env_->GetLowLevelAllocator()))
    , metadata_space_(new MetadataSpace(env_->GetLowLevelAllocator()))
    , scheduler_(new Scheduler(opts.concurrency <= 0 ? env_->GetNumberOfCPUCores() : opts.concurrency)) {
}

Isolate::~Isolate() {
    delete metadata_space_;
    delete scheduler_;
    delete heap_;
}

void Isolate::Enter() {
    std::call_once(init_flag, [this]() {
        DCHECK(__isolate == nullptr) << "Reinitialize isolate";
        __isolate = this;
    });
    DCHECK_EQ(this, __isolate);

    // Init machine0
    scheduler_->machine0()->Enter();
}

void Isolate::Exit() {
    // Exit machine0
    scheduler_->machine0()->Exit();

    DCHECK_EQ(this, __isolate);
    __isolate = nullptr;
}

} // namespace lang

} // namespace mai
