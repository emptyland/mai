#pragma once
#ifndef MAI_LANG_SYNTAX_H_
#define MAI_LANG_SYNTAX_H_

#include "base/arena.h"
#include "base/slice.h"
#include "base/base.h"

namespace mai {
namespace base {
class ArenaString;
} // namespace base
namespace lang {

using ASTString = base::ArenaString;

struct Operator {
    enum Kind {
        NOT_BINARY, // Flag value
        NOT_UNARY, // Flag value
        
        kAdd,
        kSub,
        kMul,
        kDiv,
        kMod,
        kMinus,
        
        kBitwiseShl,
        kBitwiseShr,
        kBitwiseAnd,
        kBitwiseOr,
        kBitwiseNot,
        kBitwiseXor,
        
        kEqual,
        kNotEqual,
        kLess,
        kLessEqual,
        kGreater,
        kGreaterEqual,
        
        kAnd,
        kOr,
        kNot,
        
        kContcat,
        kRecv, // Channel receive
        kSend, // Channel send
    };
    Kind kind;
    int operands;
    int left;
    int right;
    
    constexpr Operator(Kind k, int o, int l, int r): kind(k), operands(o), left(l), right(r) {}
    Operator() {}
}; // struct Operator

struct Operators {
    static constexpr int kUnaryPrio = 110;
    
    // Unary:
    static constexpr auto kNot = Operator(Operator::kNot, 1, kUnaryPrio, kUnaryPrio); // !
    static constexpr auto kBitwiseNot = Operator(Operator::kBitwiseNot, 1, kUnaryPrio, kUnaryPrio); // ^
    static constexpr auto kMinus = Operator(Operator::kMinus, 1, kUnaryPrio, kUnaryPrio); // -
    static constexpr auto kRecv = Operator(Operator::kRecv, 1, kUnaryPrio, kUnaryPrio); // <-

    // Binary:
    static constexpr auto kAdd = Operator(Operator::kAdd, 2, 90, 90); // +
    static constexpr auto kSub = Operator(Operator::kSub, 2, 90, 90); // -
    static constexpr auto kMul = Operator(Operator::kMul, 2, 100, 100); // *
    static constexpr auto kDiv = Operator(Operator::kDiv, 2, 100, 100); // /
    static constexpr auto kMod = Operator(Operator::kMod, 2, 100, 100); // %

    static constexpr auto kSend = Operator(Operator::kSend, 2, 100, 100); // chan <- data
    
    // Bitwise op
    static constexpr auto kBitwiseShl = Operator(Operator::kBitwiseShl, 2, 70, 70); // <<
    static constexpr auto kBitwiseShr = Operator(Operator::kBitwiseShr, 2, 70, 70); // >>
    
    // Compare
    static constexpr auto kEqual = Operator(Operator::kEqual, 2, 60, 60); // ==
    static constexpr auto kNotEqual = Operator(Operator::kNotEqual, 2, 60, 60); // !=
    static constexpr auto kLess = Operator(Operator::kLess, 2, 60, 60); // <
    static constexpr auto kLessEqual = Operator(Operator::kLessEqual, 2, 60, 60); // <=
    static constexpr auto kGreater = Operator(Operator::kGreater, 2, 60, 60); // >
    static constexpr auto kGreaterEqual = Operator(Operator::kGreaterEqual, 2, 60, 60); // >=

    // Or/Xor/And
    static constexpr auto kBitwiseOr = Operator(Operator::kBitwiseOr, 2, 30, 30); // |
    static constexpr auto kBitwiseXor = Operator(Operator::kBitwiseXor, 2, 40, 40); // ^
    static constexpr auto kBitwiseAnd = Operator(Operator::kBitwiseAnd, 2, 40, 40); // &
    
    // And/Or
    static constexpr auto kAnd = Operator(Operator::kAnd, 2, 20, 20); // &&
    static constexpr auto kOr = Operator(Operator::kOr, 2, 10, 10); // ||
    
    static constexpr auto kContact = Operator(Operator::kContcat, 2, 10, 10); // string concat
    
    static constexpr auto NOT_UNARY = Operator(Operator::NOT_UNARY, 0, 0, 0);
    static constexpr auto NOT_BINARY = Operator(Operator::NOT_BINARY, 0, 0, 0);
}; // struct Operators

struct SourceLocation {
    int begin_line;
    int begin_row;
    int end_line;
    int end_row;
    
    void LinkEnd(const SourceLocation &loc) {
        end_line = loc.end_line;
        end_row  = loc.end_row;
    }
    
    static SourceLocation One(int line, int row) {
        return {line, row, line, row};
    }
}; // struct SourceLocation


// Feed back for error or warning
class SyntaxFeedback {
public:
    void Feedback(const SourceLocation &location, const char *message) {
        DidFeedback(location, message, ::strlen(message));
    }

    __attribute__ (( __format__ (__printf__, 3, 4)))
    void Printf(const SourceLocation &location, const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        std::string message(base::Vsprintf(fmt, ap));
        va_end(ap);
        DidFeedback(location, message.data(), message.size());
    }
    
    virtual void DidFeedback(const SourceLocation &location, const char *z, size_t n) = 0;

    DEF_VAL_PROP_RW(std::string, package_name);
    DEF_VAL_PROP_RW(std::string, file_name);
protected:
    std::string package_name_;
    std::string file_name_;
}; // class SyntaxFeedback

void InitializeSyntaxLibrary();

void FreeSyntaxLibrary();

#define CHECK_OK ok); if (!*ok) { return nullptr; } ((void)0

} // namespace lang

} // namespace mai

#endif // MAI_LANG_SYNTAX_H_
