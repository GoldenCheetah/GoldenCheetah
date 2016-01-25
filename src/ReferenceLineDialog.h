/*
 * Copyright (c) 2012 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#ifndef _GC_ReferenceLineDialog_h
#define _GC_ReferenceLineDialog_h 1
#include "GoldenCheetah.h"
#include "AllPlot.h"

#include <QtGui>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>

class Context;
class RideFile;

class ReferenceLineDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        ReferenceLineDialog(AllPlot *parent, Context *context, RideFile::SeriesType, bool allowDelete=false);

        void setValueForAxis(double value, int axis);

    private slots:
        void addClicked();
        void deleteRef();
        void closed();

    private:
        AllPlot *parent;
        Context *context;
        RideFile::SeriesType serie;
        bool allowDelete;

        QLineEdit *refValue;
        QLabel *refUnit;

        int axis;

        QTableWidget *refsTable;
        void refreshTable();

        QPushButton *addButton, *cancelButton;
        QPushButton *deleteRefButton;
};

#endif // _GC_ReferenceLineDialog_h

