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

#ifndef _GC_AddIntervalDialog_h
#define _GC_AddIntervalDialog_h 1
#include "GoldenCheetah.h"
#include "RideFile.h"

#include <QtGui>
#include <QDialog>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QCheckBox>
#include <QButtonGroup>

class Context;

class AddIntervalDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:

        struct AddedInterval {
            QString name;
            double start, stop, avg;
            AddedInterval() : start(0), stop(0), avg(0) {}
            AddedInterval(double start, double stop, double avg) :
                start(start), stop(stop), avg(avg) {}
        };

        AddIntervalDialog(Context *context);

        static void findPeakPowerStandard(Context *context, const RideFile *ride, QList<AddedInterval> &results);

        static void findPeaks(Context *context, bool typeTime, const RideFile *ride, RideFile::SeriesType series,
                              RideFile::Conversion conversion, double windowSizeSecs,
                              int maxIntervals, QList<AddedInterval> &results, QString prefixe, QString overideName);

        static void findFirsts(bool typeTime, const RideFile *ride, double windowSizeSecs,
                               int maxIntervals, QList<AddedInterval> &results);

    private slots:
        void createClicked();
        void addClicked(); // add to inverval selections

        void methodFirstClicked();
        void methodPeakPowerClicked();
        void methodWPrimeClicked();
        void methodClimbClicked();
        void methodHeartRateClicked();
        void methodPeakPaceClicked();
        void methodPeakSpeedClicked();
        void peakPowerStandardClicked();
        void peakPowerCustomClicked();
        void typeTimeClicked();
        void typeDistanceClicked();


    private:

        Context *context;
        QWidget *intervalMethodWidget, *intervalPeakPowerWidget, *intervalTypeWidget, 
                *intervalTimeWidget, *intervalDistanceWidget, *intervalClimbWidget,
                *intervalCountWidget, *intervalWPrimeWidget;

        QHBoxLayout *intervalPeakPowerTypeLayout;
        QPushButton *createButton, *addButton;
        QDoubleSpinBox *hrsSpinBox, *minsSpinBox, *secsSpinBox, *altSpinBox,
                       *countSpinBox,*kmsSpinBox, *msSpinBox, *kjSpinBox;
        QRadioButton *methodFirst, *methodPeakPower, *methodWPrime, *methodClimb, *methodHeartRate,
                     *methodPeakSpeed, *methodPeakPace;
        QRadioButton *typeDistance, *typeTime, *peakPowerStandard, *peakPowerCustom;
        QTableWidget *resultsTable;
};

#endif // _GC_AddIntervalDialog_h

