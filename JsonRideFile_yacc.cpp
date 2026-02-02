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
#define yyparse         JsonRideFileparse
#define yylex           JsonRideFilelex
#define yyerror         JsonRideFileerror
#define yydebug         JsonRideFiledebug
#define yynerrs         JsonRideFilenerrs

/* First part of user prologue.  */
#line 1 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"

/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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
// know to work well with bison v2.4.1.
//
// To make the grammar readable I have placed the code
// for each nterm at column 40, this source file is best
// edited / viewed in an editor which is at least 120
// columns wide (e.g. vi in xterm of 120x40)
//
//
// The grammar is specific to the RideFile format serialised
// in writeRideFile below, this is NOT a generic json parser.

#include "JsonRideFile.h"
#include "RideMetadata.h"

// now we have a reentrant parser we save context data
// in a structure rather than in global variables -- so
// you can run the parser concurrently.
struct JsonContext {

    // the scanner
    void *scanner;

    // Set during parser processing, using same
    // naming conventions as yacc/lex -p
    RideFile *JsonRide;

    // term state data is held in these variables
    RideFilePoint JsonPoint;
    RideFileInterval JsonInterval;
    RideFileCalibration JsonCalibration;
    QString JsonString,
                JsonTagKey, JsonTagValue,
                JsonOverName, JsonOverKey, JsonOverValue;
    double JsonNumber;
    QStringList JsonRideFileerrors;
    QMap <QString, QString> JsonOverrides;

    XDataSeries xdataseries;
    XDataPoint xdatapoint;
    QStringList stringlist;
    QVector<double> numberlist;

};

#define YYSTYPE QString

// Lex scanner
extern int JsonRideFilelex(YYSTYPE*,void*); // the lexer aka yylex()
extern int JsonRideFilelex_init(void**);
extern void JsonRideFile_setString(QString, void *);
extern int JsonRideFilelex_destroy(void*); // the cleaner for lexer

// yacc parser
void JsonRideFileerror(void*jc, const char *error) // used by parser aka yyerror()
{ static_cast<JsonContext*>(jc)->JsonRideFileerrors << error; }

//
// Utility functions
//

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

// extract scanner from the context
#define scanner jc->scanner


#line 185 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"

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

#include "JsonRideFile_yacc.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_JS_STRING = 3,                  /* JS_STRING  */
  YYSYMBOL_JS_INTEGER = 4,                 /* JS_INTEGER  */
  YYSYMBOL_JS_FLOAT = 5,                   /* JS_FLOAT  */
  YYSYMBOL_RIDE = 6,                       /* RIDE  */
  YYSYMBOL_STARTTIME = 7,                  /* STARTTIME  */
  YYSYMBOL_RECINTSECS = 8,                 /* RECINTSECS  */
  YYSYMBOL_DEVICETYPE = 9,                 /* DEVICETYPE  */
  YYSYMBOL_IDENTIFIER = 10,                /* IDENTIFIER  */
  YYSYMBOL_OVERRIDES = 11,                 /* OVERRIDES  */
  YYSYMBOL_TAGS = 12,                      /* TAGS  */
  YYSYMBOL_INTERVALS = 13,                 /* INTERVALS  */
  YYSYMBOL_NAME = 14,                      /* NAME  */
  YYSYMBOL_START = 15,                     /* START  */
  YYSYMBOL_STOP = 16,                      /* STOP  */
  YYSYMBOL_COLOR = 17,                     /* COLOR  */
  YYSYMBOL_TEST = 18,                      /* TEST  */
  YYSYMBOL_CALIBRATIONS = 19,              /* CALIBRATIONS  */
  YYSYMBOL_VALUE = 20,                     /* VALUE  */
  YYSYMBOL_VALUES = 21,                    /* VALUES  */
  YYSYMBOL_UNIT = 22,                      /* UNIT  */
  YYSYMBOL_UNITS = 23,                     /* UNITS  */
  YYSYMBOL_REFERENCES = 24,                /* REFERENCES  */
  YYSYMBOL_XDATA = 25,                     /* XDATA  */
  YYSYMBOL_SAMPLES = 26,                   /* SAMPLES  */
  YYSYMBOL_SECS = 27,                      /* SECS  */
  YYSYMBOL_KM = 28,                        /* KM  */
  YYSYMBOL_WATTS = 29,                     /* WATTS  */
  YYSYMBOL_NM = 30,                        /* NM  */
  YYSYMBOL_CAD = 31,                       /* CAD  */
  YYSYMBOL_KPH = 32,                       /* KPH  */
  YYSYMBOL_HR = 33,                        /* HR  */
  YYSYMBOL_ALTITUDE = 34,                  /* ALTITUDE  */
  YYSYMBOL_LAT = 35,                       /* LAT  */
  YYSYMBOL_LON = 36,                       /* LON  */
  YYSYMBOL_HEADWIND = 37,                  /* HEADWIND  */
  YYSYMBOL_SLOPE = 38,                     /* SLOPE  */
  YYSYMBOL_TEMP = 39,                      /* TEMP  */
  YYSYMBOL_LRBALANCE = 40,                 /* LRBALANCE  */
  YYSYMBOL_LTE = 41,                       /* LTE  */
  YYSYMBOL_RTE = 42,                       /* RTE  */
  YYSYMBOL_LPS = 43,                       /* LPS  */
  YYSYMBOL_RPS = 44,                       /* RPS  */
  YYSYMBOL_THB = 45,                       /* THB  */
  YYSYMBOL_SMO2 = 46,                      /* SMO2  */
  YYSYMBOL_RVERT = 47,                     /* RVERT  */
  YYSYMBOL_RCAD = 48,                      /* RCAD  */
  YYSYMBOL_RCON = 49,                      /* RCON  */
  YYSYMBOL_LPCO = 50,                      /* LPCO  */
  YYSYMBOL_RPCO = 51,                      /* RPCO  */
  YYSYMBOL_LPPB = 52,                      /* LPPB  */
  YYSYMBOL_RPPB = 53,                      /* RPPB  */
  YYSYMBOL_LPPE = 54,                      /* LPPE  */
  YYSYMBOL_RPPE = 55,                      /* RPPE  */
  YYSYMBOL_LPPPB = 56,                     /* LPPPB  */
  YYSYMBOL_RPPPB = 57,                     /* RPPPB  */
  YYSYMBOL_LPPPE = 58,                     /* LPPPE  */
  YYSYMBOL_RPPPE = 59,                     /* RPPPE  */
  YYSYMBOL_60_ = 60,                       /* '{'  */
  YYSYMBOL_61_ = 61,                       /* '}'  */
  YYSYMBOL_62_ = 62,                       /* ','  */
  YYSYMBOL_63_ = 63,                       /* ':'  */
  YYSYMBOL_64_ = 64,                       /* '['  */
  YYSYMBOL_65_ = 65,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 66,                  /* $accept  */
  YYSYMBOL_document = 67,                  /* document  */
  YYSYMBOL_ride_list = 68,                 /* ride_list  */
  YYSYMBOL_ride = 69,                      /* ride  */
  YYSYMBOL_rideelement_list = 70,          /* rideelement_list  */
  YYSYMBOL_rideelement = 71,               /* rideelement  */
  YYSYMBOL_starttime = 72,                 /* starttime  */
  YYSYMBOL_recordint = 73,                 /* recordint  */
  YYSYMBOL_devicetype = 74,                /* devicetype  */
  YYSYMBOL_identifier = 75,                /* identifier  */
  YYSYMBOL_overrides = 76,                 /* overrides  */
  YYSYMBOL_overrides_list = 77,            /* overrides_list  */
  YYSYMBOL_override = 78,                  /* override  */
  YYSYMBOL_override_name = 79,             /* override_name  */
  YYSYMBOL_override_values = 80,           /* override_values  */
  YYSYMBOL_override_value_list = 81,       /* override_value_list  */
  YYSYMBOL_override_value = 82,            /* override_value  */
  YYSYMBOL_override_key = 83,              /* override_key  */
  YYSYMBOL_tags = 84,                      /* tags  */
  YYSYMBOL_tags_list = 85,                 /* tags_list  */
  YYSYMBOL_tag = 86,                       /* tag  */
  YYSYMBOL_tag_key = 87,                   /* tag_key  */
  YYSYMBOL_tag_value = 88,                 /* tag_value  */
  YYSYMBOL_intervals = 89,                 /* intervals  */
  YYSYMBOL_interval_list = 90,             /* interval_list  */
  YYSYMBOL_interval_test = 91,             /* interval_test  */
  YYSYMBOL_interval_color = 92,            /* interval_color  */
  YYSYMBOL_interval = 93,                  /* interval  */
  YYSYMBOL_94_1 = 94,                      /* $@1  */
  YYSYMBOL_95_2 = 95,                      /* $@2  */
  YYSYMBOL_96_3 = 96,                      /* $@3  */
  YYSYMBOL_calibrations = 97,              /* calibrations  */
  YYSYMBOL_calibration_list = 98,          /* calibration_list  */
  YYSYMBOL_calibration = 99,               /* calibration  */
  YYSYMBOL_100_4 = 100,                    /* $@4  */
  YYSYMBOL_101_5 = 101,                    /* $@5  */
  YYSYMBOL_102_6 = 102,                    /* $@6  */
  YYSYMBOL_references = 103,               /* references  */
  YYSYMBOL_reference_list = 104,           /* reference_list  */
  YYSYMBOL_reference = 105,                /* reference  */
  YYSYMBOL_xdata = 106,                    /* xdata  */
  YYSYMBOL_xdata_list = 107,               /* xdata_list  */
  YYSYMBOL_xdata_series = 108,             /* xdata_series  */
  YYSYMBOL_xdata_items = 109,              /* xdata_items  */
  YYSYMBOL_xdata_item = 110,               /* xdata_item  */
  YYSYMBOL_xdata_samples = 111,            /* xdata_samples  */
  YYSYMBOL_xdata_sample = 112,             /* xdata_sample  */
  YYSYMBOL_xdata_value_list = 113,         /* xdata_value_list  */
  YYSYMBOL_xdata_value = 114,              /* xdata_value  */
  YYSYMBOL_samples = 115,                  /* samples  */
  YYSYMBOL_sample_list = 116,              /* sample_list  */
  YYSYMBOL_sample = 117,                   /* sample  */
  YYSYMBOL_series_list = 118,              /* series_list  */
  YYSYMBOL_series = 119,                   /* series  */
  YYSYMBOL_number = 120,                   /* number  */
  YYSYMBOL_string = 121,                   /* string  */
  YYSYMBOL_string_list = 122,              /* string_list  */
  YYSYMBOL_number_list = 123               /* number_list  */
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
typedef yytype_int16 yy_state_t;

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
#define YYLAST   303

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  66
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  58
/* YYNRULES -- Number of rules.  */
#define YYNRULES  133
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  338

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   314


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
       2,     2,     2,     2,    62,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    63,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    64,     2,    65,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    60,     2,    61,     2,     2,     2,     2,
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
      55,    56,    57,    58,    59
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   129,   129,   130,   134,   135,   138,   139,   140,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
     159,   164,   165,   166,   171,   172,   172,   174,   179,   182,
     183,   183,   184,   185,   186,   191,   192,   192,   193,   196,
     199,   204,   205,   205,   206,   207,   210,   211,   214,   215,
     216,   214,   232,   233,   233,   234,   235,   236,   234,   248,
     252,   252,   253,   260,   261,   262,   265,   273,   274,   277,
     278,   279,   280,   282,   284,   287,   288,   290,   295,   295,
     297,   298,   299,   300,   303,   304,   310,   311,   311,   312,
     332,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   374,   375,   378,
     381,   382,   385,   386
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
  "\"end of file\"", "error", "\"invalid token\"", "JS_STRING",
  "JS_INTEGER", "JS_FLOAT", "RIDE", "STARTTIME", "RECINTSECS",
  "DEVICETYPE", "IDENTIFIER", "OVERRIDES", "TAGS", "INTERVALS", "NAME",
  "START", "STOP", "COLOR", "TEST", "CALIBRATIONS", "VALUE", "VALUES",
  "UNIT", "UNITS", "REFERENCES", "XDATA", "SAMPLES", "SECS", "KM", "WATTS",
  "NM", "CAD", "KPH", "HR", "ALTITUDE", "LAT", "LON", "HEADWIND", "SLOPE",
  "TEMP", "LRBALANCE", "LTE", "RTE", "LPS", "RPS", "THB", "SMO2", "RVERT",
  "RCAD", "RCON", "LPCO", "RPCO", "LPPB", "RPPB", "LPPE", "RPPE", "LPPPB",
  "RPPPB", "LPPPE", "RPPPE", "'{'", "'}'", "','", "':'", "'['", "']'",
  "$accept", "document", "ride_list", "ride", "rideelement_list",
  "rideelement", "starttime", "recordint", "devicetype", "identifier",
  "overrides", "overrides_list", "override", "override_name",
  "override_values", "override_value_list", "override_value",
  "override_key", "tags", "tags_list", "tag", "tag_key", "tag_value",
  "intervals", "interval_list", "interval_test", "interval_color",
  "interval", "$@1", "$@2", "$@3", "calibrations", "calibration_list",
  "calibration", "$@4", "$@5", "$@6", "references", "reference_list",
  "reference", "xdata", "xdata_list", "xdata_series", "xdata_items",
  "xdata_item", "xdata_samples", "xdata_sample", "xdata_value_list",
  "xdata_value", "samples", "sample_list", "sample", "series_list",
  "series", "number", "string", "string_list", "number_list", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-155)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-34)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      30,   -59,    10,     6,   -42,  -155,   -33,   -28,  -155,    10,
       0,  -155,  -155,    29,    38,    54,    55,    57,    59,    95,
     100,   101,   102,   103,    37,  -155,  -155,  -155,  -155,  -155,
    -155,  -155,  -155,  -155,  -155,  -155,  -155,    92,    98,    92,
      92,    46,   107,   104,   105,   106,   108,   109,  -155,     0,
    -155,  -155,  -155,  -155,  -155,  -155,  -155,   114,    92,   115,
     116,   117,   118,   119,  -155,    92,   -48,  -155,    43,  -155,
     120,  -155,   147,   -47,  -155,   166,   -30,  -155,    97,   -21,
    -155,    17,   -20,  -155,    97,   -16,  -155,   121,  -155,   114,
    -155,  -155,    92,    92,   122,   115,  -155,   123,   116,  -155,
     125,   128,   129,   131,   134,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   148,   150,   152,   153,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   167,
     168,   171,   172,   176,   175,   117,  -155,   177,   178,   179,
     180,   181,   182,    45,  -155,   118,  -155,    47,  -155,   119,
    -155,   169,  -155,  -155,  -155,  -155,    92,  -155,    92,  -155,
      98,    98,    98,    98,    98,    98,    98,    98,    98,    98,
      98,    98,    98,    98,    98,    98,    98,    98,    98,    98,
      98,    98,    98,    98,    98,    98,    98,    98,    98,    98,
      98,    98,    98,  -155,    49,  -155,    92,    92,   183,    92,
     184,   185,  -155,    17,  -155,  -155,    97,  -155,    92,   189,
     190,   191,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,
    -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,
    -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,
    -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,    92,
    -155,    92,   186,  -155,  -155,    50,  -155,   188,   192,  -155,
    -155,  -155,  -155,   -15,   -14,     2,    31,  -155,  -155,    92,
      92,   224,   239,    92,  -155,  -155,   193,   194,   197,   198,
      52,  -155,   199,   186,  -155,  -155,  -155,   200,   201,  -155,
      98,   202,    98,    98,  -155,     2,    49,  -155,    98,    98,
    -155,    98,  -155,  -155,  -155,  -155,  -155,   203,   205,  -155,
      32,  -155,  -155,    98,  -155,   165,   248,  -155,   206,   207,
      98,    98,  -155,  -155,   209,   211,   256,   212,  -155,   213,
     257,   216,    92,   215,  -155,  -155,    92,  -155
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     3,     4,     0,     0,     1,     0,
       0,     2,     5,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    19,    18,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     6,     0,
     129,    20,   127,   128,    21,    22,    23,     0,     0,     0,
       0,     0,     0,     0,     7,     0,     0,    25,     0,    36,
       0,    39,     0,     0,    42,     0,     0,    53,     0,     0,
      60,     0,     0,    64,     0,     0,    87,     0,    28,     0,
      24,    35,     0,     0,     0,     0,    41,     0,     0,    52,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    59,     0,     0,     0,
       0,     0,     0,     0,    67,     0,    63,     0,    90,     0,
      86,     0,    26,    37,    38,    40,     0,    43,     0,    54,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    62,     0,    61,     0,     0,     0,     0,
       0,     0,    66,     0,    65,    89,     0,    88,     0,     0,
       0,     0,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     121,   120,   122,   123,   124,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   125,   126,    69,    70,     0,
      71,     0,     0,    68,    91,     0,    30,     0,    34,    27,
      48,    55,   130,     0,     0,     0,     0,    75,    29,     0,
       0,     0,     0,     0,    72,    73,     0,     0,     0,     0,
       0,    78,     0,     0,    74,    31,    32,     0,     0,   131,
       0,     0,     0,     0,    77,     0,     0,    76,     0,     0,
      82,     0,    80,    81,    79,    84,    85,     0,     0,   132,
       0,    49,    56,     0,    83,     0,     0,   133,     0,     0,
       0,     0,    50,    57,    46,     0,     0,    44,    58,     0,
       0,     0,     0,     0,    51,    47,     0,    45
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -155,  -155,   277,   173,  -155,   231,  -155,  -155,  -155,  -155,
    -155,  -155,   195,  -155,  -155,  -155,  -154,  -155,  -155,  -155,
     196,  -155,  -155,  -155,  -155,  -155,  -155,   187,  -155,  -155,
    -155,  -155,  -155,   204,  -155,  -155,  -155,  -155,  -155,   151,
    -155,  -155,   149,  -155,    78,  -155,     4,  -155,   -12,  -155,
    -155,   154,  -155,   -83,  -103,   -37,    34,  -155
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     3,     4,     5,    24,    25,    26,    27,    28,    29,
      30,    66,    67,    87,   209,   255,   256,   257,    31,    68,
      69,    70,   154,    32,    73,   331,   327,    74,   271,   315,
     324,    33,    76,    77,   272,   316,   325,    34,    79,    80,
      35,    82,    83,   143,   144,   266,   267,   280,   281,    36,
      85,    86,   147,   133,    54,   134,   263,   310
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      51,   148,    55,    56,     6,    50,     8,    13,    14,    15,
      16,    17,    18,    19,    89,    95,     1,    90,    96,    20,
       9,    71,   276,   277,    21,    22,    23,    10,    88,   278,
     279,   137,    98,    11,     9,    99,     1,   138,   139,   140,
     141,   135,   145,   142,   136,   146,   149,   273,   273,   150,
     274,   275,    50,    52,    53,    71,   155,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
       2,   245,    37,   283,   313,    50,   284,   314,    48,    49,
      50,    38,    52,    53,    91,    92,   202,   203,   205,   206,
      57,   268,   269,   294,   295,   285,   286,    39,    40,   210,
      41,   211,    42,   254,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   246,    43,   247,
     248,    94,   250,    44,    45,    46,    47,    58,    59,    60,
      61,   258,    62,    63,    65,    72,    75,    78,    81,    84,
      97,   318,    12,    93,   151,   156,   158,   300,   160,   302,
     303,   161,   162,   305,   163,   307,   308,   164,   309,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     317,   176,   262,   177,   262,   178,   179,   322,   323,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   282,   208,
     189,   190,   258,   258,   191,   192,   289,   193,   194,   287,
     196,   197,   198,   199,   200,   201,   265,   249,   251,   252,
     259,   270,   260,   261,   288,   -33,   290,   291,   282,   306,
     292,   293,   296,   298,   299,   311,   301,   312,   319,   320,
     321,   326,   328,   329,   330,   333,   332,   334,   336,     7,
      64,   253,   157,   304,   152,   264,   195,   297,   153,     0,
       0,     0,     0,     0,   204,   335,     0,     0,     0,   337,
       0,     0,   159,   207
};

static const yytype_int16 yycheck[] =
{
      37,    84,    39,    40,    63,     3,     0,     7,     8,     9,
      10,    11,    12,    13,    62,    62,     6,    65,    65,    19,
      62,    58,    20,    21,    24,    25,    26,    60,    65,    27,
      28,    14,    62,    61,    62,    65,     6,    20,    21,    22,
      23,    62,    62,    26,    65,    65,    62,    62,    62,    65,
      65,    65,     3,     4,     5,    92,    93,   160,   161,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,   192,
      60,   194,    63,    62,    62,     3,    65,    65,    61,    62,
       3,    63,     4,     5,    61,    62,    61,    62,    61,    62,
      64,    61,    62,    61,    62,   269,   270,    63,    63,   156,
      63,   158,    63,   206,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,   194,    63,   196,
     197,    14,   199,    63,    63,    63,    63,    60,    64,    64,
      64,   208,    64,    64,    60,    60,    60,    60,    60,    60,
      14,    16,     9,    63,    63,    63,    63,   290,    63,   292,
     293,    63,    63,   296,    63,   298,   299,    63,   301,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
     313,    63,   249,    63,   251,    63,    63,   320,   321,    63,
      63,    63,    63,    63,    63,    63,    63,    63,   265,    60,
      63,    63,   269,   270,    63,    63,   273,    61,    63,    15,
      63,    63,    63,    63,    63,    63,    60,    64,    64,    64,
      61,    63,    62,    62,    15,    63,    63,    63,   295,   296,
      63,    63,    63,    63,    63,    62,    64,    62,    20,    63,
      63,    62,    61,    17,    62,    18,    63,    61,    63,     2,
      49,   203,    95,   295,    89,   251,   135,   283,    92,    -1,
      -1,    -1,    -1,    -1,   145,   332,    -1,    -1,    -1,   336,
      -1,    -1,    98,   149
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     6,    60,    67,    68,    69,    63,    68,     0,    62,
      60,    61,    69,     7,     8,     9,    10,    11,    12,    13,
      19,    24,    25,    26,    70,    71,    72,    73,    74,    75,
      76,    84,    89,    97,   103,   106,   115,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    61,    62,
       3,   121,     4,     5,   120,   121,   121,    64,    60,    64,
      64,    64,    64,    64,    71,    60,    77,    78,    85,    86,
      87,   121,    60,    90,    93,    60,    98,    99,    60,   104,
     105,    60,   107,   108,    60,   116,   117,    79,   121,    62,
      65,    61,    62,    63,    14,    62,    65,    14,    62,    65,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,   119,   121,    62,    65,    14,    20,    21,
      22,    23,    26,   109,   110,    62,    65,   118,   119,    62,
      65,    63,    78,    86,    88,   121,    63,    93,    63,    99,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    61,    63,   105,    63,    63,    63,    63,
      63,    63,    61,    62,   108,    61,    62,   117,    60,    80,
     121,   121,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   121,   121,   121,    64,
     121,    64,    64,   110,   119,    81,    82,    83,   121,    61,
      62,    62,   121,   122,   122,    60,   111,   112,    61,    62,
      63,    94,   100,    62,    65,    65,    20,    21,    27,    28,
     113,   114,   121,    62,    65,    82,    82,    15,    15,   121,
      63,    63,    63,    63,    61,    62,    63,   112,    63,    63,
     120,    64,   120,   120,   114,   120,   121,   120,   120,   120,
     123,    62,    62,    62,    65,    95,   101,   120,    16,    20,
      63,    63,   120,   120,    96,   102,    62,    92,    61,    17,
      62,    91,    63,    18,    61,   121,    63,   121
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    66,    67,    67,    68,    68,    69,    70,    70,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      72,    73,    74,    75,    76,    77,    77,    78,    79,    80,
      81,    81,    82,    83,    82,    84,    85,    85,    86,    87,
      88,    89,    90,    90,    91,    91,    92,    92,    94,    95,
      96,    93,    97,    98,    98,   100,   101,   102,    99,   103,
     104,   104,   105,   106,   107,   107,   108,   109,   109,   110,
     110,   110,   110,   110,   110,   111,   111,   112,   113,   113,
     114,   114,   114,   114,   114,   114,   115,   116,   116,   117,
     118,   118,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   120,   120,   121,
     122,   122,   123,   123
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     3,     1,     1,     3,     5,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     3,     3,     3,     5,     1,     3,     5,     1,     3,
       1,     3,     3,     1,     1,     5,     1,     3,     3,     1,
       1,     5,     1,     3,     0,     4,     0,     4,     0,     0,
       0,    18,     5,     1,     3,     0,     0,     0,    16,     5,
       1,     3,     3,     5,     1,     3,     3,     1,     3,     3,
       3,     3,     5,     5,     5,     1,     3,     3,     1,     3,
       3,     3,     3,     5,     3,     3,     5,     1,     3,     3,
       1,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     1,     1,     1,
       1,     3,     1,     3
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
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, struct JsonContext *jc)
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
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, struct JsonContext *jc)
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
                 int yyrule, struct JsonContext *jc)
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
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, struct JsonContext *jc)
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
yyparse (struct JsonContext *jc)
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
  case 20: /* starttime: STARTTIME ':' string  */
#line 159 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        {
                                          QDateTime aslocal = QDateTime::fromString(jc->JsonString, DATETIME_FORMAT);
                                          QDateTime asUTC = QDateTime(aslocal.date(), aslocal.time(), Qt::UTC);
                                          jc->JsonRide->setStartTime(asUTC.toLocalTime());
                                        }
#line 1526 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 21: /* recordint: RECINTSECS ':' number  */
#line 164 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonRide->setRecIntSecs(jc->JsonNumber); }
#line 1532 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 22: /* devicetype: DEVICETYPE ':' string  */
#line 165 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonRide->setDeviceType(jc->JsonString); }
#line 1538 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 23: /* identifier: IDENTIFIER ':' string  */
#line 166 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonRide->setId(jc->JsonString); }
#line 1544 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 27: /* override: '{' override_name ':' override_values '}'  */
#line 174 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                                    { jc->JsonRide->metricOverrides.insert(jc->JsonOverName, jc->JsonOverrides);
                                                      jc->JsonOverrides.clear();
                                                    }
#line 1552 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 28: /* override_name: string  */
#line 179 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { if (jc->JsonString == "Time Riding") jc->JsonOverName = "Time Moving";
                                          else jc->JsonOverName = jc->JsonString; }
#line 1559 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 32: /* override_value: override_key ':' override_value  */
#line 184 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                                { jc->JsonOverrides.insert(jc->JsonOverKey, jc->JsonOverValue); }
#line 1565 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 33: /* override_key: string  */
#line 185 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonOverKey = jc->JsonString; }
#line 1571 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 34: /* override_value: string  */
#line 186 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonOverValue = jc->JsonString; }
#line 1577 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 38: /* tag: tag_key ':' tag_value  */
#line 193 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonRide->setTag(jc->JsonTagKey, jc->JsonTagValue); }
#line 1583 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 39: /* tag_key: string  */
#line 196 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { if (jc->JsonString == "Time Riding") jc->JsonTagKey = "Time Moving";
                                          else jc->JsonTagKey = jc->JsonString; }
#line 1590 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 40: /* tag_value: string  */
#line 199 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonTagValue = jc->JsonString; }
#line 1596 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 45: /* interval_test: ',' TEST ':' string  */
#line 207 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                            { jc->JsonInterval.test = (jc->JsonString == "true" ? true : false); }
#line 1602 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 47: /* interval_color: ',' COLOR ':' string  */
#line 211 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                            { jc->JsonInterval.color.setNamedColor(jc->JsonString); }
#line 1608 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 48: /* $@1: %empty  */
#line 214 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonInterval.name = jc->JsonString; }
#line 1614 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 49: /* $@2: %empty  */
#line 215 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonInterval.start = jc->JsonNumber; }
#line 1620 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 50: /* $@3: %empty  */
#line 216 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonInterval.stop = jc->JsonNumber; }
#line 1626 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 51: /* interval: '{' NAME ':' string ',' $@1 START ':' number ',' $@2 STOP ':' number $@3 interval_color interval_test '}'  */
#line 220 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonRide->addInterval(RideFileInterval::USER,
                                                                jc->JsonInterval.start,
                                                                jc->JsonInterval.stop,
                                                                jc->JsonInterval.name,
                                                                jc->JsonInterval.color,
                                                                jc->JsonInterval.test);
                                          jc->JsonInterval = RideFileInterval();
                                        }
#line 1639 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 55: /* $@4: %empty  */
#line 234 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonCalibration.name = jc->JsonString; }
#line 1645 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 56: /* $@5: %empty  */
#line 235 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonCalibration.start = jc->JsonNumber; }
#line 1651 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 57: /* $@6: %empty  */
#line 236 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonCalibration.value = jc->JsonNumber; }
#line 1657 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 58: /* calibration: '{' NAME ':' string ',' $@4 START ':' number ',' $@5 VALUE ':' number $@6 '}'  */
#line 238 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonRide->addCalibration(jc->JsonCalibration.start,
                                                                   jc->JsonCalibration.value,
                                                                   jc->JsonCalibration.name);
                                          jc->JsonCalibration = RideFileCalibration();
                                        }
#line 1667 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 59: /* references: REFERENCES ':' '[' reference_list ']'  */
#line 249 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        {
                                          jc->JsonPoint = RideFilePoint();
                                        }
#line 1675 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 62: /* reference: '{' series '}'  */
#line 253 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonRide->appendReference(jc->JsonPoint);
                                          jc->JsonPoint = RideFilePoint();
                                        }
#line 1683 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 66: /* xdata_series: '{' xdata_items '}'  */
#line 265 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                               { XDataSeries *add = new XDataSeries(jc->xdataseries);
                                                 jc->JsonRide->addXData(add->name, add);

                                                 // clear for next one
                                                 jc->xdataseries = XDataSeries();
                                               }
#line 1694 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 69: /* xdata_item: NAME ':' string  */
#line 277 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                                { jc->xdataseries.name = yyvsp[0]; }
#line 1700 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 70: /* xdata_item: VALUE ':' string  */
#line 278 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                                { jc->xdataseries.valuename << yyvsp[0]; }
#line 1706 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 71: /* xdata_item: UNIT ':' string  */
#line 279 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                                { jc->xdataseries.unitname << yyvsp[0]; }
#line 1712 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 72: /* xdata_item: VALUES ':' '[' string_list ']'  */
#line 280 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                                { jc->xdataseries.valuename = jc->stringlist;
                                                  jc->stringlist.clear(); }
#line 1719 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 73: /* xdata_item: UNITS ':' '[' string_list ']'  */
#line 282 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                                { jc->xdataseries.unitname = jc->stringlist;
                                                  jc->stringlist.clear(); }
#line 1726 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 77: /* xdata_sample: '{' xdata_value_list '}'  */
#line 290 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                                { jc->xdataseries.datapoints.append(new XDataPoint(jc->xdatapoint));
                                                  jc->xdatapoint = XDataPoint();
                                                }
#line 1734 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 80: /* xdata_value: SECS ':' number  */
#line 297 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                                { jc->xdatapoint.secs = jc->JsonNumber; }
#line 1740 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 81: /* xdata_value: KM ':' number  */
#line 298 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                                { jc->xdatapoint.km = jc->JsonNumber; }
#line 1746 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 82: /* xdata_value: VALUE ':' number  */
#line 299 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                                { jc->xdatapoint.number[0] = jc->JsonNumber; }
#line 1752 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 83: /* xdata_value: VALUES ':' '[' number_list ']'  */
#line 300 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                                { for(int i=0; i<jc->numberlist.count() && i<XDATA_MAXVALUES; i++)
                                                      jc->xdatapoint.number[i]= jc->numberlist[i];
                                                  jc->numberlist.clear(); }
#line 1760 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 84: /* xdata_value: string ':' number  */
#line 303 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                                { /* ignored for future compatibility */ }
#line 1766 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 85: /* xdata_value: string ':' string  */
#line 304 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                                { /* ignored for future compatibility */ }
#line 1772 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 89: /* sample: '{' series_list '}'  */
#line 312 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonRide->appendPoint(jc->JsonPoint.secs, jc->JsonPoint.cad,
                                                    jc->JsonPoint.hr, jc->JsonPoint.km, jc->JsonPoint.kph,
                                                    jc->JsonPoint.nm, jc->JsonPoint.watts, jc->JsonPoint.alt,
                                                    jc->JsonPoint.lon, jc->JsonPoint.lat,
                                                    jc->JsonPoint.headwind,
                                                    jc->JsonPoint.slope, jc->JsonPoint.temp, jc->JsonPoint.lrbalance,
                                                    jc->JsonPoint.lte, jc->JsonPoint.rte,
                                                    jc->JsonPoint.lps, jc->JsonPoint.rps,
                                                    jc->JsonPoint.lpco, jc->JsonPoint.rpco,
                                                    jc->JsonPoint.lppb, jc->JsonPoint.rppb,
                                                    jc->JsonPoint.lppe, jc->JsonPoint.rppe,
                                                    jc->JsonPoint.lpppb, jc->JsonPoint.rpppb,
                                                    jc->JsonPoint.lpppe, jc->JsonPoint.rpppe,
                                                    jc->JsonPoint.smo2, jc->JsonPoint.thb,
                                                    jc->JsonPoint.rvert, jc->JsonPoint.rcad, jc->JsonPoint.rcontact,
                                                    jc->JsonPoint.tcore,
                                                    jc->JsonPoint.interval);
                                          jc->JsonPoint = RideFilePoint();
                                        }
#line 1796 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 92: /* series: SECS ':' number  */
#line 333 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.secs = jc->JsonNumber; }
#line 1802 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 93: /* series: KM ':' number  */
#line 334 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.km = jc->JsonNumber; }
#line 1808 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 94: /* series: WATTS ':' number  */
#line 335 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.watts = jc->JsonNumber; }
#line 1814 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 95: /* series: NM ':' number  */
#line 336 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.nm = jc->JsonNumber; }
#line 1820 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 96: /* series: CAD ':' number  */
#line 337 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.cad = jc->JsonNumber; }
#line 1826 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 97: /* series: KPH ':' number  */
#line 338 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.kph = jc->JsonNumber; }
#line 1832 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 98: /* series: HR ':' number  */
#line 339 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.hr = jc->JsonNumber; }
#line 1838 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 99: /* series: ALTITUDE ':' number  */
#line 340 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.alt = jc->JsonNumber; }
#line 1844 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 100: /* series: LAT ':' number  */
#line 341 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.lat = jc->JsonNumber; }
#line 1850 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 101: /* series: LON ':' number  */
#line 342 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.lon = jc->JsonNumber; }
#line 1856 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 102: /* series: HEADWIND ':' number  */
#line 343 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.headwind = jc->JsonNumber; }
#line 1862 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 103: /* series: SLOPE ':' number  */
#line 344 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.slope = jc->JsonNumber; }
#line 1868 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 104: /* series: TEMP ':' number  */
#line 345 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.temp = jc->JsonNumber; }
#line 1874 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 105: /* series: LRBALANCE ':' number  */
#line 346 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.lrbalance = jc->JsonNumber; }
#line 1880 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 106: /* series: LTE ':' number  */
#line 347 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.lte = jc->JsonNumber; }
#line 1886 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 107: /* series: RTE ':' number  */
#line 348 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.rte = jc->JsonNumber; }
#line 1892 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 108: /* series: LPS ':' number  */
#line 349 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.lps = jc->JsonNumber; }
#line 1898 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 109: /* series: RPS ':' number  */
#line 350 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.rps = jc->JsonNumber; }
#line 1904 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 110: /* series: LPCO ':' number  */
#line 351 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.lpco = jc->JsonNumber; }
#line 1910 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 111: /* series: RPCO ':' number  */
#line 352 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.rpco = jc->JsonNumber; }
#line 1916 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 112: /* series: LPPB ':' number  */
#line 353 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.lppb = jc->JsonNumber; }
#line 1922 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 113: /* series: RPPB ':' number  */
#line 354 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.rppb = jc->JsonNumber; }
#line 1928 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 114: /* series: LPPE ':' number  */
#line 355 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.lppe = jc->JsonNumber; }
#line 1934 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 115: /* series: RPPE ':' number  */
#line 356 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.rppe = jc->JsonNumber; }
#line 1940 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 116: /* series: LPPPB ':' number  */
#line 357 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.lpppb = jc->JsonNumber; }
#line 1946 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 117: /* series: RPPPB ':' number  */
#line 358 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.rpppb = jc->JsonNumber; }
#line 1952 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 118: /* series: LPPPE ':' number  */
#line 359 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.lpppe = jc->JsonNumber; }
#line 1958 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 119: /* series: RPPPE ':' number  */
#line 360 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.rpppe = jc->JsonNumber; }
#line 1964 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 120: /* series: SMO2 ':' number  */
#line 361 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.smo2 = jc->JsonNumber; }
#line 1970 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 121: /* series: THB ':' number  */
#line 362 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.thb = jc->JsonNumber; }
#line 1976 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 122: /* series: RVERT ':' number  */
#line 363 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.rvert = jc->JsonNumber; }
#line 1982 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 123: /* series: RCAD ':' number  */
#line 364 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.rcad = jc->JsonNumber; }
#line 1988 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 124: /* series: RCON ':' number  */
#line 365 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { jc->JsonPoint.rcontact = jc->JsonNumber; }
#line 1994 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 125: /* series: string ':' number  */
#line 366 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                        { }
#line 2000 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 127: /* number: JS_INTEGER  */
#line 374 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                           { jc->JsonNumber = QString(yyvsp[0]).toInt(); }
#line 2006 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 128: /* number: JS_FLOAT  */
#line 375 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                           { jc->JsonNumber = QString(yyvsp[0]).toDouble(); }
#line 2012 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 129: /* string: JS_STRING  */
#line 378 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                           { jc->JsonString = yyvsp[0]; }
#line 2018 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 130: /* string_list: string  */
#line 381 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                           { jc->stringlist << yyvsp[0]; }
#line 2024 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 131: /* string_list: string_list ',' string  */
#line 382 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                           { jc->stringlist << yyvsp[0]; }
#line 2030 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 132: /* number_list: number  */
#line 385 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                           { jc->numberlist << QString(yyvsp[0]).toDouble(); }
#line 2036 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;

  case 133: /* number_list: number_list ',' number  */
#line 386 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"
                                           { jc->numberlist << QString(yyvsp[0]).toDouble(); }
#line 2042 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"
    break;


#line 2046 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/JsonRideFile_yacc.cpp"

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

#line 388 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/FileIO/JsonRideFile.y"



static int jsonFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "json", "GoldenCheetah Json", new JsonFileReader());

RideFile *
JsonFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    // Read the entire file into a QString -- we avoid using fopen since it
    // doesn't handle foreign characters well. Instead we use QFile and parse
    // from a QString
    QString contents;
    if (file.exists() && file.open(QFile::ReadOnly | QFile::Text)) {

        // read in the whole thing
        QTextStream in(&file);
        // GC .JSON is stored in UTF-8 with BOM(Byte order mark) for identification
#if QT_VERSION < 0x060000
        in.setCodec ("UTF-8");
#endif
        contents = in.readAll();
        file.close();

        // check if the text string contains the replacement character for UTF-8 encoding
        // if yes, try to read with Latin1/ISO 8859-1 (assuming this is an "old" non-UTF-8 Json file)
        if (contents.contains(QChar::ReplacementCharacter)) {
           if (file.exists() && file.open(QFile::ReadOnly | QFile::Text)) {
             QTextStream in(&file);
#if QT_VERSION < 0x060000
             in.setCodec ("ISO 8859-1");
#else
             in.setEncoding (QStringConverter::Latin1);
#endif
             contents = in.readAll();
             file.close();
           }
         }

    } else {

        errors << "unable to open file" + file.fileName();
        return NULL; 
    }

    // create scanner context for reentrant parsing
    JsonContext *jc = new JsonContext;
    JsonRideFilelex_init(&scanner);

    // inform the parser/lexer we have a new file
    JsonRideFile_setString(contents, scanner);

    // setup
    jc->JsonRide = new RideFile;
    jc->JsonRideFileerrors.clear();

    // set to non-zero if you want to
    // to debug the yyparse() state machine
    // sending state transitions to stderr
    //yydebug = 1;

    // parse it
    JsonRideFileparse(jc);

    // clean up
    JsonRideFilelex_destroy(scanner);

    // Only get errors so fail if we have any
    // and always delete context now we're done
    if (errors.count()) {
        errors << jc->JsonRideFileerrors;
        delete jc->JsonRide;
        delete jc;
        return NULL;
    }

    RideFile *returning = jc->JsonRide;
    delete jc;
    return returning;
}

QByteArray
JsonFileReader::toByteArray(Context *, const RideFile *ride, bool withAlt, bool withWatts, bool withHr, bool withCad) const
{
    QString out;

    // start of document and ride
    out += "{\n\t\"RIDE\":{\n";

    // first class variables
    out += "\t\t\"STARTTIME\":\"" + protect(ride->startTime().toUTC().toString(DATETIME_FORMAT)) + "\",\n";
    out += "\t\t\"RECINTSECS\":" + QString("%1").arg(ride->recIntSecs()) + ",\n";
    out += "\t\t\"DEVICETYPE\":\"" + protect(ride->deviceType()) + "\",\n";
    out += "\t\t\"IDENTIFIER\":\"" + protect(ride->id()) + "\"";

    //
    // OVERRIDES
    //
    bool nonblanks = false; // if an override has been deselected it may be blank
                            // so we only output the OVERRIDES section if we find an
                            // override whilst iterating over the QMap

    if (ride->metricOverrides.count()) {


        QMap<QString,QMap<QString, QString> >::const_iterator k;
        for (k=ride->metricOverrides.constBegin(); k != ride->metricOverrides.constEnd(); k++) {

            if (nonblanks == false) {
                out += ",\n\t\t\"OVERRIDES\":[\n";
                nonblanks = true;

            }
            // begin of overrides
            out += "\t\t\t{ \"" + k.key() + "\":{ ";

            // key/value pairs
            QMap<QString, QString>::const_iterator j;
            for (j=k.value().constBegin(); j != k.value().constEnd(); j++) {

                // comma separated
                out += "\"" + j.key() + "\":\"" + j.value() + "\"";
                if (std::next(j) != k.value().constEnd()) out += ", ";
            }
            if (std::next(k) != ride->metricOverrides.constEnd()) out += " }},\n";
            else out += " }}\n";
        }

        if (nonblanks == true) {
            // end of the overrides
            out += "\t\t]";
        }
    }

    //
    // TAGS
    //
    if (ride->tags().count()) {

        out += ",\n\t\t\"TAGS\":{\n";

        QMap<QString,QString>::const_iterator i;
        for (i=ride->tags().constBegin(); i != ride->tags().constEnd(); i++) {

                out += "\t\t\t\"" + i.key() + "\":\"" + protect(i.value()) + "\"";
                if (std::next(i) != ride->tags().constEnd()) out += ",\n";
        }

        foreach(RideFileInterval *inter, ride->intervals()) {
            bool first=true;
            QMap<QString,QString>::const_iterator i;
            for (i=inter->tags().constBegin(); i != inter->tags().constEnd(); i++) {

                    if (first) {
                        out += ",\n";
                        first=false;
                    }
                    out += "\t\t\t\"" + inter->name + "##" + i.key() + "\":\"" + protect(i.value()) + "\"";
                    if (std::next(i) != inter->tags().constEnd()) out += ",\n";
            }
        }

        // end of the tags
        out += "\n\t\t}";
    }

    //
    // INTERVALS
    //
    if (!ride->intervals().empty()) {

        out += ",\n\t\t\"INTERVALS\":[\n";
        bool first = true;

        foreach (RideFileInterval *i, ride->intervals()) {
            if (first) first=false;
            else out += ",\n";

            out += "\t\t\t{ ";
            out += "\"NAME\":\"" + protect(i->name) + "\"";
            out += ", \"START\": " + QString("%1").arg(i->start);
            out += ", \"STOP\": " + QString("%1").arg(i->stop);
            out += ", \"COLOR\":" + QString("\"%1\"").arg(i->color.name());
            out += ", \"PTEST\":\"" + QString("%1").arg(i->test ? "true" : "false") + "\" }";
        }
        out += "\n\t\t]";
    }

    //
    // CALIBRATION
    //
    if (!ride->calibrations().empty()) {

        out += ",\n\t\t\"CALIBRATIONS\":[\n";
        bool first = true;

        foreach (RideFileCalibration *i, ride->calibrations()) {
            if (first) first=false;
            else out += ",\n";

            out += "\t\t\t{ ";
            out += "\"NAME\":\"" + protect(i->name) + "\"";
            out += ", \"START\": " + QString("%1").arg(i->start);
            out += ", \"VALUE\": " + QString("%1").arg(i->value) + " }";
        }
        out += "\n\t\t]";
    }

    //
    // REFERENCES
    //
    if (!ride->referencePoints().empty()) {

        out += ",\n\t\t\"REFERENCES\":[\n";
        bool first = true;

        foreach (RideFilePoint *p, ride->referencePoints()) {
            if (first) first=false;
            else out += ",\n";

            out += "\t\t\t{ ";

            if (p->watts > 0) out += " \"WATTS\":" + QString("%1").arg(p->watts);
            if (p->cad > 0) out += " \"CAD\":" + QString("%1").arg(p->cad);
            if (p->hr > 0) out += " \"HR\":"  + QString("%1").arg(p->hr);
            if (p->secs > 0) out += " \"SECS\":" + QString("%1").arg(p->secs);

            // sample points in here!
            out += " }";
        }
        out +="\n\t\t]";
    }

    //
    // SAMPLES
    //
    if (ride->dataPoints().count()) {

        out += ",\n\t\t\"SAMPLES\":[\n";
        bool first = true;

        foreach (RideFilePoint *p, ride->dataPoints()) {

            if (first) first=false;
            else out += ",\n";

            out += "\t\t\t{ ";

            // always store time
            out += "\"SECS\":" + QString("%1").arg(p->secs);

            if (ride->areDataPresent()->km) out += ", \"KM\":" + QString("%1").arg(p->km);
            if (ride->areDataPresent()->watts && withWatts) out += ", \"WATTS\":" + QString("%1").arg(p->watts);
            if (ride->areDataPresent()->nm) out += ", \"NM\":" + QString("%1").arg(p->nm);
            if (ride->areDataPresent()->cad && withCad) out += ", \"CAD\":" + QString("%1").arg(p->cad);
            if (ride->areDataPresent()->kph) out += ", \"KPH\":" + QString("%1").arg(p->kph);
            if (ride->areDataPresent()->hr && withHr) out += ", \"HR\":"  + QString("%1").arg(p->hr);
            if (ride->areDataPresent()->alt && withAlt)
			    out += ", \"ALT\":" + QString("%1").arg(p->alt, 0, 'g', 11);
            if (ride->areDataPresent()->lat)
                out += ", \"LAT\":" + QString("%1").arg(p->lat, 0, 'g', 11);
            if (ride->areDataPresent()->lon)
                out += ", \"LON\":" + QString("%1").arg(p->lon, 0, 'g', 11);
            if (ride->areDataPresent()->headwind) out += ", \"HEADWIND\":" + QString("%1").arg(p->headwind);
            if (ride->areDataPresent()->slope) out += ", \"SLOPE\":" + QString("%1").arg(p->slope);
            if (ride->areDataPresent()->temp && p->temp != RideFile::NA) out += ", \"TEMP\":" + QString("%1").arg(p->temp);
            if (ride->areDataPresent()->lrbalance && p->lrbalance != RideFile::NA) out += ", \"LRBALANCE\":" + QString("%1").arg(p->lrbalance);
            if (ride->areDataPresent()->lte) out += ", \"LTE\":" + QString("%1").arg(p->lte);
            if (ride->areDataPresent()->rte) out += ", \"RTE\":" + QString("%1").arg(p->rte);
            if (ride->areDataPresent()->lps) out += ", \"LPS\":" + QString("%1").arg(p->lps);
            if (ride->areDataPresent()->rps) out += ", \"RPS\":" + QString("%1").arg(p->rps);
            if (ride->areDataPresent()->lpco) out += ", \"LPCO\":" + QString("%1").arg(p->lpco);
            if (ride->areDataPresent()->rpco) out += ", \"RPCO\":" + QString("%1").arg(p->rpco);
            if (ride->areDataPresent()->lppb) out += ", \"LPPB\":" + QString("%1").arg(p->lppb);
            if (ride->areDataPresent()->rppb) out += ", \"RPPB\":" + QString("%1").arg(p->rppb);
            if (ride->areDataPresent()->lppe) out += ", \"LPPE\":" + QString("%1").arg(p->lppe);
            if (ride->areDataPresent()->rppe) out += ", \"RPPE\":" + QString("%1").arg(p->rppe);
            if (ride->areDataPresent()->lpppb) out += ", \"LPPPB\":" + QString("%1").arg(p->lpppb);
            if (ride->areDataPresent()->rpppb) out += ", \"RPPPB\":" + QString("%1").arg(p->rpppb);
            if (ride->areDataPresent()->lpppe) out += ", \"LPPPE\":" + QString("%1").arg(p->lpppe);
            if (ride->areDataPresent()->rpppe) out += ", \"RPPPE\":" + QString("%1").arg(p->rpppe);
            if (ride->areDataPresent()->smo2) out += ", \"SMO2\":" + QString("%1").arg(p->smo2);
            if (ride->areDataPresent()->thb) out += ", \"THB\":" + QString("%1").arg(p->thb);
            if (ride->areDataPresent()->rcad) out += ", \"RCAD\":" + QString("%1").arg(p->rcad);
            if (ride->areDataPresent()->rvert) out += ", \"RVERT\":" + QString("%1").arg(p->rvert);
            if (ride->areDataPresent()->rcontact) out += ", \"RCON\":" + QString("%1").arg(p->rcontact);

            // sample points in here!
            out += " }";
        }
        out +="\n\t\t]";
    }

    //
    // XDATA
    //
    if (const_cast<RideFile*>(ride)->xdata().count()) {
        // output the xdata series
        out += ",\n\t\t\"XDATA\":[\n";

        bool first = true;
        QMapIterator<QString,XDataSeries*> xdata(const_cast<RideFile*>(ride)->xdata());
        xdata.toFront();
        while(xdata.hasNext()) {

            // iterate
            xdata.next();

            XDataSeries *series = xdata.value();

            // does it have values names?
            if (series->valuename.isEmpty()) continue;

            if (!first) out += ",\n";
            out += "\t\t{\n";

            // series name
            out += "\t\t\t\"NAME\" : \"" + xdata.key() + "\",\n";

            // value names
            if (series->valuename.count() > 1) {
                out += "\t\t\t\"VALUES\" : [ ";
                bool firstv=true;
                foreach(QString x, series->valuename) {
                    if (!firstv) out += ", ";
                    out += "\"" + x + "\"";
                    firstv=false;
                }
                out += " ]";
            } else {
                out += "\t\t\t\"VALUE\" : \"" + series->valuename[0] + "\"";
            }

            // unit names
            if (series->unitname.count() > 1) {
                out += ",\n\t\t\t\"UNITS\" : [ ";
                bool firstv=true;
                foreach(QString x, series->unitname) {
                    if (!firstv) out += ", ";
                    out += "\"" + x + "\"";
                    firstv=false;
                }
                out += " ]";
            } else {
                if (series->unitname.count() > 0) out += ",\n\t\t\t\"UNIT\" : \"" + series->unitname[0] + "\"";
            }

            // samples
            if (series->datapoints.count()) {
                out += ",\n\t\t\t\"SAMPLES\" : [\n";

                bool firsts=true;
                foreach(XDataPoint *p, series->datapoints) {
                    if (!firsts) out += ",\n";

                    // multi value sample
                    if (series->valuename.count()>1) {

                        out += "\t\t\t\t{ \"SECS\":"+QString("%1").arg(p->secs) +", "
                            + "\"KM\":"+QString("%1").arg(p->km) + ", "
                            + "\"VALUES\":[ ";

                        bool firstvv=true;
                        for(int i=0; i<series->valuename.count(); i++) {
                            if (!firstvv) out += ", ";
                            out += QString("%1").arg(p->number[i]);
                            firstvv=false;
                         }
                         out += " ] }";

                    } else {

                        out += "\t\t\t\t{ \"SECS\":"+QString("%1").arg(p->secs) + ", "
                            + "\"KM\":"+QString("%1").arg(p->km) + ", "
                            + "\"VALUE\":" + QString("%1").arg(p->number[0]) + " }";
                    }
                    firsts = false;
                }

                out += "\n\t\t\t]\n";
            } else {
                out += "\n";
            }

            out += "\t\t}";

            // now do next
            first = false;
        }

        out += "\n\t\t]";
    }

    // end of ride and document
    out += "\n\t}\n}\n";

    return out.toUtf8();
}

// Writes valid .json (validated at www.jsonlint.com)
bool
JsonFileReader::writeRideFile(Context *context, const RideFile *ride, QFile &file) const
{
    // can we open the file for writing?
    if (!file.open(QIODevice::WriteOnly)) return false;

    // truncate existing
    file.resize(0);

    QByteArray xml = toByteArray(context, ride, true, true, true, true);

    // setup streamer
    QTextStream out(&file);
    // unified codepage and BOM for identification on all platforms
#if QT_VERSION < 0x060000
    out.setCodec("UTF-8");
#endif
    out.setGenerateByteOrderMark(true);

    out << xml;
    out.flush();

    // close
    file.close();

    return true;
}
