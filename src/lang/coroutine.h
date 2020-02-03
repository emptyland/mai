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

class Coroutine final {
public:
    enum State {
        kDead,
        kIdle,
        kWaitting,
        kRunnable,
        kRunning,
        kFallIn,
    }; // enum State
    
    static const int32_t kOffsetSysBP;
    static const int32_t kOffsetSysSP;
    static const int32_t kOffsetSysPC;
    static const int32_t kOffsetBP;
    static const int32_t kOffsetSP;
    static const int32_t kOffsetPC;
    static const int32_t kOffsetYield;
    static const int32_t kOffsetReentrant;
    static const int32_t kOffsetEntry;

    Coroutine(uint64_t coid, Closure *entry, Stack *stack) {
        Reinitialize(coid, entry, stack);
    }
    ~Coroutine() { Dispose(); }

    void Reinitialize(uint64_t coid, Closure *entry, Stack *stack);
    
    void Dispose();
    
    DEF_VAL_GETTER(State, state);
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
    DEF_VAL_GETTER(Address, bp);
    DEF_VAL_GETTER(Address, sp);
    DEF_VAL_GETTER(uint32_t, pc);
    DEF_VAL_GETTER(Address, sys_bp);
    DEF_VAL_GETTER(Address, sys_sp);
    DEF_VAL_GETTER(Address, sys_pc);
    
    void CopyArgv(void *data, size_t n) {
        DCHECK_EQ(reentrant_, 0);
        DCHECK_EQ(0, n % kStackAligmentSize);
        sp_ -= n;
        ::memcpy(sp_, data, n);
    }
    
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
    Address bp_; // Mai frame pointer
    Address sp_; // Mai stack pointer
    uint32_t pc_; // Mai program counter
    intptr_t acc_;  // Saved ACC
    double facc_; // Saved FACC
    Address stack_guard0_; // guard0 of stack
    Address stack_guard1_; // guard1 of stack
    Stack *stack_; // Coroutine owned calling stack
}; //class Coroutine

} // namespace lang

} // namespace mai


#endif // MAI_LANG_COROUTINE_H_
