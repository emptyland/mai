#ifndef MAI_LANG_SPACE_H_
#define MAI_LANG_SPACE_H_

#include "lang/page.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

class NewSpace;
class OldSpace;
class LargeSpace;
class SemiSpace;

class Space {
public:
    DEF_VAL_GETTER(SpaceKind, kind);
    
protected:
    Space(SpaceKind kind, Allocator *lla)
        : kind_(kind)
        , lla_(lla) {}

    SpaceKind kind_;
    Allocator *lla_;
}; // class Space


class NewSpace : public Space {
public:
    
private:
    
}; // class NewSpace


class SemiSpace final {
public:
    
private:
    
}; // class SemiSpace


class OldSpace final : public Space {
public:
    OldSpace(Allocator *lla)
        : Space(kOldSpace, lla)
        , dummy_(new PageHeader(kOldSpace))
        , free_(new PageHeader(kOldSpace)) {
    }
    
    ~OldSpace() {
        delete free_;
        delete dummy_;
    }
    
private:
    PageHeader *dummy_; // Page double-linked list dummy
    PageHeader *free_;  // Free page double-linked list dummy
    int allocated_pages_; // Number of allocated pages
    size_t usage_size_; // Allocated bytes size
}; // class OldSpace


class LargeSpace : public Space {
public:
    
private:
    
}; // class OldSpace

} // namespace lang

} // namespace mai

#endif // MAI_LANG_SPACE_H_
