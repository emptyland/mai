#include "lang/pgo.h"
#include "asm/utils.h"

namespace mai {

namespace lang {

#define MEMBER_OFFSET_OF(owns, field) \
    arch::ObjectTemplate<owns, int32_t>::OffsetOf(&owns :: field)

/*static*/ const int32_t Tracer::kOffsetState = MEMBER_OFFSET_OF(Tracer, state_);
/*static*/ const int32_t Tracer::kOffsetPath = MEMBER_OFFSET_OF(Tracer, path_);
/*static*/ const int32_t Tracer::kOffsetPathSize = MEMBER_OFFSET_OF(Tracer, path_size_);
/*static*/ const int32_t Tracer::kOffsetPathCapacity = MEMBER_OFFSET_OF(Tracer, path_capacity_);
/*static*/ const int32_t Tracer::kOffsetLimitSize = MEMBER_OFFSET_OF(Tracer, limit_size_);
/*static*/ const int32_t Tracer::kOffsetGuardSlot = MEMBER_OFFSET_OF(Tracer, guard_slot_);

void Tracer::Start(Function *fun, int slot, int pc) {
    DCHECK(state_ == kIdle || state_ == kEnd || state_ == kError);
    guard_fun_  = fun;
    guard_slot_ = slot;
    guard_pc_   = pc;
    state_      = kPending;

    if (path_ != dummy_) {
        delete[] path_;
        path_ = dummy_;
        path_capacity_ = kDummySize;
    }
    path_size_ = 0;
}

void Tracer::GrowTracingPath() {
    size_t growing = std::max(path_capacity_ << 1, limit_size_);
    if (path_ != dummy_) {
        BytecodeInstruction *old = path_;
        path_ = new BytecodeInstruction[growing];
        ::memcpy(path_, old, path_size_ * sizeof(*path_));
        delete[] old;
    } else {
        path_ = new BytecodeInstruction[growing];
        ::memcpy(path_, dummy_, path_size_ * sizeof(*path_));
    }
    path_capacity_ = growing;
}

} // namespace lang

} // namespace mai
