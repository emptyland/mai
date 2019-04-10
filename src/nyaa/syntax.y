// bison -d syntax.y -o syntax.cc
//%language "C++"
%defines
%locations
%define api.pure
%define parse.error verbose
%define api.prefix {nyaa_yy}
%parse-param {parser_ctx *ctx}
%lex-param {void *YYLEX_PARAM}
//%error-verbose

%{
#include "nyaa/ast.h"
#include "nyaa/parser-ctx.h"
#include "nyaa/syntax.hh"
#include "nyaa/lex.yy.h"
#include <string.h>

#define YYLEX_PARAM ctx->lex

using ::mai::nyaa::ast::Location;
using ::mai::nyaa::Operator;

void yyerror(YYLTYPE *, parser_ctx *, const char *);
%}

%union {
    ::mai::nyaa::ast::String *name;
    ::mai::nyaa::ast::VarDeclaration::NameList *names;
    ::mai::nyaa::ast::Block *block;
    ::mai::nyaa::ast::Expression *expr;
    ::mai::nyaa::ast::Statement *stmt;
    ::mai::nyaa::ast::Block::StmtList *stmts;
    ::mai::nyaa::ast::Return::ExprList *exprs;
    ::mai::nyaa::Operator::ID op;
    ::mai::nyaa::ast::String *str_val;
    ::mai::nyaa::ast::String *int_val;
    ::mai::nyaa::f64_t f64_val;
    int64_t smi_val;
}

%token DEF VAR LAMBDA NAME COMPARISON OP_OR OP_XOR OP_AND OP_LSHIFT OP_RSHIFT UMINUS RETURN
%token STRING_LITERAL SMI_LITERAL APPROX_LITERAL INT_LITERAL
%token TOKEN_ERROR

%type <block> Script
%type <stmt> Statement VarDeclaration
%type <stmts> StatementList
%type <expr> Expression
%type <exprs> ExpressionList
%type <name> NAME
%type <names> NameList
%type <str_val> STRING_LITERAL
%type <smi_val> SMI_LITERAL
%type <int_val> INT_LITERAL
%type <f64_val> APPROX_LITERAL

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
%left '*' '/' '%'
%left '^'
%nonassoc UMINUS

//%type column
%%
Script: StatementList {
    $$ = ctx->factory->NewBlock($1, @1);
    ctx->block = $$;
}

StatementList : Statement {
    $$ = ctx->factory->NewList($1);
}
| StatementList Statement {
    $$->push_back($2);
}
| {
    $$ = nullptr;
}

Statement : RETURN ExpressionList {
    $$ = ctx->factory->NewReturn($2, @2);
}
| VarDeclaration {
    $$ = $1;
}

VarDeclaration : VAR NameList '=' ExpressionList {
    $$ = ctx->factory->NewVarDeclaration($2, $4, Location::Concat(@1, @4));
}

ExpressionList : Expression {
    $$ = ctx->factory->NewList($1);
}
| ExpressionList ',' Expression {
    $$->push_back($3);
}
| {
    $$ = nullptr;
}

Expression : NAME {
    $$ = ctx->factory->NewVariable($1, @1);
}
| SMI_LITERAL {
    $$ = ctx->factory->NewSmiLiteral($1, @1);
}
| INT_LITERAL {
    $$ = ctx->factory->NewIntLiteral($1, @1);
}
| APPROX_LITERAL {
    $$ = ctx->factory->NewApproxLiteral($1, @1);
}
| STRING_LITERAL {
    $$ = ctx->factory->NewStringLiteral($1, @1);
}
| Expression COMPARISON Expression {
    $$ = ctx->factory->NewBinary($2, $1, $3, Location::Concat(@1, @3));
}
| Expression '+' Expression {
    $$ = ctx->factory->NewBinary(Operator::kAdd, $1, $3, Location::Concat(@1, @3));
}
| Expression '-' Expression {
    $$ = ctx->factory->NewBinary(Operator::kSub, $1, $3, Location::Concat(@1, @3));
}
| Expression '*' Expression {
    $$ = ctx->factory->NewBinary(Operator::kMul, $1, $3, Location::Concat(@1, @3));
}
| Expression '/' Expression {
    $$ = ctx->factory->NewBinary(Operator::kDiv, $1, $3, Location::Concat(@1, @3));
}
| '(' Expression ')' {
    $$ = $2;
}

NameList : NAME {
    $$ = ctx->factory->NewList($1);
}
| NameList ',' NAME {
    $$->push_back($3);
}


%%
void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg) {
    ctx->err_line   = yyl->first_line;
    ctx->err_column = yyl->first_column;
    ctx->err_msg    = ::mai::nyaa::ast::String::New(ctx->arena, msg);
}

