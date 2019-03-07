/* A Bison parser, made by GNU Bison 3.3.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
   Inc.

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
#define YYBISON_VERSION "3.3.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 13 "sql.y" /* yacc.c:337  */

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

#line 92 "sql.cc" /* yacc.c:337  */
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
    TIME_VAL = 323,
    DATETIME_VAL = 324,
    DECIMAL_VAL = 325,
    EQ = 326,
    NOT = 327,
    OP_AND = 328,
    BIGINT = 329,
    INT = 330,
    SMALLINT = 331,
    TINYINT = 332,
    DECIMAL = 333,
    NUMERIC = 334,
    CHAR = 335,
    VARCHAR = 336,
    DATE = 337,
    DATETIME = 338,
    TIMESTMAP = 339,
    AUTO_INCREMENT = 340,
    COMMENT = 341,
    TOKEN_ERROR = 342,
    ASSIGN = 343,
    OP_OR = 344,
    XOR = 345,
    IS = 346,
    LIKE = 347,
    REGEXP = 348,
    BETWEEN = 349,
    COMPARISON = 350,
    LSHIFT = 351,
    RSHIFT = 352,
    MOD = 353,
    UMINUS = 354
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 35 "sql.y" /* yacc.c:352  */

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
    int64_t i64_val;
    double approx_val;
    bool bool_val;
    const ::mai::sql::AstDecimal *dec_val;
    const ::mai::sql::AstString *name;
    ::mai::sql::SQLKeyType key_type;
    ::mai::sql::SQLOperator op;
    ::mai::sql::SQLJoinKind join_kind;
    ::mai::sql::SQLDateTime dt_val;
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

#line 285 "sql.cc" /* yacc.c:352  */
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
#define YYFINAL  34
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   586

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  118
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  62
/* YYNRULES -- Number of rules.  */
#define YYNRULES  201
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  393

#define YYUNDEFTOK  2
#define YYMAXUTOK   354

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    94,     2,     2,     2,   105,    98,     2,
     110,   111,   103,   101,   112,   102,   113,   104,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   109,
       2,     2,     2,   116,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   107,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   114,    97,   115,   117,     2,     2,     2,
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
      85,    86,    87,    88,    89,    90,    91,    92,    93,    95,
      96,    99,   100,   106,   108
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   151,   151,   156,   161,   163,   164,   165,   168,   171,
     174,   179,   182,   185,   189,   193,   196,   200,   210,   213,
     216,   219,   222,   225,   228,   231,   234,   237,   240,   243,
     247,   251,   256,   260,   265,   268,   272,   275,   279,   282,
     285,   289,   292,   295,   298,   301,   304,   308,   311,   315,
     319,   322,   326,   329,   332,   335,   338,   342,   346,   349,
     352,   355,   358,   361,   364,   367,   370,   373,   376,   379,
     382,   386,   390,   394,   399,   402,   409,   410,   411,   413,
     427,   430,   433,   437,   440,   444,   448,   455,   461,   462,
     466,   469,   473,   476,   480,   485,   491,   496,   500,   503,
     507,   510,   513,   516,   519,   522,   526,   529,   533,   536,
     540,   544,   548,   552,   557,   560,   564,   568,   572,   576,
     582,   585,   589,   596,   602,   610,   611,   615,   618,   622,
     623,   625,   628,   632,   636,   639,   643,   644,   648,   651,
     655,   658,   662,   671,   680,   684,   694,   697,   701,   705,
     710,   713,   716,   719,   722,   725,   730,   733,   736,   739,
     742,   745,   751,   754,   757,   760,   763,   766,   769,   772,
     775,   781,   784,   787,   790,   793,   796,   799,   802,   805,
     808,   811,   814,   817,   823,   826,   829,   832,   835,   838,
     841,   844,   847,   850,   853,   856,   859,   864,   868,   871,
     875,   879
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
  "DELETE", "VALUES", "VALUE", "SET", "IN", "INTO", "DUPLICATE", "DEFAULT",
  "BINARY", "VARBINARY", "ID", "NULL_VAL", "INTEGRAL_VAL", "STRING_VAL",
  "APPROX_VAL", "DATE_VAL", "TIME_VAL", "DATETIME_VAL", "DECIMAL_VAL",
  "EQ", "NOT", "OP_AND", "BIGINT", "INT", "SMALLINT", "TINYINT", "DECIMAL",
  "NUMERIC", "CHAR", "VARCHAR", "DATE", "DATETIME", "TIMESTMAP",
  "AUTO_INCREMENT", "COMMENT", "TOKEN_ERROR", "ASSIGN", "OP_OR", "XOR",
  "IS", "LIKE", "REGEXP", "'!'", "BETWEEN", "COMPARISON", "'|'", "'&'",
  "LSHIFT", "RSHIFT", "'+'", "'-'", "'*'", "'/'", "'%'", "MOD", "'^'",
  "UMINUS", "';'", "'('", "')'", "','", "'.'", "'{'", "'}'", "'?'", "'~'",
  "$accept", "Block", "Statement", "Command", "DDL", "CreateTableStmt",
  "ColumnDefinitionList", "ColumnDefinition", "TypeDefinition",
  "FixedSizeDescription", "FloatingSizeDescription", "DefaultOption",
  "AutoIncrementOption", "NullOption", "KeyOption", "CommentOption",
  "AlterTableStmt", "AlterTableSpecList", "AlterTableSpec",
  "AlterColPosOption", "NameList", "DML", "SelectStmt", "DistinctOption",
  "ProjectionColumnList", "ProjectionColumn", "AliasOption", "Alias",
  "FromClause", "Relation", "OnClause", "JoinOp", "WhereClause",
  "HavingClause", "OrderByClause", "GroupByClause", "LimitOffsetClause",
  "ForUpdateOption", "InsertStmt", "NameListOption", "OverwriteOption",
  "ValueToken", "RowValuesList", "RowValues", "ValueList", "Value",
  "OnDuplicateClause", "AssignmentList", "Assignment", "UpdateStmt",
  "UpdateLimitOption", "ExpressionList", "Parameters", "Expression",
  "BoolPrimary", "Predicate", "BitExpression", "Simple", "Subquery",
  "Identifier", "Name", "IntVal", YY_NULLPTR
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
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,    33,   349,   350,   124,    38,   351,
     352,    43,    45,    42,    47,    37,   353,    94,   354,    59,
      40,    41,    44,    46,   123,   125,    63,   126
};
# endif

#define YYPACT_NINF -263

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-263)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     405,    11,    41,    55,    36,    74,    62,  -263,  -263,    31,
      63,   185,  -263,    26,  -263,  -263,  -263,  -263,  -263,  -263,
    -263,  -263,  -263,   355,    31,    31,  -263,    31,  -263,  -263,
     147,    97,  -263,   157,  -263,  -263,  -263,  -263,  -263,  -263,
    -263,   398,   398,   414,  -263,   398,   192,  -263,   414,     7,
    -263,   322,    84,  -263,   170,  -263,  -263,    60,   106,  -263,
     166,    31,    31,    31,   122,    93,   122,  -263,   408,   113,
     115,   129,  -263,   -24,   355,   231,    31,   398,   398,   398,
      72,  -263,  -263,  -263,    58,   414,   414,   179,    76,   414,
     414,   414,   414,   414,   414,   414,   414,   414,   414,   414,
     414,   414,    11,   -13,    31,   184,    19,   335,    30,   169,
    -263,   -12,  -263,   226,  -263,    79,  -263,  -263,  -263,  -263,
     321,    42,    42,  -263,   398,   295,  -263,   122,    69,     6,
     227,   227,  -263,   288,  -263,   242,   117,  -263,   243,   414,
     414,   242,   193,   271,   287,   225,   225,    20,    20,     3,
       3,     3,     3,   341,   372,  -263,    37,  -263,   498,   342,
      31,    31,    31,  -263,    31,    21,    31,    31,   190,    31,
      31,    31,    31,    31,  -263,    31,    31,   166,    31,   295,
     337,    31,   245,    34,  -263,   247,    38,  -263,   151,   312,
     316,   321,  -263,  -263,  -263,   254,   125,   151,   117,  -263,
     242,   211,   414,  -263,   255,   268,  -263,    31,   256,   256,
     256,   256,   256,   256,   270,   270,   256,   256,  -263,  -263,
      73,  -263,  -263,  -263,  -263,   263,    31,   190,   263,   140,
      31,    31,  -263,   354,   356,  -263,  -263,   358,    31,   190,
    -263,  -263,   334,  -263,  -263,  -263,   -21,    31,  -263,  -263,
     364,   290,    42,  -263,   187,   217,   371,   -24,   -24,   398,
     363,   375,  -263,  -263,   398,   143,   414,  -263,  -263,  -263,
     345,  -263,  -263,  -263,  -263,  -263,  -263,   345,  -263,  -263,
    -263,  -263,  -263,   353,   338,   409,   413,  -263,   323,   171,
    -263,   325,  -263,  -263,  -263,    31,    31,    31,   190,  -263,
     345,  -263,   373,  -263,  -263,  -263,   337,    -3,  -263,  -263,
    -263,   389,  -263,   399,  -263,    42,    42,    25,   398,   330,
     393,   151,  -263,  -263,  -263,   332,   333,  -263,  -263,   386,
    -263,  -263,    31,  -263,    31,  -263,  -263,  -263,  -263,  -263,
     431,   191,  -263,   290,  -263,  -263,  -263,    -5,    -5,  -263,
    -263,   255,   398,   345,   404,  -263,   345,   398,   263,   208,
     223,   412,  -263,   337,  -263,   346,  -263,  -263,   473,   -28,
     420,  -263,   350,   151,   387,  -263,  -263,    31,  -263,   398,
    -263,   345,   345,  -263,  -263,   418,  -263,   365,   475,  -263,
    -263,  -263,  -263
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    82,     0,     0,     0,     0,     0,     8,     9,     0,
     128,     0,     2,     0,     5,    11,    13,     6,    76,    77,
      78,    80,    81,     0,     0,     0,    10,     0,     7,   200,
       0,   198,   127,     0,     1,     3,     4,   186,   185,   187,
     188,     0,     0,     0,    87,     0,     0,   193,     0,    93,
      83,    89,   155,   161,   170,   183,   184,   198,     0,    12,
       0,     0,     0,     0,   153,   198,   154,   194,     0,     0,
       0,     0,   195,     0,     0,   107,     0,     0,     0,     0,
       0,    85,    88,    91,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    82,     0,     0,     0,     0,     0,     0,    49,
      50,   107,   140,     0,   199,   126,   196,   189,   190,   191,
       0,    92,    89,    84,     0,   113,    90,   152,   150,   151,
       0,     0,   156,     0,   158,   179,     0,   163,     0,     0,
       0,   169,     0,   171,   172,   173,   174,   175,   176,   177,
     178,   180,   181,   182,     0,    86,     0,    15,     0,     0,
       0,     0,     0,    67,     0,     0,     0,     0,    73,     0,
       0,     0,     0,     0,    63,     0,     0,     0,     0,   113,
       0,     0,   125,     0,    74,     0,     0,    97,   106,     0,
     115,     0,   160,   159,   157,     0,     0,   146,     0,   162,
     168,     0,     0,   148,   149,     0,    14,     0,    31,    31,
      31,    31,    31,    31,    33,    33,    31,    31,    28,    29,
      40,    70,    69,    66,    68,    46,     0,    73,    46,     0,
       0,     0,    52,     0,     0,    64,    65,     0,     0,    73,
      51,   141,   145,   137,   142,   136,   139,     0,   130,   129,
     139,     0,     0,   100,     0,     0,     0,     0,     0,     0,
       0,   109,   197,   165,     0,     0,     0,   167,   192,    16,
       0,    26,    27,    18,    19,    20,    21,     0,    22,    23,
      24,    25,    39,     0,    37,    42,    44,    41,     0,     0,
      53,     0,    54,    72,    71,     0,     0,     0,    73,    58,
       0,   143,     0,   123,    75,   124,     0,   139,   131,    94,
     102,     0,   104,     0,   101,     0,     0,   112,     0,     0,
     119,   147,   164,   166,   201,     0,     0,    38,    36,    35,
      43,    45,     0,    55,     0,    62,    60,    61,    59,   144,
       0,     0,   134,     0,   122,   103,   105,    99,    99,   110,
     111,   114,     0,     0,   121,    30,     0,     0,    46,     0,
       0,     0,   133,     0,   132,     0,    95,    96,     0,   116,
       0,    79,     0,    34,    48,    57,    56,     0,   135,     0,
     108,     0,     0,   120,    32,     0,    17,   138,     0,   118,
     117,    47,    98
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -263,  -263,   474,  -263,  -263,  -263,  -137,   -99,  -263,   340,
     272,  -263,  -263,  -263,  -218,  -263,  -263,  -263,   313,  -182,
    -260,  -263,     8,   391,  -263,   417,   374,  -119,  -263,    47,
     146,  -263,   384,  -263,   320,  -263,  -263,  -263,  -263,  -263,
    -263,  -263,  -263,   158,  -263,  -262,  -204,  -178,   324,  -263,
    -263,  -142,  -263,   -19,  -263,   -80,   443,   -42,    16,     0,
      -9,  -238
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    11,    12,    13,    14,    15,   156,   157,   220,   271,
     278,   358,   329,   284,   288,   386,    16,   109,   110,   232,
     182,    17,   195,    23,    49,    50,    81,    82,    75,   121,
     366,   258,   125,   320,   190,   261,   354,   371,    19,   183,
      33,   251,   307,   308,   341,   244,   303,   111,   112,    20,
     301,   196,   205,   197,    52,    53,    54,    55,   137,    56,
      65,   325
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint16 yytable[] =
{
      31,    67,   186,   246,    51,   134,    72,   168,    18,    30,
     291,    73,   204,   302,    57,    31,    31,    86,    31,    18,
     124,   381,    64,    66,    58,    59,    68,    60,   253,   365,
     229,   302,   254,   255,    86,   256,   164,     1,    29,   326,
     165,    21,    83,    26,   342,   290,   305,    24,   166,    29,
      22,   175,   113,   114,    31,    51,   265,   299,   127,   128,
     129,    25,   339,   115,    31,    57,   227,   126,   349,   350,
      76,   253,   359,   122,   360,   254,   255,   239,   256,    77,
      27,    29,    28,    29,   382,   130,   120,   248,   249,   289,
     155,   178,    29,    29,   114,   158,   163,   158,   174,   176,
     178,   378,    80,   344,    29,   188,   184,   257,   269,   343,
     101,   131,    83,    83,    32,   369,   338,   317,   372,    74,
       1,   132,   267,    97,    98,    99,   100,   101,   185,   167,
     133,   226,   138,   309,   181,    36,   282,   264,   245,   298,
     374,    29,    77,   389,   390,   283,   192,   193,   206,   207,
     257,   222,   223,   224,   199,   225,   158,   228,   158,    79,
     233,   234,   235,   236,   237,    80,   238,   158,   139,   113,
     102,   140,   113,   103,   105,    84,   351,   106,   107,    29,
      85,    37,    38,    39,    86,    34,   323,    40,     1,    41,
       2,   250,   108,     3,     4,     5,   347,   348,   158,   387,
     159,   160,    61,   102,     6,   161,    62,    86,     7,     8,
      62,    42,   230,   162,    63,   231,   104,   158,    80,    43,
     310,   293,   294,   311,    77,    86,    87,    45,   117,   158,
     118,    46,     9,    47,    48,    10,   263,   264,   304,    86,
      78,    79,    88,    83,   119,   321,    29,    80,    31,    31,
     312,   292,   207,   313,   322,   264,    86,   122,   122,    69,
      70,    71,    89,   124,   245,    90,   202,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   285,   286,
     287,   177,   333,   207,   266,    86,   335,   336,   337,   136,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,    86,   362,   363,   315,   316,    83,    83,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   375,
     247,   245,   180,   184,     1,   184,    95,    96,    97,    98,
      99,   100,   101,   368,   376,   247,   189,   191,   373,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
      76,   194,   169,   198,   259,    86,   170,   247,   252,   221,
     388,   260,   171,   172,   173,   262,   270,   264,   113,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   268,
     277,   295,   300,   296,    29,   297,    93,    94,    95,    96,
      97,    98,    99,   100,   101,    77,   243,    29,   302,    29,
     306,    37,    38,    39,   314,   318,   319,    40,     1,   324,
       2,    78,    79,     3,     4,     5,   327,    29,    80,    37,
      38,    39,   345,   328,     6,    40,   330,    41,     7,     8,
     331,   340,   346,   332,    29,   334,    37,    38,    39,    43,
     352,   353,    40,   355,    41,   357,   356,    45,   361,    42,
     370,    46,     9,    47,    48,    10,   379,    43,    44,   377,
      29,   384,    37,    38,    39,    45,    42,   383,    40,    46,
      41,    47,    48,   385,    43,   203,    29,   178,    37,    38,
      39,    77,    45,   391,    40,    35,    46,   279,    47,    48,
     240,   123,    42,   154,   367,   179,   187,    78,    79,   242,
      43,   364,   241,     0,    80,     0,     0,     0,    45,     0,
       0,     0,    46,     0,    47,    48,    43,     0,     0,   116,
       0,     0,     0,     0,    45,     0,     0,     0,    46,   135,
      47,    48,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,     0,    77,     0,    77,   272,
     273,   274,   275,   276,     0,     0,   280,   281,   208,   209,
       0,     0,    78,    79,    78,    79,     0,     0,     0,    80,
       0,    80,   210,   211,   212,   213,   214,   215,   216,   217,
     218,   219,   200,   201,   380,     0,   392
};

static const yytype_int16 yycheck[] =
{
       9,    43,   121,   181,    23,    85,    48,   106,     0,     9,
     228,     4,   154,    34,    23,    24,    25,    14,    27,    11,
      32,    49,    41,    42,    24,    25,    45,    27,    33,    34,
     167,    34,    37,    38,    14,    40,    17,     3,    62,   277,
      21,    30,    51,     7,   306,   227,   250,     6,    29,    62,
      39,    21,    61,    62,    63,    74,   198,   239,    77,    78,
      79,     6,   300,    63,    73,    74,   165,    76,    43,    44,
      28,    33,   332,    73,   334,    37,    38,   176,    40,    73,
       6,    62,    20,    62,   112,    13,   110,    53,    54,   226,
     103,   112,    62,    62,   103,   104,   105,   106,   107,   108,
     112,   363,    96,   307,    62,   124,   115,   112,   207,   112,
     107,    39,   121,   122,    51,   353,   298,   259,   356,   112,
       3,    63,   202,   103,   104,   105,   106,   107,   120,   110,
      72,   110,    56,   252,    55,   109,    63,   112,   180,   238,
     358,    62,    73,   381,   382,    72,   130,   131,   111,   112,
     112,   160,   161,   162,   138,   164,   165,   166,   167,    90,
     169,   170,   171,   172,   173,    96,   175,   176,    92,   178,
     110,    95,   181,   113,     8,    91,   318,    11,    12,    62,
      96,    64,    65,    66,    14,     0,   266,    70,     3,    72,
       5,   183,    26,     8,     9,    10,   315,   316,   207,   377,
      16,    17,    55,   110,    19,    21,   113,    14,    23,    24,
     113,    94,    22,    29,    57,    25,   110,   226,    96,   102,
      33,   230,   231,    36,    73,    14,    56,   110,   115,   238,
     115,   114,    47,   116,   117,    50,   111,   112,   247,    14,
      89,    90,    72,   252,   115,   264,    62,    96,   257,   258,
      33,   111,   112,    36,   111,   112,    14,   257,   258,    67,
      68,    69,    92,    32,   306,    95,    73,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,    15,    16,
      17,   112,   111,   112,    73,    14,   295,   296,   297,   110,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,    14,   111,   112,   257,   258,   315,   316,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   111,
     112,   363,    96,   332,     3,   334,   101,   102,   103,   104,
     105,   106,   107,   352,   111,   112,    41,   110,   357,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
      28,    63,    17,   110,    42,    14,    21,   112,   111,    17,
     379,    45,    27,    28,    29,   111,   110,   112,   377,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   111,
     110,    27,    48,    27,    62,    27,    99,   100,   101,   102,
     103,   104,   105,   106,   107,    73,    59,    62,    34,    62,
     110,    64,    65,    66,    33,    42,    31,    70,     3,    64,
       5,    89,    90,     8,     9,    10,    63,    62,    96,    64,
      65,    66,    33,    85,    19,    70,    17,    72,    23,    24,
      17,    58,    33,   110,    62,   110,    64,    65,    66,   102,
     110,    48,    70,   111,    72,    59,   113,   110,    17,    94,
      46,   114,    47,   116,   117,    50,   110,   102,   103,    47,
      62,   111,    64,    65,    66,   110,    94,    47,    70,   114,
      72,   116,   117,    86,   102,   103,    62,   112,    64,    65,
      66,    73,   110,    65,    70,    11,   114,   215,   116,   117,
     177,    74,    94,   102,   348,   111,   122,    89,    90,   179,
     102,   343,   178,    -1,    96,    -1,    -1,    -1,   110,    -1,
      -1,    -1,   114,    -1,   116,   117,   102,    -1,    -1,   111,
      -1,    -1,    -1,    -1,   110,    -1,    -1,    -1,   114,    86,
     116,   117,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,    -1,    73,    -1,    73,   209,
     210,   211,   212,   213,    -1,    -1,   216,   217,    60,    61,
      -1,    -1,    89,    90,    89,    90,    -1,    -1,    -1,    96,
      -1,    96,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,   139,   140,   111,    -1,   111
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     5,     8,     9,    10,    19,    23,    24,    47,
      50,   119,   120,   121,   122,   123,   134,   139,   140,   156,
     167,    30,    39,   141,     6,     6,     7,     6,    20,    62,
     177,   178,    51,   158,     0,   120,   109,    64,    65,    66,
      70,    72,    94,   102,   103,   110,   114,   116,   117,   142,
     143,   171,   172,   173,   174,   175,   177,   178,   177,   177,
     177,    55,   113,    57,   171,   178,   171,   175,   171,    67,
      68,    69,   175,     4,   112,   146,    28,    73,    89,    90,
      96,   144,   145,   178,    91,    96,    14,    56,    72,    92,
      95,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   110,   113,   110,     8,    11,    12,    26,   135,
     136,   165,   166,   178,   178,   177,   111,   115,   115,   115,
     110,   147,   177,   143,    32,   150,   178,   171,   171,   171,
      13,    39,    63,    72,   173,   174,   110,   176,    56,    92,
      95,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   141,   103,   124,   125,   178,    16,
      17,    21,    29,   178,    17,    21,    29,   110,   125,    17,
      21,    27,    28,    29,   178,    21,   178,   112,   112,   150,
      96,    55,   138,   157,   178,   140,   145,   144,   171,    41,
     152,   110,   176,   176,    63,   140,   169,   171,   110,   176,
     174,   174,    73,   103,   169,   170,   111,   112,    60,    61,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
     126,    17,   178,   178,   178,   178,   110,   125,   178,   124,
      22,    25,   137,   178,   178,   178,   178,   178,   178,   125,
     136,   166,   152,    59,   163,   175,   165,   112,    53,    54,
     140,   159,   111,    33,    37,    38,    40,   112,   149,    42,
      45,   153,   111,   111,   112,   169,    73,   173,   111,   125,
     110,   127,   127,   127,   127,   127,   127,   110,   128,   128,
     127,   127,    63,    72,   131,    15,    16,    17,   132,   124,
     137,   132,   111,   178,   178,    27,    27,    27,   125,   137,
      48,   168,    34,   164,   178,   164,   110,   160,   161,   145,
      33,    36,    33,    36,    33,   147,   147,   169,    42,    31,
     151,   171,   111,   173,    64,   179,   179,    63,    85,   130,
      17,    17,   110,   111,   110,   178,   178,   178,   137,   179,
      58,   162,   163,   112,   164,    33,    33,   145,   145,    43,
      44,   169,   110,    48,   154,   111,   113,    59,   129,   138,
     138,    17,   111,   112,   161,    34,   148,   148,   171,   179,
      46,   155,   179,   171,   132,   111,   111,    47,   163,   110,
     111,    49,   112,    47,   111,    86,   133,   165,   171,   179,
     179,    65,   111
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   118,   119,   119,   120,   121,   121,   121,   121,   121,
     121,   122,   122,   122,   123,   124,   124,   125,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     127,   127,   128,   128,   129,   129,   130,   130,   131,   131,
     131,   132,   132,   132,   132,   132,   132,   133,   133,   134,
     135,   135,   136,   136,   136,   136,   136,   136,   136,   136,
     136,   136,   136,   136,   136,   136,   136,   136,   136,   136,
     136,   137,   137,   137,   138,   138,   139,   139,   139,   140,
     141,   141,   141,   142,   142,   143,   143,   143,   144,   144,
     145,   145,   146,   146,   147,   147,   147,   147,   148,   148,
     149,   149,   149,   149,   149,   149,   150,   150,   151,   151,
     152,   152,   152,   152,   153,   153,   154,   154,   154,   154,
     155,   155,   156,   156,   156,   157,   157,   158,   158,   159,
     159,   160,   160,   161,   162,   162,   163,   163,   164,   164,
     165,   165,   166,   167,   168,   168,   169,   169,   170,   170,
     171,   171,   171,   171,   171,   171,   172,   172,   172,   172,
     172,   172,   173,   173,   173,   173,   173,   173,   173,   173,
     173,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   175,   175,   175,   175,   175,   175,
     175,   175,   175,   175,   175,   175,   175,   176,   177,   177,
     178,   179
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     2,     1,     1,     2,     1,     1,
       2,     1,     3,     1,     6,     1,     3,     7,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     1,     1,
       3,     0,     5,     0,     2,     0,     1,     0,     2,     1,
       0,     1,     1,     2,     1,     2,     0,     2,     0,     4,
       1,     3,     3,     4,     4,     5,     7,     7,     4,     5,
       5,     5,     5,     2,     3,     3,     3,     2,     3,     3,
       3,     2,     2,     0,     1,     3,     1,     1,     1,    10,
       1,     1,     0,     1,     3,     2,     3,     1,     1,     0,
       2,     1,     2,     0,     4,     6,     6,     2,     4,     0,
       1,     2,     2,     3,     2,     3,     2,     0,     4,     0,
       4,     4,     3,     0,     3,     0,     2,     4,     4,     0,
       2,     0,     8,     7,     7,     1,     0,     1,     0,     1,
       1,     1,     3,     3,     1,     3,     1,     1,     5,     0,
       1,     3,     3,     7,     2,     0,     1,     3,     1,     1,
       3,     3,     3,     2,     2,     1,     3,     4,     3,     4,
       4,     1,     4,     3,     6,     5,     6,     5,     4,     3,
       1,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     1,     1,     1,     1,     1,     1,     3,
       3,     3,     5,     1,     2,     2,     3,     3,     1,     3,
       1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
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
                       &yyvsp[(yyi + 1) - (yynrhs)]
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
            else
              goto append;

          append:
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
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
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
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
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
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yynewstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  *yyssp = (yytype_int16) yystate;

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = (YYSIZE_T) (yyssp - yyss + 1);

# if defined yyoverflow
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
# else /* defined YYSTACK_RELOCATE */
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
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

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
| yyreduce -- do a reduction.  |
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
#line 151 "sql.y" /* yacc.c:1667  */
    {
    (yyval.block) = ctx->factory->NewBlock();
    (yyval.block)->AddStmt((yyvsp[0].stmt));
    ctx->block = (yyval.block);
}
#line 1841 "sql.cc" /* yacc.c:1667  */
    break;

  case 3:
#line 156 "sql.y" /* yacc.c:1667  */
    {
    (yyval.block)->AddStmt((yyvsp[0].stmt));
    ctx->block = (yyval.block);
}
#line 1850 "sql.cc" /* yacc.c:1667  */
    break;

  case 7:
#line 165 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(ast::TCLStatement::TXN_BEGIN);
}
#line 1858 "sql.cc" /* yacc.c:1667  */
    break;

  case 8:
#line 168 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(ast::TCLStatement::TXN_COMMIT);
}
#line 1866 "sql.cc" /* yacc.c:1667  */
    break;

  case 9:
#line 171 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(ast::TCLStatement::TXN_ROLLBACK);
}
#line 1874 "sql.cc" /* yacc.c:1667  */
    break;

  case 10:
#line 174 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewShowTables();
}
#line 1882 "sql.cc" /* yacc.c:1667  */
    break;

  case 11:
#line 179 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1890 "sql.cc" /* yacc.c:1667  */
    break;

  case 12:
#line 182 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewDropTable((yyvsp[0].id));
}
#line 1898 "sql.cc" /* yacc.c:1667  */
    break;

  case 13:
#line 185 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1906 "sql.cc" /* yacc.c:1667  */
    break;

  case 14:
#line 189 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewCreateTable((yyvsp[-3].id), (yyvsp[-1].col_def_list));
}
#line 1914 "sql.cc" /* yacc.c:1667  */
    break;

  case 15:
#line 193 "sql.y" /* yacc.c:1667  */
    {
    (yyval.col_def_list) = ctx->factory->NewColumnDefinitionList((yyvsp[0].col_def));
}
#line 1922 "sql.cc" /* yacc.c:1667  */
    break;

  case 16:
#line 196 "sql.y" /* yacc.c:1667  */
    {
    (yyval.col_def_list)->push_back((yyvsp[0].col_def));
}
#line 1930 "sql.cc" /* yacc.c:1667  */
    break;

  case 17:
#line 200 "sql.y" /* yacc.c:1667  */
    {
    auto *def = ctx->factory->NewColumnDefinition((yyvsp[-6].name), (yyvsp[-5].type_def));
    def->set_is_not_null((yyvsp[-4].bool_val));
    def->set_auto_increment((yyvsp[-3].bool_val));
    def->set_default_value((yyvsp[-2].expr));
    def->set_key((yyvsp[-1].key_type));
    def->set_comment((yyvsp[0].name));
    (yyval.col_def) = def;
}
#line 1944 "sql.cc" /* yacc.c:1667  */
    break;

  case 18:
#line 210 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_BIGINT, (yyvsp[0].size).fixed_size);
}
#line 1952 "sql.cc" /* yacc.c:1667  */
    break;

  case 19:
#line 213 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_INT, (yyvsp[0].size).fixed_size);
}
#line 1960 "sql.cc" /* yacc.c:1667  */
    break;

  case 20:
#line 216 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_SMALLINT, (yyvsp[0].size).fixed_size);
}
#line 1968 "sql.cc" /* yacc.c:1667  */
    break;

  case 21:
#line 219 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_TINYINT, (yyvsp[0].size).fixed_size);
}
#line 1976 "sql.cc" /* yacc.c:1667  */
    break;

  case 22:
#line 222 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DECIMAL, (yyvsp[0].size).fixed_size, (yyvsp[0].size).float_size);
}
#line 1984 "sql.cc" /* yacc.c:1667  */
    break;

  case 23:
#line 225 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_NUMERIC, (yyvsp[0].size).fixed_size, (yyvsp[0].size).float_size);
}
#line 1992 "sql.cc" /* yacc.c:1667  */
    break;

  case 24:
#line 228 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_NUMERIC, (yyvsp[0].size).fixed_size);
}
#line 2000 "sql.cc" /* yacc.c:1667  */
    break;

  case 25:
#line 231 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_VARCHAR, (yyvsp[0].size).fixed_size);
}
#line 2008 "sql.cc" /* yacc.c:1667  */
    break;

  case 26:
#line 234 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_BINARY, (yyvsp[0].size).fixed_size);
}
#line 2016 "sql.cc" /* yacc.c:1667  */
    break;

  case 27:
#line 237 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_VARBINARY, (yyvsp[0].size).fixed_size);
}
#line 2024 "sql.cc" /* yacc.c:1667  */
    break;

  case 28:
#line 240 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DATE);
}
#line 2032 "sql.cc" /* yacc.c:1667  */
    break;

  case 29:
#line 243 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DATETIME);
}
#line 2040 "sql.cc" /* yacc.c:1667  */
    break;

  case 30:
#line 247 "sql.y" /* yacc.c:1667  */
    {
    (yyval.size).fixed_size = (yyvsp[-1].int_val);
    (yyval.size).float_size = 0;
}
#line 2049 "sql.cc" /* yacc.c:1667  */
    break;

  case 31:
#line 251 "sql.y" /* yacc.c:1667  */
    {
    (yyval.size).fixed_size = 0;
    (yyval.size).float_size = 0;
}
#line 2058 "sql.cc" /* yacc.c:1667  */
    break;

  case 32:
#line 256 "sql.y" /* yacc.c:1667  */
    {
    (yyval.size).fixed_size = (yyvsp[-3].int_val);
    (yyval.size).float_size = (yyvsp[-1].int_val);
}
#line 2067 "sql.cc" /* yacc.c:1667  */
    break;

  case 33:
#line 260 "sql.y" /* yacc.c:1667  */
    {
    (yyval.size).fixed_size = 0;
    (yyval.size).float_size = 0;
}
#line 2076 "sql.cc" /* yacc.c:1667  */
    break;

  case 34:
#line 265 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2084 "sql.cc" /* yacc.c:1667  */
    break;

  case 35:
#line 268 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = nullptr;
}
#line 2092 "sql.cc" /* yacc.c:1667  */
    break;

  case 36:
#line 272 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = true;
}
#line 2100 "sql.cc" /* yacc.c:1667  */
    break;

  case 37:
#line 275 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 2108 "sql.cc" /* yacc.c:1667  */
    break;

  case 38:
#line 279 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = true;
}
#line 2116 "sql.cc" /* yacc.c:1667  */
    break;

  case 39:
#line 282 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 2124 "sql.cc" /* yacc.c:1667  */
    break;

  case 40:
#line 285 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 2132 "sql.cc" /* yacc.c:1667  */
    break;

  case 41:
#line 289 "sql.y" /* yacc.c:1667  */
    {
    (yyval.key_type) = SQL_KEY;
}
#line 2140 "sql.cc" /* yacc.c:1667  */
    break;

  case 42:
#line 292 "sql.y" /* yacc.c:1667  */
    {
    (yyval.key_type) = SQL_UNIQUE_KEY;
}
#line 2148 "sql.cc" /* yacc.c:1667  */
    break;

  case 43:
#line 295 "sql.y" /* yacc.c:1667  */
    {
    (yyval.key_type) = SQL_UNIQUE_KEY;
}
#line 2156 "sql.cc" /* yacc.c:1667  */
    break;

  case 44:
#line 298 "sql.y" /* yacc.c:1667  */
    {
    (yyval.key_type) = SQL_PRIMARY_KEY;
}
#line 2164 "sql.cc" /* yacc.c:1667  */
    break;

  case 45:
#line 301 "sql.y" /* yacc.c:1667  */
    {
    (yyval.key_type) = SQL_PRIMARY_KEY;
}
#line 2172 "sql.cc" /* yacc.c:1667  */
    break;

  case 46:
#line 304 "sql.y" /* yacc.c:1667  */
    {
    (yyval.key_type) = SQL_NOT_KEY;
}
#line 2180 "sql.cc" /* yacc.c:1667  */
    break;

  case 47:
#line 308 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name) = ctx->factory->NewString((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 2188 "sql.cc" /* yacc.c:1667  */
    break;

  case 48:
#line 311 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name) = AstString::kEmpty;
}
#line 2196 "sql.cc" /* yacc.c:1667  */
    break;

  case 49:
#line 315 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewAlterTable((yyvsp[-1].id), (yyvsp[0].alter_table_spce_list));
}
#line 2204 "sql.cc" /* yacc.c:1667  */
    break;

  case 50:
#line 319 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce_list) = ctx->factory->NewAlterTableSpecList((yyvsp[0].alter_table_spce));
}
#line 2212 "sql.cc" /* yacc.c:1667  */
    break;

  case 51:
#line 322 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce_list)->push_back((yyvsp[0].alter_table_spce));
}
#line 2220 "sql.cc" /* yacc.c:1667  */
    break;

  case 52:
#line 326 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2228 "sql.cc" /* yacc.c:1667  */
    break;

  case 53:
#line 329 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2236 "sql.cc" /* yacc.c:1667  */
    break;

  case 54:
#line 332 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def_list));
}
#line 2244 "sql.cc" /* yacc.c:1667  */
    break;

  case 55:
#line 335 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def_list));
}
#line 2252 "sql.cc" /* yacc.c:1667  */
    break;

  case 56:
#line 338 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddIndex((yyvsp[-4].name), (yyvsp[-3].key_type) == SQL_NOT_KEY ? SQL_KEY : (yyvsp[-3].key_type),
                                             (yyvsp[-1].name_list));
}
#line 2261 "sql.cc" /* yacc.c:1667  */
    break;

  case 57:
#line 342 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddIndex((yyvsp[-4].name), (yyvsp[-3].key_type) == SQL_NOT_KEY ? SQL_KEY : (yyvsp[-3].key_type),
                                             (yyvsp[-1].name_list));
}
#line 2270 "sql.cc" /* yacc.c:1667  */
    break;

  case 58:
#line 346 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableChangeColumn((yyvsp[-2].name), (yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2278 "sql.cc" /* yacc.c:1667  */
    break;

  case 59:
#line 349 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableChangeColumn((yyvsp[-2].name), (yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2286 "sql.cc" /* yacc.c:1667  */
    break;

  case 60:
#line 352 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameColumn((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2294 "sql.cc" /* yacc.c:1667  */
    break;

  case 61:
#line 355 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameIndex((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2302 "sql.cc" /* yacc.c:1667  */
    break;

  case 62:
#line 358 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameIndex((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2310 "sql.cc" /* yacc.c:1667  */
    break;

  case 63:
#line 361 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2318 "sql.cc" /* yacc.c:1667  */
    break;

  case 64:
#line 364 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2326 "sql.cc" /* yacc.c:1667  */
    break;

  case 65:
#line 367 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2334 "sql.cc" /* yacc.c:1667  */
    break;

  case 66:
#line 370 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropColumn((yyvsp[0].name));
}
#line 2342 "sql.cc" /* yacc.c:1667  */
    break;

  case 67:
#line 373 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropColumn((yyvsp[0].name));
}
#line 2350 "sql.cc" /* yacc.c:1667  */
    break;

  case 68:
#line 376 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex((yyvsp[0].name), false);
}
#line 2358 "sql.cc" /* yacc.c:1667  */
    break;

  case 69:
#line 379 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex((yyvsp[0].name), false);
}
#line 2366 "sql.cc" /* yacc.c:1667  */
    break;

  case 70:
#line 382 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex(AstString::kEmpty, true);
}
#line 2374 "sql.cc" /* yacc.c:1667  */
    break;

  case 71:
#line 386 "sql.y" /* yacc.c:1667  */
    {
    (yyval.col_pos).name  = (yyvsp[0].name);
    (yyval.col_pos).after = false;
}
#line 2383 "sql.cc" /* yacc.c:1667  */
    break;

  case 72:
#line 390 "sql.y" /* yacc.c:1667  */
    {
    (yyval.col_pos).name  = (yyvsp[0].name);
    (yyval.col_pos).after = true;
}
#line 2392 "sql.cc" /* yacc.c:1667  */
    break;

  case 73:
#line 394 "sql.y" /* yacc.c:1667  */
    {
    (yyval.col_pos).name  = AstString::kEmpty;
    (yyval.col_pos).after = false;
}
#line 2401 "sql.cc" /* yacc.c:1667  */
    break;

  case 74:
#line 399 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name_list) = ctx->factory->NewNameList((yyvsp[0].name));
}
#line 2409 "sql.cc" /* yacc.c:1667  */
    break;

  case 75:
#line 402 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name_list)->push_back((yyvsp[0].name));
}
#line 2417 "sql.cc" /* yacc.c:1667  */
    break;

  case 79:
#line 413 "sql.y" /* yacc.c:1667  */
    {
    auto *stmt = ctx->factory->NewSelect((yyvsp[-8].bool_val), (yyvsp[-7].proj_col_list), AstString::kEmpty);
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
#line 2435 "sql.cc" /* yacc.c:1667  */
    break;

  case 80:
#line 427 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = true;
}
#line 2443 "sql.cc" /* yacc.c:1667  */
    break;

  case 81:
#line 430 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 2451 "sql.cc" /* yacc.c:1667  */
    break;

  case 82:
#line 433 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 2459 "sql.cc" /* yacc.c:1667  */
    break;

  case 83:
#line 437 "sql.y" /* yacc.c:1667  */
    {
    (yyval.proj_col_list) = ctx->factory->NewProjectionColumnList((yyvsp[0].proj_col));
}
#line 2467 "sql.cc" /* yacc.c:1667  */
    break;

  case 84:
#line 440 "sql.y" /* yacc.c:1667  */
    {
    (yyval.proj_col_list)->push_back((yyvsp[0].proj_col));
}
#line 2475 "sql.cc" /* yacc.c:1667  */
    break;

  case 85:
#line 444 "sql.y" /* yacc.c:1667  */
    {
    (yyval.proj_col) = ctx->factory->NewProjectionColumn((yyvsp[-1].expr), (yyvsp[0].name),
                                           Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2484 "sql.cc" /* yacc.c:1667  */
    break;

  case 86:
#line 448 "sql.y" /* yacc.c:1667  */
    {
    Identifier *id = ctx->factory->NewIdentifierWithPlaceholder((yyvsp[-2].name),
        ctx->factory->NewStarPlaceholder((yylsp[0])),
        Location::Concat((yylsp[-2]), (yylsp[0])));
    (yyval.proj_col) = ctx->factory->NewProjectionColumn(id, AstString::kEmpty,
        Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2496 "sql.cc" /* yacc.c:1667  */
    break;

  case 87:
#line 455 "sql.y" /* yacc.c:1667  */
    {
    Placeholder *ph = ctx->factory->NewStarPlaceholder((yylsp[0]));
    (yyval.proj_col) = ctx->factory->NewProjectionColumn(ph, AstString::kEmpty, (yylsp[0]));
}
#line 2505 "sql.cc" /* yacc.c:1667  */
    break;

  case 89:
#line 462 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name) = AstString::kEmpty;
}
#line 2513 "sql.cc" /* yacc.c:1667  */
    break;

  case 90:
#line 466 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name) = (yyvsp[0].name);
}
#line 2521 "sql.cc" /* yacc.c:1667  */
    break;

  case 91:
#line 469 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name) = (yyvsp[0].name);
}
#line 2529 "sql.cc" /* yacc.c:1667  */
    break;

  case 92:
#line 473 "sql.y" /* yacc.c:1667  */
    {
    (yyval.query) = (yyvsp[0].query);
}
#line 2537 "sql.cc" /* yacc.c:1667  */
    break;

  case 93:
#line 476 "sql.y" /* yacc.c:1667  */
    {
    (yyval.query) = nullptr;
}
#line 2545 "sql.cc" /* yacc.c:1667  */
    break;

  case 94:
#line 480 "sql.y" /* yacc.c:1667  */
    {
    Query *query = ::mai::down_cast<Query>((yyvsp[-2].stmt));
    query->set_alias((yyvsp[0].name));
    (yyval.query) = query;
}
#line 2555 "sql.cc" /* yacc.c:1667  */
    break;

  case 95:
#line 485 "sql.y" /* yacc.c:1667  */
    {
    (yyvsp[-5].query)->set_alias((yyvsp[-4].name));
    (yyvsp[-2].query)->set_alias((yyvsp[-1].name));
    (yyval.query) = ctx->factory->NewJoinRelation((yyvsp[-5].query), SQL_CROSS_JOIN, (yyvsp[-2].query), (yyvsp[0].expr),
        AstString::kEmpty);
}
#line 2566 "sql.cc" /* yacc.c:1667  */
    break;

  case 96:
#line 491 "sql.y" /* yacc.c:1667  */
    {
    (yyvsp[-5].query)->set_alias((yyvsp[-4].name));
    (yyvsp[-2].query)->set_alias((yyvsp[-1].name));
    (yyval.query) = ctx->factory->NewJoinRelation((yyvsp[-5].query), (yyvsp[-3].join_kind), (yyvsp[-2].query), (yyvsp[0].expr), AstString::kEmpty);
}
#line 2576 "sql.cc" /* yacc.c:1667  */
    break;

  case 97:
#line 496 "sql.y" /* yacc.c:1667  */
    {
    (yyval.query) = ctx->factory->NewNameRelation((yyvsp[-1].id), (yyvsp[0].name));
}
#line 2584 "sql.cc" /* yacc.c:1667  */
    break;

  case 98:
#line 500 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2592 "sql.cc" /* yacc.c:1667  */
    break;

  case 99:
#line 503 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = nullptr;
}
#line 2600 "sql.cc" /* yacc.c:1667  */
    break;

  case 100:
#line 507 "sql.y" /* yacc.c:1667  */
    {
    (yyval.join_kind) = SQL_CROSS_JOIN;
}
#line 2608 "sql.cc" /* yacc.c:1667  */
    break;

  case 101:
#line 510 "sql.y" /* yacc.c:1667  */
    {
    (yyval.join_kind) = SQL_CROSS_JOIN;
}
#line 2616 "sql.cc" /* yacc.c:1667  */
    break;

  case 102:
#line 513 "sql.y" /* yacc.c:1667  */
    {
    (yyval.join_kind) = SQL_LEFT_OUTTER_JOIN;
}
#line 2624 "sql.cc" /* yacc.c:1667  */
    break;

  case 103:
#line 516 "sql.y" /* yacc.c:1667  */
    {
    (yyval.join_kind) = SQL_LEFT_OUTTER_JOIN;
}
#line 2632 "sql.cc" /* yacc.c:1667  */
    break;

  case 104:
#line 519 "sql.y" /* yacc.c:1667  */
    {
    (yyval.join_kind) = SQL_RIGHT_OUTTER_JOIN;
}
#line 2640 "sql.cc" /* yacc.c:1667  */
    break;

  case 105:
#line 522 "sql.y" /* yacc.c:1667  */
    {
    (yyval.join_kind) = SQL_RIGHT_OUTTER_JOIN;
}
#line 2648 "sql.cc" /* yacc.c:1667  */
    break;

  case 106:
#line 526 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2656 "sql.cc" /* yacc.c:1667  */
    break;

  case 107:
#line 529 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = nullptr;
}
#line 2664 "sql.cc" /* yacc.c:1667  */
    break;

  case 108:
#line 533 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2672 "sql.cc" /* yacc.c:1667  */
    break;

  case 109:
#line 536 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = nullptr;
}
#line 2680 "sql.cc" /* yacc.c:1667  */
    break;

  case 110:
#line 540 "sql.y" /* yacc.c:1667  */
    {
    (yyval.order_by).expr_list = (yyvsp[-1].expr_list);
    (yyval.order_by).desc = false;
}
#line 2689 "sql.cc" /* yacc.c:1667  */
    break;

  case 111:
#line 544 "sql.y" /* yacc.c:1667  */
    {
    (yyval.order_by).expr_list = (yyvsp[-1].expr_list);
    (yyval.order_by).desc = true;
}
#line 2698 "sql.cc" /* yacc.c:1667  */
    break;

  case 112:
#line 548 "sql.y" /* yacc.c:1667  */
    {
    (yyval.order_by).expr_list = (yyvsp[0].expr_list);
    (yyval.order_by).desc = false;
}
#line 2707 "sql.cc" /* yacc.c:1667  */
    break;

  case 113:
#line 552 "sql.y" /* yacc.c:1667  */
    {
    (yyval.order_by).expr_list = nullptr;
    (yyval.order_by).desc = false;
}
#line 2716 "sql.cc" /* yacc.c:1667  */
    break;

  case 114:
#line 557 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr_list) = (yyvsp[0].expr_list);
}
#line 2724 "sql.cc" /* yacc.c:1667  */
    break;

  case 115:
#line 560 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr_list) = nullptr;
}
#line 2732 "sql.cc" /* yacc.c:1667  */
    break;

  case 116:
#line 564 "sql.y" /* yacc.c:1667  */
    {
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
    (yyval.limit).offset_val = 0;
}
#line 2741 "sql.cc" /* yacc.c:1667  */
    break;

  case 117:
#line 568 "sql.y" /* yacc.c:1667  */
    {
    (yyval.limit).offset_val = (yyvsp[-2].int_val);
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
}
#line 2750 "sql.cc" /* yacc.c:1667  */
    break;

  case 118:
#line 572 "sql.y" /* yacc.c:1667  */
    {
    (yyval.limit).limit_val  = (yyvsp[-2].int_val);
    (yyval.limit).offset_val = (yyvsp[0].int_val);
}
#line 2759 "sql.cc" /* yacc.c:1667  */
    break;

  case 119:
#line 576 "sql.y" /* yacc.c:1667  */
    {
    (yyval.limit).limit_val  = 0;
    (yyval.limit).offset_val = 0;
}
#line 2768 "sql.cc" /* yacc.c:1667  */
    break;

  case 120:
#line 582 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = true;
}
#line 2776 "sql.cc" /* yacc.c:1667  */
    break;

  case 121:
#line 585 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 2784 "sql.cc" /* yacc.c:1667  */
    break;

  case 122:
#line 589 "sql.y" /* yacc.c:1667  */
    {
    Insert *stmt = ctx->factory->NewInsert((yyvsp[-6].bool_val), (yyvsp[-4].id));
    stmt->set_col_names((yyvsp[-3].name_list));
    stmt->set_row_values_list((yyvsp[-1].row_vals_list));
    stmt->set_on_duplicate_clause((yyvsp[0].assignment_list));
    (yyval.stmt) = stmt;
}
#line 2796 "sql.cc" /* yacc.c:1667  */
    break;

  case 123:
#line 596 "sql.y" /* yacc.c:1667  */
    {
    Insert *stmt = ctx->factory->NewInsert((yyvsp[-5].bool_val), (yyvsp[-3].id));
    stmt->SetAssignmentList((yyvsp[-1].assignment_list), ctx->factory->arena());
    stmt->set_on_duplicate_clause((yyvsp[0].assignment_list));
    (yyval.stmt) = stmt;
}
#line 2807 "sql.cc" /* yacc.c:1667  */
    break;

  case 124:
#line 602 "sql.y" /* yacc.c:1667  */
    {
    Insert *stmt = ctx->factory->NewInsert((yyvsp[-5].bool_val), (yyvsp[-3].id));
    stmt->set_col_names((yyvsp[-2].name_list));
    stmt->set_select_clause(::mai::down_cast<Query>((yyvsp[-1].stmt)));
    stmt->set_on_duplicate_clause((yyvsp[0].assignment_list));
    (yyval.stmt) = stmt;
}
#line 2819 "sql.cc" /* yacc.c:1667  */
    break;

  case 126:
#line 611 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name_list) = nullptr;
}
#line 2827 "sql.cc" /* yacc.c:1667  */
    break;

  case 127:
#line 615 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = true;
}
#line 2835 "sql.cc" /* yacc.c:1667  */
    break;

  case 128:
#line 618 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 2843 "sql.cc" /* yacc.c:1667  */
    break;

  case 131:
#line 625 "sql.y" /* yacc.c:1667  */
    {
    (yyval.row_vals_list) = ctx->factory->NewRowValuesList((yyvsp[0].expr_list));
}
#line 2851 "sql.cc" /* yacc.c:1667  */
    break;

  case 132:
#line 628 "sql.y" /* yacc.c:1667  */
    {
    (yyval.row_vals_list)->push_back((yyvsp[0].expr_list));
}
#line 2859 "sql.cc" /* yacc.c:1667  */
    break;

  case 133:
#line 632 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr_list) = (yyvsp[-1].expr_list);
}
#line 2867 "sql.cc" /* yacc.c:1667  */
    break;

  case 134:
#line 636 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr_list) = ctx->factory->NewExpressionList((yyvsp[0].expr));
}
#line 2875 "sql.cc" /* yacc.c:1667  */
    break;

  case 135:
#line 639 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr_list)->push_back((yyvsp[0].expr));
}
#line 2883 "sql.cc" /* yacc.c:1667  */
    break;

  case 137:
#line 644 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewDefaultPlaceholderLiteral((yylsp[0]));
}
#line 2891 "sql.cc" /* yacc.c:1667  */
    break;

  case 138:
#line 648 "sql.y" /* yacc.c:1667  */
    {
    (yyval.assignment_list) = (yyvsp[0].assignment_list);
}
#line 2899 "sql.cc" /* yacc.c:1667  */
    break;

  case 139:
#line 651 "sql.y" /* yacc.c:1667  */
    {
    (yyval.assignment_list) = nullptr;
}
#line 2907 "sql.cc" /* yacc.c:1667  */
    break;

  case 140:
#line 655 "sql.y" /* yacc.c:1667  */
    {
    (yyval.assignment_list) = ctx->factory->NewAssignmentList((yyvsp[0].assignment));
}
#line 2915 "sql.cc" /* yacc.c:1667  */
    break;

  case 141:
#line 658 "sql.y" /* yacc.c:1667  */
    {
    (yyval.assignment_list)->push_back((yyvsp[0].assignment));
}
#line 2923 "sql.cc" /* yacc.c:1667  */
    break;

  case 142:
#line 662 "sql.y" /* yacc.c:1667  */
    {
    if ((yyvsp[-1].op) != SQL_CMP_EQ) {
        yyerror(&(yylsp[-2]), ctx, "incorrect assignment.");
        YYERROR;
    }
    (yyval.assignment) = ctx->factory->NewAssignment((yyvsp[-2].name), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2935 "sql.cc" /* yacc.c:1667  */
    break;

  case 143:
#line 671 "sql.y" /* yacc.c:1667  */
    {
    Update *stmt = ctx->factory->NewUpdate((yyvsp[-5].id), (yyvsp[-3].assignment_list));
    stmt->set_where_clause((yyvsp[-2].expr));
    stmt->set_order_by_desc((yyvsp[-1].order_by).desc);
    stmt->set_order_by_clause((yyvsp[-1].order_by).expr_list);
    stmt->set_limit_val((yyvsp[0].limit).limit_val);
    (yyval.stmt) = stmt;
}
#line 2948 "sql.cc" /* yacc.c:1667  */
    break;

  case 144:
#line 680 "sql.y" /* yacc.c:1667  */
    {
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
    (yyval.limit).offset_val = 0;
}
#line 2957 "sql.cc" /* yacc.c:1667  */
    break;

  case 145:
#line 684 "sql.y" /* yacc.c:1667  */
    {
    (yyval.limit).limit_val  = 0;
    (yyval.limit).offset_val = 0;
}
#line 2966 "sql.cc" /* yacc.c:1667  */
    break;

  case 146:
#line 694 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr_list) = ctx->factory->NewExpressionList((yyvsp[0].expr));
}
#line 2974 "sql.cc" /* yacc.c:1667  */
    break;

  case 147:
#line 697 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr_list)->push_back((yyvsp[0].expr));
}
#line 2982 "sql.cc" /* yacc.c:1667  */
    break;

  case 148:
#line 701 "sql.y" /* yacc.c:1667  */
    {
    Placeholder *ph = ctx->factory->NewStarPlaceholder((yylsp[0]));
    (yyval.expr_list) = ctx->factory->NewExpressionList(ph);
}
#line 2991 "sql.cc" /* yacc.c:1667  */
    break;

  case 150:
#line 710 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_OR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2999 "sql.cc" /* yacc.c:1667  */
    break;

  case 151:
#line 713 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_XOR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3007 "sql.cc" /* yacc.c:1667  */
    break;

  case 152:
#line 716 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_AND, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3015 "sql.cc" /* yacc.c:1667  */
    break;

  case 153:
#line 719 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_NOT, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 3023 "sql.cc" /* yacc.c:1667  */
    break;

  case 154:
#line 722 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_NOT, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 3031 "sql.cc" /* yacc.c:1667  */
    break;

  case 156:
#line 730 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_IS_NULL, (yyvsp[-2].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3039 "sql.cc" /* yacc.c:1667  */
    break;

  case 157:
#line 733 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_IS_NOT_NULL, (yyvsp[-3].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3047 "sql.cc" /* yacc.c:1667  */
    break;

  case 158:
#line 736 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-2].expr), (yyvsp[-1].op), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3055 "sql.cc" /* yacc.c:1667  */
    break;

  case 159:
#line 739 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-3].expr), (yyvsp[-2].op), (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3063 "sql.cc" /* yacc.c:1667  */
    break;

  case 160:
#line 742 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-3].expr), (yyvsp[-2].op), (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3071 "sql.cc" /* yacc.c:1667  */
    break;

  case 162:
#line 751 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-3].expr), SQL_NOT_IN, (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3079 "sql.cc" /* yacc.c:1667  */
    break;

  case 163:
#line 754 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-2].expr), SQL_IN, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3087 "sql.cc" /* yacc.c:1667  */
    break;

  case 164:
#line 757 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-5].expr), SQL_NOT_IN, (yyvsp[-1].expr_list), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 3095 "sql.cc" /* yacc.c:1667  */
    break;

  case 165:
#line 760 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-4].expr), SQL_IN, (yyvsp[-1].expr_list), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 3103 "sql.cc" /* yacc.c:1667  */
    break;

  case 166:
#line 763 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-5].expr), SQL_NOT_BETWEEN, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 3111 "sql.cc" /* yacc.c:1667  */
    break;

  case 167:
#line 766 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-4].expr), SQL_BETWEEN, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 3119 "sql.cc" /* yacc.c:1667  */
    break;

  case 168:
#line 769 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-3].expr), SQL_NOT_LIKE, (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3127 "sql.cc" /* yacc.c:1667  */
    break;

  case 169:
#line 772 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_LIKE, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3135 "sql.cc" /* yacc.c:1667  */
    break;

  case 171:
#line 781 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_OR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3143 "sql.cc" /* yacc.c:1667  */
    break;

  case 172:
#line 784 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_AND, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3151 "sql.cc" /* yacc.c:1667  */
    break;

  case 173:
#line 787 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_LSHIFT, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3159 "sql.cc" /* yacc.c:1667  */
    break;

  case 174:
#line 790 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_RSHIFT, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3167 "sql.cc" /* yacc.c:1667  */
    break;

  case 175:
#line 793 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_PLUS, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3175 "sql.cc" /* yacc.c:1667  */
    break;

  case 176:
#line 796 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_SUB, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3183 "sql.cc" /* yacc.c:1667  */
    break;

  case 177:
#line 799 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MUL, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3191 "sql.cc" /* yacc.c:1667  */
    break;

  case 178:
#line 802 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_DIV, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3199 "sql.cc" /* yacc.c:1667  */
    break;

  case 179:
#line 805 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_DIV, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3207 "sql.cc" /* yacc.c:1667  */
    break;

  case 180:
#line 808 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MOD, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3215 "sql.cc" /* yacc.c:1667  */
    break;

  case 181:
#line 811 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MOD, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3223 "sql.cc" /* yacc.c:1667  */
    break;

  case 182:
#line 814 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_XOR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3231 "sql.cc" /* yacc.c:1667  */
    break;

  case 184:
#line 823 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[0].id);
}
#line 3239 "sql.cc" /* yacc.c:1667  */
    break;

  case 185:
#line 826 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewStringLiteral((yyvsp[0].text).buf, (yyvsp[0].text).len, (yylsp[0]));
}
#line 3247 "sql.cc" /* yacc.c:1667  */
    break;

  case 186:
#line 829 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewIntegerLiteral((yyvsp[0].i64_val), (yylsp[0]));
}
#line 3255 "sql.cc" /* yacc.c:1667  */
    break;

  case 187:
#line 832 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewApproxLiteral((yyvsp[0].approx_val), (yylsp[0]));
}
#line 3263 "sql.cc" /* yacc.c:1667  */
    break;

  case 188:
#line 835 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewDecimalLiteral((yyvsp[0].dec_val), (yylsp[0]));
}
#line 3271 "sql.cc" /* yacc.c:1667  */
    break;

  case 189:
#line 838 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewDateLiteral((yyvsp[-1].dt_val).date, (yylsp[-1]));
}
#line 3279 "sql.cc" /* yacc.c:1667  */
    break;

  case 190:
#line 841 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewTimeLiteral((yyvsp[-1].dt_val).time, (yylsp[-1]));
}
#line 3287 "sql.cc" /* yacc.c:1667  */
    break;

  case 191:
#line 844 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewDateTimeLiteral((yyvsp[-1].dt_val), (yylsp[-1]));
}
#line 3295 "sql.cc" /* yacc.c:1667  */
    break;

  case 192:
#line 847 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewCall((yyvsp[-4].name), (yyvsp[-2].bool_val), (yyvsp[-1].expr_list), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 3303 "sql.cc" /* yacc.c:1667  */
    break;

  case 193:
#line 850 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBindPlaceholder((yylsp[0]));
}
#line 3311 "sql.cc" /* yacc.c:1667  */
    break;

  case 194:
#line 853 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_MINUS, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 3319 "sql.cc" /* yacc.c:1667  */
    break;

  case 195:
#line 856 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_BIT_INV, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 3327 "sql.cc" /* yacc.c:1667  */
    break;

  case 196:
#line 859 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 3335 "sql.cc" /* yacc.c:1667  */
    break;

  case 197:
#line 864 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewSubquery(true, ::mai::down_cast<Query>((yyvsp[-1].stmt)), (yylsp[-1]));
}
#line 3343 "sql.cc" /* yacc.c:1667  */
    break;

  case 198:
#line 868 "sql.y" /* yacc.c:1667  */
    {
    (yyval.id) = ctx->factory->NewIdentifier(AstString::kEmpty, (yyvsp[0].name), (yylsp[0]));
}
#line 3351 "sql.cc" /* yacc.c:1667  */
    break;

  case 199:
#line 871 "sql.y" /* yacc.c:1667  */
    {
    (yyval.id) = ctx->factory->NewIdentifier((yyvsp[-2].name), (yyvsp[0].name), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3359 "sql.cc" /* yacc.c:1667  */
    break;

  case 200:
#line 875 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name) = ctx->factory->NewString((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 3367 "sql.cc" /* yacc.c:1667  */
    break;

  case 201:
#line 879 "sql.y" /* yacc.c:1667  */
    {
    if ((yyvsp[0].i64_val) > INT32_MAX || (yyvsp[0].i64_val) < INT32_MIN) {
        yyerror(&(yylsp[0]), ctx, "int val out of range.");
        YYERROR;
    }
    (yyval.int_val) = static_cast<int>((yyvsp[0].i64_val));
}
#line 3379 "sql.cc" /* yacc.c:1667  */
    break;


#line 3383 "sql.cc" /* yacc.c:1667  */
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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

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


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
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
#line 887 "sql.y" /* yacc.c:1918  */

void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg) {
    ctx->err_line   = yyl->first_line;
    ctx->err_column = yyl->first_column;
    ctx->err_msg    = ctx->factory->NewString(msg);
}
