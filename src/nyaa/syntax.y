// bison -d syntax.y -o syntax.cc
//%language "C++"
%defines
%locations
%define api.pure
%define parse.error verbose
%parse-param {parser_ctx *ctx}
%lex-param {void *YYLEX_PARAM}
//%error-verbose

%{
#include "nyaa/ast.h"
#include "nyaa/parser-ctx.h"
#include "nyaa/syntax.hh"
//#if defined(__cplusplus)
//extern "C" {
//#endif
#include "nyaa/lex.yy.h"
//#if defined(__cplusplus)
//}
//#endif
#include <string.h>

using namespace ::mai::nyaa;
using namespace ::mai::nyaa::ast;

#define YYLEX_PARAM ctx->lex

void yyerror(YYLTYPE *, parser_ctx *, const char *);
%}

%union {
    Object *value;
    NyString *name;
    Block *block;
    Operator op;
}

%token DEF VAR LAMBDA NAME COMPARISON VALUE 
%token TOKEN_ERROR

%type <block> Block
%type <name> NAME
%type <value> VALUE

%right '='
%left OP_OR
%left OP_XOR
%left OP_AND
%nonassoc IN IS
%left OP_NOT '!'
%left <op> COMPARISON
%left '|'
%left '&'
%left OP_LSHIFT OP_RSHIFT
%left '+' '-'
%left '*' '/' '%' MOD
%left '^'
%nonassoc UMINUS

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



%%
void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg) {
    ctx->err_line   = yyl->first_line;
    ctx->err_column = yyl->first_column;
    ctx->err_msg    = ctx->factory->NewString(msg);
}
