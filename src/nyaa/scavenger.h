#ifndef MAI_NYAA_SCAVENGER_H_
#define MAI_NYAA_SCAVENGER_H_

#include "base/base.h"
#include "glog/logging.h"

namespace mai {

namespace nyaa {
    
class Heap;
class Object;
class NyaaCore;
    
class Scavenger final {
public:
    Scavenger(NyaaCore *core, Heap *heap)
        : core_(core)
        , heap_(heap) {}
    ~Scavenger() {}
    
    void Run();

    DISALLOW_IMPLICIT_CONSTRUCTORS(Scavenger);
private:
    class RootVisitorImpl;
    class HeapVisitorImpl;
    class ObjectVisitorImpl;
    
    Object *MoveObject(Object *addr, size_t size);
    
    NyaaCore *const core_;
    Heap *const heap_;
}; // class Scavenger
    

} // namespace nyaa

} // namespace mai


#endif // MAI_NYAA_SCAVENGER_H_
