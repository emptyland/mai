#ifndef MAI_LANG_VALUES_H_
#define MAI_LANG_VALUES_H_

#include "mai-lang/handles.h"
#include <stdio.h>
#include <assert.h>
#include <string>

namespace mai {
    
namespace nyaa {
    
class Nyaa;
class Value;
class HeapObject;
class String;
class Decimal;
class Script;
class Table;
    
//--------------------------------------------------------------------------------------------------
// [Value]
class Value {
public:
    bool IsObject() const;
    
    bool IsInt() const;
    
    bool IsString() const;
    
    bool IsDecimal() const;
    
    bool IsScript() const;
    
    int64_t AsInt() const;
}; // class Value
    
//--------------------------------------------------------------------------------------------------
// [Integer]
class Integral final {
public:
    static Handle<Value> New(Nyaa *N, int64_t val);
    
}; // class Integral


//--------------------------------------------------------------------------------------------------
// [String]
class String final : public Value {
public:
    static Handle<String> New(Nyaa *N, const char *s) { return New(N, s, !s ? 0 : ::strlen(s)); }
    
    static Handle<String> New(Nyaa *N, const char *s, size_t n);
    
    uint32_t HashVal() const;
    
    const char *Bytes() const;
    
    size_t Length() const;
}; // class String


//--------------------------------------------------------------------------------------------------
// [Script]
class Script final : public Value {
public:
    static Handle<Script> Compile(Nyaa *N, Handle<String> source);
    
    static Handle<Script> Compile(Nyaa *N, FILE *fp);
    
    int Run(Nyaa *N);
}; // class Script

    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_LANG_VALUES_H_


