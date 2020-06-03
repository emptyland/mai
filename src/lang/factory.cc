#include "lang/factory.h"
#include "lang/isolate-inl.h"
#include "lang/heap.h"
#include "lang/scheduler.h"
#include "lang/object-visitor.h"

namespace mai {

namespace lang {

Factory::Factory()
    : cached_number_slots_(new NumberValueSlot[NumberValueSlot::kMaxSlots]) {

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

Factory::~Factory() {
    for (int i = NumberValueSlot::kIndexUnused + 2; i < NumberValueSlot::kMaxSlots; i++) {
        delete[] cached_number_slots_[i].values;
    }
    delete[] cached_number_slots_[NumberValueSlot::kIndex_bool].values;
}

void Factory::Initialize() {
    // Init heap values:
    empty_string_ = Machine::This()->NewUtf8String("", 0, Heap::kOld);
    true_string_ = Machine::This()->NewUtf8String(STR_WITH_LEN("true"), Heap::kOld);
    false_string_ = Machine::This()->NewUtf8String(STR_WITH_LEN("false"), Heap::kOld);
    oom_text_ = Machine::This()->NewUtf8String(STR_WITH_LEN("Doki Doki Panic: OOM"), Heap::kOld);
    oom_panic_ = static_cast<Panic *>(Machine::This()->NewPanic(Panic::kCrash, oom_text_,
                                                                Heap::kOld));
    out_of_bound_error_text_ =
        Machine::This()->NewUtf8String(STR_WITH_LEN("Out of bound in array"), Heap::kOld);
    arithmetic_text_ =
        Machine::This()->NewUtf8String(STR_WITH_LEN("Arithmetic fatal"), Heap::kOld);
    stack_overflow_error_text_ =
        Machine::This()->NewUtf8String(STR_WITH_LEN("Doki Doki Panic: Stackoverflow"), Heap::kOld);
    nil_error_text_ = Machine::This()->NewUtf8String(STR_WITH_LEN("Attempt nil object"), Heap::kOld);
    dup_close_chan_error_text_ =
        Machine::This()->NewUtf8String(STR_WITH_LEN("Duplicated close channel"), Heap::kOld);
    
    // Init True and False values
    if (!cached_number_slot(kType_bool)->values[0].load()) {
        int8_t false_val = 0;
        Machine *m0 = STATE->scheduler()->machine0();
        AbstractValue *val = m0->NewNumber(kType_bool, &false_val, 1, Heap::kOld);
        cached_number_slot(kType_bool)->values[0].store(val);

        int8_t true_val = 1;
        val = m0->NewNumber(kType_bool, &true_val, 1, Heap::kOld);
        cached_number_slot(kType_bool)->values[1].store(val);
    }
}

void Factory::VisitRoot(RootVisitor *visitor) {
#define DEFINE_VISIT(name, ...) visitor->VisitRootPointer(reinterpret_cast<Any **>(&name##_));
    DECLARE_FACTORY_VALUES(DEFINE_VISIT)
#undef DEFINE_VISIT
    
    for (int i = 1/*skip void type*/; i < NumberValueSlot::kMaxSlots; i++) {
        NumberValueSlot *slot = cached_number_slot(i);
        Any **begin = reinterpret_cast<Any **>(slot->values);
        Any **end = reinterpret_cast<Any **>(slot->values + (i == 1 ? 2 : kNumberOfCachedNumberValues));
        visitor->VisitRootPointers(begin, end);
    }
}

} // namespace lang

} // namespace mai
