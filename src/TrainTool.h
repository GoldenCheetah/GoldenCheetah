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

#ifndef _GC_TrainTool_h
#define _GC_TrainTool_h 1
#include "GoldenCheetah.h"

#include "MainWindow.h"
#include "ViewSelection.h"
#include "GoldenClient.h"
#include <QDir>
#include <QtGui>

#define HEAD_TYPE    6666
#define SERVER_TYPE  5555
#define WORKOUT_TYPE 4444
#define DEVICE_TYPE  0

class TrainTool : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        TrainTool(MainWindow *parent, const QDir &home);
        QStringList listWorkoutFiles(const QDir &) const;

        const QTreeWidgetItem *currentWorkout() { return workout; }
        const QTreeWidgetItem *workoutItems() { return allWorkouts; }
        const QTreeWidgetItem *currentServer() { return server; }
        const QTreeWidgetItem *serverItems() { return allServers; }

        int selectedDeviceNumber();
        int selectedServerNumber();

        // button meanings are changed by the windows
        void setStartText(QString);
        void setPauseText(QString);

        // notify widgets of race update
        void notifyRaceStandings(RaceStatus x) { raceStandings(x); }

        // this
        QTabWidget  *trainTabs;
    signals:

        void deviceSelected();
        void serverSelected();
        void workoutSelected();
        void raceStandings(RaceStatus);
        void start();
        void pause();
        void stop();

    private slots:
        void serverTreeWidgetSelectionChanged();
        void deviceTreeWidgetSelectionChanged();
        void workoutTreeWidgetSelectionChanged();
        void configChanged();
        void Start();
        void Pause();
        void Stop();

    private:

        const QDir home;
        MainWindow *main;
        ViewSelection *viewSelection;

        QTreeWidget *workoutTree;
        QTreeWidget *deviceTree;
        QTreeWidgetItem *allWorkouts;
        QTreeWidgetItem *workout;
        QSplitter   *trainSplitter;
        QTreeWidget *serverTree;
        QTreeWidgetItem *allServers;
        QTreeWidgetItem *allDevices;
        QTreeWidgetItem *server;

        // those buttons
        QFrame *buttonPanel;
        QPushButton *startButton,
                    *pauseButton,
                    *stopButton;

    public:
        // everyone else wants this
        QCheckBox   *recordSelector;
        boost::shared_ptr<QFileSystemWatcher> watcher;
};

#endif // _GC_TrainTool_h

