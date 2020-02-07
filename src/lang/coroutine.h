#pragma once
#ifndef MAI_LANG_COROUTINE_H_
#define MAI_LANG_COROUTINE_H_

#include "lang/isolate-inl.h"
#include "lang/value-inl.h"
#include "lang/mm.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

class Machine;
class Stack;

struct CaughtNode {
    CaughtNode *next;
    Address pc;
    Address bp;
    Address sp;
}; // struct CaughtNode

static constexpr int32_t kOffsetCaught_Next = offsetof(CaughtNode, next);
static constexpr int32_t kOffsetCaught_PC = offsetof(CaughtNode, pc);
static constexpr int32_t kOffsetCaught_BP = offsetof(CaughtNode, bp);
static constexpr int32_t kOffsetCaught_SP = offsetof(CaughtNode, sp);

class Coroutine final {
public:
    enum State: int32_t {
        kDead,
        kIdle,
        kWaitting,
        kRunnable, // Can runnable
        kRunning, // Is runing
        kFallIn, // In C++ function calling
        kInterrupted, // Yied
        kPanic, // Uncaught exception, unable recover
    }; // enum State
    
    static const int32_t kOffsetState;
    static const int32_t kOffsetACC;
    static const int32_t kOffsetFACC;
    static const int32_t kOffsetCaught;
    static const int32_t kOffsetSysBP;
    static const int32_t kOffsetSysSP;
    static const int32_t kOffsetSysPC;
    static const int32_t kOffsetBP;
    static const int32_t kOffsetSP;
    static const int32_t kOffsetPC;
    static const int32_t kOffsetBP1;
    static const int32_t kOffsetSP1;
    static const int32_t kOffsetPC1;
    static const int32_t kOffsetYield;
    static const int32_t kOffsetReentrant;
    static const int32_t kOffsetEntry;
    static const int32_t kOffsetException;
  
    enum SavedStateIndex {
        kBPIndex, // Mai frame pointer
        kSPIndex, // Mai stack pointer
        kPCIndex, // Mai program counter
        kACCIndex, // Saved ACC
        kFACCIndex, // Saved ACC
        kMaxSavedState,
    };

    Coroutine(uint64_t coid, Closure *entry, Stack *stack) {
        Reinitialize(coid, entry, stack);
    }
    ~Coroutine() { Dispose(); }

    void Reinitialize(uint64_t coid, Closure *entry, Stack *stack);
    
    void Dispose();
    
    DEF_VAL_PROP_RW(State, state);
    DEF_VAL_GETTER(uint64_t, coid);
    DEF_PTR_PROP_RW(Machine, owner);
    DEF_PTR_PROP_RW(Closure, entry);
    DEF_VAL_GETTER(Address, heap_guard0);
    DEF_VAL_GETTER(Address, heap_guard1);
    DEF_VAL_GETTER(uint32_t, yield);
    DEF_VAL_PROP_RW(uint32_t, reentrant);
    DEF_VAL_GETTER(Address, stack_guard0);
    DEF_VAL_GETTER(Address, stack_guard1);
    DEF_PTR_GETTER(Stack, stack);
    DEF_VAL_GETTER(Address, sys_bp);
    DEF_VAL_GETTER(Address, sys_sp);
    DEF_VAL_GETTER(Address, sys_pc);
    
    Address sp() const { return bit_cast<Address>(saved_state0_[kSPIndex]); }
    Address bp() const { return bit_cast<Address>(saved_state0_[kBPIndex]); }
    
    void SwitchState(State expected, State want) {
        DCHECK_EQ(state_, expected);
        if (state_ == expected) {
            state_ = want;
        }
    }
    
    void CopyArgv(void *data, size_t n) {
        DCHECK_EQ(reentrant_, 0);
        DCHECK_EQ(0, n % kStackAligmentSize);
        saved_state0_[kSPIndex] -= n;
        ::memcpy(sp(), data, n);
    }
    
    void Uncaught(Any *expection);
    void Suspend(intptr_t acc, double facc);
    
    static Coroutine *NewDummy() {
        void *chunk = ::malloc(sizeof(Coroutine *) * 2); // next_ and prev_
        Coroutine *dummy = static_cast<Coroutine *>(chunk);
        dummy->next_ = dummy;
        dummy->prev_ = dummy;
        return dummy;
    }

    static void DeleteDummy(Coroutine *dummy) { ::free(dummy); }
    
    friend class Machine;
    friend class Scheduler;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Coroutine);
private:
    DEF_PTR_GETTER(Coroutine, next);
    DEF_PTR_GETTER(Coroutine, prev);
    
    QUEUE_HEADER(Coroutine); // NOTICE: MUST BE HEADER!!
    uint64_t coid_; // Coroutine ID
    Machine *owner_; // Owner machine
    Closure *entry_; // [strong ref] Entry function
    Address heap_guard0_; // New space address guard0 for write barrier
    Address heap_guard1_; // New space address guard1 for write barrier
    State state_; // Coroutine current state
    Any *exception_; // [strong ref] Native function thrown exception
    CaughtNode *caught_; // Exception hook for exception caught
    uint32_t yield_; // Yield requests, if yield > 0, need yield
    uint32_t reentrant_; // Number of reentrant times
    Address sys_bp_; // System frame pointer
    Address sys_sp_; // System stack pointer
    Address sys_pc_; // System program counter
    uintptr_t saved_state0_[kMaxSavedState]; // Main mai-execution state
    uintptr_t saved_state1_[kMaxSavedState]; // Secondare saved state
    Address stack_guard0_; // guard0 of stack
    Address stack_guard1_; // guard1 of stack
    Stack *stack_; // Coroutine owned calling stack
}; //class Coroutine

} // namespace lang

} // namespace mai


#endif // MAI_LANG_COROUTINE_H_
