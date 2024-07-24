%{
/*
 * Copyright (c) 2024 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "TrainerDayAPIQuery.h"

#include <stdio.h>
#include <stdlib.h>
#include <QStringList>
#include <QString>
#include <QUrlQuery>
#include <QDebug>

extern int yylex(void);
extern void yyerror(const char *);
extern QUrlQuery trainerDayAPIQuery;
extern QString trainerDayAPIQueryErrorMsg;
extern const QList<QString> trainerDayAPIQueryZones = {
    "recovery",
    "endurance",
    "tempo",
    "threshold",
    "vo2max",
    "anaerobic",
    "test"
};
%}

%union {
    int numValue;
    char *word;
    QStringList *wordList;
}


%destructor { qDebug() << "Deleting wordList"; delete $$; } <wordList>

%token DOMINANTZONE
%token DURATION
%token RANGESYMBOL
%token ZONE_RECOVERY
%token ZONE_ENDURANCE
%token ZONE_TEMPO
%token ZONE_THRESHOLD
%token ZONE_VO2MAX
%token ZONE_ANAEROBIC
%token ZONE_TEST
%token <numValue> NUMBER ZONE TIME
%token <word> WORD

%type <filter> dominantzone duration
%type <numValue> minuteValue
%type <wordList> word words

%%

statements: statement ',' statements
    |       statement
	|
    ;

statement:  dominantzone
    |       duration
    |       words                                 { trainerDayAPIQuery.addQueryItem("workoutName", QUrl::toPercentEncoding($1->join(" "))); }
    ;

dominantzone: DOMINANTZONE ZONE                   { trainerDayAPIQuery.addQueryItem("dominantZone", trainerDayAPIQueryZones[$2 - 1]); }
    |         DOMINANTZONE ZONE_RECOVERY          { trainerDayAPIQuery.addQueryItem("dominantZone", "recovery"); }
    |         DOMINANTZONE ZONE_ENDURANCE         { trainerDayAPIQuery.addQueryItem("dominantZone", "endurance"); }
    |         DOMINANTZONE ZONE_TEMPO             { trainerDayAPIQuery.addQueryItem("dominantZone", "Tempo"); }
    |         DOMINANTZONE ZONE_THRESHOLD         { trainerDayAPIQuery.addQueryItem("dominantZone", "threshold"); }
    |         DOMINANTZONE ZONE_VO2MAX            { trainerDayAPIQuery.addQueryItem("dominantZone", "vo2max"); }
    |         DOMINANTZONE ZONE_ANAEROBIC         { trainerDayAPIQuery.addQueryItem("dominantZone", "anaerobic"); }
    |         DOMINANTZONE ZONE_TEST              { trainerDayAPIQuery.addQueryItem("dominantZone", "test"); }
    ;

duration: DURATION minuteValue                          { trainerDayAPIQuery.addQueryItem("fromMinutes", QString::number(std::max(0, $2 - 5))); trainerDayAPIQuery.addQueryItem("toMinutes", QString::number($2 + 5)); }
    |     DURATION minuteValue RANGESYMBOL minuteValue  { trainerDayAPIQuery.addQueryItem("fromMinutes", QString::number($2)); trainerDayAPIQuery.addQueryItem("toMinutes", QString::number($4)); }
    |     DURATION RANGESYMBOL minuteValue              { trainerDayAPIQuery.addQueryItem("toMinutes", QString::number($3)); }
    |     DURATION minuteValue RANGESYMBOL              { trainerDayAPIQuery.addQueryItem("fromMinutes", QString::number($2)); }
    ;

minuteValue: NUMBER  { $$ = $1; }
    |        TIME    { $$ = $1; }
    ;

words: word words  { *$1 << *$2; delete $2; $$ = $1; }
    |  word        { $$ = $1; }
    ;

word: WORD  { $$ = new QStringList($1); }
    ;

%%

void yyerror(const char *s)
{
    trainerDayAPIQueryErrorMsg = QString(s);
}
