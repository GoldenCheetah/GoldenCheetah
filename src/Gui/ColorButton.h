/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_ColorButton_h
#define _GC_ColorButton_h 1
#include "GoldenCheetah.h"
#include "Colors.h"

#include <QPushButton>
#include <QDialog>
#include <QColor>

class ColorButton : public QPushButton
{
    Q_OBJECT
    G_OBJECT


    public:
        ColorButton(QWidget *parent, QString, QColor, bool gc=false, bool ignore=false);

        void setColor(QColor);
        QColor getColor() { return color; }
        QString getName() { return name; }
        void setSelectAll(bool x) { all=x; }

    public slots:
        void clicked();

    signals:
        void colorChosen(QColor);

    protected:
        bool gc;
        bool all;
        QColor color;
        QString name;
};

class QColorDialog;
class QTabWidget;
class QBoxLayout;
class QTreeWidget;
class QSignalMapper;
class QLineEdit;
class GColorDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

    public:
        // main constructor
        GColorDialog(QColor selected, QWidget *parent, bool all);
        QColor returned() { return returning; }

        // User entry point- opens a dialog gets the answer
        // and returns, intended as a drop in replacement for
        // the standard dialog QColorDialog::getColor() method
        //
        // It differs in so much that it will return a color encoded
        // 1,1,x for GC colors configured in preferences
        // where 1,1,1 is the marker color for historic reasons
        // all other cases can be passed to the GColor() macro
        // to convert from the GC color settings (set in appearances)
        //
        // or just r,g,b for normal colors (meaning that: 1,1,x is never a possible color)
        //
        static QColor getColor(QColor color, bool all=false);

    public slots:

        // search filter
        void searchFilter(QString);            // sets rows hidden if they are not found

        // trap buttons
        void cancelClicked();           // return what we got passed

        void standardOKClicked(QColor); // standard dialog selected r,g,b
        void gcOKClicked();             // gc dialog selected 1,1,x
        void gcClicked(int);            // color button clicked in dialog- saves a user click

    private:
        // original color passed as #rrggbb string
        QColor original;
        QColor returning;

        // container and basic setup
        QVBoxLayout *mainLayout;
        QTabWidget *tabwidget;

        // standard color dialog (first tab)
        QColorDialog *colordialog;

        // gc dialog (second tab)
        const Colors *colorSet;
        QWidget *gcdialog;
        QLineEdit *searchEdit;
        QTreeWidget *colorlist;
        QPushButton *cancel, *ok;
        QSignalMapper *mapper;

        bool all;
};

#endif
