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
                DataFiltererrors << QString("Unknown: %1").arg(*(leaf->lvalue.n));
            }
        }
    break;

    case Leaf::Operation  : 
        {
            // first lets make sure the lhs and rhs are of the same type
            bool lhsType = Leaf::isNumber(df, leaf->lvalue.l);
            bool rhsType = Leaf::isNumber(df, leaf->rvalue.l);
            if (lhsType != rhsType) {
                DataFiltererrors << QString("Type mismatch");
            }

            // what about using string operations on a lhs/rhs that
            // are numeric?
            if ((lhsType || rhsType) && leaf->op >= MATCHES && leaf->op <= CONTAINS) {
                DataFiltererrors << "Mixing string operations with numbers";
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

DataFilter::DataFilter(QObject *parent, MainWindow *main) : QObject(parent), main(main), treeRoot(NULL)
{
    configUpdate();
    connect(main, SIGNAL(configChanged()), this, SLOT(configUpdate()));
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
        qDebug()<<"parse filter errors:"<<DataFiltererrors;
        clearFilter();

    } else { // yep! .. we have a winner!

        // successfuly parsed, lets check semantics
        //treeRoot->print(treeRoot);
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
    foreach(FieldDefinition field, main->rideMetadata()->getFields()) {
            QString underscored = field.name;
            lookupMap.insert(field.name.replace(" ","_"), field.name);
            lookupType.insert(field.name.replace(" ","_"), (field.type > 2)); // true if is number
    }

#if 0
    QMapIterator<QString, QString>r(lookupMap);
    while(r.hasNext()) {

        r.next();
        qDebug()<<"Lookup"<<r.key()<<"to get"<<r.value();
    }
#endif
}

bool Leaf::eval(Leaf *leaf, SummaryMetrics m)
{
    switch(leaf->type) {

    case Leaf::Logical  : 
        switch (leaf->op) {
            case AND :
                return (eval(leaf->lvalue.l, m) && eval(leaf->rvalue.l, m));

            case OR :
                return (eval(leaf->lvalue.l, m) || eval(leaf->rvalue.l, m));
        }

    case Leaf::Operation : 
    {
        switch (leaf->op) {

        case EQ:
        case NEQ:
        case LT:
        case LTE:
        case GT:
        case GTE:
        case MATCHES:
        case ENDSWITH:
        case BEGINSWITH:
        case CONTAINS:
        default:
            break;
        }
    }
        break;
    default:
        break;
    }
    return false;
}
