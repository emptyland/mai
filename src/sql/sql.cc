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
    const ::mai::sql::AstString *name;
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
    ::mai::sql::RowValuesList *row_vals_list;
    ::mai::sql::Assignment *assignment;
    ::mai::sql::AssignmentList *assignment_list;
    ::mai::sql::Identifier *id;

#line 276 "sql.cc" /* yacc.c:353  */
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
#define YYLAST   507

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  112
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  59
/* YYNRULES -- Number of rules.  */
#define YYNRULES  189
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  370

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
       0,   143,   143,   148,   153,   155,   156,   157,   160,   163,
     166,   171,   174,   177,   181,   185,   188,   192,   197,   200,
     203,   206,   209,   212,   215,   218,   221,   224,   228,   232,
     237,   241,   246,   249,   253,   256,   259,   263,   266,   269,
     272,   275,   278,   282,   285,   289,   293,   296,   300,   303,
     306,   309,   312,   316,   320,   323,   326,   329,   332,   335,
     338,   341,   344,   347,   350,   353,   356,   360,   364,   368,
     373,   376,   383,   384,   385,   387,   401,   404,   407,   411,
     414,   418,   422,   429,   435,   436,   440,   443,   447,   450,
     454,   459,   465,   470,   474,   477,   481,   484,   487,   490,
     493,   496,   500,   503,   507,   510,   514,   518,   522,   526,
     531,   534,   538,   542,   546,   550,   556,   559,   563,   570,
     576,   584,   585,   589,   592,   596,   597,   599,   602,   606,
     610,   613,   617,   618,   622,   625,   629,   632,   636,   644,
     649,   653,   663,   666,   673,   676,   679,   682,   685,   688,
     691,   696,   699,   702,   705,   708,   711,   716,   719,   722,
     725,   728,   731,   734,   737,   740,   746,   749,   752,   755,
     758,   761,   764,   767,   770,   773,   776,   779,   782,   788,
     791,   794,   797,   800,   803,   806,   811,   815,   818,   822
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
  "FixedSizeDescription", "FloatingSizeDescription", "AutoIncrementOption",
  "NullOption", "KeyOption", "CommentOption", "AlterTableStmt",
  "AlterTableSpecList", "AlterTableSpec", "AlterColPosOption", "NameList",
  "DML", "SelectStmt", "DistinctOption", "ProjectionColumnList",
  "ProjectionColumn", "AliasOption", "Alias", "FromClause", "Relation",
  "OnClause", "JoinOp", "WhereClause", "HavingClause", "OrderByClause",
  "GroupByClause", "LimitOffsetClause", "ForUpdateOption", "InsertStmt",
  "NameListOption", "OverwriteOption", "ValueToken", "RowValuesList",
  "RowValues", "ValueList", "Value", "OnDuplicateClause", "AssignmentList",
  "Assignment", "UpdateStmt", "UpdateLimitOption", "ExpressionList",
  "Expression", "BoolPrimary", "Predicate", "BitExpression", "Simple",
  "Subquery", "Identifier", "Name", YY_NULLPTR
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

#define YYPACT_NINF -232

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-232)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     426,    28,    40,    47,    58,    65,    68,  -232,  -232,    50,
      90,   418,  -232,    71,  -232,  -232,  -232,  -232,  -232,  -232,
    -232,  -232,  -232,   274,    50,    50,  -232,    50,  -232,  -232,
     125,    86,  -232,   154,  -232,  -232,  -232,  -232,  -232,  -232,
     293,   293,   129,  -232,   293,  -232,   129,     6,  -232,   136,
     151,  -232,   156,  -232,  -232,   114,   128,  -232,   277,    50,
      50,    50,   187,   187,  -232,   290,  -232,   -34,   274,   251,
      50,   293,   293,   293,   111,  -232,  -232,  -232,   -21,   129,
     129,   142,   109,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   -33,    50,   216,     3,
     341,    18,   189,  -232,    -2,  -232,   203,  -232,   232,  -232,
     261,    10,    10,  -232,   293,   265,  -232,   187,   213,   -18,
     196,   196,  -232,   247,  -232,   228,    57,  -232,   233,   129,
     129,   228,   172,   249,     2,    59,    59,    72,    72,     5,
       5,     5,     5,   327,  -232,   -71,  -232,   412,   337,    50,
      50,    50,  -232,    50,   -29,    50,    50,   107,    50,    50,
      50,    50,    50,  -232,    50,    50,   277,    50,   265,   119,
      50,   252,    22,  -232,   256,   176,  -232,   208,   329,   334,
     261,  -232,  -232,  -232,   267,    23,   208,    57,  -232,   228,
     217,   129,  -232,    50,   275,   275,   275,   275,   280,   280,
     275,   275,  -232,  -232,    -4,  -232,  -232,  -232,  -232,   350,
      50,   107,   350,    45,    50,    50,  -232,   363,   369,  -232,
    -232,   371,    50,   107,  -232,  -232,   352,  -232,  -232,  -232,
       7,    50,  -232,  -232,   388,   318,    10,  -232,   133,   271,
     392,   -34,   -34,   293,   390,   399,  -232,  -232,   293,    94,
     129,  -232,  -232,   376,  -232,  -232,  -232,  -232,   378,  -232,
    -232,  -232,  -232,  -232,   372,   365,   427,   430,  -232,   342,
     183,  -232,   347,  -232,  -232,  -232,    50,    50,    50,   107,
    -232,   395,  -232,   396,  -232,  -232,  -232,   119,     8,  -232,
    -232,  -232,   425,  -232,   428,  -232,    10,    10,   -15,   293,
     357,   416,   208,  -232,  -232,   360,   361,  -232,  -232,   350,
    -232,  -232,    50,  -232,    50,  -232,  -232,  -232,  -232,  -232,
     452,   225,  -232,   318,  -232,  -232,  -232,   170,   170,  -232,
    -232,   364,   293,   409,   429,  -232,   417,   398,   270,   287,
     431,  -232,   119,  -232,   368,  -232,  -232,   370,   -14,   445,
    -232,   386,   432,  -232,  -232,  -232,    50,  -232,   293,  -232,
     434,   435,  -232,  -232,  -232,   391,   374,  -232,  -232,  -232
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    78,     0,     0,     0,     0,     0,     8,     9,     0,
     124,     0,     2,     0,     5,    11,    13,     6,    72,    73,
      74,    76,    77,     0,     0,     0,    10,     0,     7,   189,
       0,   187,   123,     0,     1,     3,     4,   181,   180,   182,
       0,     0,     0,    83,     0,   183,     0,    89,    79,    85,
     150,   156,   165,   178,   179,   187,     0,    12,     0,     0,
       0,     0,   147,   148,   184,     0,   185,     0,     0,   103,
       0,     0,     0,     0,     0,    81,    84,    87,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    45,    46,   103,   136,     0,   188,   122,   149,
       0,    88,    85,    80,     0,   109,    86,   146,   144,   145,
       0,     0,   151,     0,   153,   174,     0,   158,     0,     0,
       0,   164,     0,   166,   167,   168,   169,   170,   171,   172,
     173,   175,   176,   177,    82,     0,    15,     0,     0,     0,
       0,     0,    63,     0,     0,     0,     0,    69,     0,     0,
       0,     0,     0,    59,     0,     0,     0,     0,   109,     0,
       0,   121,     0,    70,     0,     0,    93,   102,     0,   111,
       0,   155,   154,   152,     0,     0,   142,     0,   157,   163,
       0,     0,    14,     0,    29,    29,    29,    29,    31,    31,
      29,    29,    26,    27,    36,    66,    65,    62,    64,    42,
       0,    69,    42,     0,     0,     0,    48,     0,     0,    60,
      61,     0,     0,    69,    47,   137,   141,   133,   138,   132,
     135,     0,   126,   125,   135,     0,     0,    96,     0,     0,
       0,     0,     0,     0,     0,   105,   186,   160,     0,     0,
       0,   162,    16,     0,    18,    19,    20,    21,     0,    22,
      23,    24,    25,    35,     0,    33,    38,    40,    37,     0,
       0,    49,     0,    50,    68,    67,     0,     0,     0,    69,
      54,     0,   139,     0,   119,    71,   120,     0,   135,   127,
      90,    98,     0,   100,     0,    97,     0,     0,   108,     0,
       0,   115,   143,   159,   161,     0,     0,    34,    32,    42,
      39,    41,     0,    51,     0,    58,    56,    57,    55,   140,
       0,     0,   130,     0,   118,    99,   101,    95,    95,   106,
     107,   110,     0,     0,   117,    28,     0,    44,     0,     0,
       0,   129,     0,   128,     0,    91,    92,     0,   112,     0,
      75,     0,     0,    17,    53,    52,     0,   131,     0,   104,
       0,     0,   116,    30,    43,   134,     0,   114,   113,    94
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -232,  -232,   483,  -232,  -232,  -232,   -97,   -86,  -232,   192,
     299,  -232,  -232,  -197,  -232,  -232,  -232,   335,  -189,  -225,
    -232,    12,  -232,  -232,   436,   393,  -109,  -232,   178,   174,
    -232,   402,  -232,   332,  -232,  -232,  -232,  -232,  -232,  -232,
    -232,  -232,   180,  -232,  -231,  -165,  -166,   340,  -232,  -232,
    -181,   -23,  -232,   -65,   322,   -41,    98,    -6,   -16
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    11,    12,    13,    14,    15,   145,   146,   204,   254,
     259,   309,   265,   269,   353,    16,   102,   103,   216,   171,
      17,   184,    23,    47,    48,    75,    76,    69,   111,   345,
     242,   115,   301,   179,   245,   334,   350,    19,   172,    33,
     235,   288,   289,   321,   228,   284,   104,   105,    20,   282,
     185,   186,    50,    51,    52,    53,   127,    54,    31
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint16 yytable[] =
{
      49,    64,   175,    30,   230,    66,   249,    55,    56,    57,
      67,    58,    18,   157,   124,   272,    80,    62,    63,    80,
     153,    65,   271,    18,   154,     1,    29,    29,   329,   330,
     114,    29,   155,    77,   280,   360,   192,   193,    70,   164,
     122,   283,   283,   106,   107,    49,    24,   123,   117,   118,
     119,    71,    55,    25,   116,   108,   322,   263,    21,   213,
       1,   112,   298,    29,   264,    26,   144,    22,   211,   286,
      29,    27,   110,    80,    74,   232,   233,   210,    29,   223,
     107,   147,   152,   147,   163,   165,    80,   338,    28,   339,
     318,   177,   173,   248,   361,    77,    77,    87,    88,    89,
      90,    91,    92,    93,    94,    95,   167,   252,    95,   156,
      29,   357,   337,   270,    68,   167,   323,    29,   331,    37,
      38,    39,   174,   324,   120,    40,   251,   290,   229,   214,
     247,   248,   215,   206,   207,   208,   279,   209,   147,   212,
     147,    32,   217,   218,   219,   220,   221,    41,   222,   147,
     121,   106,   273,   193,   106,    42,    89,    90,    91,    92,
      93,    94,    95,    44,    70,   128,   291,    45,    46,   292,
      80,    91,    92,    93,    94,    95,    36,   147,   227,    29,
      59,    37,    38,    39,   234,   304,    80,   327,   328,    29,
     365,    37,    38,    39,   147,    60,    29,   129,   274,   275,
     130,   303,   248,   237,   344,    71,   147,   238,   239,   237,
     240,    61,    81,   238,   239,   285,   240,    42,   181,   182,
      77,    72,    73,    96,    82,   302,   188,    42,    74,    45,
      46,    80,   148,   149,    97,   112,   112,   150,    78,    45,
      46,   191,    80,    79,    83,   151,   229,    84,   126,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
     315,   316,   317,    80,     1,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    29,    71,   241,    74,
      77,    77,    71,   114,   241,    98,   250,   170,    99,   100,
     313,   193,    29,    72,    73,   169,   173,   166,   173,    73,
      74,   229,   180,   101,   293,    74,   178,   294,   183,   347,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,   341,   342,    29,   366,    37,    38,    39,   187,
     106,    80,    40,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    29,   205,    37,    38,    39,   158,    71,
     231,    40,   159,   236,    41,   266,   267,   268,   160,   161,
     162,   243,    42,    43,   246,    72,    73,   354,   231,   244,
      44,   253,    74,    41,    45,    46,   258,   255,   256,   257,
     276,    42,   261,   262,   355,   231,   277,   109,   278,    44,
     281,    29,   125,    45,    46,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,    34,   296,
     297,     1,   283,     2,   287,   295,     3,     4,     5,     1,
     300,     2,   299,   307,     3,     4,     5,     6,   305,    71,
     306,     7,     8,    71,   310,     6,   308,   311,   312,     7,
       8,   189,   190,   314,   320,    72,    73,   319,   325,    72,
      73,   326,    74,   332,   333,     9,    74,   335,    10,   340,
     336,   348,   248,     9,   358,   349,    10,   359,   356,   351,
     352,   369,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   362,   363,    35,   364,   367,   368,   260,   167,
     226,   224,   346,   343,   113,   176,   168,   225
};

static const yytype_uint16 yycheck[] =
{
      23,    42,   111,     9,   170,    46,   187,    23,    24,    25,
       4,    27,     0,    99,    79,   212,    14,    40,    41,    14,
      17,    44,   211,    11,    21,     3,    60,    60,    43,    44,
      32,    60,    29,    49,   223,    49,   107,   108,    28,    21,
      61,    34,    34,    59,    60,    68,     6,    68,    71,    72,
      73,    69,    68,     6,    70,    61,   287,    61,    30,   156,
       3,    67,   243,    60,    68,     7,    99,    39,   154,   234,
      60,     6,   106,    14,    92,    53,    54,   106,    60,   165,
      96,    97,    98,    99,   100,   101,    14,   312,    20,   314,
     279,   114,   108,   108,   108,   111,   112,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   108,   193,   103,   106,
      60,   342,   309,   210,   108,   108,   108,    60,   299,    62,
      63,    64,   110,   288,    13,    68,   191,   236,   169,    22,
     107,   108,    25,   149,   150,   151,   222,   153,   154,   155,
     156,    51,   158,   159,   160,   161,   162,    90,   164,   165,
      39,   167,   107,   108,   170,    98,    97,    98,    99,   100,
     101,   102,   103,   106,    28,    56,    33,   110,   111,    36,
      14,    99,   100,   101,   102,   103,   105,   193,    59,    60,
      55,    62,    63,    64,   172,   250,    14,   296,   297,    60,
     356,    62,    63,    64,   210,   109,    60,    88,   214,   215,
      91,   107,   108,    33,    34,    69,   222,    37,    38,    33,
      40,    57,    56,    37,    38,   231,    40,    98,   120,   121,
     236,    85,    86,   109,    68,   248,   128,    98,    92,   110,
     111,    14,    16,    17,   106,   241,   242,    21,    87,   110,
     111,    69,    14,    92,    88,    29,   287,    91,   106,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     276,   277,   278,    14,     3,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,    60,    69,   108,    92,
     296,   297,    69,    32,   108,     8,    69,    55,    11,    12,
     107,   108,    60,    85,    86,    92,   312,   108,   314,    86,
      92,   342,   106,    26,    33,    92,    41,    36,    61,   332,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   107,   108,    60,   358,    62,    63,    64,   106,
     356,    14,    68,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,    60,    17,    62,    63,    64,    17,    69,
     108,    68,    21,   107,    90,    15,    16,    17,    27,    28,
      29,    42,    98,    99,   107,    85,    86,   107,   108,    45,
     106,   106,    92,    90,   110,   111,   106,   195,   196,   197,
      27,    98,   200,   201,   107,   108,    27,   107,    27,   106,
      48,    60,    80,   110,   111,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,     0,   241,
     242,     3,    34,     5,   106,    33,     8,     9,    10,     3,
      31,     5,    42,    61,     8,     9,    10,    19,    62,    69,
      62,    23,    24,    69,    17,    19,    81,    17,   106,    23,
      24,   129,   130,   106,    58,    85,    86,    62,    33,    85,
      86,    33,    92,   106,    48,    47,    92,   107,    50,    17,
     109,    62,   108,    47,   106,    46,    50,   107,    47,    62,
      82,   107,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    47,   107,    11,    63,    62,    62,   199,   108,
     168,   166,   328,   323,    68,   112,   104,   167
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     5,     8,     9,    10,    19,    23,    24,    47,
      50,   113,   114,   115,   116,   117,   127,   132,   133,   149,
     160,    30,    39,   134,     6,     6,     7,     6,    20,    60,
     169,   170,    51,   151,     0,   114,   105,    62,    63,    64,
      68,    90,    98,    99,   106,   110,   111,   135,   136,   163,
     164,   165,   166,   167,   169,   170,   170,   170,   170,    55,
     109,    57,   163,   163,   167,   163,   167,     4,   108,   139,
      28,    69,    85,    86,    92,   137,   138,   170,    87,    92,
      14,    56,    68,    88,    91,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   109,   106,     8,    11,
      12,    26,   128,   129,   158,   159,   170,   170,   169,   107,
     106,   140,   169,   136,    32,   143,   170,   163,   163,   163,
      13,    39,    61,    68,   165,   166,   106,   168,    56,    88,
      91,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   166,   166,    99,   118,   119,   170,    16,    17,
      21,    29,   170,    17,    21,    29,   106,   119,    17,    21,
      27,    28,    29,   170,    21,   170,   108,   108,   143,    92,
      55,   131,   150,   170,   133,   138,   137,   163,    41,   145,
     106,   168,   168,    61,   133,   162,   163,   106,   168,   166,
     166,    69,   107,   108,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,   120,    17,   170,   170,   170,   170,
     106,   119,   170,   118,    22,    25,   130,   170,   170,   170,
     170,   170,   170,   119,   129,   159,   145,    59,   156,   167,
     158,   108,    53,    54,   133,   152,   107,    33,    37,    38,
      40,   108,   142,    42,    45,   146,   107,   107,   108,   162,
      69,   165,   119,   106,   121,   121,   121,   121,   106,   122,
     122,   121,   121,    61,    68,   124,    15,    16,    17,   125,
     118,   130,   125,   107,   170,   170,    27,    27,    27,   119,
     130,    48,   161,    34,   157,   170,   157,   106,   153,   154,
     138,    33,    36,    33,    36,    33,   140,   140,   162,    42,
      31,   144,   163,   107,   165,    62,    62,    61,    81,   123,
      17,    17,   106,   107,   106,   170,   170,   170,   130,    62,
      58,   155,   156,   108,   157,    33,    33,   138,   138,    43,
      44,   162,   106,    48,   147,   107,   109,   125,   131,   131,
      17,   107,   108,   154,    34,   141,   141,   163,    62,    46,
     148,    62,    82,   126,   107,   107,    47,   156,   106,   107,
      49,   108,    47,   107,    63,   158,   163,    62,    62,   107
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   112,   113,   113,   114,   115,   115,   115,   115,   115,
     115,   116,   116,   116,   117,   118,   118,   119,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   121,   121,
     122,   122,   123,   123,   124,   124,   124,   125,   125,   125,
     125,   125,   125,   126,   126,   127,   128,   128,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   130,   130,   130,
     131,   131,   132,   132,   132,   133,   134,   134,   134,   135,
     135,   136,   136,   136,   137,   137,   138,   138,   139,   139,
     140,   140,   140,   140,   141,   141,   142,   142,   142,   142,
     142,   142,   143,   143,   144,   144,   145,   145,   145,   145,
     146,   146,   147,   147,   147,   147,   148,   148,   149,   149,
     149,   150,   150,   151,   151,   152,   152,   153,   153,   154,
     155,   155,   156,   156,   157,   157,   158,   158,   159,   160,
     161,   161,   162,   162,   163,   163,   163,   163,   163,   163,
     163,   164,   164,   164,   164,   164,   164,   165,   165,   165,
     165,   165,   165,   165,   165,   165,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   167,
     167,   167,   167,   167,   167,   167,   168,   169,   169,   170
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
       1,     3,     1,     1,     1,    10,     1,     1,     0,     1,
       3,     2,     3,     1,     1,     0,     2,     1,     2,     0,
       4,     6,     6,     2,     4,     0,     1,     2,     2,     3,
       2,     3,     2,     0,     4,     0,     4,     4,     3,     0,
       3,     0,     2,     4,     4,     0,     2,     0,     8,     7,
       7,     1,     0,     1,     0,     1,     1,     1,     3,     3,
       1,     3,     1,     1,     5,     0,     1,     3,     3,     7,
       2,     0,     1,     3,     3,     3,     3,     2,     2,     3,
       1,     3,     4,     3,     4,     4,     1,     4,     3,     6,
       5,     6,     5,     4,     3,     1,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     1,     1,
       1,     1,     1,     1,     2,     2,     3,     1,     3,     1
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
#line 143 "sql.y" /* yacc.c:1660  */
    {
    (yyval.block) = ctx->factory->NewBlock();
    (yyval.block)->AddStmt((yyvsp[0].stmt));
    ctx->block = (yyval.block);
}
#line 1788 "sql.cc" /* yacc.c:1660  */
    break;

  case 3:
#line 148 "sql.y" /* yacc.c:1660  */
    {
    (yyval.block)->AddStmt((yyvsp[0].stmt));
    ctx->block = (yyval.block);
}
#line 1797 "sql.cc" /* yacc.c:1660  */
    break;

  case 7:
#line 157 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(TCLStatement::TXN_BEGIN);
}
#line 1805 "sql.cc" /* yacc.c:1660  */
    break;

  case 8:
#line 160 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(TCLStatement::TXN_COMMIT);
}
#line 1813 "sql.cc" /* yacc.c:1660  */
    break;

  case 9:
#line 163 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewTCLStatement(TCLStatement::TXN_ROLLBACK);
}
#line 1821 "sql.cc" /* yacc.c:1660  */
    break;

  case 10:
#line 166 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewShowTables();
}
#line 1829 "sql.cc" /* yacc.c:1660  */
    break;

  case 11:
#line 171 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1837 "sql.cc" /* yacc.c:1660  */
    break;

  case 12:
#line 174 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewDropTable((yyvsp[0].name));
}
#line 1845 "sql.cc" /* yacc.c:1660  */
    break;

  case 13:
#line 177 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1853 "sql.cc" /* yacc.c:1660  */
    break;

  case 14:
#line 181 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewCreateTable((yyvsp[-3].name), (yyvsp[-1].col_def_list));
}
#line 1861 "sql.cc" /* yacc.c:1660  */
    break;

  case 15:
#line 185 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_def_list) = ctx->factory->NewColumnDefinitionList((yyvsp[0].col_def));
}
#line 1869 "sql.cc" /* yacc.c:1660  */
    break;

  case 16:
#line 188 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_def_list)->push_back((yyvsp[0].col_def));
}
#line 1877 "sql.cc" /* yacc.c:1660  */
    break;

  case 17:
#line 192 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_def) = ctx->factory->NewColumnDefinition((yyvsp[-5].name), (yyvsp[-4].type_def), (yyvsp[-3].bool_val), (yyvsp[-2].bool_val), (yyvsp[-1].key_type));
    (yyval.col_def)->set_comment((yyvsp[0].name));
}
#line 1886 "sql.cc" /* yacc.c:1660  */
    break;

  case 18:
#line 197 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_BIGINT, (yyvsp[0].size).fixed_size);
}
#line 1894 "sql.cc" /* yacc.c:1660  */
    break;

  case 19:
#line 200 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_INT, (yyvsp[0].size).fixed_size);
}
#line 1902 "sql.cc" /* yacc.c:1660  */
    break;

  case 20:
#line 203 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_SMALLINT, (yyvsp[0].size).fixed_size);
}
#line 1910 "sql.cc" /* yacc.c:1660  */
    break;

  case 21:
#line 206 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_TINYINT, (yyvsp[0].size).fixed_size);
}
#line 1918 "sql.cc" /* yacc.c:1660  */
    break;

  case 22:
#line 209 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DECIMAL, (yyvsp[0].size).fixed_size, (yyvsp[0].size).float_size);
}
#line 1926 "sql.cc" /* yacc.c:1660  */
    break;

  case 23:
#line 212 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_NUMERIC, (yyvsp[0].size).fixed_size, (yyvsp[0].size).float_size);
}
#line 1934 "sql.cc" /* yacc.c:1660  */
    break;

  case 24:
#line 215 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_NUMERIC, (yyvsp[0].size).fixed_size);
}
#line 1942 "sql.cc" /* yacc.c:1660  */
    break;

  case 25:
#line 218 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_VARCHAR, (yyvsp[0].size).fixed_size);
}
#line 1950 "sql.cc" /* yacc.c:1660  */
    break;

  case 26:
#line 221 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DATE);
}
#line 1958 "sql.cc" /* yacc.c:1660  */
    break;

  case 27:
#line 224 "sql.y" /* yacc.c:1660  */
    {
    (yyval.type_def) = ctx->factory->NewTypeDefinition(SQL_DATETIME);
}
#line 1966 "sql.cc" /* yacc.c:1660  */
    break;

  case 28:
#line 228 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = (yyvsp[-1].int_val);
    (yyval.size).float_size = 0;
}
#line 1975 "sql.cc" /* yacc.c:1660  */
    break;

  case 29:
#line 232 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = 0;
    (yyval.size).float_size = 0;
}
#line 1984 "sql.cc" /* yacc.c:1660  */
    break;

  case 30:
#line 237 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = (yyvsp[-3].int_val);
    (yyval.size).float_size = (yyvsp[-1].int_val);
}
#line 1993 "sql.cc" /* yacc.c:1660  */
    break;

  case 31:
#line 241 "sql.y" /* yacc.c:1660  */
    {
    (yyval.size).fixed_size = 0;
    (yyval.size).float_size = 0;
}
#line 2002 "sql.cc" /* yacc.c:1660  */
    break;

  case 32:
#line 246 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2010 "sql.cc" /* yacc.c:1660  */
    break;

  case 33:
#line 249 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2018 "sql.cc" /* yacc.c:1660  */
    break;

  case 34:
#line 253 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2026 "sql.cc" /* yacc.c:1660  */
    break;

  case 35:
#line 256 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2034 "sql.cc" /* yacc.c:1660  */
    break;

  case 36:
#line 259 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2042 "sql.cc" /* yacc.c:1660  */
    break;

  case 37:
#line 263 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_KEY;
}
#line 2050 "sql.cc" /* yacc.c:1660  */
    break;

  case 38:
#line 266 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_UNIQUE_KEY;
}
#line 2058 "sql.cc" /* yacc.c:1660  */
    break;

  case 39:
#line 269 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_UNIQUE_KEY;
}
#line 2066 "sql.cc" /* yacc.c:1660  */
    break;

  case 40:
#line 272 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_PRIMARY_KEY;
}
#line 2074 "sql.cc" /* yacc.c:1660  */
    break;

  case 41:
#line 275 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_PRIMARY_KEY;
}
#line 2082 "sql.cc" /* yacc.c:1660  */
    break;

  case 42:
#line 278 "sql.y" /* yacc.c:1660  */
    {
    (yyval.key_type) = SQL_NOT_KEY;
}
#line 2090 "sql.cc" /* yacc.c:1660  */
    break;

  case 43:
#line 282 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = ctx->factory->NewString((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 2098 "sql.cc" /* yacc.c:1660  */
    break;

  case 44:
#line 285 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = AstString::kEmpty;
}
#line 2106 "sql.cc" /* yacc.c:1660  */
    break;

  case 45:
#line 289 "sql.y" /* yacc.c:1660  */
    {
    (yyval.stmt) = ctx->factory->NewAlterTable((yyvsp[-1].name), (yyvsp[0].alter_table_spce_list));
}
#line 2114 "sql.cc" /* yacc.c:1660  */
    break;

  case 46:
#line 293 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce_list) = ctx->factory->NewAlterTableSpecList((yyvsp[0].alter_table_spce));
}
#line 2122 "sql.cc" /* yacc.c:1660  */
    break;

  case 47:
#line 296 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce_list)->push_back((yyvsp[0].alter_table_spce));
}
#line 2130 "sql.cc" /* yacc.c:1660  */
    break;

  case 48:
#line 300 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2138 "sql.cc" /* yacc.c:1660  */
    break;

  case 49:
#line 303 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2146 "sql.cc" /* yacc.c:1660  */
    break;

  case 50:
#line 306 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def_list));
}
#line 2154 "sql.cc" /* yacc.c:1660  */
    break;

  case 51:
#line 309 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddColumn((yyvsp[-1].col_def_list));
}
#line 2162 "sql.cc" /* yacc.c:1660  */
    break;

  case 52:
#line 312 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddIndex((yyvsp[-4].name), (yyvsp[-3].key_type) == SQL_NOT_KEY ? SQL_KEY : (yyvsp[-3].key_type),
                                             (yyvsp[-1].name_list));
}
#line 2171 "sql.cc" /* yacc.c:1660  */
    break;

  case 53:
#line 316 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableAddIndex((yyvsp[-4].name), (yyvsp[-3].key_type) == SQL_NOT_KEY ? SQL_KEY : (yyvsp[-3].key_type),
                                             (yyvsp[-1].name_list));
}
#line 2180 "sql.cc" /* yacc.c:1660  */
    break;

  case 54:
#line 320 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableChangeColumn((yyvsp[-2].name), (yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2188 "sql.cc" /* yacc.c:1660  */
    break;

  case 55:
#line 323 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableChangeColumn((yyvsp[-2].name), (yyvsp[-1].col_def), (yyvsp[0].col_pos).after, (yyvsp[0].col_pos).name);
}
#line 2196 "sql.cc" /* yacc.c:1660  */
    break;

  case 56:
#line 326 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameColumn((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2204 "sql.cc" /* yacc.c:1660  */
    break;

  case 57:
#line 329 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameIndex((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2212 "sql.cc" /* yacc.c:1660  */
    break;

  case 58:
#line 332 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRenameIndex((yyvsp[-2].name), (yyvsp[0].name));
}
#line 2220 "sql.cc" /* yacc.c:1660  */
    break;

  case 59:
#line 335 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2228 "sql.cc" /* yacc.c:1660  */
    break;

  case 60:
#line 338 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2236 "sql.cc" /* yacc.c:1660  */
    break;

  case 61:
#line 341 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableRename((yyvsp[0].name));
}
#line 2244 "sql.cc" /* yacc.c:1660  */
    break;

  case 62:
#line 344 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropColumn((yyvsp[0].name));
}
#line 2252 "sql.cc" /* yacc.c:1660  */
    break;

  case 63:
#line 347 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropColumn((yyvsp[0].name));
}
#line 2260 "sql.cc" /* yacc.c:1660  */
    break;

  case 64:
#line 350 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex((yyvsp[0].name), false);
}
#line 2268 "sql.cc" /* yacc.c:1660  */
    break;

  case 65:
#line 353 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex((yyvsp[0].name), false);
}
#line 2276 "sql.cc" /* yacc.c:1660  */
    break;

  case 66:
#line 356 "sql.y" /* yacc.c:1660  */
    {
    (yyval.alter_table_spce) = ctx->factory->NewAlterTableDropIndex(AstString::kEmpty, true);
}
#line 2284 "sql.cc" /* yacc.c:1660  */
    break;

  case 67:
#line 360 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = (yyvsp[0].name);
    (yyval.col_pos).after = false;
}
#line 2293 "sql.cc" /* yacc.c:1660  */
    break;

  case 68:
#line 364 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = (yyvsp[0].name);
    (yyval.col_pos).after = true;
}
#line 2302 "sql.cc" /* yacc.c:1660  */
    break;

  case 69:
#line 368 "sql.y" /* yacc.c:1660  */
    {
    (yyval.col_pos).name  = AstString::kEmpty;
    (yyval.col_pos).after = false;
}
#line 2311 "sql.cc" /* yacc.c:1660  */
    break;

  case 70:
#line 373 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name_list) = ctx->factory->NewNameList((yyvsp[0].name));
}
#line 2319 "sql.cc" /* yacc.c:1660  */
    break;

  case 71:
#line 376 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name_list)->push_back((yyvsp[0].name));
}
#line 2327 "sql.cc" /* yacc.c:1660  */
    break;

  case 75:
#line 387 "sql.y" /* yacc.c:1660  */
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
#line 2345 "sql.cc" /* yacc.c:1660  */
    break;

  case 76:
#line 401 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2353 "sql.cc" /* yacc.c:1660  */
    break;

  case 77:
#line 404 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2361 "sql.cc" /* yacc.c:1660  */
    break;

  case 78:
#line 407 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2369 "sql.cc" /* yacc.c:1660  */
    break;

  case 79:
#line 411 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col_list) = ctx->factory->NewProjectionColumnList((yyvsp[0].proj_col));
}
#line 2377 "sql.cc" /* yacc.c:1660  */
    break;

  case 80:
#line 414 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col_list)->push_back((yyvsp[0].proj_col));
}
#line 2385 "sql.cc" /* yacc.c:1660  */
    break;

  case 81:
#line 418 "sql.y" /* yacc.c:1660  */
    {
    (yyval.proj_col) = ctx->factory->NewProjectionColumn((yyvsp[-1].expr), (yyvsp[0].name),
                                           Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2394 "sql.cc" /* yacc.c:1660  */
    break;

  case 82:
#line 422 "sql.y" /* yacc.c:1660  */
    {
    Identifier *id = ctx->factory->NewIdentifierWithPlaceholder((yyvsp[-2].name),
        ctx->factory->NewStarPlaceholder((yylsp[0])),
        Location::Concat((yylsp[-2]), (yylsp[0])));
    (yyval.proj_col) = ctx->factory->NewProjectionColumn(id, AstString::kEmpty,
        Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2406 "sql.cc" /* yacc.c:1660  */
    break;

  case 83:
#line 429 "sql.y" /* yacc.c:1660  */
    {
    Placeholder *ph = ctx->factory->NewStarPlaceholder((yylsp[0]));
    (yyval.proj_col) = ctx->factory->NewProjectionColumn(ph, AstString::kEmpty, (yylsp[0]));
}
#line 2415 "sql.cc" /* yacc.c:1660  */
    break;

  case 85:
#line 436 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = AstString::kEmpty;
}
#line 2423 "sql.cc" /* yacc.c:1660  */
    break;

  case 86:
#line 440 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = (yyvsp[0].name);
}
#line 2431 "sql.cc" /* yacc.c:1660  */
    break;

  case 87:
#line 443 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = (yyvsp[0].name);
}
#line 2439 "sql.cc" /* yacc.c:1660  */
    break;

  case 88:
#line 447 "sql.y" /* yacc.c:1660  */
    {
    (yyval.query) = (yyvsp[0].query);
}
#line 2447 "sql.cc" /* yacc.c:1660  */
    break;

  case 89:
#line 450 "sql.y" /* yacc.c:1660  */
    {
    (yyval.query) = nullptr;
}
#line 2455 "sql.cc" /* yacc.c:1660  */
    break;

  case 90:
#line 454 "sql.y" /* yacc.c:1660  */
    {
    Query *query = ::mai::down_cast<Query>((yyvsp[-2].stmt));
    query->set_alias((yyvsp[0].name));
    (yyval.query) = query;
}
#line 2465 "sql.cc" /* yacc.c:1660  */
    break;

  case 91:
#line 459 "sql.y" /* yacc.c:1660  */
    {
    (yyvsp[-5].query)->set_alias((yyvsp[-4].name));
    (yyvsp[-2].query)->set_alias((yyvsp[-1].name));
    (yyval.query) = ctx->factory->NewJoinRelation((yyvsp[-5].query), SQL_CROSS_JOIN, (yyvsp[-2].query), (yyvsp[0].expr),
        AstString::kEmpty);
}
#line 2476 "sql.cc" /* yacc.c:1660  */
    break;

  case 92:
#line 465 "sql.y" /* yacc.c:1660  */
    {
    (yyvsp[-5].query)->set_alias((yyvsp[-4].name));
    (yyvsp[-2].query)->set_alias((yyvsp[-1].name));
    (yyval.query) = ctx->factory->NewJoinRelation((yyvsp[-5].query), (yyvsp[-3].join_kind), (yyvsp[-2].query), (yyvsp[0].expr), AstString::kEmpty);
}
#line 2486 "sql.cc" /* yacc.c:1660  */
    break;

  case 93:
#line 470 "sql.y" /* yacc.c:1660  */
    {
    (yyval.query) = ctx->factory->NewNameRelation((yyvsp[-1].id), (yyvsp[0].name));
}
#line 2494 "sql.cc" /* yacc.c:1660  */
    break;

  case 94:
#line 474 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2502 "sql.cc" /* yacc.c:1660  */
    break;

  case 95:
#line 477 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = nullptr;
}
#line 2510 "sql.cc" /* yacc.c:1660  */
    break;

  case 96:
#line 481 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_CROSS_JOIN;
}
#line 2518 "sql.cc" /* yacc.c:1660  */
    break;

  case 97:
#line 484 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_CROSS_JOIN;
}
#line 2526 "sql.cc" /* yacc.c:1660  */
    break;

  case 98:
#line 487 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_LEFT_OUTTER_JOIN;
}
#line 2534 "sql.cc" /* yacc.c:1660  */
    break;

  case 99:
#line 490 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_LEFT_OUTTER_JOIN;
}
#line 2542 "sql.cc" /* yacc.c:1660  */
    break;

  case 100:
#line 493 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_RIGHT_OUTTER_JOIN;
}
#line 2550 "sql.cc" /* yacc.c:1660  */
    break;

  case 101:
#line 496 "sql.y" /* yacc.c:1660  */
    {
    (yyval.join_kind) = SQL_RIGHT_OUTTER_JOIN;
}
#line 2558 "sql.cc" /* yacc.c:1660  */
    break;

  case 102:
#line 500 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2566 "sql.cc" /* yacc.c:1660  */
    break;

  case 103:
#line 503 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = nullptr;
}
#line 2574 "sql.cc" /* yacc.c:1660  */
    break;

  case 104:
#line 507 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2582 "sql.cc" /* yacc.c:1660  */
    break;

  case 105:
#line 510 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = nullptr;
}
#line 2590 "sql.cc" /* yacc.c:1660  */
    break;

  case 106:
#line 514 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = (yyvsp[-1].expr_list);
    (yyval.order_by).desc = false;
}
#line 2599 "sql.cc" /* yacc.c:1660  */
    break;

  case 107:
#line 518 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = (yyvsp[-1].expr_list);
    (yyval.order_by).desc = true;
}
#line 2608 "sql.cc" /* yacc.c:1660  */
    break;

  case 108:
#line 522 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = (yyvsp[0].expr_list);
    (yyval.order_by).desc = false;
}
#line 2617 "sql.cc" /* yacc.c:1660  */
    break;

  case 109:
#line 526 "sql.y" /* yacc.c:1660  */
    {
    (yyval.order_by).expr_list = nullptr;
    (yyval.order_by).desc = false;
}
#line 2626 "sql.cc" /* yacc.c:1660  */
    break;

  case 110:
#line 531 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = (yyvsp[0].expr_list);
}
#line 2634 "sql.cc" /* yacc.c:1660  */
    break;

  case 111:
#line 534 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = nullptr;
}
#line 2642 "sql.cc" /* yacc.c:1660  */
    break;

  case 112:
#line 538 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
    (yyval.limit).offset_val = 0;
}
#line 2651 "sql.cc" /* yacc.c:1660  */
    break;

  case 113:
#line 542 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).offset_val = (yyvsp[-2].int_val);
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
}
#line 2660 "sql.cc" /* yacc.c:1660  */
    break;

  case 114:
#line 546 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = (yyvsp[-2].int_val);
    (yyval.limit).offset_val = (yyvsp[0].int_val);
}
#line 2669 "sql.cc" /* yacc.c:1660  */
    break;

  case 115:
#line 550 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = 0;
    (yyval.limit).offset_val = 0;
}
#line 2678 "sql.cc" /* yacc.c:1660  */
    break;

  case 116:
#line 556 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2686 "sql.cc" /* yacc.c:1660  */
    break;

  case 117:
#line 559 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2694 "sql.cc" /* yacc.c:1660  */
    break;

  case 118:
#line 563 "sql.y" /* yacc.c:1660  */
    {
    Insert *stmt = ctx->factory->NewInsert((yyvsp[-6].bool_val), (yyvsp[-4].id));
    stmt->set_col_names((yyvsp[-3].name_list));
    stmt->set_row_values_list((yyvsp[-1].row_vals_list));
    stmt->set_on_duplicate_clause((yyvsp[0].assignment_list));
    (yyval.stmt) = stmt;
}
#line 2706 "sql.cc" /* yacc.c:1660  */
    break;

  case 119:
#line 570 "sql.y" /* yacc.c:1660  */
    {
    Insert *stmt = ctx->factory->NewInsert((yyvsp[-5].bool_val), (yyvsp[-3].id));
    stmt->SetAssignmentList((yyvsp[-1].assignment_list), ctx->factory->arena());
    stmt->set_on_duplicate_clause((yyvsp[0].assignment_list));
    (yyval.stmt) = stmt;
}
#line 2717 "sql.cc" /* yacc.c:1660  */
    break;

  case 120:
#line 576 "sql.y" /* yacc.c:1660  */
    {
    Insert *stmt = ctx->factory->NewInsert((yyvsp[-5].bool_val), (yyvsp[-3].id));
    stmt->set_col_names((yyvsp[-2].name_list));
    stmt->set_select_clause(::mai::down_cast<Query>((yyvsp[-1].stmt)));
    stmt->set_on_duplicate_clause((yyvsp[0].assignment_list));
    (yyval.stmt) = stmt;
}
#line 2729 "sql.cc" /* yacc.c:1660  */
    break;

  case 122:
#line 585 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name_list) = nullptr;
}
#line 2737 "sql.cc" /* yacc.c:1660  */
    break;

  case 123:
#line 589 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = true;
}
#line 2745 "sql.cc" /* yacc.c:1660  */
    break;

  case 124:
#line 592 "sql.y" /* yacc.c:1660  */
    {
    (yyval.bool_val) = false;
}
#line 2753 "sql.cc" /* yacc.c:1660  */
    break;

  case 127:
#line 599 "sql.y" /* yacc.c:1660  */
    {
    (yyval.row_vals_list) = ctx->factory->NewRowValuesList((yyvsp[0].expr_list));
}
#line 2761 "sql.cc" /* yacc.c:1660  */
    break;

  case 128:
#line 602 "sql.y" /* yacc.c:1660  */
    {
    (yyval.row_vals_list)->push_back((yyvsp[0].expr_list));
}
#line 2769 "sql.cc" /* yacc.c:1660  */
    break;

  case 129:
#line 606 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = (yyvsp[-1].expr_list);
}
#line 2777 "sql.cc" /* yacc.c:1660  */
    break;

  case 130:
#line 610 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = ctx->factory->NewExpressionList((yyvsp[0].expr));
}
#line 2785 "sql.cc" /* yacc.c:1660  */
    break;

  case 131:
#line 613 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list)->push_back((yyvsp[0].expr));
}
#line 2793 "sql.cc" /* yacc.c:1660  */
    break;

  case 133:
#line 618 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewDefaultPlaceholderLiteral((yylsp[0]));
}
#line 2801 "sql.cc" /* yacc.c:1660  */
    break;

  case 134:
#line 622 "sql.y" /* yacc.c:1660  */
    {
    (yyval.assignment_list) = (yyvsp[0].assignment_list);
}
#line 2809 "sql.cc" /* yacc.c:1660  */
    break;

  case 135:
#line 625 "sql.y" /* yacc.c:1660  */
    {
    (yyval.assignment_list) = nullptr;
}
#line 2817 "sql.cc" /* yacc.c:1660  */
    break;

  case 136:
#line 629 "sql.y" /* yacc.c:1660  */
    {
    (yyval.assignment_list) = ctx->factory->NewAssignmentList((yyvsp[0].assignment));
}
#line 2825 "sql.cc" /* yacc.c:1660  */
    break;

  case 137:
#line 632 "sql.y" /* yacc.c:1660  */
    {
    (yyval.assignment_list)->push_back((yyvsp[0].assignment));
}
#line 2833 "sql.cc" /* yacc.c:1660  */
    break;

  case 138:
#line 636 "sql.y" /* yacc.c:1660  */
    {
    if ((yyvsp[-1].op) != SQL_CMP_EQ) {
        yyerror(&(yylsp[-2]), ctx, "incorrect assignment.");
        YYERROR;
    }
}
#line 2844 "sql.cc" /* yacc.c:1660  */
    break;

  case 139:
#line 644 "sql.y" /* yacc.c:1660  */
    {
    // TODO:
    (yyval.stmt) = nullptr;
}
#line 2853 "sql.cc" /* yacc.c:1660  */
    break;

  case 140:
#line 649 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = (yyvsp[0].int_val);
    (yyval.limit).offset_val = 0;
}
#line 2862 "sql.cc" /* yacc.c:1660  */
    break;

  case 141:
#line 653 "sql.y" /* yacc.c:1660  */
    {
    (yyval.limit).limit_val  = 0;
    (yyval.limit).offset_val = 0;
}
#line 2871 "sql.cc" /* yacc.c:1660  */
    break;

  case 142:
#line 663 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list) = ctx->factory->NewExpressionList((yyvsp[0].expr));
}
#line 2879 "sql.cc" /* yacc.c:1660  */
    break;

  case 143:
#line 666 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr_list)->push_back((yyvsp[0].expr));
}
#line 2887 "sql.cc" /* yacc.c:1660  */
    break;

  case 144:
#line 673 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_OR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2895 "sql.cc" /* yacc.c:1660  */
    break;

  case 145:
#line 676 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_XOR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2903 "sql.cc" /* yacc.c:1660  */
    break;

  case 146:
#line 679 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_AND, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2911 "sql.cc" /* yacc.c:1660  */
    break;

  case 147:
#line 682 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_NOT, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2919 "sql.cc" /* yacc.c:1660  */
    break;

  case 148:
#line 685 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_NOT, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2927 "sql.cc" /* yacc.c:1660  */
    break;

  case 149:
#line 688 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2935 "sql.cc" /* yacc.c:1660  */
    break;

  case 151:
#line 696 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_IS_NULL, (yyvsp[-2].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2943 "sql.cc" /* yacc.c:1660  */
    break;

  case 152:
#line 699 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_IS_NOT_NULL, (yyvsp[-3].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2951 "sql.cc" /* yacc.c:1660  */
    break;

  case 153:
#line 702 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-2].expr), (yyvsp[-1].op), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2959 "sql.cc" /* yacc.c:1660  */
    break;

  case 154:
#line 705 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-3].expr), (yyvsp[-2].op), (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2967 "sql.cc" /* yacc.c:1660  */
    break;

  case 155:
#line 708 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewComparison((yyvsp[-3].expr), (yyvsp[-2].op), (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2975 "sql.cc" /* yacc.c:1660  */
    break;

  case 157:
#line 716 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-3].expr), SQL_NOT_IN, (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2983 "sql.cc" /* yacc.c:1660  */
    break;

  case 158:
#line 719 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-2].expr), SQL_IN, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2991 "sql.cc" /* yacc.c:1660  */
    break;

  case 159:
#line 722 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-5].expr), SQL_NOT_IN, (yyvsp[-1].expr_list), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 2999 "sql.cc" /* yacc.c:1660  */
    break;

  case 160:
#line 725 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-4].expr), SQL_IN, (yyvsp[-1].expr_list), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 3007 "sql.cc" /* yacc.c:1660  */
    break;

  case 161:
#line 728 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-5].expr), SQL_NOT_BETWEEN, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 3015 "sql.cc" /* yacc.c:1660  */
    break;

  case 162:
#line 731 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewMultiExpression((yyvsp[-4].expr), SQL_BETWEEN, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 3023 "sql.cc" /* yacc.c:1660  */
    break;

  case 163:
#line 734 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-3].expr), SQL_NOT_LIKE, (yyvsp[0].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 3031 "sql.cc" /* yacc.c:1660  */
    break;

  case 164:
#line 737 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_LIKE, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3039 "sql.cc" /* yacc.c:1660  */
    break;

  case 166:
#line 746 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_OR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3047 "sql.cc" /* yacc.c:1660  */
    break;

  case 167:
#line 749 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_AND, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3055 "sql.cc" /* yacc.c:1660  */
    break;

  case 168:
#line 752 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_LSHIFT, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3063 "sql.cc" /* yacc.c:1660  */
    break;

  case 169:
#line 755 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_RSHIFT, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3071 "sql.cc" /* yacc.c:1660  */
    break;

  case 170:
#line 758 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_PLUS, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3079 "sql.cc" /* yacc.c:1660  */
    break;

  case 171:
#line 761 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_SUB, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3087 "sql.cc" /* yacc.c:1660  */
    break;

  case 172:
#line 764 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MUL, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3095 "sql.cc" /* yacc.c:1660  */
    break;

  case 173:
#line 767 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_DIV, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3103 "sql.cc" /* yacc.c:1660  */
    break;

  case 174:
#line 770 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_DIV, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3111 "sql.cc" /* yacc.c:1660  */
    break;

  case 175:
#line 773 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MOD, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3119 "sql.cc" /* yacc.c:1660  */
    break;

  case 176:
#line 776 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_MOD, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3127 "sql.cc" /* yacc.c:1660  */
    break;

  case 177:
#line 779 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewBinaryExpression((yyvsp[-2].expr), SQL_BIT_XOR, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3135 "sql.cc" /* yacc.c:1660  */
    break;

  case 179:
#line 788 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = (yyvsp[0].id);
}
#line 3143 "sql.cc" /* yacc.c:1660  */
    break;

  case 180:
#line 791 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewStringLiteral((yyvsp[0].text).buf, (yyvsp[0].text).len, (yylsp[0]));
}
#line 3151 "sql.cc" /* yacc.c:1660  */
    break;

  case 181:
#line 794 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewIntegerLiteral((yyvsp[0].int_val), (yylsp[0]));
}
#line 3159 "sql.cc" /* yacc.c:1660  */
    break;

  case 182:
#line 797 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewApproxLiteral((yyvsp[0].approx_val), (yylsp[0]));
}
#line 3167 "sql.cc" /* yacc.c:1660  */
    break;

  case 183:
#line 800 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewParamPlaceholder((yylsp[0]));
}
#line 3175 "sql.cc" /* yacc.c:1660  */
    break;

  case 184:
#line 803 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_MINUS, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 3183 "sql.cc" /* yacc.c:1660  */
    break;

  case 185:
#line 806 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewUnaryExpression(SQL_BIT_INV, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 3191 "sql.cc" /* yacc.c:1660  */
    break;

  case 186:
#line 811 "sql.y" /* yacc.c:1660  */
    {
    (yyval.expr) = ctx->factory->NewSubquery(true, ::mai::down_cast<Query>((yyvsp[-1].stmt)), (yylsp[-1]));
}
#line 3199 "sql.cc" /* yacc.c:1660  */
    break;

  case 187:
#line 815 "sql.y" /* yacc.c:1660  */
    {
    (yyval.id) = ctx->factory->NewIdentifier(AstString::kEmpty, (yyvsp[0].name), (yylsp[0]));
}
#line 3207 "sql.cc" /* yacc.c:1660  */
    break;

  case 188:
#line 818 "sql.y" /* yacc.c:1660  */
    {
    (yyval.id) = ctx->factory->NewIdentifier((yyvsp[-2].name), (yyvsp[0].name), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 3215 "sql.cc" /* yacc.c:1660  */
    break;

  case 189:
#line 822 "sql.y" /* yacc.c:1660  */
    {
    (yyval.name) = ctx->factory->NewString((yyvsp[0].text).buf, (yyvsp[0].text).len);
}
#line 3223 "sql.cc" /* yacc.c:1660  */
    break;


#line 3227 "sql.cc" /* yacc.c:1660  */
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
#line 826 "sql.y" /* yacc.c:1903  */

void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg) {
    ctx->err_line   = yyl->first_line;
    ctx->err_column = yyl->first_column;
    ctx->err_msg    = ctx->factory->NewString(msg);
}
