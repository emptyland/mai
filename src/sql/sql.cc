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

#line 263 "sql.cc" /* yacc.c:353  */
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
#define YYLAST   436

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  103
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  40
/* YYNRULES -- Number of rules.  */
#define YYNRULES  144
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  279

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   341

#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    81,     2,     2,     2,    92,    85,     2,
      97,    98,    90,    88,    99,    89,   100,    91,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    96,
       2,     2,     2,   101,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    94,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    84,     2,   102,     2,     2,     2,
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
      75,    76,    77,    78,    79,    80,    82,    83,    86,    87,
      93,    95
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   132,   132,   137,   142,   144,   145,   146,   149,   152,
     155,   160,   163,   166,   170,   174,   177,   181,   186,   189,
     192,   195,   198,   201,   204,   207,   210,   213,   217,   221,
     226,   230,   235,   238,   242,   245,   248,   252,   255,   258,
     261,   264,   267,   271,   274,   278,   282,   285,   289,   292,
     295,   298,   301,   305,   309,   312,   315,   318,   321,   324,
     327,   330,   333,   336,   339,   342,   345,   349,   353,   357,
     362,   365,   371,   373,   387,   390,   393,   397,   400,   404,
     408,   415,   421,   422,   426,   429,   433,   436,   440,   445,
     451,   456,   460,   465,   468,   472,   475,   478,   481,   484,
     487,   491,   494,   498,   501,   505,   509,   513,   517,   522,
     525,   529,   533,   537,   541,   547,   550,   556,   559,   563,
     566,   569,   572,   575,   578,   581,   584,   587,   590,   593,
     596,   599,   602,   605,   608,   611,   614,   617,   620,   623,
     626,   629,   632,   635,   639
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
  "DISTINCT", "HAVING", "WHERE", "JOIN", "ON", "INNER", "OUTTER", "LEFT",
  "RIGHT", "ALL", "CROSS", "ORDER", "BY", "ASC", "DESC", "GROUP", "FOR",
  "UPDATE", "LIMIT", "OFFSET", "INSERT", "OVERWRITE", "DELETE", "VALUES",
  "SET", "ID", "NULL_VAL", "INTEGRAL_VAL", "STRING_VAL", "APPROX_VAL",
  "EQ", "NOT", "OP_AND", "BIGINT", "INT", "SMALLINT", "TINYINT", "DECIMAL",
  "NUMERIC", "CHAR", "VARCHAR", "DATE", "DATETIME", "TIMESTMAP",
  "AUTO_INCREMENT", "COMMENT", "ASSIGN", "OP_OR", "XOR", "IN", "IS",
  "LIKE", "REGEXP", "'!'", "BETWEEN", "COMPARISON", "'|'", "'&'", "LSHIFT",
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
     335,    33,   336,   337,   124,    38,   338,   339,    43,    45,
      42,    47,    37,   340,    94,   341,    59,    40,    41,    44,
      46,    63,   126
};
# endif

#define YYPACT_NINF -151

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-151)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     354,    56,     4,     6,     9,    16,     8,  -151,  -151,   211,
    -151,   -66,  -151,  -151,  -151,  -151,  -151,  -151,  -151,   150,
      19,    19,  -151,    19,  -151,  -151,  -151,  -151,  -151,  -151,
    -151,  -151,   134,   134,  -151,   134,  -151,   134,     5,  -151,
      87,   -21,   -14,  -151,   133,   306,    22,    36,   170,   245,
     -20,   150,   107,    19,   134,   134,   -46,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   134,   134,  -151,  -151,
    -151,    -3,    19,    97,    55,   106,    54,    62,  -151,    19,
    -151,   161,    25,   -15,  -151,   134,   114,  -151,   294,   264,
    -151,   115,   185,   326,   335,   342,   342,    36,    36,    86,
      86,    86,  -151,  -151,  -151,    44,  -151,   340,   173,    19,
      19,    19,  -151,    19,    18,    19,    19,   100,    19,    19,
      19,    19,    19,  -151,    19,    19,   133,    94,   -11,    19,
    -151,   245,   154,   159,  -151,  -151,    19,   111,   111,   111,
     111,   116,   116,   111,   111,  -151,  -151,    60,  -151,  -151,
    -151,  -151,   153,    19,   100,   153,    84,    19,    19,  -151,
     179,   190,  -151,  -151,   197,    19,   100,  -151,    25,  -151,
     117,   129,   194,   -20,   -20,    25,   134,   187,   200,  -151,
     186,  -151,  -151,  -151,  -151,   188,  -151,  -151,  -151,  -151,
    -151,   180,   172,   227,   235,  -151,   168,    99,  -151,   169,
    -151,  -151,  -151,    19,    19,    19,   100,  -151,  -151,  -151,
     236,  -151,   249,  -151,    25,    25,  -151,   -13,   245,   134,
     199,   237,   184,   198,  -151,  -151,   153,  -151,  -151,    19,
    -151,    19,  -151,  -151,  -151,  -151,  -151,  -151,    30,    30,
    -151,  -151,   134,   201,   134,   248,   241,  -151,   251,   231,
     101,  -151,   119,   220,  -151,  -151,   245,   203,   -40,   274,
    -151,   223,   269,  -151,  -151,    19,  -151,   134,  -151,   271,
     272,  -151,  -151,  -151,  -151,   224,  -151,  -151,  -151
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    76,     0,     0,     0,     0,     0,     8,     9,     0,
       2,     0,     5,    11,    13,     6,    72,    74,    75,     0,
       0,     0,    10,     0,     7,     1,     3,     4,   144,   122,
     121,   123,     0,     0,    81,     0,   124,     0,    87,    77,
      83,   119,     0,    12,     0,   139,   119,   138,     0,   140,
       0,     0,   102,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    79,    82,
      85,     0,     0,     0,     0,     0,     0,    45,    46,     0,
     143,     0,    86,    83,    78,     0,   108,    84,   136,   137,
     142,     0,   125,   133,   132,   134,   135,   126,   127,   128,
     129,   130,   131,    80,   120,     0,    15,     0,     0,     0,
       0,     0,    63,     0,     0,     0,     0,    69,     0,     0,
       0,     0,     0,    59,     0,     0,     0,     0,     0,     0,
      91,   101,     0,   110,   141,    14,     0,    29,    29,    29,
      29,    31,    31,    29,    29,    26,    27,    36,    66,    65,
      62,    64,    42,     0,    69,    42,     0,     0,     0,    48,
       0,     0,    60,    61,     0,     0,    69,    47,     0,    95,
       0,     0,     0,     0,     0,    83,     0,     0,   104,    16,
       0,    18,    19,    20,    21,     0,    22,    23,    24,    25,
      35,     0,    33,    38,    40,    37,     0,     0,    49,     0,
      50,    68,    67,     0,     0,     0,    69,    54,    88,    97,
       0,    99,     0,    96,     0,     0,    92,   107,   117,     0,
       0,   114,     0,     0,    34,    32,    42,    39,    41,     0,
      51,     0,    58,    56,    57,    55,    98,   100,    94,    94,
     105,   106,     0,   109,     0,     0,   116,    28,     0,    44,
       0,    70,     0,     0,    89,    90,   118,     0,   111,     0,
      73,     0,     0,    17,    53,     0,    52,     0,   103,     0,
       0,   115,    30,    43,    71,     0,   113,   112,    93
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -151,  -151,   329,  -151,  -151,  -151,   -35,   -56,  -151,   230,
     202,  -151,  -151,  -150,  -151,  -151,  -151,   214,  -131,   110,
    -151,   262,  -151,  -151,   295,   -77,   -79,  -151,    64,   121,
    -151,  -151,  -151,  -151,  -151,  -151,  -151,   126,   -18,   -19
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     9,    10,    11,    12,    13,   105,   106,   147,   181,
     186,   226,   192,   196,   263,    14,    77,    78,   159,   250,
      15,    16,    19,    38,    39,    68,    69,    52,    82,   254,
     174,    86,   221,   133,   178,   246,   260,   217,    40,    46
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint16 yytable[] =
{
      41,    42,    43,   128,    44,   199,   130,   269,    90,    50,
      20,    53,    21,    91,    45,    47,    22,    48,   117,    49,
     169,    70,    23,   198,   170,   171,    24,   172,   240,   241,
      27,    83,    41,    28,    87,   207,    88,    89,    28,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
      28,    53,   104,   107,   112,   107,   123,   125,   154,   270,
     104,   169,   253,    70,    70,   170,   171,   131,   172,   166,
     113,    28,    28,   124,   114,   235,   249,    81,    28,    71,
     179,   156,   115,    72,    17,   129,   242,   103,   173,   208,
     149,   150,   151,    18,   152,   107,   155,   107,   216,   160,
     161,   162,   163,   164,    51,   165,   107,    28,    28,   206,
     175,   108,   109,    53,   190,   153,   110,   107,   197,   191,
     157,   118,    79,   158,   111,   119,    64,    65,    66,   173,
      67,   120,   121,   122,   107,   238,   239,    85,   201,   202,
      28,    73,   135,   136,    74,    75,   107,    54,   209,    70,
      28,   210,   116,   132,    83,    83,    70,    76,   218,    28,
     211,   126,    55,   212,     1,    56,   193,   194,   195,   134,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    67,   200,   136,   232,   233,   234,    28,   148,    29,
      30,    31,   168,    32,   176,    70,    70,   230,   136,   264,
     265,   218,   177,    28,   203,    29,    30,    31,   180,    32,
     251,    25,   251,   185,     1,   204,     2,   266,   265,     3,
       4,     5,   205,    33,   256,   213,   257,   219,     6,   220,
      54,    35,     7,     8,   224,    36,    37,   214,   215,    33,
      34,   222,   227,   223,   225,    55,   274,    35,    56,   275,
     228,    36,    37,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    54,    67,   229,   231,   236,    80,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    55,    67,
     237,    56,   247,   245,    54,   259,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,   244,    67,   248,    55,
     242,   268,    56,   258,   262,    54,   261,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,   267,    67,   271,
      55,   272,   278,    56,    54,   273,   276,   277,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    26,    67,
     167,   252,    56,   127,   187,   243,    84,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,     1,    67,     2,
     255,     0,     3,     4,     5,     0,     0,     0,   182,   183,
     184,     6,    56,   188,   189,     7,     8,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,     0,    67,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,     0,
      67,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,    59,    60,    61,    62,    63,    64,    65,    66,     0,
      67,    60,    61,    62,    63,    64,    65,    66,     0,    67,
      62,    63,    64,    65,    66,     0,    67
};

static const yytype_int16 yycheck[] =
{
      19,    20,    21,    82,    23,   155,    83,    47,    54,     4,
       6,    26,     6,    59,    32,    33,     7,    35,    74,    37,
      31,    40,     6,   154,    35,    36,    18,    38,    41,    42,
      96,    50,    51,    53,    53,   166,    54,    55,    53,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      53,    26,    71,    72,    73,    74,    75,    76,   114,    99,
      79,    31,    32,    82,    83,    35,    36,    85,    38,   125,
      15,    53,    53,    19,    19,   206,   226,    97,    53,   100,
     136,   116,    27,    97,    28,   100,    99,    90,    99,   168,
     109,   110,   111,    37,   113,   114,   115,   116,   175,   118,
     119,   120,   121,   122,    99,   124,   125,    53,    53,   165,
     129,    14,    15,    26,    54,    97,    19,   136,   153,    59,
      20,    15,   100,    23,    27,    19,    90,    91,    92,    99,
      94,    25,    26,    27,   153,   214,   215,    30,   157,   158,
      53,     8,    98,    99,    11,    12,   165,    60,    31,   168,
      53,    34,    97,    39,   173,   174,   175,    24,   176,    53,
      31,    99,    75,    34,     3,    78,    13,    14,    15,    54,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      94,    94,    98,    99,   203,   204,   205,    53,    15,    55,
      56,    57,    98,    59,    40,   214,   215,    98,    99,    98,
      99,   219,    43,    53,    25,    55,    56,    57,    97,    59,
     229,     0,   231,    97,     3,    25,     5,    98,    99,     8,
       9,    10,    25,    89,   242,    31,   244,    40,    17,    29,
      60,    97,    21,    22,    54,   101,   102,   173,   174,    89,
      90,    55,    15,    55,    72,    75,   265,    97,    78,   267,
      15,   101,   102,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    60,    94,    97,    97,    31,    98,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    75,    94,
      31,    78,    98,    46,    60,    44,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    97,    94,   100,    75,
      99,    98,    78,    55,    73,    60,    55,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    97,    94,    45,
      75,    98,    98,    78,    60,    56,    55,    55,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,     9,    94,
     126,   231,    78,    81,   142,   219,    51,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,     3,    94,     5,
     239,    -1,     8,     9,    10,    -1,    -1,    -1,   138,   139,
     140,    17,    78,   143,   144,    21,    22,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    -1,    94,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    -1,
      94,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    85,    86,    87,    88,    89,    90,    91,    92,    -1,
      94,    86,    87,    88,    89,    90,    91,    92,    -1,    94,
      88,    89,    90,    91,    92,    -1,    94
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     5,     8,     9,    10,    17,    21,    22,   104,
     105,   106,   107,   108,   118,   123,   124,    28,    37,   125,
       6,     6,     7,     6,    18,     0,   105,    96,    53,    55,
      56,    57,    59,    89,    90,    97,   101,   102,   126,   127,
     141,   142,   142,   142,   142,   141,   142,   141,   141,   141,
       4,    99,   130,    26,    60,    75,    78,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    94,   128,   129,
     142,   100,    97,     8,    11,    12,    24,   119,   120,   100,
      98,    97,   131,   142,   127,    30,   134,   142,   141,   141,
      54,    59,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,    90,   142,   109,   110,   142,    14,    15,
      19,    27,   142,    15,    19,    27,    97,   110,    15,    19,
      25,    26,    27,   142,    19,   142,    99,   124,   129,   100,
     128,   141,    39,   136,    54,    98,    99,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,   111,    15,   142,
     142,   142,   142,    97,   110,   142,   109,    20,    23,   121,
     142,   142,   142,   142,   142,   142,   110,   120,    98,    31,
      35,    36,    38,    99,   133,   142,    40,    43,   137,   110,
      97,   112,   112,   112,   112,    97,   113,   113,   112,   112,
      54,    59,   115,    13,    14,    15,   116,   109,   121,   116,
      98,   142,   142,    25,    25,    25,   110,   121,   129,    31,
      34,    31,    34,    31,   131,   131,   128,   140,   141,    40,
      29,   135,    55,    55,    54,    72,   114,    15,    15,    97,
      98,    97,   142,   142,   142,   121,    31,    31,   129,   129,
      41,    42,    99,   140,    97,    46,   138,    98,   100,   116,
     122,   142,   122,    32,   132,   132,   141,   141,    55,    44,
     139,    55,    73,   117,    98,    99,    98,    97,    98,    47,
      99,    45,    98,    56,   142,   141,    55,    55,    98
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   103,   104,   104,   105,   106,   106,   106,   106,   106,
     106,   107,   107,   107,   108,   109,   109,   110,   111,   111,
     111,   111,   111,   111,   111,   111,   111,   111,   112,   112,
     113,   113,   114,   114,   115,   115,   115,   116,   116,   116,
     116,   116,   116,   117,   117,   118,   119,   119,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   121,   121,   121,
     122,   122,   123,   124,   125,   125,   125,   126,   126,   127,
     127,   127,   128,   128,   129,   129,   130,   130,   131,   131,
     131,   131,   131,   132,   132,   133,   133,   133,   133,   133,
     133,   134,   134,   135,   135,   136,   136,   136,   136,   137,
     137,   138,   138,   138,   138,   139,   139,   140,   140,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   142
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
       0,     2,     4,     4,     0,     2,     0,     1,     3,     1,
       3,     1,     1,     1,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       2,     4,     3,     3,     1
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
#line 132 "sql.y" /* yacc.c:1660  */
    {
    (yyval.block) = ctx->factory->NewBlock();
    (yyval.block)->AddStmt((yyvsp[0].stmt));
    ctx->block = (yyval.block);
}
#line 1711 "sql.cc" /* yacc.c:1660  */
    break;

  case 3:
#line 137 "sql.y" /* yacc.c:1660  */
    {
    (yyval.block)->AddStmt((yyvsp[0].stmt));
    ctx->block = (yyval.block);
}
#line 1720 "sql.cc" /* yacc.c:1660  */
    break;

  case 7:
#line 146 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(TCLStatement::TXN_BEGIN);
}
#line 1728 "sql.cc" /* yacc.c:1660  */
    break;

  case 8:
#line 149 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(TCLStatement::TXN_COMMIT);
}
#line 1736 "sql.cc" /* yacc.c:1660  */
    break;

  case 9:
#line 152 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(TCLStatement::TXN_ROLLBACK);
}
#line 1744 "sql.cc" /* yacc.c:1660  */
    break;

  case 10:
#line 155 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewShowTables();
}
#line 1752 "sql.cc" /* yacc.c:1660  */
    break;

  case 11:
#line 160 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1760 "sql.cc" /* yacc.c:1660  */
    break;

  case 12:
#line 163 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewDropTable((yyvsp[0].name));
}
#line 1768 "sql.cc" /* yacc.c:1660  */
    break;

  case 13:
#line 166 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1776 "sql.cc" /* yacc.c:1660  */
    break;

  case 14:
#line 170 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewCreateTable((yyvsp[-3].name), (yyvsp[-1].col_def_list));
}
#line 1784 "sql.cc" /* yacc.c:1660  */
    break;

  case 15:
#line 174 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_def_list) = ctx->factory->NewColumnDefinitionList((yyvsp[0].col_def));
}
#line 1792 "sql.cc" /* yacc.c:1660  */
    break;

  case 16:
#line 177 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_def_list)->push_back((yyvsp[0].col_def));
}
#line 1800 "sql.cc" /* yacc.c:1660  */
    break;

  case 17:
#line 181 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_def) = ctx->factory->NewColumnDefinition((yyvsp[-5].name), (yyvsp[-4].type_def), (yyvsp[-3].bool_val), (yyvsp[-2].bool_val), (yyvsp[-1].key_type));
    (yyval.col_def)->set_comment((yyvsp[0].name));
}
#line 1809 "sql.cc" /* yacc.c:1660  */
    break;

  case 18:
#line 186 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_BIGINT, (yyvsp[0].size).fixed_size);
}
#line 1817 "sql.cc" /* yacc.c:1660  */
    break;

  case 19:
#line 189 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_INT, (yyvsp[0].size).fixed_size);
}
#line 1825 "sql.cc" /* yacc.c:1660  */
    break;

  case 20:
#line 192 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_SMALLINT, (yyvsp[0].size).fixed_size);
}
#line 1833 "sql.cc" /* yacc.c:1660  */
    break;

  case 21:
#line 195 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_TINYINT, (yyvsp[0].size).fixed_size);
}
#line 1841 "sql.cc" /* yacc.c:1660  */
    break;

  case 22:
#line 198 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DECIMAL, (yyvsp[0].size).fixed_size, (yyvsp[0].size).float_size);
}
#line 1849 "sql.cc" /* yacc.c:1660  */
    break;

  case 23:
#line 201 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_NUMERIC, (yyvsp[0].size).fixed_size, (yyvsp[0].size).float_size);
}
#line 1857 "sql.cc" /* yacc.c:1660  */
    break;

  case 24:
#line 204 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_NUMERIC, (yyvsp[0].size).fixed_size);
}
#line 1865 "sql.cc" /* yacc.c:1660  */
    break;

  case 25:
#line 207 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_VARCHAR, (yyvsp[0].size).fixed_size);
}
#line 1873 "sql.cc" /* yacc.c:1660  */
    break;

  case 26:
#line 210 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DATE);
}
#line 1881 "sql.cc" /* yacc.c:1660  */
    break;

  case 27:
#line 213 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DATETIME);
}
#line 1889 "sql.cc" /* yacc.c:1660  */
    break;

  case 28:
#line 217 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = (yyvsp[-1].int_val);
    (yyval.size).float_size = 0;
}
#line 1898 "sql.cc" /* yacc.c:1660  */
    break;

  case 29:
#line 221 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = 0;
    (yyval.size).float_size = 0;
}
#line 1907 "sql.cc" /* yacc.c:1660  */
    break;

  case 30:
#line 226 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = (yyvsp[-3].int_val);
    (yyval.size).float_size = (yyvsp[-1].int_val);
}
#line 1916 "sql.cc" /* yacc.c:1660  */
    break;

  case 31:
#line 230 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = 0;
    (yyval.size).float_size = 0;
}
#line 1925 "sql.cc" /* yacc.c:1660  */
    break;

  case 32:
#line 235 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 1933 "sql.cc" /* yacc.c:1660  */
    break;

  case 33:
#line 238 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 1941 "sql.cc" /* yacc.c:1660  */
    break;

  case 34:
#line 242 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 1949 "sql.cc" /* yacc.c:1660  */
    break;

  case 35:
#line 245 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 1957 "sql.cc" /* yacc.c:1660  */
    break;

  case 36:
#line 248 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 1965 "sql.cc" /* yacc.c:1660  */
    break;

  case 37:
#line 252 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_KEY;
}
#line 1973 "sql.cc" /* yacc.c:1660  */
    break;

  case 38:
#line 255 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_UNIQUE_KEY;
}
#line 1981 "sql.cc" /* yacc.c:1660  */
    break;

  case 39:
#line 258 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_UNIQUE_KEY;
}
#line 1989 "sql.cc" /* yacc.c:1660  */
    break;

  case 40:
#line 261 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_PRIMARY_KEY;
}
#line 1997 "sql.cc" /* yacc.c:1660  */
    break;

  case 41:
#line 264 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_PRIMARY_KEY;
}
#line 2005 "sql.cc" /* yacc.c:1660  */
    break;

  case 42:
#line 267 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_NOT_KEY;
}
#line 2013 "sql.cc" /* yacc.c:1660  */
    break;

  case 43:
#line 271 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = ctx->factory->NewString((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 2021 "sql.cc" /* yacc.c:1660  */
    break;

  case 44:
#line 274 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = AstString::kEmpty;
}
#line 2029 "sql.cc" /* yacc.c:1660  */
    break;

  case 45:
#line 278 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewAlterTable((yyvsp[-1].name), (yyvsp[0].alter_table_spce_list));
}
#line 2037 "sql.cc" /* yacc.c:1660  */
    break;

  case 46:
#line 282 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce_list) = ctx->factory->NewAlterTableSpecList((yyvsp[0].alter_table_spce));
}
#line 2045 "sql.cc" /* yacc.c:1660  */
    break;

  case 47:
#line 285 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce_list)->push_back((yyvsp[0].alter_table_spce));
}
#line 2053 "sql.cc" /* yacc.c:1660  */
    break;

  case 48:
#line 289 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2061 "sql.cc" /* yacc.c:1660  */
    break;

  case 49:
#line 292 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2069 "sql.cc" /* yacc.c:1660  */
    break;

  case 50:
#line 295 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def_list));
}
#line 2077 "sql.cc" /* yacc.c:1660  */
    break;

  case 51:
#line 298 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def_list));
}
#line 2085 "sql.cc" /* yacc.c:1660  */
    break;

  case 52:
#line 301 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddIndex((yyvsp[-4].name), (yyvsp[-3].key_type) == SQL_NOT_KEY ? SQL_KEY : (yyvsp[-3].key_type),
                                             (yyvsp[-1].name_list));
}
#line 2094 "sql.cc" /* yacc.c:1660  */
    break;

  case 53:
#line 305 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddIndex((yyvsp[-4].name), (yyvsp[-3].key_type) == SQL_NOT_KEY ? SQL_KEY : (yyvsp[-3].key_type),
                                             (yyvsp[-1].name_list));
}
#line 2103 "sql.cc" /* yacc.c:1660  */
    break;

  case 54:
#line 309 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableChangeColumn((yyvsp[-2].name), (yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2111 "sql.cc" /* yacc.c:1660  */
    break;

  case 55:
#line 312 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableChangeColumn((yyvsp[-2].name), (yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2119 "sql.cc" /* yacc.c:1660  */
    break;

  case 56:
#line 315 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameColumn((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2127 "sql.cc" /* yacc.c:1660  */
    break;

  case 57:
#line 318 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameIndex((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2135 "sql.cc" /* yacc.c:1660  */
    break;

  case 58:
#line 321 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameIndex((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2143 "sql.cc" /* yacc.c:1660  */
    break;

  case 59:
#line 324 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2151 "sql.cc" /* yacc.c:1660  */
    break;

  case 60:
#line 327 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2159 "sql.cc" /* yacc.c:1660  */
    break;

  case 61:
#line 330 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2167 "sql.cc" /* yacc.c:1660  */
    break;

  case 62:
#line 333 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropColumn((yyvsp[0].name));
}
#line 2175 "sql.cc" /* yacc.c:1660  */
    break;

  case 63:
#line 336 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropColumn((yyvsp[0].name));
}
#line 2183 "sql.cc" /* yacc.c:1660  */
    break;

  case 64:
#line 339 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex((yyvsp[0].name), false);
}
#line 2191 "sql.cc" /* yacc.c:1660  */
    break;

  case 65:
#line 342 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex((yyvsp[0].name), false);
}
#line 2199 "sql.cc" /* yacc.c:1660  */
    break;

  case 66:
#line 345 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex(AstString::kEmpty, true);
}
#line 2207 "sql.cc" /* yacc.c:1660  */
    break;

  case 67:
#line 349 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = (yyvsp[0].name);
    (yyval.col_pos).after = false;
}
#line 2216 "sql.cc" /* yacc.c:1660  */
    break;

  case 68:
#line 353 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = (yyvsp[0].name);
    (yyval.col_pos).after = true;
}
#line 2225 "sql.cc" /* yacc.c:1660  */
    break;

  case 69:
#line 357 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = AstString::kEmpty;
    (yyval.col_pos).after = false;
}
#line 2234 "sql.cc" /* yacc.c:1660  */
    break;

  case 70:
#line 362 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name_list) = ctx->factory->NewNameList((yyvsp[0].name));
}
#line 2242 "sql.cc" /* yacc.c:1660  */
    break;

  case 71:
#line 365 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name_list)->push_back((yyvsp[0].name));
}
#line 2250 "sql.cc" /* yacc.c:1660  */
    break;

  case 73:
#line 373 "sql.y" /* yacc.c:1660  */
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
#line 2268 "sql.cc" /* yacc.c:1660  */
    break;

  case 74:
#line 387 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2276 "sql.cc" /* yacc.c:1660  */
    break;

  case 75:
#line 390 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2284 "sql.cc" /* yacc.c:1660  */
    break;

  case 76:
#line 393 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2292 "sql.cc" /* yacc.c:1660  */
    break;

  case 77:
#line 397 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col_list) = ctx->factory->NewProjectionColumnList((yyvsp[0].proj_col));
}
#line 2300 "sql.cc" /* yacc.c:1660  */
    break;

  case 78:
#line 400 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col_list)->push_back((yyvsp[0].proj_col));
}
#line 2308 "sql.cc" /* yacc.c:1660  */
    break;

  case 79:
#line 404 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col) = ctx->factory->NewProjectionColumn((yyvsp[-1].expr), (yyvsp[0].name),
                                           Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2317 "sql.cc" /* yacc.c:1660  */
    break;

  case 80:
#line 408 "sql.y" /* yacc.c:1660  */
    {
    Identifier *id = ctx->factory->NewIdentifierWithPlaceholder((yyvsp[-2].name),
        ctx->factory->NewStarPlaceholder((yylsp[0])),
        Location::Concat((yylsp[-2]), (yylsp[0])));
    (yyval.proj_col) = ctx->factory->NewProjectionColumn(id, AstString::kEmpty,
        Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2329 "sql.cc" /* yacc.c:1660  */
    break;

  case 81:
#line 415 "sql.y" /* yacc.c:1660  */
    {
    Placeholder *ph = ctx->factory->NewStarPlaceholder((yylsp[0]));
    (yyval.proj_col) = ctx->factory->NewProjectionColumn(ph, AstString::kEmpty, (yylsp[0]));
}
#line 2338 "sql.cc" /* yacc.c:1660  */
    break;

  case 83:
#line 422 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = AstString::kEmpty;
}
#line 2346 "sql.cc" /* yacc.c:1660  */
    break;

  case 84:
#line 426 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = (yyvsp[0].name);
}
#line 2354 "sql.cc" /* yacc.c:1660  */
    break;

  case 85:
#line 429 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = (yyvsp[0].name);
}
#line 2362 "sql.cc" /* yacc.c:1660  */
    break;

  case 86:
#line 433 "sql.y" /* yacc.c:1660  */
    {
    (yyval.query) = (yyvsp[0].query);
}
#line 2370 "sql.cc" /* yacc.c:1660  */
    break;

  case 87:
#line 436 "sql.y" /* yacc.c:1660  */
    {
    (yyval.query) = nullptr;
}
#line 2378 "sql.cc" /* yacc.c:1660  */
    break;

  case 88:
#line 440 "sql.y" /* yacc.c:1660  */
    {
    Query *query = ::mai::down_cast<Query>((yyvsp[-2].stmt));
    query->set_alias((yyvsp[0].name));
    (yyval.query) = query;
}
#line 2388 "sql.cc" /* yacc.c:1660  */
    break;

  case 89:
#line 445 "sql.y" /* yacc.c:1660  */
    {
    (yyvsp[-5].query)->set_alias((yyvsp[-4].name));
    (yyvsp[-2].query)->set_alias((yyvsp[-1].name));
    (yyval.query) = ctx->factory->NewJoinRelation((yyvsp[-5].query), SQL_CROSS_JOIN, (yyvsp[-2].query), (yyvsp[0].expr),
        AstString::kEmpty);
}
#line 2399 "sql.cc" /* yacc.c:1660  */
    break;

  case 90:
#line 451 "sql.y" /* yacc.c:1660  */
    {
    (yyvsp[-5].query)->set_alias((yyvsp[-4].name));
    (yyvsp[-2].query)->set_alias((yyvsp[-1].name));
    (yyval.query) = ctx->factory->NewJoinRelation((yyvsp[-5].query), (yyvsp[-3].join_kind), (yyvsp[-2].query), (yyvsp[0].expr), AstString::kEmpty);
}
#line 2409 "sql.cc" /* yacc.c:1660  */
    break;

  case 91:
#line 456 "sql.y" /* yacc.c:1660  */
    {
    Identifier *id = ctx->factory->NewIdentifier(AstString::kEmpty, (yyvsp[-1].name), (yylsp[-1]));
    (yyval.query) = ctx->factory->NewNameRelation(id, (yyvsp[0].name));
}
#line 2418 "sql.cc" /* yacc.c:1660  */
    break;

  case 92:
#line 460 "sql.y" /* yacc.c:1660  */
    {
    Identifier *id = ctx->factory->NewIdentifier((yyvsp[-3].name), (yyvsp[-1].name), (yylsp[-3]));
    (yyval.query) = ctx->factory->NewNameRelation(id, (yyvsp[0].name));
}
#line 2427 "sql.cc" /* yacc.c:1660  */
    break;

  case 93:
#line 465 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2435 "sql.cc" /* yacc.c:1660  */
    break;

  case 94:
#line 468 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = nullptr;
}
#line 2443 "sql.cc" /* yacc.c:1660  */
    break;

  case 95:
#line 472 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_CROSS_JOIN;
}
#line 2451 "sql.cc" /* yacc.c:1660  */
    break;

  case 96:
#line 475 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_CROSS_JOIN;
}
#line 2459 "sql.cc" /* yacc.c:1660  */
    break;

  case 97:
#line 478 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_LEFT_OUTTER_JOIN;
}
#line 2467 "sql.cc" /* yacc.c:1660  */
    break;

  case 98:
#line 481 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_LEFT_OUTTER_JOIN;
}
#line 2475 "sql.cc" /* yacc.c:1660  */
    break;

  case 99:
#line 484 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_RIGHT_OUTTER_JOIN;
}
#line 2483 "sql.cc" /* yacc.c:1660  */
    break;

  case 100:
#line 487 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_RIGHT_OUTTER_JOIN;
}
#line 2491 "sql.cc" /* yacc.c:1660  */
    break;

  case 101:
#line 491 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2499 "sql.cc" /* yacc.c:1660  */
    break;

  case 102:
#line 494 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = nullptr;
}
#line 2507 "sql.cc" /* yacc.c:1660  */
    break;

  case 103:
#line 498 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2515 "sql.cc" /* yacc.c:1660  */
    break;

  case 104:
#line 501 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = nullptr;
}
#line 2523 "sql.cc" /* yacc.c:1660  */
    break;

  case 105:
#line 505 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = (yyvsp[-1].expr_list);
    (yyval.order_by).desc = false;
}
#line 2532 "sql.cc" /* yacc.c:1660  */
    break;

  case 106:
#line 509 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = (yyvsp[-1].expr_list);
    (yyval.order_by).desc = true;
}
#line 2541 "sql.cc" /* yacc.c:1660  */
    break;

  case 107:
#line 513 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = (yyvsp[0].expr_list);
    (yyval.order_by).desc = false;
}
#line 2550 "sql.cc" /* yacc.c:1660  */
    break;

  case 108:
#line 517 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = nullptr;
    (yyval.order_by).desc = false;
}
#line 2559 "sql.cc" /* yacc.c:1660  */
    break;

  case 109:
#line 522 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = (yyvsp[0].expr_list);
}
#line 2567 "sql.cc" /* yacc.c:1660  */
    break;

  case 110:
#line 525 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = nullptr;
}
#line 2575 "sql.cc" /* yacc.c:1660  */
    break;

  case 111:
#line 529 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
    (yyval.limit).offset_val = 0;
}
#line 2584 "sql.cc" /* yacc.c:1660  */
    break;

  case 112:
#line 533 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).offset_val = (yyvsp[-2].int_val);
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
}
#line 2593 "sql.cc" /* yacc.c:1660  */
    break;

  case 113:
#line 537 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = (yyvsp[-2].int_val);
    (yyval.limit).offset_val = (yyvsp[0].int_val);
}
#line 2602 "sql.cc" /* yacc.c:1660  */
    break;

  case 114:
#line 541 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = 0;
    (yyval.limit).offset_val = 0;
}
#line 2611 "sql.cc" /* yacc.c:1660  */
    break;

  case 115:
#line 547 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2619 "sql.cc" /* yacc.c:1660  */
    break;

  case 116:
#line 550 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2627 "sql.cc" /* yacc.c:1660  */
    break;

  case 117:
#line 556 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = ctx->factory->NewExpressionList((yyvsp[0].expr));
}
#line 2635 "sql.cc" /* yacc.c:1660  */
    break;

  case 118:
#line 559 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list)->push_back((yyvsp[0].expr));
}
#line 2643 "sql.cc" /* yacc.c:1660  */
    break;

  case 119:
#line 563 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewIdentifier(AstString::kEmpty, (yyvsp[0].name), (yylsp[0]));
}
#line 2651 "sql.cc" /* yacc.c:1660  */
    break;

  case 120:
#line 566 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewIdentifier((yyvsp[-2].name), (yyvsp[0].name), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2659 "sql.cc" /* yacc.c:1660  */
    break;

  case 121:
#line 569 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewStringLiteral((yyvsp[0].text).buf, (yyvsp[0].text).len, (yylsp[0]));
}
#line 2667 "sql.cc" /* yacc.c:1660  */
    break;

  case 122:
#line 572 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewIntegerLiteral((yyvsp[0].int_val), (yylsp[0]));
}
#line 2675 "sql.cc" /* yacc.c:1660  */
    break;

  case 123:
#line 575 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewApproxLiteral((yyvsp[0].approx_val), (yylsp[0]));
}
#line 2683 "sql.cc" /* yacc.c:1660  */
    break;

  case 124:
#line 578 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewParamPlaceholder((yylsp[0]));
}
#line 2691 "sql.cc" /* yacc.c:1660  */
    break;

  case 125:
#line 581 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-2].expr), (yyvsp[-1].op), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2699 "sql.cc" /* yacc.c:1660  */
    break;

  case 126:
#line 584 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_PLUS, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2707 "sql.cc" /* yacc.c:1660  */
    break;

  case 127:
#line 587 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_SUB, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2715 "sql.cc" /* yacc.c:1660  */
    break;

  case 128:
#line 590 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MUL, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2723 "sql.cc" /* yacc.c:1660  */
    break;

  case 129:
#line 593 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_DIV, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2731 "sql.cc" /* yacc.c:1660  */
    break;

  case 130:
#line 596 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MOD, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2739 "sql.cc" /* yacc.c:1660  */
    break;

  case 131:
#line 599 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_XOR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2747 "sql.cc" /* yacc.c:1660  */
    break;

  case 132:
#line 602 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_AND, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2755 "sql.cc" /* yacc.c:1660  */
    break;

  case 133:
#line 605 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_OR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2763 "sql.cc" /* yacc.c:1660  */
    break;

  case 134:
#line 608 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_LSHIFT, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2771 "sql.cc" /* yacc.c:1660  */
    break;

  case 135:
#line 611 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_RSHIFT, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2779 "sql.cc" /* yacc.c:1660  */
    break;

  case 136:
#line 614 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_AND, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2787 "sql.cc" /* yacc.c:1660  */
    break;

  case 137:
#line 617 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_OR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2795 "sql.cc" /* yacc.c:1660  */
    break;

  case 138:
#line 620 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_MINUS, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2803 "sql.cc" /* yacc.c:1660  */
    break;

  case 139:
#line 623 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_NOT, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2811 "sql.cc" /* yacc.c:1660  */
    break;

  case 140:
#line 626 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_BIT_INV, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2819 "sql.cc" /* yacc.c:1660  */
    break;

  case 141:
#line 629 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_IS_NOT_NULL, (yyvsp[-3].expr), Location::Concat((yylsp[-3]), (yylsp[-2])));
}
#line 2827 "sql.cc" /* yacc.c:1660  */
    break;

  case 142:
#line 632 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_IS_NULL, (yyvsp[-2].expr), Location::Concat((yylsp[-2]), (yylsp[-1])));
}
#line 2835 "sql.cc" /* yacc.c:1660  */
    break;

  case 143:
#line 635 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) =(yyvsp[-1].expr);
}
#line 2843 "sql.cc" /* yacc.c:1660  */
    break;

  case 144:
#line 639 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = ctx->factory->NewString((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 2851 "sql.cc" /* yacc.c:1660  */
    break;


#line 2855 "sql.cc" /* yacc.c:1660  */
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
#line 643 "sql.y" /* yacc.c:1903  */

void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg) {
    ctx->err_line   = yyl->first_line;
    ctx->err_column = yyl->first_column;
    ctx->err_msg    = ctx->factory->NewString(msg);
}
