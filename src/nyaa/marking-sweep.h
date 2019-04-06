#ifndef MAI_NYAA_MARKING_SWEEP_H_
#define MAI_NYAA_MARKING_SWEEP_H_

#include "nyaa/memory.h"
#include "base/base.h"
#include <stack>

namespace mai {

namespace nyaa {

class Heap;
class Object;
class NyObject;
class NyaaCore;
    
// Normal spped gc implements;
// This implements never move old space object(s).
class MarkingSweep final : public GarbageCollectionPolicy {
public:
    MarkingSweep(NyaaCore *core, Heap *heap);
    virtual ~MarkingSweep() override;
    
    virtual void Run() override;

    DEF_VAL_PROP_RW(bool, full);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(MarkingSweep);
private:
    class RootVisitorImpl;
    class HeapVisitorImpl;
    class ObjectVisitorImpl;
    class WeakVisitorImpl;

    bool full_ = false; // full gc?
    std::stack<NyObject *> gray_;
}; // class MarkingSweep
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_MARKING_SWEEP_H_
