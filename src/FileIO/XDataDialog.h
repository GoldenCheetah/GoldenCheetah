/*
 * Copyright (c) 2016 Mark Liversedge <liversedge@gmail.com>
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

#ifndef _GC_XDataDialog_h
#define _GC_XDataDialog_h 1
#include "GoldenCheetah.h"
#include "RideFile.h"

#include <QtGui>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>

class Context;
class RideFile;

class XDataDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        XDataDialog(QWidget *parent);
        void setRideItem(RideItem *item);

    private slots:

        void xdataSelected();
        void close();

        void removeXDataClicked();
        void addXDataClicked();

        void removeXDataSeriesClicked();
        void addXDataSeriesClicked();

    private:
        RideItem *item;

        QTableWidget *xdataTable;
        QTableWidget *xdataSeriesTable;

        QPushButton *closeButton;
};

class XDataSettingsDialog : public QDialog
{
    Q_OBJECT

    public:
        XDataSettingsDialog(QWidget *parent, XDataSeries &series);

    private slots:
        void okClicked();

    private:
        XDataSeries &series;

        QLineEdit *xdataName;
        QLineEdit *xdataSeriesName[8];
        QLineEdit *xdataUnitName[8];

        QPushButton *cancelButton, *okButton;

};

class XDataSeriesSettingsDialog : public QDialog
{
    Q_OBJECT

    public:
        XDataSeriesSettingsDialog(QWidget *parent, QString &name, QString &unit);

    private slots:
        void okClicked();

    private:
        QString &name, &unit;
        QLineEdit *nameEdit;
        QLineEdit *unitEdit;

        QPushButton *cancelButton, *okButton;
};

#endif // _GC_XDataDialog_h
