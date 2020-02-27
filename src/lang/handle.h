#ifndef MAI_LANG_HANDLE_H_
#define MAI_LANG_HANDLE_H_

#include <type_traits>

namespace mai {

namespace lang {

template<class T> class Local;

class HandleScope {
public:
    enum Initializer {
        INITIALIZER
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
    template<class T> inline Local<T> CloseAndEscape(Local<T> in_scope);
    
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

struct GlobalHandles {
    
    template<class T>
    static inline T **NewLocation(T *value) {
        return reinterpret_cast<T **>(NewHandle(value));
    }

    // New global handle address and set heap object pointer
    static void **NewHandle(const void *pointer);

    // Delete global handle by location
    static void DeleteHandle(void **location);
    
    // Get number of global handles
    static int GetNumberOfHandles();

}; // struct GlobalHandles

template<class T>
class Handle {
public:
    // Getters
    inline T *operator -> () const { return get(); }
    inline T *operator * () const { return get(); }

    // Tester
    inline bool operator ! () const { return is_value_not_null(); }
    inline bool operator == (const Handle &other) const { return get() == other.get(); }
    inline bool operator != (const Handle &other) const { return !operator==(other); }
    
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
    inline Handle(T **location) : location_(location) {}
    T **location_;
}; // class HandleBase


// Handle is thread local value, dont use it in other threads
template<class T>
class Local : public Handle<T> {
public:
    using Handle<T>::location_;
    using Handle<T>::is_value_null;
    using Handle<T>::is_value_not_null;
    using Handle<T>::is_empty;
    using Handle<T>::is_not_empty;
    using Handle<T>::get;
    
    // Initialize a empty handle
    inline Local() : Local(static_cast<T **>(nullptr)) {}
    
    // Initialize by heap object pointer
    template<class S> inline explicit Local(S *pointer)
        : Local(HandleScope::NewLocation<T>(static_cast<T *>(pointer))) {
        static_assert(std::is_convertible<S*, T*>::value || std::is_same<S, T>::value,
                      "can not cast to");
    }

    // Initialize by other handle
    template<class S> inline explicit Local(const Local<S> &other)
        : Local(other.is_empty() ? nullptr : HandleScope::NewLocation<T>(static_cast<T*>(*other))) {
        static_assert(std::is_convertible<S*, T*>::value || std::is_same<S, T>::value,
                      "can not cast to");
    }

    // Copy constructor
    inline Local(const Local &other)
        : Local(other.is_empty() ? nullptr : HandleScope::NewLocation<T>(*other)) {}

    // Right reference constructor
    inline Local(Local &&other): Local(other.location_) { other.location_ = nullptr; }

    // Assignment
    inline void operator = (const Local &other) { Assign(other.location_); }

    inline void operator = (Local &&other) {
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
    template<class S> static inline Local<T> Cast(const Local<S> &other) {
        return Local<T>(other);
    }
    // Make a null heap pointer handle(handle address is not null)
    static inline Local<T> Null() { return Local<T>(static_cast<T *>(nullptr)); }
    // Make a null handle address handle
    static inline Local<T> Empty() { return Local<T>(static_cast<T **>(nullptr)); }
    
private:
    // Initialize by a handle address
    inline explicit Local(T **location) : Handle<T>(location) {}

    inline void Assign(T **location) {
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
class Persistent : public Handle<T> {
public:
    using Handle<T>::location_;
    using Handle<T>::is_value_null;
    using Handle<T>::is_value_not_null;
    using Handle<T>::is_empty;
    using Handle<T>::is_not_empty;
    using Handle<T>::get;
    
    // Initialize a empty handle
    inline Persistent() : Persistent(nullptr) {}

    // Copy constructor
    inline Persistent(const Persistent &other)
        : Persistent(other.is_empty() ? nullptr : GlobalHandles::NewLocation(*other)) {}

    // Right reference constructor
    inline Persistent(Persistent &&other)
        : Persistent(other.location_) { other.location_ = nullptr; }

    // Manual drop this handle, object mybe release by next GC
    inline void Dispose() {
        if (is_not_empty()) {
            GlobalHandles::DeleteHandle(reinterpret_cast<void **>(location_));
            location_ = nullptr;
        }
    }

    // Assignment
    inline void operator = (const Persistent &other) { Assign(other.location_); }

    inline void operator = (Persistent &&other) {
        if (Assign(other.location_)) {
            other.location_ = nullptr;
        }
    }

    inline void operator = (const Local<T> &other) { Assign(other.location_); }

    inline void operator = (T *pointer) {
        if (location_ && *location_ == pointer) {
            return;
        }
        if (is_empty()) {
            location_ = GlobalHandles::NewLocation(pointer);
        } else {
            *location_ = pointer;
        }
    }

    // Utils
    // Handle cast
    template<class S> static inline Persistent<T> Cast(const Persistent<S> other) {
        static_assert(std::is_convertible<S*, T*>::value || std::is_same<S, T>::value,
        "can not cast to");
        if (other.is_empty()) {
            return Persistent<T>();
        }
        return Persistent<T>(GlobalHandles::NewLocation(*other));
    }

    // Create by local handle
    static inline Persistent<T> New(const Local<T> &local) {
        if (local.is_empty()) {
            return Persistent<T>();
        }
        return Persistent<T>(GlobalHandles::NewLocation(*local));
    }

private:
    inline Persistent(T **location): Handle<T>(location) {}
    
    inline bool Assign(T **location) {
        if (location_ == location) {
            return false;
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
        return true;
    }
}; // class Persistent


// Escape a owned local handle
template<class T>
inline Local<T> HandleScope::CloseAndEscape(Local<T> in_scope) {
    if (in_scope.is_empty()) {
        return Local<T>::Empty();
    }
    Local<T> escaped(GetPrev()->NewLocation(in_scope.get()));
    Close();
    return escaped;
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_HANDLE_H_
