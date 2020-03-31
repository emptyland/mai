#pragma once
#ifndef MAI_LANG_MARKING_SWEEP_H_
#define MAI_LANG_MARKING_SWEEP_H_

#include "lang/garbage-collector.h"
#include <stack>

namespace mai {

namespace lang {

class Any;

class MarkingSweep final : public GarbageCollectionPolicy {
public:
    MarkingSweep(Isolate *isolate, Heap *heap): GarbageCollectionPolicy(isolate, heap) {}
    ~MarkingSweep() override = default;
    
    DEF_VAL_PROP_RW(bool, full);

    void Run(base::AbstractPrinter *logger) override;
private:
    class RootVisitorImpl;
    class ObjectVisitorImpl;
    class WeakVisitorImpl;

    bool full_ = false; // full gc?
    std::stack<Any *> gray_;
}; // class MarkingSweep

} // namespace lang

} // namespace mai

#endif // MAI_LANG_MARKING_SWEEP_H_
