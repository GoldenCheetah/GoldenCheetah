/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_RealtimePlotWindow_h
#define _GC_RealtimePlotWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QObject> // for Q_PROPERTY


#include "MainWindow.h"
#include "RideFile.h" // for data series types
#include "RealtimePlot.h"
#include "RealtimeData.h" // for realtimedata structure

#include "Settings.h"
#include "Colors.h"

class RealtimePlotWindow : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    public:

        RealtimePlotWindow(MainWindow *mainWindow);

   public slots:

        // trap signals
        void telemetryUpdate(RealtimeData rtData); // got new data
        void start();
        void stop();
        void pause();

    private:

        MainWindow *mainWindow;
        RealtimePlot *rtPlot;
};

#endif // _GC_RealtimePlotWindow_h
