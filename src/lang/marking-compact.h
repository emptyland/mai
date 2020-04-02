#pragma once
#ifndef MAI_LANG_MARKING_COMPACT_H_
#define MAI_LANG_MARKING_COMPACT_H_

#include "lang/garbage-collector.h"
#include <stack>

namespace mai {

namespace lang {

class Any;

class MarkingCompact final : public PartialMarkingPolicy {
public:
    MarkingCompact(Isolate *isolate, Heap *heap): PartialMarkingPolicy(isolate, heap) {}
    ~MarkingCompact() override = default;
    
    void Run(base::AbstractPrinter *logger) override;
private:
    class WeakVisitorImpl;
}; // class MarkingSweep


} // namespace lang

} // namespace mai


#endif // MAI_LANG_MARKING_COMPACT_H_
