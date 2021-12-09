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

#include "TrainSidebar.h"
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"
#include "DeviceTypes.h"
#include "DeviceConfiguration.h"
#include "RideImportWizard.h"
#include "HelpWhatsThis.h"
#include <QApplication>
#include <QtGui>
#include <QRegExp>
#include <QStyle>
#include <QStyleFactory>
#include <QScrollBar>

#include <QEvent>
#include <QInputEvent>
#include <QKeyEvent>

#include <QSound>

// Three current realtime device types supported are:
#include "RealtimeController.h"
#include "ComputrainerController.h"
#include "MonarkController.h"
#include "KettlerController.h"
#include "KettlerRacerController.h"
#include "ErgofitController.h"
#include "DaumController.h"
#include "ANTlocalController.h"
#include "NullController.h"
#ifdef QT_BLUETOOTH_LIB
#include "BT40Controller.h"
#endif
#ifdef GC_HAVE_LIBUSB
#include "FortiusController.h"
#include "ImagicController.h"
#endif

// Media selection helper
#if defined(GC_VIDEO_AV) || defined(GC_VIDEO_QUICKTIME)
#include "QtMacVideoWindow.h"
#else
#include "VideoWindow.h"
#endif
#ifdef Q_OS_MAC
#include <CoreServices/CoreServices.h>
#include <QStyle>
#include <QStyleFactory>
#import <IOKit/pwr_mgt/IOPMLib.h>
#endif

#ifdef WIN32
#include "windows.h"
#endif

#include "TrainDB.h"
#include "Library.h"

TrainSidebar::TrainSidebar(Context *context) : GcWindow(context), context(context),
    bicycle(context)
{
    QWidget *c = new QWidget;
    //c->setContentsMargins(0,0,0,0); // bit of space is useful
    QVBoxLayout *cl = new QVBoxLayout(c);
    setControls(c);
    autoConnect = false;
    trainView=NULL;
    useSimulatedSpeed = false;

    cl->setSpacing(0);
    cl->setContentsMargins(0,0,0,0);

    // don't set the source for telemetry
    lastAppliedIntensity = 100;
    bpmTelemetry = wattsTelemetry = kphTelemetry = rpmTelemetry = -1;

#if !defined GC_VIDEO_NONE
    videoModel = new QSqlTableModel(this, trainDB->connection());
    videoModel->setTable("videos");
    videoModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    videoModel->select();
    while (videoModel->canFetchMore(QModelIndex())) videoModel->fetchMore(QModelIndex());

    vsortModel = new QSortFilterProxyModel(this);
    vsortModel->setSourceModel(videoModel);
    vsortModel->setDynamicSortFilter(true);
    vsortModel->sort(1, Qt::AscendingOrder); //filename order

    mediaTree = new QTreeView;
    mediaTree->setModel(vsortModel);

    // hide unwanted columns and header
    for(int i=0; i<mediaTree->header()->count(); i++)
        mediaTree->setColumnHidden(i, true);
    mediaTree->setColumnHidden(1, false); // show filename
    mediaTree->header()->hide();

    mediaTree->setSortingEnabled(false);
    mediaTree->setAlternatingRowColors(false);
    mediaTree->setEditTriggers(QAbstractItemView::NoEditTriggers); // read-only
    mediaTree->expandAll();
    mediaTree->header()->setCascadingSectionResizes(true); // easier to resize this way
    mediaTree->setContextMenuPolicy(Qt::CustomContextMenu);
    mediaTree->header()->setStretchLastSection(true);
    mediaTree->header()->setMinimumSectionSize(0);
    mediaTree->header()->setFocusPolicy(Qt::NoFocus);
    mediaTree->setFrameStyle(QFrame::NoFrame);
#ifdef Q_OS_MAC
    mediaTree->header()->setSortIndicatorShown(false); // blue looks nasty
    mediaTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
#ifdef Q_OS_WIN
    QStyle *cde = QStyleFactory::create(OS_STYLE);
    mediaTree->verticalScrollBar()->setStyle(cde);
#endif

#ifdef GC_HAVE_VLC  // RLV currently only support for VLC
    videosyncModel = new QSqlTableModel(this, trainDB->connection());
    videosyncModel->setTable("videosyncs");
    videosyncModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    videosyncModel->select();
    while (videosyncModel->canFetchMore(QModelIndex())) videosyncModel->fetchMore(QModelIndex());

    vssortModel = new QSortFilterProxyModel(this);
    vssortModel->setSourceModel(videosyncModel);
    vssortModel->setDynamicSortFilter(true);
    vssortModel->sort(1, Qt::AscendingOrder); //filename order

    videosyncTree = new QTreeView;
    videosyncTree->setModel(vssortModel);

    // hide unwanted columns and header
    for(int i=0; i<videosyncTree->header()->count(); i++)
        videosyncTree->setColumnHidden(i, true);
    videosyncTree->setColumnHidden(1, false); // show filename
    videosyncTree->header()->hide();

    videosyncTree->setSortingEnabled(false);
    videosyncTree->setAlternatingRowColors(false);
    videosyncTree->setEditTriggers(QAbstractItemView::NoEditTriggers); // read-only
    videosyncTree->expandAll();
    videosyncTree->header()->setCascadingSectionResizes(true); // easier to resize this way
    videosyncTree->setContextMenuPolicy(Qt::CustomContextMenu);
    videosyncTree->header()->setStretchLastSection(true);
    videosyncTree->header()->setMinimumSectionSize(0);
    videosyncTree->header()->setFocusPolicy(Qt::NoFocus);
    videosyncTree->setFrameStyle(QFrame::NoFrame);
#ifdef Q_OS_MAC
    videosyncTree->header()->setSortIndicatorShown(false); // blue looks nasty
    videosyncTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
#ifdef Q_OS_WIN
    QStyle *cdevideosync = QStyleFactory::create(OS_STYLE);
    videosyncTree->verticalScrollBar()->setStyle(cdevideosync);
#endif

#endif //GC_HAVE_VLC

#endif //GC_VIDEO_NONE

    deviceTree = new DeviceTreeView;
    deviceTree->setFrameStyle(QFrame::NoFrame);
    if (appsettings->value(this, TRAIN_MULTI, false).toBool() == true)
        deviceTree->setSelectionMode(QAbstractItemView::MultiSelection);
    else
        deviceTree->setSelectionMode(QAbstractItemView::SingleSelection);
    deviceTree->setColumnCount(1);
    deviceTree->header()->hide();
    deviceTree->setAlternatingRowColors (false);
    deviceTree->setIndentation(5);
    deviceTree->expandItem(deviceTree->invisibleRootItem());
    deviceTree->setContextMenuPolicy(Qt::CustomContextMenu);
#ifdef Q_OS_WIN
    QStyle *xde = QStyleFactory::create(OS_STYLE);
    deviceTree->verticalScrollBar()->setStyle(xde);
#endif

    workoutModel = new QSqlTableModel(this, trainDB->connection());
    workoutModel->setTable("workouts");
    workoutModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    workoutModel->select();
    while (workoutModel->canFetchMore(QModelIndex())) workoutModel->fetchMore(QModelIndex());

    sortModel = new QSortFilterProxyModel(this);
    sortModel->setSourceModel(workoutModel);
    sortModel->setDynamicSortFilter(true);
    sortModel->sort(1, Qt::AscendingOrder); //filename order

    workoutTree = new QTreeView;
    workoutTree->setModel(sortModel);

    // hide unwanted columns and header
    for(int i=0; i<workoutTree->header()->count(); i++)
        workoutTree->setColumnHidden(i, true);
    workoutTree->setColumnHidden(1, false); // show filename
    workoutTree->header()->hide();
    workoutTree->setFrameStyle(QFrame::NoFrame);
    workoutTree->setAlternatingRowColors(false);
    workoutTree->setEditTriggers(QAbstractItemView::NoEditTriggers); // read-only
    workoutTree->expandAll();
    workoutTree->header()->setCascadingSectionResizes(true); // easier to resize this way
    workoutTree->setContextMenuPolicy(Qt::CustomContextMenu);
    workoutTree->header()->setStretchLastSection(true);
    workoutTree->header()->setMinimumSectionSize(0);
    workoutTree->header()->setFocusPolicy(Qt::NoFocus);
    workoutTree->setFrameStyle(QFrame::NoFrame);
#ifdef Q_OS_MAC
    workoutTree->header()->setSortIndicatorShown(false); // blue looks nasty
    workoutTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
#ifdef Q_OS_WIN
    xde = QStyleFactory::create(OS_STYLE);
    workoutTree->verticalScrollBar()->setStyle(xde);
#endif

    connect(context, SIGNAL(newLap()), this, SLOT(resetLapTimer()));
    connect(context, SIGNAL(viewChanged(int)), this, SLOT(viewChanged(int)));

    // not used but kept in case re-instated in the future
    recordSelector = new QCheckBox(this);
    recordSelector->setText(tr("Save workout data"));
    recordSelector->setChecked(true);
    recordSelector->hide(); // we don't let users change this for now

    trainSplitter = new GcSplitter(Qt::Vertical);
    trainSplitter->setContentsMargins(0,0,0,0);
    deviceItem = new GcSplitterItem(tr("Devices"), iconFromPNG(":images/sidebar/power.png"), this);

    // devices splitter actions
    QAction *moreDeviceAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    deviceItem->addAction(moreDeviceAct);
    connect(moreDeviceAct, SIGNAL(triggered(void)), this, SLOT(devicePopup(void)));

    workoutItem = new GcSplitterItem(tr("Workouts"), iconFromPNG(":images/sidebar/folder.png"), this);
    QAction *moreWorkoutAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    workoutItem->addAction(moreWorkoutAct);
    connect(moreWorkoutAct, SIGNAL(triggered(void)), this, SLOT(workoutPopup(void)));

    deviceItem->addWidget(deviceTree);
    HelpWhatsThis *helpDeviceTree = new HelpWhatsThis(deviceTree);
    deviceTree->setWhatsThis(helpDeviceTree->getWhatsThisText(HelpWhatsThis::SideBarTrainView_Devices));
    trainSplitter->addWidget(deviceItem);

    workoutItem->addWidget(workoutTree);
    HelpWhatsThis *helpWorkoutTree = new HelpWhatsThis(workoutTree);
    workoutTree->setWhatsThis(helpWorkoutTree->getWhatsThisText(HelpWhatsThis::SideBarTrainView_Workouts));
    trainSplitter->addWidget(workoutItem);

    cl->addWidget(trainSplitter);


#if !defined GC_VIDEO_NONE
    mediaItem = new GcSplitterItem(tr("Media"), iconFromPNG(":images/sidebar/movie.png"), this);
    QAction *moreMediaAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    mediaItem->addAction(moreMediaAct);
    connect(moreMediaAct, SIGNAL(triggered(void)), this, SLOT(mediaPopup(void)));
    mediaItem->addWidget(mediaTree);
    HelpWhatsThis *helpMediaTree = new HelpWhatsThis(mediaTree);
    mediaTree->setWhatsThis(helpMediaTree->getWhatsThisText(HelpWhatsThis::SideBarTrainView_Media));
    trainSplitter->addWidget(mediaItem);

#ifdef GC_HAVE_VLC  // RLV currently only support for VLC
    videosyncItem = new GcSplitterItem(tr("VideoSync"), iconFromPNG(":images/sidebar/sync.png"), this);
    QAction *moreVideoSyncAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    videosyncItem->addAction(moreVideoSyncAct);
    connect(moreVideoSyncAct, SIGNAL(triggered(void)), this, SLOT(videosyncPopup(void)));
    videosyncItem->addWidget(videosyncTree);
    HelpWhatsThis *helpVideosyncTree = new HelpWhatsThis(videosyncTree);
    videosyncTree->setWhatsThis(helpVideosyncTree->getWhatsThisText(HelpWhatsThis::SideBarTrainView_VideoSync));
    trainSplitter->addWidget(videosyncItem);
#endif //GC_HAVE_VLC
#endif //GC_VIDEO_NONE
    trainSplitter->prepare(context->athlete->cyclist, "train");

#ifdef Q_OS_MAC
    // get rid of annoying focus rectangle for sidebar components
#if !defined GC_VIDEO_NONE
    mediaTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#if defined GC_HAVE_VLC // not on OSX at present
    videosyncTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
#endif
    workoutTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
    deviceTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

    // handle config changes
    connect(deviceTree,SIGNAL(itemSelectionChanged()), this, SLOT(deviceTreeWidgetSelectionChanged()));
    connect(deviceTree,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(deviceTreeMenuPopup(const QPoint &)));
    connect(deviceTree,SIGNAL(itemMoved(int, int)), this, SLOT(moveDevices(int, int)));

#if !defined GC_VIDEO_NONE
    connect(mediaTree->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
                            this, SLOT(mediaTreeWidgetSelectionChanged()));
    connect(context, SIGNAL(selectMedia(QString)), this, SLOT(selectVideo(QString)));
#ifdef GC_HAVE_VLC  // RLV currently only support for VLC
    connect(videosyncTree->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
                            this, SLOT(videosyncTreeWidgetSelectionChanged()));
    connect(context, SIGNAL(selectVideoSync(QString)), this, SLOT(selectVideoSync(QString)));
#endif //GC_HAVE_VLC
#endif //GC_VIDEO_NONE
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(selectWorkout(QString)), this, SLOT(selectWorkout(QString)));
    connect(trainDB, SIGNAL(dataChanged()), this, SLOT(refresh()));

    connect(workoutTree->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(workoutTreeWidgetSelectionChanged()));

    videosyncFile = NULL;
    calibrating = false;

    // Remote control support
    remote = new RemoteControl;

    // now the GUI is setup lets sort our control variables
    gui_timer = new QTimer(this);
    disk_timer = new QTimer(this);
    load_timer = new QTimer(this);
    start_timer = new QTimer(this);
    start_timer->setSingleShot(true);

    session_time = QTime();
    session_elapsed_msec = 0;
    lap_time = QTime();
    lap_elapsed_msec = 0;
    secs_to_start = 0;

    rrFile = recordFile = vo2File = NULL;
    lastRecordSecs = 0;
    status = 0;
    setStatusFlags(RT_MODE_ERGO);         // ergo mode by default
    mode = ERG;
    pendingConfigChange = false;

    displayWorkoutLap = 0;
    pwrcount = 0;
    cadcount = 0;
    hrcount = 0;
    spdcount = 0;
    lodcount = 0;
    wbalr = wbal = 0;
    load_msecs = total_msecs = lap_msecs = 0;
    displayWorkoutDistance = displayDistance = displayPower = displayHeartRate =
    displaySpeed = displayCadence = slope = load = 0;
    displaySMO2 = displayTHB = displayO2HB = displayHHB = 0;
    displayLRBalance = displayLTE = displayRTE = displayLPS = displayRPS = 0;
    displayLatitude = displayLongitude = displayAltitude = 0.0;

    connect(gui_timer, SIGNAL(timeout()), this, SLOT(guiUpdate()));
    connect(disk_timer, SIGNAL(timeout()), this, SLOT(diskUpdate()));
    connect(load_timer, SIGNAL(timeout()), this, SLOT(loadUpdate()));
    connect(start_timer, SIGNAL(timeout()), this, SLOT(Start()));

    configChanged(CONFIG_APPEARANCE | CONFIG_DEVICES | CONFIG_ZONES); // will reset the workout tree
    setLabels();

    // capture keyboard events so we can control during
    // a workout using basic keyboard controls
    context->mainWindow->installEventFilter(this);

#ifndef Q_OS_MAC
    //toolbarButtons->hide();
#endif

}

void
TrainSidebar::refresh()
{
    int row;

#if !defined GC_VIDEO_NONE
    // remember selection
    row = mediaTree->currentIndex().row();
    QString videoPath = mediaTree->model()->data(mediaTree->model()->index(row,0)).toString();

#ifdef GC_HAVE_VLC  // RLV currently only support for VLC
    // refresh data
    videoModel->select();
    while (videoModel->canFetchMore(QModelIndex())) videoModel->fetchMore(QModelIndex());
#endif

    // restore selection
    selectVideo(videoPath);

#ifdef GC_HAVE_VLC  // RLV currently only support for VLC
    // remember selection
    row = videosyncTree->currentIndex().row();
    QString videosyncPath = videosyncTree->model()->data(videosyncTree->model()->index(row,0)).toString();

    // refresh data
    videosyncModel->select();
    while (videosyncModel->canFetchMore(QModelIndex())) videosyncModel->fetchMore(QModelIndex());

    // restore selection
    selectVideoSync(videosyncPath);
#endif // GC_HAVE_VLC

#endif // GC_VIDEO_NONE

    row = workoutTree->currentIndex().row();
    QString workoutPath = workoutTree->model()->data(workoutTree->model()->index(row,0)).toString();

    workoutModel->select();
    while (workoutModel->canFetchMore(QModelIndex())) workoutModel->fetchMore(QModelIndex());

    // restore selection
    selectWorkout(workoutPath);
}

void
TrainSidebar::workoutPopup()
{
    // OK - we are working with a specific event..
    QMenu menu(workoutTree);
    QAction *import = new QAction(tr("Import Workout from File"), workoutTree);
    QAction *download = new QAction(tr("Get Workouts from ErgDB"), workoutTree);
    QAction *dlTodaysPlan = new QAction(tr("Get Workouts from Today's Plan"), workoutTree);
    QAction *wizard = new QAction(tr("Create Workout via Wizard"), workoutTree);
    QAction *scan = new QAction(tr("Scan for Workouts"), workoutTree);

    menu.addAction(import);
    menu.addAction(download);
    menu.addAction(dlTodaysPlan);
    menu.addAction(wizard);
    menu.addAction(scan);


    // we can delete too
    int delNumber = 0;

    QModelIndexList list = workoutTree->selectionModel()->selectedRows();
    foreach (QModelIndex index, list)
    {
        QModelIndex target = sortModel->mapToSource(index);

        QString filename = workoutModel->data(workoutModel->index(target.row(), 0), Qt::DisplayRole).toString();
        if (QFileInfo(filename).exists()) {
            delNumber++;
        }
    }

    if (delNumber > 0) {
        QAction *del = new QAction(tr("Delete selected Workout"), workoutTree);

        if (delNumber > 1) {
            del->setText(QString(tr("Delete %1 selected Workouts")).arg(delNumber));
        }
        menu.addAction(del);
        connect(del, SIGNAL(triggered(void)), this, SLOT(deleteWorkouts(void)));
    }

    // connect menu to functions
    connect(import, SIGNAL(triggered(void)), context->mainWindow, SLOT(importWorkout(void)));
    connect(wizard, SIGNAL(triggered(void)), context->mainWindow, SLOT(showWorkoutWizard(void)));
    connect(download, SIGNAL(triggered(void)), context->mainWindow, SLOT(downloadErgDB(void)));
    connect(dlTodaysPlan, SIGNAL(triggered(void)), context->mainWindow, SLOT(downloadTodaysPlanWorkouts(void)));
    connect(scan, SIGNAL(triggered(void)), context->mainWindow, SLOT(manageLibrary(void)));

    // execute the menu
    menu.exec(trainSplitter->mapToGlobal(QPoint(workoutItem->pos().x()+workoutItem->width()-20,
                                           workoutItem->pos().y())));
}

bool
TrainSidebar::eventFilter(QObject *, QEvent *event)
{
    // do not allow to close the Window when active
    if (event->type() == QEvent::Close) {
        if (status & RT_RUNNING) {
            QMessageBox::warning(this, tr("Train mode active"), tr("Please stop the train mode before closing the window or application."));
            event->ignore();
            return true;
        } else if (gui_timer->isActive()) {
            // we just disconnecting before allowing the window to close
            Disconnect();
            return false;
        }
    }

    // only when we are recording !
    if (status & RT_RECORDING) {
#if 0
        if (event->type() == QEvent::KeyPress) {

            // we care about cmd / ctrl
            Qt::KeyboardModifiers kmod = static_cast<QInputEvent*>(event)->modifiers();
            bool ctrl = (kmod & Qt::ControlModifier) != 0;
            Q_UNUSED(ctrl);

            // what was pressed
            int key =static_cast<QKeyEvent*>(event)->key();

            //
            // We can process the keyboard event here
            // and call any training method
            //
            // KEY        FUNCTION
            //            Start() - will pause/unpause if running
            //            Stop() - will end workout
            //            Pause() - pause only
            //            UnPause() - unpause
            //            FFwd() - nip forward
            //            Rewind() - nip back
            //            newLap() - new lap marker
            //
            switch(key) {

                //XXX TODO

                default:
                break;

            }
            return true; // we listen to 'em all
        }
#endif
    }
    return false;
}

void
TrainSidebar::mediaPopup()
{
    // OK - we are working with a specific event..
    QMenu menu(mediaTree);
    QAction *import = new QAction(tr("Import Video from File"), mediaTree);
    QAction *scan = new QAction(tr("Scan for Videos"), mediaTree);

    menu.addAction(import);
    menu.addAction(scan);

    // connect menu to functions
    connect(import, SIGNAL(triggered(void)), context->mainWindow, SLOT(importWorkout(void)));
    connect(scan, SIGNAL(triggered(void)), context->mainWindow, SLOT(manageLibrary(void)));

    QModelIndex current = mediaTree->currentIndex();
    QModelIndex target = vsortModel->mapToSource(current);
    QString filename = videoModel->data(videoModel->index(target.row(), 0), Qt::DisplayRole).toString();
    if (QFileInfo(filename).exists()) {
        QAction *del = new QAction(tr("Remove reference to selected video"), workoutTree);
        menu.addAction(del);
        connect(del, SIGNAL(triggered(void)), this, SLOT(deleteVideos(void)));
    }

    // execute the menu
    menu.exec(trainSplitter->mapToGlobal(QPoint(mediaItem->pos().x()+mediaItem->width()-20,
                                           mediaItem->pos().y())));
}

void
TrainSidebar::videosyncPopup()
{
    // OK - we are working with a specific event..
    QMenu menu(videosyncTree);
    QAction *import = new QAction(tr("Import VideoSync from File"), videosyncTree);
    QAction *scan = new QAction(tr("Scan for VideoSyncs"), videosyncTree);

    menu.addAction(import);
    menu.addAction(scan);

    // connect menu to functions
    connect(import, SIGNAL(triggered(void)), context->mainWindow, SLOT(importWorkout(void)));
    connect(scan, SIGNAL(triggered(void)), context->mainWindow, SLOT(manageLibrary(void)));

    QModelIndex current = videosyncTree->currentIndex();
    QModelIndex target = vssortModel->mapToSource(current);
    QString filename = videosyncModel->data(videosyncModel->index(target.row(), 0), Qt::DisplayRole).toString();
    if (QFileInfo(filename).exists()) {
        QAction *del = new QAction(tr("Delete selected VideoSync"), workoutTree);
        menu.addAction(del);
        connect(del, SIGNAL(triggered(void)), this, SLOT(deleteVideoSyncs(void)));
    }

    // execute the menu
    menu.exec(trainSplitter->mapToGlobal(QPoint(videosyncItem->pos().x()+videosyncItem->width()-20,
                                           videosyncItem->pos().y())));
}

void
TrainSidebar::configChanged(qint32)
{
    // do not refresh if workout running, defer to end of workout
    if (status&RT_RUNNING) {
        pendingConfigChange = true;
        return;
    }

    // auto connect is off by default
    autoConnect = appsettings->value(this, TRAIN_AUTOCONNECT, false).toBool();

    // lap sounds are off by default
    lapAudioEnabled = appsettings->value(this, TRAIN_LAPALERT, false).toBool();

    useSimulatedSpeed = appsettings->value(this, TRAIN_USESIMULATEDSPEED, false).toBool();

    setProperty("color", GColor(CTRAINPLOTBACKGROUND));
#if !defined GC_VIDEO_NONE
    mediaTree->setStyleSheet(GCColor::stylesheet(true));
#ifdef GC_HAVE_VLC  // RLV currently only support for VLC
    videosyncTree->setStyleSheet(GCColor::stylesheet(true));
#endif
#endif
    workoutTree->setStyleSheet(GCColor::stylesheet(true));
    deviceTree->setStyleSheet(GCColor::stylesheet(true));

    // DEVICES

    // Disconnect any running telemetry before manipulating device list
    Disconnect();

    // zap whats there
    QList<QTreeWidgetItem *> devices = deviceTree->invisibleRootItem()->takeChildren();
    for (int i=0; i<devices.count(); i++) delete devices.at(i);

    if (appsettings->value(this, TRAIN_MULTI, false).toBool() == true)
        deviceTree->setSelectionMode(QAbstractItemView::MultiSelection);
    else
        deviceTree->setSelectionMode(QAbstractItemView::SingleSelection);

    // wipe whatever is there
    foreach(DeviceConfiguration x, Devices) delete x.controller;
    Devices.clear();

    DeviceConfigurations all;
    Devices = all.getList();
    for (int i=0; i<Devices.count(); i++) {

        // add to the selection tree
        QTreeWidgetItem *device = new QTreeWidgetItem(deviceTree->invisibleRootItem(), i);
        device->setText(0, Devices.at(i).name);

        // Create the controllers for each device
        // we can call upon each of these when we need
        // to interact with the device
        if (Devices.at(i).type == DEV_CT) {
            Devices[i].controller = new ComputrainerController(this, &Devices[i]);
        } else if (Devices.at(i).type == DEV_MONARK) {
            Devices[i].controller = new MonarkController(this, &Devices[i]);
        } else if (Devices.at(i).type == DEV_KETTLER) {
            Devices[i].controller = new KettlerController(this, &Devices[i]);
        } else if (Devices.at(i).type == DEV_KETTLER_RACER) {
            Devices[i].controller = new KettlerRacerController(this, &Devices[i]);
        } else if (Devices.at(i).type == DEV_ERGOFIT) {
            Devices[i].controller = new ErgofitController(this, &Devices[i]);
        } else if (Devices.at(i).type == DEV_DAUM) {
            Devices[i].controller = new DaumController(this, &Devices[i]);
#ifdef GC_HAVE_LIBUSB
        } else if (Devices.at(i).type == DEV_FORTIUS) {
            Devices[i].controller = new FortiusController(this, &Devices[i]);
        } else if (Devices.at(i).type == DEV_IMAGIC) {
            Devices[i].controller = new ImagicController(this, &Devices[i]);
#endif
        } else if (Devices.at(i).type == DEV_NULL) {
            Devices[i].controller = new NullController(this, &Devices[i]);
        } else if (Devices.at(i).type == DEV_ANTLOCAL) {
            Devices[i].controller = new ANTlocalController(this, &Devices[i]);
            // connect slot for receiving remote control commands
            connect(Devices[i].controller, SIGNAL(remoteControl(uint16_t)), this, SLOT(remoteControl(uint16_t)));
            // connect slot for receiving rrData
            connect(Devices[i].controller, SIGNAL(rrData(uint16_t,uint8_t,uint8_t)), this, SLOT(rrData(uint16_t,uint8_t,uint8_t)));
#ifdef QT_BLUETOOTH_LIB
        } else if (Devices.at(i).type == DEV_BT40) {
            Devices[i].controller = new BT40Controller(this, &Devices[i]);
            connect(Devices[i].controller, SIGNAL(vo2Data(double,double,double,double,double,double)),
                    this, SLOT(vo2Data(double,double,double,double,double,double)));
#endif
        }
    }

    // select the first device
    if (Devices.count()) {
        deviceTree->setCurrentItem(deviceTree->invisibleRootItem()->child(0));
    }
    // And select default workout to Ergo
    QModelIndex firstWorkout = sortModel->index(0, 0, QModelIndex());
    workoutTree->setCurrentIndex(firstWorkout);

    // Re-read ANT remote control command mappings
    remote->readConfig();

    // Athlete
    FTP=285; // default to 285 if zones are not set
    WPRIME = 20000;

    int range = context->athlete->zones("Bike") ? context->athlete->zones("Bike")->whichRange(QDate::currentDate()) : -1;
    if (range != -1) {
        FTP = context->athlete->zones("Bike")->getCP(range);
        WPRIME = context->athlete->zones("Bike")->getWprime(range);
    }

    // Reinit Bicycle
    bicycle.Reset(context);
}

/*----------------------------------------------------------------------
 * Device Selected
 *--------------------------------------------------------------------*/
void
TrainSidebar::deviceTreeWidgetSelectionChanged()
{
    //qDebug() << "TrainSidebar::deviceTreeWidgetSelectionChanged()";

    bpmTelemetry = wattsTelemetry = kphTelemetry = rpmTelemetry = -1;
    deviceSelected();

    if (status&RT_CONNECTED) Disconnect(); // disconnect first
    if (autoConnect) Connect(); // re-connect
}

int
TrainSidebar::selectedDeviceNumber()
{
    if (deviceTree->selectedItems().isEmpty()) return -1;

    QTreeWidgetItem *selected = deviceTree->selectedItems().first();

    if (selected->type() == HEAD_TYPE) return -1;
    else return selected->type();
}

QList<int>
TrainSidebar::devices()
{
    QList<int> returning;
    foreach(QTreeWidgetItem *item, deviceTree->selectedItems())
        if (item->type() != HEAD_TYPE)
            returning << item->type();

    return returning;
}

/*----------------------------------------------------------------------
 * Workout Selected
 *--------------------------------------------------------------------*/
void
TrainSidebar::workoutTreeWidgetSelectionChanged()
{
    QModelIndex current = workoutTree->currentIndex();
    QModelIndex target = sortModel->mapToSource(current);
    QString filename = workoutModel->data(workoutModel->index(target.row(), 0), Qt::DisplayRole).toString();

    // wipe away the current selected workout once we've told everyone
    // since they might be editing it and want to save changes first (!!)
    ErgFile *prior = const_cast<ErgFile*>(ergFileQueryAdapter.getErgFile());
    workoutfile = filename;

    if (filename == "") {

        // an empty workout
        context->notifyErgFileSelected(NULL);
        ergFileQueryAdapter.setErgFile(NULL);

        // clean last
        if (prior) delete prior;

        return;
    }

    // is it the auto mode?
    int index = target.row();
    if (index == 0) {
        // ergo mode
        context->notifyErgFileSelected(NULL);
        ergFileQueryAdapter.setErgFile(NULL);
        mode = ERG;
        setLabels();
        clearStatusFlags(RT_WORKOUT);
        //ergPlot->setVisible(false);
    } else if (index == 1) {
        // slope mode
        context->notifyErgFileSelected(NULL);
        ergFileQueryAdapter.setErgFile(NULL);
        mode = CRS;
        setLabels();
        clearStatusFlags(RT_WORKOUT);
        //ergPlot->setVisible(false);
    } else {
        // workout mode
        ErgFile* ergFile = new ErgFile(filename, mode, context);
        mode = ergFile->mode;

        if (ergFile->isValid()) {

            setStatusFlags(RT_WORKOUT);

            // success! we have a load file
            // setup the course profile in the
            // display!
            context->notifyErgFileSelected(ergFile);
            ergFileQueryAdapter.setErgFile(ergFile);
            adjustIntensity(100);
            setLabels();

#if !defined GC_VIDEO_NONE
            // Try to select matching media and videosync files
            QString workoutName = QFileInfo(filename).baseName();
            mediaTree->setFocus();
            mediaTree->keyboardSearch(workoutName);
#ifdef GC_HAVE_VLC  // RLV currently only support for VLC
            videosyncTree->setFocus();
            videosyncTree->keyboardSearch(workoutName);
#endif
            workoutTree->setFocus();
#endif
        } else {

            // couldn't parse fall back to ERG mode
            delete ergFile;
            ergFile = NULL;
            context->notifyErgFileSelected(NULL);
            ergFileQueryAdapter.setErgFile(NULL);
            removeInvalidWorkout();
            mode = ERG;
            clearStatusFlags(RT_WORKOUT);
            setLabels();
        }
    }

    // set the device to the right mode
    if (mode == ERG || mode == MRC) {
        setStatusFlags(RT_MODE_ERGO);
        clearStatusFlags(RT_MODE_SPIN);

        // update every active device
        foreach(int dev, activeDevices) Devices[dev].controller->setMode(RT_MODE_ERGO);

    } else { // SLOPE MODE
        setStatusFlags(RT_MODE_SPIN);
        clearStatusFlags(RT_MODE_ERGO);

        // update every active device
        foreach(int dev, activeDevices) Devices[dev].controller->setMode(RT_MODE_SPIN);
    }

    maintainLapDistanceState();
    
    ergFileQueryAdapter.resetQueryState();

    // clean last
    if (prior) delete prior;
}

/*
 * Calculate Lap State with respect to distance.
 * Doesn't apply to time-based workouts.
 * Calculates the current lap distance.
 * Calculates the lap distance remaining in the current lap.
 * Route span is used if workout has no laps.
 */
void
TrainSidebar::maintainLapDistanceState()
{
    // lapDistance, lapDistanceRemaining are only relevant when
    // running in slope mode.
    if (!ergFileQueryAdapter.getErgFile() || !(status&RT_MODE_SLOPE)) {
        displayLapDistance = 0;
        displayLapDistanceRemaining = -1;
        return;
    }

    double currentpositionM = displayWorkoutDistance * 1000.;
    double lapmarkerM = ergFileQueryAdapter.currentLap(currentpositionM);

    // If no current lap then handle route as lap.
    if (lapmarkerM < 0.) {
        displayLapDistance = displayWorkoutDistance;
        displayLapDistanceRemaining = (ergFileQueryAdapter.Duration() / 1000.) - displayWorkoutDistance;
        return;
    }

    displayLapDistance = (currentpositionM - lapmarkerM) / 1000.;
    double nextlapmarkerM = ergFileQueryAdapter.nextLap(currentpositionM);

    // If no next lap then use distance to end of route.
    if (nextlapmarkerM < 0.) {
        displayLapDistanceRemaining = (ergFileQueryAdapter.Duration() / 1000.) - displayWorkoutDistance;
        return;
    }

    displayLapDistanceRemaining = (nextlapmarkerM - currentpositionM) / 1000.;
}

QStringList
TrainSidebar::listWorkoutFiles(const QDir &dir) const
{
    QStringList filters;
    filters << "*.erg";
    filters << "*.mrc";
    filters << "*.crs";
    filters << "*.pgmf";

    return dir.entryList(filters, QDir::Files, QDir::Name);
}

void
TrainSidebar::deleteVideos()
{
    QModelIndex current = mediaTree->currentIndex();
    QModelIndex target = vsortModel->mapToSource(current);
    QString filename = videoModel->data(videoModel->index(target.row(), 0), Qt::DisplayRole).toString();

    if (QFileInfo(filename).exists()) {
        // are you sure?
        QMessageBox msgBox;
        msgBox.setText(tr("Are you sure you want to remove the reference to this video?"));
        msgBox.setInformativeText(filename);
        QPushButton *deleteButton = msgBox.addButton(tr("Remove"),QMessageBox::YesRole);
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        if(msgBox.clickedButton() != deleteButton) return;

        // delete from disk
        //XXX QFile(filename).remove(); // lets not for now..

        // remove any reference (from drag and drop)
        Library *l = Library::findLibrary("Media Library");
        if (l) l->removeRef(context, filename);

        // delete from DB
        trainDB->startLUW();
        trainDB->deleteVideo(filename);
        trainDB->endLUW();
    }
}


void
TrainSidebar::deleteVideoSyncs()
{
    QModelIndex current = videosyncTree->currentIndex();
    QModelIndex target = vssortModel->mapToSource(current);
    QString filename = videosyncModel->data(videosyncModel->index(target.row(), 0), Qt::DisplayRole).toString();

    if (QFileInfo(filename).exists()) {
        // are you sure?
        QMessageBox msgBox;
        msgBox.setText(tr("Are you sure you want to delete this VideoSync?"));
        msgBox.setInformativeText(filename);
        QPushButton *deleteButton = msgBox.addButton(tr("Delete"),QMessageBox::YesRole);
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        if(msgBox.clickedButton() != deleteButton) return;

        // delete from disk
        QFile(filename).remove();

        // delete from DB
        trainDB->startLUW();
        trainDB->deleteVideoSync(filename);
        trainDB->endLUW();
    }
}

void
TrainSidebar::removeInvalidVideoSync()
{
    QModelIndex current = videosyncTree->currentIndex();
    QModelIndex target = vssortModel->mapToSource(current);
    QString filename = videosyncModel->data(videosyncModel->index(target.row(), 0), Qt::DisplayRole).toString();
    QMessageBox msgBox;
    msgBox.setText(tr("The VideoSync file is either not valid or not existing and will be removed from the library."));
    msgBox.setInformativeText(filename);
    QPushButton *removeButton = msgBox.addButton(tr("Remove"),QMessageBox::YesRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setDefaultButton(removeButton);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();

    if(msgBox.clickedButton() != removeButton) return;

    // delete from DB
    trainDB->startLUW();
    trainDB->deleteVideoSync(filename);
    trainDB->endLUW();

}

void
TrainSidebar::deleteWorkouts()
{
    QStringList nameList;
    QModelIndexList list = workoutTree->selectionModel()->selectedRows();
    foreach (QModelIndex index, list) {
        QModelIndex target = sortModel->mapToSource(index);
        QString filename = workoutModel->data(workoutModel->index(target.row(), 0), Qt::DisplayRole).toString();
        if (QFileInfo(filename).exists()) {
            nameList.append(filename);
        }
    }

    if (nameList.count()>0) {
        // are you sure?
        QMessageBox msgBox;
        if (nameList.count()==1) {
            msgBox.setText(tr("Are you sure you want to delete this Workout?"));
        }
        else {
            msgBox.setText(QString(tr("Are you sure you want to delete this %1 workouts?")).arg(nameList.count()));
        }
        QString info;
        for (int i=0;i<nameList.count() && i<21;i++) {
           info += nameList.at(i)+"\n";
           if (i == 20) {
               info += "...\n";
           }
        }
        msgBox.setInformativeText(info);
        QPushButton *deleteButton = msgBox.addButton(tr("Delete"),QMessageBox::YesRole);
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        if(msgBox.clickedButton() != deleteButton) return;

        foreach (QString filename, nameList) {
            if (QFileInfo(filename).exists()) {
                // delete from disk
                QFile(filename).remove();
                // delete from DB
                trainDB->startLUW();
                trainDB->deleteWorkout(filename);
                trainDB->endLUW();
            }
        }
    }
}

void
TrainSidebar::removeInvalidWorkout()
{
    QModelIndex current = workoutTree->currentIndex();
    QModelIndex target = sortModel->mapToSource(current);
    QString filename = workoutModel->data(workoutModel->index(target.row(), 0), Qt::DisplayRole).toString();

    QMessageBox msgBox;
    msgBox.setText(tr("The Workout file is either not valid or not existing and will be removed from the library."));
    msgBox.setInformativeText(filename);
    QPushButton *removeButton = msgBox.addButton(tr("Remove"),QMessageBox::YesRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setDefaultButton(removeButton);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();

    if(msgBox.clickedButton() != removeButton) return;

    // delete from DB
    trainDB->startLUW();
    trainDB->deleteWorkout(filename);
    trainDB->endLUW();

}

void
TrainSidebar::mediaTreeWidgetSelectionChanged()
{

    QModelIndex current = mediaTree->currentIndex();
    QModelIndex target = vsortModel->mapToSource(current);
    QString filename = videoModel->data(videoModel->index(target.row(), 0), Qt::DisplayRole).toString();
    if (filename == context->videoFilename) {
        mediafile = "";
        context->notifyMediaSelected(""); // CTRL+Click to clear selection
    } else {
        mediafile = filename;
        context->notifyMediaSelected(filename);
    }
}

void
TrainSidebar::videosyncTreeWidgetSelectionChanged()
{

    QModelIndex current = videosyncTree->currentIndex();
    QModelIndex target = vssortModel->mapToSource(current);
    QString filename = videosyncModel->data(videosyncModel->index(target.row(), 0), Qt::DisplayRole).toString();

    // wip away the current selected videosync
    if (videosyncFile) {
        delete videosyncFile;
        videosyncFile = NULL;
    }

     if (filename == "") {
        context->notifyVideoSyncFileSelected(NULL);
        return;
    }

    // is "None" selected?
    int index = target.row();
    if (index == 0) {
        // None menu entry
        context->notifyVideoSyncFileSelected(NULL);
    } else {
        videosyncFile = new VideoSyncFile(filename, mode, context);
        if (videosyncFile->isValid()) {
            context->notifyVideoSyncFileSelected(videosyncFile);
        } else {
            delete videosyncFile;
            videosyncFile = NULL;
            context->notifyVideoSyncFileSelected(NULL);
            removeInvalidVideoSync();
        }
    }
}

/*--------------------------------------------------------------------------------
 * Was realtime window, now local and manages controller and chart updates etc
 *------------------------------------------------------------------------------*/

void TrainSidebar::Start()       // when start button is pressed
{
    if (status&RT_PAUSED) {

        qDebug() << "unpause...";

        // UN PAUSE!
        session_time.start();
        lap_time.start();
        clearStatusFlags(RT_PAUSED);

        // Reset speed simulation timer.
        bicycle.resettimer();

        maintainLapDistanceState();

        //foreach(int dev, activeDevices) Devices[dev].controller->restart();
        //gui_timer->start(REFRESHRATE);
        if (status & RT_RECORDING) disk_timer->start(SAMPLERATE);
        load_period.restart();
        load_timer->start(LOADRATE);

#if !defined GC_VIDEO_NONE
        mediaTree->setEnabled(false);
#ifdef GC_HAVE_VLC  // RLV currently only support for VLC
        videosyncTree->setEnabled(false);
#endif
#endif

        // tell the world
        context->notifyUnPause();

        emit setNotification(tr("Resuming.."), 2);

    } else if (status&RT_RUNNING) {

        qDebug() << "pause...";

        // Pause!
        session_elapsed_msec += session_time.elapsed();
        lap_elapsed_msec += lap_time.elapsed();
        setStatusFlags(RT_PAUSED);
        //foreach(int dev, activeDevices) Devices[dev].controller->pause();
        //gui_timer->stop();
        if (status & RT_RECORDING) disk_timer->stop();
        load_timer->stop();
        load_msecs += load_period.restart();

#if !defined GC_VIDEO_NONE
        // enable media tree so we can change movie - mid workout
        mediaTree->setEnabled(true);
#ifdef GC_HAVE_VLC  // RLV currently only support for VLC
        videosyncTree->setEnabled(true);
#endif
#endif

        // tell the world
        context->notifyPause();

        emit setNotification(tr("Paused.."), 2);

    } else if (status&RT_CONNECTED) {

        // Delayed start handling
        if (secs_to_start == 0) {
            secs_to_start = appsettings->value(this, TRAIN_STARTDELAY, 0).toUInt();
        } else {
            secs_to_start--;
        }
        if (secs_to_start > 0) {
            emit setNotification(tr("Starting in %1").arg(secs_to_start), 1);
            start_timer->start(1000);
            return;
        }

        qDebug() << "start...";

#ifdef WIN32
        // disable the screen saver on Windows
        SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_CONTINUOUS);
#endif

        // Stop users from selecting different devices
        // media or workouts whilst a workout is in progress
#if !defined GC_VIDEO_NONE
        mediaTree->setEnabled(false);
#ifdef GC_HAVE_VLC  // RLV currently only support for VLC
        videosyncTree->setEnabled(false);
#endif
#endif
        workoutTree->setEnabled(false);
        deviceTree->setEnabled(false);

        // START!
        load = 100;
        slope = 0.0;

        // Reset Speed Simulation
        bicycle.clear();

        maintainLapDistanceState();

        if (mode == ERG || mode == MRC) {
            setStatusFlags(RT_MODE_ERGO);
            clearStatusFlags(RT_MODE_SPIN);
            foreach(int dev, activeDevices) Devices[dev].controller->setMode(RT_MODE_ERGO);
        } else { // SLOPE MODE
            setStatusFlags(RT_MODE_SPIN);
            clearStatusFlags(RT_MODE_ERGO);
            foreach(int dev, activeDevices) Devices[dev].controller->setMode(RT_MODE_SPIN);
        }

        // tell the world
        context->notifyStart();

        // we're away!
        setStatusFlags(RT_RUNNING);

        // tell the world
        context->notifyStart();

        load_period.restart();
        session_time.start();
        session_elapsed_msec = 0;
        lap_time.start();
        lap_elapsed_msec = 0;
        wbalr = 0;
        wbal = WPRIME;
        
        resetTextAudioEmitTracking();

        //reset all calibration data
        calibrating = startCalibration = restartCalibration = finishCalibration = false;
        calibrationSpindownTime = calibrationZeroOffset = calibrationSlope = calibrationTargetSpeed = 0;
        calibrationCadence = calibrationCurrentSpeed = calibrationTorque = 0;
        calibrationState = CALIBRATION_STATE_IDLE;
        calibrationType = CALIBRATION_TYPE_NOT_SUPPORTED;
        calibrationDeviceIndex = -1;
        clearStatusFlags(RT_CALIBRATING);
        //foreach(int dev, activeDevices) { // Do for selected device only
        //    Devices[dev].controller->resetCalibrationState();
        //}

        load_timer->start(LOADRATE);      // start recording

        if (recordSelector->isChecked()) {
            setStatusFlags(RT_RECORDING);
        }

        if (status & RT_RECORDING) {

            QString workoutName;
            if (context->currentErgFile()) {
                workoutName = QFileInfo(context->currentErgFile()->filename).baseName();
            }

            QDateTime now = QDateTime::currentDateTime();

            // setup file
            QString filename = now.toString(QString("yyyy_MM_dd_hh_mm_ss")) + "_" + workoutName + QString(".csv");

            if (!context->athlete->home->records().exists())
                context->athlete->home->createAllSubdirs();

            QString fulltarget = context->athlete->home->records().canonicalPath() + "/" + filename;

            if (recordFile) delete recordFile;
            recordFile = new QFile(fulltarget);
            lastRecordSecs = 0;
            if (!recordFile->open(QFile::WriteOnly | QFile::Truncate)) {
                clearStatusFlags(RT_RECORDING);
            } else {

                // CSV File header

                QTextStream recordFileStream(recordFile);
                recordFileStream << "secs, cad, hr, km, kph, nm, watts, alt, lon, lat, headwind, slope, temp, interval, lrbalance, lte, rte, lps, rps, smo2, thb, o2hb, hhb, target\n";

                disk_timer->start(SAMPLERATE);  // start screen
            }
        }
        gui_timer->start(REFRESHRATE);      // start recording

        emit setNotification(tr("Starting.."), 2);
    }
}

void TrainSidebar::Pause()        // pause capture to recalibrate
{
    // Not convinced this function is ever reached, these are handled in Start()

    // we're not running fool!
    if ((status&RT_RUNNING) == 0) return;

    if (status&RT_PAUSED) {

        session_time.start();
        lap_time.start();
        clearStatusFlags(RT_PAUSED);
        foreach(int dev, activeDevices) Devices[dev].controller->restart();
        gui_timer->start(REFRESHRATE);
        if (status & RT_RECORDING) disk_timer->start(SAMPLERATE);
        load_period.restart();
        load_timer->start(LOADRATE);

#if !defined GC_VIDEO_NONE
        mediaTree->setEnabled(false);
#ifdef GC_HAVE_VLC  // RLV currently only support for VLC
        videosyncTree->setEnabled(false);
#endif
#endif

        // tell the world
        context->notifyUnPause();
    } else {

        session_elapsed_msec += session_time.elapsed();
        lap_elapsed_msec += lap_time.elapsed();
        foreach(int dev, activeDevices) Devices[dev].controller->pause();
        setStatusFlags(RT_PAUSED);
        gui_timer->stop();
        if (status & RT_RECORDING) disk_timer->stop();
        load_timer->stop();
        load_msecs += load_period.restart();

        // enable media tree so we can change movie
#if !defined GC_VIDEO_NONE
        mediaTree->setEnabled(true);
#ifdef GC_HAVE_VLC  // RLV currently only support for VLC
        videosyncTree->setEnabled(true);
#endif
#endif

        // tell the world
        context->notifyPause();
    }
}

void TrainSidebar::Stop(int deviceStatus)        // when stop button is pressed
{
    if ((status&RT_RUNNING) == 0) return;

    // re-enable the screen saver on Windows
#ifdef WIN32
    SetThreadExecutionState(ES_CONTINUOUS);
#endif

    clearStatusFlags(RT_RUNNING|RT_PAUSED);

    // Stop users from selecting different devices
    // media or workouts whilst a workout is in progress
#if !defined GC_VIDEO_NONE
    mediaTree->setEnabled(true);
#ifdef GC_HAVE_VLC  // RLV currently only support for VLC
    videosyncTree->setEnabled(true);
#endif
#endif
    workoutTree->setEnabled(true);
    deviceTree->setEnabled(true);

    //reset all calibration data
    calibrating = startCalibration = restartCalibration = finishCalibration = false;
    calibrationSpindownTime = calibrationZeroOffset = calibrationSlope = calibrationTargetSpeed = 0;
    calibrationCadence = calibrationCurrentSpeed = calibrationTorque = 0;
    calibrationState = CALIBRATION_STATE_IDLE;
    calibrationType = CALIBRATION_TYPE_NOT_SUPPORTED;
    calibrationDeviceIndex = -1;
    clearStatusFlags(RT_CALIBRATING);
    //foreach(int dev, activeDevices) { // Do for selected device only
    //    Devices[dev].controller->resetCalibrationState();
    //}

    load = 0;
    slope = 0.0;

    QDateTime now = QDateTime::currentDateTime();

    if (status & RT_RECORDING) {
        disk_timer->stop();

        // close and reset File
        recordFile->close();

        // close rrFile
        if (rrFile) {
            //fprintf(stderr, "Closing r-r file\n"); fflush(stderr);
            rrFile->close();
            delete rrFile;
            rrFile=NULL;
        }

        // close vo2File
        if (vo2File) {
            fprintf(stderr, "Closing vo2 file\n"); fflush(stderr);
            vo2File->close();
            delete vo2File;
            vo2File=NULL;
        }

        if(deviceStatus == DEVICE_ERROR)
        {
            recordFile->remove();
        }
        else {
            // add to the view - using basename ONLY
            QString name;
            name = recordFile->fileName();

            QList<QString> list;
            list.append(name);

            RideImportWizard *dialog = new RideImportWizard (list, context);
            dialog->process(); // do it!
        }

        // cancel recording
        status &= ~RT_RECORDING;
    }

    load_timer->stop();
    load_msecs = 0;

    // get back to normal after it may have been adusted by the user
    //lastAppliedIntensity=100;
    adjustIntensity(100);
    if (context->currentErgFile()) context->currentErgFile()->reload();
    context->notifySetNow(load_msecs);


    // tell the world
    context->notifyStop();

    // if a config change was requested while workout was running, action it now
    if (pendingConfigChange) {
        pendingConfigChange = false;
        configChanged(CONFIG_APPEARANCE | CONFIG_DEVICES | CONFIG_ZONES);
    }

    // Re-enable gui elements
    // reset counters etc
    pwrcount = 0;
    cadcount = 0;
    hrcount = 0;
    spdcount = 0;
    lodcount = 0;
    displayWorkoutLap = 0;
    wbalr = 0;
    wbal = WPRIME;
    session_elapsed_msec = 0;
    session_time.restart();
    lap_elapsed_msec = 0;
    lap_time.restart();
    displayWorkoutDistance = displayDistance = 0;
    displayLapDistance = 0;
    displayLapDistanceRemaining = -1;
    displayAltitude = 0;

    ergFileQueryAdapter.resetQueryState();
    guiUpdate();

    emit setNotification(tr("Stopped.."), 2);

    return;
}


// Called by push devices (e.g. ANT+)
void TrainSidebar::updateData(RealtimeData &rtData)
{
    displayPower = rtData.getWatts();
    displayCadence = rtData.getCadence();
    displayHeartRate = rtData.getHr();
    displaySpeed = rtData.getSpeed();
    load = rtData.getLoad();
    displayLRBalance = rtData.getLRBalance();
    displayLTE = rtData.getLTE();
    displayRTE = rtData.getRTE();
    displayLPS = rtData.getLPS();
    displayRPS = rtData.getRPS();
    displaySMO2 = rtData.getSmO2();
    displayTHB  = rtData.gettHb();
    displayO2HB = rtData.getO2Hb();
    displayHHB = rtData.getHHb();
    displayLatitude = rtData.getLatitude();
    displayLongitude = rtData.getLongitude();
    displayAltitude = rtData.getAltitude();
    // Gradient not supported
    return;
}

void TrainSidebar::toggleConnect()
{
    if (status&RT_CONNECTED)
        Disconnect();
    else
        Connect();
}

void TrainSidebar::Connect()
{
    //qDebug() << "current tab:" << context->tab->currentView();

    // only try and connect if we are in train view..
    // fixme: these values are hard-coded throughout
    if (!(context->tab->currentView() == 3)) return;

    if (status&RT_CONNECTED) return; // already connected

    qDebug() << "connecting..";

    // if we have selected multiple devices lets
    // configure the series we collect from each one
    if (deviceTree->selectedItems().count() > 1) {
        MultiDeviceDialog *multisetup = new MultiDeviceDialog(context, this);
        if (multisetup->exec() == false) {
            return;
        }
    } else if (deviceTree->selectedItems().count() == 1) {
        bpmTelemetry = wattsTelemetry = kphTelemetry = rpmTelemetry =
        deviceTree->selectedItems().first()->type();
    } else {
        return;
    }

    activeDevices = devices();

    foreach(int dev, activeDevices) {
        Devices[dev].controller->setWheelCircumference(Devices[dev].wheelSize);
        Devices[dev].controller->setRollingResistance(bicycle.RollingResistance());
        Devices[dev].controller->setWeight(bicycle.MassKG());
        Devices[dev].controller->setWindSpeed(0); // Move to loadUpdate when wind simulation is added

        Devices[dev].controller->start();
        Devices[dev].controller->resetCalibrationState();
        connect(Devices[dev].controller, &RealtimeController::setNotification, this, &TrainSidebar::setNotification);
    }
    setStatusFlags(RT_CONNECTED);
    gui_timer->start(REFRESHRATE);

    emit setNotification(tr("Connected.."), 2);

    // lets SWITCH PERSPECTIVE as we are connected, but only
    // if everything has been initialised properly (aka lazy load)
    // given the connect widget is on the train view it is unlikely
    // below will ever be false, but no harm in checking
    if (trainView && trainView->page()) {

        Perspective::switchenum want=Perspective::None;
        if (mediafile != "") want=Perspective::Video; // if media file selected
        else want = (mode == ERG || mode == MRC) ? Perspective::Erg : Perspective::Slope; // mode always known

        // so we want a view type and the current page isn't what
        // we want then lets go find one to switch to and switch
        // to the first one that matches
        if (want != Perspective::None && trainView->page()->trainSwitch() != want) {

            for(int i=0; i<trainView->perspectives_.count(); i++) {
                if (trainView->perspectives_[i]->trainSwitch() == want) {
                    context->mainWindow->switchPerspective(i);
                    break;
                }
            }
        }
    }
}

void TrainSidebar::Disconnect()
{
    // cancel any pending start
    if (secs_to_start > 0) {
        secs_to_start = 0;
        start_timer->stop();
    }

    // don't try to disconnect if running or not connected
    if ((status&RT_RUNNING) || ((status&RT_CONNECTED) == 0)) return;

    static QIcon connectedIcon(":images/oxygen/power-on.png");
    static QIcon disconnectedIcon(":images/oxygen/power-off.png");

    qDebug() << "disconnecting..";

    foreach(int dev, activeDevices) {
        disconnect(Devices[dev].controller, &RealtimeController::setNotification, this, &TrainSidebar::setNotification);
        Devices[dev].controller->stop();
    }
    clearStatusFlags(RT_CONNECTED);

    gui_timer->stop();

    emit setNotification(tr("Disconnected.."), 2);
}

//----------------------------------------------------------------------
// SCREEN UPDATE FUNCTIONS
//----------------------------------------------------------------------

void TrainSidebar::guiUpdate()           // refreshes the telemetry
{
    RealtimeData rtData;
    rtData.setLap(displayWorkoutLap);
    rtData.mode = mode;

    // get latest telemetry from devices
    if ((status&RT_RUNNING) || (status&RT_CONNECTED)) {

#ifdef Q_OS_MAC
        // On a Mac prevent the screensaver from kicking in
        // this is apparently the 'supported' mechanism for
        // disabling the screen saver on a Mac instead of
        // temporarily adjusting/disabling the user preferences
        // for screen saving and power management. Makes sense.

        CFStringRef reasonForActivity = CFSTR("TrainSidebar::guiUpdate");
        IOPMAssertionID assertionID;
        IOReturn suspendSreensaverSuccess = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn, reasonForActivity, &assertionID);

#elif defined(WIN32)
        // Multimedia applications, such as video players and presentation applications, must use ES_DISPLAY_REQUIRED
        // when they display video for long periods of time without user input.
        SetThreadExecutionState(ES_DISPLAY_REQUIRED);
#endif

        if(calibrating) {
            foreach(int dev, activeDevices) { // Do for selected device only
                RealtimeData local = rtData;

                if (calibrationDeviceIndex == dev) {
                    // need telemetry for calibration dialog updates
                    // (and F3 button press for Computrainer)

                    Devices[dev].controller->getRealtimeData(local);
                    calibrationCurrentSpeed = local.getSpeed();
                    calibrationTorque = local.getTorque();
                    calibrationCadence = local.getCadence();

                    calibrationTargetSpeed = Devices[dev].controller->getCalibrationTargetSpeed();

                    calibrationState = Devices[dev].controller->getCalibrationState();

                    calibrationSpindownTime =  Devices[dev].controller->getCalibrationSpindownTime();
                    calibrationZeroOffset =  Devices[dev].controller->getCalibrationZeroOffset();
                    calibrationSlope = Devices[dev].controller->getCalibrationSlope();

                    // if calibration is moving out of pending state..
                    if ((calibrationState == CALIBRATION_STATE_PENDING) && startCalibration) {
                        startCalibration = false;
                        qDebug() << "Sending calibration request..";
                        Devices[dev].controller->setCalibrationState(CALIBRATION_STATE_REQUESTED);
                    }

                    // if calibration was already requested, but not receiving updates, then try again..
                    if ((calibrationState == CALIBRATION_STATE_STARTING) && restartCalibration) {
                        restartCalibration = false;
                        qDebug() << "No response to our calibration request, re-requesting..";
                        Devices[dev].controller->setCalibrationState(CALIBRATION_STATE_REQUESTED);
                    }

                }
            }

            // calibration has completed (or failed), toggle out of calibration state
            // - do this outside of dev loop in case of no valid/supported device
            if (finishCalibration) {
                finishCalibration = false;
                Calibrate();
            }

            // Update the calibration dialog if necessary
            updateCalibration();

            // and exit.  Nothing else to do until we finish calibrating
            return;
        } else {
            rtData.setLoad(load); // always set load..
            rtData.setSlope(slope); // always set load..
            rtData.setAltitude(displayAltitude); // always set display altitude

            double distanceTick = 0;

            // fetch the right data from each device...
            foreach(int dev, activeDevices) {

                RealtimeData local = rtData;
                Devices[dev].controller->getRealtimeData(local);

                // get spinscan data from a computrainer?
                if (Devices[dev].type == DEV_CT) {
                    memcpy((uint8_t*)rtData.spinScan, (uint8_t*)local.spinScan, 24);
                    rtData.setLoad(local.getLoad()); // and get load in case it was adjusted
                    rtData.setSlope(local.getSlope()); // and get slope in case it was adjusted
                    // to within defined limits
                }

                if (Devices[dev].type == DEV_FORTIUS || Devices[dev].type == DEV_IMAGIC) {
	                rtData.setLoad(local.getLoad()); // and get load in case it was adjusted
                    rtData.setSlope(local.getSlope()); // and get slope in case it was adjusted
					// to within defined limits
				}

                if (Devices[dev].type == DEV_ANTLOCAL || Devices[dev].type == DEV_NULL) {
                    rtData.setHb(local.getSmO2(), local.gettHb()); //only moxy data from ant and robot devices right now
                }

                if (Devices[dev].type == DEV_NULL || Devices[dev].type == DEV_BT40) {
                    // Only robot and BT40 devices provides VO2 metrics
                    rtData.setRf(local.getRf());
                    rtData.setRMV(local.getRMV());
                    rtData.setVO2_VCO2(local.getVO2(), local.getVCO2());
                    rtData.setTv(local.getTv());
                    rtData.setFeO2(local.getFeO2());
                }

                // what are we getting from this one?
                if (dev == bpmTelemetry) rtData.setHr(local.getHr());
                if (dev == rpmTelemetry) rtData.setCadence(local.getCadence());
                if (dev == kphTelemetry) {
                    rtData.setSpeed(local.getSpeed());
                    rtData.setDistance(local.getDistance());
                    rtData.setRouteDistance(local.getRouteDistance());
                    rtData.setDistanceRemaining(local.getDistanceRemaining());
                    rtData.setLapDistance(local.getLapDistance());
                    rtData.setLapDistanceRemaining(local.getLapDistanceRemaining());
                }
                if (dev == wattsTelemetry) {
                    rtData.setWatts(local.getWatts());
                    rtData.setAltWatts(local.getAltWatts());
                    rtData.setLRBalance(local.getLRBalance());
                    rtData.setLTE(local.getLTE());
                    rtData.setRTE(local.getRTE());
                    rtData.setLPS(local.getLPS());
                    rtData.setRPS(local.getRPS());
                }
                if (local.getTrainerStatusAvailable())
                {
                    rtData.setTrainerStatusAvailable(true);
                    rtData.setTrainerReady(local.getTrainerReady());
                    rtData.setTrainerRunning(local.getTrainerRunning());
                    rtData.setTrainerCalibRequired(local.getTrainerCalibRequired());
                    rtData.setTrainerConfigRequired(local.getTrainerConfigRequired());
                    rtData.setTrainerBrakeFault(local.getTrainerBrakeFault());
                }
            }

            // If simulated speed is *not* checked then you get speed reported by
            // trainer which in ergo mode will be dictated by your gear and cadence,
            // and in slope mode is whatever the trainer happens to implement.
            if (useSimulatedSpeed) {
                BicycleSimState newState(rtData);
                SpeedDistance ret = bicycle.SampleSpeed(newState);

                rtData.setSpeed(ret.v);

                displaySpeed = ret.v;
                distanceTick = ret.d;
            } else {
                distanceTick = displaySpeed / (5 * 3600); // assumes 200ms refreshrate
            }

            // only update time & distance if actively running (not just connected, and not running but paused)
            if ((status&RT_RUNNING) && ((status&RT_PAUSED) == 0)) {

                displayDistance += distanceTick;
                displayLapDistance += distanceTick;
                displayLapDistanceRemaining -= distanceTick;
                displayWorkoutDistance += distanceTick;

                if (!(status&RT_MODE_ERGO) && (context->currentVideoSyncFile()))
                {
                    // If we reached the end of the RLV then stop
                    if (displayWorkoutDistance >= context->currentVideoSyncFile()->Distance) {
                        Stop(DEVICE_OK);
                        return;
                    }
                    // TODO : graphs to be shown at seek position
                }

                // If we just tripped over the end of the lap, we need to look at base data
                // to find distance to next lap. This is primarily due to lap display updates
                // -0.999 is chosen as a number that is less than 0, but greater than -1
                if (displayLapDistanceRemaining < 0 && displayLapDistanceRemaining > -0.999) {
                    maintainLapDistanceState();
                }

                rtData.setDistance(displayDistance);
                rtData.setRouteDistance(displayWorkoutDistance);
                rtData.setLapDistance(displayLapDistance);
                rtData.setLapDistanceRemaining(displayLapDistanceRemaining);

                const ErgFile* ergFile = ergFileQueryAdapter.getErgFile();
                if (ergFile) {

                    // update DistanceRemaining
                    if (ergFile->Duration / 1000.0 > displayWorkoutDistance)
                        rtData.setDistanceRemaining(ergFile->Duration / 1000.0 - displayWorkoutDistance);
                    else
                        rtData.setDistanceRemaining(0.0);

                    // If ergfile has no gradient then there is no location, or altitude (or slope.)
                    if (ergFile->hasGradient()) {
                        bool fAltitudeSet = false;
                        if (!ergFile->StrictGradient) {
                            // Attempt to obtain location and derived slope from altitude in ergfile.
                            geolocation geoloc;
                            if (ergFileQueryAdapter.locationAt(displayWorkoutDistance * 1000, displayWorkoutLap, geoloc, slope)) {
                                displayLatitude = geoloc.Lat();
                                displayLongitude = geoloc.Long();
                                displayAltitude = geoloc.Alt();

                                if (displayLatitude && displayLongitude) {
                                    rtData.setLatitude(displayLatitude);
                                    rtData.setLongitude(displayLongitude);
                                }
                                fAltitudeSet = true;
                            }
                        }

                        if (ergFile->StrictGradient || !fAltitudeSet) {
                            slope = ergFileQueryAdapter.gradientAt(displayWorkoutDistance * 1000, displayWorkoutLap);
                        }

                        if (!fAltitudeSet) {
                            // Since we have gradient, we also have altitude
                            displayAltitude = ergFileQueryAdapter.altitudeAt(displayWorkoutDistance * 1000, displayWorkoutLap);
                        }

                        rtData.setSlope(slope);
                        rtData.setAltitude(displayAltitude);
                    }
                }
                else if (!(status & RT_MODE_ERGO)) {
                    // For manual slope mode, estimate vertical change based upon time passed and slope.
                    // Note this isn't exactly right but is very close - we should use the previous slope for the time passed.
                    double altitudeDeltaMeters = slope * (10 * distanceTick); // ((slope / 100) * distanceTick) * 1000
                    displayAltitude += altitudeDeltaMeters;
                    rtData.setAltitude(displayAltitude);
                }

                // time
                total_msecs = session_elapsed_msec + session_time.elapsed();
                lap_msecs = lap_elapsed_msec + lap_time.elapsed();

                rtData.setMsecs(total_msecs);
                rtData.setLapMsecs(lap_msecs);

                long lapTimeRemaining;
                if (ergFile) lapTimeRemaining = ergFile->nextLap(load_msecs) - load_msecs;
                else lapTimeRemaining = 0;

                long ergTimeRemaining;
                if (ergFile) ergTimeRemaining = ergFileQueryAdapter.currentTime() - load_msecs;
                else ergTimeRemaining = 0;

                double lapPosition = status & RT_MODE_ERGO ? load_msecs : displayWorkoutDistance * 1000;

                // alert when approaching end of lap
                if (lapAudioEnabled && lapAudioThisLap) {

                    // alert when 3 seconds from end of ERG lap, or 20 meters from end of CRS lap
                    bool fPlayAudio = false;
                    if (status & RT_MODE_ERGO && lapTimeRemaining > 0 && lapTimeRemaining < 3000) {
                        fPlayAudio = true;
                    } else {
                        double lapmarker = ergFileQueryAdapter.nextLap(lapPosition);
                        if (status&RT_MODE_SLOPE && (lapmarker >= 0.) && lapmarker - lapPosition < 20) {
                            fPlayAudio = true;
                        }
                    }

                    if (fPlayAudio) {
                        lapAudioThisLap = false;
                        QSound::play(":audio/lap.wav");
                    }
                }

                // Text Cues
                if (lapPosition > textPositionEmitted) {
                    double searchRange = (status & RT_MODE_ERGO) ? 1000 : 10;
                    int rangeStart, rangeEnd;
                    if (ergFileQueryAdapter.textsInRange(lapPosition, searchRange, rangeStart, rangeEnd)) {
                        for (int idx = rangeStart; idx <= rangeEnd; idx++) {
                            ErgFileText cue = ergFile->Texts.at(idx);
                            emit setNotification(cue.text, cue.duration);
                        }
                    }
                    textPositionEmitted = lapPosition + searchRange;
                }

                // Maintain time in ERGO mode
                if (status& RT_MODE_ERGO) {
                    if (lapTimeRemaining < 0) {
                        if (ergFile) lapTimeRemaining = ergFile->Duration - load_msecs;
                        if (lapTimeRemaining < 0)
                            lapTimeRemaining = 0;
                    }
                    rtData.setLapMsecsRemaining(lapTimeRemaining);

                    if (ergTimeRemaining < 0) {
                        if (ergFile) ergTimeRemaining = ergFile->Duration - load_msecs;
                        if (ergTimeRemaining < 0)
                            ergTimeRemaining = 0;
                    }
                    rtData.setErgMsecsRemaining(ergTimeRemaining);
                }
            } else {
                rtData.setDistance(displayDistance);
                rtData.setRouteDistance(displayWorkoutDistance);
                rtData.setLapDistance(displayLapDistance);
                rtData.setLapDistanceRemaining(displayLapDistanceRemaining);
                rtData.setMsecs(session_elapsed_msec);
                rtData.setLapMsecs(lap_elapsed_msec);
            }

            // local stuff ...
            displayPower = rtData.getWatts();
            displayCadence = rtData.getCadence();
            displayHeartRate = rtData.getHr();
            displaySpeed = rtData.getSpeed();
            load = rtData.getLoad();
            slope = rtData.getSlope();
            displayLRBalance = rtData.getLRBalance();
            displayLTE = rtData.getLTE();
            displayRTE = rtData.getRTE();
            displayLPS = rtData.getLPS();
            displayRPS = rtData.getRPS();
            displaySMO2 = rtData.getSmO2();
            displayTHB = rtData.gettHb();
            displayO2HB = rtData.getO2Hb();
            displayHHB = rtData.getHHb();
            displayLatitude = rtData.getLatitude();
            displayLongitude = rtData.getLongitude();
            displayAltitude = rtData.getAltitude();

            double weightKG = context->athlete->getWeight(QDate::currentDate()) + 10; // 10kg bike
            double vs = computeInstantSpeed(weightKG, rtData.getSlope(), rtData.getAltitude(), rtData.getWatts());

            rtData.setVirtualSpeed(vs);

            // W'bal on the fly
            // using Dave Waterworth's reformulation
            double TAU = appsettings->cvalue(context->athlete->cyclist, GC_WBALTAU, 300).toInt();

            // any watts expended in last 200msec?
            double JOULES = double(rtData.getWatts() - FTP) / 5.00f;
            if (JOULES < 0) JOULES = 0;

            // running total of replenishment
            wbalr += JOULES * exp((total_msecs/1000.00f) / TAU);
            wbal = WPRIME - (wbalr * exp((-total_msecs/1000.00f) / TAU));

            rtData.setWbal(wbal);

            // go update the displays...
            context->notifyTelemetryUpdate(rtData); // signal everyone to update telemetry
        }

#ifdef Q_OS_MAC
        if (suspendSreensaverSuccess == kIOReturnSuccess)
        {
            // Re-enable screen saver
            suspendSreensaverSuccess = IOPMAssertionRelease(assertionID);
            //The system will be able to sleep again.
        }
#endif
    }
}

// can be called from the controller - when user presses "Lap" button
void TrainSidebar::newLap()
{
    qDebug() << "running:" << (status&RT_RUNNING) << "paused:" << (status&RT_PAUSED);

    if ((status&RT_RUNNING) && ((status&RT_PAUSED) == 0)) {

        pwrcount  = 0;
        cadcount  = 0;
        hrcount   = 0;
        spdcount  = 0;

        ergFileQueryAdapter.addNewLap(displayWorkoutDistance * 1000.);
        
        resetTextAudioEmitTracking();
        maintainLapDistanceState();

        context->notifyNewLap();

        emit setNotification(tr("New lap.."), 2);
    }
}

void TrainSidebar::resetLapTimer()
{
    lap_time.restart();
    lap_elapsed_msec = 0;
    displayLapDistance = 0;
    this->resetTextAudioEmitTracking();
    this->maintainLapDistanceState();
}

void TrainSidebar::resetTextAudioEmitTracking()
{
    lapAudioThisLap = true;
    textPositionEmitted = -1;
}

// Can be called from the controller - when user steers to scroll display
void TrainSidebar::steerScroll(int scrollAmount)
{
    if (scrollAmount == 0)
        emit setNotification(tr("Recalibrating steering.."), 10);
    else
        context->notifySteerScroll(scrollAmount);
}

void TrainSidebar::warnnoConfig()
{
    QMessageBox::warning(this, tr("No Devices Configured"), tr("Please configure a device in Preferences."));
}

template<class T, typename T_GetMethod, typename T_SetMethod, typename T_arg>
struct ScopedOp
{
    T* m_pt;
    T_SetMethod m_setMethod;
    T_arg m_argSave;

    ScopedOp(T* pt, T_GetMethod getMethod, T_SetMethod setMethod, T_arg argNew) : m_pt(pt), m_setMethod(setMethod) {
        m_argSave = (m_pt->*getMethod)();
        (m_pt->*m_setMethod)(argNew);
    }

    virtual ~ScopedOp() { (m_pt->*m_setMethod)(m_argSave); }
};

struct ScopedPrecision : ScopedOp<QTextStream, int (QTextStream::*)() const, void (QTextStream::*)(int), int> {
    ScopedPrecision(QTextStream* pc, int tempPrecision) : ScopedOp::ScopedOp(pc, &QTextStream::realNumberPrecision, &QTextStream::setRealNumberPrecision, tempPrecision) {}
};

//----------------------------------------------------------------------
// DISK UPDATE FUNCTIONS
//----------------------------------------------------------------------
void TrainSidebar::diskUpdate()
{
    int  secs;

    long torq = 0;
    QTextStream recordFileStream(recordFile);

    if (calibrating) return;

    // convert from milliseconds to secondes
    total_msecs = session_elapsed_msec + session_time.elapsed();
    secs = round(total_msecs / 1000.0);

    if (secs <= lastRecordSecs) return; // Avoid duplicates
    lastRecordSecs = secs;

    // GoldenCheetah CVS Format "secs, cad, hr, km, kph, nm, watts, alt, lon, lat, headwind, slope, temp, interval, lrbalance, lte, rte, lps, rps, smo2, thb, o2hb, hhb\n";

    recordFileStream    << secs
                        << "," << displayCadence
                        << "," << displayHeartRate
                        << "," << displayDistance
                        << "," << displaySpeed
                        << "," << torq
                        << "," << displayPower;

    // QTextStream default precision is 6, location data needs much more than that.
    // Avoid extra precision for other fields since it isn't needed and would grow
    // the intermediate file for no reason.
    {
        ScopedPrecision tempPrecision(&recordFileStream, 20);

        recordFileStream << "," << displayAltitude
                         << "," << displayLongitude
                         << "," << displayLatitude;
    }

    recordFileStream    << "," // headwind
                        << "," // slope
                        << "," // temp
                        << "," << displayWorkoutLap
                        << "," << displayLRBalance
                        << "," << displayLTE
                        << "," << displayRTE
                        << "," << displayLPS
                        << "," << displayRPS
                        << "," << displaySMO2
                        << "," << displayTHB
                        << "," << displayO2HB
                        << "," << displayHHB
                        << "," << load
                        << "," << "\n";
}

//----------------------------------------------------------------------
// WORKOUT MODE
//----------------------------------------------------------------------

void TrainSidebar::loadUpdate()
{
    int curLap = 0;

    // we hold our horses whilst calibration is taking place...
    if (calibrating) return;

    // the period between loadUpdate calls is not constant, and not exactly LOADRATE,
    // therefore, use a QTime timer to measure the load period
    load_msecs += load_period.restart();

    if (status&RT_MODE_ERGO) {
        if (context->currentErgFile()) {
            load = ergFileQueryAdapter.wattsAt(load_msecs, curLap);

            if (displayWorkoutLap != curLap)
            {
                context->notifyNewLap();
                maintainLapDistanceState();
            }
            displayWorkoutLap = curLap;
        }

        // we got to the end!
        if (load == -100) {
            Stop(DEVICE_OK);
        } else {
            foreach(int dev, activeDevices) Devices[dev].controller->setLoad(load);
            context->notifySetNow(load_msecs);
        }
    } else {
        if (context->currentErgFile()) {
            // Call gradientAt to obtain current lap num.
            ergFileQueryAdapter.gradientAt(displayWorkoutDistance * 1000., curLap);

            if (displayWorkoutLap != curLap) {
                context->notifyNewLap();
                maintainLapDistanceState();
            }

            displayWorkoutLap = curLap;
        }

        // we got to the end!
        if (slope == -100) {
            Stop(DEVICE_OK);
        } else {
            foreach(int dev, activeDevices) {
                Devices[dev].controller->setGradient(slope);
                Devices[dev].controller->setWindResistance(bicycle.WindResistance(displayAltitude));
            }
            context->notifySetNow(displayWorkoutDistance * 1000);
        }
    }
}

void TrainSidebar::Calibrate()
{
    // Check we're running (and not paused) before attempting
    // calibration, buttons should be disabled to prevent this,
    // but could be triggered by remote control..
    if ((status & RT_RUNNING) && ((status&RT_PAUSED) == 0)) {
        toggleCalibration();
        updateCalibration();
    }
}

void TrainSidebar::toggleCalibration()
{
    if (calibrating) {

        // exiting calibration - restart gui etc
        session_time.start();
        lap_time.start();
        load_period.restart();

        clearStatusFlags(RT_CALIBRATING);
        load_timer->start(LOADRATE);
        if (status & RT_RECORDING) disk_timer->start(SAMPLERATE);
        context->notifyUnPause(); // get video started again, amongst other things

        // back to ergo/slope mode and restore load/gradient
        if (status&RT_MODE_ERGO) {

            foreach(int dev, activeDevices) {
                if (calibrationDeviceIndex == dev) {
                    Devices[dev].controller->setCalibrationState(CALIBRATION_STATE_IDLE);
                    Devices[dev].controller->setMode(RT_MODE_ERGO);
                    Devices[dev].controller->setLoad(load);
                }
            }
        } else {

            foreach(int dev, activeDevices) {
                if (calibrationDeviceIndex == dev) {
                    Devices[dev].controller->setCalibrationState(CALIBRATION_STATE_IDLE);
                    Devices[dev].controller->setMode(RT_MODE_SPIN);
                    Devices[dev].controller->setGradient(slope);
                }
            }
        }

    } else {

        // entering calibration - pause gui/load, streaming and recording
        // but keep the gui ticking so we get realtime telemetry for calibration
        session_elapsed_msec += session_time.elapsed();
        lap_elapsed_msec += lap_time.elapsed();

        setStatusFlags(RT_CALIBRATING);
        if (status & RT_RECORDING) disk_timer->stop();
        load_timer->stop();
        load_msecs += load_period.restart();

        context->notifyPause(); // get video started again, amongst other things

        calibrationDeviceIndex = getCalibrationIndex();

        // only do this for the selected device
        foreach(int dev, devices()) {
            if (calibrationDeviceIndex == dev) {
                calibrationType = Devices[dev].controller->getCalibrationType();

                // trainer (tacx vortex smart) doesn't appear to reduce resistance automatically when entering calibration mode
                if (status&RT_MODE_ERGO)
                    Devices[dev].controller->setLoad(0);
                else
                    Devices[dev].controller->setGradient(0);


                Devices[dev].controller->setMode(RT_MODE_CALIBRATE);
                Devices[dev].controller->setCalibrationState(CALIBRATION_STATE_PENDING);
            }
        }

        if (calibrationDeviceIndex == -1)
            qDebug() << "No device(s) found with calibration support";
        else
            qDebug() << "Device" << calibrationDeviceIndex << "being used for calibration";
    }

    startCalibration = restartCalibration = finishCalibration = false;
    calibrating = !calibrating; // toggle calibration
}

void TrainSidebar::updateCalibration()
{
    static QString status;
    static uint8_t lastState, stateCount;

    //qDebug() << "TrainSidebar::updateCalibration()" << calibrating << calibrationState << stateCount;

    if (!calibrating) {

        stateCount = 1;

        // leaving calibration, clear any notification text
        status = QString(tr("Exiting calibration.."));
        emit setNotification(status,3);

    } else {

        // Track how long we've been in the same state
        if (calibrationState == lastState)
            stateCount++;
        else
            stateCount = 1;

        // update message depending on calibration type and state
        switch (calibrationType) {

        case CALIBRATION_TYPE_NOT_SUPPORTED:
            status = QString(tr("Calibration not supported for this device."));
            if ((stateCount % 10) == 0) {
                finishCalibration = true;
            }
            break;

        case CALIBRATION_TYPE_COMPUTRAINER:
            status = QString(tr("Calibrating...\nPress F3 on Controller when done."));
            break;

        case CALIBRATION_TYPE_SPINDOWN:

            switch (calibrationState) {

            case CALIBRATION_STATE_IDLE:
                break;

            case CALIBRATION_STATE_PENDING:
                // Can go straight into spindown calibration
                startCalibration = true;
                break;

            case CALIBRATION_STATE_REQUESTED:
                status = QString(tr("Requesting calibration.."));
                break;

            case CALIBRATION_STATE_STARTING:
                status = QString(tr("Requesting calibration.."));
                // if just spinning here, the device has not responded to calibration request
                if ((stateCount % 5) == 0)
                    restartCalibration = true;
                break;

            case CALIBRATION_STATE_STARTED:
                status = QString(tr("Calibrating..."));
                break;

            case CALIBRATION_STATE_POWER:
                status = QString(tr("Calibrating...\nCurrent speed %1 kph\nIncrease speed to %2 kph")).arg(QString::number(calibrationCurrentSpeed, 'f', 1), QString::number(calibrationTargetSpeed, 'f', 1));
                break;

            case CALIBRATION_STATE_COAST:
                status = QString(tr("Calibrating...\nStop pedalling until speed drops to 0"));
                break;

            case CALIBRATION_STATE_SUCCESS:
                // display zero offset and spindown stats
                status = QString(tr("Calibration completed successfully!\nSpindown %1 ms\nZero Offset %2")).arg(QString::number(calibrationSpindownTime), QString::number(calibrationZeroOffset));;

                // No further ANT messages to set state, so must move ourselves on..
                if ((stateCount % 25) == 0)
                    finishCalibration = true;
                break;

            case CALIBRATION_STATE_FAILURE:
                status = QString(tr("Calibration failed!"));

                // No further ANT messages to set state, so must move ourselves on..
                if ((stateCount % 25) == 0)
                    finishCalibration = true;
                break;

            case CALIBRATION_STATE_FAILURE_SPINDOWN_TOO_FAST:
                status = QString(tr("Calibration Failed: Loosen Roller"));

                // No further ANT messages to set state, so must move ourselves on..
                if ((stateCount % 25) == 0)
                    finishCalibration = true;
                break;

            case CALIBRATION_STATE_FAILURE_SPINDOWN_TOO_SLOW:
                status = QString(tr("Calibration Failed: Tighten Roller"));

                // No further ANT messages to set state, so must move ourselves on..
                if ((stateCount % 25) == 0)
                    finishCalibration = true;
                break;
            }

            break;

        case CALIBRATION_TYPE_ZERO_OFFSET:

            switch (calibrationState) {

            case CALIBRATION_STATE_IDLE:
                break;

            case CALIBRATION_STATE_PENDING:
                // Wait for cadence to be zero before requesting zero offset calibration
                status = QString(tr("Unclip or stop pedalling to begin calibration.."));
                if (calibrationCadence == 0)
                    startCalibration = true;
                break;

            case CALIBRATION_STATE_REQUESTED:
                status = QString(tr("Requesting calibration.."));
                break;

            case CALIBRATION_STATE_STARTING:
                break;

            case CALIBRATION_STATE_STARTED:
                break;

            case CALIBRATION_STATE_POWER:
                break;

            case CALIBRATION_STATE_COAST:
                status = QString(tr("Calibrating...\nUnclip or stop pedalling until process is completed..\nTorque %1")).arg(QString::number(calibrationTorque, 'f', 3));
                break;

            case CALIBRATION_STATE_SUCCESS:
                // yuk, zero offset for FE-C devices is unsigned, but for power meters is signed..
                status = QString(tr("Calibration completed successfully!\nZero Offset %1\nTorque %2")).arg(QString::number((int16_t)calibrationZeroOffset), QString::number(calibrationTorque, 'f', 3));;

                // No further ANT messages to set state, so must move ourselves on..
                if ((stateCount % 25) == 0)
                    finishCalibration = true;
                break;

            case CALIBRATION_STATE_FAILURE:
                status = QString(tr("Calibration failed!"));

                // No further ANT messages to set state, so must move ourselves on..
                if ((stateCount % 25) == 0)
                    finishCalibration = true;
                break;

            }
            break;

        case CALIBRATION_TYPE_ZERO_OFFSET_SRM:

            switch (calibrationState) {

            case CALIBRATION_STATE_IDLE:
                break;

            case CALIBRATION_STATE_PENDING:
                // Wait for cadence to be zero before requesting zero offset calibration
                status = QString(tr("Unclip or stop pedalling to begin calibration.."));
                if (calibrationCadence == 0)
                    startCalibration = true;
                break;

            case CALIBRATION_STATE_REQUESTED:
                status = QString(tr("Requesting calibration.."));
                break;

            case CALIBRATION_STATE_STARTING:
                status = QString(tr("Requesting calibration.."));
                break;

            case CALIBRATION_STATE_STARTED:
                break;

            case CALIBRATION_STATE_POWER:
                break;

            case CALIBRATION_STATE_COAST:
                status = QString(tr("Calibrating...\nUnclip or stop pedalling until process is completed..\nZero Offset %1")).arg(QString::number((int16_t)calibrationZeroOffset));
                break;

            case CALIBRATION_STATE_SUCCESS:
                // yuk, zero offset for FE-C devices is unsigned, but for power meters is signed..
                status = QString(tr("Calibration completed successfully!\nZero Offset %1\nSlope %2")).arg(QString::number((int16_t)calibrationZeroOffset), QString::number(calibrationSlope));;

                // No further ANT messages to set state, so must move ourselves on..
                if ((stateCount % 25) == 0)
                    finishCalibration = true;
                break;

            case CALIBRATION_STATE_FAILURE:
                status = QString(tr("Calibration failed!"));

                // No further ANT messages to set state, so must move ourselves on..
                if ((stateCount % 25) == 0)
                    finishCalibration = true;
                break;

            }
            break;

#ifdef GC_HAVE_LIBUSB
        case CALIBRATION_TYPE_FORTIUS:

            switch (calibrationState) {

            case CALIBRATION_STATE_IDLE:
            case CALIBRATION_STATE_PENDING:
                break;

            case CALIBRATION_STATE_REQUESTED:
                if (calibrationZeroOffset == 0)
                    status = QString(tr("Give the pedal a kick to start calibration...\nThe motor will run until calibration is complete."));
                else
                    status = QString(tr("Allow wheel speed to settle, DO NOT PEDAL...\nThe motor will run until calibration is complete."));

                break;

            case CALIBRATION_STATE_STARTING:
                status = QString(tr("Calibrating... DO NOT PEDAL, remain seated...\nGathering enough samples to calculate average: %1"))
                            .arg(QString::number((int16_t)calibrationZeroOffset));
                break;

            case CALIBRATION_STATE_STARTED:
                {
                    const double calibrationPower_W = Fortius::rawForce_to_N(calibrationZeroOffset) * Fortius::kph_to_ms(calibrationTargetSpeed);
                    status = QString(tr("Calibrating... DO NOT PEDAL, remain seated...\nWaiting for value to stabilize (max %1s): %2 (%3W @ %4kph)"))
                            .arg(QString::number((int16_t)FortiusController::calibrationDurationLimit_s),
                                 QString::number((int16_t)calibrationZeroOffset),
                                 QString::number((int16_t)calibrationPower_W),
                                 QString::number((int16_t)calibrationTargetSpeed));
                }
                break;

            case CALIBRATION_STATE_POWER:
            case CALIBRATION_STATE_COAST:
                break;

            case CALIBRATION_STATE_SUCCESS:
                {
                    const double calibrationPower_W = Fortius::rawForce_to_N(calibrationZeroOffset) * Fortius::kph_to_ms(calibrationTargetSpeed);
                    status = QString(tr("Calibration completed successfully!\nFinal calibration value %1 (%2W @ %3kph)"))
                            .arg(QString::number((int16_t)calibrationZeroOffset),
                                 QString::number((int16_t)calibrationPower_W),
                                 QString::number((int16_t)calibrationTargetSpeed));

                    // No further ANT messages to set state, so must move ourselves on..
                    if ((stateCount % 25) == 0)
                        finishCalibration = true;
                }
                break;

            case CALIBRATION_STATE_FAILURE:
                status = QString(tr("Calibration failed! Do not pedal while calibration is taking place.\nAllow wheel to run freely."));

                // No further ANT messages to set state, so must move ourselves on..
                if ((stateCount % 25) == 0)
                    finishCalibration = true;
                break;
            }
            break;
#endif

        }

        lastState = calibrationState;

        // set notification text, no timeout
        emit setNotification(status, 0);
    }
}

void TrainSidebar::FFwd()
{
    if (((status&RT_RUNNING) == 0) || (status&RT_PAUSED)) return;

    if (status&RT_MODE_ERGO) {
        // In ergo mode seek is of time.
        load_msecs += 10000; // jump forward 10 seconds
        context->notifySeek(load_msecs);
    }
    else {
        // Otherwise Seek is of Distance.
        double stepSize = 1.; // jump forward a kilometer in the workout
        if (context->currentVideoSyncFile()) {
            // If step would take us past the end then step to end.
            double videoDistance = context->currentVideoSyncFile()->Distance;
            if ((displayWorkoutDistance + stepSize) > videoDistance) {
                stepSize = videoDistance - displayWorkoutDistance;
            }
            context->notifySeek(stepSize); // in case of video with RLV file synchronisation just ask to go forward
        }
        displayWorkoutDistance += stepSize;
    }

    resetTextAudioEmitTracking();

    maintainLapDistanceState();

    emit setNotification(tr("Fast forward.."), 2);
}

void TrainSidebar::Rewind()
{
    if (((status&RT_RUNNING) == 0) || (status&RT_PAUSED)) return;

    if (status&RT_MODE_ERGO) {
        // In ergo mode seek is of time.
        load_msecs -=10000; // jump back 10 seconds
        if (load_msecs < 0) load_msecs = 0;
        context->notifySeek(load_msecs);
    }
    else {
        // Otherwise Seek is of distance.
        double stepSize = -1.; // jump back a kilometer

        // If step would take us before the start then step to the start.
        if ((displayWorkoutDistance + stepSize) < 0) {
            stepSize = -displayWorkoutDistance;
        }

        if (context->currentVideoSyncFile()) {
            context->notifySeek(stepSize);
        }

        displayWorkoutDistance += stepSize;
    }

    resetTextAudioEmitTracking();

    maintainLapDistanceState();

    emit setNotification(tr("Rewind.."), 2);
}


// jump to next Lap marker (if there is one?)
void TrainSidebar::FFwdLap()
{
    if (((status&RT_RUNNING) == 0) || (status&RT_PAUSED)) return;

    double lapmarker;

    if (status&RT_MODE_ERGO) {
        lapmarker = ergFileQueryAdapter.nextLap(load_msecs);
        if (lapmarker >= 0.) load_msecs = lapmarker; // jump forward to lapmarker
        context->notifySeek(load_msecs);
    } else {
        static const double s_BeforeOffset = 10.1;
        lapmarker = ergFileQueryAdapter.nextLap((displayWorkoutDistance*1000) + s_BeforeOffset);
        if (lapmarker >= 0) {
            // Go to slightly before lap marker so the lap transition message will be displayed.
            lapmarker = std::max(0., lapmarker - s_BeforeOffset);

            displayWorkoutDistance = lapmarker / 1000; // jump forward to lapmarker
        }
    }

    resetTextAudioEmitTracking();

    maintainLapDistanceState();    

    if (lapmarker >= 0) emit setNotification(tr("Next Lap.."), 2);
}

// jump to next Lap marker (if there is one?)
void TrainSidebar::RewindLap()
{
    if (((status & RT_RUNNING) == 0) || (status & RT_PAUSED)) return;

    double lapmarker;

    if (status & RT_MODE_ERGO) {
        // Search for lap prior to 1 second ago.
        long target = std::max<long>(0, load_msecs - 1000);
        lapmarker = ergFileQueryAdapter.prevLap(target);
        if (lapmarker >= 0.) load_msecs = lapmarker; // jump to lapmarker
        context->notifySeek(load_msecs);
    }
    else {
        // Search for lap prior to 50 meters ago.
        double target = std::max(0., (displayWorkoutDistance * 1000) - 50.);

        lapmarker = ergFileQueryAdapter.prevLap(target);

        // Go to slightly before lap marker so the lap transition message will be displayed.
        lapmarker = std::max(0., lapmarker - 10.1);

        if (lapmarker >= 0.) displayWorkoutDistance = lapmarker / 1000; // jump to lapmarker
    }

    resetTextAudioEmitTracking();

    maintainLapDistanceState();

    if (lapmarker >= 0) emit setNotification(tr("Back Lap.."), 2);
}


// higher load/gradient
void TrainSidebar::Higher()
{
    if ((status&RT_CONNECTED) == 0) return;

    if (context->currentErgFile()) {
        // adjust the workout IF
        adjustIntensity(lastAppliedIntensity+5);

    } else {
        if (status&RT_MODE_ERGO) load += 5;
        else slope += 0.1;

        if (load >1500) load = 1500;
        if (slope >40) slope = 40;

        if (status&RT_MODE_ERGO)
            foreach(int dev, activeDevices) Devices[dev].controller->setLoad(load);
        else
            foreach(int dev, activeDevices) Devices[dev].controller->setGradient(slope);
    }

    emit setNotification(tr("Increasing intensity.."), 2);
}

// lower load/gradient
void TrainSidebar::Lower()
{
    if ((status&RT_CONNECTED) == 0) return;

    if (context->currentErgFile()) {
        // adjust the workout IF
        adjustIntensity(std::max<int>(5, lastAppliedIntensity - 5));

    } else {

        if (status&RT_MODE_ERGO) load -= 5;
        else slope -= 0.1;

        if (load <0) load = 0;
        if (slope <-40) slope = -40;

        if (status&RT_MODE_ERGO)
            foreach(int dev, activeDevices) Devices[dev].controller->setLoad(load);
        else
            foreach(int dev, activeDevices) Devices[dev].controller->setGradient(slope);
    }

    emit setNotification(tr("Decreasing intensity.."), 2);
}

void TrainSidebar::setLabels()
{
    /* should this be kept, or removed? Currently these are always hidden.
    if (context->currentErgFile()) {

        if (context->currentErgFile()->format == CRS) {

            stress->setText(QString("Elevation %1").arg(context->currentErgFile()->ELE, 0, 'f', 0));
            intensity->setText(QString("Grade %1 %").arg(context->currentErgFile()->GRADE, 0, 'f', 1));

        } else {

            stress->setText(QString("BikeStress %1").arg(context->currentErgFile()->BikeStress, 0, 'f', 0));
            intensity->setText(QString("IF %1").arg(context->currentErgFile()->IF, 0, 'f', 3));
        }

    } else {
        stress->setText("");
        intensity->setText("");
    }
    */
}

void TrainSidebar::adjustIntensity(int value)
{
    if (value == lastAppliedIntensity) {
        return;
    }

    ErgFile* ergFile = const_cast<ErgFile*>(ergFileQueryAdapter.getErgFile());
    if (!ergFile) return; // no workout selected

    // block signals temporarily
    context->mainWindow->blockSignals(true);

    // work through the ergFile from NOW
    // adjusting back from last setting
    // and increasing to new intensity setting

    double from = double(lastAppliedIntensity) / 100.00;
    double to = double(value) / 100.00;

    lastAppliedIntensity = value;

    long starttime = context->getNow();

    bool insertedNow = context->getNow() ? false : true; // don't add if at start

    // what about gradient courses?
    ErgFilePoint last;
    for(int i = 0; i < ergFile->Points.count(); i++) {

        if (ergFile->Points.at(i).x >= starttime) {

            if (insertedNow == false) {

                if (i) {
                    // add a point to adjust from

                    // This pass simply modifies load or gradient.
                    // Start with copy of 'last', then overwrite only the part we wish to change,
                    // this is necessary so crs point will start with an intact location and not
                    // zeros.
                    ErgFilePoint add = last;
                    add.x = context->getNow();
                    add.val = last.val / from * to;

                    // recalibrate altitude if gradient changing
                    if (ergFile->format == CRS) add.y = last.y + ((add.x-last.x) * (add.val/100));
                    else add.y = add.val;

                    ergFile->Points.insert(i, add);

                    last = add;
                    i++; // move on to next point (i.e. where we were!)
                }
                insertedNow = true;
            }

            ErgFilePoint *p = &ergFile->Points[i];

            // recalibrate altitude if in CRS mode
            p->val = p->val / from * to;
            if (ergFile->format == CRS) {
                if (i) p->y = last.y + ((p->x-last.x) * (last.val/100));
            }
            else p->y = p->val;
        }

        // remember last
        last = ergFile->Points.at(i);
    }

    // recalculate metrics
    ergFile->calculateMetrics();
    setLabels();

    // Ergfile points have been edited so reset interpolation and
    // query state.
    ergFileQueryAdapter.resetQueryState();

    // unblock signals now we are done
    context->mainWindow->blockSignals(false);

    // force replot
    context->notifySetNow(context->getNow());

    emit intensityChanged(lastAppliedIntensity);
}

MultiDeviceDialog::MultiDeviceDialog(Context *, TrainSidebar *traintool) : traintool(traintool)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("Multiple Device Configuration"));

    QVBoxLayout *main = new QVBoxLayout(this);
    QFormLayout *mainLayout = new QFormLayout;
    main->addLayout(mainLayout);

    bpmSelect = new QComboBox(this);
    mainLayout->addRow(new QLabel("Heartrate", this), bpmSelect);

    wattsSelect = new QComboBox(this);
    mainLayout->addRow(new QLabel("Power", this), wattsSelect);

    rpmSelect = new QComboBox(this);
    mainLayout->addRow(new QLabel("Cadence", this), rpmSelect);

    kphSelect = new QComboBox(this);
    mainLayout->addRow(new QLabel("Speed", this), kphSelect);

    // update the device selections for the drop downs
    foreach(QTreeWidgetItem *selected, traintool->deviceTree->selectedItems()) {
        if (selected->type() == HEAD_TYPE) continue;

        bpmSelect->addItem(selected->text(0), selected->type());
        wattsSelect->addItem(selected->text(0), selected->type());
        rpmSelect->addItem(selected->text(0), selected->type());
        kphSelect->addItem(selected->text(0), selected->type());
    }

    bpmSelect->addItem("None", -1);
    wattsSelect->addItem("None", -1);
    rpmSelect->addItem("None", -1);
    kphSelect->addItem("None", -1);

    // set to the current values (if set)
    if (traintool->bpmTelemetry != -1) {
        int index = bpmSelect->findData(traintool->bpmTelemetry);
        if (index >=0) bpmSelect->setCurrentIndex(index);
    }
    if (traintool->wattsTelemetry != -1) {
        int index = wattsSelect->findData(traintool->wattsTelemetry);
        if (index >=0) wattsSelect->setCurrentIndex(index);
    }
    if (traintool->rpmTelemetry != -1) {
        int index = rpmSelect->findData(traintool->rpmTelemetry);
        if (index >=0) rpmSelect->setCurrentIndex(index);
    }
    if (traintool->kphTelemetry != -1) {
        int index = kphSelect->findData(traintool->kphTelemetry);
        if (index >=0) kphSelect->setCurrentIndex(index);
    }

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch();
    main->addLayout(buttons);

    cancelButton = new QPushButton(tr("Cancel"), this);
    buttons->addWidget(cancelButton);

    applyButton = new QPushButton(tr("Apply"), this);
    buttons->addWidget(applyButton);

    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(applyButton, SIGNAL(clicked()), this, SLOT(applyClicked()));
}

void
MultiDeviceDialog::applyClicked()
{
    traintool->rpmTelemetry = rpmSelect->itemData(rpmSelect->currentIndex()).toInt();
    traintool->bpmTelemetry = bpmSelect->itemData(bpmSelect->currentIndex()).toInt();
    traintool->wattsTelemetry = wattsSelect->itemData(wattsSelect->currentIndex()).toInt();
    traintool->kphTelemetry = kphSelect->itemData(kphSelect->currentIndex()).toInt();
    accept();
}

void
MultiDeviceDialog::cancelClicked()
{
    reject();
}

void
TrainSidebar::devicePopup()
{
    // OK - we are working with a specific event..
    QMenu menu(deviceTree);

    QAction *addDevice = new QAction(tr("Add Device"), deviceTree);
    connect(addDevice, SIGNAL(triggered(void)), context->mainWindow, SLOT(addDevice()));
    menu.addAction(addDevice);

    if (deviceTree->selectedItems().size() == 1) {
        QAction *delDevice = new QAction(tr("Delete Device"), deviceTree);
        connect(delDevice, SIGNAL(triggered(void)), this, SLOT(deleteDevice()));
        menu.addAction(delDevice);
    }

    // execute the menu
    menu.exec(trainSplitter->mapToGlobal(QPoint(deviceItem->pos().x()+deviceItem->width()-20,
                                           deviceItem->pos().y())));
}
void
TrainSidebar::deviceTreeMenuPopup(const QPoint &pos)
{
    QMenu menu(deviceTree);
    QAction *addDevice = new QAction(tr("Add Device"), deviceTree);
    connect(addDevice, SIGNAL(triggered(void)), context->mainWindow, SLOT(addDevice()));
    menu.addAction(addDevice);

    if (deviceTree->selectedItems().size() == 1) {
        QAction *delDevice = new QAction(tr("Delete Device"), deviceTree);
        connect(delDevice, SIGNAL(triggered(void)), this, SLOT(deleteDevice()));
        menu.addAction(delDevice);
    }

    menu.exec(deviceTree->mapToGlobal(pos));
}

void
TrainSidebar::deleteDevice()
{
    // get the configuration
    DeviceConfigurations all;
    QList<DeviceConfiguration>list = all.getList();

    // Delete the selected device
    QTreeWidgetItem *selected = deviceTree->selectedItems().first();
    int index = deviceTree->invisibleRootItem()->indexOfChild(selected);

    if (index < 0 || index > list.size()) return;

    // make sure they really mean this!
    QMessageBox msgBox;
    msgBox.setText(tr("Are you sure you want to delete this device?"));
    msgBox.setInformativeText(list[index].name);
    QPushButton *deleteButton = msgBox.addButton(tr("Delete"),QMessageBox::YesRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();

    if(msgBox.clickedButton() != deleteButton) return;

    // find this one and delete it
    list.removeAt(index);
    all.writeConfig(list);

    // tell everyone
    context->notifyConfigChanged(CONFIG_DEVICES);
}

void
TrainSidebar::moveDevices(int oldposition, int newposition)
{
    // get the configuration
    DeviceConfigurations all;
    QList<DeviceConfiguration>list = all.getList();

    // move the devices
    list.move(oldposition, newposition);
    all.writeConfig(list);

    // tell everyone
    context->notifyConfigChanged(CONFIG_DEVICES);
}

// we have been told to select this video (usually because
// the user just dragndropped it in)
void
TrainSidebar::selectVideo(QString fullpath)
{
    // look at each entry in the top workoutTree
    for (int i=0; i<mediaTree->model()->rowCount(); i++) {
        QString path = mediaTree->model()->data(mediaTree->model()->index(i,0)).toString();
        if (path == fullpath) {
            mediaTree->setCurrentIndex(mediaTree->model()->index(i,0));
            break;
        }
    }
}

void
TrainSidebar::selectVideoSync(QString fullpath)
{
    // look at each entry in the top videosyncTree
    for (int i=0; i<videosyncTree->model()->rowCount(); i++) {
        QString path = videosyncTree->model()->data(videosyncTree->model()->index(i,0)).toString();
        if (path == fullpath) {
            videosyncTree->setCurrentIndex(videosyncTree->model()->index(i,0));
            break;
        }
    }
}

// we have been told to select this workout (usually because
// the user just dragndropped it in)
void
TrainSidebar::selectWorkout(QString fullpath)
{
    // look at each entry in the top workoutTree
    for (int i=0; i<workoutTree->model()->rowCount(); i++) {
        QString path = workoutTree->model()->data(workoutTree->model()->index(i,0)).toString();
        if (path == fullpath) {
            workoutTree->setCurrentIndex(workoutTree->model()->index(i,0));
            break;
        }
    }
}

// got a remote control command
void
TrainSidebar::remoteControl(uint16_t command)
{
    //qDebug() << "TrainSidebar::remoteControl()" << command;

    switch(command){

    case GC_REMOTE_CMD_START:
        this->Start();
        break;

    case GC_REMOTE_CMD_STOP:
        this->Stop();
        break;

    case GC_REMOTE_CMD_LAP:
        this->newLap();
        break;

    case GC_REMOTE_CMD_HIGHER:
        this->Higher();
        break;

    case GC_REMOTE_CMD_LOWER:
        this->Lower();
        break;

    case GC_REMOTE_CMD_CALIBRATE:
        this->Calibrate();
        break;

    default:
        break;
    }
}

// HRV R-R data received
void TrainSidebar::rrData(uint16_t  rrtime, uint8_t count, uint8_t bpm)
{
    Q_UNUSED(count)
    if (status&RT_RECORDING && rrFile == NULL && recordFile != NULL) {
        QString rrfile = recordFile->fileName().replace("csv", "rr");
        //fprintf(stderr, "First r-r, need to open file %s\n", rrfile.toStdString().c_str()); fflush(stderr);

        // setup the rr file
        rrFile = new QFile(rrfile);
        if (!rrFile->open(QFile::WriteOnly | QFile::Truncate)) {
            delete rrFile;
            rrFile=NULL;
        } else {

            // CSV File header
            QTextStream recordFileStream(rrFile);
            recordFileStream << "secs, hr, msecs\n";
        }
    }

    // output a line if recording and file ready
    if (status&RT_RECORDING && rrFile) {
        QTextStream recordFileStream(rrFile);

        // convert from milliseconds to secondes
        double secs = double(session_elapsed_msec + session_time.elapsed()) / 1000.00;

        // output a line
        recordFileStream << secs << ", " << bpm << ", " << rrtime << "\n";
    }
    //fprintf(stderr, "R-R: %d ms, HR=%d, count=%d\n", rrtime, bpm, count); fflush(stderr);
}

// VO2 Measurement data received
void TrainSidebar::vo2Data(double rf, double rmv, double vo2, double vco2, double tv, double feo2)
{
    if (status&RT_RECORDING && vo2File == NULL && recordFile != NULL) {
        QString vo2filename = recordFile->fileName().replace("csv", "vo2");

        // setup the rr file
        vo2File = new QFile(vo2filename);
        if (!vo2File->open(QFile::WriteOnly | QFile::Truncate)) {
            delete vo2File;
            vo2File=NULL;
        } else {

            // CSV File header
            QTextStream recordFileStream(vo2File);
            recordFileStream << "secs, rf, rmv, vo2, vco2, tv, feo2\n";
        }
    }

    // output a line if recording and file ready
    if (status&RT_RECORDING && vo2File) {
        QTextStream recordFileStream(vo2File);

        // convert from milliseconds to secondes
        double secs = double(session_elapsed_msec + session_time.elapsed()) / 1000.00;

        // output a line
        recordFileStream << secs << ", " << rf << ", " << rmv << ", " << vo2 << ", " << vco2 << ", " << tv << ", " << feo2 << "\n";
    }
}

// connect/disconnect automatically when view changes
void
TrainSidebar::viewChanged(int index)
{
    //qDebug() << "view has changed to:" << index;

    // ensure buttons reflect current state
    setStatusFlags(0);

    if (!autoConnect) return;

    // fixme: value hard-coded throughout
    if (index == 3) {
        //train view
        if ((status&RT_CONNECTED) == 0) {
            Connect();
        }
    } else {
        // other view
        if ((status&RT_CONNECTED) && (status&RT_RUNNING) == 0) {
            Disconnect();
        }
    }

}

void TrainSidebar::setStatusFlags(int flags)
{
    status |= flags;
    context->isRunning = (status&RT_RUNNING);
    context->isPaused  = (status&RT_PAUSED);

    emit statusChanged(status);
}

void TrainSidebar::clearStatusFlags(int flags)
{
    status &=~flags;
    context->isRunning = (status&RT_RUNNING);
    context->isPaused  = (status&RT_PAUSED);

    emit statusChanged(status);
}

int TrainSidebar::getCalibrationIndex()
{
    // select the least recently calibrated GC device that reports calibration capabilities.
    //
    // note that for for ANT devices, the calibration class handles the case of multiple sensors
    // with support for calibration, and will select a suitable device/channel for calibration each time.

    int   index = -1;
    QTime lastCal = QTime::currentTime();

    foreach(int dev, devices()) {
        if (Devices[dev].controller->getCalibrationType()) {
            // device supports calibration
            //qDebug() << "Device" << dev << "supports calibration, last cal attempt timestamp is" << Devices[dev].controller->getCalibrationTimestamp();
            if (Devices[dev].controller->getCalibrationTimestamp() < lastCal) {
                // older (or no) calibration timestamp, select this device
                lastCal = Devices[dev].controller->getCalibrationTimestamp();
                index = dev;
            }
        }
    }
    if (index != -1)
        Devices[index].controller->setCalibrationTimestamp();

    return index;
}

DeviceTreeView::DeviceTreeView()
{
    setDragDropMode(QAbstractItemView::InternalMove);
    setDragDropOverwriteMode(true);
}

void
DeviceTreeView::dropEvent(QDropEvent* event)
{
    // get the list of the items that are about to be dropped
    QTreeWidgetItem* item = selectedItems()[0];

    // row we started on
    int idx1 = indexFromItem(item).row();

    // the default implementation takes care of the actual move inside the tree
    QTreeWidget::dropEvent(event);

    // moved to !
    int idx2 = indexFromItem(item).row();

    // notify subscribers in some useful way
    Q_EMIT itemMoved(idx1, idx2);
}
