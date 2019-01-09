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
    struct {
        int fixed_size;
        int float_size;
    } size;
    int int_val;
    bool bool_val;
    ::mai::sql::SQLKeyType key_type;
    ::mai::sql::Block *block;
    ::mai::sql::Statement *stmt;
    ::mai::sql::TypeDefinition *type_def;
    ::mai::sql::ColumnDefinition *col_def;
    ::mai::sql::CreateTable *create_table_stmt;
    const ::mai::sql::AstString *str;
}

%token SELECT FROM CREATE TABLE TABLES DROP SHOW ALTER ADD
%token UNIQUE PRIMARY KEY ENGINE TXN_BEGIN TRANSACTION
%token TXN_COMMIT TXN_ROLLBACK

%token ID NULL_VAL INTEGRAL_VAL STRING_VAL

%token EQ NOT

%token BIGINT INT SMALLINT TINYINT DECIMAL NUMERIC
%token CHAR VARCHAR DATE DATETIME TIMESTMAP AUTO_INCREMENT COMMENT

%type <block> Block
%type <stmt> Statement Command DDL
%type <text> ID STRING_VAL
%type <str> Identifier CommentOption
%type <bool_val> NullOption AutoIncrementOption
%type <size> FixedSizeDescription FloatingSizeDescription
%type <type_def> TypeDefinition
%type <col_def> ColumnDefinition
%type <create_table_stmt> CreateTableStmt ColumnDefinitionList
%type <int_val> INTEGRAL_VAL
%type <key_type> KeyOption

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
DDL: CreateTableStmt {
    $$ = $1;
}
| DROP TABLE Identifier {
    $$ = ctx->factory->NewDropTable($3);
}

CreateTableStmt : CREATE TABLE Identifier '(' ColumnDefinitionList ')' {
    $$ = $5;
    $$->set_schema_name($3);
}

ColumnDefinitionList: ColumnDefinition {
    $$ = ctx->factory->NewCreateTable(NULL);
    $$->AddColumn($1);
}
| ColumnDefinitionList ',' ColumnDefinition {
    $$->AddColumn($3);
}

ColumnDefinition: Identifier TypeDefinition NullOption AutoIncrementOption KeyOption CommentOption {
    $$ = ctx->factory->NewColumnDefinition($1, $2, $3, $4, $5);
    $$->set_comment($6);
}

TypeDefinition: BIGINT FixedSizeDescription {
    $$ = ctx->factory->NewTypeDefinition(SQL_BIGINT, $2.fixed_size);
}
| INT FixedSizeDescription {
    $$ = ctx->factory->NewTypeDefinition(SQL_INT, $2.fixed_size);
}
| SMALLINT FixedSizeDescription {
    $$ = ctx->factory->NewTypeDefinition(SQL_SMALLINT, $2.fixed_size);
}
| TINYINT FixedSizeDescription {
    $$ = ctx->factory->NewTypeDefinition(SQL_TINYINT, $2.fixed_size);
}
| DECIMAL FloatingSizeDescription {
    $$ = ctx->factory->NewTypeDefinition(SQL_DECIMAL, $2.fixed_size, $2.float_size);
}
| NUMERIC FloatingSizeDescription {
    $$ = ctx->factory->NewTypeDefinition(SQL_NUMERIC, $2.fixed_size, $2.float_size);
}
| CHAR FixedSizeDescription {
    $$ = ctx->factory->NewTypeDefinition(SQL_NUMERIC, $2.fixed_size);
}
| VARCHAR FixedSizeDescription {
    $$ = ctx->factory->NewTypeDefinition(SQL_VARCHAR, $2.fixed_size);
}
| DATE {
    $$ = ctx->factory->NewTypeDefinition(SQL_DATE);
}
| DATETIME {
    $$ = ctx->factory->NewTypeDefinition(SQL_DATETIME);
}

FixedSizeDescription: '(' INTEGRAL_VAL ')' {
    $$.fixed_size = $2;
    $$.float_size = 0;
}
| {
    $$.fixed_size = 0;
    $$.float_size = 0;
}

FloatingSizeDescription: '(' INTEGRAL_VAL '.' INTEGRAL_VAL ')' {
    $$.fixed_size = $2;
    $$.float_size = $4;
}
| {
    $$.fixed_size = 0;
    $$.float_size = 0;
}

AutoIncrementOption: AUTO_INCREMENT {
    $$ = true;
}
| {
    $$ = false;
}

NullOption: NOT NULL_VAL {
    $$ = true;
}
| NULL_VAL {
    $$ = false;
}
| {
    $$ = false;
}

KeyOption: KEY {
    $$ = SQL_KEY;
}
| UNIQUE {
    $$ = SQL_UNIQUE_KEY;
}
| UNIQUE KEY {
    $$ = SQL_UNIQUE_KEY;
}
| PRIMARY {
    $$ = SQL_PRIMARY_KEY;
}
| PRIMARY KEY {
    $$ = SQL_PRIMARY_KEY;
}
| {
    $$ = SQL_NOT_KEY;
}

CommentOption: COMMENT STRING_VAL {
    $$ = ctx->factory->NewString($2.buf, $2.len);
}
| {
    $$ = AstString::kEmpty;
}

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
