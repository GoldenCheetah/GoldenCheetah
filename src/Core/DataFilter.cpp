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

#include "DataFilter.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "RideNavigator.h"
#include "RideFileCache.h"
#include "PMCData.h"
#include "Banister.h"
#include "LTMSettings.h"
#include "VDOTCalculator.h"
#include "DataProcessor.h"
#include <QDebug>
#include <QMutex>

#ifdef GC_WANT_PYTHON
#include "PythonEmbed.h"
QMutex pythonMutex;
#endif

#include "Zones.h"
#include "PaceZones.h"
#include "HrZones.h"

#include "DataFilter_yacc.h"

// control access to runtimes to avoid calls from multiple threads

// v4 functions
static struct {

    QString name;
    int parameters; // -1 is end of list, 0 is variable number, >0 is number of parms needed

} DataFilterFunctions[] = {

    // ALWAYS ADD TO BOTTOM OF LIST AS ARRAY OFFSET
    // USED IN SWITCH STATEMENT AT RUNTIME DO NOT BE
    // TEMPTED TO ADD IN THE MIDDLE !!!!

    // math.h
    { "cos", 1 },
    { "tan", 1 },
    { "sin", 1 },
    { "acos", 1 },
    { "atan", 1 },
    { "asin", 1 },
    { "cosh", 1 },
    { "tanh", 1 },
    { "sinh", 1 },
    { "acosh", 1 },
    { "atanh", 1 },
    { "asinh", 1 },

    { "exp", 1 },
    { "log", 1 },
    { "log10", 1 },

    { "ceil", 1 },
    { "floor", 1 },
    { "round", 1 },

    { "fabs", 1 },
    { "isinf", 1 },
    { "isnan", 1 },

    // primarily for working with vectors, but can
    // have variable number of parameters so probably
    // quite useful for a number of things
    { "sum", 0 },
    { "mean", 0 },
    { "max", 0 },
    { "min", 0 },
    { "count", 0 },

    // PMC functions
    { "lts", 1 },
    { "sts", 1 },
    { "sb", 1 },
    { "rr", 1 },

    // estimate
    { "estimate", 2 }, // estimate(model, (cp|ftp|w'|pmax|x))

    // more vector operations
    { "which", 0 }, // which(expr, ...) - create vector contain values that pass expr

    // set
    { "set", 3 }, // set(symbol, value, filter)
    { "unset", 2 }, // unset(symbol, filter)
    { "isset", 1 }, // isset(symbol) - is the metric or metadata overridden/defined

    // VDOT and time/distance functions
    { "vdottime", 2 }, // vdottime(VDOT, distance[km]) - result is seconds
    { "besttime", 1 }, // besttime(distance[km]) - result is seconds

    // XDATA access
    { "XDATA", 3 },     // e.g. xdata("WEATHER","HUMIDITY", repeat|sparse|interpolate |resample)

    // print to qDebug for debugging
    { "print", 1 },     // print(..) to qDebug for debugging

    // Data Processor functions on filtered activiies
    { "autoprocess", 1 }, // autoprocess(filter) to run auto data processors
    { "postprocess", 2 }, // postprocess(processor, filter) to run processor

    // XDATA access
    { "XDATA_UNITS", 2 }, // e.g. xdata("WEATHER", "HUMIDITY") returns "Relative Humidity"

    // Daily Measures Access by date
    { "measure", 3 },   // measure(DATE, "Hrv", "RMSSD")

    // how many performance tests in the ride?
    { "tests", 0 },

    // banister function
    { "banister", 2 }, // banister(metric, nte|pte|perf|cp)

    // add new ones above this line
    { "", -1 }
};

static QStringList pdmodels()
{
    QStringList returning;

    returning << "2p";
    returning << "3p";
    returning << "ext";
    returning << "ws";
    returning << "velo";

    return returning;
}

QStringList
DataFilter::builtins()
{
    QStringList returning;

    for(int i=0; DataFilterFunctions[i].parameters != -1; i++) {

        if (i == 30) { // special case 'estimate' we describe it

            foreach(QString model, pdmodels())
                returning << "estimate(" + model + ", x)";
        } else if (i == 31) { // which example
            returning << "which(x>0, ...)";

        } else if (i == 32) { // set example
            returning <<"set(field, value, expr)";
        } else if (i == 37) {
            returning << "XDATA(\"xdata\", \"series\", sparse|repeat|interpolate|resample)";
        } else if (i == 41) {
            returning << "XDATA_UNITS(\"xdata\", \"series\")";
        } else if (i == 42) {
            Measures measures = Measures();
            QStringList groupSymbols = measures.getGroupSymbols();
            for (int g=0; g<groupSymbols.count(); g++)
                foreach (QString fieldSymbol, measures.getFieldSymbols(g))
                    returning << QString("measure(Date, \"%1\", \"%2\")").arg(groupSymbols[g]).arg(fieldSymbol);
        } else if (i == 44) {
            // banister
            returning << "banister(metric, nte|pte|perf|cp)";
        } else {
            QString function;
            function = DataFilterFunctions[i].name + "(";
            for(int j=0; j<DataFilterFunctions[i].parameters; j++) {
                if (j) function += ", ";
                function += QString("p%1").arg(j+1);
            }
            if (DataFilterFunctions[i].parameters) function += ")";
            else function += "...)";
            returning << function;
        }
    }
    return returning;
}

// LEXER VARIABLES WE INTERACT WITH
// Standard yacc/lex variables / functions
extern int DataFilterlex(); // the lexer aka yylex()
extern char *DataFiltertext; // set by the lexer aka yytext

extern void DataFilter_setString(QString);
extern void DataFilter_clearString();

// PARSER STATE VARIABLES
QStringList DataFiltererrors;
extern int DataFilterparse();

Leaf *DataFilterroot; // root node for parsed statement

static RideFile::SeriesType nameToSeries(QString name)
{
    if (!name.compare("power", Qt::CaseInsensitive)) return RideFile::watts;
    if (!name.compare("apower", Qt::CaseInsensitive)) return RideFile::aPower;
    if (!name.compare("cadence", Qt::CaseInsensitive)) return RideFile::cad;
    if (!name.compare("hr", Qt::CaseInsensitive)) return RideFile::hr;
    if (!name.compare("speed", Qt::CaseInsensitive)) return RideFile::kph;
    if (!name.compare("torque", Qt::CaseInsensitive)) return RideFile::nm;
    if (!name.compare("IsoPower", Qt::CaseInsensitive)) return RideFile::IsoPower;
    if (!name.compare("xPower", Qt::CaseInsensitive)) return RideFile::xPower;
    if (!name.compare("VAM", Qt::CaseInsensitive)) return RideFile::vam;
    if (!name.compare("wpk", Qt::CaseInsensitive)) return RideFile::wattsKg;
    if (!name.compare("lrbalance", Qt::CaseInsensitive)) return RideFile::lrbalance;

    return RideFile::none;

}

bool
Leaf::isDynamic(Leaf *leaf)
{
    switch(leaf->type) {
    default:
    case Leaf::Symbol :
        return leaf->dynamic;
    case Leaf::UnaryOperation :
        return leaf->isDynamic(leaf->lvalue.l);
    case Leaf::Logical  :
        if (leaf->op == 0)
            return leaf->isDynamic(leaf->lvalue.l);
        // intentional fallthrough
    case Leaf::Operation :
    case Leaf::BinaryOperation :
        return (leaf->isDynamic(leaf->lvalue.l) ||
                leaf->isDynamic(leaf->rvalue.l));
    case Leaf::Function :
        if (leaf->series && leaf->lvalue.l)
            return leaf->isDynamic(leaf->lvalue.l);
        else
            return leaf->dynamic;
    case Leaf::Conditional :
        return (leaf->isDynamic(leaf->cond.l) ||
                leaf->isDynamic(leaf->lvalue.l) ||
                (leaf->rvalue.l && leaf->isDynamic(leaf->rvalue.l)));
    case Leaf::Script :
    case Leaf::Vector :
        return true;

    }
    return false;
}

void
DataFilter::setSignature(QString &query)
{
    sig = fingerprint(query);
}

QString
DataFilter::fingerprint(QString &query)
{
    QString sig;

    bool incomment=false;
    bool instring=false;

    for (int i=0; i<query.length(); i++) {

        // get out of comments and strings at end of a line
        if (query[i] == '\n') instring = incomment=false;

        // get into comments
        if (!instring && query[i] == '#') incomment=true;

        // not in comment and not escaped
        if (query[i] == '"' && !incomment && (((i && query[i-1] != '\\') || i==0))) instring = !instring;

        // keep anything that isn't whitespace, or a comment
        if (instring || (!incomment && !query[i].isSpace())) sig += query[i];
    }

    return sig;
}

void
DataFilter::colorSyntax(QTextDocument *document, int pos)
{
    // matched bracket position
    int bpos = -1;

    // matched brace position
    int cpos = -1;

    // for looking for comments
    QString string = document->toPlainText();

    // clear formatting and color comments
    QTextCharFormat normal;
    normal.setFontWeight(QFont::Normal);
    normal.setUnderlineStyle(QTextCharFormat::NoUnderline);
    normal.setForeground(Qt::black);

    QTextCharFormat cyanbg;
    cyanbg.setBackground(Qt::cyan);
    QTextCharFormat redbg;
    redbg.setBackground(QColor(255,153,153));

    QTextCharFormat function;
    function.setUnderlineStyle(QTextCharFormat::NoUnderline);
    function.setForeground(Qt::blue);

    QTextCharFormat symbol;
    symbol.setUnderlineStyle(QTextCharFormat::NoUnderline);
    symbol.setForeground(Qt::red);

    QTextCharFormat literal;
    literal.setFontWeight(QFont::Normal);
    literal.setUnderlineStyle(QTextCharFormat::NoUnderline);
    literal.setForeground(Qt::magenta);

    QTextCharFormat comment;
    comment.setFontWeight(QFont::Normal);
    comment.setUnderlineStyle(QTextCharFormat::NoUnderline);
    comment.setForeground(Qt::darkGreen);

    QTextCursor cursor(document);

    // set all black
    cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    cursor.selectionStart();
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.selectionEnd();
    cursor.setCharFormat(normal);

    // color comments
    bool instring=false;
    bool innumber=false;
    bool incomment=false;
    bool insymbol=false;
    int commentstart=0;
    int stringstart=0;
    int numberstart=0;
    int symbolstart=0;
    int brace=0;
    int brack=0;

    for(int i=0; i<string.length(); i++) {

        // enter into symbol
        if (!insymbol && !incomment && !instring && string[i].isLetter()) {
            insymbol = true;
            symbolstart = i;

            // it starts with numbers but ends with letters - number becomes symbol
            if (innumber) { symbolstart=numberstart; innumber=false; }
        }

        // end of symbol ?
        if (insymbol && (!string[i].isLetterOrNumber() && string[i] != '_')) {

            bool isfunction = false;
            insymbol = false;
            QString sym = string.mid(symbolstart, i-symbolstart);

            bool found=false;
            QString lookup = rt.lookupMap.value(sym, "");
            if (lookup == "") {

                // isRun isa special, we may add more later (e.g. date)
                if (!sym.compare("Date", Qt::CaseInsensitive) ||
                    !sym.compare("banister", Qt::CaseInsensitive) ||
                    !sym.compare("best", Qt::CaseInsensitive) ||
                    !sym.compare("tiz", Qt::CaseInsensitive) ||
                    !sym.compare("const", Qt::CaseInsensitive) ||
                    !sym.compare("config", Qt::CaseInsensitive) ||
                    !sym.compare("ctl", Qt::CaseInsensitive) ||
                    !sym.compare("tsb", Qt::CaseInsensitive) ||
                    !sym.compare("atl", Qt::CaseInsensitive) ||
                    !sym.compare("daterange", Qt::CaseInsensitive) ||
                    !sym.compare("Today", Qt::CaseInsensitive) ||
                    !sym.compare("Current", Qt::CaseInsensitive) ||
                    !sym.compare("RECINTSECS", Qt::CaseInsensitive) ||
                    !sym.compare("NA", Qt::CaseInsensitive) ||
                    sym == "isSwim" || sym == "isRun") {
                    isfunction = found = true;
                }

                // ride sample symbol
                if (rt.dataSeriesSymbols.contains(sym)) found = true;

                // still not found ?
                // is it a function then ?
                for(int j=0; DataFilterFunctions[j].parameters != -1; j++) {
                    if (DataFilterFunctions[j].name == sym) {
                        isfunction = found = true;
                        break;
                    }
                }
            } else {
                found = true;
            }

            if (found) {

                // lets color it red, its a literal.
                cursor.setPosition(symbolstart, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.setCharFormat(isfunction ? function : symbol);
            }
        }

        // numeric literal
        if (!insymbol && string[i].isNumber()) {

            // start of number ?
            if (!incomment && !instring && !innumber) {

                innumber = true;
                numberstart = i;

            }

        } else if (!insymbol && !incomment && !instring &&innumber) {

            // now out of number

            innumber = false;

            // lets color it red, its a literal.
            cursor.setPosition(numberstart, QTextCursor::MoveAnchor);
            cursor.selectionStart();
            cursor.setPosition(i, QTextCursor::KeepAnchor);
            cursor.selectionEnd();
            cursor.setCharFormat(literal);
        }

        // watch for being in a string, but remember escape!
        if ((i==0 || string[i-1] != '\\') && string[i] == '"') {

            if (!incomment && instring) {

                instring = false;

                // lets color it red, its a literal.
                cursor.setPosition(stringstart, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.setCharFormat(literal);

            } else if (!incomment && !instring) {

                stringstart=i;
                instring=true;
            }
        }

        // watch for being in a comment
        if (string[i] == '#' && !instring && !incomment) {
            incomment = true;
            commentstart=i;
        }

        // end of text or line
        if (i+1 == string.length() || (string[i] == '\r' || string[i] == '\n')) {

            // mark if we have a comment
            if (incomment) {

                // we have a line of comments to here from commentstart
                incomment = false;

                // comments are blue
                cursor.setPosition(commentstart, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.setCharFormat(comment);

            } else {

                int start=0;
                if (instring) start=stringstart;
                if (innumber) start=numberstart;

                // end of string ...
                if (instring || innumber) {

                    innumber = instring = false; // always quit on end of line

                    // lets color it red, its a literal.
                    cursor.setPosition(start, QTextCursor::MoveAnchor);
                    cursor.selectionStart();
                    cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                    cursor.selectionEnd();
                    cursor.setCharFormat(literal);
                }
            }
        }

        // are the brackets balanced  ( ) ?
        if (!instring && !incomment && string[i]=='(') {
            brack++;

            // match close/open if over cursor
            if (i==pos-1) {
                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(cyanbg);

                // run forward looking for match
                int bb=0;
                for(int j=i; j<string.length(); j++) {
                    if (string[j]=='(') bb++;
                    if (string[j]==')') {
                        bb--;
                        if (bb == 0) {
                            bpos = j; // matched brack here, don't change color!

                            cursor.setPosition(j, QTextCursor::MoveAnchor);
                            cursor.selectionStart();
                            cursor.setPosition(j+1, QTextCursor::KeepAnchor);
                            cursor.selectionEnd();
                            cursor.mergeCharFormat(cyanbg);
                            break;
                        }
                    }
                }
            }
        }
        if (!instring && !incomment && string[i]==')') {
            brack--;

            if (i==pos-1) {

                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(cyanbg);

                // run backward looking for match
                int bb=0;
                for(int j=i; j>=0; j--) {
                    if (string[j]==')') bb++;
                    if (string[j]=='(') {
                        bb--;
                        if (bb == 0) {
                            bpos = j; // matched brack here, don't change color!

                            cursor.setPosition(j, QTextCursor::MoveAnchor);
                            cursor.selectionStart();
                            cursor.setPosition(j+1, QTextCursor::KeepAnchor);
                            cursor.selectionEnd();
                            cursor.mergeCharFormat(cyanbg);
                            break;
                        }
                    }
                }

            } else if (brack < 0 && i != bpos-1) {

                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(redbg);
            }
        }

        // are the braces balanced  ( ) ?
        if (!instring && !incomment && string[i]=='{') {
            brace++;

            // match close/open if over cursor
            if (i==pos-1) {
                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(cyanbg);

                // run forward looking for match
                int bb=0;
                for(int j=i; j<string.length(); j++) {
                    if (string[j]=='{') bb++;
                    if (string[j]=='}') {
                        bb--;
                        if (bb == 0) {
                            cpos = j; // matched brace here, don't change color!

                            cursor.setPosition(j, QTextCursor::MoveAnchor);
                            cursor.selectionStart();
                            cursor.setPosition(j+1, QTextCursor::KeepAnchor);
                            cursor.selectionEnd();
                            cursor.mergeCharFormat(cyanbg);
                            break;
                        }
                    }
                }
            }
        }
        if (!instring && !incomment && string[i]=='}') {
            brace--;

            if (i==pos-1) {

                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(cyanbg);

                // run backward looking for match
                int bb=0;
                for(int j=i; j>=0; j--) {
                    if (string[j]=='}') bb++;
                    if (string[j]=='{') {
                        bb--;
                        if (bb == 0) {
                            cpos = j; // matched brace here, don't change color!

                            cursor.setPosition(j, QTextCursor::MoveAnchor);
                            cursor.selectionStart();
                            cursor.setPosition(j+1, QTextCursor::KeepAnchor);
                            cursor.selectionEnd();
                            cursor.mergeCharFormat(cyanbg);
                            break;
                        }
                    }
                }

            } else if (brace < 0 && i != cpos-1) {

                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(redbg);
            }
        }
    }

    // unbalanced braces - do same as above, backwards?
    //XXX braces in comments fuck things up ... XXX
    if (brace > 0) {
        brace = 0;
        for(int i=string.length(); i>=0; i--) {

            if (string[i] == '}') brace++;
            if (string[i] == '{') brace--;

            if (brace < 0 && string[i] == '{' && i != pos-1 && i != cpos-1) {
                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(redbg);
            }
        }
    }

    if (brack > 0) {
        brack = 0;
        for(int i=string.length(); i>=0; i--) {

            if (string[i] == ')') brace++;
            if (string[i] == '(') brace--;

            if (brack < 0 && string[i] == '(' && i != pos-1 && i != bpos-1) {
                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(redbg);
            }
        }
    }

    // apply selective coloring to the symbols and expressions
    if(treeRoot) treeRoot->color(treeRoot, document);
}

void Leaf::color(Leaf *leaf, QTextDocument *document)
{
    // nope
    if (leaf == NULL) return;

    QTextCharFormat apply;

    switch(leaf->type) {
    case Leaf::Float :
    case Leaf::Integer :
    case Leaf::String :
                    break;

    case Leaf::Symbol :
                    break;

    case Leaf::Logical  :
                    leaf->color(leaf->lvalue.l, document);
                    if (leaf->op) leaf->color(leaf->rvalue.l, document);
                    return;
                    break;

    case Leaf::Operation :
                    leaf->color(leaf->lvalue.l, document);
                    leaf->color(leaf->rvalue.l, document);
                    return;
                    break;

    case Leaf::UnaryOperation :
                    leaf->color(leaf->lvalue.l, document);
                    return;
                    break;
    case Leaf::BinaryOperation :
                    leaf->color(leaf->lvalue.l, document);
                    leaf->color(leaf->rvalue.l, document);
                    return;
                    break;

    case Leaf::Function :
                    if (leaf->series) {
                        if (leaf->lvalue.l) leaf->color(leaf->lvalue.l, document);
                        return;
                    } else {
                        foreach(Leaf*l, leaf->fparms) leaf->color(l, document);
                    }
                    break;

    case Leaf::Vector :
                    leaf->color(leaf->lvalue.l, document);
                    leaf->color(leaf->fparms[0], document);
                    leaf->color(leaf->fparms[1], document);
                    return;
                    break;

    case Leaf::Conditional :
        {
                    leaf->color(leaf->cond.l, document);
                    leaf->color(leaf->lvalue.l, document);
                    if (leaf->rvalue.l) leaf->color(leaf->rvalue.l, document);
                    return;
        }
        break;

    case Leaf::Compound :
        {
            foreach(Leaf *statement, *(leaf->lvalue.b)) leaf->color(statement, document);
            // don't return in case the whole thing is a mess...
        }
        break;

    default:
        return;
        break;

    }

    // all we do now is highlight if in error.
    if (leaf->inerror == true) {

        QTextCursor cursor(document);

        // highlight this token and apply
        cursor.setPosition(leaf->loc, QTextCursor::MoveAnchor);
        cursor.selectionStart();
        cursor.setPosition(leaf->leng+1, QTextCursor::KeepAnchor);
        cursor.selectionEnd();

        apply.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        apply.setUnderlineColor(Qt::red);

        cursor.mergeCharFormat(apply);
    }
}

// convert expression to string, without white space
// this can be used as a signature when caching values etc

QString
Leaf::toString()
{
    switch(type) {
    case Leaf::Script : return *lvalue.s;
    case Leaf::Float : return QString("%1").arg(lvalue.f); break;
    case Leaf::Integer : return QString("%1").arg(lvalue.i); break;
    case Leaf::String : return *lvalue.s; break;
    case Leaf::Symbol : return *lvalue.n; break;
    case Leaf::Logical  :
    case Leaf::Operation :
    case Leaf::BinaryOperation :
                    return QString("%1%2%3")
                    .arg(lvalue.l->toString())
                    .arg(op)
                    .arg(op ? rvalue.l->toString() : "");
                    break;
    case Leaf::UnaryOperation :
                    return QString("-%1").arg(lvalue.l->toString());
                    break;
    case Leaf::Function :
                    if (series) {

                        if (lvalue.l) {
                            return QString("%1(%2,%3)")
                                       .arg(function)
                                       .arg(*(series->lvalue.n))
                                       .arg(lvalue.l->toString());
                        } else {
                            return QString("%1(%2)")
                                       .arg(function)
                                       .arg(*(series->lvalue.n));
                        }

                    } else {
                        QString f= function + "(";
                        bool first=true;
                        foreach(Leaf*l, fparms) {
                            f+= (first ? "," : "") + l->toString();
                            first = false;
                        }
                        f += ")";
                        return f;
                    }
                    break;
    case Leaf::Vector :
                    return QString("%1[%2:%3]")
                        .arg(lvalue.l->toString())
                        .arg(fparms[0]->toString())
                        .arg(fparms[1]->toString());

    case Leaf::Conditional : //qDebug()<<"cond:"<<op;
        {
                    if (rvalue.l) {
                        return QString("%1?%2:%3")
                        .arg(cond.l->toString())
                        .arg(lvalue.l->toString())
                        .arg(rvalue.l->toString());
                    } else {
                        return QString("%1?%2")
                        .arg(cond.l->toString())
                        .arg(lvalue.l->toString());
                    }
        }
        break;

    default:
        break;

    }
    return "";
}

void Leaf::print(int level, DataFilterRuntime *df)
{
    qDebug()<<"LEVEL"<<level;
    switch(type) {
    case Leaf::Script :
        qDebug()<<lvalue.s;
        break;
    case Leaf::Compound :
        qDebug()<<"{";
        foreach(Leaf *p, *(lvalue.b))
            p->print(level+1, df);
        qDebug()<<"}";
        break;

    case Leaf::Float :
        qDebug()<<"float"<<lvalue.f<<dynamic;
        break;
    case Leaf::Integer :
        qDebug()<<"integer"<<lvalue.i<<dynamic;
        break;
    case Leaf::String :
        qDebug()<<"string"<<*lvalue.s<<dynamic;
        break;
    case Leaf::Symbol :
        {
            double value = (df ? df->symbols.value(*lvalue.n).number : 0);
            qDebug()<<"symbol"<<*lvalue.n<<dynamic<<value;
        }
        break;
    case Leaf::Logical :
        qDebug()<<"lop"<<op;
        lvalue.l->print(level+1, df);
        if (op) // nonzero ?
            rvalue.l->print(level+1, df);
        break;
    case Leaf::Operation:
        qDebug()<<"cop"<<op;
        lvalue.l->print(level+1, df);
        rvalue.l->print(level+1, df);
        break;
    case Leaf::UnaryOperation:
        qDebug()<<"uop"<<op;
        lvalue.l->print(level+1, df);
        break;
    case Leaf::BinaryOperation:
        qDebug()<<"bop"<<op;
        lvalue.l->print(level+1, df);
        rvalue.l->print(level+1, df);
        break;
    case Leaf::Function:
        if (series) {
            qDebug()<<"function"<<function<<"parm="<<*(series->lvalue.n);
            if (lvalue.l)
                lvalue.l->print(level+1, df);
        } else {
            qDebug()<<"function"<<function<<"parms:"<<fparms.count();
            foreach(Leaf* l, fparms)
                l->print(level+1, df);
        }
        break;
    case Leaf::Vector:
        qDebug()<<"vector";
        lvalue.l->print(level+1, df);
        fparms[0]->print(level+1, df);
        fparms[1]->print(level+1, df);
        break;
    case Leaf::Conditional:
        qDebug()<<"cond"<<op;
        cond.l->print(level+1, df);
        lvalue.l->print(level+1, df);
        if (rvalue.l)
            rvalue.l->print(level+1, df);
        break;

    default:
        break;

    }
}

static bool isCoggan(QString symbol)
{
    if (!symbol.compare("ctl", Qt::CaseInsensitive)) return true;
    if (!symbol.compare("tsb", Qt::CaseInsensitive)) return true;
    if (!symbol.compare("atl", Qt::CaseInsensitive)) return true;
    return false;
}

bool Leaf::isNumber(DataFilterRuntime *df, Leaf *leaf)
{
    switch(leaf->type) {
    case Leaf::Script : return true;
    case Leaf::Compound : return true; // last statement is value of block
    case Leaf::Float : return true;
    case Leaf::Integer : return true;
    case Leaf::String :
        {
            // strings that evaluate as a date
            // will be returned as a number of days
            // since 1900!
            QString string = *(leaf->lvalue.s);
            if (QDate::fromString(string, "yyyy/MM/dd").isValid())
                return true;
            else {
                return false;
            }
        }
    case Leaf::Symbol :
        {
            QString symbol = *(leaf->lvalue.n);
            if (df->symbols.contains(symbol)) return true;
            if (symbol == "isRun") return true;
            if (symbol == "x") return true;
            else if (symbol == "isSwim") return true;
            else if (!symbol.compare("Date", Qt::CaseInsensitive)) return true;
            else if (!symbol.compare("Today", Qt::CaseInsensitive)) return true;
            else if (!symbol.compare("Current", Qt::CaseInsensitive)) return true;
            else if (!symbol.compare("RECINTSECS", Qt::CaseInsensitive)) return true;
            else if (!symbol.compare("NA", Qt::CaseInsensitive)) return true;
            else if (isCoggan(symbol)) return true;
            else if (df->dataSeriesSymbols.contains(symbol)) return true;
            else return df->lookupType.value(symbol, false);
        }
        break;
    case Leaf::Logical  : return true; // not possible!
    case Leaf::Operation : return true;
    case Leaf::UnaryOperation : return true;
    case Leaf::BinaryOperation : return true;
    case Leaf::Function : return true;
    case Leaf::Vector :
    case Leaf::Index :
    case Leaf::Conditional :
        {
            return true;
        }
        break;

    default:
        return false;
        break;

    }
}

void Leaf::clear(Leaf *leaf)
{
Q_UNUSED(leaf);
#if 0 // memory leak!!!
    switch(leaf->type) {
    case Leaf::String : delete leaf->lvalue.s; break;
    case Leaf::Symbol : delete leaf->lvalue.n; break;
    case Leaf::Logical  :
    case Leaf::BinaryOperation :
    case Leaf::Operation : clear(leaf->lvalue.l);
                           clear(leaf->rvalue.l);
                           delete(leaf->lvalue.l);
                           delete(leaf->rvalue.l);
                           break;
    case Leaf::Function :  clear(leaf->lvalue.l);
                           delete(leaf->lvalue.l);
                            break;
    default:
        break;
    }
#endif
}

void Leaf::validateFilter(Context *context, DataFilterRuntime *df, Leaf *leaf)
{
    leaf->inerror = false;

    switch(leaf->type) {

    case Leaf::Script : // just assume script is well formed...
        return;

    case Leaf::Symbol :
        {
            // are the symbols correct?
            // if so set the type to meta or metric
            // and save the technical name used to do
            // a lookup at execution time
            QString symbol = *(leaf->lvalue.n);
            QString lookup = df->lookupMap.value(symbol, "");
            if (lookup == "") {

                // isRun isa special, we may add more later (e.g. date)
                if (symbol.compare("Date", Qt::CaseInsensitive) &&
                    symbol.compare("x", Qt::CaseInsensitive) && // used by which
                    symbol.compare("Today", Qt::CaseInsensitive) &&
                    symbol.compare("Current", Qt::CaseInsensitive) &&
                    symbol.compare("RECINTSECS", Qt::CaseInsensitive) &&
                    symbol.compare("NA", Qt::CaseInsensitive) &&
                    !df->dataSeriesSymbols.contains(symbol) &&
                    symbol != "isSwim" && symbol != "isRun" && !isCoggan(symbol)) {

                    // unknown, is it user defined ?
                    if (!df->symbols.contains(symbol)) {
                        DataFiltererrors << QString(tr("%1 is unknown")).arg(symbol);
                        leaf->inerror = true;
                    }
                }

                if (symbol.compare("Current", Qt::CaseInsensitive))
                    leaf->dynamic = true;
            }
        }
        break;

    case Leaf::Vector :
        {
            leaf->validateFilter(context, df, leaf->lvalue.l);
            leaf->validateFilter(context, df, leaf->fparms[0]);
            leaf->validateFilter(context, df, leaf->fparms[1]);
        }
        return;

    case Leaf::Index :
        {
            if (leaf->lvalue.l->type != Leaf::Symbol) {
                leaf->inerror = true;
                DataFiltererrors << QString(tr("Array subscript needs a symbol name."));
                return;
            }
            QString symbol = leaf->lvalue.l ? *(leaf->lvalue.l->lvalue.n) : "";
            if (df->dataSeriesSymbols.contains(symbol)) {
                leaf->seriesType =  RideFile::seriesForSymbol(symbol);
            } else {
                leaf->seriesType = RideFile::none;
            }
            leaf->validateFilter(context, df, leaf->fparms[0]);
            if (!Leaf::isNumber(df, leaf->fparms[0])) {
                leaf->fparms[0]->inerror = true;
                DataFiltererrors << QString(tr("Index must be numeric."));
            }
            leaf->validateFilter(context, df, leaf->lvalue.l);
            return;
        }

    case Leaf::Function :
        {
            // is the symbol valid?
            QRegExp bestValidSymbols("^(apower|power|hr|cadence|speed|torque|vam|xpower|isopower|wpk)$", Qt::CaseInsensitive);
            QRegExp tizValidSymbols("^(power|hr)$", Qt::CaseInsensitive);
            QRegExp configValidSymbols("^(cranklength|cp|ftp|w\\'|pmax|cv|d\\'|scv|sd\\'|height|weight|lthr|maxhr|rhr|units)$", Qt::CaseInsensitive);
            QRegExp constValidSymbols("^(e|pi)$", Qt::CaseInsensitive); // just do basics for now
            QRegExp dateRangeValidSymbols("^(start|stop)$", Qt::CaseInsensitive); // date range

            if (leaf->series) { // old way of hand crafting each function in the lexer including support for literal parameter e.g. (power, 1)

                QString symbol = leaf->series->lvalue.n->toLower();

                if (leaf->function == "best" && !bestValidSymbols.exactMatch(symbol)) {
                    DataFiltererrors << QString(tr("invalid data series for best(): %1")).arg(symbol);
                    leaf->inerror = true;
                }

                if (leaf->function == "tiz" && !tizValidSymbols.exactMatch(symbol)) {
                    DataFiltererrors << QString(tr("invalid data series for tiz(): %1")).arg(symbol);
                    leaf->inerror = true;
                }

                if (leaf->function == "daterange") {

                    if (!dateRangeValidSymbols.exactMatch(symbol)) {
                        DataFiltererrors << QString(tr("invalid literal for daterange(): %1")).arg(symbol);
                        leaf->inerror = true;

                    } else {
                        // convert to int days since using current date range config
                        // should be able to get from parent somehow
                        leaf->type = Leaf::Integer;
                        if (symbol == "start") leaf->lvalue.i = QDate(1900,01,01).daysTo(context->currentDateRange().from);
                        else if (symbol == "stop") leaf->lvalue.i = QDate(1900,01,01).daysTo(context->currentDateRange().to);
                        else leaf->lvalue.i = 0;
                    }
                }

                if (leaf->function == "config" && !configValidSymbols.exactMatch(symbol)) {
                    DataFiltererrors << QString(tr("invalid literal for config(): %1")).arg(symbol);
                    leaf->inerror = true;
                }

                if (leaf->function == "const") {
                    if (!constValidSymbols.exactMatch(symbol)) {
                        DataFiltererrors << QString(tr("invalid literal for const(): %1")).arg(symbol);
                        leaf->inerror = true;
                    } else {

                        // convert to a float
                        leaf->type = Leaf::Float;
                        leaf->lvalue.f = 0.0L;
                        if (symbol == "e") leaf->lvalue.f = MATHCONST_E;
                        if (symbol == "pi") leaf->lvalue.f = MATHCONST_PI;
                    }
                }

                if (leaf->function == "best" || leaf->function == "tiz") {
                    // now set the series type used as parameter 1 to best/tiz
                    leaf->seriesType = nameToSeries(symbol);
                }

            } else { // generic functions, math etc

                bool found=false;

                // are the parameters well formed ?
                if (leaf->function == "which") {

                    // 2 or more
                    if (leaf->fparms.count() < 2) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("which function has at least 2 parameters."));
                    }

                    // still normal parm check !
                    foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);

                } else if (leaf->function == "banister") {

                    // 2 or more
                    if (leaf->fparms.count() != 2) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be banister(metric, nte|pte|perf|cp)"));
                    } else {

                        Leaf *first=leaf->fparms[0];
                        Leaf *second=leaf->fparms[1];

                        // check metric name is valid
                        QString metric = first->signature();
                        QString lookup = df->lookupMap.value(metric, "");
                        if (lookup == "") {
                            leaf->inerror = true;
                            DataFiltererrors << QString("unknown metric '%1'.").arg(metric);
                        }

                        // check value
                        QString value = second->signature();
                        QRegExp banSymbols("^(nte|pte|perf|cp)$", Qt::CaseInsensitive);
                        if (!banSymbols.exactMatch(value)) {
                            leaf->inerror = true;
                            DataFiltererrors << QString("unknown %1, should be nte,pte,perf or cp.").arg(value);
                        }
                    }

                } else if (leaf->function == "XDATA") {

                    leaf->dynamic = false;

                    if (leaf->fparms.count() != 3) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("XDATA needs 3 parameters."));
                    } else {

                        // are the first two strings ?
                        Leaf *first=leaf->fparms[0];
                        Leaf *second=leaf->fparms[1];
                        if (first->type != Leaf::String || second->type != Leaf::String) {
                            DataFiltererrors << QString(tr("XDATA expects a string for first two parameters"));
                            leaf->inerror = true;
                        }

                        // is the third a symbol we like?
                        Leaf *third=leaf->fparms[2];
                        if (third->type != Leaf::Symbol) {
                            DataFiltererrors << QString(tr("XDATA expects a symbol, one of sparse, repeat, interpolate or resample for third parameter."));
                            leaf->inerror = true;
                        } else {
                            QStringList xdataValidSymbols;
                            xdataValidSymbols << "sparse" << "repeat" << "interpolate" << "resample";
                            QString symbol = *(third->lvalue.n);
                            if (!xdataValidSymbols.contains(symbol, Qt::CaseInsensitive)) {
                                DataFiltererrors << QString(tr("XDATA expects one of sparse, repeat, interpolate or resample for third parameter. (%1)").arg(symbol));
                                leaf->inerror = true;
                            } else {
                                // remember what algorithm was selected
                                int index = xdataValidSymbols.indexOf(symbol, Qt::CaseInsensitive);
                                // we're going to set explicitly rather than by cast
                                // so we don't need to worry about ordering the enum and stringlist
                                switch(index) {
                                case 0: leaf->xjoin = RideFile::SPARSE; break;
                                case 1: leaf->xjoin = RideFile::REPEAT; break;
                                case 2: leaf->xjoin = RideFile::INTERPOLATE; break;
                                case 3: leaf->xjoin = RideFile::RESAMPLE; break;
                                }
                            }
                        }
                    }

                } else if (leaf->function == "XDATA_UNITS") {

                    leaf->dynamic = false;

                    if (leaf->fparms.count() != 2) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("XDATA_UNITS needs 2 parameters."));
                    } else {

                        // are the first two strings ?
                        Leaf *first=leaf->fparms[0];
                        Leaf *second=leaf->fparms[1];
                        if (first->type != Leaf::String || second->type != Leaf::String) {
                            DataFiltererrors << QString(tr("XDATA_UNITS expects a string for first two parameters"));
                            leaf->inerror = true;
                        }

                    }

                } else if (leaf->function == "isset" || leaf->function == "set" || leaf->function == "unset") {

                    // don't run it everytime a ride is selected!
                    leaf->dynamic = false;

                    // is the first a symbol ?
                    if (leaf->fparms.count() > 0) {
                        if (leaf->fparms[0]->type != Leaf::Symbol) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("isset/set/unset function first parameter is field/metric to set."));
                        } else {
                            QString symbol = *(leaf->fparms[0]->lvalue.n);

                            //  some specials are not allowed
                            if (!symbol.compare("Date", Qt::CaseInsensitive) ||
                                !symbol.compare("x", Qt::CaseInsensitive) || // used by which
                                !symbol.compare("Today", Qt::CaseInsensitive) ||
                                !symbol.compare("Current", Qt::CaseInsensitive) ||
                                !symbol.compare("RECINTSECS", Qt::CaseInsensitive) ||
                                !symbol.compare("NA", Qt::CaseInsensitive) ||
                                df->dataSeriesSymbols.contains(symbol) ||
                                symbol == "isSwim" || symbol == "isRun" || isCoggan(symbol)) {
                                DataFiltererrors << QString(tr("%1 is not supported in isset/set/unset operations")).arg(symbol);
                                leaf->inerror = true;
                            }
                        }
                    }

                    // make sure we have the right parameters though!
                    if (leaf->function == "issset" && leaf->fparms.count() != 1) {

                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("isset has one parameter, a symbol to check."));

                    } else if ((leaf->function == "set" && leaf->fparms.count() != 3) ||
                        (leaf->function == "unset" && leaf->fparms.count() != 2)) {

                        leaf->inerror = true;
                        DataFiltererrors << (leaf->function == "set" ?
                            QString(tr("set function needs 3 paramaters; symbol, value and expression.")) :
                            QString(tr("unset function needs 2 paramaters; symbol and expression.")));

                    } else {

                        // still normal parm check !
                        foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);
                    }

                } else if (leaf->function == "estimate") {

                    // we only want two parameters and they must be
                    // a model name and then either ftp, cp, pmax, w'
                    // or a duration
                    if (leaf->fparms.count() > 0) {
                        // check the model name
                        if (leaf->fparms[0]->type != Leaf::Symbol) {

                            leaf->fparms[0]->inerror = true;
                            DataFiltererrors << QString(tr("estimate function expects model name as first parameter."));

                        } else {

                            if (!pdmodels().contains(*(leaf->fparms[0]->lvalue.n))) {
                                leaf->inerror = leaf->fparms[0]->inerror = true;
                                DataFiltererrors << QString(tr("estimate function expects model name as first parameter"));
                            }
                        }

                        if (leaf->fparms.count() > 1) {

                            // check symbol name if it is a symbol
                            if (leaf->fparms[1]->type == Leaf::Symbol) {
                                QRegExp estimateValidSymbols("^(cp|ftp|pmax|w')$", Qt::CaseInsensitive);
                                if (!estimateValidSymbols.exactMatch(*(leaf->fparms[1]->lvalue.n))) {
                                    leaf->inerror = leaf->fparms[1]->inerror = true;
                                    DataFiltererrors << QString(tr("estimate function expects parameter or duration as second parameter"));
                                }
                            } else {
                                validateFilter(context, df, leaf->fparms[1]);
                            }
                        }
                    }

                } else {

                    // normal parm check !
                    foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);
                }

                // does it exist?
                for(int i=0; DataFilterFunctions[i].parameters != -1; i++) {
                    if (DataFilterFunctions[i].name == leaf->function) {

                        // with the right number of parameters?
                        if (DataFilterFunctions[i].parameters && leaf->fparms.count() != DataFilterFunctions[i].parameters) {
                            DataFiltererrors << QString(tr("function '%1' expects %2 parameter(s) not %3")).arg(leaf->function)
                                                .arg(DataFilterFunctions[i].parameters).arg(fparms.count());
                            leaf->inerror = true;
                        }
                        found = true;
                        break;
                    }
                }

                // calling a user defined function, does it exist >=?
                if (found == false && !df->functions.contains(leaf->function)) {

                    DataFiltererrors << QString(tr("unknown function %1")).arg(leaf->function);
                    leaf->inerror = true;
                }
            }
        }
        break;

    case Leaf::UnaryOperation :
        { // unary minus needs a number, unary ! is happy with anything
            if (leaf->op == '-' && !Leaf::isNumber(df, leaf->lvalue.l)) {
                DataFiltererrors << QString(tr("unary negation on a string!"));
                leaf->inerror = true;
            }
        }
        break;
    case Leaf::BinaryOperation  :
    case Leaf::Operation  :
        {
            if (leaf->op == ASSIGN) {

                // add the symbol first
                if (leaf->lvalue.l->type == Leaf::Symbol) {

                    // add symbol
                    QString symbol = *(leaf->lvalue.l->lvalue.n);
                    df->symbols.insert(symbol, Result(0));

                    // validate rhs is numeric
                    bool rhsType = Leaf::isNumber(df, leaf->rvalue.l);
                    if (!rhsType) {
                        DataFiltererrors << QString(tr("variables must be numeric."));
                        leaf->inerror = true;
                    }

                } else if (leaf->lvalue.l->type == Leaf::Index) {

                    // add symbol
                    QString symbol = *(leaf->lvalue.l->lvalue.l->lvalue.n);

                    // add generic symbols
                    if (!df->dataSeriesSymbols.contains(symbol)) {
                        df->symbols.insert(symbol, Result(0));
                    }

                    // validate rhs is numeric
                    bool rhsType = Leaf::isNumber(df, leaf->rvalue.l);
                    if (!rhsType) {
                        DataFiltererrors << QString(tr("variables must be numeric."));
                        leaf->inerror = true;
                    }

                    // validate the symbol (after we added it!)
                    validateFilter(context, df, leaf->lvalue.l);

                } else {

                    DataFiltererrors << QString(tr("assignment must be to a symbol."));
                    leaf->inerror = true;
                }

                // and check the rhs is good too
                validateFilter(context, df, leaf->rvalue.l);

            } else {

                // first lets make sure the lhs and rhs are of the same type
                bool lhsType = Leaf::isNumber(df, leaf->lvalue.l);
                bool rhsType = Leaf::isNumber(df, leaf->rvalue.l);
                if (lhsType != rhsType) {
                    DataFiltererrors << QString(tr("comparing strings with numbers"));
                    leaf->inerror = true;
                }

                // what about using string operations on a lhs/rhs that
                // are numeric?
                if ((lhsType || rhsType) && leaf->op >= MATCHES && leaf->op <= CONTAINS) {
                    DataFiltererrors << tr("using a string operations with a number");
                    leaf->inerror = true;
                }

                validateFilter(context, df, leaf->lvalue.l);
                validateFilter(context, df, leaf->rvalue.l);
            }
        }
        break;

    case Leaf::Logical :
        {
            validateFilter(context, df, leaf->lvalue.l);
            if (leaf->op) validateFilter(context, df, leaf->rvalue.l);
        }
        break;

    case Leaf::Conditional :
        {
            // three expressions to validate
            validateFilter(context, df, leaf->cond.l);
            validateFilter(context, df, leaf->lvalue.l);
            if (leaf->rvalue.l) validateFilter(context, df, leaf->rvalue.l);
        }
        break;

    case Leaf::Compound :
        {
            // is this a user defined function ?
            if (leaf->function != "") {

                // register it
                df->functions.insert(leaf->function, leaf);
            }

            // a list of statements, the last of which is what we
            // evaluate to for the purposes of filtering etc
            foreach(Leaf *p, *(leaf->lvalue.b)) validateFilter(context, df, p);
        }
        break;

    default:
        break;
    }
}

DataFilter::DataFilter(QObject *parent, Context *context) : QObject(parent), context(context), treeRoot(NULL)
{
    // be sure not to enable this by accident!
    rt.isdynamic = false;

    // set up the models we support
    rt.models << new CP2Model(context);
    rt.models << new CP3Model(context);
    rt.models << new MultiModel(context);
    rt.models << new ExtendedModel(context);
    rt.models << new WSModel(context);

    configChanged(CONFIG_FIELDS);
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(rideSelected(RideItem*)), this, SLOT(dynamicParse()));
}

DataFilter::DataFilter(QObject *parent, Context *context, QString formula) : QObject(parent), context(context), treeRoot(NULL)
{
    // be sure not to enable this by accident!
    rt.isdynamic = false;

    // set up the models we support
    rt.models << new CP2Model(context);
    rt.models << new CP3Model(context);
    rt.models << new MultiModel(context);
    rt.models << new ExtendedModel(context);
    rt.models << new WSModel(context);

    configChanged(CONFIG_FIELDS);

    // regardless of success or failure set signature
    setSignature(formula);

    DataFiltererrors.clear(); // clear out old errors
    DataFilter_setString(formula);
    DataFilterparse();
    DataFilter_clearString();
    treeRoot = DataFilterroot;

    // if it parsed (syntax) then check logic (semantics)
    if (treeRoot && DataFiltererrors.count() == 0)
        treeRoot->validateFilter(context, &rt, treeRoot);
    else
        treeRoot=NULL;

    // save away the results if it passed semantic validation
    if (DataFiltererrors.count() != 0)
        treeRoot= NULL;
}

Result DataFilter::evaluate(RideItem *item, RideFilePoint *p)
{
    if (!item || !treeRoot || DataFiltererrors.count())
        return Result(0);

    // reset stack
    rt.stack = 0;

    Result res(0);

    // if we are a set of functions..
    if (rt.functions.count()) {

        // ... start at main
        if (rt.functions.contains("main"))
            res = treeRoot->eval(&rt, rt.functions.value("main"), 0, item, p);

    } else {

        // otherwise just evaluate the entire tree
        res = treeRoot->eval(&rt, treeRoot, 0, item, p);
    }

    return res;
}

QStringList DataFilter::check(QString query)
{
    // since we may use it afterwards
    setSignature(query);

    // remember where we apply
    rt.isdynamic=false;

    // Parse from string
    DataFiltererrors.clear(); // clear out old errors
    DataFilter_setString(query);
    DataFilterparse();
    DataFilter_clearString();

    // save away the results
    treeRoot = DataFilterroot;

    // if it passed syntax lets check semantics
    if (treeRoot && DataFiltererrors.count() == 0) treeRoot->validateFilter(context, &rt, treeRoot);

    // ok, did it pass all tests?
    if (!treeRoot || DataFiltererrors.count() > 0) { // nope

        // no errors just failed to finish
        if (!treeRoot) DataFiltererrors << tr("malformed expression.");

    }

    errors = DataFiltererrors;
    return errors;
}

QStringList DataFilter::parseFilter(Context *context, QString query, QStringList *list)
{
    // remember where we apply
    this->list = list;
    rt.isdynamic=false;
    rt.symbols.clear();

    // regardless of fail/pass set the signature
    setSignature(query);

    //DataFilterdebug = 2; // no debug -- needs bison -t in src.pro
    DataFilterroot = NULL;

    // if something was left behind clear it up now
    clearFilter();

    // Parse from string
    DataFiltererrors.clear(); // clear out old errors
    DataFilter_setString(query);
    DataFilterparse();
    DataFilter_clearString();

    // save away the results
    treeRoot = DataFilterroot;

    // if it passed syntax lets check semantics
    if (treeRoot && DataFiltererrors.count() == 0) treeRoot->validateFilter(context, &rt, treeRoot);

    // ok, did it pass all tests?
    if (!treeRoot || DataFiltererrors.count() > 0) { // nope
        // no errors just failed to finish
        if (!treeRoot) DataFiltererrors << tr("malformed expression.");

        // Bzzzt, malformed
        emit parseBad(DataFiltererrors);
        clearFilter();

    } else { // yep! .. we have a winner!

        rt.isdynamic = treeRoot->isDynamic(treeRoot);

        // successfully parsed, lets check semantics
        //treeRoot->print(0,NULL);
        emit parseGood();

        // clear current filter list
        filenames.clear();

        // get all fields...
        foreach(RideItem *item, context->athlete->rideCache->rides()) {

            // evaluate each ride...
            Result result = treeRoot->eval(&rt, treeRoot, 0, item, NULL);
            if (result.isNumber && result.number) {
                filenames << item->fileName;
            }
        }
        emit results(filenames);
        if (list) *list = filenames;
    }

    errors = DataFiltererrors;
    return errors;
}

void
DataFilter::dynamicParse()
{
    if (rt.isdynamic) {
        // need to reapply on current state

        // clear current filter list
        filenames.clear();

        // get all fields...
        foreach(RideItem *item, context->athlete->rideCache->rides()) {

            // evaluate each ride...
            Result result = treeRoot->eval(&rt, treeRoot, 0, item, NULL);
            if (result.isNumber && result.number)
                filenames << item->fileName;
        }
        emit results(filenames);
        if (list) *list = filenames;
    }
}

void DataFilter::clearFilter()
{
    if (treeRoot) {
        treeRoot->clear(treeRoot);
        treeRoot = NULL;
    }
    rt.isdynamic = false;
    sig = "";
}

void DataFilter::configChanged(qint32)
{
    rt.lookupMap.clear();
    rt.lookupType.clear();

    // create lookup map from 'friendly name' to INTERNAL-name used in summaryMetrics
    // to enable a quick lookup && the lookup for the field type (number, text)
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++) {
        QString symbol = factory.metricName(i);
        QString name = context->specialFields.internalName(factory.rideMetric(symbol)->name());

        rt.lookupMap.insert(name.replace(" ","_"), symbol);
        rt.lookupType.insert(name.replace(" ","_"), true);
    }

    // now add the ride metadata fields -- should be the same generally
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
            QString underscored = field.name;
            if (!context->specialFields.isMetric(underscored)) {

                // translate to internal name if name has non Latin1 characters
                underscored = context->specialFields.internalName(underscored);
                field.name = context->specialFields.internalName((field.name));

                rt.lookupMap.insert(underscored.replace(" ","_"), field.name);
                rt.lookupType.insert(underscored.replace(" ","_"), (field.type > 2)); // true if is number
            }
    }

    // sample date series
    rt.dataSeriesSymbols = RideFile::symbols();
}

Result Leaf::eval(DataFilterRuntime *df, Leaf *leaf, float x, RideItem *m, RideFilePoint *p, const QHash<QString,RideMetric*> *c, Specification s)
{
    // if error state all bets are off
    //if (inerror) return Result(0);

    switch(leaf->type) {

    //
    // LOGICAL EXPRESSION
    //
    case Leaf::Logical  :
    {
        switch (leaf->op) {
            case AND :
            {
                Result left = eval(df, leaf->lvalue.l, x, m, p, c, s);
                if (left.isNumber && left.number) {
                    Result right = eval(df, leaf->rvalue.l, x, m, p, c, s);
                    if (right.isNumber && right.number) return Result(true);
                }
                return Result(false);
            }
            case OR :
            {
                Result left = eval(df, leaf->lvalue.l, x, m, p, c, s);
                if (left.isNumber && left.number) return Result(true);

                Result right = eval(df, leaf->rvalue.l, x, m, p, c, s);
                if (right.isNumber && right.number) return Result(true);

                return Result(false);
            }

            default : // parenthesis
                return (eval(df, leaf->lvalue.l, x, m, p, c, s));
        }
    }
    break;

    //
    // FUNCTIONS
    //
    case Leaf::Function :
    {
        double duration;

        // calling a user defined function
        if (df->functions.contains(leaf->function)) {

            // going down
            df->stack += 1;

            // stack overflow
            if (df->stack > 500) {
                qDebug()<<"stack overflow";
                df->stack = 0;
                return Result(0);
            }

            Result res = eval(df, df->functions.value(leaf->function), x, m, p, c, s);

            // pop stack - if we haven't overflowed and reset
            if (df->stack > 0) df->stack -= 1;

            return res;
        }

        if (leaf->function == "config") {
            //
            // Get CP and W' estimates for date of ride
            //
            double CP = 0;
            double FTP = 0;
            double WPRIME = 0;
            double PMAX = 0;
            int zoneRange;

            if (m->context->athlete->zones(m->isRun)) {

                // if range is -1 we need to fall back to a default value
                zoneRange = m->context->athlete->zones(m->isRun)->whichRange(m->dateTime.date());
                FTP = CP = zoneRange >= 0 ? m->context->athlete->zones(m->isRun)->getCP(zoneRange) : 0;
                WPRIME = zoneRange >= 0 ? m->context->athlete->zones(m->isRun)->getWprime(zoneRange) : 0;
                PMAX = zoneRange >= 0 ? m->context->athlete->zones(m->isRun)->getPmax(zoneRange) : 0;

                // use CP for FTP, or is it configured separately
                bool useCPForFTP = (appsettings->cvalue(m->context->athlete->cyclist,
                                    m->context->athlete->zones(m->isRun)->useCPforFTPSetting(), 0).toInt() == 0);
                if (zoneRange >= 0 && !useCPForFTP) {
                    FTP = m->context->athlete->zones(m->isRun)->getFTP(zoneRange);
                }

                // did we override CP in metadata ?
                int oCP = m->getText("CP","0").toInt();
                int oW = m->getText("W'","0").toInt();
                int oPMAX = m->getText("Pmax","0").toInt();
                if (oCP) CP=oCP;
                if (oW) WPRIME=oW;
                if (oPMAX) PMAX=oPMAX;
            }
            //
            // LTHR, MaxHR, RHR
            //
            int hrZoneRange = m->context->athlete->hrZones(m->isRun) ?
                              m->context->athlete->hrZones(m->isRun)->whichRange(m->dateTime.date())
                              : -1;

            int LTHR = hrZoneRange != -1 ?  m->context->athlete->hrZones(m->isRun)->getLT(hrZoneRange) : 0;
            int RHR = hrZoneRange != -1 ?  m->context->athlete->hrZones(m->isRun)->getRestHr(hrZoneRange) : 0;
            int MaxHR = hrZoneRange != -1 ?  m->context->athlete->hrZones(m->isRun)->getMaxHr(hrZoneRange) : 0;

            //
            // CV' D'
            //
            int paceZoneRange = m->context->athlete->paceZones(m->isSwim) ?
                                m->context->athlete->paceZones(m->isSwim)->whichRange(m->dateTime.date()) :
                                -1;

            double CV = (paceZoneRange != -1) ? m->context->athlete->paceZones(false)->getCV(paceZoneRange) : 0.0;
            double DPRIME = 0; //XXX(paceZoneRange != -1) ? m->context->athlete->paceZones(false)->getDPrime(paceZoneRange) : 0.0;

            int spaceZoneRange = m->context->athlete->paceZones(true) ?
                                m->context->athlete->paceZones(true)->whichRange(m->dateTime.date()) :
                                -1;
            double SCV = (spaceZoneRange != -1) ? m->context->athlete->paceZones(true)->getCV(spaceZoneRange) : 0.0;
            double SDPRIME = 0; //XXX (spaceZoneRange != -1) ? m->context->athlete->paceZones(true)->getDPrime(spaceZoneRange) : 0.0;

            //
            // HEIGHT and WEIGHT
            //
            double HEIGHT = m->getText("Height","0").toDouble();
            if (HEIGHT == 0) HEIGHT = m->context->athlete->getHeight(NULL);
            double WEIGHT = m->getWeight();

            QString symbol = leaf->series->lvalue.n->toLower();

            if (symbol == "cranklength") {
                return Result(appsettings->cvalue(m->context->athlete->cyclist, GC_CRANKLENGTH, 175.00f).toDouble() / 1000.0);
            }

            if (symbol == "cp") {
                return Result(CP);
            }
            if (symbol == "ftp") {
                return Result(FTP);
            }
            if (symbol == "w'") {
                return Result(WPRIME);
            }
            if (symbol == "pmax") {
                return Result(PMAX);
            }
            if (symbol == "scv") {
                return Result(SCV);
            }
            if (symbol == "sd'") {
                return Result(SDPRIME);
            }
            if (symbol == "cv") {
                return Result(CV);
            }
            if (symbol == "d'") {
                return Result(DPRIME);
            }
            if (symbol == "lthr") {
                return Result(LTHR);
            }
            if (symbol == "rhr") {
                return Result(RHR);
            }
            if (symbol == "maxhr") {
                return Result(MaxHR);
            }
            if (symbol == "weight") {
                return Result(WEIGHT);
            }
            if (symbol == "height") {
                return Result(HEIGHT);
            }
            if (symbol == "units") {
                return Result(m->context->athlete->useMetricUnits ? 1 : 0);
            }
        }

        // banister
        if (leaf->function == "banister") {
            Leaf *first=leaf->fparms[0];
            Leaf *second=leaf->fparms[1];

            // check metric name is valid
            QString metric = df->lookupMap.value(first->signature(), "");
            QString value = second->signature();
            QDate when = m->dateTime.date();
            Banister *banister = m->context->athlete->getBanisterFor(metric, 0,0);
            int type = BANISTER_PERFORMANCE;

            if (value == "nte") type = BANISTER_NTE;
            if (value == "pte") type = BANISTER_PTE;
            if (value == "perf") type = BANISTER_PERFORMANCE;
            if (value == "cp") type = BANISTER_CP;

            // value for the date..
            return Result(banister->value(when, type));
        }

        // get here for tiz and best
        if (leaf->function == "best" || leaf->function == "tiz") {

            switch (leaf->lvalue.l->type) {

                default:
                case Leaf::Function :
                {
                    duration = eval(df, leaf->lvalue.l, x, m, p, c, s).number; // duration will be zero if string
                }
                break;

                case Leaf::Symbol :
                {
                    QString rename;
                    // get symbol value
                    if (df->lookupType.value(*(leaf->lvalue.l->lvalue.n)) == true) {
                        // numeric
                        if (c) duration = RideMetric::getForSymbol(rename=df->lookupMap.value(*(leaf->lvalue.l->lvalue.n),""), c);
                        else duration = m->getForSymbol(rename=df->lookupMap.value(*(leaf->lvalue.l->lvalue.n),""));
                    } else if (*(leaf->lvalue.l->lvalue.n) == "x") {
                        duration = x;
                    } else {
                        duration = 0;
                    }
                }
                break;

                case Leaf::Float :
                    duration = leaf->lvalue.l->lvalue.f;
                    break;

                case Leaf::Integer :
                    duration = leaf->lvalue.l->lvalue.i;
                    break;

                case Leaf::String :
                    duration = (leaf->lvalue.l->lvalue.s)->toDouble();
                    break;

                break;
            }

            if (leaf->function == "best")
                return Result(RideFileCache::best(m->context, m->fileName, leaf->seriesType, duration));

            if (leaf->function == "tiz") // duration is really zone number
                return Result(RideFileCache::tiz(m->context, m->fileName, leaf->seriesType, duration));
        }

        // if we get here its general function handling
        // what function is being called?
        int fnum=-1;
        for (int i=0; DataFilterFunctions[i].parameters != -1; i++) {
            if (DataFilterFunctions[i].name == leaf->function) {

                // parameter mismatch not allowed; function signature mismatch
                // should be impossible...
                if (DataFilterFunctions[i].parameters && DataFilterFunctions[i].parameters != leaf->fparms.count())
                    fnum=-1;
                else
                    fnum = i;
                break;
            }
        }

        // not found...
        if (fnum < 0) return Result(0);

        switch (fnum) {
        case 0 : { return Result(cos(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // COS(x)
        case 1 : { return Result(tan(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // TAN(x)
        case 2 : { return Result(sin(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // SIN(x)
        case 3 : { return Result(acos(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // ACOS(x)
        case 4 : { return Result(atan(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // ATAN(x)
        case 5 : { return Result(asin(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // ASIN(x)
        case 6 : { return Result(cosh(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // COSH(x)
        case 7 : { return Result(tanh(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // TANH(x)
        case 8 : { return Result(sinh(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // SINH(x)
        case 9 : { return Result(acosh(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // ACOSH(x)
        case 10 : { return Result(atanh(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // ATANH(x)
        case 11 : { return Result(asinh(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // ASINH(x)

        case 12 : { return Result(exp(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // EXP(x)
        case 13 : { return Result(log(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // LOG(x)
        case 14 : { return Result(log10(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // LOG10(x)

        case 15 : { return Result(ceil(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // CEIL(x)
        case 16 : { return Result(floor(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // FLOOR(x)
        case 17 : { return Result(round(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // ROUND(x)

        case 18 : { return Result(fabs(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // FABS(x)
        case 19 : { return Result(std::isinf(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // ISINF(x)
        case 20 : { return Result(std::isnan(eval(df, leaf->fparms[0], x, m, p, c, s).number)); } // ISNAN(x)

        case 21 : { /* SUM( ... ) */
                    double sum=0;

                    foreach(Leaf *l, leaf->fparms) {
                        sum += eval(df, l, x, m, p, c, s).number; // for vectors number is sum
                    }
                    return Result(sum);
                  }
                  break;

        case 22 : { /* MEAN( ... ) */
                    double sum=0;
                    int count=0;

                    foreach(Leaf *l, leaf->fparms) {
                        Result res = eval(df, l, x, m, p, c, s); // for vectors number is sum
                        sum += res.number;
                        if (res.vector.count()) count += res.vector.count();
                        else count++;
                    }
                    return count ? Result(sum/double(count)) : Result(0);
                  }
                  break;

        case 23 : { /* MAX( ... ) */
                    double max=0;
                    bool set=false;

                    foreach(Leaf *l, leaf->fparms) {
                        Result res = eval(df, l, x, m, p, c, s);
                        if (res.vector.count()) {
                            foreach(double x, res.vector) {
                                if (set && x>max) max=x;
                                else if (!set) { set=true; max=x; }
                            }

                        } else {
                            if (set && res.number>max) max=res.number;
                            else if (!set) { set=true; max=res.number; }
                        }
                    }
                    return Result(max);
                  }
                  break;

        case 24 : { /* MIN( ... ) */
                    double min=0;
                    bool set=false;

                    foreach(Leaf *l, leaf->fparms) {
                        Result res = eval(df, l, x, m, p, c, s);
                        if (res.vector.count()) {
                            foreach(double x, res.vector) {
                                if (set && x<min) min=x;
                                else if (!set) { set=true; min=x; }
                            }

                        } else {
                            if (set && res.number<min) min=res.number;
                            else if (!set) { set=true; min=res.number; }
                        }
                    }
                    return Result(min);
                  }
                  break;

        case 25 : { /* COUNT( ... ) */

                    int count = 0;
                    foreach(Leaf *l, leaf->fparms) {
                        Result res = eval(df, l, x, m, p, c, s);
                        if (res.vector.count()) count += res.vector.count();
                        else count++;
                    }
                    return Result(count);
                  }
                  break;

        case 26 : { /* LTS (expr) */
                    PMCData *pmcData = m->context->athlete->getPMCFor(leaf->fparms[0], df);
                    return Result(pmcData->lts(m->dateTime.date()));
                  }
                  break;

        case 27 : { /* STS (expr) */
                    PMCData *pmcData = m->context->athlete->getPMCFor(leaf->fparms[0], df);
                    return Result(pmcData->sts(m->dateTime.date()));
                  }
                  break;

        case 28 : { /* SB (expr) */
                    PMCData *pmcData = m->context->athlete->getPMCFor(leaf->fparms[0], df);
                    return Result(pmcData->sb(m->dateTime.date()));
                  }
                  break;

        case 29 : { /* RR (expr) */
                    PMCData *pmcData = m->context->athlete->getPMCFor(leaf->fparms[0], df);
                    return Result(pmcData->rr(m->dateTime.date()));
                  }
                  break;

        case 30 :
                { /* ESTIMATE( model, CP | FTP | W' | PMAX | duration ) */

                    // which model ?
                    QString model = *leaf->fparms[0]->lvalue.n;
                    if (model == "2p") model = "2 Parm";
                    if (model == "3p") model = "3 Parm";
                    if (model == "ws") model = "WS";
                    if (model == "velo") model = "Velo";
                    if (model == "ext") model = "Ext";

                    // what we looking for ?
                    QString parm = leaf->fparms[1]->type == Leaf::Symbol ? *leaf->fparms[1]->lvalue.n : "";
                    bool toDuration = parm == "" ? true : false;
                    double duration = toDuration ? eval(df, leaf->fparms[1], x, m, p, c, s).number : 0;

                    // get the PD Estimate for this date - note we always work with the absolulte
                    // power estimates in formulas, since the user can just divide by config(weight)
                    // or Athlete_Weight (which takes into account values stored in ride files.
                    PDEstimate pde = m->context->athlete->getPDEstimateFor(m->dateTime.date(), model, false);

                    // no model estimate for this date
                    if (pde.parameters.count() == 0) return Result(0);

                    // get a duration
                    if (toDuration == true) {

                        double value = 0;

                        // we need to find the model
                        foreach(PDModel *pdm, df->models) {

                            // not the one we want
                            if (pdm->code() != model) continue;

                            // set the parameters previously derived
                            pdm->loadParameters(pde.parameters);

                            // use seconds
                            pdm->setMinutes(false);

                            // get the model estimate for our duration
                            value = pdm->y(duration);

                            // our work here is done
                            return Result(value);
                        }

                    } else {
                        if (parm == "cp") return Result(pde.CP);
                        if (parm == "w'") return Result(pde.WPrime);
                        if (parm == "ftp") return Result(pde.FTP);
                        if (parm == "pmax") return Result(pde.PMax);
                    }
                    return Result(0);
                }
                break;

        case 31 :
                {   // WHICH ( expr, ... )
                    Result returning(0);

                    // runs through all parameters, evaluating expression
                    // in first param, and if true, adding to the results
                    // this is a select statement.
                    // e.g. which(x > 0, 1,2,3,-5,-6,-7) would return
                    //      (1,2,3). More meaningfully it is used when
                    //      working with vectors
                    if (leaf->fparms.count() < 2) return returning;

                    for(int i=1; i< leaf->fparms.count(); i++) {

                        // evaluate the parameter
                        Result ex = eval(df, leaf->fparms[i], x, m, p, c, s);

                        if (ex.vector.count()) {

                            // tiz a vector
                            foreach(double x, ex.vector) {

                                // did it get selected?
                                Result which = eval(df, leaf->fparms[0], x, m, p, c, s);
                                if (which.number) {
                                    returning.vector << x;
                                    returning.number += x;
                                }
                            }

                        } else {

                            // does the parameter get selected ?
                            Result which = eval(df, leaf->fparms[0], ex.number, m, p, c, s);
                            if (which.number) {
                                returning.vector << ex.number;
                                returning.number += ex.number;
                            }
                        }
                    }
                    return Result(returning);
                }
                break;

        case 32 :
                {   // SET (field, value, expression ) returns expression evaluated
                    Result returning(0);

                    if (leaf->fparms.count() < 3) return returning;
                    else returning = eval(df, leaf->fparms[2], x, m, p, c, s);

                    if (returning.number) {

                        // symbol we are setting
                        QString symbol = *(leaf->fparms[0]->lvalue.n);

                        // lookup metrics (we override them)
                        QString o_symbol = df->lookupMap.value(symbol,"");
                        RideMetricFactory &factory = RideMetricFactory::instance();
                        const RideMetric *e = factory.rideMetric(o_symbol);

                        // ack ! we need to set, so open the ride
                        RideFile *f = m->ride();

                        if (!f) return Result(0); // eek!

                        // evaluate second argument, its the value
                        Result r = eval(df, leaf->fparms[1], x, m, p, c, s);

                        // now set an override or a tag
                        if (o_symbol != "" && e) { // METRIC OVERRIDE

                            // lets set the override
                            QMap<QString,QString> override;
                            override  = f->metricOverrides.value(o_symbol);

                            // clear and reset override value for this metric
                            override.insert("value", QString("%1").arg(r.number)); // add metric value

                            // update overrides for this metric in the main QMap
                            f->metricOverrides.insert(o_symbol, override);

                            // rideFile is now dirty!
                            m->setDirty(true);

                            // get refresh done, coz overrides state has changed
                            m->notifyRideMetadataChanged();

                        } else { // METADATA TAG

                            // need to set metadata tag
                            bool isnumeric = df->lookupType.value(symbol);

                            // are we using the right types ?
                            if (r.isNumber && isnumeric) {
                                f->setTag(o_symbol, QString("%1").arg(r.number));
                            } else if (!r.isNumber && !isnumeric) {
                                f->setTag(o_symbol, r.string);
                            } else {
                                // nope
                                return Result(0); // not changing it !
                            }

                            // rideFile is now dirty!
                            m->setDirty(true);

                            // get refresh done, coz overrides state has changed
                            m->notifyRideMetadataChanged();

                        }
                    }
                    return returning;
                }
                break;
        case 33 :
                {   // UNSET (field, expression ) remove override or tag
                    Result returning(0);

                    if (leaf->fparms.count() < 2) return returning;
                    else returning = eval(df, leaf->fparms[1], x, m, p, c, s);

                    if (returning.number) {

                        // symbol we are setting
                        QString symbol = *(leaf->fparms[0]->lvalue.n);

                        // lookup metrics (we override them)
                        QString o_symbol = df->lookupMap.value(symbol,"");
                        RideMetricFactory &factory = RideMetricFactory::instance();
                        const RideMetric *e = factory.rideMetric(o_symbol);

                        // ack ! we need to set, so open the ride
                        RideFile *f = m->ride();

                        if (!f) return Result(0); // eek!

                        // now remove the override
                        if (o_symbol != "" && e) { // METRIC OVERRIDE

                            // update overrides for this metric in the main QMap
                            f->metricOverrides.remove(o_symbol);

                            // rideFile is now dirty!
                            m->setDirty(true);

                            // get refresh done, coz overrides state has changed
                            m->notifyRideMetadataChanged();

                        } else { // METADATA TAG

                            // remove the tag
                            f->removeTag(o_symbol);

                            // rideFile is now dirty!
                            m->setDirty(true);

                            // get refresh done, coz overrides state has changed
                            m->notifyRideMetadataChanged();

                        }
                    }
                    return returning;
                }
                break;

        case 34 :
                {   // ISSET (field) is the metric overriden or metadata set ?

                    if (leaf->fparms.count() != 1) return Result(0);

                    // symbol we are setting
                    QString symbol = *(leaf->fparms[0]->lvalue.n);

                    // lookup metrics (we override them)
                    QString o_symbol = df->lookupMap.value(symbol,"");
                    RideMetricFactory &factory = RideMetricFactory::instance();
                    const RideMetric *e = factory.rideMetric(o_symbol);

                    // now remove the override
                    if (o_symbol != "" && e) { // METRIC OVERRIDE

                        return Result (m->overrides_.contains(o_symbol) == true);

                    } else { // METADATA TAG

                        return Result (m->hasText(o_symbol));
                    }
                }
                break;

        case 35 :
                {   // VDOTTIME (VDOT, distance[km])

                    if (leaf->fparms.count() != 2) return Result(0);

                    return Result (60*VDOTCalculator::eqvTime(eval(df, leaf->fparms[0], x, m, p, c, s).number, 1000*eval(df, leaf->fparms[1], x, m, p, c, s).number));
                }
                break;

        case 36 :
                {   // BESTTIME (distance[km])

                    if (leaf->fparms.count() != 1 || m->fileCache() == NULL) return Result(0);

                    return Result (m->fileCache()->bestTime(eval(df, leaf->fparms[0], x, m, p, c, s).number));
                 }

        case 37 :
                {   // XDATA ("XDATA", "XDATASERIES", (sparse, repeat, interpolate, resample)

                    if (!p) {

                        // processing ride item (e.g. filter, formula)
                        // we return true or false if the xdata series exists for the ride in question
                        QString xdata = *(leaf->fparms[0]->lvalue.s);
                        QString series = *(leaf->fparms[1]->lvalue.s);

                        if (m->xdataMatch(xdata, series, xdata, series)) return Result(1);
                        else return Result(0);

                    } else {

                        // get iteration state from datafilter runtime
                        int idx = df->indexes.value(this, 0);

                        QString xdata = *(leaf->fparms[0]->lvalue.s);
                        QString series = *(leaf->fparms[1]->lvalue.s);

                        double returning = 0;

                        // get the xdata value for this sample (if it exists)
                        if (m->xdataMatch(xdata, series, xdata, series))
                            returning = m->ride()->xdataValue(p, idx, xdata,series, leaf->xjoin);

                        // update state
                        df->indexes.insert(this, idx);

                        return Result(returning);

                    }
                    return Result(0);
                }
                break;
        case 38:  // PRINT(x) to qDebug
                {

                    // what is the parameter?
                    if (leaf->fparms.count() != 1) qDebug()<<"bad print.";

                    // symbol we are setting
                    leaf->fparms[0]->print(0, df);
                }
                break;

        case 39 :
                {   // AUTOPROCESS(expression) to run automatic data processors
                    Result returning(0);

                    if (leaf->fparms.count() != 1) return returning;
                    else returning = eval(df, leaf->fparms[0], x, m, p, c, s);

                    if (returning.number) {

                        // ack ! we need to autoprocess, so open the ride
                        RideFile *f = m->ride();

                        if (!f) return Result(0); // eek!

                        // now run auto data processors
                        if (DataProcessorFactory::instance().autoProcess(f, "Auto", "UPDATE")) {
                            // rideFile is now dirty!
                            m->setDirty(true);
                        }
                    }
                    return returning;
                }
                break;

        case 40 :
                {   // POSTPROCESS (processor, expression ) run processor
                    Result returning(0);

                    if (leaf->fparms.count() < 2) return returning;
                    else returning = eval(df, leaf->fparms[1], x, m, p, c, s);

                    if (returning.number) {

                        // processor we are running
                        QString dp_name = *(leaf->fparms[0]->lvalue.n);

                        // lookup processor
                        DataProcessor* dp = DataProcessorFactory::instance().getProcessors().value(dp_name, NULL);

                        if (!dp) return Result(0); // No such data processor

                        // ack ! we need to autoprocess, so open the ride
                        RideFile *f = m->ride();

                        if (!f) return Result(0); // eek!

                        // now run the data processor
                        if (dp->postProcess(f)) {
                            // rideFile is now dirty!
                            m->setDirty(true);
                        }
                    }
                    return returning;
                }
                break;

        case 41 :
                {   // XDATA_UNITS ("XDATA", "XDATASERIES")

                    if (p) { // only valid when iterating

                        // processing ride item (e.g. filter, formula)
                        // we return true or false if the xdata series exists for the ride in question
                        QString xdata = *(leaf->fparms[0]->lvalue.s);
                        QString series = *(leaf->fparms[1]->lvalue.s);

                        if (m->xdataMatch(xdata, series, xdata, series)) {

                            // we matched, xdata and series contain what was matched
                            XDataSeries *xs = m->ride()->xdata(xdata);

                            if (xs && m->xdata().value(xdata,QStringList()).contains(series)) {
                                int idx = m->xdata().value(xdata,QStringList()).indexOf(series);
                                QString units;
                                const int count = xs->unitname.count();
                                if (idx >= 0 && idx < count)
                                    units = xs->unitname[idx];
                                return Result(units);
                            }

                        } else return Result("");

                    } else return Result(""); // not for filtering
                }
                break;

        case 42 :
                {   // MEASURE (DATE, GROUP, FIELD) get measure
                    if (leaf->fparms.count() < 3) return Result(0);

                    Result days = eval(df, leaf->fparms[0], x, m, p, c, s);
                    if (!days.isNumber) return Result(0); // invalid date
                    QDate date = QDate(1900,01,01).addDays(days.number);
                    if (!date.isValid()) return Result(0); // invalid date

                    if (leaf->fparms[1]->type != String) return Result(0);
                    QString group_symbol = *(leaf->fparms[1]->lvalue.s);
                    int group = m->context->athlete->measures->getGroupSymbols().indexOf(group_symbol);
                    if (group < 0) return Result(0); // unknown group

                    if (leaf->fparms[2]->type != String) return Result(0);
                    QString field_symbol = *(leaf->fparms[2]->lvalue.s);
                    int field = m->context->athlete->measures->getFieldSymbols(group).indexOf(field_symbol);
                    if (field < 0) return Result(0); // unknown field

                    // retrieve measure value
                    double value = m->context->athlete->measures->getFieldValue(group, date, field);
                    return Result(value);
                }
                break;

        case 43 :
                {   // TESTS() for now just return a count of performance test intervals in the rideitem
                    if (m == NULL) return 0;
                    else {
                        int count=0;
                        foreach(IntervalItem *i, m->intervals())
                            if (i->istest()) count++;
                        return Result(count);
                    }
                }
                break;
        default:
            return Result(0);
        }
    }
    break;

    //
    // SCRIPT
    //
    case Leaf::Script :
    {

        // run a script
 #ifdef GC_WANT_PYTHON
        if (leaf->function == "python")  return Result(df->runPythonScript(m->context, *leaf->lvalue.s, m, c, s));
 #endif
        return Result(0);
    }
    break;

    //
    // SYMBOLS
    //
    case Leaf::Symbol :
    {
        double lhsdouble=0.0f;
        bool lhsisNumber=false;
        QString lhsstring;
        QString rename;
        QString symbol = *(leaf->lvalue.n);

        // ride series name when running through sample override metrics etc
        if (p && (lhsisNumber = df->dataSeriesSymbols.contains(*(leaf->lvalue.n))) == true) {

            RideFile::SeriesType type = RideFile::seriesForSymbol((*(leaf->lvalue.n)));
            if (type == RideFile::index) return Result(m->ride()->dataPoints().indexOf(p));
            return Result(p->value(type));
        }

        // user defined symbols override all others !
        if (df->symbols.contains(symbol)) return Result(df->symbols.value(symbol));

        // is it isRun ?
        if (symbol == "x") {

            lhsdouble = x;
            lhsisNumber = true;

        } else if (symbol == "isRun") {
            lhsdouble = m->isRun ? 1 : 0;
            lhsisNumber = true;

        } else if (symbol == "isSwim") {
            lhsdouble = m->isSwim ? 1 : 0;
            lhsisNumber = true;

        } else if (!symbol.compare("NA", Qt::CaseInsensitive)) {

            lhsdouble = RideFile::NA;
            lhsisNumber = true;

        } else if (!symbol.compare("RECINTSECS", Qt::CaseInsensitive)) {

            lhsdouble = 1; // if in doubt
            if (m->ride(false)) lhsdouble = m->ride(false)->recIntSecs();
            lhsisNumber = true;

        } else if (!symbol.compare("Current", Qt::CaseInsensitive)) {

            if (m->context->currentRideItem())
                lhsdouble = QDate(1900,01,01).
                daysTo(m->context->currentRideItem()->dateTime.date());
            else
                lhsdouble = 0;
            lhsisNumber = true;

        } else if (!symbol.compare("Today", Qt::CaseInsensitive)) {

            lhsdouble = QDate(1900,01,01).daysTo(QDate::currentDate());
            lhsisNumber = true;

        } else if (!symbol.compare("Date", Qt::CaseInsensitive)) {

            lhsdouble = QDate(1900,01,01).daysTo(m->dateTime.date());
            lhsisNumber = true;

        } else if (isCoggan(symbol)) {
            // a coggan PMC metric
            PMCData *pmcData = m->context->athlete->getPMCFor("coggan_tss");
            if (!symbol.compare("ctl", Qt::CaseInsensitive)) lhsdouble = pmcData->lts(m->dateTime.date());
            if (!symbol.compare("atl", Qt::CaseInsensitive)) lhsdouble = pmcData->sts(m->dateTime.date());
            if (!symbol.compare("tsb", Qt::CaseInsensitive)) lhsdouble = pmcData->sb(m->dateTime.date());
            lhsisNumber = true;

        } else if ((lhsisNumber = df->lookupType.value(*(leaf->lvalue.n))) == true) {
            // get symbol value
            // check metadata string to number first ...
            QString meta = m->getText(rename=df->lookupMap.value(symbol,""), "unknown");
            if (meta == "unknown")
                if (c) lhsdouble = RideMetric::getForSymbol(rename=df->lookupMap.value(symbol,""), c);
                else lhsdouble = m->getForSymbol(rename=df->lookupMap.value(symbol,""));
            else
                lhsdouble = meta.toDouble();
            lhsisNumber = true;

            //qDebug()<<"symbol" << *(lvalue.n) << "is" << lhsdouble << "via" << rename;
        } else {
            // string symbol will evaluate to zero as unary expression
            lhsstring = m->getText(rename=df->lookupMap.value(symbol,""), "");
            //qDebug()<<"symbol" << *(lvalue.n) << "is" << lhsstring << "via" << rename;
        }
        if (lhsisNumber) return Result(lhsdouble);
        else return Result(lhsstring);
    }
    break;

    //
    // LITERALS
    //
    case Leaf::Float :
    {
        return Result(leaf->lvalue.f);
    }
    break;

    case Leaf::Integer :
    {
        return Result(leaf->lvalue.i);
    }
    break;

    case Leaf::String :
    {
        QString string = *(leaf->lvalue.s);

        // dates are returned as numbers
        QDate date = QDate::fromString(string, "yyyy/MM/dd");
        if (date.isValid()) return Result(QDate(1900,01,01).daysTo(date));
        else return Result(string);
    }
    break;

    //
    // UNARY EXPRESSION
    //
    case Leaf::UnaryOperation :
    {
        // get result
        Result lhs = eval(df, leaf->lvalue.l, x, m, p, c, s);

        // unary minus
        if (leaf->op == '-') return Result(lhs.number * -1);

        // unary not
        if (leaf->op == '!') return Result(!lhs.number);

        // unknown
        return(Result(0));
    }
    break;

    //
    // BINARY EXPRESSION
    //
    case Leaf::BinaryOperation :
    case Leaf::Operation :
    {
        // lhs and rhs
        Result lhs;
        if (leaf->op != ASSIGN) lhs = eval(df, leaf->lvalue.l, x, m, p, c, s);

        // if elvis we only evaluate rhs if we are null
        Result rhs;
        if (leaf->op != ELVIS || lhs.number == 0) {
            rhs = eval(df, leaf->rvalue.l, x, m, p, c, s);
        }

        // NOW PERFORM OPERATION
        switch (leaf->op) {

        case ASSIGN:
        {
            // LHS MUST be a symbol...
            if (leaf->lvalue.l->type == Leaf::Symbol || leaf->lvalue.l->type == Leaf::Index) {

                Result  value(rhs.isNumber ? rhs.number : 0);

                if (leaf->lvalue.l->type == Leaf::Symbol) {

                    QString symbol = *(leaf->lvalue.l->lvalue.n);
                    df->symbols.insert(symbol, value);

                } else {

                    if (leaf->lvalue.l->seriesType == RideFile::none) {

                        QString symbol = *(leaf->lvalue.l->lvalue.l->lvalue.n);
                        int index = eval(df,leaf->lvalue.l->fparms[0], x, m, p, c, s).number;

                        // generic symbol
                        if (df->symbols.contains(symbol)) {
                            Result sym = df->symbols.value(symbol);

                            // resize if need to
                            if (sym.vector.count() <= index) {
                                sym.vector.resize(index+1);
                            }

                            // add value
                            sym.vector[index] = value.number;

                            // update
                            df->symbols.insert(symbol, sym);
                        }
                    }
                }
                return value;
            }
            // shouldn't get here!
            return Result(RideFile::NA);
        }
        break;

        case ADD:
        {
            if (lhs.isNumber) return Result(lhs.number + rhs.number);
            else return Result(0);
        }
        break;

        case SUBTRACT:
        {
            if (lhs.isNumber) return Result(lhs.number - rhs.number);
            else return Result(0);
        }
        break;

        case DIVIDE:
        {
            // avoid divide by zero
            if (lhs.isNumber && rhs.number) return Result(lhs.number / rhs.number);
            else return Result(0);
        }
        break;

        case MULTIPLY:
        {
            if (lhs.isNumber && rhs.isNumber) return Result(lhs.number * rhs.number);
            else return Result(0);
        }
        break;

        case POW:
        {
            if (lhs.isNumber && rhs.number) return Result(pow(lhs.number,rhs.number));
            else return Result(0);
        }
        break;

        case EQ:
        {
            if (lhs.isNumber) return Result(lhs.number == rhs.number);
            else return Result(lhs.string == rhs.string);
        }
        break;

        case NEQ:
        {
            if (lhs.isNumber) return Result(lhs.number != rhs.number);
            else return Result(lhs.string != rhs.string);
        }
        break;

        case LT:
        {
            if (lhs.isNumber) return Result(lhs.number < rhs.number);
            else return Result(lhs.string < rhs.string);
        }
        break;
        case LTE:
        {
            if (lhs.isNumber) return Result(lhs.number <= rhs.number);
            else return Result(lhs.string <= rhs.string);
        }
        break;
        case GT:
        {
            if (lhs.isNumber) return Result(lhs.number > rhs.number);
            else return Result(lhs.string > rhs.string);
        }
        break;
        case GTE:
        {
            if (lhs.isNumber) return Result(lhs.number >= rhs.number);
            else return Result(lhs.string >= rhs.string);
        }
        break;

        case ELVIS:
        {
            // it was evaluated above, which is kinda cheating
            // but its optimal and this is a special case.
            if (lhs.isNumber && lhs.number) return Result(lhs.number);
            else return Result(rhs.number);
        }
        case MATCHES:
            if (!lhs.isNumber && !rhs.isNumber) return Result(QRegExp(rhs.string).exactMatch(lhs.string));
            else return Result(false);
            break;

        case ENDSWITH:
            if (!lhs.isNumber && !rhs.isNumber) return Result(lhs.string.endsWith(rhs.string));
            else return Result(false);
            break;

        case BEGINSWITH:
            if (!lhs.isNumber && !rhs.isNumber) return Result(lhs.string.startsWith(rhs.string));
            else return Result(false);
            break;

        case CONTAINS:
            {
            if (!lhs.isNumber && !rhs.isNumber) return Result(lhs.string.contains(rhs.string) ? true : false);
            else return Result(false);
            }
            break;

        default:
            break;
        }
    }
    break;

    //
    // CONDITIONAL TERNARY / IF .. ELSE ../ WHILE
    //
    case Leaf::Conditional :
    {

        switch(leaf->op) {

        case IF_:
        case 0 :
            {
                Result cond = eval(df, leaf->cond.l, x, m, p, c, s);
                if (cond.isNumber && cond.number) return eval(df, leaf->lvalue.l, x, m, p, c, s);
                else {

                    // conditional may not have an else clause!
                    if (leaf->rvalue.l) return eval(df, leaf->rvalue.l, x, m, p, c, s);
                    else return Result(0);
                }
            }
        case WHILE :
            {
                // we bound while to make sure it doesn't consume all
                // CPU and 'hang' for badly written code..
                static int maxwhile = 10000;
                int count=0;
                QTime timer;
                timer.start();

                Result returning(0);
                while (count++ < maxwhile && eval(df, leaf->cond.l, x, m, p, c, s).number) {
                    returning = eval(df, leaf->lvalue.l, x, m, p, c, s);
                }

                // we had to terminate warn user !
                if (count >= maxwhile) {
                    qDebug()<<"WARNING: "<< "[ loops="<<count<<"ms="<<timer.elapsed() <<"] runaway while loop terminated, check formula/filter.";
                }

                return returning;
            }
        }
    }
    break;

    //
    // VECTORS
    //
    case Leaf::Vector :
    {
        // places results in vector, and number is sum of all
        // explicit funcions sum/avg/max/min will return non-sum
        // values. No vector operations are supported at present
        // as a DESIGN DECISION. They are too complex for the average
        // user to understand. We will integrate to R for users
        // that want that kind of power.
        //
        // Vectors are about collecting data from across a date range
        // so you can use them within a formula for simple kinds of
        // operations; e.g. how much of todays' workouts in time
        // does this workout represent would be:
        // Duration / Duration[[today:today]] * 100.00

        Specification spec;

        // get date range
        int fromDS = eval(df, leaf->fparms[0], x, m, p, c, s).number;
        int toDS = eval(df, leaf->fparms[1], x, m, p, c, s).number;

        // swap dates if needed
        if (toDS < fromDS) {
            int swap=fromDS;
            fromDS = toDS;
            toDS = swap;
        }

        spec.setDateRange(DateRange(QDate(1900,01,01).addDays(fromDS),QDate(1900,01,01).addDays(toDS)));

        Result returning;

        // now iterate and evaluate for each
        int count=0;
        foreach(RideItem *ride, m->context->athlete->rideCache->rides()) {

            if (!spec.pass(ride)) continue;

            count++;

            // calculate value
            Result res = eval(df, leaf->lvalue.l, x, ride, p, c, s);
            if (res.isNumber) {
                returning.number += res.number; // sum for easy access
                returning.vector.resize(count);
                returning.vector[count-1] = res.number;
            }
        }

        // always return as sum number (for now)
        return returning;
    }
    break;

    //
    // INDEXING INTO ACTIVITY SAMPLES
    //
    case Leaf::Index :
    {
        if (!p) return Result(0); // only applies when iterating over samples
        int index = eval(df,leaf->fparms[0], x, m, p, c, s).number;

        // ZERO if out of bounds (save a check)
        if (index < 0 || index >= m->ride()->dataPoints().count())
            return RideFile::NIL;

        if (leaf->seriesType == RideFile::none) {

            // generic symbol
            QString symbol = *(leaf->lvalue.l->lvalue.n);
            if (df->symbols.contains(symbol)) {
                Result sym = df->symbols.value(symbol);
                if (sym.vector.count() > index) {
                    return Result(sym.vector[index]);
                } else {
                    return Result(RideFile::NIL);
                }
            }
            // shouldn't get here!
            return Result(RideFile::NIL);
        }
        // otherwise lets return the value !
        return Result(m->ride()->dataPoints()[index]->value(leaf->seriesType));
    }
    break;

    //
    // COMPOUND EXPRESSION
    //
    case Leaf::Compound :
    {
        Result returning(0);

        // evaluate each statement
        foreach(Leaf *statement, *(leaf->lvalue.b)) returning = eval(df, statement, x, m, p, c, s);

        // compound statements evaluate to the value of the last statement
        return returning;
    }
    break;

    default: // we don't need to evaluate any lower - they are leaf nodes handled above
        break;
    }
    return Result(0); // false
}

#ifdef GC_WANT_PYTHON
double
DataFilterRuntime::runPythonScript(Context *context, QString script, RideItem *m, const QHash<QString,RideMetric*> *metrics, Specification spec)
{
    if (python == NULL) return(0);

    // get the lock
    pythonMutex.lock();

    // return result
    double result = 0;

    // run it !!
    python->canvas = NULL;
    python->chart = NULL;
    python->result = 0;

    try {

        // run it
        python->runline(ScriptContext(context, m, metrics, spec), script);
        result = python->result;

    } catch(std::exception& ex) {

        python->messages.clear();

    } catch(...) {

        python->messages.clear();

    }

    // clear context
    python->canvas = NULL;
    python->chart = NULL;

    // free up the interpreter
    pythonMutex.unlock();

    return result;
}
#endif
