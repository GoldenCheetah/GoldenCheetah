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
#include <QStringList>
#include "RideCache.h"
#include "RideFile.h" //for SeriesType

class Context;
class RideItem;
class RideMetric;
class FieldDefinition;
class DataFilter;

class Leaf {

    public:

        Leaf() : type(none),op(0),series(NULL) { }

        // evaluate against a RideItem
        double eval(DataFilter *df, Leaf *, RideItem *m);

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
        RideFile::SeriesType seriesType; // for ridefilecache
};

class DataFilter : public QObject
{
    Q_OBJECT

    public:
        DataFilter(QObject *parent, Context *context);

        Context *context;
        QStringList &files() { return filenames; }

        // used by Leaf
        QMap<QString,QString> lookupMap;
        QMap<QString,bool> lookupType; // true if a number, false if a string

    public slots:
        QStringList parseFilter(QString query, QStringList *list=0);
        void clearFilter();
        void configUpdate();

        //void setData(); // set the file list from the current filter

    signals:
        void parseGood();
        void parseBad(QStringList erorrs);

        void results(QStringList);

    private:
        Leaf *treeRoot;
        QStringList errors;

        QStringList filenames;
};

extern int DataFilterdebug;
#endif
