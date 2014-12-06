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
#include "Athlete.h"
#include "Context.h"
#include "Colors.h"

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
    box->addItem(tr("Headwind"), MODEL_HEADWIND);
    box->addItem(tr("Slope"), MODEL_SLOPE);
    box->addItem(tr("Temperature"), MODEL_TEMP);
    box->addItem(tr("L/R Balance"), MODEL_LRBALANCE);
    box->addItem(tr("L/R Torque Effectiveness"), MODEL_TE);
    box->addItem(tr("L/R Pedal Smoothness"), MODEL_TE);
    box->addItem(tr("Running Vertical Oscillation"), MODEL_RV);
    box->addItem(tr("Running Cadence"), MODEL_RCAD);
    box->addItem(tr("Running GCT"), MODEL_RGCT);
    box->addItem(tr("Gear Ratio"), MODEL_GEAR);
    box->addItem(tr("Muscle Oxygen"), MODEL_SMO2);
    box->addItem(tr("Haemoglobin Mass"), MODEL_THB);
    //box->addItem(tr("Interval"), MODEL_INTERVAL); //supported differently for now
    //box->addItem(tr("Latitude"), MODEL_LAT); //weird values make the plot ugly
    //box->addItem(tr("Longitude"), MODEL_LONG); //weird values make the plot ugly
}

void
ScatterWindow::addrStandardChannels(QxtStringSpinBox *box)
{
    QStringList list;
    list.append(tr("Power"));
    list.append(tr("Cadence"));
    list.append(tr("Heartrate"));
    list.append(tr("Speed"));
    list.append(tr("Altitude"));
    list.append(tr("Torque"));
    list.append(tr("AEPF"));
    list.append(tr("CPV"));
    list.append(tr("Time"));
    list.append(tr("Distance"));
    list.append(tr("Headwind"));
    list.append(tr("Slope"));
    list.append(tr("Temperature"));
    list.append(tr("L/R Balance"));
    list.append(tr("L/R Torque Effectiveness"));
    list.append(tr("L/R Pedal Smoothness"));
    list.append(tr("Running Vertical Oscillation"));
    list.append(tr("Running Cadence"));
    list.append(tr("Running GCT"));
    list.append(tr("Gear Ratio"));
    list.append(tr("Muscle Oxygen"));
    list.append(tr("Haemoglobin Mass"));
    box->setStrings(list);
}

ScatterWindow::ScatterWindow(Context *context) :
    GcChartWindow(context), context(context), ride(NULL), current(NULL)
{
    //
    // reveal controls widget
    //

    // reveal controls
    QLabel *rxLabel = new QLabel(tr("X-Axis:"), this);
    rxSelector = new QxtStringSpinBox();
    addrStandardChannels(rxSelector);

    QLabel *ryLabel = new QLabel(tr("Y-Axis:"), this);
    rySelector = new QxtStringSpinBox();
    addrStandardChannels(rySelector);

    rIgnore = new QCheckBox(tr("Ignore Zero"));
    rIgnore->setChecked(true);
    rIgnore->hide();
    rFrameInterval = new QCheckBox(tr("Frame intervals"));
    rFrameInterval->setChecked(true);

    // layout reveal controls
    QHBoxLayout *r = new QHBoxLayout;
    r->setContentsMargins(0,0,0,0);
    r->addStretch();
    r->addWidget(rxLabel);
    r->addWidget(rxSelector);
    r->addWidget(ryLabel);
    r->addWidget(rySelector);
    r->addSpacing(5);
    r->addWidget(rIgnore);
    r->addWidget(rFrameInterval);
    r->addStretch();
    setRevealLayout(r);

    QWidget *c = new QWidget;
    QFormLayout *cl = new QFormLayout(c);
    setControls(c);

    // the plot widget
    scatterPlot= new ScatterPlot(context);
    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->addWidget(scatterPlot);

    setChartLayout(vlayout);

    // labels
    xLabel = new QLabel(tr("X-Axis:"), this);
    xSelector = new QComboBox;
    addStandardChannels(xSelector);
    xSelector->setCurrentIndex(0); // power
    rxSelector->setValue(0);
    cl->addRow(xLabel, xSelector);

    yLabel = new QLabel(tr("Y-Axis:"), this);
    ySelector = new QComboBox;
    addStandardChannels(ySelector);
    ySelector->setCurrentIndex(2); // heartrate
    rySelector->setValue(2);
    cl->addRow(yLabel, ySelector);

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
    connect(context, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    connect(context, SIGNAL(intervalsChanged()), this, SLOT(intervalSelected()));
    connect(xSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(xSelectorChanged(int)));
    connect(ySelector, SIGNAL(currentIndexChanged(int)), this, SLOT(ySelectorChanged(int)));
    connect(rxSelector, SIGNAL(valueChanged(int)), this, SLOT(rxSelectorChanged(int)));
    connect(rySelector, SIGNAL(valueChanged(int)), this, SLOT(rySelectorChanged(int)));
    connect(grid, SIGNAL(stateChanged(int)), this, SLOT(setGrid()));
    //connect(legend, SIGNAL(stateChanged(int)), this, SLOT(setLegend()));
    connect(frame, SIGNAL(stateChanged(int)), this, SLOT(setFrame()));
    connect(ignore, SIGNAL(stateChanged(int)), this, SLOT(setIgnore()));
    connect(rFrameInterval, SIGNAL(stateChanged(int)), this, SLOT(setrFrame()));
    connect(rIgnore, SIGNAL(stateChanged(int)), this, SLOT(setrIgnore()));
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));

    // comparing things
    connect(context, SIGNAL(compareIntervalsStateChanged(bool)), this, SLOT(compareChanged()));
    connect(context, SIGNAL(compareIntervalsChanged()), this, SLOT(compareChanged()));

    // set colors
    configChanged();
}

void
ScatterWindow::configChanged()
{
    setProperty("color", GColor(CPLOTBACKGROUND));

    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    setPalette(palette);
}

void
ScatterWindow::rideSelected()
{
    if (!amVisible())
        return;

    ride = myRideItem;

    if (ride == current) return;

    if (!ride || !ride->ride() || !ride->ride()->dataPoints().count()) {
        current = NULL;
        setIsBlank(true);
        return;
    } else
        setIsBlank(false);

    current = ride;
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
    rFrameInterval->setChecked(frame->isChecked());
    setData();
}

void
ScatterWindow::setrFrame()
{
    settings.frame = rFrameInterval->isChecked();
    frame->setChecked(rFrameInterval->isChecked());
    setData();
}

void
ScatterWindow::setIgnore()
{
    settings.ignore = ignore->isChecked();
    rIgnore->setChecked(ignore->isChecked());
    setData();
}

void
ScatterWindow::setrIgnore()
{
    settings.ignore = rIgnore->isChecked();
    ignore->setChecked(rIgnore->isChecked());
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
ScatterWindow::xSelectorChanged(int value)
{
    rxSelector->setValue(value);
    setData();
}

void
ScatterWindow::rxSelectorChanged(int value)
{
    xSelector->setCurrentIndex(value);
    setData();
}

void
ScatterWindow::ySelectorChanged(int value)
{
    rySelector->setValue(value);
    setData();
}

void
ScatterWindow::rySelectorChanged(int value)
{
    ySelector->setCurrentIndex(value);
    setData();
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

    /* Not a blank state ? Just no data and we can change series ?
    if ((setting.x == MODEL_POWER || setting.y == MODEL_POWER ) && !ride->ride()->isDataPresent(RideFile::watts))
        setIsBlank(true);
    else if ((setting.x == MODEL_CADENCE || setting.y == MODEL_CADENCE ) && !ride->ride()->isDataPresent(RideFile::cad))
        setIsBlank(true);
    else if ((setting.x == MODEL_HEARTRATE || setting.y == MODEL_HEARTRATE ) && !ride->ride()->isDataPresent(RideFile::hr))
        setIsBlank(true);
    else if ((setting.x == MODEL_SPEED || setting.y == MODEL_SPEED ) && !ride->ride()->isDataPresent(RideFile::kph))
        setIsBlank(true);
    else
        setIsBlank(false);
    */

    // any intervals to plot?
    settings.intervals.clear();
    for (int i=0; i<context->athlete->allIntervalItems()->childCount(); i++) {
        IntervalItem *current = dynamic_cast<IntervalItem *>(context->athlete->allIntervalItems()->child(i));
        if (current != NULL && current->isSelected() == true)
                settings.intervals.append(current);
    }
    scatterPlot->setData(&settings);
}

void
ScatterWindow::compareChanged()
{
    /* no stale mode ?
    if (!amVisible()) {
        return;
    }*/

    setData();
    repaint();

}


