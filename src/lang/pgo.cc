#include "lang/pgo.h"
#include "lang/compiler.h"
#include "asm/utils.h"

namespace mai {

namespace lang {

#define MEMBER_OFFSET_OF(owns, field) \
    arch::ObjectTemplate<owns, int32_t>::OffsetOf(&owns :: field)

/*static*/ const int32_t Tracer::kOffsetState = MEMBER_OFFSET_OF(Tracer, state_);
/*static*/ const int32_t Tracer::kOffsetPath = MEMBER_OFFSET_OF(Tracer, path_);
/*static*/ const int32_t Tracer::kOffsetPC = MEMBER_OFFSET_OF(Tracer, pc_);
/*static*/ const int32_t Tracer::kOffsetPathSize = MEMBER_OFFSET_OF(Tracer, path_size_);
/*static*/ const int32_t Tracer::kOffsetRepeatedCount = MEMBER_OFFSET_OF(Tracer, repeated_count_);
/*static*/ const int32_t Tracer::kOffsetPathCapacity = MEMBER_OFFSET_OF(Tracer, path_capacity_);
/*static*/ const int32_t Tracer::kOffsetLimitSize = MEMBER_OFFSET_OF(Tracer, limit_size_);
/*static*/ const int32_t Tracer::kOffsetGuardSlot = MEMBER_OFFSET_OF(Tracer, guard_slot_);

void Tracer::Start(Function *fun, int slot, int pc) {
    DCHECK(state_ == kIdle || state_ == kEnd || state_ == kError);
    guard_fun_  = fun;
    guard_slot_ = slot;
    guard_pc_   = pc;
    state_      = kPending;
    repeated_count_ = kRepeatedCount;

    if (path_ != dummy_path_) {
        delete[] path_;
        path_ = dummy_path_;
        
        DCHECK(pc_ != dummy_pc_);
        pc_ = dummy_pc_;
        path_capacity_ = kDummySize;
    }
    path_size_ = 0;
}

void Tracer::Abort() {
    DCHECK_EQ(state_, kPending);
    if (path_ != dummy_path_) {
        delete[] path_;
        DCHECK(pc_ != dummy_pc_);
        delete[] pc_;
        path_capacity_ = kDummySize;
    }
    path_size_ = 0;
    repeated_count_ = kRepeatedCount;
    state_ = kError;
}

void Tracer::GrowTracingPath() {
    size_t growing = std::max(path_capacity_ << 1, limit_size_);
    if (path_ != dummy_path_) {
        BytecodeInstruction *old_path = path_;
        path_ = new BytecodeInstruction[growing];
        ::memcpy(path_, old_path, path_size_ * sizeof(*path_));
        delete[] old_path;
        
        DCHECK(pc_ != dummy_pc_);
        uint32_t *old_pc = pc_;
        pc_ = new uint32_t[growing];
        ::memcpy(pc_, old_pc, path_size_ * sizeof(*pc_));
        delete[] old_pc;
    } else {
        path_ = new BytecodeInstruction[growing];
        ::memcpy(path_, dummy_path_, path_size_ * sizeof(*path_));
        pc_ = new uint32_t[growing];
        ::memcpy(pc_, dummy_pc_, path_size_ * sizeof(*pc_));
    }
    path_capacity_ = growing;
}

CompilationInfo *Tracer::MakeCompilationInfo(Machine *mach, Coroutine *owns) const {
    std::vector<BytecodeInstruction> path(path_size_);
    ::memcpy(&path[0], path_, path_size_ * sizeof(*path_));
    std::vector<uint32_t> pc(path_size_);
    ::memcpy(&pc[0], pc_, path_size_ * sizeof(*pc_));
    
    std::vector<CompilationInfo::InvokeInfo> invoke_info;
    invoke_info.push_back({guard_fun_, guard_slot_, 0});
    for (const auto &level : invoke_info_) {
        invoke_info.push_back({level.fun, level.slot, level.position});
    }
    
    return new CompilationInfo{
        mach,
        owns,
        guard_fun_,
        guard_slot_,
        guard_pc_,
        std::move(path),
        std::move(pc),
        std::move(invoke_info)
    };
}

} // namespace lang

} // namespace mai
