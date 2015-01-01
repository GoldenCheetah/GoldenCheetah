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
#include "RideNavigator.h"
#include "RideFileCache.h"
#include "PMCData.h"
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
    if (!name.compare("apower", Qt::CaseInsensitive)) return RideFile::aPower;
    if (!name.compare("cadence", Qt::CaseInsensitive)) return RideFile::cad;
    if (!name.compare("hr", Qt::CaseInsensitive)) return RideFile::hr;
    if (!name.compare("speed", Qt::CaseInsensitive)) return RideFile::kph;
    if (!name.compare("torque", Qt::CaseInsensitive)) return RideFile::nm;
    if (!name.compare("NP", Qt::CaseInsensitive)) return RideFile::NP;
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
                    break;

    case Leaf::Logical  : 
                    if (leaf->op == 0) return leaf->isDynamic(leaf->lvalue.l);
    case Leaf::Operation :
    case Leaf::BinaryOperation : 
                    return leaf->isDynamic(leaf->lvalue.l) || leaf->isDynamic(leaf->rvalue.l);
                    break;
    case Leaf::Function : 
                    if (leaf->lvalue.l) return leaf->isDynamic(leaf->lvalue.l);
                    else return leaf->dynamic;
                    break;
        break;

    }
    return false;
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
                    if (leaf->op) // nonzero ?
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
                    if (leaf->lvalue.l) leaf->print(leaf->lvalue.l, level+1);
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

bool Leaf::isNumber(DataFilter *df, Leaf *leaf)
{
    switch(leaf->type) {
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
            if (symbol == "isRun") return true;
            else if (!symbol.compare("Date", Qt::CaseInsensitive)) return true;
            else if (!symbol.compare("Today", Qt::CaseInsensitive)) return true;
            else if (!symbol.compare("Current", Qt::CaseInsensitive)) return true;
            else if (isCoggan(symbol)) return true;
            else return df->lookupType.value(symbol, false);
        }
        break;
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
            QString symbol = *(leaf->lvalue.n);
            QString lookup = df->lookupMap.value(symbol, "");
            if (lookup == "") {

                // isRun isa special, we may add more later (e.g. date)
                if (symbol.compare("Date", Qt::CaseInsensitive) && 
                    symbol.compare("Today", Qt::CaseInsensitive) && 
                    symbol.compare("Current", Qt::CaseInsensitive) && 
                    symbol != "isRun" && !isCoggan(symbol))
                    DataFiltererrors << QString(QObject::tr("%1 is unknown")).arg(symbol);

                if (symbol.compare("Current", Qt::CaseInsensitive))
                    dynamic = true;
            }
        }
        break;

    case Leaf::Function :
        {
            // is the symbol valid?
            QRegExp bestValidSymbols("^(apower|power|hr|cadence|speed|torque|vam|xpower|np|wpk)$", Qt::CaseInsensitive);
            QRegExp tizValidSymbols("^(power|hr)$", Qt::CaseInsensitive);
            QString symbol = *(leaf->series->lvalue.n); 

            if (leaf->function == "sts" || leaf->function == "lts" || leaf->function == "sb") {

                // does the symbol exist though ?
                QString lookup = df->lookupMap.value(symbol, "");
                if (lookup == "") DataFiltererrors << QString(QObject::tr("%1 is unknown")).arg(symbol);

            } else {

                if (leaf->function == "best" && !bestValidSymbols.exactMatch(symbol)) 
                    DataFiltererrors << QString(QObject::tr("invalid data series for best(): %1")).arg(symbol);

                if (leaf->function == "tiz" && !tizValidSymbols.exactMatch(symbol)) 
                    DataFiltererrors << QString(QObject::tr("invalid data series for tiz(): %1")).arg(symbol);

                // now set the series type
                leaf->seriesType = nameToSeries(symbol);
            }

        }
        break;

    case Leaf::BinaryOperation  :
    case Leaf::Operation  :
        {
            // first lets make sure the lhs and rhs are of the same type
            bool lhsType = Leaf::isNumber(df, leaf->lvalue.l);
            bool rhsType = Leaf::isNumber(df, leaf->rvalue.l);
            if (lhsType != rhsType) {
                DataFiltererrors << QString(QObject::tr("comparing strings with numbers"));
            }

            // what about using string operations on a lhs/rhs that
            // are numeric?
            if ((lhsType || rhsType) && leaf->op >= MATCHES && leaf->op <= CONTAINS) {
                DataFiltererrors << QObject::tr("using a string operations with a number");
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

DataFilter::DataFilter(QObject *parent, Context *context) : QObject(parent), context(context), isdynamic(false), treeRoot(NULL)
{
    configChanged(CONFIG_FIELDS);
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(rideSelected(RideItem*)), this, SLOT(dynamicParse()));
}

QStringList DataFilter::parseFilter(QString query, QStringList *list)
{
    // remember where we apply
    this->list = list;
    isdynamic=false;

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
        if (!treeRoot) DataFiltererrors << tr("malformed expression.");

        // Bzzzt, malformed
        emit parseBad(DataFiltererrors);
        clearFilter();

    } else { // yep! .. we have a winner!

        isdynamic = treeRoot->isDynamic(treeRoot);

        // successfuly parsed, lets check semantics
        //treeRoot->print(treeRoot);
        emit parseGood();

        // clear current filter list
        filenames.clear();

        // get all fields...
        foreach(RideItem *item, context->athlete->rideCache->rides()) {

            // evaluate each ride...
            if(treeRoot->eval(context, this, treeRoot, item))
                filenames << item->fileName;
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
    if (isdynamic) {
        // need to reapply on current state

        // clear current filter list
        filenames.clear();

        // get all fields...
        foreach(RideItem *item, context->athlete->rideCache->rides()) {

            // evaluate each ride...
            if(treeRoot->eval(context, this, treeRoot, item))
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
    isdynamic = false;
}

void DataFilter::configChanged(qint32)
{
    lookupMap.clear();
    lookupType.clear();

    // create lookup map from 'friendly name' to INTERNAL-name used in summaryMetrics
    // to enable a quick lookup && the lookup for the field type (number, text)
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++) {
        QString symbol = factory.metricName(i);
        QString name = context->specialFields.internalName(factory.rideMetric(symbol)->name());

        lookupMap.insert(name.replace(" ","_"), symbol);
        lookupType.insert(name.replace(" ","_"), true);
    }

    // now add the ride metadata fields -- should be the same generally
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
            QString underscored = field.name;
            if (!context->specialFields.isMetric(underscored)) {

                // translate to internal name if name has non Latin1 characters
                underscored = context->specialFields.internalName(underscored);
                field.name = context->specialFields.internalName((field.name));

                lookupMap.insert(underscored.replace(" ","_"), field.name);
                lookupType.insert(underscored.replace(" ","_"), (field.type > 2)); // true if is number
            }
    }

}

double Leaf::eval(Context *context, DataFilter *df, Leaf *leaf, RideItem *m)
{
    switch(leaf->type) {

    case Leaf::Logical  :
    {
        switch (leaf->op) {
            case AND :
                return (eval(context, df, leaf->lvalue.l, m) && eval(context, df, leaf->rvalue.l, m));

            case OR :
                return (eval(context, df, leaf->lvalue.l, m) || eval(context, df, leaf->rvalue.l, m));

            default : // parenthesis
                return (eval(context, df, leaf->lvalue.l, m));
        }
    }
    break;

    case Leaf::Function :
    {
        double duration;

        // pmc data ...
        if (leaf->function == "sts" || leaf->function == "lts" || leaf->function == "sb") {

                // get metric technical name
                QString symbol = *(leaf->series->lvalue.n); 
                QString lookup = df->lookupMap.value(symbol, "");
                PMCData *pmcData = context->athlete->getPMCFor(lookup);

                if (leaf->function == "sts") return pmcData->sts(m->dateTime.date());
                if (leaf->function == "lts") return pmcData->lts(m->dateTime.date());
                if (leaf->function == "sb") return pmcData->sb(m->dateTime.date());
        }


        // GET LHS VALUE
        switch (leaf->lvalue.l->type) {

            default:
            case Leaf::Function :
            {
                duration = eval(context, df, leaf->lvalue.l, m); // duration
            }
            break;

            case Leaf::Symbol :
            {
                QString rename;
                // get symbol value
                if (df->lookupType.value(*(leaf->lvalue.l->lvalue.n)) == true) {
                    // numeric
                    duration = m->getForSymbol(rename=df->lookupMap.value(*(leaf->lvalue.l->lvalue.n),""));
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
            return RideFileCache::best(df->context, m->fileName, leaf->seriesType, duration);

        if (leaf->function == "tiz") // duration is really zone number
            return RideFileCache::tiz(df->context, m->fileName, leaf->seriesType, duration); 

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
                lhsdouble = eval(context, df, leaf->lvalue.l, m); // duration
                lhsisNumber=true;
            }
            break;

            case Leaf::Symbol :
            {
                QString rename;
                QString symbol = *(leaf->lvalue.l->lvalue.n);

                // is it isRun ?
                if (symbol == "isRun") {
                    lhsdouble = m->isRun ? 1 : 0;
                    lhsisNumber = true;

                } else if (!symbol.compare("Current", Qt::CaseInsensitive)) {

                    if (context->currentRideItem())
                        lhsdouble = QDate(1900,01,01).
                        daysTo(context->currentRideItem()->dateTime.date());
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
                    PMCData *pmcData = context->athlete->getPMCFor("coggan_tss");
                    if (!symbol.compare("ctl", Qt::CaseInsensitive)) lhsdouble = pmcData->lts(m->dateTime.date());
                    if (!symbol.compare("atl", Qt::CaseInsensitive)) lhsdouble = pmcData->sts(m->dateTime.date());
                    if (!symbol.compare("tsb", Qt::CaseInsensitive)) lhsdouble = pmcData->sb(m->dateTime.date());
                    lhsisNumber = true;

                } else if ((lhsisNumber = df->lookupType.value(*(leaf->lvalue.l->lvalue.n))) == true) {
                    // get symbol value
                    // check metadata string to number first ...
                    QString meta = m->getText(rename=df->lookupMap.value(*(leaf->lvalue.l->lvalue.n),""), "unknown");
                    if (meta == "unknown")
                        lhsdouble = m->getForSymbol(rename=df->lookupMap.value(*(leaf->lvalue.l->lvalue.n),""));
                    else
                        lhsdouble = meta.toDouble();

                    //qDebug()<<"symbol" << *(leaf->lvalue.l->lvalue.n) << "is" << lhsdouble << "via" << rename;
                } else {
                    // string
                    lhsstring = m->getText(rename=df->lookupMap.value(*(leaf->lvalue.l->lvalue.n),""), "");
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
                QString string = *(leaf->lvalue.l->lvalue.s);
                QDate date = QDate::fromString(string, "yyyy/MM/dd");
                if (date.isValid()) {
                    lhsdouble = QDate(1900,01,01).daysTo(date);
                    lhsisNumber = true;
                } else {
                    lhsstring = string;
                    lhsisNumber = false;
                }
                break;

            break;
        }

        // GET RHS VALUE -- BUT NOT FOR FUNCTIONS
        switch (leaf->rvalue.l->type) {

            default:
            case Leaf::Function :
            {
                rhsdouble = eval(context, df, leaf->rvalue.l, m);
                rhsisNumber=true;
            }
            break;
            case Leaf::Symbol :
            {
                QString rename;
                QString symbol = *(leaf->rvalue.l->lvalue.n);

                // is it isRun ?
                if (symbol == "isRun") {

                    rhsdouble = m->isRun ? 1 : 0;
                    rhsisNumber = true;

                } else if (!symbol.compare("Current", Qt::CaseInsensitive)) {

                    if (context->currentRideItem())
                        rhsdouble = QDate(1900,01,01).
                        daysTo(context->currentRideItem()->dateTime.date());
                    else
                        rhsdouble = 0;
                    rhsisNumber = true;

                } else if (!symbol.compare("Today", Qt::CaseInsensitive)) {

                    rhsdouble = QDate(1900,01,01).daysTo(QDate::currentDate());
                    rhsisNumber = true;

                } else if (!symbol.compare("Date", Qt::CaseInsensitive)) {

                    rhsdouble = QDate(1900,01,01).daysTo(m->dateTime.date());
                    rhsisNumber = true;

                } else if (isCoggan(symbol)) {
 
                    // a coggan PMC metric
                    PMCData *pmcData = context->athlete->getPMCFor("coggan_tss");
                    if (!symbol.compare("ctl", Qt::CaseInsensitive)) rhsdouble = pmcData->lts(m->dateTime.date());
                    if (!symbol.compare("atl", Qt::CaseInsensitive)) rhsdouble = pmcData->sts(m->dateTime.date());
                    if (!symbol.compare("tsb", Qt::CaseInsensitive)) rhsdouble = pmcData->sb(m->dateTime.date());
                    rhsisNumber = true;

                // get symbol value
                } else if ((rhsisNumber=df->lookupType.value(*(leaf->rvalue.l->lvalue.n))) == true) {
                    // numeric
                    QString meta = m->getText(rename=df->lookupMap.value(*(leaf->rvalue.l->lvalue.n),""), "unknown");
                    if (meta == "unknown")
                        rhsdouble = m->getForSymbol(rename=df->lookupMap.value(*(leaf->rvalue.l->lvalue.n),""));
                    else
                        rhsdouble = meta.toDouble();
                    //qDebug()<<"symbol" << *(leaf->rvalue.l->lvalue.n) << "is" << rhsdouble << "via" << rename;
                } else {
                    // string
                    rhsstring = m->getText(rename=df->lookupMap.value(*(leaf->rvalue.l->lvalue.n),""), "notfound");
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
                QString string = *(leaf->rvalue.l->lvalue.s);
                QDate date = QDate::fromString(string, "yyyy/MM/dd");
                if (date.isValid()) {
                    rhsdouble = QDate(1900,01,01).daysTo(date);
                    rhsisNumber = true;
                } else {
                    rhsstring = string;
                    rhsisNumber = false;
                }
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
