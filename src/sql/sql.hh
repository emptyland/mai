/* A Bison parser, made by GNU Bison 3.2.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_SQL_HH_INCLUDED
# define YY_YY_SQL_HH_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    SELECT = 258,
    FROM = 259,
    CREATE = 260,
    TABLE = 261,
    TABLES = 262,
    DROP = 263,
    SHOW = 264,
    ALTER = 265,
    ADD = 266,
    RENAME = 267,
    ANY = 268,
    DIV = 269,
    UNIQUE = 270,
    PRIMARY = 271,
    KEY = 272,
    ENGINE = 273,
    TXN_BEGIN = 274,
    TRANSACTION = 275,
    COLUMN = 276,
    AFTER = 277,
    TXN_COMMIT = 278,
    TXN_ROLLBACK = 279,
    FIRST = 280,
    CHANGE = 281,
    TO = 282,
    AS = 283,
    INDEX = 284,
    DISTINCT = 285,
    HAVING = 286,
    WHERE = 287,
    JOIN = 288,
    ON = 289,
    INNER = 290,
    OUTTER = 291,
    LEFT = 292,
    RIGHT = 293,
    ALL = 294,
    CROSS = 295,
    ORDER = 296,
    BY = 297,
    ASC = 298,
    DESC = 299,
    GROUP = 300,
    FOR = 301,
    UPDATE = 302,
    LIMIT = 303,
    OFFSET = 304,
    INSERT = 305,
    OVERWRITE = 306,
    DELETE = 307,
    VALUES = 308,
    VALUE = 309,
    SET = 310,
    IN = 311,
    INTO = 312,
    DUPLICATE = 313,
    DEFAULT = 314,
    BINARY = 315,
    VARBINARY = 316,
    ID = 317,
    NULL_VAL = 318,
    INTEGRAL_VAL = 319,
    STRING_VAL = 320,
    APPROX_VAL = 321,
    DATE_VAL = 322,
    DATETIME_VAL = 323,
    EQ = 324,
    NOT = 325,
    OP_AND = 326,
    BIGINT = 327,
    INT = 328,
    SMALLINT = 329,
    TINYINT = 330,
    DECIMAL = 331,
    NUMERIC = 332,
    CHAR = 333,
    VARCHAR = 334,
    DATE = 335,
    DATETIME = 336,
    TIMESTMAP = 337,
    AUTO_INCREMENT = 338,
    COMMENT = 339,
    TOKEN_ERROR = 340,
    ASSIGN = 341,
    OP_OR = 342,
    XOR = 343,
    IS = 344,
    LIKE = 345,
    REGEXP = 346,
    BETWEEN = 347,
    COMPARISON = 348,
    LSHIFT = 349,
    RSHIFT = 350,
    MOD = 351,
    UMINUS = 352
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 34 "sql.y" /* yacc.c:1912  */

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

#line 202 "sql.hh" /* yacc.c:1912  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



int yyparse (parser_ctx *ctx);

#endif /* !YY_YY_SQL_HH_INCLUDED  */
