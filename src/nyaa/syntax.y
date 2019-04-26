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
    const ::mai::nyaa::ast::String *name;
    ::mai::nyaa::ast::VarDeclaration::NameList *names;
    ::mai::nyaa::ast::Block *block;
    ::mai::nyaa::ast::Expression *expr;
    ::mai::nyaa::ast::LValue *lval;
    ::mai::nyaa::ast::Assignment::LValList *lvals;
    ::mai::nyaa::ast::Multiple *entry;
    ::mai::nyaa::ast::MapInitializer::EntryList *entries;
    ::mai::nyaa::ast::Statement *stmt;
    ::mai::nyaa::ast::Block::StmtList *stmts;
    ::mai::nyaa::ast::Return::ExprList *exprs;
    struct {
        ::mai::nyaa::ast::LambdaLiteral::ParameterList *params;
        bool vargs;
    } params;
    ::mai::nyaa::Operator::ID op;
    ::mai::nyaa::ast::String *str_val;
    ::mai::nyaa::ast::String *int_val;
    ::mai::nyaa::f64_t f64_val;
    int64_t smi_val;
    bool bool_val;
}

%token DEF VAR LAMBDA NAME COMPARISON OP_OR OP_XOR OP_AND OP_LSHIFT OP_RSHIFT UMINUS RETURN VARGS DO NEW
%token IF ELSE WHILE OBJECT CLASS PROPERTY
%token STRING_LITERAL SMI_LITERAL APPROX_LITERAL INT_LITERAL NIL_LITERAL BOOL_LITERAL
%token TOKEN_ERROR

%type <block> Script Block
%type <stmt> Statement VarDeclaration FunctionDefinition Assignment IfStatement ElseClause ObjectDefinition ClassDefinition MemberDefinition PropertyDeclaration
%type <stmts> StatementList MemberList
%type <expr> Expression Call LambdaLiteral MapInitializer InheritClause
%type <exprs> ExpressionList Arguments
%type <entry> MapEntry
%type <entries> MapEntryList
%type <lval> LValue
%type <lvals> LValList
%type <name> NAME
%type <names> NameList Attributes
%type <params> Parameters
%type <str_val> STRING_LITERAL
%type <smi_val> SMI_LITERAL BOOL_LITERAL
%type <int_val> INT_LITERAL
%type <f64_val> APPROX_LITERAL
%type <bool_val> ParameterVargs

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

Block : '{' StatementList '}' {
    $$ = ctx->factory->NewBlock($2, @2);
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
| ObjectDefinition {
    $$ = $1;
}
| ClassDefinition {
    $$ = $1;
}
| FunctionDefinition {
    $$ = $1;
}
| Assignment {
    $$ = $1;
}
| Expression {
    $$ = $1;
}
| DO Block {
    $$ = $2;
}
| IfStatement {
    $$ = $1;
}

IfStatement: IF '(' Expression ')' Block ElseClause {
    $$ = ctx->factory->NewIfStatement($3, $5, $6, Location::Concat(@1, @6));
}

ElseClause: ELSE Block {
    $$ = $2;
}
| ELSE IfStatement {
    $$ = $2;
}
| {
    $$ = nullptr;
}

ObjectDefinition : OBJECT Attributes NAME '{' MemberList '}' {
    $$ = ctx->factory->NewObjectDefinition($2, $3, $5, Location::Concat(@1, @6));
}

ClassDefinition : CLASS Attributes NAME InheritClause '{' MemberList '}' {
    $$ = ctx->factory->NewClassDefinition($2, $3, $4, $6, Location::Concat(@1, @7));
}

InheritClause : ':' NAME {
    $$ = ctx->factory->NewVariable($2, @2);
}
| {
    $$ = nullptr;
}

MemberList : MemberDefinition {
    $$ = ctx->factory->NewList($1);
}
| MemberList MemberDefinition {
    $$->push_back($2);
}
| {
    $$ = nullptr;
}

MemberDefinition : PropertyDeclaration {
    $$ = $1;
}
| FunctionDefinition {
    $$ = $1;
}

PropertyDeclaration : PROPERTY Attributes NameList {
    $$ = ctx->factory->NewPropertyDeclaration($2, $3, nullptr, Location::Concat(@1, @3));
}

FunctionDefinition : DEF NAME '.' NAME '(' Parameters ')' Block {
    auto lambda = ctx->factory->NewLambdaLiteral($6.params, $6.vargs, $8, Location::Concat(@5, @8));
    auto self = ctx->factory->NewVariable($2, @2);
    $$ =  ctx->factory->NewFunctionDefinition(self, $4, lambda, Location::Concat(@1, @8));
}
| DEF NAME '(' Parameters ')' Block {
    auto lambda = ctx->factory->NewLambdaLiteral($4.params, $4.vargs, $6, Location::Concat(@3, @6));
    $$ =  ctx->factory->NewFunctionDefinition(nullptr, $2, lambda, Location::Concat(@1, @6));
}

Parameters : NameList ParameterVargs {
    $$.params = $1;
    $$.vargs  = $2;
}
| VARGS {
    $$.params = nullptr;
    $$.vargs  = true;    
}
| {
    $$.params = nullptr;
    $$.vargs  = false;
}

ParameterVargs : ',' VARGS {
    $$ =  true;
}
| {
    $$ = false;
}

VarDeclaration : VAR NameList '=' ExpressionList {
    $$ = ctx->factory->NewVarDeclaration($2, $4, Location::Concat(@1, @4));
}
| VAR NameList {
    $$ = ctx->factory->NewVarDeclaration($2, nullptr, Location::Concat(@1, @2));
}

Assignment : LValList '=' ExpressionList {
    $$ = ctx->factory->NewAssignment($1, $3, Location::Concat(@1, @3));
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

Expression : LValue {
    $$ = $1;
}
| Call {
    $$ = $1;
}
| NIL_LITERAL {
    $$ = ctx->factory->NewNilLiteral(@1);
}
| SMI_LITERAL {
    $$ = ctx->factory->NewSmiLiteral($1, @1);
}
| BOOL_LITERAL {
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
| LambdaLiteral {
    $$ = $1;
}
| MapInitializer {
    $$ = $1;
}
| VARGS {
    $$ = ctx->factory->NewVariableArguments(@1);
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

LambdaLiteral : LAMBDA '(' Parameters ')' Block {
    $$ = ctx->factory->NewLambdaLiteral($3.params, $3.vargs, $5, Location::Concat(@1, @5));
}
| LAMBDA Block {
    $$ = ctx->factory->NewLambdaLiteral(nullptr, false, $2, Location::Concat(@1, @2));
}

MapInitializer : '{' MapEntryList '}' {
    $$ = ctx->factory->NewMapInitializer($2, Location::Concat(@1, @3));
}

MapEntryList : MapEntry {
    $$ = ctx->factory->NewList($1);
}
| MapEntryList ',' MapEntry {
    $$->push_back($3);
}
| {
    $$ = nullptr;
}

MapEntry : NAME ':' Expression {
    auto key = ctx->factory->NewStringLiteral($1, @1);
    $$ = ctx->factory->NewEntry(key, $3, Location::Concat(@1, @3));
}
| Expression {
    $$ = ctx->factory->NewEntry(nullptr, $1, @1);
}
| '[' Expression ']' '=' Expression {
    $$ = ctx->factory->NewEntry($2, $5, Location::Concat(@1, @5));
}

LValList : LValue {
    $$ = ctx->factory->NewList($1);
}
| LValList ',' LValue {
    $$->push_back($3);
}

LValue : NAME {
    $$ = ctx->factory->NewVariable($1, @1);
}
| Expression '[' Expression ']' {
    $$ = ctx->factory->NewIndex($1, $3, Location::Concat(@1, @4));
}
| Expression '.' NAME {
    $$ = ctx->factory->NewDotField($1, $3, Location::Concat(@1, @3));
}

Call : Expression Arguments {
    $$ = ctx->factory->NewCall($1, $2, Location::Concat(@1, @2));
}
| Expression ':' NAME Arguments {
    $$ = ctx->factory->NewSelfCall($1, $3, $4, Location::Concat(@1, @4));
}
| NEW NAME '(' ExpressionList ')' {
    auto callee = ctx->factory->NewVariable($2, @2);
    $$ = ctx->factory->NewNew(callee, $4, Location::Concat(@1, @5));
}

Arguments : '(' ExpressionList ')' {
    $$ = $2;
}
| STRING_LITERAL {
    auto arg0 = ctx->factory->NewStringLiteral($1, @1);
    $$ = ctx->factory->NewList<mai::nyaa::ast::Expression *>(arg0);
}
| MapInitializer {
    $$ = ctx->factory->NewList<mai::nyaa::ast::Expression *>($1);
}

Attributes: '[' NameList ']' {
    $$ = $2;
}
| {
    $$ = nullptr;
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

