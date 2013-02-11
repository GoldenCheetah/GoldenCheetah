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

#ifndef _GC_RaceWindow_h
#define _GC_RaceWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QTimer>
#include "MainWindow.h"
#include "DeviceConfiguration.h"
#include "DeviceTypes.h"
#include "RealtimeData.h"

#include "RaceCourse.h"
#include "RaceLeaderboard.h"
#include "RaceRiders.h"

#include "TrainTool.h"

class RaceWindow : public GcWindow
{
    Q_OBJECT
    G_OBJECT


    public:

        RealtimeController *deviceController;   // read from

        RaceWindow(MainWindow *, TrainTool *, const QDir &);

    public slots:

    protected:


        // passed from MainWindow
        QDir home;
        MainWindow *main;
        TrainTool *trainTool;

        QSplitter *splitter;
        RaceLeaderboard *raceLeaderboard;
        RaceCourse *raceCourse;
        RaceRiders *raceRiders;
};

#endif // _GC_RaceWindow_h

