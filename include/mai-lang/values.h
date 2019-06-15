#ifndef MAI_LANG_VALUES_H_
#define MAI_LANG_VALUES_H_

#include "mai-lang/handles.h"
#include <stdio.h>
#include <assert.h>
#include <string>
#include <vector>

namespace mai {
    
namespace nyaa {
    
class Nyaa;
class Value;
class String;
class Long;
class Script;
class Table;
template<class T> class FunctionCallbackInfo;
    
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
    
class Objective : public Value {
public:
    Handle<Value> Get(Handle<Value> key) const;
    void Set(Handle<Value> key, Handle<Value> value);
    
    Handle<Value> Add(Handle<Value> rhs);
    Handle<Value> Sub(Handle<Value> rhs);
    Handle<Value> Mul(Handle<Value> rhs);
    Handle<Value> Div(Handle<Value> rhs);
    Handle<Value> Mod(Handle<Value> rhs);
    Handle<Value> Unm();
    Handle<Value> Concat(Handle<Value> rhs);
    Handle<String> Str();
    
    bool Equal(Handle<Value> rhs);
    bool LessThan(Handle<Value> rhs);
    bool LessEqual(Handle<Value> rhs);
    
    int Call(Handle<Value> argv[], int argc, Handle<Value> rets[], int wanted);
    int Call(Handle<Value> argv[], int argc, std::vector<Handle<Value>> *rets, int wanted);
}; // class Objective
    
//--------------------------------------------------------------------------------------------------
// [Integer]
class Number final : public Objective {
public:
    static Handle<Number> NewI64(int64_t val);
    static Handle<Number> NewF64(double val);
    
    int64_t I64Value() const;
    double F64Value() const;
    
    
}; // class Integer


//--------------------------------------------------------------------------------------------------
// [String]
class String final : public Objective {
public:
    static Handle<String> New(const char *s) { return New(s, !s ? 0 : ::strlen(s)); }
    static Handle<String> New(const char *s, size_t n);
    
    uint32_t HashVal() const;
    const char *Bytes() const;
    size_t Length() const;

    std::string ToStl() const { return std::string(Bytes(), Length()); }
}; // class String


//--------------------------------------------------------------------------------------------------
// [Function]
class Function : public Objective {
public:
    using InvocationCallback = void (*)(const FunctionCallbackInfo<Value> &);
    
    static Handle<Function> New(InvocationCallback callback, size_t nupvals);

    void Bind(int i, Handle<Value> val);
    Handle<Value> GetUpVal(int i);

    int Call(Handle<Value> argv[], int argc, Handle<Value> rets[], int wanted);
    int Call(Handle<Value> argv[], int argc, std::vector<Handle<Value>> *rets, int wanted);
}; // class Function
    

//--------------------------------------------------------------------------------------------------
// [Script]
class Script final : public Function {
public:
    static Handle<Script> Compile(Handle<String> source);
    
    static Handle<Script> Compile(const char *file_name, FILE *fp);
}; // class Script

    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_LANG_VALUES_H_


