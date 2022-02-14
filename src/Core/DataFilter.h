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

#ifndef _GC_DataFilter_h
#define _GC_DataFilter_h

#include <QString>
#include <QObject>
#include <QDebug>
#include <QList>
#include <QMap>
#include <QHash>
#include <QStringList>
#include <QTextDocument>
#include "RideCache.h"
#include "RideFile.h" //for SeriesType
#include "Utils.h" //for SeriesType

#include <gsl/gsl_randist.h>

class Context;
class RideItem;
class RideMetric;
class FieldDefinition;
class DataFilter;
class DataFilterRuntime;

class Result {
    public:

        // construct a result
        Result (double value) : isNumber(true), string_(""), number_(value) {}
        Result (QVector<double>x) : isNumber(true), string_(""), number_(0), vector(x) { foreach(double n, x) number_ += n; }
        Result (QString value) : isNumber(false), string_(value), number_(0.0f) {}
        Result (QVector<QString> &list) : isNumber(false), string_(""), number_(0.0f), strings(list) {}
        Result () : isNumber(true), string_(""), number_(0) {}

        // vectorize, turn into vector of size n
        void vectorize(int size);

        // we can't use QString with union
        bool isNumber;           // if true, value is numeric
        bool isVector() const { return vector.count() > 0 || strings.count() > 0; }

        // return as number or string, coerce if needed
        double &number() {
            if (!isNumber) {
                if (!isVector()) number_ = string_.toDouble();
                else asNumeric(); // this will coerce and crucially compute sum
            }
            return number_;
        }

        QString &string() { if (isNumber) string_ = Utils::removeDP("%1").arg(number_);
                            else if (strings.count() == 1) string_ = strings.at(0); // when vector is only 1 entry
                            return string_; }

        // coerce strings to numbers
        QVector<double>&asNumeric() {
            if (!isNumber) {
                if (strings.count() == vector.count()) return vector;
                else {
                    vector.clear();
                    number_=0;
                    for(int i=0; i<strings.count(); i++) {
                        double v = strings.at(i).toDouble();
                        vector << v;
                        number_ += v;
                    }
                }
            }
            return vector;
        }

        // coerce numbers to strings
        QVector<QString> &asString() {
            if (isNumber) {
                if (strings.count() == vector.count()) return strings;
                else {
                    strings.clear();
                    for(int i=0; i<vector.count(); i++)  strings << Utils::removeDP(QString("%1").arg(vector.at(i)));
                }
            }
            return strings;
        }

    private:

        QString string_;
        double number_;
        QVector<double> vector;
        QVector<QString> strings;

};

class DataFilterRuntime;
class Leaf {
    Q_DECLARE_TR_FUNCTIONS(Leaf)

    public:

        Leaf(int loc, int leng) : type(none),op(0),series(NULL),dynamic(false),loc(loc),leng(leng),inerror(false) { }

        // evaluate against a RideItem using its context
        //
        // Used in the following contexts;
        //
        // LTM chart - using symbols from RideItem *m
        // Ride Plot - using symbols from RideItem *m
        // Search/Filter - using symbols from RideItem *m
        // User Metric - using symbols from QHash<..> (RideItem + Interval) and
        // Spec to delimit samples in R/Python Scripts
        //
        Result eval(DataFilterRuntime *df, Leaf *, const Result &x, long it, RideItem *m, RideFilePoint *p = NULL, const QHash<QString,RideMetric*> *metrics=NULL, const Specification &spec=Specification(), const  DateRange &d=DateRange());

        // tree traversal etc
        void print(int level, DataFilterRuntime*);  // print leaf and all children
        void color(Leaf *, QTextDocument *);  // update the document to match
        bool isDynamic(Leaf *);
        void validateFilter(Context *context, DataFilterRuntime *, Leaf*); // validate
        bool isNumber(DataFilterRuntime *df, Leaf *leaf);
        void findSymbols(QStringList &symbols); // when working with formulas
        void clear(Leaf*);
        QString toString(); // return as string
        QString signature() { return toString(); }

        enum { none, Float, Integer, String, Symbol, 
               Logical, Operation, BinaryOperation, UnaryOperation,
               Function, Conditional, Index, Select,
               Compound, Script } type;

        union value {
            float f;
            int i;
            QString *s;
            QString *n;
            Leaf *l;
            QList<Leaf *> *b;
        } lvalue, rvalue, cond;

        int op;
        QString function;    // function
        QList<Leaf*> fparms; // passed parameters

        Leaf *series; // is a symbol
        bool dynamic;
        RideFile::SeriesType seriesType; // for ridefilecache
        int loc, leng;
        bool inerror;
        RideFile::XDataJoin xjoin; // how to join xdata with main
};

class UserChart;
class GenericAnnotationInfo;
class DataFilterRuntime {

    // allocated for each thread to avoid race
    // conditions when computing user metrics
    // in a QConcurrent::map operation

public:

    // stack count (to stop recursion 'hanging'
    int stack;

    // needs to be reapplied as the ride selection changes
    bool isdynamic;

    // the user chart if were used for that
    // enables use of cache() command
    UserChart *chart;

    // Lookup tables
    QMap<QString,QString> lookupMap;
    QMap<QString,bool> lookupType; // true if a number, false if a string

    // map to adata series
    QStringList dataSeriesSymbols;

    // user defined symbols
    QHash<QString, Result> symbols;

    // user defined functions
    QHash<QString, Leaf*> functions;

    QHash<Leaf*, int> indexes;

    // pd models for estimates
    QList <PDModel*>models;

#ifdef GC_WANT_PYTHON
    // embedded python runtime
    double runPythonScript(Context *context, QString script, RideItem *m, const QHash<QString,RideMetric*> *metrics, Specification spec);
#endif

    DataFilter *owner;
};

class DataFilter : public QObject
{
    Q_OBJECT

    public:
        DataFilter(QObject *parent, Context *context);
        DataFilter(QObject *parent, Context *context, QString formula);

        // runtime passed by datafilter
        DataFilterRuntime rt;
        QObject *parent() { return parent_; }

        // compile time errors
        QStringList &errorList() { return errors; }

        // get a signature for a datafilter
        static QString fingerprint(QString &query);

        Context *context;
        QStringList &files() { return filenames; }
        QString signature() { return sig; }
        Leaf *root() { return treeRoot; }

        // for random number generation
        const gsl_rng_type *T;
        gsl_rng *r;

        // RideItem always available and supplies th context
        Result evaluate(RideItem *rideItem, RideFilePoint *p);
        Result evaluate(DateRange dr, QString filter="");
        Result evaluate(Specification spec, DateRange dr);
        QStringList getErrors() { return errors; };
        void colorSyntax(QTextDocument *content, int pos);

        static QStringList builtins(Context *); // return list of functions supported

        int refcount; // used by user metrics

    public slots:
        QStringList parseFilter(Context *context, QString query, QStringList *list=0);
        QStringList check(QString query);
        void clearFilter();
        void configChanged(qint32);
        void dynamicParse();

        //void setData(); // set the file list from the current filter

    signals:
        void parseGood();
        void parseBad(QStringList erorrs);
        void results(QStringList);

        void annotate(GenericAnnotationInfo&);

    private:
        void setSignature(QString &query);

        Leaf *treeRoot;
        QStringList errors;

        QStringList filenames;
        QStringList *list;
        QString sig;

        QObject *parent_;
};

// general purpose model fitting to x/y data
class DFModel : public PDModel
{
    Q_OBJECT

    public:
        DFModel(RideItem *item, Leaf *formula, DataFilterRuntime *df);

        // we override locally because we are more general
        // purpose and also, always use lmfit
        // we don't fit in to the usual PDModel framework
        // so just do our own thing mostly.
        bool fitData(QVector<double>&x, QVector<double>&y);

        // synthetic data for a curve
        virtual double y(double t) const;

        //  number of parameters (not including t)
        int nparms() { return parameters.count(); }

        // passed value t and candidate parameters
        // must unpack into df symbols and calulate a result
        // used during the fitting process
        double f(double t, const double *parms);

        // managing the formula and parameters
        RideItem *item;
        Leaf *formula;          // the actual formula
        DataFilterRuntime *df;  // runtime with symbols and values
        QStringList parameters; // parameter names and position

        // basically everything else below here is
        // ignored for the most part
        bool setParms(double *) {  return true;}

        // model methods for CP chart are irrelevant
        // but we need to set them anyway
        bool hasWPrime() { return false; }  // can estimate W'
        bool hasCP()     { return false; }  // can CP
        bool hasFTP()    { return false; }  // can estimate FTP
        bool hasPMax()   { return false; }  // can estimate p-Max

        QString name()   { return "Datafilter General Purpose Model"; }
        QString code()   { return "DF Model"; }        // short name used in metric names e.g. 2P model

        void saveParameters(QList<double>&) {}
        void loadParameters(QList<double>&) {}

    public slots:

        // we are only used in data filter expressions
        void onDataChanged() {}
        void onIntervalsChanged() {}
};
extern int DataFilterdebug;

// some constants we provide to help accessed via const(name)
#define MATHCONST_E 		    2.718281828459045235360287471352662498L /* e */
#define MATHCONST_LOG2E 	    1.442695040888963407359924681001892137L /* log_2 e */
#define MATHCONST_LOG10E 	    0.434294481903251827651128918916605082L /* log_10 e */
#define MATHCONST_LN2 		    0.693147180559945309417232121458176568L /* log_e 2 */
#define MATHCONST_LN10 	    2.302585092994045684017991454684364208L /* log_e 10 */
#define MATHCONST_PI 		    3.141592653589793238462643383279502884L /* pi */
#define MATHCONST_PI_2 	    1.570796326794896619231321691639751442L /* pi/2 */
#define MATHCONST_PI_4 	    0.785398163397448309615660845819875721L /* pi/4 */
#define MATHCONST_1_PI 	    0.318309886183790671537767526745028724L /* 1/pi */
#define MATHCONST_2_PI 	    0.636619772367581343075535053490057448L /* 2/pi */
#define MATHCONST_2_SQRTPI 	1.128379167095512573896158903121545172L /* 2/sqrt(pi) */
#define MATHCONST_SQRT2 	    1.414213562373095048801688724209698079L /* sqrt(2) */
#define MATHCONST_SQRT1_2 	    0.707106781186547524400844362104849039L /* 1/sqrt(2) */

#endif
