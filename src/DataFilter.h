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

#include <QString>
#include <QObject>
#include <QDebug>
#include <QList>
#include <QStringList>

class Context;
class RideMetric;
class FieldDefinition;
class SummaryMetrics;
class DataFilter;

class Leaf {

    public:

        Leaf() : type(none),op(0),series(NULL) { }

        // evaluate against a SummaryMetric
        double eval(DataFilter *df, Leaf *, SummaryMetrics);

        // tree traversal etc
        void print(Leaf *, int level);  // print leaf and all children
        void validateFilter(DataFilter *, Leaf*); // validate
        bool isNumber(DataFilter *df, Leaf *leaf);
        void clear(Leaf*);

        enum { none, Float, Integer, String, Symbol, Logical, Operation, BinaryOperation, Function } type;
        union value {
            float f;
            int i;
            QString *s;
            QString *n;
            Leaf *l;
        } lvalue, rvalue;
        int op;
        QString function;
        Leaf *series; // is a symbol
};

class DataFilter : public QObject
{
    Q_OBJECT

    public:
        DataFilter(QObject *parent, Context *context);
        QStringList &files() { return filenames; }

        // used by Leaf
        QMap<QString,QString> lookupMap;
        QMap<QString,bool> lookupType; // true if a number, false if a string

    public slots:
        QStringList parseFilter(QString query);
        void clearFilter();
        void configUpdate();

        //void setData(); // set the file list from the current filter

    signals:
        void parseGood();
        void parseBad(QStringList erorrs);

        void results(QStringList);

    private:
        Context *context;
        Leaf *treeRoot;
        QStringList errors;

        QStringList filenames;
};

extern int DataFilterdebug;
