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

#ifndef _GC_MultiWindow_h
#define _GC_MultiWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QTimer>
#include "MainWindow.h"
#include "DeviceConfiguration.h"
#include "DeviceTypes.h"

class TrainTool;
class TrainTabs;
class RealtimeController;

class MultiWindow : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        MultiWindow(MainWindow *, TrainTool *, const QDir &);

    public slots:

    protected:


        // passed from MainWindow
        QDir home;
        MainWindow *main;
        TrainTool *trainTool;

};

#endif // _GC_MultiWindow_h

