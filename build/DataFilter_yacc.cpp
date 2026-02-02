/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
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
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         DataFilterparse
#define yylex           DataFilterlex
#define yyerror         DataFiltererror
#define yydebug         DataFilterdebug
#define yynerrs         DataFilternerrs
#define yylval          DataFilterlval
#define yychar          DataFilterchar
#define yylloc          DataFilterlloc

/* First part of user prologue.  */
#line 1 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"

/*
 * Copyright (c) 2012-2016 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// This grammar should work with yacc and bison, but has
// only been tested with bison. In addition, since qmake
// uses the -p flag to rename all the yy functions to
// enable multiple grammars in a single executable you
// should make sure you use the very latest bison since it
// has been known to be problematic in the past. It is
// know to work well with bison v2.7.
//
// To make the grammar readable I have placed the code
// for each nterm at column 40, this source file is best
// edited / viewed in an editor which is at least 120
// columns wide (e.g. vi in xterm of 120x40)
//
//

#include "DataFilter.h"

extern int DataFilterlex(); // the lexer aka yylex()
extern char *DataFiltertext; // set by the lexer aka yytext

extern void DataFilter_setString(QString);
extern void DataFilter_clearString();

// PARSER STATE VARIABLES
extern QStringList DataFiltererrors;
static void DataFiltererror(const char *error) { DataFiltererrors << QString(error);}
extern int DataFilterparse();

extern Leaf *DataFilterroot; // root node for parsed statement


#line 130 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
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

#include "DataFilter_yacc.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_SYMBOL = 3,                     /* SYMBOL  */
  YYSYMBOL_PYTHON = 4,                     /* PYTHON  */
  YYSYMBOL_DF_STRING = 5,                  /* DF_STRING  */
  YYSYMBOL_DF_INTEGER = 6,                 /* DF_INTEGER  */
  YYSYMBOL_DF_FLOAT = 7,                   /* DF_FLOAT  */
  YYSYMBOL_BEST = 8,                       /* BEST  */
  YYSYMBOL_TIZ = 9,                        /* TIZ  */
  YYSYMBOL_CONFIG = 10,                    /* CONFIG  */
  YYSYMBOL_CONST_ = 11,                    /* CONST_  */
  YYSYMBOL_IF_ = 12,                       /* IF_  */
  YYSYMBOL_ELSE_ = 13,                     /* ELSE_  */
  YYSYMBOL_WHILE = 14,                     /* WHILE  */
  YYSYMBOL_EQ = 15,                        /* EQ  */
  YYSYMBOL_NEQ = 16,                       /* NEQ  */
  YYSYMBOL_LT = 17,                        /* LT  */
  YYSYMBOL_LTE = 18,                       /* LTE  */
  YYSYMBOL_GT = 19,                        /* GT  */
  YYSYMBOL_GTE = 20,                       /* GTE  */
  YYSYMBOL_ELVIS = 21,                     /* ELVIS  */
  YYSYMBOL_ASSIGN = 22,                    /* ASSIGN  */
  YYSYMBOL_ADD = 23,                       /* ADD  */
  YYSYMBOL_SUBTRACT = 24,                  /* SUBTRACT  */
  YYSYMBOL_DIVIDE = 25,                    /* DIVIDE  */
  YYSYMBOL_MULTIPLY = 26,                  /* MULTIPLY  */
  YYSYMBOL_POW = 27,                       /* POW  */
  YYSYMBOL_MATCHES = 28,                   /* MATCHES  */
  YYSYMBOL_ENDSWITH = 29,                  /* ENDSWITH  */
  YYSYMBOL_BEGINSWITH = 30,                /* BEGINSWITH  */
  YYSYMBOL_CONTAINS = 31,                  /* CONTAINS  */
  YYSYMBOL_AND = 32,                       /* AND  */
  YYSYMBOL_OR = 33,                        /* OR  */
  YYSYMBOL_34_ = 34,                       /* '?'  */
  YYSYMBOL_35_ = 35,                       /* ':'  */
  YYSYMBOL_36_ = 36,                       /* '['  */
  YYSYMBOL_37_ = 37,                       /* ']'  */
  YYSYMBOL_38_ = 38,                       /* '{'  */
  YYSYMBOL_39_ = 39,                       /* '}'  */
  YYSYMBOL_40_ = 40,                       /* ';'  */
  YYSYMBOL_41_ = 41,                       /* '('  */
  YYSYMBOL_42_ = 42,                       /* ')'  */
  YYSYMBOL_43_ = 43,                       /* ','  */
  YYSYMBOL_44_ = 44,                       /* '!'  */
  YYSYMBOL_45_ = 45,                       /* '$'  */
  YYSYMBOL_YYACCEPT = 46,                  /* $accept  */
  YYSYMBOL_filter = 47,                    /* filter  */
  YYSYMBOL_expression = 48,                /* expression  */
  YYSYMBOL_block = 49,                     /* block  */
  YYSYMBOL_statements = 50,                /* statements  */
  YYSYMBOL_statement = 51,                 /* statement  */
  YYSYMBOL_simple_statement = 52,          /* simple_statement  */
  YYSYMBOL_python_script = 53,             /* python_script  */
  YYSYMBOL_if_clause = 54,                 /* if_clause  */
  YYSYMBOL_while_clause = 55,              /* while_clause  */
  YYSYMBOL_function_def = 56,              /* function_def  */
  YYSYMBOL_parameter = 57,                 /* parameter  */
  YYSYMBOL_parms = 58,                     /* parms  */
  YYSYMBOL_lexpr = 59,                     /* lexpr  */
  YYSYMBOL_array = 60,                     /* array  */
  YYSYMBOL_select = 61,                    /* select  */
  YYSYMBOL_cexpr = 62,                     /* cexpr  */
  YYSYMBOL_expr = 63,                      /* expr  */
  YYSYMBOL_symbol = 64,                    /* symbol  */
  YYSYMBOL_literal = 65                    /* literal  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

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


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
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

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

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
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
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
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  51
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   387

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  46
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  20
/* YYNRULES -- Number of rules.  */
#define YYNRULES  69
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  139

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   288


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    44,     2,     2,    45,     2,     2,     2,
      41,    42,     2,     2,    43,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    35,    40,
       2,     2,     2,    34,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    36,     2,    37,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    38,     2,    39,     2,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    31,    32,    33
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   111,   111,   112,   113,   114,   123,   124,   132,   143,
     144,   152,   153,   154,   155,   163,   164,   171,   182,   196,
     203,   219,   235,   242,   243,   251,   256,   267,   268,   269,
     274,   281,   287,   293,   302,   311,   325,   331,   337,   344,
     350,   357,   363,   369,   375,   381,   387,   400,   406,   412,
     418,   424,   430,   436,   441,   446,   451,   457,   463,   472,
     477,   478,   479,   480,   481,   488,   493,   506,   511,   515
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "SYMBOL", "PYTHON",
  "DF_STRING", "DF_INTEGER", "DF_FLOAT", "BEST", "TIZ", "CONFIG", "CONST_",
  "IF_", "ELSE_", "WHILE", "EQ", "NEQ", "LT", "LTE", "GT", "GTE", "ELVIS",
  "ASSIGN", "ADD", "SUBTRACT", "DIVIDE", "MULTIPLY", "POW", "MATCHES",
  "ENDSWITH", "BEGINSWITH", "CONTAINS", "AND", "OR", "'?'", "':'", "'['",
  "']'", "'{'", "'}'", "';'", "'('", "')'", "','", "'!'", "'$'", "$accept",
  "filter", "expression", "block", "statements", "statement",
  "simple_statement", "python_script", "if_clause", "while_clause",
  "function_def", "parameter", "parms", "lexpr", "array", "select",
  "cexpr", "expr", "symbol", "literal", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-111)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     137,  -111,  -111,  -111,  -111,  -111,   -27,   -21,   -17,    20,
      21,    29,   269,   181,   247,   247,    38,    59,  -111,  -111,
    -111,  -111,  -111,   -16,    49,  -111,  -111,   351,   -18,  -111,
       4,     4,     4,     4,   247,   247,   269,  -111,   -15,    32,
     159,  -111,    34,  -111,  -111,  -111,   -19,    18,   300,   -23,
    -111,  -111,   247,   247,   247,   247,   269,   269,   269,   269,
     269,   269,   269,   269,   269,   269,   269,   269,   269,   269,
     269,   269,   247,   247,   203,    33,    52,    68,    70,    35,
      69,    30,  -111,  -111,  -111,  -111,  -111,  -111,   -23,   -23,
     120,   -16,    73,    73,    73,    73,    73,    73,    73,    12,
      12,   -15,   -15,   -15,    73,    73,    73,    73,    96,   328,
     -16,  -111,  -111,  -111,   -14,   -16,   247,   247,  -111,  -111,
     137,   137,   247,  -111,  -111,  -111,   225,    72,    92,   102,
    -111,  -111,  -111,   -16,  -111,  -111,  -111,   137,  -111
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    66,    18,    67,    69,    68,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     5,     2,
      64,     3,     4,    15,    60,    61,    28,    27,    63,    62,
       0,     0,     0,     0,     0,     0,     0,    60,    52,    63,
       0,     9,     0,    12,    13,    14,    63,     0,    27,    31,
      65,     1,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     8,    10,    11,    22,    29,    59,    33,    32,
       0,    17,    36,    37,    38,    39,    40,    41,    42,    48,
      47,    49,    50,    51,    43,    44,    45,    46,     0,    27,
      16,    58,    24,    25,     0,    23,     0,     0,    55,    56,
       0,     0,     0,    35,    34,    57,     0,     0,     0,    19,
       7,     6,    21,    30,    26,    53,    54,     0,    20
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -111,  -111,  -110,     1,  -111,    -5,   116,  -111,   118,   119,
    -111,     5,  -111,    -9,     2,  -111,  -111,    22,     0,  -111
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,    17,   129,   130,    40,   131,    42,    20,    43,    44,
      45,   113,   114,    23,    37,    25,    26,    27,    39,    29
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      28,    18,    24,    73,    73,    47,    49,     1,    41,    52,
      53,   132,    67,    46,    30,    24,    52,    53,    54,    13,
      31,    72,    74,    74,    32,    79,    80,   138,   125,   126,
      75,    76,    77,    78,    38,    83,    48,    65,    66,    67,
      46,    50,    24,    88,    89,    90,    91,    85,    72,    16,
      52,    53,    54,    63,    64,    65,    66,    67,    81,    51,
      86,    33,    34,   108,   110,   115,    72,    52,    53,    54,
      35,    55,    87,    74,    84,   112,   116,   120,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   109,   117,    63,    64,    65,    66,
      67,    52,    53,    54,    52,    53,    54,   127,   128,    72,
     118,   121,   119,   133,   135,   137,    19,   115,    21,    22,
      46,    46,    24,    24,    52,    53,    54,   112,    52,    53,
      54,   134,     0,   123,   136,     0,     0,    46,     0,    24,
       1,     2,     3,     4,     5,     6,     7,     8,     9,    10,
       0,    11,    52,    53,    54,   122,     0,     0,     0,     0,
       0,    12,     1,     2,     3,     4,     5,     6,     7,     8,
       9,    10,     0,    11,     0,    13,     0,     0,    14,     0,
       0,    15,    16,    12,     1,     2,     3,     4,     5,     6,
       7,     8,     9,    10,     0,    11,     0,     0,    82,     0,
      14,     0,     0,    15,    16,    12,     1,     2,     3,     4,
       5,     6,     7,     8,     9,     0,     0,     0,     0,     0,
       0,     0,    14,     0,     0,    15,    16,    12,     1,     2,
       3,     4,     5,     6,     7,     8,     9,     0,     0,     0,
       0,    13,     0,     0,    14,   111,     0,    15,    16,    12,
       1,     2,     3,     4,     5,     6,     7,     8,     9,     0,
       0,     0,     0,    13,     0,     0,    14,     0,     0,    15,
      16,    12,     1,     2,     3,     4,     5,     6,     7,     8,
       9,     0,     0,     0,     0,     0,     0,     0,    14,     0,
       0,    15,    16,    12,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      36,     0,     0,     0,    16,    56,    57,    58,    59,    60,
      61,    62,     0,    63,    64,    65,    66,    67,    68,    69,
      70,    71,     0,     0,     0,     0,    72,     0,     0,     0,
       0,     0,    87,    56,    57,    58,    59,    60,    61,    62,
       0,    63,    64,    65,    66,    67,    68,    69,    70,    71,
       0,     0,     0,     0,    72,   124,    56,    57,    58,    59,
      60,    61,    62,     0,    63,    64,    65,    66,    67,    68,
      69,    70,    71,     0,     0,     0,     0,    72
};

static const yytype_int16 yycheck[] =
{
       0,     0,     0,    22,    22,    14,    15,     3,    13,    32,
      33,   121,    27,    13,    41,    13,    32,    33,    34,    38,
      41,    36,    41,    41,    41,    34,    35,   137,    42,    43,
      30,    31,    32,    33,    12,    40,    14,    25,    26,    27,
      40,     3,    40,    52,    53,    54,    55,    46,    36,    45,
      32,    33,    34,    23,    24,    25,    26,    27,    36,     0,
      42,    41,    41,    72,    73,    74,    36,    32,    33,    34,
      41,    22,    42,    41,    40,    74,    43,    42,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    43,    23,    24,    25,    26,
      27,    32,    33,    34,    32,    33,    34,   116,   117,    36,
      42,    42,    42,   122,    42,    13,     0,   126,     0,     0,
     120,   121,   120,   121,    32,    33,    34,   126,    32,    33,
      34,   126,    -1,    37,    42,    -1,    -1,   137,    -1,   137,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    14,    32,    33,    34,    35,    -1,    -1,    -1,    -1,
      -1,    24,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    -1,    14,    -1,    38,    -1,    -1,    41,    -1,
      -1,    44,    45,    24,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    -1,    14,    -1,    -1,    39,    -1,
      41,    -1,    -1,    44,    45,    24,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    -1,    -1,    44,    45,    24,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    -1,    -1,    -1,
      -1,    38,    -1,    -1,    41,    42,    -1,    44,    45,    24,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    -1,
      -1,    -1,    -1,    38,    -1,    -1,    41,    -1,    -1,    44,
      45,    24,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,
      -1,    44,    45,    24,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      41,    -1,    -1,    -1,    45,    15,    16,    17,    18,    19,
      20,    21,    -1,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,
      -1,    -1,    42,    15,    16,    17,    18,    19,    20,    21,
      -1,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    -1,    -1,    -1,    36,    37,    15,    16,    17,    18,
      19,    20,    21,    -1,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    -1,    -1,    36
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    14,    24,    38,    41,    44,    45,    47,    49,    52,
      53,    54,    55,    59,    60,    61,    62,    63,    64,    65,
      41,    41,    41,    41,    41,    41,    41,    60,    63,    64,
      50,    51,    52,    54,    55,    56,    64,    59,    63,    59,
       3,     0,    32,    33,    34,    22,    15,    16,    17,    18,
      19,    20,    21,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    36,    22,    41,    64,    64,    64,    64,    59,
      59,    63,    39,    51,    40,    49,    42,    42,    59,    59,
      59,    59,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    59,    63,
      59,    42,    49,    57,    58,    59,    43,    43,    42,    42,
      42,    42,    35,    37,    37,    42,    43,    59,    59,    48,
      49,    51,    48,    59,    57,    42,    42,    13,    48
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    46,    47,    47,    47,    47,    48,    48,    49,    50,
      50,    51,    51,    51,    51,    52,    52,    52,    53,    54,
      54,    55,    56,    57,    57,    58,    58,    59,    59,    59,
      59,    59,    59,    59,    60,    61,    62,    62,    62,    62,
      62,    62,    62,    62,    62,    62,    62,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    64,    64,    65,    65,    65
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     3,     1,
       2,     2,     1,     1,     1,     1,     3,     3,     1,     5,
       7,     5,     2,     1,     1,     1,     3,     1,     1,     3,
       5,     2,     3,     3,     4,     4,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     6,     6,     4,     4,     4,     3,     3,
       1,     1,     1,     1,     1,     2,     1,     1,     1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


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
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

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


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

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

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]));
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_expression: /* expression  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1145 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_block: /* block  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1151 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_statements: /* statements  */
#line 75 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { foreach (Leaf* l, *((*yyvaluep).comp)) { l->clear(l); delete l; } delete ((*yyvaluep).comp); }
#line 1157 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_statement: /* statement  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1163 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_simple_statement: /* simple_statement  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1169 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_python_script: /* python_script  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1175 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_if_clause: /* if_clause  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1181 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_while_clause: /* while_clause  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1187 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_function_def: /* function_def  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1193 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_parameter: /* parameter  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1199 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_parms: /* parms  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1205 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_lexpr: /* lexpr  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1211 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_array: /* array  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1217 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_select: /* select  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1223 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_cexpr: /* cexpr  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1229 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_expr: /* expr  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1235 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_symbol: /* symbol  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1241 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

    case YYSYMBOL_literal: /* literal  */
#line 74 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
            { ((*yyvaluep).leaf)->clear(((*yyvaluep).leaf)); delete ((*yyvaluep).leaf); }
#line 1247 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
        break;

      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

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
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

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

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
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
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
  case 2: /* filter: simple_statement  */
#line 111 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { DataFilterroot = (yyvsp[0].leaf); }
#line 1542 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 3: /* filter: if_clause  */
#line 112 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { DataFilterroot = (yyvsp[0].leaf); }
#line 1548 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 4: /* filter: while_clause  */
#line 113 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { DataFilterroot = (yyvsp[0].leaf); }
#line 1554 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 5: /* filter: block  */
#line 114 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { DataFilterroot = (yyvsp[0].leaf); }
#line 1560 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 6: /* expression: statement  */
#line 123 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = (yyvsp[0].leaf); }
#line 1566 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 7: /* expression: block  */
#line 124 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = (yyvsp[0].leaf); }
#line 1572 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 8: /* block: '{' statements '}'  */
#line 132 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Compound;
                                                  (yyval.leaf)->lvalue.b = (yyvsp[-1].comp);
                                                }
#line 1581 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 9: /* statements: statement  */
#line 143 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.comp) = new QList<Leaf*>(); (yyval.comp)->append((yyvsp[0].leaf)); }
#line 1587 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 10: /* statements: statements statement  */
#line 144 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyvsp[-1].comp)->append((yyvsp[0].leaf)); (yyval.comp) = (yyvsp[-1].comp); }
#line 1593 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 15: /* simple_statement: lexpr  */
#line 163 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = (yyvsp[0].leaf); }
#line 1599 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 16: /* simple_statement: symbol ASSIGN lexpr  */
#line 164 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                {
                                                  (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Operation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1611 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 17: /* simple_statement: array ASSIGN lexpr  */
#line 171 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                {
                                                  (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Operation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1623 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 18: /* python_script: PYTHON  */
#line 182 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[0]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Script;
                                                  (yyval.leaf)->function = "python";
                                                  QString full(DataFiltertext);
                                                  (yyval.leaf)->lvalue.s = new QString(full.mid(8,full.length()-10));
                                                }
#line 1634 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 19: /* if_clause: IF_ '(' lexpr ')' expression  */
#line 196 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-4]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Conditional;
                                                  (yyval.leaf)->op = IF_;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[0].leaf);
                                                  (yyval.leaf)->rvalue.l = NULL;
                                                  (yyval.leaf)->cond.l = (yyvsp[-2].leaf);
                                                }
#line 1646 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 20: /* if_clause: IF_ '(' lexpr ')' expression ELSE_ expression  */
#line 204 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-6]).first_column, (yylsp[-2]).last_column);
                                                  (yyval.leaf)->type = Leaf::Conditional;
                                                  (yyval.leaf)->op = IF_; 
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                  (yyval.leaf)->cond.l = (yyvsp[-4].leaf);
                                                }
#line 1658 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 21: /* while_clause: WHILE '(' lexpr ')' expression  */
#line 219 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-4]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Conditional;
                                                  (yyval.leaf)->op = WHILE; 
                                                  (yyval.leaf)->lvalue.l = (yyvsp[0].leaf);
                                                  (yyval.leaf)->rvalue.l = NULL;
                                                  (yyval.leaf)->cond.l = (yyvsp[-2].leaf);
                                                }
#line 1670 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 22: /* function_def: symbol block  */
#line 235 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = (yyvsp[0].leaf); 
                                                  (yyval.leaf)->function = *((yyvsp[-1].leaf)->lvalue.n);
                                                }
#line 1678 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 23: /* parameter: lexpr  */
#line 242 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = (yyvsp[0].leaf); }
#line 1684 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 24: /* parameter: block  */
#line 243 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = (yyvsp[0].leaf); }
#line 1690 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 25: /* parms: parameter  */
#line 251 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[0]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Function;
                                                  (yyval.leaf)->series = NULL; // not tiz/best
                                                  (yyval.leaf)->fparms << (yyvsp[0].leaf);
                                                }
#line 1700 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 26: /* parms: parms ',' parameter  */
#line 256 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyvsp[-2].leaf)->fparms << (yyvsp[0].leaf);
                                                  (yyvsp[-2].leaf)->leng = (yylsp[0]).last_column;
                                                  (yyval.leaf) = (yyvsp[-2].leaf);
                                                }
#line 1709 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 27: /* lexpr: expr  */
#line 267 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = (yyvsp[0].leaf); }
#line 1715 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 28: /* lexpr: cexpr  */
#line 268 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = (yyvsp[0].leaf); }
#line 1721 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 29: /* lexpr: '(' lexpr ')'  */
#line 269 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-1]).first_column, (yylsp[-1]).last_column);
                                                  (yyval.leaf)->type = Leaf::Logical;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-1].leaf);
                                                  (yyval.leaf)->op = 0;
                                                }
#line 1731 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 30: /* lexpr: lexpr '?' lexpr ':' lexpr  */
#line 274 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-4]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Conditional;
                                                  (yyval.leaf)->op = 0; // zero for tertiary
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                  (yyval.leaf)->cond.l = (yyvsp[-4].leaf);
                                                }
#line 1743 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 31: /* lexpr: '!' lexpr  */
#line 281 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-1]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::UnaryOperation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[0].leaf);
                                                  (yyval.leaf)->op = '!';
                                                  (yyval.leaf)->rvalue.l = NULL;
                                                }
#line 1754 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 32: /* lexpr: lexpr OR lexpr  */
#line 287 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Logical;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1765 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 33: /* lexpr: lexpr AND lexpr  */
#line 293 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Logical;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1776 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 34: /* array: expr '[' expr ']'  */
#line 302 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                              {
                                                  (yyval.leaf) = new Leaf((yylsp[-3]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Index;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-3].leaf);
                                                  (yyval.leaf)->fparms << (yyvsp[-1].leaf);
                                                  (yyval.leaf)->op = 0;
                                                }
#line 1788 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 35: /* select: expr '[' lexpr ']'  */
#line 311 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                             {
                                                  (yyval.leaf) = new Leaf((yylsp[-3]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Select;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-3].leaf);
                                                  (yyval.leaf)->fparms << (yyvsp[-1].leaf);
                                                  (yyval.leaf)->op = 0;
                                             }
#line 1800 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 36: /* cexpr: expr EQ expr  */
#line 325 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Operation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1811 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 37: /* cexpr: expr NEQ expr  */
#line 331 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Operation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1822 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 38: /* cexpr: expr LT expr  */
#line 337 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Operation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1833 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 39: /* cexpr: expr LTE expr  */
#line 344 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Operation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1844 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 40: /* cexpr: expr GT expr  */
#line 350 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Operation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1855 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 41: /* cexpr: expr GTE expr  */
#line 357 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Operation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1866 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 42: /* cexpr: expr ELVIS expr  */
#line 363 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Operation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1877 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 43: /* cexpr: expr MATCHES expr  */
#line 369 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Operation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1888 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 44: /* cexpr: expr ENDSWITH expr  */
#line 375 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Operation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1899 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 45: /* cexpr: expr BEGINSWITH expr  */
#line 381 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Operation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1910 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 46: /* cexpr: expr CONTAINS expr  */
#line 387 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Operation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1921 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 47: /* expr: expr SUBTRACT expr  */
#line 400 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::BinaryOperation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1932 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 48: /* expr: expr ADD expr  */
#line 406 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::BinaryOperation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1943 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 49: /* expr: expr DIVIDE expr  */
#line 412 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::BinaryOperation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1954 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 50: /* expr: expr MULTIPLY expr  */
#line 418 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::BinaryOperation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1965 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 51: /* expr: expr POW expr  */
#line 424 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-2]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::BinaryOperation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-2].leaf);
                                                  (yyval.leaf)->op = (yyvsp[-1].op);
                                                  (yyval.leaf)->rvalue.l = (yyvsp[0].leaf);
                                                }
#line 1976 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 52: /* expr: SUBTRACT expr  */
#line 430 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-1]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::UnaryOperation;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[0].leaf);
                                                  (yyval.leaf)->op = '-';
                                                  (yyval.leaf)->rvalue.l = NULL;
                                                }
#line 1987 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 53: /* expr: BEST '(' symbol ',' lexpr ')'  */
#line 436 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-5]).first_column, (yylsp[0]).last_column); (yyval.leaf)->type = Leaf::Function;
                                                  (yyval.leaf)->function = QString((yyvsp[-5].function));
                                                  (yyval.leaf)->series = (yyvsp[-3].leaf);
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-1].leaf);
                                                }
#line 1997 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 54: /* expr: TIZ '(' symbol ',' lexpr ')'  */
#line 441 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-5]).first_column, (yylsp[0]).last_column); (yyval.leaf)->type = Leaf::Function;
                                                  (yyval.leaf)->function = QString((yyvsp[-5].function));
                                                  (yyval.leaf)->series = (yyvsp[-3].leaf);
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-1].leaf);
                                                }
#line 2007 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 55: /* expr: CONFIG '(' symbol ')'  */
#line 446 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-3]).first_column, (yylsp[0]).last_column); (yyval.leaf)->type = Leaf::Function;
                                                  (yyval.leaf)->function = QString((yyvsp[-3].function));
                                                  (yyval.leaf)->series = (yyvsp[-1].leaf);
                                                  (yyval.leaf)->lvalue.l = NULL;
                                                }
#line 2017 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 56: /* expr: CONST_ '(' symbol ')'  */
#line 451 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-3]).first_column, (yylsp[0]).last_column); (yyval.leaf)->type = Leaf::Function;
                                                  (yyval.leaf)->function = QString((yyvsp[-3].function));
                                                  (yyval.leaf)->series = (yyvsp[-1].leaf);
                                                  (yyval.leaf)->lvalue.l = NULL;
                                                }
#line 2027 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 57: /* expr: symbol '(' parms ')'  */
#line 457 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { /* need to convert params to a function */
                                                  (yyvsp[-1].leaf)->loc = (yylsp[-3]).first_column;
                                                  (yyvsp[-1].leaf)->leng = (yylsp[0]).last_column;
                                                  (yyvsp[-1].leaf)->function = *((yyvsp[-3].leaf)->lvalue.n);
                                                  (yyval.leaf) = (yyvsp[-1].leaf);
                                                }
#line 2038 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 58: /* expr: symbol '(' ')'  */
#line 463 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { /* need to convert symbol to function */
                                                  (yyvsp[-2].leaf)->type = Leaf::Function;
                                                  (yyvsp[-2].leaf)->series = NULL; // not tiz/best
                                                  (yyvsp[-2].leaf)->function = *((yyvsp[-2].leaf)->lvalue.n);
                                                  delete (yyvsp[-2].leaf)->lvalue.n; // not used anymore
                                                  (yyvsp[-2].leaf)->lvalue.l = NULL; // avoid double deletion
                                                  (yyvsp[-2].leaf)->fparms.clear(); // no parameters!
                                                  (yyval.leaf) = (yyvsp[-2].leaf);
                                                }
#line 2052 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 59: /* expr: '(' expr ')'  */
#line 472 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[-1]).first_column, (yylsp[-1]).last_column);
                                                  (yyval.leaf)->type = Leaf::Logical;
                                                  (yyval.leaf)->lvalue.l = (yyvsp[-1].leaf);
                                                  (yyval.leaf)->op = 0;
                                                }
#line 2062 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 60: /* expr: array  */
#line 477 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = (yyvsp[0].leaf); }
#line 2068 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 61: /* expr: select  */
#line 478 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = (yyvsp[0].leaf); }
#line 2074 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 62: /* expr: literal  */
#line 479 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = (yyvsp[0].leaf); }
#line 2080 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 63: /* expr: symbol  */
#line 480 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = (yyvsp[0].leaf); }
#line 2086 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 64: /* expr: python_script  */
#line 481 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = (yyvsp[0].leaf); }
#line 2092 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 65: /* symbol: '$' SYMBOL  */
#line 488 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                  { (yyval.leaf) = new Leaf((yylsp[-1]).first_column, (yylsp[0]).last_column);
                                                    (yyval.leaf)->type = Leaf::Symbol;
                                                    (yyval.leaf)->op = 1; // prompted variable (from user)
                                                    (yyval.leaf)->lvalue.n = new QString(DataFiltertext);
                                                  }
#line 2102 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 66: /* symbol: SYMBOL  */
#line 493 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                  { (yyval.leaf) = new Leaf((yylsp[0]).first_column, (yylsp[0]).last_column);
                                                    (yyval.leaf)->type = Leaf::Symbol;
                                                    (yyval.leaf)->op = 0; // metric or builtin reference
                                                    if (QString(DataFiltertext) == "BikeScore") (yyval.leaf)->lvalue.n = new QString("BikeScore&#8482;");
                                                    else (yyval.leaf)->lvalue.n = new QString(DataFiltertext);
                                                  }
#line 2113 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 67: /* literal: DF_STRING  */
#line 506 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[0]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::String;
                                                  QString s2 = Utils::unescape(DataFiltertext); // user can escape chars in string
                                                  (yyval.leaf)->lvalue.s = new QString(s2.mid(1,s2.length()-2));
                                                }
#line 2123 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 68: /* literal: DF_FLOAT  */
#line 511 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[0]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Float;
                                                  (yyval.leaf)->lvalue.f = QString(DataFiltertext).toFloat();
                                                }
#line 2132 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;

  case 69: /* literal: DF_INTEGER  */
#line 515 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"
                                                { (yyval.leaf) = new Leaf((yylsp[0]).first_column, (yylsp[0]).last_column);
                                                  (yyval.leaf)->type = Leaf::Integer;
                                                  (yyval.leaf)->lvalue.i = QString(DataFiltertext).toInt();
                                                }
#line 2141 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"
    break;


#line 2145 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/DataFilter_yacc.cpp"

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
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

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
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
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
                      yytoken, &yylval, &yylloc);
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
  ++yynerrs;

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

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 521 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/DataFilter.y"


