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
    DROP = 262,
    UNIQUE = 263,
    PRIMARY = 264,
    KEY = 265,
    ENGINE = 266,
    TXN_BEGIN = 267,
    TRANSACTION = 268,
    COMMIT = 269,
    ROLLBACK = 270,
    ID = 271,
    NULL_VAL = 272,
    INTEGRAL_VAL = 273,
    STRING_VAL = 274,
    EQ = 275,
    NOT = 276,
    BIGINT = 277,
    INT = 278,
    SMALLINT = 279,
    TINYINT = 280,
    DECIMAL = 281,
    NUMERIC = 282,
    CHAR = 283,
    VARCHAR = 284,
    DATE = 285,
    DATETIME = 286,
    TIMESTMAP = 287
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 30 "sql.y" /* yacc.c:1912  */

    struct {
        const char *buf;
        size_t      len;
    } text;
    int int_val;

#line 98 "sql.hh" /* yacc.c:1912  */
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
