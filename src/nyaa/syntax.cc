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

/* Substitute the type names.  */
#define YYSTYPE         NYAA_YYSTYPE
#define YYLTYPE         NYAA_YYLTYPE
/* Substitute the variable and function names.  */
#define yyparse         nyaa_yyparse
#define yylex           nyaa_yylex
#define yyerror         nyaa_yyerror
#define yydebug         nyaa_yydebug
#define yynerrs         nyaa_yynerrs


/* First part of user prologue.  */
#line 12 "syntax.y" /* yacc.c:337  */

#include "nyaa/ast.h"
#include "nyaa/parser-ctx.h"
#include "nyaa/syntax.hh"
#include "nyaa/lex.yy.h"
#include <string.h>

#define YYLEX_PARAM ctx->lex

using ::mai::nyaa::ast::Location;
using ::mai::nyaa::Operator;

void yyerror(YYLTYPE *, parser_ctx *, const char *);

#line 93 "syntax.cc" /* yacc.c:337  */
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
   by #include "syntax.hh".  */
#ifndef YY_NYAA_YY_SYNTAX_HH_INCLUDED
# define YY_NYAA_YY_SYNTAX_HH_INCLUDED
/* Debug traces.  */
#ifndef NYAA_YYDEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define NYAA_YYDEBUG 1
#  else
#   define NYAA_YYDEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define NYAA_YYDEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined NYAA_YYDEBUG */
#if NYAA_YYDEBUG
extern int nyaa_yydebug;
#endif

/* Token type.  */
#ifndef NYAA_YYTOKENTYPE
# define NYAA_YYTOKENTYPE
  enum nyaa_yytokentype
  {
    DEF = 258,
    VAR = 259,
    LAMBDA = 260,
    NAME = 261,
    COMPARISON = 262,
    OP_OR = 263,
    OP_XOR = 264,
    OP_AND = 265,
    OP_LSHIFT = 266,
    OP_RSHIFT = 267,
    UMINUS = 268,
    RETURN = 269,
    VARGS = 270,
    DO = 271,
    NEW = 272,
    IF = 273,
    ELSE = 274,
    WHILE = 275,
    OBJECT = 276,
    CLASS = 277,
    PROPERTY = 278,
    STRING_LITERAL = 279,
    SMI_LITERAL = 280,
    APPROX_LITERAL = 281,
    INT_LITERAL = 282,
    NIL_LITERAL = 283,
    BOOL_LITERAL = 284,
    TOKEN_ERROR = 285,
    IN = 286,
    IS = 287,
    OP_NOT = 288
  };
#endif

/* Value type.  */
#if ! defined NYAA_YYSTYPE && ! defined NYAA_YYSTYPE_IS_DECLARED

union NYAA_YYSTYPE
{
#line 27 "syntax.y" /* yacc.c:352  */

    const ::mai::nyaa::ast::String *name;
    ::mai::nyaa::ast::VarDeclaration::NameList *names;
    ::mai::nyaa::ast::Block *block;
    ::mai::nyaa::ast::Expression *expr;
    ::mai::nyaa::ast::LValue *lval;
    ::mai::nyaa::ast::Assignment::LValList *lvals;
    ::mai::nyaa::ast::Multiple *entry;
    ::mai::nyaa::ast::MapInitializer::EntryList *entries;
    ::mai::nyaa::ast::Statement *stmt;
    ::mai::nyaa::ast::Block::StmtList *stmts;
    ::mai::nyaa::ast::Return::ExprList *exprs;
    struct {
        ::mai::nyaa::ast::LambdaLiteral::ParameterList *params;
        bool vargs;
    } params;
    ::mai::nyaa::Operator::ID op;
    ::mai::nyaa::ast::String *str_val;
    ::mai::nyaa::ast::String *int_val;
    ::mai::nyaa::f64_t f64_val;
    int64_t smi_val;
    bool bool_val;

#line 202 "syntax.cc" /* yacc.c:352  */
};

typedef union NYAA_YYSTYPE NYAA_YYSTYPE;
# define NYAA_YYSTYPE_IS_TRIVIAL 1
# define NYAA_YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined NYAA_YYLTYPE && ! defined NYAA_YYLTYPE_IS_DECLARED
typedef struct NYAA_YYLTYPE NYAA_YYLTYPE;
struct NYAA_YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define NYAA_YYLTYPE_IS_DECLARED 1
# define NYAA_YYLTYPE_IS_TRIVIAL 1
#endif



int nyaa_yyparse (parser_ctx *ctx);

#endif /* !YY_NYAA_YY_SYNTAX_HH_INCLUDED  */



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
         || (defined NYAA_YYLTYPE_IS_TRIVIAL && NYAA_YYLTYPE_IS_TRIVIAL \
             && defined NYAA_YYSTYPE_IS_TRIVIAL && NYAA_YYSTYPE_IS_TRIVIAL)))

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
#define YYFINAL  56
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   372

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  53
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  30
/* YYNRULES -- Number of rules.  */
#define YYNRULES  83
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  153

#define YYUNDEFTOK  2
#define YYMAXUTOK   288

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
       2,     2,     2,    35,     2,     2,     2,    42,    37,     2,
      46,    47,    40,    38,    50,    39,    49,    41,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    48,     2,
       2,    31,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    51,     2,    52,    43,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    44,    36,    45,     2,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    32,    33,    34
};

#if NYAA_YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    91,    91,    96,   100,   103,   106,   110,   113,   116,
     119,   122,   125,   128,   131,   134,   138,   142,   145,   148,
     152,   156,   160,   163,   167,   170,   173,   177,   180,   184,
     188,   193,   198,   202,   206,   211,   214,   218,   221,   225,
     229,   232,   235,   239,   242,   245,   248,   251,   254,   257,
     260,   263,   266,   269,   272,   275,   278,   281,   284,   287,
     291,   294,   298,   302,   305,   308,   312,   316,   319,   323,
     326,   330,   333,   336,   340,   343,   346,   351,   354,   358,
     362,   365,   370,   373
};
#endif

#if NYAA_YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "DEF", "VAR", "LAMBDA", "NAME",
  "COMPARISON", "OP_OR", "OP_XOR", "OP_AND", "OP_LSHIFT", "OP_RSHIFT",
  "UMINUS", "RETURN", "VARGS", "DO", "NEW", "IF", "ELSE", "WHILE",
  "OBJECT", "CLASS", "PROPERTY", "STRING_LITERAL", "SMI_LITERAL",
  "APPROX_LITERAL", "INT_LITERAL", "NIL_LITERAL", "BOOL_LITERAL",
  "TOKEN_ERROR", "'='", "IN", "IS", "OP_NOT", "'!'", "'|'", "'&'", "'+'",
  "'-'", "'*'", "'/'", "'%'", "'^'", "'{'", "'}'", "'('", "')'", "':'",
  "'.'", "','", "'['", "']'", "$accept", "Script", "Block",
  "StatementList", "Statement", "IfStatement", "ElseClause",
  "ObjectDefinition", "ClassDefinition", "InheritClause", "MemberList",
  "MemberDefinition", "PropertyDeclaration", "FunctionDefinition",
  "Parameters", "ParameterVargs", "VarDeclaration", "Assignment",
  "ExpressionList", "Expression", "LambdaLiteral", "MapInitializer",
  "MapEntryList", "MapEntry", "LValList", "LValue", "Call", "Arguments",
  "Attributes", "NameList", YY_NULLPTR
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
     285,    61,   286,   287,   288,    33,   124,    38,    43,    45,
      42,    47,    37,    94,   123,   125,    40,    41,    58,    46,
      44,    91,    93
};
# endif

#define YYPACT_NINF -122

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-122)))

#define YYTABLE_NINF -71

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     286,    16,    27,    43,  -122,   311,  -122,    42,    65,   -35,
      48,    48,  -122,  -122,  -122,  -122,  -122,  -122,   119,   311,
      93,   286,  -122,  -122,  -122,  -122,  -122,  -122,  -122,   204,
    -122,  -122,   -24,   -16,  -122,    10,  -122,     7,   286,    12,
    -122,    57,   204,  -122,  -122,    62,   311,    27,   103,   104,
      63,   311,   204,   -33,  -122,   166,  -122,  -122,   311,  -122,
     311,   311,   311,   311,   311,   106,   107,   311,  -122,  -122,
     311,   311,    12,   109,   311,   110,   253,  -122,    71,    69,
     311,   311,   185,    53,    76,    75,   311,   128,  -122,   119,
    -122,   303,   321,   321,    28,    28,    20,    -5,  -122,   147,
      57,   204,    18,    79,    81,    57,  -122,  -122,    42,    77,
    -122,   204,    34,    42,  -122,    13,   122,    85,   204,    99,
    -122,  -122,  -122,  -122,    42,    12,  -122,  -122,  -122,   112,
      48,     5,  -122,  -122,  -122,  -122,    13,   311,  -122,    90,
     -13,  -122,    27,  -122,  -122,     6,   204,    42,  -122,  -122,
      82,  -122,  -122
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       6,     0,     0,     0,    71,    42,    53,     0,     0,     0,
      81,    81,    50,    46,    49,    48,    45,    47,    65,     0,
       0,     2,     4,    15,     9,    10,    11,     8,    12,    13,
      51,    52,     0,    43,    44,     0,    82,    38,     6,    34,
      61,     7,    40,    43,    14,     0,     0,     0,     0,     0,
      71,     0,    67,     0,    63,     0,     1,     5,     0,    78,
       0,     0,     0,     0,    42,     0,     0,     0,    79,    74,
      42,     0,    34,     0,    42,     0,     0,    33,     0,    36,
       0,    42,     0,     0,     0,    23,     0,     0,    62,     0,
      59,    54,    55,    56,    57,    58,     0,     0,    73,     0,
      39,     0,    43,     0,     0,    37,    83,     3,     0,     0,
      32,    41,     0,     0,    80,    26,     0,     0,    66,     0,
      64,    77,    75,    72,     0,    34,    60,    35,    76,    19,
      81,     0,    24,    27,    28,    22,    26,     0,    31,     0,
       0,    16,     0,    20,    25,     0,    68,     0,    17,    18,
      29,    21,    30
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -122,  -122,    -7,   100,   -15,    11,  -122,  -122,  -122,  -122,
      14,  -121,  -122,  -101,   -59,  -122,  -122,  -122,   -27,     2,
    -122,     3,  -122,    64,  -122,     4,  -122,    45,    -8,    -1
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    20,    40,    21,    22,    23,   141,    24,    25,   117,
     131,   132,   133,    26,    78,   110,    27,    28,    41,    42,
      30,    31,    53,    54,    32,    43,    34,    69,    48,    79
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      44,    37,    29,    49,    33,     9,    57,    70,     1,     1,
     144,    46,    88,   103,   134,   -69,     1,    89,    36,    59,
      52,    55,    35,    29,   144,    33,    71,    77,   130,   130,
     134,    38,    68,    36,   -69,   134,   130,    96,    74,    18,
      29,    64,    33,   100,   134,    68,    83,   105,    82,   -70,
     143,   151,    59,    87,   112,    68,    72,    75,    68,    73,
      91,    57,    92,    93,    94,    95,   139,   121,   -70,    99,
      80,    45,    18,   101,    64,   102,    65,    66,    29,    67,
      33,   128,   111,   106,    80,    68,    38,    38,   118,    39,
      68,    52,   127,    56,    68,    68,    68,    68,    68,    47,
      68,   126,    68,    75,    68,   114,   129,    80,    81,    84,
      85,    86,    97,    98,    68,   104,   106,   138,   108,   109,
     115,    68,   142,   116,     3,    50,   124,   125,   135,   136,
     137,   140,    75,   148,     6,    58,     8,   147,    76,   146,
     152,   150,   122,    12,    13,    14,    15,    16,    17,    68,
     145,   149,    59,   120,    58,     0,     0,     0,     0,     0,
       0,     0,     0,    18,     0,    19,    60,    61,    62,    63,
      51,    59,    18,    58,    64,     0,    65,    66,     0,    67,
     119,     0,     0,     0,     0,    60,    61,    62,    63,     0,
      59,    18,    58,    64,     0,    65,    66,     0,    67,   123,
       0,     0,     0,     0,    60,    61,    62,    63,     0,    59,
      18,    58,    64,    90,    65,    66,     0,    67,     0,     0,
       0,     0,     0,    60,    61,    62,    63,     0,    59,    18,
       0,    64,   113,    65,    66,     0,    67,     0,     0,     0,
       0,     0,    60,    61,    62,    63,     0,     0,    18,     0,
      64,     0,    65,    66,     0,    67,     1,     2,     3,     4,
       0,     0,     0,     0,     0,     0,     0,     5,     6,     7,
       8,     9,     0,     0,    10,    11,     0,    12,    13,    14,
      15,    16,    17,     0,     0,     0,     0,     0,     0,     1,
       2,     3,     4,     0,     0,     0,     0,    18,   107,    19,
       5,     6,     7,     8,     9,     0,     0,    10,    11,     0,
      12,    13,    14,    15,    16,    17,     3,     4,     0,     0,
       0,     0,     0,     0,     0,     0,     6,    59,     8,     0,
      18,     0,    19,     0,     0,    12,    13,    14,    15,    16,
      17,    60,    61,    62,    63,    59,     0,    18,     0,    64,
       0,    65,    66,     0,    67,    18,     0,    19,     0,     0,
       0,    62,    63,     0,     0,    18,     0,    64,     0,    65,
      66,     0,    67
};

static const yytype_int16 yycheck[] =
{
       7,     2,     0,    11,     0,    18,    21,    31,     3,     3,
     131,    46,    45,    72,   115,    31,     3,    50,     6,    24,
      18,    19,     6,    21,   145,    21,    50,    15,    23,    23,
     131,    44,    29,     6,    50,   136,    23,    64,    31,    44,
      38,    46,    38,    70,   145,    42,    47,    74,    46,    31,
      45,    45,    24,    51,    81,    52,    46,    50,    55,    49,
      58,    76,    60,    61,    62,    63,   125,    47,    50,    67,
      50,     6,    44,    71,    46,    71,    48,    49,    76,    51,
      76,    47,    80,     6,    50,    82,    44,    44,    86,    46,
      87,    89,    15,     0,    91,    92,    93,    94,    95,    51,
      97,   108,    99,    50,   101,    52,   113,    50,    46,     6,
       6,    48,     6,     6,   111,     6,     6,   124,    47,    50,
      44,   118,   130,    48,     5,     6,    47,    46,     6,    44,
      31,    19,    50,   140,    15,     7,    17,    47,    38,   137,
     147,   142,    97,    24,    25,    26,    27,    28,    29,   146,
     136,   140,    24,    89,     7,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    44,    -1,    46,    38,    39,    40,    41,
      51,    24,    44,     7,    46,    -1,    48,    49,    -1,    51,
      52,    -1,    -1,    -1,    -1,    38,    39,    40,    41,    -1,
      24,    44,     7,    46,    -1,    48,    49,    -1,    51,    52,
      -1,    -1,    -1,    -1,    38,    39,    40,    41,    -1,    24,
      44,     7,    46,    47,    48,    49,    -1,    51,    -1,    -1,
      -1,    -1,    -1,    38,    39,    40,    41,    -1,    24,    44,
      -1,    46,    47,    48,    49,    -1,    51,    -1,    -1,    -1,
      -1,    -1,    38,    39,    40,    41,    -1,    -1,    44,    -1,
      46,    -1,    48,    49,    -1,    51,     3,     4,     5,     6,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    14,    15,    16,
      17,    18,    -1,    -1,    21,    22,    -1,    24,    25,    26,
      27,    28,    29,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,    -1,    -1,    -1,    -1,    44,    45,    46,
      14,    15,    16,    17,    18,    -1,    -1,    21,    22,    -1,
      24,    25,    26,    27,    28,    29,     5,     6,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    15,    24,    17,    -1,
      44,    -1,    46,    -1,    -1,    24,    25,    26,    27,    28,
      29,    38,    39,    40,    41,    24,    -1,    44,    -1,    46,
      -1,    48,    49,    -1,    51,    44,    -1,    46,    -1,    -1,
      -1,    40,    41,    -1,    -1,    44,    -1,    46,    -1,    48,
      49,    -1,    51
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,    14,    15,    16,    17,    18,
      21,    22,    24,    25,    26,    27,    28,    29,    44,    46,
      54,    56,    57,    58,    60,    61,    66,    69,    70,    72,
      73,    74,    77,    78,    79,     6,     6,    82,    44,    46,
      55,    71,    72,    78,    55,     6,    46,    51,    81,    81,
       6,    51,    72,    75,    76,    72,     0,    57,     7,    24,
      38,    39,    40,    41,    46,    48,    49,    51,    74,    80,
      31,    50,    46,    49,    31,    50,    56,    15,    67,    82,
      50,    46,    72,    82,     6,     6,    48,    72,    45,    50,
      47,    72,    72,    72,    72,    72,    71,     6,     6,    72,
      71,    72,    78,    67,     6,    71,     6,    45,    47,    50,
      68,    72,    71,    47,    52,    44,    48,    62,    72,    52,
      76,    47,    80,    52,    47,    46,    55,    15,    47,    55,
      23,    63,    64,    65,    66,     6,    44,    31,    55,    67,
      19,    59,    81,    45,    64,    63,    72,    47,    55,    58,
      82,    45,    55
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    53,    54,    55,    56,    56,    56,    57,    57,    57,
      57,    57,    57,    57,    57,    57,    58,    59,    59,    59,
      60,    61,    62,    62,    63,    63,    63,    64,    64,    65,
      66,    66,    67,    67,    67,    68,    68,    69,    69,    70,
      71,    71,    71,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      73,    73,    74,    75,    75,    75,    76,    76,    76,    77,
      77,    78,    78,    78,    79,    79,    79,    80,    80,    80,
      81,    81,    82,    82
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     3,     1,     2,     0,     2,     1,     1,
       1,     1,     1,     1,     2,     1,     6,     2,     2,     0,
       6,     7,     2,     0,     1,     2,     0,     1,     1,     3,
       8,     6,     2,     1,     0,     2,     0,     4,     2,     3,
       1,     3,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     3,     3,     3,     3,     3,
       5,     2,     3,     1,     3,     0,     3,     1,     5,     1,
       3,     1,     4,     3,     2,     4,     5,     3,     1,     1,
       3,     0,     1,     3
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
#if NYAA_YYDEBUG

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
# if defined NYAA_YYLTYPE_IS_TRIVIAL && NYAA_YYLTYPE_IS_TRIVIAL

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
#else /* !NYAA_YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !NYAA_YYDEBUG */


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
# if defined NYAA_YYLTYPE_IS_TRIVIAL && NYAA_YYLTYPE_IS_TRIVIAL
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
#line 91 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.block) = ctx->factory->NewBlock((yyvsp[0].stmts), (yylsp[0]));
    ctx->block = (yyval.block);
}
#line 1570 "syntax.cc" /* yacc.c:1667  */
    break;

  case 3:
#line 96 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.block) = ctx->factory->NewBlock((yyvsp[-1].stmts), (yylsp[-1]));
}
#line 1578 "syntax.cc" /* yacc.c:1667  */
    break;

  case 4:
#line 100 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmts) = ctx->factory->NewList((yyvsp[0].stmt));
}
#line 1586 "syntax.cc" /* yacc.c:1667  */
    break;

  case 5:
#line 103 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmts)->push_back((yyvsp[0].stmt));
}
#line 1594 "syntax.cc" /* yacc.c:1667  */
    break;

  case 6:
#line 106 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmts) = nullptr;
}
#line 1602 "syntax.cc" /* yacc.c:1667  */
    break;

  case 7:
#line 110 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewReturn((yyvsp[0].exprs), (yylsp[0]));
}
#line 1610 "syntax.cc" /* yacc.c:1667  */
    break;

  case 8:
#line 113 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1618 "syntax.cc" /* yacc.c:1667  */
    break;

  case 9:
#line 116 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1626 "syntax.cc" /* yacc.c:1667  */
    break;

  case 10:
#line 119 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1634 "syntax.cc" /* yacc.c:1667  */
    break;

  case 11:
#line 122 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1642 "syntax.cc" /* yacc.c:1667  */
    break;

  case 12:
#line 125 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1650 "syntax.cc" /* yacc.c:1667  */
    break;

  case 13:
#line 128 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].expr);
}
#line 1658 "syntax.cc" /* yacc.c:1667  */
    break;

  case 14:
#line 131 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].block);
}
#line 1666 "syntax.cc" /* yacc.c:1667  */
    break;

  case 15:
#line 134 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1674 "syntax.cc" /* yacc.c:1667  */
    break;

  case 16:
#line 138 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewIfStatement((yyvsp[-3].expr), (yyvsp[-1].block), (yyvsp[0].stmt), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 1682 "syntax.cc" /* yacc.c:1667  */
    break;

  case 17:
#line 142 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].block);
}
#line 1690 "syntax.cc" /* yacc.c:1667  */
    break;

  case 18:
#line 145 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1698 "syntax.cc" /* yacc.c:1667  */
    break;

  case 19:
#line 148 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = nullptr;
}
#line 1706 "syntax.cc" /* yacc.c:1667  */
    break;

  case 20:
#line 152 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewObjectDefinition((yyvsp[-4].names), (yyvsp[-3].name), (yyvsp[-1].stmts), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 1714 "syntax.cc" /* yacc.c:1667  */
    break;

  case 21:
#line 156 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewClassDefinition((yyvsp[-5].names), (yyvsp[-4].name), (yyvsp[-3].expr), (yyvsp[-1].stmts), Location::Concat((yylsp[-6]), (yylsp[0])));
}
#line 1722 "syntax.cc" /* yacc.c:1667  */
    break;

  case 22:
#line 160 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewVariable((yyvsp[0].name), (yylsp[0]));
}
#line 1730 "syntax.cc" /* yacc.c:1667  */
    break;

  case 23:
#line 163 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = nullptr;
}
#line 1738 "syntax.cc" /* yacc.c:1667  */
    break;

  case 24:
#line 167 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmts) = ctx->factory->NewList((yyvsp[0].stmt));
}
#line 1746 "syntax.cc" /* yacc.c:1667  */
    break;

  case 25:
#line 170 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmts)->push_back((yyvsp[0].stmt));
}
#line 1754 "syntax.cc" /* yacc.c:1667  */
    break;

  case 26:
#line 173 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmts) = nullptr;
}
#line 1762 "syntax.cc" /* yacc.c:1667  */
    break;

  case 27:
#line 177 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1770 "syntax.cc" /* yacc.c:1667  */
    break;

  case 28:
#line 180 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1778 "syntax.cc" /* yacc.c:1667  */
    break;

  case 29:
#line 184 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewPropertyDeclaration((yyvsp[-1].names), (yyvsp[0].names), nullptr, Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 1786 "syntax.cc" /* yacc.c:1667  */
    break;

  case 30:
#line 188 "syntax.y" /* yacc.c:1667  */
    {
    auto lambda = ctx->factory->NewLambdaLiteral((yyvsp[-2].params).params, (yyvsp[-2].params).vargs, (yyvsp[0].block), Location::Concat((yylsp[-3]), (yylsp[0])));
    auto self = ctx->factory->NewVariable((yyvsp[-6].name), (yylsp[-6]));
    (yyval.stmt) =  ctx->factory->NewFunctionDefinition(self, (yyvsp[-4].name), lambda, Location::Concat((yylsp[-7]), (yylsp[0])));
}
#line 1796 "syntax.cc" /* yacc.c:1667  */
    break;

  case 31:
#line 193 "syntax.y" /* yacc.c:1667  */
    {
    auto lambda = ctx->factory->NewLambdaLiteral((yyvsp[-2].params).params, (yyvsp[-2].params).vargs, (yyvsp[0].block), Location::Concat((yylsp[-3]), (yylsp[0])));
    (yyval.stmt) =  ctx->factory->NewFunctionDefinition(nullptr, (yyvsp[-4].name), lambda, Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 1805 "syntax.cc" /* yacc.c:1667  */
    break;

  case 32:
#line 198 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.params).params = (yyvsp[-1].names);
    (yyval.params).vargs  = (yyvsp[0].bool_val);
}
#line 1814 "syntax.cc" /* yacc.c:1667  */
    break;

  case 33:
#line 202 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.params).params = nullptr;
    (yyval.params).vargs  = true;    
}
#line 1823 "syntax.cc" /* yacc.c:1667  */
    break;

  case 34:
#line 206 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.params).params = nullptr;
    (yyval.params).vargs  = false;
}
#line 1832 "syntax.cc" /* yacc.c:1667  */
    break;

  case 35:
#line 211 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) =  true;
}
#line 1840 "syntax.cc" /* yacc.c:1667  */
    break;

  case 36:
#line 214 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 1848 "syntax.cc" /* yacc.c:1667  */
    break;

  case 37:
#line 218 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewVarDeclaration((yyvsp[-2].names), (yyvsp[0].exprs), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 1856 "syntax.cc" /* yacc.c:1667  */
    break;

  case 38:
#line 221 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewVarDeclaration((yyvsp[0].names), nullptr, Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 1864 "syntax.cc" /* yacc.c:1667  */
    break;

  case 39:
#line 225 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewAssignment((yyvsp[-2].lvals), (yyvsp[0].exprs), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 1872 "syntax.cc" /* yacc.c:1667  */
    break;

  case 40:
#line 229 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.exprs) = ctx->factory->NewList((yyvsp[0].expr));
}
#line 1880 "syntax.cc" /* yacc.c:1667  */
    break;

  case 41:
#line 232 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.exprs)->push_back((yyvsp[0].expr));
}
#line 1888 "syntax.cc" /* yacc.c:1667  */
    break;

  case 42:
#line 235 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.exprs) = nullptr;
}
#line 1896 "syntax.cc" /* yacc.c:1667  */
    break;

  case 43:
#line 239 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[0].lval);
}
#line 1904 "syntax.cc" /* yacc.c:1667  */
    break;

  case 44:
#line 242 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 1912 "syntax.cc" /* yacc.c:1667  */
    break;

  case 45:
#line 245 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewNilLiteral((yylsp[0]));
}
#line 1920 "syntax.cc" /* yacc.c:1667  */
    break;

  case 46:
#line 248 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewSmiLiteral((yyvsp[0].smi_val), (yylsp[0]));
}
#line 1928 "syntax.cc" /* yacc.c:1667  */
    break;

  case 47:
#line 251 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewSmiLiteral((yyvsp[0].smi_val), (yylsp[0]));
}
#line 1936 "syntax.cc" /* yacc.c:1667  */
    break;

  case 48:
#line 254 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewIntLiteral((yyvsp[0].int_val), (yylsp[0]));
}
#line 1944 "syntax.cc" /* yacc.c:1667  */
    break;

  case 49:
#line 257 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewApproxLiteral((yyvsp[0].f64_val), (yylsp[0]));
}
#line 1952 "syntax.cc" /* yacc.c:1667  */
    break;

  case 50:
#line 260 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewStringLiteral((yyvsp[0].str_val), (yylsp[0]));
}
#line 1960 "syntax.cc" /* yacc.c:1667  */
    break;

  case 51:
#line 263 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 1968 "syntax.cc" /* yacc.c:1667  */
    break;

  case 52:
#line 266 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 1976 "syntax.cc" /* yacc.c:1667  */
    break;

  case 53:
#line 269 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewVariableArguments((yylsp[0]));
}
#line 1984 "syntax.cc" /* yacc.c:1667  */
    break;

  case 54:
#line 272 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinary((yyvsp[-1].op), (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 1992 "syntax.cc" /* yacc.c:1667  */
    break;

  case 55:
#line 275 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinary(Operator::kAdd, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2000 "syntax.cc" /* yacc.c:1667  */
    break;

  case 56:
#line 278 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinary(Operator::kSub, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2008 "syntax.cc" /* yacc.c:1667  */
    break;

  case 57:
#line 281 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinary(Operator::kMul, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2016 "syntax.cc" /* yacc.c:1667  */
    break;

  case 58:
#line 284 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinary(Operator::kDiv, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2024 "syntax.cc" /* yacc.c:1667  */
    break;

  case 59:
#line 287 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2032 "syntax.cc" /* yacc.c:1667  */
    break;

  case 60:
#line 291 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewLambdaLiteral((yyvsp[-2].params).params, (yyvsp[-2].params).vargs, (yyvsp[0].block), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 2040 "syntax.cc" /* yacc.c:1667  */
    break;

  case 61:
#line 294 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewLambdaLiteral(nullptr, false, (yyvsp[0].block), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2048 "syntax.cc" /* yacc.c:1667  */
    break;

  case 62:
#line 298 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewMapInitializer((yyvsp[-1].entries), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2056 "syntax.cc" /* yacc.c:1667  */
    break;

  case 63:
#line 302 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.entries) = ctx->factory->NewList((yyvsp[0].entry));
}
#line 2064 "syntax.cc" /* yacc.c:1667  */
    break;

  case 64:
#line 305 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.entries)->push_back((yyvsp[0].entry));
}
#line 2072 "syntax.cc" /* yacc.c:1667  */
    break;

  case 65:
#line 308 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.entries) = nullptr;
}
#line 2080 "syntax.cc" /* yacc.c:1667  */
    break;

  case 66:
#line 312 "syntax.y" /* yacc.c:1667  */
    {
    auto key = ctx->factory->NewStringLiteral((yyvsp[-2].name), (yylsp[-2]));
    (yyval.entry) = ctx->factory->NewEntry(key, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2089 "syntax.cc" /* yacc.c:1667  */
    break;

  case 67:
#line 316 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.entry) = ctx->factory->NewEntry(nullptr, (yyvsp[0].expr), (yylsp[0]));
}
#line 2097 "syntax.cc" /* yacc.c:1667  */
    break;

  case 68:
#line 319 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.entry) = ctx->factory->NewEntry((yyvsp[-3].expr), (yyvsp[0].expr), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 2105 "syntax.cc" /* yacc.c:1667  */
    break;

  case 69:
#line 323 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.lvals) = ctx->factory->NewList((yyvsp[0].lval));
}
#line 2113 "syntax.cc" /* yacc.c:1667  */
    break;

  case 70:
#line 326 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.lvals)->push_back((yyvsp[0].lval));
}
#line 2121 "syntax.cc" /* yacc.c:1667  */
    break;

  case 71:
#line 330 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.lval) = ctx->factory->NewVariable((yyvsp[0].name), (yylsp[0]));
}
#line 2129 "syntax.cc" /* yacc.c:1667  */
    break;

  case 72:
#line 333 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.lval) = ctx->factory->NewIndex((yyvsp[-3].expr), (yyvsp[-1].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2137 "syntax.cc" /* yacc.c:1667  */
    break;

  case 73:
#line 336 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.lval) = ctx->factory->NewDotField((yyvsp[-2].expr), (yyvsp[0].name), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2145 "syntax.cc" /* yacc.c:1667  */
    break;

  case 74:
#line 340 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewCall((yyvsp[-1].expr), (yyvsp[0].exprs), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2153 "syntax.cc" /* yacc.c:1667  */
    break;

  case 75:
#line 343 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewSelfCall((yyvsp[-3].expr), (yyvsp[-1].name), (yyvsp[0].exprs), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2161 "syntax.cc" /* yacc.c:1667  */
    break;

  case 76:
#line 346 "syntax.y" /* yacc.c:1667  */
    {
    auto callee = ctx->factory->NewVariable((yyvsp[-3].name), (yylsp[-3]));
    (yyval.expr) = ctx->factory->NewNew(callee, (yyvsp[-1].exprs), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 2170 "syntax.cc" /* yacc.c:1667  */
    break;

  case 77:
#line 351 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.exprs) = (yyvsp[-1].exprs);
}
#line 2178 "syntax.cc" /* yacc.c:1667  */
    break;

  case 78:
#line 354 "syntax.y" /* yacc.c:1667  */
    {
    auto arg0 = ctx->factory->NewStringLiteral((yyvsp[0].str_val), (yylsp[0]));
    (yyval.exprs) = ctx->factory->NewList<mai::nyaa::ast::Expression *>(arg0);
}
#line 2187 "syntax.cc" /* yacc.c:1667  */
    break;

  case 79:
#line 358 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.exprs) = ctx->factory->NewList<mai::nyaa::ast::Expression *>((yyvsp[0].expr));
}
#line 2195 "syntax.cc" /* yacc.c:1667  */
    break;

  case 80:
#line 362 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.names) = (yyvsp[-1].names);
}
#line 2203 "syntax.cc" /* yacc.c:1667  */
    break;

  case 81:
#line 365 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.names) = nullptr;
}
#line 2211 "syntax.cc" /* yacc.c:1667  */
    break;

  case 82:
#line 370 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.names) = ctx->factory->NewList((yyvsp[0].name));
}
#line 2219 "syntax.cc" /* yacc.c:1667  */
    break;

  case 83:
#line 373 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.names)->push_back((yyvsp[0].name));
}
#line 2227 "syntax.cc" /* yacc.c:1667  */
    break;


#line 2231 "syntax.cc" /* yacc.c:1667  */
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
#line 378 "syntax.y" /* yacc.c:1918  */

void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg) {
    ctx->err_line   = yyl->first_line;
    ctx->err_column = yyl->first_column;
    ctx->err_msg    = ::mai::nyaa::ast::String::New(ctx->arena, msg);
}

