/* A Bison parser, made by GNU Bison 3.4.1.  */

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
#define YYBISON_VERSION "3.4.1"

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
#line 12 "syntax.y"

#include "nyaa/ast.h"
#include "nyaa/parser-ctx.h"
#include "nyaa/syntax.hh"
#include "nyaa/lex.yy.h"
#include <string.h>

#define YYLEX_PARAM ctx->lex
#define NEXT_TRACE_ID (ctx->next_trace_id++)

using ::mai::nyaa::ast::Location;
using ::mai::nyaa::Operator;

void yyerror(YYLTYPE *, parser_ctx *, const char *);

#line 94 "syntax.cc"

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

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
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
    BLOCK_BEGIN = 259,
    BLOCK_END = 260,
    VAR = 261,
    VAL = 262,
    LAMBDA = 263,
    NAME = 264,
    COMPARISON = 265,
    OP_OR = 266,
    OP_XOR = 267,
    OP_AND = 268,
    OP_LSHIFT = 269,
    OP_RSHIFT = 270,
    UMINUS = 271,
    OP_CONCAT = 272,
    NEW = 273,
    TO = 274,
    UNTIL = 275,
    IF = 276,
    ELSE = 277,
    WHILE = 278,
    FOR = 279,
    IN = 280,
    OBJECT = 281,
    CLASS = 282,
    PROPERTY = 283,
    BREAK = 284,
    CONTINUE = 285,
    RETURN = 286,
    VARGS = 287,
    DO = 288,
    THIN_ARROW = 289,
    FAT_ARROW = 290,
    UNLESS = 291,
    THEN = 292,
    STRING_LITERAL = 293,
    SMI_LITERAL = 294,
    APPROX_LITERAL = 295,
    INT_LITERAL = 296,
    NIL_LITERAL = 297,
    BOOL_LITERAL = 298,
    TOKEN_ERROR = 299,
    IS = 300,
    OP_NOT = 301
  };
#endif

/* Value type.  */
#if ! defined NYAA_YYSTYPE && ! defined NYAA_YYSTYPE_IS_DECLARED
union NYAA_YYSTYPE
{
#line 28 "syntax.y"

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

#line 216 "syntax.cc"

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


#define YY_ASSERT(E) ((void) (0 && (E)))

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
#define YYFINAL  61
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   526

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  67
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  34
/* YYNRULES -- Number of rules.  */
#define YYNRULES  100
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  203

#define YYUNDEFTOK  2
#define YYMAXUTOK   301

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
       2,     2,     2,    48,     2,     2,     2,    55,    50,     2,
      60,    61,    53,    51,    64,    52,    63,    54,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    62,     2,
       2,    45,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    65,     2,    66,    56,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    58,    49,    59,    57,     2,     2,     2,
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
      46,    47
};

#if NYAA_YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    93,    93,    98,   102,   105,   109,   112,   115,   118,
     121,   124,   127,   130,   133,   136,   139,   142,   145,   149,
     153,   156,   159,   163,   167,   171,   174,   178,   182,   186,
     189,   193,   196,   200,   204,   209,   214,   217,   221,   225,
     229,   234,   237,   241,   244,   247,   251,   255,   258,   262,
     265,   268,   271,   274,   277,   280,   283,   286,   289,   292,
     295,   298,   301,   304,   307,   310,   313,   316,   319,   322,
     325,   328,   331,   334,   337,   340,   344,   348,   352,   355,
     358,   362,   366,   369,   373,   376,   379,   384,   387,   391,
     394,   398,   401,   404,   408,   411,   415,   419,   422,   426,
     429
};
#endif

#if NYAA_YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "DEF", "BLOCK_BEGIN", "BLOCK_END", "VAR",
  "VAL", "LAMBDA", "NAME", "COMPARISON", "OP_OR", "OP_XOR", "OP_AND",
  "OP_LSHIFT", "OP_RSHIFT", "UMINUS", "OP_CONCAT", "NEW", "TO", "UNTIL",
  "IF", "ELSE", "WHILE", "FOR", "IN", "OBJECT", "CLASS", "PROPERTY",
  "BREAK", "CONTINUE", "RETURN", "VARGS", "DO", "THIN_ARROW", "FAT_ARROW",
  "UNLESS", "THEN", "STRING_LITERAL", "SMI_LITERAL", "APPROX_LITERAL",
  "INT_LITERAL", "NIL_LITERAL", "BOOL_LITERAL", "TOKEN_ERROR", "'='", "IS",
  "OP_NOT", "'!'", "'|'", "'&'", "'+'", "'-'", "'*'", "'/'", "'%'", "'^'",
  "'~'", "'{'", "'}'", "'('", "')'", "':'", "'.'", "','", "'['", "']'",
  "$accept", "Script", "Block", "StatementList", "Statement",
  "IfStatement", "ElseClause", "WhileLoop", "ForIterateLoop",
  "ForStepLoop", "ObjectDefinition", "ClassDefinition", "MemberList",
  "MemberDefinition", "PropertyDeclaration", "FunctionDefinition",
  "FunctionBody", "Parameters", "ParameterVargs", "VarDeclaration",
  "Assignment", "ExpressionList", "Expression", "LambdaLiteral",
  "MapInitializer", "MapEntryList", "MapEntry", "Call", "Primary",
  "LValList", "LValue", "Arguments", "Attributes", "NameList", YY_NULLPTR
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
     295,   296,   297,   298,   299,    61,   300,   301,    33,   124,
      38,    43,    45,    42,    47,    37,    94,   126,   123,   125,
      40,    41,    58,    46,    44,    91,    93
};
# endif

#define YYPACT_NINF -118

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-118)))

#define YYTABLE_NINF -90

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     495,    39,    44,    44,  -118,    51,   -52,     1,     8,    41,
      41,  -118,  -118,   224,   111,   495,  -118,  -118,  -118,  -118,
    -118,  -118,  -118,  -118,  -118,  -118,    29,    54,    -7,    55,
      45,  -118,    -5,    40,    -4,   224,   224,   112,    44,   113,
     130,    80,  -118,  -118,  -118,  -118,  -118,  -118,  -118,   224,
     224,   224,    85,   224,    77,   397,  -118,  -118,  -118,    54,
      50,  -118,  -118,  -118,   188,   135,  -118,   224,   137,   145,
     224,    10,   146,   224,   153,   224,  -118,   244,   259,   122,
     -10,    72,   105,   108,    10,   413,  -118,  -118,   133,   224,
     397,   -13,  -118,   275,   224,   224,   224,   224,   224,   224,
     224,   224,   224,   224,   224,   224,   224,   224,  -118,    48,
      -4,    77,    50,  -118,   120,  -118,   107,   118,   117,    77,
    -118,    77,   125,   125,   224,   224,  -118,    13,    13,   123,
     224,   138,  -118,    85,  -118,   397,   429,   405,   413,   378,
     378,   456,   289,    43,    43,   129,   129,   129,  -118,  -118,
    -118,  -118,    -8,    12,  -118,    10,   495,   178,  -118,   386,
     308,    41,     4,  -118,  -118,  -118,    36,   125,   397,   157,
    -118,   224,  -118,  -118,  -118,   142,   192,   -15,  -118,   224,
     224,   125,    44,  -118,  -118,  -118,  -118,   224,    77,    -8,
    -118,  -118,  -118,   324,   337,  -118,   141,   397,  -118,   125,
     125,  -118,  -118
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,    91,     0,     0,     0,     0,    98,
      98,     7,     8,     0,     0,     2,     4,    13,    14,    15,
      16,    17,    18,     9,    10,    11,    12,     0,     0,    87,
       0,    99,    45,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    55,    54,    50,    53,    52,    49,    51,     0,
       0,     0,    80,     0,     6,    47,    75,    74,    88,    73,
      87,     1,     5,    95,     0,     0,    84,     0,     0,     0,
       0,    40,     0,     0,     0,     0,    86,     0,     0,    99,
       0,     0,     0,     0,    40,    56,    58,    57,    91,     0,
      82,     0,    78,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    96,     0,
       0,    46,    90,    93,     0,    39,     0,    42,     0,    43,
     100,    44,     0,     0,     0,     0,    97,     0,     0,     0,
       0,     0,    77,     0,    72,    48,    61,    60,    59,    67,
      68,    69,    70,    62,    63,    64,    65,    66,    71,    94,
      85,    92,     0,     0,    38,    40,     0,    22,    23,     0,
       0,    98,     0,    29,    31,    32,     0,     0,    81,     0,
      79,     0,    36,    35,    41,     0,     0,     0,    19,     0,
       0,     0,     0,    27,    30,    28,    76,     0,    37,     0,
       3,    20,    21,     0,     0,    24,    33,    83,    34,     0,
       0,    25,    26
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -118,  -118,  -112,    52,   -11,    30,  -118,  -118,  -118,  -118,
    -118,  -118,    81,  -117,  -118,  -104,    22,   -69,  -118,  -118,
    -118,   -42,   -23,  -118,  -118,  -118,    79,     3,     5,  -118,
       2,   -20,    -1,    -2
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    14,   172,    15,    16,    17,   178,    18,    19,    20,
      21,    22,   162,   163,   164,    23,   173,   116,   154,    24,
      25,    54,    55,    56,    57,    91,    92,    58,    59,    28,
      60,    66,    39,   117
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      32,    33,    29,    26,    62,    27,     6,     1,    35,    40,
     157,   158,    77,    78,    76,   129,     1,    29,    26,    31,
      27,   120,   109,   165,   165,   111,    85,    86,    87,    90,
      93,   119,   161,   121,    63,    80,    81,   171,    67,     1,
      73,   161,   115,   156,   174,   184,   132,   114,    30,   184,
     156,   133,   125,    31,    74,   186,    64,    68,   165,    74,
      34,    36,   165,   183,   161,   191,   131,   -88,    37,   195,
     112,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,    75,   175,   201,   202,   -88,
     150,   -88,    63,    41,    88,   185,   104,   105,   106,   107,
     -89,   159,   160,     5,    74,    71,    38,   168,    72,   149,
      90,    61,    94,    69,    64,    70,    65,    42,    69,   -89,
      70,    79,    82,    43,    44,    45,    46,    47,    48,   188,
      95,    96,    49,    97,    98,    99,    74,    50,   126,    83,
      84,    94,    51,    52,   110,    53,     4,   124,    95,    96,
      89,    97,    98,    99,   113,   118,   193,   194,    29,    26,
     182,    27,   120,   127,   197,    62,   128,   130,   152,   100,
     101,   102,   103,   104,   105,   106,   107,   155,    29,    26,
     196,    27,   153,   156,   167,   107,   151,   100,   101,   102,
     103,   104,   105,   106,   107,     1,    41,     4,     2,     3,
     177,     4,   187,   189,   169,    74,     5,   192,   176,   166,
       5,   198,   170,     6,     0,     7,     8,     0,     9,    10,
      42,    11,    12,    13,     0,     0,    43,    44,    45,    46,
      47,    48,    41,     4,     0,    49,     0,     0,     0,     0,
      50,     0,     5,     0,     0,    51,    52,     0,    53,   108,
       0,   190,     0,     0,    95,    96,    42,    97,    98,    99,
       0,     0,    43,    44,    45,    46,    47,    48,     0,    95,
      96,    49,    97,    98,    99,     0,    50,     0,     0,     0,
       0,    51,    52,     0,    53,    95,    96,     0,    97,    98,
      99,     0,     0,   100,   101,   102,   103,   104,   105,   106,
     107,     0,     0,    98,    99,   122,     0,     0,   100,   101,
     102,   103,   104,   105,   106,   107,     0,     0,    95,    96,
     123,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,     0,     0,    95,    96,   134,    97,    98,    99,
     102,   103,   104,   105,   106,   107,     0,    95,    96,     0,
      97,    98,    99,     0,     0,     0,     0,   100,   101,   102,
     103,   104,   105,   106,   107,     0,     0,     0,     0,   181,
       0,     0,     0,   100,   101,   102,   103,   104,   105,   106,
     107,     0,     0,     0,     0,   199,   100,   101,   102,   103,
     104,   105,   106,   107,     0,     0,    95,    96,   200,    97,
      98,    99,     0,     0,     0,   179,   180,    95,    96,     0,
      97,    98,    99,     0,     0,    95,     0,     0,    97,    98,
      99,     0,     0,    95,     0,     0,     0,    98,    99,   102,
     103,   104,   105,   106,   107,   100,   101,   102,   103,   104,
     105,   106,   107,    98,    99,     0,   100,   101,   102,   103,
     104,   105,   106,   107,   100,   101,   102,   103,   104,   105,
     106,   107,   100,   101,   102,   103,   104,   105,   106,   107,
      98,    99,     0,     0,     0,     0,     0,     0,   100,   101,
     102,   103,   104,   105,   106,   107,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     1,     0,
       0,     2,     3,     0,     4,     0,   101,   102,   103,   104,
     105,   106,   107,     5,     0,     0,     6,     0,     7,     8,
       0,     9,    10,     0,    11,    12,    13
};

static const yytype_int16 yycheck[] =
{
       2,     3,     0,     0,    15,     0,    21,     3,    60,    10,
     122,   123,    35,    36,    34,    84,     3,    15,    15,     9,
      15,     9,    64,   127,   128,    67,    49,    50,    51,    52,
      53,    73,    28,    75,    38,    37,    38,    45,    45,     3,
      45,    28,    32,    58,    32,   162,    59,    70,     9,   166,
      58,    64,    62,     9,    64,   167,    60,    64,   162,    64,
       9,    60,   166,    59,    28,   177,    89,    38,    60,   181,
      68,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,    45,   155,   199,   200,    60,
     110,    62,    38,     8,     9,    59,    53,    54,    55,    56,
      45,   124,   125,    18,    64,    60,    65,   130,    63,    61,
     133,     0,    64,    63,    60,    65,    62,    32,    63,    64,
      65,     9,     9,    38,    39,    40,    41,    42,    43,   171,
      10,    11,    47,    13,    14,    15,    64,    52,    66,     9,
      60,    64,    57,    58,     9,    60,     9,    25,    10,    11,
      65,    13,    14,    15,     9,     9,   179,   180,   156,   156,
     161,   156,     9,    58,   187,   176,    58,    34,    61,    49,
      50,    51,    52,    53,    54,    55,    56,    60,   176,   176,
     182,   176,    64,    58,    61,    56,    66,    49,    50,    51,
      52,    53,    54,    55,    56,     3,     8,     9,     6,     7,
      22,     9,    45,    61,    66,    64,    18,   177,   156,   128,
      18,   189,   133,    21,    -1,    23,    24,    -1,    26,    27,
      32,    29,    30,    31,    -1,    -1,    38,    39,    40,    41,
      42,    43,     8,     9,    -1,    47,    -1,    -1,    -1,    -1,
      52,    -1,    18,    -1,    -1,    57,    58,    -1,    60,    61,
      -1,    59,    -1,    -1,    10,    11,    32,    13,    14,    15,
      -1,    -1,    38,    39,    40,    41,    42,    43,    -1,    10,
      11,    47,    13,    14,    15,    -1,    52,    -1,    -1,    -1,
      -1,    57,    58,    -1,    60,    10,    11,    -1,    13,    14,
      15,    -1,    -1,    49,    50,    51,    52,    53,    54,    55,
      56,    -1,    -1,    14,    15,    61,    -1,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    -1,    -1,    10,    11,
      61,    13,    14,    15,    49,    50,    51,    52,    53,    54,
      55,    56,    -1,    -1,    10,    11,    61,    13,    14,    15,
      51,    52,    53,    54,    55,    56,    -1,    10,    11,    -1,
      13,    14,    15,    -1,    -1,    -1,    -1,    49,    50,    51,
      52,    53,    54,    55,    56,    -1,    -1,    -1,    -1,    61,
      -1,    -1,    -1,    49,    50,    51,    52,    53,    54,    55,
      56,    -1,    -1,    -1,    -1,    61,    49,    50,    51,    52,
      53,    54,    55,    56,    -1,    -1,    10,    11,    61,    13,
      14,    15,    -1,    -1,    -1,    19,    20,    10,    11,    -1,
      13,    14,    15,    -1,    -1,    10,    -1,    -1,    13,    14,
      15,    -1,    -1,    10,    -1,    -1,    -1,    14,    15,    51,
      52,    53,    54,    55,    56,    49,    50,    51,    52,    53,
      54,    55,    56,    14,    15,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    49,    50,    51,    52,    53,    54,
      55,    56,    49,    50,    51,    52,    53,    54,    55,    56,
      14,    15,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,    -1,
      -1,     6,     7,    -1,     9,    -1,    50,    51,    52,    53,
      54,    55,    56,    18,    -1,    -1,    21,    -1,    23,    24,
      -1,    26,    27,    -1,    29,    30,    31
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     6,     7,     9,    18,    21,    23,    24,    26,
      27,    29,    30,    31,    68,    70,    71,    72,    74,    75,
      76,    77,    78,    82,    86,    87,    94,    95,    96,    97,
       9,     9,   100,   100,     9,    60,    60,    60,    65,    99,
      99,     8,    32,    38,    39,    40,    41,    42,    43,    47,
      52,    57,    58,    60,    88,    89,    90,    91,    94,    95,
      97,     0,    71,    38,    60,    62,    98,    45,    64,    63,
      65,    60,    63,    45,    64,    45,    98,    89,    89,     9,
     100,   100,     9,     9,    60,    89,    89,    89,     9,    65,
      89,    92,    93,    89,    64,    10,    11,    13,    14,    15,
      49,    50,    51,    52,    53,    54,    55,    56,    61,    88,
       9,    88,    97,     9,    89,    32,    84,   100,     9,    88,
       9,    88,    61,    61,    25,    62,    66,    58,    58,    84,
      34,    89,    59,    64,    61,    89,    89,    89,    89,    89,
      89,    89,    89,    89,    89,    89,    89,    89,    89,    61,
      98,    66,    61,    64,    85,    60,    58,    69,    69,    89,
      89,    28,    79,    80,    81,    82,    79,    61,    89,    66,
      93,    45,    69,    83,    32,    84,    70,    22,    73,    19,
      20,    61,    99,    59,    80,    59,    69,    45,    88,    61,
      59,    69,    72,    89,    89,    69,   100,    89,    83,    61,
      61,    69,    69
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    67,    68,    69,    70,    70,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    72,
      73,    73,    73,    74,    75,    76,    76,    77,    78,    79,
      79,    80,    80,    81,    82,    82,    83,    83,    84,    84,
      84,    85,    85,    86,    86,    86,    87,    88,    88,    89,
      89,    89,    89,    89,    89,    89,    89,    89,    89,    89,
      89,    89,    89,    89,    89,    89,    89,    89,    89,    89,
      89,    89,    89,    89,    89,    89,    90,    91,    92,    92,
      92,    93,    93,    93,    94,    94,    94,    95,    95,    96,
      96,    97,    97,    97,    98,    98,    98,    99,    99,   100,
     100
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     3,     1,     2,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     6,
       2,     2,     0,     5,     7,     9,     9,     6,     6,     1,
       2,     1,     1,     3,     8,     6,     1,     2,     2,     1,
       0,     2,     0,     4,     4,     2,     3,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     2,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     1,     1,     1,     5,     3,     1,     3,
       0,     3,     1,     5,     2,     4,     3,     1,     1,     1,
       3,     1,     4,     3,     3,     1,     2,     3,     0,     1,
       3
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
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
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
#line 93 "syntax.y"
    {
    (yyval.block) = ctx->factory->NewBlock((yyvsp[0].stmts), (yylsp[0]));
    ctx->block = (yyval.block);
}
#line 1644 "syntax.cc"
    break;

  case 3:
#line 98 "syntax.y"
    {
    (yyval.block) = ctx->factory->NewBlock((yyvsp[-1].stmts), (yylsp[-1]));
}
#line 1652 "syntax.cc"
    break;

  case 4:
#line 102 "syntax.y"
    {
    (yyval.stmts) = ctx->factory->NewList((yyvsp[0].stmt));
}
#line 1660 "syntax.cc"
    break;

  case 5:
#line 105 "syntax.y"
    {
    (yyval.stmts)->push_back((yyvsp[0].stmt));
}
#line 1668 "syntax.cc"
    break;

  case 6:
#line 109 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewReturn((yyvsp[0].exprs), (yylsp[0]));
}
#line 1676 "syntax.cc"
    break;

  case 7:
#line 112 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewBreak((yylsp[0]));
}
#line 1684 "syntax.cc"
    break;

  case 8:
#line 115 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewContinue((yylsp[0]));
}
#line 1692 "syntax.cc"
    break;

  case 9:
#line 118 "syntax.y"
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1700 "syntax.cc"
    break;

  case 10:
#line 121 "syntax.y"
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1708 "syntax.cc"
    break;

  case 11:
#line 124 "syntax.y"
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1716 "syntax.cc"
    break;

  case 12:
#line 127 "syntax.y"
    {
    (yyval.stmt) = (yyvsp[0].expr);
}
#line 1724 "syntax.cc"
    break;

  case 13:
#line 130 "syntax.y"
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1732 "syntax.cc"
    break;

  case 14:
#line 133 "syntax.y"
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1740 "syntax.cc"
    break;

  case 15:
#line 136 "syntax.y"
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1748 "syntax.cc"
    break;

  case 16:
#line 139 "syntax.y"
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1756 "syntax.cc"
    break;

  case 17:
#line 142 "syntax.y"
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1764 "syntax.cc"
    break;

  case 18:
#line 145 "syntax.y"
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1772 "syntax.cc"
    break;

  case 19:
#line 149 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewIfStatement(NEXT_TRACE_ID, (yyvsp[-3].expr), (yyvsp[-1].block), (yyvsp[0].stmt), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 1780 "syntax.cc"
    break;

  case 20:
#line 153 "syntax.y"
    {
    (yyval.stmt) = (yyvsp[0].block);
}
#line 1788 "syntax.cc"
    break;

  case 21:
#line 156 "syntax.y"
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1796 "syntax.cc"
    break;

  case 22:
#line 159 "syntax.y"
    {
    (yyval.stmt) = nullptr;
}
#line 1804 "syntax.cc"
    break;

  case 23:
#line 163 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewWhileLoop((yyvsp[-2].expr), (yyvsp[0].block), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 1812 "syntax.cc"
    break;

  case 24:
#line 167 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewForIterateLoop(NEXT_TRACE_ID, (yyvsp[-4].names), (yyvsp[-2].expr), (yyvsp[0].block), Location::Concat((yylsp[-6]), (yylsp[0])));
}
#line 1820 "syntax.cc"
    break;

  case 25:
#line 171 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewForStepLoop(&ctx->next_trace_id, (yyvsp[-6].name), (yyvsp[-4].expr), false, (yyvsp[-2].expr), nullptr, (yyvsp[0].block), Location::Concat((yylsp[-8]), (yylsp[0])));
}
#line 1828 "syntax.cc"
    break;

  case 26:
#line 174 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewForStepLoop(&ctx->next_trace_id, (yyvsp[-6].name), (yyvsp[-4].expr), true,  (yyvsp[-2].expr), nullptr, (yyvsp[0].block), Location::Concat((yylsp[-8]), (yylsp[0])));
}
#line 1836 "syntax.cc"
    break;

  case 27:
#line 178 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewObjectDefinition((yyvsp[-4].names), (yyvsp[-3].name), (yyvsp[-1].stmts), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 1844 "syntax.cc"
    break;

  case 28:
#line 182 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewClassDefinition((yyvsp[-4].names), (yyvsp[-3].name), nullptr, (yyvsp[-1].stmts), Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 1852 "syntax.cc"
    break;

  case 29:
#line 186 "syntax.y"
    {
    (yyval.stmts) = ctx->factory->NewList((yyvsp[0].stmt));
}
#line 1860 "syntax.cc"
    break;

  case 30:
#line 189 "syntax.y"
    {
    (yyval.stmts)->push_back((yyvsp[0].stmt));
}
#line 1868 "syntax.cc"
    break;

  case 31:
#line 193 "syntax.y"
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1876 "syntax.cc"
    break;

  case 32:
#line 196 "syntax.y"
    {
    (yyval.stmt) = (yyvsp[0].stmt);
}
#line 1884 "syntax.cc"
    break;

  case 33:
#line 200 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewPropertyDeclaration((yyvsp[-1].names), (yyvsp[0].names), nullptr, Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 1892 "syntax.cc"
    break;

  case 34:
#line 204 "syntax.y"
    {
    auto lambda = ctx->factory->NewLambdaLiteral((yyvsp[-2].params).params, (yyvsp[-2].params).vargs, (yyvsp[0].block), Location::Concat((yylsp[-3]), (yylsp[0])));
    auto self = ctx->factory->NewVariable((yyvsp[-6].name), (yylsp[-6]));
    (yyval.stmt) =  ctx->factory->NewFunctionDefinition(NEXT_TRACE_ID, self, (yyvsp[-4].name), lambda, Location::Concat((yylsp[-7]), (yylsp[0])));
}
#line 1902 "syntax.cc"
    break;

  case 35:
#line 209 "syntax.y"
    {
    auto lambda = ctx->factory->NewLambdaLiteral((yyvsp[-2].params).params, (yyvsp[-2].params).vargs, (yyvsp[0].block), Location::Concat((yylsp[-3]), (yylsp[0])));
    (yyval.stmt) =  ctx->factory->NewFunctionDefinition(NEXT_TRACE_ID, nullptr, (yyvsp[-4].name), lambda, Location::Concat((yylsp[-5]), (yylsp[0])));
}
#line 1911 "syntax.cc"
    break;

  case 36:
#line 214 "syntax.y"
    {
    (yyval.block) = (yyvsp[0].block);
}
#line 1919 "syntax.cc"
    break;

  case 37:
#line 217 "syntax.y"
    {
    (yyval.block) = nullptr; // TODO:
}
#line 1927 "syntax.cc"
    break;

  case 38:
#line 221 "syntax.y"
    {
    (yyval.params).params = (yyvsp[-1].names);
    (yyval.params).vargs  = (yyvsp[0].bool_val);
}
#line 1936 "syntax.cc"
    break;

  case 39:
#line 225 "syntax.y"
    {
    (yyval.params).params = nullptr;
    (yyval.params).vargs  = true;    
}
#line 1945 "syntax.cc"
    break;

  case 40:
#line 229 "syntax.y"
    {
    (yyval.params).params = nullptr;
    (yyval.params).vargs  = false;
}
#line 1954 "syntax.cc"
    break;

  case 41:
#line 234 "syntax.y"
    {
    (yyval.bool_val) =  true;
}
#line 1962 "syntax.cc"
    break;

  case 42:
#line 237 "syntax.y"
    {
    (yyval.bool_val) = false;
}
#line 1970 "syntax.cc"
    break;

  case 43:
#line 241 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewVarDeclaration((yyvsp[-2].names), (yyvsp[0].exprs), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 1978 "syntax.cc"
    break;

  case 44:
#line 244 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewVarDeclaration((yyvsp[-2].names), (yyvsp[0].exprs), Location::Concat((yylsp[-3]), (yylsp[0]))); // TODO: readonly
}
#line 1986 "syntax.cc"
    break;

  case 45:
#line 247 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewVarDeclaration((yyvsp[0].names), nullptr, Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 1994 "syntax.cc"
    break;

  case 46:
#line 251 "syntax.y"
    {
    (yyval.stmt) = ctx->factory->NewAssignment((yyvsp[-2].lvals), (yyvsp[0].exprs), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2002 "syntax.cc"
    break;

  case 47:
#line 255 "syntax.y"
    {
    (yyval.exprs) = ctx->factory->NewList((yyvsp[0].expr));
}
#line 2010 "syntax.cc"
    break;

  case 48:
#line 258 "syntax.y"
    {
    (yyval.exprs)->push_back((yyvsp[0].expr));
}
#line 2018 "syntax.cc"
    break;

  case 49:
#line 262 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewNilLiteral((yylsp[0]));
}
#line 2026 "syntax.cc"
    break;

  case 50:
#line 265 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewSmiLiteral((yyvsp[0].smi_val), (yylsp[0]));
}
#line 2034 "syntax.cc"
    break;

  case 51:
#line 268 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewSmiLiteral((yyvsp[0].smi_val), (yylsp[0]));
}
#line 2042 "syntax.cc"
    break;

  case 52:
#line 271 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewIntLiteral((yyvsp[0].int_val), (yylsp[0]));
}
#line 2050 "syntax.cc"
    break;

  case 53:
#line 274 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewApproxLiteral((yyvsp[0].f64_val), (yylsp[0]));
}
#line 2058 "syntax.cc"
    break;

  case 54:
#line 277 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewStringLiteral((yyvsp[0].str_val), (yylsp[0]));
}
#line 2066 "syntax.cc"
    break;

  case 55:
#line 280 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewVariableArguments((yylsp[0]));
}
#line 2074 "syntax.cc"
    break;

  case 56:
#line 283 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewUnary(NEXT_TRACE_ID, Operator::kNot, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2082 "syntax.cc"
    break;

  case 57:
#line 286 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewUnary(NEXT_TRACE_ID, Operator::kBitInv, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2090 "syntax.cc"
    break;

  case 58:
#line 289 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewUnary(NEXT_TRACE_ID, Operator::kUnm, (yyvsp[0].expr), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2098 "syntax.cc"
    break;

  case 59:
#line 292 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewAnd((yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2106 "syntax.cc"
    break;

  case 60:
#line 295 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewOr((yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2114 "syntax.cc"
    break;

  case 61:
#line 298 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewBinary(NEXT_TRACE_ID, (yyvsp[-1].op), (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2122 "syntax.cc"
    break;

  case 62:
#line 301 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewBinary(NEXT_TRACE_ID, Operator::kAdd, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2130 "syntax.cc"
    break;

  case 63:
#line 304 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewBinary(NEXT_TRACE_ID, Operator::kSub, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2138 "syntax.cc"
    break;

  case 64:
#line 307 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewBinary(NEXT_TRACE_ID, Operator::kMul, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2146 "syntax.cc"
    break;

  case 65:
#line 310 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewBinary(NEXT_TRACE_ID, Operator::kDiv, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2154 "syntax.cc"
    break;

  case 66:
#line 313 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewBinary(NEXT_TRACE_ID, Operator::kMod, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2162 "syntax.cc"
    break;

  case 67:
#line 316 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewBinary(NEXT_TRACE_ID, Operator::kShl, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2170 "syntax.cc"
    break;

  case 68:
#line 319 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewBinary(NEXT_TRACE_ID, Operator::kShr, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2178 "syntax.cc"
    break;

  case 69:
#line 322 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewBinary(NEXT_TRACE_ID, Operator::kBitOr, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2186 "syntax.cc"
    break;

  case 70:
#line 325 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewBinary(NEXT_TRACE_ID, Operator::kBitAnd, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2194 "syntax.cc"
    break;

  case 71:
#line 328 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewBinary(NEXT_TRACE_ID, Operator::kBitXor, (yyvsp[-2].expr), (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2202 "syntax.cc"
    break;

  case 72:
#line 331 "syntax.y"
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 2210 "syntax.cc"
    break;

  case 73:
#line 334 "syntax.y"
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2218 "syntax.cc"
    break;

  case 74:
#line 337 "syntax.y"
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2226 "syntax.cc"
    break;

  case 75:
#line 340 "syntax.y"
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2234 "syntax.cc"
    break;

  case 76:
#line 344 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewLambdaLiteral((yyvsp[-2].params).params, (yyvsp[-2].params).vargs, (yyvsp[0].block), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 2242 "syntax.cc"
    break;

  case 77:
#line 348 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewMapInitializer((yyvsp[-1].entries), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2250 "syntax.cc"
    break;

  case 78:
#line 352 "syntax.y"
    {
    (yyval.entries) = ctx->factory->NewList((yyvsp[0].entry));
}
#line 2258 "syntax.cc"
    break;

  case 79:
#line 355 "syntax.y"
    {
    (yyval.entries)->push_back((yyvsp[0].entry));
}
#line 2266 "syntax.cc"
    break;

  case 80:
#line 358 "syntax.y"
    {
    (yyval.entries) = nullptr;
}
#line 2274 "syntax.cc"
    break;

  case 81:
#line 362 "syntax.y"
    {
    auto key = ctx->factory->NewStringLiteral((yyvsp[-2].name), (yylsp[-2]));
    (yyval.entry) = ctx->factory->NewEntry(key, (yyvsp[0].expr), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2283 "syntax.cc"
    break;

  case 82:
#line 366 "syntax.y"
    {
    (yyval.entry) = ctx->factory->NewEntry(nullptr, (yyvsp[0].expr), (yylsp[0]));
}
#line 2291 "syntax.cc"
    break;

  case 83:
#line 369 "syntax.y"
    {
    (yyval.entry) = ctx->factory->NewEntry((yyvsp[-3].expr), (yyvsp[0].expr), Location::Concat((yylsp[-4]), (yylsp[0])));
}
#line 2299 "syntax.cc"
    break;

  case 84:
#line 373 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewCall(NEXT_TRACE_ID, (yyvsp[-1].expr), (yyvsp[0].exprs), Location::Concat((yylsp[-1]), (yylsp[0])));
}
#line 2307 "syntax.cc"
    break;

  case 85:
#line 376 "syntax.y"
    {
    (yyval.expr) = ctx->factory->NewSelfCall(NEXT_TRACE_ID, (yyvsp[-3].expr), (yyvsp[-1].name), (yyvsp[0].exprs), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2315 "syntax.cc"
    break;

  case 86:
#line 379 "syntax.y"
    {
    auto callee = ctx->factory->NewVariable((yyvsp[-1].name), (yylsp[-1]));
    (yyval.expr) = ctx->factory->NewNew(NEXT_TRACE_ID, callee, (yyvsp[0].exprs), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2324 "syntax.cc"
    break;

  case 87:
#line 384 "syntax.y"
    {
    (yyval.expr) = (yyvsp[0].lval);
}
#line 2332 "syntax.cc"
    break;

  case 88:
#line 387 "syntax.y"
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 2340 "syntax.cc"
    break;

  case 89:
#line 391 "syntax.y"
    {
    (yyval.lvals) = ctx->factory->NewList((yyvsp[0].lval));
}
#line 2348 "syntax.cc"
    break;

  case 90:
#line 394 "syntax.y"
    {
    (yyval.lvals)->push_back((yyvsp[0].lval));
}
#line 2356 "syntax.cc"
    break;

  case 91:
#line 398 "syntax.y"
    {
    (yyval.lval) = ctx->factory->NewVariable((yyvsp[0].name), (yylsp[0]));
}
#line 2364 "syntax.cc"
    break;

  case 92:
#line 401 "syntax.y"
    {
    (yyval.lval) = ctx->factory->NewIndex(NEXT_TRACE_ID, (yyvsp[-3].lval), (yyvsp[-1].expr), Location::Concat((yylsp[-3]), (yylsp[0])));
}
#line 2372 "syntax.cc"
    break;

  case 93:
#line 404 "syntax.y"
    {
    (yyval.lval) = ctx->factory->NewDotField(NEXT_TRACE_ID, (yyvsp[-2].lval), (yyvsp[0].name), Location::Concat((yylsp[-2]), (yylsp[0])));
}
#line 2380 "syntax.cc"
    break;

  case 94:
#line 408 "syntax.y"
    {
    (yyval.exprs) = (yyvsp[-1].exprs);
}
#line 2388 "syntax.cc"
    break;

  case 95:
#line 411 "syntax.y"
    {
    auto arg0 = ctx->factory->NewStringLiteral((yyvsp[0].str_val), (yylsp[0]));
    (yyval.exprs) = ctx->factory->NewList<mai::nyaa::ast::Expression *>(arg0);
}
#line 2397 "syntax.cc"
    break;

  case 96:
#line 415 "syntax.y"
    {
    (yyval.exprs) = nullptr;
}
#line 2405 "syntax.cc"
    break;

  case 97:
#line 419 "syntax.y"
    {
    (yyval.names) = (yyvsp[-1].names);
}
#line 2413 "syntax.cc"
    break;

  case 98:
#line 422 "syntax.y"
    {
    (yyval.names) = nullptr;
}
#line 2421 "syntax.cc"
    break;

  case 99:
#line 426 "syntax.y"
    {
    (yyval.names) = ctx->factory->NewList((yyvsp[0].name));
}
#line 2429 "syntax.cc"
    break;

  case 100:
#line 429 "syntax.y"
    {
    (yyval.names)->push_back((yyvsp[0].name));
}
#line 2437 "syntax.cc"
    break;


#line 2441 "syntax.cc"

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
#line 434 "syntax.y"

void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg) {
    ctx->err_line   = yyl->first_line;
    ctx->err_column = yyl->first_column;
    ctx->err_msg    = ::mai::nyaa::ast::String::New(ctx->arena, msg);
}

