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

#include "TrainTool.h"
#include "MainWindow.h"
#include "ViewSelection.h"
#include "Settings.h"
#include "Units.h"
#include "DeviceTypes.h"
#include "DeviceConfiguration.h"
#include <assert.h>
#include <QApplication>
#include <QtGui>
#include <QRegExp>


TrainTool::TrainTool(MainWindow *parent, const QDir &home) : QWidget(parent), home(home), main(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    //XXX Commented out for this release
    //serverTree = new QTreeWidget;
    //serverTree->setColumnCount(1);
    //serverTree->setSelectionMode(QAbstractItemView::SingleSelection);
    //serverTree->header()->hide();
    //serverTree->setAlternatingRowColors (true);
    //serverTree->setIndentation(5);

    //allServers = new QTreeWidgetItem(serverTree, ROOT_TYPE);
    //allServers->setText(0, tr("Race Servers"));
    //serverTree->expandItem(allServers);
    //DeviceConfigurations all;
    //QList<DeviceConfiguration> Devices;
    //Devices = all.getList();
    //for (int i=0; i<Devices.count(); i++) {
    //    if (Devices.at(i).type == DEV_GSERVER) {
    //        QTreeWidgetItem *server = new QTreeWidgetItem(allServers, SERVER_TYPE);
    //        server->setText(0, Devices.at(i).name);
    //    }
    //}
    //connect(serverTree,SIGNAL(itemSelectionChanged()),
    //        this, SLOT(serverTreeWidgetSelectionChanged()));

    viewSelection = new ViewSelection(main, VIEW_TRAIN);

    workoutTree = new QTreeWidget;
    workoutTree->setColumnCount(1);
    workoutTree->setSelectionMode(QAbstractItemView::SingleSelection);
    workoutTree->header()->hide();
    workoutTree->setAlternatingRowColors (true);
    workoutTree->setIndentation(5);

    allWorkouts = new QTreeWidgetItem(workoutTree, ROOT_TYPE);
    allWorkouts->setText(0, tr("Workout Library"));
    workoutTree->expandItem(allWorkouts);

    configChanged(); // will reset the workout tree

    trainSplitter = new QSplitter;
    trainSplitter->setOrientation(Qt::Vertical);

    mainLayout->addWidget(trainSplitter);
    trainSplitter->addWidget(viewSelection);
    trainSplitter->setCollapsible(0, false);
    trainSplitter->addWidget(workoutTree);
    trainSplitter->setCollapsible(1, true);
    //trainSplitter->addWidget(serverTree);
    //trainSplitter->setCollapsible(2, true);

    connect(workoutTree,SIGNAL(itemSelectionChanged()),
            this, SLOT(workoutTreeWidgetSelectionChanged()));
    connect(main, SIGNAL(configChanged()),
            this, SLOT(configChanged()));


    // add a watch on all directories
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    QVariant workoutDir = settings->value(GC_WORKOUTDIR);
    watcher = boost::shared_ptr<QFileSystemWatcher>(new QFileSystemWatcher());
    watcher->addPaths(workoutDir.toStringList());

    connect(&*watcher,SIGNAL(directoryChanged(QString)),this,SLOT(configChanged()));
    connect(&*watcher,SIGNAL(fileChanged(QString)),this,SLOT(configChanged()));
}

void
TrainTool::configChanged()
{
    // zap whats there
    QList<QTreeWidgetItem *> workouts = allWorkouts->takeChildren();
    for (int i=0; i<workouts.count(); i++) delete workouts.at(i);

    // standard workouts - ergo and slope
    QTreeWidgetItem *ergomode = new QTreeWidgetItem(allWorkouts, WORKOUT_TYPE);
    ergomode->setText(0, tr("Manual Ergo Mode"));
    QTreeWidgetItem *slopemode = new QTreeWidgetItem(allWorkouts, WORKOUT_TYPE);
    slopemode->setText(0, tr("Manual Slope  Mode"));

    // add all the workouts in the library
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    QVariant workoutDir = settings->value(GC_WORKOUTDIR);
    QStringListIterator w(listWorkoutFiles(workoutDir.toString()));
    while (w.hasNext()) {
        QString name = w.next();
        QTreeWidgetItem *work = new QTreeWidgetItem(allWorkouts, WORKOUT_TYPE);
        work->setText(0, name);
    }
}

/*----------------------------------------------------------------------
 * Race Server or Workout Selected
 *----------------------------------------------------------------------*/
//void
//TrainTool::serverTreeWidgetSelectionChanged()
//{
//    serverSelected();
//}

void
TrainTool::workoutTreeWidgetSelectionChanged()
{
    assert(workoutTree->selectedItems().size() <= 1);
    if (workoutTree->selectedItems().isEmpty())
        workout = NULL;
    else {
        QTreeWidgetItem *which = workoutTree->selectedItems().first();
        if (which->type() != WORKOUT_TYPE)
            workout = NULL;
        else
            workout = which;
    }
    workoutSelected();
}

QStringList 
TrainTool::listWorkoutFiles(const QDir &dir) const
{
    QStringList filters;
    filters << "*.erg";
    filters << "*.mrc";
    filters << "*.crs";

    return dir.entryList(filters, QDir::Files, QDir::Name);
}
