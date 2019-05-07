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
class Long;
class Script;
class Table;
    
//--------------------------------------------------------------------------------------------------
// [Value]
class Value {
public:
    bool IsObject() const;

    bool IsNumber() const;

    bool IsString() const;

    bool IsFunction() const;

    bool IsScript() const;
}; // class Value
    
//--------------------------------------------------------------------------------------------------
// [Integer]
class Number final {
public:
    static Handle<Number> NewI64(Nyaa *N, int64_t val);

    static Handle<Number> NewF64(Nyaa *N, double val);
    
}; // class Integer


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
// [Result]
class Result final : public Value {
public:
    Handle<Value> Get(size_t i) const;
    
    bool IsEmpty() const { return Length() == 0; }
    
    size_t Length() const;
}; // class Result
    

//--------------------------------------------------------------------------------------------------
// [Function]
class Function final : public Value {
public:
    
    void Bind(int i, Handle<Value> val);
    
    Handle<Value> GetUpVal(int i);
    
}; // class Function
    

//--------------------------------------------------------------------------------------------------
// [Script]
class Script final : public Value {
public:
    static Handle<Script> Compile(Nyaa *N, Handle<String> source);
    
    static Handle<Script> Compile(Nyaa *N, const char *file_name, FILE *fp);
    
    Handle<Result> Run(Nyaa *N, int wanted);
}; // class Script

    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_LANG_VALUES_H_


