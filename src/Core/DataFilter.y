%{
/*
 * Copyright (c) 2012-2016 Mark Liversedge (liversedge@gmail.com)
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
%token <leaf> SYMBOL PYTHON

// Constants can be a string or a number
%token <leaf> DF_STRING DF_INTEGER DF_FLOAT
%token <function> BEST TIZ CONFIG CONST_

// comparative operators
%token <op> IF_ ELSE_ WHILE
%token <op> EQ NEQ LT LTE GT GTE ELVIS ASSIGN
%token <op> ADD SUBTRACT DIVIDE MULTIPLY POW
%token <op> MATCHES ENDSWITH BEGINSWITH CONTAINS
%type <op> AND OR;

%union {
   Leaf *leaf;
   QList<Leaf*> *comp;
   int op;
   char function[32];
}

%locations

%type <leaf> symbol array select literal lexpr cexpr expr parms block statement expression parameter;
%type <leaf> simple_statement if_clause while_clause function_def;
%type <leaf> python_script;
%type <comp> statements

%right '?' ':'
%right AND OR
%right EQ NEQ LT LTE GT GTE MATCHES ENDSWITH CONTAINS
%left ADD SUBTRACT
%left MULTIPLY DIVIDE
%right POW
%right '[' ']'

%start filter;
%%

/* a one line filter needs to be accounted for as well as a
   complex filter comprised of a block, this is an anomaly
   caused by using the same code for filtering e.g. BikeStress > 100
   as well as custom metric code involving variables, if and
   while clauses etc

   As a result the 'filter' needs to explicitly list every
   type of clause that could be used rather than reference
   a 'statement', since we don't want to force the user to
   add a semi-colon to one line filters and formulas. This
   is particularly important since this was all that was
   possible before 3.3
*/

filter: 

        simple_statement                        { DataFilterroot = $1; }
        | if_clause                             { DataFilterroot = $1; }
        | while_clause                          { DataFilterroot = $1; }
        | block                                 { DataFilterroot = $1; }
        ;

/*
 * A one line statement or a block of code. These are evaluated
 * independently or as part of an if or while clause.
 */
expression: 

        statement                               { $$ = $1; }
        | block                                 { $$ = $1; }
        ;

/*
 * A block of code within braces.
 */
block: 

        '{' statements '}'                      { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Compound;
                                                  $$->lvalue.b = $2;
                                                }
        ;

/*
 * A set of lines of code
 */
statements: 

        statement                               { $$ = new QList<Leaf*>(); $$->append($1); }
        | statements statement                  { $$->append($2); }
        ;

/*
 * A single line of code, terminated with ';' if needed.
 */
statement: 

        simple_statement ';'
        | if_clause
        | while_clause
        | function_def
        ;

/*
 * A single line of code unterminated to use as a one-line filter
 */
simple_statement: 

        lexpr                                   { $$ = $1; }
        | symbol ASSIGN lexpr                   {
                                                  $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Operation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | array ASSIGN lexpr                    {
                                                  $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Operation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        ;

python_script:

        PYTHON                                  { $$ = new Leaf(@1.first_column, @1.last_column);
                                                  $$->type = Leaf::Script;
                                                  $$->function = "python";
                                                  QString full(DataFiltertext);
                                                  $$->lvalue.s = new QString(full.mid(8,full.length()-10));
                                                }
        ;

/*
 * if then else clause AST is same as a ternary
 * dangling else handled by bison default shift
 */
if_clause: 

        IF_ '(' lexpr ')' expression            { $$ = new Leaf(@1.first_column, @5.last_column);
                                                  $$->type = Leaf::Conditional;
                                                  $$->op = IF_;
                                                  $$->lvalue.l = $5;
                                                  $$->rvalue.l = NULL;
                                                  $$->cond.l = $3;
                                                }
        | IF_ '(' lexpr ')' expression 
          ELSE_ expression                      { $$ = new Leaf(@1.first_column, @5.last_column);
                                                  $$->type = Leaf::Conditional;
                                                  $$->op = IF_; 
                                                  $$->lvalue.l = $5;
                                                  $$->rvalue.l = $7;
                                                  $$->cond.l = $3;
                                                }

        ;

/*
 * While clause, handled similarly to if/else
 */
while_clause:

        WHILE '(' lexpr ')' expression          { $$ = new Leaf(@1.first_column, @5.last_column);
                                                  $$->type = Leaf::Conditional;
                                                  $$->op = WHILE; 
                                                  $$->lvalue.l = $5;
                                                  $$->rvalue.l = NULL;
                                                  $$->cond.l = $3;
                                                }
        ;

/*
 * A user defined function which may be a function that is called by
 * the user metric framework; e.g. init, sample, value, count etc
 */

function_def:

        symbol block                            { $$ = $2; 
                                                  $$->function = *($1->lvalue.n);
                                                }
        ;

parameter:

        lexpr                                   { $$ = $1; }
        | block                                 { $$ = $1; }
        ;

/*
 * A parameter list, as passed to a function
 */
parms: 

        parameter                               { $$ = new Leaf(@1.first_column, @1.last_column);
                                                  $$->type = Leaf::Function;
                                                  $$->series = NULL; // not tiz/best
                                                  $$->fparms << $1;
                                                }
        | parms ',' parameter                   { $1->fparms << $3;
                                                  $1->leng = @3.last_column; }
        ;

/*
 * A logical expression that is ultimately used to evaluate to bool
 */
lexpr:

        expr                                    { $$ = $1; }
        | cexpr                                 { $$ = $1; }
        | '(' lexpr ')'                         { $$ = new Leaf(@2.first_column, @2.last_column);
                                                  $$->type = Leaf::Logical;
                                                  $$->lvalue.l = $2;
                                                  $$->op = 0;
                                                }
        | lexpr '?' lexpr ':' lexpr             { $$ = new Leaf(@1.first_column, @5.last_column);
                                                  $$->type = Leaf::Conditional;
                                                  $$->op = 0; // zero for tertiary
                                                  $$->lvalue.l = $3;
                                                  $$->rvalue.l = $5;
                                                  $$->cond.l = $1;
                                                }
        | '!' lexpr %prec OR                    { $$ = new Leaf(@1.first_column, @2.last_column);
                                                  $$->type = Leaf::UnaryOperation;
                                                  $$->lvalue.l = $2;
                                                  $$->op = '!';
                                                  $$->rvalue.l = NULL;
                                                }
        | lexpr OR lexpr                        { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Logical;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | lexpr AND lexpr                       { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Logical;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        ;

array:
         expr '[' expr ']'                    {
                                                  $$ = new Leaf(@1.first_column, @4.last_column);
                                                  $$->type = Leaf::Index;
                                                  $$->lvalue.l = $1;
                                                  $$->fparms << $3;
                                                  $$->op = 0;
                                                }
        ;

select: expr '[' lexpr ']'                   {
                                                  $$ = new Leaf(@1.first_column, @4.last_column);
                                                  $$->type = Leaf::Select;
                                                  $$->lvalue.l = $1;
                                                  $$->fparms << $3;
                                                  $$->op = 0;
                                             }
        ;

/*
 * A compare expression evaluates to bool
 */
cexpr:

        expr EQ expr                            { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Operation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | expr NEQ expr                         { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Operation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | expr LT expr                          { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Operation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }

        | expr LTE expr                         { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Operation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | expr GT expr                          { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Operation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }

        | expr GTE expr                         { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Operation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | expr ELVIS expr                       { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Operation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | expr MATCHES expr                     { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Operation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | expr ENDSWITH expr                    { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Operation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | expr BEGINSWITH expr                  { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Operation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | expr CONTAINS expr                    { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::Operation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        ;

/*
 * An arithmetic expression, includes function calls and similar
 */
expr:

        expr SUBTRACT expr                      { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::BinaryOperation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | expr ADD expr                         { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::BinaryOperation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | expr DIVIDE expr                      { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::BinaryOperation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | expr MULTIPLY expr                    { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::BinaryOperation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | expr POW expr                         { $$ = new Leaf(@1.first_column, @3.last_column);
                                                  $$->type = Leaf::BinaryOperation;
                                                  $$->lvalue.l = $1;
                                                  $$->op = $2;
                                                  $$->rvalue.l = $3;
                                                }
        | SUBTRACT expr %prec MULTIPLY          { $$ = new Leaf(@1.first_column, @2.last_column);
                                                  $$->type = Leaf::UnaryOperation;
                                                  $$->lvalue.l = $2;
                                                  $$->op = '-';
                                                  $$->rvalue.l = NULL;
                                                }
        | BEST '(' symbol ',' lexpr ')'         { $$ = new Leaf(@1.first_column, @6.last_column); $$->type = Leaf::Function;
                                                  $$->function = QString($1);
                                                  $$->series = $3;
                                                  $$->lvalue.l = $5;
                                                }
        | TIZ '(' symbol ',' lexpr ')'          { $$ = new Leaf(@1.first_column, @6.last_column); $$->type = Leaf::Function;
                                                  $$->function = QString($1);
                                                  $$->series = $3;
                                                  $$->lvalue.l = $5;
                                                }
        | CONFIG '(' symbol ')'                 { $$ = new Leaf(@1.first_column, @4.last_column); $$->type = Leaf::Function;
                                                  $$->function = QString($1);
                                                  $$->series = $3;
                                                  $$->lvalue.l = NULL;
                                                }
        | CONST_ '(' symbol ')'                 { $$ = new Leaf(@1.first_column, @4.last_column); $$->type = Leaf::Function;
                                                  $$->function = QString($1);
                                                  $$->series = $3;
                                                  $$->lvalue.l = NULL;
                                                }
                                                  /* functions all have zero or more parameters */
        | symbol '(' parms ')'                  { /* need to convert params to a function */
                                                  $3->loc = @1.first_column;
                                                  $3->leng = @4.last_column;
                                                  $3->function = *($1->lvalue.n);
                                                  $$ = $3;
                                                }
        | symbol '(' ')'                        { /* need to convert symbol to function */
                                                  $1->type = Leaf::Function;
                                                  $1->series = NULL; // not tiz/best
                                                  $1->function = *($1->lvalue.n);
                                                  $1->fparms.clear(); // no parameters!
                                                }
        | '(' expr ')'                          { $$ = new Leaf(@2.first_column, @2.last_column);
                                                  $$->type = Leaf::Logical;
                                                  $$->lvalue.l = $2;
                                                  $$->op = 0;
                                                }
        | array                                 { $$ = $1; }
        | select                                { $$ = $1; }
        | literal                               { $$ = $1; }
        | symbol                                { $$ = $1; }
        | python_script                         { $$ = $1; }
        ;

/*
 * A built in or user defined symbol
 */
symbol:
        '$' SYMBOL                                { $$ = new Leaf(@1.first_column, @2.last_column);
                                                    $$->type = Leaf::Symbol;
                                                    $$->op = 1; // prompted variable (from user)
                                                    $$->lvalue.n = new QString(DataFiltertext);
                                                  }
        | SYMBOL                                  { $$ = new Leaf(@1.first_column, @1.last_column);
                                                    $$->type = Leaf::Symbol;
                                                    $$->op = 0; // metric or builtin reference
                                                    if (QString(DataFiltertext) == "BikeScore") $$->lvalue.n = new QString("BikeScore&#8482;");
                                                    else $$->lvalue.n = new QString(DataFiltertext);
                                                  }
        ;

/*
 * Any string or numeric literal
 */
literal:

        DF_STRING                               { $$ = new Leaf(@1.first_column, @1.last_column);
                                                  $$->type = Leaf::String;
                                                  QString s2 = Utils::unescape(DataFiltertext); // user can escape chars in string
                                                  $$->lvalue.s = new QString(s2.mid(1,s2.length()-2));
                                                }
        | DF_FLOAT                              { $$ = new Leaf(@1.first_column, @1.last_column);
                                                  $$->type = Leaf::Float;
                                                  $$->lvalue.f = QString(DataFiltertext).toFloat();
                                                }
        | DF_INTEGER                            { $$ = new Leaf(@1.first_column, @1.last_column);
                                                  $$->type = Leaf::Integer;
                                                  $$->lvalue.i = QString(DataFiltertext).toInt();
                                                }
      ;

%%

