#pragma once
#ifndef MAI_LANG_LEXER_H_
#define MAI_LANG_LEXER_H_

#include "lang/token.h"
#include "lang/parser-utils.h"
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
    
    static bool IsTerm(int ch);
    
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
    uint64_t available_ = 0;
    size_t buffer_position_ = 0; // Position for 'buffered_'
    std::string_view buffered_; // Buffered data
    std::string scratch_; // Scratch for 'buffered_'
    std::string string_template_buffer_; // buffer for string-template
}; // class Lexer


} // namespace lang

} // namespace mai

#endif // MAI_LANG_LEXER_H_
