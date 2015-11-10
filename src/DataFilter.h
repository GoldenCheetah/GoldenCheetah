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

class Context;
class RideItem;
class RideMetric;
class FieldDefinition;
class DataFilter;

class Result {
    public:

        // construct a result
        Result (double value) : isNumber(true), string(""), number(value) {}
        Result (QString value) : isNumber(false), string(value), number(0.0f) {}
        Result () : isNumber(true), string(""), number(0) {}

        // we can't use QString with union
        bool isNumber;           // if true, value is numeric
        QString string;
        double number;
        QList<double> vector;
};

class Leaf {
    Q_DECLARE_TR_FUNCTIONS(Leaf)

    public:

        Leaf(int loc, int leng) : type(none),op(0),series(NULL),dynamic(false),loc(loc),leng(leng),inerror(false) { }

        // evaluate against a RideItem
        Result eval(Context *context, DataFilter *df, Leaf *, float x, RideItem *m, RideFilePoint *p = NULL);

        // tree traversal etc
        void print(Leaf *, int level);  // print leaf and all children
        void color(Leaf *, QTextDocument *);  // update the document to match
        bool isDynamic(Leaf *);
        void validateFilter(DataFilter *, Leaf*); // validate
        bool isNumber(DataFilter *df, Leaf *leaf);
        void clear(Leaf*);
        QString toString(); // return as string
        QString signature() { return toString(); }

        enum { none, Float, Integer, String, Symbol, 
               Logical, Operation, BinaryOperation, UnaryOperation,
               Function, Conditional, Vector,
               Parameters } type;

        union value {
            float f;
            int i;
            QString *s;
            QString *n;
            Leaf *l;
        } lvalue, rvalue, cond;

        int op;
        QString function;    // function
        QList<Leaf*> fparms; // passed parameters

        Leaf *series; // is a symbol
        bool dynamic;
        RideFile::SeriesType seriesType; // for ridefilecache
        int loc, leng;
        bool inerror;
};

class DataFilter : public QObject
{
    Q_OBJECT

    public:
        DataFilter(QObject *parent, Context *context);
        DataFilter(QObject *parent, Context *context, QString formula);

        Context *context;
        QStringList &files() { return filenames; }
        QString signature() { return sig; }
        Leaf *root() { return treeRoot; }

        // needs to be reapplied as the ride selection changes
        bool isdynamic;

        // used by Leaf
        QMap<QString,QString> lookupMap;
        QMap<QString,bool> lookupType; // true if a number, false if a string

        // when used for formulas
        Result evaluate(RideItem *rideItem, RideFilePoint *p = NULL);
        QStringList getErrors() { return errors; };
        void colorSyntax(QTextDocument *content, int pos);

        static QStringList functions(); // return list of functions supported
        QStringList dataSeriesSymbols;

        // pd models for estimates
        QList <PDModel*>models;

        // microcache for oft-repeated vector operations
        QHash<QString, Result> snips;

    public slots:
        QStringList parseFilter(QString query, QStringList *list=0);
        QStringList check(QString query);
        void clearFilter();
        void configChanged(qint32);
        void dynamicParse();

        //void setData(); // set the file list from the current filter

    signals:
        void parseGood();
        void parseBad(QStringList erorrs);

        void results(QStringList);


    private:
        void setSignature(QString &query);

        Leaf *treeRoot;
        QStringList errors;

        QStringList filenames;
        QStringList *list;
        QString sig;
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
