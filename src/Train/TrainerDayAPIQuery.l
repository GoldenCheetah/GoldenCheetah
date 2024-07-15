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

#include <QString>
#include <QDebug>

#include "TrainerDayAPIQuery.h"
#include "TrainerDayAPIQuery_yacc.h"

extern YYSTYPE yylval;
%}
%option nounput
%option noinput
%option noyywrap
%option 8bit
%option yylineno
%option prefix="TrainerDayAPIQuery"
%option never-interactive

%%
\.\. |
-                    { return RANGESYMBOL; }
[0-9]+:[0-6][0-9]    { int minutes = atoi(yytext + strlen(yytext) - 2); yytext[strlen(yytext) - 2] = '\0'; TrainerDayAPIQuerylval.numValue = atoi(yytext) * 60 + minutes; return TIME; }
[0-9]+               { TrainerDayAPIQuerylval.numValue = atoi(yytext); return NUMBER; }

[ \t]                ;   /* ignore white space */

[Dd][Uu][Rr][Aa][Tt][Ii][Oo][Nn]                   { return DURATION; }
[Dd][Oo][Mm][Ii][Nn][Aa][Nn][Tt][Zz][Oo][Nn][Ee]   { return DOMINANTZONE; }
[zZ][1-7]                                          { yytext += 1; TrainerDayAPIQuerylval.numValue = atoi(yytext); return ZONE; }
[Rr][Ee][Cc][Oo][Vv][Ee][Rr][Yy]                   { return ZONE_RECOVERY; }
[Ee][Nn][Dd][Uu][Rr][Aa][Nn][Cc][Ee]               { return ZONE_ENDURANCE; }
[Tt][Ee][Mm][Pp][Oo]                               { return ZONE_TEMPO; }
[Tt][Hh][Rr][Ee][Ss][Hh][Oo][Ll][Dd]               { return ZONE_THRESHOLD; }
[Vv][Oo][2][Mm][Aa][Xx]                            { return ZONE_VO2MAX; }
[Aa][Nn][Aa][Ee][Rr][Oo][Bb][Ii][Cc]               { return ZONE_ANAEROBIC; }
[Tt][Ee][Ss][Tt]                                   { return ZONE_TEST; }
[^ ,.]+                                            { TrainerDayAPIQuerylval.word = yytext; return WORD; }
.                                                  { return yytext[0]; }

%%


void TrainerDayAPIQuery_setString(QString p)
{
    BEGIN(0);
    TrainerDayAPIQuery_scan_string(p.toUtf8().data());
}
