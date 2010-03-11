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
#include <QtGui>
#include <assert.h>

HistogramWindow::HistogramWindow(MainWindow *mainWindow) :
    QWidget(mainWindow), mainWindow(mainWindow)
{
    QVBoxLayout *vlayout = new QVBoxLayout;
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

    lnYHistCheckBox = new QCheckBox;
    lnYHistCheckBox->setText(tr("Log Y"));
    binWidthLayout->addWidget(lnYHistCheckBox);

    withZerosCheckBox = new QCheckBox;
    withZerosCheckBox->setText(tr("With zeros"));
    binWidthLayout->addWidget(withZerosCheckBox);
    histParameterCombo = new QComboBox();
    binWidthLayout->addWidget(histParameterCombo);

    powerHist = new PowerHist(mainWindow);
    setHistTextValidator();

    lnYHistCheckBox->setChecked(powerHist->islnY());
    withZerosCheckBox->setChecked(powerHist->withZeros());
    binWidthSlider->setValue(powerHist->binWidth());
    setHistBinWidthText();
    vlayout->addWidget(powerHist);
    vlayout->addLayout(binWidthLayout);
    setLayout(vlayout);

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
    connect(mainWindow, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
    connect(mainWindow, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    connect(mainWindow, SIGNAL(zonesChanged()), this, SLOT(zonesChanged()));
}

void
HistogramWindow::rideSelected()
{
    if (mainWindow->activeTab() != this)
        return;
    RideItem *ride = mainWindow->rideItem();
    if (!ride)
        return;
    // set the histogram data
    powerHist->setData(ride);
    // make sure the histogram has a legal selection
    powerHist->fixSelection();
    // update the options in the histogram combobox
    setHistWidgets(ride);
}

void
HistogramWindow::intervalSelected()
{
    if (mainWindow->activeTab() != this)
        return;
    RideItem *ride = mainWindow->rideItem();
    if (!ride) return;

    // set the histogram data
    powerHist->setData(ride);
}

void
HistogramWindow::zonesChanged()
{
    if (mainWindow->activeTab() != this)
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
HistogramWindow::setHistSelection(int id)
{
    if (id == histWattsShadedID)
	powerHist->setSelection(PowerHist::wattsShaded);
    else if (id == histWattsUnshadedID)
	powerHist->setSelection(PowerHist::wattsUnshaded);
    else if (id == histNmID)
	powerHist->setSelection(PowerHist::nm);
    else if (id == histHrID)
	powerHist->setSelection(PowerHist::hr);
    else if (id == histKphID)
	powerHist->setSelection(PowerHist::kph);
    else if (id == histCadID)
	powerHist->setSelection(PowerHist::cad);
    else
	fprintf(stderr, "Illegal id encountered: %d", id);

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

void
HistogramWindow::setHistWidgets(RideItem *rideItem)
{
    int count = 0;
    assert(rideItem);
    RideFile *ride = rideItem->ride();

    // prevent selection from changing during reconstruction of options
    disconnect(histParameterCombo, SIGNAL(currentIndexChanged(int)),
	       this, SLOT(setHistSelection(int)));

    if (ride) {
	// we want to retain the present selection
	PowerHist::Selection s = powerHist->selection();

	histParameterCombo->clear();

	histWattsShadedID =
	    histWattsUnshadedID =
	    histNmID =
	    histHrID =
	    histKphID =
	    histCadID =
	    -1;

	if (ride->areDataPresent()->watts) {
	    histWattsShadedID = count ++;
	    histParameterCombo->addItem(tr("Watts(shaded)"));
	    histWattsUnshadedID = count ++;
	    histParameterCombo->addItem(tr("Watts(unshaded)"));
	}
	if (ride->areDataPresent()->nm) {
	    histNmID = count ++;
	    histParameterCombo->addItem(tr("Torque"));
	}
	if (ride->areDataPresent()->hr) {
	    histHrID = count ++;
	    histParameterCombo->addItem(tr("Heartrate"));
	}
	if (ride->areDataPresent()->kph) {
	    histKphID = count ++;
	    histParameterCombo->addItem(tr("Speed"));
	}
	if (ride->areDataPresent()->cad) {
	    histCadID = count ++;
	    histParameterCombo->addItem(tr("Cadence"));
	}
	if (count > 0) {
	    histParameterCombo->setEnabled(true);
	    binWidthLineEdit->setEnabled(true);
	    binWidthSlider->setEnabled(true);
	    withZerosCheckBox->setEnabled(true);
	    lnYHistCheckBox->setEnabled(true);

	    // set widget to proper value
	    if ((s == PowerHist::wattsShaded) && (histWattsShadedID >= 0))
		histParameterCombo->setCurrentIndex(histWattsShadedID);
	    else if ((s == PowerHist::wattsUnshaded) && (histWattsUnshadedID >= 0))
		histParameterCombo->setCurrentIndex(histWattsUnshadedID);
	    else if ((s == PowerHist::nm) && (histNmID >= 0))
		histParameterCombo->setCurrentIndex(histNmID);
	    else if ((s == PowerHist::hr) && (histHrID >= 0))
		histParameterCombo->setCurrentIndex(histHrID);
	    else if ((s == PowerHist::kph) && (histKphID >= 0))
		histParameterCombo->setCurrentIndex(histKphID);
	    else if ((s == PowerHist::cad) && (histCadID >= 0))
		histParameterCombo->setCurrentIndex(histCadID);
	    else
		histParameterCombo->setCurrentIndex(0);

	    // reconnect widget
	    connect(histParameterCombo, SIGNAL(currentIndexChanged(int)),
		    this, SLOT(setHistSelection(int)));

	    return;
	}
    }

    histParameterCombo->addItem(tr("no data"));
    histParameterCombo->setEnabled(false);
    binWidthLineEdit->setEnabled(false);
    binWidthSlider->setEnabled(false);
    withZerosCheckBox->setEnabled(false);
    lnYHistCheckBox->setEnabled(false);
}

