// bison -d sql.y -o sql.cc
//%language "C++"
%defines
//%define parser_class_name {sql_parser}
%locations
%define api.pure
//%lex-param {void *ctx}
%parse-param {parser_ctx *ctx}
%lex-param {void *YYLEX_PARAM}

%{
#include "sql/ast.h"
#include "sql/ast-factory.h"
#include "sql/parser-ctx.h"
#include "sql/sql.hh"
//#if defined(__cplusplus)
//extern "C" {
//#endif
#include "sql/sql.yy.h"
//#if defined(__cplusplus)
//}
//#endif
#include <string.h>

using namespace ::mai::sql;

#define YYLEX_PARAM ctx->lex

void yyerror(YYLTYPE *, parser_ctx *, const char *);
%}

%union {
    struct {
        const char *buf;
        size_t      len;
    } text;
    int int_val;
    ::mai::sql::Block *block;
    ::mai::sql::Statement *stmt;
    const ::mai::sql::AstString *str;
}

%token SELECT FROM CREATE TABLE TABLES DROP SHOW
%token UNIQUE PRIMARY KEY ENGINE TXN_BEGIN TRANSACTION
%token TXN_COMMIT TXN_ROLLBACK

%token ID NULL_VAL INTEGRAL_VAL STRING_VAL

%token EQ NOT

%token BIGINT INT SMALLINT TINYINT DECIMAL NUMERIC
%token CHAR VARCHAR DATE DATETIME TIMESTMAP

%type <block> Block
%type <stmt> Statement Command DDL
%type <text> ID
%type <str> Identifier

//%type column
%%
Block: Statement {
    $$ = ctx->factory->NewBlock();
    $$->AddStmt($1);
    ctx->block = $$;
}
| Block Statement {
    $$->AddStmt($2);
    ctx->block = $$;
}

Statement: Command ';'

Command: DDL
| TXN_BEGIN TRANSACTION {
    $$ = ctx->factory->NewTCLStatement(TCLStatement::TXN_BEGIN);
}
| TXN_COMMIT {
    $$ = ctx->factory->NewTCLStatement(TCLStatement::TXN_COMMIT);
}
| TXN_ROLLBACK {
    $$ = ctx->factory->NewTCLStatement(TCLStatement::TXN_ROLLBACK);
}
| SHOW TABLES {
    $$ = ctx->factory->NewShowTables();
}

// CREATE DROP
DDL: CREATE TABLE '(' ColumnDeclarationList ')' {
    $$ = NULL;
}
| DROP TABLE Identifier {
    $$ = ctx->factory->NewDropTable($3);
}

ColumnDeclarationList: ColumnDeclaration
| ColumnDeclaration ',' ColumnDeclarationList

ColumnDeclaration: Identifier TypeDeclaration NullDeclaration UniqueDeclaration PrimaryKeyDeclaration

TypeDeclaration: BIGINT FixedSizeDescription
| INT FixedSizeDescription
| SMALLINT FixedSizeDescription
| TINYINT FixedSizeDescription
| DECIMAL FloatingSizeDescription
| NUMERIC FloatingSizeDescription
| CHAR FixedSizeDescription
| VARCHAR FixedSizeDescription
| DATE
| DATETIME
| TIMESTMAP

FixedSizeDescription: '(' INTEGRAL_VAL ')'
|

FloatingSizeDescription: '(' INTEGRAL_VAL '.' INTEGRAL_VAL ')'
|

NullDeclaration: NOT NULL_VAL
| NULL_VAL
|

UniqueDeclaration: UNIQUE
|

PrimaryKeyDeclaration: PRIMARY KEY
|

Identifier : ID {
    $$ = ctx->factory->NewString($1.buf, $1.len);
}


// INSERT UPDATE DELETE
// DML:

%%
void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg) {
    ctx->err_line   = yyl->first_line;
    ctx->err_column = yyl->first_column;
    ctx->err_msg    = ctx->factory->NewString(msg);
}
