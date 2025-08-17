%top{
  #include <stdint.h>
}

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

#include <QString>
#include <QDebug>

#include "ModelFilter.h"
#include "WorkoutFilter_yacc.h"

extern YYSTYPE yylval;
%}
%option nounput
%option noinput
%option noyywrap
%option 8bit
%option yylineno
%option prefix="WorkoutFilter"
%option never-interactive

%%
\.\. |
-                    { return RANGESYMBOL; }
[0-9]+%              { yytext[strlen(yytext) - 1] = '\0'; WorkoutFilterlval.numValue = atoi(yytext); return PERCENT; }
[0-9]+d              { yytext[strlen(yytext) - 1] = '\0'; WorkoutFilterlval.numValue = atoi(yytext); return DAYS; }
[0-9]+w              { yytext[strlen(yytext) - 1] = '\0'; WorkoutFilterlval.numValue = atoi(yytext) * 7; return DAYS; }
[0-9]+m              { yytext[strlen(yytext) - 1] = '\0'; WorkoutFilterlval.numValue = atoi(yytext) * 30; return DAYS; }
[0-9]+y              { yytext[strlen(yytext) - 1] = '\0'; WorkoutFilterlval.numValue = atoi(yytext) * 365; return DAYS; }
[0-9]+:[0-6][0-9]    { int minutes = atoi(yytext + strlen(yytext) - 2); yytext[strlen(yytext) - 2] = '\0'; WorkoutFilterlval.numValue = atoi(yytext) * 60 + minutes; return TIME; }
[0-9]+\.[0-9]+       { WorkoutFilterlval.floatValue = QString(yytext).toFloat(); return FLOAT; }
[0-9]+               { WorkoutFilterlval.numValue = atoi(yytext); return NUMBER; }

[ \t]                ;   /* ignore white space */

[Dd][Uu][Rr][Aa][Tt][Ii][Oo][Nn]                   { return DURATION; }
[Dd][Oo][Mm][Ii][Nn][Aa][Nn][Tt][Zz][Oo][Nn][Ee]   { return DOMINANTZONE; }
[Ss][Tt][Rr][Ee][Ss][Ss]                           { return STRESS; }
[Ii][Nn][Tt][Ee][Nn][Ss][Ii][Tt][Yy]               { return INTENSITY; }
[Vv][Aa][Rr][Ii][Aa][Bb][Ii][Ll][Ii][Tt][Yy]       { return VI; }
[Rr][Ii]                                           { return RI; }
[Bb][Ii][Kk][Ee][Ss][Cc][Oo][Rr][Ee]               { return BIKESCORE; }
[Ss][Vv][Ii]                                       { return SVI; }
[Rr][Aa][Tt][Ii][Nn][Gg]                           { return RATING; }
[Mm][Ii][Nn][Pp][Oo][Ww][Ee][Rr]                   { return MINPOWER; }
[Mm][Aa][Xx][Pp][Oo][Ww][Ee][Rr]                   { return MAXPOWER; }
[Aa][Vv][Gg][Pp][Oo][Ww][Ee][Rr]                   { return AVGPOWER; }
[Xx][Pp][Oo][Ww][Ee][Rr]                           { return XPOWER; }
[Ii][Ss][Oo][Pp][Oo][Ww][Ee][Rr]                   { return ISOPOWER; }
[Pp][Oo][Ww][Ee][Rr]                               { return POWER; }
[Ll][Aa][Ss][Tt][Rr][Uu][Nn]                       { return LASTRUN; }
[Cc][Rr][Ee][Aa][Tt][Ee][Dd]                       { return CREATED; }
[Dd][Ii][Ss][Tt][Aa][Nn][Cc][Ee]                   { return DISTANCE; }
[Ee][Ll][Ee][Vv][Aa][Tt][Ii][Oo][Nn]               { return ELEVATION; }
[Gg][Rr][Aa][Dd][Ee]                               { return GRADE; }
[Ff][Ii][Ll][Ee]                                   { return FILEPATH; }
[zZ]([1-9]|10)                                     { yytext += 1; WorkoutFilterlval.numValue = atoi(yytext); return ZONE; }
\"[^\"]+\"                                         { yytext[strlen(yytext) - 1] = '\0'; WorkoutFilterlval.word = yytext + 1; return WORD; }
[^ ,.]+                                            { WorkoutFilterlval.word = yytext; return WORD; }
.                                                  { return yytext[0]; }

%%


void WorkoutFilter_setString(QString p)
{
    BEGIN(0);
    WorkoutFilter_scan_string(p.toUtf8().data());
}
