#ifndef MAI_LANG_HANDLES_H_
#define MAI_LANG_HANDLES_H_

#include <assert.h>

namespace mai {
    
namespace nyaa {
    
class Nyaa;
class Value;
class Object;
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Scope class
////////////////////////////////////////////////////////////////////////////////////////////////////
    
class HandleScope final {
public:
    HandleScope(Nyaa *N);
    ~HandleScope();
    
    Value **NewHandle(Value *input);
    Object **NewHandle(Object *input);
    
    static HandleScope *current();
    
private:
    Nyaa *nyaa_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Base class
////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
class Handless {
public:
    inline explicit Handless(T **location) : location_(location) {}
    
    inline Handless(const Handless &other) : location_(other.location_) {}
    
    inline Handless(Handless &&other) : location_(other.location_) { other.location_ = nullptr; }
    
    inline ~Handless() {}
    
    inline T *get() const { return *location_; }
    
    inline bool is_null() const { return !get(); }

    inline bool is_not_null() const { return !is_null(); }
    
    inline bool is_empty() const { return !location_; }
    
    inline bool is_not_empty() const { return !is_empty(); }
    
protected:
    inline T **location() const { return location_; }
    
    inline void set_location(T **location) { location_ = location; }

private:
    T **location_;
}; // class Local


////////////////////////////////////////////////////////////////////////////////////////////////////
// Local Handle
////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
class Handle final : public Handless<T> {
public:
    inline explicit Handle(T **location) : Handless<T>(location) {}
    
    template<class S> inline Handle(S *pointer)
        : Handless<T>(reinterpret_cast<T**>(HandleScope::current()->NewHandle(pointer))) {}
    
    template<class S> inline Handle(Handle<S> other)
        : Handless<T>(reinterpret_cast<T**>(HandleScope::current()->NewHandle(other.get()))) {}
    
    inline Handle(const Handle &other) : Handless<T>(other) {}
    
    inline Handle(Handle &&other) : Handless<T>(other) {}
    
    inline ~Handle() {}
    
    using Handless<T>::get;
    
    inline T *operator -> () const { return get(); }
    
    inline bool operator ! () const { return !get(); }
    
    using Handless<T>::set_location;
    using Handless<T>::location;
    
    inline void operator = (const Handle &other) { set_location(other.location()); }
    
    inline void operator = (Handle &&other) {
        set_location(other.location());
        other.set_location(nullptr);
    }
    
    inline static Handle<T> Null() { return Handle<T>(nullptr); }
}; // class Handle

template<class T>
class Local final : public Handless<T> {
public:
    inline explicit Local(T **location) : Handless<T>(location) {}
    
    template<class S> inline Local(S *pointer)
        : Handless<T>(reinterpret_cast<T**>(HandleScope::current()->NewHandle(pointer))) {}
    
    template<class S> inline Local(Local<S> other)
        : Handless<T>(reinterpret_cast<T**>(HandleScope::current()->NewHandle(other.location()))) {}
    
    template<class S> inline Local(Handle<S> other)
        : Handless<T>(reinterpret_cast<T**>(HandleScope::current()->NewHandle(other.location()))) {}
    
    inline Local(const Local &other) : Handless<T>(other) {}
    
    inline Local(Local &&other) : Handless<T>(other) {}
    
    inline ~Local() {}
    
    using Handless<T>::get;
    
    inline T *operator -> () const { return get(); }
    
    inline bool operator ! () const { return !get(); }

    using Handless<T>::set_location;
    using Handless<T>::location;

    inline void operator = (const Local &other) { set_location(other.location()); }
    
    inline void operator = (const Handle<T> &other) { set_location(other.location()); }
    
    inline void operator = (Local &&other) {
        set_location(other.location());
        other.set_location(nullptr);
    }
    
    inline static Local<T> Null() { return Local<T>(nullptr); }
}; // class Local
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_LANG_HANDLES_H_
