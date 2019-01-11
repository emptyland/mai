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
    struct {
        const ::mai::sql::AstString *name;
        bool after;
    } col_pos;
    int int_val;
    bool bool_val;
    ::mai::sql::SQLKeyType key_type;
    ::mai::sql::Block *block;
    ::mai::sql::Statement *stmt;
    ::mai::sql::TypeDefinition *type_def;
    ::mai::sql::ColumnDefinition *col_def;
    ::mai::sql::ColumnDefinitionList *col_def_list;
    ::mai::sql::AlterTableSpecList *alter_table_spce_list;
    ::mai::sql::AlterTableSpec *alter_table_spce;
    ::mai::sql::NameList *name_list;
    const ::mai::sql::AstString *name;
}

%token SELECT FROM CREATE TABLE TABLES DROP SHOW ALTER ADD RENAME
%token UNIQUE PRIMARY KEY ENGINE TXN_BEGIN TRANSACTION COLUMN AFTER
%token TXN_COMMIT TXN_ROLLBACK FIRST CHANGE TO AS INDEX

%token ID NULL_VAL INTEGRAL_VAL STRING_VAL

%token EQ NOT

%token BIGINT INT SMALLINT TINYINT DECIMAL NUMERIC
%token CHAR VARCHAR DATE DATETIME TIMESTMAP AUTO_INCREMENT COMMENT

%type <block> Block
%type <stmt> Statement Command DDL CreateTableStmt AlterTableStmt
%type <text> ID STRING_VAL
%type <name> Identifier CommentOption
%type <bool_val> NullOption AutoIncrementOption
%type <size> FixedSizeDescription FloatingSizeDescription
%type <type_def> TypeDefinition
%type <int_val> INTEGRAL_VAL
%type <key_type> KeyOption
%type <col_def_list> ColumnDefinitionList
%type <col_def> ColumnDefinition
%type <col_pos> AlterColPosOption
%type <alter_table_spce> AlterTableSpec
%type <alter_table_spce_list> AlterTableSpecList
%type <name_list> NameList;

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
| AlterTableStmt {
    $$ = $1;
}

CreateTableStmt : CREATE TABLE Identifier '(' ColumnDefinitionList ')' {
    $$ = ctx->factory->NewCreateTable($3, $5);
}

ColumnDefinitionList: ColumnDefinition {
    $$ = ctx->factory->NewColumnDefinitionList($1);
}
| ColumnDefinitionList ',' ColumnDefinition {
    $$->push_back($3);
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

AlterTableStmt : ALTER TABLE Identifier AlterTableSpecList {
    $$ = ctx->factory->NewAlterTable($3, $4);
}

AlterTableSpecList : AlterTableSpec {
    $$ = ctx->factory->NewAlterTableSpecList($1);
}
| AlterTableSpecList ',' AlterTableSpec {
    $$->push_back($3);
}

AlterTableSpec : ADD ColumnDefinition AlterColPosOption {
    $$ = ctx->factory->NewAlterTableAddColumn($2, $3.after, $3.name);
}
| ADD COLUMN ColumnDefinition AlterColPosOption {
    $$ = ctx->factory->NewAlterTableAddColumn($3, $4.after, $4.name);
}
| ADD '(' ColumnDefinitionList ')' {
    $$ = ctx->factory->NewAlterTableAddColumn($3);
}
| ADD COLUMN '(' ColumnDefinitionList ')' {
    $$ = ctx->factory->NewAlterTableAddColumn($4);
}
| ADD INDEX Identifier KeyOption '(' NameList ')' {
    $$ = ctx->factory->NewAlterTableAddIndex($3, $4 == SQL_NOT_KEY ? SQL_KEY : $4,
                                             $6);
}
| ADD KEY Identifier KeyOption '(' NameList ')' {
    $$ = ctx->factory->NewAlterTableAddIndex($3, $4 == SQL_NOT_KEY ? SQL_KEY : $4,
                                             $6);
}
| CHANGE Identifier ColumnDefinition AlterColPosOption {
    $$ = ctx->factory->NewAlterTableChangeColumn($2, $3, $4.after, $4.name);
}
| CHANGE COLUMN Identifier ColumnDefinition AlterColPosOption {
    $$ = ctx->factory->NewAlterTableChangeColumn($3, $4, $5.after, $5.name);
}
| RENAME COLUMN Identifier TO Identifier {
    $$ = ctx->factory->NewAlterTableRenameColumn($3, $5);
}
| RENAME INDEX Identifier TO Identifier {
    $$ = ctx->factory->NewAlterTableRenameIndex($3, $5);
}
| RENAME KEY Identifier TO Identifier {
    $$ = ctx->factory->NewAlterTableRenameIndex($3, $5);
}
| RENAME Identifier {
    $$ = ctx->factory->NewAlterTableRename($2);
}
| RENAME TO Identifier {
    $$ = ctx->factory->NewAlterTableRename($3);
}
| RENAME AS Identifier {
    $$ = ctx->factory->NewAlterTableRename($3);
}
| DROP COLUMN Identifier {
    $$ = ctx->factory->NewAlterTableDropColumn($3);
}
| DROP Identifier {
    $$ = ctx->factory->NewAlterTableDropColumn($2);
}
| DROP INDEX Identifier {
    $$ = ctx->factory->NewAlterTableDropIndex($3, false);
}
| DROP KEY Identifier {
    $$ = ctx->factory->NewAlterTableDropIndex($3, false);
}
| DROP PRIMARY KEY {
    $$ = ctx->factory->NewAlterTableDropIndex(AstString::kEmpty, true);
}

AlterColPosOption : FIRST Identifier {
    $$.name  = $2;
    $$.after = false;
}
| AFTER Identifier {
    $$.name  = $2;
    $$.after = true;
}
| {
    $$.name  = AstString::kEmpty;
    $$.after = false;
}

NameList : Identifier {
    $$ = ctx->factory->NewNameList($1);
}
| NameList ',' Identifier {
    $$->push_back($3);
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
