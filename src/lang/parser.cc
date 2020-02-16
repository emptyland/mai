#include "lang/parser.h"
#include "lang/lexer.h"
#include "lang/ast.h"

namespace mai {

namespace lang {

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

            default: {
                error_feedback_->Printf(lookahead_.source_location(), "Unexpected token %d", lookahead_.kind());
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
    const ASTString *name = ParseIdentifier(CHECK_OK);
    for (size_t i = 0; i < name->size(); i++) {
        int ch = name->data()[i];
        if (!isdigit(ch) && !isalpha(ch) && ch != '_') {
            error_feedback_->Printf(lookahead_.source_location(),
                                    "Incorrect package name: %s", name->data());
            *ok = false;
            return nullptr;
        }
    }
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
        alias = ParseIdentifier(CHECK_OK);
    }

    int position = file_unit_->InsertSourceLocation(*loc);
    return new (arena_) ImportStatement(position, alias, ASTString::New(arena_, buf.c_str()));
}

const ASTString *Parser::ParseIdentifier(bool *ok) {
    if (lookahead_.kind() != Token::kIdentifier) {
        error_feedback_->Printf(lookahead_.source_location(),
                                "Unexpected: `Identifier', expected: %d", lookahead_.kind());
        *ok = false;
        return nullptr;
    }
    const ASTString *text = lookahead_.text_val();
    lookahead_ = lexer_->Next();
    return text;
}

void Parser::Match(Token::Kind kind, bool *ok) {
    if (lookahead_.kind() != kind) {
        error_feedback_->Printf(lookahead_.source_location(), "Unexpected: %d, expected: %d", kind,
                                lookahead_.kind());
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
