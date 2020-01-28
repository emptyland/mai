#ifndef MAI_LANG_HANDLE_H_
#define MAI_LANG_HANDLE_H_

#include <type_traits>

namespace mai {

namespace lang {

template<class T> class Handle;

class HandleScope {
public:
    enum Initializer {
        ON_EXIT_SCOPE_INITIALIZER
    };
    
    // Into this scope
    HandleScope(Initializer/*avoid c++ compiler optimize this object*/);
    // Out this scope
    ~HandleScope();
    
    // Get number of handles
    int GetNumberOfHanldes() const;
    
    // Get this scope's HandleScope
    static HandleScope *Current();
    
    // Escape a owned local handle
    template<class T> inline Handle<T> CloseAndEscape(Handle<T> in_scope);
    
    template<class T>
    static inline T **NewLocation(T *value) {
        return reinterpret_cast<T **>(Current()->NewHandle(value));
    }
private:
    // Get the prev(scope) HandleScope
    HandleScope *GetPrev();
    
    // New handle address and set heap object pointer
    void **NewHandle(const void *value);
    
    // Close this scope
    void Close();
    
    bool close_ = true;
}; // class HandleScope


template<class T>
class HandleBase {
public:
    // Get heap object pointer, is value
    inline T *get() const { return *location_; }

    // Get handle address, not value
    inline T **location() const { return location_; }

    // Test heap object pointer is null or not
    inline bool is_value_null() const { return get() == nullptr; }
    inline bool is_value_not_null() const { return !is_value_null(); }

    // Test handle address is null or not
    inline bool is_empty() const { return location_ == nullptr; }
    inline bool is_not_empty() const { return !is_empty(); }
protected:
    // Raw initialize handle
    inline HandleBase(T **location) : location_(location) {}
    T **location_;
}; // class HandleBase


// Handle is thread local value, dont use it in other threads
template<class T>
class Handle : public HandleBase<T> {
public:
    // Initialize by a handle address
    inline explicit Handle(T **location = nullptr) : HandleBase<T>(location) {}

    // Initialize by heap object pointer
    template<class S> inline explicit Handle(S *pointer)
        : Handle(HandleScope::NewLocation<T>(pointer)) {
        static_assert(std::is_convertible<S*, T*>::value || std::is_same<S, T>::value,
                      "can not cast to");
    }

    // Initialize by other handle
    template<class S> inline explicit Handle(const Handle<S> &other)
        : Handle(other.is_empty() ? nullptr : HandleScope::NewLocation<T>(other.get())) {
        static_assert(std::is_convertible<S*, T*>::value || std::is_same<S, T>::value,
                      "can not cast to");
    }
    
    using HandleBase<T>::location_;

    // Copy constructor
    inline Handle(const Handle &other)
        : Handle(other.is_empty() ? nullptr : HandleScope::NewLocation<T>(other.get())) {}

    // Right reference constructor
    inline Handle(Handle &&other): Handle(other.location_) { other.location_ = nullptr; }

    using HandleBase<T>::get;
    
    // Getters
    inline T *operator -> () const { return get(); }
    inline T *operator * () const { return get(); }
    
    using HandleBase<T>::is_value_null;
    using HandleBase<T>::is_value_not_null;
    using HandleBase<T>::is_empty;
    using HandleBase<T>::is_not_empty;

    // Tester
    inline bool operator ! () const { return is_value_not_null(); }
    inline bool operator == (const Handle &other) const { return get() == other.get(); }

    // Assignment
    inline void operator = (const Handle &other) { Assign(other.location_); }

    inline void operator = (const Handle &&other) {
        Assign(other.location_);
        other.location_ = nullptr;
    }

    inline void operator = (T *pointer) {
        if (location_ && *location_ == pointer) {
            return;
        }
        if (is_empty()) {
            location_ = HandleScope::NewLocation(pointer);
        } else {
            *location_ = pointer;
        }
    }

    // Utils
    // Handle cast
    template<class S> static inline Handle<T> Cast(const Handle<S> &other) {
        return Handle<T>(other);
    }
    // Make a null heap pointer handle(handle address is not null)
    static inline Handle<T> Null() { return Handle<T>(static_cast<T *>(nullptr)); }
    // Make a null handle address handle
    static inline Handle<T> Empty() { return Handle<T>(static_cast<T **>(nullptr)); }
    
private:
    void Assign(T **location) {
        if (location_ == location) {
            return;
        }
        if (is_empty()) {
            if (location) {
                operator=(*location);
            } else {
                // Nothing
            }
        } else { // No empty
            if (location) {
                *location_ = *location;
            } else {
                location_ = nullptr;
            }
        }
    }
}; // class Handle



template<class T>
class Persistent : public HandleBase<T> {
public:
    // Initialize a empty handle
    inline Persistent();
    // Copy constructor
    inline Persistent(const Persistent &);
    // Right reference constructor
    inline Persistent(Persistent &&);

    // Manual drop this handle, object mybe release by next GC
    inline void Dispose();
  
      // Getters
    T *operator -> () const;
    T *operator * () const;

    // Tester
    bool operator ! () const;
    bool operator == (const Handle<T> &) const;

    // Utils
    // Handle cast
    template<class S> static inline Persistent<T> Cast(const Persistent<S> other);

    // Create by local handle
    static inline Persistent<T> New(const Handle<T> &local);
}; // class Persistent


// Escape a owned local handle
template<class T>
inline Handle<T> HandleScope::CloseAndEscape(Handle<T> in_scope) {
    if (in_scope.is_empty()) {
        return Handle<T>::Empty();
    }
    Handle<T> escaped(GetPrev()->NewLocation(in_scope.get()));
    Close();
    return escaped;
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_HANDLE_H_
