#ifndef MAI_BASE_REFERENCE_COUNT_H_
#define MAI_BASE_REFERENCE_COUNT_H_

#include "base/base.h"
#include "glog/logging.h"
#include <atomic>

namespace mai {
    
namespace base {
    
template<class T>
class intrusive_ptr final {
public:
    intrusive_ptr() : intrusive_ptr(nullptr) {}
    
    intrusive_ptr(T *naked)
        : naked_(naked) {
        if (naked_) { naked_->AddRef(); }
    }
    
    intrusive_ptr(const intrusive_ptr &other) : intrusive_ptr(other.naked_) {}
    
    intrusive_ptr(intrusive_ptr &&other)
        : naked_(other.naked_) {
        other.naked_ = nullptr;
    }
    
    ~intrusive_ptr() {
        if (naked_) { naked_->ReleaseRef(); }
    }
    
    void reset(T *naked) {
        if (naked_) { naked_->ReleaseRef(); }
        naked_ = naked;
        if (naked_) { naked_->AddRef(); }
    }
    
    void operator = (const intrusive_ptr &other) { reset(other.naked_); }
    
    void operator = (intrusive_ptr &&other) {
        naked_ = other.naked_;
        other.naked_ = nullptr;
    }
    
    T *operator -> () const { return DCHECK_NOTNULL(naked_); }

    bool operator ! () const { return !naked_; }
    
    T &operator * () const { return *naked_; }
    
    T *get() const { return naked_; }
    
    bool is_null() const { return naked_ == nullptr; }
    
    template<class S>
    static intrusive_ptr Make(S *naked) { return intrusive_ptr<T>(naked); }
    
    static intrusive_ptr Null() { return intrusive_ptr<T>(nullptr); }
    
private:
    T *naked_;
}; // class intrusive_ptr
    

template<class T>
class atomic_intrusive_ptr final {
public:
    atomic_intrusive_ptr() : atomic_intrusive_ptr(nullptr) {}
    
    atomic_intrusive_ptr(T *naked)
        : naked_(naked) {
        if (naked_) { naked_->AddRef(); }
    }
    
private:
    std::atomic<T *> naked_;
}; // class
    

template<class T>
inline bool operator == (const intrusive_ptr<T> &lhs, const intrusive_ptr<T> &rhs) {
    return lhs.get() == rhs.get();
}
    
template<class T>
inline intrusive_ptr<T> MakeRef(T *naked) { return intrusive_ptr<T>(naked); }
    
class ReferenceCountable {
public:
    ReferenceCountable() : ref_count_(0) {}
    virtual ~ReferenceCountable() {}
    
    void AddRef() const {
        ref_count_.fetch_add(1, std::memory_order_acq_rel);
    }
    
    void ReleaseRef() const {
        DCHECK_GT(ref_count(), 0);
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete this;
        }
    }
    
    int ref_count() const { return ref_count_.load(std::memory_order_acquire); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ReferenceCountable);
private:
    mutable std::atomic<int> ref_count_;
}; // class ReferenceCountable
    
    
template<class T>
class ReferenceCounted {
public:
    ReferenceCounted() : ref_count_(0) {}
    ~ReferenceCounted() {}
    
    void AddRef() const {
        ref_count_.fetch_add(1, std::memory_order_acq_rel);
    }
    
    void ReleaseRef() const {
        DCHECK_GT(ref_count(), 0);
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete static_cast<const T*>(this);
        }
    }
    
    int ref_count() const { return ref_count_.load(std::memory_order_acquire); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ReferenceCounted);
private:
    mutable std::atomic<int> ref_count_;
}; // class ReferenceCounted
    
} // namespace base
    
} // namespace mai


#endif // MAI_BASE_REFERENCE_COUNT_H_
