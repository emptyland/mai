#ifndef MAI_NYAA_SCAVENGER_H_
#define MAI_NYAA_SCAVENGER_H_

#include "nyaa/memory.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {

namespace nyaa {
    
class Heap;
class Object;
class NyaaCore;
    
// Fast gc implements, only for new space.
class Scavenger final : public GarbageCollectionPolicy {
public:
    Scavenger(NyaaCore *core, Heap *heap);
    virtual ~Scavenger() override {}
    
    DEF_VAL_PROP_RW(bool, force_upgrade);
    
    virtual void Run() override;

    DISALLOW_IMPLICIT_CONSTRUCTORS(Scavenger);
private:
    class RootVisitorImpl;
    class HeapVisitorImpl;
    class ObjectVisitorImpl;
    class WeakVisitorImpl;

    bool force_upgrade_ = false;
    Address upgrade_level_ = nullptr;
}; // class Scavenger
    

} // namespace nyaa

} // namespace mai


#endif // MAI_NYAA_SCAVENGER_H_
