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

thread_local TLSStorage *__tls_storage; // Global tls 

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

    if (auto err = heap_->Initialize(new_space_initial_size_); err.fail()) {
        return err;
    }

    if (auto err = metadata_space_->Initialize(); err.fail()) {
        return err;
    }
    
    for (int i = 0; i < kMax_Bytecodes; i++) {
        bytecode_handler_entries_[i] = DCHECK_NOTNULL(metadata_space_->bytecode_handlers()[i])->entry();
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
    , trampoline_suspend_point_(reinterpret_cast<Address>(BadSuspendPointDummy))
    , cached_number_slots_(new NumberValueSlot[NumberValueSlot::kMaxSlots]) {

    cached_number_slots_[NumberValueSlot::kIndexUnused].values = nullptr;
    // Bool only has true or false
    cached_number_slots_[NumberValueSlot::kIndex_bool].values = new std::atomic<AbstractValue *>[2];
    cached_number_slots_[NumberValueSlot::kIndex_bool].values[0].store(nullptr);
    cached_number_slots_[NumberValueSlot::kIndex_bool].values[1].store(nullptr);
    for (int i = NumberValueSlot::kIndexUnused + 2; i < NumberValueSlot::kMaxSlots; i++) {
        cached_number_slots_[i].values = new std::atomic<AbstractValue *>[kNumberOfCachedNumberValues];
        ::memset(cached_number_slots_[i].values, 0,
                 kNumberOfCachedNumberValues * sizeof(cached_number_slots_[i].values[0]));
    }
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

    // Init True and False values
    if (!cached_number_slot(kType_bool)->values[0].load()) {
        int8_t false_val = 0;
        AbstractValue *val = scheduler_->machine0()->NewNumber(kType_bool, &false_val, 1,
                                                               Heap::kOld);
        cached_number_slot(kType_bool)->values[0].store(val);

        int8_t true_val = 1;
        val = scheduler_->machine0()->NewNumber(kType_bool, &true_val, 1, Heap::kOld);
        cached_number_slot(kType_bool)->values[1].store(val);
    }
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
