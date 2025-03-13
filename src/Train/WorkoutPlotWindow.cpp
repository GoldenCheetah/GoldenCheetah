/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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


#include "WorkoutPlotWindow.h"
#include "Context.h"
#include "HelpWhatsThis.h"

#include <QFormLayout>
#include <QGroupBox>


WorkoutPlotWindow::WorkoutPlotWindow(Context *context) :
    GcChartWindow(context), context(context)
{
    HelpWhatsThis *helpContents = new HelpWhatsThis(this);
    this->setWhatsThis(helpContents->getWhatsThisText(HelpWhatsThis::ChartTrain_Workout));

    // Chart settings
    QWidget *settingsWidget = new QWidget(this);
    settingsWidget->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *commonLayout = new QVBoxLayout(settingsWidget);

    ctrlsGroupBox = new QGroupBox(tr("Ergmode specific settings"));
    commonLayout->addWidget(ctrlsGroupBox);
    commonLayout->addStretch();

    QFormLayout *ergmodeLayout = new QFormLayout(ctrlsGroupBox);

    ctrlsSituationLabel = new QLabel(tr("Color power zones"));
    ctrlsSituation = new QComboBox();
    ctrlsSituation->addItem(tr("Never"));
    ctrlsSituation->addItem(tr("Always"));
    ctrlsSituation->addItem(tr("When stopped"));
    connect(ctrlsSituation, SIGNAL(currentIndexChanged(int)), this, SLOT(setShowColorZones(int)));
    ergmodeLayout->addRow(ctrlsSituationLabel, ctrlsSituation);

    ctrlsShowTooltipLabel = new QLabel(tr("Show tooltip"));
    ctrlsShowTooltip = new QComboBox();
    ctrlsShowTooltip->addItem(tr("Never"));
    ctrlsShowTooltip->addItem(tr("When stopped"));
    connect(ctrlsShowTooltip, SIGNAL(currentIndexChanged(int)), this, SLOT(setShowTooltip(int)));
    ergmodeLayout->addRow(ctrlsShowTooltipLabel, ctrlsShowTooltip);

    setContentsMargins(0,0,0,0);
    setControls(settingsWidget);
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));

    QVBoxLayout *layout = new QVBoxLayout;
    setChartLayout(layout);
    layout->setSpacing(0);
    layout->setContentsMargins(2,2,2,2);
    ergPlot = new ErgFilePlot(context);
    layout->addWidget(ergPlot);

    connect(context, SIGNAL(setNow(long)), this, SLOT(setNow(long)));
    connect(context, SIGNAL(ergFileSelected(ErgFile*)), this, SLOT(ergFileSelected(ErgFile*)));
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), ergPlot, SLOT(performancePlot(RealtimeData)));
    connect(context, SIGNAL(start()), ergPlot, SLOT(start()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // Initil setup based on currently selected workout
    ergFileSelected(context->currentErgFile());
}

void
WorkoutPlotWindow::ergFileSelected(ErgFile *f)
{
    // rename window to workout name
    if (f && f->name() != "") setProperty("subtitle", f->name());
    else setProperty("subtitle", "");

    ergPlot->setData(f);
    ergPlot->replot();
}

void
WorkoutPlotWindow::setNow(long now)
{
    if (isVisible())
        ergPlot->setNow(now);
}

void
WorkoutPlotWindow::configChanged(qint32)
{
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));

    ctrlsGroupBox->setTitle(tr("Ergmode specific settings"));
    ctrlsSituationLabel->setText(tr("Color power zones"));
    ctrlsSituation->setItemText(0, tr("Never"));
    ctrlsSituation->setItemText(1, tr("Always"));
    ctrlsSituation->setItemText(2, tr("When stopped"));

    ctrlsShowTooltipLabel->setText(tr("Show tooltip"));
    ctrlsShowTooltip->setItemText(0, tr("Never"));
    ctrlsShowTooltip->setItemText(1, tr("When stopped"));

    repaint();
}


int
WorkoutPlotWindow::showColorZones
() const
{
    return ctrlsSituation->currentIndex();
}


void
WorkoutPlotWindow::setShowColorZones
(int index)
{
    ctrlsSituation->setCurrentIndex(index);
    ergPlot->setShowColorZones(index);
}


int
WorkoutPlotWindow::showTooltip
() const
{
    return ctrlsShowTooltip->currentIndex();
}


void
WorkoutPlotWindow::setShowTooltip
(int index)
{
    ctrlsShowTooltip->setCurrentIndex(index);
    ergPlot->setShowTooltip(index);
}
