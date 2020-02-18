#include "lang/parser.h"
#include "lang/lexer.h"
#include "lang/ast.h"

namespace mai {

namespace lang {

#define MoveNext() lookahead_ = lexer_->Next()

Parser::Parser(base::Arena *arena, SyntaxFeedback *error_feedback)
    : arena_(arena)
    , error_feedback_(error_feedback)
    , lexer_(new Lexer(arena, error_feedback)) {
}

Error Parser::SwitchInputFile(const std::string &name, SequentialFile *file) {
    if (auto rs = lexer_->SwitchInputFile(name, file); rs.fail()) {
        return rs;
    }
    lookahead_ = lexer_->Next();
    file_unit_ = new (arena_) FileUnit(arena_);
    file_unit_->set_file_name(ASTString::New(arena_, name.c_str()));
    return Error::OK();
}

FileUnit *Parser::Parse(bool *ok) {
    while (lookahead_.kind() != Token::kEOF) {
        Token token = Peek();
        switch (token.kind()) {

            case Token::kPackage: {
                const ASTString *name = ParsePackageStatement(CHECK_OK);
                file_unit_->set_package_name(name);
            } break;

            case Token::kImport: {
                ParseImportBlock(CHECK_OK);
            } break;

            case Token::kClass: {
                TODO();
            } break;

            case Token::kObject: {
                TODO();
            } break;

            case Token::kInterface: {
                TODO();
            } break;

            case Token::kImplements: {
                TODO();
            } break;
                
            case Token::kNative: {
                TODO();
            } break;
                
            case Token::kDef: {
                TODO();
            } break;

            case Token::kVal:
            case Token::kVar: {
                VariableDeclaration *decl = ParseVariableDeclaration(CHECK_OK);
                file_unit_->InsertGlobalVariable(decl);
            } break;

            default: {
                error_feedback_->Printf(lookahead_.source_location(), "Unexpected token %s",
                                        lookahead_.ToString().c_str());
                *ok = false;
                return nullptr;
            }
        }
    }
    return file_unit_;
}

// package_statement := `package' identifier
const ASTString *Parser::ParsePackageStatement(bool *ok) {
    if (file_unit_->package_name() != nullptr) {
        error_feedback_->Printf(lookahead_.source_location(), "Duplicated package name");
        *ok = false;
        return nullptr;
    }
    Match(Token::kPackage, CHECK_OK);
    const ASTString *name = ParseStrictIdentifier(CHECK_OK);
    return name;
}

// import_stmt := `import' identifier { `.' identifier } [ `as' alias ]
// import_stmt := `import' `{' { identifier { `.' identifier } [ `as' alias ] } `}'
// alias := `*' | identifier
ImportStatement *Parser::ParseImportBlock(bool *ok) {
    SourceLocation loc = Peek().source_location();
    Match(Token::kImport, CHECK_OK);

    // Single import statement
    if (Peek().kind() == Token::kIdentifier) {
        ImportStatement *stmt = ParseImportStatement(&loc, CHECK_OK);
        file_unit_->InsertImportStatement(stmt);
        return stmt;
    }

    Match(Token::kLBrace, CHECK_OK);
    
    ImportStatement *stmt = nullptr;
    while (!Test(Token::kRBrace)) {
        SourceLocation loc = Peek().source_location();
        ImportStatement *stmt = ParseImportStatement(&loc, CHECK_OK);
        file_unit_->InsertImportStatement(stmt);
    }

    return stmt;
}

ImportStatement *Parser::ParseImportStatement(SourceLocation *loc, bool *ok) {
    std::string buf;
    const ASTString *part = ParseIdentifier(CHECK_OK);
    buf.append(part->ToString());
    while (Test(Token::kDot)) {
        loc->LinkEnd(Peek().source_location());
        part = ParseIdentifier(CHECK_OK);
        buf.append(1, '.');
        buf.append(part->ToString());
    }
    
    const ASTString *alias = nullptr;
    if (Test(Token::kAs)) {
        loc->LinkEnd(Peek().source_location());
        alias = ParseAlias(CHECK_OK);
    }

    int position = file_unit_->InsertSourceLocation(*loc);
    return new (arena_) ImportStatement(position, alias, ASTString::New(arena_, buf.c_str()));
}

// variable_declaration := <`val'|`var'> identifier [`:' type [= initializer]]
VariableDeclaration *Parser::ParseVariableDeclaration(bool *ok) {
    SourceLocation loc = lookahead_.source_location();
    
    VariableDeclaration::Kind kind;
    if (Test(Token::kVal)) {
        kind = VariableDeclaration::VAL;
    } else {
        Match(Token::kVar, CHECK_OK);
        kind = VariableDeclaration::VAR;
    }

    const ASTString *name = ParseIdentifier(CHECK_OK);
    TypeSign *type = nullptr;
    if (Test(Token::kColon)) {
        loc.LinkEnd(lookahead_.source_location());
        type = ParseTypeSign(CHECK_OK);
    }
    
    Expression *init = nullptr;
    if (kind == VariableDeclaration::VAR) {
        if (Test(Token::kAssign)) {
            loc.LinkEnd(lookahead_.source_location());
            init = ParseExpression(0, CHECK_OK);
        }
    } else {
        Match(Token::kAssign, CHECK_OK);
        loc.LinkEnd(lookahead_.source_location());
        init = ParseExpression(0, CHECK_OK);
    }

    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) VariableDeclaration(position, kind, name, type, init);
}

#define DEFINE_BASE_TYPE_SIGN(name, ...) static TypeSign kType##name(Token::k##name);
    DECLARE_BASE_TYPE_TOKEN(DEFINE_BASE_TYPE_SIGN)
#undef DEFINE_BASE_TYPE_SIGN

TypeSign *Parser::ParseTypeSign(bool *ok) {
    SourceLocation loc = Peek().source_location();
    switch (Peek().kind()) {
    #define DEFINE_TYPE_CASE(name, ...) case Token::k##name: return &kType##name;
        DECLARE_BASE_TYPE_TOKEN(DEFINE_TYPE_CASE)
    #undef DEFINE_TYPE_CASE
            
        case Token::kIdentifier: {
            const ASTString *name = ParseIdentifier(CHECK_OK);
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) TypeSign(arena_, position, Token::kIdentifier, name);
        } break;

        // array := `array' `[' type `]'
        case Token::kArray: {
            Match(Token::kArray, CHECK_OK);
            TypeSign *elem_type = nullptr;
            if (Test(Token::kLBrack)) {
                elem_type = ParseTypeSign(CHECK_OK);
                loc.LinkEnd(Peek().source_location());
                Match(Token::kRBrack, CHECK_OK);
            }
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) TypeSign(arena_, position, Token::kArray, elem_type);
        } break;
        
        // mutable_array := `mutable_array' `[' type `]'
        case Token::kMutableArray: {
            Match(Token::kMutableArray, CHECK_OK);
            TypeSign *elem_type = nullptr;
            if (Test(Token::kLBrack)) {
                elem_type = ParseTypeSign(CHECK_OK);
                loc.LinkEnd(Peek().source_location());
                Match(Token::kRBrack, CHECK_OK);
            }
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) TypeSign(arena_, position, Token::kMutableArray, elem_type);
        } break;
        
        // map := `map' `[' type `,' type `]'
        case Token::kMap: {
            Match(Token::kMap, CHECK_OK);
            TypeSign *key_type = nullptr, *val_type = nullptr;
            if (Test(Token::kLBrack)) {
                key_type = ParseTypeSign(CHECK_OK);
                Match(Token::kComma, CHECK_OK);
                val_type = ParseTypeSign(CHECK_OK);
                loc.LinkEnd(Peek().source_location());
                Match(Token::kRBrack, CHECK_OK);
            }
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) TypeSign(arena_, position, Token::kMap, key_type, val_type);
        } break;
        
        // mutable := `mutable' `[' type `,' type `]'
        case Token::kMutableMap: {
            Match(Token::kMutableMap, CHECK_OK);
            TypeSign *key_type = nullptr, *val_type = nullptr;
            if (Test(Token::kLBrack)) {
                key_type = ParseTypeSign(CHECK_OK);
                Match(Token::kComma, CHECK_OK);
                val_type = ParseTypeSign(CHECK_OK);
                loc.LinkEnd(Peek().source_location());
                Match(Token::kRBrack, CHECK_OK);
            }
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) TypeSign(arena_, position, Token::kMutableMap, key_type, val_type);
        } break;

        case Token::kChannel: {
            Match(Token::kChannel, CHECK_OK);
            TypeSign *elem_type = nullptr;
            if (Test(Token::kLBrack)) {
                elem_type = ParseTypeSign(CHECK_OK);
                loc.LinkEnd(Peek().source_location());
                Match(Token::kRBrack, CHECK_OK);
            }
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) TypeSign(arena_, position, Token::kArray, elem_type);
        } break;

        default:
            error_feedback_->Printf(Peek().source_location(), "Unexpected type sign, expected: %s",
                                    Peek().ToString().c_str());
            *ok = false;
            return nullptr;
    }
}

static Operator GetUnaryOp(Token::Kind kind) {
    switch (kind) {
        case Token::kNot:
            return Operators::kNot;
        case Token::kWave:
            return Operators::kBitwiseNot;
        case Token::kMinus:
            return Operators::kMinus;

        default:
            return Operators::NOT_UNARY;
    }
}

static Operator GetBinaryOp(Token::Kind kind) {
    switch (kind) {
        case Token::kPlus:
            return Operators::kAdd;
        case Token::kMinus:
            return Operators::kSub;
        case Token::kStar:
            return Operators::kMul;
        case Token::kDiv:
            return Operators::kDiv;
        case Token::kPercent:
            return Operators::kMod;
        case Token::kLShift:
            return Operators::kBitwiseShl;
        case Token::kRShift:
            return Operators::kBitwiseShr;
        case Token::kBitwiseAnd:
            return Operators::kBitwiseAnd;
        case Token::kBitwiseOr:
            return Operators::kBitwiseOr;
        case Token::kBitwiseXor:
            return Operators::kBitwiseXor;
        case Token::kLess:
            return Operators::kLess;
        case Token::kLessEqual:
            return Operators::kLessEqual;
        case Token::kGreater:
            return Operators::kGreater;
        case Token::kGreaterEqual:
            return Operators::kGreaterEqual;
        case Token::kAnd:
            return Operators::kAnd;
        case Token::kOr:
            return Operators::kOr;    
        default:
            return Operators::NOT_BINARY;
    }
}

Expression *Parser::ParseExpression(int limit, bool *ok) {
    SourceLocation loc = Peek().source_location();
    Expression *expr = nullptr;
    Operator op = GetUnaryOp(Peek().kind());
    if (op.kind != Operator::NOT_UNARY) {
        Match(Peek().kind(), CHECK_OK);
        Expression *operand = ParseExpression(Operators::kUnaryPrio, CHECK_OK);
        loc.LinkEnd(file_unit_->FindSourceLocation(operand));
        int position = file_unit_->InsertSourceLocation(loc);
        return new (arena_) UnaryExpression(position, op, operand);
    } else {
        expr = ParseSimple(CHECK_OK);
    }
    op = GetBinaryOp(Peek().kind());
    while (op.kind != Operator::NOT_BINARY && op.left > limit) {
        MoveNext();
        Expression *rhs = ParseExpression(op.right, CHECK_OK);
        
        loc.LinkEnd(file_unit_->FindSourceLocation(rhs));
        int position = file_unit_->InsertSourceLocation(loc);
        expr = new (arena_) BinaryExpression(position, op, expr, rhs);
        op = rhs->GetOperator();
    }
    return expr;
}

Expression *Parser::ParseSimple(bool *ok) {
    SourceLocation loc = Peek().source_location();
    switch (Peek().kind()) {
        case Token::kNil: {
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) NilLiteral(position, &kTypeAny, nullptr);
        } break;

        case Token::kTrue: {
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) BoolLiteral(position, &kTypeBool, true);
        } break;

        case Token::kFalse: {
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) BoolLiteral(position, &kTypeBool, false);
        } break;

        case Token::kStringLine:
        case Token::kStringBlock: {
            const ASTString *literal = Peek().text_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) StringLiteral(position, &kTypeString, literal);
        } break;

        case Token::kStringTempletePrefix: {
            return ParseStringTemplate(ok);
        } break;

        default:
            return ParseSuffixed(ok);
    }
}

Expression *Parser::ParseSuffixed(bool *ok) {
    SourceLocation loc = Peek().source_location();
    Expression *expr = ParsePrimary(CHECK_OK);
    for (;;) {
        switch (Peek().kind()) {
            case Token::kDot: { // .
                MoveNext();
                loc.LinkEnd(Peek().source_location());
                int position = file_unit_->InsertSourceLocation(loc);
                const ASTString *name = ParseIdentifier(CHECK_OK);
                expr = new (arena_) DotExpression(position, expr, name);
            } break;
                
            case Token::kLBrack: { // [
                MoveNext();
                Expression *index = ParseExpression(0, CHECK_OK);
                loc.LinkEnd(Peek().source_location());
                int position = file_unit_->InsertSourceLocation(loc);
                Match(Token::kRBrack, CHECK_OK);
                expr = new (arena_) IndexExpression(position, expr, index);
            } break;
                
            case Token::kLParen: { // (
                MoveNext();
                if (Peek().kind() == Token::kRParen) { // call()
                    loc.LinkEnd(Peek().source_location());
                    MoveNext();
                    int position = file_unit_->InsertSourceLocation(loc);
                    expr = new (arena_) CallExpression(arena_, position, {});
                } else {
                    std::vector<Expression *> argv;
                    Expression *arg = ParseExpression(0, CHECK_OK);
                    argv.push_back(arg);
                    while (Test(Token::kComma)) {
                        arg = ParseExpression(0, CHECK_OK);
                        argv.push_back(arg);
                    }
                    loc.LinkEnd(Peek().source_location());
                    Match(Token::kRParen, CHECK_OK);
                    int position = file_unit_->InsertSourceLocation(loc);
                    expr = new (arena_) CallExpression(arena_, position, argv);
                }
            } break;

            default:
                return expr;
        }
    }
}

Expression *Parser::ParsePrimary(bool *ok) {
    SourceLocation loc = Peek().source_location();
    switch (Peek().kind()) {
        case Token::kLParen: { // `(' expr `)'
            MoveNext();
            Expression *expr = ParseExpression(0, CHECK_OK);
            Match(Token::kRParen, CHECK_OK);
            return expr;
        } break;
            
        case Token::kIdentifier: {
            const ASTString *name = Peek().text_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) Identifier(position, name);
        } break;
            
        case Token::kIntVal: {
            int32_t val = Peek().i32_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) IntLiteral(position, &kTypeInt, val);
        } break;
            
        case Token::kUIntVal: {
            uint32_t val = Peek().u32_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) UIntLiteral(position, &kTypeUInt, val);
        } break;

        default:
            error_feedback_->Printf(Peek().source_location(), "Unexpected");
            *ok = false;
            return nullptr;
    }
}

StringTemplateExpression *Parser::ParseStringTemplate(bool *ok) {
    SourceLocation loc = Peek().source_location();

    const ASTString *literal = Peek().text_val();
    MoveNext();
    int position = file_unit_->InsertSourceLocation(loc);
    Expression *prefix = new (arena_) StringLiteral(position, &kTypeString, literal);
    std::vector<Expression *> parts;
    parts.push_back(prefix);
    
    for (;;) {
        switch (Peek().kind()) {
            case Token::kStringTempletePart: {
                int position = file_unit_->InsertSourceLocation(Peek().source_location());
                const ASTString *literal = Peek().text_val();
                MoveNext();
                Expression *part = new (arena_) StringLiteral(position, &kTypeString, literal);
                parts.push_back(part);
            } break;
                
            case Token::kIdentifier: {
                Expression *part = ParseExpression(0, CHECK_OK);
                parts.push_back(part);
            } break;
                
            case Token::kStringTempleteExpressBegin: {
                MoveNext();
                Expression *part = ParseExpression(0, CHECK_OK);
                parts.push_back(part);
                Match(Token::kStringTempleteExpressEnd, CHECK_OK);
            } break;

            case Token::kStringTempleteSuffix: {
                int position = file_unit_->InsertSourceLocation(Peek().source_location());
                const ASTString *literal = Peek().text_val();
                MoveNext();
                if (literal->size() > 0) {
                    Expression *part = new (arena_) StringLiteral(position, &kTypeString, literal);
                    parts.push_back(part);
                }
            } goto done;

            default:
                NOREACHED();
                break;
        }
    }
done:
    return new (arena_) StringTemplateExpression(arena_, position, parts);
}

const ASTString *Parser::ParseStrictIdentifier(bool *ok) {
    const ASTString *name = ParseIdentifier(CHECK_OK);
    for (size_t i = 0; i < name->size(); i++) {
        int ch = name->data()[i];
        if (!isdigit(ch) && !isalpha(ch) && ch != '_') {
            error_feedback_->Printf(lookahead_.source_location(), "Incorrect strict name: %s",
                                    name->data());
            *ok = false;
            return nullptr;
        }
    }
    return name;
}

const ASTString *Parser::ParseIdentifier(bool *ok) {
    if (lookahead_.kind() != Token::kIdentifier) {
        error_feedback_->Printf(lookahead_.source_location(), "Unexpected: Identifier, expected: %s",
                                lookahead_.ToString().c_str());
        *ok = false;
        return nullptr;
    }
    const ASTString *text = lookahead_.text_val();
    lookahead_ = lexer_->Next();
    return text;
}

const ASTString *Parser::ParseAlias(bool *ok) {
    if (Peek().kind() == Token::kIdentifier) {
        const ASTString *name = ParseStrictIdentifier(CHECK_OK);
        return name;
    } else if (Peek().kind() == Token::kStar) {
        lookahead_ = lexer_->Next();
        return ASTString::New(arena_, "*");
    }

    error_feedback_->Printf(lookahead_.source_location(), "Unexpected: Alias, expected: %s",
                            lookahead_.ToString().c_str());
    *ok = false;
    return nullptr;
}

void Parser::Match(Token::Kind kind, bool *ok) {
    if (lookahead_.kind() != kind) {
        error_feedback_->Printf(lookahead_.source_location(), "Unexpected: %s, expected: %s",
                                Token::ToString(kind).c_str(), lookahead_.ToString().c_str());
        *ok = false;
        return;
    }
    lookahead_ = lexer_->Next();
}

bool Parser::Test(Token::Kind kind) {
    if (lookahead_.kind() == kind) {
        lookahead_ = lexer_->Next();
        return true;
    }
    return false;
}

} // namespace lang

} // namespace mai
