#pragma once
#ifndef MAI_LANG_FACTORY_H_
#define MAI_LANG_FACTORY_H_

#include "lang/value-inl.h"
#include "mai/error.h"
#include <atomic>

namespace mai {

namespace lang {

#define DECLARE_FACTORY_VALUES(V) \
    V(empty_string, String) \
    V(true_string, String) \
    V(false_string, String) \
    V(oom_text, String) \
    V(nil_error_text, String) \
    V(stack_overflow_error_text, String) \
    V(out_of_bound_error_text, String) \
    V(arithmetic_text, String) \
    V(dup_close_chan_error_text, String) \
    V(new_coroutine_error_text, String) \
    V(oom_panic, Panic)

class RootVisitor;

struct NumberValueSlot {
    enum Index {
        kIndexUnused, // Skip type: void
    #define DEFINE_ENUM(name, ...) kIndex_##name,
        DECLARE_PRIMITIVE_TYPES(DEFINE_ENUM)
    #undef DEFINE_ENUM
        kMaxSlots,
    }; // enum Index

    static constexpr uintptr_t kPendingMask = 1;
    static constexpr uintptr_t kCreatedMask = ~kPendingMask;

    std::atomic<AbstractValue *> *values; // [strong ref]
}; // struct NumberValueSlot

class Factory final {
public:
    static constexpr int64_t kNumberOfCachedNumberValues = 128;
    
    Factory();
    ~Factory();

    void Initialize();
    
    void VisitRoot(RootVisitor *visitor);
    
    NumberValueSlot *cached_number_slot(int index) {
        DCHECK_GT(index, NumberValueSlot::kIndexUnused);
        DCHECK_LT(index, NumberValueSlot::kMaxSlots);
        return &cached_number_slots_[index];
    }
    
    std::atomic<AbstractValue *> *cached_number_value(int slot, int64_t index) {
        DCHECK_GE(index, 0);
        DCHECK_LT(index, kNumberOfCachedNumberValues);
        return cached_number_slot(slot)->values + index;
    }
    
#define DEFINE_VALUE(name, type) type *name() const { return DCHECK_NOTNULL(name##_); }
    DECLARE_FACTORY_VALUES(DEFINE_VALUE)
#undef  DEFINE_VALUE

private:
#define DEFINE_VALUE(name, type) type *name##_ = nullptr;
    DECLARE_FACTORY_VALUES(DEFINE_VALUE) // [strong ref]
#undef  DEFINE_VALUE
    
    // Cached numbers for ValueOf() functions
    std::unique_ptr<NumberValueSlot[]> cached_number_slots_; // [nested strong ref]
}; // class Factory

} // namespace lang

} // namespace mai


#endif // MAI_LANG_FACTORY_H_
