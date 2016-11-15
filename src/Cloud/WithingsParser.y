%{
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

#include "WithingsParser.h"
#include <QDebug>
#include <math.h> // for pow

// term state data is held in these variables
static QString WithingsString;
static int WithingsNumber;
static int WithingsType;
static int WithingsValue;
static int WithingsUnit;

static QStringList WithingsParsererrors;

// Standard yacc/lex variables / functions
extern int WithingsParserlex(); // the lexer aka yylex()
extern void WithingsParser_set(QString);
extern void WithingsParser_reset();

extern char *WithingsParsertext; // set by the lexer aka yytext
void WithingsParsererror(const char *error) // used by parser aka yyerror()
{ WithingsParsererrors << error; }

static WithingsReading reading;
static QList<WithingsReading> Readings;

%}

%token STRING INTEGER FLOAT
%token STATUS BODY UPDATETIME MEASUREGRPS GRPID ATTRIB DATE CATEGORY COMMENT MEASURES VALUE TYPE UNIT

%start document
%%

/* we always get a response in curly-braces */
document: '{' responses '}'
        ;

/* responses are either a "status":x or "status":0, "body":{ */
responses: response
        | responses ',' response
        ;
response:
        | status
        | body
        ;

/* status are just integer codes */
status: STATUS ':' number { /* we ignore status for now */ }
        ;

/* body contains a set of elements */
body: BODY ':' '{' body_elements '}'
        ;

body_elements: body_element
        | body_elements ',' body_element
        ;
body_element: updatetime
        | measuregrps
        ;

/* update time is when the data was retrieved (?) */
updatetime : UPDATETIME ':' number { /* we ignore these for now */ }
        ;

/* measuregrps is an array of measurements */
measuregrps: MEASUREGRPS ':' '[' measuregrp_list ']'
        ;
measuregrp_list: measuregrp
        | measuregrp_list ',' measuregrp
        ;

/* the measures are where the action is! */
measuregrp: '{'   { reading = WithingsReading(); }
            measuregrp_elements '}' { Readings << reading; }
        ;
measuregrp_elements: measuregrp_element
        | measuregrp_elements ',' measuregrp_element
        ;
measuregrp_element: grpid
        | attrib
        | date
        | category
        | comment
        | measures
        ;

grpid: GRPID ':' number { reading.groupId = WithingsNumber; }
        ;
attrib: ATTRIB ':' number { reading.attribution = WithingsNumber; }
        ;
date: DATE ':' number { reading.when.setTime_t(WithingsNumber); }
        ;
category: CATEGORY ':' number { reading.category = WithingsNumber; }
        ;
comment: COMMENT ':' string { reading.comment = WithingsString; }
        ;

measures: MEASURES ':' '[' measure_list ']'
        ;
measure_list: measure
        | measure_list ',' measure
        ;
/* a measure might be weight, fat ratio, fat free mass etc */
measure : '{' VALUE ':' number ',' { WithingsValue = WithingsNumber; }
              TYPE  ':' number ',' { WithingsType = WithingsNumber; }
              UNIT ':' number      { WithingsUnit = WithingsNumber; }
           '}'                     { /* value x 10^unit is high precision value */
                                     /* type tells us which kind of measure this is */
                                     double value = (double)WithingsValue * pow(10.00, WithingsUnit);
                                     switch (WithingsType) {

                                     case 1 : reading.weightkg = value; break;
                                     case 4 : reading.sizemeter = value; break;
                                     case 5 : reading.leankg = value; break;
                                     case 6 : reading.fatpercent = value; break;
                                     case 8 : reading.fatkg = value; break;
                                     default: break;

                                     }
                                   }
        ;

/*
 * Primitives
 */
number: INTEGER                         { WithingsNumber = QString(WithingsParsertext).toInt(); }
        | FLOAT                         { WithingsNumber = QString(WithingsParsertext).toDouble(); }
        ;

string: STRING                          { WithingsString = WithingsParsertext; }
        ;
%%


bool
WithingsParser::parse(QString &text, QStringList &errors)
{
    // setup the input buffer and clear errors
    WithingsParser_set(text);
    WithingsParsererrors.clear();

    // clear previous parse data
    Readings.clear();
    _readings.clear();
    reading = WithingsReading();

    // parse that sucker
    //yydebug = 0; -- needs -t on bison command line
    WithingsParserparse();

    // reset the input buffer
    WithingsParser_reset();

    // Only get errors so fail if we have any
    if (errors.count()) {
        errors << WithingsParsererrors;
        return false;
    }
    _readings = Readings;
    return true;
}
