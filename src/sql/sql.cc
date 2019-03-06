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
    DATETIME_VAL = 323,
    DECIMAL_VAL = 324,
    EQ = 325,
    NOT = 326,
    OP_AND = 327,
    BIGINT = 328,
    INT = 329,
    SMALLINT = 330,
    TINYINT = 331,
    DECIMAL = 332,
    NUMERIC = 333,
    CHAR = 334,
    VARCHAR = 335,
    DATE = 336,
    DATETIME = 337,
    TIMESTMAP = 338,
    AUTO_INCREMENT = 339,
    COMMENT = 340,
    TOKEN_ERROR = 341,
    ASSIGN = 342,
    OP_OR = 343,
    XOR = 344,
    IS = 345,
    LIKE = 346,
    REGEXP = 347,
    BETWEEN = 348,
    COMPARISON = 349,
    LSHIFT = 350,
    RSHIFT = 351,
    MOD = 352,
    UMINUS = 353
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

#line 283 "sql.cc" /* yacc.c:352  */
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
#define YYLAST   549

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  115
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  62
/* YYNRULES -- Number of rules.  */
#define YYNRULES  198
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  386

#define YYUNDEFTOK  2
#define YYMAXUTOK   353

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
       2,     2,     2,    93,     2,     2,     2,   104,    97,     2,
     109,   110,   102,   100,   111,   101,   112,   103,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   108,
       2,     2,     2,   113,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   106,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    96,     2,   114,     2,     2,     2,
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
      85,    86,    87,    88,    89,    90,    91,    92,    94,    95,
      98,    99,   105,   107
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   149,   149,   154,   159,   161,   162,   163,   166,   169,
     172,   177,   180,   183,   187,   191,   194,   198,   208,   211,
     214,   217,   220,   223,   226,   229,   232,   235,   238,   241,
     245,   249,   254,   258,   263,   266,   270,   273,   277,   280,
     283,   287,   290,   293,   296,   299,   302,   306,   309,   313,
     317,   320,   324,   327,   330,   333,   336,   340,   344,   347,
     350,   353,   356,   359,   362,   365,   368,   371,   374,   377,
     380,   384,   388,   392,   397,   400,   407,   408,   409,   411,
     425,   428,   431,   435,   438,   442,   446,   453,   459,   460,
     464,   467,   471,   474,   478,   483,   489,   494,   498,   501,
     505,   508,   511,   514,   517,   520,   524,   527,   531,   534,
     538,   542,   546,   550,   555,   558,   562,   566,   570,   574,
     580,   583,   587,   594,   600,   608,   609,   613,   616,   620,
     621,   623,   626,   630,   634,   637,   641,   642,   646,   649,
     653,   656,   660,   669,   678,   682,   692,   695,   699,   703,
     708,   711,   714,   717,   720,   723,   728,   731,   734,   737,
     740,   743,   749,   752,   755,   758,   761,   764,   767,   770,
     773,   779,   782,   785,   788,   791,   794,   797,   800,   803,
     806,   809,   812,   815,   821,   824,   827,   830,   833,   836,
     839,   842,   845,   848,   853,   857,   860,   864,   868
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
  "APPROX_VAL", "DATE_VAL", "DATETIME_VAL", "DECIMAL_VAL", "EQ", "NOT",
  "OP_AND", "BIGINT", "INT", "SMALLINT", "TINYINT", "DECIMAL", "NUMERIC",
  "CHAR", "VARCHAR", "DATE", "DATETIME", "TIMESTMAP", "AUTO_INCREMENT",
  "COMMENT", "TOKEN_ERROR", "ASSIGN", "OP_OR", "XOR", "IS", "LIKE",
  "REGEXP", "'!'", "BETWEEN", "COMPARISON", "'|'", "'&'", "LSHIFT",
  "RSHIFT", "'+'", "'-'", "'*'", "'/'", "'%'", "MOD", "'^'", "UMINUS",
  "';'", "'('", "')'", "','", "'.'", "'?'", "'~'", "$accept", "Block",
  "Statement", "Command", "DDL", "CreateTableStmt", "ColumnDefinitionList",
  "ColumnDefinition", "TypeDefinition", "FixedSizeDescription",
  "FloatingSizeDescription", "DefaultOption", "AutoIncrementOption",
  "NullOption", "KeyOption", "CommentOption", "AlterTableStmt",
  "AlterTableSpecList", "AlterTableSpec", "AlterColPosOption", "NameList",
  "DML", "SelectStmt", "DistinctOption", "ProjectionColumnList",
  "ProjectionColumn", "AliasOption", "Alias", "FromClause", "Relation",
  "OnClause", "JoinOp", "WhereClause", "HavingClause", "OrderByClause",
  "GroupByClause", "LimitOffsetClause", "ForUpdateOption", "InsertStmt",
  "NameListOption", "OverwriteOption", "ValueToken", "RowValuesList",
  "RowValues", "ValueList", "Value", "OnDuplicateClause", "AssignmentList",
  "Assignment", "UpdateStmt", "UpdateLimitOption", "ExpressionList",
  "Parameters", "Expression", "BoolPrimary", "Predicate", "BitExpression",
  "Simple", "Subquery", "Identifier", "Name", "IntVal", YY_NULLPTR
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
     345,   346,   347,    33,   348,   349,   124,    38,   350,   351,
      43,    45,    42,    47,    37,   352,    94,   353,    59,    40,
      41,    44,    46,    63,   126
};
# endif

#define YYPACT_NINF -261

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-261)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     495,   147,    52,    70,   100,    76,   116,  -261,  -261,   105,
     143,   466,  -261,    96,  -261,  -261,  -261,  -261,  -261,  -261,
    -261,  -261,  -261,   313,   105,   105,  -261,   105,  -261,  -261,
     179,   111,  -261,   188,  -261,  -261,  -261,  -261,  -261,  -261,
    -261,   369,   369,   379,  -261,   369,  -261,   379,     7,  -261,
     155,    47,  -261,   168,  -261,  -261,    80,   149,  -261,   185,
     105,   105,   105,   183,   121,   183,  -261,    75,  -261,    10,
     313,   228,   105,   369,   369,   369,   122,  -261,  -261,  -261,
     -16,   379,   379,   206,   -29,   379,   379,   379,   379,   379,
     379,   379,   379,   379,   379,   379,   379,   379,   147,    -5,
     105,   112,    11,   178,   113,   234,  -261,   -12,  -261,   222,
    -261,   163,  -261,   340,    18,    18,  -261,   369,   315,  -261,
     183,   137,    71,   267,   267,  -261,   320,  -261,   223,   107,
    -261,   277,   379,   379,   223,   189,   249,   232,   268,   268,
      20,    20,    -2,    -2,    -2,    -2,   386,   323,  -261,   102,
    -261,   447,   376,   105,   105,   105,  -261,   105,    21,   105,
     105,   213,   105,   105,   105,   105,   105,  -261,   105,   105,
     185,   105,   315,   298,   105,   290,    34,  -261,   292,     5,
    -261,   -18,   361,   360,   340,  -261,  -261,  -261,   299,   118,
     -18,   107,  -261,   223,   205,   379,  -261,   297,   300,  -261,
     105,   304,   304,   304,   304,   304,   304,   308,   308,   304,
     304,  -261,  -261,    69,  -261,  -261,  -261,  -261,   281,   105,
     213,   281,   144,   105,   105,  -261,   391,   393,  -261,  -261,
     394,   105,   213,  -261,  -261,   375,  -261,  -261,  -261,   -11,
     105,  -261,  -261,   395,   319,    18,  -261,   216,   220,   397,
      10,    10,   369,   400,   408,  -261,  -261,   369,   165,   379,
    -261,  -261,  -261,   383,  -261,  -261,  -261,  -261,  -261,  -261,
     383,  -261,  -261,  -261,  -261,  -261,   401,   381,   446,   450,
    -261,   359,   173,  -261,   363,  -261,  -261,  -261,   105,   105,
     105,   213,  -261,   383,  -261,   415,  -261,  -261,  -261,   298,
      -8,  -261,  -261,  -261,   444,  -261,   448,  -261,    18,    18,
     -13,   369,   370,   436,   -18,  -261,  -261,  -261,   377,   374,
    -261,  -261,   432,  -261,  -261,   105,  -261,   105,  -261,  -261,
    -261,  -261,  -261,   477,   202,  -261,   319,  -261,  -261,  -261,
      41,    41,  -261,  -261,   297,   369,   383,   449,  -261,   383,
     369,   281,   230,   280,   452,  -261,   298,  -261,   392,  -261,
    -261,   270,     1,   455,  -261,   396,   -18,   424,  -261,  -261,
     105,  -261,   369,  -261,   383,   383,  -261,  -261,   445,  -261,
     404,   309,  -261,  -261,  -261,  -261
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    82,     0,     0,     0,     0,     0,     8,     9,     0,
     128,     0,     2,     0,     5,    11,    13,     6,    76,    77,
      78,    80,    81,     0,     0,     0,    10,     0,     7,   197,
       0,   195,   127,     0,     1,     3,     4,   186,   185,   187,
     188,     0,     0,     0,    87,     0,   190,     0,    93,    83,
      89,   155,   161,   170,   183,   184,   195,     0,    12,     0,
       0,     0,     0,   153,   195,   154,   191,     0,   192,     0,
       0,   107,     0,     0,     0,     0,     0,    85,    88,    91,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    82,     0,
       0,     0,     0,     0,     0,    49,    50,   107,   140,     0,
     196,   126,   193,     0,    92,    89,    84,     0,   113,    90,
     152,   150,   151,     0,     0,   156,     0,   158,   179,     0,
     163,     0,     0,     0,   169,     0,   171,   172,   173,   174,
     175,   176,   177,   178,   180,   181,   182,     0,    86,     0,
      15,     0,     0,     0,     0,     0,    67,     0,     0,     0,
       0,    73,     0,     0,     0,     0,     0,    63,     0,     0,
       0,     0,   113,     0,     0,   125,     0,    74,     0,     0,
      97,   106,     0,   115,     0,   160,   159,   157,     0,     0,
     146,     0,   162,   168,     0,     0,   148,   149,     0,    14,
       0,    31,    31,    31,    31,    31,    31,    33,    33,    31,
      31,    28,    29,    40,    70,    69,    66,    68,    46,     0,
      73,    46,     0,     0,     0,    52,     0,     0,    64,    65,
       0,     0,    73,    51,   141,   145,   137,   142,   136,   139,
       0,   130,   129,   139,     0,     0,   100,     0,     0,     0,
       0,     0,     0,     0,   109,   194,   165,     0,     0,     0,
     167,   189,    16,     0,    26,    27,    18,    19,    20,    21,
       0,    22,    23,    24,    25,    39,     0,    37,    42,    44,
      41,     0,     0,    53,     0,    54,    72,    71,     0,     0,
       0,    73,    58,     0,   143,     0,   123,    75,   124,     0,
     139,   131,    94,   102,     0,   104,     0,   101,     0,     0,
     112,     0,     0,   119,   147,   164,   166,   198,     0,     0,
      38,    36,    35,    43,    45,     0,    55,     0,    62,    60,
      61,    59,   144,     0,     0,   134,     0,   122,   103,   105,
      99,    99,   110,   111,   114,     0,     0,   121,    30,     0,
       0,    46,     0,     0,     0,   133,     0,   132,     0,    95,
      96,     0,   116,     0,    79,     0,    34,    48,    57,    56,
       0,   135,     0,   108,     0,     0,   120,    32,     0,    17,
     138,     0,   118,   117,    47,    98
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -261,  -261,   500,  -261,  -261,  -261,  -111,   -73,  -261,   328,
     327,  -261,  -261,  -261,  -212,  -261,  -261,  -261,   342,  -207,
    -137,  -261,     8,   419,  -261,   469,   421,  -107,  -261,   145,
     199,  -261,   434,  -261,   371,  -261,  -261,  -261,  -261,  -261,
    -261,  -261,  -261,   208,  -261,  -255,  -187,  -172,   378,  -261,
    -261,  -143,  -261,    -6,  -261,   -78,   364,   -42,    56,    -3,
      -9,  -260
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    11,    12,    13,    14,    15,   149,   150,   213,   264,
     271,   351,   322,   277,   281,   379,    16,   105,   106,   225,
     175,    17,   188,    23,    48,    49,    77,    78,    71,   114,
     359,   251,   118,   313,   183,   254,   347,   364,    19,   176,
      33,   244,   300,   301,   334,   237,   296,   107,   108,    20,
     294,   189,   198,   190,    51,    52,    53,    54,   130,    55,
      64,   318
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint16 yytable[] =
{
      31,    66,   239,   127,   197,    68,    30,   179,    18,   284,
     319,    69,    82,   283,    56,    31,    31,    50,    31,    18,
     117,    57,    58,   295,    59,   292,   295,   131,   157,   161,
     342,   343,   158,   332,    82,    63,    65,     1,   246,    67,
     159,    79,   247,   248,   335,   249,    72,   125,   258,   222,
     374,   109,   110,    31,    73,   126,   298,    29,    24,   111,
      31,    56,   132,   119,    50,   133,   115,   120,   121,   122,
      74,    75,    29,    29,   246,   358,    25,    76,   247,   248,
      29,   249,    27,    29,   331,   220,   362,   241,   242,   365,
     110,   151,   156,   151,   167,   169,   232,   148,   257,   171,
     171,   371,   177,   336,    97,    79,    79,    26,   282,   310,
       1,   181,   375,   337,   382,   383,   250,   260,    70,   113,
     160,   178,    93,    94,    95,    96,    97,   262,   152,   153,
     219,   238,   275,   154,   168,   123,    28,    80,   302,   367,
     276,   155,    81,    73,   215,   216,   217,    73,   218,   151,
     221,   151,   250,   226,   227,   228,   229,   230,   291,   231,
     151,   124,   109,    74,    75,   109,    76,    29,   344,    29,
      76,    37,    38,    39,    29,    29,    40,    21,    41,   185,
     186,   316,    82,    72,   243,   112,    22,   192,   352,    98,
     353,   151,    99,   101,    32,   162,   102,   103,   380,   163,
      42,   340,   341,    82,    36,   164,   165,   166,    43,    73,
     151,   104,   199,   200,   286,   287,    45,    29,   174,    82,
      46,    47,   151,    61,    83,    29,    75,    73,   256,   257,
      98,   297,    76,    61,    60,   223,    79,    82,   224,    84,
      29,    31,    31,    74,    75,    62,    82,   115,   115,   303,
      76,   314,   304,   305,   285,   200,   306,   238,   100,    85,
     117,   195,    86,    82,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,   315,   257,   259,    76,   328,
     329,   330,    82,   326,   200,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,   278,   279,   280,    79,
      79,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,   355,   356,   238,   129,   177,   173,   177,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      89,    90,    91,    92,    93,    94,    95,    96,    97,   361,
     368,   240,    73,     1,   366,   170,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,   182,   236,    74,    75,
      29,   109,    37,    38,    39,    76,   381,    40,    91,    92,
      93,    94,    95,    96,    97,    29,   184,    37,    38,    39,
     373,    73,    40,   187,    41,    29,   191,    37,    38,    39,
     369,   240,    40,   214,    41,   308,   309,    74,    75,    43,
      82,   240,   245,   252,    76,   253,    42,    45,   257,   255,
     261,    46,    47,   263,    43,    44,    42,   270,   288,   385,
     289,   290,    45,   293,    43,   196,    46,    47,   299,   295,
     307,    29,    45,    37,    38,    39,    46,    47,    40,   312,
      41,    29,   311,    37,    38,    39,   128,   317,    40,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,    42,   323,   320,   321,    34,   324,   325,     1,
      43,     2,   327,   333,     3,     4,     5,   338,    45,   345,
      43,   339,    46,    47,   346,     6,   349,   348,    45,     7,
       8,   350,    46,    47,   354,   363,   193,   194,     1,   370,
       2,   372,   376,     3,     4,     5,   377,   201,   202,   378,
     384,    35,   233,     9,     6,   171,    10,   147,     7,     8,
     203,   204,   205,   206,   207,   208,   209,   210,   211,   212,
     265,   266,   267,   268,   269,   272,   180,   273,   274,   116,
     360,   172,     9,   235,   357,    10,     0,     0,     0,   234
};

static const yytype_int16 yycheck[] =
{
       9,    43,   174,    81,   147,    47,     9,   114,     0,   221,
     270,     4,    14,   220,    23,    24,    25,    23,    27,    11,
      32,    24,    25,    34,    27,   232,    34,    56,    17,   102,
      43,    44,    21,   293,    14,    41,    42,     3,    33,    45,
      29,    50,    37,    38,   299,    40,    28,    63,   191,   160,
      49,    60,    61,    62,    72,    71,   243,    62,     6,    62,
      69,    70,    91,    72,    70,    94,    69,    73,    74,    75,
      88,    89,    62,    62,    33,    34,     6,    95,    37,    38,
      62,    40,     6,    62,   291,   158,   346,    53,    54,   349,
      99,   100,   101,   102,   103,   104,   169,   102,   111,   111,
     111,   356,   111,   111,   106,   114,   115,     7,   219,   252,
       3,   117,   111,   300,   374,   375,   111,   195,   111,   109,
     109,   113,   102,   103,   104,   105,   106,   200,    16,    17,
     109,   173,    63,    21,    21,    13,    20,    90,   245,   351,
      71,    29,    95,    72,   153,   154,   155,    72,   157,   158,
     159,   160,   111,   162,   163,   164,   165,   166,   231,   168,
     169,    39,   171,    88,    89,   174,    95,    62,   311,    62,
      95,    64,    65,    66,    62,    62,    69,    30,    71,   123,
     124,   259,    14,    28,   176,   110,    39,   131,   325,   109,
     327,   200,   112,     8,    51,    17,    11,    12,   370,    21,
      93,   308,   309,    14,   108,    27,    28,    29,   101,    72,
     219,    26,   110,   111,   223,   224,   109,    62,    55,    14,
     113,   114,   231,   112,    56,    62,    89,    72,   110,   111,
     109,   240,    95,   112,    55,    22,   245,    14,    25,    71,
      62,   250,   251,    88,    89,    57,    14,   250,   251,    33,
      95,   257,    36,    33,   110,   111,    36,   299,   109,    91,
      32,    72,    94,    14,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   110,   111,    72,    95,   288,
     289,   290,    14,   110,   111,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,    15,    16,    17,   308,
     309,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   110,   111,   356,   109,   325,    95,   327,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   345,
     110,   111,    72,     3,   350,   111,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,    41,    59,    88,    89,
      62,   370,    64,    65,    66,    95,   372,    69,   100,   101,
     102,   103,   104,   105,   106,    62,   109,    64,    65,    66,
     110,    72,    69,    63,    71,    62,   109,    64,    65,    66,
     110,   111,    69,    17,    71,   250,   251,    88,    89,   101,
      14,   111,   110,    42,    95,    45,    93,   109,   111,   110,
     110,   113,   114,   109,   101,   102,    93,   109,    27,   110,
      27,    27,   109,    48,   101,   102,   113,   114,   109,    34,
      33,    62,   109,    64,    65,    66,   113,   114,    69,    31,
      71,    62,    42,    64,    65,    66,    82,    64,    69,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    93,    17,    63,    84,     0,    17,   109,     3,
     101,     5,   109,    58,     8,     9,    10,    33,   109,   109,
     101,    33,   113,   114,    48,    19,   112,   110,   109,    23,
      24,    59,   113,   114,    17,    46,   132,   133,     3,    47,
       5,   109,    47,     8,     9,    10,   110,    60,    61,    85,
      65,    11,   170,    47,    19,   111,    50,    98,    23,    24,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
     202,   203,   204,   205,   206,   208,   115,   209,   210,    70,
     341,   107,    47,   172,   336,    50,    -1,    -1,    -1,   171
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     5,     8,     9,    10,    19,    23,    24,    47,
      50,   116,   117,   118,   119,   120,   131,   136,   137,   153,
     164,    30,    39,   138,     6,     6,     7,     6,    20,    62,
     174,   175,    51,   155,     0,   117,   108,    64,    65,    66,
      69,    71,    93,   101,   102,   109,   113,   114,   139,   140,
     168,   169,   170,   171,   172,   174,   175,   174,   174,   174,
      55,   112,    57,   168,   175,   168,   172,   168,   172,     4,
     111,   143,    28,    72,    88,    89,    95,   141,   142,   175,
      90,    95,    14,    56,    71,    91,    94,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   109,   112,
     109,     8,    11,    12,    26,   132,   133,   162,   163,   175,
     175,   174,   110,   109,   144,   174,   140,    32,   147,   175,
     168,   168,   168,    13,    39,    63,    71,   170,   171,   109,
     173,    56,    91,    94,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   138,   102,   121,
     122,   175,    16,    17,    21,    29,   175,    17,    21,    29,
     109,   122,    17,    21,    27,    28,    29,   175,    21,   175,
     111,   111,   147,    95,    55,   135,   154,   175,   137,   142,
     141,   168,    41,   149,   109,   173,   173,    63,   137,   166,
     168,   109,   173,   171,   171,    72,   102,   166,   167,   110,
     111,    60,    61,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,   123,    17,   175,   175,   175,   175,   109,
     122,   175,   121,    22,    25,   134,   175,   175,   175,   175,
     175,   175,   122,   133,   163,   149,    59,   160,   172,   162,
     111,    53,    54,   137,   156,   110,    33,    37,    38,    40,
     111,   146,    42,    45,   150,   110,   110,   111,   166,    72,
     170,   110,   122,   109,   124,   124,   124,   124,   124,   124,
     109,   125,   125,   124,   124,    63,    71,   128,    15,    16,
      17,   129,   121,   134,   129,   110,   175,   175,    27,    27,
      27,   122,   134,    48,   165,    34,   161,   175,   161,   109,
     157,   158,   142,    33,    36,    33,    36,    33,   144,   144,
     166,    42,    31,   148,   168,   110,   170,    64,   176,   176,
      63,    84,   127,    17,    17,   109,   110,   109,   175,   175,
     175,   134,   176,    58,   159,   160,   111,   161,    33,    33,
     142,   142,    43,    44,   166,   109,    48,   151,   110,   112,
      59,   126,   135,   135,    17,   110,   111,   158,    34,   145,
     145,   168,   176,    46,   152,   176,   168,   129,   110,   110,
      47,   160,   109,   110,    49,   111,    47,   110,    85,   130,
     162,   168,   176,   176,    65,   110
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   115,   116,   116,   117,   118,   118,   118,   118,   118,
     118,   119,   119,   119,   120,   121,   121,   122,   123,   123,
     123,   123,   123,   123,   123,   123,   123,   123,   123,   123,
     124,   124,   125,   125,   126,   126,   127,   127,   128,   128,
     128,   129,   129,   129,   129,   129,   129,   130,   130,   131,
     132,   132,   133,   133,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   133,   133,   133,   133,   133,   133,   133,
     133,   134,   134,   134,   135,   135,   136,   136,   136,   137,
     138,   138,   138,   139,   139,   140,   140,   140,   141,   141,
     142,   142,   143,   143,   144,   144,   144,   144,   145,   145,
     146,   146,   146,   146,   146,   146,   147,   147,   148,   148,
     149,   149,   149,   149,   150,   150,   151,   151,   151,   151,
     152,   152,   153,   153,   153,   154,   154,   155,   155,   156,
     156,   157,   157,   158,   159,   159,   160,   160,   161,   161,
     162,   162,   163,   164,   165,   165,   166,   166,   167,   167,
     168,   168,   168,   168,   168,   168,   169,   169,   169,   169,
     169,   169,   170,   170,   170,   170,   170,   170,   170,   170,
     170,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   172,   172,   172,   172,   172,   172,
     172,   172,   172,   172,   173,   174,   174,   175,   176
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
       3,     3,     3,     1,     1,     1,     1,     1,     1,     5,
       1,     2,     2,     3,     3,     1,     3,     1,     1
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
#line 149 "sql.y" /* yacc.c:1667  */
    {
    (yyval.block) = ctx->factory->NewBlock();
    (yyval.block)->AddStmt((yyvsp[0].stmt));
    ctx->block = (yyval.block);
}
#line 1824 "sql.cc" /* yacc.c:1667  */
    break;

  case 3:
#line 154 "sql.y" /* yacc.c:1667  */
    {
    (yyval.block)->AddStmt((yyvsp[0].stmt));
    ctx->block = (yyval.block);
}
#line 1833 "sql.cc" /* yacc.c:1667  */
    break;

  case 7:
#line 163 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(ast::TCLStatement::TXN_BEGIN);
}
#line 1841 "sql.cc" /* yacc.c:1667  */
    break;

  case 8:
#line 166 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(ast::TCLStatement::TXN_COMMIT);
}
#line 1849 "sql.cc" /* yacc.c:1667  */
    break;

  case 9:
#line 169 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(ast::TCLStatement::TXN_ROLLBACK);
}
#line 1857 "sql.cc" /* yacc.c:1667  */
    break;

  case 10:
#line 172 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewShowTables();
}
#line 1865 "sql.cc" /* yacc.c:1667  */
    break;

  case 11:
#line 177 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1873 "sql.cc" /* yacc.c:1667  */
    break;

  case 12:
#line 180 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewDropTable((yyvsp[0].id));
}
#line 1881 "sql.cc" /* yacc.c:1667  */
    break;

  case 13:
#line 183 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1889 "sql.cc" /* yacc.c:1667  */
    break;

  case 14:
#line 187 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewCreateTable((yyvsp[-3].id), (yyvsp[-1].col_def_list));
}
#line 1897 "sql.cc" /* yacc.c:1667  */
    break;

  case 15:
#line 191 "sql.y" /* yacc.c:1667  */
    {
    (yyval.col_def_list) = ctx->factory->NewColumnDefinitionList((yyvsp[0].col_def));
}
#line 1905 "sql.cc" /* yacc.c:1667  */
    break;

  case 16:
#line 194 "sql.y" /* yacc.c:1667  */
    {
    (yyval.col_def_list)->push_back((yyvsp[0].col_def));
}
#line 1913 "sql.cc" /* yacc.c:1667  */
    break;

  case 17:
#line 198 "sql.y" /* yacc.c:1667  */
    {
    auto *def = ctx->factory->NewColumnDefinition((yyvsp[-6].name), (yyvsp[-5].type_def));
    def->set_is_not_null((yyvsp[-4].bool_val));
    def->set_auto_increment((yyvsp[-3].bool_val));
    def->set_default_value((yyvsp[-2].expr));
    def->set_key((yyvsp[-1].key_type));
    def->set_comment((yyvsp[0].name));
    (yyval.col_def) = def;
}
#line 1927 "sql.cc" /* yacc.c:1667  */
    break;

  case 18:
#line 208 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_BIGINT, (yyvsp[0].size).fixed_size);
}
#line 1935 "sql.cc" /* yacc.c:1667  */
    break;

  case 19:
#line 211 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_INT, (yyvsp[0].size).fixed_size);
}
#line 1943 "sql.cc" /* yacc.c:1667  */
    break;

  case 20:
#line 214 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_SMALLINT, (yyvsp[0].size).fixed_size);
}
#line 1951 "sql.cc" /* yacc.c:1667  */
    break;

  case 21:
#line 217 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_TINYINT, (yyvsp[0].size).fixed_size);
}
#line 1959 "sql.cc" /* yacc.c:1667  */
    break;

  case 22:
#line 220 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DECIMAL, (yyvsp[0].size).fixed_size, (yyvsp[0].size).float_size);
}
#line 1967 "sql.cc" /* yacc.c:1667  */
    break;

  case 23:
#line 223 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_NUMERIC, (yyvsp[0].size).fixed_size, (yyvsp[0].size).float_size);
}
#line 1975 "sql.cc" /* yacc.c:1667  */
    break;

  case 24:
#line 226 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_NUMERIC, (yyvsp[0].size).fixed_size);
}
#line 1983 "sql.cc" /* yacc.c:1667  */
    break;

  case 25:
#line 229 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_VARCHAR, (yyvsp[0].size).fixed_size);
}
#line 1991 "sql.cc" /* yacc.c:1667  */
    break;

  case 26:
#line 232 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_BINARY, (yyvsp[0].size).fixed_size);
}
#line 1999 "sql.cc" /* yacc.c:1667  */
    break;

  case 27:
#line 235 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_VARBINARY, (yyvsp[0].size).fixed_size);
}
#line 2007 "sql.cc" /* yacc.c:1667  */
    break;

  case 28:
#line 238 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DATE);
}
#line 2015 "sql.cc" /* yacc.c:1667  */
    break;

  case 29:
#line 241 "sql.y" /* yacc.c:1667  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DATETIME);
}
#line 2023 "sql.cc" /* yacc.c:1667  */
    break;

  case 30:
#line 245 "sql.y" /* yacc.c:1667  */
    {
    (yyval.size).fixed_size = (yyvsp[-1].int_val);
    (yyval.size).float_size = 0;
}
#line 2032 "sql.cc" /* yacc.c:1667  */
    break;

  case 31:
#line 249 "sql.y" /* yacc.c:1667  */
    {
    (yyval.size).fixed_size = 0;
    (yyval.size).float_size = 0;
}
#line 2041 "sql.cc" /* yacc.c:1667  */
    break;

  case 32:
#line 254 "sql.y" /* yacc.c:1667  */
    {
    (yyval.size).fixed_size = (yyvsp[-3].int_val);
    (yyval.size).float_size = (yyvsp[-1].int_val);
}
#line 2050 "sql.cc" /* yacc.c:1667  */
    break;

  case 33:
#line 258 "sql.y" /* yacc.c:1667  */
    {
    (yyval.size).fixed_size = 0;
    (yyval.size).float_size = 0;
}
#line 2059 "sql.cc" /* yacc.c:1667  */
    break;

  case 34:
#line 263 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2067 "sql.cc" /* yacc.c:1667  */
    break;

  case 35:
#line 266 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = nullptr;
}
#line 2075 "sql.cc" /* yacc.c:1667  */
    break;

  case 36:
#line 270 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = true;
}
#line 2083 "sql.cc" /* yacc.c:1667  */
    break;

  case 37:
#line 273 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 2091 "sql.cc" /* yacc.c:1667  */
    break;

  case 38:
#line 277 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = true;
}
#line 2099 "sql.cc" /* yacc.c:1667  */
    break;

  case 39:
#line 280 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 2107 "sql.cc" /* yacc.c:1667  */
    break;

  case 40:
#line 283 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 2115 "sql.cc" /* yacc.c:1667  */
    break;

  case 41:
#line 287 "sql.y" /* yacc.c:1667  */
    {
    (yyval.key_type) = SQL_KEY;
}
#line 2123 "sql.cc" /* yacc.c:1667  */
    break;

  case 42:
#line 290 "sql.y" /* yacc.c:1667  */
    {
    (yyval.key_type) = SQL_UNIQUE_KEY;
}
#line 2131 "sql.cc" /* yacc.c:1667  */
    break;

  case 43:
#line 293 "sql.y" /* yacc.c:1667  */
    {
    (yyval.key_type) = SQL_UNIQUE_KEY;
}
#line 2139 "sql.cc" /* yacc.c:1667  */
    break;

  case 44:
#line 296 "sql.y" /* yacc.c:1667  */
    {
    (yyval.key_type) = SQL_PRIMARY_KEY;
}
#line 2147 "sql.cc" /* yacc.c:1667  */
    break;

  case 45:
#line 299 "sql.y" /* yacc.c:1667  */
    {
    (yyval.key_type) = SQL_PRIMARY_KEY;
}
#line 2155 "sql.cc" /* yacc.c:1667  */
    break;

  case 46:
#line 302 "sql.y" /* yacc.c:1667  */
    {
    (yyval.key_type) = SQL_NOT_KEY;
}
#line 2163 "sql.cc" /* yacc.c:1667  */
    break;

  case 47:
#line 306 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name) = ctx->factory->NewString((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 2171 "sql.cc" /* yacc.c:1667  */
    break;

  case 48:
#line 309 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name) = AstString::kEmpty;
}
#line 2179 "sql.cc" /* yacc.c:1667  */
    break;

  case 49:
#line 313 "sql.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewAlterTable((yyvsp[-1].id), (yyvsp[0].alter_table_spce_list));
}
#line 2187 "sql.cc" /* yacc.c:1667  */
    break;

  case 50:
#line 317 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce_list) = ctx->factory->NewAlterTableSpecList((yyvsp[0].alter_table_spce));
}
#line 2195 "sql.cc" /* yacc.c:1667  */
    break;

  case 51:
#line 320 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce_list)->push_back((yyvsp[0].alter_table_spce));
}
#line 2203 "sql.cc" /* yacc.c:1667  */
    break;

  case 52:
#line 324 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2211 "sql.cc" /* yacc.c:1667  */
    break;

  case 53:
#line 327 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2219 "sql.cc" /* yacc.c:1667  */
    break;

  case 54:
#line 330 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def_list));
}
#line 2227 "sql.cc" /* yacc.c:1667  */
    break;

  case 55:
#line 333 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def_list));
}
#line 2235 "sql.cc" /* yacc.c:1667  */
    break;

  case 56:
#line 336 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddIndex((yyvsp[-4].name), (yyvsp[-3].key_type) == SQL_NOT_KEY ? SQL_KEY : (yyvsp[-3].key_type),
                                             (yyvsp[-1].name_list));
}
#line 2244 "sql.cc" /* yacc.c:1667  */
    break;

  case 57:
#line 340 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddIndex((yyvsp[-4].name), (yyvsp[-3].key_type) == SQL_NOT_KEY ? SQL_KEY : (yyvsp[-3].key_type),
                                             (yyvsp[-1].name_list));
}
#line 2253 "sql.cc" /* yacc.c:1667  */
    break;

  case 58:
#line 344 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableChangeColumn((yyvsp[-2].name), (yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2261 "sql.cc" /* yacc.c:1667  */
    break;

  case 59:
#line 347 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableChangeColumn((yyvsp[-2].name), (yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2269 "sql.cc" /* yacc.c:1667  */
    break;

  case 60:
#line 350 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameColumn((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2277 "sql.cc" /* yacc.c:1667  */
    break;

  case 61:
#line 353 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameIndex((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2285 "sql.cc" /* yacc.c:1667  */
    break;

  case 62:
#line 356 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameIndex((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2293 "sql.cc" /* yacc.c:1667  */
    break;

  case 63:
#line 359 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2301 "sql.cc" /* yacc.c:1667  */
    break;

  case 64:
#line 362 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2309 "sql.cc" /* yacc.c:1667  */
    break;

  case 65:
#line 365 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2317 "sql.cc" /* yacc.c:1667  */
    break;

  case 66:
#line 368 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropColumn((yyvsp[0].name));
}
#line 2325 "sql.cc" /* yacc.c:1667  */
    break;

  case 67:
#line 371 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropColumn((yyvsp[0].name));
}
#line 2333 "sql.cc" /* yacc.c:1667  */
    break;

  case 68:
#line 374 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex((yyvsp[0].name), false);
}
#line 2341 "sql.cc" /* yacc.c:1667  */
    break;

  case 69:
#line 377 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex((yyvsp[0].name), false);
}
#line 2349 "sql.cc" /* yacc.c:1667  */
    break;

  case 70:
#line 380 "sql.y" /* yacc.c:1667  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex(AstString::kEmpty, true);
}
#line 2357 "sql.cc" /* yacc.c:1667  */
    break;

  case 71:
#line 384 "sql.y" /* yacc.c:1667  */
    {
    (yyval.col_pos).name  = (yyvsp[0].name);
    (yyval.col_pos).after = false;
}
#line 2366 "sql.cc" /* yacc.c:1667  */
    break;

  case 72:
#line 388 "sql.y" /* yacc.c:1667  */
    {
    (yyval.col_pos).name  = (yyvsp[0].name);
    (yyval.col_pos).after = true;
}
#line 2375 "sql.cc" /* yacc.c:1667  */
    break;

  case 73:
#line 392 "sql.y" /* yacc.c:1667  */
    {
    (yyval.col_pos).name  = AstString::kEmpty;
    (yyval.col_pos).after = false;
}
#line 2384 "sql.cc" /* yacc.c:1667  */
    break;

  case 74:
#line 397 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name_list) = ctx->factory->NewNameList((yyvsp[0].name));
}
#line 2392 "sql.cc" /* yacc.c:1667  */
    break;

  case 75:
#line 400 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name_list)->push_back((yyvsp[0].name));
}
#line 2400 "sql.cc" /* yacc.c:1667  */
    break;

  case 79:
#line 411 "sql.y" /* yacc.c:1667  */
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
#line 2418 "sql.cc" /* yacc.c:1667  */
    break;

  case 80:
#line 425 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = true;
}
#line 2426 "sql.cc" /* yacc.c:1667  */
    break;

  case 81:
#line 428 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 2434 "sql.cc" /* yacc.c:1667  */
    break;

  case 82:
#line 431 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 2442 "sql.cc" /* yacc.c:1667  */
    break;

  case 83:
#line 435 "sql.y" /* yacc.c:1667  */
    {
    (yyval.proj_col_list) = ctx->factory->NewProjectionColumnList((yyvsp[0].proj_col));
}
#line 2450 "sql.cc" /* yacc.c:1667  */
    break;

  case 84:
#line 438 "sql.y" /* yacc.c:1667  */
    {
    (yyval.proj_col_list)->push_back((yyvsp[0].proj_col));
}
#line 2458 "sql.cc" /* yacc.c:1667  */
    break;

  case 85:
#line 442 "sql.y" /* yacc.c:1667  */
    {
    (yyval.proj_col) = ctx->factory->NewProjectionColumn((yyvsp[-1].expr), (yyvsp[0].name),
                                           Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2467 "sql.cc" /* yacc.c:1667  */
    break;

  case 86:
#line 446 "sql.y" /* yacc.c:1667  */
    {
    Identifier *id = ctx->factory->NewIdentifierWithPlaceholder((yyvsp[-2].name),
        ctx->factory->NewStarPlaceholder((yylsp[0])),
        Location::Concat((yylsp[-2]), (yylsp[0])));
    (yyval.proj_col) = ctx->factory->NewProjectionColumn(id, AstString::kEmpty,
        Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2479 "sql.cc" /* yacc.c:1667  */
    break;

  case 87:
#line 453 "sql.y" /* yacc.c:1667  */
    {
    Placeholder *ph = ctx->factory->NewStarPlaceholder((yylsp[0]));
    (yyval.proj_col) = ctx->factory->NewProjectionColumn(ph, AstString::kEmpty, (yylsp[0]));
}
#line 2488 "sql.cc" /* yacc.c:1667  */
    break;

  case 89:
#line 460 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name) = AstString::kEmpty;
}
#line 2496 "sql.cc" /* yacc.c:1667  */
    break;

  case 90:
#line 464 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name) = (yyvsp[0].name);
}
#line 2504 "sql.cc" /* yacc.c:1667  */
    break;

  case 91:
#line 467 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name) = (yyvsp[0].name);
}
#line 2512 "sql.cc" /* yacc.c:1667  */
    break;

  case 92:
#line 471 "sql.y" /* yacc.c:1667  */
    {
    (yyval.query) = (yyvsp[0].query);
}
#line 2520 "sql.cc" /* yacc.c:1667  */
    break;

  case 93:
#line 474 "sql.y" /* yacc.c:1667  */
    {
    (yyval.query) = nullptr;
}
#line 2528 "sql.cc" /* yacc.c:1667  */
    break;

  case 94:
#line 478 "sql.y" /* yacc.c:1667  */
    {
    Query *query = ::mai::down_cast<Query>((yyvsp[-2].stmt));
    query->set_alias((yyvsp[0].name));
    (yyval.query) = query;
}
#line 2538 "sql.cc" /* yacc.c:1667  */
    break;

  case 95:
#line 483 "sql.y" /* yacc.c:1667  */
    {
    (yyvsp[-5].query)->set_alias((yyvsp[-4].name));
    (yyvsp[-2].query)->set_alias((yyvsp[-1].name));
    (yyval.query) = ctx->factory->NewJoinRelation((yyvsp[-5].query), SQL_CROSS_JOIN, (yyvsp[-2].query), (yyvsp[0].expr),
        AstString::kEmpty);
}
#line 2549 "sql.cc" /* yacc.c:1667  */
    break;

  case 96:
#line 489 "sql.y" /* yacc.c:1667  */
    {
    (yyvsp[-5].query)->set_alias((yyvsp[-4].name));
    (yyvsp[-2].query)->set_alias((yyvsp[-1].name));
    (yyval.query) = ctx->factory->NewJoinRelation((yyvsp[-5].query), (yyvsp[-3].join_kind), (yyvsp[-2].query), (yyvsp[0].expr), AstString::kEmpty);
}
#line 2559 "sql.cc" /* yacc.c:1667  */
    break;

  case 97:
#line 494 "sql.y" /* yacc.c:1667  */
    {
    (yyval.query) = ctx->factory->NewNameRelation((yyvsp[-1].id), (yyvsp[0].name));
}
#line 2567 "sql.cc" /* yacc.c:1667  */
    break;

  case 98:
#line 498 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2575 "sql.cc" /* yacc.c:1667  */
    break;

  case 99:
#line 501 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = nullptr;
}
#line 2583 "sql.cc" /* yacc.c:1667  */
    break;

  case 100:
#line 505 "sql.y" /* yacc.c:1667  */
    {
    (yyval.join_kind) = SQL_CROSS_JOIN;
}
#line 2591 "sql.cc" /* yacc.c:1667  */
    break;

  case 101:
#line 508 "sql.y" /* yacc.c:1667  */
    {
    (yyval.join_kind) = SQL_CROSS_JOIN;
}
#line 2599 "sql.cc" /* yacc.c:1667  */
    break;

  case 102:
#line 511 "sql.y" /* yacc.c:1667  */
    {
    (yyval.join_kind) = SQL_LEFT_OUTTER_JOIN;
}
#line 2607 "sql.cc" /* yacc.c:1667  */
    break;

  case 103:
#line 514 "sql.y" /* yacc.c:1667  */
    {
    (yyval.join_kind) = SQL_LEFT_OUTTER_JOIN;
}
#line 2615 "sql.cc" /* yacc.c:1667  */
    break;

  case 104:
#line 517 "sql.y" /* yacc.c:1667  */
    {
    (yyval.join_kind) = SQL_RIGHT_OUTTER_JOIN;
}
#line 2623 "sql.cc" /* yacc.c:1667  */
    break;

  case 105:
#line 520 "sql.y" /* yacc.c:1667  */
    {
    (yyval.join_kind) = SQL_RIGHT_OUTTER_JOIN;
}
#line 2631 "sql.cc" /* yacc.c:1667  */
    break;

  case 106:
#line 524 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2639 "sql.cc" /* yacc.c:1667  */
    break;

  case 107:
#line 527 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = nullptr;
}
#line 2647 "sql.cc" /* yacc.c:1667  */
    break;

  case 108:
#line 531 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2655 "sql.cc" /* yacc.c:1667  */
    break;

  case 109:
#line 534 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = nullptr;
}
#line 2663 "sql.cc" /* yacc.c:1667  */
    break;

  case 110:
#line 538 "sql.y" /* yacc.c:1667  */
    {
    (yyval.order_by).expr_list = (yyvsp[-1].expr_list);
    (yyval.order_by).desc = false;
}
#line 2672 "sql.cc" /* yacc.c:1667  */
    break;

  case 111:
#line 542 "sql.y" /* yacc.c:1667  */
    {
    (yyval.order_by).expr_list = (yyvsp[-1].expr_list);
    (yyval.order_by).desc = true;
}
#line 2681 "sql.cc" /* yacc.c:1667  */
    break;

  case 112:
#line 546 "sql.y" /* yacc.c:1667  */
    {
    (yyval.order_by).expr_list = (yyvsp[0].expr_list);
    (yyval.order_by).desc = false;
}
#line 2690 "sql.cc" /* yacc.c:1667  */
    break;

  case 113:
#line 550 "sql.y" /* yacc.c:1667  */
    {
    (yyval.order_by).expr_list = nullptr;
    (yyval.order_by).desc = false;
}
#line 2699 "sql.cc" /* yacc.c:1667  */
    break;

  case 114:
#line 555 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr_list) = (yyvsp[0].expr_list);
}
#line 2707 "sql.cc" /* yacc.c:1667  */
    break;

  case 115:
#line 558 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr_list) = nullptr;
}
#line 2715 "sql.cc" /* yacc.c:1667  */
    break;

  case 116:
#line 562 "sql.y" /* yacc.c:1667  */
    {
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
    (yyval.limit).offset_val = 0;
}
#line 2724 "sql.cc" /* yacc.c:1667  */
    break;

  case 117:
#line 566 "sql.y" /* yacc.c:1667  */
    {
    (yyval.limit).offset_val = (yyvsp[-2].int_val);
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
}
#line 2733 "sql.cc" /* yacc.c:1667  */
    break;

  case 118:
#line 570 "sql.y" /* yacc.c:1667  */
    {
    (yyval.limit).limit_val  = (yyvsp[-2].int_val);
    (yyval.limit).offset_val = (yyvsp[0].int_val);
}
#line 2742 "sql.cc" /* yacc.c:1667  */
    break;

  case 119:
#line 574 "sql.y" /* yacc.c:1667  */
    {
    (yyval.limit).limit_val  = 0;
    (yyval.limit).offset_val = 0;
}
#line 2751 "sql.cc" /* yacc.c:1667  */
    break;

  case 120:
#line 580 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = true;
}
#line 2759 "sql.cc" /* yacc.c:1667  */
    break;

  case 121:
#line 583 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 2767 "sql.cc" /* yacc.c:1667  */
    break;

  case 122:
#line 587 "sql.y" /* yacc.c:1667  */
    {
    Insert *stmt = ctx->factory->NewInsert((yyvsp[-6].bool_val), (yyvsp[-4].id));
    stmt->set_col_names((yyvsp[-3].name_list));
    stmt->set_row_values_list((yyvsp[-1].row_vals_list));
    stmt->set_on_duplicate_clause((yyvsp[0].assignment_list));
    (yyval.stmt) = stmt;
}
#line 2779 "sql.cc" /* yacc.c:1667  */
    break;

  case 123:
#line 594 "sql.y" /* yacc.c:1667  */
    {
    Insert *stmt = ctx->factory->NewInsert((yyvsp[-5].bool_val), (yyvsp[-3].id));
    stmt->SetAssignmentList((yyvsp[-1].assignment_list), ctx->factory->arena());
    stmt->set_on_duplicate_clause((yyvsp[0].assignment_list));
    (yyval.stmt) = stmt;
}
#line 2790 "sql.cc" /* yacc.c:1667  */
    break;

  case 124:
#line 600 "sql.y" /* yacc.c:1667  */
    {
    Insert *stmt = ctx->factory->NewInsert((yyvsp[-5].bool_val), (yyvsp[-3].id));
    stmt->set_col_names((yyvsp[-2].name_list));
    stmt->set_select_clause(::mai::down_cast<Query>((yyvsp[-1].stmt)));
    stmt->set_on_duplicate_clause((yyvsp[0].assignment_list));
    (yyval.stmt) = stmt;
}
#line 2802 "sql.cc" /* yacc.c:1667  */
    break;

  case 126:
#line 609 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name_list) = nullptr;
}
#line 2810 "sql.cc" /* yacc.c:1667  */
    break;

  case 127:
#line 613 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = true;
}
#line 2818 "sql.cc" /* yacc.c:1667  */
    break;

  case 128:
#line 616 "sql.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 2826 "sql.cc" /* yacc.c:1667  */
    break;

  case 131:
#line 623 "sql.y" /* yacc.c:1667  */
    {
    (yyval.row_vals_list) = ctx->factory->NewRowValuesList((yyvsp[0].expr_list));
}
#line 2834 "sql.cc" /* yacc.c:1667  */
    break;

  case 132:
#line 626 "sql.y" /* yacc.c:1667  */
    {
    (yyval.row_vals_list)->push_back((yyvsp[0].expr_list));
}
#line 2842 "sql.cc" /* yacc.c:1667  */
    break;

  case 133:
#line 630 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr_list) = (yyvsp[-1].expr_list);
}
#line 2850 "sql.cc" /* yacc.c:1667  */
    break;

  case 134:
#line 634 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr_list) = ctx->factory->NewExpressionList((yyvsp[0].expr));
}
#line 2858 "sql.cc" /* yacc.c:1667  */
    break;

  case 135:
#line 637 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr_list)->push_back((yyvsp[0].expr));
}
#line 2866 "sql.cc" /* yacc.c:1667  */
    break;

  case 137:
#line 642 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewDefaultPlaceholderLiteral((yylsp[0]));
}
#line 2874 "sql.cc" /* yacc.c:1667  */
    break;

  case 138:
#line 646 "sql.y" /* yacc.c:1667  */
    {
    (yyval.assignment_list) = (yyvsp[0].assignment_list);
}
#line 2882 "sql.cc" /* yacc.c:1667  */
    break;

  case 139:
#line 649 "sql.y" /* yacc.c:1667  */
    {
    (yyval.assignment_list) = nullptr;
}
#line 2890 "sql.cc" /* yacc.c:1667  */
    break;

  case 140:
#line 653 "sql.y" /* yacc.c:1667  */
    {
    (yyval.assignment_list) = ctx->factory->NewAssignmentList((yyvsp[0].assignment));
}
#line 2898 "sql.cc" /* yacc.c:1667  */
    break;

  case 141:
#line 656 "sql.y" /* yacc.c:1667  */
    {
    (yyval.assignment_list)->push_back((yyvsp[0].assignment));
}
#line 2906 "sql.cc" /* yacc.c:1667  */
    break;

  case 142:
#line 660 "sql.y" /* yacc.c:1667  */
    {
    if ((yyvsp[-1].op) != SQL_CMP_EQ) {
        yyerror(&(yylsp[-2]), ctx, "incorrect assignment.");
        YYERROR;
    }
    (yyval.assignment) = ctx->factory->NewAssignment((yyvsp[-2].name), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2918 "sql.cc" /* yacc.c:1667  */
    break;

  case 143:
#line 669 "sql.y" /* yacc.c:1667  */
    {
    Update *stmt = ctx->factory->NewUpdate((yyvsp[-5].id), (yyvsp[-3].assignment_list));
    stmt->set_where_clause((yyvsp[-2].expr));
    stmt->set_order_by_desc((yyvsp[-1].order_by).desc);
    stmt->set_order_by_clause((yyvsp[-1].order_by).expr_list);
    stmt->set_limit_val((yyvsp[0].limit).limit_val);
    (yyval.stmt) = stmt;
}
#line 2931 "sql.cc" /* yacc.c:1667  */
    break;

  case 144:
#line 678 "sql.y" /* yacc.c:1667  */
    {
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
    (yyval.limit).offset_val = 0;
}
#line 2940 "sql.cc" /* yacc.c:1667  */
    break;

  case 145:
#line 682 "sql.y" /* yacc.c:1667  */
    {
    (yyval.limit).limit_val  = 0;
    (yyval.limit).offset_val = 0;
}
#line 2949 "sql.cc" /* yacc.c:1667  */
    break;

  case 146:
#line 692 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr_list) = ctx->factory->NewExpressionList((yyvsp[0].expr));
}
#line 2957 "sql.cc" /* yacc.c:1667  */
    break;

  case 147:
#line 695 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr_list)->push_back((yyvsp[0].expr));
}
#line 2965 "sql.cc" /* yacc.c:1667  */
    break;

  case 148:
#line 699 "sql.y" /* yacc.c:1667  */
    {
    Placeholder *ph = ctx->factory->NewStarPlaceholder((yylsp[0]));
    (yyval.expr_list) = ctx->factory->NewExpressionList(ph);
}
#line 2974 "sql.cc" /* yacc.c:1667  */
    break;

  case 150:
#line 708 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_OR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2982 "sql.cc" /* yacc.c:1667  */
    break;

  case 151:
#line 711 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_XOR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2990 "sql.cc" /* yacc.c:1667  */
    break;

  case 152:
#line 714 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_AND, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2998 "sql.cc" /* yacc.c:1667  */
    break;

  case 153:
#line 717 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_NOT, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 3006 "sql.cc" /* yacc.c:1667  */
    break;

  case 154:
#line 720 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_NOT, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 3014 "sql.cc" /* yacc.c:1667  */
    break;

  case 156:
#line 728 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_IS_NULL, (yyvsp[-2].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3022 "sql.cc" /* yacc.c:1667  */
    break;

  case 157:
#line 731 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_IS_NOT_NULL, (yyvsp[-3].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3030 "sql.cc" /* yacc.c:1667  */
    break;

  case 158:
#line 734 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-2].expr), (yyvsp[-1].op), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3038 "sql.cc" /* yacc.c:1667  */
    break;

  case 159:
#line 737 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-3].expr), (yyvsp[-2].op), (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3046 "sql.cc" /* yacc.c:1667  */
    break;

  case 160:
#line 740 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-3].expr), (yyvsp[-2].op), (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3054 "sql.cc" /* yacc.c:1667  */
    break;

  case 162:
#line 749 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-3].expr), SQL_NOT_IN, (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3062 "sql.cc" /* yacc.c:1667  */
    break;

  case 163:
#line 752 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-2].expr), SQL_IN, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3070 "sql.cc" /* yacc.c:1667  */
    break;

  case 164:
#line 755 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-5].expr), SQL_NOT_IN, (yyvsp[-1].expr_list), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 3078 "sql.cc" /* yacc.c:1667  */
    break;

  case 165:
#line 758 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-4].expr), SQL_IN, (yyvsp[-1].expr_list), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 3086 "sql.cc" /* yacc.c:1667  */
    break;

  case 166:
#line 761 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-5].expr), SQL_NOT_BETWEEN, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 3094 "sql.cc" /* yacc.c:1667  */
    break;

  case 167:
#line 764 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-4].expr), SQL_BETWEEN, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 3102 "sql.cc" /* yacc.c:1667  */
    break;

  case 168:
#line 767 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-3].expr), SQL_NOT_LIKE, (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3110 "sql.cc" /* yacc.c:1667  */
    break;

  case 169:
#line 770 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_LIKE, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3118 "sql.cc" /* yacc.c:1667  */
    break;

  case 171:
#line 779 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_OR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3126 "sql.cc" /* yacc.c:1667  */
    break;

  case 172:
#line 782 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_AND, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3134 "sql.cc" /* yacc.c:1667  */
    break;

  case 173:
#line 785 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_LSHIFT, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3142 "sql.cc" /* yacc.c:1667  */
    break;

  case 174:
#line 788 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_RSHIFT, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3150 "sql.cc" /* yacc.c:1667  */
    break;

  case 175:
#line 791 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_PLUS, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3158 "sql.cc" /* yacc.c:1667  */
    break;

  case 176:
#line 794 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_SUB, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3166 "sql.cc" /* yacc.c:1667  */
    break;

  case 177:
#line 797 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MUL, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3174 "sql.cc" /* yacc.c:1667  */
    break;

  case 178:
#line 800 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_DIV, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3182 "sql.cc" /* yacc.c:1667  */
    break;

  case 179:
#line 803 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_DIV, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3190 "sql.cc" /* yacc.c:1667  */
    break;

  case 180:
#line 806 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MOD, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3198 "sql.cc" /* yacc.c:1667  */
    break;

  case 181:
#line 809 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MOD, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3206 "sql.cc" /* yacc.c:1667  */
    break;

  case 182:
#line 812 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_XOR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3214 "sql.cc" /* yacc.c:1667  */
    break;

  case 184:
#line 821 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[0].id);
}
#line 3222 "sql.cc" /* yacc.c:1667  */
    break;

  case 185:
#line 824 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewStringLiteral((yyvsp[0].text).buf, (yyvsp[0].text).len, (yylsp[0]));
}
#line 3230 "sql.cc" /* yacc.c:1667  */
    break;

  case 186:
#line 827 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewIntegerLiteral((yyvsp[0].i64_val), (yylsp[0]));
}
#line 3238 "sql.cc" /* yacc.c:1667  */
    break;

  case 187:
#line 830 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewApproxLiteral((yyvsp[0].approx_val), (yylsp[0]));
}
#line 3246 "sql.cc" /* yacc.c:1667  */
    break;

  case 188:
#line 833 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewDecimalLiteral((yyvsp[0].dec_val), (yylsp[0]));
}
#line 3254 "sql.cc" /* yacc.c:1667  */
    break;

  case 189:
#line 836 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewCall((yyvsp[-4].name), (yyvsp[-2].bool_val), (yyvsp[-1].expr_list), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 3262 "sql.cc" /* yacc.c:1667  */
    break;

  case 190:
#line 839 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBindPlaceholder((yylsp[0]));
}
#line 3270 "sql.cc" /* yacc.c:1667  */
    break;

  case 191:
#line 842 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_MINUS, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 3278 "sql.cc" /* yacc.c:1667  */
    break;

  case 192:
#line 845 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_BIT_INV, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 3286 "sql.cc" /* yacc.c:1667  */
    break;

  case 193:
#line 848 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 3294 "sql.cc" /* yacc.c:1667  */
    break;

  case 194:
#line 853 "sql.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewSubquery(true, ::mai::down_cast<Query>((yyvsp[-1].stmt)), (yylsp[-1]));
}
#line 3302 "sql.cc" /* yacc.c:1667  */
    break;

  case 195:
#line 857 "sql.y" /* yacc.c:1667  */
    {
    (yyval.id) = ctx->factory->NewIdentifier(AstString::kEmpty, (yyvsp[0].name), (yylsp[0]));
}
#line 3310 "sql.cc" /* yacc.c:1667  */
    break;

  case 196:
#line 860 "sql.y" /* yacc.c:1667  */
    {
    (yyval.id) = ctx->factory->NewIdentifier((yyvsp[-2].name), (yyvsp[0].name), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3318 "sql.cc" /* yacc.c:1667  */
    break;

  case 197:
#line 864 "sql.y" /* yacc.c:1667  */
    {
    (yyval.name) = ctx->factory->NewString((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 3326 "sql.cc" /* yacc.c:1667  */
    break;

  case 198:
#line 868 "sql.y" /* yacc.c:1667  */
    {
    if ((yyvsp[0].i64_val) > INT32_MAX || (yyvsp[0].i64_val) < INT32_MIN) {
        yyerror(&(yylsp[0]), ctx, "int val out of range.");
        YYERROR;
    }
    (yyval.int_val) = static_cast<int>((yyvsp[0].i64_val));
}
#line 3338 "sql.cc" /* yacc.c:1667  */
    break;


#line 3342 "sql.cc" /* yacc.c:1667  */
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
#line 876 "sql.y" /* yacc.c:1918  */

void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg) {
    ctx->err_line   = yyl->first_line;
    ctx->err_column = yyl->first_column;
    ctx->err_msg    = ctx->factory->NewString(msg);
}
