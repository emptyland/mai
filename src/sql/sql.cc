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
using namespace ::mai::sql::ast;

#define YYLEX_PARAM ctx->lex

void yyerror(YYLTYPE *, parser_ctx *, const char *);

#line 91 "sql.cc" /* yacc.c:338  */
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
    ID = 315,
    NULL_VAL = 316,
    INTEGRAL_VAL = 317,
    STRING_VAL = 318,
    APPROX_VAL = 319,
    DATE_VAL = 320,
    DATETIME_VAL = 321,
    EQ = 322,
    NOT = 323,
    OP_AND = 324,
    BIGINT = 325,
    INT = 326,
    SMALLINT = 327,
    TINYINT = 328,
    DECIMAL = 329,
    NUMERIC = 330,
    CHAR = 331,
    VARCHAR = 332,
    DATE = 333,
    DATETIME = 334,
    TIMESTMAP = 335,
    AUTO_INCREMENT = 336,
    COMMENT = 337,
    TOKEN_ERROR = 338,
    ASSIGN = 339,
    OP_OR = 340,
    XOR = 341,
    IS = 342,
    LIKE = 343,
    REGEXP = 344,
    BETWEEN = 345,
    COMPARISON = 346,
    LSHIFT = 347,
    RSHIFT = 348,
    MOD = 349,
    UMINUS = 350
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 34 "sql.y" /* yacc.c:353  */

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

#line 277 "sql.cc" /* yacc.c:353  */
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
#define YYLAST   513

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  112
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  60
/* YYNRULES -- Number of rules.  */
#define YYNRULES  191
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  373

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   350

#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    90,     2,     2,     2,   101,    94,     2,
     106,   107,    99,    97,   108,    98,   109,   100,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   105,
       2,     2,     2,   110,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   103,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    93,     2,   111,     2,     2,     2,
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
      85,    86,    87,    88,    89,    91,    92,    95,    96,   102,
     104
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   144,   144,   149,   154,   156,   157,   158,   161,   164,
     167,   172,   175,   178,   182,   186,   189,   193,   203,   206,
     209,   212,   215,   218,   221,   224,   227,   230,   234,   238,
     243,   247,   252,   255,   259,   262,   266,   269,   272,   276,
     279,   282,   285,   288,   291,   295,   298,   302,   306,   309,
     313,   316,   319,   322,   325,   329,   333,   336,   339,   342,
     345,   348,   351,   354,   357,   360,   363,   366,   369,   373,
     377,   381,   386,   389,   396,   397,   398,   400,   414,   417,
     420,   424,   427,   431,   435,   442,   448,   449,   453,   456,
     460,   463,   467,   472,   478,   483,   487,   490,   494,   497,
     500,   503,   506,   509,   513,   516,   520,   523,   527,   531,
     535,   539,   544,   547,   551,   555,   559,   563,   569,   572,
     576,   583,   589,   597,   598,   602,   605,   609,   610,   612,
     615,   619,   623,   626,   630,   631,   635,   638,   642,   645,
     649,   658,   667,   671,   681,   684,   691,   694,   697,   700,
     703,   706,   709,   714,   717,   720,   723,   726,   729,   734,
     737,   740,   743,   746,   749,   752,   755,   758,   764,   767,
     770,   773,   776,   779,   782,   785,   788,   791,   794,   797,
     800,   806,   809,   812,   815,   818,   821,   824,   829,   833,
     836,   840
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
  "ID", "NULL_VAL", "INTEGRAL_VAL", "STRING_VAL", "APPROX_VAL", "DATE_VAL",
  "DATETIME_VAL", "EQ", "NOT", "OP_AND", "BIGINT", "INT", "SMALLINT",
  "TINYINT", "DECIMAL", "NUMERIC", "CHAR", "VARCHAR", "DATE", "DATETIME",
  "TIMESTMAP", "AUTO_INCREMENT", "COMMENT", "TOKEN_ERROR", "ASSIGN",
  "OP_OR", "XOR", "IS", "LIKE", "REGEXP", "'!'", "BETWEEN", "COMPARISON",
  "'|'", "'&'", "LSHIFT", "RSHIFT", "'+'", "'-'", "'*'", "'/'", "'%'",
  "MOD", "'^'", "UMINUS", "';'", "'('", "')'", "','", "'.'", "'?'", "'~'",
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
  "UpdateLimitOption", "ExpressionList", "Expression", "BoolPrimary",
  "Predicate", "BitExpression", "Simple", "Subquery", "Identifier", "Name", YY_NULLPTR
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
      33,   345,   346,   124,    38,   347,   348,    43,    45,    42,
      47,    37,   349,    94,   350,    59,    40,    41,    44,    46,
      63,   126
};
# endif

#define YYPACT_NINF -274

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-274)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     437,    16,    63,   187,   134,   190,   182,  -274,  -274,   148,
     160,   429,  -274,   118,  -274,  -274,  -274,  -274,  -274,  -274,
    -274,  -274,  -274,   280,   148,   148,  -274,   148,  -274,  -274,
     173,   121,  -274,   175,  -274,  -274,  -274,  -274,  -274,  -274,
     304,   304,   271,  -274,   304,  -274,   271,     8,  -274,   141,
     -45,  -274,   156,  -274,  -274,   131,   139,  -274,   205,   148,
     148,   148,   214,   214,  -274,   381,  -274,   -36,   280,   216,
     148,   304,   304,   304,   116,  -274,  -274,  -274,   110,   271,
     271,   191,    94,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   271,   271,   271,   271,   -25,   148,   278,    11,
     159,   135,   200,  -274,     6,  -274,   240,  -274,   154,  -274,
     310,    93,    93,  -274,   304,   289,  -274,   214,   -33,   105,
     231,   231,  -274,   286,  -274,   189,    62,  -274,   247,   271,
     271,   189,   170,   260,     9,    64,    64,    -1,    -1,    12,
      12,    12,    12,   351,  -274,   169,  -274,   427,   354,   148,
     148,   148,  -274,   148,    -9,   148,   148,    35,   148,   148,
     148,   148,   148,  -274,   148,   148,   205,   148,   289,   241,
     148,   269,    65,  -274,   273,   167,  -274,   149,   343,   342,
     310,  -274,  -274,  -274,   281,   171,   149,    62,  -274,   189,
     224,   271,  -274,   148,   283,   283,   283,   283,   287,   287,
     283,   283,  -274,  -274,   122,  -274,  -274,  -274,  -274,   142,
     148,    35,   142,   221,   148,   148,  -274,   365,   368,  -274,
    -274,   369,   148,    35,  -274,  -274,   349,  -274,  -274,  -274,
     -19,   148,  -274,  -274,   364,   293,    93,  -274,    25,    30,
     367,   -36,   -36,   304,   359,   372,  -274,  -274,   304,   238,
     271,  -274,  -274,   344,  -274,  -274,  -274,  -274,   345,  -274,
    -274,  -274,  -274,  -274,   347,   323,   388,   392,  -274,   305,
     242,  -274,   306,  -274,  -274,  -274,   148,   148,   148,    35,
    -274,   371,  -274,   373,  -274,  -274,  -274,   241,   -18,  -274,
    -274,  -274,   397,  -274,   402,  -274,    93,    93,   -14,   304,
     330,   393,   149,  -274,  -274,   336,   335,  -274,  -274,   390,
    -274,  -274,   148,  -274,   148,  -274,  -274,  -274,  -274,  -274,
     434,   266,  -274,   293,  -274,  -274,  -274,    39,    39,  -274,
    -274,   356,   304,   395,   409,  -274,   403,   304,   142,   268,
     276,   412,  -274,   241,  -274,   362,  -274,  -274,   385,   -22,
     422,  -274,   375,   149,   396,  -274,  -274,   148,  -274,   304,
    -274,   410,   418,  -274,  -274,   420,  -274,   377,   389,  -274,
    -274,  -274,  -274
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    80,     0,     0,     0,     0,     0,     8,     9,     0,
     126,     0,     2,     0,     5,    11,    13,     6,    74,    75,
      76,    78,    79,     0,     0,     0,    10,     0,     7,   191,
       0,   189,   125,     0,     1,     3,     4,   183,   182,   184,
       0,     0,     0,    85,     0,   185,     0,    91,    81,    87,
     152,   158,   167,   180,   181,   189,     0,    12,     0,     0,
       0,     0,   149,   150,   186,     0,   187,     0,     0,   105,
       0,     0,     0,     0,     0,    83,    86,    89,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    47,    48,   105,   138,     0,   190,   124,   151,
       0,    90,    87,    82,     0,   111,    88,   148,   146,   147,
       0,     0,   153,     0,   155,   176,     0,   160,     0,     0,
       0,   166,     0,   168,   169,   170,   171,   172,   173,   174,
     175,   177,   178,   179,    84,     0,    15,     0,     0,     0,
       0,     0,    65,     0,     0,     0,     0,    71,     0,     0,
       0,     0,     0,    61,     0,     0,     0,     0,   111,     0,
       0,   123,     0,    72,     0,     0,    95,   104,     0,   113,
       0,   157,   156,   154,     0,     0,   144,     0,   159,   165,
       0,     0,    14,     0,    29,    29,    29,    29,    31,    31,
      29,    29,    26,    27,    38,    68,    67,    64,    66,    44,
       0,    71,    44,     0,     0,     0,    50,     0,     0,    62,
      63,     0,     0,    71,    49,   139,   143,   135,   140,   134,
     137,     0,   128,   127,   137,     0,     0,    98,     0,     0,
       0,     0,     0,     0,     0,   107,   188,   162,     0,     0,
       0,   164,    16,     0,    18,    19,    20,    21,     0,    22,
      23,    24,    25,    37,     0,    35,    40,    42,    39,     0,
       0,    51,     0,    52,    70,    69,     0,     0,     0,    71,
      56,     0,   141,     0,   121,    73,   122,     0,   137,   129,
      92,   100,     0,   102,     0,    99,     0,     0,   110,     0,
       0,   117,   145,   161,   163,     0,     0,    36,    34,    33,
      41,    43,     0,    53,     0,    60,    58,    59,    57,   142,
       0,     0,   132,     0,   120,   101,   103,    97,    97,   108,
     109,   112,     0,     0,   119,    28,     0,     0,    44,     0,
       0,     0,   131,     0,   130,     0,    93,    94,     0,   114,
       0,    77,     0,    32,    46,    55,    54,     0,   133,     0,
     106,     0,     0,   118,    30,     0,    17,   136,     0,   116,
     115,    45,    96
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -274,  -274,   475,  -274,  -274,  -274,  -122,   -90,  -274,   115,
     290,  -274,  -274,  -274,  -202,  -274,  -274,  -274,   324,  -186,
    -273,  -274,     3,  -274,  -274,   423,   382,  -105,  -274,     1,
     165,  -274,   391,  -274,   339,  -274,  -274,  -274,  -274,  -274,
    -274,  -274,  -274,   185,  -274,  -256,  -107,  -168,   346,  -274,
    -274,  -176,   -23,  -274,   -71,   333,   -41,   101,    -5,   -16
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    11,    12,    13,    14,    15,   145,   146,   204,   254,
     259,   338,   309,   265,   269,   366,    16,   102,   103,   216,
     171,    17,   184,    23,    47,    48,    75,    76,    69,   111,
     346,   242,   115,   301,   179,   245,   334,   351,    19,   172,
      33,   235,   288,   289,   321,   228,   284,   104,   105,    20,
     282,   185,   186,    50,    51,    52,    53,   127,    54,    31
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint16 yytable[] =
{
      49,    64,   230,    18,    30,    66,   175,    55,   124,   157,
     272,   249,    67,    80,    18,   283,   283,    62,    63,    56,
      57,    65,    58,    80,    29,   271,    80,   361,   153,   329,
     330,   322,   154,    77,   213,    29,    71,   280,   114,   339,
     155,   340,    78,   106,   107,    49,    21,    79,   117,   118,
     119,    29,    55,    73,   116,    22,   108,   214,   291,    74,
     215,   292,   112,   293,   211,     1,   294,   298,     1,    24,
     110,    29,   237,   345,   144,   223,   238,   239,    80,   240,
     107,   147,   152,   147,   163,   165,   362,   358,   270,   167,
     323,   177,   173,   318,   248,    77,    77,   210,    91,    92,
      93,    94,    95,   252,    87,    88,    89,    90,    91,    92,
      93,    94,    95,   174,   167,    95,    68,   156,   232,   233,
     251,    70,    29,   331,    37,    38,    39,   286,   229,   120,
      40,   290,   279,   206,   207,   208,   354,   209,   147,   212,
     147,    26,   217,   218,   219,   220,   221,   241,   222,   147,
     128,   106,    41,    29,   106,   121,   164,   266,   267,   268,
      42,    89,    90,    91,    92,    93,    94,    95,    44,    70,
      80,   122,    45,    46,    71,   234,   158,   147,   123,   304,
     159,   324,   129,   263,    80,   130,   160,   161,   162,   367,
     264,   327,   328,    25,   147,    29,    27,    74,   274,   275,
     237,    29,    28,    80,   238,   239,   147,   240,    29,   170,
      71,    32,    81,    98,    29,   285,    99,   100,    71,    29,
      77,   181,   182,    36,    82,   302,    72,    73,    59,   188,
      60,   101,    61,    74,    72,    73,   112,   112,    80,   191,
      96,    74,   296,   297,    83,    97,   229,    84,   114,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
     315,   316,   317,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    80,   241,   192,   193,   247,   248,
      77,    77,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,   250,   148,   149,   173,   126,   173,   150,
     227,    29,   229,    37,    38,    39,    74,   151,   166,   348,
     255,   256,   257,     1,   353,   261,   262,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,   273,   193,
     178,    29,   169,    37,    38,    39,   368,   180,    29,    42,
      29,   106,    37,    38,    39,   303,   248,   183,    40,   313,
     193,    45,    46,   187,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    29,    80,    37,    38,    39,    42,
      41,   205,    40,   342,   343,   355,   231,   231,    42,    43,
     236,    45,    46,   356,   231,   243,    44,   244,   246,   253,
      45,    46,   276,   258,    41,   277,   278,   281,   283,   287,
     295,   299,    42,   300,   308,   310,   305,   306,   307,   311,
      44,   312,   314,   125,    45,    46,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,    34,
     325,   320,     1,   319,     2,   326,   332,     3,     4,     5,
       1,   333,     2,   335,   336,     3,     4,     5,     6,   337,
      71,   341,     7,     8,    71,   350,     6,   349,    71,   357,
       7,     8,   189,   190,   248,   352,    72,    73,   359,   363,
      72,    73,   369,    74,    72,    73,     9,    74,   365,    10,
     370,    74,   364,   371,     9,   167,    35,    10,   109,   260,
     224,   113,   360,   347,   176,   168,   372,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   226,   344,     0,
       0,     0,     0,   225
};

static const yytype_int16 yycheck[] =
{
      23,    42,   170,     0,     9,    46,   111,    23,    79,    99,
     212,   187,     4,    14,    11,    34,    34,    40,    41,    24,
      25,    44,    27,    14,    60,   211,    14,    49,    17,    43,
      44,   287,    21,    49,   156,    60,    69,   223,    32,   312,
      29,   314,    87,    59,    60,    68,    30,    92,    71,    72,
      73,    60,    68,    86,    70,    39,    61,    22,    33,    92,
      25,    36,    67,    33,   154,     3,    36,   243,     3,     6,
     106,    60,    33,    34,    99,   165,    37,    38,    14,    40,
      96,    97,    98,    99,   100,   101,   108,   343,   210,   108,
     108,   114,   108,   279,   108,   111,   112,   106,    99,   100,
     101,   102,   103,   193,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   110,   108,   103,   108,   106,    53,    54,
     191,    28,    60,   299,    62,    63,    64,   234,   169,    13,
      68,   236,   222,   149,   150,   151,   338,   153,   154,   155,
     156,     7,   158,   159,   160,   161,   162,   108,   164,   165,
      56,   167,    90,    60,   170,    39,    21,    15,    16,    17,
      98,    97,    98,    99,   100,   101,   102,   103,   106,    28,
      14,    61,   110,   111,    69,   172,    17,   193,    68,   250,
      21,   288,    88,    61,    14,    91,    27,    28,    29,   357,
      68,   296,   297,     6,   210,    60,     6,    92,   214,   215,
      33,    60,    20,    14,    37,    38,   222,    40,    60,    55,
      69,    51,    56,     8,    60,   231,    11,    12,    69,    60,
     236,   120,   121,   105,    68,   248,    85,    86,    55,   128,
     109,    26,    57,    92,    85,    86,   241,   242,    14,    69,
     109,    92,   241,   242,    88,   106,   287,    91,    32,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     276,   277,   278,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,    14,   108,   107,   108,   107,   108,
     296,   297,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,    69,    16,    17,   312,   106,   314,    21,
      59,    60,   343,    62,    63,    64,    92,    29,   108,   332,
     195,   196,   197,     3,   337,   200,   201,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   107,   108,
      41,    60,    92,    62,    63,    64,   359,   106,    60,    98,
      60,   357,    62,    63,    64,   107,   108,    61,    68,   107,
     108,   110,   111,   106,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,    60,    14,    62,    63,    64,    98,
      90,    17,    68,   107,   108,   107,   108,   108,    98,    99,
     107,   110,   111,   107,   108,    42,   106,    45,   107,   106,
     110,   111,    27,   106,    90,    27,    27,    48,    34,   106,
      33,    42,    98,    31,    81,    17,    62,    62,    61,    17,
     106,   106,   106,    80,   110,   111,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,     0,
      33,    58,     3,    62,     5,    33,   106,     8,     9,    10,
       3,    48,     5,   107,   109,     8,     9,    10,    19,    59,
      69,    17,    23,    24,    69,    46,    19,    62,    69,    47,
      23,    24,   129,   130,   108,    62,    85,    86,   106,    47,
      85,    86,    62,    92,    85,    86,    47,    92,    82,    50,
      62,    92,   107,    63,    47,   108,    11,    50,   107,   199,
     166,    68,   107,   328,   112,   104,   107,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,   168,   323,    -1,
      -1,    -1,    -1,   167
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     5,     8,     9,    10,    19,    23,    24,    47,
      50,   113,   114,   115,   116,   117,   128,   133,   134,   150,
     161,    30,    39,   135,     6,     6,     7,     6,    20,    60,
     170,   171,    51,   152,     0,   114,   105,    62,    63,    64,
      68,    90,    98,    99,   106,   110,   111,   136,   137,   164,
     165,   166,   167,   168,   170,   171,   170,   170,   170,    55,
     109,    57,   164,   164,   168,   164,   168,     4,   108,   140,
      28,    69,    85,    86,    92,   138,   139,   171,    87,    92,
      14,    56,    68,    88,    91,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   109,   106,     8,    11,
      12,    26,   129,   130,   159,   160,   171,   171,   170,   107,
     106,   141,   170,   137,    32,   144,   171,   164,   164,   164,
      13,    39,    61,    68,   166,   167,   106,   169,    56,    88,
      91,   167,   167,   167,   167,   167,   167,   167,   167,   167,
     167,   167,   167,   167,    99,   118,   119,   171,    16,    17,
      21,    29,   171,    17,    21,    29,   106,   119,    17,    21,
      27,    28,    29,   171,    21,   171,   108,   108,   144,    92,
      55,   132,   151,   171,   134,   139,   138,   164,    41,   146,
     106,   169,   169,    61,   134,   163,   164,   106,   169,   167,
     167,    69,   107,   108,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,   120,    17,   171,   171,   171,   171,
     106,   119,   171,   118,    22,    25,   131,   171,   171,   171,
     171,   171,   171,   119,   130,   160,   146,    59,   157,   168,
     159,   108,    53,    54,   134,   153,   107,    33,    37,    38,
      40,   108,   143,    42,    45,   147,   107,   107,   108,   163,
      69,   166,   119,   106,   121,   121,   121,   121,   106,   122,
     122,   121,   121,    61,    68,   125,    15,    16,    17,   126,
     118,   131,   126,   107,   171,   171,    27,    27,    27,   119,
     131,    48,   162,    34,   158,   171,   158,   106,   154,   155,
     139,    33,    36,    33,    36,    33,   141,   141,   163,    42,
      31,   145,   164,   107,   166,    62,    62,    61,    81,   124,
      17,    17,   106,   107,   106,   171,   171,   171,   131,    62,
      58,   156,   157,   108,   158,    33,    33,   139,   139,    43,
      44,   163,   106,    48,   148,   107,   109,    59,   123,   132,
     132,    17,   107,   108,   155,    34,   142,   142,   164,    62,
      46,   149,    62,   164,   126,   107,   107,    47,   157,   106,
     107,    49,   108,    47,   107,    82,   127,   159,   164,    62,
      62,    63,   107
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   112,   113,   113,   114,   115,   115,   115,   115,   115,
     115,   116,   116,   116,   117,   118,   118,   119,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   121,   121,
     122,   122,   123,   123,   124,   124,   125,   125,   125,   126,
     126,   126,   126,   126,   126,   127,   127,   128,   129,   129,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   131,
     131,   131,   132,   132,   133,   133,   133,   134,   135,   135,
     135,   136,   136,   137,   137,   137,   138,   138,   139,   139,
     140,   140,   141,   141,   141,   141,   142,   142,   143,   143,
     143,   143,   143,   143,   144,   144,   145,   145,   146,   146,
     146,   146,   147,   147,   148,   148,   148,   148,   149,   149,
     150,   150,   150,   151,   151,   152,   152,   153,   153,   154,
     154,   155,   156,   156,   157,   157,   158,   158,   159,   159,
     160,   161,   162,   162,   163,   163,   164,   164,   164,   164,
     164,   164,   164,   165,   165,   165,   165,   165,   165,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   167,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   167,   167,
     167,   168,   168,   168,   168,   168,   168,   168,   169,   170,
     170,   171
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     2,     1,     1,     2,     1,     1,
       2,     1,     3,     1,     6,     1,     3,     7,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     1,     3,     0,
       5,     0,     2,     0,     1,     0,     2,     1,     0,     1,
       1,     2,     1,     2,     0,     2,     0,     4,     1,     3,
       3,     4,     4,     5,     7,     7,     4,     5,     5,     5,
       5,     2,     3,     3,     3,     2,     3,     3,     3,     2,
       2,     0,     1,     3,     1,     1,     1,    10,     1,     1,
       0,     1,     3,     2,     3,     1,     1,     0,     2,     1,
       2,     0,     4,     6,     6,     2,     4,     0,     1,     2,
       2,     3,     2,     3,     2,     0,     4,     0,     4,     4,
       3,     0,     3,     0,     2,     4,     4,     0,     2,     0,
       8,     7,     7,     1,     0,     1,     0,     1,     1,     1,
       3,     3,     1,     3,     1,     1,     5,     0,     1,     3,
       3,     7,     2,     0,     1,     3,     3,     3,     3,     2,
       2,     3,     1,     3,     4,     3,     4,     4,     1,     4,
       3,     6,     5,     6,     5,     4,     3,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       1,     1,     1,     1,     1,     1,     2,     2,     3,     1,
       3,     1
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
#line 144 "sql.y" /* yacc.c:1660  */
    {
    (yyval.block) = ctx->factory->NewBlock();
    (yyval.block)->AddStmt((yyvsp[0].stmt));
    ctx->block = (yyval.block);
}
#line 1797 "sql.cc" /* yacc.c:1660  */
    break;

  case 3:
#line 149 "sql.y" /* yacc.c:1660  */
    {
    (yyval.block)->AddStmt((yyvsp[0].stmt));
    ctx->block = (yyval.block);
}
#line 1806 "sql.cc" /* yacc.c:1660  */
    break;

  case 7:
#line 158 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(ast::TCLStatement::TXN_BEGIN);
}
#line 1814 "sql.cc" /* yacc.c:1660  */
    break;

  case 8:
#line 161 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(ast::TCLStatement::TXN_COMMIT);
}
#line 1822 "sql.cc" /* yacc.c:1660  */
    break;

  case 9:
#line 164 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(ast::TCLStatement::TXN_ROLLBACK);
}
#line 1830 "sql.cc" /* yacc.c:1660  */
    break;

  case 10:
#line 167 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewShowTables();
}
#line 1838 "sql.cc" /* yacc.c:1660  */
    break;

  case 11:
#line 172 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1846 "sql.cc" /* yacc.c:1660  */
    break;

  case 12:
#line 175 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewDropTable((yyvsp[0].id));
}
#line 1854 "sql.cc" /* yacc.c:1660  */
    break;

  case 13:
#line 178 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1862 "sql.cc" /* yacc.c:1660  */
    break;

  case 14:
#line 182 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewCreateTable((yyvsp[-3].id), (yyvsp[-1].col_def_list));
}
#line 1870 "sql.cc" /* yacc.c:1660  */
    break;

  case 15:
#line 186 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_def_list) = ctx->factory->NewColumnDefinitionList((yyvsp[0].col_def));
}
#line 1878 "sql.cc" /* yacc.c:1660  */
    break;

  case 16:
#line 189 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_def_list)->push_back((yyvsp[0].col_def));
}
#line 1886 "sql.cc" /* yacc.c:1660  */
    break;

  case 17:
#line 193 "sql.y" /* yacc.c:1660  */
    {
    auto *def = ctx->factory->NewColumnDefinition((yyvsp[-6].name), (yyvsp[-5].type_def));
    def->set_is_not_null((yyvsp[-4].bool_val));
    def->set_auto_increment((yyvsp[-3].bool_val));
    def->set_default_value((yyvsp[-2].expr));
    def->set_key((yyvsp[-1].key_type));
    def->set_comment((yyvsp[0].name));
    (yyval.col_def) = def;
}
#line 1900 "sql.cc" /* yacc.c:1660  */
    break;

  case 18:
#line 203 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_BIGINT, (yyvsp[0].size).fixed_size);
}
#line 1908 "sql.cc" /* yacc.c:1660  */
    break;

  case 19:
#line 206 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_INT, (yyvsp[0].size).fixed_size);
}
#line 1916 "sql.cc" /* yacc.c:1660  */
    break;

  case 20:
#line 209 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_SMALLINT, (yyvsp[0].size).fixed_size);
}
#line 1924 "sql.cc" /* yacc.c:1660  */
    break;

  case 21:
#line 212 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_TINYINT, (yyvsp[0].size).fixed_size);
}
#line 1932 "sql.cc" /* yacc.c:1660  */
    break;

  case 22:
#line 215 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DECIMAL, (yyvsp[0].size).fixed_size, (yyvsp[0].size).float_size);
}
#line 1940 "sql.cc" /* yacc.c:1660  */
    break;

  case 23:
#line 218 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_NUMERIC, (yyvsp[0].size).fixed_size, (yyvsp[0].size).float_size);
}
#line 1948 "sql.cc" /* yacc.c:1660  */
    break;

  case 24:
#line 221 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_NUMERIC, (yyvsp[0].size).fixed_size);
}
#line 1956 "sql.cc" /* yacc.c:1660  */
    break;

  case 25:
#line 224 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_VARCHAR, (yyvsp[0].size).fixed_size);
}
#line 1964 "sql.cc" /* yacc.c:1660  */
    break;

  case 26:
#line 227 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DATE);
}
#line 1972 "sql.cc" /* yacc.c:1660  */
    break;

  case 27:
#line 230 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DATETIME);
}
#line 1980 "sql.cc" /* yacc.c:1660  */
    break;

  case 28:
#line 234 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = (yyvsp[-1].int_val);
    (yyval.size).float_size = 0;
}
#line 1989 "sql.cc" /* yacc.c:1660  */
    break;

  case 29:
#line 238 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = 0;
    (yyval.size).float_size = 0;
}
#line 1998 "sql.cc" /* yacc.c:1660  */
    break;

  case 30:
#line 243 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = (yyvsp[-3].int_val);
    (yyval.size).float_size = (yyvsp[-1].int_val);
}
#line 2007 "sql.cc" /* yacc.c:1660  */
    break;

  case 31:
#line 247 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = 0;
    (yyval.size).float_size = 0;
}
#line 2016 "sql.cc" /* yacc.c:1660  */
    break;

  case 32:
#line 252 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2024 "sql.cc" /* yacc.c:1660  */
    break;

  case 33:
#line 255 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = nullptr;
}
#line 2032 "sql.cc" /* yacc.c:1660  */
    break;

  case 34:
#line 259 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2040 "sql.cc" /* yacc.c:1660  */
    break;

  case 35:
#line 262 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2048 "sql.cc" /* yacc.c:1660  */
    break;

  case 36:
#line 266 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2056 "sql.cc" /* yacc.c:1660  */
    break;

  case 37:
#line 269 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2064 "sql.cc" /* yacc.c:1660  */
    break;

  case 38:
#line 272 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2072 "sql.cc" /* yacc.c:1660  */
    break;

  case 39:
#line 276 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_KEY;
}
#line 2080 "sql.cc" /* yacc.c:1660  */
    break;

  case 40:
#line 279 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_UNIQUE_KEY;
}
#line 2088 "sql.cc" /* yacc.c:1660  */
    break;

  case 41:
#line 282 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_UNIQUE_KEY;
}
#line 2096 "sql.cc" /* yacc.c:1660  */
    break;

  case 42:
#line 285 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_PRIMARY_KEY;
}
#line 2104 "sql.cc" /* yacc.c:1660  */
    break;

  case 43:
#line 288 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_PRIMARY_KEY;
}
#line 2112 "sql.cc" /* yacc.c:1660  */
    break;

  case 44:
#line 291 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_NOT_KEY;
}
#line 2120 "sql.cc" /* yacc.c:1660  */
    break;

  case 45:
#line 295 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = ctx->factory->NewString((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 2128 "sql.cc" /* yacc.c:1660  */
    break;

  case 46:
#line 298 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = AstString::kEmpty;
}
#line 2136 "sql.cc" /* yacc.c:1660  */
    break;

  case 47:
#line 302 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewAlterTable((yyvsp[-1].id), (yyvsp[0].alter_table_spce_list));
}
#line 2144 "sql.cc" /* yacc.c:1660  */
    break;

  case 48:
#line 306 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce_list) = ctx->factory->NewAlterTableSpecList((yyvsp[0].alter_table_spce));
}
#line 2152 "sql.cc" /* yacc.c:1660  */
    break;

  case 49:
#line 309 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce_list)->push_back((yyvsp[0].alter_table_spce));
}
#line 2160 "sql.cc" /* yacc.c:1660  */
    break;

  case 50:
#line 313 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2168 "sql.cc" /* yacc.c:1660  */
    break;

  case 51:
#line 316 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2176 "sql.cc" /* yacc.c:1660  */
    break;

  case 52:
#line 319 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def_list));
}
#line 2184 "sql.cc" /* yacc.c:1660  */
    break;

  case 53:
#line 322 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def_list));
}
#line 2192 "sql.cc" /* yacc.c:1660  */
    break;

  case 54:
#line 325 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddIndex((yyvsp[-4].name), (yyvsp[-3].key_type) == SQL_NOT_KEY ? SQL_KEY : (yyvsp[-3].key_type),
                                             (yyvsp[-1].name_list));
}
#line 2201 "sql.cc" /* yacc.c:1660  */
    break;

  case 55:
#line 329 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddIndex((yyvsp[-4].name), (yyvsp[-3].key_type) == SQL_NOT_KEY ? SQL_KEY : (yyvsp[-3].key_type),
                                             (yyvsp[-1].name_list));
}
#line 2210 "sql.cc" /* yacc.c:1660  */
    break;

  case 56:
#line 333 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableChangeColumn((yyvsp[-2].name), (yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2218 "sql.cc" /* yacc.c:1660  */
    break;

  case 57:
#line 336 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableChangeColumn((yyvsp[-2].name), (yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2226 "sql.cc" /* yacc.c:1660  */
    break;

  case 58:
#line 339 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameColumn((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2234 "sql.cc" /* yacc.c:1660  */
    break;

  case 59:
#line 342 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameIndex((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2242 "sql.cc" /* yacc.c:1660  */
    break;

  case 60:
#line 345 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameIndex((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2250 "sql.cc" /* yacc.c:1660  */
    break;

  case 61:
#line 348 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2258 "sql.cc" /* yacc.c:1660  */
    break;

  case 62:
#line 351 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2266 "sql.cc" /* yacc.c:1660  */
    break;

  case 63:
#line 354 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2274 "sql.cc" /* yacc.c:1660  */
    break;

  case 64:
#line 357 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropColumn((yyvsp[0].name));
}
#line 2282 "sql.cc" /* yacc.c:1660  */
    break;

  case 65:
#line 360 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropColumn((yyvsp[0].name));
}
#line 2290 "sql.cc" /* yacc.c:1660  */
    break;

  case 66:
#line 363 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex((yyvsp[0].name), false);
}
#line 2298 "sql.cc" /* yacc.c:1660  */
    break;

  case 67:
#line 366 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex((yyvsp[0].name), false);
}
#line 2306 "sql.cc" /* yacc.c:1660  */
    break;

  case 68:
#line 369 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex(AstString::kEmpty, true);
}
#line 2314 "sql.cc" /* yacc.c:1660  */
    break;

  case 69:
#line 373 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = (yyvsp[0].name);
    (yyval.col_pos).after = false;
}
#line 2323 "sql.cc" /* yacc.c:1660  */
    break;

  case 70:
#line 377 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = (yyvsp[0].name);
    (yyval.col_pos).after = true;
}
#line 2332 "sql.cc" /* yacc.c:1660  */
    break;

  case 71:
#line 381 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = AstString::kEmpty;
    (yyval.col_pos).after = false;
}
#line 2341 "sql.cc" /* yacc.c:1660  */
    break;

  case 72:
#line 386 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name_list) = ctx->factory->NewNameList((yyvsp[0].name));
}
#line 2349 "sql.cc" /* yacc.c:1660  */
    break;

  case 73:
#line 389 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name_list)->push_back((yyvsp[0].name));
}
#line 2357 "sql.cc" /* yacc.c:1660  */
    break;

  case 77:
#line 400 "sql.y" /* yacc.c:1660  */
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
#line 2375 "sql.cc" /* yacc.c:1660  */
    break;

  case 78:
#line 414 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2383 "sql.cc" /* yacc.c:1660  */
    break;

  case 79:
#line 417 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2391 "sql.cc" /* yacc.c:1660  */
    break;

  case 80:
#line 420 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2399 "sql.cc" /* yacc.c:1660  */
    break;

  case 81:
#line 424 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col_list) = ctx->factory->NewProjectionColumnList((yyvsp[0].proj_col));
}
#line 2407 "sql.cc" /* yacc.c:1660  */
    break;

  case 82:
#line 427 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col_list)->push_back((yyvsp[0].proj_col));
}
#line 2415 "sql.cc" /* yacc.c:1660  */
    break;

  case 83:
#line 431 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col) = ctx->factory->NewProjectionColumn((yyvsp[-1].expr), (yyvsp[0].name),
                                           Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2424 "sql.cc" /* yacc.c:1660  */
    break;

  case 84:
#line 435 "sql.y" /* yacc.c:1660  */
    {
    Identifier *id = ctx->factory->NewIdentifierWithPlaceholder((yyvsp[-2].name),
        ctx->factory->NewStarPlaceholder((yylsp[0])),
        Location::Concat((yylsp[-2]), (yylsp[0])));
    (yyval.proj_col) = ctx->factory->NewProjectionColumn(id, AstString::kEmpty,
        Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2436 "sql.cc" /* yacc.c:1660  */
    break;

  case 85:
#line 442 "sql.y" /* yacc.c:1660  */
    {
    Placeholder *ph = ctx->factory->NewStarPlaceholder((yylsp[0]));
    (yyval.proj_col) = ctx->factory->NewProjectionColumn(ph, AstString::kEmpty, (yylsp[0]));
}
#line 2445 "sql.cc" /* yacc.c:1660  */
    break;

  case 87:
#line 449 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = AstString::kEmpty;
}
#line 2453 "sql.cc" /* yacc.c:1660  */
    break;

  case 88:
#line 453 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = (yyvsp[0].name);
}
#line 2461 "sql.cc" /* yacc.c:1660  */
    break;

  case 89:
#line 456 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = (yyvsp[0].name);
}
#line 2469 "sql.cc" /* yacc.c:1660  */
    break;

  case 90:
#line 460 "sql.y" /* yacc.c:1660  */
    {
    (yyval.query) = (yyvsp[0].query);
}
#line 2477 "sql.cc" /* yacc.c:1660  */
    break;

  case 91:
#line 463 "sql.y" /* yacc.c:1660  */
    {
    (yyval.query) = nullptr;
}
#line 2485 "sql.cc" /* yacc.c:1660  */
    break;

  case 92:
#line 467 "sql.y" /* yacc.c:1660  */
    {
    Query *query = ::mai::down_cast<Query>((yyvsp[-2].stmt));
    query->set_alias((yyvsp[0].name));
    (yyval.query) = query;
}
#line 2495 "sql.cc" /* yacc.c:1660  */
    break;

  case 93:
#line 472 "sql.y" /* yacc.c:1660  */
    {
    (yyvsp[-5].query)->set_alias((yyvsp[-4].name));
    (yyvsp[-2].query)->set_alias((yyvsp[-1].name));
    (yyval.query) = ctx->factory->NewJoinRelation((yyvsp[-5].query), SQL_CROSS_JOIN, (yyvsp[-2].query), (yyvsp[0].expr),
        AstString::kEmpty);
}
#line 2506 "sql.cc" /* yacc.c:1660  */
    break;

  case 94:
#line 478 "sql.y" /* yacc.c:1660  */
    {
    (yyvsp[-5].query)->set_alias((yyvsp[-4].name));
    (yyvsp[-2].query)->set_alias((yyvsp[-1].name));
    (yyval.query) = ctx->factory->NewJoinRelation((yyvsp[-5].query), (yyvsp[-3].join_kind), (yyvsp[-2].query), (yyvsp[0].expr), AstString::kEmpty);
}
#line 2516 "sql.cc" /* yacc.c:1660  */
    break;

  case 95:
#line 483 "sql.y" /* yacc.c:1660  */
    {
    (yyval.query) = ctx->factory->NewNameRelation((yyvsp[-1].id), (yyvsp[0].name));
}
#line 2524 "sql.cc" /* yacc.c:1660  */
    break;

  case 96:
#line 487 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2532 "sql.cc" /* yacc.c:1660  */
    break;

  case 97:
#line 490 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = nullptr;
}
#line 2540 "sql.cc" /* yacc.c:1660  */
    break;

  case 98:
#line 494 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_CROSS_JOIN;
}
#line 2548 "sql.cc" /* yacc.c:1660  */
    break;

  case 99:
#line 497 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_CROSS_JOIN;
}
#line 2556 "sql.cc" /* yacc.c:1660  */
    break;

  case 100:
#line 500 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_LEFT_OUTTER_JOIN;
}
#line 2564 "sql.cc" /* yacc.c:1660  */
    break;

  case 101:
#line 503 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_LEFT_OUTTER_JOIN;
}
#line 2572 "sql.cc" /* yacc.c:1660  */
    break;

  case 102:
#line 506 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_RIGHT_OUTTER_JOIN;
}
#line 2580 "sql.cc" /* yacc.c:1660  */
    break;

  case 103:
#line 509 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_RIGHT_OUTTER_JOIN;
}
#line 2588 "sql.cc" /* yacc.c:1660  */
    break;

  case 104:
#line 513 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2596 "sql.cc" /* yacc.c:1660  */
    break;

  case 105:
#line 516 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = nullptr;
}
#line 2604 "sql.cc" /* yacc.c:1660  */
    break;

  case 106:
#line 520 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2612 "sql.cc" /* yacc.c:1660  */
    break;

  case 107:
#line 523 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = nullptr;
}
#line 2620 "sql.cc" /* yacc.c:1660  */
    break;

  case 108:
#line 527 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = (yyvsp[-1].expr_list);
    (yyval.order_by).desc = false;
}
#line 2629 "sql.cc" /* yacc.c:1660  */
    break;

  case 109:
#line 531 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = (yyvsp[-1].expr_list);
    (yyval.order_by).desc = true;
}
#line 2638 "sql.cc" /* yacc.c:1660  */
    break;

  case 110:
#line 535 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = (yyvsp[0].expr_list);
    (yyval.order_by).desc = false;
}
#line 2647 "sql.cc" /* yacc.c:1660  */
    break;

  case 111:
#line 539 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = nullptr;
    (yyval.order_by).desc = false;
}
#line 2656 "sql.cc" /* yacc.c:1660  */
    break;

  case 112:
#line 544 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = (yyvsp[0].expr_list);
}
#line 2664 "sql.cc" /* yacc.c:1660  */
    break;

  case 113:
#line 547 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = nullptr;
}
#line 2672 "sql.cc" /* yacc.c:1660  */
    break;

  case 114:
#line 551 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
    (yyval.limit).offset_val = 0;
}
#line 2681 "sql.cc" /* yacc.c:1660  */
    break;

  case 115:
#line 555 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).offset_val = (yyvsp[-2].int_val);
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
}
#line 2690 "sql.cc" /* yacc.c:1660  */
    break;

  case 116:
#line 559 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = (yyvsp[-2].int_val);
    (yyval.limit).offset_val = (yyvsp[0].int_val);
}
#line 2699 "sql.cc" /* yacc.c:1660  */
    break;

  case 117:
#line 563 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = 0;
    (yyval.limit).offset_val = 0;
}
#line 2708 "sql.cc" /* yacc.c:1660  */
    break;

  case 118:
#line 569 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2716 "sql.cc" /* yacc.c:1660  */
    break;

  case 119:
#line 572 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2724 "sql.cc" /* yacc.c:1660  */
    break;

  case 120:
#line 576 "sql.y" /* yacc.c:1660  */
    {
    Insert *stmt = ctx->factory->NewInsert((yyvsp[-6].bool_val), (yyvsp[-4].id));
    stmt->set_col_names((yyvsp[-3].name_list));
    stmt->set_row_values_list((yyvsp[-1].row_vals_list));
    stmt->set_on_duplicate_clause((yyvsp[0].assignment_list));
    (yyval.stmt) = stmt;
}
#line 2736 "sql.cc" /* yacc.c:1660  */
    break;

  case 121:
#line 583 "sql.y" /* yacc.c:1660  */
    {
    Insert *stmt = ctx->factory->NewInsert((yyvsp[-5].bool_val), (yyvsp[-3].id));
    stmt->SetAssignmentList((yyvsp[-1].assignment_list), ctx->factory->arena());
    stmt->set_on_duplicate_clause((yyvsp[0].assignment_list));
    (yyval.stmt) = stmt;
}
#line 2747 "sql.cc" /* yacc.c:1660  */
    break;

  case 122:
#line 589 "sql.y" /* yacc.c:1660  */
    {
    Insert *stmt = ctx->factory->NewInsert((yyvsp[-5].bool_val), (yyvsp[-3].id));
    stmt->set_col_names((yyvsp[-2].name_list));
    stmt->set_select_clause(::mai::down_cast<Query>((yyvsp[-1].stmt)));
    stmt->set_on_duplicate_clause((yyvsp[0].assignment_list));
    (yyval.stmt) = stmt;
}
#line 2759 "sql.cc" /* yacc.c:1660  */
    break;

  case 124:
#line 598 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name_list) = nullptr;
}
#line 2767 "sql.cc" /* yacc.c:1660  */
    break;

  case 125:
#line 602 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2775 "sql.cc" /* yacc.c:1660  */
    break;

  case 126:
#line 605 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2783 "sql.cc" /* yacc.c:1660  */
    break;

  case 129:
#line 612 "sql.y" /* yacc.c:1660  */
    {
    (yyval.row_vals_list) = ctx->factory->NewRowValuesList((yyvsp[0].expr_list));
}
#line 2791 "sql.cc" /* yacc.c:1660  */
    break;

  case 130:
#line 615 "sql.y" /* yacc.c:1660  */
    {
    (yyval.row_vals_list)->push_back((yyvsp[0].expr_list));
}
#line 2799 "sql.cc" /* yacc.c:1660  */
    break;

  case 131:
#line 619 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = (yyvsp[-1].expr_list);
}
#line 2807 "sql.cc" /* yacc.c:1660  */
    break;

  case 132:
#line 623 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = ctx->factory->NewExpressionList((yyvsp[0].expr));
}
#line 2815 "sql.cc" /* yacc.c:1660  */
    break;

  case 133:
#line 626 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list)->push_back((yyvsp[0].expr));
}
#line 2823 "sql.cc" /* yacc.c:1660  */
    break;

  case 135:
#line 631 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewDefaultPlaceholderLiteral((yylsp[0]));
}
#line 2831 "sql.cc" /* yacc.c:1660  */
    break;

  case 136:
#line 635 "sql.y" /* yacc.c:1660  */
    {
    (yyval.assignment_list) = (yyvsp[0].assignment_list);
}
#line 2839 "sql.cc" /* yacc.c:1660  */
    break;

  case 137:
#line 638 "sql.y" /* yacc.c:1660  */
    {
    (yyval.assignment_list) = nullptr;
}
#line 2847 "sql.cc" /* yacc.c:1660  */
    break;

  case 138:
#line 642 "sql.y" /* yacc.c:1660  */
    {
    (yyval.assignment_list) = ctx->factory->NewAssignmentList((yyvsp[0].assignment));
}
#line 2855 "sql.cc" /* yacc.c:1660  */
    break;

  case 139:
#line 645 "sql.y" /* yacc.c:1660  */
    {
    (yyval.assignment_list)->push_back((yyvsp[0].assignment));
}
#line 2863 "sql.cc" /* yacc.c:1660  */
    break;

  case 140:
#line 649 "sql.y" /* yacc.c:1660  */
    {
    if ((yyvsp[-1].op) != SQL_CMP_EQ) {
        yyerror(&(yylsp[-2]), ctx, "incorrect assignment.");
        YYERROR;
    }
    (yyval.assignment) = ctx->factory->NewAssignment((yyvsp[-2].name), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2875 "sql.cc" /* yacc.c:1660  */
    break;

  case 141:
#line 658 "sql.y" /* yacc.c:1660  */
    {
    Update *stmt = ctx->factory->NewUpdate((yyvsp[-5].id), (yyvsp[-3].assignment_list));
    stmt->set_where_clause((yyvsp[-2].expr));
    stmt->set_order_by_desc((yyvsp[-1].order_by).desc);
    stmt->set_order_by_clause((yyvsp[-1].order_by).expr_list);
    stmt->set_limit_val((yyvsp[0].limit).limit_val);
    (yyval.stmt) = stmt;
}
#line 2888 "sql.cc" /* yacc.c:1660  */
    break;

  case 142:
#line 667 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
    (yyval.limit).offset_val = 0;
}
#line 2897 "sql.cc" /* yacc.c:1660  */
    break;

  case 143:
#line 671 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = 0;
    (yyval.limit).offset_val = 0;
}
#line 2906 "sql.cc" /* yacc.c:1660  */
    break;

  case 144:
#line 681 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = ctx->factory->NewExpressionList((yyvsp[0].expr));
}
#line 2914 "sql.cc" /* yacc.c:1660  */
    break;

  case 145:
#line 684 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list)->push_back((yyvsp[0].expr));
}
#line 2922 "sql.cc" /* yacc.c:1660  */
    break;

  case 146:
#line 691 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_OR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2930 "sql.cc" /* yacc.c:1660  */
    break;

  case 147:
#line 694 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_XOR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2938 "sql.cc" /* yacc.c:1660  */
    break;

  case 148:
#line 697 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_AND, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2946 "sql.cc" /* yacc.c:1660  */
    break;

  case 149:
#line 700 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_NOT, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2954 "sql.cc" /* yacc.c:1660  */
    break;

  case 150:
#line 703 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_NOT, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2962 "sql.cc" /* yacc.c:1660  */
    break;

  case 151:
#line 706 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2970 "sql.cc" /* yacc.c:1660  */
    break;

  case 153:
#line 714 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_IS_NULL, (yyvsp[-2].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2978 "sql.cc" /* yacc.c:1660  */
    break;

  case 154:
#line 717 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_IS_NOT_NULL, (yyvsp[-3].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2986 "sql.cc" /* yacc.c:1660  */
    break;

  case 155:
#line 720 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-2].expr), (yyvsp[-1].op), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2994 "sql.cc" /* yacc.c:1660  */
    break;

  case 156:
#line 723 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-3].expr), (yyvsp[-2].op), (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3002 "sql.cc" /* yacc.c:1660  */
    break;

  case 157:
#line 726 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-3].expr), (yyvsp[-2].op), (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3010 "sql.cc" /* yacc.c:1660  */
    break;

  case 159:
#line 734 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-3].expr), SQL_NOT_IN, (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3018 "sql.cc" /* yacc.c:1660  */
    break;

  case 160:
#line 737 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-2].expr), SQL_IN, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3026 "sql.cc" /* yacc.c:1660  */
    break;

  case 161:
#line 740 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-5].expr), SQL_NOT_IN, (yyvsp[-1].expr_list), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 3034 "sql.cc" /* yacc.c:1660  */
    break;

  case 162:
#line 743 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-4].expr), SQL_IN, (yyvsp[-1].expr_list), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 3042 "sql.cc" /* yacc.c:1660  */
    break;

  case 163:
#line 746 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-5].expr), SQL_NOT_BETWEEN, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 3050 "sql.cc" /* yacc.c:1660  */
    break;

  case 164:
#line 749 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-4].expr), SQL_BETWEEN, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 3058 "sql.cc" /* yacc.c:1660  */
    break;

  case 165:
#line 752 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-3].expr), SQL_NOT_LIKE, (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3066 "sql.cc" /* yacc.c:1660  */
    break;

  case 166:
#line 755 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_LIKE, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3074 "sql.cc" /* yacc.c:1660  */
    break;

  case 168:
#line 764 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_OR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3082 "sql.cc" /* yacc.c:1660  */
    break;

  case 169:
#line 767 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_AND, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3090 "sql.cc" /* yacc.c:1660  */
    break;

  case 170:
#line 770 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_LSHIFT, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3098 "sql.cc" /* yacc.c:1660  */
    break;

  case 171:
#line 773 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_RSHIFT, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3106 "sql.cc" /* yacc.c:1660  */
    break;

  case 172:
#line 776 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_PLUS, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3114 "sql.cc" /* yacc.c:1660  */
    break;

  case 173:
#line 779 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_SUB, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3122 "sql.cc" /* yacc.c:1660  */
    break;

  case 174:
#line 782 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MUL, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3130 "sql.cc" /* yacc.c:1660  */
    break;

  case 175:
#line 785 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_DIV, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3138 "sql.cc" /* yacc.c:1660  */
    break;

  case 176:
#line 788 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_DIV, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3146 "sql.cc" /* yacc.c:1660  */
    break;

  case 177:
#line 791 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MOD, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3154 "sql.cc" /* yacc.c:1660  */
    break;

  case 178:
#line 794 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MOD, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3162 "sql.cc" /* yacc.c:1660  */
    break;

  case 179:
#line 797 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_XOR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3170 "sql.cc" /* yacc.c:1660  */
    break;

  case 181:
#line 806 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[0].id);
}
#line 3178 "sql.cc" /* yacc.c:1660  */
    break;

  case 182:
#line 809 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewStringLiteral((yyvsp[0].text).buf, (yyvsp[0].text).len, (yylsp[0]));
}
#line 3186 "sql.cc" /* yacc.c:1660  */
    break;

  case 183:
#line 812 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewIntegerLiteral((yyvsp[0].int_val), (yylsp[0]));
}
#line 3194 "sql.cc" /* yacc.c:1660  */
    break;

  case 184:
#line 815 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewApproxLiteral((yyvsp[0].approx_val), (yylsp[0]));
}
#line 3202 "sql.cc" /* yacc.c:1660  */
    break;

  case 185:
#line 818 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewParamPlaceholder((yylsp[0]));
}
#line 3210 "sql.cc" /* yacc.c:1660  */
    break;

  case 186:
#line 821 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_MINUS, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 3218 "sql.cc" /* yacc.c:1660  */
    break;

  case 187:
#line 824 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_BIT_INV, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 3226 "sql.cc" /* yacc.c:1660  */
    break;

  case 188:
#line 829 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewSubquery(true, ::mai::down_cast<Query>((yyvsp[-1].stmt)), (yylsp[-1]));
}
#line 3234 "sql.cc" /* yacc.c:1660  */
    break;

  case 189:
#line 833 "sql.y" /* yacc.c:1660  */
    {
    (yyval.id) = ctx->factory->NewIdentifier(AstString::kEmpty, (yyvsp[0].name), (yylsp[0]));
}
#line 3242 "sql.cc" /* yacc.c:1660  */
    break;

  case 190:
#line 836 "sql.y" /* yacc.c:1660  */
    {
    (yyval.id) = ctx->factory->NewIdentifier((yyvsp[-2].name), (yyvsp[0].name), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3250 "sql.cc" /* yacc.c:1660  */
    break;

  case 191:
#line 840 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = ctx->factory->NewString((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 3258 "sql.cc" /* yacc.c:1660  */
    break;


#line 3262 "sql.cc" /* yacc.c:1660  */
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
#line 844 "sql.y" /* yacc.c:1903  */

void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg) {
    ctx->err_line   = yyl->first_line;
    ctx->err_column = yyl->first_column;
    ctx->err_msg    = ctx->factory->NewString(msg);
}
