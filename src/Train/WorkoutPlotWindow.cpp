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


WorkoutPlotWindow::WorkoutPlotWindow(Context *context) :
    GcChartWindow(context), context(context)
{
    HelpWhatsThis *helpContents = new HelpWhatsThis(this);
    this->setWhatsThis(helpContents->getWhatsThisText(HelpWhatsThis::ChartTrain_Workout));

    ctrlsShowNotification = new QCheckBox();
    connect(ctrlsShowNotification, &QCheckBox::toggled, this, &WorkoutPlotWindow::setShowNotifications);

    ctrlsLineWidthLabel = new QLabel();
    ctrlsLineWidth = new QSlider(Qt::Horizontal);
    ctrlsLineWidth->setRange(0, 8);

    ctrlsShowCurveLabel = new QLabel();
    ctrlsShowWbalCurvePredict = new QCheckBox();
    ctrlsShowWbalCurvePredict->setChecked(true);
    ctrlsShowWbalCurve = new QCheckBox();
    ctrlsShowWbalCurve->setChecked(true);
    ctrlsShowWattsCurve = new QCheckBox();
    ctrlsShowWattsCurve->setChecked(true);
    ctrlsShowHrCurve = new QCheckBox();
    ctrlsShowHrCurve->setChecked(true);
    ctrlsShowCadCurve = new QCheckBox();
    ctrlsShowCadCurve->setChecked(true);
    ctrlsShowSpeedCurve = new QCheckBox();
    ctrlsShowSpeedCurve->setChecked(true);

    ctrlsCommonLabel = new QLabel();
    ctrlsErgmodeLabel = new QLabel();

    ctrlsSituationLabel = new QLabel();
    ctrlsSituation = new QComboBox();
    ctrlsSituation->addItem("");
    ctrlsSituation->addItem("");
    ctrlsSituation->addItem("");
    connect(ctrlsSituation, SIGNAL(currentIndexChanged(int)), this, SLOT(setShowColorZones(int)));

    ctrlsTransparencyLabel = new QLabel();
    ctrlsTransparencySlider = new QSlider(Qt::Horizontal);
    ctrlsTransparencySlider->setRange(0, 100);

    ctrlsShowTooltipLabel = new QLabel();
    ctrlsShowTooltip = new QComboBox();
    ctrlsShowTooltip->addItem("");
    ctrlsShowTooltip->addItem("");
    connect(ctrlsShowTooltip, SIGNAL(currentIndexChanged(int)), this, SLOT(setShowTooltip(int)));

    QFormLayout *settingsLayout = newQFormLayout();
    settingsLayout->addRow(ctrlsCommonLabel);
    settingsLayout->addRow("", ctrlsShowNotification);
    settingsLayout->addRow(ctrlsLineWidthLabel, ctrlsLineWidth);
    settingsLayout->addRow(ctrlsShowCurveLabel, ctrlsShowWbalCurvePredict);
    settingsLayout->addRow("", ctrlsShowWbalCurve);
    settingsLayout->addRow("", ctrlsShowWattsCurve);
    settingsLayout->addRow("", ctrlsShowHrCurve);
    settingsLayout->addRow("", ctrlsShowCadCurve);
    settingsLayout->addRow("", ctrlsShowSpeedCurve);
    settingsLayout->addItem(new QSpacerItem(0, 15 * dpiYFactor));
    settingsLayout->addRow(ctrlsErgmodeLabel);
    settingsLayout->addRow(ctrlsSituationLabel, ctrlsSituation);
    settingsLayout->addRow(ctrlsTransparencyLabel, ctrlsTransparencySlider);
    settingsLayout->addRow(ctrlsShowTooltipLabel, ctrlsShowTooltip);

    setContentsMargins(0,0,0,0);
    setControls(centerLayoutInWidget(settingsLayout, false));
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));

    QVBoxLayout *layout = new QVBoxLayout;
    setChartLayout(layout);
    layout->setSpacing(0);
    layout->setContentsMargins(2,2,2,2);
    ergPlot = new ErgFilePlot(context);
    layout->addWidget(ergPlot);

    QTimer *notificationTimer = new QTimer(this);

    connect(context, SIGNAL(setNow(long)), this, SLOT(setNow(long)));
    connect(context, SIGNAL(ergFileSelected(ErgFile*)), this, SLOT(ergFileSelected(ErgFile*)));
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), ergPlot, SLOT(performancePlot(RealtimeData)));
    connect(context, SIGNAL(start()), ergPlot, SLOT(start()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    connect(notificationTimer, &QTimer::timeout, this, [this]() {
        setProperty("subtitle", title);
    });
    connect(context, &Context::setNotification, this, [this, notificationTimer](QString notification, int timeout) {
        if (! showNotifications()) {
            return;
        }
        if (timeout > 0) {
            notificationTimer->setInterval(timeout * 1000);
            notificationTimer->setSingleShot(true);
            notificationTimer->start();
        } else {
            notificationTimer->stop();
        }

        setProperty("subtitle", notification);
    });
    connect(context, &Context::clearNotification, this, [this]() {
        setProperty("subtitle", title);
    });
    connect(ctrlsShowWbalCurvePredict, &QCheckBox::toggled, ergPlot, &ErgFilePlot::showWbalCurvePredict);
    connect(ctrlsShowWbalCurve, &QCheckBox::toggled, ergPlot, &ErgFilePlot::showWbalCurve);
    connect(ctrlsShowWattsCurve, &QCheckBox::toggled, ergPlot, &ErgFilePlot::showWattsCurve);
    connect(ctrlsShowHrCurve, &QCheckBox::toggled, ergPlot, &ErgFilePlot::showHrCurve);
    connect(ctrlsShowCadCurve, &QCheckBox::toggled, ergPlot, &ErgFilePlot::showCadCurve);
    connect(ctrlsShowSpeedCurve, &QCheckBox::toggled, ergPlot, &ErgFilePlot::showSpeedCurve);
    connect(ctrlsLineWidth, &QSlider::valueChanged, this, [this](int value) {
        ergPlot->setPlotLineWidth(1 + value / 2.0);
    });
    connect(ctrlsTransparencySlider, &QSlider::valueChanged, this, [this](int value) {
        ergPlot->setActiveCurveAlpha(255 - value * 2.5);
    });

    configChanged(0);

    // Initil setup based on currently selected workout
    ergFileSelected(context->currentErgFile());
}

void
WorkoutPlotWindow::ergFileSelected(ErgFile *f)
{
    // rename window to workout name
    if (f && f->name() != "") title = f->name();
    else title = "";
    setProperty("subtitle", title);

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

    ctrlsCommonLabel->setText("<b>" + tr("Common settings") + "</b>");

    ctrlsShowNotification->setText(tr("Show notifications and textcues in title"));

    ctrlsLineWidthLabel->setText(tr("Plot line width"));

    ctrlsShowCurveLabel->setText(tr("Show Curve"));
    ctrlsShowWbalCurvePredict->setText(tr("W'bal Prediction"));
    ctrlsShowWbalCurve->setText(tr("W'bal"));
    ctrlsShowWattsCurve->setText(tr("Power"));
    ctrlsShowHrCurve->setText(tr("Heartrate"));
    ctrlsShowCadCurve->setText(tr("Cadence"));
    ctrlsShowSpeedCurve->setText(tr("Speed"));

    ctrlsErgmodeLabel->setText("<b>" + tr("Ergmode specific settings") + "</b>");
    ctrlsSituationLabel->setText(tr("Color power zones"));
    ctrlsSituation->setItemText(0, tr("Never"));
    ctrlsSituation->setItemText(1, tr("Always"));
    ctrlsSituation->setItemText(2, tr("When stopped"));

    ctrlsTransparencyLabel->setText(tr("Power zones transparency (when active)"));

    ctrlsShowTooltipLabel->setText(tr("Show tooltip"));
    ctrlsShowTooltip->setItemText(0, tr("Never"));
    ctrlsShowTooltip->setItemText(1, tr("When stopped"));

    repaint();
}


bool
WorkoutPlotWindow::showNotifications
() const
{
    return ctrlsShowNotification->isChecked();
}


void
WorkoutPlotWindow::setShowNotifications
(bool show)
{
    if (! show) {
        setProperty("subtitle", title);
    }
    ctrlsShowNotification->setChecked(show);
}


double
WorkoutPlotWindow::lineWidth
() const
{
    return ctrlsLineWidth->value();
}


void
WorkoutPlotWindow::setLineWidth
(double width)
{
    ctrlsLineWidth->setValue(width);
}


int
WorkoutPlotWindow::showCurves
() const
{
    int ret =   (ctrlsShowWbalCurvePredict->isChecked() ? 1 : 0) << 0
              | (ctrlsShowWbalCurve->isChecked() ? 1 : 0) << 1
              | (ctrlsShowWattsCurve->isChecked() ? 1 : 0) << 2
              | (ctrlsShowHrCurve->isChecked() ? 1 : 0) << 3
              | (ctrlsShowCadCurve->isChecked() ? 1 : 0) << 4
              | (ctrlsShowSpeedCurve->isChecked() ? 1 : 0) << 5;
    return ret;
}


void
WorkoutPlotWindow::setShowCurves
(int curves)
{
    ctrlsShowWbalCurvePredict->setChecked((curves & (1 << 0)) > 0);
    ctrlsShowWbalCurve->setChecked((curves & (1 << 1)) > 0);
    ctrlsShowWattsCurve->setChecked((curves & (1 << 2)) > 0);
    ctrlsShowHrCurve->setChecked((curves & (1 << 3)) > 0);
    ctrlsShowCadCurve->setChecked((curves & (1 << 4)) > 0);
    ctrlsShowSpeedCurve->setChecked((curves & (1 << 5)) > 0);
    ergPlot->showWbalCurvePredict(ctrlsShowWbalCurvePredict->isChecked());
    ergPlot->showWbalCurve(ctrlsShowWbalCurve->isChecked());
    ergPlot->showWattsCurve(ctrlsShowWattsCurve->isChecked());
    ergPlot->showHrCurve(ctrlsShowHrCurve->isChecked());
    ergPlot->showCadCurve(ctrlsShowCadCurve->isChecked());
    ergPlot->showSpeedCurve(ctrlsShowSpeedCurve->isChecked());
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
    ctrlsTransparencySlider->setEnabled(index == 1);
}


int
WorkoutPlotWindow::colorZonesTransparency
() const
{
    return ctrlsTransparencySlider->value();
}


void
WorkoutPlotWindow::setColorZonesTransparency
(int transparency)
{
    ctrlsTransparencySlider->setValue(transparency);
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
