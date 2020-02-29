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
class FunctionDefinition;
class ClassDefinition;
class ObjectDefinition;
class InterfaceDefinition;
class ClassImplementsBlock;
class TypeSign;
class StringTemplateExpression;
class Expression;
class IfExpression;
class Statement;
class StatementBlock;
class WhileLoop;
class FunctionPrototype;
class ArrayInitializer;
class MapInitializer;
class PairExpression;
class LambdaLiteral;
struct IncompleteClassDefinition;

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
    FunctionDefinition *ParseFunctionDefinition(bool *ok);
    InterfaceDefinition *ParseInterfaceDefinition(bool *ok);
    ObjectDefinition *ParseObjectDefinition(bool *ok);
    ClassDefinition *ParseClassDefinition(bool *ok);
    ClassImplementsBlock *ParseClassImplementsBlock(bool *ok);
    Statement *ParseStatement(bool *ok);
    StatementBlock *ParseStatementBlock(bool *ok);
    WhileLoop *ParseWhileLoop(bool *ok);
    Statement *ParseAssignmentOrExpression(bool *ok);
    TypeSign *ParseTypeSign(bool *ok);
    FunctionPrototype *ParseFunctionPrototype(bool requrie_param_name, SourceLocation *loc, bool *ok);
    Expression *ParseExpression(bool *ok) { return ParseExpression(0, nullptr, ok); }
    Expression *ParseExpression(int limit, Operator *op, bool *ok);
    Expression *ParseSimple(bool *ok);
    Expression *ParseSuffixed(bool *ok);
    Expression *ParsePrimary(bool *ok);
    StringTemplateExpression *ParseStringTemplate(bool *ok);
    ArrayInitializer *ParseArrayInitializer(bool *ok);
    MapInitializer *ParseMapInitializer(bool *ok);
    PairExpression *ParsePairExpression(bool *ok);
    LambdaLiteral *ParseLambdaLiteral(bool *ok);
    IfExpression *ParseIfExpression(bool *ok);
private:
    void *ParseConstructor(IncompleteClassDefinition *def, bool *ok);
    const ASTString *ParseDotName(SourceLocation *loc, bool *ok);
    
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
