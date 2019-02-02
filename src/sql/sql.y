// bison -d sql.y -o sql.cc
//%language "C++"
%defines
//%define parser_class_name {sql_parser}
%locations
%define api.pure
//%lex-param {void *ctx}
%parse-param {parser_ctx *ctx}
%lex-param {void *YYLEX_PARAM}
%error-verbose

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
using namespace ::mai::sql::ast;

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
        int limit_val;
        int offset_val;
    } limit;
    struct {
        const ::mai::sql::AstString *name;
        bool after;
    } col_pos;
    struct {
        ::mai::sql::ast::ExpressionList *expr_list;
        bool desc;
    } order_by;
    int int_val;
    double approx_val;
    bool bool_val;
    const ::mai::sql::AstString *name;
    ::mai::sql::SQLKeyType key_type;
    ::mai::sql::SQLOperator op;
    ::mai::sql::SQLJoinKind join_kind;
    ::mai::sql::ast::Block *block;
    ::mai::sql::ast::Statement *stmt;
    ::mai::sql::ast::TypeDefinition *type_def;
    ::mai::sql::ast::ColumnDefinition *col_def;
    ::mai::sql::ast::ColumnDefinitionList *col_def_list;
    ::mai::sql::ast::AlterTableSpecList *alter_table_spce_list;
    ::mai::sql::ast::AlterTableSpec *alter_table_spce;
    ::mai::sql::ast::NameList *name_list;
    ::mai::sql::ast::Expression *expr;
    ::mai::sql::ast::ExpressionList *expr_list;
    ::mai::sql::ast::ProjectionColumn *proj_col;
    ::mai::sql::ast::ProjectionColumnList *proj_col_list;
    ::mai::sql::ast::Query *query;
    ::mai::sql::ast::RowValuesList *row_vals_list;
    ::mai::sql::ast::Assignment *assignment;
    ::mai::sql::ast::AssignmentList *assignment_list;
    ::mai::sql::ast::Identifier *id;
}

%token SELECT FROM CREATE TABLE TABLES DROP SHOW ALTER ADD RENAME ANY DIV
%token UNIQUE PRIMARY KEY ENGINE TXN_BEGIN TRANSACTION COLUMN AFTER
%token TXN_COMMIT TXN_ROLLBACK FIRST CHANGE TO AS INDEX DISTINCT HAVING
%token WHERE JOIN ON INNER OUTTER LEFT RIGHT ALL CROSS ORDER BY ASC DESC
%token GROUP FOR UPDATE LIMIT OFFSET INSERT OVERWRITE DELETE VALUES VALUE SET IN
%token INTO DUPLICATE DEFAULT

%token ID NULL_VAL INTEGRAL_VAL STRING_VAL APPROX_VAL DATE_VAL DATETIME_VAL

%token EQ NOT OP_AND

%token BIGINT INT SMALLINT TINYINT DECIMAL NUMERIC
%token CHAR VARCHAR DATE DATETIME TIMESTMAP AUTO_INCREMENT COMMENT
%token TOKEN_ERROR

%type <block> Block
%type <stmt> Statement Command DDL DML CreateTableStmt AlterTableStmt SelectStmt InsertStmt UpdateStmt
%type <text> ID STRING_VAL
%type <name> Name CommentOption Alias AliasOption
%type <bool_val> NullOption AutoIncrementOption DistinctOption ForUpdateOption OverwriteOption
%type <size> FixedSizeDescription FloatingSizeDescription
%type <type_def> TypeDefinition
%type <int_val> INTEGRAL_VAL
%type <approx_val> APPROX_VAL
%type <key_type> KeyOption
%type <join_kind> JoinOp
%type <col_def_list> ColumnDefinitionList
%type <col_def> ColumnDefinition
%type <col_pos> AlterColPosOption
%type <alter_table_spce> AlterTableSpec
%type <alter_table_spce_list> AlterTableSpecList
%type <name_list> NameList NameListOption
%type <expr> Expression BoolPrimary Predicate BitExpression Simple OnClause WhereClause HavingClause Subquery Value DefaultOption
%type <expr_list> ExpressionList GroupByClause ValueList RowValues
%type <proj_col> ProjectionColumn
%type <proj_col_list> ProjectionColumnList
%type <query> Relation FromClause
%type <order_by> OrderByClause
%type <limit> LimitOffsetClause UpdateLimitOption
%type <row_vals_list> RowValuesList
%type <assignment> Assignment
%type <assignment_list> AssignmentList OnDuplicateClause
%type <id> Identifier


%right ASSIGN
%left OP_OR
%left XOR
%left OP_AND
%nonassoc IN IS LIKE REGEXP
%left NOT '!'
%left BETWEEN
%left <op> COMPARISON
%left '|'
%left '&'
%left LSHIFT RSHIFT
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

Statement: Command ';'

Command: DDL
| DML
| TXN_BEGIN TRANSACTION {
    $$ = ctx->factory->NewTCLStatement(ast::TCLStatement::TXN_BEGIN);
}
| TXN_COMMIT {
    $$ = ctx->factory->NewTCLStatement(ast::TCLStatement::TXN_COMMIT);
}
| TXN_ROLLBACK {
    $$ = ctx->factory->NewTCLStatement(ast::TCLStatement::TXN_ROLLBACK);
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

ColumnDefinition: Name TypeDefinition NullOption AutoIncrementOption DefaultOption KeyOption CommentOption {
    auto *def = ctx->factory->NewColumnDefinition($1, $2);
    def->set_is_not_null($3);
    def->set_auto_increment($4);
    def->set_default_value($5);
    def->set_key($6);
    def->set_comment($7);
    $$ = def;
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

DefaultOption : DEFAULT Expression {
    $$ = $2;
}
| {
    $$ = nullptr;
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
| ADD INDEX Name KeyOption '(' NameList ')' {
    $$ = ctx->factory->NewAlterTableAddIndex($3, $4 == SQL_NOT_KEY ? SQL_KEY : $4,
                                             $6);
}
| ADD KEY Name KeyOption '(' NameList ')' {
    $$ = ctx->factory->NewAlterTableAddIndex($3, $4 == SQL_NOT_KEY ? SQL_KEY : $4,
                                             $6);
}
| CHANGE Name ColumnDefinition AlterColPosOption {
    $$ = ctx->factory->NewAlterTableChangeColumn($2, $3, $4.after, $4.name);
}
| CHANGE COLUMN Name ColumnDefinition AlterColPosOption {
    $$ = ctx->factory->NewAlterTableChangeColumn($3, $4, $5.after, $5.name);
}
| RENAME COLUMN Name TO Name {
    $$ = ctx->factory->NewAlterTableRenameColumn($3, $5);
}
| RENAME INDEX Name TO Name {
    $$ = ctx->factory->NewAlterTableRenameIndex($3, $5);
}
| RENAME KEY Name TO Name {
    $$ = ctx->factory->NewAlterTableRenameIndex($3, $5);
}
| RENAME Name {
    $$ = ctx->factory->NewAlterTableRename($2);
}
| RENAME TO Name {
    $$ = ctx->factory->NewAlterTableRename($3);
}
| RENAME AS Name {
    $$ = ctx->factory->NewAlterTableRename($3);
}
| DROP COLUMN Name {
    $$ = ctx->factory->NewAlterTableDropColumn($3);
}
| DROP Name {
    $$ = ctx->factory->NewAlterTableDropColumn($2);
}
| DROP INDEX Name {
    $$ = ctx->factory->NewAlterTableDropIndex($3, false);
}
| DROP KEY Name {
    $$ = ctx->factory->NewAlterTableDropIndex($3, false);
}
| DROP PRIMARY KEY {
    $$ = ctx->factory->NewAlterTableDropIndex(AstString::kEmpty, true);
}

AlterColPosOption : FIRST Name {
    $$.name  = $2;
    $$.after = false;
}
| AFTER Name {
    $$.name  = $2;
    $$.after = true;
}
| {
    $$.name  = AstString::kEmpty;
    $$.after = false;
}

NameList : Name {
    $$ = ctx->factory->NewNameList($1);
}
| NameList ',' Name {
    $$->push_back($3);
}

//-----------------------------------------------------------------------------
// DML: INSERT UPDATE DELETE
//-----------------------------------------------------------------------------
DML : SelectStmt 
| InsertStmt
| UpdateStmt

SelectStmt : SELECT DistinctOption ProjectionColumnList FromClause WhereClause OrderByClause GroupByClause HavingClause LimitOffsetClause ForUpdateOption {
    auto *stmt = ctx->factory->NewSelect($2, $3, AstString::kEmpty);
    stmt->set_from_clause($4);
    stmt->set_where_clause($5);
    stmt->set_order_by_desc($6.desc);
    stmt->set_order_by_clause($6.expr_list);
    stmt->set_group_by_clause($7);
    stmt->set_having_clause($8);
    stmt->set_limit_val($9.limit_val);
    stmt->set_offset_val($9.offset_val);
    stmt->set_for_update($10);
    $$ = stmt;
}

DistinctOption : DISTINCT {
    $$ = true;
}
| ALL {
    $$ = false;
}
| {
    $$ = false;
}

ProjectionColumnList : ProjectionColumn {
    $$ = ctx->factory->NewProjectionColumnList($1);
}
| ProjectionColumnList ',' ProjectionColumn {
    $$->push_back($3);
}

ProjectionColumn : Expression AliasOption {
    $$ = ctx->factory->NewProjectionColumn($1, $2,
                                           Location::Concat(@1, @2));
}
| Name '.' '*' {
    Identifier *id = ctx->factory->NewIdentifierWithPlaceholder($1,
        ctx->factory->NewStarPlaceholder(@3),
        Location::Concat(@1, @3));
    $$ = ctx->factory->NewProjectionColumn(id, AstString::kEmpty,
        Location::Concat(@1, @3));
}
| '*' {
    Placeholder *ph = ctx->factory->NewStarPlaceholder(@1);
    $$ = ctx->factory->NewProjectionColumn(ph, AstString::kEmpty, @1);
}


AliasOption : Alias 
| {
    $$ = AstString::kEmpty;
}

Alias : AS Name {
    $$ = $2;
}
| Name {
    $$ = $1;
}

FromClause : FROM Relation {
    $$ = $2;
}
| {
    $$ = nullptr;
}

Relation : '(' SelectStmt ')' Alias {
    Query *query = ::mai::down_cast<Query>($2);
    query->set_alias($4);
    $$ = query;
}
| Relation Alias ',' Relation Alias OnClause {
    $1->set_alias($2);
    $4->set_alias($5);
    $$ = ctx->factory->NewJoinRelation($1, SQL_CROSS_JOIN, $4, $6,
        AstString::kEmpty);
}
| Relation Alias JoinOp Relation Alias OnClause {
    $1->set_alias($2);
    $4->set_alias($5);
    $$ = ctx->factory->NewJoinRelation($1, $3, $4, $6, AstString::kEmpty);
}
| Identifier AliasOption {
    $$ = ctx->factory->NewNameRelation($1, $2);
}

OnClause : ON '(' Expression ')' {
    $$ = $3;
}
| {
    $$ = nullptr;
}

JoinOp : JOIN {
    $$ = SQL_CROSS_JOIN;
}
| CROSS JOIN {
    $$ = SQL_CROSS_JOIN;
}
| LEFT JOIN {
    $$ = SQL_LEFT_OUTTER_JOIN;
}
| LEFT OUTTER JOIN {
    $$ = SQL_LEFT_OUTTER_JOIN;
}
| RIGHT JOIN {
    $$ = SQL_RIGHT_OUTTER_JOIN;
}
| RIGHT OUTTER JOIN {
    $$ = SQL_RIGHT_OUTTER_JOIN;
}

WhereClause : WHERE Expression {
    $$ = $2;
}
| {
    $$ = nullptr;
}

HavingClause : HAVING '(' Expression ')' {
    $$ = $3;
}
| {
    $$ = nullptr;
}

OrderByClause : ORDER BY ExpressionList ASC {
    $$.expr_list = $3;
    $$.desc = false;
}
| ORDER BY ExpressionList DESC {
    $$.expr_list = $3;
    $$.desc = true;
}
| ORDER BY ExpressionList {
    $$.expr_list = $3;
    $$.desc = false;
}
| {
    $$.expr_list = nullptr;
    $$.desc = false;
}

GroupByClause : GROUP BY ExpressionList {
    $$ = $3;
}
| {
    $$ = nullptr;
}

LimitOffsetClause : LIMIT INTEGRAL_VAL {
    $$.limit_val  = $2;
    $$.offset_val = 0;
}
| LIMIT INTEGRAL_VAL ',' INTEGRAL_VAL {
    $$.offset_val = $2;
    $$.limit_val  = $4;
}
| LIMIT INTEGRAL_VAL OFFSET INTEGRAL_VAL {
    $$.limit_val  = $2;
    $$.offset_val = $4;
}
| {
    $$.limit_val  = 0;
    $$.offset_val = 0;
}


ForUpdateOption : FOR UPDATE {
    $$ = true;
}
| {
    $$ = false;
}

InsertStmt : INSERT OverwriteOption INTO Identifier NameListOption ValueToken RowValuesList OnDuplicateClause {
    Insert *stmt = ctx->factory->NewInsert($2, $4);
    stmt->set_col_names($5);
    stmt->set_row_values_list($7);
    stmt->set_on_duplicate_clause($8);
    $$ = stmt;
}
| INSERT OverwriteOption INTO Identifier SET AssignmentList OnDuplicateClause {
    Insert *stmt = ctx->factory->NewInsert($2, $4);
    stmt->SetAssignmentList($6, ctx->factory->arena());
    stmt->set_on_duplicate_clause($7);
    $$ = stmt;
}
| INSERT OverwriteOption INTO Identifier NameListOption SelectStmt OnDuplicateClause {
    Insert *stmt = ctx->factory->NewInsert($2, $4);
    stmt->set_col_names($5);
    stmt->set_select_clause(::mai::down_cast<Query>($6));
    stmt->set_on_duplicate_clause($7);
    $$ = stmt;
}

NameListOption : NameList
| {
    $$ = nullptr;
}

OverwriteOption : OVERWRITE {
    $$ = true;
}
| {
    $$ = false;
}

ValueToken : VALUE
| VALUES

RowValuesList : RowValues {
    $$ = ctx->factory->NewRowValuesList($1);
}
| RowValuesList ',' RowValues {
    $$->push_back($3);
}

RowValues : '(' ValueList ')' {
    $$ = $2;
}

ValueList : Value {
    $$ = ctx->factory->NewExpressionList($1);
}
| ValueList ',' Value {
    $$->push_back($3);
}

Value : Simple
| DEFAULT {
    $$ = ctx->factory->NewDefaultPlaceholderLiteral(@1);
}

OnDuplicateClause : ON DUPLICATE KEY UPDATE AssignmentList {
    $$ = $5;
} 
| {
    $$ = nullptr;
}

AssignmentList : Assignment {
    $$ = ctx->factory->NewAssignmentList($1);
}
| AssignmentList ',' Assignment {
    $$->push_back($3);
}

Assignment : Name COMPARISON Value {
    if ($2 != SQL_CMP_EQ) {
        yyerror(&@1, ctx, "incorrect assignment.");
        YYERROR;
    }
    $$ = ctx->factory->NewAssignment($1, $3, Location::Concat(@1, @3));
}


UpdateStmt : UPDATE Identifier SET AssignmentList WhereClause OrderByClause UpdateLimitOption {
    Update *stmt = ctx->factory->NewUpdate($2, $4);
    stmt->set_where_clause($5);
    stmt->set_order_by_desc($6.desc);
    stmt->set_order_by_clause($6.expr_list);
    stmt->set_limit_val($7.limit_val);
    $$ = stmt;
}

UpdateLimitOption : LIMIT INTEGRAL_VAL {
    $$.limit_val  = $2;
    $$.offset_val = 0;
}
| {
    $$.limit_val  = 0;
    $$.offset_val = 0;
}



//-----------------------------------------------------------------------------
// Expressions:
//-----------------------------------------------------------------------------
ExpressionList: Expression {
    $$ = ctx->factory->NewExpressionList($1);
}
| ExpressionList ',' Expression {
    $$->push_back($3);
}

//-----------------------------------------------------------------------------
// Expressions
//-----------------------------------------------------------------------------
Expression : Expression OP_OR Expression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_OR, $3, Location::Concat(@1, @3));
}
| Expression XOR Expression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_XOR, $3, Location::Concat(@1, @3));
}
| Expression OP_AND Expression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_AND, $3, Location::Concat(@1, @3));
}
| NOT Expression {
    $$ = ctx->factory->NewUnaryExpression(SQL_NOT, $2, Location::Concat(@1, @2));
}
| '!' Expression {
    $$ = ctx->factory->NewUnaryExpression(SQL_NOT, $2, Location::Concat(@1, @2));
}
| '(' Expression ')' {
    $$ = $2;
}
| BoolPrimary

//-----------------------------------------------------------------------------
// BoolPrimary
//-----------------------------------------------------------------------------
BoolPrimary : BoolPrimary IS NULL_VAL {
    $$ = ctx->factory->NewUnaryExpression(SQL_IS_NULL, $1, Location::Concat(@1, @3));
}
| BoolPrimary IS NOT NULL_VAL {
    $$ = ctx->factory->NewUnaryExpression(SQL_IS_NOT_NULL, $1, Location::Concat(@1, @4));
}
| BoolPrimary COMPARISON Predicate {
    $$ = ctx->factory->NewComparison($1, $2, $3, Location::Concat(@1, @3));
}
| Expression COMPARISON ALL Subquery {
    $$ = ctx->factory->NewComparison($1, $2, $4, Location::Concat(@1, @4));
}
| Expression COMPARISON ANY Subquery {
    $$ = ctx->factory->NewComparison($1, $2, $4, Location::Concat(@1, @4));
}
| Predicate

//-----------------------------------------------------------------------------
// Predicate
//-----------------------------------------------------------------------------
Predicate : BitExpression NOT IN Subquery {
    $$ = ctx->factory->NewMultiExpression($1, SQL_NOT_IN, $4, Location::Concat(@1, @4));
}
| BitExpression IN Subquery {
    $$ = ctx->factory->NewMultiExpression($1, SQL_IN, $3, Location::Concat(@1, @3));
}
| BitExpression NOT IN '(' ExpressionList ')' {
    $$ = ctx->factory->NewMultiExpression($1, SQL_NOT_IN, $5, Location::Concat(@1, @6));
}
| BitExpression IN '(' ExpressionList ')' {
    $$ = ctx->factory->NewMultiExpression($1, SQL_IN, $4, Location::Concat(@1, @5));
}
| BitExpression NOT BETWEEN BitExpression OP_AND Predicate {
    $$ = ctx->factory->NewMultiExpression($1, SQL_NOT_BETWEEN, $4, $6, Location::Concat(@1, @6));
}
| BitExpression BETWEEN BitExpression OP_AND Predicate {
    $$ = ctx->factory->NewMultiExpression($1, SQL_BETWEEN, $3, $5, Location::Concat(@1, @5));
}
| BitExpression NOT LIKE BitExpression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_NOT_LIKE, $4, Location::Concat(@1, @4));
}
| BitExpression LIKE BitExpression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_LIKE, $3, Location::Concat(@1, @3));
} 
| BitExpression


//-----------------------------------------------------------------------------
// BitExpression
//-----------------------------------------------------------------------------
BitExpression : BitExpression '|' BitExpression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_BIT_OR, $3, Location::Concat(@1, @3));
}
| BitExpression '&' BitExpression  {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_BIT_AND, $3, Location::Concat(@1, @3));
}
| BitExpression LSHIFT BitExpression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_LSHIFT, $3, Location::Concat(@1, @3));
}
| BitExpression RSHIFT BitExpression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_RSHIFT, $3, Location::Concat(@1, @3));
}
| BitExpression '+' BitExpression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_PLUS, $3, Location::Concat(@1, @3));
}
| BitExpression '-' BitExpression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_SUB, $3, Location::Concat(@1, @3));
}
| BitExpression '*' BitExpression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_MUL, $3, Location::Concat(@1, @3));
}
| BitExpression '/' BitExpression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_DIV, $3, Location::Concat(@1, @3));
}
| BitExpression DIV BitExpression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_DIV, $3, Location::Concat(@1, @3));
}
| BitExpression '%' BitExpression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_MOD, $3, Location::Concat(@1, @3));
}
| BitExpression MOD BitExpression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_MOD, $3, Location::Concat(@1, @3));
}
| BitExpression '^' BitExpression {
    $$ = ctx->factory->NewBinaryExpression($1, SQL_BIT_XOR, $3, Location::Concat(@1, @3));
}
| Simple


//-----------------------------------------------------------------------------
// Simple
//-----------------------------------------------------------------------------
Simple : Identifier {
    $$ = $1;
}
| STRING_VAL {
    $$ = ctx->factory->NewStringLiteral($1.buf, $1.len, @1);
}
| INTEGRAL_VAL {
    $$ = ctx->factory->NewIntegerLiteral($1, @1);
}
| APPROX_VAL {
    $$ = ctx->factory->NewApproxLiteral($1, @1);
}
| '?' {
    $$ = ctx->factory->NewParamPlaceholder(@1);
}
| '-' Simple {
    $$ = ctx->factory->NewUnaryExpression(SQL_MINUS, $2, Location::Concat(@1, @2));
}
| '~' Simple {
    $$ = ctx->factory->NewUnaryExpression(SQL_BIT_INV, $2, Location::Concat(@1, @2));
}


Subquery : '(' SelectStmt ')' {
    $$ = ctx->factory->NewSubquery(true, ::mai::down_cast<Query>($2), @2);
}

Identifier : Name {
    $$ = ctx->factory->NewIdentifier(AstString::kEmpty, $1, @1);
}
| Name '.' Name {
    $$ = ctx->factory->NewIdentifier($1, $3, Location::Concat(@1, @3));
}

Name : ID {
    $$ = ctx->factory->NewString($1.buf, $1.len);
}

%%
void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg) {
    ctx->err_line   = yyl->first_line;
    ctx->err_column = yyl->first_column;
    ctx->err_msg    = ctx->factory->NewString(msg);
}
