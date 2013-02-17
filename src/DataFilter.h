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

class MainWindow;
class RideMetric;
class FieldDefinition;
class SummaryMetrics;

class SymbolDef {

    public:

        enum { Float, Integer, String } type;
        union {

            RideMetric *metric;    // into ride factory
            FieldDefinition *meta; // into field definitions in MainWindow

        } def;
};

class DataFilter;
class Leaf {

    public:

        Leaf() : type(none) { }

        // evaluate against a SummaryMetric
        bool eval(DataFilter *df, Leaf *, SummaryMetrics);

        // tree traversal etc
        void print(Leaf *);  // print leaf and all children
        void validateFilter(DataFilter *, Leaf*); // validate
        bool isNumber(DataFilter *df, Leaf *leaf);
        void clear(Leaf*);

        enum { none, Float, Integer, String, Symbol, Logical, Operation } type;
        union value {
            float f;
            int i;
            QString *s;
            QString *n;
            Leaf *l;
        } lvalue, rvalue;
        int op;
};

class DataFilter : public QObject
{
    Q_OBJECT

    public:
        DataFilter(QObject *parent, MainWindow *main);
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
        MainWindow *main;
        Leaf *treeRoot;
        QStringList errors;

        QStringList filenames;
};
