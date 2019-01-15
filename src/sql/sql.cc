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
#line 12 "sql.y" /* yacc.c:338  */

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
# define YYERROR_VERBOSE 1
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
    SET = 309,
    IN = 310,
    ID = 311,
    NULL_VAL = 312,
    INTEGRAL_VAL = 313,
    STRING_VAL = 314,
    APPROX_VAL = 315,
    DATE_VAL = 316,
    DATETIME_VAL = 317,
    EQ = 318,
    NOT = 319,
    OP_AND = 320,
    BIGINT = 321,
    INT = 322,
    SMALLINT = 323,
    TINYINT = 324,
    DECIMAL = 325,
    NUMERIC = 326,
    CHAR = 327,
    VARCHAR = 328,
    DATE = 329,
    DATETIME = 330,
    TIMESTMAP = 331,
    AUTO_INCREMENT = 332,
    COMMENT = 333,
    TOKEN_ERROR = 334,
    ASSIGN = 335,
    OP_OR = 336,
    XOR = 337,
    IS = 338,
    LIKE = 339,
    REGEXP = 340,
    BETWEEN = 341,
    COMPARISON = 342,
    LSHIFT = 343,
    RSHIFT = 344,
    MOD = 345,
    UMINUS = 346
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 33 "sql.y" /* yacc.c:353  */

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

#line 268 "sql.cc" /* yacc.c:353  */
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
#define YYFINAL  25
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   462

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  108
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  45
/* YYNRULES -- Number of rules.  */
#define YYNRULES  163
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  322

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   346

#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    86,     2,     2,     2,    97,    90,     2,
     102,   103,    95,    93,   104,    94,   105,    96,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   101,
       2,     2,     2,   106,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    99,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    89,     2,   107,     2,     2,     2,
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
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    87,    88,    91,    92,    98,   100
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   134,   134,   139,   144,   146,   147,   148,   151,   154,
     157,   162,   165,   168,   172,   176,   179,   183,   188,   191,
     194,   197,   200,   203,   206,   209,   212,   215,   219,   223,
     228,   232,   237,   240,   244,   247,   250,   254,   257,   260,
     263,   266,   269,   273,   276,   280,   284,   287,   291,   294,
     297,   300,   303,   307,   311,   314,   317,   320,   323,   326,
     329,   332,   335,   338,   341,   344,   347,   351,   355,   359,
     364,   367,   373,   375,   389,   392,   395,   399,   402,   406,
     410,   417,   423,   424,   428,   431,   435,   438,   442,   447,
     453,   458,   462,   467,   470,   474,   477,   480,   483,   486,
     489,   493,   496,   500,   503,   507,   511,   515,   519,   524,
     527,   531,   535,   539,   543,   549,   552,   559,   562,   569,
     572,   575,   578,   581,   584,   587,   592,   595,   598,   601,
     604,   607,   612,   615,   618,   621,   624,   627,   630,   633,
     636,   642,   645,   648,   651,   654,   657,   660,   663,   666,
     669,   672,   675,   678,   684,   687,   690,   693,   696,   699,
     702,   705,   710,   714
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "SELECT", "FROM", "CREATE", "TABLE",
  "TABLES", "DROP", "SHOW", "ALTER", "ADD", "RENAME", "ANY", "DIV",
  "UNIQUE", "PRIMARY", "KEY", "ENGINE", "TXN_BEGIN", "TRANSACTION",
  "COLUMN", "AFTER", "TXN_COMMIT", "TXN_ROLLBACK", "FIRST", "CHANGE", "TO",
  "AS", "INDEX", "DISTINCT", "HAVING", "WHERE", "JOIN", "ON", "INNER",
  "OUTTER", "LEFT", "RIGHT", "ALL", "CROSS", "ORDER", "BY", "ASC", "DESC",
  "GROUP", "FOR", "UPDATE", "LIMIT", "OFFSET", "INSERT", "OVERWRITE",
  "DELETE", "VALUES", "SET", "IN", "ID", "NULL_VAL", "INTEGRAL_VAL",
  "STRING_VAL", "APPROX_VAL", "DATE_VAL", "DATETIME_VAL", "EQ", "NOT",
  "OP_AND", "BIGINT", "INT", "SMALLINT", "TINYINT", "DECIMAL", "NUMERIC",
  "CHAR", "VARCHAR", "DATE", "DATETIME", "TIMESTMAP", "AUTO_INCREMENT",
  "COMMENT", "TOKEN_ERROR", "ASSIGN", "OP_OR", "XOR", "IS", "LIKE",
  "REGEXP", "'!'", "BETWEEN", "COMPARISON", "'|'", "'&'", "LSHIFT",
  "RSHIFT", "'+'", "'-'", "'*'", "'/'", "'%'", "MOD", "'^'", "UMINUS",
  "';'", "'('", "')'", "','", "'.'", "'?'", "'~'", "$accept", "Block",
  "Statement", "Command", "DDL", "CreateTableStmt", "ColumnDefinitionList",
  "ColumnDefinition", "TypeDefinition", "FixedSizeDescription",
  "FloatingSizeDescription", "AutoIncrementOption", "NullOption",
  "KeyOption", "CommentOption", "AlterTableStmt", "AlterTableSpecList",
  "AlterTableSpec", "AlterColPosOption", "NameList", "DML", "SelectStmt",
  "DistinctOption", "ProjectionColumnList", "ProjectionColumn",
  "AliasOption", "Alias", "FromClause", "Relation", "OnClause", "JoinOp",
  "WhereClause", "HavingClause", "OrderByClause", "GroupByClause",
  "LimitOffsetClause", "ForUpdateOption", "ExpressionList", "Expression",
  "BoolPrimary", "Predicate", "BitExpression", "Simple", "Subquery",
  "Identifier", YY_NULLPTR
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
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,    33,   341,   342,   124,
      38,   343,   344,    43,    45,    42,    47,    37,   345,    94,
     346,    59,    40,    41,    44,    46,    63,   126
};
# endif

#define YYPACT_NINF -182

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-182)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     428,     0,    40,    67,     5,    73,    71,  -182,  -182,   406,
    -182,    10,  -182,  -182,  -182,  -182,  -182,  -182,  -182,   252,
      44,    44,  -182,    44,  -182,  -182,  -182,  -182,  -182,  -182,
    -182,  -182,   277,   277,   284,  -182,   277,  -182,   284,    -2,
    -182,   274,   -31,  -182,   128,  -182,     8,    21,  -182,   159,
      63,    16,    63,  -182,   186,  -182,   -49,   252,   121,    44,
     277,   277,   277,    36,  -182,  -182,  -182,   -32,   284,   284,
      55,    82,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   -27,    44,   174,     7,   131,
      27,    57,  -182,    44,  -182,   161,    42,     3,  -182,   277,
     143,  -182,    63,   178,   -38,    84,    84,  -182,   139,  -182,
     228,    18,  -182,    86,   284,   284,   228,   142,    83,   202,
      33,    33,    19,    19,    -5,    -5,    -5,    -5,   192,  -182,
    -182,   110,  -182,   387,   183,    44,    44,    44,  -182,    44,
     -30,    44,    44,    12,    44,    44,    44,    44,    44,  -182,
      44,    44,   159,   105,    29,    44,  -182,   287,   206,   199,
     161,  -182,  -182,  -182,   147,   125,   287,    18,  -182,   228,
     188,   284,  -182,    44,   162,   162,   162,   162,   190,   190,
     162,   162,  -182,  -182,    -3,  -182,  -182,  -182,  -182,   255,
      44,    12,   255,   151,    44,    44,  -182,   234,   249,  -182,
    -182,   261,    44,    12,  -182,    42,  -182,   132,   216,   276,
     -49,   -49,    42,   277,   248,   297,  -182,  -182,   277,   200,
     284,  -182,  -182,   273,  -182,  -182,  -182,  -182,   295,  -182,
    -182,  -182,  -182,  -182,   258,   268,   315,   347,  -182,   263,
     210,  -182,   264,  -182,  -182,  -182,    44,    44,    44,    12,
    -182,  -182,  -182,   334,  -182,   337,  -182,    42,    42,  -182,
       1,   277,   271,   326,   287,  -182,  -182,   285,   272,  -182,
    -182,   255,  -182,  -182,    44,  -182,    44,  -182,  -182,  -182,
    -182,  -182,  -182,   225,   225,  -182,  -182,   283,   277,   318,
     361,  -182,   350,   332,   245,  -182,   257,   310,  -182,  -182,
     269,   -39,   366,  -182,   314,   359,  -182,  -182,    44,  -182,
     277,  -182,   362,   365,  -182,  -182,  -182,  -182,   340,  -182,
    -182,  -182
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    76,     0,     0,     0,     0,     0,     8,     9,     0,
       2,     0,     5,    11,    13,     6,    72,    74,    75,     0,
       0,     0,    10,     0,     7,     1,     3,     4,   163,   157,
     156,   158,     0,     0,     0,    81,     0,   159,     0,    87,
      77,    83,   125,   131,   140,   153,   154,     0,    12,     0,
     122,   154,   123,   160,     0,   161,     0,     0,   102,     0,
       0,     0,     0,     0,    79,    82,    85,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    45,    46,     0,   124,     0,    86,    83,    78,     0,
     108,    84,   121,   119,   120,     0,     0,   126,     0,   128,
     149,     0,   133,     0,     0,     0,   139,     0,   141,   142,
     143,   144,   145,   146,   147,   148,   150,   151,   152,    80,
     155,     0,    15,     0,     0,     0,     0,     0,    63,     0,
       0,     0,     0,    69,     0,     0,     0,     0,     0,    59,
       0,     0,     0,     0,     0,     0,    91,   101,     0,   110,
       0,   130,   129,   127,     0,     0,   117,     0,   132,   138,
       0,     0,    14,     0,    29,    29,    29,    29,    31,    31,
      29,    29,    26,    27,    36,    66,    65,    62,    64,    42,
       0,    69,    42,     0,     0,     0,    48,     0,     0,    60,
      61,     0,     0,    69,    47,     0,    95,     0,     0,     0,
       0,     0,    83,     0,     0,   104,   162,   135,     0,     0,
       0,   137,    16,     0,    18,    19,    20,    21,     0,    22,
      23,    24,    25,    35,     0,    33,    38,    40,    37,     0,
       0,    49,     0,    50,    68,    67,     0,     0,     0,    69,
      54,    88,    97,     0,    99,     0,    96,     0,     0,    92,
     107,     0,     0,   114,   118,   134,   136,     0,     0,    34,
      32,    42,    39,    41,     0,    51,     0,    58,    56,    57,
      55,    98,   100,    94,    94,   105,   106,   109,     0,     0,
     116,    28,     0,    44,     0,    70,     0,     0,    89,    90,
       0,   111,     0,    73,     0,     0,    17,    53,     0,    52,
       0,   103,     0,     0,   115,    30,    43,    71,     0,   113,
     112,    93
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -182,  -182,   410,  -182,  -182,  -182,  -126,   -80,  -182,   205,
     247,  -182,  -182,  -181,  -182,  -182,  -182,   275,  -168,   148,
    -182,     6,  -182,  -182,   375,   -93,   -95,  -182,    95,   155,
    -182,  -182,  -182,  -182,  -182,  -182,  -182,  -162,   -19,  -182,
     -65,   320,    69,    92,    -1
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     9,    10,    11,    12,    13,   131,   132,   184,   224,
     229,   271,   235,   239,   306,    14,    91,    92,   196,   294,
      15,   164,    19,    39,    40,    64,    65,    58,    96,   298,
     211,   100,   263,   159,   215,   290,   303,   165,   166,    42,
      43,    44,    45,   112,    51
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint16 yytable[] =
{
      41,   154,    56,   109,   156,   219,    16,    28,   143,    69,
     312,   242,    22,    50,    52,    16,   193,    54,    46,    47,
      48,     1,    49,   241,   139,   107,    28,    60,   140,    28,
      17,    59,   108,    69,   194,   250,   141,   195,    41,    18,
      66,   102,   103,   104,   285,   286,    20,    69,   150,   105,
      63,   260,    67,    95,   233,    97,    46,    68,   101,    28,
     191,   234,   206,    28,   240,   313,   207,   208,   129,   209,
      59,   203,   190,    21,    28,   106,    29,    30,    31,    23,
     157,   280,    32,    28,   130,   133,   138,   133,   149,   151,
     293,    24,   130,   222,    84,    66,    66,    69,    28,   287,
      28,   153,    57,    53,    33,   218,   221,    55,   155,   142,
     251,    27,    34,    85,    80,    81,    82,    83,    84,   259,
      36,    93,   249,    86,    37,    38,    78,    79,    80,    81,
      82,    83,    84,   210,   186,   187,   188,   113,   189,   133,
     192,   133,    69,   197,   198,   199,   200,   201,   144,   202,
     133,    63,   145,    99,   212,   266,    69,   111,   146,   147,
     148,   152,   283,   284,     1,   252,   114,    87,   253,   115,
      88,    89,   133,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    70,   158,    90,   160,    28,   167,   133,
     134,   135,    71,   244,   245,   136,   163,   161,   162,   264,
     185,   133,    69,   137,    66,   168,    69,   171,   205,    97,
      97,    66,    72,   172,   173,    73,    69,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,   217,   218,
      28,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    69,    60,   214,   277,   278,   279,   213,   254,
     216,    60,   255,   220,   243,   173,    66,    66,   206,   297,
      62,   246,   207,   208,   223,   209,    63,    61,    62,   300,
     236,   237,   238,   295,    63,   295,   247,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,   248,    94,
     261,   318,   228,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    59,   265,   218,   257,   258,   317,    28,   256,
      29,    30,    31,   275,   173,   269,    32,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,   262,   210,
      28,   267,   272,    28,    60,    29,    30,    31,    33,    60,
      28,    32,    29,    30,    31,   270,    34,    35,   307,   308,
      61,    62,    60,   268,    36,    61,    62,    63,    37,    38,
     309,   308,    63,    33,   273,   274,   276,   281,    61,    62,
     282,    34,   311,   288,   289,    63,   301,   292,    34,    36,
     225,   226,   227,    37,    38,   231,   232,   218,   291,   110,
      37,    38,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,    60,    25,   302,   304,     1,
     305,     2,   310,   314,     3,     4,     5,   315,   316,    26,
     319,    61,    62,   320,   296,     6,   230,   204,    63,     7,
       8,     1,    98,     2,   169,   170,     3,     4,     5,   299,
       0,     0,     0,   321,     0,     0,     0,     6,     0,     0,
       0,     7,     8,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183
};

static const yytype_int16 yycheck[] =
{
      19,    96,     4,    68,    97,   167,     0,    56,    88,    14,
      49,   192,     7,    32,    33,     9,   142,    36,    19,    20,
      21,     3,    23,   191,    17,    57,    56,    65,    21,    56,
      30,    28,    64,    14,    22,   203,    29,    25,    57,    39,
      41,    60,    61,    62,    43,    44,     6,    14,    21,    13,
      88,   213,    83,   102,    57,    56,    57,    88,    59,    56,
     140,    64,    33,    56,   190,   104,    37,    38,    95,    40,
      28,   151,   102,     6,    56,    39,    58,    59,    60,     6,
      99,   249,    64,    56,    85,    86,    87,    88,    89,    90,
     271,    20,    93,   173,    99,    96,    97,    14,    56,   261,
      56,    95,   104,    34,    86,   104,   171,    38,   105,   102,
     205,   101,    94,   105,    95,    96,    97,    98,    99,   212,
     102,   105,   202,   102,   106,   107,    93,    94,    95,    96,
      97,    98,    99,   104,   135,   136,   137,    55,   139,   140,
     141,   142,    14,   144,   145,   146,   147,   148,    17,   150,
     151,    88,    21,    32,   155,   220,    14,   102,    27,    28,
      29,   104,   257,   258,     3,    33,    84,     8,    36,    87,
      11,    12,   173,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,    55,    41,    26,   102,    56,   102,   190,
      16,    17,    64,   194,   195,    21,    57,   105,   106,   218,
      17,   202,    14,    29,   205,   113,    14,    65,   103,   210,
     211,   212,    84,   103,   104,    87,    14,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   103,   104,
      56,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,    14,    65,    45,   246,   247,   248,    42,    33,
     103,    65,    36,    65,   103,   104,   257,   258,    33,    34,
      82,    27,    37,    38,   102,    40,    88,    81,    82,   288,
      15,    16,    17,   274,    88,   276,    27,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,    27,   103,
      42,   310,   102,    91,    92,    93,    94,    95,    96,    97,
      98,    99,    28,   103,   104,   210,   211,   308,    56,    33,
      58,    59,    60,   103,   104,    57,    64,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,    31,   104,
      56,    58,    17,    56,    65,    58,    59,    60,    86,    65,
      56,    64,    58,    59,    60,    77,    94,    95,   103,   104,
      81,    82,    65,    58,   102,    81,    82,    88,   106,   107,
     103,   104,    88,    86,    17,   102,   102,    33,    81,    82,
      33,    94,   103,   102,    48,    88,    58,   105,    94,   102,
     175,   176,   177,   106,   107,   180,   181,   104,   103,    69,
     106,   107,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    65,     0,    46,    58,     3,
      78,     5,   102,    47,     8,     9,    10,   103,    59,     9,
      58,    81,    82,    58,   276,    19,   179,   152,    88,    23,
      24,     3,    57,     5,   114,   115,     8,     9,    10,   284,
      -1,    -1,    -1,   103,    -1,    -1,    -1,    19,    -1,    -1,
      -1,    23,    24,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     5,     8,     9,    10,    19,    23,    24,   109,
     110,   111,   112,   113,   123,   128,   129,    30,    39,   130,
       6,     6,     7,     6,    20,     0,   110,   101,    56,    58,
      59,    60,    64,    86,    94,    95,   102,   106,   107,   131,
     132,   146,   147,   148,   149,   150,   152,   152,   152,   152,
     146,   152,   146,   150,   146,   150,     4,   104,   135,    28,
      65,    81,    82,    88,   133,   134,   152,    83,    88,    14,
      55,    64,    84,    87,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   105,   102,     8,    11,    12,
      26,   124,   125,   105,   103,   102,   136,   152,   132,    32,
     139,   152,   146,   146,   146,    13,    39,    57,    64,   148,
     149,   102,   151,    55,    84,    87,   149,   149,   149,   149,
     149,   149,   149,   149,   149,   149,   149,   149,   149,    95,
     152,   114,   115,   152,    16,    17,    21,    29,   152,    17,
      21,    29,   102,   115,    17,    21,    27,    28,    29,   152,
      21,   152,   104,   129,   134,   105,   133,   146,    41,   141,
     102,   151,   151,    57,   129,   145,   146,   102,   151,   149,
     149,    65,   103,   104,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,   116,    17,   152,   152,   152,   152,
     102,   115,   152,   114,    22,    25,   126,   152,   152,   152,
     152,   152,   152,   115,   125,   103,    33,    37,    38,    40,
     104,   138,   152,    42,    45,   142,   103,   103,   104,   145,
      65,   148,   115,   102,   117,   117,   117,   117,   102,   118,
     118,   117,   117,    57,    64,   120,    15,    16,    17,   121,
     114,   126,   121,   103,   152,   152,    27,    27,    27,   115,
     126,   134,    33,    36,    33,    36,    33,   136,   136,   133,
     145,    42,    31,   140,   146,   103,   148,    58,    58,    57,
      77,   119,    17,    17,   102,   103,   102,   152,   152,   152,
     126,    33,    33,   134,   134,    43,    44,   145,   102,    48,
     143,   103,   105,   121,   127,   152,   127,    34,   137,   137,
     146,    58,    46,   144,    58,    78,   122,   103,   104,   103,
     102,   103,    49,   104,    47,   103,    59,   152,   146,    58,
      58,   103
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   108,   109,   109,   110,   111,   111,   111,   111,   111,
     111,   112,   112,   112,   113,   114,   114,   115,   116,   116,
     116,   116,   116,   116,   116,   116,   116,   116,   117,   117,
     118,   118,   119,   119,   120,   120,   120,   121,   121,   121,
     121,   121,   121,   122,   122,   123,   124,   124,   125,   125,
     125,   125,   125,   125,   125,   125,   125,   125,   125,   125,
     125,   125,   125,   125,   125,   125,   125,   126,   126,   126,
     127,   127,   128,   129,   130,   130,   130,   131,   131,   132,
     132,   132,   133,   133,   134,   134,   135,   135,   136,   136,
     136,   136,   136,   137,   137,   138,   138,   138,   138,   138,
     138,   139,   139,   140,   140,   141,   141,   141,   141,   142,
     142,   143,   143,   143,   143,   144,   144,   145,   145,   146,
     146,   146,   146,   146,   146,   146,   147,   147,   147,   147,
     147,   147,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   149,   149,   149,   149,   149,   149,   149,   149,   149,
     149,   149,   149,   149,   150,   150,   150,   150,   150,   150,
     150,   150,   151,   152
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
       1,     3,     1,    10,     1,     1,     0,     1,     3,     2,
       3,     1,     1,     0,     2,     1,     2,     0,     4,     6,
       6,     2,     4,     4,     0,     1,     2,     2,     3,     2,
       3,     2,     0,     4,     0,     4,     4,     3,     0,     3,
       0,     2,     4,     4,     0,     2,     0,     1,     3,     3,
       3,     3,     2,     2,     3,     1,     3,     4,     3,     4,
       4,     1,     4,     3,     6,     5,     6,     5,     4,     3,
       1,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     1,     1,     3,     1,     1,     1,     1,
       2,     2,     3,     1
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
#line 134 "sql.y" /* yacc.c:1660  */
    {
    (yyval.block) = ctx->factory->NewBlock();
    (yyval.block)->AddStmt((yyvsp[0].stmt));
    ctx->block = (yyval.block);
}
#line 1747 "sql.cc" /* yacc.c:1660  */
    break;

  case 3:
#line 139 "sql.y" /* yacc.c:1660  */
    {
    (yyval.block)->AddStmt((yyvsp[0].stmt));
    ctx->block = (yyval.block);
}
#line 1756 "sql.cc" /* yacc.c:1660  */
    break;

  case 7:
#line 148 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(TCLStatement::TXN_BEGIN);
}
#line 1764 "sql.cc" /* yacc.c:1660  */
    break;

  case 8:
#line 151 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(TCLStatement::TXN_COMMIT);
}
#line 1772 "sql.cc" /* yacc.c:1660  */
    break;

  case 9:
#line 154 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(TCLStatement::TXN_ROLLBACK);
}
#line 1780 "sql.cc" /* yacc.c:1660  */
    break;

  case 10:
#line 157 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewShowTables();
}
#line 1788 "sql.cc" /* yacc.c:1660  */
    break;

  case 11:
#line 162 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1796 "sql.cc" /* yacc.c:1660  */
    break;

  case 12:
#line 165 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewDropTable((yyvsp[0].name));
}
#line 1804 "sql.cc" /* yacc.c:1660  */
    break;

  case 13:
#line 168 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1812 "sql.cc" /* yacc.c:1660  */
    break;

  case 14:
#line 172 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewCreateTable((yyvsp[-3].name), (yyvsp[-1].col_def_list));
}
#line 1820 "sql.cc" /* yacc.c:1660  */
    break;

  case 15:
#line 176 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_def_list) = ctx->factory->NewColumnDefinitionList((yyvsp[0].col_def));
}
#line 1828 "sql.cc" /* yacc.c:1660  */
    break;

  case 16:
#line 179 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_def_list)->push_back((yyvsp[0].col_def));
}
#line 1836 "sql.cc" /* yacc.c:1660  */
    break;

  case 17:
#line 183 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_def) = ctx->factory->NewColumnDefinition((yyvsp[-5].name), (yyvsp[-4].type_def), (yyvsp[-3].bool_val), (yyvsp[-2].bool_val), (yyvsp[-1].key_type));
    (yyval.col_def)->set_comment((yyvsp[0].name));
}
#line 1845 "sql.cc" /* yacc.c:1660  */
    break;

  case 18:
#line 188 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_BIGINT, (yyvsp[0].size).fixed_size);
}
#line 1853 "sql.cc" /* yacc.c:1660  */
    break;

  case 19:
#line 191 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_INT, (yyvsp[0].size).fixed_size);
}
#line 1861 "sql.cc" /* yacc.c:1660  */
    break;

  case 20:
#line 194 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_SMALLINT, (yyvsp[0].size).fixed_size);
}
#line 1869 "sql.cc" /* yacc.c:1660  */
    break;

  case 21:
#line 197 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_TINYINT, (yyvsp[0].size).fixed_size);
}
#line 1877 "sql.cc" /* yacc.c:1660  */
    break;

  case 22:
#line 200 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DECIMAL, (yyvsp[0].size).fixed_size, (yyvsp[0].size).float_size);
}
#line 1885 "sql.cc" /* yacc.c:1660  */
    break;

  case 23:
#line 203 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_NUMERIC, (yyvsp[0].size).fixed_size, (yyvsp[0].size).float_size);
}
#line 1893 "sql.cc" /* yacc.c:1660  */
    break;

  case 24:
#line 206 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_NUMERIC, (yyvsp[0].size).fixed_size);
}
#line 1901 "sql.cc" /* yacc.c:1660  */
    break;

  case 25:
#line 209 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_VARCHAR, (yyvsp[0].size).fixed_size);
}
#line 1909 "sql.cc" /* yacc.c:1660  */
    break;

  case 26:
#line 212 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DATE);
}
#line 1917 "sql.cc" /* yacc.c:1660  */
    break;

  case 27:
#line 215 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DATETIME);
}
#line 1925 "sql.cc" /* yacc.c:1660  */
    break;

  case 28:
#line 219 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = (yyvsp[-1].int_val);
    (yyval.size).float_size = 0;
}
#line 1934 "sql.cc" /* yacc.c:1660  */
    break;

  case 29:
#line 223 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = 0;
    (yyval.size).float_size = 0;
}
#line 1943 "sql.cc" /* yacc.c:1660  */
    break;

  case 30:
#line 228 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = (yyvsp[-3].int_val);
    (yyval.size).float_size = (yyvsp[-1].int_val);
}
#line 1952 "sql.cc" /* yacc.c:1660  */
    break;

  case 31:
#line 232 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = 0;
    (yyval.size).float_size = 0;
}
#line 1961 "sql.cc" /* yacc.c:1660  */
    break;

  case 32:
#line 237 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 1969 "sql.cc" /* yacc.c:1660  */
    break;

  case 33:
#line 240 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 1977 "sql.cc" /* yacc.c:1660  */
    break;

  case 34:
#line 244 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 1985 "sql.cc" /* yacc.c:1660  */
    break;

  case 35:
#line 247 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 1993 "sql.cc" /* yacc.c:1660  */
    break;

  case 36:
#line 250 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2001 "sql.cc" /* yacc.c:1660  */
    break;

  case 37:
#line 254 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_KEY;
}
#line 2009 "sql.cc" /* yacc.c:1660  */
    break;

  case 38:
#line 257 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_UNIQUE_KEY;
}
#line 2017 "sql.cc" /* yacc.c:1660  */
    break;

  case 39:
#line 260 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_UNIQUE_KEY;
}
#line 2025 "sql.cc" /* yacc.c:1660  */
    break;

  case 40:
#line 263 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_PRIMARY_KEY;
}
#line 2033 "sql.cc" /* yacc.c:1660  */
    break;

  case 41:
#line 266 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_PRIMARY_KEY;
}
#line 2041 "sql.cc" /* yacc.c:1660  */
    break;

  case 42:
#line 269 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_NOT_KEY;
}
#line 2049 "sql.cc" /* yacc.c:1660  */
    break;

  case 43:
#line 273 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = ctx->factory->NewString((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 2057 "sql.cc" /* yacc.c:1660  */
    break;

  case 44:
#line 276 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = AstString::kEmpty;
}
#line 2065 "sql.cc" /* yacc.c:1660  */
    break;

  case 45:
#line 280 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewAlterTable((yyvsp[-1].name), (yyvsp[0].alter_table_spce_list));
}
#line 2073 "sql.cc" /* yacc.c:1660  */
    break;

  case 46:
#line 284 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce_list) = ctx->factory->NewAlterTableSpecList((yyvsp[0].alter_table_spce));
}
#line 2081 "sql.cc" /* yacc.c:1660  */
    break;

  case 47:
#line 287 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce_list)->push_back((yyvsp[0].alter_table_spce));
}
#line 2089 "sql.cc" /* yacc.c:1660  */
    break;

  case 48:
#line 291 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2097 "sql.cc" /* yacc.c:1660  */
    break;

  case 49:
#line 294 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2105 "sql.cc" /* yacc.c:1660  */
    break;

  case 50:
#line 297 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def_list));
}
#line 2113 "sql.cc" /* yacc.c:1660  */
    break;

  case 51:
#line 300 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def_list));
}
#line 2121 "sql.cc" /* yacc.c:1660  */
    break;

  case 52:
#line 303 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddIndex((yyvsp[-4].name), (yyvsp[-3].key_type) == SQL_NOT_KEY ? SQL_KEY : (yyvsp[-3].key_type),
                                             (yyvsp[-1].name_list));
}
#line 2130 "sql.cc" /* yacc.c:1660  */
    break;

  case 53:
#line 307 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddIndex((yyvsp[-4].name), (yyvsp[-3].key_type) == SQL_NOT_KEY ? SQL_KEY : (yyvsp[-3].key_type),
                                             (yyvsp[-1].name_list));
}
#line 2139 "sql.cc" /* yacc.c:1660  */
    break;

  case 54:
#line 311 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableChangeColumn((yyvsp[-2].name), (yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2147 "sql.cc" /* yacc.c:1660  */
    break;

  case 55:
#line 314 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableChangeColumn((yyvsp[-2].name), (yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2155 "sql.cc" /* yacc.c:1660  */
    break;

  case 56:
#line 317 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameColumn((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2163 "sql.cc" /* yacc.c:1660  */
    break;

  case 57:
#line 320 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameIndex((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2171 "sql.cc" /* yacc.c:1660  */
    break;

  case 58:
#line 323 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameIndex((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2179 "sql.cc" /* yacc.c:1660  */
    break;

  case 59:
#line 326 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2187 "sql.cc" /* yacc.c:1660  */
    break;

  case 60:
#line 329 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2195 "sql.cc" /* yacc.c:1660  */
    break;

  case 61:
#line 332 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2203 "sql.cc" /* yacc.c:1660  */
    break;

  case 62:
#line 335 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropColumn((yyvsp[0].name));
}
#line 2211 "sql.cc" /* yacc.c:1660  */
    break;

  case 63:
#line 338 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropColumn((yyvsp[0].name));
}
#line 2219 "sql.cc" /* yacc.c:1660  */
    break;

  case 64:
#line 341 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex((yyvsp[0].name), false);
}
#line 2227 "sql.cc" /* yacc.c:1660  */
    break;

  case 65:
#line 344 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex((yyvsp[0].name), false);
}
#line 2235 "sql.cc" /* yacc.c:1660  */
    break;

  case 66:
#line 347 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex(AstString::kEmpty, true);
}
#line 2243 "sql.cc" /* yacc.c:1660  */
    break;

  case 67:
#line 351 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = (yyvsp[0].name);
    (yyval.col_pos).after = false;
}
#line 2252 "sql.cc" /* yacc.c:1660  */
    break;

  case 68:
#line 355 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = (yyvsp[0].name);
    (yyval.col_pos).after = true;
}
#line 2261 "sql.cc" /* yacc.c:1660  */
    break;

  case 69:
#line 359 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = AstString::kEmpty;
    (yyval.col_pos).after = false;
}
#line 2270 "sql.cc" /* yacc.c:1660  */
    break;

  case 70:
#line 364 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name_list) = ctx->factory->NewNameList((yyvsp[0].name));
}
#line 2278 "sql.cc" /* yacc.c:1660  */
    break;

  case 71:
#line 367 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name_list)->push_back((yyvsp[0].name));
}
#line 2286 "sql.cc" /* yacc.c:1660  */
    break;

  case 73:
#line 375 "sql.y" /* yacc.c:1660  */
    {
    Select *stmt = ctx->factory->NewSelect((yyvsp[-8].bool_val), (yyvsp[-7].proj_col_list), AstString::kEmpty);
    stmt->set_from_clause((yyvsp[-6].query));
    stmt->set_where_clause((yyvsp[-5].expr));
    stmt->set_order_by_desc((yyvsp[-4].order_by).desc);
    stmt->set_order_by_clause((yyvsp[-4].order_by).expr_list);
    stmt->set_group_by_clause((yyvsp[-3].expr_list));
    stmt->set_having_clause((yyvsp[-2].expr));
    stmt->set_limit_val((yyvsp[-1].limit).limit_val);
    stmt->set_offset_val((yyvsp[-1].limit).offset_val);
    stmt->set_for_update((yyvsp[0].bool_val));
    (yyval.stmt) = stmt;
}
#line 2304 "sql.cc" /* yacc.c:1660  */
    break;

  case 74:
#line 389 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2312 "sql.cc" /* yacc.c:1660  */
    break;

  case 75:
#line 392 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2320 "sql.cc" /* yacc.c:1660  */
    break;

  case 76:
#line 395 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2328 "sql.cc" /* yacc.c:1660  */
    break;

  case 77:
#line 399 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col_list) = ctx->factory->NewProjectionColumnList((yyvsp[0].proj_col));
}
#line 2336 "sql.cc" /* yacc.c:1660  */
    break;

  case 78:
#line 402 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col_list)->push_back((yyvsp[0].proj_col));
}
#line 2344 "sql.cc" /* yacc.c:1660  */
    break;

  case 79:
#line 406 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col) = ctx->factory->NewProjectionColumn((yyvsp[-1].expr), (yyvsp[0].name),
                                           Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2353 "sql.cc" /* yacc.c:1660  */
    break;

  case 80:
#line 410 "sql.y" /* yacc.c:1660  */
    {
    Identifier *id = ctx->factory->NewIdentifierWithPlaceholder((yyvsp[-2].name),
        ctx->factory->NewStarPlaceholder((yylsp[0])),
        Location::Concat((yylsp[-2]), (yylsp[0])));
    (yyval.proj_col) = ctx->factory->NewProjectionColumn(id, AstString::kEmpty,
        Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2365 "sql.cc" /* yacc.c:1660  */
    break;

  case 81:
#line 417 "sql.y" /* yacc.c:1660  */
    {
    Placeholder *ph = ctx->factory->NewStarPlaceholder((yylsp[0]));
    (yyval.proj_col) = ctx->factory->NewProjectionColumn(ph, AstString::kEmpty, (yylsp[0]));
}
#line 2374 "sql.cc" /* yacc.c:1660  */
    break;

  case 83:
#line 424 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = AstString::kEmpty;
}
#line 2382 "sql.cc" /* yacc.c:1660  */
    break;

  case 84:
#line 428 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = (yyvsp[0].name);
}
#line 2390 "sql.cc" /* yacc.c:1660  */
    break;

  case 85:
#line 431 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = (yyvsp[0].name);
}
#line 2398 "sql.cc" /* yacc.c:1660  */
    break;

  case 86:
#line 435 "sql.y" /* yacc.c:1660  */
    {
    (yyval.query) = (yyvsp[0].query);
}
#line 2406 "sql.cc" /* yacc.c:1660  */
    break;

  case 87:
#line 438 "sql.y" /* yacc.c:1660  */
    {
    (yyval.query) = nullptr;
}
#line 2414 "sql.cc" /* yacc.c:1660  */
    break;

  case 88:
#line 442 "sql.y" /* yacc.c:1660  */
    {
    Query *query = ::mai::down_cast<Query>((yyvsp[-2].stmt));
    query->set_alias((yyvsp[0].name));
    (yyval.query) = query;
}
#line 2424 "sql.cc" /* yacc.c:1660  */
    break;

  case 89:
#line 447 "sql.y" /* yacc.c:1660  */
    {
    (yyvsp[-5].query)->set_alias((yyvsp[-4].name));
    (yyvsp[-2].query)->set_alias((yyvsp[-1].name));
    (yyval.query) = ctx->factory->NewJoinRelation((yyvsp[-5].query), SQL_CROSS_JOIN, (yyvsp[-2].query), (yyvsp[0].expr),
        AstString::kEmpty);
}
#line 2435 "sql.cc" /* yacc.c:1660  */
    break;

  case 90:
#line 453 "sql.y" /* yacc.c:1660  */
    {
    (yyvsp[-5].query)->set_alias((yyvsp[-4].name));
    (yyvsp[-2].query)->set_alias((yyvsp[-1].name));
    (yyval.query) = ctx->factory->NewJoinRelation((yyvsp[-5].query), (yyvsp[-3].join_kind), (yyvsp[-2].query), (yyvsp[0].expr), AstString::kEmpty);
}
#line 2445 "sql.cc" /* yacc.c:1660  */
    break;

  case 91:
#line 458 "sql.y" /* yacc.c:1660  */
    {
    Identifier *id = ctx->factory->NewIdentifier(AstString::kEmpty, (yyvsp[-1].name), (yylsp[-1]));
    (yyval.query) = ctx->factory->NewNameRelation(id, (yyvsp[0].name));
}
#line 2454 "sql.cc" /* yacc.c:1660  */
    break;

  case 92:
#line 462 "sql.y" /* yacc.c:1660  */
    {
    Identifier *id = ctx->factory->NewIdentifier((yyvsp[-3].name), (yyvsp[-1].name), (yylsp[-3]));
    (yyval.query) = ctx->factory->NewNameRelation(id, (yyvsp[0].name));
}
#line 2463 "sql.cc" /* yacc.c:1660  */
    break;

  case 93:
#line 467 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2471 "sql.cc" /* yacc.c:1660  */
    break;

  case 94:
#line 470 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = nullptr;
}
#line 2479 "sql.cc" /* yacc.c:1660  */
    break;

  case 95:
#line 474 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_CROSS_JOIN;
}
#line 2487 "sql.cc" /* yacc.c:1660  */
    break;

  case 96:
#line 477 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_CROSS_JOIN;
}
#line 2495 "sql.cc" /* yacc.c:1660  */
    break;

  case 97:
#line 480 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_LEFT_OUTTER_JOIN;
}
#line 2503 "sql.cc" /* yacc.c:1660  */
    break;

  case 98:
#line 483 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_LEFT_OUTTER_JOIN;
}
#line 2511 "sql.cc" /* yacc.c:1660  */
    break;

  case 99:
#line 486 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_RIGHT_OUTTER_JOIN;
}
#line 2519 "sql.cc" /* yacc.c:1660  */
    break;

  case 100:
#line 489 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_RIGHT_OUTTER_JOIN;
}
#line 2527 "sql.cc" /* yacc.c:1660  */
    break;

  case 101:
#line 493 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2535 "sql.cc" /* yacc.c:1660  */
    break;

  case 102:
#line 496 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = nullptr;
}
#line 2543 "sql.cc" /* yacc.c:1660  */
    break;

  case 103:
#line 500 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2551 "sql.cc" /* yacc.c:1660  */
    break;

  case 104:
#line 503 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = nullptr;
}
#line 2559 "sql.cc" /* yacc.c:1660  */
    break;

  case 105:
#line 507 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = (yyvsp[-1].expr_list);
    (yyval.order_by).desc = false;
}
#line 2568 "sql.cc" /* yacc.c:1660  */
    break;

  case 106:
#line 511 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = (yyvsp[-1].expr_list);
    (yyval.order_by).desc = true;
}
#line 2577 "sql.cc" /* yacc.c:1660  */
    break;

  case 107:
#line 515 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = (yyvsp[0].expr_list);
    (yyval.order_by).desc = false;
}
#line 2586 "sql.cc" /* yacc.c:1660  */
    break;

  case 108:
#line 519 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = nullptr;
    (yyval.order_by).desc = false;
}
#line 2595 "sql.cc" /* yacc.c:1660  */
    break;

  case 109:
#line 524 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = (yyvsp[0].expr_list);
}
#line 2603 "sql.cc" /* yacc.c:1660  */
    break;

  case 110:
#line 527 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = nullptr;
}
#line 2611 "sql.cc" /* yacc.c:1660  */
    break;

  case 111:
#line 531 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
    (yyval.limit).offset_val = 0;
}
#line 2620 "sql.cc" /* yacc.c:1660  */
    break;

  case 112:
#line 535 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).offset_val = (yyvsp[-2].int_val);
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
}
#line 2629 "sql.cc" /* yacc.c:1660  */
    break;

  case 113:
#line 539 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = (yyvsp[-2].int_val);
    (yyval.limit).offset_val = (yyvsp[0].int_val);
}
#line 2638 "sql.cc" /* yacc.c:1660  */
    break;

  case 114:
#line 543 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = 0;
    (yyval.limit).offset_val = 0;
}
#line 2647 "sql.cc" /* yacc.c:1660  */
    break;

  case 115:
#line 549 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2655 "sql.cc" /* yacc.c:1660  */
    break;

  case 116:
#line 552 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2663 "sql.cc" /* yacc.c:1660  */
    break;

  case 117:
#line 559 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = ctx->factory->NewExpressionList((yyvsp[0].expr));
}
#line 2671 "sql.cc" /* yacc.c:1660  */
    break;

  case 118:
#line 562 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list)->push_back((yyvsp[0].expr));
}
#line 2679 "sql.cc" /* yacc.c:1660  */
    break;

  case 119:
#line 569 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_OR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2687 "sql.cc" /* yacc.c:1660  */
    break;

  case 120:
#line 572 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_XOR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2695 "sql.cc" /* yacc.c:1660  */
    break;

  case 121:
#line 575 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_AND, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2703 "sql.cc" /* yacc.c:1660  */
    break;

  case 122:
#line 578 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_NOT, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2711 "sql.cc" /* yacc.c:1660  */
    break;

  case 123:
#line 581 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_NOT, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2719 "sql.cc" /* yacc.c:1660  */
    break;

  case 124:
#line 584 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2727 "sql.cc" /* yacc.c:1660  */
    break;

  case 126:
#line 592 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_IS_NULL, (yyvsp[-2].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2735 "sql.cc" /* yacc.c:1660  */
    break;

  case 127:
#line 595 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_IS_NOT_NULL, (yyvsp[-3].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2743 "sql.cc" /* yacc.c:1660  */
    break;

  case 128:
#line 598 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-2].expr), (yyvsp[-1].op), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2751 "sql.cc" /* yacc.c:1660  */
    break;

  case 129:
#line 601 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-3].expr), (yyvsp[-2].op), (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2759 "sql.cc" /* yacc.c:1660  */
    break;

  case 130:
#line 604 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-3].expr), (yyvsp[-2].op), (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2767 "sql.cc" /* yacc.c:1660  */
    break;

  case 132:
#line 612 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-3].expr), SQL_NOT_IN, (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2775 "sql.cc" /* yacc.c:1660  */
    break;

  case 133:
#line 615 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-2].expr), SQL_IN, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2783 "sql.cc" /* yacc.c:1660  */
    break;

  case 134:
#line 618 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-5].expr), SQL_NOT_IN, (yyvsp[-1].expr_list), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 2791 "sql.cc" /* yacc.c:1660  */
    break;

  case 135:
#line 621 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-4].expr), SQL_IN, (yyvsp[-1].expr_list), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 2799 "sql.cc" /* yacc.c:1660  */
    break;

  case 136:
#line 624 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-5].expr), SQL_NOT_BETWEEN, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 2807 "sql.cc" /* yacc.c:1660  */
    break;

  case 137:
#line 627 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-4].expr), SQL_BETWEEN, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 2815 "sql.cc" /* yacc.c:1660  */
    break;

  case 138:
#line 630 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-3].expr), SQL_NOT_LIKE, (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2823 "sql.cc" /* yacc.c:1660  */
    break;

  case 139:
#line 633 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_LIKE, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2831 "sql.cc" /* yacc.c:1660  */
    break;

  case 141:
#line 642 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_OR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2839 "sql.cc" /* yacc.c:1660  */
    break;

  case 142:
#line 645 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_AND, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2847 "sql.cc" /* yacc.c:1660  */
    break;

  case 143:
#line 648 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_LSHIFT, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2855 "sql.cc" /* yacc.c:1660  */
    break;

  case 144:
#line 651 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_RSHIFT, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2863 "sql.cc" /* yacc.c:1660  */
    break;

  case 145:
#line 654 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_PLUS, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2871 "sql.cc" /* yacc.c:1660  */
    break;

  case 146:
#line 657 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_SUB, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2879 "sql.cc" /* yacc.c:1660  */
    break;

  case 147:
#line 660 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MUL, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2887 "sql.cc" /* yacc.c:1660  */
    break;

  case 148:
#line 663 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_DIV, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2895 "sql.cc" /* yacc.c:1660  */
    break;

  case 149:
#line 666 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_DIV, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2903 "sql.cc" /* yacc.c:1660  */
    break;

  case 150:
#line 669 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MOD, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2911 "sql.cc" /* yacc.c:1660  */
    break;

  case 151:
#line 672 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MOD, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2919 "sql.cc" /* yacc.c:1660  */
    break;

  case 152:
#line 675 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_XOR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2927 "sql.cc" /* yacc.c:1660  */
    break;

  case 154:
#line 684 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewIdentifier(AstString::kEmpty, (yyvsp[0].name), (yylsp[0]));
}
#line 2935 "sql.cc" /* yacc.c:1660  */
    break;

  case 155:
#line 687 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewIdentifier((yyvsp[-2].name), (yyvsp[0].name), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2943 "sql.cc" /* yacc.c:1660  */
    break;

  case 156:
#line 690 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewStringLiteral((yyvsp[0].text).buf, (yyvsp[0].text).len, (yylsp[0]));
}
#line 2951 "sql.cc" /* yacc.c:1660  */
    break;

  case 157:
#line 693 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewIntegerLiteral((yyvsp[0].int_val), (yylsp[0]));
}
#line 2959 "sql.cc" /* yacc.c:1660  */
    break;

  case 158:
#line 696 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewApproxLiteral((yyvsp[0].approx_val), (yylsp[0]));
}
#line 2967 "sql.cc" /* yacc.c:1660  */
    break;

  case 159:
#line 699 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewParamPlaceholder((yylsp[0]));
}
#line 2975 "sql.cc" /* yacc.c:1660  */
    break;

  case 160:
#line 702 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_MINUS, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2983 "sql.cc" /* yacc.c:1660  */
    break;

  case 161:
#line 705 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_BIT_INV, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2991 "sql.cc" /* yacc.c:1660  */
    break;

  case 162:
#line 710 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewSubquery(true, ::mai::down_cast<Query>((yyvsp[-1].stmt)), (yylsp[-1]));
}
#line 2999 "sql.cc" /* yacc.c:1660  */
    break;

  case 163:
#line 714 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = ctx->factory->NewString((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 3007 "sql.cc" /* yacc.c:1660  */
    break;


#line 3011 "sql.cc" /* yacc.c:1660  */
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
#line 718 "sql.y" /* yacc.c:1903  */

void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg) {
    ctx->err_line   = yyl->first_line;
    ctx->err_column = yyl->first_column;
    ctx->err_msg    = ctx->factory->NewString(msg);
}
