#pragma once
#ifndef MAI_LANG_PARSER_UTILS_H_
#define MAI_LANG_PARSER_UTILS_H_

#include "base/arena.h"
#include "base/slice.h"
#include "base/base.h"

namespace mai {
namespace base {
class ArenaString;
} // namespace base
namespace lang {

using ASTString = base::ArenaString;

struct SourceLocation {
    int begin_line;
    int begin_row;
    int end_line;
    int end_row;
    
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

} // namespace lang

} // namespace mai

#endif // MAI_LANG_PARSER_UTILS_H_
