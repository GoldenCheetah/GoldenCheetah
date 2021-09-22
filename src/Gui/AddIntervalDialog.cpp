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

#include "AddIntervalDialog.h"
#include "Settings.h"
#include "Athlete.h"
#include "Context.h"
#include "Units.h"
#include "IntervalItem.h"
#include "RideFile.h"
#include "RideItem.h"
#include "Colors.h"
#include "WPrime.h"
#include "HelpWhatsThis.h"
#include <QMap>
#include <cmath>

// helper function
static void clearResultsTable(QTableWidget *);

AddIntervalDialog::AddIntervalDialog(Context *context) :
    context(context)
{
    setWindowIcon(QIcon(":/images/gc.png"));
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Find Intervals"));
    setMinimumWidth(400 *dpiXFactor);
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::FindIntervals));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    intervalMethodWidget = new QWidget();
    mainLayout->addWidget(intervalMethodWidget);
    QHBoxLayout *intervalMethodLayout = new QHBoxLayout(intervalMethodWidget);;
    intervalMethodLayout->addStretch();

    QLabel *intervalMethodLabel = new QLabel(tr("Method: "), this);
    intervalMethodLayout->addWidget(intervalMethodLabel);
    QButtonGroup *methodButtonGroup = new QButtonGroup(this);

    QVBoxLayout *methodRadios = new QVBoxLayout;
    intervalMethodLayout->addLayout(methodRadios);
    intervalMethodLayout->addStretch();

    methodPeakPower = new QRadioButton(tr("Peak Power"));
    methodPeakPower->setChecked(true);
    methodButtonGroup->addButton(methodPeakPower);
    methodRadios->addWidget(methodPeakPower);

    methodClimb = new QRadioButton(tr("Ascent (elevation)"));
    methodClimb->setChecked(false);
    methodButtonGroup->addButton(methodClimb);
    methodRadios->addWidget(methodClimb);

    methodWPrime = new QRadioButton(tr("W' (Energy)"));
    methodWPrime->setChecked(false);
    methodButtonGroup->addButton(methodWPrime);
    methodRadios->addWidget(methodWPrime);

    methodHeartRate = new QRadioButton(tr("Heart rate"));
    methodHeartRate->setChecked(false);
    methodButtonGroup->addButton(methodHeartRate);
    methodRadios->addWidget(methodHeartRate);

    methodPeakSpeed = new QRadioButton(tr("Peak Speed"));
    methodPeakSpeed->setChecked(false);
    methodButtonGroup->addButton(methodPeakSpeed);
    methodRadios->addWidget(methodPeakSpeed);

    methodPeakPace = new QRadioButton(tr("Peak Pace"));
    methodPeakPace->setChecked(false);
    methodButtonGroup->addButton(methodPeakPace);
    methodRadios->addWidget(methodPeakPace);

    methodFirst = new QRadioButton(tr("Time / Distance"));
    methodFirst->setChecked(false);
    methodButtonGroup->addButton(methodFirst);
    methodRadios->addWidget(methodFirst);

    intervalPeakPowerWidget = new QWidget();
    intervalPeakPowerTypeLayout = new QHBoxLayout;
    intervalPeakPowerTypeLayout->addStretch();
    QLabel *intervalPeakPowerTypeLabel = new QLabel(tr("Type: "), this);
    intervalPeakPowerTypeLayout->addWidget(intervalPeakPowerTypeLabel);
    QButtonGroup *peakPowerTypeButtonGroup = new QButtonGroup(this);
    peakPowerStandard = new QRadioButton(tr("Standard"));
    peakPowerStandard->setChecked(true);
    peakPowerTypeButtonGroup->addButton(peakPowerStandard);
    intervalPeakPowerTypeLayout->addWidget(peakPowerStandard);
    peakPowerCustom = new QRadioButton(tr("Custom"));
    peakPowerTypeButtonGroup->addButton(peakPowerCustom);
    intervalPeakPowerTypeLayout->addWidget(peakPowerCustom);
    intervalPeakPowerTypeLayout->addStretch();
    //mainLayout->addLayout(intervalPeakPowerTypeLayout);
    intervalPeakPowerWidget->setLayout(intervalPeakPowerTypeLayout);
    intervalPeakPowerWidget->hide();
    mainLayout->addWidget(intervalPeakPowerWidget);

    intervalTypeWidget = new QWidget();
    QHBoxLayout *intervalTypeLayout = new QHBoxLayout;
    intervalTypeLayout->addStretch();
    QLabel *intervalTypeLabel = new QLabel(tr("Length: "), this);
    intervalTypeLayout->addWidget(intervalTypeLabel);
    QButtonGroup *typeButtonGroup = new QButtonGroup(this);
    typeTime = new QRadioButton(tr("By time"));
    typeTime->setChecked(true);
    typeButtonGroup->addButton(typeTime);
    intervalTypeLayout->addWidget(typeTime);
    typeDistance = new QRadioButton(tr("By distance"));
    typeButtonGroup->addButton(typeDistance);
    intervalTypeLayout->addWidget(typeDistance);
    intervalTypeLayout->addStretch();
    //mainLayout->addLayout(intervalTypeLayout);
    intervalTypeWidget->setLayout(intervalTypeLayout);
    mainLayout->addWidget(intervalTypeWidget);

    intervalTimeWidget = new QWidget();
    QHBoxLayout *intervalTimeLayout = new QHBoxLayout;
    QLabel *intervalTimeLabel = new QLabel(tr("Time: "), this);
    intervalTimeLayout->addWidget(intervalTimeLabel);
    hrsSpinBox = new QDoubleSpinBox(this);
    hrsSpinBox->setDecimals(0);
    hrsSpinBox->setMinimum(0.0);
    hrsSpinBox->setSuffix(tr(" hrs"));
    hrsSpinBox->setSingleStep(1.0);
    hrsSpinBox->setAlignment(Qt::AlignRight);
    intervalTimeLayout->addWidget(hrsSpinBox);
    minsSpinBox = new QDoubleSpinBox(this);
    minsSpinBox->setDecimals(0);
    minsSpinBox->setRange(0.0, 59.0);
    minsSpinBox->setSuffix(tr(" mins"));
    minsSpinBox->setSingleStep(1.0);
    minsSpinBox->setWrapping(true);
    minsSpinBox->setAlignment(Qt::AlignRight);
    minsSpinBox->setValue(1.0);
    intervalTimeLayout->addWidget(minsSpinBox);
    secsSpinBox = new QDoubleSpinBox(this);
    secsSpinBox->setDecimals(0);
    secsSpinBox->setRange(0.0, 59.0);
    secsSpinBox->setSuffix(tr(" secs"));
    secsSpinBox->setSingleStep(1.0);
    secsSpinBox->setWrapping(true);
    secsSpinBox->setAlignment(Qt::AlignRight);
    intervalTimeLayout->addWidget(secsSpinBox);
    //mainLayout->addLayout(intervalLengthLayout);
    intervalTimeWidget->setLayout(intervalTimeLayout);
    mainLayout->addWidget(intervalTimeWidget);

    intervalDistanceWidget = new QWidget();
    QHBoxLayout *intervalDistanceLayout = new QHBoxLayout;
    QLabel *intervalDistanceLabel = new QLabel(tr("Distance: "), this);
    intervalDistanceLayout->addWidget(intervalDistanceLabel);
    kmsSpinBox = new QDoubleSpinBox(this);
    kmsSpinBox->setDecimals(0);
    kmsSpinBox->setRange(0.0, 999);
    kmsSpinBox->setValue(5.0);
    kmsSpinBox->setSuffix(" km");
    kmsSpinBox->setSingleStep(1.0);
    kmsSpinBox->setAlignment(Qt::AlignRight);
    intervalDistanceLayout->addWidget(kmsSpinBox);
    msSpinBox = new QDoubleSpinBox(this);
    msSpinBox->setDecimals(0);
    msSpinBox->setRange(0.0, 999);
    msSpinBox->setSuffix(" m");
    msSpinBox->setSingleStep(1.0);
    msSpinBox->setAlignment(Qt::AlignRight);
    intervalDistanceLayout->addWidget(msSpinBox);
    //mainLayout->addLayout(intervalDistanceLayout);
    intervalDistanceWidget->setLayout(intervalDistanceLayout);
    intervalDistanceWidget->hide();
    mainLayout->addWidget(intervalDistanceWidget);

    intervalCountWidget = new QWidget();
    QHBoxLayout *intervalCountLayout = new QHBoxLayout;
    QLabel *intervalCountLabel = new QLabel(tr("How many to find: "), this);
    intervalCountLayout->addWidget(intervalCountLabel);
    countSpinBox = new QDoubleSpinBox(this);
    countSpinBox->setDecimals(0);
    countSpinBox->setMinimum(1.0);
    countSpinBox->setValue(5.0); // lets default to the top 5 powers
    countSpinBox->setSingleStep(1.0);
    countSpinBox->setAlignment(Qt::AlignRight);
    intervalCountLayout->addWidget(countSpinBox);
    //mainLayout->addLayout(intervalCountLayout);
    intervalCountWidget->setLayout(intervalCountLayout);
    mainLayout->addWidget(intervalCountWidget);

    intervalWPrimeWidget = new QWidget();
    QHBoxLayout *intervalWPrimeLayout = new QHBoxLayout;
    QLabel *intervalWPrimeLabel = new QLabel(tr("Minimum W' Cost: "), this);
    intervalWPrimeLayout->addStretch();
    intervalWPrimeLayout->addWidget(intervalWPrimeLabel);
    kjSpinBox = new QDoubleSpinBox(this);
    kjSpinBox->setDecimals(1);
    kjSpinBox->setRange(0.1, 50);
    kjSpinBox->setValue(2.0);
    kjSpinBox->setSuffix(" kj");
    kjSpinBox->setSingleStep(0.1);
    kjSpinBox->setAlignment(Qt::AlignRight);
    intervalWPrimeLayout->addWidget(kjSpinBox);
    intervalWPrimeLayout->addStretch();
    intervalWPrimeWidget->setLayout(intervalWPrimeLayout);
    intervalWPrimeWidget->hide();
    mainLayout->addWidget(intervalWPrimeWidget);

    intervalClimbWidget = new QWidget();
    QHBoxLayout *intervalClimbLayout = new QHBoxLayout;
    QLabel *intervalClimbLabel = new QLabel(tr("Minimum Ascent: "), this);
    intervalClimbLayout->addStretch();
    intervalClimbLayout->addWidget(intervalClimbLabel);
    altSpinBox = new QDoubleSpinBox(this);
    altSpinBox->setDecimals(1);
    altSpinBox->setRange(10, 5000);
    altSpinBox->setValue(100);
    altSpinBox->setSuffix(" metres");
    altSpinBox->setSingleStep(10);
    altSpinBox->setAlignment(Qt::AlignRight);
    intervalClimbLayout->addWidget(altSpinBox);
    intervalClimbLayout->addStretch();
    intervalClimbWidget->setLayout(intervalClimbLayout);
    intervalClimbWidget->hide();
    mainLayout->addWidget(intervalClimbWidget);

    QHBoxLayout *findbuttonLayout = new QHBoxLayout;
    findbuttonLayout->addStretch();
    createButton = new QPushButton(tr("&Find"), this);
    findbuttonLayout->addWidget(createButton);
    findbuttonLayout->addStretch();
    mainLayout->addLayout(findbuttonLayout);

    QLabel *resultsLabel = new QLabel(tr("Results:"), this);
    mainLayout->addWidget(resultsLabel);

    // user can select from the results to add
    // to the ride intervals
    resultsTable = new QTableWidget;
    mainLayout->addWidget(resultsTable);
    resultsTable->setColumnCount(5);
    resultsTable->setColumnHidden(3, true); // has start time in secs
    resultsTable->setColumnHidden(4, true); // has stop time in secs
    resultsTable->horizontalHeader()->hide();
    resultsTable->horizontalHeader()->setStretchLastSection(true);
//    resultsTable->verticalHeader()->hide();
    resultsTable->setShowGrid(false);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    addButton = new QPushButton(tr("&Add to Activity"));
    buttonLayout->addWidget(addButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    connect(methodFirst, SIGNAL(clicked()), this, SLOT(methodFirstClicked()));
    connect(methodPeakPower, SIGNAL(clicked()), this, SLOT(methodPeakPowerClicked()));
    connect(methodWPrime, SIGNAL(clicked()), this, SLOT(methodWPrimeClicked()));
    connect(methodClimb, SIGNAL(clicked()), this, SLOT(methodClimbClicked()));
    connect(methodHeartRate, SIGNAL(clicked()), this, SLOT(methodHeartRateClicked()));
    connect(methodPeakPace, SIGNAL(clicked()), this, SLOT(methodPeakPaceClicked()));
    connect(methodPeakSpeed, SIGNAL(clicked()), this, SLOT(methodPeakSpeedClicked()));
    connect(peakPowerStandard, SIGNAL(clicked()), this, SLOT(peakPowerStandardClicked()));
    connect(peakPowerCustom, SIGNAL(clicked()), this, SLOT(peakPowerCustomClicked()));
    connect(typeTime, SIGNAL(clicked()), this, SLOT(typeTimeClicked()));
    connect(typeDistance, SIGNAL(clicked()), this, SLOT(typeDistanceClicked()));
    connect(createButton, SIGNAL(clicked()), this, SLOT(createClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));

    // get set to default to best powers (peaks) 
    methodPeakPowerClicked();
}

void
AddIntervalDialog::methodFirstClicked()
{
    // clear the table
    clearResultsTable(resultsTable);

    intervalPeakPowerWidget->hide();
    intervalClimbWidget->hide();
    intervalWPrimeWidget->hide();
    intervalTypeWidget->show();
    if (typeDistance->isChecked())
        typeDistanceClicked();
    else
        typeTimeClicked();
    intervalCountWidget->show();
}

void
AddIntervalDialog::methodPeakPowerClicked()
{
    // clear the table
    clearResultsTable(resultsTable);

    intervalWPrimeWidget->hide();
    intervalPeakPowerWidget->show();
    intervalClimbWidget->hide();

    if (peakPowerCustom->isChecked())
        peakPowerCustomClicked();
    else
        peakPowerStandardClicked();
}

void
AddIntervalDialog::methodWPrimeClicked()
{
    // clear the table
    clearResultsTable(resultsTable);

    intervalPeakPowerWidget->hide();
    intervalClimbWidget->hide();
    intervalTypeWidget->hide();
    intervalDistanceWidget->hide();
    intervalTimeWidget->hide();
    intervalCountWidget->hide();
    intervalWPrimeWidget->show();
}

void
AddIntervalDialog::methodClimbClicked()
{
    // clear the table
    clearResultsTable(resultsTable);

    intervalClimbWidget->show();
    intervalPeakPowerWidget->hide();
    intervalTypeWidget->hide();
    intervalDistanceWidget->hide();
    intervalTimeWidget->hide();
    intervalCountWidget->hide();
    intervalWPrimeWidget->hide();
}

void
AddIntervalDialog::methodPeakSpeedClicked()
{
    // clear the table
    clearResultsTable(resultsTable);

    intervalClimbWidget->hide();
    intervalPeakPowerWidget->hide();
    intervalWPrimeWidget->hide();

    intervalTypeWidget->show();
    if (typeDistance->isChecked())
        typeDistanceClicked();
    else
        typeTimeClicked();
    intervalCountWidget->show();
}

void
AddIntervalDialog::methodPeakPaceClicked()
{
    // clear the table
    clearResultsTable(resultsTable);

    intervalClimbWidget->hide();
    intervalPeakPowerWidget->hide();
    intervalWPrimeWidget->hide();

    intervalTypeWidget->show();
    if (typeDistance->isChecked())
        typeDistanceClicked();
    else
        typeTimeClicked();
    intervalCountWidget->show();
}

void
AddIntervalDialog::methodHeartRateClicked()
{
    // clear the table
    clearResultsTable(resultsTable);

    intervalClimbWidget->hide();
    intervalPeakPowerWidget->hide();
    intervalWPrimeWidget->hide();

    intervalTypeWidget->show();
    if (typeDistance->isChecked())
        typeDistanceClicked();
    else
        typeTimeClicked();
    intervalCountWidget->show();
}

void
AddIntervalDialog::peakPowerStandardClicked()
{
    // clear the table
    clearResultsTable(resultsTable);

    intervalTypeWidget->hide();
    intervalTimeWidget->hide();
    intervalDistanceWidget->hide();
    intervalCountWidget->hide();
}

void
AddIntervalDialog::peakPowerCustomClicked()
{
    // clear the table
    clearResultsTable(resultsTable);

    intervalTypeWidget->show();
    if (typeDistance->isChecked())
        typeDistanceClicked();
    else
        typeTimeClicked();
    intervalCountWidget->show();
}

void
AddIntervalDialog::typeTimeClicked()
{
    // clear the table
    clearResultsTable(resultsTable);

    intervalDistanceWidget->hide();
    intervalTimeWidget->show();
}

void
AddIntervalDialog::typeDistanceClicked()
{
    // clear the table
    clearResultsTable(resultsTable);

    intervalDistanceWidget->show();
    intervalTimeWidget->hide();
}

// little helper function
static void
clearResultsTable(QTableWidget *resultsTable)
{
    // zap the 3 main cols and two hidden ones
    for (int i=0; i<resultsTable->rowCount(); i++) {
        for (int j=0; j<resultsTable->columnCount(); j++) {
            delete resultsTable->takeItem(i,j);
        }
    }
    resultsTable->setRowCount(0);
}

static double
intervalDuration(const RideFilePoint *start, const RideFilePoint *stop, const RideFile *ride)
{
    return stop->secs - start->secs + ride->recIntSecs();
}

static double
intervalDistance(const RideFilePoint *start, const RideFilePoint *stop, const RideFile *)
{
    return 1000*(stop->km - start->km);// + (ride->recIntSecs()*stop->kph/3600));
}

static bool
intervalsOverlap(const AddIntervalDialog::AddedInterval &a,
                 const AddIntervalDialog::AddedInterval &b)
{
    if ((a.start <= b.start) && (a.stop > b.start))
        return true;
    if ((b.start <= a.start) && (b.stop > a.start))
        return true;
    return false;
}

struct CompareBests {
    // Sort by decreasing power and increasing start time.
    bool operator()(const AddIntervalDialog::AddedInterval &a,
                    const AddIntervalDialog::AddedInterval &b) const {
        if (a.avg > b.avg)
            return true;
        if (b.avg > a.avg)
            return false;
        return a.start < b.start;
    }
};

void
AddIntervalDialog::createClicked()
{
    const RideFile *ride = context->ride ? context->ride->ride() : NULL;
    if (!ride) {
        QMessageBox::critical(this, tr("Select Activity"), tr("No activity selected!"));
        return;
    }

    int maxIntervals = (int) countSpinBox->value();

    double windowSizeMeters = (kmsSpinBox->value() * 1000.0
                             + msSpinBox->value());

    double windowSizeSecs = (hrsSpinBox->value() * 3600.0
                             + minsSpinBox->value() * 60.0
                             + secsSpinBox->value());

    bool byTime = typeTime->isChecked();

    double minWPrime = kjSpinBox->value() * 1000;

    QList<AddedInterval> results;

    // FIND PEAKS
    if (methodPeakPower->isChecked() || methodPeakSpeed->isChecked() ||
            methodPeakPace->isChecked() || methodHeartRate->isChecked()) {

        if (methodPeakPower->isChecked() && peakPowerStandard->isChecked())
            findPeakPowerStandard(context, ride, results);
        else {

            // bad window size?
            if (windowSizeSecs == 0.0) {
                QMessageBox::critical(this, tr("Bad Interval Length"), tr("Interval length must be greater than zero!"));
                return;
            }

            if (methodPeakPower->isChecked()) {
                findPeaks(context, byTime, ride, Specification(), RideFile::watts, RideFile::original, (byTime?windowSizeSecs:windowSizeMeters), maxIntervals, results, tr("Peak Power"),"");
            }
            else if (methodPeakSpeed->isChecked()) {
                findPeaks(context, byTime, ride, Specification(), RideFile::kph, RideFile::original, (byTime?windowSizeSecs:windowSizeMeters), maxIntervals, results, tr("Peak Speed"),"");
            }
            else if (methodPeakPace->isChecked()) {
                findPeaks(context, byTime, ride, Specification(), RideFile::kph, RideFile::pace, (byTime?windowSizeSecs:windowSizeMeters), maxIntervals, results, tr("Peak Pace"), "");
            }
            else if (methodHeartRate->isChecked()) {
                findPeaks(context, byTime, ride, Specification(), RideFile::hr, RideFile::original, (byTime?windowSizeSecs:windowSizeMeters), maxIntervals, results, tr("Peak HR"), "");
            }
        }

    } 

    // FIND PEAKS
    if (methodHeartRate->isChecked()) {

        // bad window size?
        if (windowSizeSecs == 0.0) {
            QMessageBox::critical(this, tr("Bad Interval Length"), tr("Interval length must be greater than zero!"));
            return;
        }


    }




    // FIND BY TIME OR DISTANCE
    if (methodFirst->isChecked()) {

        findFirsts(byTime, ride, (byTime?windowSizeSecs:windowSizeMeters), maxIntervals, results);
    }

    // FIND ASCENTS
    if (methodClimb->isChecked()) {

        // we need altitude and more than 3 data points
        if (ride->areDataPresent()->alt == false || ride->dataPoints().count() < 3) return;

        double hysteresis = appsettings->value(NULL, GC_ELEVATION_HYSTERESIS).toDouble();
        if (hysteresis <= 0.1) hysteresis = 3.00;

        // first apply hysteresis
        QVector<QPoint> points; 

        int index=0;
        int runningAlt = ride->dataPoints().first()->alt;

        foreach(RideFilePoint *p, ride->dataPoints()) {

            // up
            if (p->alt > (runningAlt + hysteresis)) {
                runningAlt = p->alt;
                points << QPoint(index, runningAlt);
            }

            // down
            if (p->alt < (runningAlt - hysteresis)) {
                runningAlt = p->alt;
                points << QPoint(index, runningAlt);
            }
            index++;
        }

        // now find peaks and troughs in the point data
        // there will only be ups and downs, no flats
        QVector<QPoint> peaks;
        for(int i=1; i<(points.count()-1); i++) {

            // peak
            if (points[i].y() > points[i-1].y() &&
                points[i].y() > points[i+1].y()) peaks << points[i];

            // trough
            if (points[i].y() < points[i-1].y() &&
                points[i].y() < points[i+1].y()) peaks << points[i];
        }

        // now run through looking for diffs > requested
        int counter=0;
        for (int i=0; i<(peaks.count()-1); i++) {

            int ascent = 0; // ascent found in meters
            if ((ascent=peaks[i+1].y() - peaks[i].y()) >= altSpinBox->value()) {

                // found one so increment from zero
                counter++;

                // we have a winner...
                struct AddedInterval add;
                add.start = ride->dataPoints()[peaks[i].x()]->secs;
                add.stop = ride->dataPoints()[peaks[i+1].x()]->secs;
                add.name = QString(tr("Climb #%1 (%2m)")).arg(counter)
                                                        .arg(ascent);
                results << add;

            }
        }
    }

    // FIND W' BAL DROPS
    if (methodWPrime->isChecked()) {
        WPrime wp;
        wp.setRide((RideFile*)ride);

        foreach(struct Match match, wp.matches) {
            if (match.cost > minWPrime) {
                struct AddedInterval add;
                add.start = match.start;
                add.stop = match.stop;

                int lenSecs = match.stop-match.start+1;
                QString duration;

                // format the duration nicely!
                if (lenSecs < 120) duration = QString("%1s").arg(lenSecs);
                else {
                    int mins = lenSecs / 60;
                    int secs = lenSecs - (mins * 60);
                    if (secs) {

                        QChar zero = QLatin1Char ( '0' );
                        duration = QString("%1:%2").arg(mins,2,10,zero)
                                                   .arg(secs,2,10,zero);
                    } else duration = QString("%1min").arg(mins);
            
                }
                add.name = QString(tr("Match %1 (%2kJ)")).arg(duration)
                                                        .arg(match.cost/1000.00, 0, 'f', 1);
                results << add;
            }
        }
    }

    // clear the table
    clearResultsTable(resultsTable);

    // populate the table
    resultsTable->setRowCount(results.size());
    int row = 0;
    foreach (const AddedInterval &interval, results) {

        double secs = interval.start;
        double mins = floor(secs / 60);
        secs = secs - mins * 60.0;
        double hrs = floor(mins / 60);
        mins = mins - hrs * 60.0;

        // check box
        QCheckBox *c = new QCheckBox;
        c->setCheckState(Qt::Checked);
        resultsTable->setCellWidget(row, 0, c);

        // start time
        QString start = "%1:%2:%3";
        start = start.arg(hrs, 0, 'f', 0);
        start = start.arg(mins, 2, 'f', 0, QLatin1Char('0'));
        start = start.arg(round(secs), 2, 'f', 0, QLatin1Char('0'));

        QTableWidgetItem *t = new QTableWidgetItem;
        t->setText(start);
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        resultsTable->setItem(row, 1, t);

        QTableWidgetItem *n = new QTableWidgetItem;
        n->setText(interval.name);
        n->setFlags(n->flags() | (Qt::ItemIsEditable));
        resultsTable->setItem(row, 2, n);

        // hidden columns - start, stop
        QString strt = QString("%1").arg(interval.start); // can't use secs as it gets modified
        QTableWidgetItem *st = new QTableWidgetItem;
        st->setText(strt);
        resultsTable->setItem(row, 3, st);

        QString stp = QString("%1").arg(interval.stop); // was interval.start+x
        QTableWidgetItem *sp = new QTableWidgetItem;
        sp->setText(stp);
        resultsTable->setItem(row, 4, sp);

        row++;
    }
    resultsTable->resizeColumnToContents(0);
    resultsTable->resizeColumnToContents(1);
}

void
AddIntervalDialog::findFirsts(bool typeTime, const RideFile *ride, double windowSize,
                              int maxIntervals, QList<AddedInterval> &results)
{
    QList<AddedInterval> bests;

    double secsDelta = ride->recIntSecs();
    double totalWatts = 0.0;
    QList<const RideFilePoint*> window;

    // ride is shorter than the window size!
    if (typeTime && windowSize > ride->dataPoints().last()->secs + secsDelta) return;
    else if (!typeTime && windowSize > ride->dataPoints().last()->km*1000) return;

    double rest = 0;
    // We're looking for intervals with durations in [windowSizeSecs, windowSizeSecs + secsDelta).
    foreach (const RideFilePoint *point, ride->dataPoints()) {

        // Add points until interval duration is >= windowSizeSecs.
        totalWatts += point->watts;
        window.append(point);
        double duration = intervalDuration(window.first(), window.last(), ride);
        double distance = intervalDistance(window.first(), window.last(), ride);

        if ((typeTime && duration >= (windowSize - rest)) ||
            (!typeTime && distance >= (windowSize - rest))) {
            double start = window.first()->secs;
            double stop = start + duration - (typeTime?0:ride->recIntSecs()); // correction for distance
            double avg = totalWatts * secsDelta / duration;
            bests.append(AddedInterval(start, stop, avg));
            rest = (typeTime?duration-windowSize+rest:distance-windowSize+rest);
            // Discard points
            window.clear();
            totalWatts = 0;

            // For distance the last point of an interval is also the first point of the next
            if (!typeTime)  {
                totalWatts += point->watts;
                window.append(point);
            }
        }
    }

    while (!bests.empty() && (results.size() < maxIntervals)) {
        AddedInterval candidate = bests.takeFirst();

        QString name = "#%1 %2%3";
        name = name.arg(results.count()+1);

        if (typeTime)  {
            // best n mins
            if (windowSize < 60) {
                // whole seconds
                name = name.arg(windowSize);
                name = name.arg("sec");
            } else if (windowSize >= 60 && !(((int)windowSize)%60)) {
                // whole minutes
                name = name.arg(windowSize/60);
                name = name.arg("min");
            } else {
                double secs = windowSize;
                double mins = ((int) secs) / 60;
                secs = secs - mins * 60.0;
                double hrs = ((int) mins) / 60;
                mins = mins - hrs * 60.0;
                QString tm = "%1:%2:%3";
                tm = tm.arg(hrs, 0, 'f', 0);
                tm = tm.arg(mins, 2, 'f', 0, QLatin1Char('0'));
                tm = tm.arg(secs, 2, 'f', 0, QLatin1Char('0'));

                // mins and secs
                name = name.arg(tm);
                name = name.arg("");
            }
        } else {
            // best n mins
            if (windowSize < 1000) {
                // whole seconds
                name = name.arg(windowSize);
                name = name.arg("m");
            } else {
                double dist = windowSize;
                double kms = ((int) dist) / 1000;
                dist = dist - kms * 1000.0;
                double ms = dist;

                QString tm = "%1,%2";
                tm = tm.arg(kms);
                tm = tm.arg(ms);

                // km and m
                name = name.arg(tm);
                name = name.arg("km");
            }
        }
        if (candidate.avg > 0) // add average watts only if present
            name = name + QString("  (%4w)").arg(round(candidate.avg));
        candidate.name = name;

        results.append(candidate);
    }

}

void
AddIntervalDialog::findPeakPowerStandard(Context *context, const RideFile *ride, QList<AddedInterval> &results)
{
    QString prefix = tr("Peak");

    findPeaks(context, true, ride, Specification(), RideFile::watts, RideFile::original, 5, 1, results, prefix, "");
    findPeaks(context, true, ride, Specification(), RideFile::watts, RideFile::original, 10, 1, results, prefix, "");
    findPeaks(context, true, ride, Specification(), RideFile::watts, RideFile::original, 20, 1, results, prefix, "");
    findPeaks(context, true, ride, Specification(), RideFile::watts, RideFile::original, 30, 1, results, prefix, "");
    findPeaks(context, true, ride, Specification(), RideFile::watts, RideFile::original, 60, 1, results, prefix, "");
    findPeaks(context, true, ride, Specification(), RideFile::watts, RideFile::original, 120, 1, results, prefix, "");
    findPeaks(context, true, ride, Specification(), RideFile::watts, RideFile::original, 300, 1, results, prefix, "");
    findPeaks(context, true, ride, Specification(), RideFile::watts, RideFile::original, 600, 1, results, prefix, "");
    findPeaks(context, true, ride, Specification(), RideFile::watts, RideFile::original, 1200, 1, results, prefix, "");
    findPeaks(context, true, ride, Specification(), RideFile::watts, RideFile::original, 1800, 1, results, prefix, "");
    findPeaks(context, true, ride, Specification(), RideFile::watts, RideFile::original, 3600, 1, results, prefix, "");
}

void
AddIntervalDialog::findPeaks(Context *context, bool typeTime, const RideFile *ride, Specification spec,
                             RideFile::SeriesType series, RideFile::Conversion conversion, double windowSize,
                              int maxIntervals, QList<AddedInterval> &results, QString prefixe, QString overideName)
{
    QList<AddedInterval> bests;
    QList<AddedInterval> _results;

    double secsDelta = ride->recIntSecs();
    double total = 0.0;
    QList<const RideFilePoint*> window;

    // ride is shorter than the window size!
    if (typeTime && windowSize > ride->dataPoints().last()->secs + secsDelta) return;
    if (!typeTime && windowSize > ride->dataPoints().last()->km*1000) return;

    // We're looking for intervals with durations in [windowSizeSecs, windowSizeSecs + secsDelta).
    RideFileIterator it(const_cast<RideFile*>(ride), spec);
    while (it.hasNext()) {
        struct RideFilePoint *point = it.next();

        // Discard points until interval duration is < windowSizeSecs + secsDelta.
        while ((typeTime && !window.empty() && intervalDuration(window.first(), point, ride) >= windowSize + secsDelta) ||
               (!typeTime && window.length()>1 && intervalDistance(window.at(1), point, ride) >= windowSize)) {
            total -= window.first()->value(series);
            window.takeFirst();
        }
        // Add points until interval duration or distance is >= windowSize.
        total += point->value(series);
        window.append(point);
        double duration = intervalDuration(window.first(), window.last(), ride);
        double distance = intervalDistance(window.first(), window.last(), ride);

        if ((typeTime && duration >= windowSize) ||
            (!typeTime && distance >= windowSize)) {
            double start = window.first()->secs;
            double stop = window.last()->secs; //start + duration;
            double avg = total * secsDelta / duration;
            bests.append(AddedInterval(start, stop, avg));
        }
    }

    std::sort(bests.begin(), bests.end(), CompareBests());

    while (!bests.empty() && (_results.size() < maxIntervals)) {
        AddedInterval candidate = bests.takeFirst();
        bool overlaps = false;
        foreach (const AddedInterval &existing, _results) {
            if (intervalsOverlap(candidate, existing)) {
                overlaps = true;
                break;
            }
        }
        if (!overlaps) {
            QString name = overideName;
            if (overideName == "") {
                name = tr("%1 %3%4 %2");

                if (prefixe == "")
                    name = name.arg(tr("Peak"));
                else
                    name = name.arg(prefixe);

                if (maxIntervals>1)
                    name = name.arg(QString("#%1").arg(_results.count()+1));
                else
                    name = name.arg("");

                if (typeTime)  {
                    // best n mins
                    if (windowSize < 60) {
                        // whole seconds
                        name = name.arg(windowSize);
                        name = name.arg("sec");
                    } else if (windowSize >= 60 && !(((int)windowSize)%60)) {
                        // whole minutes
                        name = name.arg(windowSize/60);
                        name = name.arg("min");
                    } else {
                        double secs = windowSize;
                        double mins = ((int) secs) / 60;
                        secs = secs - mins * 60.0;
                        double hrs = ((int) mins) / 60;
                        mins = mins - hrs * 60.0;
                        QString tm = "%1:%2:%3";
                        tm = tm.arg(hrs, 0, 'f', 0);
                        tm = tm.arg(mins, 2, 'f', 0, QLatin1Char('0'));
                        tm = tm.arg(secs, 2, 'f', 0, QLatin1Char('0'));

                        // mins and secs
                        name = name.arg(tm);
                        name = name.arg("");
                    }
                } else {
                    // best n mins
                    if (windowSize < 1000) {
                        // whole seconds
                        name = name.arg(windowSize);
                        name = name.arg("m");
                    } else {
                        double dist = windowSize;
                        double kms = ((int) dist) / 1000;
                        dist = dist - kms * 1000.0;
                        double ms = dist;

                        QString tm = "%1,%2";
                        tm = tm.arg(kms);
                        tm = tm.arg(ms);

                        // km and m
                        name = name.arg(tm);
                        name = name.arg("km");
                    }
                }
            }
            name += " (%4)";
            name = name.arg(ride->formatValueWithUnit(candidate.avg, series, conversion, context, ride->isSwim()));

            candidate.name = name;
            name = "";
            _results.append(candidate);
        }
    }
    results.append(_results);
}

void
AddIntervalDialog::addClicked()
{
    RideItem *rideItem = const_cast<RideItem*>(context->currentRideItem());
    RideFile *ride = rideItem->ride();

    // run through the table row by row
    // and when the checkbox is shown
    // get name from column 2
    // get start in secs as a string from column 3
    // get stop in secs as a string from column 4
    for (int i=0; i<resultsTable->rowCount(); i++) {

        // is it checked?
        QCheckBox *c = (QCheckBox *)resultsTable->cellWidget(i,0);
        if (c->isChecked()) {
            double start = resultsTable->item(i,3)->text().toDouble();
            double stop = resultsTable->item(i,4)->text().toDouble();
            QString name = resultsTable->item(i,2)->text();


            // add new one to the ride
            rideItem->newInterval(name, start, stop,
                                 ride->timeToDistance(start),
                                 ride->timeToDistance(stop),
                                 Qt::black, false);
        }
    }

    // update the sidebar
    context->notifyIntervalsUpdate(rideItem);

    // charts need to update to reflect the intervals
    context->notifyIntervalsChanged();

    done(0);
}
