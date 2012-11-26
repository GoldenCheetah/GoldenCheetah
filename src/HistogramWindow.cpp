/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
 *               2011 Mark Liversedge (liversedge@gmail.com)
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

#include "HistogramWindow.h"
#include "MainWindow.h"
#include "PowerHist.h"
#include "RideFile.h"
#include "RideFileCache.h"
#include "RideItem.h"
#include "Settings.h"
#include <QtGui>
#include <assert.h>

#include "Zones.h"
#include "HrZones.h"

HistogramWindow::HistogramWindow(MainWindow *mainWindow, bool rangemode) : GcWindow(mainWindow), mainWindow(mainWindow), stale(true), source(NULL), rangemode(rangemode)
{
    setInstanceName("Histogram Window");

    QWidget *c = new QWidget;
    QVBoxLayout *cl = new QVBoxLayout(c);
    setControls(c);

    // plot
    QVBoxLayout *vlayout = new QVBoxLayout;
    powerHist = new PowerHist(mainWindow);
    vlayout->addWidget(powerHist);
    setLayout(vlayout);

#ifdef GC_HAVE_LUCENE
    // search filter box
    isFiltered = false;
    searchBox = new SearchFilterBox(this, mainWindow);
    connect(searchBox, SIGNAL(searchClear()), this, SLOT(clearFilter()));
    connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(setFilter(QStringList)));
    cl->addWidget(searchBox);
#endif

    // bin width
    QHBoxLayout *binWidthLayout = new QHBoxLayout;
    QLabel *binWidthLabel = new QLabel(tr("Bin width"), this);
    binWidthLineEdit = new QLineEdit(this);
    binWidthLineEdit->setFixedWidth(30);

    binWidthLayout->addWidget(binWidthLabel);
    binWidthLayout->addWidget(binWidthLineEdit);
    binWidthSlider = new QSlider(Qt::Horizontal);
    binWidthSlider->setTickPosition(QSlider::TicksBelow);
    binWidthSlider->setTickInterval(1);
    binWidthSlider->setMinimum(1);
    binWidthSlider->setMaximum(100);
    binWidthLayout->addWidget(binWidthSlider);
    cl->addLayout(binWidthLayout);

    showLnY = new QCheckBox;
    showLnY->setText(tr("Log Y"));
    cl->addWidget(showLnY);

    showZeroes = new QCheckBox;
    showZeroes->setText(tr("With zeros"));
    cl->addWidget(showZeroes);

    shadeZones = new QCheckBox;
    shadeZones->setText(tr("Shade zones"));
    shadeZones->setChecked(powerHist->shade);
    cl->addWidget(shadeZones);

    showInZones = new QCheckBox;
    showInZones->setText(tr("Show in zones"));
    cl->addWidget(showInZones);

    seriesCombo = new QComboBox();
    addSeries();
    cl->addWidget(seriesCombo);

    showSumY = new QComboBox();
    showSumY->addItem(tr("Absolute Time"));
    showSumY->addItem(tr("Percentage Time"));

    cl->addWidget(showSumY);
    cl->addStretch();

    // sort out default values
    setHistTextValidator();
    showLnY->setChecked(powerHist->islnY());
    showZeroes->setChecked(powerHist->withZeros());
    binWidthSlider->setValue(powerHist->binWidth());
    setHistBinWidthText();

    // set the defaults etc
    updateChart();

    // the bin slider/input update each other
    // only the input box triggers an update to the chart
    connect(binWidthSlider, SIGNAL(valueChanged(int)), this, SLOT(setBinWidthFromSlider()));
    connect(binWidthLineEdit, SIGNAL(editingFinished()), this, SLOT(setBinWidthFromLineEdit()));

    // when season changes we need to retrieve data from the cache then update the chart
    if (rangemode) connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
    else {
        connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
        connect(mainWindow, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    }

    // if any of the controls change we pass the chart everything
    connect(showLnY, SIGNAL(stateChanged(int)), this, SLOT(updateChart()));
    connect(showZeroes, SIGNAL(stateChanged(int)), this, SLOT(updateChart()));
    connect(seriesCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateChart()));
    connect(showInZones, SIGNAL(stateChanged(int)), this, SLOT(updateChart()));
    connect(shadeZones, SIGNAL(stateChanged(int)), this, SLOT(updateChart()));
    connect(showSumY, SIGNAL(currentIndexChanged(int)), this, SLOT(updateChart()));

    connect(mainWindow, SIGNAL(zonesChanged()), this, SLOT(zonesChanged()));
    connect(mainWindow, SIGNAL(configChanged()), powerHist, SLOT(configChanged()));

    connect(mainWindow, SIGNAL(rideAdded(RideItem*)), this, SLOT(rideAddorRemove(RideItem*)));
    connect(mainWindow, SIGNAL(rideDeleted(RideItem*)), this, SLOT(rideAddorRemove(RideItem*)));
}

void
HistogramWindow::rideSelected()
{
    if (!amVisible()) return;

    RideItem *ride = myRideItem;

    if (!ride || (rangemode && !stale)) return;

    if (rangemode) {
        // get range that applies to this ride
        powerRange = mainWindow->zones()->whichRange(ride->dateTime.date());
        hrRange = mainWindow->hrZones()->whichRange(ride->dateTime.date());
    }

    // update
    updateChart();
}

void
HistogramWindow::rideAddorRemove(RideItem *)
{
    if (rangemode) {
        stale = true;
        updateChart();
    }
}

void
HistogramWindow::intervalSelected()
{
    if (!amVisible()) return;

    RideItem *ride = myRideItem;

    // null? or not plotting current ride, ignore signal
    if (!ride || rangemode) return;

    // update
    interval = true;
    updateChart();
}

void
HistogramWindow::zonesChanged()
{
    if (!amVisible()) return;

    powerHist->refreshZoneLabels();
    powerHist->replot();
}

void HistogramWindow::dateRangeChanged(DateRange)
{
    if (!amVisible()) return;

    stale = true;
    updateChart();
}

void HistogramWindow::addSeries()
{
    // setup series list
    seriesList << RideFile::watts
               << RideFile::wattsKg
               << RideFile::hr
               << RideFile::kph
               << RideFile::cad
               << RideFile::nm;

    foreach (RideFile::SeriesType x, seriesList) 
        seriesCombo->addItem(RideFile::seriesName(x), static_cast<int>(x));
}

void
HistogramWindow::setBinWidthFromSlider()
{
    if (powerHist->binWidth() != binWidthSlider->value()) {
        //RideFile::SeriesType series = static_cast<RideFile::SeriesType>(seriesCombo->itemData(seriesCombo->currentIndex()).toInt());
        powerHist->setBinWidth(binWidthSlider->value());
        setHistBinWidthText();

        updateChart();
    }
}

void
HistogramWindow::setHistBinWidthText()
{
    binWidthLineEdit->setText(QString("%1").arg(powerHist->getBinWidthRealUnits(), 0, 'g', 3));
}

void
HistogramWindow::setHistTextValidator()
{
    double delta = powerHist->getDelta();
    int digits = powerHist->getDigits();

    QValidator *validator;
    if (digits == 0) {

        validator = new QIntValidator(binWidthSlider->minimum() * delta,
                                      binWidthSlider->maximum() * delta,
                                      binWidthLineEdit);
    } else {

        validator = new QDoubleValidator(binWidthSlider->minimum() * delta,
                                         binWidthSlider->maximum() * delta,
                                         digits,
                                         binWidthLineEdit);
    }
    binWidthLineEdit->setValidator(validator);
}

void
HistogramWindow::setBinWidthFromLineEdit()
{
    double value = binWidthLineEdit->text().toDouble();
    if (value != powerHist->binWidth()) {
        binWidthSlider->setValue(powerHist->setBinWidthRealUnits(value));
        setHistBinWidthText();

        updateChart();
    }
}

void
HistogramWindow::updateChart()
{
    // refresh the ridefile cache if it is stale
    if (stale) {

        RideFileCache *old = source;

        if (rangemode) {

#ifdef GC_HAVE_LUCENE
            source = new RideFileCache(mainWindow, myDateRange.from, myDateRange.to, isFiltered, files);
#else
            source = new RideFileCache(mainWindow, myDateRange.from, myDateRange.to);
#endif

            stale = false;

            if (old) delete old; // guarantee source pointer changes
        }
        stale = false; // well we tried
    }

    // set data
    if (rangemode && source) {
        powerHist->setData(source);
    } else 
        powerHist->setData(myRideItem, interval); // intervals selected forces data to
                                                  // be recomputed since interval selection
                                                  // has changed.

    // and now the controls
    powerHist->setShading(shadeZones->isChecked() ? true : false);
    powerHist->setZoned(showInZones->isChecked() ? true : false);
    powerHist->setlnY(showLnY->isChecked() ? true : false);
    powerHist->setWithZeros(showZeroes->isChecked() ? true : false);
    powerHist->setSumY(showSumY->currentIndex()== 0 ? true : false);
    powerHist->setBinWidth(binWidthLineEdit->text().toDouble());

    // and which series to plot
    powerHist->setSeries(static_cast<RideFile::SeriesType>(seriesCombo->itemData(seriesCombo->currentIndex()).toInt()));

    // now go plot yourself
    //powerHist->setAxisTitle(int axis, QString label);
    powerHist->recalc(interval); // interval changed? force recalc
    powerHist->replot();

    interval = false;// we force a recalc whem called coz intervals
                     // have been selected. The recalc routine in
                     // powerhist optimises out, but doesn't keep track
                     // of interval selection -- simplifies the setters
                     // and getters, so worth this 'hack'.
}

#ifdef GC_HAVE_LUCENE
void 
HistogramWindow::clearFilter()
{
    isFiltered = false;
    files.clear();
    stale = true;
    updateChart();
}

void
HistogramWindow::setFilter(QStringList list)
{
    isFiltered = true;
    files = list;
    stale = true;
    updateChart();
}
#endif
