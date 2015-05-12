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

#include "ModelWindow.h"
#include "ModelPlot.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "cmath"
#include "Units.h" // for MILES_PER_KM
#include "Colors.h" // for MILES_PER_KM
#include "HelpWhatsThis.h"

#include <QtGui>
#include <QString>

void
ModelWindow::addStandardChannels(QComboBox *box)
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
    box->addItem(tr("Slope"), MODEL_SLOPE);
    box->addItem(tr("Latitude"), MODEL_LAT);
    box->addItem(tr("Longitude"), MODEL_LONG);
    box->addItem(tr("L/R Balance"), MODEL_LRBALANCE);
    box->addItem(tr("Running Vertical Oscillation"), MODEL_RV);
    box->addItem(tr("Running Cadence"), MODEL_RCAD);
    box->addItem(tr("Running GCT"), MODEL_RGCT);
    box->addItem(tr("Gear Ratio"), MODEL_GEAR);
    box->addItem(tr("Muscle Oxygen"), MODEL_SMO2);
    box->addItem(tr("Haemoglobin Mass"), MODEL_THB);
}

ModelWindow::ModelWindow(Context *context) :
    GcChartWindow(context), context(context), ride(NULL), current(NULL)
{
    QWidget *c = new QWidget(this);
    HelpWhatsThis *helpConfig = new HelpWhatsThis(c);
    c->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::ChartRides_3D));
    QFormLayout *cl = new QFormLayout(c);
    setControls(c);

    // the plot widget
    QHBoxLayout *mainLayout = new QHBoxLayout;
    modelPlot= new ModelPlot(context, this);
    zpane = new QSlider(Qt::Vertical);
    zpane->setTickInterval(1);
    zpane->setMinimum(0);
    zpane->setMaximum(100);
    zpane->setValue(0);
    mainLayout->addWidget(zpane);
    mainLayout->addWidget(modelPlot);
    setChartLayout(mainLayout);

    HelpWhatsThis *help = new HelpWhatsThis(modelPlot);
    modelPlot->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_3D));

    // preset Values
    presetLabel = new QLabel(tr("Analyse"), this);
    presetValues = new QComboBox;
    fillPresets(presetValues);
    presetValues->setCurrentIndex(1);
    cl->addRow(presetLabel, presetValues);

    // labels
    xLabel = new QLabel(tr("X-Axis:"), this);
    xSelector = new QComboBox;
    addStandardChannels(xSelector);
    xSelector->setCurrentIndex(0); // power
    cl->addRow(xLabel, xSelector);

    yLabel = new QLabel(tr("Y-Axis:"), this);
    ySelector = new QComboBox;
    addStandardChannels(ySelector);
    ySelector->setCurrentIndex(1); // cadence
    cl->addRow(yLabel, ySelector);

    zLabel = new QLabel(tr("Z-Axis:"), this);
    zSelector = new QComboBox;
    addStandardChannels(zSelector);
    zSelector->addItem(tr("Time at X&Y"), MODEL_XYTIME);
    zSelector->setCurrentIndex(12); // time at xy
    cl->addRow(zLabel, zSelector);

    colorLabel = new QLabel(tr("Color:"), this);
    colorSelector = new QComboBox;
    addStandardChannels(colorSelector);
    colorSelector->addItem(tr("Power Zone"), MODEL_POWERZONE);
    colorSelector->addItem(tr("Time at X&Y"), MODEL_XYTIME);
    colorSelector->setCurrentIndex(12); // power zone
    cl->addRow(colorLabel, colorSelector);

    binLabel = new QLabel(tr("Bin Width:"), this);
    binWidthLineEdit = new QLineEdit(this);
    binWidthLineEdit->setFixedWidth(30);
    binWidthLineEdit->setText("5");
    cl->addRow(binLabel, binWidthLineEdit);

    binWidthSlider = new QSlider(Qt::Horizontal);
    binWidthSlider->setTickPosition(QSlider::TicksBelow);
    binWidthSlider->setTickInterval(1);
    binWidthSlider->setMinimum(3);
    binWidthSlider->setMaximum(100);
    binWidthSlider->setValue(5);
    cl->addRow(binWidthSlider);

    // selectors
    styleSelector = new QComboBox;
    styleSelector->addItem(tr("Bar"));
    styleSelector->addItem(tr("Grid"));
    styleSelector->addItem(tr("Surface"));
    styleSelector->addItem(tr("Dots"));
    styleSelector->setCurrentIndex(0);
    cl->addRow(styleSelector);

    ignore = new QCheckBox(tr("Ignore Zero"));
    ignore->setChecked(true);
    cl->addRow(ignore);

    grid = new QCheckBox(tr("Show Grid"));
    grid->setChecked(true);
    cl->addRow(grid);

    frame = new QCheckBox(tr("Frame Intervals"));
    frame->setChecked(true);
    cl->addRow(frame);

    legend = new QCheckBox(tr("Legend"));
    legend->setChecked(true);
    cl->addRow(legend);

    // now connect up the widgets
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(context, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    connect(presetValues, SIGNAL(currentIndexChanged(int)), this, SLOT(applyPreset(int)));
    connect(xSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(setDirty()));
    connect(ySelector, SIGNAL(currentIndexChanged(int)), this, SLOT(setDirty()));
    connect(zSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(setDirty()));
    connect(colorSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(setDirty()));
    connect(grid, SIGNAL(stateChanged(int)), this, SLOT(setGrid()));
    connect(legend, SIGNAL(stateChanged(int)), this, SLOT(setLegend()));
    connect(frame, SIGNAL(stateChanged(int)), this, SLOT(setFrame()));
    connect(styleSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(styleSelected(int)));
    connect(ignore, SIGNAL(stateChanged(int)), this, SLOT(setDirty()));
    connect(binWidthSlider, SIGNAL(valueChanged(int)), this, SLOT(setBinWidthFromSlider()));
    connect(binWidthLineEdit, SIGNAL(editingFinished()), this, SLOT(setBinWidthFromLineEdit()));
    connect(zpane, SIGNAL(valueChanged(int)), this, SLOT(setZPane(int)));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // set colors on first run
    configChanged(CONFIG_APPEARANCE);
}

void
ModelWindow::configChanged(qint32)
{
    setProperty("color", GColor(CPLOTBACKGROUND));
}

void
ModelWindow::rideSelected()
{
    if (!amVisible())
        return;
    ride = myRideItem;

    if (!ride || !ride->ride() || ride == current)

    current = ride;
    setData(true);
}

void
ModelWindow::styleSelected(int index)
{
    modelPlot->setStyle(index); // 0 = bar, 1 = surface
}

void
ModelWindow::setGrid()
{
    modelPlot->setGrid(grid->isChecked());
}

void
ModelWindow::setLegend()
{
    modelPlot->setLegend(legend->isChecked(), settings.color);
}

void
ModelWindow::setFrame()
{
    modelPlot->setFrame(frame->isChecked());
}
void
ModelWindow::setZPane(int z)
{
    modelPlot->setZPane(z);
}

void
ModelWindow::intervalSelected()
{
    if (!amVisible())
        return;
    setData(false);
}

void
ModelWindow::setData(bool adjustPlot)
{
    settings.ride = ride;
    settings.x = xSelector->itemData(xSelector->currentIndex()).toInt();
    settings.y = ySelector->itemData(ySelector->currentIndex()).toInt();
    settings.z = zSelector->itemData(zSelector->currentIndex()).toInt();
    settings.color = colorSelector->itemData(colorSelector->currentIndex()).toInt();
    settings.xbin = binWidthSlider->value();
    settings.ybin = binWidthSlider->value();
    settings.crop = false; // not implemented yet
    settings.zpane = 0;
    settings.ignore = ignore->isChecked();
    settings.gridlines = grid->isChecked();
    settings.frame = frame->isChecked();
    settings.legend = legend->isChecked();
    settings.adjustPlot = adjustPlot;
    zpane->setValue(0); // reset it!

    // any intervals to plot?
    if (ride) settings.intervals = ride->intervalsSelected();
    else settings.intervals.clear();

    setUpdatesEnabled(false);

    // reset the model parameters
    modelPlot->setData(&settings);

    // if setdata resulted in the plot being hidden
    // then the settings were not valid.
    if (modelPlot->isHidden()) {
        zpane->hide();
        setIsBlank(true);
    } else {
        zpane->show();
        setIsBlank(false);
    }
    setClean();

    setUpdatesEnabled(true);
}

void
ModelWindow::setBinWidthFromSlider()
{
    binWidthLineEdit->setText(QString("%1").arg(binWidthSlider->value()));
    setData(false);
}

void
ModelWindow::setBinWidthFromLineEdit()
{
    binWidthSlider->setValue(binWidthLineEdit->text().toInt());
}

void
ModelWindow::resetViewPoint()
{
    // either replot or center after fiddling
    if (dirty) setData(true);
    else modelPlot->resetViewPoint();
}

void
ModelWindow::setDirty()
{
    dirty = true;
    setData(false);
}

void
ModelWindow::setClean()
{
    dirty = false;
}

//
// Prepare some preset analysis (initialization moved to fillPresets to enable translation)
//
typedef struct preset {
    QString name;       // QComboBox value
    int x, y, z, color; // values for xselector, yselector and zselector and color
    bool ignore;
    int bin;            // value for binwidth
} t_preset;
static t_preset *presets;

void
ModelWindow::applyPreset(int index)
{
    if (index >=0) {
        xSelector->setCurrentIndex(presets[index].x);
        ySelector->setCurrentIndex(presets[index].y);
        zSelector->setCurrentIndex(presets[index].z);
        colorSelector->setCurrentIndex(presets[index].color);
        ignore->setChecked(presets[index].ignore);
        binWidthSlider->setValue(presets[index].bin);
        binWidthLineEdit->setText(QString("%1").arg(presets[index].bin));

        setDirty();
        if (index) setData(true);
    }
}

void
ModelWindow::fillPresets(QComboBox *p)
{
    static t_preset presetsInit[] = {

        { tr("User Defined"), 0, 0, 0, 0, true, 20 },
        { tr("Natural Cadence Selection"), 0, 1, 12, 12, false, 5 }, // don't ignore zero for cadences!
        { tr("Route Visualisation"), 11, 10, 4, 4, false, 5 }, // don't ignore zero for cadences!
        { tr("Power Fatigue"), 9, 0, 12, 12, true, 5 },
        { tr("Impact of Altitude"), 4, 2, 0, 12, true, 10 },
        { "", 0, 0, 0, 0, false, 0 }
    };
    presets = presetsInit;
    for (int i=0; presets[i].name != ""; i++) {
        p->addItem(presets[i].name);
    }
}
