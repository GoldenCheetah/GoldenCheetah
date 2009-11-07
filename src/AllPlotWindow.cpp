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


#include "AllPlotWindow.h"
#include "AllPlot.h"
#include "RideFile.h"
#include "RideItem.h"
#include <qwt_plot_panner.h>
#include <qwt_plot_zoomer.h>

AllPlotWindow::AllPlotWindow(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *vlayout = new QVBoxLayout;

    QHBoxLayout *showLayout = new QHBoxLayout;
    QLabel *showLabel = new QLabel("Show:", this);
    showLayout->addWidget(showLabel);

    QCheckBox *showGrid = new QCheckBox("Grid", this);
    showGrid->setCheckState(Qt::Checked);
    showLayout->addWidget(showGrid);

    showHr = new QCheckBox("Heart Rate", this);
    showHr->setCheckState(Qt::Checked);
    showLayout->addWidget(showHr);

    showSpeed = new QCheckBox("Speed", this);
    showSpeed->setCheckState(Qt::Checked);
    showLayout->addWidget(showSpeed);

    showCad = new QCheckBox("Cadence", this);
    showCad->setCheckState(Qt::Checked);
    showLayout->addWidget(showCad);

    showAlt = new QCheckBox("Altitude", this);
    showAlt->setCheckState(Qt::Checked);
    showLayout->addWidget(showAlt);

    showPower = new QComboBox();
    showPower->addItem(tr("Power + shade"));
    showPower->addItem(tr("Power - shade"));
    showPower->addItem(tr("No Power"));
    showLayout->addWidget(showPower);

    QHBoxLayout *smoothLayout = new QHBoxLayout;
    QComboBox *comboDistance = new QComboBox();
    comboDistance->addItem(tr("X Axis Shows Time"));
    comboDistance->addItem(tr("X Axis Shows Distance"));
    smoothLayout->addWidget(comboDistance);

    QLabel *smoothLabel = new QLabel(tr("Smoothing (secs)"), this);
    smoothLineEdit = new QLineEdit(this);
    smoothLineEdit->setFixedWidth(40);

    smoothLayout->addWidget(smoothLabel);
    smoothLayout->addWidget(smoothLineEdit);
    smoothSlider = new QSlider(Qt::Horizontal);
    smoothSlider->setTickPosition(QSlider::TicksBelow);
    smoothSlider->setTickInterval(10);
    smoothSlider->setMinimum(2);
    smoothSlider->setMaximum(600);
    smoothLineEdit->setValidator(new QIntValidator(smoothSlider->minimum(),
                                                   smoothSlider->maximum(),
                                                   smoothLineEdit));
    smoothLayout->addWidget(smoothSlider);
    allPlot = new AllPlot(this);
    smoothSlider->setValue(allPlot->smoothing());
    smoothLineEdit->setText(QString("%1").arg(allPlot->smoothing()));

    allZoomer = new QwtPlotZoomer(allPlot->canvas());
    allZoomer->setRubberBand(QwtPicker::RectRubberBand);
    allZoomer->setRubberBandPen(QColor(Qt::black));
    allZoomer->setSelectionFlags(QwtPicker::DragSelection
                                 | QwtPicker::CornerToCorner);
    allZoomer->setTrackerMode(QwtPicker::AlwaysOff);
    allZoomer->setEnabled(true);

    // TODO: Hack for OS X one-button mouse
    // allZoomer->initMousePattern(1);

    // RightButton: zoom out by 1
    // Ctrl+RightButton: zoom out to full size
    allZoomer->setMousePattern(QwtEventPattern::MouseSelect2,
                               Qt::RightButton, Qt::ControlModifier);
    allZoomer->setMousePattern(QwtEventPattern::MouseSelect3,
                               Qt::RightButton);

    allPanner = new QwtPlotPanner(allPlot->canvas());
    allPanner->setMouseButton(Qt::MidButton);

    // TODO: zoomer doesn't interact well with automatic axis resizing

    vlayout->addWidget(allPlot);
    vlayout->addLayout(showLayout);
    vlayout->addLayout(smoothLayout);
    setLayout(vlayout);

    connect(showPower, SIGNAL(currentIndexChanged(int)),
            allPlot, SLOT(showPower(int)));
    connect(showHr, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showHr(int)));
    connect(showSpeed, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showSpeed(int)));
    connect(showCad, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showCad(int)));
    connect(showAlt, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showAlt(int)));
    connect(showGrid, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showGrid(int)));
    connect(comboDistance, SIGNAL(currentIndexChanged(int)),
            allPlot, SLOT(setByDistance(int)));
    connect(smoothSlider, SIGNAL(valueChanged(int)),
            this, SLOT(setSmoothingFromSlider()));
    connect(smoothLineEdit, SIGNAL(editingFinished()),
            this, SLOT(setSmoothingFromLineEdit()));
}

void
AllPlotWindow::setData(RideItem *ride)
{
    setAllPlotWidgets(ride);
    allPlot->setData(ride);
    allZoomer->setZoomBase();
}

void
AllPlotWindow::zonesChanged()
{
    allPlot->refreshZoneLabels();
    allPlot->replot();
}

void
AllPlotWindow::setSmoothingFromSlider()
{
    if (allPlot->smoothing() != smoothSlider->value()) {
        allPlot->setSmoothing(smoothSlider->value());
        smoothLineEdit->setText(QString("%1").arg(allPlot->smoothing()));
    }
}

void
AllPlotWindow::setSmoothingFromLineEdit()
{
    int value = smoothLineEdit->text().toInt();
    if (value != allPlot->smoothing()) {
        allPlot->setSmoothing(value);
        smoothSlider->setValue(value);
    }
}

void
AllPlotWindow::setAllPlotWidgets(RideItem *ride)
{
    if (ride->ride) {
	const RideFileDataPresent *dataPresent = ride->ride->areDataPresent();
	showPower->setEnabled(dataPresent->watts);
	showHr->setEnabled(dataPresent->hr);
	showSpeed->setEnabled(dataPresent->kph);
	showCad->setEnabled(dataPresent->cad);
	showAlt->setEnabled(dataPresent->alt);
    }
    else {
	showPower->setEnabled(false);
	showHr->setEnabled(false);
	showSpeed->setEnabled(false);
	showCad->setEnabled(false);
	showAlt->setEnabled(false);
    }
}

