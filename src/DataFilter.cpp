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
    case Leaf::Conditional :
        {
                    return leaf->isDynamic(leaf->cond.l) ||
                           leaf->isDynamic(leaf->lvalue.l) ||
                           leaf->isDynamic(leaf->rvalue.l);
                    break;
        }
        break;

    }
    return false;
}

void Leaf::print(Leaf *leaf, int level)
{
    qDebug()<<"LEVEL"<<level;
    switch(leaf->type) {
    case Leaf::Float : qDebug()<<"float"<<leaf->lvalue.f<<leaf->dynamic; break;
    case Leaf::Integer : qDebug()<<"integer"<<leaf->lvalue.i<<leaf->dynamic; break;
    case Leaf::String : qDebug()<<"string"<<*leaf->lvalue.s<<leaf->dynamic; break;
    case Leaf::Symbol : qDebug()<<"symbol"<<*leaf->lvalue.n<<leaf->dynamic; break;
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
    case Leaf::Conditional : qDebug()<<"cond";
        {
                    leaf->print(leaf->cond.l, level+1);
                    leaf->print(leaf->lvalue.l, level+1);
                    leaf->print(leaf->rvalue.l, level+1);
        }
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
            else if (symbol == "isSwim") return true;
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
                    symbol != "isSwim" && symbol != "isRun" && !isCoggan(symbol))
                    DataFiltererrors << QString(QObject::tr("%1 is unknown")).arg(symbol);

                if (symbol.compare("Current", Qt::CaseInsensitive))
                    leaf->dynamic = true;
            }
        }
        break;

    case Leaf::Function :
        {
            // is the symbol valid?
            QRegExp bestValidSymbols("^(apower|power|hr|cadence|speed|torque|vam|xpower|np|wpk)$", Qt::CaseInsensitive);
            QRegExp tizValidSymbols("^(power|hr)$", Qt::CaseInsensitive);
            QString symbol = *(leaf->series->lvalue.n);

            if (leaf->function == "sts" || leaf->function == "lts" || leaf->function == "sb" || leaf->function == "rr") {

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

    case Leaf::Conditional :
        {
            // three expressions to validate
            validateFilter(df, leaf->cond.l);
            validateFilter(df, leaf->lvalue.l);
            validateFilter(df, leaf->rvalue.l);
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

DataFilter::DataFilter(QObject *parent, Context *context, QString formula) : QObject(parent), context(context), isdynamic(false), treeRoot(NULL)
{
    configChanged(CONFIG_FIELDS);

    DataFiltererrors.clear(); // clear out old errors
    DataFilter_setString(formula);
    DataFilterparse();
    DataFilter_clearString();
    treeRoot = root;

    // if it parsed (syntax) then check logic (semantics)
    if (treeRoot && DataFiltererrors.count() == 0)
        treeRoot->validateFilter(this, treeRoot);
    else
        treeRoot=NULL;

    // save away the results if it passed semantic validation
    if (DataFiltererrors.count() != 0)
        treeRoot= NULL;
}

Result DataFilter::evaluate(RideItem *item)
{
    if (!item || !treeRoot || DataFiltererrors.count()) return Result(0);

    Result res = treeRoot->eval(context, this, treeRoot, item);
    return res;
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

        // successfully parsed, lets check semantics
        //treeRoot->print(treeRoot);
        emit parseGood();

        // clear current filter list
        filenames.clear();

        // get all fields...
        foreach(RideItem *item, context->athlete->rideCache->rides()) {

            // evaluate each ride...
            Result result = treeRoot->eval(context, this, treeRoot, item);
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
    if (isdynamic) {
        // need to reapply on current state

        // clear current filter list
        filenames.clear();

        // get all fields...
        foreach(RideItem *item, context->athlete->rideCache->rides()) {

            // evaluate each ride...
            Result result = treeRoot->eval(context, this, treeRoot, item);
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

Result Leaf::eval(Context *context, DataFilter *df, Leaf *leaf, RideItem *m)
{
    switch(leaf->type) {

    //
    // LOGICAL EXPRESSION
    //
    case Leaf::Logical  :
    {
        switch (leaf->op) {
            case AND :
            {
                Result left = eval(context, df, leaf->lvalue.l, m);
                if (left.isNumber && left.number) {
                    Result right = eval(context, df, leaf->rvalue.l, m);
                    if (right.isNumber && right.number) return Result(true);
                }
                return Result(false);
            }
            case OR :
            {
                Result left = eval(context, df, leaf->lvalue.l, m);
                if (left.isNumber && left.number) return Result(true);

                Result right = eval(context, df, leaf->rvalue.l, m);
                if (right.isNumber && right.number) return Result(true);

                return Result(false);
            }

            default : // parenthesis
                return (eval(context, df, leaf->lvalue.l, m));
        }
    }
    break;

    //
    // FUNCTIONS
    //
    case Leaf::Function :
    {
        double duration;

        // pmc data ...
        if (leaf->function == "sts" || leaf->function == "lts" || leaf->function == "sb" || leaf->function == "rr") {

                // get metric technical name
                QString symbol = *(leaf->series->lvalue.n);
                QString lookup = df->lookupMap.value(symbol, "");
                PMCData *pmcData = context->athlete->getPMCFor(lookup);

                if (leaf->function == "sts") return Result(pmcData->sts(m->dateTime.date()));
                if (leaf->function == "lts") return Result(pmcData->lts(m->dateTime.date()));
                if (leaf->function == "sb") return Result(pmcData->sb(m->dateTime.date()));
                if (leaf->function == "rr") return Result(pmcData->rr(m->dateTime.date()));
        }


        switch (leaf->lvalue.l->type) {

            default:
            case Leaf::Function :
            {
                duration = eval(context, df, leaf->lvalue.l, m).number; // duration will be zero if string
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
            return Result(RideFileCache::best(df->context, m->fileName, leaf->seriesType, duration));

        if (leaf->function == "tiz") // duration is really zone number
            return Result(RideFileCache::tiz(df->context, m->fileName, leaf->seriesType, duration));

        // unknown function!?
        return Result(0) ;
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

        // is it isRun ?
        if (symbol == "isRun") {
            lhsdouble = m->isRun ? 1 : 0;
            lhsisNumber = true;

        } else if (symbol == "isSwim") {
            lhsdouble = m->isSwim ? 1 : 0;
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

        } else if ((lhsisNumber = df->lookupType.value(*(leaf->lvalue.n))) == true) {
            // get symbol value
            // check metadata string to number first ...
            QString meta = m->getText(rename=df->lookupMap.value(symbol,""), "unknown");
            if (meta == "unknown")
                lhsdouble = m->getForSymbol(rename=df->lookupMap.value(symbol,""));
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
    // BINARY EXPRESSION
    //
    case Leaf::BinaryOperation :
    case Leaf::Operation :
    {
        // lhs and rhs
        Result lhs = eval(context, df, leaf->lvalue.l, m);
        Result rhs = eval(context, df, leaf->rvalue.l, m);

        // NOW PERFORM OPERATION
        switch (leaf->op) {

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

    case Leaf::Conditional :
    {
        Result cond = eval(context, df, leaf->cond.l, m);
        if (cond.isNumber && cond.number) return eval(context, df, leaf->lvalue.l, m);
        else return eval(context, df, leaf->rvalue.l, m);
    }
    break;

    default: // we don't need to evaluate any lower - they are leaf nodes handled above
        break;
    }
    return Result(false);
}
