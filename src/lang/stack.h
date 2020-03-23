#pragma once
#ifndef MAI_LANG_STACK_H_
#define MAI_LANG_STACK_H_

#include "base/queue-macros.h"
#include "lang/mm.h"

namespace mai {

namespace lang {

class Stack final {
public:
    static constexpr size_t kGuard0Size = kStackAligmentSize * 50 - sizeof(size_t) - kPointerSize * 4;
    static constexpr size_t kGuard1Size = kStackAligmentSize * 2;

    void Reinitialize() {
        next_ = this;
        prev_ = this;
    #if defined(DEBUG) || defined(_DEBUG)
        ::memset(guard0_, 0xcc, kGuard0Size);
        ::memset(guard1() - kGuard1Size, 0xcc, kGuard1Size);
    #endif // defined(DEBUG) || defined(_DEBUG)
        DbgFillInitZag(stack_lo_, GetAvailableSize());
    }

    Address guard0() { return &guard0_[0] + kGuard0Size; }
    Address guard1() { return reinterpret_cast<Address>(this) + size_; }

    size_t GetAvailableSize() const { return stack_hi_ - stack_lo_; }

    DEF_VAL_GETTER(size_t, size);
    DEF_VAL_GETTER(Address, stack_lo);
    DEF_VAL_GETTER(Address, stack_hi);

    static Stack *New(size_t available_size, Allocator *lla) {
        const size_t total_size = available_size + sizeof(Stack) + kGuard1Size + kStackAligmentSize * 2;
        const size_t request_size = RoundUp(total_size, lla->granularity());
        void *chunk = lla->Allocate(request_size);
        if (!chunk) {
            return nullptr;
        }
        return new (chunk) Stack(request_size);
    }

    void Dispose(Allocator *lla) { lla->Free(this, size_); }

    static Stack *NewDummy() {
        Stack *dummy = static_cast<Stack *>(::malloc(kPointerSize * 2));
        dummy->next_ = dummy;
        dummy->prev_ = dummy;
        return dummy;
    }

    static void DeleteDummy(Stack *dummy) { ::free(dummy); }

    friend class Scheduler;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Stack);
private:
    Stack(size_t size): size_(size) {
        stack_lo_ = RoundUp(&data_[0], kStackAligmentSize);
        stack_hi_ = RoundDown(reinterpret_cast<Address>(this) + size - kGuard1Size, kStackAligmentSize);
        Reinitialize();
    }

    DEF_PTR_GETTER(Stack, next);
    DEF_PTR_GETTER(Stack, prev);

    QUEUE_HEADER(Stack);
    size_t size_; // Total size of stack
    Address stack_lo_; // Low address of stack
    Address stack_hi_; // High address of stack
    uint8_t guard0_[kGuard0Size]; // Guard of low address
    uint8_t data_[0]; // data
}; // class Stack

} // namespace lang

} // namespace mai


#endif // MAI_LANG_STACK_H_
