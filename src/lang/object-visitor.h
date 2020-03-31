#pragma once
#ifndef MAI_LANG_OBJECT_VISITORS_H_
#define MAI_LANG_OBJECT_VISITORS_H_

#include "base/base.h"

namespace mai {
    
namespace lang {
    
class Any;
    
class RootVisitor {
public:
    RootVisitor() {}
    virtual ~RootVisitor() {}
    virtual void VisitRootPointers(Any **begin, Any **end) = 0;
    virtual void VisitRootPointer(Any **p) { VisitRootPointers(p, p + 1); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(RootVisitor);
}; // class RootVisitor


class ObjectVisitor {
public:
    ObjectVisitor() {}
    virtual ~ObjectVisitor() {}
    virtual void VisitPointers(Any *host, Any **begin, Any **end) = 0;
    virtual void VisitPointer(Any *host, Any **p) { VisitPointers(host, p, p + 1); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectVisitor);
}; // class ObjectVisitor
    
void IterateObject(Any *host, ObjectVisitor *visitor);

} // namespace lang
    
} // namespace mai

#endif // MAI_LANG_OBJECT_VISITORS_H_

