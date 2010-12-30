/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "ScatterWindow.h"
#include "ScatterPlot.h"
#include "MainWindow.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "math.h"
#include "Units.h" // for MILES_PER_KM

#include <QtGui>
#include <QString>

void
ScatterWindow::addStandardChannels(QComboBox *box)
{
    box->addItem(tr("Power"), MODEL_POWER);
    box->addItem(tr("Cadence"), MODEL_CADENCE);
    box->addItem(tr("Heartrate"), MODEL_HEARTRATE);
    box->addItem(tr("Speed"), MODEL_SPEED);
    box->addItem(tr("Altitude"), MODEL_ALT);
    box->addItem(tr("Torque"), MODEL_TORQUE);
    box->addItem(tr("AEPF"), MODEL_AEPF);
    box->addItem(tr("CPV"), MODEL_CPV);
    box->addItem(tr("Time"), MODEL_TIME);
    box->addItem(tr("Distance"), MODEL_DISTANCE);
    //box->addItem(tr("Interval"), MODEL_INTERVAL); //XXX supported differently for now
    //box->addItem(tr("Latitude"), MODEL_LAT); //XXX weird values make the plot ugly
    //box->addItem(tr("Longitude"), MODEL_LONG); //XXX weird values make the plot ugly
}

ScatterWindow::ScatterWindow(MainWindow *parent, const QDir &home) :
    GcWindow(parent), home(home), main(parent), ride(NULL), current(NULL)
{
    setInstanceName("2D Window");

    QWidget *c = new QWidget;
    QFormLayout *cl = new QFormLayout(c);
    setControls(c);

    // the plot widget
    scatterPlot= new ScatterPlot(main);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(scatterPlot);
    setLayout(mainLayout);

    // labels
    xLabel = new QLabel(tr("X-Axis:"), this);
    xSelector = new QComboBox;
    addStandardChannels(xSelector);
    xSelector->setCurrentIndex(0); // power
    cl->addRow(xLabel, xSelector);

    yLabel = new QLabel(tr("Y-Axis:"), this);
    ySelector = new QComboBox;
    addStandardChannels(ySelector);
    ySelector->setCurrentIndex(2); // heartrate
    cl->addRow(yLabel, ySelector);

    timeLabel = new QLabel(tr("00:00:00"), this);
    timeSlider = new QSlider(Qt::Horizontal);
    timeSlider->setTickPosition(QSlider::TicksBelow);
    timeSlider->setTickInterval(1);
    timeSlider->setMinimum(0);
    timeSlider->setTickInterval(5);
    timeSlider->setMaximum(100);
    timeSlider->setValue(0);
    timeSlider->setTracking(true);
    cl->addRow(timeLabel, timeSlider);

    // selectors
    ignore = new QCheckBox(tr("Ignore Zero"));
    ignore->setChecked(true);
    cl->addRow(ignore);

    grid = new QCheckBox(tr("Show Grid"));
    grid->setChecked(true);
    cl->addRow(grid);

    frame = new QCheckBox(tr("Frame Intervals"));
    frame->setChecked(true);
    cl->addRow(frame);

    // now connect up the widgets
    //connect(main, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(main, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    connect(main, SIGNAL(intervalsChanged()), this, SLOT(intervalSelected()));
    connect(xSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(setData()));
    connect(ySelector, SIGNAL(currentIndexChanged(int)), this, SLOT(setData()));
    connect(timeSlider, SIGNAL(valueChanged(int)), this, SLOT(setTime(int)));
    connect(grid, SIGNAL(stateChanged(int)), this, SLOT(setGrid()));
    //connect(legend, SIGNAL(stateChanged(int)), this, SLOT(setLegend()));
    connect(frame, SIGNAL(stateChanged(int)), this, SLOT(setFrame()));
    connect(ignore, SIGNAL(stateChanged(int)), this, SLOT(setIgnore()));
}

void
ScatterWindow::rideSelected()
{
    if (!amVisible())
        return;

    ride = myRideItem;

    if (!ride || !ride->ride() || ride == current) return;

    current = ride;

    timeSlider->setMinimum(0);

    if (ride->ride()->dataPoints().count()) timeSlider->setMaximum(ride->ride()->dataPoints().last()->secs);
    else timeSlider->setMaximum(100);

    timeSlider->setTickInterval(timeSlider->maximum()/20);
    setData();
}

void
ScatterWindow::setGrid()
{
    settings.gridlines = grid->isChecked();
    setData();
}

void
ScatterWindow::setFrame()
{
    settings.frame = frame->isChecked();
    setData();
}

void
ScatterWindow::setIgnore()
{
    settings.ignore = ignore->isChecked();
    setData();
}

void
ScatterWindow::intervalSelected()
{
    if (!amVisible())
        return;
    setData();
}

void
ScatterWindow::setTime(int value)
{
   QChar zero = QLatin1Char ( '0' );
   QString time = QString("%1:%2:%3").arg(value/3600,2,10,zero)
                                          .arg(value%3600/60,2,10,zero)
                                          .arg(value%60,2,10,zero);
    timeLabel->setText(time);
    scatterPlot->showTime(&settings, value, 30);
}

void
ScatterWindow::setData()
{
    settings.ride = ride;
    settings.x = xSelector->itemData(xSelector->currentIndex()).toInt();
    settings.y = ySelector->itemData(ySelector->currentIndex()).toInt();
    settings.ignore = ignore->isChecked();
    settings.gridlines = grid->isChecked();
    settings.frame = frame->isChecked();

    // any intervals to plot?
    settings.intervals.clear();
    for (int i=0; i<main->allIntervalItems()->childCount(); i++) {
        IntervalItem *current = dynamic_cast<IntervalItem *>(main->allIntervalItems()->child(i));
        if (current != NULL && current->isSelected() == true)
                settings.intervals.append(current);
    }
    scatterPlot->setData(&settings);
}
