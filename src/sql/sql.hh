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
    UNIQUE = 268,
    PRIMARY = 269,
    KEY = 270,
    ENGINE = 271,
    TXN_BEGIN = 272,
    TRANSACTION = 273,
    COLUMN = 274,
    AFTER = 275,
    TXN_COMMIT = 276,
    TXN_ROLLBACK = 277,
    FIRST = 278,
    CHANGE = 279,
    TO = 280,
    AS = 281,
    INDEX = 282,
    DISTINCT = 283,
    HAVING = 284,
    WHERE = 285,
    JOIN = 286,
    ON = 287,
    INNER = 288,
    OUTTER = 289,
    LEFT = 290,
    RIGHT = 291,
    ALL = 292,
    CROSS = 293,
    ORDER = 294,
    BY = 295,
    ASC = 296,
    DESC = 297,
    GROUP = 298,
    FOR = 299,
    UPDATE = 300,
    LIMIT = 301,
    OFFSET = 302,
    INSERT = 303,
    OVERWRITE = 304,
    DELETE = 305,
    VALUES = 306,
    SET = 307,
    ID = 308,
    NULL_VAL = 309,
    INTEGRAL_VAL = 310,
    STRING_VAL = 311,
    APPROX_VAL = 312,
    EQ = 313,
    NOT = 314,
    OP_AND = 315,
    BIGINT = 316,
    INT = 317,
    SMALLINT = 318,
    TINYINT = 319,
    DECIMAL = 320,
    NUMERIC = 321,
    CHAR = 322,
    VARCHAR = 323,
    DATE = 324,
    DATETIME = 325,
    TIMESTMAP = 326,
    AUTO_INCREMENT = 327,
    COMMENT = 328,
    ASSIGN = 329,
    OP_OR = 330,
    XOR = 331,
    IN = 332,
    IS = 333,
    LIKE = 334,
    REGEXP = 335,
    BETWEEN = 336,
    COMPARISON = 337,
    LSHIFT = 338,
    RSHIFT = 339,
    MOD = 340,
    UMINUS = 341
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 32 "sql.y" /* yacc.c:1912  */

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
        ::mai::sql::ExpressionList *expr_list;
        bool desc;
    } order_by;
    int int_val;
    double approx_val;
    bool bool_val;
    ::mai::sql::SQLKeyType key_type;
    ::mai::sql::SQLOperator op;
    ::mai::sql::SQLJoinKind join_kind;
    ::mai::sql::Block *block;
    ::mai::sql::Statement *stmt;
    ::mai::sql::TypeDefinition *type_def;
    ::mai::sql::ColumnDefinition *col_def;
    ::mai::sql::ColumnDefinitionList *col_def_list;
    ::mai::sql::AlterTableSpecList *alter_table_spce_list;
    ::mai::sql::AlterTableSpec *alter_table_spce;
    ::mai::sql::NameList *name_list;
    ::mai::sql::Expression *expr;
    ::mai::sql::ExpressionList *expr_list;
    ::mai::sql::ProjectionColumn *proj_col;
    ::mai::sql::ProjectionColumnList *proj_col_list;
    ::mai::sql::Query *query;
    const ::mai::sql::AstString *name;

#line 187 "sql.hh" /* yacc.c:1912  */
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
