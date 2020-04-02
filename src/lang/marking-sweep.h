#pragma once
#ifndef MAI_LANG_MARKING_SWEEP_H_
#define MAI_LANG_MARKING_SWEEP_H_

#include "lang/garbage-collector.h"
#include <stack>

namespace mai {

namespace lang {

class Any;

class MarkingSweep final : public PartialMarkingPolicy {
public:
    MarkingSweep(Isolate *isolate, Heap *heap): PartialMarkingPolicy(isolate, heap) {}
    ~MarkingSweep() override = default;

    void Run(base::AbstractPrinter *logger) override;
}; // class MarkingSweep

} // namespace lang

} // namespace mai

#endif // MAI_LANG_MARKING_SWEEP_H_
