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
    bool IsSmi() const;
    
    bool IsObject() const;
    
    int64_t AsSmi() const;
    
    HeapObject *AsHeapObject() const;
}; // class Value
    
//--------------------------------------------------------------------------------------------------
// [Integer]
class Integral final {
public:
    static Handle<Value> New(Nyaa *N, int64_t val);
    
}; // class Integral
    

//--------------------------------------------------------------------------------------------------
// [Base Heap Object]
class HeapObject : public Value {
public:
    bool IsString() const;
    
    bool IsDecimal() const;
    
    bool IsScript() const;

}; // class HeapObject

    
//--------------------------------------------------------------------------------------------------
// [String]
class String final : public HeapObject {
public:
    static Handle<String> New(Nyaa *N, const char *s) { return New(N, s, !s ? 0 : ::strlen(s)); }
    
    static Handle<String> New(Nyaa *N, const char *s, size_t n);
    
    uint32_t GetHashVal() const;
    
    const char *GetBytes() const;
    
    size_t Length() const;
}; // class String


//--------------------------------------------------------------------------------------------------
// [Script]
class Script final : public HeapObject {
public:
    static Handle<Script> Compile(Nyaa *N, Handle<String> source);
    
    static Handle<Script> Compile(Nyaa *N, FILE *fp);
    
    Handle<Value> Run(Nyaa *N);
}; // class Script
    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_LANG_VALUES_H_


