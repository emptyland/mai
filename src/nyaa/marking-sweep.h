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
// This implements never move object.
class MarkingSweep final : public GarbageCollectionPolicy {
public:
    MarkingSweep(NyaaCore *core, Heap *heap);
    ~MarkingSweep();
    
    virtual void Run() override;
    
    DEF_VAL_GETTER(size_t, collected_bytes);
    DEF_VAL_GETTER(size_t, collected_objs);
    DEF_VAL_GETTER(double, time_cost);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(MarkingSweep);
private:
    class RootVisitorImpl;
    class HeapVisitorImpl;
    class ObjectVisitorImpl;
    class KzPoolVisitorImpl;

    std::stack<NyObject *> gray_;
}; // class MarkingSweep
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_MARKING_SWEEP_H_
