#ifndef MAI_BASE_REFERENCE_COUNT_H_
#define MAI_BASE_REFERENCE_COUNT_H_

#include "base/base.h"
#include "glog/logging.h"
#include <atomic>

namespace mai {
    
namespace base {
    
template<class T>
class Handle final {
public:
    Handle() : Handle(nullptr) {}
    
    Handle(T *naked)
        : naked_(naked) {
        if (naked_) { naked_->AddRef(); }
    }
    
    Handle(const Handle &other) : Handle(other.naked_) {}
    
    Handle(Handle &&other)
        : naked_(other.naked_) {
        other.naked_ = nullptr;
    }
    
    ~Handle() {
        if (naked_) { naked_->ReleaseRef(); }
    }
    
    void operator = (const Handle &other) {
        if (naked_) { naked_->ReleaseRef(); }
        naked_ = other->naked_;
        if (naked_) { naked_->AddRef(); }
    }
    
    void operator = (Handle &&other) {
        naked_ = other.naked_;
        other.naked_ = nullptr;
    }
    
    T *operator -> () const { return DCHECK_NOTNULL(naked_); }

    bool operator ! () const { return !naked_; }
    
    T *get() const { return naked_; }
    
    bool is_null() const { return naked_ == nullptr; }
    
    template<class S>
    static Handle Make(S *naked) { return Handle<T>(naked); }
    
    static Handle Null() { return Handle<T>(nullptr); }
    
private:
    T *naked_;
}; // class Handle
    

template<class T>
inline bool operator == (const Handle<T> &lhs, const Handle<T> &rhs) {
    return lhs.get() == rhs.get();
}
    
template<class T>
inline Handle<T> MakeRef(T *naked) { return Handle<T>(naked); }
    
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
    ReferenceCounted() {}
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
