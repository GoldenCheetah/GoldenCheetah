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
#include "MainWindow.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "math.h"
#include "Units.h" // for MILES_PER_KM

#include <QtGui>
#include <QString>

//
// Prepare some preset analysis (initialization moved to constructor to enable translation)
//
typedef struct preset {
    QString name;       // QComboBox value
    int x, y, z, color; // values for xselector, yselector and zselector and color
    bool ignore;
    int bin;            // value for binwidth
} preset;
static preset *presets;

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
    box->addItem(tr("Latitude"), MODEL_LAT);
    box->addItem(tr("Longitude"), MODEL_LONG);
}

ModelWindow::ModelWindow(MainWindow *parent, const QDir &home) :
    QWidget(parent), home(home), main(parent), ride(NULL), current(NULL)
{
    static preset presetsInit[] = {

        { tr("User Defined"), 0, 0, 0, 0, true, 20 },
        { tr("Natural Cadence Selection"), 0, 1, 12, 12, false, 5 }, // don't ignore zero for cadences!
        { tr("Route Visualisation"), 11, 10, 4, 4, false, 5 }, // don't ignore zero for cadences!
        { tr("Power Fatigue"), 9, 0, 12, 12, true, 5 },
        { tr("Impact of Altitude"), 4, 2, 0, 12, true, 10 },
        { "", 0, 0, 0, 0, false, 0 }
    };
    presets = presetsInit;

    // Layouts
    QVBoxLayout *mainLayout = new QVBoxLayout;
    QHBoxLayout *topLayout = new QHBoxLayout;
    QHBoxLayout *chartLayout = new QHBoxLayout;
    QHBoxLayout *control1Layout = new QHBoxLayout;
    QHBoxLayout *control2Layout = new QHBoxLayout;

    // presetValues
    presetValues = new QComboBox;
    fillPresets(presetValues);
    presetValues->setCurrentIndex(1);

    // labels
    presetLabel = new QLabel(tr("Analyse"), this);
    xLabel = new QLabel(tr("X-Axis:"), this);
    yLabel = new QLabel(tr("Y-Axis:"), this);
    zLabel = new QLabel(tr("Z-Axis:"), this);
    colorLabel = new QLabel(tr("Color:"), this);
    binLabel = new QLabel(tr("Bin Width:"), this);

    // selectors
    xSelector = new QComboBox;
    addStandardChannels(xSelector);
    xSelector->setCurrentIndex(0); // power

    ySelector = new QComboBox;
    addStandardChannels(ySelector);
    ySelector->setCurrentIndex(1); // cadence

    zSelector = new QComboBox;
    addStandardChannels(zSelector);
    zSelector->addItem(tr("Time at X&Y"), MODEL_XYTIME);
    zSelector->setCurrentIndex(12); // time at xy

    colorSelector = new QComboBox;
    addStandardChannels(colorSelector);
    colorSelector->addItem(tr("Power Zone"), MODEL_POWERZONE);
    colorSelector->addItem(tr("Time at X&Y"), MODEL_XYTIME);
    colorSelector->setCurrentIndex(12); // power zone

    styleSelector = new QComboBox;
    styleSelector->addItem(tr("Bar"));
    styleSelector->addItem(tr("Grid"));
    styleSelector->addItem(tr("Surface"));
    styleSelector->addItem(tr("Dots"));
    styleSelector->setCurrentIndex(0);

    ignore = new QCheckBox(tr("Ignore Zero"));
    ignore->setChecked(true);
    grid = new QCheckBox(tr("Show Grid"));
    grid->setChecked(true);
    frame = new QCheckBox(tr("Frame Intervals"));
    frame->setChecked(true);
    legend = new QCheckBox(tr("Legend"));
    legend->setChecked(true);

    binWidthLineEdit = new QLineEdit(this);
    binWidthLineEdit->setFixedWidth(30);
    binWidthLineEdit->setText("5");
    binWidthSlider = new QSlider(Qt::Horizontal);
    binWidthSlider->setTickPosition(QSlider::TicksBelow);
    binWidthSlider->setTickInterval(1);
    binWidthSlider->setMinimum(3);
    binWidthSlider->setMaximum(100);
    binWidthSlider->setValue(5);

    resetView = new QPushButton(tr("Reset View"));

    // the plot widget
    modelPlot= new ModelPlot(main, NULL);
    zpane = new QSlider(Qt::Vertical);
    zpane->setTickInterval(1);
    zpane->setMinimum(0);
    zpane->setMaximum(100);
    zpane->setValue(0);

    chartLayout->addWidget(zpane);
    chartLayout->addWidget(modelPlot);

    // Build Layouts
    topLayout->addWidget(presetLabel);
    topLayout->addWidget(presetValues);
    topLayout->insertStretch(-1);
    topLayout->addWidget(grid);
    topLayout->addWidget(legend);
    topLayout->addWidget(frame);
    topLayout->addWidget(styleSelector);
    topLayout->setSpacing(10);

    control1Layout->addWidget(xLabel);
    control1Layout->addWidget(xSelector);
    control1Layout->addWidget(yLabel);
    control1Layout->addWidget(ySelector);
    control1Layout->addWidget(zLabel);
    control1Layout->addWidget(zSelector);
    control1Layout->addWidget(colorLabel);
    control1Layout->addWidget(colorSelector);
    control1Layout->insertStretch(0);
    control1Layout->insertStretch(-1);
    control1Layout->setSpacing(10);

    control2Layout->addWidget(binLabel);
    control2Layout->addWidget(binWidthLineEdit);
    control2Layout->addWidget(binWidthSlider);
    control2Layout->addWidget(ignore);
    control2Layout->addWidget(resetView);
    control2Layout->setSpacing(10);

    // Now layout the screen with the new widgets
    mainLayout->addItem(topLayout);
    mainLayout->addItem(chartLayout);
    mainLayout->addItem(control1Layout);
    mainLayout->addItem(control2Layout);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    setLayout(mainLayout);

    // now connect up the widgets
    connect(main, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
    connect(main, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
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
    connect(resetView, SIGNAL(clicked()), this, SLOT(resetViewPoint()));
    connect(zpane, SIGNAL(valueChanged(int)), this, SLOT(setZPane(int)));
}

void
ModelWindow::rideSelected()
{
    if (main->activeTab() != this)
        return;
    ride = main->rideItem();

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
    if (main->activeTab() != this)
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
    settings.xbin = binWidthSlider->value(); // XXX fixed to single bin width
    settings.ybin = binWidthSlider->value(); // XXX due to issues with bar geometry
    settings.crop = false; // XXX not implemented
    settings.zpane = 0;
    settings.ignore = ignore->isChecked();
    settings.gridlines = grid->isChecked();
    settings.frame = frame->isChecked();
    settings.legend = legend->isChecked();
    settings.adjustPlot = adjustPlot;
    zpane->setValue(0); // reset it!

    // any intervals to plot?
    settings.intervals.clear();
    for (int i=0; i<main->allIntervalItems()->childCount(); i++) {
        IntervalItem *current = dynamic_cast<IntervalItem *>(main->allIntervalItems()->child(i));
        if (current != NULL && current->isSelected() == true)
                settings.intervals.append(current);
    }
    modelPlot->setData(&settings);
    setClean();
}

void
ModelWindow::setBinWidthFromSlider()
{
    binWidthLineEdit->setText(QString("%1").arg(binWidthSlider->value()));
    setDirty();
}

void
ModelWindow::setBinWidthFromLineEdit()
{
    binWidthSlider->setValue(binWidthLineEdit->text().toInt());
    setDirty();
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
    resetView->setText(tr("Plot"));
}

void
ModelWindow::setClean()
{
    dirty = false;
    resetView->setText(tr("Reset View"));
}

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
    for (int i=0; presets[i].name != ""; i++) {
        p->addItem(presets[i].name);
    }
}
