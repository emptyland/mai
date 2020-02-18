#pragma once
#ifndef MAI_LANG_PARSER_H_
#define MAI_LANG_PARSER_H_

#include "lang/token.h"
#include "lang/syntax.h"

namespace mai {
class SequentialFile;
namespace lang {

class Lexer;
class FileUnit;
class ImportStatement;
class VariableDeclaration;
class TypeSign;
class StringTemplateExpression;
class Expression;


class Parser final {
public:
    Parser(base::Arena *arena, SyntaxFeedback *error_feedback);
    
    Error SwitchInputFile(const std::string &name, SequentialFile *file);
    
    FileUnit *Parse(bool *ok);
    const ASTString *ParseStrictIdentifier(bool *ok);
    const ASTString *ParseIdentifier(bool *ok);
    const ASTString *ParseAlias(bool *ok);
    const ASTString *ParsePackageStatement(bool *ok);
    ImportStatement *ParseImportBlock(bool *ok);
    ImportStatement *ParseImportStatement(SourceLocation *loc, bool *ok);
    VariableDeclaration *ParseVariableDeclaration(bool *ok);
    TypeSign *ParseTypeSign(bool *ok);
    Expression *ParseExpression(int limit, bool *ok);
    Expression *ParseSimple(bool *ok);
    Expression *ParseSuffixed(bool *ok);
    Expression *ParsePrimary(bool *ok);
    StringTemplateExpression *ParseStringTemplate(bool *ok);
private:
    const Token &Peek() const { return lookahead_; }
    void Match(Token::Kind kind, bool *ok);
    bool Test(Token::Kind kind);

    base::Arena *arena_; // Memory arena allocator
    SyntaxFeedback *error_feedback_; // Error feedback interface
    std::unique_ptr<Lexer> lexer_; // Lexer of parser
    Token lookahead_ = Token(Token::kError, {}); // Look a head token
    FileUnit *file_unit_ = nullptr;
}; // class Parser


} // namespace lang

} // namespace mai

#endif // MAI_LANG_PARSER_H_
