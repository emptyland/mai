#pragma once
#ifndef MAI_LANG_LEXER_H_
#define MAI_LANG_LEXER_H_

#include "lang/token.h"
#include "lang/syntax.h"
#include "mai/env.h"
#include "glog/logging.h"
#include <stack>

namespace mai {

namespace lang {

class Lexer final {
public:
    static constexpr size_t kBufferSize = 4096;
    
    Lexer(base::Arena *arena, SyntaxFeedback *error_feedback)
        : arena_(DCHECK_NOTNULL(arena))
        , error_feedback_(error_feedback) {}

    ~Lexer() {
        if (input_file_ownership_) { delete input_file_; }
    }
    
    Error SwitchInputFile(const std::string &name, SequentialFile *file);
    
    Token Next();
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Lexer);
private:
    enum TemplateKind {
        kNotTemplate,
        kSimpleTemplate,
        kExpressionTemplate,
    };

    Token MatchString();
    Token MatchSimpleTemplateString();
    Token MatchIdentifier();
    
    bool MatchEscapeCharacter(std::string *buf);
    bool MatchUtf8Character(std::string *buf);
    
    static bool IsTerm(int ch) { return ::isalpha(ch) || ::isdigit(ch) || IsTermChar(ch); }
    static bool IsTermChar(int ch);

    static bool IsHexChar(int ch) {
        return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
    }
    
    static int HexCharToInt(int ch) {
        if (ch >= '0' && ch <= '9') {
            return ch - '0';
        }
        if (ch >= 'a' && ch <= 'f') {
            return 10 + (ch - 'a');
        }
        if (ch >= 'A' && ch <= 'F') {
            return 10 + (ch - 'A');
        }
        NOREACHED();
        return -1;
    }

    ALWAYS_INLINE static int IsUtf8Prefix(int ch) {
        if ((ch & 0x80) == 0) {
            return 1;
        } else if ((ch & 0xe0) == 0xc0) { // 110x xxxx : 2 bytes
            return 2;
        } else if ((ch & 0xf0) == 0xe0) { // 1110 xxxx : 3 bytes
            return 3;
        } else if ((ch & 0xf8) == 0xf0) { // 1111 0xxx : 4 bytes
            return 4;
        } else if ((ch & 0xfc) == 0xf8) { // 1111 10xx : 5 bytes
            return 5;
        } else if ((ch & 0xfe) == 0xfc) { // 1111 110x : 6 bytes
            return 6;
        }
        return 0;
    }
    
    int Peek();
    
    int MoveNext() {
        buffer_position_++;
        row_++;
        return Peek();
    }
    
    base::Arena *arena_;
    SyntaxFeedback *error_feedback_ = nullptr;
    SequentialFile *input_file_ = nullptr;
    bool input_file_ownership_ = false;
    std::stack<TemplateKind> in_string_template_;
    int line_ = 0;
    int row_ = 0;
    uint64_t available_ = 0; // How many bytes can read
    size_t buffer_position_ = 0; // Position for 'buffered_'
    std::string_view buffered_; // Buffered data
    std::string scratch_; // Scratch for 'buffered_'
}; // class Lexer


} // namespace lang

} // namespace mai

#endif // MAI_LANG_LEXER_H_
