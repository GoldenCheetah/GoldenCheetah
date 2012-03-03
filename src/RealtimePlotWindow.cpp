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


#include "RealtimePlotWindow.h"

RealtimePlotWindow::RealtimePlotWindow(MainWindow *mainWindow) :
    GcWindow(mainWindow), mainWindow(mainWindow), active(false)
{
    setContentsMargins(0,0,0,0);
    setInstanceName("RT Plot");
    setProperty("color", GColor(CRIDEPLOTBACKGROUND));

    QWidget *c = new QWidget;
    QVBoxLayout *cl = new QVBoxLayout(c);
    setControls(c);

    // setup the controls
    QLabel *showLabel = new QLabel(tr("Show"), c);
    cl->addWidget(showLabel);

    showHr = new QCheckBox(tr("Heart Rate"), this);
    showHr->setCheckState(Qt::Checked);
    cl->addWidget(showHr);

    showSpeed = new QCheckBox(tr("Speed"), this);
    showSpeed->setCheckState(Qt::Checked);
    cl->addWidget(showSpeed);

    showCad = new QCheckBox(tr("Cadence"), this);
    showCad->setCheckState(Qt::Checked);
    cl->addWidget(showCad);

    showPower = new QCheckBox(tr("Power"), this);
    showPower->setCheckState(Qt::Checked);
    cl->addWidget(showPower);

    showAlt = new QCheckBox(tr("Alternate Power"), this);
    showAlt->setCheckState(Qt::Checked);
    cl->addWidget(showAlt);

    showPow30s = new QCheckBox(tr("30s Power"), this);
    showPow30s->setCheckState(Qt::Checked);
    cl->addWidget(showPow30s);

    QLabel *smoothLabel = new QLabel(tr("Smoothing (5Hz Samples)"), this);
    smoothLineEdit = new QLineEdit(this);
    smoothLineEdit->setFixedWidth(40);

    cl->addWidget(smoothLabel);
    cl->addWidget(smoothLineEdit);
    smoothSlider = new QSlider(Qt::Horizontal);
    smoothSlider->setTickPosition(QSlider::TicksBelow);
    smoothSlider->setTickInterval(10);
    smoothSlider->setMinimum(1);
    smoothSlider->setMaximum(150);
    smoothLineEdit->setValidator(new QIntValidator(smoothSlider->minimum(),
                                                   smoothSlider->maximum(),
                                                   smoothLineEdit));
    cl->addWidget(smoothSlider);
    cl->addStretch();

    QVBoxLayout *layout = new QVBoxLayout(this);
    rtPlot = new RealtimePlot();
    layout->addWidget(rtPlot);

    // common controls
    connect(showPower, SIGNAL(stateChanged(int)), this, SLOT(setShowPower(int)));
    connect(showPow30s, SIGNAL(stateChanged(int)), this, SLOT(setShowPow30s(int)));
    connect(showHr, SIGNAL(stateChanged(int)), this, SLOT(setShowHr(int)));
    connect(showSpeed, SIGNAL(stateChanged(int)), this, SLOT(setShowSpeed(int)));
    connect(showCad, SIGNAL(stateChanged(int)), this, SLOT(setShowCad(int)));
    connect(showAlt, SIGNAL(stateChanged(int)), this, SLOT(setShowAlt(int)));
    connect(smoothSlider, SIGNAL(valueChanged(int)), this, SLOT(setSmoothingFromSlider()));
    connect(smoothLineEdit, SIGNAL(editingFinished()), this, SLOT(setSmoothingFromLineEdit()));

    // get updates..
    connect(mainWindow, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));

    // lets initialise all the smoothing variables
    hrtot = hrindex = cadtot = cadindex = spdtot = spdindex = alttot = altindex = powtot = powindex = 0;
    for(int i=0; i<150; i++) powHist[i] = altHist[i] = spdHist[i] = cadHist[i] = hrHist[i] = 0;

    // set to zero
    telemetryUpdate(RealtimeData());
}

void
RealtimePlotWindow::start()
{
    // lets initialise all the smoothing variables
    hrtot = hrindex = cadtot = cadindex = spdtot = spdindex = alttot = altindex = powtot = powindex = 0;
    for(int i=0; i<150; i++) powHist[i] = altHist[i] = spdHist[i] = cadHist[i] = hrHist[i] = 0;
}

void
RealtimePlotWindow::stop()
{
    // lets initialise all the smoothing variables
    hrtot = hrindex = cadtot = cadindex = spdtot = spdindex = alttot = altindex = powtot = powindex = 0;
    for(int i=0; i<150; i++) powHist[i] = altHist[i] = spdHist[i] = cadHist[i] = hrHist[i] = 0;
}

void
RealtimePlotWindow::pause()
{
}

void
RealtimePlotWindow::telemetryUpdate(RealtimeData rtData)
{
    // lets apply smoothing if we have to
    if (rtPlot->smooth) {

        // Heartrate
        double hr = rtData.value(RealtimeData::HeartRate);
        hrtot += hr;
        hrtot -= hrHist[hrindex];
        hrHist[hrindex] = hr;
        hrindex++;
        if (hrindex >= rtPlot->smooth) hrindex = 0;
        hr = hrtot / rtPlot->smooth;
        rtPlot->hrData->addData(hr);
        
        // Speed
        double spd= rtData.value(RealtimeData::Speed);
        spdtot += spd; spdtot -= spdHist[spdindex]; spdHist[spdindex] = spd;
        spdindex++; if (spdindex >= rtPlot->smooth) spdindex = 0;
        spd = spdtot / rtPlot->smooth;
        if (!mainWindow->useMetricUnits) spd *= MILES_PER_KM;
        rtPlot->spdData->addData(spd);

        // Power
        double pow = rtData.value(RealtimeData::Watts);
        powtot += pow; powtot -= powHist[powindex]; powHist[powindex] = pow;
        powindex++; if (powindex >= rtPlot->smooth) powindex = 0;
        pow = powtot / rtPlot->smooth;
        rtPlot->pwrData->addData(pow);

        // Alternate Power
        double alt = rtData.value(RealtimeData::AltWatts);
        alttot += alt; alttot -= altHist[altindex]; altHist[altindex] = alt;
        altindex++; if (altindex >= rtPlot->smooth) altindex = 0;
        alt = alttot / rtPlot->smooth;
        rtPlot->altPwrData->addData(alt);

        // Cadence
        double cad = rtData.value(RealtimeData::Cadence);
        cadtot += cad; cadtot -= cadHist[cadindex]; cadHist[cadindex] = cad;
        cadindex++; if (cadindex >= rtPlot->smooth) cadindex = 0;
        cad = cadtot / rtPlot->smooth;
        rtPlot->cadData->addData(cad);

        // its smoothed to 30s anyway
        rtPlot->pwr30Data->addData(rtData.value(RealtimeData::Watts));

    } else {
        rtPlot->pwrData->addData(rtData.value(RealtimeData::Watts));
        rtPlot->altPwrData->addData(rtData.value(RealtimeData::AltWatts));
        rtPlot->pwr30Data->addData(rtData.value(RealtimeData::Watts));
        rtPlot->cadData->addData(rtData.value(RealtimeData::Cadence));
        rtPlot->spdData->addData(rtData.value(RealtimeData::Speed));
        rtPlot->hrData->addData(rtData.value(RealtimeData::HeartRate));
    }
    rtPlot->replot();                // redraw
}

void
RealtimePlotWindow::setSmoothingFromSlider()
{
    // active tells us we have been triggered by
    // the setSmoothingFromLineEdit which will also
    // recalculates smoothing, lets not double up...
    if (active) return;
    else active = true;

    if (rtPlot->smooth != smoothSlider->value()) {
        setSmoothing(smoothSlider->value());
        smoothLineEdit->setText(QString("%1").arg(rtPlot->smooth));
    }
    active = false;
}

void
RealtimePlotWindow::setSmoothingFromLineEdit()
{
    // active tells us we have been triggered by
    // the setSmoothingFromSlider which will also
    // recalculates smoothing, lets not double up...
    if (active) return;
    else active = true;

    int value = smoothLineEdit->text().toInt();
    if (value != rtPlot->smooth) {
        smoothSlider->setValue(value);
        setSmoothing(value);
    }
    active = false;
}

void
RealtimePlotWindow::setShowPow30s(int value)
{
    showPow30s->setChecked(value);
    rtPlot->showPow30s(value);
    rtPlot->replot();
}

void
RealtimePlotWindow::setShowPower(int value)
{
    showPower->setChecked(value);
    rtPlot->showPower(value);
    rtPlot->replot();
}

void
RealtimePlotWindow::setShowHr(int value)
{
    showHr->setChecked(value);
    rtPlot->showHr(value);
    rtPlot->replot();
}

void
RealtimePlotWindow::setShowSpeed(int value)
{
    showSpeed->setChecked(value);
    rtPlot->showSpeed(value);
    rtPlot->replot();
}

void
RealtimePlotWindow::setShowCad(int value)
{
    showCad->setChecked(value);
    rtPlot->showCad(value);
    rtPlot->replot();
}

void
RealtimePlotWindow::setShowAlt(int value)
{
    showAlt->setChecked(value);
    rtPlot->showAlt(value);
    rtPlot->replot();
}

void
RealtimePlotWindow::setSmoothing(int value)
{
    hrtot = hrindex = cadtot = cadindex = spdtot = spdindex = alttot = altindex = powtot = powindex = 0;
    smoothSlider->setValue(value);
    rtPlot->setSmoothing(value);
}
