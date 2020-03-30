#ifndef MAI_LANG_SCAVENGER_H_
#define MAI_LANG_SCAVENGER_H_

#include "lang/garbage-collector.h"
#include <deque>

namespace mai {

namespace lang {

class Any;

class Scavenger final : public GarbageCollectionPolicy {
public:
    Scavenger(Isolate *isolate, Heap *heap): GarbageCollectionPolicy(isolate, heap) {}
    ~Scavenger() override;

    void Run() override;
private:
    class RootVisitorImpl;
    class ObjectVisitorImpl;

    bool force_promote_ = false;
    Address promote_level_ = nullptr;
    std::deque<Any *> promoted_obs_;
}; // class Scavenger

} // namespace lang

} // namespace mai

#endif // MAI_LANG_SCAVENGER_H_
