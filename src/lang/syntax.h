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
        kAdd,
        kSub,
        kMul,
        kDiv,
    };
    Kind kind;
    int operands;
    int left;
    int right;
    
    constexpr Operator(Kind k, int o, int l, int r): kind(k), operands(o), left(l), right(r) {}
}; // struct Operator

struct Operators {
    static constexpr auto kAdd = Operator(Operator::kAdd, 2, 1, 0);
    static constexpr auto kSub = Operator(Operator::kSub, 2, 1, 0);
    static constexpr auto kMul = Operator(Operator::kMul, 2, 1, 0);
    static constexpr auto kDiv = Operator(Operator::kDiv, 2, 1, 0);
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
