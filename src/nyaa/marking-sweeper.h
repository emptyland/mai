#ifndef MAI_NYAA_MARKING_SWEEPER_H_
#define MAI_NYAA_MARKING_SWEEPER_H_

#include "base/base.h"
#include "glog/logging.h"
#include <stack>

namespace mai {

namespace nyaa {

class Heap;
class Object;
class NyObject;
class NyaaCore;
    
// Normal spped gc implements;
// This implements never move object.
class MarkingSweeper final {
public:
    MarkingSweeper(NyaaCore *core, Heap *heap);
    ~MarkingSweeper();
    
    void Run();
    
    DEF_VAL_GETTER(size_t, collected_bytes);
    DEF_VAL_GETTER(size_t, collected_objs);
    DEF_VAL_GETTER(double, time_cost);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(MarkingSweeper);
private:
    class RootVisitorImpl;
    class HeapVisitorImpl;
    class ObjectVisitorImpl;
    class KzPoolVisitorImpl;
    
    NyaaCore *const core_;
    Heap *const heap_;
    std::stack<NyObject *> gray_;
    
    size_t collected_bytes_ = 0; // size of freed memory
    size_t collected_objs_ = 0; // number of freed object
    double time_cost_ = 0;
};
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_MARKING_SWEEPER_H_
