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
#define yyparse         WorkoutFilterparse
#define yylex           WorkoutFilterlex
#define yyerror         WorkoutFiltererror
#define yydebug         WorkoutFilterdebug
#define yynerrs         WorkoutFilternerrs
#define yylval          WorkoutFilterlval
#define yychar          WorkoutFilterchar

/* First part of user prologue.  */
#line 1 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"

/*
 * Copyright (c) 2023 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "WorkoutFilter.h"

#include <stdio.h>
#include <stdlib.h>
#include <QStringList>
#include <QString>
#include <QDateTime>
#include <QDebug>

#include "ModelFilter.h"
#include "TrainDB.h"

extern int yylex(void);
extern void yyerror(const char *);
extern QList<ModelFilter*> workoutModelFilters;
extern QString workoutModelErrorMsg;

#line 115 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"

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

#include "WorkoutFilter_yacc.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_DOMINANTZONE = 3,               /* DOMINANTZONE  */
  YYSYMBOL_DURATION = 4,                   /* DURATION  */
  YYSYMBOL_INTENSITY = 5,                  /* INTENSITY  */
  YYSYMBOL_VI = 6,                         /* VI  */
  YYSYMBOL_XPOWER = 7,                     /* XPOWER  */
  YYSYMBOL_RI = 8,                         /* RI  */
  YYSYMBOL_BIKESCORE = 9,                  /* BIKESCORE  */
  YYSYMBOL_SVI = 10,                       /* SVI  */
  YYSYMBOL_STRESS = 11,                    /* STRESS  */
  YYSYMBOL_RATING = 12,                    /* RATING  */
  YYSYMBOL_RANGESYMBOL = 13,               /* RANGESYMBOL  */
  YYSYMBOL_SEPARATOR = 14,                 /* SEPARATOR  */
  YYSYMBOL_FLOAT = 15,                     /* FLOAT  */
  YYSYMBOL_NUMBER = 16,                    /* NUMBER  */
  YYSYMBOL_ZONE = 17,                      /* ZONE  */
  YYSYMBOL_PERCENT = 18,                   /* PERCENT  */
  YYSYMBOL_DAYS = 19,                      /* DAYS  */
  YYSYMBOL_TIME = 20,                      /* TIME  */
  YYSYMBOL_MINPOWER = 21,                  /* MINPOWER  */
  YYSYMBOL_MAXPOWER = 22,                  /* MAXPOWER  */
  YYSYMBOL_AVGPOWER = 23,                  /* AVGPOWER  */
  YYSYMBOL_ISOPOWER = 24,                  /* ISOPOWER  */
  YYSYMBOL_POWER = 25,                     /* POWER  */
  YYSYMBOL_LASTRUN = 26,                   /* LASTRUN  */
  YYSYMBOL_CREATED = 27,                   /* CREATED  */
  YYSYMBOL_DISTANCE = 28,                  /* DISTANCE  */
  YYSYMBOL_ELEVATION = 29,                 /* ELEVATION  */
  YYSYMBOL_GRADE = 30,                     /* GRADE  */
  YYSYMBOL_WORD = 31,                      /* WORD  */
  YYSYMBOL_32_ = 32,                       /* ','  */
  YYSYMBOL_YYACCEPT = 33,                  /* $accept  */
  YYSYMBOL_statements = 34,                /* statements  */
  YYSYMBOL_statement = 35,                 /* statement  */
  YYSYMBOL_dominantzone = 36,              /* dominantzone  */
  YYSYMBOL_zone = 37,                      /* zone  */
  YYSYMBOL_duration = 38,                  /* duration  */
  YYSYMBOL_intensity = 39,                 /* intensity  */
  YYSYMBOL_vi = 40,                        /* vi  */
  YYSYMBOL_xpower = 41,                    /* xpower  */
  YYSYMBOL_ri = 42,                        /* ri  */
  YYSYMBOL_bikescore = 43,                 /* bikescore  */
  YYSYMBOL_svi = 44,                       /* svi  */
  YYSYMBOL_stress = 45,                    /* stress  */
  YYSYMBOL_minpower = 46,                  /* minpower  */
  YYSYMBOL_maxpower = 47,                  /* maxpower  */
  YYSYMBOL_avgpower = 48,                  /* avgpower  */
  YYSYMBOL_isopower = 49,                  /* isopower  */
  YYSYMBOL_power = 50,                     /* power  */
  YYSYMBOL_lastrun = 51,                   /* lastrun  */
  YYSYMBOL_created = 52,                   /* created  */
  YYSYMBOL_rating = 53,                    /* rating  */
  YYSYMBOL_distance = 54,                  /* distance  */
  YYSYMBOL_elevation = 55,                 /* elevation  */
  YYSYMBOL_grade = 56,                     /* grade  */
  YYSYMBOL_words = 57,                     /* words  */
  YYSYMBOL_word = 58,                      /* word  */
  YYSYMBOL_mixedNumValue = 59,             /* mixedNumValue  */
  YYSYMBOL_minuteValue = 60                /* minuteValue  */
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
#define YYFINAL  96
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   163

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  33
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  28
/* YYNRULES -- Number of rules.  */
#define YYNRULES  118
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  166

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   286


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
       2,     2,     2,     2,    32,     2,     2,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    31
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    76,    76,    77,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   104,   105,   106,   107,
     110,   111,   112,   113,   114,   115,   116,   117,   120,   121,
     122,   123,   126,   127,   128,   129,   132,   133,   134,   135,
     138,   139,   140,   141,   144,   145,   146,   147,   150,   151,
     152,   153,   156,   157,   158,   159,   162,   163,   164,   165,
     168,   169,   170,   171,   174,   175,   176,   177,   180,   181,
     182,   183,   186,   187,   188,   189,   192,   194,   196,   199,
     201,   203,   207,   209,   211,   215,   216,   217,   218,   221,
     222,   223,   224,   227,   228,   229,   230,   233,   234,   235,
     236,   239,   240,   243,   244,   247,   248,   251,   252
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
  "\"end of file\"", "error", "\"invalid token\"", "DOMINANTZONE",
  "DURATION", "INTENSITY", "VI", "XPOWER", "RI", "BIKESCORE", "SVI",
  "STRESS", "RATING", "RANGESYMBOL", "SEPARATOR", "FLOAT", "NUMBER",
  "ZONE", "PERCENT", "DAYS", "TIME", "MINPOWER", "MAXPOWER", "AVGPOWER",
  "ISOPOWER", "POWER", "LASTRUN", "CREATED", "DISTANCE", "ELEVATION",
  "GRADE", "WORD", "','", "$accept", "statements", "statement",
  "dominantzone", "zone", "duration", "intensity", "vi", "xpower", "ri",
  "bikescore", "svi", "stress", "minpower", "maxpower", "avgpower",
  "isopower", "power", "lastrun", "created", "rating", "distance",
  "elevation", "grade", "words", "word", "mixedNumValue", "minuteValue", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-8)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
       3,    24,    30,    49,    54,    58,    62,    66,    70,    71,
      75,    -8,    22,    76,    77,    81,    83,    85,     4,    26,
      96,    98,    34,    -8,    16,    21,    -8,    -8,    -8,    -8,
      -8,    -8,    -8,    -8,    -8,    -8,    -8,    -8,    -8,    -8,
      -8,    -8,    -8,    -8,    -8,    -8,    -8,    -8,     5,    43,
      55,     2,    -8,    -8,    59,    33,    -8,    -8,    63,    33,
      67,    33,    90,    33,    92,    33,    94,    33,   100,   101,
     103,   102,   106,    38,   107,   108,   109,   110,   111,   113,
     112,   116,   114,   118,   117,   119,   105,   121,   120,   122,
     124,   123,   125,   129,   104,   130,    -8,     3,    -8,    -8,
     127,    -8,     2,    -8,    33,    -8,    33,    -8,    33,    -8,
      33,    -8,    33,    -8,    33,    -8,   131,    -8,   132,    -8,
      -8,   128,     2,    -8,   133,    -8,   134,    -8,   135,    -8,
     136,    -8,   137,    -8,   126,    -8,   138,    -8,   139,    -8,
     140,    -8,   141,    -8,    -8,    -8,    -8,    -8,    -8,    -8,
      -8,    -8,    -8,    -8,    -8,    -8,    -8,    -8,    -8,    -8,
      -8,    -8,    -8,    -8,    -8,    -8
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   114,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   113,     0,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    15,    16,    17,    18,
      19,    20,    21,    14,    22,    23,    24,    25,   112,     0,
      26,     0,   117,   118,    38,     0,   115,   116,    42,     0,
      46,     0,    50,     0,    54,     0,    58,     0,    62,     0,
      66,     0,    95,     0,    34,    30,     0,    70,     0,    74,
       0,    78,     0,    82,     0,     0,     0,     0,     0,     0,
       0,    99,     0,   103,     0,   107,     1,     0,   111,    28,
      29,    40,    41,    44,    45,    48,    49,    52,    53,    56,
      57,    60,    61,    64,    65,    68,    69,    97,    98,    36,
      32,    37,    33,    72,    73,    76,    77,    80,    81,    84,
      85,    87,    88,    90,    91,    93,    94,   101,   102,   105,
     106,   109,   110,     2,    27,    39,    43,    47,    51,    55,
      59,    63,    67,    96,    35,    31,    71,    75,    79,    83,
      86,    89,    92,   100,   104,   108
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
      -8,    40,    -8,    -8,    -8,    -8,    -8,    -8,    -8,    -8,
      -8,    -8,    -8,    -8,    -8,    -8,    -8,    -8,    -8,    -8,
      -8,    -8,    -8,    -8,   115,    -8,    -4,    -7
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    58,    54
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      60,    62,    64,    66,    68,    75,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    96,    86,    52,    11,
      12,    11,    53,    87,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    73,    23,    49,    52,    88,
      74,    50,    53,    51,   101,    89,    52,    94,    56,    57,
      53,   103,    95,    97,    52,   105,   119,   107,    53,   109,
      99,   111,    55,   113,    56,    57,   120,    59,   100,    56,
      57,    61,   102,    56,    57,    63,   104,    56,    57,    65,
     106,    56,    57,    67,    69,    56,    57,    70,    71,    76,
      78,    72,    77,    79,    80,   145,    82,    81,    84,    83,
     146,    85,   147,   108,   148,   110,   149,   112,   150,    90,
     151,    92,    91,   114,    93,   155,   116,   115,   117,   118,
     121,   122,   141,   124,   133,   123,   126,   125,   127,   128,
     129,   130,   132,   131,   134,   136,   138,   143,     0,   135,
     137,   139,   140,   142,   144,   161,   154,   152,   153,   156,
     157,   158,   159,   160,     0,   163,   164,   162,     0,   165,
       0,     0,     0,    98
};

static const yytype_int8 yycheck[] =
{
       4,     5,     6,     7,     8,    12,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,     0,    13,    16,    16,
      17,    16,    20,    19,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    13,    31,    13,    16,    13,
      18,    17,    20,    13,    51,    19,    16,    13,    15,    16,
      20,    55,    18,    32,    16,    59,    18,    61,    20,    63,
      17,    65,    13,    67,    15,    16,    73,    13,    13,    15,
      16,    13,    13,    15,    16,    13,    13,    15,    16,    13,
      13,    15,    16,    13,    13,    15,    16,    16,    13,    13,
      13,    16,    16,    16,    13,   102,    13,    16,    13,    16,
     104,    16,   106,    13,   108,    13,   110,    13,   112,    13,
     114,    13,    16,    13,    16,   122,    13,    16,    16,    13,
      13,    13,    18,    13,    19,    16,    13,    16,    16,    13,
      16,    13,    13,    16,    13,    13,    13,    97,    -1,    19,
      16,    16,    13,    13,    17,    19,    18,    16,    16,    16,
      16,    16,    16,    16,    -1,    16,    16,    19,    -1,    18,
      -1,    -1,    -1,    48
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    16,    17,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    13,
      17,    13,    16,    20,    60,    13,    15,    16,    59,    13,
      59,    13,    59,    13,    59,    13,    59,    13,    59,    13,
      16,    13,    16,    13,    18,    60,    13,    16,    13,    16,
      13,    16,    13,    16,    13,    16,    13,    19,    13,    19,
      13,    16,    13,    16,    13,    18,     0,    32,    57,    17,
      13,    60,    13,    59,    13,    59,    13,    59,    13,    59,
      13,    59,    13,    59,    13,    16,    13,    16,    13,    18,
      60,    13,    13,    16,    13,    16,    13,    16,    13,    16,
      13,    16,    13,    19,    13,    19,    13,    16,    13,    16,
      13,    18,    13,    34,    17,    60,    59,    59,    59,    59,
      59,    59,    16,    16,    18,    60,    16,    16,    16,    16,
      16,    19,    19,    16,    16,    18
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    33,    34,    34,    35,    35,    35,    35,    35,    35,
      35,    35,    35,    35,    35,    35,    35,    35,    35,    35,
      35,    35,    35,    35,    35,    35,    36,    36,    36,    36,
      37,    37,    37,    37,    37,    37,    37,    37,    38,    38,
      38,    38,    39,    39,    39,    39,    40,    40,    40,    40,
      41,    41,    41,    41,    42,    42,    42,    42,    43,    43,
      43,    43,    44,    44,    44,    44,    45,    45,    45,    45,
      46,    46,    46,    46,    47,    47,    47,    47,    48,    48,
      48,    48,    49,    49,    49,    49,    50,    50,    50,    51,
      51,    51,    52,    52,    52,    53,    53,    53,    53,    54,
      54,    54,    54,    55,    55,    55,    55,    56,    56,    56,
      56,    57,    57,    58,    58,    59,    59,    60,    60
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     4,     3,     3,
       2,     4,     3,     3,     2,     4,     3,     3,     2,     4,
       3,     3,     2,     4,     3,     3,     2,     4,     3,     3,
       2,     4,     3,     3,     2,     4,     3,     3,     2,     4,
       3,     3,     2,     4,     3,     3,     2,     4,     3,     3,
       2,     4,     3,     3,     2,     4,     3,     3,     2,     4,
       3,     3,     2,     4,     3,     3,     4,     3,     3,     4,
       3,     3,     4,     3,     3,     2,     4,     3,     3,     2,
       4,     3,     3,     2,     4,     3,     3,     2,     4,     3,
       3,     2,     1,     1,     1,     1,     1,     1,     1
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
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
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
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
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
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
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
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_dominantzone: /* dominantzone  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1016 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_zone: /* zone  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1022 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_duration: /* duration  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1028 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_intensity: /* intensity  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1034 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_vi: /* vi  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1040 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_xpower: /* xpower  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1046 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_ri: /* ri  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1052 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_bikescore: /* bikescore  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1058 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_svi: /* svi  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1064 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_stress: /* stress  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1070 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_minpower: /* minpower  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1076 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_maxpower: /* maxpower  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1082 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_avgpower: /* avgpower  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1088 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_isopower: /* isopower  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1094 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_power: /* power  */
#line 50 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting filterPair"; delete static_cast<std::pair<ModelFilter*, ModelFilter*>*>(((*yyvaluep).filterPair)); }
#line 1100 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_lastrun: /* lastrun  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1106 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_created: /* created  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1112 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_rating: /* rating  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1118 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_distance: /* distance  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1124 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_elevation: /* elevation  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1130 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_grade: /* grade  */
#line 48 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting " << static_cast<ModelFilter*>(((*yyvaluep).filter))->toString(); delete static_cast<ModelFilter*>(((*yyvaluep).filter)); }
#line 1136 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_words: /* words  */
#line 49 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting wordList"; delete static_cast<QStringList*>(((*yyvaluep).wordList)); }
#line 1142 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
        break;

    case YYSYMBOL_word: /* word  */
#line 49 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
            { qDebug() << "Deleting wordList"; delete static_cast<QStringList*>(((*yyvaluep).wordList)); }
#line 1148 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
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
  case 4: /* statement: dominantzone  */
#line 80 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1418 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 5: /* statement: zone  */
#line 81 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1424 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 6: /* statement: duration  */
#line 82 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1430 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 7: /* statement: intensity  */
#line 83 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1436 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 8: /* statement: vi  */
#line 84 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1442 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 9: /* statement: xpower  */
#line 85 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1448 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 10: /* statement: ri  */
#line 86 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1454 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 11: /* statement: bikescore  */
#line 87 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1460 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 12: /* statement: svi  */
#line 88 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1466 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 13: /* statement: stress  */
#line 89 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1472 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 14: /* statement: rating  */
#line 90 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1478 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 15: /* statement: minpower  */
#line 91 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1484 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 16: /* statement: maxpower  */
#line 92 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1490 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 17: /* statement: avgpower  */
#line 93 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1496 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 18: /* statement: isopower  */
#line 94 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1502 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 19: /* statement: power  */
#line 95 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { auto pair = static_cast<std::pair<ModelFilter*, ModelFilter*>*>((yyvsp[0].filterPair)); workoutModelFilters << pair->first << pair->second; }
#line 1508 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 20: /* statement: lastrun  */
#line 96 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1514 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 21: /* statement: created  */
#line 97 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1520 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 22: /* statement: distance  */
#line 98 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1526 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 23: /* statement: elevation  */
#line 99 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1532 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 24: /* statement: grade  */
#line 100 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << static_cast<ModelFilter*>((yyvsp[0].filter)); }
#line 1538 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 25: /* statement: words  */
#line 101 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                          { workoutModelFilters << new ModelStringContainsFilter(TdbWorkoutModelIdx::fulltext, *static_cast<QStringList*>((yyvsp[0].wordList))); }
#line 1544 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 26: /* dominantzone: DOMINANTZONE ZONE  */
#line 104 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                  { (yyval.filter) = new ModelNumberEqualFilter(TdbWorkoutModelIdx::dominantZone, (yyvsp[0].numValue)); }
#line 1550 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 27: /* dominantzone: DOMINANTZONE ZONE RANGESYMBOL ZONE  */
#line 105 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                  { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::dominantZone, (yyvsp[-2].numValue), (yyvsp[0].numValue)); }
#line 1556 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 28: /* dominantzone: DOMINANTZONE RANGESYMBOL ZONE  */
#line 106 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                  { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::dominantZone, 1, (yyvsp[0].numValue)); }
#line 1562 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 29: /* dominantzone: DOMINANTZONE ZONE RANGESYMBOL  */
#line 107 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                  { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::dominantZone, (yyvsp[-1].numValue), 10); }
#line 1568 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 30: /* zone: ZONE minuteValue  */
#line 110 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                { (yyval.filter) = new ModelNumberEqualFilter(workoutModelZoneIndex((yyvsp[-1].numValue), ZoneContentType::seconds), (yyvsp[0].numValue) * 60); }
#line 1574 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 31: /* zone: ZONE minuteValue RANGESYMBOL minuteValue  */
#line 111 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                { (yyval.filter) = new ModelNumberRangeFilter(workoutModelZoneIndex((yyvsp[-3].numValue), ZoneContentType::seconds), (yyvsp[-2].numValue) * 60, (yyvsp[0].numValue) * 60); }
#line 1580 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 32: /* zone: ZONE RANGESYMBOL minuteValue  */
#line 112 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                { (yyval.filter) = new ModelNumberRangeFilter(workoutModelZoneIndex((yyvsp[-2].numValue), ZoneContentType::seconds), 0, (yyvsp[0].numValue) * 60); }
#line 1586 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 33: /* zone: ZONE minuteValue RANGESYMBOL  */
#line 113 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                { (yyval.filter) = new ModelNumberRangeFilter(workoutModelZoneIndex((yyvsp[-2].numValue), ZoneContentType::seconds), (yyvsp[-1].numValue) * 60); }
#line 1592 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 34: /* zone: ZONE PERCENT  */
#line 114 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                { (yyval.filter) = new ModelNumberEqualFilter(workoutModelZoneIndex((yyvsp[-1].numValue), ZoneContentType::percent), (yyvsp[0].numValue)); }
#line 1598 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 35: /* zone: ZONE PERCENT RANGESYMBOL PERCENT  */
#line 115 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                { (yyval.filter) = new ModelNumberRangeFilter(workoutModelZoneIndex((yyvsp[-3].numValue), ZoneContentType::percent), (yyvsp[-2].numValue), (yyvsp[0].numValue)); }
#line 1604 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 36: /* zone: ZONE RANGESYMBOL PERCENT  */
#line 116 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                { (yyval.filter) = new ModelNumberRangeFilter(workoutModelZoneIndex((yyvsp[-2].numValue), ZoneContentType::percent), 0, (yyvsp[0].numValue)); }
#line 1610 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 37: /* zone: ZONE PERCENT RANGESYMBOL  */
#line 117 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                { (yyval.filter) = new ModelNumberRangeFilter(workoutModelZoneIndex((yyvsp[-2].numValue), ZoneContentType::percent), (yyvsp[-1].numValue)); }
#line 1616 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 38: /* duration: DURATION minuteValue  */
#line 120 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                        { (yyval.filter) = new ModelNumberEqualFilter(TdbWorkoutModelIdx::duration, (yyvsp[0].numValue) * 60000, 5 * 60000); }
#line 1622 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 39: /* duration: DURATION minuteValue RANGESYMBOL minuteValue  */
#line 121 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                        { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::duration, (yyvsp[-2].numValue) * 60000, (yyvsp[0].numValue) * 60000); }
#line 1628 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 40: /* duration: DURATION RANGESYMBOL minuteValue  */
#line 122 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                        { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::duration, 0, (yyvsp[0].numValue) * 60000); }
#line 1634 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 41: /* duration: DURATION minuteValue RANGESYMBOL  */
#line 123 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                        { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::duration, (yyvsp[-1].numValue) * 60000); }
#line 1640 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 42: /* intensity: INTENSITY mixedNumValue  */
#line 126 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                                { (yyval.filter) = new ModelFloatEqualFilter(TdbWorkoutModelIdx::intensity, (yyvsp[0].floatValue), 0.05); }
#line 1646 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 43: /* intensity: INTENSITY mixedNumValue RANGESYMBOL mixedNumValue  */
#line 127 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                                { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::intensity, (yyvsp[-2].floatValue), (yyvsp[0].floatValue)); }
#line 1652 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 44: /* intensity: INTENSITY RANGESYMBOL mixedNumValue  */
#line 128 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                                { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::intensity, 0, (yyvsp[0].floatValue)); }
#line 1658 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 45: /* intensity: INTENSITY mixedNumValue RANGESYMBOL  */
#line 129 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                                { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::intensity, (yyvsp[-1].floatValue)); }
#line 1664 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 46: /* vi: VI mixedNumValue  */
#line 132 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                    { (yyval.filter) = new ModelFloatEqualFilter(TdbWorkoutModelIdx::vi, (yyvsp[0].floatValue), 0.05); }
#line 1670 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 47: /* vi: VI mixedNumValue RANGESYMBOL mixedNumValue  */
#line 133 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                    { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::vi, (yyvsp[-2].floatValue), (yyvsp[0].floatValue)); }
#line 1676 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 48: /* vi: VI RANGESYMBOL mixedNumValue  */
#line 134 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                    { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::vi, 0, (yyvsp[0].floatValue)); }
#line 1682 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 49: /* vi: VI mixedNumValue RANGESYMBOL  */
#line 135 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                    { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::vi, (yyvsp[-1].floatValue)); }
#line 1688 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 50: /* xpower: XPOWER mixedNumValue  */
#line 138 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                          { (yyval.filter) = new ModelFloatEqualFilter(TdbWorkoutModelIdx::xp, (yyvsp[0].floatValue), 0.05); }
#line 1694 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 51: /* xpower: XPOWER mixedNumValue RANGESYMBOL mixedNumValue  */
#line 139 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                          { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::xp, (yyvsp[-2].floatValue), (yyvsp[0].floatValue)); }
#line 1700 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 52: /* xpower: XPOWER RANGESYMBOL mixedNumValue  */
#line 140 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                          { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::xp, 0, (yyvsp[0].floatValue)); }
#line 1706 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 53: /* xpower: XPOWER mixedNumValue RANGESYMBOL  */
#line 141 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                          { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::xp, (yyvsp[-1].floatValue)); }
#line 1712 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 54: /* ri: RI mixedNumValue  */
#line 144 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                    { (yyval.filter) = new ModelFloatEqualFilter(TdbWorkoutModelIdx::ri, (yyvsp[0].floatValue), 0.05); }
#line 1718 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 55: /* ri: RI mixedNumValue RANGESYMBOL mixedNumValue  */
#line 145 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                    { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::ri, (yyvsp[-2].floatValue), (yyvsp[0].floatValue)); }
#line 1724 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 56: /* ri: RI RANGESYMBOL mixedNumValue  */
#line 146 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                    { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::ri, 0, (yyvsp[0].floatValue)); }
#line 1730 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 57: /* ri: RI mixedNumValue RANGESYMBOL  */
#line 147 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                    { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::ri, (yyvsp[-1].floatValue)); }
#line 1736 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 58: /* bikescore: BIKESCORE mixedNumValue  */
#line 150 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                                { (yyval.filter) = new ModelFloatEqualFilter(TdbWorkoutModelIdx::bs, (yyvsp[0].floatValue), 0.05); }
#line 1742 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 59: /* bikescore: BIKESCORE mixedNumValue RANGESYMBOL mixedNumValue  */
#line 151 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                                { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::bs, (yyvsp[-2].floatValue), (yyvsp[0].floatValue)); }
#line 1748 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 60: /* bikescore: BIKESCORE RANGESYMBOL mixedNumValue  */
#line 152 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                                { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::bs, 0, (yyvsp[0].floatValue)); }
#line 1754 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 61: /* bikescore: BIKESCORE mixedNumValue RANGESYMBOL  */
#line 153 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                                { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::bs, (yyvsp[-1].floatValue)); }
#line 1760 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 62: /* svi: SVI mixedNumValue  */
#line 156 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                     { (yyval.filter) = new ModelFloatEqualFilter(TdbWorkoutModelIdx::svi, (yyvsp[0].floatValue), 0.05); }
#line 1766 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 63: /* svi: SVI mixedNumValue RANGESYMBOL mixedNumValue  */
#line 157 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                     { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::svi, (yyvsp[-2].floatValue), (yyvsp[0].floatValue)); }
#line 1772 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 64: /* svi: SVI RANGESYMBOL mixedNumValue  */
#line 158 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                     { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::svi, 0, (yyvsp[0].floatValue)); }
#line 1778 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 65: /* svi: SVI mixedNumValue RANGESYMBOL  */
#line 159 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                     { (yyval.filter) = new ModelFloatRangeFilter(TdbWorkoutModelIdx::svi, (yyvsp[-1].floatValue)); }
#line 1784 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 66: /* stress: STRESS NUMBER  */
#line 162 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                          { (yyval.filter) = new ModelNumberEqualFilter(TdbWorkoutModelIdx::bikestress, (yyvsp[0].numValue)); }
#line 1790 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 67: /* stress: STRESS NUMBER RANGESYMBOL NUMBER  */
#line 163 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                          { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::bikestress, (yyvsp[-2].numValue), (yyvsp[0].numValue)); }
#line 1796 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 68: /* stress: STRESS RANGESYMBOL NUMBER  */
#line 164 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                          { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::bikestress, 0, (yyvsp[0].numValue)); }
#line 1802 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 69: /* stress: STRESS NUMBER RANGESYMBOL  */
#line 165 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                          { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::bikestress, (yyvsp[-1].numValue)); }
#line 1808 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 70: /* minpower: MINPOWER NUMBER  */
#line 168 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberEqualFilter(TdbWorkoutModelIdx::minPower, (yyvsp[0].numValue)); }
#line 1814 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 71: /* minpower: MINPOWER NUMBER RANGESYMBOL NUMBER  */
#line 169 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::minPower, (yyvsp[-2].numValue), (yyvsp[0].numValue)); }
#line 1820 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 72: /* minpower: MINPOWER RANGESYMBOL NUMBER  */
#line 170 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::minPower, 0, (yyvsp[0].numValue)); }
#line 1826 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 73: /* minpower: MINPOWER NUMBER RANGESYMBOL  */
#line 171 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::minPower, (yyvsp[-1].numValue)); }
#line 1832 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 74: /* maxpower: MAXPOWER NUMBER  */
#line 174 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberEqualFilter(TdbWorkoutModelIdx::maxPower, (yyvsp[0].numValue)); }
#line 1838 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 75: /* maxpower: MAXPOWER NUMBER RANGESYMBOL NUMBER  */
#line 175 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::maxPower, (yyvsp[-2].numValue), (yyvsp[0].numValue)); }
#line 1844 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 76: /* maxpower: MAXPOWER RANGESYMBOL NUMBER  */
#line 176 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::maxPower, 0, (yyvsp[0].numValue)); }
#line 1850 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 77: /* maxpower: MAXPOWER NUMBER RANGESYMBOL  */
#line 177 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::maxPower, (yyvsp[-1].numValue)); }
#line 1856 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 78: /* avgpower: AVGPOWER NUMBER  */
#line 180 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberEqualFilter(TdbWorkoutModelIdx::avgPower, (yyvsp[0].numValue), 5); }
#line 1862 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 79: /* avgpower: AVGPOWER NUMBER RANGESYMBOL NUMBER  */
#line 181 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::avgPower, (yyvsp[-2].numValue), (yyvsp[0].numValue)); }
#line 1868 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 80: /* avgpower: AVGPOWER RANGESYMBOL NUMBER  */
#line 182 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::avgPower, 0, (yyvsp[0].numValue)); }
#line 1874 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 81: /* avgpower: AVGPOWER NUMBER RANGESYMBOL  */
#line 183 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::avgPower, (yyvsp[-1].numValue)); }
#line 1880 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 82: /* isopower: ISOPOWER NUMBER  */
#line 186 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberEqualFilter(TdbWorkoutModelIdx::isoPower, (yyvsp[0].numValue), 5); }
#line 1886 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 83: /* isopower: ISOPOWER NUMBER RANGESYMBOL NUMBER  */
#line 187 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::isoPower, (yyvsp[-2].numValue), (yyvsp[0].numValue)); }
#line 1892 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 84: /* isopower: ISOPOWER RANGESYMBOL NUMBER  */
#line 188 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::isoPower, 0, (yyvsp[0].numValue)); }
#line 1898 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 85: /* isopower: ISOPOWER NUMBER RANGESYMBOL  */
#line 189 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::isoPower, (yyvsp[-1].numValue)); }
#line 1904 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 86: /* power: POWER NUMBER RANGESYMBOL NUMBER  */
#line 192 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                        { (yyval.filterPair) = new std::pair<ModelFilter*, ModelFilter*>(new ModelNumberRangeFilter(TdbWorkoutModelIdx::minPower, (yyvsp[-2].numValue), (yyvsp[0].numValue)),
                                                                                         new ModelNumberRangeFilter(TdbWorkoutModelIdx::maxPower, (yyvsp[-2].numValue), (yyvsp[0].numValue))); }
#line 1911 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 87: /* power: POWER RANGESYMBOL NUMBER  */
#line 194 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                        { (yyval.filterPair) = new std::pair<ModelFilter*, ModelFilter*>(new ModelNumberRangeFilter(TdbWorkoutModelIdx::minPower, 0, (yyvsp[0].numValue)),
                                                                                         new ModelNumberRangeFilter(TdbWorkoutModelIdx::maxPower, 0, (yyvsp[0].numValue))); }
#line 1918 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 88: /* power: POWER NUMBER RANGESYMBOL  */
#line 196 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                        { (yyval.filterPair) = new std::pair<ModelFilter*, ModelFilter*>(new ModelNumberRangeFilter(TdbWorkoutModelIdx::minPower, (yyvsp[-1].numValue)),
                                                                                         new ModelNumberRangeFilter(TdbWorkoutModelIdx::maxPower, (yyvsp[-1].numValue))); }
#line 1925 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 89: /* lastrun: LASTRUN DAYS RANGESYMBOL DAYS  */
#line 199 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                        { long now = QDateTime::currentDateTime().toSecsSinceEpoch();
                                          (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::lastRun, now - (yyvsp[0].numValue) * 86400, now - (yyvsp[-2].numValue) * 86400); }
#line 1932 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 90: /* lastrun: LASTRUN RANGESYMBOL DAYS  */
#line 201 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                        { long now = QDateTime::currentDateTime().toSecsSinceEpoch();
                                          (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::lastRun, now - (yyvsp[0].numValue) * 86400, now); }
#line 1939 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 91: /* lastrun: LASTRUN DAYS RANGESYMBOL  */
#line 203 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                        { long now = QDateTime::currentDateTime().toSecsSinceEpoch();
                                          (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::lastRun, 0, now - (yyvsp[-1].numValue) * 86400); }
#line 1946 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 92: /* created: CREATED DAYS RANGESYMBOL DAYS  */
#line 207 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                        { long now = QDateTime::currentDateTime().toSecsSinceEpoch();
                                          (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::created, now - (yyvsp[0].numValue) * 86400, now - (yyvsp[-2].numValue) * 86400); }
#line 1953 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 93: /* created: CREATED RANGESYMBOL DAYS  */
#line 209 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                        { long now = QDateTime::currentDateTime().toSecsSinceEpoch();
                                          (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::created, now - (yyvsp[0].numValue) * 86400, now); }
#line 1960 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 94: /* created: CREATED DAYS RANGESYMBOL  */
#line 211 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                        { long now = QDateTime::currentDateTime().toSecsSinceEpoch();
                                          (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::created, 0, now - (yyvsp[-1].numValue) * 86400); }
#line 1967 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 95: /* rating: RATING NUMBER  */
#line 215 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                          { (yyval.filter) = new ModelNumberEqualFilter(TdbWorkoutModelIdx::rating, (yyvsp[0].numValue)); }
#line 1973 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 96: /* rating: RATING NUMBER RANGESYMBOL NUMBER  */
#line 216 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                          { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::rating, (yyvsp[-2].numValue), (yyvsp[0].numValue)); }
#line 1979 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 97: /* rating: RATING RANGESYMBOL NUMBER  */
#line 217 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                          { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::rating, 1, (yyvsp[0].numValue)); }
#line 1985 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 98: /* rating: RATING NUMBER RANGESYMBOL  */
#line 218 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                          { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::rating, (yyvsp[-1].numValue), 5); }
#line 1991 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 99: /* distance: DISTANCE NUMBER  */
#line 221 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberEqualFilter(TdbWorkoutModelIdx::distance, (yyvsp[0].numValue) * 1000, 5000); }
#line 1997 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 100: /* distance: DISTANCE NUMBER RANGESYMBOL NUMBER  */
#line 222 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::distance, (yyvsp[-2].numValue) * 1000, (yyvsp[0].numValue) * 1000); }
#line 2003 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 101: /* distance: DISTANCE RANGESYMBOL NUMBER  */
#line 223 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::distance, 0, (yyvsp[0].numValue) * 1000); }
#line 2009 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 102: /* distance: DISTANCE NUMBER RANGESYMBOL  */
#line 224 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                              { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::distance, (yyvsp[-1].numValue) * 1000); }
#line 2015 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 103: /* elevation: ELEVATION NUMBER  */
#line 227 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                { (yyval.filter) = new ModelNumberEqualFilter(TdbWorkoutModelIdx::elevation, (yyvsp[0].numValue), 5); }
#line 2021 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 104: /* elevation: ELEVATION NUMBER RANGESYMBOL NUMBER  */
#line 228 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::elevation, (yyvsp[-2].numValue), (yyvsp[0].numValue)); }
#line 2027 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 105: /* elevation: ELEVATION RANGESYMBOL NUMBER  */
#line 229 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::elevation, 0, (yyvsp[0].numValue)); }
#line 2033 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 106: /* elevation: ELEVATION NUMBER RANGESYMBOL  */
#line 230 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                                { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::elevation, (yyvsp[-1].numValue)); }
#line 2039 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 107: /* grade: GRADE PERCENT  */
#line 233 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                          { (yyval.filter) = new ModelNumberEqualFilter(TdbWorkoutModelIdx::avgGrade, (yyvsp[0].numValue), 5); }
#line 2045 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 108: /* grade: GRADE PERCENT RANGESYMBOL PERCENT  */
#line 234 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                          { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::avgGrade, (yyvsp[-2].numValue), (yyvsp[0].numValue)); }
#line 2051 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 109: /* grade: GRADE RANGESYMBOL PERCENT  */
#line 235 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                          { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::avgGrade, 0, (yyvsp[0].numValue)); }
#line 2057 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 110: /* grade: GRADE PERCENT RANGESYMBOL  */
#line 236 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                                          { (yyval.filter) = new ModelNumberRangeFilter(TdbWorkoutModelIdx::avgGrade, (yyvsp[-1].numValue)); }
#line 2063 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 111: /* words: word words  */
#line 239 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                   { auto w1 = static_cast<QStringList*>((yyvsp[-1].wordList)); auto w2 = static_cast<QStringList*>((yyvsp[0].wordList)); *w1 << *w2; delete w2; (yyval.wordList) = w1; }
#line 2069 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 112: /* words: word  */
#line 240 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                   { (yyval.wordList) = (yyvsp[0].wordList); }
#line 2075 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 113: /* word: WORD  */
#line 243 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
             { (yyval.wordList) = new QStringList((yyvsp[0].word)); }
#line 2081 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 114: /* word: NUMBER  */
#line 244 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
             { (yyval.wordList) = new QStringList(QString::number((yyvsp[0].numValue))); }
#line 2087 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 115: /* mixedNumValue: FLOAT  */
#line 247 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                       { (yyval.floatValue) = (yyvsp[0].floatValue); }
#line 2093 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 116: /* mixedNumValue: NUMBER  */
#line 248 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                       { (yyval.floatValue) = (yyvsp[0].numValue); }
#line 2099 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 117: /* minuteValue: NUMBER  */
#line 251 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                     { (yyval.numValue) = (yyvsp[0].numValue); }
#line 2105 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;

  case 118: /* minuteValue: TIME  */
#line 252 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"
                     { (yyval.numValue) = (yyvsp[0].numValue); }
#line 2111 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"
    break;


#line 2115 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.cpp"

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
      yyerror (YY_("syntax error"));
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
                      yytoken, &yylval);
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
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
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 255 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"


void yyerror(const char *s)
{
    workoutModelErrorMsg = QString(s);
}
