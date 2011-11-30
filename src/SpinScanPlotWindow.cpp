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


#include "SpinScanPlotWindow.h"

SpinScanPlotWindow::SpinScanPlotWindow(MainWindow *mainWindow) :
    GcWindow(mainWindow), mainWindow(mainWindow)
{
    setContentsMargins(0,0,0,0);
    setInstanceName("SpinScan Plot");
    setControls(NULL);
    setProperty("color", GColor(CRIDEPLOTBACKGROUND));

    // init to zero
    history[0] = set1;
    history[1] = set2;
    history[2] = set3;
    history[3] = set4;
    history[4] = set5;
    history[5] = set6;
    history[6] = set7;
    history[7] = set8;
    history[8] = set8;
    history[9] = set9;
    history[10] = set11;
    history[11] = set12;
    history[12] = set13;
    history[13] = set14;
    history[14] = set15;
    history[15] = set16;

    QVBoxLayout *layout = new QVBoxLayout(this);
    rtPlot = new SpinScanPlot(spinData);
    layout->addWidget(rtPlot);

    // get updates..
    connect(mainWindow, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));
    connect(mainWindow, SIGNAL(start()), this, SLOT(start()));
    connect(mainWindow, SIGNAL(stop()), this, SLOT(stop()));

    // set to zero
    stop(); // resets the array
}

void
SpinScanPlotWindow::start()
{
    //resetValues();
    // init to zero
    for (int i=0; i<16; i++) memset(history[i], 0, 24);
    for (int i=0; i<24; i++) rtot[i] = 0;
    memset(spinData, 0, 24);
    current=0;
}

void
SpinScanPlotWindow::stop()
{
    //resetValues();
    // init to zero
    for (int i=0; i<16; i++) memset(history[i], 0, 24);
    for (int i=0; i<24; i++) rtot[i] = 0;
    memset(spinData, 0, 24);
    current=0;
}

void
SpinScanPlotWindow::pause()
{
}

void
SpinScanPlotWindow::telemetryUpdate(RealtimeData rtData)
{
    for (int i=0; i<24; i++) {
        rtot[i] += rtData.spinScan[i];
        rtot[i] -= history[current][i];
        spinData[i] = rtot[i]/16;
    }
    memcpy(history[current++], rtData.spinScan, 24);
    if (current==16) current=0;

    rtPlot->replot();                // redraw
}
