/*
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_BestIntervalDialog_h
#define _GC_BestIntervalDialog_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QTableWidget>
#include <QCheckBox>
#include <QDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QLabel>

class Context;
class RideFile;
class Specification;

class BestIntervalDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:

        struct BestInterval {
            double start, stop, avg;
            BestInterval(double start, double stop, double avg) :
                start(start), stop(stop), avg(avg) {}
        };

        BestIntervalDialog(Context *context);

        static void findBests(const RideFile *ride, Specification spec, double windowSizeSecs,
                              int maxIntervals, QList<BestInterval> &results);

        static void findBestsKPH(const RideFile *ride, Specification spec, double windowSizeSecs,
                              int maxIntervals, QList<BestInterval> &results);

    private slots:
        void findClicked();
        void doneClicked();
        void addClicked(); // add to inverval selections

    private:

        Context *context;
        QPushButton *findButton, *doneButton, *addButton;
        QDoubleSpinBox *hrsSpinBox, *minsSpinBox, *secsSpinBox, *countSpinBox;
        QTableWidget *resultsTable;
};

#endif // _GC_BestIntervalDialog_h

