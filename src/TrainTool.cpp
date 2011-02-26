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

    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    //setLineWidth(1);
    //setMidLineWidth(0);
    //setFrameStyle(QFrame::Plain | QFrame::Sunken);
    setContentsMargins(0,0,0,0);


    serverTree = new QTreeWidget;
    serverTree->setColumnCount(1);
    serverTree->setSelectionMode(QAbstractItemView::SingleSelection);
    serverTree->header()->hide();
    serverTree->setAlternatingRowColors (true);
    serverTree->setIndentation(5);
    allServers = new QTreeWidgetItem(serverTree, HEAD_TYPE);
    allServers->setText(0, tr("Race Servers"));
    serverTree->expandItem(allServers);

    deviceTree = new QTreeWidget;
    deviceTree->setSelectionMode(QAbstractItemView::MultiSelection);
    deviceTree->setColumnCount(1);
    deviceTree->header()->hide();
    deviceTree->setAlternatingRowColors (true);
    deviceTree->setIndentation(5);
    allDevices = new QTreeWidgetItem(deviceTree, HEAD_TYPE);
    allDevices->setText(0, tr("Devices"));
    deviceTree->expandItem(allDevices);

    workoutTree = new QTreeWidget;
    workoutTree->setColumnCount(1);
    workoutTree->setSelectionMode(QAbstractItemView::SingleSelection);
    workoutTree->header()->hide();
    workoutTree->setAlternatingRowColors (true);
    workoutTree->setIndentation(5);

    allWorkouts = new QTreeWidgetItem(workoutTree, HEAD_TYPE);
    allWorkouts->setText(0, tr("Workout Library"));
    workoutTree->expandItem(allWorkouts);

    configChanged(); // will reset the workout tree

    buttonPanel = new QFrame;
    buttonPanel->setLineWidth(1);
    buttonPanel->setFrameStyle(QFrame::Box | QFrame::Raised);
    buttonPanel->setContentsMargins(0,0,0,0);
    QVBoxLayout *panel = new QVBoxLayout;
    QHBoxLayout *buttons = new QHBoxLayout;
    startButton = new QPushButton(tr("Start"), this);
    startButton->setMaximumHeight(100);
    pauseButton = new QPushButton(tr("Pause"), this);
    pauseButton->setMaximumHeight(100);
    stopButton = new QPushButton(tr("Stop"), this);
    stopButton->setMaximumHeight(100);
    recordSelector = new QCheckBox(this);
    recordSelector->setText(tr("Save workout data"));
    recordSelector->setChecked(Qt::Checked);
    buttons->addWidget(startButton);
    buttons->addWidget(pauseButton);
    buttons->addWidget(stopButton);
    panel->addLayout(buttons);
    panel->addWidget(recordSelector);
    buttonPanel->setLayout(panel);
    buttonPanel->setFixedHeight(90);

    trainSplitter = new QSplitter;
    trainSplitter->setOrientation(Qt::Vertical);
    trainSplitter->setContentsMargins(0,0,0,0);
    trainSplitter->setLineWidth(0);
    trainSplitter->setMidLineWidth(0);

    mainLayout->addWidget(trainSplitter);
    trainSplitter->addWidget(buttonPanel);
    trainSplitter->setCollapsible(0, false);
    trainSplitter->addWidget(deviceTree);
    trainSplitter->setCollapsible(1, true);
    trainSplitter->addWidget(serverTree);
    trainSplitter->setCollapsible(2, true);
    trainSplitter->addWidget(workoutTree);
    trainSplitter->setCollapsible(3, true);

    connect(serverTree,SIGNAL(itemSelectionChanged()),
            this, SLOT(serverTreeWidgetSelectionChanged()));
    connect(deviceTree,SIGNAL(itemSelectionChanged()),
            this, SLOT(deviceTreeWidgetSelectionChanged()));
    connect(workoutTree,SIGNAL(itemSelectionChanged()),
            this, SLOT(workoutTreeWidgetSelectionChanged()));
    connect(main, SIGNAL(configChanged()),
            this, SLOT(configChanged()));

    // connect train tool buttons!
    connect(startButton, SIGNAL(clicked()), this, SLOT(Start()));
    connect(pauseButton, SIGNAL(clicked()), this, SLOT(Pause()));
    connect(stopButton, SIGNAL(clicked()), this, SLOT(Stop()));

    // add a watch on all directories
    QVariant workoutDir = appsettings->value(NULL, GC_WORKOUTDIR);
    watcher = boost::shared_ptr<QFileSystemWatcher>(new QFileSystemWatcher());
    watcher->addPaths(workoutDir.toStringList());

    connect(&*watcher,SIGNAL(directoryChanged(QString)),this,SLOT(configChanged()));
    connect(&*watcher,SIGNAL(fileChanged(QString)),this,SLOT(configChanged()));
}

void
TrainTool::configChanged()
{

    // SERVERS & DEVICES
    // zap whats there
    QList<QTreeWidgetItem *> servers = allServers->takeChildren();
    for (int i=0; i<servers.count(); i++) delete servers.at(i);
    QList<QTreeWidgetItem *> devices = allDevices->takeChildren();
    for (int i=0; i<devices.count(); i++) delete devices.at(i);

    DeviceConfigurations all;
    QList<DeviceConfiguration> Devices;
    Devices = all.getList();
    for (int i=0; i<Devices.count(); i++) {
        if (Devices.at(i).type == DEV_GSERVER) {
            QTreeWidgetItem *server = new QTreeWidgetItem(allServers, i);
            server->setText(0, Devices.at(i).name);
        } else {
            QTreeWidgetItem *device = new QTreeWidgetItem(allDevices, i);
            device->setText(0, Devices.at(i).name);
        }
    }

    // WORKOUTS
    // zap whats there
    QList<QTreeWidgetItem *> workouts = allWorkouts->takeChildren();
    for (int i=0; i<workouts.count(); i++) delete workouts.at(i);

    // standard workouts - ergo and slope
    QTreeWidgetItem *ergomode = new QTreeWidgetItem(allWorkouts, WORKOUT_TYPE);
    ergomode->setText(0, tr("Manual Ergo Mode"));
    QTreeWidgetItem *slopemode = new QTreeWidgetItem(allWorkouts, WORKOUT_TYPE);
    slopemode->setText(0, tr("Manual Slope  Mode"));

    // add all the workouts in the library
    QVariant workoutDir = appsettings->value(this, GC_WORKOUTDIR);
    QStringListIterator w(listWorkoutFiles(workoutDir.toString()));
    while (w.hasNext()) {
        QString name = w.next();
        QTreeWidgetItem *work = new QTreeWidgetItem(allWorkouts, WORKOUT_TYPE);
        work->setText(0, name);
    }
}

/*----------------------------------------------------------------------
 * Buttons!
 *----------------------------------------------------------------------*/
void
TrainTool::Start()
{
    start();
}

void
TrainTool::Stop()
{
    stop();
}

void
TrainTool::Pause()
{
    pause();
}

void
TrainTool::setStartText(QString string)
{
    startButton->setText(string);
}
void
TrainTool::setPauseText(QString string)
{
    pauseButton->setText(string);
}

/*----------------------------------------------------------------------
 * Race Server Selected
 *----------------------------------------------------------------------*/
void
TrainTool::serverTreeWidgetSelectionChanged()
{
    serverSelected();
}

int
TrainTool::selectedServerNumber()
{
    if (serverTree->selectedItems().isEmpty()) return -1;

    QTreeWidgetItem *selected = serverTree->selectedItems().first();

    if (selected->type() == HEAD_TYPE) return -1;
    else return selected->type();
}

/*----------------------------------------------------------------------
 * Device Selected
 *--------------------------------------------------------------------*/
void
TrainTool::deviceTreeWidgetSelectionChanged()
{
    deviceSelected();
}

int
TrainTool::selectedDeviceNumber()
{
    if (deviceTree->selectedItems().isEmpty()) return -1;

    QTreeWidgetItem *selected = deviceTree->selectedItems().first();

    if (selected->type() == HEAD_TYPE) return -1;
    else return selected->type();
}

/*----------------------------------------------------------------------
 * Workout Selected
 *--------------------------------------------------------------------*/
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
