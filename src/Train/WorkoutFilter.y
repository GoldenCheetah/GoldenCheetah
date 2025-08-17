%{
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
%}

%union {
    float floatValue;
    int numValue;
    char *word;
    QStringList *wordList;
    ModelFilter *filter;
    std::pair<ModelFilter*, ModelFilter*> *filterPair;
}


%destructor { qDebug() << "Deleting " << $$->toString(); delete $$; } <filter>
%destructor { qDebug() << "Deleting wordList"; delete $$; } <wordList>
%destructor { qDebug() << "Deleting filterPair"; delete $$; } <filterPair>

%token DOMINANTZONE
%token DURATION
%token INTENSITY
%token VI
%token XPOWER
%token RI
%token BIKESCORE
%token SVI
%token STRESS
%token RATING
%token RANGESYMBOL
%token SEPARATOR
%token <floatValue> WF_FLOAT
%token <numValue> WF_NUMBER ZONE PERCENT DAYS TIME MINPOWER MAXPOWER AVGPOWER ISOPOWER POWER LASTRUN CREATED DISTANCE ELEVATION GRADE
%token <word> WF_WORD FILEPATH

%type <filter> zone dominantzone duration stress intensity vi xpower ri bikescore svi rating minpower maxpower avgpower isopower lastrun created distance elevation grade filepath
%type <filterPair> power
%type <wordList> word words
%type <floatValue> mixedNumValue
%type <numValue> minuteValue

%%

statements: statement ',' statements
    |       statement
    ;

statement:  dominantzone  { workoutModelFilters << $1; }
    |       zone          { workoutModelFilters << $1; }
    |       duration      { workoutModelFilters << $1; }
    |       intensity     { workoutModelFilters << $1; }
    |       vi            { workoutModelFilters << $1; }
    |       xpower        { workoutModelFilters << $1; }
    |       ri            { workoutModelFilters << $1; }
    |       bikescore     { workoutModelFilters << $1; }
    |       svi           { workoutModelFilters << $1; }
    |       stress        { workoutModelFilters << $1; }
    |       rating        { workoutModelFilters << $1; }
    |       minpower      { workoutModelFilters << $1; }
    |       maxpower      { workoutModelFilters << $1; }
    |       avgpower      { workoutModelFilters << $1; }
    |       isopower      { workoutModelFilters << $1; }
    |       power         { workoutModelFilters << $1->first << $1->second; }
    |       lastrun       { workoutModelFilters << $1; }
    |       created       { workoutModelFilters << $1; }
    |       distance      { workoutModelFilters << $1; }
    |       elevation     { workoutModelFilters << $1; }
    |       grade         { workoutModelFilters << $1; }
    |       filepath      { workoutModelFilters << $1; }
    |       words         { workoutModelFilters << new ModelStringContainsFilter(TdbWorkoutModelIdx::fulltext, *$1); }
    ;

dominantzone: DOMINANTZONE ZONE                   { $$ = new ModelNumberEqualFilter(TdbWorkoutModelIdx::dominantZone, $2); }
    |         DOMINANTZONE ZONE RANGESYMBOL ZONE  { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::dominantZone, $2, $4); }
    |         DOMINANTZONE RANGESYMBOL ZONE       { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::dominantZone, 1, $3); }
    |         DOMINANTZONE ZONE RANGESYMBOL       { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::dominantZone, $2, 10); }
    ;

zone: ZONE minuteValue                          { $$ = new ModelNumberEqualFilter(workoutModelZoneIndex($1, ZoneContentType::seconds), $2 * 60); }
    | ZONE minuteValue RANGESYMBOL minuteValue  { $$ = new ModelNumberRangeFilter(workoutModelZoneIndex($1, ZoneContentType::seconds), $2 * 60, $4 * 60); }
    | ZONE RANGESYMBOL minuteValue              { $$ = new ModelNumberRangeFilter(workoutModelZoneIndex($1, ZoneContentType::seconds), 0, $3 * 60); }
    | ZONE minuteValue RANGESYMBOL              { $$ = new ModelNumberRangeFilter(workoutModelZoneIndex($1, ZoneContentType::seconds), $2 * 60); }
    | ZONE PERCENT                              { $$ = new ModelNumberEqualFilter(workoutModelZoneIndex($1, ZoneContentType::percent), $2); }
    | ZONE PERCENT RANGESYMBOL PERCENT          { $$ = new ModelNumberRangeFilter(workoutModelZoneIndex($1, ZoneContentType::percent), $2, $4); }
    | ZONE RANGESYMBOL PERCENT                  { $$ = new ModelNumberRangeFilter(workoutModelZoneIndex($1, ZoneContentType::percent), 0, $3); }
    | ZONE PERCENT RANGESYMBOL                  { $$ = new ModelNumberRangeFilter(workoutModelZoneIndex($1, ZoneContentType::percent), $2); }
    ;

duration: DURATION minuteValue                          { $$ = new ModelNumberEqualFilter(TdbWorkoutModelIdx::duration, $2 * 60000, 5 * 60000); }
    |     DURATION minuteValue RANGESYMBOL minuteValue  { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::duration, $2 * 60000, $4 * 60000); }
    |     DURATION RANGESYMBOL minuteValue              { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::duration, 0, $3 * 60000); }
    |     DURATION minuteValue RANGESYMBOL              { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::duration, $2 * 60000); }
    ;

intensity: INTENSITY mixedNumValue                            { $$ = new ModelFloatEqualFilter(TdbWorkoutModelIdx::intensity, $2, 0.05); }
    |      INTENSITY mixedNumValue RANGESYMBOL mixedNumValue  { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::intensity, $2, $4); }
    |      INTENSITY RANGESYMBOL mixedNumValue                { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::intensity, 0, $3); }
    |      INTENSITY mixedNumValue RANGESYMBOL                { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::intensity, $2); }
    ;

vi:   VI mixedNumValue                            { $$ = new ModelFloatEqualFilter(TdbWorkoutModelIdx::vi, $2, 0.05); }
    | VI mixedNumValue RANGESYMBOL mixedNumValue  { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::vi, $2, $4); }
    | VI RANGESYMBOL mixedNumValue                { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::vi, 0, $3); }
    | VI mixedNumValue RANGESYMBOL                { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::vi, $2); }
    ;

xpower: XPOWER mixedNumValue                            { $$ = new ModelFloatEqualFilter(TdbWorkoutModelIdx::xp, $2, 0.05); }
    |   XPOWER mixedNumValue RANGESYMBOL mixedNumValue  { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::xp, $2, $4); }
    |   XPOWER RANGESYMBOL mixedNumValue                { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::xp, 0, $3); }
    |   XPOWER mixedNumValue RANGESYMBOL                { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::xp, $2); }
    ;

ri:   RI mixedNumValue                            { $$ = new ModelFloatEqualFilter(TdbWorkoutModelIdx::ri, $2, 0.05); }
    | RI mixedNumValue RANGESYMBOL mixedNumValue  { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::ri, $2, $4); }
    | RI RANGESYMBOL mixedNumValue                { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::ri, 0, $3); }
    | RI mixedNumValue RANGESYMBOL                { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::ri, $2); }
    ;

bikescore: BIKESCORE mixedNumValue                            { $$ = new ModelFloatEqualFilter(TdbWorkoutModelIdx::bs, $2, 0.05); }
    |      BIKESCORE mixedNumValue RANGESYMBOL mixedNumValue  { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::bs, $2, $4); }
    |      BIKESCORE RANGESYMBOL mixedNumValue                { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::bs, 0, $3); }
    |      BIKESCORE mixedNumValue RANGESYMBOL                { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::bs, $2); }
    ;

svi:  SVI mixedNumValue                            { $$ = new ModelFloatEqualFilter(TdbWorkoutModelIdx::svi, $2, 0.05); }
    | SVI mixedNumValue RANGESYMBOL mixedNumValue  { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::svi, $2, $4); }
    | SVI RANGESYMBOL mixedNumValue                { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::svi, 0, $3); }
    | SVI mixedNumValue RANGESYMBOL                { $$ = new ModelFloatRangeFilter(TdbWorkoutModelIdx::svi, $2); }
    ;

stress: STRESS WF_NUMBER                        { $$ = new ModelNumberEqualFilter(TdbWorkoutModelIdx::bikestress, $2); }
    |   STRESS WF_NUMBER RANGESYMBOL WF_NUMBER  { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::bikestress, $2, $4); }
    |   STRESS RANGESYMBOL WF_NUMBER            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::bikestress, 0, $3); }
    |   STRESS WF_NUMBER RANGESYMBOL            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::bikestress, $2); }
    ;

minpower: MINPOWER WF_NUMBER                        { $$ = new ModelNumberEqualFilter(TdbWorkoutModelIdx::minPower, $2); }
    |     MINPOWER WF_NUMBER RANGESYMBOL WF_NUMBER  { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::minPower, $2, $4); }
    |     MINPOWER RANGESYMBOL WF_NUMBER            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::minPower, 0, $3); }
    |     MINPOWER WF_NUMBER RANGESYMBOL            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::minPower, $2); }
    ;

maxpower: MAXPOWER WF_NUMBER                        { $$ = new ModelNumberEqualFilter(TdbWorkoutModelIdx::maxPower, $2); }
    |     MAXPOWER WF_NUMBER RANGESYMBOL WF_NUMBER  { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::maxPower, $2, $4); }
    |     MAXPOWER RANGESYMBOL WF_NUMBER            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::maxPower, 0, $3); }
    |     MAXPOWER WF_NUMBER RANGESYMBOL            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::maxPower, $2); }
    ;

avgpower: AVGPOWER WF_NUMBER                        { $$ = new ModelNumberEqualFilter(TdbWorkoutModelIdx::avgPower, $2, 5); }
    |     AVGPOWER WF_NUMBER RANGESYMBOL WF_NUMBER  { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::avgPower, $2, $4); }
    |     AVGPOWER RANGESYMBOL WF_NUMBER            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::avgPower, 0, $3); }
    |     AVGPOWER WF_NUMBER RANGESYMBOL            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::avgPower, $2); }
    ;

isopower: ISOPOWER WF_NUMBER                        { $$ = new ModelNumberEqualFilter(TdbWorkoutModelIdx::isoPower, $2, 5); }
    |     ISOPOWER WF_NUMBER RANGESYMBOL WF_NUMBER  { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::isoPower, $2, $4); }
    |     ISOPOWER RANGESYMBOL WF_NUMBER            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::isoPower, 0, $3); }
    |     ISOPOWER WF_NUMBER RANGESYMBOL            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::isoPower, $2); }
    ;

power: POWER WF_NUMBER RANGESYMBOL WF_NUMBER  { $$ = new std::pair<ModelFilter*, ModelFilter*>(new ModelNumberRangeFilter(TdbWorkoutModelIdx::minPower, $2, $4),
                                                                                               new ModelNumberRangeFilter(TdbWorkoutModelIdx::maxPower, $2, $4)); }
    |  POWER RANGESYMBOL WF_NUMBER            { $$ = new std::pair<ModelFilter*, ModelFilter*>(new ModelNumberRangeFilter(TdbWorkoutModelIdx::minPower, 0, $3),
                                                                                               new ModelNumberRangeFilter(TdbWorkoutModelIdx::maxPower, 0, $3)); }
    |  POWER WF_NUMBER RANGESYMBOL            { $$ = new std::pair<ModelFilter*, ModelFilter*>(new ModelNumberRangeFilter(TdbWorkoutModelIdx::minPower, $2),
                                                                                               new ModelNumberRangeFilter(TdbWorkoutModelIdx::maxPower, $2)); }

lastrun: LASTRUN DAYS RANGESYMBOL DAYS  { long now = QDateTime::currentDateTime().toSecsSinceEpoch();
                                          $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::lastRun, now - $4 * 86400, now - $2 * 86400); }
    |    LASTRUN RANGESYMBOL DAYS       { long now = QDateTime::currentDateTime().toSecsSinceEpoch();
                                          $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::lastRun, now - $3 * 86400, now); }
    |    LASTRUN DAYS RANGESYMBOL       { long now = QDateTime::currentDateTime().toSecsSinceEpoch();
                                          $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::lastRun, 0, now - $2 * 86400); }
    ;

created: CREATED DAYS RANGESYMBOL DAYS  { long now = QDateTime::currentDateTime().toSecsSinceEpoch();
                                          $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::created, now - $4 * 86400, now - $2 * 86400); }
    |    CREATED RANGESYMBOL DAYS       { long now = QDateTime::currentDateTime().toSecsSinceEpoch();
                                          $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::created, now - $3 * 86400, now); }
    |    CREATED DAYS RANGESYMBOL       { long now = QDateTime::currentDateTime().toSecsSinceEpoch();
                                          $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::created, 0, now - $2 * 86400); }
    ;

rating: RATING WF_NUMBER                        { $$ = new ModelNumberEqualFilter(TdbWorkoutModelIdx::rating, $2); }
    |   RATING WF_NUMBER RANGESYMBOL WF_NUMBER  { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::rating, $2, $4); }
    |   RATING RANGESYMBOL WF_NUMBER            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::rating, 1, $3); }
    |   RATING WF_NUMBER RANGESYMBOL            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::rating, $2, 5); }
    ;

distance: DISTANCE WF_NUMBER                        { $$ = new ModelNumberEqualFilter(TdbWorkoutModelIdx::distance, $2 * 1000, 5000); }
    |     DISTANCE WF_NUMBER RANGESYMBOL WF_NUMBER  { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::distance, $2 * 1000, $4 * 1000); }
    |     DISTANCE RANGESYMBOL WF_NUMBER            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::distance, 0, $3 * 1000); }
    |     DISTANCE WF_NUMBER RANGESYMBOL            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::distance, $2 * 1000); }
    ;

elevation: ELEVATION WF_NUMBER                        { $$ = new ModelNumberEqualFilter(TdbWorkoutModelIdx::elevation, $2, 5); }
    |      ELEVATION WF_NUMBER RANGESYMBOL WF_NUMBER  { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::elevation, $2, $4); }
    |      ELEVATION RANGESYMBOL WF_NUMBER            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::elevation, 0, $3); }
    |      ELEVATION WF_NUMBER RANGESYMBOL            { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::elevation, $2); }
    ;

grade: GRADE PERCENT                      { $$ = new ModelNumberEqualFilter(TdbWorkoutModelIdx::avgGrade, $2, 5); }
    |  GRADE PERCENT RANGESYMBOL PERCENT  { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::avgGrade, $2, $4); }
    |  GRADE RANGESYMBOL PERCENT          { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::avgGrade, 0, $3); }
    |  GRADE PERCENT RANGESYMBOL          { $$ = new ModelNumberRangeFilter(TdbWorkoutModelIdx::avgGrade, $2); }
    ;

filepath: FILEPATH word  { $$ = new ModelStringContainsFilter(TdbWorkoutModelIdx::filepath, *$2); }
    ;

words: word words  { *$1 << *$2; delete $2; $$ = $1; }
    |  word        { $$ = $1; }
    ;

word: WF_WORD    { $$ = new QStringList($1); }
    | WF_NUMBER  { $$ = new QStringList(QString::number($1)); }
    ;

mixedNumValue: WF_FLOAT   { $$ = $1; }
    |          WF_NUMBER  { $$ = $1; }
    ;

minuteValue: WF_NUMBER  { $$ = $1; }
    |        TIME       { $$ = $1; }
    ;

%%

void yyerror(const char *s)
{
    workoutModelErrorMsg = QString(s);
}
