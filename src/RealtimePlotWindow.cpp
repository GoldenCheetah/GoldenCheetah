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
    GcWindow(mainWindow), mainWindow(mainWindow)
{
    setContentsMargins(0,0,0,0);
    setInstanceName("RT Plot");
    setControls(NULL);
    setProperty("color", GColor(CRIDEPLOTBACKGROUND));

    QVBoxLayout *layout = new QVBoxLayout(this);
    rtPlot = new RealtimePlot();
    layout->addWidget(rtPlot);

    // get updates..
    connect(mainWindow, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));

    // set to zero
    telemetryUpdate(RealtimeData());
}

void
RealtimePlotWindow::start()
{
    //resetValues();
}

void
RealtimePlotWindow::stop()
{
    //resetValues();
}

void
RealtimePlotWindow::pause()
{
}

void
RealtimePlotWindow::telemetryUpdate(RealtimeData rtData)
{
    rtPlot->pwrData.addData(rtData.value(RealtimeData::Watts));
    rtPlot->cadData.addData(rtData.value(RealtimeData::Cadence));
    rtPlot->spdData.addData(rtData.value(RealtimeData::Speed));
    rtPlot->hrData.addData(rtData.value(RealtimeData::HeartRate));
    rtPlot->replot();                // redraw
}
