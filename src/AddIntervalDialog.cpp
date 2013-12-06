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
#include "MainWindow.h"
#include "IntervalItem.h"
#include "RideFile.h"
#include <QMap>
#include <math.h>

AddIntervalDialog::AddIntervalDialog(MainWindow *mainWindow) :
    mainWindow(mainWindow)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Add Intervals"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    intervalMethodWidget = new QWidget();
    QHBoxLayout *intervalMethodLayout = new QHBoxLayout;
    QLabel *intervalMethodLabel = new QLabel(tr("Method: "), this);
    intervalMethodLayout->addWidget(intervalMethodLabel);
    QButtonGroup *methodButtonGroup = new QButtonGroup(this);
    methodFirst = new QRadioButton(tr("First"));
    methodFirst->setChecked(true);
    methodButtonGroup->addButton(methodFirst);
    intervalMethodLayout->addWidget(methodFirst);
    methodBestPower = new QRadioButton(tr("Peak Power"));
    methodButtonGroup->addButton(methodBestPower);
    intervalMethodLayout->addWidget(methodBestPower);
    //mainLayout->addLayout(intervalMethodLayout);
    intervalMethodWidget->setLayout(intervalMethodLayout);
    mainLayout->addWidget(intervalMethodWidget);

    intervalPeakPowerWidget = new QWidget();
    intervalPeakPowerTypeLayout = new QHBoxLayout;
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
    //mainLayout->addLayout(intervalPeakPowerTypeLayout);
    intervalPeakPowerWidget->setLayout(intervalPeakPowerTypeLayout);
    intervalPeakPowerWidget->hide();
    mainLayout->addWidget(intervalPeakPowerWidget);

    intervalTypeWidget = new QWidget();
    QHBoxLayout *intervalTypeLayout = new QHBoxLayout;
    QLabel *intervalTypeLabel = new QLabel(tr("Interval length: "), this);
    intervalTypeLayout->addWidget(intervalTypeLabel);
    QButtonGroup *typeButtonGroup = new QButtonGroup(this);
    typeTime = new QRadioButton(tr("By time"));
    typeTime->setChecked(true);
    typeButtonGroup->addButton(typeTime);
    intervalTypeLayout->addWidget(typeTime);
    typeDistance = new QRadioButton(tr("By distance"));
    typeButtonGroup->addButton(typeDistance);
    intervalTypeLayout->addWidget(typeDistance);
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
    hrsSpinBox->setSuffix(" hrs");
    hrsSpinBox->setSingleStep(1.0);
    hrsSpinBox->setAlignment(Qt::AlignRight);
    intervalTimeLayout->addWidget(hrsSpinBox);
    minsSpinBox = new QDoubleSpinBox(this);
    minsSpinBox->setDecimals(0);
    minsSpinBox->setRange(0.0, 59.0);
    minsSpinBox->setSuffix(" mins");
    minsSpinBox->setSingleStep(1.0);
    minsSpinBox->setWrapping(true);
    minsSpinBox->setAlignment(Qt::AlignRight);
    minsSpinBox->setValue(1.0);
    intervalTimeLayout->addWidget(minsSpinBox);
    secsSpinBox = new QDoubleSpinBox(this);
    secsSpinBox->setDecimals(0);
    secsSpinBox->setRange(0.0, 59.0);
    secsSpinBox->setSuffix(" secs");
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
//    resultsTable->verticalHeader()->hide();
    resultsTable->setShowGrid(false);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    createButton = new QPushButton(tr("&Create Intervals"), this);
    buttonLayout->addWidget(createButton);
    doneButton = new QPushButton(tr("&Done"), this);
    buttonLayout->addWidget(doneButton);
    addButton = new QPushButton(tr("&Add to Intervals"));
    buttonLayout->addWidget(addButton);
    mainLayout->addLayout(buttonLayout);

    connect(methodFirst, SIGNAL(clicked()), this, SLOT(methodFirstClicked()));
    connect(methodBestPower, SIGNAL(clicked()), this, SLOT(methodBestPowerClicked()));
    connect(peakPowerStandard, SIGNAL(clicked()), this, SLOT(peakPowerStandardClicked()));
    connect(peakPowerCustom, SIGNAL(clicked()), this, SLOT(peakPowerCustomClicked()));
    connect(typeTime, SIGNAL(clicked()), this, SLOT(typeTimeClicked()));
    connect(typeDistance, SIGNAL(clicked()), this, SLOT(typeDistanceClicked()));




    connect(createButton, SIGNAL(clicked()), this, SLOT(createClicked()));
    connect(doneButton, SIGNAL(clicked()), this, SLOT(doneClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
}

void
AddIntervalDialog::methodFirstClicked()
{
    intervalPeakPowerWidget->hide();
    intervalTypeWidget->show();
    if (typeDistance->isChecked())
        typeDistanceClicked();
    else
        typeTimeClicked();
    intervalCountWidget->show();
}

void
AddIntervalDialog::methodBestPowerClicked()
{
    intervalPeakPowerWidget->show();
    if (peakPowerCustom->isChecked())
        peakPowerCustomClicked();
    else
        peakPowerStandardClicked();
}

void
AddIntervalDialog::peakPowerStandardClicked()
{
    intervalTypeWidget->hide();
    intervalTimeWidget->hide();
    intervalDistanceWidget->hide();
    intervalCountWidget->hide();
}

void
AddIntervalDialog::peakPowerCustomClicked()
{
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
    intervalDistanceWidget->hide();
    intervalTimeWidget->show();
}

void
AddIntervalDialog::typeDistanceClicked()
{
    intervalDistanceWidget->show();
    intervalTimeWidget->hide();
}




// little helper function
static void
clearResultsTable(QTableWidget *resultsTable)
{
    // zap the 3 main cols and two hidden ones
    for (int i=0; i<resultsTable->rowCount(); i++) {
        for (int j=0; j<resultsTable->columnCount(); j++)
            delete resultsTable->takeItem(i,j);
    }
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
    const RideFile *ride = mainWindow->currentRide();
    if (!ride) {
        QMessageBox::critical(this, tr("Select Ride"), tr("No ride selected!"));
        return;
    }

    int maxIntervals = (int) countSpinBox->value();

    double windowSizeSecs = (hrsSpinBox->value() * 3600.0
                             + minsSpinBox->value() * 60.0
                             + secsSpinBox->value());

    double windowSizeMeters = (kmsSpinBox->value() * 1000.0
                             + msSpinBox->value());

    if (windowSizeSecs == 0.0) {
        QMessageBox::critical(this, tr("Bad Interval Length"),
                              tr("Interval length must be greater than zero!"));
        return;
    }

    bool byTime = typeTime->isChecked();

    QList<AddedInterval> results;
    if (methodBestPower->isChecked()) {
        if (peakPowerStandard->isChecked())
            findPeakPowerStandard(ride, results);
        else
            findBests(byTime, ride, (byTime?windowSizeSecs:windowSizeMeters), maxIntervals, results, "");
    }
    else
        findFirsts(byTime, ride, (byTime?windowSizeSecs:windowSizeMeters), maxIntervals, results);

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
    resultsTable->setColumnWidth(2,200);
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
    else if (windowSize > ride->dataPoints().last()->km*1000) return;

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

        QString name = "#%1 %2%3  (%4w)";
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
        name = name.arg(round(candidate.avg));
        candidate.name = name;

        results.append(candidate);
    }

}

void
AddIntervalDialog::findPeakPowerStandard(const RideFile *ride, QList<AddedInterval> &results)
{
    findBests(true, ride, 5, 1, results, "Peak 5s");
    findBests(true, ride, 10, 1, results, "Peak 10s");
    findBests(true, ride, 20, 1, results, "Peak 20s");
    findBests(true, ride, 30, 1, results, "Peak 30s");
    findBests(true, ride, 60, 1, results, "Peak 1min");
    findBests(true, ride, 120, 1, results, "Peak 2min");
    findBests(true, ride, 300, 1, results, "Peak 5min");
    findBests(true, ride, 600, 1, results, "Peak 10min");
    findBests(true, ride, 1200, 1, results, "Peak 20min");
    findBests(true, ride, 1800, 1, results, "Peak 30min");
    findBests(true, ride, 3600, 1, results, "Peak 60min");
}

void
AddIntervalDialog::findBests(bool typeTime, const RideFile *ride, double windowSize,
                              int maxIntervals, QList<AddedInterval> &results, QString prefix)
{
    QList<AddedInterval> bests;
    QList<AddedInterval> _results;

    double secsDelta = ride->recIntSecs();
    double totalWatts = 0.0;
    QList<const RideFilePoint*> window;

    // ride is shorter than the window size!
    if (typeTime && windowSize > ride->dataPoints().last()->secs + secsDelta) return;
    if (!typeTime && windowSize > ride->dataPoints().last()->km*1000) return;

    // We're looking for intervals with durations in [windowSizeSecs, windowSizeSecs + secsDelta).
    foreach (const RideFilePoint *point, ride->dataPoints()) {
        // Discard points until interval duration is < windowSizeSecs + secsDelta.
        while ((typeTime && !window.empty() && intervalDuration(window.first(), point, ride) >= windowSize + secsDelta) ||
               (!typeTime && window.length()>1 && intervalDistance(window.at(1), point, ride) >= windowSize)) {
            totalWatts -= window.first()->watts;
            window.takeFirst();
        }
        // Add points until interval duration or distance is >= windowSize.
        totalWatts += point->watts;
        window.append(point);
        double duration = intervalDuration(window.first(), window.last(), ride);
        double distance = intervalDistance(window.first(), window.last(), ride);

        if ((typeTime && duration >= windowSize) ||
            (!typeTime && distance >= windowSize)) {
            double start = window.first()->secs;
            double stop = window.last()->secs; //start + duration;
            double avg = totalWatts * secsDelta / duration;
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
            QString name = prefix;
            if (prefix == "") {
                name = "Best %2%3 #%1";
                name = name.arg(_results.count()+1);
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
            name += " (%4w)";
            name = name.arg(round(candidate.avg));
            candidate.name = name;
            name = "";
            _results.append(candidate);
        }
    }
    results.append(_results);
}

void
AddIntervalDialog::doneClicked()
{
    clearResultsTable(resultsTable); // clear out that table!
    done(0);
}

void
AddIntervalDialog::addClicked()
{
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
            const RideFile *ride = mainWindow->currentRide();

            QTreeWidgetItem *allIntervals = mainWindow->mutableIntervalItems();
            QTreeWidgetItem *last =
                new IntervalItem(ride, name, start, stop,
                                 ride->timeToDistance(start),
                                 ride->timeToDistance(stop),
                                 allIntervals->childCount()+1);
            last->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
            // add
            allIntervals->addChild(last);
        }
    }
    mainWindow->updateRideFileIntervals();
}
