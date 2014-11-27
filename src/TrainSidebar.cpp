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
#include <QApplication>
#include <QtGui>
#include <QRegExp>
#include <QStyle>
#include <QStyleFactory>
#include <QScrollBar>

// Three current realtime device types supported are:
#include "RealtimeController.h"
#include "ComputrainerController.h"
#include "ANTlocalController.h"
#include "NullController.h"
#ifdef GC_HAVE_WFAPI
#include "KickrController.h"
#endif
#ifdef GC_HAVE_LIBUSB
#include "FortiusController.h"
#endif

// Media selection helper
#ifndef Q_OS_MAC
#include "VideoWindow.h"
#endif

#ifdef Q_OS_MAC
#include "QtMacVideoWindow.h"
#include <CoreServices/CoreServices.h>
#include <QStyle>
#include <QStyleFactory>
#endif

#include <math.h> // isnan and isinf
#include "TrainDB.h"
#include "Library.h"

TrainSidebar::TrainSidebar(Context *context) : GcWindow(context), context(context)
{
    QWidget *c = new QWidget;
    //c->setContentsMargins(0,0,0,0); // bit of space is useful
    QVBoxLayout *cl = new QVBoxLayout(c);
    setControls(c);

    cl->setSpacing(0);
    cl->setContentsMargins(0,0,0,0);

    // don't set the source for telemetry
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
#endif

    deviceTree = new QTreeWidget;
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

    // TOOLBAR BUTTONS ETC
    QHBoxLayout *toolallbuttons=new QHBoxLayout; // on toolbar
    toolallbuttons->setSpacing(0);
    toolallbuttons->setContentsMargins(0,0,0,0);

    QHBoxLayout *toolbuttons = new QHBoxLayout;
    toolallbuttons->addLayout(toolbuttons);
    toolbuttons->setSpacing(0);
    toolbuttons->setContentsMargins(0,0,0,0);
    toolbuttons->addStretch();

    QIcon rewIcon(":images/oxygen/rewind.png");
    QPushButton *rewind = new QPushButton(rewIcon, "", this);
    rewind->setFocusPolicy(Qt::NoFocus);
    rewind->setIconSize(QSize(64,64));
    rewind->setAutoFillBackground(false);
    rewind->setAutoDefault(false);
    rewind->setFlat(true);
    rewind->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    rewind->setAutoRepeat(true);
    rewind->setAutoRepeatDelay(200);
    toolbuttons->addWidget(rewind);

    QIcon stopIcon(":images/oxygen/stop.png");
    QPushButton *stop = new QPushButton(stopIcon, "", this);
    stop->setFocusPolicy(Qt::NoFocus);
    stop->setIconSize(QSize(64,64));
    stop->setAutoFillBackground(false);
    stop->setAutoDefault(false);
    stop->setFlat(true);
    stop->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    toolbuttons->addWidget(stop);

    QIcon playIcon(":images/oxygen/play.png");
    play = new QPushButton(playIcon, "", this);
    play->setFocusPolicy(Qt::NoFocus);
    play->setIconSize(QSize(64,64));
    play->setAutoFillBackground(false);
    play->setAutoDefault(false);
    play->setFlat(true);
    play->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    toolbuttons->addWidget(play);

    QIcon fwdIcon(":images/oxygen/ffwd.png");
    QPushButton *forward = new QPushButton(fwdIcon, "", this);
    forward->setFocusPolicy(Qt::NoFocus);
    forward->setIconSize(QSize(64,64));
    forward->setAutoFillBackground(false);
    forward->setAutoDefault(false);
    forward->setFlat(true);
    forward->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    forward->setAutoRepeat(true);
    forward->setAutoRepeatDelay(200);
    toolbuttons->addWidget(forward);

    QIcon lapIcon(":images/oxygen/lap.png");
    QPushButton *lap = new QPushButton(lapIcon, "", this);
    lap->setFocusPolicy(Qt::NoFocus);
    lap->setIconSize(QSize(64,64));
    lap->setAutoFillBackground(false);
    lap->setAutoDefault(false);
    lap->setFlat(true);
    lap->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    toolbuttons->addWidget(lap);
    toolbuttons->addStretch();

    QHBoxLayout *slideLayout = new QHBoxLayout;
    slideLayout->setSpacing(0);
    slideLayout->setContentsMargins(0,0,0,0);
    toolallbuttons->addLayout(slideLayout);

    intensitySlider = new QSlider(Qt::Horizontal, this);
    intensitySlider->setAutoFillBackground(false);
    intensitySlider->setFocusPolicy(Qt::NoFocus);
    intensitySlider->setMinimum(75);
    intensitySlider->setMaximum(125);
    intensitySlider->setValue(100);
    slideLayout->addStretch();
    slideLayout->addWidget(intensitySlider);
intensitySlider->hide(); //XXX!!! temporary

#ifdef Q_OS_MAC
#if QT_VERSION > 0x5000
    QStyle *macstyler = QStyleFactory::create("fusion");
#else
    QStyle *macstyler = QStyleFactory::create("Cleanlooks");
#endif
    play->setStyle(macstyler);
    stop->setStyle(macstyler);
    rewind->setStyle(macstyler);
    forward->setStyle(macstyler);
    lap->setStyle(macstyler);
#endif

    QPalette pal;
    stress = new QLabel(this);
    stress->setAutoFillBackground(false);
    stress->setFixedWidth(100);
    stress->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    pal.setColor(stress->foregroundRole(), Qt::white);
    stress->setPalette(pal);
stress->hide(); //XXX!!! temporary

    intensity = new QLabel(this);
    intensity->setAutoFillBackground(false);
    intensity->setFixedWidth(100);
    intensity->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    pal.setColor(intensity->foregroundRole(), Qt::white);
    intensity->setPalette(pal);
intensity->hide(); //XXX!!! temporary

    slideLayout->addWidget(stress, Qt::AlignVCenter|Qt::AlignCenter);
    slideLayout->addWidget(intensity, Qt::AlignVCenter|Qt::AlignCenter);
    slideLayout->addStretch();

    toolbarButtons = new QWidget(this);
    toolbarButtons->setContentsMargins(0,0,0,0);
    toolbarButtons->setFocusPolicy(Qt::NoFocus);
    toolbarButtons->setAutoFillBackground(false);
    toolbarButtons->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    toolbarButtons->setLayout(toolallbuttons);

    connect(play, SIGNAL(clicked()), this, SLOT(Start()));
    connect(stop, SIGNAL(clicked()), this, SLOT(Stop()));
    connect(forward, SIGNAL(clicked()), this, SLOT(FFwd()));
    connect(rewind, SIGNAL(clicked()), this, SLOT(Rewind()));
    connect(lap, SIGNAL(clicked()), this, SLOT(newLap()));
    connect(context, SIGNAL(newLap()), this, SLOT(resetLapTimer()));
    connect(intensitySlider, SIGNAL(valueChanged(int)), this, SLOT(adjustIntensity()));

    // not used but kept in case re-instated in the future
    recordSelector = new QCheckBox(this);
    recordSelector->setText(tr("Save workout data"));
    recordSelector->setChecked(Qt::Checked);
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
    trainSplitter->addWidget(deviceItem);
    workoutItem->addWidget(workoutTree);
    trainSplitter->addWidget(workoutItem);
    cl->addWidget(trainSplitter);


#if !defined GC_VIDEO_NONE
    mediaItem = new GcSplitterItem(tr("Media"), iconFromPNG(":images/sidebar/movie.png"), this);
    QAction *moreMediaAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    mediaItem->addAction(moreMediaAct);
    connect(moreMediaAct, SIGNAL(triggered(void)), this, SLOT(mediaPopup(void)));
    mediaItem->addWidget(mediaTree);
    trainSplitter->addWidget(mediaItem);
#endif
    trainSplitter->prepare(context->athlete->cyclist, "train");

#ifdef Q_OS_MAC
    // get rid of annoying focus rectangle for sidebar components
#if !defined GC_VIDEO_NONE
    mediaTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    workoutTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
    deviceTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

    // handle config changes
    connect(deviceTree,SIGNAL(itemSelectionChanged()), this, SLOT(deviceTreeWidgetSelectionChanged()));
    connect(deviceTree,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(deviceTreeMenuPopup(const QPoint &)));

#if !defined GC_VIDEO_NONE
    connect(mediaTree->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
                            this, SLOT(mediaTreeWidgetSelectionChanged()));
    connect(context, SIGNAL(selectMedia(QString)), this, SLOT(selectVideo(QString)));
#endif
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(context, SIGNAL(selectWorkout(QString)), this, SLOT(selectWorkout(QString)));
    connect(trainDB, SIGNAL(dataChanged()), this, SLOT(refresh()));

    connect(workoutTree->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(workoutTreeWidgetSelectionChanged()));
    // add a watch on all directories
    QVariant workoutDir = appsettings->value(NULL, GC_WORKOUTDIR);

    // set home
    //XXX ??? main = parent;
    ergFile = NULL;
    calibrating = false;

    // now the GUI is setup lets sort our control variables
    gui_timer = new QTimer(this);
    disk_timer = new QTimer(this);
    load_timer = new QTimer(this);

    session_time = QTime();
    session_elapsed_msec = 0;
    lap_time = QTime();
    lap_elapsed_msec = 0;

    recordFile = NULL;
    status = 0;
    status |= RT_MODE_ERGO;         // ergo mode by default
    mode = ERG;

    displayWorkoutLap = displayLap = 0;
    pwrcount = 0;
    cadcount = 0;
    hrcount = 0;
    spdcount = 0;
    lodcount = 0;
    wbalr = wbal = 0;
    load_msecs = total_msecs = lap_msecs = 0;
    displayWorkoutDistance = displayDistance = displayPower = displayHeartRate =
    displaySpeed = displayCadence = slope = load = 0;

    connect(gui_timer, SIGNAL(timeout()), this, SLOT(guiUpdate()));
    connect(disk_timer, SIGNAL(timeout()), this, SLOT(diskUpdate()));
    connect(load_timer, SIGNAL(timeout()), this, SLOT(loadUpdate()));

    configChanged(); // will reset the workout tree
    setLabels();

#ifndef Q_OS_MAC
    toolbarButtons->hide();
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

    // refresh data
    videoModel->select();
    while (videoModel->canFetchMore(QModelIndex())) videoModel->fetchMore(QModelIndex());

    // restore selection
    selectVideo(videoPath);
#endif

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
    QAction *wizard = new QAction(tr("Create Workout via Wizard"), workoutTree);
    QAction *scan = new QAction(tr("Scan for Workouts"), workoutTree);

    menu.addAction(import);
    menu.addAction(download);
    menu.addAction(wizard);
    menu.addAction(scan);

    // we can delete too
    QModelIndex current = workoutTree->currentIndex();
    QModelIndex target = sortModel->mapToSource(current);
    QString filename = workoutModel->data(workoutModel->index(target.row(), 0), Qt::DisplayRole).toString();
    if (QFileInfo(filename).exists()) {
        QAction *del = new QAction(tr("Delete selected workout"), workoutTree);
        menu.addAction(del);
        connect(del, SIGNAL(triggered(void)), this, SLOT(deleteWorkouts(void)));
    }

    // connect menu to functions
    connect(import, SIGNAL(triggered(void)), context->mainWindow, SLOT(importWorkout(void)));
    connect(wizard, SIGNAL(triggered(void)), context->mainWindow, SLOT(showWorkoutWizard(void)));
    connect(download, SIGNAL(triggered(void)), context->mainWindow, SLOT(downloadErgDB(void)));
    connect(scan, SIGNAL(triggered(void)), context->mainWindow, SLOT(manageLibrary(void)));

    // execute the menu
    menu.exec(trainSplitter->mapToGlobal(QPoint(workoutItem->pos().x()+workoutItem->width()-20,
                                           workoutItem->pos().y())));
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
        QAction *del = new QAction(tr("Delete selected video"), workoutTree);
        menu.addAction(del);
        connect(del, SIGNAL(triggered(void)), this, SLOT(deleteVideos(void)));
    }

    // execute the menu
    menu.exec(trainSplitter->mapToGlobal(QPoint(mediaItem->pos().x()+mediaItem->width()-20,
                                           mediaItem->pos().y())));
}

void
TrainSidebar::configChanged()
{
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));
#if !defined GC_VIDEO_NONE
    mediaTree->setStyleSheet(GCColor::stylesheet());
#endif
    workoutTree->setStyleSheet(GCColor::stylesheet());
    deviceTree->setStyleSheet(GCColor::stylesheet());

    // DEVICES

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
#ifdef GC_HAVE_LIBUSB
        } else if (Devices.at(i).type == DEV_FORTIUS) {
            Devices[i].controller = new FortiusController(this, &Devices[i]);
#endif
        } else if (Devices.at(i).type == DEV_NULL) {
            Devices[i].controller = new NullController(this, &Devices[i]);
        } else if (Devices.at(i).type == DEV_ANTLOCAL) {
            Devices[i].controller = new ANTlocalController(this, &Devices[i]);
#ifdef GC_HAVE_WFAPI
        } else if (Devices.at(i).type == DEV_KICKR) {
            Devices[i].controller = new KickrController(this, &Devices[i]);
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

    // Athlete
    FTP=285; // default to 285 if zones are not set
    WPRIME = 20000;

    int range = context->athlete->zones()->whichRange(QDate::currentDate());
    if (range != -1) {
        FTP = context->athlete->zones()->getCP(range);
        WPRIME = context->athlete->zones()->getWprime(range);
    }
}

/*----------------------------------------------------------------------
 * Device Selected
 *--------------------------------------------------------------------*/
void
TrainSidebar::deviceTreeWidgetSelectionChanged()
{
    bpmTelemetry = wattsTelemetry = kphTelemetry = rpmTelemetry = -1;
    deviceSelected();
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

    // wip away the current selected workout
    if (ergFile) {
        delete ergFile;
        ergFile = NULL;
    }

    if (filename == "") {
        context->notifyErgFileSelected(NULL);
        return;
    }

    // is it the auto mode?
    int index = target.row();
    if (index == 0) {
        // ergo mode
        context->notifyErgFileSelected(NULL);
        mode = ERG;
        setLabels();
        status &= ~RT_WORKOUT;
        //ergPlot->setVisible(false);
    } else if (index == 1) {
        // slope mode
        context->notifyErgFileSelected(NULL);
        mode = CRS;
        setLabels();
        status &= ~RT_WORKOUT;
        //ergPlot->setVisible(false);
    } else {
        // workout mode
        ergFile = new ErgFile(filename, mode, context);
        if (ergFile->isValid()) {

            status |= RT_WORKOUT;

            // success! we have a load file
            // setup the course profile in the
            // display!
            context->notifyErgFileSelected(ergFile);
            intensitySlider->setValue(100);
            lastAppliedIntensity = 100;
            setLabels();
        } else {

            // couldn't parse fall back to ERG mode
            delete ergFile;
            ergFile = NULL;
            context->notifyErgFileSelected(NULL);
            mode = ERG;
            status &= ~RT_WORKOUT;
            setLabels();
        }
    }

    // set the device to the right mode
    if (mode == ERG || mode == MRC) {
        status |= RT_MODE_ERGO;
        status &= ~RT_MODE_SPIN;

        // update every active device
        foreach(int dev, devices()) Devices[dev].controller->setMode(RT_MODE_ERGO);

    } else { // SLOPE MODE
        status |= RT_MODE_SPIN;
        status &= ~RT_MODE_ERGO;

        // update every active device
        foreach(int dev, devices()) Devices[dev].controller->setMode(RT_MODE_SPIN);
    }
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
        msgBox.setText(tr("Are you sure you want to delete this video?"));
        msgBox.setInformativeText(filename);
        QPushButton *deleteButton = msgBox.addButton(tr("Delete"),QMessageBox::YesRole);
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
TrainSidebar::deleteWorkouts()
{
    QModelIndex current = workoutTree->currentIndex();
    QModelIndex target = sortModel->mapToSource(current);
    QString filename = workoutModel->data(workoutModel->index(target.row(), 0), Qt::DisplayRole).toString();

    if (QFileInfo(filename).exists()) {
        // are you sure?
        QMessageBox msgBox;
        msgBox.setText(tr("Are you sure you want to delete this workout?"));
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
        trainDB->deleteWorkout(filename);
        trainDB->endLUW();
    }
}

void
TrainSidebar::mediaTreeWidgetSelectionChanged()
{

    QModelIndex current = mediaTree->currentIndex();
    QModelIndex target = vsortModel->mapToSource(current);
    QString filename = videoModel->data(videoModel->index(target.row(), 0), Qt::DisplayRole).toString();
    context->notifyMediaSelected(filename);
}

/*--------------------------------------------------------------------------------
 * Was realtime window, now local and manages controller and chart updates etc
 *------------------------------------------------------------------------------*/

void TrainSidebar::Start()       // when start button is pressed
{
    static QIcon playIcon(":images/oxygen/play.png");
    static QIcon pauseIcon(":images/oxygen/pause.png");

    if (status&RT_PAUSED) {

        // UN PAUSE!
        play->setIcon(pauseIcon);

        session_time.start();
        lap_time.start();
        status &=~RT_PAUSED;
        foreach(int dev, devices()) Devices[dev].controller->restart();
        gui_timer->start(REFRESHRATE);
        if (status & RT_RECORDING) disk_timer->start(SAMPLERATE);
        load_period.restart();
        if (status & RT_WORKOUT) load_timer->start(LOADRATE);

#if !defined GC_VIDEO_NONE
        mediaTree->setEnabled(false);
#endif

        // tell the world
        context->notifyUnPause();

    } else if (status&RT_RUNNING) {

        // Pause!
        play->setIcon(playIcon);

        session_elapsed_msec += session_time.elapsed();
        lap_elapsed_msec += lap_time.elapsed();
        foreach(int dev, devices()) Devices[dev].controller->pause();
        status |=RT_PAUSED;
        gui_timer->stop();
        if (status & RT_RECORDING) disk_timer->stop();
        if (status & RT_WORKOUT) load_timer->stop();
        load_msecs += load_period.restart();

#if !defined GC_VIDEO_NONE
        // enable media tree so we can change movie - mid workout
        mediaTree->setEnabled(true);
#endif

        // tell the world
        context->notifyPause();

    } else {

        // Stop users from selecting different devices
        // media or workouts whilst a workout is in progress

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

#if !defined GC_VIDEO_NONE
        mediaTree->setEnabled(false);
#endif
        workoutTree->setEnabled(false);
        deviceTree->setEnabled(false);

        // START!
        play->setIcon(pauseIcon);

        load = 0;
        slope = 0.0;

        if (mode == ERG || mode == MRC) {
            status |= RT_MODE_ERGO;
            status &= ~RT_MODE_SPIN;
            foreach(int dev, devices()) Devices[dev].controller->setMode(RT_MODE_ERGO);
        } else { // SLOPE MODE
            status |= RT_MODE_SPIN;
            status &= ~RT_MODE_ERGO;
            foreach(int dev, devices()) Devices[dev].controller->setMode(RT_MODE_SPIN);
        }

        foreach(int dev, devices()) Devices[dev].controller->start();

        // tell the world
        context->notifyStart();

        // we're away!
        status |=RT_RUNNING;

        load_period.restart();
        session_time.start();
        session_elapsed_msec = 0;
        lap_time.start();
        lap_elapsed_msec = 0;
        wbalr = 0;
        wbal = WPRIME;
        calibrating = false;

        if (status & RT_WORKOUT) {
            load_timer->start(LOADRATE);      // start recording
        }

        if (recordSelector->isChecked()) {
            status |= RT_RECORDING;
        }

        if (status & RT_RECORDING) {
            QDateTime now = QDateTime::currentDateTime();

            // setup file
            QString filename = now.toString(QString("yyyy_MM_dd_hh_mm_ss")) + QString(".csv");

            if (!context->athlete->home->records().exists())
                context->athlete->home->createAllSubdirs();

            QString fulltarget = context->athlete->home->records().canonicalPath() + "/" + filename;

            if (recordFile) delete recordFile;
            recordFile = new QFile(fulltarget);
            if (!recordFile->open(QFile::WriteOnly | QFile::Truncate)) {
                status &= ~RT_RECORDING;
            } else {

                // CSV File header

                QTextStream recordFileStream(recordFile);
                recordFileStream << "secs, cad, hr, km, kph, nm, watts, alt, lon, lat, headwind, slope, temp, interval, lrbalance, lte, rte, lps, rps, smo2, thb, o2hb, hhb\n";

                disk_timer->start(SAMPLERATE);  // start screen
            }
        }
        gui_timer->start(REFRESHRATE);      // start recording

    }
}

void TrainSidebar::Pause()        // pause capture to recalibrate
{
    // we're not running fool!
    if ((status&RT_RUNNING) == 0) return;

    if (status&RT_PAUSED) {

        session_time.start();
        lap_time.start();
        status &=~RT_PAUSED;
        foreach(int dev, devices()) Devices[dev].controller->restart();
        gui_timer->start(REFRESHRATE);
        if (status & RT_RECORDING) disk_timer->start(SAMPLERATE);
        load_period.restart();
        if (status & RT_WORKOUT) load_timer->start(LOADRATE);

#if !defined GC_VIDEO_NONE
        mediaTree->setEnabled(false);
#endif

        // tell the world
        context->notifyUnPause();

    } else {

        session_elapsed_msec += session_time.elapsed();
        lap_elapsed_msec += lap_time.elapsed();
        foreach(int dev, devices()) Devices[dev].controller->pause();
        status |=RT_PAUSED;
        gui_timer->stop();
        if (status & RT_RECORDING) disk_timer->stop();
        if (status & RT_WORKOUT) load_timer->stop();
        load_msecs += load_period.restart();

        // enable media tree so we can change movie
#if !defined GC_VIDEO_NONE
        mediaTree->setEnabled(true);
#endif

        // tell the world
        context->notifyPause();
    }
}

void TrainSidebar::Stop(int deviceStatus)        // when stop button is pressed
{
    if ((status&RT_RUNNING) == 0) return;

    status &= ~(RT_RUNNING|RT_PAUSED);

    // Stop users from selecting different devices
    // media or workouts whilst a workout is in progress
#if !defined GC_VIDEO_NONE
    mediaTree->setEnabled(true);
#endif
    workoutTree->setEnabled(true);
    deviceTree->setEnabled(true);

    // wipe connection
    foreach(int dev, devices()) Devices[dev].controller->stop();

    gui_timer->stop();
    calibrating = false;

    load = 0;
    slope = 0.0;

    QDateTime now = QDateTime::currentDateTime();

    if (status & RT_RECORDING) {
        disk_timer->stop();

        // close and reset File
        recordFile->close();

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
    }

    if (status & RT_WORKOUT) {
        load_timer->stop();
        load_msecs = 0;
    }

    // get back to normal after it may have been adusted by the user
    lastAppliedIntensity=100;
    intensitySlider->setValue(100);
    if (context->currentErgFile()) context->currentErgFile()->reload();
    context->notifySetNow(load_msecs);

    // reset the play button
    QIcon playIcon(":images/oxygen/play.png");
    play->setIcon(playIcon);

    // tell the world
    context->notifyStop();

    // Re-enable gui elements
    // reset counters etc
    pwrcount = 0;
    cadcount = 0;
    hrcount = 0;
    spdcount = 0;
    lodcount = 0;
    displayWorkoutLap = displayLap =0;
    wbalr = 0;
    wbal = WPRIME;
    session_elapsed_msec = 0;
    session_time.restart();
    lap_elapsed_msec = 0;
    lap_time.restart();
    displayWorkoutDistance = displayDistance = 0;
    guiUpdate();

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
    // Gradient not supported
    return;
}

//----------------------------------------------------------------------
// SCREEN UPDATE FUNCTIONS
//----------------------------------------------------------------------

void TrainSidebar::guiUpdate()           // refreshes the telemetry
{
    RealtimeData rtData;
    rtData.setLap(displayLap + displayWorkoutLap); // user laps + predefined workout lap
    rtData.mode = mode;

    // On a Mac prevent the screensaver from kicking in
    // this is apparently the 'supported' mechanism for
    // disabling the screen saver on a Mac instead of
    // temporarily adjusting/disabling the user preferences
    // for screen saving and power management. Makes sense.
#ifdef Q_OS_MAC
    UpdateSystemActivity(OverallAct);
#endif

    // get latest telemetry from devices
    if (status&RT_RUNNING) {
        if(calibrating) {
            foreach(int dev, devices()) { // Do for Computrainers only.  Need to check with other devices
                RealtimeData local = rtData;
                if (Devices[dev].type == DEV_CT)
                  Devices[dev].controller->getRealtimeData(local); // See if the F3 button has been pressed
            }
            // and exit.  Nothing else to do until we finish calibrating
            return;
        } else {
            rtData.setLoad(load); // always set load..
            rtData.setSlope(slope); // always set load..

            // fetch the right data from each device...
            foreach(int dev, devices()) {

                RealtimeData local = rtData;
                Devices[dev].controller->getRealtimeData(local);

                // get spinscan data from a computrainer?
                if (Devices[dev].type == DEV_CT) {
                    memcpy((uint8_t*)rtData.spinScan, (uint8_t*)local.spinScan, 24);
                    rtData.setLoad(local.getLoad()); // and get load in case it was adjusted
                    rtData.setSlope(local.getSlope()); // and get slope in case it was adjusted
                    // to within defined limits
                }

				if (Devices[dev].type == DEV_FORTIUS) {
	                rtData.setLoad(local.getLoad()); // and get load in case it was adjusted
                    rtData.setSlope(local.getSlope()); // and get slope in case it was adjusted	
					// to within defined limits					
				}

                if (Devices[dev].type == DEV_ANTLOCAL) {
                    rtData.setHb(local.getSmO2(), local.gettHb()); //only moxy data from ant devices right now
                }
				
                // what are we getting from this one?
                if (dev == bpmTelemetry) rtData.setHr(local.getHr());
                if (dev == rpmTelemetry) rtData.setCadence(local.getCadence());
                if (dev == kphTelemetry) {
                    rtData.setSpeed(local.getSpeed());
                    rtData.setDistance(local.getDistance());
                }
                if (dev == wattsTelemetry) {
                    rtData.setWatts(local.getWatts());
                    rtData.setAltWatts(local.getAltWatts());
                }
            }

            // Distance assumes current speed for the last second. from km/h to km/sec
            displayDistance += displaySpeed / (5 * 3600); // assumes 200ms refreshrate
            displayWorkoutDistance += displaySpeed / (5 * 3600); // assumes 200ms refreshrate
            rtData.setDistance(displayDistance);

            // time
            total_msecs = session_elapsed_msec + session_time.elapsed();
            lap_msecs = lap_elapsed_msec + lap_time.elapsed();

            rtData.setMsecs(total_msecs);
            rtData.setLapMsecs(lap_msecs);

            long lapTimeRemaining;
            if (ergFile) lapTimeRemaining = ergFile->nextLap(load_msecs) - load_msecs;
            else lapTimeRemaining = 0;

            if(lapTimeRemaining < 0) {
                    if (ergFile) lapTimeRemaining =  ergFile->Duration - load_msecs;
                    if(lapTimeRemaining < 0)
                        lapTimeRemaining = 0;
            }
            rtData.setLapMsecsRemaining(lapTimeRemaining);

            // local stuff ...
            displayPower = rtData.getWatts();
            displayCadence = rtData.getCadence();
            displayHeartRate = rtData.getHr();
            displaySpeed = rtData.getSpeed();
            load = rtData.getLoad();
            slope = rtData.getSlope();

            // virtual speed
            double crr = 0.004f; // typical for asphalt surfaces
            double g = 9.81;     // g constant 9.81 m/s
            double weight = appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT, 0.0).toDouble();
            double m = weight ? weight + 8 : 83; // default to 75kg weight, plus 8kg bike
            double sl = slope / 100; // 10% = 0.1
            double ad = 1.226f; // default air density at sea level
            double cdA = 0.5f; // typical
            double pw = rtData.getWatts();

            // algorithm supplied by Tom Compton
            // from www.AnalyticCycling.com
            // 3.6 * ... converts from meters per second to kph
            double vs = 3.6f * (
            (-2*pow(2,0.3333333333333333)*(crr*m + g*m*sl)) /
                pow(54*pow(ad,2)*pow(cdA,2)*pw +
                sqrt(2916*pow(ad,4)*pow(cdA,4)*pow(pw,2) +
                864*pow(ad,3)*pow(cdA,3)*pow(crr*m +
                g*m*sl,3)),0.3333333333333333) +
                pow(54*pow(ad,2)*pow(cdA,2)*pw +
                sqrt(2916*pow(ad,4)*pow(cdA,4)*pow(pw,2) +
                864*pow(ad,3)*pow(cdA,3)*pow(crr*m +
                g*m*sl,3)),0.3333333333333333)/
                (3.*pow(2,0.3333333333333333)*ad*cdA));

            // just in case...
            if (isnan(vs) || isinf(vs)) vs = 0.00f;
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

            // set now to current time when not using a workout
            // but limit to almost every second (account for
            // slight timing errors of 100ms or so)

            // Do some rounding to the hundreds because as time goes by, rtData.getMsecs() drifts just below and then it does not pass the mod 1000 < 100 test
            // For example:  msecs = 42199.  Mod 1000 = 199 versus msecs = 42000.  Mod 1000 = 0
            // With this, it will now call tick just about every second
            long rmsecs = round((rtData.getMsecs() + 99) / 100) * 100;
            // Test for <= 100ms
            if (!(status&RT_WORKOUT) && ((rmsecs % 1000) <= 100)) {
                context->notifySetNow(rtData.getMsecs());
            }
        }
    }
}

// can be called from the controller - when user presses "Lap" button
void TrainSidebar::newLap()
{
    if ((status&RT_RUNNING) == RT_RUNNING) {
        displayLap++;

        pwrcount  = 0;
        cadcount  = 0;
        hrcount   = 0;
        spdcount  = 0;

        context->notifyNewLap();
    }
}

void TrainSidebar::resetLapTimer()
{
    lap_time.restart();
    lap_elapsed_msec = 0;
}

// can be called from the controller
void TrainSidebar::nextDisplayMode()
{
}

void TrainSidebar::warnnoConfig()
{
    QMessageBox::warning(this, tr("No Devices Configured"), "Please configure a device in Preferences.");
}

//----------------------------------------------------------------------
// DISK UPDATE FUNCTIONS
//----------------------------------------------------------------------
void TrainSidebar::diskUpdate()
{
    int  secs;

    long torq = 0, altitude = 0;
    QTextStream recordFileStream(recordFile);

    if (calibrating) return;

    // convert from milliseconds to secondes
    total_msecs = session_elapsed_msec + session_time.elapsed();
    secs = total_msecs;
    secs /= 1000.0;

    // GoldenCheetah CVS Format "secs, cad, hr, km, kph, nm, watts, alt, lon, lat, headwind, slope, temp, interval, lrbalance, lte, rte, lps, rps, smo2, thb, o2hb, hhb\n";

    recordFileStream    << secs
                        << "," << displayCadence
                        << "," << displayHeartRate
                        << "," << displayDistance
                        << "," << displaySpeed
                        << "," << torq
                        << "," << displayPower
                        << "," << altitude
                        << "," // lon
                        << "," // lat
                        << "," // headwind
                        << "," // slope
                        << "," // temp
                        << "," << (displayLap + displayWorkoutLap)
                        << "," // lrbalance
                        << "," // lte
                        << "," // rte
                        << "," // lps
                        << "," // rps
                        << "," // smo2
                        << "," // thb
                        << "," // o2hb
                        << "," // hhb\n

                        << "," << "\n";
}

//----------------------------------------------------------------------
// WORKOUT MODE
//----------------------------------------------------------------------

void TrainSidebar::loadUpdate()
{
    int curLap;

    // we hold our horses whilst calibration is taking place...
    if (calibrating) return;

    // the period between loadUpdate calls is not constant, and not exactly LOADRATE,
    // therefore, use a QTime timer to measure the load period
    load_msecs += load_period.restart();

    if (status&RT_MODE_ERGO) {
        load = ergFile->wattsAt(load_msecs, curLap);

        if(displayWorkoutLap != curLap)
        {
            context->notifyNewLap();
        }
        displayWorkoutLap = curLap;

        // we got to the end!
        if (load == -100) {
            Stop(DEVICE_OK);
        } else {
            foreach(int dev, devices()) Devices[dev].controller->setLoad(load);
            context->notifySetNow(load_msecs);
        }
    } else {
        slope = ergFile->gradientAt(displayWorkoutDistance*1000, curLap);

        if(displayWorkoutLap != curLap)
        {
            context->notifyNewLap();
        }
        displayWorkoutLap = curLap;

        // we got to the end!
        if (slope == -100) {
            Stop(DEVICE_OK);
        } else {
            foreach(int dev, devices()) Devices[dev].controller->setGradient(slope);
            context->notifySetNow(displayWorkoutDistance * 1000);
        }
    }
}

void TrainSidebar::Calibrate()
{
    static QProgressDialog *bar=NULL;

    // toggle calibration
    if (calibrating) {
        bar->reset(); // will hide...

        // restart gui etc
        session_time.start();
        lap_time.start();
        load_period.restart();
        if (status & RT_WORKOUT) load_timer->start(LOADRATE);
        if (status & RT_RECORDING) disk_timer->start(SAMPLERATE);
        context->notifyUnPause(); // get video started again, amongst other things

        // back to ergo/slope mode and restore load/gradient
        if (status&RT_MODE_ERGO) {

            foreach(int dev, devices()) {
                Devices[dev].controller->setMode(RT_MODE_ERGO);
                Devices[dev].controller->setLoad(load);
            }
        } else {

            foreach(int dev, devices()) {
                Devices[dev].controller->setMode(RT_MODE_SPIN);
                Devices[dev].controller->setGradient(slope);
            }
        }

    } else {

        if (bar == NULL) {
            QString title = tr("Calibrating...\nPress F3 on Controller when done.");
            bar = new QProgressDialog(title, tr("Done"), 0, 0, this);
            bar->setWindowModality(Qt::WindowModal);
            bar->setMinimumDuration(0);
            bar->setAutoClose(true); // hide when reset
            connect(bar, SIGNAL(canceled()), this, SLOT(Calibrate()));
        }
        bar->show();

        // pause gui/load, streaming and recording
        // but keep the gui ticking so we get realtime telemetry to detect the F3 keypad =button press
        session_elapsed_msec += session_time.elapsed();
        lap_elapsed_msec += lap_time.elapsed();

        if (status & RT_RECORDING) disk_timer->stop();
        if (status & RT_WORKOUT) load_timer->stop();
        load_msecs += load_period.restart();

        context->notifyPause(); // get video started again, amongst other things

        // only do this for computrainers!
        foreach(int dev, devices())
            if (Devices[dev].type == DEV_CT)
                Devices[dev].controller->setMode(RT_MODE_CALIBRATE);
    }
    calibrating = !calibrating;
}

void TrainSidebar::FFwd()
{
    if ((status&RT_RUNNING) == 0) return;

    if (status&RT_MODE_ERGO) {
        load_msecs += 10000; // jump forward 10 seconds
        context->notifySeek(load_msecs);
    }
    else displayWorkoutDistance += 1; // jump forward a kilometer in the workout

}

void TrainSidebar::Rewind()
{
    if ((status&RT_RUNNING) == 0) return;

    if (status&RT_MODE_ERGO) {
        load_msecs -=10000; // jump back 10 seconds
        if (load_msecs < 0) load_msecs = 0;
        context->notifySeek(load_msecs);
    } else {
        displayWorkoutDistance -=1; // jump back a kilometer
        if (displayWorkoutDistance < 0) displayWorkoutDistance = 0;
    }
}


// jump to next Lap marker (if there is one?)
void TrainSidebar::FFwdLap()
{
    if ((status&RT_RUNNING) == 0) return;

    double lapmarker;

    if (status&RT_MODE_ERGO) {
        lapmarker = ergFile->nextLap(load_msecs);
        if (lapmarker != -1) load_msecs = lapmarker; // jump forward to lapmarker
        context->notifySeek(load_msecs);
    } else {
        lapmarker = ergFile->nextLap(displayWorkoutDistance*1000);
        if (lapmarker != -1) displayWorkoutDistance = lapmarker/1000; // jump forward to lapmarker
    }
}

// higher load/gradient
void TrainSidebar::Higher()
{
    if ((status&RT_RUNNING) == 0) return;

    if (context->currentErgFile()) {
        // adjust the workout IF
        intensitySlider->setValue(intensitySlider->value()+5);

    } else {
        if (status&RT_MODE_ERGO) load += 5;
        else slope += 0.1;

        if (load >1500) load = 1500;
        if (slope >15) slope = 15;

        if (status&RT_MODE_ERGO)
            foreach(int dev, devices()) Devices[dev].controller->setLoad(load);
        else
            foreach(int dev, devices()) Devices[dev].controller->setGradient(slope);
    }
}

// higher load/gradient
void TrainSidebar::Lower()
{
    if ((status&RT_RUNNING) == 0) return;

    if (context->currentErgFile()) {
        // adjust the workout IF
        intensitySlider->setValue(intensitySlider->value()-5);

    } else {

        if (status&RT_MODE_ERGO) load -= 5;
        else slope -= 0.1;

        if (load <0) load = 0;
        if (slope <-10) slope = -10;

        if (status&RT_MODE_ERGO)
            foreach(int dev, devices()) Devices[dev].controller->setLoad(load);
        else
            foreach(int dev, devices()) Devices[dev].controller->setGradient(slope);
    }
}

void TrainSidebar::setLabels()
{
    if (context->currentErgFile()) {

        //intensitySlider->show();//XXX!!! temporary

        if (context->currentErgFile()->format == CRS) {

            stress->setText(QString("Elevation %1").arg(context->currentErgFile()->ELE, 0, 'f', 0));
            intensity->setText(QString("Grade %1 %").arg(context->currentErgFile()->GRADE, 0, 'f', 1));

        } else {

            stress->setText(QString("TSS %1").arg(context->currentErgFile()->TSS, 0, 'f', 0));
            intensity->setText(QString("IF %1").arg(context->currentErgFile()->IF, 0, 'f', 3));
        }

    } else {

        intensitySlider->hide();
        stress->setText("");
        intensity->setText("");
    }
}

void TrainSidebar::adjustIntensity()
{
    if (!context->currentErgFile()) return; // no workout selected

    // block signals temporarily
    context->mainWindow->blockSignals(true);

    // work through the ergFile from NOW
    // adjusting back from last intensity setting
    // and increasing to new intensity setting

    double from = double(lastAppliedIntensity) / 100.00;
    double to = double(intensitySlider->value()) / 100.00;
    lastAppliedIntensity = intensitySlider->value();

    long starttime = context->getNow();

    bool insertedNow = context->getNow() ? false : true; // don't add if at start

    // what about gradient courses?
    ErgFilePoint last;
    for(int i = 0; i < context->currentErgFile()->Points.count(); i++) {

        if (context->currentErgFile()->Points.at(i).x >= starttime) {

            if (insertedNow == false) {

                if (i) {
                    // add a point to adjust from
                    ErgFilePoint add;
                    add.x = context->getNow();
                    add.val = last.val / from * to;

                    // recalibrate altitude if gradient changing
                    if (context->currentErgFile()->format == CRS) add.y = last.y + ((add.x-last.x) * (add.val/100));
                    else add.y = add.val;

                    context->currentErgFile()->Points.insert(i, add);

                    last = add;
                    i++; // move on to next point (i.e. where we were!)

                }
                insertedNow = true;
            }

            ErgFilePoint *p = &context->currentErgFile()->Points[i];

            // recalibrate altitude if in CRS mode
            p->val = p->val / from * to;
            if (context->currentErgFile()->format == CRS) {
                if (i) p->y = last.y + ((p->x-last.x) * (last.val/100));
            }
            else p->y = p->val;
        }

        // remember last
        last = context->currentErgFile()->Points.at(i);
    }

    // recalculate metrics
    context->currentErgFile()->calculateMetrics();
    setLabels();

    // unblock signals now we are done
    context->mainWindow->blockSignals(false);

    // force replot
    context->notifySetNow(context->getNow());
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

    cancelButton = new QPushButton("Cancel", this);
    buttons->addWidget(cancelButton);

    applyButton = new QPushButton("Apply", this);
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
    context->notifyConfigChanged();
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
