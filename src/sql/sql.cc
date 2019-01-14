/* A Bison parser, made by GNU Bison 3.2.4.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.2.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 11 "sql.y" /* yacc.c:338  */

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

#line 90 "sql.cc" /* yacc.c:338  */
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "sql.hh".  */
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
    ID = 284,
    NULL_VAL = 285,
    INTEGRAL_VAL = 286,
    STRING_VAL = 287,
    APPROX_VAL = 288,
    EQ = 289,
    NOT = 290,
    OP_AND = 291,
    BIGINT = 292,
    INT = 293,
    SMALLINT = 294,
    TINYINT = 295,
    DECIMAL = 296,
    NUMERIC = 297,
    CHAR = 298,
    VARCHAR = 299,
    DATE = 300,
    DATETIME = 301,
    TIMESTMAP = 302,
    AUTO_INCREMENT = 303,
    COMMENT = 304,
    ASSIGN = 305,
    OP_OR = 306,
    XOR = 307,
    IN = 308,
    IS = 309,
    LIKE = 310,
    REGEXP = 311,
    BETWEEN = 312,
    COMPARISON = 313,
    LSHIFT = 314,
    RSHIFT = 315,
    MOD = 316,
    UMINUS = 317
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 32 "sql.y" /* yacc.c:353  */

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
    double approx_val;
    bool bool_val;
    ::mai::sql::SQLKeyType key_type;
    ::mai::sql::SQLOperator op;
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
    const ::mai::sql::AstString *name;

#line 229 "sql.cc" /* yacc.c:353  */
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



#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  23
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   160

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  77
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  27
/* YYNRULES -- Number of rules.  */
#define YYNRULES  87
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  167

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   317

#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    57,     2,     2,     2,    68,    61,     2,
      73,    74,    66,    64,    75,    65,    76,    67,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    72,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    70,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    60,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    58,    59,    62,    63,    69,    71
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   115,   115,   120,   125,   127,   128,   129,   132,   135,
     138,   143,   146,   149,   153,   157,   160,   164,   169,   172,
     175,   178,   181,   184,   187,   190,   193,   196,   200,   204,
     209,   213,   218,   221,   225,   228,   231,   235,   238,   241,
     244,   247,   250,   254,   257,   261,   265,   268,   272,   275,
     278,   281,   284,   288,   292,   295,   298,   301,   304,   307,
     310,   313,   316,   319,   322,   325,   328,   332,   336,   340,
     345,   348,   354,   358,   361,   365,   368,   372,   375,   379,
     382,   383,   395,   398,   401,   404,   407,   411
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "SELECT", "FROM", "CREATE", "TABLE",
  "TABLES", "DROP", "SHOW", "ALTER", "ADD", "RENAME", "UNIQUE", "PRIMARY",
  "KEY", "ENGINE", "TXN_BEGIN", "TRANSACTION", "COLUMN", "AFTER",
  "TXN_COMMIT", "TXN_ROLLBACK", "FIRST", "CHANGE", "TO", "AS", "INDEX",
  "DISTINCT", "ID", "NULL_VAL", "INTEGRAL_VAL", "STRING_VAL", "APPROX_VAL",
  "EQ", "NOT", "OP_AND", "BIGINT", "INT", "SMALLINT", "TINYINT", "DECIMAL",
  "NUMERIC", "CHAR", "VARCHAR", "DATE", "DATETIME", "TIMESTMAP",
  "AUTO_INCREMENT", "COMMENT", "ASSIGN", "OP_OR", "XOR", "IN", "IS",
  "LIKE", "REGEXP", "'!'", "BETWEEN", "COMPARISON", "'|'", "'&'", "LSHIFT",
  "RSHIFT", "'+'", "'-'", "'*'", "'/'", "'%'", "MOD", "'^'", "UMINUS",
  "';'", "'('", "')'", "','", "'.'", "$accept", "Block", "Statement",
  "Command", "DDL", "CreateTableStmt", "ColumnDefinitionList",
  "ColumnDefinition", "TypeDefinition", "FixedSizeDescription",
  "FloatingSizeDescription", "AutoIncrementOption", "NullOption",
  "KeyOption", "CommentOption", "AlterTableStmt", "AlterTableSpecList",
  "AlterTableSpec", "AlterColPosOption", "NameList", "DML",
  "DistinctOption", "ProjectionColumnList", "ProjectionColumn",
  "AliasOption", "Expression", "Identifier", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,    33,   312,   313,
     124,    38,   314,   315,    43,    45,    42,    47,    37,   316,
      94,   317,    59,    40,    41,    44,    46
};
# endif

#define YYPACT_NINF -86

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-86)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      82,   -16,    10,    16,    31,    20,    22,   -86,   -86,    62,
     -86,   -28,   -86,   -86,   -86,   -86,   -86,    80,    30,    30,
     -86,    30,   -86,   -86,   -86,   -86,   -86,   -86,   -86,   -86,
     -20,   -86,    43,     1,     7,   -86,    24,    30,    80,   -86,
     -86,    30,    30,    30,    49,   -12,    81,   -11,    11,   -86,
     -86,   -86,   -86,   -86,    14,   -86,    94,    90,    30,    30,
      30,   -86,    30,   -15,    30,    30,    26,    30,    30,    30,
      30,    30,   -86,    30,    30,    24,   -86,    30,    41,    41,
      41,    41,    42,    42,    41,    41,   -86,   -86,    -2,   -86,
     -86,   -86,   -86,    60,    30,    26,    60,    19,    30,    30,
     -86,    70,   101,   -86,   -86,   102,    30,    26,   -86,   -86,
      98,   -86,   -86,   -86,   -86,   110,   -86,   -86,   -86,   -86,
     -86,   112,    95,   129,   131,   -86,    74,    23,   -86,    75,
     -86,   -86,   -86,    30,    30,    30,    26,   -86,    76,    73,
     -86,   -86,    60,   -86,   -86,    30,   -86,    30,   -86,   -86,
     -86,   -86,   -86,   120,   103,    27,   -86,    48,    79,   122,
     -86,   -86,    30,   -86,   -86,   -86,   -86
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    74,     0,     0,     0,     0,     0,     8,     9,     0,
       2,     0,     5,    11,    13,     6,    73,     0,     0,     0,
      10,     0,     7,     1,     3,     4,    87,    85,    84,    86,
      81,    75,    77,    82,     0,    12,     0,     0,     0,    72,
      80,     0,     0,     0,     0,     0,     0,     0,    45,    46,
      79,    76,    78,    83,     0,    15,     0,     0,     0,     0,
       0,    63,     0,     0,     0,     0,    69,     0,     0,     0,
       0,     0,    59,     0,     0,     0,    14,     0,    29,    29,
      29,    29,    31,    31,    29,    29,    26,    27,    36,    66,
      65,    62,    64,    42,     0,    69,    42,     0,     0,     0,
      48,     0,     0,    60,    61,     0,     0,    69,    47,    16,
       0,    18,    19,    20,    21,     0,    22,    23,    24,    25,
      35,     0,    33,    38,    40,    37,     0,     0,    49,     0,
      50,    68,    67,     0,     0,     0,    69,    54,     0,     0,
      34,    32,    42,    39,    41,     0,    51,     0,    58,    56,
      57,    55,    28,     0,    44,     0,    70,     0,     0,     0,
      17,    53,     0,    52,    30,    43,    71
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -86,   -86,   146,   -86,   -86,   -86,   -55,   -40,   -86,    40,
      77,   -86,   -86,   -85,   -86,   -86,   -86,    83,   -76,     9,
     -86,   -86,   -86,   119,   -86,   -86,   -17
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     9,    10,    11,    12,    13,    54,    55,    88,   111,
     116,   142,   122,   126,   160,    14,    48,    49,   100,   155,
      15,    17,    30,    31,    39,    32,    56
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      33,    34,    35,    62,    36,    66,    37,    63,    73,    26,
      97,   129,    16,    40,    26,    64,    18,    26,    26,   128,
      50,    33,    19,    95,    52,    53,    21,    61,   120,    72,
      74,   137,    44,   121,   107,    45,    46,   109,    20,   127,
      22,    90,    91,    92,    25,    93,    98,    96,    47,    99,
     101,   102,   103,   104,   105,    38,   106,   154,    94,    26,
     151,    65,    23,    57,    58,     1,   136,     2,    59,    41,
       3,     4,     5,   123,   124,   125,    60,    42,    26,     6,
      43,   131,   132,     7,     8,     1,    75,     2,    76,    77,
       3,     4,     5,   130,    77,   133,    67,   146,    77,     6,
      68,   161,   162,     7,     8,    89,    69,    70,    71,    26,
      26,    27,    28,    29,   110,   115,   148,   149,   150,   112,
     113,   114,   163,   162,   118,   119,   134,   135,   156,   138,
     156,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,   139,   140,   141,   143,   166,   144,   145,   147,   153,
     152,   158,   159,   164,   165,    24,   157,    51,   108,     0,
     117
};

static const yytype_int16 yycheck[] =
{
      17,    18,    19,    15,    21,    45,    26,    19,    19,    29,
      65,    96,    28,    30,    29,    27,     6,    29,    29,    95,
      37,    38,     6,    63,    41,    42,     6,    44,    30,    46,
      47,   107,     8,    35,    74,    11,    12,    77,     7,    94,
      18,    58,    59,    60,    72,    62,    20,    64,    24,    23,
      67,    68,    69,    70,    71,    75,    73,   142,    73,    29,
     136,    73,     0,    14,    15,     3,   106,     5,    19,    26,
       8,     9,    10,    13,    14,    15,    27,    76,    29,    17,
      73,    98,    99,    21,    22,     3,    75,     5,    74,    75,
       8,     9,    10,    74,    75,    25,    15,    74,    75,    17,
      19,    74,    75,    21,    22,    15,    25,    26,    27,    29,
      29,    31,    32,    33,    73,    73,   133,   134,   135,    79,
      80,    81,    74,    75,    84,    85,    25,    25,   145,    31,
     147,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    31,    30,    48,    15,   162,    15,    73,    73,    76,
      74,    31,    49,    74,    32,     9,   147,    38,    75,    -1,
      83
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     5,     8,     9,    10,    17,    21,    22,    78,
      79,    80,    81,    82,    92,    97,    28,    98,     6,     6,
       7,     6,    18,     0,    79,    72,    29,    31,    32,    33,
      99,   100,   102,   103,   103,   103,   103,    26,    75,   101,
     103,    26,    76,    73,     8,    11,    12,    24,    93,    94,
     103,   100,   103,   103,    83,    84,   103,    14,    15,    19,
      27,   103,    15,    19,    27,    73,    84,    15,    19,    25,
      26,    27,   103,    19,   103,    75,    74,    75,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    85,    15,
     103,   103,   103,   103,    73,    84,   103,    83,    20,    23,
      95,   103,   103,   103,   103,   103,   103,    84,    94,    84,
      73,    86,    86,    86,    86,    73,    87,    87,    86,    86,
      30,    35,    89,    13,    14,    15,    90,    83,    95,    90,
      74,   103,   103,    25,    25,    25,    84,    95,    31,    31,
      30,    48,    88,    15,    15,    73,    74,    73,   103,   103,
     103,    95,    74,    76,    90,    96,   103,    96,    31,    49,
      91,    74,    75,    74,    74,    32,   103
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    77,    78,    78,    79,    80,    80,    80,    80,    80,
      80,    81,    81,    81,    82,    83,    83,    84,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    86,    86,
      87,    87,    88,    88,    89,    89,    89,    90,    90,    90,
      90,    90,    90,    91,    91,    92,    93,    93,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    95,    95,    95,
      96,    96,    97,    98,    98,    99,    99,   100,   100,   101,
     101,   101,   102,   102,   102,   102,   102,   103
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     2,     1,     1,     2,     1,     1,
       2,     1,     3,     1,     6,     1,     3,     6,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     1,     3,     0,
       5,     0,     1,     0,     2,     1,     0,     1,     1,     2,
       1,     2,     0,     2,     0,     4,     1,     3,     3,     4,
       4,     5,     7,     7,     4,     5,     5,     5,     5,     2,
       3,     3,     3,     2,     3,     3,     3,     2,     2,     0,
       1,     3,     4,     1,     0,     1,     3,     1,     3,     2,
       1,     0,     1,     3,     1,     1,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (&yylloc, ctx, YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, Location, ctx); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, parser_ctx *ctx)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  YYUSE (yylocationp);
  YYUSE (ctx);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, parser_ctx *ctx)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyo, *yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yytype, yyvaluep, yylocationp, ctx);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, parser_ctx *ctx)
{
  unsigned long yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                       , &(yylsp[(yyi + 1) - (yynrhs)])                       , ctx);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, ctx); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return (YYSIZE_T) (yystpcpy (yyres, yystr) - yyres);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, parser_ctx *ctx)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (ctx);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (parser_ctx *ctx)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.
       'yyls': related to locations.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yylsp = yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  yylsp[0] = yylloc;
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = (yytype_int16) yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = (YYSIZE_T) (yyssp - yyss + 1);

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yyls1, yysize * sizeof (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (&yylval, &yylloc, YYLEX_PARAM);
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 115 "sql.y" /* yacc.c:1660  */
    {
    (yyval.block) = ctx->factory->NewBlock();
    (yyval.block)->AddStmt((yyvsp[0].stmt));
    ctx->block = (yyval.block);
}
#line 1558 "sql.cc" /* yacc.c:1660  */
    break;

  case 3:
#line 120 "sql.y" /* yacc.c:1660  */
    {
    (yyval.block)->AddStmt((yyvsp[0].stmt));
    ctx->block = (yyval.block);
}
#line 1567 "sql.cc" /* yacc.c:1660  */
    break;

  case 7:
#line 129 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(TCLStatement::TXN_BEGIN);
}
#line 1575 "sql.cc" /* yacc.c:1660  */
    break;

  case 8:
#line 132 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(TCLStatement::TXN_COMMIT);
}
#line 1583 "sql.cc" /* yacc.c:1660  */
    break;

  case 9:
#line 135 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(TCLStatement::TXN_ROLLBACK);
}
#line 1591 "sql.cc" /* yacc.c:1660  */
    break;

  case 10:
#line 138 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewShowTables();
}
#line 1599 "sql.cc" /* yacc.c:1660  */
    break;

  case 11:
#line 143 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1607 "sql.cc" /* yacc.c:1660  */
    break;

  case 12:
#line 146 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewDropTable((yyvsp[0].name));
}
#line 1615 "sql.cc" /* yacc.c:1660  */
    break;

  case 13:
#line 149 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1623 "sql.cc" /* yacc.c:1660  */
    break;

  case 14:
#line 153 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewCreateTable((yyvsp[-3].name), (yyvsp[-1].col_def_list));
}
#line 1631 "sql.cc" /* yacc.c:1660  */
    break;

  case 15:
#line 157 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_def_list) = ctx->factory->NewColumnDefinitionList((yyvsp[0].col_def));
}
#line 1639 "sql.cc" /* yacc.c:1660  */
    break;

  case 16:
#line 160 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_def_list)->push_back((yyvsp[0].col_def));
}
#line 1647 "sql.cc" /* yacc.c:1660  */
    break;

  case 17:
#line 164 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_def) = ctx->factory->NewColumnDefinition((yyvsp[-5].name), (yyvsp[-4].type_def), (yyvsp[-3].bool_val), (yyvsp[-2].bool_val), (yyvsp[-1].key_type));
    (yyval.col_def)->set_comment((yyvsp[0].name));
}
#line 1656 "sql.cc" /* yacc.c:1660  */
    break;

  case 18:
#line 169 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_BIGINT, (yyvsp[0].size).fixed_size);
}
#line 1664 "sql.cc" /* yacc.c:1660  */
    break;

  case 19:
#line 172 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_INT, (yyvsp[0].size).fixed_size);
}
#line 1672 "sql.cc" /* yacc.c:1660  */
    break;

  case 20:
#line 175 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_SMALLINT, (yyvsp[0].size).fixed_size);
}
#line 1680 "sql.cc" /* yacc.c:1660  */
    break;

  case 21:
#line 178 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_TINYINT, (yyvsp[0].size).fixed_size);
}
#line 1688 "sql.cc" /* yacc.c:1660  */
    break;

  case 22:
#line 181 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DECIMAL, (yyvsp[0].size).fixed_size, (yyvsp[0].size).float_size);
}
#line 1696 "sql.cc" /* yacc.c:1660  */
    break;

  case 23:
#line 184 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_NUMERIC, (yyvsp[0].size).fixed_size, (yyvsp[0].size).float_size);
}
#line 1704 "sql.cc" /* yacc.c:1660  */
    break;

  case 24:
#line 187 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_NUMERIC, (yyvsp[0].size).fixed_size);
}
#line 1712 "sql.cc" /* yacc.c:1660  */
    break;

  case 25:
#line 190 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_VARCHAR, (yyvsp[0].size).fixed_size);
}
#line 1720 "sql.cc" /* yacc.c:1660  */
    break;

  case 26:
#line 193 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DATE);
}
#line 1728 "sql.cc" /* yacc.c:1660  */
    break;

  case 27:
#line 196 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DATETIME);
}
#line 1736 "sql.cc" /* yacc.c:1660  */
    break;

  case 28:
#line 200 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = (yyvsp[-1].int_val);
    (yyval.size).float_size = 0;
}
#line 1745 "sql.cc" /* yacc.c:1660  */
    break;

  case 29:
#line 204 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = 0;
    (yyval.size).float_size = 0;
}
#line 1754 "sql.cc" /* yacc.c:1660  */
    break;

  case 30:
#line 209 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = (yyvsp[-3].int_val);
    (yyval.size).float_size = (yyvsp[-1].int_val);
}
#line 1763 "sql.cc" /* yacc.c:1660  */
    break;

  case 31:
#line 213 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = 0;
    (yyval.size).float_size = 0;
}
#line 1772 "sql.cc" /* yacc.c:1660  */
    break;

  case 32:
#line 218 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 1780 "sql.cc" /* yacc.c:1660  */
    break;

  case 33:
#line 221 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 1788 "sql.cc" /* yacc.c:1660  */
    break;

  case 34:
#line 225 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 1796 "sql.cc" /* yacc.c:1660  */
    break;

  case 35:
#line 228 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 1804 "sql.cc" /* yacc.c:1660  */
    break;

  case 36:
#line 231 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 1812 "sql.cc" /* yacc.c:1660  */
    break;

  case 37:
#line 235 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_KEY;
}
#line 1820 "sql.cc" /* yacc.c:1660  */
    break;

  case 38:
#line 238 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_UNIQUE_KEY;
}
#line 1828 "sql.cc" /* yacc.c:1660  */
    break;

  case 39:
#line 241 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_UNIQUE_KEY;
}
#line 1836 "sql.cc" /* yacc.c:1660  */
    break;

  case 40:
#line 244 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_PRIMARY_KEY;
}
#line 1844 "sql.cc" /* yacc.c:1660  */
    break;

  case 41:
#line 247 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_PRIMARY_KEY;
}
#line 1852 "sql.cc" /* yacc.c:1660  */
    break;

  case 42:
#line 250 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_NOT_KEY;
}
#line 1860 "sql.cc" /* yacc.c:1660  */
    break;

  case 43:
#line 254 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = ctx->factory->NewString((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 1868 "sql.cc" /* yacc.c:1660  */
    break;

  case 44:
#line 257 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = AstString::kEmpty;
}
#line 1876 "sql.cc" /* yacc.c:1660  */
    break;

  case 45:
#line 261 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewAlterTable((yyvsp[-1].name), (yyvsp[0].alter_table_spce_list));
}
#line 1884 "sql.cc" /* yacc.c:1660  */
    break;

  case 46:
#line 265 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce_list) = ctx->factory->NewAlterTableSpecList((yyvsp[0].alter_table_spce));
}
#line 1892 "sql.cc" /* yacc.c:1660  */
    break;

  case 47:
#line 268 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce_list)->push_back((yyvsp[0].alter_table_spce));
}
#line 1900 "sql.cc" /* yacc.c:1660  */
    break;

  case 48:
#line 272 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 1908 "sql.cc" /* yacc.c:1660  */
    break;

  case 49:
#line 275 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 1916 "sql.cc" /* yacc.c:1660  */
    break;

  case 50:
#line 278 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def_list));
}
#line 1924 "sql.cc" /* yacc.c:1660  */
    break;

  case 51:
#line 281 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def_list));
}
#line 1932 "sql.cc" /* yacc.c:1660  */
    break;

  case 52:
#line 284 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddIndex((yyvsp[-4].name), (yyvsp[-3].key_type) == SQL_NOT_KEY ? SQL_KEY : (yyvsp[-3].key_type),
                                             (yyvsp[-1].name_list));
}
#line 1941 "sql.cc" /* yacc.c:1660  */
    break;

  case 53:
#line 288 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddIndex((yyvsp[-4].name), (yyvsp[-3].key_type) == SQL_NOT_KEY ? SQL_KEY : (yyvsp[-3].key_type),
                                             (yyvsp[-1].name_list));
}
#line 1950 "sql.cc" /* yacc.c:1660  */
    break;

  case 54:
#line 292 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableChangeColumn((yyvsp[-2].name), (yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 1958 "sql.cc" /* yacc.c:1660  */
    break;

  case 55:
#line 295 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableChangeColumn((yyvsp[-2].name), (yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 1966 "sql.cc" /* yacc.c:1660  */
    break;

  case 56:
#line 298 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameColumn((yyvsp[-2].name), (yyvsp[0].name));
}
#line 1974 "sql.cc" /* yacc.c:1660  */
    break;

  case 57:
#line 301 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameIndex((yyvsp[-2].name), (yyvsp[0].name));
}
#line 1982 "sql.cc" /* yacc.c:1660  */
    break;

  case 58:
#line 304 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameIndex((yyvsp[-2].name), (yyvsp[0].name));
}
#line 1990 "sql.cc" /* yacc.c:1660  */
    break;

  case 59:
#line 307 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 1998 "sql.cc" /* yacc.c:1660  */
    break;

  case 60:
#line 310 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2006 "sql.cc" /* yacc.c:1660  */
    break;

  case 61:
#line 313 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2014 "sql.cc" /* yacc.c:1660  */
    break;

  case 62:
#line 316 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropColumn((yyvsp[0].name));
}
#line 2022 "sql.cc" /* yacc.c:1660  */
    break;

  case 63:
#line 319 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropColumn((yyvsp[0].name));
}
#line 2030 "sql.cc" /* yacc.c:1660  */
    break;

  case 64:
#line 322 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex((yyvsp[0].name), false);
}
#line 2038 "sql.cc" /* yacc.c:1660  */
    break;

  case 65:
#line 325 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex((yyvsp[0].name), false);
}
#line 2046 "sql.cc" /* yacc.c:1660  */
    break;

  case 66:
#line 328 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex(AstString::kEmpty, true);
}
#line 2054 "sql.cc" /* yacc.c:1660  */
    break;

  case 67:
#line 332 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = (yyvsp[0].name);
    (yyval.col_pos).after = false;
}
#line 2063 "sql.cc" /* yacc.c:1660  */
    break;

  case 68:
#line 336 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = (yyvsp[0].name);
    (yyval.col_pos).after = true;
}
#line 2072 "sql.cc" /* yacc.c:1660  */
    break;

  case 69:
#line 340 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = AstString::kEmpty;
    (yyval.col_pos).after = false;
}
#line 2081 "sql.cc" /* yacc.c:1660  */
    break;

  case 70:
#line 345 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name_list) = ctx->factory->NewNameList((yyvsp[0].name));
}
#line 2089 "sql.cc" /* yacc.c:1660  */
    break;

  case 71:
#line 348 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name_list)->push_back((yyvsp[0].name));
}
#line 2097 "sql.cc" /* yacc.c:1660  */
    break;

  case 72:
#line 354 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewSelect((yyvsp[-2].bool_val), (yyvsp[-1].proj_col_list), (yyvsp[0].name));
}
#line 2105 "sql.cc" /* yacc.c:1660  */
    break;

  case 73:
#line 358 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2113 "sql.cc" /* yacc.c:1660  */
    break;

  case 74:
#line 361 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2121 "sql.cc" /* yacc.c:1660  */
    break;

  case 75:
#line 365 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col_list) = ctx->factory->NewProjectionColumnList((yyvsp[0].proj_col));
}
#line 2129 "sql.cc" /* yacc.c:1660  */
    break;

  case 76:
#line 368 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col_list)->push_back((yyvsp[0].proj_col));
}
#line 2137 "sql.cc" /* yacc.c:1660  */
    break;

  case 77:
#line 372 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col) = ctx->factory->NewProjectionColumn((yyvsp[0].expr), AstString::kEmpty);
}
#line 2145 "sql.cc" /* yacc.c:1660  */
    break;

  case 78:
#line 375 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col) = ctx->factory->NewProjectionColumn((yyvsp[-2].expr), (yyvsp[0].name));
}
#line 2153 "sql.cc" /* yacc.c:1660  */
    break;

  case 79:
#line 379 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = (yyvsp[0].name);
}
#line 2161 "sql.cc" /* yacc.c:1660  */
    break;

  case 81:
#line 383 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = AstString::kEmpty;
}
#line 2169 "sql.cc" /* yacc.c:1660  */
    break;

  case 82:
#line 395 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewIdentifier(AstString::kEmpty, (yyvsp[0].name));
}
#line 2177 "sql.cc" /* yacc.c:1660  */
    break;

  case 83:
#line 398 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewIdentifier((yyvsp[-2].name), (yyvsp[0].name));   
}
#line 2185 "sql.cc" /* yacc.c:1660  */
    break;

  case 84:
#line 401 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewStringLiteral((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 2193 "sql.cc" /* yacc.c:1660  */
    break;

  case 85:
#line 404 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewIntegerLiteral((yyvsp[0].int_val));
}
#line 2201 "sql.cc" /* yacc.c:1660  */
    break;

  case 86:
#line 407 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewApproxLiteral((yyvsp[0].approx_val));
}
#line 2209 "sql.cc" /* yacc.c:1660  */
    break;

  case 87:
#line 411 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = ctx->factory->NewString((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 2217 "sql.cc" /* yacc.c:1660  */
    break;


#line 2221 "sql.cc" /* yacc.c:1660  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, ctx, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (&yylloc, ctx, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, ctx);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, yylsp, ctx);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, ctx, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, ctx);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, yylsp, ctx);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 415 "sql.y" /* yacc.c:1903  */

void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg) {
    ctx->err_line   = yyl->first_line;
    ctx->err_column = yyl->first_column;
    ctx->err_msg    = ctx->factory->NewString(msg);
}