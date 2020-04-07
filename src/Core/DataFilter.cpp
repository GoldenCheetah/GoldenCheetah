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

#include "Utils.h"
#include "Statistic.h"
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
#include "GenericSelectTool.h" // for generic calculator
#include <QDebug>
#include <QMutex>
#include "lmcurve.h"
#include "LTMTrend.h" // for LR when copying CP chart filtering mechanism

#ifdef GC_WANT_PYTHON
#include "PythonEmbed.h"
QMutex pythonMutex;
#endif

#include "Zones.h"
#include "PaceZones.h"
#include "HrZones.h"
#include "UserChart.h"

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
    { "banister", 3 }, // banister(load_metric, perf_metric, nte|pte|perf|cp)

    // working with vectors

    { "c", 0 }, // return an array from concatated from paramaters (same as R) e.g. c(1,2,3,4,5)
    { "seq", 3 }, // create a vector with a range seq(start,stop,step)
    { "rep", 2 }, // create a vector of repeated values rep(value, n)
    { "length", 1}, // get length of a vector (can be zero where isnumber not a vector)

    { "append", 0}, // append vector append(symbol, expr [, at]) -- must reference a symbol
    { "remove", 3}, // remove vector elements remove(symbol, start, count) -- must reference a symbol
    { "mid", 3}, // subset of a vector mid(a,pos,count) -- returns a vector of size count from pos ion a

    { "samples", 1 }, // e.g. samples(POWER) - when on analysis view get vector of samples for the current activity
    { "metrics", 0 }, // metrics(Metrics [,start [,stop]]) - returns a vector of values for the metric specified
                      // if no start/stop is supplied it uses the currently selected date range, otherwise start thru today
                      // or start - stop.

    { "argsort", 2 }, // argsort(ascend|descend, list) - return a sorting index (ala numpy.argsort).

    { "sort", 0 }, // sort(ascend|descend, list1 [, list2, listn]) - sorts each list together, based upon list1, no limit to the
                   // number of lists but they must have the same length. the first list contains the values that define
                   // the sort order. since the sort is 'in-situ' the lists must all be user symbols. returns number of items
                   // sorted. this is impure from a functional programming perspective, but allows us to avoid using dataframes
                   // to manage x,y,z,t style data series when sorting.

    { "head", 2 }, // head(list, n) - returns vector of first n elements of list (or fewer if not so big)
    { "tail", 2 }, // tail(list, n) - returns vector of last n elements of list (or fewer if not so big)

    { "meanmax", 1 }, // meanmax(POWER) - when on trend view get a vector of meanmaximal data for the specific series
    { "pmc", 2 },  // pmc(symbol, stress|lts|sts|sb|rr|date) - get a vector of PMC series for the metric in symbol for the current date range.

    { "sapply", 2 }, // sapply(vector, expr) - returns a vector where expr has been applied to every element. x and i
                     // are both available in the expr for element value and index position.

    { "lr", 2 },   // lr(xlist, ylist) - linear regression on x,y co-ords returns vector [slope, intercept, r2, see]

    { "smooth", 0 }, // smooth(list, algorithm, ... parameters) - returns smoothed data.

    { "sqrt", 1 }, // sqrt(x) - returns square root, for vectors returns the sqrt of the sum

    { "lm", 3 }, // lm(formula, xseries, yseries) - fit using LM and return fit goodness.
                 // formula is an expression involving existing user symbols, their current values
                 // will be used as the starting values by the fit. once the fit has been completed
                 // the user symbols will contain the estimated parameters and lm will return some
                 // diagnostics goodness of fit measures [ success, RMSE, CV ] where a non-zero value
                 // for success means true, if it is false, RMSE and CV will be set to -1

    { "bool", 1 }, // bool(e) - will turn the passed parameter into a boolean with value 0 or 1
                   // this is useful for embedding logical expresissions into formulas. Since the
                   // grammar does not support (a*x>1), instead we can use a*bool(x>1). All non
                   // zero expressions will evaluate to 1.

    { "annotate", 0 }, // annotate(type, parms) - add an annotation to the chart, will no doubt
                       // extend over time to cover lots of different types, but for now
                       // supports 'label', which has n texts and numbers which are concatenated
                       // together to make a label; eg. annotate(label, "CP ", cpval, " watts");

    { "arguniq", 1 },  // returns an index of the uniq values in a vector, in the same way
                       // argsort returns an index, can then be used to select from samples
                       // or activity vectors

    { "uniq", 0 },     // stable uniq will keep original sequence but remove duplicates, does
                       // not need the data to be sorted, as it uses argsort internally. As
                       // you can pass multiple vectors they are uniqued in sync with the first list.

    { "variance", 1 }, // variance(v) - calculates the variance for the elements in the vector.
    { "stddev", 1 },   // stddev(v) - calculates the standard deviation for elements in the vector.

    { "curve", 2 },    // curve(series, x|y|z|d|t) - fetch the computed values for another curve
                       // will need to be on a user chart, and the series will need to have already
                       // been computed.

    { "lowerbound", 2 }, // lowerbound(list, value) - returns the index of the first entry in list
                         // that does not compare less than value, analogous to std::lower_bound
                         // will return -1 if no value found.


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

    // add special functions
    returning <<"isRide"<<"isSwim"<<"isXtrain"; // isRun is included in RideNavigator

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
            returning << "banister(load_metric, perf_metric, nte|pte|perf|cp|date)";

        } else if (i == 45) {

            // concat
            returning << "c(...)";

        } else if (i == 46) {

            // seq
            returning << "seq(start,stop,step)";

        } else if (i == 47) {

            // seq
            returning << "rep(value,n)";

        } else if (i == 48) {

            // length
            returning << "length(vector)";

        } else if (i == 49) {
            // append
            returning << "append(a,b,[pos])"; // position is optional

        } else if (i == 50) {
            // remove
            returning << "remove(a,pos,count)";

        } else if (i == 51) {
            // mid
            returning << "mid(a,pos,count)"; // subset

        } else if (i == 52) {

            // get a vector of data series samples
            returning << "samples(POWER|HR etc)";

        } else if (i == 53) {

            // get a vector of activity metrics - date is integer days since 1900
            returning << "metrics(symbol|date [,start [,stop]])";

        } else if (i == 54) {

            // argsort
            returning << "argsort(ascend|descend, list)";

        } else if (i == 55) {

            // argsort
            returning << "sort(ascend|descend, list [, list2 .. ,listn])";

        } else if (i == 56) {

            // head
            returning << "head(list, n)";

        } else if (i == 57) {

            // tail
            returning << "tail(list, n)";

        } else if (i== 58) {
            // meanmax
            returning << "meanmax(POWER|WPK|HR|CADENCE|SPEED)";

        } else if (i == 59) {

            // pmc
            returning << "pmc(metric, stress|lts|sts|sb|rr|date)";

        } else if (i == 60) {

            // sapply
            returning << "sapply(list, expr)";

        } else if (i == 61) {

            returning << "lr(xlist, ylist)";

        } else if (i == 64) {

            returning << "lm(formula, xlist, ylist)";

        } else if (i == 66) {

            returning << "annotate(label, ...)";

        } else if (i == 67) {

            returning << "arguniq(list)";

        } else if (i == 68) {

            returning << "uniq(list [,list n])";

        } else if (i == 71) {

            returning << "curve(series, x|y|z|d|t)";

        } else if (i == 72) {

            returning << "lowerbound(list, value";

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
                    sym == "isRide" || sym == "isSwim" ||
                    sym == "isRun" || sym == "isXtrain") {
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

void
Leaf::findSymbols(QStringList &symbols)
{
    switch(type) {
    case Leaf::Script :
        break;
    case Leaf::Compound :
        foreach(Leaf *p, *(lvalue.b)) p->findSymbols(symbols);
        break;

    case Leaf::Float :
    case Leaf::Integer :
    case Leaf::String :
        break;
    case Leaf::Symbol :
        {
            symbols << (*lvalue.n);
        }
        break;
    case Leaf::Operation:
    case Leaf::BinaryOperation:
    case Leaf::Logical :
        lvalue.l->findSymbols(symbols);
        if (op) rvalue.l->findSymbols(symbols);
        break;
        break;
    case Leaf::UnaryOperation:
        lvalue.l->findSymbols(symbols);
        break;
    case Leaf::Function:
        foreach(Leaf* l, fparms) l->findSymbols(symbols);
        break;
    case Leaf::Index:
    case Leaf::Select:
        lvalue.l->findSymbols(symbols);
        fparms[0]->findSymbols(symbols);
        break;
    case Leaf::Conditional:
        cond.l->findSymbols(symbols);
        lvalue.l->findSymbols(symbols);
        if (rvalue.l) rvalue.l->findSymbols(symbols);
        break;

    default:
        break;
    }
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
            Result value = df->symbols.value(*lvalue.n), Result(11);
            qDebug()<<"symbol"<<*lvalue.n<<dynamic<<value.number;

            // output vectors, truncate to 20 els
            if (value.vector.count()) {
                QString output = QString("Vector[%1]:").arg(value.vector.count());
                for(int i=0; i<value.vector.count() && i<20; i++)
                    output += QString("%1, ").arg(value.vector[i]);
                qDebug() <<output;
            }
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
    case Leaf::Select:
        qDebug()<<"select";
        lvalue.l->print(level+1, df);
        fparms[0]->print(level+1,df);
        break;
    case Leaf::Index:
        qDebug()<<"index";
        lvalue.l->print(level+1, df);
        fparms[0]->print(level+1,df);
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
            else
                return false;
        }
    case Leaf::Symbol :
        {
            QString symbol = *(leaf->lvalue.n);
            if (df->symbols.contains(symbol)) return true;
            if (symbol == "isRide" || symbol == "isSwim" ||
                symbol == "isRun" || symbol == "isXtrain") return true;
            if (symbol == "x" || symbol == "i") return true;
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
    case Leaf::Select :
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
                    symbol.compare("x", Qt::CaseInsensitive) && // used by which and [lexpr]
                    symbol.compare("i", Qt::CaseInsensitive) && // used by which and [lexpr]
                    symbol.compare("Today", Qt::CaseInsensitive) &&
                    symbol.compare("Current", Qt::CaseInsensitive) &&
                    symbol.compare("RECINTSECS", Qt::CaseInsensitive) &&
                    symbol.compare("NA", Qt::CaseInsensitive) &&
                    !df->dataSeriesSymbols.contains(symbol) &&
                    symbol != "isRide" && symbol != "isSwim" &&
                    symbol != "isRun" && symbol != "isXtrain" &&
                    !isCoggan(symbol)) {

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

    case Leaf::Select:
        {
            // anything goes really.
            leaf->validateFilter(context, df, leaf->fparms[0]);
            leaf->validateFilter(context, df, leaf->lvalue.l);
        }
        return;

    case Leaf::Index :
        {
            //if (leaf->lvalue.l->type != Leaf::Symbol) {
            leaf->validateFilter(context, df, leaf->fparms[0]);
            if (!Leaf::isNumber(df, leaf->fparms[0])) {
                leaf->fparms[0]->inerror = true;
                DataFiltererrors << QString(tr("Index must be numeric."));
            }
            leaf->validateFilter(context, df, leaf->lvalue.l);
        }
        return;

    case Leaf::Function :
        {
            // is the symbol valid?
            QRegExp bestValidSymbols("^(apower|power|hr|cadence|speed|torque|vam|xpower|isopower|wpk)$", Qt::CaseInsensitive);
            QRegExp tizValidSymbols("^(power|hr)$", Qt::CaseInsensitive);
            QRegExp configValidSymbols("^(cranklength|cp|ftp|w\\'|pmax|cv|d\\'|scv|sd\\'|height|weight|lthr|maxhr|rhr|units)$", Qt::CaseInsensitive);
            QRegExp constValidSymbols("^(e|pi)$", Qt::CaseInsensitive); // just do basics for now
            QRegExp dateRangeValidSymbols("^(start|stop)$", Qt::CaseInsensitive); // date range
            QRegExp pmcValidSymbols("^(stress|lts|sts|sb|rr|date)$", Qt::CaseInsensitive);
            QRegExp smoothAlgos("^(sma|ewma)$", Qt::CaseInsensitive);
            QRegExp annotateTypes("^(label)$", Qt::CaseInsensitive);
            QRegExp curveData("^(x|y|z|d|t)$", Qt::CaseInsensitive);

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

                } else if (leaf->function == "c") {

                    // pretty much anything goes, can be empty or a list..
                    // .. but still check parameters
                    foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);

                } else if (leaf->function == "rep") {

                    if (leaf->fparms.count() != 2) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be rep(value, n)"));
                    }

                    // still normal parm check !
                    foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);

                } else if (leaf->function == "seq") {

                    if (leaf->fparms.count() != 3) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be seq(start, stop, step)"));
                    }

                    // still normal parm check !
                    foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);

                } else if (leaf->function == "length") {

                    if (leaf->fparms.count() != 1) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be length(expr)"));
                    }

                    // still normal parm check !
                    foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);

                } else if (leaf->function == "append") {

                    if (leaf->fparms.count() != 2 && leaf->fparms.count() != 3) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be append(a,b,[pos])"));
                    } else {

                        // still normal parm check !
                        foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);

                        // check parameter 1 is actually a symbol
                        if (leaf->fparms[0]->type != Leaf::Symbol) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("append(a,b,[pos]) but 'a' must be a symbol"));
                        } else {
                            QString symbol = *(leaf->fparms[0]->lvalue.n);
                            if (!df->symbols.contains(symbol)) {
                                DataFiltererrors << QString(tr("append(a,b,[pos]) but 'a' must be a user symbol"));
                                leaf->inerror = true;
                            }
                        }
                    }

                } else if (leaf->function == "remove") {

                    if (leaf->fparms.count() != 3) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be remove(a,pos,count)"));
                    } else {
                        // still normal parm check !
                        foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);

                        // check parameter 1 is actually a symbol
                        if (leaf->fparms[0]->type != Leaf::Symbol) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("remove(a,pos,count) but 'a' must be a symbol"));
                        } else {
                            QString symbol = *(leaf->fparms[0]->lvalue.n);
                            if (!df->symbols.contains(symbol)) {
                                DataFiltererrors << QString(tr("remove(a,pos, count) but 'a' must be a user symbol"));
                                leaf->inerror = true;
                            }

                        }

                    }

                } else if (leaf->function == "mid") {

                    if (leaf->fparms.count() != 3) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be mid(a,pos,count)"));
                    }

                } else if (leaf->function == "samples") {

                    if (leaf->fparms.count() < 1) {
                        leaf->inerror=true;
                        DataFiltererrors << QString(tr("samples(SERIES), SERIES should be POWER, SECS, HEARTRATE etc."));

                    } else if (leaf->fparms[0]->type != Leaf::Symbol) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("samples(SERIES), SERIES should be POWER, SECS, HEARTRATE etc."));
                    } else {
                        QString symbol=*(leaf->fparms[0]->lvalue.n);
                        leaf->seriesType = RideFile::seriesForSymbol(symbol);
                        if (leaf->seriesType==RideFile::none) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("invalid series name '%1'").arg(symbol));
                        }
                    }

                } else if (leaf->function == "metrics") {

                    // is the param a symbol and either a metric name or 'date'
                    if (leaf->fparms.count() < 1 || leaf->fparms[0]->type != Leaf::Symbol) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("metrics(symbol|date), symbol should be a metric name"));

                    } else if (leaf->fparms.count() >= 1) {

                        QString symbol=*(leaf->fparms[0]->lvalue.n);
                        if (symbol != "date" && df->lookupMap.value(symbol,"") == "") {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("invalid symbol '%1', should be either a metric name or 'date'").arg(symbol));
                        }
                    } else if (leaf->fparms.count() >= 2) {

                        // validate what was passed as second value - can be number or datestring
                        validateFilter(context, df, leaf->fparms[1]);

                    } else if (leaf->fparms.count() == 3) {

                        // validate what was passed as second value - can be number or datestring
                        validateFilter(context, df, leaf->fparms[2]);

                    } else if (leaf->fparms.count() > 3) {

                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("too many parameters: metrics(symbol|date, start, stop)"));
                    }

                } else if (leaf->function == "sort") {

                    if (leaf->fparms.count() < 2) {
                        leaf->inerror = true;
                       DataFiltererrors << QString(tr("sort(ascend|descend, list [, .. list n])"));
                    }

                    // need ascend|descend then a list
                    if (leaf->fparms.count() > 0 && leaf->fparms[0]->type != Leaf::Symbol) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("sort(ascend|descend, list [, .. list n]), need to specify ascend or descend"));
                    }

                    // need all remaining parameters to be symbols
                    for(int i=1; i<fparms.count(); i++) {

                        // check parameter is actually a symbol
                        if (leaf->fparms[i]->type != Leaf::Symbol) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("sort: list arguments must be a symbol"));
                        } else {
                            QString symbol = *(leaf->fparms[i]->lvalue.n);
                            if (!df->symbols.contains(symbol)) {
                                DataFiltererrors << QString(tr("'%1' is not a user symbol").arg(symbol));
                                leaf->inerror = true;
                            }

                        }
                    }

                } else if (leaf->function == "argsort") {

                    // need ascend|descend then a list
                    if (leaf->fparms.count() < 1 || leaf->fparms[0]->type != Leaf::Symbol) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("argsort(ascend|descend, list), need to specify ascend or descend"));
                    }

                } else if (leaf->function == "uniq") {

                    if (leaf->fparms.count() < 1) {
                        leaf->inerror = true;
                       DataFiltererrors << QString(tr("uniq(list [, .. list n])"));
                    }

                    // need all remaining parameters to be symbols
                    for(int i=0; i<fparms.count(); i++) {

                        // check parameter is actually a symbol
                        if (leaf->fparms[i]->type != Leaf::Symbol) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("uniq: list arguments must be a symbol"));
                        } else {
                            QString symbol = *(leaf->fparms[i]->lvalue.n);
                            if (!df->symbols.contains(symbol)) {
                                DataFiltererrors << QString(tr("'%1' is not a user symbol").arg(symbol));
                                leaf->inerror = true;
                            }

                        }
                    }

                } else if (leaf->function == "arguniq") {

                    // need ascend|descend then a list
                    if (leaf->fparms.count() != 1 ||  leaf->fparms[0]->type != Leaf::Symbol) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("argsort(ascend|descend, list), need to specify ascend or descend"));
                    } else {
                        QString symbol = *(leaf->fparms[0]->lvalue.n);
                        if (!df->symbols.contains(symbol)) {
                            DataFiltererrors << QString(tr("'%1' is not a user symbol").arg(symbol));
                            leaf->inerror = true;
                        }
                    }

                } else if (leaf->function == "curve") {

                    // on a user chart we can access computed values
                    // for other series, reduces overhead etc
                    if (leaf->fparms.count() != 2 || leaf->fparms[0]->type != Leaf::Symbol || leaf->fparms[1]->type != Leaf::Symbol) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("curve(seriesname, x|y|z|d|t), need to specify series name and data."));
                    } else {
                        // check series data
                        QString symbol = *(leaf->fparms[1]->lvalue.n);
                        if (!curveData.exactMatch(symbol)) {
                            DataFiltererrors << QString(tr("'%1' is not a valid, x, y, z, d or t expected").arg(symbol));
                            leaf->inerror = true;
                        }
                    }

                } else if (leaf->function == "meanmax") {

                    // is the param 1 a valid data series?
                    if (leaf->fparms.count() < 1 || leaf->fparms[0]->type != Leaf::Symbol) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("meanmax(SERIES), SERIES should be POWER, HEARTRATE etc."));
                    } else {
                        QString symbol=*(leaf->fparms[0]->lvalue.n);
                        leaf->seriesType = RideFile::seriesForSymbol(symbol);
                        if (symbol != "efforts" && leaf->seriesType==RideFile::none) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("invalid series name '%1'").arg(symbol));
                        }
                    }

                } else if (leaf->function == "annotate") {

                    if (leaf->fparms.count() < 2 || leaf->fparms[0]->type != Leaf::Symbol) {

                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("annotate(label, list of strings, numbers) need at least 2 parameters."));

                    } else {

                        QString type = *(leaf->fparms[0]->lvalue.n);
                        if (!annotateTypes.exactMatch(type)) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("annotation type '%1' not available").arg(type));
                        }
                    }

                } else if (leaf->function == "smooth") {

                    if (leaf->fparms.count() < 2 || leaf->fparms[1]->type != Leaf::Symbol) {

                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("smooth(list, algorithm [,parameters]) need at least 2 parameters."));

                    } else {

                        QString algo = *(leaf->fparms[1]->lvalue.n);
                        if (!smoothAlgos.exactMatch(algo)) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("smoothing algorithm '%1' not available").arg(algo));
                        } else {

                            if (algo == "sma") {
                                // smooth(list, sma, centred|forward|backward, window)
                                if (leaf->fparms.count() != 4 || leaf->fparms[2]->type != Leaf::Symbol) {
                                    leaf->inerror = true;
                                    DataFiltererrors << QString(tr("smooth(list, sma, forward|centered|backward, windowsize"));

                                } else {
                                    QRegExp parms("^(forward|centered|backward)$");
                                    QString parm1 = *(leaf->fparms[2]->lvalue.n);
                                    if (!parms.exactMatch(parm1)) {

                                        leaf->inerror = true;
                                        DataFiltererrors << QString(tr("smooth(list, sma, forward|centered|backward, windowsize"));
                                    }

                                    // check list and windowsize
                                    validateFilter(context, df, leaf->fparms[0]);
                                    validateFilter(context, df, leaf->fparms[3]);
                                }

                            } else if (algo == "ewma") {
                                // smooth(list, ewma, alpha)

                                if (leaf->fparms.count() != 3) {
                                    leaf->inerror = true;
                                    DataFiltererrors << QString(tr("smooth(list, ewma, alpha (between 0 and 1)"));
                                } else {
                                    // check list and windowsize
                                    validateFilter(context, df, leaf->fparms[0]);
                                    validateFilter(context, df, leaf->fparms[2]);
                                }
                            }
                        }
                    }

                } else if (leaf->function == "lowerbound") {

                    if (leaf->fparms.count() != 2) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("lowerbound(list, value), need list and value to find"));

                    } else {
                        validateFilter(context, df, leaf->fparms[0]);
                        validateFilter(context, df, leaf->fparms[1]);
                    }

                } else if (leaf->function == "lr") {

                    if (leaf->fparms.count() != 2) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("lr(x, y), need x and y vectors."));

                    } else {
                        validateFilter(context, df, leaf->fparms[0]);
                        validateFilter(context, df, leaf->fparms[1]);
                    }

                } else if (leaf->function == "lm") {

                    if (leaf->fparms.count() != 3) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("lm(expr, xlist, ylist), need formula, x and y data to fit to."));

                    } else {

                        // at this point pretty much anything goes, so long as it is
                        // syntactically correct. user needs to know what they are doing !
                        validateFilter(context, df, leaf->fparms[0]);
                        validateFilter(context, df, leaf->fparms[1]);
                        validateFilter(context, df, leaf->fparms[2]);

                        QStringList symbols;
                        leaf->fparms[0]->findSymbols(symbols);
                        symbols.removeDuplicates();

                        // do we have any parameters that are not x ????
                        int count=0;
                        foreach(QString s, symbols)  if (s != "x") count++;

                        if (count == 0) {
                            leaf->inerror= true;
                            DataFiltererrors << QString(tr("lm(expr, xlist, ylist), formula must have at least one parameter to estimate.\n"));
                        }
                    }

                } else if (leaf->function == "sapply") {

                    if (leaf->fparms.count() != 2) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("sapply(list, expr), need 2 parameters."));
                    } else {
                        validateFilter(context, df, leaf->fparms[0]);
                        validateFilter(context, df, leaf->fparms[1]);
                    }

                } else if (leaf->function == "pmc") {

                    if (leaf->fparms.count() < 2 || leaf->fparms[1]->type != Leaf::Symbol) {

                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("pmc(metric, stress|lts|sts|sb|rr|date), need to specify a metric and series."));

                    } else {

                        // expression good?
                        validateFilter(context, df, leaf->fparms[0]);

                        QString symbol=*(leaf->fparms[1]->lvalue.n);
                        if (!pmcValidSymbols.exactMatch(symbol)) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("invalid PMC series '%1'").arg(symbol));
                        }
                    }

                } else if (leaf->function == "banister") {

                    // 3 parameters
                    if (leaf->fparms.count() != 3) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be banister(load_metric, perf_metric, nte|pte|perf|cp|date)"));
                    } else {

                        Leaf *first=leaf->fparms[0];
                        Leaf *second=leaf->fparms[1];
                        Leaf *third=leaf->fparms[2];

                        // check load metric name is valid
                        QString metric = first->signature();
                        QString lookup = df->lookupMap.value(metric, "");
                        if (lookup == "") {
                            leaf->inerror = true;
                            DataFiltererrors << QString("unknown load metric '%1'.").arg(metric);
                        }

                        // check perf metric name is valid
                        metric = second->signature();
                        lookup = df->lookupMap.value(metric, "");
                        if (lookup == "") {
                            leaf->inerror = true;
                            DataFiltererrors << QString("unknown perf metric '%1'.").arg(metric);
                        }

                        // check value
                        QString value = third->signature();
                        QRegExp banSymbols("^(nte|pte|perf|cp|date)$", Qt::CaseInsensitive);
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
                                !symbol.compare("i", Qt::CaseInsensitive) || // used by which
                                !symbol.compare("Today", Qt::CaseInsensitive) ||
                                !symbol.compare("Current", Qt::CaseInsensitive) ||
                                !symbol.compare("RECINTSECS", Qt::CaseInsensitive) ||
                                !symbol.compare("Device", Qt::CaseInsensitive) ||
                                !symbol.compare("NA", Qt::CaseInsensitive) ||
                                df->dataSeriesSymbols.contains(symbol) ||
                                symbol == "isRide" || symbol == "isSwim" ||
                                symbol == "isRun" || symbol == "isXtrain" ||
                                isCoggan(symbol)) {
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

                // assigm to user symbol - also creates first reference
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

                // assign to symbol[i] - must be to a user symbol
                } else if (leaf->lvalue.l->type == Leaf::Index) {

                    // this is being used in assignment so MUST reference
                    // a user symbol to be of any use
                    if (leaf->lvalue.l->lvalue.l->type != Leaf::Symbol) {

                        DataFiltererrors << QString(tr("array assignment must be to symbol."));
                        leaf->inerror = true;

                    } else {

                        // lets make sure it exists as a user symbol
                        QString symbol = *(leaf->lvalue.l->lvalue.l->lvalue.n);
                        if (!df->symbols.contains(symbol)) {
                            DataFiltererrors << QString(tr("'%1' unknown variable").arg(symbol));
                            leaf->inerror = true;
                        }
                    }
                } else if (leaf->lvalue.l->type == Leaf::Select) {

                        DataFiltererrors << QString(tr("assign to selection not supported at present. sorry."));
                        leaf->inerror = true;
                }

                // validate rhs is numeric
                bool rhsType = Leaf::isNumber(df, leaf->rvalue.l);
                if (!rhsType) {
                    DataFiltererrors << QString(tr("variables must be numeric."));
                    leaf->inerror = true;
                }

                // validate the lhs anyway
                validateFilter(context, df, leaf->lvalue.l);

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
    // let folks know who owns this rumtime for signalling
    rt.owner = this;
    rt.chart = NULL;

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
    // let folks know who owns this rumtime for signalling
    rt.owner = this;
    rt.chart = NULL;

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
            res = treeRoot->eval(&rt, rt.functions.value("main"), 0, 0, item, p);

    } else {

        // otherwise just evaluate the entire tree
        res = treeRoot->eval(&rt, treeRoot, 0, 0, item, p);
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
            Result result = treeRoot->eval(&rt, treeRoot, 0, 0,item, NULL);
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
            Result result = treeRoot->eval(&rt, treeRoot, 0, 0, item, NULL);
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

static double myisinf(double x) { return std::isinf(x); }
static double myisnan(double x) { return std::isnan(x); }

void
Result::vectorize(int count)
{

    if (vector.count() >= count) return;

    // ok, so must have at least 1 element to repeat
    if (vector.count() == 0) vector << number;

    // repeat for size
    int it=0;
    int n=vector.count();

    // repeat whatever we have
    while (vector.count() < count) {
        vector << vector[it];
        number += vector[it];

        // loop thru wot we have
        it++; if (it == n) it=0;
    }
}

// used by lowerbound
struct comparedouble { bool operator()(const double p1, const double p2) { return p1 < p2; } };

Result Leaf::eval(DataFilterRuntime *df, Leaf *leaf, float x, long it, RideItem *m, RideFilePoint *p, const QHash<QString,RideMetric*> *c, Specification s, DateRange d)
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
                Result left = eval(df, leaf->lvalue.l,x, it, m, p, c, s, d);
                if (left.isNumber && left.number) {
                    Result right = eval(df, leaf->rvalue.l,x, it, m, p, c, s, d);
                    if (right.isNumber && right.number) return Result(true);
                }
                return Result(false);
            }
            case OR :
            {
                Result left = eval(df, leaf->lvalue.l,x, it, m, p, c, s, d);
                if (left.isNumber && left.number) return Result(true);

                Result right = eval(df, leaf->rvalue.l,x, it, m, p, c, s, d);
                if (right.isNumber && right.number) return Result(true);

                return Result(false);
            }

            default : // parenthesis
                return (eval(df, leaf->lvalue.l,x, it, m, p, c, s, d));
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

            Result res = eval(df, df->functions.value(leaf->function),x, it, m, p, c, s, d);

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

        // bool(expr)
        if (leaf->function == "bool") {
            Result r=eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            if (r.number != 0) return Result(1);
            else return Result(0);
        }

        // c (concat into a vector)
        if (leaf->function == "c") {

            // the return value of a vector is always its sum
            // so we need to keep that up to date too
            Result returning(0);

            for(int i=0; i<leaf->fparms.count(); i++) {
                Result r=eval(df, leaf->fparms[i],x, it, m, p, c, s, d);
                if (r.vector.count()) {
                    returning.vector.append(r.vector);
                    returning.number += r.number;
                } else if (r.isNumber) {
                    returning.vector.append(r.number);
                    returning.number += r.number;
                } else {
                    // convert strings to a number, or at least try
                    double val = r.string.toDouble();
                    returning.number += val;
                    returning.vector.append(val);
                }
            }
            return returning;
        }

        // seq
        if (leaf->function == "seq") {
            Result returning(0);

            double start= eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number;
            double stop= eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number;
            double step= eval(df, leaf->fparms[2],x, it, m, p, c, s, d).number;

            if (step == 0) return returning; // nope!
            if (start > stop && step >0) return returning; // nope
            if (stop > start && step <0) return returning; // nope

            // ok lets go
            if (step > 0) {
                while(start <= stop) {
                    returning.vector.append(start);
                    start += step;
                    returning.number += start;
                }
            } else {
                while (start >= stop) {
                    returning.vector.append(start);
                    start += step;
                    returning.number += start;
                }
            }

            // sequence
            return returning;
        }

        // rep
        if (leaf->function == "rep") {
            Result returning(0);

            double value= eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number;
            double count= eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number;

            if (count <= 0) return returning;

            while (count >0) {
                returning.vector.append(value);
                returning.number += value;
                count--;
            }
            return returning;
        }

        // length
        if (leaf->function == "length") {
            double len = eval(df, leaf->fparms[0],x, it, m, p, c, s, d).vector.count();
            //fprintf(stderr, "len: %f\n",len); fflush(stderr);
            return Result(len);
        }

        // append
        if (leaf->function == "append") {

            // append (symbol, stuff, pos)

            // lets get the symbol and a pointer to it's value
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            Result current = df->symbols.value(symbol);

            // where to place it? -1 on end (not passed as a parameter)
            int pos=-1;
            if (leaf->fparms.count() == 3) pos = eval(df, leaf->fparms[2],x, it, m, p, c, s, d).number;

            // check for bounds
            if (pos <0 || pos >current.vector.count()) pos=-1;

            // ok, what to append
            Result append = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

            // do it...
            if (append.vector.count()) {

                if (pos==-1) current.vector.append(append.vector);
                else {
                    // insert at pos
                    for(int i=0; i<append.vector.count(); i++)
                        current.vector.insert(pos+i, append.vector.at(i));
                }

            } else {

                if (pos == -1) current.vector.append(append.number); // just a single number
                else current.vector.insert(pos, append.number); // just a single number
            }

            // update value
            df->symbols.insert(symbol, current);

            return Result(current.vector.count());
        }

        // remove
        if (leaf->function == "remove") {

            // remove (symbol, pos, count)

            // lets get the symbol and a pointer to it's value
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            Result current = df->symbols.value(symbol);

            // where to place it? -1 on end (not passed as a parameter)
            long pos = eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number;

            // ok, what to append
            long count = eval(df, leaf->fparms[2],x, it, m, p, c, s, d).number;


            // check.. and return unchanged if out of bounds
            if (pos < 0 || pos > current.vector.count() || pos+count >current.vector.count()) {
                return Result(current.vector.count());
            }

            // so lets do it
            // do it...
            current.vector.remove(pos, count);

            // update value
            df->symbols.insert(symbol, current);

            return Result(current.vector.count());
        }

        // mid
        if (leaf->function == "mid") {

            Result returning(0);

            // mid (a, pos, count)
            // where to place it? -1 on end (not passed as a parameter)
            Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            long pos = eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number;
            long count = eval(df, leaf->fparms[2],x, it, m, p, c, s, d).number;


            // check.. and return unchanged if out of bounds
            if (pos < 0 || pos > v.vector.count() || pos+count >v.vector.count()) {
                return returning;
            }

            // so lets do it- remember to sum
            returning.vector = v.vector.mid(pos, count);
            returning.number = 0;
            for(int i=0; i<returning.vector.count(); i++) returning.number += returning.vector[i];

            return returning;
        }

        if (leaf->function == "samples") {

            // nothing to return -- note we check if the ride is open
            // this is to avoid misuse outside of a filter when working
            // with a specific ride.
            if (m == NULL || !m->isOpen() || m->ride(false) == NULL || m->ride(false)->dataPoints().count() == 0) {
                return Result(0);

            } else {

                // create a vector for the currently selected ride.
                // should be used with care by the user !!
                // if they use it in a filter or metric sample() function
                // it could get ugly.. but thats no reason to avoid
                // the usefulness of getting the entire data series
                // in one hit for those that want to work with vectors
                Result returning(0);
                foreach(RideFilePoint *p, m->ride()->dataPoints()) {
                    double value=p->value(leaf->seriesType);
                    returning.number += value;
                    returning.vector.append(value);
                }
                return returning;
            }
        }

        if (leaf->function == "metrics") {

            QDate earliest(1900,01,01);
            bool wantdate=false;
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            if (symbol == "date") wantdate=true;
            Result returning(0);

            FilterSet fs;
            fs.addFilter(m->context->isfiltered, m->context->filters);
            fs.addFilter(m->context->ishomefiltered, m->context->homeFilters);
            Specification spec;
            spec.setFilterSet(fs);

            // date range can be controlled, if no date range is set then we just
            // use the currently selected date range, otherwise start - today or start - stop
            if (leaf->fparms.count() == 3 && Leaf::isNumber(df, leaf->fparms[1]) && Leaf::isNumber(df, leaf->fparms[2])) {

                // start to stop
                Result b = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
                QDate start = earliest.addDays(b.number);

                Result e = eval(df, leaf->fparms[2],x, it, m, p, c, s, d);
                QDate stop = earliest.addDays(e.number);

                spec.setDateRange(DateRange(start,stop));

            } else if (leaf->fparms.count() == 2 && Leaf::isNumber(df, leaf->fparms[1])) {

                // start to today
                Result b = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
                QDate start = earliest.addDays(b.number);
                QDate stop = QDate::currentDate();

                spec.setDateRange(DateRange(start,stop));

            } else {
                spec.setDateRange(d); // fallback to daterange selected
            }

            // loop through rides for daterange
            int count=0;
            foreach(RideItem *ride, m->context->athlete->rideCache->rides()) {

                if (!s.pass(ride)) continue; // relies upon the daterange being passed to eval...
                if (!spec.pass(ride)) continue; // relies upon the daterange being passed to eval...

                count++;

                double value=0;
                if(wantdate) value= QDate(1900,01,01).daysTo(ride->dateTime.date());
                else value =  ride->getForSymbol(df->lookupMap.value(symbol,""));
                returning.number += value;
                returning.vector.append(value);
            }
            return returning;
        }

        // meanmax array
        if (leaf->function == "meanmax") {

            Result returning(0);

            QString symbol = *(leaf->fparms[0]->lvalue.n);

            // go get it for the current date range
            bool rangemode = true;
            if (d.from==QDate() && d.to==QDate()) {

                // not in rangemode
                rangemode = false;

                // the ride mean max
                if (symbol == "efforts") {

                    // keep all from a ride -- XXX TODO averaging tails removal...
                    returning.vector = m->fileCache()->meanMaxArray(RideFile::watts);
                    for(int i=0; i<returning.vector.count(); i++) returning.vector[i]=i;

                } else returning.vector = m->fileCache()->meanMaxArray(leaf->seriesType);

            } else {

                // use a season meanmax
                RideFileCache bestsCache(m->context, d.from, d.to, false, QStringList(), true, NULL);

                // get meanmax, unless its efforts, where we do rather more...
                if (symbol != "efforts") returning.vector = bestsCache.meanMaxArray(leaf->seriesType);

                else {

                    // get power anyway
                    returning.vector = bestsCache.meanMaxArray(RideFile::watts);

                    // need more than 2 entries
                    if (returning.vector.count() > 3) {

                        // we need to return an index to use to filter values
                        // in the meanmax array; remove sub-maximals by
                        // averaging tails or clearly submaximal using the
                        // same filter that is used in the CP plot

                        // get data to filter
                        QVector<double> t;
                        t.resize(returning.vector.size());
                        for (int i=0; i<t.count(); i++) t[i]=i;

                        QVector<double> p = returning.vector;
                        QVector<QDate> w = bestsCache.meanMaxDates(leaf->seriesType);
                        t.remove(0);
                        p.remove(0);
                        w.remove(0);


                        // linear regression of the full data, to help determine
                        // the maximal point on the MMP curve for each day
                        // using brace to set scope and descope temporary variables
                        // as we use a fair few, but not worth making a function
                        double slope=0, intercept=0;
                        {
                            // we want 2m to 20min data (check bounds)
                            int want = p.count() > 1200 ? 1200-121 : p.count()-121;
                            QVector<double> j = p.mid(120, want);
                            QVector<double> ts = t.mid(120, want);

                            // convert time data to seconds (is in minutes)
                            // and power to joules (power x time)
                            for(int i=0; i<j.count(); i++) {
                                ts[i] = ts[i];
                                j[i] = (j[i] * ts[i]) ;
                            }

                            // LTMTrend does a linear regression for us, lets reuse it
                            // I know, we see, to have a zillion ways to do this...
                            LTMTrend regress(ts.data(), j.data(), ts.count());

                            // save away the slope and intercept
                            slope = regress.slope();
                            intercept = regress.intercept();
                        }

                        // filter out efforts on same day that are the furthest
                        // away from a linear regression

                        // the best we found is stored in here
                        struct { int i; double p, t, d, pix; } keep;

                        for(int i=0; i<t.count(); i++) {

                            // reset our holding variable - it will be updated
                            // with the maximal point we want to retain for the
                            // day we are filtering for. Initial means no value
                            // has been set yet, so the first point will set it.
                            if (w[i] != QDate()) {

                                // lets filter all on today, use first one to set the best found so far
                                keep.d = (p[i] * t[i] ) - ((slope * t[i] ) + intercept);
                                keep.i=i;
                                keep.p=p[i];
                                keep.t=t[i];
                                keep.pix=powerIndex(keep.p, keep.t);

                                // but clear since we iterate beyond
                                if (i>0) { // always keep pmax point
                                    p[i]=0;
                                    t[i]=0;
                                }

                                // from here to the end of all the points, lets see if there is one further away?
                                for(int z=i+1; z<t.count(); z++) {

                                    if (w[z] == w[i]) {

                                        // if its beloe the line multiply distance by -1
                                        double d = (p[z] * t[z] ) - ((slope * t[z]) + intercept);
                                        double pix = powerIndex(p[z],t[z]);

                                        // use the regression for shorter durations and 3p for longer
                                        if ((keep.t < 120 && keep.d < d) || (keep.t >= 120 && keep.pix < pix)) {
                                            keep.d = d;
                                            keep.i = z;
                                            keep.p = p[z];
                                            keep.t = t[z];
                                        }

                                        if (z>0) { // always keep pmax point
                                            w[z] = QDate();
                                            p[z] = 0;
                                            t[z] = 0;
                                        }
                                    }
                                }

                                // reinstate best we found
                                p[keep.i] = keep.p;
                                t[keep.i] = keep.t;
                            }
                        }

                        // so lets send over the indexes
                        // we keep t[0] as it gets removed in
                        // all cases below, saves a bit of
                        // torturous logic later
                        returning.vector.clear();
                        returning.vector << 0;// it gets removed
                        for(int i=0; i<t.count(); i++)  if (t[i] > 0) returning.vector << i;
                    }
                }
            }

            // really annoying that it starts from 0s not 1s, this is a legacy
            // bug that we cannot fix easily, but this is new, so lets not
            // have that damned 0s 0w entry!
            if (returning.vector.count()>0) returning.vector.remove(0);

            // compute the sum, ugh.
            for(int i=0; i<returning.vector.count(); i++) returning.number += returning.vector[i];

            // return a vector
            return returning;
        }

        // argsort
        if (leaf->function == "argsort") {
            Result returning(0);

            // ascending or descending?
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            bool ascending= (symbol=="ascend") ? true : false;

            // use the utils function to actually do it
            Result v = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
            QVector<int> r = Utils::argsort(v.vector, ascending);

            // put the index into the result we are returning.
            foreach(int x, r) {
                returning.vector << static_cast<double>(x);
                returning.number += x;
            }

            return returning;
        }

        // arguniq
        if (leaf->function == "arguniq") {
            Result returning(0);

            // get vector and an argsort
            Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            QVector<int> r = Utils::arguniq(v.vector);

            for(int i=0; i<r.count(); i++) {
                returning.vector << r[i];
                returning.number += r[i];
            }
            return returning;
        }

        // uniq
        if (leaf->function == "uniq") {

            // evaluate all the lists
            for(int i=0; i<fparms.count(); i++) eval(df, leaf->fparms[i],x, it, m, p, c, s, d);

            // get first and argsort it
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            Result current = df->symbols.value(symbol);
            long len = current.vector.count();
            QVector<int> index = Utils::arguniq(current.vector);

            // sort all the lists in place
            int count=0;
            for (int i=0; i<leaf->fparms.count(); i++) {
                // get the vector
                symbol = *(leaf->fparms[i]->lvalue.n);
                Result current = df->symbols.value(symbol);

                // diff length?
                if (current.vector.count() != len) {
                    fprintf(stderr, "uniq list '%s': not the same length, ignored\n", symbol.toStdString().c_str()); fflush(stderr);
                    continue;
                }

                // ok so now we can adjust
                QVector<double> replace;
                for(int idx=0; idx<index.count(); idx++) replace << current.vector[index[idx]];
                current.vector = replace;

                // replace
                df->symbols.insert(symbol, current);

                count++;
            }
            return Result(count);
        }

        // access user chart curve data, if it's there
        if (leaf->function == "curve") {

            // not on a chart m8
            if (df->chart == NULL) return Result(0);

            Result returning(0);

            // lets see if we can find the series
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            for(int i=0; i<df->chart->seriesinfo.count(); i++) {
                if (df->chart->seriesinfo[i].name == symbol) {
                    // woop!
                    QString data=*(leaf->fparms[1]->lvalue.n);
                    if (data == "x") returning.vector = df->chart->seriesinfo[i].xseries;
                    if (data == "y") returning.vector = df->chart->seriesinfo[i].yseries;
                    // z d t still todo XXX
                }
            }
            return returning;
        }

        // lowerbound
        if (leaf->function == "lowerbound") {

            Result returning(-1);

            Result list= eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            Result value= eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

            // empty list - error
            if (list.vector.count() == 0) return returning;

            // lets do it with std::lower_bound then
            QVector<double>::const_iterator i = std::lower_bound(list.vector.begin(), list.vector.end(), value.number, comparedouble());

            if (i == list.vector.end()) return Result(list.vector.size());
            return Result(i - list.vector.begin());
        }

        // sort
        if (leaf->function == "sort") {

            // ascend/descend?
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            bool ascending=symbol=="ascend" ? true : false;

            // evaluate all the lists
            for(int i=1; i<fparms.count(); i++) eval(df, leaf->fparms[i],x, it, m, p, c, s, d);

            // get first and argsort it
            symbol = *(leaf->fparms[1]->lvalue.n);
            Result current = df->symbols.value(symbol);
            long len = current.vector.count();
            QVector<int> index = Utils::argsort(current.vector, ascending);

            // sort all the lists in place
            int count=0;
            for (int i=1; i<leaf->fparms.count(); i++) {
                // get the vector
                symbol = *(leaf->fparms[i]->lvalue.n);
                Result current = df->symbols.value(symbol);

                // diff length?
                if (current.vector.count() != len) {
                    fprintf(stderr, "sort list '%s': not the same length, ignored\n", symbol.toStdString().c_str()); fflush(stderr);
                    continue;
                }

                // ok so now we can adjust
                QVector<double> replace = current.vector;
                for(int idx=0; idx<index.count(); idx++) replace[idx] = current.vector[index[idx]];
                current.vector = replace;

                // replace
                df->symbols.insert(symbol, current);

                count++;
            }
            return Result(count);
        }

        if (leaf->function == "head") {

            Result returning(0);

            Result list= eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            Result count= eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
            int n=count.number;
            if (n > list.vector.count()) n=list.vector.count();

            if (n<=0) return Result(0);// nope

            returning.vector = list.vector.mid(0, n);
            for(int i=0; i<returning.vector.count(); i++) returning.number += returning.vector[i];

            return returning;
        }

        if (leaf->function == "tail") {

            Result returning(0);

            Result list= eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            Result count= eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
            int n=count.number;
            if (n > list.vector.count()) n=list.vector.count();

            if (n<=0) return Result(0);// nope

            returning.vector = list.vector.mid(list.vector.count()-n, n);
            for(int i=0; i<returning.vector.count(); i++) returning.number += returning.vector[i];

            return returning;
        }

        // sapply
        if (leaf->function == "sapply") {
            Result returning(0);

            Result value = eval(df,leaf->fparms[0],x, it, m, p, c, s, d); // lhs might also be a symbol

            // need a vector, always
            if (!value.vector.count()) return returning;

            // loop and evaluate, non-zero we keep, zero we lose
            for(int i=0; i<value.vector.count(); i++) {
                x = value.vector.at(i);
                double r = eval(df,leaf->fparms[1],x, i, m, p, c, s, d).number;

                // we want it
                returning.vector << r;
                returning.number += r;
            }
            return returning;
        }

        // annotate
        if (leaf->function == "annotate") {


            if (*(leaf->fparms[0]->lvalue.n) == "label") {

                QStringList list;

                // loop through parameters
                for(int i=1; i<leaf->fparms.count(); i++) {

                    if (leaf->fparms[i]->type == Leaf::String)
                        list << *(leaf->fparms[i]->lvalue.s);
                    else {
                        double value =  eval(df,leaf->fparms[i],x, i, m, p, c, s, d).number;
                        list << Utils::removeDP(QString("%1").arg(value));
                    }
                }

                // send the signal.
                if (list.count())  df->owner->annotateLabel(list);
            }
        }

        // smooth
        if (leaf->function == "smooth") {

            Result returning(0);

            // moving average
            if (*(leaf->fparms[1]->lvalue.n) == "sma") {

                QString type =  *(leaf->fparms[2]->lvalue.n);
                int window = eval(df,leaf->fparms[3],x, it, m, p, c, s, d).number;
                Result data = eval(df,leaf->fparms[0],x, it, m, p, c, s, d);
                int pos=2; // fallback

                if (type=="backward") pos=0;
                if (type=="forward") pos=1;
                if (type=="centered") pos=2;

                returning.vector = Utils::smooth_sma(data.vector, pos, window);

            } else if (*(leaf->fparms[1]->lvalue.n) == "ewma") {

                // exponentially weighted moving average
                double alpha = eval(df,leaf->fparms[2],x, it, m, p, c, s, d).number;
                Result data = eval(df,leaf->fparms[0],x, it, m, p, c, s, d);

                returning.vector = Utils::smooth_ewma(data.vector, alpha);
            }

            // sum. ugh.
            for(int i=0; i<returning.vector.count(); i++) returning.number += returning.vector[i];

            return returning;
        }

        // levenberg-marquardt nls
        if (leaf->function == "lm") {
            Result returning(0);
            returning.vector << 0 << -1 << -1 ; // assume failure

            Leaf *formula = leaf->fparms[0];
            Result xv = eval(df,leaf->fparms[1],x, it, m, p, c, s, d);
            Result yv = eval(df,leaf->fparms[2],x, it, m, p, c, s, d);

            // check ok to proceed
            if (xv.vector.count() < 3 || xv.vector.count() != yv.vector.count()) return returning;

            // use the power duration model using for a data filter expression
            DFModel model(m, formula, df);
            bool success = model.fitData(xv.vector, yv.vector);

            if (success) {
                // first  entry is sucess
                returning.vector[0] = 1;

                // second entry is RMSE
                double sume2=0, sum=0;
                for(int index=0; index<xv.vector.count(); index++) {
                    double predict = eval(df,formula, xv.vector[index], 0, m, p, c, s, d).number;
                    double actual = yv.vector[index];
                    double error = predict - actual;
                    sume2 +=  pow(error, 2);
                    sum += predict;
                }
                double mean = sum / xv.vector.count();

                // RMSE
                returning.vector[1] = sqrt(sume2 / double(xv.vector.count()-2));

                // CV
                returning.vector[2] = (returning.vector[1] / mean) * 100.0;
                //fprintf(stderr, "RMSE=%f, CV=%f%% \n", returning.vector[1], returning.vector[2]); fflush(stderr);

            }

            return returning;
        }

        // linear regression
        if (leaf->function == "lr") {
            Result returning(0);
            returning.vector << 0 << 0 << 0 << 0; // set slope, intercept, r2 and see to 0

            Result xv = eval(df,leaf->fparms[0],x, it, m, p, c, s, d);
            Result yv = eval(df,leaf->fparms[1],x, it, m, p, c, s, d);

            // check ok to proceed
            if (xv.vector.count() < 2 || xv.vector.count() != yv.vector.count()) return returning;

            // use the generic calculator, its quick and easy
            GenericCalculator calc;
            calc.initialise();
            for (int i=0; i< xv.vector.count(); i++)
                calc.addPoint(QPointF(xv.vector[i], yv.vector[i]));
            calc.finalise();

            // extract LR results
            returning.vector[0]=calc.m;
            returning.vector[1]=calc.b;
            returning.vector[2]=calc.r2;
            returning.vector[3]=calc.see;
            returning.number = calc.m + calc.b + calc.r2 + calc.see; // sum

            return returning;
        }

        // stddev
        if (leaf->function == "variance") {
            // array
            Result v = eval(df,leaf->fparms[0],x, it, m, p, c, s, d);
            Statistic calc;
            return calc.variance(v.vector, v.vector.count());
        }

        if (leaf->function == "stddev") {
            // array
            Result v = eval(df,leaf->fparms[0],x, it, m, p, c, s, d);
            Statistic calc;
            return calc.standarddeviation(v.vector, v.vector.count());
        }

        // pmc
        if (leaf->function == "pmc") {

            if (d.from==QDate() || d.to==QDate()) return Result(0);

            QString series = *(leaf->fparms[1]->lvalue.n);
            QDateTime earliest(QDate(1900,01,01),QTime(0,0,0));
            PMCData *pmcData = m->context->athlete->getPMCFor(leaf->fparms[0], df); // use default days
            Result returning(0);
            int  si=0;

            for(QDate date=pmcData->start(); date < pmcData->end(); date=date.addDays(1)) {
                // index
                if (date >= d.from && date <= d.to) {
                    double value=0;

                    // lets copy into our array
                    if (series == "date") value = earliest.daysTo(QDateTime(date, QTime(0,0,0)));
                    if (series == "lts") value = pmcData->lts()[si];
                    if (series == "stress") value = pmcData->stress()[si];
                    if (series == "sts") value = pmcData->sts()[si];
                    if (series == "rr") value = pmcData->rr()[si];
                    if (series == "sb") value = pmcData->sb()[si];

                    returning.vector << value;
                    returning.number += value;
                }

                si++;
            }
            return returning;
        }

        // banister
        if (leaf->function == "banister") {
            Leaf *first=leaf->fparms[0];
            Leaf *second=leaf->fparms[1];
            Leaf *third=leaf->fparms[2];

            // check metric name is valid
            QString metric = df->lookupMap.value(first->signature(), "");
            QString perf_metric = df->lookupMap.value(second->signature(), "");
            QString value = third->signature();
            Banister *banister = m->context->athlete->getBanisterFor(metric, perf_metric, 0,0);
            QDateTime earliest(QDate(1900,01,01),QTime(0,0,0));

            // prepare result
            Result returning(0);
            int  si=0;

            for(QDate date=banister->start; date < banister->stop; date=date.addDays(1)) {
                // index
                if (date >= d.from && date <= d.to) {
                    double x=0;

                    // lets copy into our array
                    if (value == "nte") x =banister->data[si].nte;
                    if (value == "pte") x =banister->data[si].pte;
                    if (value == "perf") x =banister->data[si].perf;
                    if (value == "cp") x = banister->data[si].perf ? (banister->data[si].perf * 261.0 / 100.0) : 0;
                    if (value == "date") x = earliest.daysTo(QDateTime(date, QTime(0,0,0)));

                    returning.vector << x;
                    returning.number += x;
                }

                si++;
            }
            return returning;
        }

        // get here for tiz and best
        if (leaf->function == "best" || leaf->function == "tiz") {

            switch (leaf->lvalue.l->type) {

                default:
                case Leaf::Function :
                {
                    duration = eval(df, leaf->lvalue.l,x, it, m, p, c, s, d).number; // duration will be zero if string
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
                    } else if (*(leaf->lvalue.l->lvalue.n) == "i") {
                        duration = it;
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
            case 0 : case 1 : case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10:
            case 11 : case 12: case 13: case 14: case 15: case 16: case 17: case 18: case 19: case 20:
            {
                Result returning(0);

                // TRIG FUNCTIONS

                // bit ugly but cleanest way of doing this without repeating
                // looping stuff - we use a function pointer to save that...
                double (*func)(double);
                switch (fnum) {
                default:
                case 0: func = cos; break;
                case 1 : func = tan; break;
                case 2 : func = sin; break;
                case 3 : func = acos; break;
                case 4 : func = atan; break;
                case 5 : func = asin; break;
                case 6 : func = cosh; break;
                case 7 : func = tanh; break;
                case 8 : func = sinh; break;
                case 9 : func = acosh; break;
                case 10 : func = atanh; break;
                case 11 : func = asinh; break;

                case 12 : func = exp; break;
                case 13 : func = log; break;
                case 14 : func = log10; break;

                case 15 : func = ceil; break;
                case 16 : func = floor; break;
                case 17 : func = round; break;

                case 18 : func = fabs; break;
                case 19 : func = myisinf; break;
                case 20 : func = myisnan; break;
                }

                Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
                if (v.vector.count()) {
                    for(int i=0; i<v.vector.count(); i++) {
                        double r = func(v.vector[i]);
                        returning.vector << r;
                        returning.number += r;
                    }
                } else {
                    returning.number =  func(v.number);
                }
                return returning;
            }
            break;



        case 21 : { /* SUM( ... ) */
                    double sum=0;

                    foreach(Leaf *l, leaf->fparms) {
                        sum += eval(df, l,x, it, m, p, c, s, d).number; // for vectors number is sum
                    }
                    return Result(sum);
                  }
                  break;

        case 22 : { /* MEAN( ... ) */
                    double sum=0;
                    int count=0;

                    foreach(Leaf *l, leaf->fparms) {
                        Result res = eval(df, l,x, it, m, p, c, s, d); // for vectors number is sum
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
                        Result res = eval(df, l,x, it, m, p, c, s, d);
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
                        Result res = eval(df, l,x, it, m, p, c, s, d);
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
                        Result res = eval(df, l,x, it, m, p, c, s, d);
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
                    double duration = toDuration ? eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number : 0;

                    // get the PD Estimate for this date - note we always work with the absolulte
                    // power estimates in formulas, since the user can just divide by config(weight)
                    // or Athlete_Weight (which takes into account values stored in ride files.
                    // Bike or Run models are used according to activity type
                    PDEstimate pde = m->context->athlete->getPDEstimateFor(m->dateTime.date(), model, false, m->isRun);

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
                        Result ex = eval(df, leaf->fparms[i],x, it, m, p, c, s, d);

                        if (ex.vector.count()) {

                            // tiz a vector
                            foreach(double x, ex.vector) {

                                // did it get selected?
                                Result which = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
                                if (which.number) {
                                    returning.vector << x;
                                    returning.number += x;
                                }
                            }

                        } else {

                            // does the parameter get selected ?
                            Result which = eval(df, leaf->fparms[0], ex.number, it, m, p, c, s); //XXX it should be local index
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
                    else returning = eval(df, leaf->fparms[2],x, it, m, p, c, s, d);

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
                        Result r = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

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
                    else returning = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

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

                    return Result (60*VDOTCalculator::eqvTime(eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number, 1000*eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number));
                }
                break;

        case 36 :
                {   // BESTTIME (distance[km])

                    if (leaf->fparms.count() != 1 || m->fileCache() == NULL) return Result(0);

                    return Result (m->fileCache()->bestTime(eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number));
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
                    else returning = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);

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
                    else returning = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

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

                    Result days = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
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
        case 63 : { return Result(sqrt(eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number)); } // SQRT(x)

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
        if (symbol == "i") {

            lhsdouble = it;
            lhsisNumber = true;

        } else if (symbol == "x") {

            lhsdouble = x;
            lhsisNumber = true;

        } else if (symbol == "isRide") {
            lhsdouble = m->isBike ? 1 : 0;
            lhsisNumber = true;

        } else if (symbol == "isRun") {
            lhsdouble = m->isRun ? 1 : 0;
            lhsisNumber = true;

        } else if (symbol == "isSwim") {
            lhsdouble = m->isSwim ? 1 : 0;
            lhsisNumber = true;

        } else if (symbol == "isXtrain") {
            lhsdouble = m->isXtrain ? 1 : 0;
            lhsisNumber = true;

        } else if (!symbol.compare("NA", Qt::CaseInsensitive)) {

            lhsdouble = RideFile::NA;
            lhsisNumber = true;

        } else if (!symbol.compare("RECINTSECS", Qt::CaseInsensitive)) {

            lhsdouble = 1; // if in doubt
            if (m->ride(false)) lhsdouble = m->ride(false)->recIntSecs();
            lhsisNumber = true;

        } else if (!symbol.compare("Device", Qt::CaseInsensitive)) {

            if (m->ride(false)) lhsstring = m->ride(false)->deviceType();

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
        Result lhs = eval(df, leaf->lvalue.l,x, it, m, p, c, s, d);

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
        if (leaf->op != ASSIGN) lhs = eval(df, leaf->lvalue.l,x, it, m, p, c, s, d);

        // if elvis we only evaluate rhs if we are null
        Result rhs;
        if (leaf->op != ELVIS || lhs.number == 0) {
            rhs = eval(df, leaf->rvalue.l,x, it, m, p, c, s, d);
        }

        // NOW PERFORM OPERATION
        switch (leaf->op) {

        case ASSIGN:
        {
            // LHS MUST be a symbol...
            if (leaf->lvalue.l->type == Leaf::Symbol || leaf->lvalue.l->type == Leaf::Index) {

                // get value to assign from rhs
                Result  value(rhs.isNumber ? rhs : Result(0));

                if (leaf->lvalue.l->type == Leaf::Symbol) {

                    // update the symbol value
                    QString symbol = *(leaf->lvalue.l->lvalue.n);
                    df->symbols.insert(symbol, value);

                } else {

                    // bit harder we need to get the symbol first
                    // to update its vector
                    QString symbol = *(leaf->lvalue.l->lvalue.l->lvalue.n);

                    // we may have multiple indexes to assign!
                    Result indexes = eval(df,leaf->lvalue.l->fparms[0],x, it, m, p, c, s, d);

                    // generic symbol
                    if (df->symbols.contains(symbol)) {
                        Result sym = df->symbols.value(symbol);

                        QVector<double> selected;
                        if (indexes.vector.count()) selected=indexes.vector;
                        else selected << indexes.number;

                        for(int i=0; i< selected.count(); i++) {

                            int index=static_cast<int>(selected[i]);

                            // resize if need to
                            if (sym.vector.count() <= index) {
                                sym.vector.resize(index+1);
                            }

                            // add value
                            sym.vector[index] = value.number;
                        }

                        // update
                        df->symbols.insert(symbol, sym);
                    }
                }
                return value;
            }
            // shouldn't get here!
            return Result(RideFile::NA);
        }
        break;

        break;

        // basic operations should all work with vectors or numbers
        case ADD:
        case SUBTRACT:
        case DIVIDE:
        case MULTIPLY:
        case POW:
        {
            Result returning(0);

            // only if numberic on both sides
            if (lhs.isNumber && rhs.isNumber) {


                // its a vector operation...
                if (lhs.vector.count() || rhs.vector.count()) {

                    int size = lhs.vector.count() > rhs.vector.count() ? lhs.vector.count() : rhs.vector.count();

                    // coerce both into a vector of matching size
                    lhs.vectorize(size);
                    rhs.vectorize(size);

                    for(int i=0; i<size; i++) {
                        double left = lhs.vector[i];
                        double right = rhs.vector[i];
                        double value = 0;

                        switch (leaf->op) {
                        case ADD: value = left + right; break;
                        case SUBTRACT: value = left - right; break;
                        case DIVIDE: value = right ? left / right : 0; break;
                        case MULTIPLY: value = left * right; break;
                        case POW: value = pow(left,right); break;
                        }
                        returning.vector << value;
                        returning.number += value;
                    }

                } else {
                    switch (leaf->op) {
                    case ADD: returning.number = lhs.number + rhs.number; break;
                    case SUBTRACT: returning.number = lhs.number - rhs.number; break;
                    case DIVIDE: returning.number = rhs.number ? lhs.number / rhs.number : 0; break;
                    case MULTIPLY: returning.number = lhs.number * rhs.number; break;
                    case POW: returning.number = pow(lhs.number, rhs.number); break;
                    }
                }
            }
            return returning;
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
                Result cond = eval(df, leaf->cond.l,x, it, m, p, c, s, d);
                if (cond.isNumber && cond.number) return eval(df, leaf->lvalue.l,x, it, m, p, c, s, d);
                else {

                    // conditional may not have an else clause!
                    if (leaf->rvalue.l) return eval(df, leaf->rvalue.l,x, it, m, p, c, s, d);
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
                while (count++ < maxwhile && eval(df, leaf->cond.l,x, it, m, p, c, s, d).number) {
                    returning = eval(df, leaf->lvalue.l,x, it, m, p, c, s, d);
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

    // INDEXING INTO VECTORS
    case Leaf::Index :
    {
        Result index = eval(df,leaf->fparms[0],x, it, m, p, c, s, d);
        Result value = eval(df,leaf->lvalue.l,x, it, m, p, c, s, d); // lhs might also be a symbol

        // are we returning the value or a vector of values?
        if (index.vector.count()) {

            Result returning(0);

            // a range
            for(int i=0; i<index.vector.count(); i++) {
                int it=index.vector[i];
                if (it < 0 || it >= value.vector.count()) continue; // ignore out of bounds
                returning.vector << value.vector[it];
                returning.number += value.vector[it];
            }

            return returning;

        } else {
            // a single value
            if (index.number < 0 || index.number >= value.vector.count()) return Result(0);
            return Result(value.vector[index.number]);
        }
    }

    // SELECTING FROM VECTORS
    case Leaf::Select :
    {
        Result returning(0);

        //int index = eval(df,leaf->fparms[0],x, it, m, p, c, s, d).number;
        Result value = eval(df,leaf->lvalue.l,x, it, m, p, c, s, d); // lhs might also be a symbol

        // need a vector, always
        if (!value.vector.count()) return returning;

        // loop and evaluate, non-zero we keep, zero we lose
        for(int i=0; i<value.vector.count(); i++) {
            x = value.vector.at(i);
            int boolresult = eval(df,leaf->fparms[0],x, i, m, p, c, s, d).number;

            // we want it
            if (boolresult != 0) returning.vector << x;
        }

        return returning;

    }
    break;

    //
    // COMPOUND EXPRESSION
    //
    case Leaf::Compound :
    {
        Result returning(0);

        // evaluate each statement
        foreach(Leaf *statement, *(leaf->lvalue.b)) returning = eval(df, statement,x, it, m, p, c, s, d);

        // compound statements evaluate to the value of the last statement
        return returning;
    }
    break;

    default: // we don't need to evaluate any lower - they are leaf nodes handled above
        break;
    }
    return Result(0); // false
}

DFModel::DFModel(RideItem *item, Leaf *formula, DataFilterRuntime *df) : PDModel(item->context), item(item), formula(formula), df(df)
{
    // extract info from the passed params
    formula->findSymbols(parameters);
    parameters.removeDuplicates();
    parameters.removeOne("x"); // not a parameter we declare to lmfit

}

double
DFModel::f(double t, const double *parms)
{
    // set parms in the runtime
    for (int i=0; i< parameters.count(); i++)  df->symbols.insert(parameters[i], Result(parms[i]));

    // calulcate
    return formula->eval(df, formula, t, 0, item, NULL, NULL, Specification(), DateRange()).number;
}

double
DFModel::y(double t) const
{
    // calculate using current runtime
    return formula->eval(df, formula, t, 0, item, NULL, NULL, Specification(), DateRange()).number;
}

bool
DFModel::fitData(QVector<double>&x, QVector<double>&y)
{
    // lets call the fitting routine
    if (parameters.count() < 1) return false;

    // unpack starting parameters
    QVector<double> startingparms;
    foreach(QString symbol, parameters)  startingparms << df->symbols.value(symbol).number;

    // get access to lmfit, single threaded :(
    lm_control_struct control = lm_control_double;
    lm_status_struct status;

    // use forwarder via global variable, so mutex around this !
    calllmfit.lock();
    calllmfitmodel = this;

    //fprintf(stderr, "Fitting ...\n" ); fflush(stderr);
    lmcurve(parameters.count(), const_cast<double*>(startingparms.constData()), x.count(), x.constData(), y.constData(), calllmfitf, &control, &status);

    // release for others
    calllmfit.unlock();

    // starting parms now contain final output lets
    // update the runtime to get them back to the user
    int i=0;
    foreach(QString symbol, parameters) {
        Result value(startingparms[i]);
        df->symbols.insert(symbol, value);
        i++;
    }

    //fprintf(stderr, "Results:\n" );
    //fprintf(stderr, "status after %d function evaluations:\n  %s\n",
    //        status.nfev, lm_infmsg[status.outcome] ); fflush(stderr);
    //fprintf(stderr,"obtained parameters:\n"); fflush(stderr);
    //for (int i = 0; i < this->nparms(); ++i)
    //    fprintf(stderr,"  par[%i] = %12g\n", i, startingparms[i]);
    //fprintf(stderr,"obtained norm:\n  %12g\n", status.fnorm ); fflush(stderr);

    if (status.outcome < 4) return true;
    return false;
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
