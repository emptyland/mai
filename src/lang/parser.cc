#include "lang/parser.h"
#include "lang/lexer.h"
#include "lang/ast.h"

namespace mai {

namespace lang {

#define CHECK_OK ok); if (!*ok) { return 0; } ((void)0

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
                Definition *def = ParseClassDefinition(CHECK_OK);
                file_unit_->InsertDefinition(def);
            } break;

            case Token::kObject: {
                Definition *def = ParseObjectDefinition(CHECK_OK);
                file_unit_->InsertDefinition(def);
            } break;

            case Token::kInterface: {
                Definition *def = ParseInterfaceDefinition(CHECK_OK);
                file_unit_->InsertDefinition(def);
            } break;

            case Token::kImplements: {
                ClassImplementsBlock *impl = ParseClassImplementsBlock(CHECK_OK);
                file_unit_->InsertImplement(impl);
            } break;
                
            case Token::kDollar: {
                Statement *stmt = ParseStatementWithAttributes(CHECK_OK);
                if (stmt->IsObjectDefinition() || stmt->IsClassDefinition() ||
                    stmt->IsFunctionDefinition()) {
                    file_unit_->InsertDefinition(static_cast<Definition *>(stmt));
                } else if (stmt->IsVariableDeclaration()) {
                    file_unit_->InsertGlobalVariable(static_cast<Declaration *>(stmt));
                } else {
                    NOREACHED();
                }
            } break;

            case Token::kNative: {
                Definition *def = ParseFunctionDefinition(0, CHECK_OK);
                file_unit_->InsertDefinition(def);
            } break;

            case Token::kFun: {
                Definition *def = ParseFunctionDefinition(0, CHECK_OK);
                file_unit_->InsertDefinition(def);
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
    const ASTString *original = ParseDotName(loc, CHECK_OK);

    const ASTString *alias = nullptr;
    if (Test(Token::kAs)) {
        loc->LinkEnd(Peek().source_location());
        alias = ParseAlias(CHECK_OK);
    }

    int position = file_unit_->InsertSourceLocation(*loc);
    return new (arena_) ImportStatement(position, alias, original);
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

FunctionDefinition *Parser::ParseFunctionDefinition(uint32_t attributes, bool *ok) {
    SourceLocation loc = Peek().source_location();

    bool native = Test(Token::kNative);
    Match(Token::kFun, CHECK_OK);
    
    const ASTString *name = ParseIdentifier(CHECK_OK);
    if (native) {
        SourceLocation end;
        FunctionPrototype *proto = ParseFunctionPrototype(true, &end, CHECK_OK);
        loc.LinkEnd(end);
        int position = file_unit_->InsertSourceLocation(loc);
        return new (arena_) FunctionDefinition(position, attributes, native, name, proto);
    } else {
        LambdaLiteral *body = ParseLambdaLiteral(CHECK_OK);
        loc.LinkEnd(file_unit_->FindSourceLocation(body));
        int position = file_unit_->InsertSourceLocation(loc);
        return new (arena_) FunctionDefinition(position, attributes, name, body);
    }
}

InterfaceDefinition *Parser::ParseInterfaceDefinition(bool *ok) {
    SourceLocation loc = Peek().source_location();
    Match(Token::kInterface, CHECK_OK);
    const ASTString *name = ParseIdentifier(CHECK_OK);
    Match(Token::kLBrace, CHECK_OK);
    
    base::ArenaVector<FunctionDefinition *> methods(arena_);
    base::ArenaMap<std::string_view, FunctionDefinition *> named_methods(arena_);
    
    while (!Test(Token::kRBrace)) {
        const ASTString *method_name = ParseIdentifier(CHECK_OK);
        if (auto iter = named_methods.find(method_name->ToSlice()); iter != named_methods.end()) {
            *ok = false;
            error_feedback_->Printf(loc, "Duplicated interface method: %s", method_name->data());
            return nullptr;
        }
        
        SourceLocation end;
        FunctionPrototype *proto = ParseFunctionPrototype(true, &end, CHECK_OK);
        
        int position = file_unit_->InsertSourceLocation(end);
        FunctionDefinition *fun = new (arena_) FunctionDefinition(position, 0/*attributes*/,
                                                                  false/*native*/, method_name,
                                                                  proto);
        methods.push_back(fun);
        named_methods[method_name->ToSlice()] = fun;
        loc.LinkEnd(Peek().source_location());
    }

    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) InterfaceDefinition(position, name, std::move(methods),
                                            std::move(named_methods));
}

ObjectDefinition *Parser::ParseObjectDefinition(bool *ok) {
    SourceLocation loc = Peek().source_location();
    Match(Token::kObject, CHECK_OK);
    const ASTString *name = ParseIdentifier(CHECK_OK);
    Match(Token::kLBrace, CHECK_OK);
    
    base::ArenaVector<StructureDefinition::Field> fields(arena_);
    while (!Test(Token::kRBrace)) {
        StructureDefinition::Field field;
        field.in_constructor = false;
        field.access = StructureDefinition::kPublic;
        field.declaration = ParseVariableDeclaration(CHECK_OK);
        fields.push_back(field);
        
        loc.LinkEnd(Peek().source_location());
    }

    int position = file_unit_->InsertSourceLocation(loc);
    ObjectDefinition *object = new (arena_) ObjectDefinition(arena_, position, name,
                                                             std::move(fields));
    if (int err = object->MakeFieldLookupTable(); err < 0) {
        VariableDeclaration *decl = object->field(-err - 1).declaration;
        SourceLocation loc = file_unit_->FindSourceLocation(decl);
        error_feedback_->Printf(loc, "Duplicated object field: %s", decl->identifier()->data());
        *ok = false;
        return nullptr;
    }
    return object;
}

struct IncompleteClassDefinition {
    IncompleteClassDefinition(base::Arena *arena)
        : args(arena)
        , fields(arena)
        , params(arena) {}

    bool need_comma = false;
    const ASTString *name = nullptr;
    const ASTString *base_prefix = nullptr;
    const ASTString *base_name = nullptr;
    base::ArenaVector<Expression *> args;
    base::ArenaVector<ClassDefinition::Field> fields;
    base::ArenaVector<ClassDefinition::Parameter> params;
    ClassDefinition::Access access = ClassDefinition::kPublic;
};

ClassDefinition *Parser::ParseClassDefinition(bool *ok) {
    SourceLocation loc = Peek().source_location();
    Match(Token::kClass, CHECK_OK);
    
    IncompleteClassDefinition incomplete(arena_);
    incomplete.name = ParseIdentifier(CHECK_OK);
    
    if (Test(Token::kLParen)) { // `('
        incomplete.access = ClassDefinition::kPublic;
        
        while (!Test(Token::kRParen)) {
            ParseConstructor(&incomplete, CHECK_OK);
        }
    }
    
    if (Test(Token::kColon)) { // `:'
        incomplete.base_name = ParseIdentifier(CHECK_OK);
        if (Test(Token::kDot)) {
            incomplete.base_prefix = incomplete.base_name;
            incomplete.base_name = ParseIdentifier(CHECK_OK);
        }
        if (Test(Token::kLParen)) {
            if (!Test(Token::kRParen)) {
                Expression *arg = ParseExpression(CHECK_OK);
                incomplete.args.push_back(arg);
                while (Test(Token::kComma)) {
                    arg = ParseExpression(CHECK_OK);
                    incomplete.args.push_back(arg);
                }
                Match(Token::kRParen, CHECK_OK);
            }
        }
    }

    if (Test(Token::kLBrace)) {
        incomplete.access = ClassDefinition::kPublic;
        loc.LinkEnd(Peek().source_location());
        while (!Test(Token::kRBrace)) {
            switch (Peek().kind()) {
                case Token::kPublic:
                    MoveNext();
                    Match(Token::kColon, CHECK_OK);
                    incomplete.access = ClassDefinition::kPublic;
                    break;
                case Token::kProtected:
                    MoveNext();
                    Match(Token::kColon, CHECK_OK);
                    incomplete.access = ClassDefinition::kProtected;
                    break;
                case Token::kPrivate:
                    MoveNext();
                    Match(Token::kColon, CHECK_OK);
                    incomplete.access = ClassDefinition::kPrivate;
                    break;
                case Token::kVal:
                case Token::kVar: {
                    ClassDefinition::Field field;
                    field.access = incomplete.access;
                    field.as_constructor = -1;
                    field.in_constructor = false;
                    field.declaration = ParseVariableDeclaration(CHECK_OK);
                    incomplete.fields.push_back(field);
                } break;
                default:
                    error_feedback_->Printf(Peek().source_location(),
                                            "Unexpected class field, exprected: %s",
                                            Peek().ToString().c_str());
                    *ok = false;
                    return nullptr;
            }
            loc.LinkEnd(Peek().source_location());
        }
    }

    int position = file_unit_->InsertSourceLocation(loc);
    ClassDefinition *clazz = new (arena_) ClassDefinition(arena_, position, incomplete.name,
                                                          std::move(incomplete.params),
                                                          std::move(incomplete.fields),
                                                          incomplete.base_prefix,
                                                          incomplete.base_name,
                                                          std::move(incomplete.args));
    if (int err = clazz->MakeParameterLookupTable(); err < 0) {
        auto param = clazz->parameter(-err - 1);
        SourceLocation loc;
        const ASTString *name = nullptr;
        if (param.field_declaration) {
            loc = file_unit_->FindSourceLocation(clazz->field(param.as_field).declaration);
            name = clazz->field(param.as_field).declaration->identifier();
        } else {
            loc = file_unit_->FindSourceLocation(param.as_parameter);
            name = param.as_parameter->identifier();
        }
        error_feedback_->Printf(loc, "Duplicated constructor parameter: %s", name->data());
        *ok = false;
        return nullptr;
    }

    if (int err = clazz->MakeFieldLookupTable(); err < 0) {
        VariableDeclaration *decl = clazz->field(-err - 1).declaration;
        SourceLocation loc = file_unit_->FindSourceLocation(decl);
        error_feedback_->Printf(loc, "Duplicated class field: %s", decl->identifier()->data());
        *ok = false;
        return nullptr;
    }
    return clazz;
}

ClassImplementsBlock *Parser::ParseClassImplementsBlock(bool *ok) {
    SourceLocation loc = Peek().source_location();
    Match(Token::kImplements, CHECK_OK);
    const ASTString *prefix = nullptr;
    const ASTString *name = ParseIdentifier(CHECK_OK);
    if (Test(Token::kDot)) {
        prefix = name;
        name = ParseIdentifier(CHECK_OK);
    }
    
    base::ArenaVector<FunctionDefinition *> methods(arena_);
    Match(Token::kLBrace, CHECK_OK);
    while (!Test(Token::kRBrace)) {
        uint32_t attr = 0;
        if (Peek().kind() == Token::kDollar) {
            attr = ParseCompilingAttributes(CHECK_OK);
        }
        FunctionDefinition *fun = ParseFunctionDefinition(attr, CHECK_OK);
        methods.push_back(fun);
    }

    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) ClassImplementsBlock(position, prefix, name, std::move(methods));
}

void *Parser::ParseConstructor(IncompleteClassDefinition *def, bool *ok) {
    SourceLocation loc = Peek().source_location();
    if (def->need_comma) {
        Match(Token::kComma, CHECK_OK);
    }
    switch (Peek().kind()) {
        case Token::kPublic:
            MoveNext();
            Match(Token::kColon, CHECK_OK);
            def->access = ClassDefinition::kPublic;
            def->need_comma = false;
            break;
        case Token::kProtected:
            MoveNext();
            Match(Token::kColon, CHECK_OK);
            def->access = ClassDefinition::kProtected;
            def->need_comma = false;
            break;
        case Token::kPrivate:
            MoveNext();
            Match(Token::kColon, CHECK_OK);
            def->access = ClassDefinition::kPrivate;
            def->need_comma = false;
            break;
        case Token::kIdentifier: {
            ClassDefinition::Parameter param;
            const ASTString *name = ParseIdentifier(CHECK_OK);
            Match(Token::kColon, CHECK_OK);
            TypeSign *type = ParseTypeSign(CHECK_OK);
            loc.LinkEnd(file_unit_->FindSourceLocation(type));
            int position = file_unit_->InsertSourceLocation(loc);
            param.as_parameter = new (arena_) VariableDeclaration(position,
                                                                  VariableDeclaration::PARAMETER,
                                                                  name, type,
                                                                  nullptr/*initializer*/);
            param.field_declaration = false;
            def->params.push_back(param);
            def->need_comma = true;
        } break;
        case Token::kVal:
        case Token::kVar: {
            VariableDeclaration::Kind kind = Peek().kind() == Token::kVal ?
            VariableDeclaration::VAL : VariableDeclaration::VAR;
            MoveNext();
            const ASTString *field_name = ParseIdentifier(CHECK_OK);
            Match(Token::kColon, CHECK_OK);
            TypeSign *type = ParseTypeSign(CHECK_OK);
            
            ClassDefinition::Field field;
            field.access = def->access;
            field.in_constructor = true;
            field.as_constructor = static_cast<int>(def->params.size());
            loc.LinkEnd(file_unit_->FindSourceLocation(type));
            int position = file_unit_->InsertSourceLocation(loc);
            field.declaration = new (arena_) VariableDeclaration(position, kind, field_name, type,
                                                                 nullptr/*initializer*/);
            
            ClassDefinition::Parameter param;
            param.as_field = static_cast<int>(def->fields.size());
            param.field_declaration = true;
            
            def->fields.push_back(field);
            def->params.push_back(param);
            def->need_comma = true;
        } break;
        default:
            break;
    }
    return nullptr;
}

Statement *Parser::ParseStatement(bool *ok) {
    SourceLocation loc = Peek().source_location();
    switch (Peek().kind()) {
        case Token::kDollar:
            return ParseStatementWithAttributes(ok);
            
        case Token::kVar:
        case Token::kVal:
            return ParseVariableDeclaration(ok);

        case Token::kFun:
            return ParseFunctionDefinition(0, ok);

        case Token::kLBrace:
            return ParseStatementBlock(ok);

        case Token::kUnless:
        case Token::kWhile:
            return ParseWhileLoop(ok);
            
        case Token::kFor:
            return ParseForLoop(ok);

        case Token::kTry:
            return ParseTryCatchFinallyBlock(ok);

        case Token::kReturn: {
            MoveNext();
            if (Peek().kind() == Token::kRBrace || Peek().kind() == Token::kSemi) {
                loc.LinkEnd(Peek().source_location());
                MoveNext();
                int position = file_unit_->InsertSourceLocation(loc);
                return new (arena_) BreakableStatement(position, BreakableStatement::RETURN);
            } else {
                Expression *val = ParseExpression(CHECK_OK);
                loc.LinkEnd(file_unit_->FindSourceLocation(val));
                int position = file_unit_->InsertSourceLocation(loc);
                return new (arena_) BreakableStatement(position, BreakableStatement::RETURN, val);
            }
        } break;

        case Token::kThrow: {
            MoveNext();
            Expression *val = ParseExpression(CHECK_OK);
            loc.LinkEnd(file_unit_->FindSourceLocation(val));
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) BreakableStatement(position, BreakableStatement::THROW, val);
        } break;

        case Token::kBreak: {
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) BreakableStatement(position, BreakableStatement::BREAK);
        } break;

        case Token::kContinue: {
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) BreakableStatement(position, BreakableStatement::CONTINUE);
        } break;
            
        case Token::kRun:
        case Token::kRunS:
            return ParseRunStatement(ok);

        default:
            return ParseAssignmentOrExpression(ok);
    }
}

StatementBlock *Parser::ParseStatementBlock(bool *ok) {
    SourceLocation loc = Peek().source_location();
    Match(Token::kLBrace, CHECK_OK);
    base::ArenaVector<Statement *> stmts(arena_);
    
    loc.LinkEnd(Peek().source_location());
    while (!Test(Token::kRBrace)) {
        Statement *stmt = ParseStatement(CHECK_OK);
        stmts.push_back(stmt);
        loc.LinkEnd(Peek().source_location());
    }
    
    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) StatementBlock(position, std::move(stmts));
}

Statement *Parser::ParseStatementWithAttributes(bool *ok) {
    uint32_t attributes = ParseCompilingAttributes(CHECK_OK);
    switch (Peek().kind()) {
        case Token::kClass:
            return ParseClassDefinition(ok);
        case Token::kObject:
            return ParseObjectDefinition(ok);
        case Token::kFun:
        case Token::kNative:
            return ParseFunctionDefinition(attributes, ok);
        default:
            *ok = false;
            error_feedback_->Printf(Peek().source_location(), "Unexpected definition or declaration,"
                                    " expected %s", Peek().ToString().c_str());
            break;
    }
    return nullptr;
}

WhileLoop *Parser::ParseWhileLoop(bool *ok) {
    SourceLocation loc = Peek().source_location();
    bool unless = Test(Token::kUnless);
    if (!unless) {
        Match(Token::kWhile, CHECK_OK);
    }
    Match(Token::kLParen, CHECK_OK);
    Expression *condition = ParseExpression(CHECK_OK);
    if (unless) {
        condition = new (arena_) UnaryExpression(condition->position(), Operators::kNot, condition);
    }
    Match(Token::kRParen, CHECK_OK);
    
    base::ArenaVector<Statement *> stmts(arena_);
    Match(Token::kLBrace, CHECK_OK);
    loc.LinkEnd(Peek().source_location());
    while (!Test(Token::kRBrace)) {
        Statement *stmt = ParseStatement(CHECK_OK);
        stmts.push_back(stmt);
        loc.LinkEnd(Peek().source_location());
    }
    
    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) WhileLoop(position, condition, std::move(stmts));
}

bool IsPlaceholder(const ASTString *id) {
    return id->size() == 1 && id->data()[0] == '_';
}

// for (i in 0, 1000) {}
// for (i in <-ch) {}
// for (k, v in arr) {}
ForLoop *Parser::ParseForLoop(bool *ok) {
    SourceLocation loc = Peek().source_location();
    Match(Token::kFor, CHECK_OK);
    Match(Token::kLParen, CHECK_OK);

    VariableDeclaration *key = nullptr;
    SourceLocation iter_loc = Peek().source_location();
    const ASTString *key_id = ParseIdentifier(CHECK_OK);
    if (!IsPlaceholder(key_id)) {
        TypeSign *type = nullptr;
        if (Test(Token::kColon)) {
            type = ParseTypeSign(CHECK_OK);
        }
        iter_loc.LinkEnd(Peek().source_location());
        int position = file_unit_->InsertSourceLocation(iter_loc);
        key = new (arena_) VariableDeclaration(position, VariableDeclaration::VAL, key_id, type,
                                               nullptr/*initializer*/);
    }
    VariableDeclaration *value = nullptr;
    if (Test(Token::kComma)) {
        iter_loc = Peek().source_location();
        const ASTString *value_id = ParseIdentifier(CHECK_OK);
        if (!IsPlaceholder(value_id)) {
            TypeSign *type = nullptr;
            if (Test(Token::kColon)) {
                type = ParseTypeSign(CHECK_OK);
            }
            iter_loc.LinkEnd(Peek().source_location());
            int position = file_unit_->InsertSourceLocation(iter_loc);
            value = new (arena_) VariableDeclaration(position, VariableDeclaration::VAL, value_id,
                                                     type, nullptr/*initializer*/);
        }
    } else {
        DCHECK(value == nullptr);
        value = key;
        key = nullptr;
    }

    Match(Token::kIn, CHECK_OK);

    ForLoop::Control type = ForLoop::ITERATE;
    Expression *subject = ParseExpression(CHECK_OK);
    if (auto maybe = subject->AsUnaryExpression()) {
        if (maybe->op().kind == Operator::kRecv) {
            type = ForLoop::CHANNEL_ITERATE;
        }
    }

    Expression *limit = nullptr;
    if (Test(Token::kComma)) {
        type = ForLoop::STEP;
        limit = ParseExpression(CHECK_OK);
    }

    Match(Token::kRParen, CHECK_OK);

    base::ArenaVector<Statement *> stmts(arena_);
    Match(Token::kLBrace, CHECK_OK);
    loc.LinkEnd(Peek().source_location());
    while (!Test(Token::kRBrace)) {
        Statement *stmt = ParseStatement(CHECK_OK);
        stmts.push_back(stmt);
        loc.LinkEnd(Peek().source_location());
    }

    loc.LinkEnd(Peek().source_location());
    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) ForLoop(position, type, key, value, subject, limit, std::move(stmts));
}

Statement *Parser::ParseAssignmentOrExpression(bool *ok) {
    SourceLocation loc = Peek().source_location();
    Expression *lval = ParseExpression(CHECK_OK);
    switch (Peek().kind()) {
        case Token::kPlusEqual: // expr += expr
        case Token::kMinusEqual: // expr -= expr
        case Token::kAssign: { // expr = expr
            if (!lval->IsLValue()) {
                error_feedback_->Printf(loc, "Can not assign non-lval");
                *ok = false;
                return nullptr;
            }
        } break;
        default:
            return lval;
    }

    Operator op;
    switch (Peek().kind()) {
        case Token::kPlusEqual:
            op = Operators::kAdd;
            break;
        case Token::kMinusEqual:
            op = Operators::kSub;
            break;
        case Token::kAssign:
            op = Operators::NOT_OPERATOR;
            break;
        default:
            NOREACHED();
            return nullptr;
    }
    
    MoveNext();
    Expression *rval = ParseExpression(CHECK_OK);
    loc.LinkEnd(file_unit_->FindSourceLocation(rval));
    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) AssignmentStatement(position, op, lval, rval);
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
            const ASTString *prefix = nullptr;
            if (Test(Token::kDot)) {
                prefix = name;
                name = ParseIdentifier(CHECK_OK);
            }
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) TypeSign(position, prefix, name);
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
            Match(Token::kLBrack, CHECK_OK);
            TypeSign *value_type = ParseTypeSign(CHECK_OK);
            loc.LinkEnd(Peek().source_location());
            Match(Token::kRBrack, CHECK_OK);
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) TypeSign(arena_, position, Token::kChannel, value_type);
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
    if (!Test(Token::kRParen)) {
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
            proto->InsertParameter(name, param);
        }

        *loc = Peek().source_location();
        Match(Token::kRParen, CHECK_OK);
    }

    if (Test(Token::kColon)) {
        TypeSign *type = ParseTypeSign(CHECK_OK);
        proto->set_return_type(type);
        *loc = file_unit_->FindSourceLocation(type);
    } else {
        int position = file_unit_->InsertSourceLocation(*loc);
        TypeSign *type = new (arena_) TypeSign(position, Token::kVoid);
        proto->set_return_type(type);
    }
    
    return proto;
}

static Operator GetPrefixOp(Token::Kind kind) {
    switch (kind) {
        case Token::kNot:
            return Operators::kNot;
        case Token::kWave:
            return Operators::kBitwiseNot;
        case Token::kMinus:
            return Operators::kMinus;
        case Token::kLArrow:
            return Operators::kRecv;
        case Token::k2Plus:
            return Operators::kIncrement;
        case Token::k2Minus:
            return Operators::kDecrement;
        default:
            return Operators::NOT_OPERATOR;
    }
}

static Operator GetPostfixOp(Token::Kind kind) {
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
        case Token::kEqual:
            return Operators::kEqual;
        case Token::kNotEqual:
            return Operators::kNotEqual;
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
        case Token::k2Plus:
            return Operators::kIncrementPost;
        case Token::k2Minus:
            return Operators::kDecrementPost;
        default:
            return Operators::NOT_OPERATOR;
    }
}

Expression *Parser::ParseExpression(int limit, Operator *receiver, bool *ok) {
    SourceLocation loc = Peek().source_location();
    Expression *expr = nullptr;
    Operator op = GetPrefixOp(Peek().kind());
    if (op.kind != Operator::NOT_OPERATOR) {
        MoveNext();
        Expression *operand = ParseExpression(Operators::kUnaryPrio, nullptr, CHECK_OK);
        if (op.assignment && !operand->IsLValue()) {
            error_feedback_->Printf(loc, "Can not assign non-lval");
            *ok = false;
            return nullptr;
        }
        loc.LinkEnd(file_unit_->FindSourceLocation(operand));
        int position = file_unit_->InsertSourceLocation(loc);
        return new (arena_) UnaryExpression(position, op, operand);
    } else {
        expr = ParseSimple(CHECK_OK);
    }
    
    op = GetPostfixOp(Peek().kind());
    while (op.kind != Operator::NOT_OPERATOR && op.left > limit) {
        if (op.assignment && !expr->IsLValue()) {
            error_feedback_->Printf(loc, "Can not assign non-lval");
            *ok = false;
            return nullptr;
        }

        MoveNext();
        if (op.operands == 1) { // Post unary operator
            loc.LinkEnd(file_unit_->FindSourceLocation(expr));
            int position = file_unit_->InsertSourceLocation(loc);
            expr = new (arena_) UnaryExpression(position, op, expr);
            op = GetPostfixOp(Peek().kind());
        } else {
            Operator next_op;
            Expression *rhs = ParseExpression(op.right, &next_op, CHECK_OK);
            
            loc.LinkEnd(file_unit_->FindSourceLocation(rhs));
            int position = file_unit_->InsertSourceLocation(loc);
            expr = new (arena_) BinaryExpression(position, op, expr, rhs);
            op = next_op;
        }
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
            return new (arena_) NilLiteral(position, nullptr);
        } break;

        case Token::kTrue: {
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) BoolLiteral(position, true);
        } break;

        case Token::kFalse: {
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) BoolLiteral(position, false);
        } break;

        case Token::kIf:
            return ParseIfExpression(ok);

        case Token::kLambda:
            return ParseLambdaLiteral(ok);

        case Token::kChannel:
            return ParseChannelInitializer(ok);

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
                base::ArenaVector<Expression *> argv(arena_);
                if (Peek().kind() == Token::kRParen) { // call()
                    loc.LinkEnd(Peek().source_location());
                    MoveNext();
                    int position = file_unit_->InsertSourceLocation(loc);
                    expr = new (arena_) CallExpression(position, expr, std::move(argv));
                } else {
                    Expression *arg = ParseExpression(CHECK_OK);
                    argv.push_back(arg);
                    while (Test(Token::kComma)) {
                        arg = ParseExpression(CHECK_OK);
                        argv.push_back(arg);
                    }
                    loc.LinkEnd(Peek().source_location());
                    Match(Token::kRParen, CHECK_OK);
                    int position = file_unit_->InsertSourceLocation(loc);
                    expr = new (arena_) CallExpression(position, expr, std::move(argv));
                }
            } break;

            case Token::kIs: { // is
                MoveNext();
                TypeSign *type = ParseTypeSign(CHECK_OK);
                loc.LinkEnd(file_unit_->FindSourceLocation(type));
                int position = file_unit_->InsertSourceLocation(loc);
                expr = new (arena_) TypeTestExpression(position, expr, type);
            } break;

            case Token::kAs: { // as
                MoveNext();
                TypeSign *type = ParseTypeSign(CHECK_OK);
                loc.LinkEnd(file_unit_->FindSourceLocation(type));
                int position = file_unit_->InsertSourceLocation(loc);
                expr = new (arena_) TypeCastExpression(position, expr, type);
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

        case Token::kI8Val: {
            int32_t val = Peek().i32_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) I8Literal(position, val);
        } break;
            
        case Token::kU8Val: {
            uint32_t val = Peek().u32_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) U8Literal(position, val);
        } break;

        case Token::kI16Val: {
            int32_t val = Peek().i32_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) I16Literal(position, val);
        } break;

        case Token::kU16Val: {
            uint32_t val = Peek().u32_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) U16Literal(position, val);
        } break;

        case Token::kI32Val: {
            int32_t val = Peek().i32_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) I32Literal(position, val);
        } break;

        case Token::kU32Val: {
            uint32_t val = Peek().u32_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) U32Literal(position, val);
        } break;
        
        case Token::kI64Val: {
            int64_t val = Peek().i64_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) I64Literal(position, val);
        } break;

        case Token::kU64Val: {
            uint64_t val = Peek().u64_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) U64Literal(position, val);
        } break;
            
        case Token::kIntVal: {
            int32_t val = Peek().i32_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) IntLiteral(position, val);
        } break;
            
        case Token::kUIntVal: {
            uint32_t val = Peek().u32_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) UIntLiteral(position, val);
        } break;

        case Token::kF32Val: {
            float val = Peek().f32_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) F32Literal(position, val);
        } break;
            
        case Token::kF64Val: {
            double val = Peek().f64_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) F64Literal(position, val);
        } break;
            
        case Token::kStringLine:
        case Token::kStringBlock: {
            const ASTString *literal = Peek().text_val();
            MoveNext();
            int position = file_unit_->InsertSourceLocation(loc);
            return new (arena_) StringLiteral(position, literal);
        } break;

        case Token::kStringTempletePrefix: {
            return ParseStringTemplate(ok);
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
            error_feedback_->Printf(Peek().source_location(),
                                    "Unexpected primary expression, expected: %s",
                                    Peek().ToString().c_str());
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

    base::ArenaVector<Expression *> elements(arena_);
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
    return new (arena_) ArrayInitializer(position, mut, element_type, std::move(elements));
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

    base::ArenaVector<Expression *> pairs(arena_);
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
    return new (arena_) MapInitializer(position, mut, key_type, value_type, std::move(pairs));
}

ChannelInitializer *Parser::ParseChannelInitializer(bool *ok) {
    SourceLocation loc = Peek().source_location();
    Match(Token::kChannel, CHECK_OK);
    Match(Token::kLBrack, CHECK_OK);
    TypeSign *value_type = ParseTypeSign(CHECK_OK);
    Match(Token::kRBrack, CHECK_OK);
    Match(Token::kLParen, CHECK_OK);
    Expression *size = ParseExpression(CHECK_OK);
    
    loc.LinkEnd(Peek().source_location());
    Match(Token::kRParen, CHECK_OK);
    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) ChannelInitializer(position, value_type, size);
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

LambdaLiteral *Parser::ParseLambdaLiteral(bool *ok) {
    SourceLocation loc = Peek().source_location();
    switch (Peek().kind()) {
        case Token::kLambdaS:
        case Token::kLambda:
            MoveNext();
            break;
        default:
            break;
    }

    SourceLocation end;
    FunctionPrototype *proto = ParseFunctionPrototype(true, &end, CHECK_OK);
    Match(Token::kLBrace, CHECK_OK);
    base::ArenaVector<Statement *> stmts(arena_);
    loc.LinkEnd(Peek().source_location());
    while (!Test(Token::kRBrace)) {
        Statement *stmt = ParseStatement(CHECK_OK);
        stmts.push_back(stmt);
        loc.LinkEnd(Peek().source_location());
    }

    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) LambdaLiteral(position, proto, std::move(stmts));
}

IfExpression *Parser::ParseIfExpression(bool *ok) {
    SourceLocation loc = Peek().source_location();
    Match(Token::kIf, CHECK_OK);
    Match(Token::kLParen, CHECK_OK);
    Statement *stmt = ParseStatement(CHECK_OK);
    Expression *condition = nullptr;
    if (Test(Token::kSemi)) {
        condition = ParseExpression(CHECK_OK);
    } else {
        if (condition = stmt->AsExpression(); !condition) {
            error_feedback_->Printf(file_unit_->FindSourceLocation(stmt), "Unexpected condition "
                                    "expression");
            return nullptr;
        }
        stmt = nullptr;
    }
    Match(Token::kRParen, CHECK_OK);
    
    Statement *branch_true = ParseStatement(CHECK_OK);
    loc.LinkEnd(file_unit_->FindSourceLocation(branch_true));

    Statement *branch_false = nullptr;
    if (Test(Token::kElse)) {
        branch_false = ParseStatement(CHECK_OK);
        loc.LinkEnd(file_unit_->FindSourceLocation(branch_false));
    }

    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) IfExpression(position, stmt, condition, branch_true, branch_false);
}

TryCatchFinallyBlock *Parser::ParseTryCatchFinallyBlock(bool *ok) {
    SourceLocation loc = Peek().source_location();
    Match(Token::kTry, CHECK_OK);
    Match(Token::kLBrace, CHECK_OK);

    base::ArenaVector<Statement *> try_block(arena_);
    while (!Test(Token::kRBrace)) {
        Statement *stmt = ParseStatement(CHECK_OK);
        try_block.push_back(stmt);
    }

    base::ArenaVector<CatchBlock *> catch_blocks(arena_);
    while (Test(Token::kCatch)) {
        SourceLocation sub = Peek().source_location();
        Match(Token::kLParen, CHECK_OK);
        const ASTString *name = ParseIdentifier(CHECK_OK);
        Match(Token::kColon, CHECK_OK);
        TypeSign *type = ParseTypeSign(CHECK_OK);
        loc.LinkEnd(Peek().source_location());
        sub.LinkEnd(Peek().source_location());
        Match(Token::kRParen, CHECK_OK);
        
        int position = file_unit_->InsertSourceLocation(sub);
        VariableDeclaration *decl = new (arena_) VariableDeclaration(position,
                                                                     VariableDeclaration::VAL,
                                                                     name, type, nullptr);
        
        Match(Token::kLBrace, CHECK_OK);
        base::ArenaVector<Statement *> stmts(arena_);
        loc.LinkEnd(Peek().source_location());
        sub.LinkEnd(Peek().source_location());
        while (!Test(Token::kRBrace)) {
            Statement *stmt = ParseStatement(CHECK_OK);
            stmts.push_back(stmt);
            loc.LinkEnd(Peek().source_location());
            sub.LinkEnd(Peek().source_location());
        }
        position = file_unit_->InsertSourceLocation(sub);
        CatchBlock *block = new (arena_) CatchBlock(position, decl, std::move(stmts));
        catch_blocks.push_back(block);
    }

    base::ArenaVector<Statement *> finally_block(arena_);
    if (Test(Token::kFinally)) {
        loc.LinkEnd(Peek().source_location());
        Match(Token::kLBrace, CHECK_OK);
        while (!Test(Token::kRBrace)) {
            Statement *stmt = ParseStatement(CHECK_OK);
            finally_block.push_back(stmt);
            loc.LinkEnd(Peek().source_location());
        }
    }

    if (finally_block.empty() && catch_blocks.empty()) {
        *ok = false;
        error_feedback_->Printf(loc, "Unexpected finally or catch block");
        return nullptr;
    }

    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) TryCatchFinallyBlock(position, std::move(try_block),
                                             std::move(catch_blocks), std::move(finally_block));
}

RunStatement *Parser::ParseRunStatement(bool *ok) {
    SourceLocation loc = Peek().source_location();
    switch (Peek().kind()) {
        case Token::kRun:
        case Token::kRunS:
            MoveNext();
            break;
        default:
            *ok = false;
            error_feedback_->Printf(loc, "Unexpected `run' expected: %s", Peek().ToString().c_str());
            return nullptr;
    }
    
    Expression *calling = ParseExpression(CHECK_OK);
    if (!calling->IsCallExpression()) {
        *ok = false;
        error_feedback_->Printf(loc, "Unexpected calling expression");
        return nullptr;
    }
    
    loc.LinkEnd(file_unit_->FindSourceLocation(calling));
    int position = file_unit_->InsertSourceLocation(loc);
    return new (arena_) RunStatement(position, calling->AsCallExpression());
}

uint32_t Parser::ParseCompilingAttributes(bool *ok) {
    Match(Token::kDollar, CHECK_OK);
    Match(Token::kLBrack, CHECK_OK);
    uint32_t attr = 0;
    int n = 0;
    while (!Test(Token::kRBrack)) {
        if (n > 0) {
            Match(Token::kComma, CHECK_OK);
        }
        if (Peek().kind() != Token::kStringLine) {
            error_feedback_->Printf(Peek().source_location(),
                                    "Unexpected string-literal, expected: %s",
                                    Peek().ToString().c_str());
            *ok = false;
            return 0;
        }
        attr |= FindCompilingAttribute(Peek().text_val()->ToSlice());
        MoveNext();
        n++;
    }
    return attr;
}

StringTemplateExpression *Parser::ParseStringTemplate(bool *ok) {
    SourceLocation loc = Peek().source_location();

    const ASTString *literal = Peek().text_val();
    MoveNext();
    int position = file_unit_->InsertSourceLocation(loc);
    Expression *prefix = new (arena_) StringLiteral(position, literal);
    base::ArenaVector<Expression *> parts(arena_);
    parts.push_back(prefix);
    
    for (;;) {
        switch (Peek().kind()) {
            case Token::kStringTempletePart: {
                int position = file_unit_->InsertSourceLocation(Peek().source_location());
                const ASTString *literal = Peek().text_val();
                MoveNext();
                Expression *part = new (arena_) StringLiteral(position, literal);
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
                    Expression *part = new (arena_) StringLiteral(position, literal);
                    parts.push_back(part);
                }
            } goto done;

            default:
                NOREACHED();
                break;
        }
    }
done:
    return new (arena_) StringTemplateExpression(position, std::move(parts));
}

const ASTString *Parser::ParseDotName(SourceLocation *loc, bool *ok) {
    std::string buf;
    const ASTString *part = ParseIdentifier(CHECK_OK);
    buf.append(part->ToString());
    while (Test(Token::kDot)) {
        loc->LinkEnd(Peek().source_location());
        part = ParseIdentifier(CHECK_OK);
        buf.append(1, '.');
        buf.append(part->ToString());
    }
    return ASTString::New(arena_, buf.data(), buf.size());
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
        error_feedback_->Printf(lookahead_.source_location(), "Unexpected: Identifier, expected: "
                                "%s", lookahead_.ToString().c_str());
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
