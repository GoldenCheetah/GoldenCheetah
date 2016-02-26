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

#include "RaceWindow.h"
#include "MainWindow.h"
#include "RealtimeData.h"
#include "RaceDispatcher.h"
#include "math.h" // for round()
#include "Units.h" // for MILES_PER_KM

// Two current realtime device types supported are:
#include "ComputrainerController.h"
#include "ANTplusController.h"

#include "TrainTool.h"

RaceWindow::RaceWindow(MainWindow *parent, TrainTool *trainTool, const QDir &home)  :
GcWindow(parent), home(home), main(parent), trainTool(trainTool)
{
    setInstanceName("Race Window");
    setControls(NULL);

    // the widgets
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
    setLayout(mainLayout);
    setContentsMargins(0,0,0,0);

    // the widgets
    //raceLeaderboard = new RaceLeaderboard(this, trainTool);
    raceCourse = new RaceCourse(this, trainTool);
    QGraphicsView *course = new QGraphicsView(raceCourse);
    course->fitInView(QRectF(0, 0, 10000, 10000), Qt::KeepAspectRatio);
    course->setAlignment(Qt::AlignLeft|Qt::AlignTop);
    raceRiders = new RaceRiders(this, trainTool);

    // top half of the screen
    QFrame *topFrame = new QFrame;
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->setSpacing(0);
    topLayout->setContentsMargins(0,0,0,0);
    topFrame->setLineWidth(1);
    topFrame->setFrameStyle(QFrame::Box | QFrame::Raised);
    topFrame->setContentsMargins(0,0,0,0);
    topFrame->setLayout(topLayout);
    topLayout->addWidget(course);
    //topLayout->addWidget(raceLeaderboard);

    splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(topFrame);
    splitter->addWidget(raceRiders);
    splitter->setContentsMargins(0,0,0,0);

    mainLayout->addWidget(splitter);

}
