#pragma once
#ifndef MAI_LANG_MARKING_COMPACT_H_
#define MAI_LANG_MARKING_COMPACT_H_

#include "lang/garbage-collector.h"
#include <vector>
#include <stack>

namespace mai {

namespace lang {

class Any;
class Page;

class MarkingCompact final : public PartialMarkingPolicy {
public:
    MarkingCompact(Isolate *isolate, Heap *heap): PartialMarkingPolicy(isolate, heap) {}
    ~MarkingCompact() override = default;

    void Run(base::AbstractPrinter *logger) override;
private:
    class RootVisitorImpl;
    class ObjectVisitorImpl;

    void CompactObject(Any **address);

    std::vector<Page *> old_survivors_;
    size_t old_used_ = 0;
    std::vector<Page *> code_survivors_;
    size_t code_used_ = 0;
}; // class MarkingSweep


} // namespace lang

} // namespace mai


#endif // MAI_LANG_MARKING_COMPACT_H_
