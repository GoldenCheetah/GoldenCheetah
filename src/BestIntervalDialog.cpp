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

#include "BestIntervalDialog.h"
#include "Athlete.h"
#include "Context.h"
#include "IntervalItem.h"
#include "RideFile.h"
#include "RideItem.h"
#include <QMap>
#include <math.h>

BestIntervalDialog::BestIntervalDialog(Context *context) :
    context(context)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Find Intervals");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *intervalLengthLayout = new QHBoxLayout;
    QLabel *intervalLengthLabel = new QLabel(tr("Interval length: "), this);
    intervalLengthLayout->addWidget(intervalLengthLabel);
    hrsSpinBox = new QDoubleSpinBox(this);
    hrsSpinBox->setDecimals(0);
    hrsSpinBox->setMinimum(0.0);
    hrsSpinBox->setSuffix(" hrs");
    hrsSpinBox->setSingleStep(1.0);
    hrsSpinBox->setAlignment(Qt::AlignRight);
    intervalLengthLayout->addWidget(hrsSpinBox);
    minsSpinBox = new QDoubleSpinBox(this);
    minsSpinBox->setDecimals(0);
    minsSpinBox->setRange(0.0, 59.0);
    minsSpinBox->setSuffix(" mins");
    minsSpinBox->setSingleStep(1.0);
    minsSpinBox->setWrapping(true);
    minsSpinBox->setAlignment(Qt::AlignRight);
    minsSpinBox->setValue(1.0);
    intervalLengthLayout->addWidget(minsSpinBox);
    secsSpinBox = new QDoubleSpinBox(this);
    secsSpinBox->setDecimals(0);
    secsSpinBox->setRange(0.0, 59.0);
    secsSpinBox->setSuffix(" secs");
    secsSpinBox->setSingleStep(1.0);
    secsSpinBox->setWrapping(true);
    secsSpinBox->setAlignment(Qt::AlignRight);
    intervalLengthLayout->addWidget(secsSpinBox);
    mainLayout->addLayout(intervalLengthLayout);

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
    mainLayout->addLayout(intervalCountLayout);

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
    findButton = new QPushButton(tr("&Find Intervals"), this);
    buttonLayout->addWidget(findButton);
    doneButton = new QPushButton(tr("&Done"), this);
    buttonLayout->addWidget(doneButton);
    addButton = new QPushButton(tr("&Add to Intervals"));
    buttonLayout->addWidget(addButton);
    mainLayout->addLayout(buttonLayout);

    connect(findButton, SIGNAL(clicked()), this, SLOT(findClicked()));
    connect(doneButton, SIGNAL(clicked()), this, SLOT(doneClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
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

static bool
intervalsOverlap(const BestIntervalDialog::BestInterval &a,
                 const BestIntervalDialog::BestInterval &b)
{
    if ((a.start <= b.start) && (a.stop > b.start))
        return true;
    if ((b.start <= a.start) && (b.stop > a.start))
        return true;
    return false;
}

struct CompareBests {
    // Sort by decreasing power and increasing start time.
    bool operator()(const BestIntervalDialog::BestInterval &a,
                    const BestIntervalDialog::BestInterval &b) const {
        if (a.avg > b.avg)
            return true;
        if (b.avg > a.avg)
            return false;
        return a.start < b.start;
    }
};

void
BestIntervalDialog::findClicked()
{
    const RideFile *ride = context->ride ? context->ride->ride() : NULL;
    if (!ride) {
        QMessageBox::critical(this, tr("Select Ride"), tr("No ride selected!"));
        return;
    }

    int maxIntervals = (int) countSpinBox->value();
    double windowSizeSecs = (hrsSpinBox->value() * 3600.0
                             + minsSpinBox->value() * 60.0
                             + secsSpinBox->value());

    if (windowSizeSecs == 0.0) {
        QMessageBox::critical(this, tr("Bad Interval Length"),
                              tr("Interval length must be greater than zero!"));
        return;
    }

    QList<BestInterval> results;
    findBests(ride, windowSizeSecs, maxIntervals, results);

    // clear the table
    clearResultsTable(resultsTable);

    // populate the table
    resultsTable->setRowCount(results.size());
    int row = 0;
    foreach (const BestInterval &interval, results) {

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

        // name
        int x = windowSizeSecs; // int is more help here
        QString name = "Best %2%3 #%1 (%4w)";
        name = name.arg(row + 1);
        // best n mins
        if (x < 60) {
            // whole seconds
            name = name.arg(x);
            name = name.arg("sec");
        } else if (x >= 60 && !(x%60)) {
            // whole minutes
            name = name.arg(x/60);
            name = name.arg("min");
        } else {
            double secs = x;
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
        name = name.arg(round(interval.avg));

        QTableWidgetItem *n = new QTableWidgetItem;
        n->setText(name);
        n->setFlags(n->flags() | (Qt::ItemIsEditable));
        resultsTable->setItem(row, 2, n);

        // hidden columns - start, stop
        QString strt = QString("%1").arg(interval.start); // can't use secs as it gets modified
        QTableWidgetItem *st = new QTableWidgetItem;
        st->setText(strt);
        resultsTable->setItem(row, 3, st);

        QString stp = QString("%1").arg(interval.start + x);
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
BestIntervalDialog::findBests(const RideFile *ride, double windowSizeSecs,
                              int maxIntervals, QList<BestInterval> &results)
{
    QList<BestInterval> bests;

    double secsDelta = ride->recIntSecs();
    double totalWatts = 0.0;
    QList<const RideFilePoint*> window;

    // ride is shorter than the window size!
    if (windowSizeSecs > ride->dataPoints().last()->secs + secsDelta) return;

    // We're looking for intervals with durations in [windowSizeSecs, windowSizeSecs + secsDelta).
    foreach (const RideFilePoint *point, ride->dataPoints()) {
        // Discard points until interval duration is < windowSizeSecs + secsDelta.
        while (!window.empty() && (intervalDuration(window.first(), point, ride) >= windowSizeSecs + secsDelta)) {
            totalWatts -= window.first()->watts;
            window.takeFirst();
        }
        // Add points until interval duration is >= windowSizeSecs.
        totalWatts += point->watts;
        window.append(point);
        double duration = intervalDuration(window.first(), window.last(), ride);
        if (duration >= windowSizeSecs) {
            double start = window.first()->secs;
            double stop = start + duration;
            double avg = totalWatts * secsDelta / duration;
            bests.append(BestInterval(start, stop, avg));
        }
    }

    std::sort(bests.begin(), bests.end(), CompareBests());

    while (!bests.empty() && (results.size() < maxIntervals)) {
        BestInterval candidate = bests.takeFirst();
        bool overlaps = false;
        foreach (const BestInterval &existing, results) {
            if (intervalsOverlap(candidate, existing)) {
                overlaps = true;
                break;
            }
        }
        if (!overlaps)
            results.append(candidate);
    }

}

void
BestIntervalDialog::doneClicked()
{
    clearResultsTable(resultsTable); // clear out that table!
    done(0);
}

void
BestIntervalDialog::addClicked()
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
            const RideFile *ride = context->ride ? context->ride->ride() : NULL;

            QTreeWidgetItem *allIntervals = context->athlete->mutableIntervalItems();
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
    context->athlete->updateRideFileIntervals();
}
