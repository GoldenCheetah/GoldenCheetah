%{
/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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

%}

// Symbol can be meta or metric name
%token <leaf> SYMBOL

// Constants can be a string or a number
%token <leaf> DF_STRING DF_INTEGER DF_FLOAT
%token <function> BEST TIZ CONFIG CONST_ DATERANGE

// comparative operators
%token <op> EQ NEQ LT LTE GT GTE
%token <op> ADD SUBTRACT DIVIDE MULTIPLY POW
%token <op> MATCHES ENDSWITH BEGINSWITH CONTAINS
%type <op> AND OR;

%union {
   Leaf *leaf;
   int op;
   char function[32];
}

%locations

%type <leaf> symbol literal lexpr cexpr expr parms;

%right '?' ':'
%right '[' ']'
%right AND OR
%right EQ NEQ LT LTE GT GTE MATCHES ENDSWITH CONTAINS
%left ADD SUBTRACT
%left MULTIPLY DIVIDE
%right POW

%start filter;
%%

filter: lexpr                       { DataFilterroot = $1; }
        ;

parms: lexpr                        { $$ = new Leaf(@1.first_column, @1.last_column);
                                      $$->type = Leaf::Parameters;
                                      $$->fparms << $1;
                                    }

        | parms ',' lexpr           { $1->fparms << $3; }
        ;

lexpr   : expr                       { $$ = $1; }

        | cexpr                      { $$ = $1; }

        | '(' lexpr ')'               { $$ = new Leaf(@2.first_column, @2.last_column);
                                      $$->type = Leaf::Logical;
                                      $$->lvalue.l = $2;
                                      $$->op = 0; }

        | lexpr '?' lexpr ':' lexpr      { $$ = new Leaf(@1.first_column, @5.last_column);
                                    $$->type = Leaf::Conditional;
                                    $$->op = 0; // unused
                                    $$->lvalue.l = $3;
                                    $$->rvalue.l = $5;
                                    $$->cond.l = $1;
                                  }

        | lexpr '[' lexpr ':' lexpr ']' { $$ = new Leaf(@1.first_column, @6.last_column);
                                          $$->type = Leaf::Vector;
                                          $$->lvalue.l = $1;
                                          $$->fparms << $3;
                                          $$->fparms << $5;
                                          $$->op = 0; }

        | lexpr OR lexpr             { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::Logical;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }
        | lexpr AND lexpr             { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::Logical;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }

        ;

cexpr   : expr EQ expr              { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::Operation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }

        | expr NEQ expr             { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::Operation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }

        | expr LT expr              { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::Operation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }

        | expr LTE expr             { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::Operation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }

        | expr GT expr              { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::Operation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }

        | expr GTE expr             { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::Operation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }

        | expr MATCHES expr         { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::Operation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }

        | expr ENDSWITH expr        { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::Operation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }

        | expr BEGINSWITH expr      { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::Operation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }

        | expr CONTAINS expr        { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::Operation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }

        ;

expr : expr SUBTRACT expr              { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::BinaryOperation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }
      | expr ADD expr              { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::BinaryOperation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }
      | expr DIVIDE expr              { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::BinaryOperation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }
      | expr MULTIPLY expr              { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::BinaryOperation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }
      | expr POW expr              { $$ = new Leaf(@1.first_column, @3.last_column);
                                      $$->type = Leaf::BinaryOperation;
                                      $$->lvalue.l = $1;
                                      $$->op = $2;
                                      $$->rvalue.l = $3; }
      | SUBTRACT expr %prec MULTIPLY { $$ = new Leaf(@1.first_column, @2.last_column);
                                      $$->type = Leaf::UnaryOperation;
                                      $$->lvalue.l = $2;
                                      $$->op = $1;
                                      $$->rvalue.l = NULL; 
                                     }


      | BEST '(' symbol ',' lexpr ')' { $$ = new Leaf(@1.first_column, @6.last_column); $$->type = Leaf::Function;
                                        $$->function = QString($1);
                                        $$->series = $3;
                                        $$->lvalue.l = $5;
                                      }


      | TIZ '(' symbol ',' lexpr ')' { $$ = new Leaf(@1.first_column, @6.last_column); $$->type = Leaf::Function;
                                        $$->function = QString($1);
                                        $$->series = $3;
                                        $$->lvalue.l = $5;
                                      }

      | CONFIG '(' symbol ')'       {   $$ = new Leaf(@1.first_column, @4.last_column); $$->type = Leaf::Function;
                                        $$->function = QString($1);
                                        $$->series = $3;
                                        $$->lvalue.l = NULL;
                                      }
      | CONST_ '(' symbol ')'       {   $$ = new Leaf(@1.first_column, @4.last_column); $$->type = Leaf::Function;
                                        $$->function = QString($1);
                                        $$->series = $3;
                                        $$->lvalue.l = NULL;
                                      }
      | DATERANGE '(' symbol ')'       {   $$ = new Leaf(@1.first_column, @4.last_column); $$->type = Leaf::Function;
                                        $$->function = QString($1);
                                        $$->series = $3;
                                        $$->lvalue.l = NULL;
                                      }

                                    /* functions all have zero or more parameters */

      | symbol '(' parms ')'    { /* need to convert symbol to a function */
                                  $1->leng = @4.last_column;
                                  $1->type = Leaf::Function;
                                  $1->series = NULL; // not tiz/best
                                  $1->function = *($1->lvalue.n);
                                  $1->fparms = $3->fparms;
                                }

      | symbol '(' ')'          {
                                  /* need to convert symbol to function */
                                  $1->type = Leaf::Function;
                                  $1->series = NULL; // not tiz/best
                                  $1->function = *($1->lvalue.n);
                                  $1->fparms.clear(); // no parameters!
                                }

      | '(' expr ')'               { $$ = new Leaf(@2.first_column, @2.last_column);
                                      $$->type = Leaf::Logical;
                                      $$->lvalue.l = $2;
                                      $$->op = 0; }

      | symbol                  { $$ = $1; }

      | literal                        { $$ = $1; }

      ;

symbol : SYMBOL                      { $$ = new Leaf(@1.first_column, @1.last_column); 
                                       $$->type = Leaf::Symbol;
                                       if (QString(DataFiltertext) == "BikeScore")
                                          $$->lvalue.n = new QString("BikeScore&#8482;");
                                       else
                                          $$->lvalue.n = new QString(DataFiltertext);
                                     }
        ;

literal : DF_STRING                      { $$ = new Leaf(@1.first_column, @1.last_column); $$->type = Leaf::String;
                                      QString s2(DataFiltertext);
                                      $$->lvalue.s = new QString(s2.mid(1,s2.length()-2)); }
      | DF_FLOAT                       { $$ = new Leaf(@1.first_column, @1.last_column); $$->type = Leaf::Float;
                                      $$->lvalue.f = QString(DataFiltertext).toFloat(); }
      | DF_INTEGER                     { $$ = new Leaf(@1.first_column, @1.last_column); $$->type = Leaf::Integer;
                                      $$->lvalue.i = QString(DataFiltertext).toInt(); }

      ;

%%

