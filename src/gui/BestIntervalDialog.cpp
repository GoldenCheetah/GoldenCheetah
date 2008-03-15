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
#include "MainWindow.h"
#include "RideFile.h"
#include <QMap>
#include <assert.h>
#include <math.h>

BestIntervalDialog::BestIntervalDialog(MainWindow *mainWindow) : 
    mainWindow(mainWindow)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Find Best Intervals");
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
    minsSpinBox->setRange(0.0, 59.0);
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
    countSpinBox->setSingleStep(1.0);
    countSpinBox->setAlignment(Qt::AlignRight);
    intervalCountLayout->addWidget(countSpinBox);
    mainLayout->addLayout(intervalCountLayout);

    QLabel *resultsLabel = new QLabel(tr("Results:"), this);
    mainLayout->addWidget(resultsLabel);

    resultsText = new QTextEdit(this);
    resultsText->setReadOnly(true);
    mainLayout->addWidget(resultsText);
 
    QHBoxLayout *buttonLayout = new QHBoxLayout; 
    findButton = new QPushButton(tr("&Find Intervals"), this);
    buttonLayout->addWidget(findButton);
    doneButton = new QPushButton(tr("&Done"), this);
    buttonLayout->addWidget(doneButton);
    mainLayout->addLayout(buttonLayout);

    connect(findButton, SIGNAL(clicked()), this, SLOT(findClicked()));
    connect(doneButton, SIGNAL(clicked()), this, SLOT(doneClicked()));
}

void 
BestIntervalDialog::findClicked()
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

    if (windowSizeSecs == 0.0) {
        QMessageBox::critical(this, tr("Bad Interval Length"), 
                              tr("Interval length must be greater than zero!"));
        return;
    }

    QList<const RideFilePoint*> window;
    QMap<double,double> bests;

    double secsDelta = ride->recIntSecs();
    int expectedSamples = (int) floor(windowSizeSecs / secsDelta);
    double totalWatts = 0.0;

    QListIterator<RideFilePoint*> i(ride->dataPoints());
    while (i.hasNext()) {
        const RideFilePoint *point = i.next();
        while (!window.empty()
               && (point->secs >= window.first()->secs + windowSizeSecs)) {
            totalWatts -= window.first()->watts;
            window.takeFirst();
        }
        totalWatts += point->watts;
        window.append(point);
        int divisor = std::max(window.size(), expectedSamples);
        double avg = totalWatts / divisor;
        bests.insertMulti(avg, point->secs);
    }

    QMap<double,double> results;
    while (!bests.empty() && maxIntervals--) {
        QMutableMapIterator<double,double> j(bests);
        j.toBack();
        j.previous();
        double secs = j.value();
        results.insert(j.value() - windowSizeSecs, j.key());
        j.remove();
        while (j.hasPrevious()) {
            j.previous();
            if (abs(secs - j.value()) < windowSizeSecs)
                j.remove();
        }
    }

    QString resultsHtml = 
        "<center>"
        "<table width=\"80%\">"
        "<tr><td align=\"center\">Start Time</td>"
        "    <td align=\"center\">Average Power</td></tr>";
    QMapIterator<double,double> j(results);
    while (j.hasNext()) {
        j.next();
        double secs = j.key();
        double mins = ((int) secs) / 60;
        secs = secs - mins * 60.0;
        double hrs = ((int) mins) / 60;
        mins = mins - hrs * 60.0;
        QString row = 
            "<tr><td align=\"center\">%1:%2:%3</td>"
            "    <td align=\"center\">%4</td></tr>";
        row = row.arg(hrs, 0, 'f', 0);
        row = row.arg(mins, 2, 'f', 0, QLatin1Char('0'));
        row = row.arg(secs, 2, 'f', 0, QLatin1Char('0'));
        row = row.arg(j.value(), 0, 'f', 0, QLatin1Char('0'));
        resultsHtml += row;
    }
    resultsHtml += "</table></center>";
    resultsText->setHtml(resultsHtml);
}

void 
BestIntervalDialog::doneClicked()
{
    done(0);
}

