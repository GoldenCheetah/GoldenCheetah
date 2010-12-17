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

#include "MainWindow.h"
#include "ViewSelection.h"
#include <QDir>
#include <QtGui>

#define ROOT_TYPE    1
#define SERVER_TYPE  2
#define WORKOUT_TYPE 3

class TrainTool : public QWidget
{
    Q_OBJECT

    public:

        TrainTool(MainWindow *parent, const QDir &home);
        QStringList listWorkoutFiles(const QDir &) const;

        const QTreeWidgetItem *currentWorkout() { return workout; }
        const QTreeWidgetItem *workoutItems() { return allWorkouts; }
        //const QTreeWidgetItem *currentServer() { return server; }
        //const QTreeWidgetItem *serverItems() { return allServers; }

    signals:

        //void serverSelected();
        void workoutSelected();

    private slots:
        //void serverTreeWidgetSelectionChanged();
        void workoutTreeWidgetSelectionChanged();
        void configChanged();

    private:

        const QDir home;
        MainWindow *main;
        ViewSelection *viewSelection;

        QTreeWidget *workoutTree;
        QTabWidget  *trainTabs;
        QTreeWidgetItem *allWorkouts;
        QTreeWidgetItem *workout;
        QSplitter   *trainSplitter;
        boost::shared_ptr<QFileSystemWatcher> watcher;

        //QTreeWidget *serverTree;        // XXX commented out for this release
        //QTreeWidgetItem *allServers;    // XXX commented out for this release
        //QTreeWidgetItem *server;        // XXX commented out for this release
};

#endif // _GC_TrainTool_h

