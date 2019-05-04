#ifndef MAI_LANG_HANDLES_H_
#define MAI_LANG_HANDLES_H_

#include <assert.h>

namespace mai {
    
namespace nyaa {
    
class Nyaa;
class Isolate;
class Value;
class Object;
    
template<class T> class Handle;
template<class T> class Local;
template<class T> class Persistent;
    
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Scope class
////////////////////////////////////////////////////////////////////////////////////////////////////
    
class HandleScope final {
public:
    HandleScope();
    HandleScope(Nyaa *N);
    ~HandleScope();
    
    void **NewHandle(void *value);
    
    template<class T>
    inline Handle<T> CloseAndEscape(Handle<T> in_scope);
    
    HandleScope *Prev();

    static HandleScope *Current();
    static HandleScope *Current(Nyaa *N);
private:
    Nyaa *N_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Base class
////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
class Handless {
public:
    inline Handless(const Handless &other) : location_(other.location_) {}
    
    inline Handless(Handless &&other) : location_(other.location_) { other.location_ = nullptr; }
    
    inline ~Handless() {}
    
    inline T *get() const { return *location_; }
    
    inline T **location() const { return location_; }
    
    inline bool is_null() const { return location_ && !*location_; }

    inline bool is_not_null() const { return !is_null(); }
    
    inline bool is_empty() const { return !location_; }
    
    inline bool is_not_empty() const { return !is_empty(); }
    
    inline bool is_valid() const { return is_not_empty() && is_not_null(); }
    
    inline bool is_not_valid() const { return !is_valid(); }
    
protected:
    inline explicit Handless(T **location) : location_(location) {}

    inline void set_location(T **location) { location_ = location; }

private:
    T **location_;
}; // class Local


////////////////////////////////////////////////////////////////////////////////////////////////////
// Local Handle
////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
class Handle : public Handless<T> {
public:
    inline explicit Handle(T **location = nullptr) : Handless<T>(location) {}
    
    template<class S> inline Handle(S *pointer)
        : Handless<T>(reinterpret_cast<T**>(HandleScope::Current()->NewHandle(pointer))) {}
    
    template<class S> inline Handle(Handle<S> other)
        : Handless<T>(reinterpret_cast<T**>(HandleScope::Current()->NewHandle(other.get()))) {}
    
    inline Handle(const Handle &other) : Handless<T>(other) {}
    
    inline Handle(Handle &&other) : Handless<T>(other) {}
    
    inline ~Handle() {}
    
    using Handless<T>::get;
    
    inline T *operator -> () const { return get(); }
    
    inline T *operator * () const { return get(); }
    
    using Handless<T>::is_empty;

    inline bool operator ! () const { return is_empty(); }
    
    using Handless<T>::set_location;
    using Handless<T>::location;
    
    inline void operator = (const Handle &other) {
        if (other.is_empty()) {
            set_location(nullptr);
        } else if (is_empty()) {
            set_location(reinterpret_cast<T**>(HandleScope::Current()->NewHandle(other.get())));
        } else {
            *location() = other.get();
        }
    }
    
    inline void operator = (T *ptr) {
        if (is_empty()) {
            set_location(reinterpret_cast<T**>(HandleScope::Current()->NewHandle(ptr)));
        } else {
            *location() = ptr;
        }
    }
    
    inline void operator = (Handle &&other) {
        set_location(other.location());
        other.set_location(nullptr);
    }

    template<class S> inline static Handle<T> Cast(const Handle<S> &input) {
        if (input.is_empty()) {
            return Handle<T>();
        }
        auto h = reinterpret_cast<T**>(HandleScope::Current()->NewHandle(*input));
        return Handle<T>(h);
    }
    
    inline static Handle<T> Null() { return Handle(static_cast<T *>(nullptr)); }

    inline static Handle<T> Empty() { return Handle(static_cast<T **>(nullptr)); }
}; // class Handle


template<class T>
class Local final : public Handle<T> {
public:
    inline Local() : Local(static_cast<T **>(nullptr)) {}

    inline Local(const Local &other) : Handle<T>(other) {}

    inline Local(Local &&other) : Handle<T>(other) {}
    
    inline ~Local() {}

    using Handless<T>::location;

    template<class S> inline static Local<T> Cast(const Local<S> &input) {
        T *unused = static_cast<T *>(*input);
        (void)unused;
        return Local<T>(reinterpret_cast<T **>(input.location()));
    }
    
    inline static Local<T> New(const Handle<T> &input);
    
    inline static Local<T> New(Nyaa *N, const Handle<T> &input);
    
    inline static Local<T> New(const Persistent<T> &input);
    
    inline static Local<T> New(Nyaa *N, const Persistent<T> &input);
    
    inline static Local<T> Warp(T **location) { return Local<T>(location); }
private:
    inline explicit Local(T **location) : Handle<T>(location) {}
}; // class Local
    

////////////////////////////////////////////////////////////////////////////////////////////////////
// Persistent Handle
////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
class Persistent final : public Handless<T> {
public:
    inline Persistent() : Persistent(static_cast<T **>(nullptr)) {}
    
    inline Persistent(const Persistent &other) : Handless<T>(other) {}
    
    inline Persistent(Persistent &&other) : Handless<T>(other) {}
    
    inline ~Persistent() {}
    
    inline void Dispose();
    
    using Handless<T>::location;
    
    template<class S> inline static Persistent<T> Cast(const Persistent<S> &input) {
        T *unused = static_cast<T *>(*input);
        (void)unused;
        return Persistent<T>(reinterpret_cast<T **>(input.location()));
    }
    
    inline static Persistent<T> New(const Handle<T> &input);
    
    inline static Persistent<T> New(Isolate *isolate, const Handle<T> &input);
    
private:
    inline explicit Persistent(T **location) : Handless<T>(location) {}
}; // template<class T> class Persistent
    

////////////////////////////////////////////////////////////////////////////////////////////////////
// Handle Inline Functions:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
template<class T>
inline Local<T> Local<T>::New(const Handle<T> &input) {
    if (input.is_empty()) {
        return Local<T>();
    }
    auto h = reinterpret_cast<T**>(HandleScope::Current()->NewHandle(*input));
    return Local<T>(h);
}

template<class T>
inline Local<T> Local<T>::New(Nyaa *N, const Handle<T> &input) {
    if (input.is_empty()) {
        return Local<T>();
    }
    auto h = reinterpret_cast<T**>(HandleScope::Current(N)->NewHandle(*input));
    return Local<T>(h);
}
    
template<class T>
inline Local<T> Local<T>::New(const Persistent<T> &input) {
    if (input.is_empty()) {
        return Local<T>();
    }
    auto h = reinterpret_cast<T**>(HandleScope::Current()->NewHandle(*input));
    return Local<T>(h);
}

template<class T>
inline Local<T> Local<T>::New(Nyaa *N, const Persistent<T> &input) {
    if (input.is_empty()) {
        return Local<T>();
    }
    auto h = reinterpret_cast<T**>(HandleScope::Current(N)->NewHandle(*input));
    return Local<T>(h);
}
    
template<class T>
inline Handle<T> HandleScope::CloseAndEscape(Handle<T> in_scope) {
    T **address = reinterpret_cast<T **>(Prev()->NewHandle(*in_scope));
    return Handle<T>(address);
}

} // namespace nyaa
    
} // namespace mai


#endif // MAI_LANG_HANDLES_H_
