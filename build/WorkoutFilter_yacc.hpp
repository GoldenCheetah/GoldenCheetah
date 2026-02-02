/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_WORKOUTFILTER_MEDIA_ANDY_TOSHIBA_EXT_BACKUP2_DOCUMENTS_GOLDENCHEETAH_BUILD_WORKOUTFILTER_YACC_HPP_INCLUDED
# define YY_WORKOUTFILTER_MEDIA_ANDY_TOSHIBA_EXT_BACKUP2_DOCUMENTS_GOLDENCHEETAH_BUILD_WORKOUTFILTER_YACC_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int WorkoutFilterdebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    DOMINANTZONE = 258,            /* DOMINANTZONE  */
    DURATION = 259,                /* DURATION  */
    INTENSITY = 260,               /* INTENSITY  */
    VI = 261,                      /* VI  */
    XPOWER = 262,                  /* XPOWER  */
    RI = 263,                      /* RI  */
    BIKESCORE = 264,               /* BIKESCORE  */
    SVI = 265,                     /* SVI  */
    STRESS = 266,                  /* STRESS  */
    RATING = 267,                  /* RATING  */
    RANGESYMBOL = 268,             /* RANGESYMBOL  */
    SEPARATOR = 269,               /* SEPARATOR  */
    FLOAT = 270,                   /* FLOAT  */
    NUMBER = 271,                  /* NUMBER  */
    ZONE = 272,                    /* ZONE  */
    PERCENT = 273,                 /* PERCENT  */
    DAYS = 274,                    /* DAYS  */
    TIME = 275,                    /* TIME  */
    MINPOWER = 276,                /* MINPOWER  */
    MAXPOWER = 277,                /* MAXPOWER  */
    AVGPOWER = 278,                /* AVGPOWER  */
    ISOPOWER = 279,                /* ISOPOWER  */
    POWER = 280,                   /* POWER  */
    LASTRUN = 281,                 /* LASTRUN  */
    CREATED = 282,                 /* CREATED  */
    DISTANCE = 283,                /* DISTANCE  */
    ELEVATION = 284,               /* ELEVATION  */
    GRADE = 285,                   /* GRADE  */
    WORD = 286                     /* WORD  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 38 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/src/Train/WorkoutFilter.y"

    float floatValue;
    int numValue;
    char *word;
    void *wordList;
    void *filter;
    void *filterPair;

#line 104 "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/build/WorkoutFilter_yacc.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE WorkoutFilterlval;


int WorkoutFilterparse (void);


#endif /* !YY_WORKOUTFILTER_MEDIA_ANDY_TOSHIBA_EXT_BACKUP2_DOCUMENTS_GOLDENCHEETAH_BUILD_WORKOUTFILTER_YACC_HPP_INCLUDED  */
