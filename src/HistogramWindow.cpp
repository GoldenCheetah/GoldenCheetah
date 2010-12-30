/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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
#include "RideItem.h"
#include "Settings.h"
#include <QtGui>
#include <assert.h>

#include "Zones.h"
#include "HrZones.h"

HistogramWindow::HistogramWindow(MainWindow *mainWindow) :
    GcWindow(mainWindow), mainWindow(mainWindow)
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

    lnYHistCheckBox = new QCheckBox;
    lnYHistCheckBox->setText(tr("Log Y"));
    cl->addWidget(lnYHistCheckBox);

    withZerosCheckBox = new QCheckBox;
    withZerosCheckBox->setText(tr("With zeros"));
    cl->addWidget(withZerosCheckBox);

    histShadeZones = new QCheckBox;
    histShadeZones->setText(tr("Shade zones"));
    cl->addWidget(histShadeZones);

    histParameterCombo = new QComboBox();
    histParameterCombo->addItem(tr("Watts"));
    histParameterCombo->addItem(tr("Watts (by Zone)"));
    histParameterCombo->addItem(tr("Torque"));
    histParameterCombo->addItem(tr("Heartrate"));
    histParameterCombo->addItem(tr("Heartrate (by Zone)"));
    histParameterCombo->addItem(tr("Speed"));
    histParameterCombo->addItem(tr("Cadence"));
    histParameterCombo->setCurrentIndex(0);
    cl->addWidget(histParameterCombo);

    histSumY = new QComboBox();
    histSumY->addItem(tr("Absolute Time"));
    histSumY->addItem(tr("Percentage Time"));
    cl->addWidget(histSumY);
    cl->addStretch();

    // sort out default values
    setHistTextValidator();
    lnYHistCheckBox->setChecked(powerHist->islnY());
    withZerosCheckBox->setChecked(powerHist->withZeros());
    binWidthSlider->setValue(powerHist->binWidth());
    setHistBinWidthText();

    connect(binWidthSlider, SIGNAL(valueChanged(int)),
            this, SLOT(setBinWidthFromSlider()));
    connect(binWidthLineEdit, SIGNAL(editingFinished()),
            this, SLOT(setBinWidthFromLineEdit()));
    connect(lnYHistCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(setlnYHistFromCheckBox()));
    connect(withZerosCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(setWithZerosFromCheckBox()));
    connect(histParameterCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(setHistSelection(int)));
    connect(histShadeZones, SIGNAL(stateChanged(int)),
            this, SLOT(setHistSelection(int)));
    connect(histSumY, SIGNAL(currentIndexChanged(int)),
            this, SLOT(setSumY(int)));
    //connect(mainWindow, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(mainWindow, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    connect(mainWindow, SIGNAL(zonesChanged()), this, SLOT(zonesChanged()));
    connect(mainWindow, SIGNAL(configChanged()), powerHist, SLOT(configChanged()));
}

void
HistogramWindow::rideSelected()
{
    if (!amVisible())
        return;
    RideItem *ride = myRideItem;
    if (!ride)
        return;

    // get range that applies to this ride
    powerRange = mainWindow->zones()->whichRange(ride->dateTime.date());
    hrRange = mainWindow->hrZones()->whichRange(ride->dateTime.date());

    // set the histogram data
    powerHist->setData(ride);
}

void
HistogramWindow::intervalSelected()
{
    if (!amVisible())
        return;
    RideItem *ride = myRideItem;
    if (!ride) return;

    // set the histogram data
    powerHist->setData(ride);
}

void
HistogramWindow::zonesChanged()
{
    if (!amVisible())
        return;
    powerHist->refreshZoneLabels();
    powerHist->replot();
}

void
HistogramWindow::setBinWidthFromSlider()
{
    if (powerHist->binWidth() != binWidthSlider->value()) {
        powerHist->setBinWidth(binWidthSlider->value());
	setHistBinWidthText();
    }
}

void
HistogramWindow::setlnYHistFromCheckBox()
{
    if (powerHist->islnY() != lnYHistCheckBox->isChecked())
	powerHist->setlnY(! powerHist->islnY());
}

void
HistogramWindow::setSumY(int index)
{
    if (index < 0) return; // being destroyed
    else powerHist->setSumY(index == 0);
}

void
HistogramWindow::setWithZerosFromCheckBox()
{
    if (powerHist->withZeros() != withZerosCheckBox->isChecked()) {
        powerHist->setWithZeros(withZerosCheckBox->isChecked());
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
    binWidthLineEdit->setValidator(
	    (digits == 0) ?
	    (QValidator *) (
		new QIntValidator(binWidthSlider->minimum() * delta,
		    binWidthSlider->maximum() * delta,
		    binWidthLineEdit
		    )
		) :
	    (QValidator *) (
		new QDoubleValidator(binWidthSlider->minimum() * delta,
		    binWidthSlider->maximum() * delta,
		    digits,
		    binWidthLineEdit
		    )
		)
	    );

}

void
HistogramWindow::setHistSelection(int /*id*/)
{
    // Set shading first, since the dataseries selection
    // below will trigger a redraw, and we need to have
    // set the shading beforehand. OK, so we could make
    // either change trigger it, but this makes for simpler
    // code here and in powerhist.cpp
    if (histShadeZones->isChecked()) powerHist->setShading(true);
    else powerHist->setShading(false);

    // Which data series are we plotting?
    switch (histParameterCombo->currentIndex()) {
    default:
    case 0 :
	    powerHist->setSelection(PowerHist::watts);
        break;

    case 1 :
	    powerHist->setSelection(PowerHist::wattsZone);
        break;

    case 2 :
	    powerHist->setSelection(PowerHist::nm);
        break;

    case 3 :
	    powerHist->setSelection(PowerHist::hr);
        break;

    case 4 :
	    powerHist->setSelection(PowerHist::hrZone);
        break;
    case 5 :
	    powerHist->setSelection(PowerHist::kph);
        break;

    case 6 :
	    powerHist->setSelection(PowerHist::cad);
    }
    setHistBinWidthText();
    setHistTextValidator();
}

void
HistogramWindow::setBinWidthFromLineEdit()
{
    double value = binWidthLineEdit->text().toDouble();
    if (value != powerHist->binWidth()) {
	binWidthSlider->setValue(powerHist->setBinWidthRealUnits(value));
	setHistBinWidthText();
    }
}
