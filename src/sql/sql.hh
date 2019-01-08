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
    TOK_SELECT = 258,
    TOK_STAR = 259,
    TOK_FROM = 260,
    TOK_CREATE = 261,
    TOK_TABLE = 262,
    TOK_DROP = 263,
    TOK_UNIQUE = 264,
    TOK_PRIMARY = 265,
    TOK_KEY = 266,
    TOK_ENGINE = 267,
    TOK_BEGIN = 268,
    TOK_TRANSACTION = 269,
    TOK_COMMIT = 270,
    TOK_ROLLBACK = 271,
    TOK_LPAREN = 272,
    TOK_RPAREN = 273,
    TOK_COMMA = 274,
    TOK_ID = 275,
    TOK_NULL = 276,
    TOK_INTEGRAL_VAL = 277,
    TOK_STRING_VAL = 278,
    TOK_EQ = 279,
    TOK_NOT = 280,
    TOK_BIGINT = 281,
    TOK_INT = 282,
    TOK_SMALLINT = 283,
    TOK_TINYINT = 284,
    TOK_DECIMAL = 285,
    TOK_NUMERIC = 286,
    TOK_CHAR = 287,
    TOK_VARCHAR = 288,
    TOK_DATE = 289,
    TOK_DATETIME = 290,
    TOK_TIMESTMAP = 291
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 28 "sql.y" /* yacc.c:1912  */

    struct {
        const char *buf;
        size_t      len;
    } text;

#line 101 "sql.hh" /* yacc.c:1912  */
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
