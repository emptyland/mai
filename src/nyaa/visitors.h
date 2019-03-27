#ifndef MAI_NYAA_VISITORS_H_
#define MAI_NYAA_VISITORS_H_

#include "base/base.h"

namespace mai {
    
namespace nyaa {
    
class Object;
    
class RootVisitor {
public:
    RootVisitor() {}
    virtual ~RootVisitor() {}
    
    virtual void VisitRootPointers(Object **begin, Object **end) = 0;
    
    virtual void VisitRootPointer(Object **p) { VisitRootPointers(p, p + 1); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(RootVisitor);
}; // class RootVisitor


class ObjectVisitor {
public:
    ObjectVisitor() {}
    virtual ~ObjectVisitor() {}
    
    virtual void VisitPointers(Object *host, Object **begin, Object **end) = 0;
    
    virtual void VisitPointer(Object *host, Object **p) { VisitPointers(host, p, p + 1); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectVisitor);
}; // class ObjectVisitor

} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_VISITORS_H_
