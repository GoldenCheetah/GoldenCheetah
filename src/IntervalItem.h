/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_IntervalItem_h
#define _GC_IntervalItem_h 1
#include "GoldenCheetah.h"

#include <QtGui>

class RideFile;

class IntervalItem : public QTreeWidgetItem
{
    public:
        const RideFile *ride;
        double start, stop; // by Time
        double startKM, stopKM; // by Distance
        int displaySequence;

        IntervalItem(const RideFile *, QString, double, double, double, double, int);
        void setDisplaySequence(int seq) { displaySequence = seq; }
};

class RenameIntervalDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        RenameIntervalDialog(QString &, QWidget *);

    public slots:
        void applyClicked();
        void cancelClicked();

    private:
        QString &string;
        QPushButton *applyButton, *cancelButton;
        QLineEdit *nameEdit;
};
#endif // _GC_IntervalItem_h

