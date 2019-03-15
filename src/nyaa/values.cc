#include "mai-lang/values.h"
#include "nyaa/nyaa-values.h"


namespace mai {
    
namespace nyaa {
    
/*static*/ Handle<Value> Integral::New(Nyaa *N, int64_t val) {
    // TODO:
    return Handle<Value>::Null();
}

/*static*/ Handle<String> String::New(Nyaa *N, const char *s, size_t n) {
    // TODO:
    return Handle<Value>::Null();
}

uint32_t String::GetHashVal() const {
    return reinterpret_cast<const NyString *>(this)->hash_val();
}

const char *String::GetBytes() const {
    return reinterpret_cast<const NyString *>(this)->bytes();
}

size_t String::Length() const {
    return reinterpret_cast<const NyString *>(this)->size();
}
    
/*static*/ Handle<Script> Script::Compile(Nyaa *N, Handle<String> source) {
    // TODO:
    return Handle<Script>::Null();
}

/*static*/ Handle<Script> Script::Compile(Nyaa *N, FILE *fp) {
    // TODO:
    return Handle<Script>::Null();
}

Handle<Value> Script::Run(Nyaa *N) {
    // TODO:
    return Handle<Value>::Null();
}

} // namespace nyaa
    
} // namespace mai
