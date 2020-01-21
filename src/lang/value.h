#ifndef MAI_LANG_VALUE_H_
#define MAI_LANG_VALUE_H_

#include <stdint.h>
#include "glog/logging.h"

namespace mai {

namespace lang {

class Class;
class Type;


// All heap object's base class.
class Any {
public:
    bool is_forward() const { return forward_ & 1; }

    Class *clazz() const {
        DCHECK(!is_forward());
        return reinterpret_cast<Class *>(forward_);
    }

    Any *forward() const {
        DCHECK(is_forward());
        return reinterpret_cast<Any *>(forward_ & ~1);
    }
    
    uint32_t tags() const { return tags_; }

    Any(const Any &) = delete;
    void operator = (const Any &) = delete;
    
    void *operator new (size_t) = delete;
    void operator delete (void *) = delete;

protected:
    Any(Class *clazz, uint32_t tags)
        : forward_(reinterpret_cast<uintptr_t>(clazz))
        , tags_(tags) {}
    
    uintptr_t forward_; // Forward pointer or Class pointer
    uint32_t tags_; // Tags for heap object
}; // class Any


// The Array's header
template<class T>
class Array : public Any {
public:
    
private:
    Array(Class *clazz, uint32_t capacity, uint32_t length)
        : Any(clazz, 0)
        , capacity_(capacity)
        , length_(length) {}

    uint32_t capacity_;
    uint32_t length_;
    T elems_[0];
}; //template<class T> class Array


} // namespace lang

} // namespace mai

#endif // MAI_LANG_VALUE_H_
