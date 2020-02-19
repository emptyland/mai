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
            init = ParseExpression(CHECK_OK);
        }
    } else {
        Match(Token::kAssign, CHECK_OK);
        loc.LinkEnd(lookahead_.source_location());
        init = ParseExpression(CHECK_OK);
    }

    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) VariableDeclaration(position, kind, name, type, init);
}

TypeSign *Parser::ParseTypeSign(bool *ok) {
    SourceLocation loc = Peek().source_location();
    switch (Peek().kind()) {
    #define DEFINE_TYPE_CASE(name, ...) case Token::k##name: { \
            MoveNext(); \
            int position = file_unit_->InsertSourceLocation(loc); \
            return new (arena_) TypeSign(position, Token::k##name); \
        }
        DECLARE_BASE_TYPE_TOKEN(DEFINE_TYPE_CASE)
    #undef DEFINE_TYPE_CASE
            
        case Token::kIdentifier: {
            const ASTString *name = ParseIdentifier(CHECK_OK);
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) TypeSign(position, name);
        } break;

        // array := `array' `[' type `]'
        case Token::kArray: {
            MoveNext();
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
            MoveNext();
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
            MoveNext();
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
            MoveNext();
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
            MoveNext();
            TypeSign *elem_type = nullptr;
            if (Test(Token::kLBrack)) {
                elem_type = ParseTypeSign(CHECK_OK);
                loc.LinkEnd(Peek().source_location());
                Match(Token::kRBrack, CHECK_OK);
            }
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) TypeSign(arena_, position, Token::kArray, elem_type);
        } break;
            
        case Token::kFun: {
            MoveNext();
            SourceLocation end;
            FunctionPrototype *proto = ParseFunctionPrototype(false/*requrie_param_name*/, &end,
                                                              CHECK_OK);
            loc.LinkEnd(end);
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) TypeSign(position, proto);
        } break;

        default:
            error_feedback_->Printf(Peek().source_location(), "Unexpected type sign, expected: %s",
                                    Peek().ToString().c_str());
            *ok = false;
            return nullptr;
    }
}

FunctionPrototype *Parser::ParseFunctionPrototype(bool requrie_param_name, SourceLocation *loc,
                                                  bool *ok) {
    Match(Token::kLParen, CHECK_OK);
    
    FunctionPrototype *proto = new (arena_) FunctionPrototype(arena_);
    const ASTString *name = nullptr;
    if (Test(Token::kVargs)) {
        proto->set_vargs(true);
        *loc = Peek().source_location();
        Match(Token::kRParen, CHECK_OK);

        if (Test(Token::kColon)) {
            TypeSign *type = ParseTypeSign(CHECK_OK);
            proto->set_return_type(type);
            *loc = file_unit_->FindSourceLocation(type);
        }
        return proto;
    }

    if (requrie_param_name) {
        name = ParseIdentifier(CHECK_OK);
        Match(Token::kColon, CHECK_OK);
    }
    TypeSign *param = ParseTypeSign(CHECK_OK);
    proto->InsertParameter(name, param);
    while (Test(Token::kComma)) {
        if (Test(Token::kVargs)) {
            proto->set_vargs(true);
            break;
        }
        if (requrie_param_name) {
            name = ParseIdentifier(CHECK_OK);
            Match(Token::kColon, CHECK_OK);
        }
        param = ParseTypeSign(CHECK_OK);
        proto->InsertParameter(nullptr, param);
    }

    *loc = Peek().source_location();
    Match(Token::kRParen, CHECK_OK);

    if (Test(Token::kColon)) {
        TypeSign *type = ParseTypeSign(CHECK_OK);
        proto->set_return_type(type);
        *loc = file_unit_->FindSourceLocation(type);
    }
    
    return proto;
}

static Operator GetUnaryOp(Token::Kind kind) {
    switch (kind) {
        case Token::kNot:
            return Operators::kNot;
        case Token::kWave:
            return Operators::kBitwiseNot;
        case Token::kMinus:
            return Operators::kMinus;
        case Token::kLArrow:
            return Operators::kRecv;
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
        case Token::kLArrow:
            return Operators::kSend;
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

Expression *Parser::ParseExpression(int limit, Operator *receiver, bool *ok) {
    SourceLocation loc = Peek().source_location();
    Expression *expr = nullptr;
    Operator op = GetUnaryOp(Peek().kind());
    if (op.kind != Operator::NOT_UNARY) {
        Match(Peek().kind(), CHECK_OK);
        Expression *operand = ParseExpression(Operators::kUnaryPrio, nullptr, CHECK_OK);
        loc.LinkEnd(file_unit_->FindSourceLocation(operand));
        int position = file_unit_->InsertSourceLocation(loc);
        return new (arena_) UnaryExpression(position, op, operand);
    } else {
        expr = ParseSimple(CHECK_OK);
    }
    op = GetBinaryOp(Peek().kind());
    while (op.kind != Operator::NOT_BINARY && op.left > limit) {
        MoveNext();
        Operator next_op;
        Expression *rhs = ParseExpression(op.right, &next_op, CHECK_OK);
        
        loc.LinkEnd(file_unit_->FindSourceLocation(rhs));
        int position = file_unit_->InsertSourceLocation(loc);
        expr = new (arena_) BinaryExpression(position, op, expr, rhs);
        op = next_op;
    }
    if (receiver) { *receiver = op; }
    return expr;
}

Expression *Parser::ParseSimple(bool *ok) {
    SourceLocation loc = Peek().source_location();
    switch (Peek().kind()) {
        case Token::kNil: {
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) NilLiteral(position, nullptr, nullptr);
        } break;

        case Token::kTrue: {
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) BoolLiteral(position, nullptr, true);
        } break;

        case Token::kFalse: {
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) BoolLiteral(position, nullptr, false);
        } break;

        case Token::kStringLine:
        case Token::kStringBlock: {
            const ASTString *literal = Peek().text_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) StringLiteral(position, nullptr, literal);
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
                Expression *index = ParseExpression(CHECK_OK);
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
                    Expression *arg = ParseExpression(CHECK_OK);
                    argv.push_back(arg);
                    while (Test(Token::kComma)) {
                        arg = ParseExpression(CHECK_OK);
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
            if (Test(Token::kMore)) { // `(' `..' -> expr `)'
                Match(Token::kRArrow, CHECK_OK);
                Expression *value = ParseExpression(CHECK_OK);
                loc.LinkEnd(Peek().source_location());
                Match(Token::kRParen, CHECK_OK);
                int position = file_unit_->InsertSourceLocation(loc);
                return new (arena_) PairExpression(position, value);
            }
            Expression *expr = ParseExpression(CHECK_OK);
            if (Test(Token::kRArrow)) { // `(' expr -> expr `)'
                Expression *value = ParseExpression(CHECK_OK);
                loc.LinkEnd(Peek().source_location());
                Match(Token::kRParen, CHECK_OK);
                int position = file_unit_->InsertSourceLocation(loc);
                return new (arena_) PairExpression(position, expr, value);
            }
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
            return new (arena_) IntLiteral(position, nullptr, val);
        } break;
            
        case Token::kUIntVal: {
            uint32_t val = Peek().u32_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) UIntLiteral(position, nullptr, val);
        } break;

        // TODO:

        case Token::kF32Val: {
            float val = Peek().f32_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) F32Literal(position, nullptr, val);
        } break;
            
        case Token::kF64Val: {
            double val = Peek().f64_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) F64Literal(position, nullptr, val);
        } break;
            
        case Token::kArray:
        case Token::kMutableArray: {
            return ParseArrayInitializer(ok);
        } break;
            
        case Token::kMap:
        case Token::kMutableMap: {
            return ParseMapInitializer(ok);
        } break;

        default:
            error_feedback_->Printf(Peek().source_location(), "Unexpected");
            *ok = false;
            return nullptr;
    }
}

ArrayInitializer *Parser::ParseArrayInitializer(bool *ok) {
    SourceLocation loc = Peek().source_location();
    bool mut = true;
    if (Test(Token::kArray)) {
        mut = false;
    } else {
        Match(Token::kMutableArray, CHECK_OK);
    }

    TypeSign *element_type = nullptr;
    if (Test(Token::kLBrack)) { // `['
        element_type = ParseTypeSign(CHECK_OK);
        Match(Token::kRBrack, CHECK_OK);
    }

    if (Test(Token::kLParen)) { // `('
        Expression *reserve = ParseExpression(CHECK_OK);
        loc.LinkEnd(Peek().source_location());
        int position = file_unit_->InsertSourceLocation(loc);
        Match(Token::kRParen, CHECK_OK);
        return new (arena_) ArrayInitializer(arena_, position, mut, element_type, reserve);
    }

    std::vector<Expression *> elements;
    Match(Token::kLBrace, CHECK_OK); // `{'
    loc.LinkEnd(Peek().source_location());
    if (!Test(Token::kRBrace)) {
        Expression *elem = ParseExpression(CHECK_OK);
        elements.push_back(elem);
        while (Test(Token::kComma)) {
            elem = ParseExpression(CHECK_OK);
            elements.push_back(elem);
        }
        loc.LinkEnd(Peek().source_location());
        Match(Token::kRBrace, CHECK_OK); // `}'
    }

    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) ArrayInitializer(arena_, position, mut, element_type, elements);
}

MapInitializer *Parser::ParseMapInitializer(bool *ok) {
    SourceLocation loc = Peek().source_location();
    bool mut = true;
    if (Test(Token::kMap)) {
        mut = false;
    } else {
        Match(Token::kMutableMap, CHECK_OK);
    }
    
    TypeSign *key_type = nullptr, *value_type = nullptr;
    if (Test(Token::kLBrack)) { // `['
        key_type = ParseTypeSign(CHECK_OK);
        Match(Token::kComma, CHECK_OK);
        value_type = ParseTypeSign(CHECK_OK);
        Match(Token::kRBrack, CHECK_OK);
    }
    
    // map[string, int](128, 1.2)
    if (Test(Token::kLParen)) { // `('
        Expression *reserve = ParseExpression(CHECK_OK);
        Expression *load_factor = nullptr;
        if (Test(Token::kComma)) {
            load_factor = ParseExpression(CHECK_OK);
        }

        loc.LinkEnd(Peek().source_location());
        int position = file_unit_->InsertSourceLocation(loc);
        Match(Token::kRParen, CHECK_OK);
        return new (arena_) MapInitializer(arena_, position, mut, key_type, value_type, reserve,
                                           load_factor);
    }

    std::vector<Expression *> pairs;
    Match(Token::kLBrace, CHECK_OK); // `{'
    loc.LinkEnd(Peek().source_location());
    if (!Test(Token::kRBrace)) {
        Expression *pair = ParsePairExpression(CHECK_OK);
        
        pairs.push_back(pair);
        while (Test(Token::kComma)) {
            pair = ParsePairExpression(CHECK_OK);
            pairs.push_back(pair);
        }
        loc.LinkEnd(Peek().source_location());
        Match(Token::kRBrace, CHECK_OK); // `}'
    }
    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) MapInitializer(arena_, position, mut, key_type, value_type, pairs);
}

PairExpression *Parser::ParsePairExpression(bool *ok) {
    SourceLocation loc = Peek().source_location();
    Expression *key = ParseExpression(CHECK_OK);
    Match(Token::kRArrow, CHECK_OK);
    Expression *value = ParseExpression(CHECK_OK);
    loc.LinkEnd(file_unit_->FindSourceLocation(value));
    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) PairExpression(position, key, value);
}

StringTemplateExpression *Parser::ParseStringTemplate(bool *ok) {
    SourceLocation loc = Peek().source_location();

    const ASTString *literal = Peek().text_val();
    MoveNext();
    int position = file_unit_->InsertSourceLocation(loc);
    Expression *prefix = new (arena_) StringLiteral(position, nullptr, literal);
    std::vector<Expression *> parts;
    parts.push_back(prefix);
    
    for (;;) {
        switch (Peek().kind()) {
            case Token::kStringTempletePart: {
                int position = file_unit_->InsertSourceLocation(Peek().source_location());
                const ASTString *literal = Peek().text_val();
                MoveNext();
                Expression *part = new (arena_) StringLiteral(position, nullptr, literal);
                parts.push_back(part);
            } break;
                
            case Token::kIdentifier: {
                Expression *part = ParseExpression(CHECK_OK);
                parts.push_back(part);
            } break;
                
            case Token::kStringTempleteExpressBegin: {
                MoveNext();
                Expression *part = ParseExpression(CHECK_OK);
                parts.push_back(part);
                Match(Token::kStringTempleteExpressEnd, CHECK_OK);
            } break;

            case Token::kStringTempleteSuffix: {
                int position = file_unit_->InsertSourceLocation(Peek().source_location());
                const ASTString *literal = Peek().text_val();
                MoveNext();
                if (literal->size() > 0) {
                    Expression *part = new (arena_) StringLiteral(position, nullptr, literal);
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
