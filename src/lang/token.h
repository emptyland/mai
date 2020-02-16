#pragma once
#ifndef MAI_LANG_TOKEN_H_
#define MAI_LANG_TOKEN_H_

#include "lang/syntax.h"

namespace mai {
namespace base {
class ArenaString;
} // namespace base
namespace lang {

using ASTString = base::ArenaString;

#define DECLARE_ALL_TOKEN(V) \
    V(EOF, nullptr, none) \
    V(Error, nullptr, none) \
    V(Identifier, nullptr, text) \
    V(StringLine, nullptr, text) \
    V(StringBlock, nullptr, text) \
    V(StringTempletePrefix, nullptr, text) \
    V(StringTempletePart, nullptr, text) \
    V(StringTempleteSuffix, nullptr, text) \
    V(StringTempleteExpressBegin, "{", text) \
    V(StringTempleteExpressEnd, "}", text) \
    V(I32Val, nullptr, i32) \
    V(Comma, ",", none) \
    V(Plus, "+", none) \
    V(2Plus, "++", none) \
    V(PlusEqual, "+=", none) \
    V(Minus, "-", none) \
    V(2Minus, "--". none) \
    V(MinusEqual, "-=", none) \
    V(Star, "*", none) \
    V(Div, "/", none) \
    V(Percent, "%", none) \
    V(LBrace, "{", none) \
    V(RBrace, "}", none) \
    V(LParen, "(", none) \
    V(RParen, ")", none) \
    V(LBrack, "[", none) \
    V(RBrack, "]", none) \
    V(Dot, ".", none) \
    V(Vargs, "...", none) \
    V(Colon, ":", none) \
    V(2Colon, "::", none) \
    V(Less, "<", none) \
    V(LessEqual, "<=", none) \
    V(LShift, "<<", none) \
    V(LArrow, "<-", none) \
    V(Greater, ">", none) \
    V(GreaterEqual, ">=", none) \
    V(RShift, ">>", none) \
    V(RArrow, "->", none) \
    V(Equal, "==", none) \
    V(Assign, "=", none) \
    DECLARE_KEYWORDS_TOKEN(V)

#define DECLARE_KEYWORDS_TOKEN(V) \
    V(Nil, "nil", none) \
    V(True, "true", none) \
    V(False, "false", none) \
    V(Val, "val", none) \
    V(Var, "var", none) \
    V(Def, "def", none) \
    V(Lambda, "lambda", none) \
    V(Bool, "bool", none) \
    V(I8, "i8", none) \
    V(U8, "u8", none) \
    V(I16, "i16", none) \
    V(U16, "u16", none) \
    V(I32, "i32", none) \
    V(U32, "u32", none) \
    V(I64, "i64", none) \
    V(U64, "u64", none) \
    V(Int, "int", none) \
    V(UInt, "uint", none) \
    V(F32, "f32", none) \
    V(F64, "f64", none) \
    V(Any, "any", none) \
    V(String, "string", none) \
    V(Array, "array", none) \
    V(MutableArray, "mutable_array", none) \
    V(Map, "map", none) \
    V(MutableMap, "mutable_map", none) \
    V(Channel, "channel", none) \
    V(Run, "run", none)

class Token final {
public:
    enum Kind {
    #define DEFINE_ENUM(name, ...) k##name,
        DECLARE_ALL_TOKEN(DEFINE_ENUM)
    #undef  DEFINE_ENUM
    }; // enum Kind
    
    Token(Kind kind, const SourceLocation &position)
        : kind_(kind)
        , source_location_(position) {
        none_val_ = nullptr;
    }
    
    template<class T>
    inline const Token &With(T val) {
        Setter<T>{}.Set(this, val);
        return *this;
    }
    
    DEF_VAL_GETTER(Kind, kind);
    DEF_PTR_PROP_RW(const ASTString, text_val);
    DEF_VAL_GETTER(SourceLocation, source_location);
    DEF_VAL_PROP_RW(float, f32_val);
    DEF_VAL_PROP_RW(double, f64_val);
    DEF_VAL_PROP_RW(int32_t, i32_val);
    DEF_VAL_PROP_RW(uint32_t, u32_val);
    DEF_VAL_PROP_RW(int64_t, i64_val);
    DEF_VAL_PROP_RW(uint64_t, u64_val);
    
    //std::string GetTextString() const { return text_val_->ToString(); }
    
    static Kind IsKeyword(const std::string &text);
private:
    template<class T>
    struct Setter {
        void Set(Token *token, T value) { token->none_val_ = nullptr; }
    };
    
    template<>
    struct Setter<const ASTString *> {
        void Set(Token *token, const ASTString *value) { token->text_val_ = value; }
    };
    
    template<>
    struct Setter<int32_t> {
        void Set(Token *token, int32_t value) { token->i32_val_ = value; }
    };
    
    template<>
    struct Setter<uint32_t> {
        void Set(Token *token, uint32_t value) { token->u32_val_ = value; }
    };
    
    template<>
    struct Setter<int64_t> {
        void Set(Token *token, int64_t value) { token->i64_val_ = value; }
    };
    
    template<>
    struct Setter<uint64_t> {
        void Set(Token *token, uint64_t value) { token->u64_val_ = value; }
    };
    
    template<>
    struct Setter<float> {
        void Set(Token *token, float value) { token->f32_val_ = value; }
    };
    
    template<>
    struct Setter<double> {
        void Set(Token *token, double value) { token->f64_val_ = value; }
    };
    
    Kind kind_;
    SourceLocation source_location_;
    union {
        const ASTString *text_val_;
        float    f32_val_;
        double   f64_val_;
        uint32_t u32_val_;
        uint64_t u64_val_;
        int32_t  i32_val_;
        int64_t  i64_val_;
        void    *none_val_;
    };
}; // class Token


} // namespace lang

} // namespace mai

#endif // MAI_LANG_TOKEN_H_
