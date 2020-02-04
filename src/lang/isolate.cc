#include "lang/isolate-inl.h"
#include "lang/scheduler.h"
#include "lang/machine.h"
#include "lang/bytecode.h"
#include "lang/heap.h"
#include "lang/metadata-space.h"
#include "asm/utils.h"
#include "glog/logging.h"
#include <mutex>

namespace mai {

namespace lang {

static std::mutex init_mutex;

Isolate *__isolate = nullptr; // Global isolate object

#define MEMBER_OFFSET_OF(field) \
    arch::ObjectTemplate<Isolate, int32_t>::OffsetOf(&Isolate :: field)

const ptrdiff_t GlobalHandleNode::kOffsetHandle = offsetof(GlobalHandleNode, handle);

const int32_t Isolate::kOffsetBytecodeHandlerEntries = MEMBER_OFFSET_OF(bytecode_handler_entries_);
const int32_t Isolate::kOffsetTrampolineSuspendPoint = MEMBER_OFFSET_OF(trampoline_suspend_point_);

static void BadSuspendPointDummy() {
    NOREACHED() << "Bad suspend-point";
}

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
    
    for (int i = 0; i < kMax_Bytecodes; i++) {
        bytecode_handler_entries_[i] = metadata_space_->bytecode_handlers()[i]->entry();
    }
    trampoline_suspend_point_ = metadata_space_->trampoline_suspend_point();
    return Error::OK();
}

Isolate::Isolate(const Options &opts)
    : new_space_initial_size_(opts.new_space_initial_size)
    , env_(opts.env)
    , heap_(new Heap(env_->GetLowLevelAllocator()))
    , metadata_space_(new MetadataSpace(env_->GetLowLevelAllocator()))
    , scheduler_(new Scheduler(opts.concurrency <= 0 ?
                               env_->GetNumberOfCPUCores() : opts.concurrency,
                               opts.env->GetLowLevelAllocator()))
    , persistent_dummy_(new GlobalHandleNode{})
    , bytecode_handler_entries_(new Address[kMax_Bytecodes])
    , trampoline_suspend_point_(reinterpret_cast<Address>(BadSuspendPointDummy)) {
}

Isolate::~Isolate() {
    delete [] bytecode_handler_entries_;
    
    while (!QUEUE_EMPTY(persistent_dummy_)) {
        auto x = persistent_dummy_->next_;
        QUEUE_REMOVE(x);
        delete x;
    }
    delete persistent_dummy_;

    delete metadata_space_;
    delete scheduler_;
    delete heap_;
}

void Isolate::Enter() {
    {
        std::lock_guard<std::mutex> lock(init_mutex);
        DCHECK(__isolate == nullptr) << "Reinitialize isolate";
        __isolate = this;
        DCHECK_EQ(this, __isolate);
    }

    // Init machine0
    scheduler_->machine0()->Enter();
}

void Isolate::Exit() {
    // Exit machine0
    scheduler_->machine0()->Exit();

    {
        std::lock_guard<std::mutex> lock(init_mutex);
        DCHECK_EQ(this, __isolate);
        __isolate = nullptr;
    }
}

} // namespace lang

} // namespace mai
