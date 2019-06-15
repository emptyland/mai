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
    OP_CONCAT = 269,
    NEW = 270,
    IF = 271,
    ELSE = 272,
    WHILE = 273,
    FOR = 274,
    IN = 275,
    OBJECT = 276,
    CLASS = 277,
    PROPERTY = 278,
    BREAK = 279,
    CONTINUE = 280,
    RETURN = 281,
    VARGS = 282,
    DO = 283,
    TINY_ARROW = 284,
    STRING_LITERAL = 285,
    SMI_LITERAL = 286,
    APPROX_LITERAL = 287,
    INT_LITERAL = 288,
    NIL_LITERAL = 289,
    BOOL_LITERAL = 290,
    TOKEN_ERROR = 291,
    IS = 292,
    OP_NOT = 293
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

#line 207 "syntax.cc" /* yacc.c:352  */
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
#define YYFINAL  69
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   524

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  58
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  33
/* YYNRULES -- Number of rules.  */
#define YYNRULES  95
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  181

#define YYUNDEFTOK  2
#define YYMAXUTOK   293

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
       2,     2,     2,    40,     2,     2,     2,    47,    42,     2,
      51,    52,    45,    43,    55,    44,    54,    46,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    53,     2,
       2,    37,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    56,     2,    57,    48,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    49,    41,    50,     2,     2,     2,     2,
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
      35,    36,    38,    39
};

#if NYAA_YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    92,    92,    97,   101,   104,   107,   111,   114,   117,
     120,   123,   126,   129,   132,   135,   138,   141,   144,   147,
     151,   155,   158,   161,   165,   169,   173,   177,   181,   184,
     188,   191,   194,   198,   201,   205,   209,   214,   219,   223,
     227,   232,   235,   239,   242,   246,   250,   253,   256,   260,
     263,   266,   269,   272,   275,   278,   281,   284,   287,   290,
     293,   296,   299,   302,   305,   308,   311,   314,   317,   320,
     323,   327,   331,   336,   339,   346,   350,   353,   356,   360,
     364,   367,   371,   374,   378,   381,   384,   388,   391,   394,
     399,   402,   407,   410,   415,   418
};
#endif

#if NYAA_YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "DEF", "VAR", "LAMBDA", "NAME",
  "COMPARISON", "OP_OR", "OP_XOR", "OP_AND", "OP_LSHIFT", "OP_RSHIFT",
  "UMINUS", "OP_CONCAT", "NEW", "IF", "ELSE", "WHILE", "FOR", "IN",
  "OBJECT", "CLASS", "PROPERTY", "BREAK", "CONTINUE", "RETURN", "VARGS",
  "DO", "TINY_ARROW", "STRING_LITERAL", "SMI_LITERAL", "APPROX_LITERAL",
  "INT_LITERAL", "NIL_LITERAL", "BOOL_LITERAL", "TOKEN_ERROR", "'='", "IS",
  "OP_NOT", "'!'", "'|'", "'&'", "'+'", "'-'", "'*'", "'/'", "'%'", "'^'",
  "'{'", "'}'", "'('", "')'", "':'", "'.'", "','", "'['", "']'", "$accept",
  "Script", "Block", "StatementList", "Statement", "IfStatement",
  "ElseClause", "WhileLoop", "ForIterateLoop", "ObjectDefinition",
  "ClassDefinition", "InheritClause", "MemberList", "MemberDefinition",
  "PropertyDeclaration", "FunctionDefinition", "Parameters",
  "ParameterVargs", "VarDeclaration", "Assignment", "ExpressionList",
  "Expression", "Concat", "LambdaLiteral", "MapInitializer",
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
     285,   286,   287,   288,   289,   290,   291,    61,   292,   293,
      33,   124,    38,    43,    45,    42,    47,    37,    94,   123,
     125,    40,    41,    58,    46,    44,    91,    93
};
# endif

#define YYPACT_NINF -153

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-153)))

#define YYTABLE_NINF -84

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     426,    11,    23,     8,  -153,    25,   -24,     6,    23,    27,
      27,  -153,  -153,   457,  -153,    12,  -153,  -153,  -153,  -153,
    -153,  -153,   457,     7,   457,    77,   426,  -153,  -153,  -153,
    -153,  -153,  -153,  -153,  -153,  -153,   267,    70,  -153,  -153,
     -22,    30,  -153,   -35,  -153,    34,  -153,     5,    68,    36,
      53,   457,   457,   -11,    23,    99,   106,    58,   267,  -153,
     426,  -153,   335,    61,   457,   267,    48,  -153,   165,  -153,
    -153,   457,   457,   457,   457,  -153,   457,   457,   457,   457,
     457,   457,   112,   117,   457,  -153,   457,   457,   457,     5,
     120,   457,   122,    84,   457,    43,  -153,   457,   192,   217,
     457,   -37,    96,   100,   457,   389,   457,    86,  -153,     7,
    -153,   456,   317,   335,   292,   468,   468,    71,    71,    71,
      83,    21,  -153,   140,   292,    58,   267,    62,   103,   109,
      58,  -153,    12,    58,  -153,    89,    12,    12,   242,  -153,
      42,   151,   114,   267,  -153,   267,   124,  -153,  -153,  -153,
    -153,    12,     5,  -153,  -153,   147,  -153,  -153,    27,     0,
    -153,  -153,  -153,  -153,    42,   457,  -153,   113,    -6,  -153,
      23,  -153,  -153,    45,   267,    12,  -153,  -153,   111,  -153,
    -153
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       6,     0,     0,    40,    84,     0,     0,     0,     0,    93,
      93,    18,    19,    48,    60,     0,    56,    52,    55,    54,
      51,    53,     0,    78,     0,     0,     2,     4,    15,    16,
      17,     9,    10,    11,     8,    12,    13,    59,    57,    58,
       0,    49,    50,     0,    94,    44,    39,    40,     0,    42,
       0,     0,     0,     0,     0,     0,     0,     7,    46,    49,
       6,    14,    61,    84,     0,    80,     0,    76,     0,     1,
       5,     0,     0,     0,     0,    91,     0,     0,     0,     0,
       0,    48,     0,     0,     0,    87,     0,    48,     0,    40,
       0,    48,     0,     0,    48,     0,    38,    48,     0,     0,
       0,     0,     0,    29,     0,     0,     0,     0,    75,     0,
      70,    64,    63,    62,    71,    65,    66,    67,    68,    69,
       0,     0,    86,     0,    72,    45,     0,    49,     0,     0,
      43,    95,     0,    74,    41,     0,     0,     0,     0,    92,
      32,     0,     0,    47,     3,    79,     0,    77,    90,    88,
      85,     0,    40,    73,    89,    23,    24,    25,    93,     0,
      30,    33,    34,    28,    32,     0,    37,     0,     0,    20,
       0,    26,    31,     0,    81,     0,    21,    22,    35,    27,
      36
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -153,  -153,   -17,   116,   -18,     1,  -153,  -153,  -153,  -153,
    -153,  -153,    10,  -152,  -153,  -104,   -42,  -153,  -153,  -153,
      65,     2,  -153,  -153,  -153,  -153,    69,  -153,     4,  -153,
      50,    -9,    -2
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    25,    61,    26,    27,    28,   169,    29,    30,    31,
      32,   142,   159,   160,   161,    33,    48,    96,    34,    35,
      57,    58,    37,    38,    39,    66,    67,    40,    59,    42,
      85,    55,    49
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      45,    56,    36,     1,    41,    93,    53,   172,    70,   100,
       6,    44,     3,    63,    44,    87,    89,    43,    92,    90,
     139,   172,     5,   158,    62,    65,    68,    51,    36,    44,
      41,    50,    46,    88,    14,    46,   162,    16,    17,    18,
      19,    20,    21,    60,    92,     1,    22,   128,     1,   131,
     171,    75,   101,    98,    99,   162,    23,    52,    24,    47,
     162,    60,    36,    64,    41,   158,   107,   -82,   158,   162,
     134,    91,    81,   111,   112,   113,   114,    69,   115,   116,
     117,   118,   119,    54,    86,   -82,   123,    70,   124,    92,
     126,    95,   127,    71,    72,   179,    73,    94,   108,   -83,
      74,    75,   138,   109,    97,   102,   143,    36,   145,    41,
     167,    65,   103,   104,   106,   153,    75,   -83,   121,   155,
     156,   157,    81,   122,    82,    83,   129,    84,   131,    76,
      77,    78,    79,    80,   166,   148,   132,    81,   104,    82,
      83,   154,    84,   146,   104,   140,   120,    71,    72,   170,
      73,   176,   125,   141,    74,   151,   130,   163,   180,   133,
     152,   165,   135,   164,   168,   175,    92,   174,   178,   177,
      75,   149,    71,    72,   173,    73,   105,     0,   147,    74,
       0,     0,     0,    76,    77,    78,    79,    80,     0,     0,
       0,    81,     0,    82,    83,    75,    84,   150,     0,    71,
      72,     0,    73,     0,     0,     0,    74,     0,    76,    77,
      78,    79,    80,     0,     0,     0,    81,   110,    82,    83,
       0,    84,    75,     0,    71,    72,     0,    73,     0,     0,
       0,    74,     0,     0,     0,    76,    77,    78,    79,    80,
       0,     0,     0,    81,   136,    82,    83,    75,    84,    71,
      72,     0,    73,     0,     0,     0,    74,     0,     0,     0,
      76,    77,    78,    79,    80,     0,     0,     0,    81,   137,
      82,    83,    75,    84,    71,    72,     0,    73,     0,     0,
       0,    74,     0,     0,     0,    76,    77,    78,    79,    80,
       0,    60,     0,    81,     0,    82,    83,    75,    84,    71,
      72,     0,    73,     0,     0,     0,     0,     0,     0,     0,
      76,    77,    78,    79,    80,     0,     0,     0,    81,     0,
      82,    83,    75,    84,    71,     0,     0,    73,     0,     0,
       0,     0,     0,     0,     0,    76,    77,    78,    79,    80,
       0,     0,    71,    81,     0,    82,    83,    75,    84,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      76,    77,    78,    79,    80,    75,     0,     0,    81,     0,
      82,    83,     0,    84,     0,     0,     0,     0,    76,    77,
      78,    79,    80,     0,     0,     0,    81,     0,    82,    83,
       0,    84,     1,     2,     3,     4,     0,     0,     0,     0,
       0,     0,     0,     0,     5,     6,     0,     7,     8,     0,
       9,    10,     0,    11,    12,    13,    14,    15,     0,    16,
      17,    18,    19,    20,    21,     0,     0,     0,    22,     1,
       2,     3,     4,     0,     0,     0,     0,     0,    23,   144,
      24,     5,     6,     0,     7,     8,     0,     9,    10,     0,
      11,    12,    13,    14,    15,     0,    16,    17,    18,    19,
      20,    21,     3,     4,     0,    22,     0,     0,     0,     0,
       0,     0,     5,     0,     0,    23,     0,    24,     0,     0,
       0,     0,     0,     0,    14,     0,    75,    16,    17,    18,
      19,    20,    21,     0,     0,     0,    22,     0,    75,    76,
      77,    78,    79,    80,     0,     0,    23,    81,    24,    82,
      83,     0,    84,    78,    79,    80,     0,     0,     0,    81,
       0,    82,    83,     0,    84
};

static const yytype_int16 yycheck[] =
{
       2,    10,     0,     3,     0,    47,     8,   159,    26,    20,
      16,     6,     5,     6,     6,    37,    51,     6,    55,    54,
      57,   173,    15,    23,    22,    23,    24,    51,    26,     6,
      26,     6,    27,    55,    27,    27,   140,    30,    31,    32,
      33,    34,    35,    49,    55,     3,    39,    89,     3,     6,
      50,    30,    54,    51,    52,   159,    49,    51,    51,    51,
     164,    49,    60,    56,    60,    23,    64,    37,    23,   173,
      27,    37,    51,    71,    72,    73,    74,     0,    76,    77,
      78,    79,    80,    56,    14,    55,    84,   105,    86,    55,
      88,    55,    88,     7,     8,    50,    10,    29,    50,    37,
      14,    30,   100,    55,    51,     6,   104,   105,   106,   105,
     152,   109,     6,    55,    53,   132,    30,    55,     6,   136,
     137,   138,    51,     6,    53,    54,     6,    56,     6,    43,
      44,    45,    46,    47,   151,    52,    52,    51,    55,    53,
      54,    52,    56,    57,    55,    49,    81,     7,     8,   158,
      10,   168,    87,    53,    14,    52,    91,     6,   175,    94,
      51,    37,    97,    49,    17,    52,    55,   165,   170,   168,
      30,   121,     7,     8,   164,    10,    60,    -1,   109,    14,
      -1,    -1,    -1,    43,    44,    45,    46,    47,    -1,    -1,
      -1,    51,    -1,    53,    54,    30,    56,    57,    -1,     7,
       8,    -1,    10,    -1,    -1,    -1,    14,    -1,    43,    44,
      45,    46,    47,    -1,    -1,    -1,    51,    52,    53,    54,
      -1,    56,    30,    -1,     7,     8,    -1,    10,    -1,    -1,
      -1,    14,    -1,    -1,    -1,    43,    44,    45,    46,    47,
      -1,    -1,    -1,    51,    52,    53,    54,    30,    56,     7,
       8,    -1,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,
      43,    44,    45,    46,    47,    -1,    -1,    -1,    51,    52,
      53,    54,    30,    56,     7,     8,    -1,    10,    -1,    -1,
      -1,    14,    -1,    -1,    -1,    43,    44,    45,    46,    47,
      -1,    49,    -1,    51,    -1,    53,    54,    30,    56,     7,
       8,    -1,    10,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      43,    44,    45,    46,    47,    -1,    -1,    -1,    51,    -1,
      53,    54,    30,    56,     7,    -1,    -1,    10,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    43,    44,    45,    46,    47,
      -1,    -1,     7,    51,    -1,    53,    54,    30,    56,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      43,    44,    45,    46,    47,    30,    -1,    -1,    51,    -1,
      53,    54,    -1,    56,    -1,    -1,    -1,    -1,    43,    44,
      45,    46,    47,    -1,    -1,    -1,    51,    -1,    53,    54,
      -1,    56,     3,     4,     5,     6,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    15,    16,    -1,    18,    19,    -1,
      21,    22,    -1,    24,    25,    26,    27,    28,    -1,    30,
      31,    32,    33,    34,    35,    -1,    -1,    -1,    39,     3,
       4,     5,     6,    -1,    -1,    -1,    -1,    -1,    49,    50,
      51,    15,    16,    -1,    18,    19,    -1,    21,    22,    -1,
      24,    25,    26,    27,    28,    -1,    30,    31,    32,    33,
      34,    35,     5,     6,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    -1,    15,    -1,    -1,    49,    -1,    51,    -1,    -1,
      -1,    -1,    -1,    -1,    27,    -1,    30,    30,    31,    32,
      33,    34,    35,    -1,    -1,    -1,    39,    -1,    30,    43,
      44,    45,    46,    47,    -1,    -1,    49,    51,    51,    53,
      54,    -1,    56,    45,    46,    47,    -1,    -1,    -1,    51,
      -1,    53,    54,    -1,    56
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,    15,    16,    18,    19,    21,
      22,    24,    25,    26,    27,    28,    30,    31,    32,    33,
      34,    35,    39,    49,    51,    59,    61,    62,    63,    65,
      66,    67,    68,    73,    76,    77,    79,    80,    81,    82,
      85,    86,    87,     6,     6,    90,    27,    51,    74,    90,
       6,    51,    51,    90,    56,    89,    89,    78,    79,    86,
      49,    60,    79,     6,    56,    79,    83,    84,    79,     0,
      62,     7,     8,    10,    14,    30,    43,    44,    45,    46,
      47,    51,    53,    54,    56,    88,    14,    37,    55,    51,
      54,    37,    55,    74,    29,    55,    75,    51,    79,    79,
      20,    90,     6,     6,    55,    61,    53,    79,    50,    55,
      52,    79,    79,    79,    79,    79,    79,    79,    79,    79,
      78,     6,     6,    79,    79,    78,    79,    86,    74,     6,
      78,     6,    52,    78,    27,    78,    52,    52,    79,    57,
      49,    53,    69,    79,    50,    79,    57,    84,    52,    88,
      57,    52,    51,    60,    52,    60,    60,    60,    23,    70,
      71,    72,    73,     6,    49,    37,    60,    74,    17,    64,
      89,    50,    71,    70,    79,    52,    60,    63,    90,    50,
      60
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    58,    59,    60,    61,    61,    61,    62,    62,    62,
      62,    62,    62,    62,    62,    62,    62,    62,    62,    62,
      63,    64,    64,    64,    65,    66,    67,    68,    69,    69,
      70,    70,    70,    71,    71,    72,    73,    73,    74,    74,
      74,    75,    75,    76,    76,    77,    78,    78,    78,    79,
      79,    79,    79,    79,    79,    79,    79,    79,    79,    79,
      79,    79,    79,    79,    79,    79,    79,    79,    79,    79,
      79,    80,    80,    81,    81,    82,    83,    83,    83,    84,
      84,    84,    85,    85,    86,    86,    86,    87,    87,    87,
      88,    88,    89,    89,    90,    90
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     3,     1,     2,     0,     2,     1,     1,
       1,     1,     1,     1,     2,     1,     1,     1,     1,     1,
       6,     2,     2,     0,     5,     5,     6,     7,     2,     0,
       1,     2,     0,     1,     1,     3,     8,     6,     2,     1,
       0,     2,     0,     4,     2,     3,     1,     3,     0,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     5,     4,     3,     1,     3,     0,     3,
       1,     5,     1,     3,     1,     4,     3,     2,     4,     5,
       3,     1,     3,     0,     1,     3
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
#line 92 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.block) = ctx->factory->NewBlock((yyvsp[0].stmts), (yylsp[0]));
    ctx->block = (yyval.block);
}
#line 1621 "syntax.cc" /* yacc.c:1667  */
    break;

  case 3:
#line 97 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.block) = ctx->factory->NewBlock((yyvsp[-1].stmts), (yylsp[-1]));
}
#line 1629 "syntax.cc" /* yacc.c:1667  */
    break;

  case 4:
#line 101 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmts) = ctx->factory->NewList((yyvsp[0].stmt));
}
#line 1637 "syntax.cc" /* yacc.c:1667  */
    break;

  case 5:
#line 104 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmts)->push_back((yyvsp[0].stmt));
}
#line 1645 "syntax.cc" /* yacc.c:1667  */
    break;

  case 6:
#line 107 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmts) = nullptr;
}
#line 1653 "syntax.cc" /* yacc.c:1667  */
    break;

  case 7:
#line 111 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewReturn((yyvsp[0].exprs), (yylsp[0]));
}
#line 1661 "syntax.cc" /* yacc.c:1667  */
    break;

  case 8:
#line 114 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1669 "syntax.cc" /* yacc.c:1667  */
    break;

  case 9:
#line 117 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1677 "syntax.cc" /* yacc.c:1667  */
    break;

  case 10:
#line 120 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1685 "syntax.cc" /* yacc.c:1667  */
    break;

  case 11:
#line 123 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1693 "syntax.cc" /* yacc.c:1667  */
    break;

  case 12:
#line 126 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1701 "syntax.cc" /* yacc.c:1667  */
    break;

  case 13:
#line 129 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].expr);
}
#line 1709 "syntax.cc" /* yacc.c:1667  */
    break;

  case 14:
#line 132 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].block);
}
#line 1717 "syntax.cc" /* yacc.c:1667  */
    break;

  case 15:
#line 135 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1725 "syntax.cc" /* yacc.c:1667  */
    break;

  case 16:
#line 138 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1733 "syntax.cc" /* yacc.c:1667  */
    break;

  case 17:
#line 141 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1741 "syntax.cc" /* yacc.c:1667  */
    break;

  case 18:
#line 144 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewBreak((yylsp[0]));
}
#line 1749 "syntax.cc" /* yacc.c:1667  */
    break;

  case 19:
#line 147 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewContinue((yylsp[0]));
}
#line 1757 "syntax.cc" /* yacc.c:1667  */
    break;

  case 20:
#line 151 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewIfStatement((yyvsp[-3].expr), (yyvsp[-1].block), (yyvsp[0].stmt), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 1765 "syntax.cc" /* yacc.c:1667  */
    break;

  case 21:
#line 155 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].block);
}
#line 1773 "syntax.cc" /* yacc.c:1667  */
    break;

  case 22:
#line 158 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1781 "syntax.cc" /* yacc.c:1667  */
    break;

  case 23:
#line 161 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = nullptr;
}
#line 1789 "syntax.cc" /* yacc.c:1667  */
    break;

  case 24:
#line 165 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewWhileLoop((yyvsp[-2].expr), (yyvsp[0].block), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 1797 "syntax.cc" /* yacc.c:1667  */
    break;

  case 25:
#line 169 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewForIterateLoop((yyvsp[-3].names), (yyvsp[-1].expr), (yyvsp[0].block), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 1805 "syntax.cc" /* yacc.c:1667  */
    break;

  case 26:
#line 173 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewObjectDefinition((yyvsp[-4].names), (yyvsp[-3].name), (yyvsp[-1].stmts), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 1813 "syntax.cc" /* yacc.c:1667  */
    break;

  case 27:
#line 177 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewClassDefinition((yyvsp[-5].names), (yyvsp[-4].name), (yyvsp[-3].expr), (yyvsp[-1].stmts), Location::Concat((yylsp[-6]), (yylsp[0])));
}
#line 1821 "syntax.cc" /* yacc.c:1667  */
    break;

  case 28:
#line 181 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewVariable((yyvsp[0].name), (yylsp[0]));
}
#line 1829 "syntax.cc" /* yacc.c:1667  */
    break;

  case 29:
#line 184 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = nullptr;
}
#line 1837 "syntax.cc" /* yacc.c:1667  */
    break;

  case 30:
#line 188 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmts) = ctx->factory->NewList((yyvsp[0].stmt));
}
#line 1845 "syntax.cc" /* yacc.c:1667  */
    break;

  case 31:
#line 191 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmts)->push_back((yyvsp[0].stmt));
}
#line 1853 "syntax.cc" /* yacc.c:1667  */
    break;

  case 32:
#line 194 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmts) = nullptr;
}
#line 1861 "syntax.cc" /* yacc.c:1667  */
    break;

  case 33:
#line 198 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1869 "syntax.cc" /* yacc.c:1667  */
    break;

  case 34:
#line 201 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1877 "syntax.cc" /* yacc.c:1667  */
    break;

  case 35:
#line 205 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewPropertyDeclaration((yyvsp[-1].names), (yyvsp[0].names), nullptr, Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 1885 "syntax.cc" /* yacc.c:1667  */
    break;

  case 36:
#line 209 "syntax.y" /* yacc.c:1667  */
    {
    auto lambda = ctx->factory->NewLambdaLiteral((yyvsp[-2].params).params, (yyvsp[-2].params).vargs, (yyvsp[0].block), Location::Concat((yylsp[-3]), (yylsp[0])));
    auto self = ctx->factory->NewVariable((yyvsp[-6].name), (yylsp[-6]));
    (yyval.stmt) =  ctx->factory->NewFunctionDefinition(self, (yyvsp[-4].name), lambda, Location::Concat((yylsp[-7]), (yylsp[0])));
}
#line 1895 "syntax.cc" /* yacc.c:1667  */
    break;

  case 37:
#line 214 "syntax.y" /* yacc.c:1667  */
    {
    auto lambda = ctx->factory->NewLambdaLiteral((yyvsp[-2].params).params, (yyvsp[-2].params).vargs, (yyvsp[0].block), Location::Concat((yylsp[-3]), (yylsp[0])));
    (yyval.stmt) =  ctx->factory->NewFunctionDefinition(nullptr, (yyvsp[-4].name), lambda, Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 1904 "syntax.cc" /* yacc.c:1667  */
    break;

  case 38:
#line 219 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.params).params = (yyvsp[-1].names);
    (yyval.params).vargs  = (yyvsp[0].bool_val);
}
#line 1913 "syntax.cc" /* yacc.c:1667  */
    break;

  case 39:
#line 223 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.params).params = nullptr;
    (yyval.params).vargs  = true;    
}
#line 1922 "syntax.cc" /* yacc.c:1667  */
    break;

  case 40:
#line 227 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.params).params = nullptr;
    (yyval.params).vargs  = false;
}
#line 1931 "syntax.cc" /* yacc.c:1667  */
    break;

  case 41:
#line 232 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) =  true;
}
#line 1939 "syntax.cc" /* yacc.c:1667  */
    break;

  case 42:
#line 235 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.bool_val) = false;
}
#line 1947 "syntax.cc" /* yacc.c:1667  */
    break;

  case 43:
#line 239 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewVarDeclaration((yyvsp[-2].names), (yyvsp[0].exprs), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 1955 "syntax.cc" /* yacc.c:1667  */
    break;

  case 44:
#line 242 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewVarDeclaration((yyvsp[0].names), nullptr, Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 1963 "syntax.cc" /* yacc.c:1667  */
    break;

  case 45:
#line 246 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.stmt) = ctx->factory->NewAssignment((yyvsp[-2].lvals), (yyvsp[0].exprs), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 1971 "syntax.cc" /* yacc.c:1667  */
    break;

  case 46:
#line 250 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.exprs) = ctx->factory->NewList((yyvsp[0].expr));
}
#line 1979 "syntax.cc" /* yacc.c:1667  */
    break;

  case 47:
#line 253 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.exprs)->push_back((yyvsp[0].expr));
}
#line 1987 "syntax.cc" /* yacc.c:1667  */
    break;

  case 48:
#line 256 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.exprs) = nullptr;
}
#line 1995 "syntax.cc" /* yacc.c:1667  */
    break;

  case 49:
#line 260 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[0].lval);
}
#line 2003 "syntax.cc" /* yacc.c:1667  */
    break;

  case 50:
#line 263 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2011 "syntax.cc" /* yacc.c:1667  */
    break;

  case 51:
#line 266 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewNilLiteral((yylsp[0]));
}
#line 2019 "syntax.cc" /* yacc.c:1667  */
    break;

  case 52:
#line 269 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewSmiLiteral((yyvsp[0].smi_val), (yylsp[0]));
}
#line 2027 "syntax.cc" /* yacc.c:1667  */
    break;

  case 53:
#line 272 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewSmiLiteral((yyvsp[0].smi_val), (yylsp[0]));
}
#line 2035 "syntax.cc" /* yacc.c:1667  */
    break;

  case 54:
#line 275 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewIntLiteral((yyvsp[0].int_val), (yylsp[0]));
}
#line 2043 "syntax.cc" /* yacc.c:1667  */
    break;

  case 55:
#line 278 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewApproxLiteral((yyvsp[0].f64_val), (yylsp[0]));
}
#line 2051 "syntax.cc" /* yacc.c:1667  */
    break;

  case 56:
#line 281 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewStringLiteral((yyvsp[0].str_val), (yylsp[0]));
}
#line 2059 "syntax.cc" /* yacc.c:1667  */
    break;

  case 57:
#line 284 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2067 "syntax.cc" /* yacc.c:1667  */
    break;

  case 58:
#line 287 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2075 "syntax.cc" /* yacc.c:1667  */
    break;

  case 59:
#line 290 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewConcat((yyvsp[0].exprs), (yylsp[0]));
}
#line 2083 "syntax.cc" /* yacc.c:1667  */
    break;

  case 60:
#line 293 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewVariableArguments((yylsp[0]));
}
#line 2091 "syntax.cc" /* yacc.c:1667  */
    break;

  case 61:
#line 296 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewUnary(Operator::kNot, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2099 "syntax.cc" /* yacc.c:1667  */
    break;

  case 62:
#line 299 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewAnd((yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2107 "syntax.cc" /* yacc.c:1667  */
    break;

  case 63:
#line 302 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewOr((yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2115 "syntax.cc" /* yacc.c:1667  */
    break;

  case 64:
#line 305 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinary((yyvsp[-1].op), (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2123 "syntax.cc" /* yacc.c:1667  */
    break;

  case 65:
#line 308 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinary(Operator::kAdd, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2131 "syntax.cc" /* yacc.c:1667  */
    break;

  case 66:
#line 311 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinary(Operator::kSub, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2139 "syntax.cc" /* yacc.c:1667  */
    break;

  case 67:
#line 314 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinary(Operator::kMul, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2147 "syntax.cc" /* yacc.c:1667  */
    break;

  case 68:
#line 317 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinary(Operator::kDiv, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2155 "syntax.cc" /* yacc.c:1667  */
    break;

  case 69:
#line 320 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewBinary(Operator::kMod, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2163 "syntax.cc" /* yacc.c:1667  */
    break;

  case 70:
#line 323 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2171 "syntax.cc" /* yacc.c:1667  */
    break;

  case 71:
#line 327 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.exprs) = ctx->factory->NewList((yyvsp[-2].expr));
    (yyval.exprs)->push_back((yyvsp[0].expr));
}
#line 2180 "syntax.cc" /* yacc.c:1667  */
    break;

  case 72:
#line 331 "syntax.y" /* yacc.c:1667  */
    {
    (yyvsp[-2].exprs)->push_back((yyvsp[0].expr));
    (yyval.exprs) = (yyvsp[-2].exprs);
}
#line 2189 "syntax.cc" /* yacc.c:1667  */
    break;

  case 73:
#line 336 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewLambdaLiteral((yyvsp[-2].params).params, (yyvsp[-2].params).vargs, (yyvsp[0].block), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 2197 "syntax.cc" /* yacc.c:1667  */
    break;

  case 74:
#line 339 "syntax.y" /* yacc.c:1667  */
    {
    auto stmt = ctx->factory->NewReturn((yyvsp[0].exprs), (yylsp[0]));
    auto stmts = ctx->factory->NewList<::mai::nyaa::ast::Statement *>(stmt);
    auto block = ctx->factory->NewBlock(stmts, (yylsp[0]));
    (yyval.expr) = ctx->factory->NewLambdaLiteral((yyvsp[-2].params).params, (yyvsp[-2].params).vargs, block, Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2208 "syntax.cc" /* yacc.c:1667  */
    break;

  case 75:
#line 346 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewMapInitializer((yyvsp[-1].entries), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2216 "syntax.cc" /* yacc.c:1667  */
    break;

  case 76:
#line 350 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.entries) = ctx->factory->NewList((yyvsp[0].entry));
}
#line 2224 "syntax.cc" /* yacc.c:1667  */
    break;

  case 77:
#line 353 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.entries)->push_back((yyvsp[0].entry));
}
#line 2232 "syntax.cc" /* yacc.c:1667  */
    break;

  case 78:
#line 356 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.entries) = nullptr;
}
#line 2240 "syntax.cc" /* yacc.c:1667  */
    break;

  case 79:
#line 360 "syntax.y" /* yacc.c:1667  */
    {
    auto key = ctx->factory->NewStringLiteral((yyvsp[-2].name), (yylsp[-2]));
    (yyval.entry) = ctx->factory->NewEntry(key, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2249 "syntax.cc" /* yacc.c:1667  */
    break;

  case 80:
#line 364 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.entry) = ctx->factory->NewEntry(nullptr, (yyvsp[0].expr), (yylsp[0]));
}
#line 2257 "syntax.cc" /* yacc.c:1667  */
    break;

  case 81:
#line 367 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.entry) = ctx->factory->NewEntry((yyvsp[-3].expr), (yyvsp[0].expr), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 2265 "syntax.cc" /* yacc.c:1667  */
    break;

  case 82:
#line 371 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.lvals) = ctx->factory->NewList((yyvsp[0].lval));
}
#line 2273 "syntax.cc" /* yacc.c:1667  */
    break;

  case 83:
#line 374 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.lvals)->push_back((yyvsp[0].lval));
}
#line 2281 "syntax.cc" /* yacc.c:1667  */
    break;

  case 84:
#line 378 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.lval) = ctx->factory->NewVariable((yyvsp[0].name), (yylsp[0]));
}
#line 2289 "syntax.cc" /* yacc.c:1667  */
    break;

  case 85:
#line 381 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.lval) = ctx->factory->NewIndex((yyvsp[-3].expr), (yyvsp[-1].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2297 "syntax.cc" /* yacc.c:1667  */
    break;

  case 86:
#line 384 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.lval) = ctx->factory->NewDotField((yyvsp[-2].expr), (yyvsp[0].name), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2305 "syntax.cc" /* yacc.c:1667  */
    break;

  case 87:
#line 388 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewCall((yyvsp[-1].expr), (yyvsp[0].exprs), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2313 "syntax.cc" /* yacc.c:1667  */
    break;

  case 88:
#line 391 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.expr) = ctx->factory->NewSelfCall((yyvsp[-3].expr), (yyvsp[-1].name), (yyvsp[0].exprs), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2321 "syntax.cc" /* yacc.c:1667  */
    break;

  case 89:
#line 394 "syntax.y" /* yacc.c:1667  */
    {
    auto callee = ctx->factory->NewVariable((yyvsp[-3].name), (yylsp[-3]));
    (yyval.expr) = ctx->factory->NewNew(callee, (yyvsp[-1].exprs), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 2330 "syntax.cc" /* yacc.c:1667  */
    break;

  case 90:
#line 399 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.exprs) = (yyvsp[-1].exprs);
}
#line 2338 "syntax.cc" /* yacc.c:1667  */
    break;

  case 91:
#line 402 "syntax.y" /* yacc.c:1667  */
    {
    auto arg0 = ctx->factory->NewStringLiteral((yyvsp[0].str_val), (yylsp[0]));
    (yyval.exprs) = ctx->factory->NewList<mai::nyaa::ast::Expression *>(arg0);
}
#line 2347 "syntax.cc" /* yacc.c:1667  */
    break;

  case 92:
#line 407 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.names) = (yyvsp[-1].names);
}
#line 2355 "syntax.cc" /* yacc.c:1667  */
    break;

  case 93:
#line 410 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.names) = nullptr;
}
#line 2363 "syntax.cc" /* yacc.c:1667  */
    break;

  case 94:
#line 415 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.names) = ctx->factory->NewList((yyvsp[0].name));
}
#line 2371 "syntax.cc" /* yacc.c:1667  */
    break;

  case 95:
#line 418 "syntax.y" /* yacc.c:1667  */
    {
    (yyval.names)->push_back((yyvsp[0].name));
}
#line 2379 "syntax.cc" /* yacc.c:1667  */
    break;


#line 2383 "syntax.cc" /* yacc.c:1667  */
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
#line 423 "syntax.y" /* yacc.c:1918  */

void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg) {
    ctx->err_line   = yyl->first_line;
    ctx->err_column = yyl->first_column;
    ctx->err_msg    = ::mai::nyaa::ast::String::New(ctx->arena, msg);
}

