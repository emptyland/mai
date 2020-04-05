#include "lang/coroutine.h"
#include "lang/isolate-inl.h"
#include "lang/scheduler.h"
#include "lang/heap.h"
#include "lang/stack.h"
#include "lang/stack-frame.h"
#include "lang/object-visitor.h"
#include "asm/utils.h"

namespace mai {

namespace lang {

#define MEMBER_OFFSET_OF(field) \
    arch::ObjectTemplate<Coroutine, int32_t>::OffsetOf(&Coroutine :: field)

const int32_t Coroutine::kOffsetOwns = MEMBER_OFFSET_OF(owner_);
const int32_t Coroutine::kOffsetState = MEMBER_OFFSET_OF(state_);
const int32_t Coroutine::kOffsetCaught = MEMBER_OFFSET_OF(caught_);
const int32_t Coroutine::kOffsetSysBP = MEMBER_OFFSET_OF(sys_bp_);
const int32_t Coroutine::kOffsetSysSP = MEMBER_OFFSET_OF(sys_sp_);
const int32_t Coroutine::kOffsetSysPC = MEMBER_OFFSET_OF(sys_pc_);
const int32_t Coroutine::kOffsetBP0 = MEMBER_OFFSET_OF(saved_state0_);
const int32_t Coroutine::kOffsetSP0 = kOffsetBP0 + kPointerSize;
const int32_t Coroutine::kOffsetPC0 = kOffsetSP0 + kPointerSize;
const int32_t Coroutine::kOffsetACC = kOffsetPC0 + kPointerSize;
const int32_t Coroutine::kOffsetFACC = kOffsetACC + kPointerSize;
const int32_t Coroutine::kOffsetBP1 = MEMBER_OFFSET_OF(saved_state1_);
const int32_t Coroutine::kOffsetSP1 = kOffsetBP1 + kPointerSize;
const int32_t Coroutine::kOffsetPC1 = kOffsetSP1 + kPointerSize;
const int32_t Coroutine::kOffsetStackGuard0 = MEMBER_OFFSET_OF(stack_guard0_);
const int32_t Coroutine::kOffsetStackGuard1 = MEMBER_OFFSET_OF(stack_guard1_);
const int32_t Coroutine::kOffsetHeapGuard0 = MEMBER_OFFSET_OF(heap_guard0_);
const int32_t Coroutine::kOffsetHeapGuard1 = MEMBER_OFFSET_OF(heap_guard1_);
const int32_t Coroutine::kOffsetYield = MEMBER_OFFSET_OF(yield_);
const int32_t Coroutine::kOffsetReentrant = MEMBER_OFFSET_OF(reentrant_);
const int32_t Coroutine::kOffsetEntry = MEMBER_OFFSET_OF(entry_);
const int32_t Coroutine::kOffsetException = MEMBER_OFFSET_OF(exception_);
const int32_t Coroutine::kOffsetGlobalGuard = MEMBER_OFFSET_OF(global_guard_);
const int32_t Coroutine::kOffsetGlobalLength = MEMBER_OFFSET_OF(global_length_);

void Coroutine::Reinitialize(uint64_t coid, Closure *entry, Stack *stack) {
    // queue header:
    next_ = this;
    prev_ = this;

    coid_      = coid;
    state_     = kDead;
    entry_     = entry;
    waitting_  = nullptr;
    owner_     = nullptr;
    exception_ = nullptr;
    caught_    = nullptr;
    yield_     = 0;
    reentrant_ = 0;

    heap_guard0_ = STATE->heap()->new_space()->original_chunk();
    heap_guard1_ = STATE->heap()->new_space()->original_limit();
    DCHECK_LT(heap_guard0_, heap_guard1_);
    global_guard_ = reinterpret_cast<Address>(STATE->global_space());
    global_length_ = STATE->global_space_length() * sizeof(*STATE->global_space());

    sys_bp_ = nullptr;
    sys_sp_ = nullptr;
    sys_pc_ = nullptr;
    saved_state0_[kSPIndex]   = bit_cast<intptr_t>(stack->stack_hi());
    saved_state0_[kBPIndex]   = bit_cast<intptr_t>(stack->stack_hi());
    saved_state0_[kACCIndex]  = 0;
    saved_state0_[kFACCIndex] = 0;

    // TODO:
    stack_ = DCHECK_NOTNULL(stack);
    stack_guard0_ = stack->guard0();
    stack_guard1_ = stack->guard1();
}

void Coroutine::Dispose() {
    DCHECK_EQ(kDead, state_);
    // Free stack
    if (stack_) {
        STATE->scheduler()->PurgreStack(stack_);
        stack_ = nullptr;
    }
}

void Coroutine::Uncaught(Throwable *thrown) {
    DCHECK(thrown->clazz()->IsBaseOf(STATE->builtin_type(kType_Throwable)))
        << "thrown is not Throwable: " << thrown->clazz()->name();

    bool should_print_stackstrace = true;
    if (thrown->Is<Panic>()) {
        Panic *error = static_cast<Panic *>(thrown);

        if (error->code() == Panic::kCrash) {
            // Fatal: Should shutdown scheduler
            owner_->set_state(Machine::kPanic);
            STATE->scheduler()->MarkShuttingDown();
            should_print_stackstrace = false;
        }
    } else {
        DCHECK(thrown->clazz()->IsSameOrBaseOf(STATE->metadata_space()->class_exception()));

        should_print_stackstrace = true;
    }

    owner_->IncrmentUncaughtCount();
    if (!should_print_stackstrace) {
        return;
    }

    if (owner_) {
        ::fprintf(stderr, "❌Uncaught: M:%d:C:%" PRId64 ":", owner_->id(), coid_);
    } else {
        ::fprintf(stderr, "❌Uncaught: M:NONE:C:%" PRId64 ":", coid_);
    }
    if (thrown->Is<Panic>()) {
        Panic *error = static_cast<Panic *>(thrown);
        ::fprintf(stderr, "😱[%s](%d) %s\n", error->clazz()->name(), error->code(),
                  error->quickly_message()->data());
        thrown->PrintStackstrace(stderr);
        return;
    }

    const Class *type = STATE->metadata_space()->class_exception();
    const Field *msg_field = STATE->metadata_space()->FindClassFieldOrNull(type, "message");
    
    const String *message = thrown->UnsafeGetField<const String *>(DCHECK_NOTNULL(msg_field));
    ::fprintf(stderr, "🤔[%s] %s\n", thrown->clazz()->name(), message->data());
    thrown->PrintStackstrace(stderr);

    const Field *cause_field = STATE->metadata_space()->FindClassFieldOrNull(type, "cause");
    Throwable *cause = thrown->UnsafeGetField<Throwable *>(DCHECK_NOTNULL(cause_field));
    while (cause) {
        message = cause->UnsafeGetField<const String *>(DCHECK_NOTNULL(msg_field));
        ::fprintf(stderr, "🔗Cause: [%s] %s\n", cause->clazz()->name(), message->data());
        cause->PrintStackstrace(stderr);
        cause = cause->UnsafeGetField<Throwable *>(DCHECK_NOTNULL(cause_field));
    }
}

void Coroutine::Suspend(intptr_t /*acc*/, double /*facc*/) {
    // TODO:
}


void VisitBytecodeFunctionStackFrame(Address frame_bp, RootVisitor *visitor) {
    Closure *callee = BytecodeStackFrame::GetCallee(frame_bp);
    int32_t pc = BytecodeStackFrame::GetPC(frame_bp);
    Function *fun = callee->function();
    int stack_ref_top = fun->FindStackRefTop(pc);

    // Scan stack local variables
    //printf("fun %s %08x\n", fun->name(), fun->stack_bitmap()[0]);
    visitor->VisitRootPointer(reinterpret_cast<Any **>(frame_bp + BytecodeStackFrame::kOffsetCallee));
    
    //printf("locals----------------------\n");
    int offset = BytecodeStackFrame::kOffsetHeaderSize;
    DCHECK_GE(stack_ref_top, offset);
    while (offset < stack_ref_top) {
        if (fun->TestStackBitmap(offset - BytecodeStackFrame::kOffsetHeaderSize)) {
            visitor->VisitRootPointer(reinterpret_cast<Any **>(frame_bp - offset - kPointerSize));
            offset += kPointerSize;
        } else {
            offset += sizeof(Span16);
        }
    }
    
    // Scan Arguments
    //printf("args----------------------\n");
    int32_t args_size = static_cast<int32_t>(fun->prototype()->GetParametersPlacedSize());
    offset = StackFrame::kBaseSize + args_size;
    for (uint32_t i = 0; i < fun->prototype()->parameter_size(); i++) {
        const Class *type = STATE->metadata_space()->type(fun->prototype()->parameter(i));
        offset -= RoundUp(type->reference_size(), kStackSizeGranularity);
        if (type->is_reference()) {
            visitor->VisitRootPointer(reinterpret_cast<Any **>(frame_bp + offset));
        }
    }
}

void Coroutine::VisitRoot(RootVisitor *visitor) {
    if (state() == kDead || state() == kPanic) {
        return;
    }
    if (entry_) {
        visitor->VisitRootPointer(reinterpret_cast<Any **>(&entry_));
    }
    if (exception_) {
        visitor->VisitRootPointer(reinterpret_cast<Any **>(&exception_));
    }
    if (entry_ && reentrant_ == 0) {
        DCHECK(entry_->is_mai_function());
        Function *fun = entry_->function();
        int32_t args_size = static_cast<int32_t>(fun->prototype()->GetParametersPlacedSize());
        Address frame_bp = stack()->stack_hi() - args_size;
        int offset = args_size;
        for (uint32_t i = 0; i < fun->prototype()->parameter_size(); i++) {
            const Class *type = STATE->metadata_space()->type(fun->prototype()->parameter(i));
            offset -= RoundUp(type->reference_size(), kStackSizeGranularity);
            if (type->is_reference()) {
                visitor->VisitRootPointer(reinterpret_cast<Any **>(frame_bp + offset));
            }
        }
        return;
    }
    
    Address frame_bp = state_ == kFallIn ? bp1() : bp0();
    while (frame_bp < stack()->stack_hi()) {
        StackFrame::Maker maker = StackFrame::GetMaker(frame_bp);
        switch (maker) {
            case StackFrame::kBytecode:
                VisitBytecodeFunctionStackFrame(frame_bp, visitor);
                break;
            case StackFrame::kStub:
                // Ignore
                break;
            case StackFrame::kTrampoline:
                // Finalize
                return;
            default:
                NOREACHED();
                break;
        }
        frame_bp = *reinterpret_cast<Address *>(frame_bp);
    }
}

} // namespace lang

} // namespace mai
