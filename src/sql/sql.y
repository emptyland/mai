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
#include "sql/parser-ctx.h"
#include "sql/sql.hh"
#if defined(__cplusplus)
extern "C" {
#endif
#include "sql/sql.yy.h"
#if defined(__cplusplus)
}
#endif
#include <string.h>

#define YYLEX_PARAM ctx->lex

void yyerror(YYLTYPE *, parser_ctx *, const char *);
%}

%union {
    struct {
        const char *buf;
        size_t      len;
    } text;
}
%token TOK_SELECT TOK_STAR TOK_FROM TOK_CREATE TOK_TABLE TOK_DROP
%token TOK_UNIQUE TOK_PRIMARY TOK_KEY TOK_ENGINE TOK_BEGIN TOK_TRANSACTION
%token TOK_COMMIT TOK_ROLLBACK

%token TOK_LPAREN TOK_RPAREN TOK_COMMA

%token TOK_ID TOK_NULL TOK_INTEGRAL_VAL TOK_STRING_VAL

%token TOK_EQ TOK_NOT

%token TOK_BIGINT TOK_INT TOK_SMALLINT TOK_TINYINT TOK_DECIMAL TOK_NUMERIC
%token TOK_CHAR TOK_VARCHAR TOK_DATE TOK_DATETIME TOK_TIMESTMAP

//%type column
%%
Command: DDL
| TOK_BEGIN TOK_TRANSACTION
| TOK_COMMIT
| TOK_ROLLBACK

// CREATE DROP
DDL: TOK_CREATE TOK_TABLE TOK_LPAREN ColumnDeclarationList TOK_RPAREN EngineDeclaration
| TOK_DROP TOK_TABLE TOK_ID

ColumnDeclarationList: ColumnDeclaration
| ColumnDeclaration TOK_COMMA ColumnDeclarationList

ColumnDeclaration: TOK_ID TypeDeclaration NullDeclaration UniqueDeclaration PrimaryKeyDeclaration

TypeDeclaration: TOK_BIGINT FixedSizeDescription
| TOK_INT FixedSizeDescription
| TOK_SMALLINT FixedSizeDescription
| TOK_TINYINT FixedSizeDescription
| TOK_DECIMAL FloatingSizeDescription
| TOK_NUMERIC FloatingSizeDescription
| TOK_CHAR FixedSizeDescription
| TOK_VARCHAR FixedSizeDescription
| TOK_DATE
| TOK_DATETIME
| TOK_TIMESTMAP

FixedSizeDescription: TOK_LPAREN TOK_INTEGRAL_VAL TOK_RPAREN
|

FloatingSizeDescription: TOK_LPAREN TOK_INTEGRAL_VAL TOK_COMMA TOK_INTEGRAL_VAL TOK_RPAREN
|

NullDeclaration: TOK_NOT TOK_NULL
| TOK_NULL
|

UniqueDeclaration: TOK_UNIQUE
|

PrimaryKeyDeclaration: TOK_PRIMARY TOK_KEY
|

EngineDeclaration: TOK_ENGINE TOK_EQ TOK_STRING_VAL
|

// INSERT UPDATE DELETE
// DML:

%%
void yyerror(YYLTYPE *, parser_ctx *, const char *msg) {
    printf("Err: %s\n", msg);
}
