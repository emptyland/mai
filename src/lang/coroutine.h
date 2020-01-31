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

    Coroutine(uint64_t coid, Closure *entry, Stack *stack) {
        Reinitialize(coid, entry, stack);
    }
    ~Coroutine() { Dispose(); }
    
    void Reinitialize(uint64_t coid, Closure *entry, Stack *stack);
    
    void Dispose();
    
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
    Address new_guard0_; // New space address guard0 for write barrier
    Address new_guard1_; // New space address guard1 for write barrier
    State state_; // Coroutine current state
    Any *exception_; // [strong ref] Native function thrown exception
    CaughtNode *caught_; // Exception hook for exception caught
    uint32_t yield_; // Yield requests, if yield > 0, need yield
    Address sys_bp_; // System frame pointer
    Address sys_sp_; // System stack pointer
    Address sys_pc_; // System program counter
    Address bp_; // Mai frame pointer
    Address sp_; // Mai stack pointer
    Address pc_; // Mai program counter
    intptr_t acc_;  // Saved ACC
    double facc_; // Saved FACC
    Address stack_guard0_; // guard0 of stack
    Address stack_guard1_; // guard1 of stack
    Stack *stack_; // Coroutine owned calling stack
}; //class Coroutine

} // namespace lang

} // namespace mai


#endif // MAI_LANG_COROUTINE_H_
