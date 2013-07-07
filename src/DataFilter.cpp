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
#include "MainWindow.h"
#include "RideNavigator.h"
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

void Leaf::print(Leaf *leaf)
{
    switch(leaf->type) {
    case Leaf::Float : qDebug()<<"float"<<leaf->lvalue.f; break;
    case Leaf::Integer : qDebug()<<"integer"<<leaf->lvalue.i; break;
    case Leaf::String : qDebug()<<"string"<<*leaf->lvalue.s; break;
    case Leaf::Symbol : qDebug()<<"symbol"<<*leaf->lvalue.n; break;
    case Leaf::Logical  : qDebug()<<"logical"<<leaf->op;
                    leaf->print(leaf->lvalue.l);
                    leaf->print(leaf->rvalue.l);
                    break;
    case Leaf::Operation : qDebug()<<"compare"<<leaf->op;
                    leaf->print(leaf->lvalue.l);
                    leaf->print(leaf->rvalue.l);
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
    case Leaf::Logical  : return false; // not possible!
    case Leaf::Operation : return false;
    default:
        return false;
        break;

    }
}

void Leaf::clear(Leaf *leaf)
{
    switch(leaf->type) {
    case Leaf::String : delete leaf->lvalue.s; break;
    case Leaf::Symbol : delete leaf->lvalue.n; break;
    case Leaf::Logical  : 
    case Leaf::Operation : clear(leaf->lvalue.l);
                           clear(leaf->rvalue.l);
                           delete(leaf->lvalue.l);
                           delete(leaf->rvalue.l);
                           break;
    default:
        break;
    }
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

    case Leaf::Logical : validateFilter(df, leaf->lvalue.l);
                         validateFilter(df, leaf->rvalue.l);
        break;
    default:
        break;
    }
}

DataFilter::DataFilter(QObject *parent, MainWindow *mainWindow) : QObject(parent), mainWindow(mainWindow), treeRoot(NULL)
{
    configUpdate();
    connect(mainWindow->context, SIGNAL(configChanged()), this, SLOT(configUpdate()));
}

QStringList DataFilter::parseFilter(QString query)
{
    //DataFilterdebug = 0; // no debug -- needs bison -t in src.pro
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
    if (DataFiltererrors.count() > 0) { // nope

        // Bzzzt, malformed
        emit parseBad(DataFiltererrors);
        clearFilter();

    } else { // yep! .. we have a winner!

        // successfuly parsed, lets check semantics
        //treeRoot->print(treeRoot);
        emit parseGood();

        // get all fields...
        QList<SummaryMetrics> allRides = mainWindow->athlete->metricDB->getAllMetricsFor(QDateTime(), QDateTime());

        filenames.clear();

        for (int i=0; i<allRides.count(); i++) {

            // evaluate each ride...
            bool result = treeRoot->eval(this, treeRoot, allRides.at(i));
            if (result) {
                filenames << allRides.at(i).getFileName();
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
    foreach(FieldDefinition field, mainWindow->athlete->rideMetadata()->getFields()) {
            QString underscored = field.name;
            if (!mainWindow->specialFields.isMetric(underscored)) { 
                lookupMap.insert(underscored.replace(" ","_"), field.name);
                lookupType.insert(underscored.replace(" ","_"), (field.type > 2)); // true if is number
            }
    }
}

bool Leaf::eval(DataFilter *df, Leaf *leaf, SummaryMetrics m)
{
    switch(leaf->type) {

    case Leaf::Logical  : 
    {
        switch (leaf->op) {
            case AND :
                return (eval(df, leaf->lvalue.l, m) && eval(df, leaf->rvalue.l, m));

            case OR :
                return (eval(df, leaf->lvalue.l, m) || eval(df, leaf->rvalue.l, m));
        }
    }
    break;

    case Leaf::Operation : 
    {
        double lhsdouble=0.00, rhsdouble=0.00;
        QString lhsstring, rhsstring;
        bool lhsisNumber=false, rhsisNumber=false;

        // GET LHS VALUE
        switch (leaf->lvalue.l->type) {

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

            default:
            break;
        }

        // GET RHS VALUE
        switch (leaf->rvalue.l->type) {

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

            default:
            break;
        }

        // NOW PERFORM OPERATION
        switch (leaf->op) {

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
            return lhsstring.contains(rhsstring);
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
