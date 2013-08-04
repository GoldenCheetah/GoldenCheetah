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
#include "Context.h"
#include "Athlete.h"
#include "RideNavigator.h"
#include "RideFileCache.h"
#include <QDebug>

#include "DataFilter_yacc.h"

// LEXER VARIABLES WE INTERACT WITH
// Standard yacc/lex variables / functions
extern int DataFilterlex(); // the lexer aka yylex()
extern char *DataFiltertext; // set by the lexer aka yytext

extern void DataFilter_setString(QString);
extern void DataFilter_clearString();

// PARSER STATE VARIABLES
QStringList DataFiltererrors;
extern int DataFilterparse();

Leaf *root; // root node for parsed statement

static RideFile::SeriesType nameToSeries(QString name)
{
    if (!name.compare("power", Qt::CaseInsensitive)) return RideFile::watts;
    if (!name.compare("cadence", Qt::CaseInsensitive)) return RideFile::cad;
    if (!name.compare("hr", Qt::CaseInsensitive)) return RideFile::hr;
    if (!name.compare("speed", Qt::CaseInsensitive)) return RideFile::kph;
    if (!name.compare("torque", Qt::CaseInsensitive)) return RideFile::nm;
    if (!name.compare("NP", Qt::CaseInsensitive)) return RideFile::NP;
    if (!name.compare("xPower", Qt::CaseInsensitive)) return RideFile::xPower;
    if (!name.compare("VAM", Qt::CaseInsensitive)) return RideFile::vam;
    if (!name.compare("wpk", Qt::CaseInsensitive)) return RideFile::wattsKg;

    return RideFile::none;

}

void Leaf::print(Leaf *leaf, int level)
{
    qDebug()<<"LEVEL"<<level;
    switch(leaf->type) {
    case Leaf::Float : qDebug()<<"float"<<leaf->lvalue.f; break;
    case Leaf::Integer : qDebug()<<"integer"<<leaf->lvalue.i; break;
    case Leaf::String : qDebug()<<"string"<<*leaf->lvalue.s; break;
    case Leaf::Symbol : qDebug()<<"symbol"<<*leaf->lvalue.n; break;
    case Leaf::Logical  : qDebug()<<"lop"<<leaf->op;
                    leaf->print(leaf->lvalue.l, level+1);
                    leaf->print(leaf->rvalue.l, level+1);
                    break;
    case Leaf::Operation : qDebug()<<"cop"<<leaf->op;
                    leaf->print(leaf->lvalue.l, level+1);
                    leaf->print(leaf->rvalue.l, level+1);
                    break;
    case Leaf::BinaryOperation : qDebug()<<"bop"<<leaf->op;
                    leaf->print(leaf->lvalue.l, level+1);
                    leaf->print(leaf->rvalue.l, level+1);
                    break;
    case Leaf::Function : qDebug()<<"function"<<leaf->function<<"series="<<*(leaf->series->lvalue.n);
                    leaf->print(leaf->lvalue.l, level+1);
                    break;
    default:
        break;

    }
}

bool Leaf::isNumber(DataFilter *df, Leaf *leaf)
{
    switch(leaf->type) {
    case Leaf::Float : return true;
    case Leaf::Integer : return true;
    case Leaf::String : return false;
    case Leaf::Symbol : return df->lookupType.value(*(leaf->lvalue.n), false);
    case Leaf::Logical  : return true; // not possible!
    case Leaf::Operation : return true;
    case Leaf::BinaryOperation : return true;
    case Leaf::Function : return true;
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

void Leaf::validateFilter(DataFilter *df, Leaf *leaf)
{
    switch(leaf->type) {
    case Leaf::Symbol :
        {
            // are the symbols correct?
            // if so set the type to meta or metric
            // and save the technical name used to do
            // a lookup at execution time
            QString lookup = df->lookupMap.value(*(leaf->lvalue.n), "");
            if (lookup == "") {
                DataFiltererrors << QString("%1 is unknown").arg(*(leaf->lvalue.n));
            }
        }
        break;

    case Leaf::Function :
        {
            // is the symbol valid?
            QRegExp bestValidSymbols("^(power|hr|cadence|speed|torque|vam|xpower|np|wpk)$", Qt::CaseInsensitive);
            QRegExp tizValidSymbols("^(power|hr)$", Qt::CaseInsensitive);
            QString symbol = *(leaf->series->lvalue.n); 

            if (leaf->function == "best" && !bestValidSymbols.exactMatch(symbol)) 
                DataFiltererrors << QString("invalid data series for best(): %1").arg(symbol);

            if (leaf->function == "tiz" && !tizValidSymbols.exactMatch(symbol)) 
                DataFiltererrors << QString("invalid data series for tiz(): %1").arg(symbol);

            // now set the series type
            leaf->seriesType = nameToSeries(symbol);
        }
        break;

    case Leaf::BinaryOperation  :
    case Leaf::Operation  :
        {
            // first lets make sure the lhs and rhs are of the same type
            bool lhsType = Leaf::isNumber(df, leaf->lvalue.l);
            bool rhsType = Leaf::isNumber(df, leaf->rvalue.l);
            if (lhsType != rhsType) {
                DataFiltererrors << QString("comparing strings with numbers");
            }

            // what about using string operations on a lhs/rhs that
            // are numeric?
            if ((lhsType || rhsType) && leaf->op >= MATCHES && leaf->op <= CONTAINS) {
                DataFiltererrors << "using a string operations with a number";
            }

            validateFilter(df, leaf->lvalue.l);
            validateFilter(df, leaf->rvalue.l);
        }
        break;

    case Leaf::Logical : 
        {
            validateFilter(df, leaf->lvalue.l);
            if (leaf->op) validateFilter(df, leaf->rvalue.l);
        }
        break;
    default:
        break;
    }
}

DataFilter::DataFilter(QObject *parent, Context *context) : QObject(parent), context(context), treeRoot(NULL)
{
    configUpdate();
    connect(context, SIGNAL(configChanged()), this, SLOT(configUpdate()));
}

QStringList DataFilter::parseFilter(QString query)
{
    //DataFilterdebug = 2; // no debug -- needs bison -t in src.pro
    root = NULL;

    // if something was left behind clear it up now
    clearFilter();

    // Parse from string
    DataFiltererrors.clear(); // clear out old errors
    DataFilter_setString(query);
    DataFilterparse();
    DataFilter_clearString();

    // save away the results
    treeRoot = root;

    // if it passed syntax lets check semantics
    if (treeRoot && DataFiltererrors.count() == 0) treeRoot->validateFilter(this, treeRoot);

    // ok, did it pass all tests?
    if (!treeRoot || DataFiltererrors.count() > 0) { // nope

        // no errors just failed to finish
        if (!treeRoot) DataFiltererrors << "malformed expression.";

        // Bzzzt, malformed
        emit parseBad(DataFiltererrors);
        clearFilter();

    } else { // yep! .. we have a winner!

        // successfuly parsed, lets check semantics
        //treeRoot->print(treeRoot);
        emit parseGood();

        // get all fields...
        QList<SummaryMetrics> allRides = context->athlete->metricDB->getAllMetricsFor(QDateTime(), QDateTime());

        filenames.clear();

        for (int i=0; i<allRides.count(); i++) {

            // evaluate each ride...
            QString f= allRides.at(i).getFileName();
            double result = treeRoot->eval(this, treeRoot, allRides.at(i), f);
            if (result) {
                filenames << f;
            }
        }
        emit results(filenames);
    }

    errors = DataFiltererrors;
    return errors;
}

void DataFilter::clearFilter()
{
    if (treeRoot) {
        treeRoot->clear(treeRoot);
        treeRoot = NULL;
    }
}

void DataFilter::configUpdate()
{
    lookupMap.clear();
    lookupType.clear();

    // create lookup map from 'friendly name' to name used in smmaryMetrics
    // to enable a quick lookup && the lookup for the field type (number, text)
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++) {
        QString symbol = factory.metricName(i);
        QString name = factory.rideMetric(symbol)->name();
        lookupMap.insert(name.replace(" ","_"), symbol);
        lookupType.insert(name.replace(" ","_"), true);
    }

    // now add the ride metadata fields -- should be the same generally
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
            QString underscored = field.name;
            if (!context->specialFields.isMetric(underscored)) {
                lookupMap.insert(underscored.replace(" ","_"), field.name);
                lookupType.insert(underscored.replace(" ","_"), (field.type > 2)); // true if is number
            }
    }
}

double Leaf::eval(DataFilter *df, Leaf *leaf, SummaryMetrics m, QString f)
{
    switch(leaf->type) {

    case Leaf::Logical  :
    {
        switch (leaf->op) {
            case AND :
                return (eval(df, leaf->lvalue.l, m, f) && eval(df, leaf->rvalue.l, m, f));

            case OR :
                return (eval(df, leaf->lvalue.l, m, f) || eval(df, leaf->rvalue.l, m, f));

            default : // parenthesis
                return (eval(df, leaf->lvalue.l, m, f));
        }
    }
    break;

    case Leaf::Function :
    {
        double duration;

        // GET LHS VALUE
        switch (leaf->lvalue.l->type) {

            default:
            case Leaf::Function :
            {
                duration = eval(df, leaf->lvalue.l, m, f); // duration
            }
            break;

            case Leaf::Symbol :
            {
                QString rename;
                // get symbol value
                if (df->lookupType.value(*(leaf->lvalue.l->lvalue.n)) == true) {
                    // numeric
                    duration = m.getForSymbol(rename=df->lookupMap.value(*(leaf->lvalue.l->lvalue.n),""));
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
            return RideFileCache::best(df->context, f, leaf->seriesType, duration);

        if (leaf->function == "tiz") // duration is really zone number
            return RideFileCache::tiz(df->context, f, leaf->seriesType, duration); 

        // unknown function!?
        return 0 ;
    }
    break;

    case Leaf::BinaryOperation :
    case Leaf::Operation :
    {
        double lhsdouble=0.00, rhsdouble=0.00;
        QString lhsstring, rhsstring;
        bool lhsisNumber=false, rhsisNumber=false;

        // GET LHS VALUE
        switch (leaf->lvalue.l->type) {

            default:
            case Leaf::Function :
            {
                lhsdouble = eval(df, leaf->lvalue.l, m, f); // duration
                lhsisNumber=true;
            }
            break;

            case Leaf::Symbol :
            {
                QString rename;
                // get symbol value
                if ((lhsisNumber = df->lookupType.value(*(leaf->lvalue.l->lvalue.n))) == true) {
                    // numeric
                    lhsdouble = m.getForSymbol(rename=df->lookupMap.value(*(leaf->lvalue.l->lvalue.n),""));
                    //qDebug()<<"symbol" << *(leaf->lvalue.l->lvalue.n) << "is" << lhsdouble << "via" << rename;
                } else {
                    // string
                    lhsstring = m.getText(rename=df->lookupMap.value(*(leaf->lvalue.l->lvalue.n),""), "");
                    //qDebug()<<"symbol" << *(leaf->lvalue.l->lvalue.n) << "is" << lhsstring << "via" << rename;
                }
            }
            break;

            case Leaf::Float :
                lhsisNumber = true;
                lhsdouble = leaf->lvalue.l->lvalue.f;
                break;

            case Leaf::Integer :
                lhsisNumber = true;
                lhsdouble = leaf->lvalue.l->lvalue.i;
                break;

            case Leaf::String :
                lhsisNumber = false;
                lhsstring = *(leaf->lvalue.l->lvalue.s);
                break;

            break;
        }

        // GET RHS VALUE -- BUT NOT FOR FUNCTIONS
        switch (leaf->rvalue.l->type) {

            default:
            case Leaf::Function :
            {
                rhsdouble = eval(df, leaf->rvalue.l, m, f);
                rhsisNumber=true;
            }
            break;
            case Leaf::Symbol :
            {
                QString rename;
                // get symbol value
                if ((rhsisNumber=df->lookupType.value(*(leaf->rvalue.l->lvalue.n))) == true) {
                    // numeric
                    rhsdouble = m.getForSymbol(rename=df->lookupMap.value(*(leaf->rvalue.l->lvalue.n),""));
                    //qDebug()<<"symbol" << *(leaf->rvalue.l->lvalue.n) << "is" << rhsdouble << "via" << rename;
                } else {
                    // string
                    rhsstring = m.getText(rename=df->lookupMap.value(*(leaf->rvalue.l->lvalue.n),""), "notfound");
                    //qDebug()<<"symbol" << *(leaf->rvalue.l->lvalue.n) << "is" << rhsstring << "via" << rename;
                }
            }
            break;

            case Leaf::Float :
                rhsisNumber = true;
                rhsdouble = leaf->rvalue.l->lvalue.f;
                break;

            case Leaf::Integer :
                rhsisNumber = true;
                rhsdouble = leaf->rvalue.l->lvalue.i;
                break;

            case Leaf::String :
                rhsisNumber = false;
                rhsstring = *(leaf->rvalue.l->lvalue.s);
                break;

            break;
        }

        // NOW PERFORM OPERATION
        //qDebug()<<"lhs="<<lhsdouble<<"rhs="<<rhsdouble;
        switch (leaf->op) {

        case ADD:
        {
            if (lhsisNumber) {
                return lhsdouble + rhsdouble;
            } else {
                return 0;
            }
        }
        break;

        case SUBTRACT:
        {
            if (lhsisNumber) {
                return lhsdouble - rhsdouble;
            } else {
                return 0;
            }
        }
        break;

        case DIVIDE:
        {
            if (lhsisNumber && rhsdouble) { // avoid divide by zero
                return lhsdouble / rhsdouble;
            } else {
                return 0;
            }
        }
        break;

        case MULTIPLY:
        {
            if (lhsisNumber) {
                return lhsdouble * rhsdouble;
            } else {
                return 0;
            }
        }
        break;

        case POW:
        {
            if (lhsisNumber && rhsdouble) {
                return pow(lhsdouble,rhsdouble);
            } else {
                return 0;
            }
        }
        break;

        case EQ:
        {
            if (lhsisNumber) {
                return lhsdouble == rhsdouble;
            } else {
                return lhsstring == rhsstring;
            }
        }
        break;

        case NEQ:
        {
            if (lhsisNumber) {
                return lhsdouble != rhsdouble;
            } else {
                return lhsstring != rhsstring;
            }
        }
        break;

        case LT:
        {
            if (lhsisNumber) {
                return lhsdouble < rhsdouble;
            } else {
                return lhsstring < rhsstring;
            }
        }
        break;
        case LTE:
        {
            if (lhsisNumber) {
                return lhsdouble <= rhsdouble;
            } else {
                return lhsstring <= rhsstring;
            }
        }
        break;
        case GT:
        {
            if (lhsisNumber) {
                return lhsdouble > rhsdouble;
            } else {
                return lhsstring > rhsstring;
            }
        }
        break;
        case GTE:
        {
            if (lhsisNumber) {
                return lhsdouble >= rhsdouble;
            } else {
                return lhsstring >= rhsstring;
            }
        }
        break;

        case MATCHES:
            return QRegExp(rhsstring).exactMatch(lhsstring);
            break;

        case ENDSWITH:
            return lhsstring.endsWith(rhsstring);
            break;

        case BEGINSWITH:
            return lhsstring.startsWith(rhsstring);
            break;

        case CONTAINS:
            return lhsstring.contains(rhsstring) ? true : false;
            break;

        default:
            break;
        }
    }
    break;

    default: // we don't need to evaluate any lower - they are leaf nodes handled above
        break;
    }
    return false;
}
