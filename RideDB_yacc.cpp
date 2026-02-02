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
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         RideDBparse
#define yylex           RideDBlex
#define yyerror         RideDBerror
#define yydebug         RideDBdebug
#define yynerrs         RideDBnerrs

/* First part of user prologue.  */
#line 1 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"

/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#include "RideDB.h"
#include "RideFileCache.h"
#include "SpecialFields.h"
#include "Settings.h"
#ifdef GC_WANT_HTTP
#include "APIWebService.h"
#endif

#define YYSTYPE QString

// Lex scanner
extern int RideDBlex(YYSTYPE*,void*); // the lexer aka yylex()
extern int RideDBlex_init(void**);
extern void RideDB_setString(QString, void *);
extern int RideDBlex_destroy(void*); // the cleaner for lexer

// yacc parser
void RideDBerror(void*jc, const char *error) // used by parser aka yyerror()
{ static_cast<RideDBContext*>(jc)->errors << error; }

// extract scanner from the context
#define scanner jc->scanner


#line 120 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"

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

#include "RideDB_yacc.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_STRING = 3,                     /* STRING  */
  YYSYMBOL_VERSION = 4,                    /* VERSION  */
  YYSYMBOL_RIDES = 5,                      /* RIDES  */
  YYSYMBOL_METRICS = 6,                    /* METRICS  */
  YYSYMBOL_TAGS = 7,                       /* TAGS  */
  YYSYMBOL_XDATA = 8,                      /* XDATA  */
  YYSYMBOL_INTERVALS = 9,                  /* INTERVALS  */
  YYSYMBOL_10_ = 10,                       /* '{'  */
  YYSYMBOL_11_ = 11,                       /* '}'  */
  YYSYMBOL_12_ = 12,                       /* ','  */
  YYSYMBOL_13_ = 13,                       /* ':'  */
  YYSYMBOL_14_ = 14,                       /* '['  */
  YYSYMBOL_15_ = 15,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 16,                  /* $accept  */
  YYSYMBOL_document = 17,                  /* document  */
  YYSYMBOL_elements = 18,                  /* elements  */
  YYSYMBOL_element = 19,                   /* element  */
  YYSYMBOL_version = 20,                   /* version  */
  YYSYMBOL_ride_list = 21,                 /* ride_list  */
  YYSYMBOL_ride = 22,                      /* ride  */
  YYSYMBOL_rideelement_list = 23,          /* rideelement_list  */
  YYSYMBOL_rideelement = 24,               /* rideelement  */
  YYSYMBOL_ride_tuple = 25,                /* ride_tuple  */
  YYSYMBOL_metrics = 26,                   /* metrics  */
  YYSYMBOL_metrics_list = 27,              /* metrics_list  */
  YYSYMBOL_metric = 28,                    /* metric  */
  YYSYMBOL_metric_key = 29,                /* metric_key  */
  YYSYMBOL_metric_value = 30,              /* metric_value  */
  YYSYMBOL_metric_count = 31,              /* metric_count  */
  YYSYMBOL_metric_stdmean = 32,            /* metric_stdmean  */
  YYSYMBOL_metric_stdvariance = 33,        /* metric_stdvariance  */
  YYSYMBOL_intervals = 34,                 /* intervals  */
  YYSYMBOL_intervals_list = 35,            /* intervals_list  */
  YYSYMBOL_interval = 36,                  /* interval  */
  YYSYMBOL_intervalelement_list = 37,      /* intervalelement_list  */
  YYSYMBOL_interval_element = 38,          /* interval_element  */
  YYSYMBOL_interval_tuple = 39,            /* interval_tuple  */
  YYSYMBOL_interval_metrics = 40,          /* interval_metrics  */
  YYSYMBOL_interval_metrics_list = 41,     /* interval_metrics_list  */
  YYSYMBOL_interval_metric = 42,           /* interval_metric  */
  YYSYMBOL_interval_metric_key = 43,       /* interval_metric_key  */
  YYSYMBOL_interval_metric_value = 44,     /* interval_metric_value  */
  YYSYMBOL_interval_metric_count = 45,     /* interval_metric_count  */
  YYSYMBOL_interval_metric_stdmean = 46,   /* interval_metric_stdmean  */
  YYSYMBOL_interval_metric_stdvariance = 47, /* interval_metric_stdvariance  */
  YYSYMBOL_tags = 48,                      /* tags  */
  YYSYMBOL_tags_list = 49,                 /* tags_list  */
  YYSYMBOL_tag = 50,                       /* tag  */
  YYSYMBOL_tag_key = 51,                   /* tag_key  */
  YYSYMBOL_tag_value = 52,                 /* tag_value  */
  YYSYMBOL_xdata = 53,                     /* xdata  */
  YYSYMBOL_xdata_list = 54,                /* xdata_list  */
  YYSYMBOL_xdata_name = 55,                /* xdata_name  */
  YYSYMBOL_xdata_values = 56,              /* xdata_values  */
  YYSYMBOL_xdata_value = 57,               /* xdata_value  */
  YYSYMBOL_string = 58                     /* string  */
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
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

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
#define YYFINAL  8
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   125

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  16
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  43
/* YYNRULES -- Number of rules.  */
#define YYNRULES  65
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  138

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   264


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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    12,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    13,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    14,     2,    15,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    10,     2,    11,     2,     2,     2,     2,
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
       5,     6,     7,     8,     9
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    56,    56,    58,    59,    62,    63,    65,    72,    73,
      76,   119,   120,   122,   123,   124,   125,   126,   133,   169,
     170,   170,   171,   180,   190,   202,   203,   204,   205,   206,
     212,   213,   214,   217,   226,   227,   230,   231,   235,   248,
     249,   249,   250,   258,   266,   278,   279,   280,   281,   282,
     287,   288,   288,   289,   294,   295,   301,   302,   302,   303,
     306,   307,   308,   309,   311,   317
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
  "\"end of file\"", "error", "\"invalid token\"", "STRING", "VERSION",
  "RIDES", "METRICS", "TAGS", "XDATA", "INTERVALS", "'{'", "'}'", "','",
  "':'", "'['", "']'", "$accept", "document", "elements", "element",
  "version", "ride_list", "ride", "rideelement_list", "rideelement",
  "ride_tuple", "metrics", "metrics_list", "metric", "metric_key",
  "metric_value", "metric_count", "metric_stdmean", "metric_stdvariance",
  "intervals", "intervals_list", "interval", "intervalelement_list",
  "interval_element", "interval_tuple", "interval_metrics",
  "interval_metrics_list", "interval_metric", "interval_metric_key",
  "interval_metric_value", "interval_metric_count",
  "interval_metric_stdmean", "interval_metric_stdvariance", "tags",
  "tags_list", "tag", "tag_key", "tag_value", "xdata", "xdata_list",
  "xdata_name", "xdata_values", "xdata_value", "string", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-46)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
      -5,    19,    47,    21,    35,    15,   -46,   -46,   -46,    46,
      36,   -46,    19,   -46,   -46,    41,   -46,     1,     4,   -46,
      39,    42,    43,    44,    17,   -46,   -46,   -46,   -46,   -46,
     -46,    45,    49,    41,   -46,    55,    57,    58,    40,   -46,
       1,    56,    46,   -46,    46,    46,     3,    59,   -46,    46,
     -46,    20,   -46,    60,   -46,    27,   -46,    61,   -46,   -46,
      30,   -46,     9,     5,   -46,    62,    63,   -46,   -46,    46,
      -1,   -46,    46,    46,   -46,     3,    66,    32,   -46,   -46,
     -46,    67,    59,   -46,   -46,    46,   -46,    46,   -46,   -46,
     -46,   -46,   -46,   -46,    71,   -46,     9,    46,   -46,   -46,
      70,    46,   -46,   -46,    46,    34,   -46,    72,   -46,     6,
     -46,   -46,    46,     0,    46,   -46,   -46,    46,   -46,   -46,
      74,   -46,    77,    46,    46,    68,   -46,    10,   -46,   -46,
      46,   -46,    78,   -46,    46,    69,   -46,   -46
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     3,     5,     1,     0,
       0,     2,     0,    65,     7,     0,     4,     0,     0,     8,
       0,     0,     0,     0,     0,    12,    13,    14,    17,    15,
      16,     0,     0,     0,     6,     0,     0,     0,     0,    10,
       0,     0,     0,     9,     0,     0,     0,     0,    11,    61,
      18,     0,    20,     0,    25,     0,    51,     0,    54,    57,
       0,    60,     0,     0,    32,     0,    62,    64,    19,     0,
       0,    50,     0,     0,    56,     0,     0,     0,    35,    36,
      37,     0,     0,    30,    59,    61,    21,     0,    22,    26,
      52,    53,    55,    58,     0,    33,     0,     0,    31,    63,
       0,     0,    34,    38,     0,     0,    40,     0,    45,     0,
      27,    39,     0,     0,     0,    23,    41,     0,    42,    46,
       0,    28,     0,     0,     0,     0,    29,     0,    47,    24,
       0,    43,     0,    48,     0,     0,    49,    44
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -46,   -46,   -46,    79,   -46,   -46,    26,   -46,    31,   -46,
     -46,   -46,    24,   -46,   -15,   -46,   -46,   -46,   -46,   -46,
      12,   -46,     2,   -46,   -46,   -46,   -16,   -46,   -20,   -46,
     -46,   -46,   -46,   -46,    28,   -46,   -46,   -45,   -46,   -46,
      14,   -46,    -9
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     2,     5,     6,     7,    18,    19,    24,    25,    26,
      27,    51,    52,    53,    88,   109,   120,   125,    28,    63,
      64,    77,    78,    79,    80,   105,   106,   107,   118,   127,
     132,   135,    29,    55,    56,    57,    91,    30,    60,    31,
      65,    66,    32
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      14,    59,    13,    13,    13,     1,    13,    20,    21,    22,
      23,    22,    13,    87,   117,    76,    33,    82,   114,    34,
      83,   115,   130,     3,     4,   131,    11,    12,    39,    40,
      93,    68,    69,    50,     9,    54,    58,    61,    71,    72,
      67,    74,    75,    95,    96,   111,   112,     8,    10,    13,
      15,    17,    35,    81,    47,    36,    37,    38,    41,    43,
      54,    89,    42,    58,    92,    44,    61,    45,    46,    62,
      49,    48,   100,    70,    73,    85,    67,    84,    89,    94,
      97,   101,   104,   129,   137,   113,   123,    81,   103,   124,
     134,    16,   108,    86,    98,   110,   116,   122,   102,    99,
      90,     0,     0,   108,   119,   121,     0,     0,   119,     0,
       0,     0,     0,     0,   126,   128,     0,     0,     0,     0,
       0,   133,     0,     0,     0,   136
};

static const yytype_int16 yycheck[] =
{
       9,    46,     3,     3,     3,    10,     3,     6,     7,     8,
       9,     8,     3,    14,    14,     6,    12,    12,    12,    15,
      15,    15,    12,     4,     5,    15,    11,    12,    11,    12,
      75,    11,    12,    42,    13,    44,    45,    46,    11,    12,
      49,    11,    12,    11,    12,    11,    12,     0,    13,     3,
      14,    10,    13,    62,    14,    13,    13,    13,    13,    33,
      69,    70,    13,    72,    73,    10,    75,    10,    10,    10,
      14,    40,    87,    13,    13,    12,    85,    15,    87,    13,
      13,    10,    12,    15,    15,    13,    12,    96,    97,    12,
      12,    12,   101,    69,    82,   104,   112,   117,    96,    85,
      72,    -1,    -1,   112,   113,   114,    -1,    -1,   117,    -1,
      -1,    -1,    -1,    -1,   123,   124,    -1,    -1,    -1,    -1,
      -1,   130,    -1,    -1,    -1,   134
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    10,    17,     4,     5,    18,    19,    20,     0,    13,
      13,    11,    12,     3,    58,    14,    19,    10,    21,    22,
       6,     7,     8,     9,    23,    24,    25,    26,    34,    48,
      53,    55,    58,    12,    15,    13,    13,    13,    13,    11,
      12,    13,    13,    22,    10,    10,    10,    14,    24,    14,
      58,    27,    28,    29,    58,    49,    50,    51,    58,    53,
      54,    58,    10,    35,    36,    56,    57,    58,    11,    12,
      13,    11,    12,    13,    11,    12,     6,    37,    38,    39,
      40,    58,    12,    15,    15,    12,    28,    14,    30,    58,
      50,    52,    58,    53,    13,    11,    12,    13,    36,    56,
      30,    10,    38,    58,    12,    41,    42,    43,    58,    31,
      58,    11,    12,    13,    12,    15,    42,    14,    44,    58,
      32,    58,    44,    12,    12,    33,    58,    45,    58,    15,
      12,    15,    46,    58,    12,    47,    58,    15
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    16,    17,    18,    18,    19,    19,    20,    21,    21,
      22,    23,    23,    24,    24,    24,    24,    24,    25,    26,
      27,    27,    28,    28,    28,    29,    30,    31,    32,    33,
      34,    35,    35,    36,    37,    37,    38,    38,    39,    40,
      41,    41,    42,    42,    42,    43,    44,    45,    46,    47,
      48,    49,    49,    50,    51,    52,    53,    54,    54,    53,
      55,    56,    56,    56,    57,    58
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     3,     1,     3,     1,     5,     3,     1,     3,
       3,     3,     1,     1,     1,     1,     1,     1,     3,     5,
       1,     3,     3,     7,    11,     1,     1,     1,     1,     1,
       5,     3,     1,     3,     3,     1,     1,     1,     3,     5,
       1,     3,     3,     7,    11,     1,     1,     1,     1,     1,
       5,     1,     3,     3,     1,     1,     5,     1,     3,     5,
       1,     0,     1,     3,     1,     1
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
        yyerror (jc, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


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




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, jc); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, struct RideDBContext *jc)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (jc);
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
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, struct RideDBContext *jc)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, jc);
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule, struct RideDBContext *jc)
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
                       &yyvsp[(yyi + 1) - (yynrhs)], jc);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, jc); \
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
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, struct RideDBContext *jc)
{
  YY_USE (yyvaluep);
  YY_USE (jc);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (struct RideDBContext *jc)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

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

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

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

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
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
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

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
      yychar = yylex (&yylval, scanner);
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


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 7: /* version: VERSION ':' string  */
#line 65 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                {
                                                                    if (yyvsp[0] != RIDEDB_VERSION) {
                                                                        jc->old=true; 
                                                                        jc->item.isstale=true; // force refresh after load
                                                                    }
                                                                }
#line 1265 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 10: /* ride: '{' rideelement_list '}'  */
#line 76 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { 
                                                                    // search for one to update using serial search,
                                                                    // if the performance is too slow we can move to
                                                                    // a binary search, but suspect this ok < 10000 rides
                                                                    if (jc->api != NULL) {
                                                                    #ifdef GC_WANT_HTTP
                                                                        // we're listing rides in the api
                                                                        jc->api->writeRideLine(jc->item, jc->request, jc->response);
                                                                    #endif
                                                                    } else {
                                                                        double progress= round(double(jc->loading++) / double(jc->cache->rides().count()) * 100.0f);
                                                                        if (progress > jc->lastProgressUpdate) {
                                                                            jc->context->notifyLoadProgress(jc->folder,progress);
                                                                            jc->lastProgressUpdate = progress;
                                                                        }

                                                                        // find entry and update it
                                                                        int index=jc->cache->find(&jc->item);
                                                                        if (index==-1)  qDebug()<<"unable to load:"<<jc->item.fileName<<jc->item.dateTime<<jc->item.weight;
                                                                        else  jc->cache->rides().at(index)->setFrom(jc->item);
                                                                    }

                                                                    // now set our ride item clean again, so we don't
                                                                    // overwrite with prior data
                                                                    jc->item.metadata().clear();
                                                                    jc->item.xdata().clear();
                                                                    jc->item.metrics().fill(0.0f);
                                                                    jc->item.counts().fill(0.0f);
                                                                    jc->item.stdmeans().clear();
                                                                    jc->item.stdvariances().clear();
                                                                    jc->interval.metrics().fill(0.0f);
                                                                    jc->interval.counts().fill(0.0f);
                                                                    jc->interval.stdmeans().clear();
                                                                    jc->interval.stdvariances().clear();
                                                                    jc->interval.route = QUuid();
                                                                    jc->item.clearIntervals();
                                                                    jc->item.overrides_.clear();
                                                                    jc->item.fileName = "";
                                                                    jc->count = "";
                                                                    jc->value = "";
                                                                }
#line 1311 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 18: /* ride_tuple: string ':' string  */
#line 133 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { 
                                                                     if (yyvsp[-2] == "filename") jc->item.fileName = yyvsp[0];
                                                                     else if (yyvsp[-2] == "fingerprint") jc->item.fingerprint = yyvsp[0].toULongLong();
                                                                     else if (yyvsp[-2] == "crc") jc->item.crc = yyvsp[0].toULongLong();
                                                                     else if (yyvsp[-2] == "metacrc") jc->item.metacrc = yyvsp[0].toULongLong();
                                                                     else if (yyvsp[-2] == "timestamp") jc->item.timestamp = yyvsp[0].toULongLong();
                                                                     else if (yyvsp[-2] == "dbversion") jc->item.dbversion = yyvsp[0].toInt();
                                                                     else if (yyvsp[-2] == "udbversion") jc->item.udbversion = yyvsp[0].toInt();
                                                                     else if (yyvsp[-2] == "color") jc->item.color = QColor(yyvsp[0]);
                                                                     else if (yyvsp[-2] == "aero") jc->item.isAero = yyvsp[0].toInt() ? true: false;
                                                                     else if (yyvsp[-2] == "sport") {
                                                                         jc->item.sport = (yyvsp[0]);
                                                                         jc->item.isBike=jc->item.isRun=jc->item.isSwim=jc->item.isXtrain=jc->item.isAero=false;
                                                                         if (yyvsp[0] == "Bike") jc->item.isBike = true;
                                                                         else if (yyvsp[0] == "Run") jc->item.isRun = true;
                                                                         else if (yyvsp[0] == "Swim") jc->item.isSwim = true;
                                                                         else if (yyvsp[0] == "Aero") jc->item.isAero = true;
                                                                         else jc->item.isXtrain = true;
                                                                     }
                                                                     else if (yyvsp[-2] == "present") jc->item.present = yyvsp[0];
                                                                     else if (yyvsp[-2] == "overrides") jc->item.overrides_ = yyvsp[0].split(",");
                                                                     else if (yyvsp[-2] == "weight") jc->item.weight = yyvsp[0].toDouble();
                                                                     else if (yyvsp[-2] == "samples") jc->item.samples = yyvsp[0].toInt() > 0 ? true : false;
                                                                     else if (yyvsp[-2] == "zonerange") jc->item.zoneRange = yyvsp[0].toInt();
                                                                     else if (yyvsp[-2] == "hrzonerange") jc->item.hrZoneRange = yyvsp[0].toInt();
                                                                     else if (yyvsp[-2] == "pacezonerange") jc->item.paceZoneRange = yyvsp[0].toInt();
                                                                     else if (yyvsp[-2] == "date") {
                                                                         QDateTime aslocal = QDateTime::fromString(yyvsp[0], DATETIME_FORMAT);
                                                                         QDateTime asUTC = QDateTime(aslocal.date(), aslocal.time(), Qt::UTC);
                                                                         jc->item.dateTime = asUTC.toLocalTime();
                                                                     }
                                                                }
#line 1348 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 22: /* metric: metric_key ':' metric_value  */
#line 172 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                {
                                                                    const RideMetric *m = RideMetricFactory::instance().rideMetric(yyvsp[-2]);
                                                                    if (m) {
                                                                        jc->item.metrics()[m->index()] = yyvsp[0].toDouble();
                                                                        jc->item.counts()[m->index()] = 0; // we don't write zero values
                                                                    }
                                                                    else qDebug()<<"metric not found:"<<yyvsp[-2];
                                                                }
#line 1361 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 23: /* metric: metric_key ':' '[' metric_value ',' metric_count ']'  */
#line 180 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                {
                                                                    const RideMetric *m = RideMetricFactory::instance().rideMetric(yyvsp[-6]);
                                                                    if (m) {
                                                                        jc->item.metrics()[m->index()] = yyvsp[-3].toDouble();
                                                                        jc->item.counts()[m->index()] = yyvsp[-1].toDouble();
                                                                    }
                                                                    else qDebug()<<"metric not found:"<<yyvsp[-6];

                                                                }
#line 1375 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 24: /* metric: metric_key ':' '[' metric_value ',' metric_count ',' metric_stdmean ',' metric_stdvariance ']'  */
#line 190 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                                                          {
                                                                    const RideMetric *m = RideMetricFactory::instance().rideMetric(yyvsp[-10]);
                                                                    if (m) {
                                                                        jc->item.metrics()[m->index()] = yyvsp[-7].toDouble();
                                                                        jc->item.counts()[m->index()] = yyvsp[-5].toDouble();
                                                                        jc->item.stdmeans().insert(m->index(), yyvsp[-3].toDouble());
                                                                        jc->item.stdvariances().insert(m->index(), yyvsp[-1].toDouble());
                                                                    }
                                                                    else qDebug()<<"metric not found:"<<yyvsp[-10];

                                                                }
#line 1391 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 25: /* metric_key: string  */
#line 202 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->key = jc->JsonString; }
#line 1397 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 26: /* metric_value: string  */
#line 203 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->value = jc->JsonString; }
#line 1403 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 27: /* metric_count: string  */
#line 204 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->count = jc->JsonString; }
#line 1409 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 28: /* metric_stdmean: string  */
#line 205 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->count = jc->JsonString; }
#line 1415 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 29: /* metric_stdvariance: string  */
#line 206 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->count = jc->JsonString; }
#line 1421 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 33: /* interval: '{' intervalelement_list '}'  */
#line 217 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                {
                                                                     jc->item.addInterval(jc->interval);
                                                                    jc->interval.metrics().fill(0.0f);
                                                                    jc->interval.counts().fill(0.0f);
                                                                    jc->interval.stdmeans().clear();
                                                                    jc->interval.stdvariances().clear();

                                                                }
#line 1434 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 38: /* interval_tuple: string ':' string  */
#line 235 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { 
                                                                     if (yyvsp[-2] == "name") jc->interval.name = yyvsp[0];
                                                                     else if (yyvsp[-2] == "start") jc->interval.start = yyvsp[0].toDouble();
                                                                     else if (yyvsp[-2] == "stop") jc->interval.stop = yyvsp[0].toDouble();
                                                                     else if (yyvsp[-2] == "startKM") jc->interval.startKM = yyvsp[0].toDouble();
                                                                     else if (yyvsp[-2] == "stopKM") jc->interval.stopKM = yyvsp[0].toDouble();
                                                                     else if (yyvsp[-2] == "type") jc->interval.type = static_cast<RideFileInterval::intervaltype>(yyvsp[0].toInt());
                                                                     else if (yyvsp[-2] == "color") jc->interval.color = QColor(yyvsp[0]);
                                                                     else if (yyvsp[-2] == "seq") jc->interval.displaySequence = yyvsp[0].toInt();
                                                                     else if (yyvsp[-2] == "route") jc->interval.route = QUuid(yyvsp[0]);
                                                                     else if (yyvsp[-2] == "test") jc->interval.test = yyvsp[0] == "true" ? true : false;
                                                                }
#line 1451 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 42: /* interval_metric: interval_metric_key ':' interval_metric_value  */
#line 251 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { 
                                                                    const RideMetric *m = RideMetricFactory::instance().rideMetric(yyvsp[-2]);
                                                                    if (m) {
                                                                        jc->interval.metrics()[m->index()] = yyvsp[0].toDouble();
                                                                        jc->interval.counts()[m->index()] = 0; /* we don't write zeroes */
                                                                    } else qDebug()<<"metric not found:"<<yyvsp[-2];
                                                               }
#line 1463 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 43: /* interval_metric: interval_metric_key ':' '[' interval_metric_value ',' interval_metric_count ']'  */
#line 259 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                {
                                                                    const RideMetric *m = RideMetricFactory::instance().rideMetric(yyvsp[-6]);
                                                                    if (m) {
                                                                        jc->interval.metrics()[m->index()] = yyvsp[-3].toDouble();
                                                                        jc->interval.counts()[m->index()] = yyvsp[-1].toDouble();
                                                                    } else qDebug()<<"metric not found:"<<yyvsp[-6];
                                                               }
#line 1475 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 44: /* interval_metric: interval_metric_key ':' '[' interval_metric_value ',' interval_metric_count ',' interval_metric_stdmean ',' interval_metric_stdvariance ']'  */
#line 267 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                {
                                                                    const RideMetric *m = RideMetricFactory::instance().rideMetric(yyvsp[-10]);
                                                                    if (m) {
                                                                        jc->interval.metrics()[m->index()] = yyvsp[-7].toDouble();
                                                                        jc->interval.counts()[m->index()] = yyvsp[-5].toDouble();
                                                                        jc->interval.stdmeans().insert(m->index(), yyvsp[-3].toDouble());
                                                                        jc->interval.stdvariances().insert(m->index(), yyvsp[-1].toDouble());
                                                                    } else qDebug()<<"metric not found:"<<yyvsp[-10];
                                                               }
#line 1489 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 45: /* interval_metric_key: string  */
#line 278 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->key = jc->JsonString; }
#line 1495 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 46: /* interval_metric_value: string  */
#line 279 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->value = jc->JsonString; }
#line 1501 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 47: /* interval_metric_count: string  */
#line 280 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->count = jc->JsonString; }
#line 1507 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 48: /* interval_metric_stdmean: string  */
#line 281 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->count = jc->JsonString; }
#line 1513 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 49: /* interval_metric_stdvariance: string  */
#line 282 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->count = jc->JsonString; }
#line 1519 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 53: /* tag: tag_key ':' tag_value  */
#line 290 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                {
                                                                    jc->item.metadata().insert(jc->key, jc->value);
                                                                }
#line 1527 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 54: /* tag_key: string  */
#line 294 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->key = jc->JsonString; }
#line 1533 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 55: /* tag_value: string  */
#line 295 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->value = jc->JsonString; }
#line 1539 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 59: /* xdata: xdata_name ':' '[' xdata_values ']'  */
#line 303 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->item.xdata().insert(jc->key,jc->JsonStringList);
                                                                  jc->key = ""; jc->JsonStringList.clear(); }
#line 1546 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 60: /* xdata_name: string  */
#line 306 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->key = jc->JsonString; }
#line 1552 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 64: /* xdata_value: string  */
#line 311 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                                { jc->JsonStringList << jc->JsonString; }
#line 1558 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;

  case 65: /* string: STRING  */
#line 317 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"
                                                             { jc->JsonString = yyvsp[0]; }
#line 1564 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"
    break;


#line 1568 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/RideDB_yacc.cpp"

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
      yyerror (jc, YY_("syntax error"));
    }

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
                      yytoken, &yylval, jc);
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


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, jc);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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
  yyerror (jc, YY_("memory exhausted"));
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
                  yytoken, &yylval, jc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, jc);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 319 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Core/RideDB.y"



void 
RideCache::load()
{
    // only load if it exists !
    QFile rideDB(QString("%1/%2").arg(context->athlete->home->cache().canonicalPath()).arg("rideDB.json"));
    if (rideDB.exists() && rideDB.open(QFile::ReadOnly)) {

        QDir directory = context->athlete->home->activities();
        QDir plannedDirectory = context->athlete->home->planned();

        // ok, lets read it in

        // Read the entire file into a QString -- we avoid using fopen since it
        // doesn't handle foreign characters well. Instead we use QFile and parse
        // from a QString
        QString contents = QString(rideDB.readAll());
        rideDB.close();

        // create scanner context for reentrant parsing
        RideDBContext *jc = new RideDBContext;
        jc->context = context;
        jc->cache = this;
        jc->api = NULL;
        jc->old = false;
        jc->loading = 0;
        jc->lastProgressUpdate = 0.0;
        jc->folder = context->athlete->home->root().canonicalPath();

        // clean item
        jc->item.path = directory.canonicalPath(); // TODO use plannedDirectory for planned
        jc->item.context = context;
        jc->item.isstale = jc->item.isdirty = jc->item.isedit = false;

        RideDBlex_init(&scanner);

        // inform the parser/lexer we have a new file
        RideDB_setString(contents, scanner);

        // setup
        jc->errors.clear();

        // set to non-zero if you want to
        // to debug the yyparse() state machine
        // sending state transitions to stderr
        //yydebug = 0;

        // parse it
        RideDBparse(jc);

        // clean up
        RideDBlex_destroy(scanner);

        // regardless of errors we're done !
        delete jc;

        return;
    }
}

// Escape special characters (JSON compliance)
static QString protect(const QString string)
{
    QString s = string;
    s.replace("\\", "\\\\"); // backslash
    s.replace("\"", "\\\""); // quote
    s.replace("\t", "\\t");  // tab
    s.replace("\n", "\\n");  // newline
    s.replace("\r", "\\r");  // carriage-return
    s.replace("\b", "\\b");  // backspace
    s.replace("\f", "\\f");  // formfeed
    s.replace("/", "\\/");   // solidus

    // add a trailing space to avoid conflicting with GC special tokens
    s += " "; 

    return s;
}

static QVector<int> mmp_durations;
static bool setup_mmp_durations()
{
    // 1s - 3mins every second
    for(int i=1; i<=180; i++) mmp_durations << i;

    // 3mins to 10mins every 5 seconds
    for(int i=185; i<=600; i+=5) mmp_durations << i;

    // 10mins to 30 mins every 30 seconds
    for(int i=630; i<=1800; i+=30) mmp_durations << i;

    // 30mins to 2hrs every minute
    for(int i=1860; i<=(120*60); i += 60) mmp_durations << i;

    // 2hrs+ every 30 mins
    for(int i=(120*60)+1800; i <= (10 * 60 * 60); i+=1800) mmp_durations << i;

    return true;
}
static bool did_mmp_durations = setup_mmp_durations();

// Fast constructon of common string. In the future we'll achieve this with a single variadic template function.
QString ConstructNameNumberString(QString s0, QString name, QString s1, double num, QString s2)
{
    QString numStr;						
    numStr.setNum(num, 'f', 5);
    
    QString m;
    m.reserve(s0.length() + name.length() + s1.length() + numStr.length() + s2.length());
    
    m.append(s0);
    m.append(name);
    m.append(s1);
    m.append(numStr);
    m.append(s2);
    
    return m;
}

// Fast constructon of common string. In the future we'll achieve this with a single variadic template function.
QString ConstructNameNumberNumberString(QString s0, QString name, QString s1, double num0, QString s2, double num1, QString s3)
{
    QString num0Str;
    num0Str.setNum(num0, 'f', 5);

	QString num1Str;
    num1Str.setNum(num1, 'f', 5);
    
    QString m;
    m.reserve(s0.length() + name.length() + s1.length() + num0Str.length() + s2.length() + num1Str.length() + s3.length());
    
    m.append(s0);
    m.append(name);
    m.append(s1);
    m.append(num0Str);
    m.append(s2);
    m.append(num1Str);
    m.append(s3);
    
    return m;
}


// save cache to disk
//
// if opendata is true then save in format for sending to the GC OpenData project
// the filename may be supplied if exporting for other purposes, if empty then save
// to ~athlete/cache/rideDB.json
//
// When writing for opendata the file this doesn't (and must not) contain PII or
// metadata, but does include some distributions for Heartrate, Power, Cadence
// and Speed along with MMP data. We need to consider what athlete info to add
// possibly only gender and year of birth.
//
// it must be valid json and can be parse with python using
//      import json
//      with open('rideDB.json') as json_data:
//          d = json.load(json_data)
//      print(len(d["RIDES"])
//
void RideCache::save(bool opendata, QString filename)
{

    // now save data away - use passed filename if set
    QFile rideDB(QString("%1/%2").arg(context->athlete->home->cache().canonicalPath()).arg("rideDB.json"));
    if (filename != "") rideDB.setFileName(filename);

    if (rideDB.open(QFile::WriteOnly)) {

        const RideMetricFactory &factory = RideMetricFactory::instance();

        // ok, lets write out the cache
        QTextStream stream(&rideDB);
#if QT_VERSION < 0x060000
        stream.setCodec("UTF-8");
#endif

        // no BOM needed for opendata as it doesn't contain textual data
        if (!opendata) stream.setGenerateByteOrderMark(true);

        stream << "{" ;
        stream << QString("\n  \"VERSION\":\"%1\",").arg(RIDEDB_VERSION);

        // send very basic athlete info with no PII
        // year of birth, gender, uuid
        if (opendata) {
            stream << QString("\n  \"ATHLETE\":{ \"gender\":\"%1\", \"yob\":\"%2\", \"id\":\"%3\" },")
                        .arg(appsettings->cvalue(context->athlete->cyclist, GC_SEX).toInt() == 0 ? "M" : "F")
                        .arg(appsettings->cvalue(context->athlete->cyclist, GC_DOB).toDate().year())
                        .arg(context->athlete->id.toString());
        }

        stream << "\n  \"RIDES\":[\n";

        bool firstRide = true;
        foreach(RideItem *item, rides()) {

            // skip if not loaded/refreshed, a special case
            // if saving during an initial refresh
            if (item->metrics().count() == 0) continue;

            // don't save files with discarded changes at exit
            if (item->skipsave == true) continue;

            // comma separate each ride
            if (!firstRide) stream << ",\n";
            firstRide = false;

            // basic ride information
            stream << "\t{\n";
            stream << "\t\t\"date\":\"" <<item->dateTime.toUTC().toString(DATETIME_FORMAT) << "\",\n";

            if (!opendata) {
                // we don't send this info when sharing as opendata
                stream << "\t\t\"filename\":\"" <<item->fileName <<"\",\n";
                stream << "\t\t\"fingerprint\":\"" <<item->fingerprint <<"\",\n";
                stream << "\t\t\"crc\":\"" <<item->crc <<"\",\n";
                stream << "\t\t\"metacrc\":\"" <<item->metacrc <<"\",\n";
                stream << "\t\t\"timestamp\":\"" <<item->timestamp <<"\",\n";
                stream << "\t\t\"dbversion\":\"" <<item->dbversion <<"\",\n";
                stream << "\t\t\"udbversion\":\"" <<item->udbversion <<"\",\n";
                stream << "\t\t\"color\":\"" <<item->color.name() <<"\",\n";
                stream << "\t\t\"present\":\"" <<item->present <<"\",\n";
                stream << "\t\t\"sport\":\"" <<item->sport <<"\",\n";
                stream << "\t\t\"aero\":\"" << (item->isAero ? "1" : "0") <<"\",\n";
                stream << "\t\t\"weight\":\"" <<item->weight <<"\",\n";

                if (item->zoneRange >= 0) stream << "\t\t\"zonerange\":\"" <<item->zoneRange <<"\",\n";
                if (item->hrZoneRange >= 0) stream << "\t\t\"hrzonerange\":\"" <<item->hrZoneRange <<"\",\n";
                if (item->paceZoneRange >= 0) stream << "\t\t\"pacezonerange\":\"" <<item->paceZoneRange <<"\",\n";

                // if there are overrides, do share them
                if (item->overrides_.count()) stream << "\t\t\"overrides\":\"" <<item->overrides_.join(",") <<"\",\n";

                stream << "\t\t\"samples\":\"" <<(item->samples ? "1" : "0") <<"\",\n";
            } else {

                // need to know what data was collected
                stream << "\t\t\"data\":\"" <<item->getText("Data","") <<"\",\n";
                stream << "\t\t\"sport\":\"" <<item->getText("Sport","") <<"\",\n";
            }

            // pre-computed metrics
            stream << "\n\t\t\"METRICS\":{\n";

            bool firstMetric = true;
            for(int i=0; i<factory.metricCount(); i++) {
                QString name = factory.metricName(i);
                int index = factory.rideMetric(name)->index();

                // don't output 0, nan or inf values, they're set to 0 by default
                // unless aggregateZero indicates the count is relevant
                if ((!std::isinf(item->metrics()[index]) && !std::isnan(item->metrics()[index]) &&
                    (item->metrics()[index] > 0.00f || item->metrics()[index] < 0.00f)) ||
                    (item->metrics()[index] == 0.00f && item->counts()[index] > 1.0 && factory.rideMetric(name)->aggregateZero())) {
                    if (!firstMetric) stream << ",\n";
                    firstMetric = false;

                    // if stdmean or variance is non-zero we write all 4
                    if (item->stdmeans().value(index, 0.0f) || item->stdvariances().value(index, 0.0f)) {

                        stream << "\t\t\t\"" << name << "\":[\"" << QString("%1").arg(item->metrics()[index], 0, 'f', 5) <<"\",\""
                                                                   << QString("%1").arg(item->counts()[index], 0, 'f', 5) << "\",\""
                                                                   << QString("%1").arg(item->stdmeans().value(index, 0.0f), 0, 'f', 5) << "\",\""
                                                                   << QString("%1").arg(item->stdvariances().value(index, 0.0f), 0, 'f', 5) <<"\"]";
                    } else if (item->counts()[index] == 0) {
                        // if count is 0 don't write it
						stream << ConstructNameNumberString(QString("\t\t\t\""), name,
                            QString("\":\""), item->metrics()[index], QString("\""));
                    } else {
					    stream << ConstructNameNumberNumberString(QString("\t\t\t\""), name,
                            QString("\":[\""), item->metrics()[index], QString("\",\""), item->counts()[index], QString("\"]"));
                    }
                }
            }

            // if opendata lets put in the distributions for power, hr, cadence and speed
            if (opendata) {

                QString data = item->getText("Data","");

                if (data.contains("P") || data.contains("H") || data.contains("S") || data.contains("C")) {

                    // get the cache -- may refresh if its out of data, so this might take a while ....
                    RideFileCache *cache =  new RideFileCache(context, item->fileName, item->weight, NULL, false, true);

                    QList<RideFile::SeriesType> list;
                    if (data.contains("P")) list <<RideFile::watts;
                    if (data.contains("H")) list <<RideFile::hr;
                    if (data.contains("S")) list <<RideFile::kph;
                    if (data.contains("C")) list <<RideFile::cad;

                    // output distribution for each series
                    foreach(RideFile::SeriesType x, list) {

                        QVector<double> &array = cache->distributionArray(x);
                        int split= x==RideFile::cad ? 5 : 10; // set bin size to use
                        int div=   x==RideFile::kph ? 10 : 1; // speed needs to be divided by 10 to get kph

                        // distribution arrays:
                        // power_dist_bins:[ n1, n2, n3 ... ],
                        // power_dist:[ p1, p2, p3 ... ]
                        //
                        // the bins are always 10w wide, so we aggregate the data from
                        // the ridefilecache before outputting the bin starts and values
                        QVector<int> bins, totals;

                        // lets aggregates
                        int count=0;
                        double total=0;
                        for(int i=0; i< array.count(); i++) {

                            // save value away
                            if (count==split) {
                                if (total > 0) { // need a value !
                                    bins << i-split;
                                    totals << total;
                                }
                                count=0;
                                total=0;
                            }

                            total += array[i];
                            count++;
                        }

                        // add last partial bin
                        if (total > 0 && count > 0) {
                            bins << array.count()-count;
                            totals << total;
                        }

                        // series name
                        QString type=RideFile::seriesName(x, true).toLower();

                        // now write the distribution and the bins
                        if (bins.count()) {

                            // totals
                            stream << ",\n\t\t\t\""<<type<<"_dist\":[" ;
                            for(int i=0; i < totals.count(); i++) {
                                stream<< QString("%1").arg(totals[i]);
                                if (i+1 != bins.count()) stream<<", ";
                            }
                            stream << " ]";

                            // bins
                            stream << ",\n\t\t\t\""<<type<<"_dist_bins\":[" ;
                            for(int i=0; i < bins.count(); i++) {
                                stream<< QString("%1").arg(bins[i] > 0 ? bins[i] / div : 0);
                                if (i+1 != bins.count()) stream<<", ";
                            }
                            stream << " ]";

                        }
                    }

                    // MMP - We want to try and keep some of the resolution as this is likely to be filtered
                    //       and/or aggregated in some fashion by the user. So we choose to keep resolution
                    //       high at short durations and low at long durations with the points at which the
                    //       resolution degrades at some point after a physiological boundary.
                    //
                    // power_mmp - the mean max values
                    // power_mmp_secs - the mean max durations
                    if (data.contains("P")) {

                        QVector<double> &array = cache->meanMaxArray(RideFile::watts);

                        if (array.count() > 0) {

                            // power_mmp
                            stream << ",\n\t\t\t\"power_mmp\":[" ;
                            for (int i=0; i<mmp_durations.count() && mmp_durations[i] < array.count(); i++) {
                                if (i>0) stream << ", "; // for all but first value add comma
                                stream<< QString("%1").arg(array[mmp_durations[i]], 0, 'f', 0);
                            }
                            stream << " ]";

                            // power_mmp_secs
                            stream << ",\n\t\t\t\"power_mmp_secs\":[" ;
                            for (int i=0; i<mmp_durations.count() && mmp_durations[i] < array.count(); i++) {
                                if (i>0) stream << ", "; // for all but first value add comma
                                stream<< QString("%1").arg(mmp_durations[i]);
                            }
                            stream << " ]";
                        }
                    }
                    delete cache;
                }

            }
            stream << "\n\t\t}";

            // pre-loaded metadata - don't send to OpenData
            if (!opendata && item->metadata().count()) {

                stream << ",\n\t\t\"TAGS\":{\n";

                QMap<QString,QString>::const_iterator i;
                for (i=item->metadata().constBegin(); i != item->metadata().constEnd(); i++) {

                    stream << "\t\t\t\"" << i.key() << "\":\"" << protect(i.value()) << "\"";
                    if (std::next(i) != item->metadata().constEnd()) stream << ",\n";
                    else stream << "\n";
                }

                // end of the tags
                stream << "\n\t\t}";

            }

            // xdata definitions
            if (item->xdata().count()) {
                stream << ",\n\t\t\"XDATA\":{\n";

                QMap<QString, QStringList>::const_iterator i;
                for (i=item->xdata().constBegin(); i != item->xdata().constEnd(); i++) {

                    stream << "\t\t\t\"" << i.key() << "\":[ ";
                    bool first=true;
                    foreach(QString x, i.value()) {
                        if (!first) {
                            stream << ", ";
                        }
                        stream << "\"" << protect(x) << "\"";
                        first=false;
                    }

                    if (std::next(i) != item->xdata().constEnd()) stream << "],\n";
                    else stream << "]\n";
                }

                // end of the xdata
                stream << "\n\t\t}";
            }

            // intervals - but not for opendata
            if (!opendata && item->intervals().count()) {

                stream << ",\n\t\t\"INTERVALS\":[\n";
                bool firstInterval = true;
                foreach(IntervalItem *interval, item->intervals()) {

                    // comma separate
                    if (!firstInterval) stream << ",\n";
                    firstInterval = false;

                    stream << "\t\t\t{\n";

                    // interval main data 
                    stream << "\t\t\t\"name\":\"" << protect(interval->name) <<"\",\n";
                    stream << "\t\t\t\"start\":\"" << interval->start <<"\",\n";
                    stream << "\t\t\t\"stop\":\"" << interval->stop <<"\",\n";
                    stream << "\t\t\t\"startKM\":\"" << interval->startKM <<"\",\n";
                    stream << "\t\t\t\"stopKM\":\"" << interval->stopKM <<"\",\n";
                    stream << "\t\t\t\"type\":\"" << static_cast<int>(interval->type) <<"\",\n";
                    stream << "\t\t\t\"test\":\"" << (interval->test ? "true" : "false") <<"\",\n";
                    stream << "\t\t\t\"color\":\"" << interval->color.name() <<"\",\n";

                    // routes have a segment identifier
                    if (interval->type == RideFileInterval::ROUTE) {
                        stream << "\t\t\t\"route\":\"" << interval->route.toString() <<"\",\n"; // last one no ',\n' see METRICS below..
                    }

                    stream << "\t\t\t\"seq\":\"" << interval->displaySequence <<"\""; // last one no ',\n' see METRICS below..


                    // check if we have any non-zero metrics
                    bool hasMetrics=false;
                    foreach(double v, interval->metrics()) {
                        if (v > 0.00f || v < 0.00f) {
                            hasMetrics=true;
                            break;
                        }
                    }

                    if (hasMetrics) {
                        stream << ",\n\n\t\t\t\"METRICS\":{\n";

                        bool firstMetric = true;
                        for(int i=0; i<factory.metricCount(); i++) {
                            QString name = factory.metricName(i);
                            int index = factory.rideMetric(name)->index();
        
                            // don't output 0 values, they're set to 0 by default
                            // unless aggregateZero indicates the count is relevant
                            if ((interval->metrics()[index] > 0.00f || interval->metrics()[index] < 0.00f) ||
                                (item->metrics()[index] == 0.00f && item->counts()[i] > 1.0 && factory.rideMetric(name)->aggregateZero())) {
                                if (!firstMetric) stream << ",\n";
                                firstMetric = false;

                                if (interval->stdmeans().value(index, 0.0f) || interval->stdvariances().value(index, 0.0f)) {

                                    stream << "\t\t\t\t\"" << name << "\": [ \"" << QString("%1").arg(interval->metrics()[index], 0, 'f', 5) <<"\",\""
                                                                               << QString("%1").arg(interval->counts()[index], 0, 'f', 5) << "\",\""
                                                                               << QString("%1").arg(interval->stdmeans().value(index, 0.0f), 0, 'f', 5) << "\",\""
                                                                               << QString("%1").arg(interval->stdvariances().value(index, 0.0f), 0, 'f', 5) <<"\"]";

                                // if count is 0 don't write it
                                } else if (interval->counts()[index] == 0) {
                                    stream << ConstructNameNumberString(QString("\t\t\t\""), name,
                                        QString("\":\""), interval->metrics()[index], QString("\""));
                                } else {
                                    stream << ConstructNameNumberNumberString(QString("\t\t\t\""), name,
                                        QString("\":[\""), interval->metrics()[index], QString("\",\""), interval->counts()[index], QString("\"]"));
                                }
                            }
                        }
                        stream << "\n\t\t\t\t}";
                    }

                    // endof interval
                    stream << "\n\t\t\t}";
                }
                // end of intervals
                stream <<"\n\t\t]";

            }


            // end of the ride
            stream << "\n\t}";
        }

        stream << "\n  ]\n}";

        rideDB.close();
    }
}

#ifdef GC_WANT_HTTP
#include "RideMetadata.h"

void
APIWebService::listRides(QString athlete, HttpRequest &request, HttpResponse &response)
{
    listRideSettings settings;

    // the ride db
    QString ridedb = QString("%1/%2/cache/rideDB.json").arg(home.absolutePath()).arg(athlete);
    QFile rideDB(ridedb);

    // list activities and associated metrics
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");

    // not known..
    if (!rideDB.exists()) {
        response.setStatus(404);
        response.write("malformed URL or unknown athlete.\n");
        return;
    }

    // intervals or rides?
    QString intervalsp = request.getParameter("intervals");
    if (intervalsp.toUpper() == "TRUE") settings.intervals = true;
    else settings.intervals = false;

    // set user data
    response.setUserData(&settings);

    // write headings
    const RideMetricFactory &factory = RideMetricFactory::instance();
    QVector<const RideMetric *> indexed(factory.metricCount());

    // get metrics indexed in same order as the array
    foreach(QString name, factory.allMetrics()) {

        const RideMetric *m = factory.rideMetric(name);
        indexed[m->index()] = m;
    }

    // was the metric parameter passed?
    QString metrics(request.getParameter("metrics"));

    // do all ?
    bool nometrics = false;
    QStringList wantedNames;
    if (metrics != "") wantedNames = metrics.split(",");

    // write headings
    response.bwrite("date, time, filename");

    // don't want metrics, so do it fast by traversing the ride directory
    if (wantedNames.count() == 1 && wantedNames[0].toUpper() == "NONE") nometrics = true;

    // if intervals, add interval name
    if (settings.intervals == true) response.bwrite(", interval name, interval type");

    // get metadata definitions into settings
    QString metadata = request.getParameter("metadata");
    bool nometa = true;
    if (metadata.toUpper() != "NONE" && metadata != "") {

        // first lets read in meta config
        QString metaConfig = home.canonicalPath()+"/metadata.xml";
        if (!QFile(metaConfig).exists()) metaConfig = ":/xml/metadata.xml";

        // params to readXML - we ignore them
        QList<KeywordDefinition> keywordDefinitions;
        QString colorfield;
        QList<DefaultDefinition> defaultDefinitions;

        RideMetadata::readXML(metaConfig, keywordDefinitions, settings.metafields, colorfield, defaultDefinitions);

        SpecialFields& sp = SpecialFields::getInstance();

        // what is being asked for ?
        if (metadata.toUpper() == "ALL") {

            // output all metadata
            foreach(FieldDefinition field, settings.metafields) {
                // don't do metric overrides !
                if(!sp.isMetric(field.name)) settings.metawanted << field.name;
            }

        } else {

            // selected fields - check they exist !
            QStringList meta = metadata.split(",");
            foreach(QString field, meta) {

                // don't do metric overrides !
                if(sp.isMetric(field)) continue;

                // does it exist ?
                QString lookup = field;
                lookup.replace("_", " ");
                foreach(FieldDefinition field, settings.metafields) {
                    if (field.name == lookup) settings.metawanted << field.name;
                }
            }
        }

        // we found some?
        if(settings.metawanted.count()) nometa = false;
    }

    // list 'em by reading the ride cache from disk
    if ((nometa == false || nometrics == false) && settings.intervals == false) {

        int i=0;
        foreach(const RideMetric *m, indexed) {

            i++;

            // if limited don't do limited headings
            QString underscored = m->name().replace(" ","_");
            if (wantedNames.count() && !wantedNames.contains(underscored)) continue;

            if (m->name().startsWith("BikeScore"))
                response.bwrite(", BikeScore");
            else {
                response.bwrite(", ");
                response.bwrite(underscored.toLocal8Bit());
            }

            // index of wanted metrics
            settings.wanted << (i-1);
        }

        // do we want metadata too ?
        foreach(QString meta, settings.metawanted) {
            meta.replace(" ", "_");
            response.bwrite(", \"");
            response.bwrite(meta.toLocal8Bit());
            response.bwrite("\"");
        }
        response.bwrite("\n");

        // parse the rideDB and write a line for each entry
        if (rideDB.exists() && rideDB.open(QFile::ReadOnly)) {

            // ok, lets read it in
            QTextStream stream(&rideDB);
#if QT_VERSION < 0x060000
            stream.setCodec("UTF-8");
#endif

            // Read the entire file into a QString -- we avoid using fopen since it
            // doesn't handle foreign characters well. Instead we use QFile and parse
            // from a QString
            QString contents = stream.readAll();
            rideDB.close();

            // create scanner context for reentrant parsing
            RideDBContext *jc = new RideDBContext;
            jc->cache = NULL;
            jc->api = this;
            jc->response = &response;
            jc->request = &request;
            jc->old = false;

            // clean item
            jc->item.path = home.absolutePath() + "/activities";
            jc->item.context = NULL;
            jc->item.isstale = jc->item.isdirty = jc->item.isedit = false;

            RideDBlex_init(&scanner);

            // inform the parser/lexer we have a new file
            RideDB_setString(contents, scanner);

            // setup
            jc->errors.clear();

            // parse it
            RideDBparse(jc);

            // clean up
            RideDBlex_destroy(scanner);

            // regardless of errors we're done !
            delete jc;
        }

    } else {

        // honour the since parameter
        QString sincep(request.getParameter("since"));
        QDate since(1900,01,01);
        if (sincep != "") since = QDate::fromString(sincep,"yyyy/MM/dd");

        // before parameter
        QString beforep(request.getParameter("before"));
        QDate before(3000,01,01);
        if (beforep != "") before = QDate::fromString(beforep,"yyyy/MM/dd");

        // fast list of rides by traversing the directory
        response.bwrite("\n"); // headings have no metric columns

        // This will read the user preferences and change the file list order as necessary:
        QFlags<QDir::Filter> spec = QDir::Files;
        QStringList names;
        names << "*"; // anything

        // loop through files, make sure in time range wanted
        QDir activities(home.absolutePath() + "/" + athlete + "/activities");
        foreach(QString name, activities.entryList(names, spec, QDir::Name)) {

            // parse it into date and time
            QDateTime dateTime;
            if (!RideFile::parseRideFileName(name, &dateTime)) continue; 

            // in range?
            if (dateTime.date() < since || dateTime.date() > before) continue;

            // is it a backup ?
            if (name.endsWith(".bak")) continue;

            // out a line
            response.bwrite(dateTime.date().toString("yyyy/MM/dd").toLocal8Bit());
            response.bwrite(", ");
            response.bwrite(dateTime.time().toString("hh:mm:ss").toLocal8Bit());;
            response.bwrite(", ");
            response.bwrite(name.toLocal8Bit());
            response.bwrite("\n");
        }
    }
    response.flush();
}
#endif
