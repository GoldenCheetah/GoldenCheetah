/*
 * Copyright (c) 2000 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_TrainTabs_h
#define _GC_TrainTabs_h 1
#include "GoldenCheetah.h"

#include <QDir>
#include <QtGui>

#include "MainWindow.h"
#include "TrainTool.h"
#include "RealtimeWindow.h"
#include "RaceWindow.h"
#include "MultiWindow.h"

#ifdef GC_HAVE_PHONON
#include "VideoWindow.h"
#endif

class TrainTabs : public QTabWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        TrainTabs(MainWindow *parent, TrainTool *trainTool, const QDir &home);

    private:
        RealtimeWindow  *realtimeWindow;

        TrainTool *trainTool;
        const QDir home;
        const MainWindow *main;
};

#endif // _GC_TrainTabs_h
