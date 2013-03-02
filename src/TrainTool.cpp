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
#include "Settings.h"
#include "Colors.h"
#include "Units.h"
#include "DeviceTypes.h"
#include "DeviceConfiguration.h"
#include <assert.h>
#include <QApplication>
#include <QtGui>
#include <QRegExp>

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

#ifdef GC_HAVE_VLC
// Media selection helper
#include "VideoWindow.h"
#endif

#ifdef Q_OS_MAC
#include "QtMacVideoWindow.h"
#include <CoreServices/CoreServices.h>
#endif

#include <math.h> // isnan and isinf
#include "TrainDB.h"

TrainTool::TrainTool(MainWindow *parent, const QDir &home) : GcWindow(parent), home(home), main(parent)
{
    setInstanceName("Train Controls");

    QWidget *c = new QWidget;
    //c->setContentsMargins(0,0,0,0); // bit of space is useful
    QVBoxLayout *cl = new QVBoxLayout(c);
    setControls(c);

    cl->setSpacing(0);
    cl->setContentsMargins(0,0,0,0);

    // don't set the source for telemetry
    bpmTelemetry = wattsTelemetry = kphTelemetry = rpmTelemetry = -1;

#if defined Q_OS_MAC || defined GC_HAVE_VLC
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
    rewind->setIconSize(QSize(20,20));
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
    stop->setIconSize(QSize(20,20));
    stop->setAutoFillBackground(false);
    stop->setAutoDefault(false);
    stop->setFlat(true);
    stop->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    toolbuttons->addWidget(stop);

    QIcon playIcon(":images/oxygen/play.png");
    play = new QPushButton(playIcon, "", this);
    play->setFocusPolicy(Qt::NoFocus);
    play->setIconSize(QSize(20,20));
    play->setAutoFillBackground(false);
    play->setAutoDefault(false);
    play->setFlat(true);
    play->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    toolbuttons->addWidget(play);

    QIcon fwdIcon(":images/oxygen/ffwd.png");
    QPushButton *forward = new QPushButton(fwdIcon, "", this);
    forward->setFocusPolicy(Qt::NoFocus);
    forward->setIconSize(QSize(20,20));
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
    lap->setIconSize(QSize(20,20));
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

#ifdef Q_OS_MAC
    QWindowsStyle *macstyler = new QWindowsStyle();
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

    intensity = new QLabel(this);
    intensity->setAutoFillBackground(false);
    intensity->setFixedWidth(100);
    intensity->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    pal.setColor(intensity->foregroundRole(), Qt::white);
    intensity->setPalette(pal);

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
    connect(main, SIGNAL(newLap()), this, SLOT(resetLapTimer()));
    connect(intensitySlider, SIGNAL(valueChanged(int)), this, SLOT(adjustIntensity()));

    // not used but kept in case re-instated in the future
    recordSelector = new QCheckBox(this);
    recordSelector->setText(tr("Save workout data"));
    recordSelector->setChecked(Qt::Checked);
    recordSelector->hide(); // we don't let users change this for now

    trainSplitter = new GcSplitter(Qt::Vertical);
    trainSplitter->setContentsMargins(0,0,0,0);
    deviceItem = new GcSplitterItem(tr("Devices"), iconFromPNG(":images/sidebar/power.png"), this);
    workoutItem = new GcSplitterItem(tr("Workouts"), iconFromPNG(":images/sidebar/folder.png"), this);
    deviceItem->addWidget(deviceTree);
    trainSplitter->addWidget(deviceItem);
    workoutItem->addWidget(workoutTree);
    trainSplitter->addWidget(workoutItem);
    cl->addWidget(trainSplitter);

#if defined Q_OS_MAC || defined GC_HAVE_VLC
    mediaItem = new GcSplitterItem(tr("Media"), iconFromPNG(":images/sidebar/movie.png"), this);
    mediaItem->addWidget(mediaTree);
    trainSplitter->addWidget(mediaItem);
#endif

#ifdef Q_OS_MAC
    // get rid of annoying focus rectangle for sidebar components
    mediaTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
    workoutTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
    deviceTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

    // handle config changes
    connect(deviceTree,SIGNAL(itemSelectionChanged()), this, SLOT(deviceTreeWidgetSelectionChanged()));
    connect(deviceTree,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(deviceTreeMenuPopup(const QPoint &)));

#if defined Q_OS_MAC || defined GC_HAVE_VLC
    connect(mediaTree->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
                            this, SLOT(mediaTreeWidgetSelectionChanged()));
    connect(main, SIGNAL(selectMedia(QString)), this, SLOT(selectVideo(QString)));
#endif
    connect(main, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(main, SIGNAL(selectWorkout(QString)), this, SLOT(selectWorkout(QString)));
    connect(trainDB, SIGNAL(dataChanged()), this, SLOT(refresh()));

    connect(workoutTree->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(workoutTreeWidgetSelectionChanged()));
    // add a watch on all directories
    QVariant workoutDir = appsettings->value(NULL, GC_WORKOUTDIR);

    // set home
    main = parent;
    ergFile = NULL;
    calibrating = false;

    // metric or imperial?
    useMetricUnits = main->useMetricUnits;

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
TrainTool::refresh()
{
    int row;

#if defined Q_OS_MAC || defined GC_HAVE_VLC
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
TrainTool::configChanged()
{
    setProperty("color", GColor(CRIDEPLOTBACKGROUND));

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
    int range = main->zones()->whichRange(QDate::currentDate());
    if (range != -1) FTP = main->zones()->getCP(range);

    // metric or imperial changed?
    useMetricUnits = main->useMetricUnits;
}

/*----------------------------------------------------------------------
 * Device Selected
 *--------------------------------------------------------------------*/
void
TrainTool::deviceTreeWidgetSelectionChanged()
{
    bpmTelemetry = wattsTelemetry = kphTelemetry = rpmTelemetry = -1;
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

QList<int>
TrainTool::devices()
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
TrainTool::workoutTreeWidgetSelectionChanged()
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
        main->notifyErgFileSelected(NULL);
        return;
    }

    // is it the auto mode?
    int index = target.row();
    if (index == 0) {
        // ergo mode
        main->notifyErgFileSelected(NULL);
        mode = ERG;
        setLabels();
        status &= ~RT_WORKOUT;
        //ergPlot->setVisible(false);
    } else if (index == 1) {
        // slope mode
        main->notifyErgFileSelected(NULL);
        mode = CRS;
        setLabels();
        status &= ~RT_WORKOUT;
        //ergPlot->setVisible(false);
    } else {
        // workout mode
        ergFile = new ErgFile(filename, mode, main);
        if (ergFile->isValid()) {

            status |= RT_WORKOUT;

            // success! we have a load file
            // setup the course profile in the
            // display!
            main->notifyErgFileSelected(ergFile);
            intensitySlider->setValue(100);
            lastAppliedIntensity = 100;
            setLabels();
        } else {

            // couldn't parse fall back to ERG mode
            delete ergFile;
            ergFile = NULL;
            main->notifyErgFileSelected(NULL);
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
TrainTool::listWorkoutFiles(const QDir &dir) const
{
    QStringList filters;
    filters << "*.erg";
    filters << "*.mrc";
    filters << "*.crs";
    filters << "*.pgmf";

    return dir.entryList(filters, QDir::Files, QDir::Name);
}

void
TrainTool::mediaTreeWidgetSelectionChanged()
{

    QModelIndex current = mediaTree->currentIndex();
    QModelIndex target = vsortModel->mapToSource(current);
    QString filename = videoModel->data(videoModel->index(target.row(), 0), Qt::DisplayRole).toString();
    main->notifyMediaSelected(filename);
}

/*--------------------------------------------------------------------------------
 * Was realtime window, now local and manages controller and chart updates etc
 *------------------------------------------------------------------------------*/

void TrainTool::Start()       // when start button is pressed
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

#if defined Q_OS_MAC || defined GC_HAVE_VLC
        mediaTree->setEnabled(false);
#endif

        // tell the world
        main->notifyUnPause();

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

#if defined Q_OS_MAC || defined GC_HAVE_VLC
        // enable media tree so we can change movie - mid workout
        mediaTree->setEnabled(true);
#endif

        // tell the world
        main->notifyPause();

    } else {

        // Stop users from selecting different devices
        // media or workouts whilst a workout is in progress

        // if we have selected multiple devices lets
        // configure the series we collect from each one
        if (deviceTree->selectedItems().count() > 1) {
            MultiDeviceDialog *multisetup = new MultiDeviceDialog(main, this);
            if (multisetup->exec() == false) {
                return;
            }
        } else if (deviceTree->selectedItems().count() == 1) {
            bpmTelemetry = wattsTelemetry = kphTelemetry = rpmTelemetry =
            deviceTree->selectedItems().first()->type();
        } else {
            return;
        }

#if defined Q_OS_MAC || defined GC_HAVE_VLC
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
        main->notifyStart();

        // we're away!
        status |=RT_RUNNING;

        load_period.restart();
        session_time.start();
        session_elapsed_msec = 0;
        lap_time.start();
        lap_elapsed_msec = 0;
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

            QString fulltarget = home.absolutePath() + "/" + filename;
            if (recordFile) delete recordFile;
            recordFile = new QFile(fulltarget);
            if (!recordFile->open(QFile::WriteOnly | QFile::Truncate)) {
                status &= ~RT_RECORDING;
            } else {

                // CSV File header

                QTextStream recordFileStream(recordFile);
                recordFileStream << "Minutes,Torq (N-m),Km/h,Watts,Km,Cadence,Hrate,ID,Altitude (m)\n";
                disk_timer->start(SAMPLERATE);  // start screen
            }
        }
        gui_timer->start(REFRESHRATE);      // start recording

    }
}

void TrainTool::Pause()        // pause capture to recalibrate
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

#if defined Q_OS_MAC || defined GC_HAVE_VLC
        mediaTree->setEnabled(false);
#endif

        // tell the world
        main->notifyUnPause();

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
#if defined Q_OS_MAC || defined GC_HAVE_VLC
        mediaTree->setEnabled(true);
#endif

        // tell the world
        main->notifyPause();
    }
}

void TrainTool::Stop(int deviceStatus)        // when stop button is pressed
{

    if ((status&RT_RUNNING) == 0) return;

    status &= ~(RT_RUNNING|RT_PAUSED);

    // Stop users from selecting different devices
    // media or workouts whilst a workout is in progress
#if defined Q_OS_MAC || defined GC_HAVE_VLC
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
            main->addRide(QFileInfo(name).fileName(), true);
        }
    }

    if (status & RT_WORKOUT) {
        load_timer->stop();
        load_msecs = 0;
    }

    // get back to normal after it may have been adusted by the user
    lastAppliedIntensity=100;
    intensitySlider->setValue(100);
    if (main->currentErgFile()) main->currentErgFile()->reload();
    main->notifySetNow(load_msecs);

    // reset the play button
    QIcon playIcon(":images/oxygen/play.png");
    play->setIcon(playIcon);

    // tell the world
    main->notifyStop();

    // Re-enable gui elements
    // reset counters etc
    pwrcount = 0;
    cadcount = 0;
    hrcount = 0;
    spdcount = 0;
    lodcount = 0;
    displayWorkoutLap = displayLap =0;
    session_elapsed_msec = 0;
    session_time.restart();
    lap_elapsed_msec = 0;
    lap_time.restart();
    displayWorkoutDistance = displayDistance = 0;
    guiUpdate();

    return;
}


// Called by push devices (e.g. ANT+)
void TrainTool::updateData(RealtimeData &rtData)
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

void TrainTool::guiUpdate()           // refreshes the telemetry
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
            double weight = appsettings->cvalue(main->cyclist, GC_WEIGHT, 0.0).toDouble();
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


            // go update the displays...
            main->notifyTelemetryUpdate(rtData); // signal everyone to update telemetry

            // set now to current time when not using a workout
            // but limit to almost every second (account for
            // slight timing errors of 100ms or so)

            // Do some rounding to the hundreds because as time goes by, rtData.getMsecs() drifts just below and then it does not pass the mod 1000 < 100 test
            // For example:  msecs = 42199.  Mod 1000 = 199 versus msecs = 42000.  Mod 1000 = 0
            // With this, it will now call tick just about every second
            long rmsecs = round((rtData.getMsecs() + 99) / 100) * 100;
            // Test for <= 100ms
            if (!(status&RT_WORKOUT) && ((rmsecs % 1000) <= 100)) {
                main->notifySetNow(rtData.getMsecs());
            }
        }
    }
}

// can be called from the controller - when user presses "Lap" button
void TrainTool::newLap()
{
    if ((status&RT_RUNNING) == RT_RUNNING) {
        displayLap++;

        pwrcount  = 0;
        cadcount  = 0;
        hrcount   = 0;
        spdcount  = 0;

        main->notifyNewLap();
    }
}

void TrainTool::resetLapTimer()
{
    lap_time.restart();
    lap_elapsed_msec = 0;
}

// can be called from the controller
void TrainTool::nextDisplayMode()
{
}

void TrainTool::warnnoConfig()
{
    QMessageBox::warning(this, tr("No Devices Configured"), "Please configure a device in Preferences.");
}

//----------------------------------------------------------------------
// DISK UPDATE FUNCTIONS
//----------------------------------------------------------------------
void TrainTool::diskUpdate()
{
    double  Minutes;

    long Torq = 0, Altitude = 0;
    QTextStream recordFileStream(recordFile);

    if (calibrating) return;

    // convert from milliseconds to minutes
    total_msecs = session_elapsed_msec + session_time.elapsed();
    Minutes = total_msecs;
    Minutes /= 1000.00;
    Minutes *= (1.0/60);

    // PowerAgent Format "Minutes,Torq (N-m),Km/h,Watts,Km,Cadence,Hrate,ID,Altitude (m)"
    recordFileStream    << Minutes
                        << "," << Torq
                        << "," << displaySpeed
                        << "," << displayPower
                        << "," << displayDistance
                        << "," << displayCadence
                        << "," << displayHeartRate
                        << "," << (displayLap + displayWorkoutLap)
                        << "," << Altitude
                        << "," << "\n";
}

//----------------------------------------------------------------------
// WORKOUT MODE
//----------------------------------------------------------------------

void TrainTool::loadUpdate()
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
            main->notifyNewLap();
        }
        displayWorkoutLap = curLap;

        // we got to the end!
        if (load == -100) {
            Stop(DEVICE_OK);
        } else {
            foreach(int dev, devices()) Devices[dev].controller->setLoad(load);
            main->notifySetNow(load_msecs);
        }
    } else {
        slope = ergFile->gradientAt(displayWorkoutDistance*1000, curLap);

        if(displayWorkoutLap != curLap)
        {
            main->notifyNewLap();
        }
        displayWorkoutLap = curLap;

        // we got to the end!
        if (slope == -100) {
            Stop(DEVICE_OK);
        } else {
            foreach(int dev, devices()) Devices[dev].controller->setGradient(slope);
            main->notifySetNow(displayWorkoutDistance * 1000);
        }
    }
}

void TrainTool::Calibrate()
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
        main->notifyUnPause(); // get video started again, amongst other things

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

        main->notifyPause(); // get video started again, amongst other things

        // only do this for computrainers!
        foreach(int dev, devices())
            if (Devices[dev].type == DEV_CT)
                Devices[dev].controller->setMode(RT_MODE_CALIBRATE);
    }
    calibrating = !calibrating;
}

void TrainTool::FFwd()
{
    if ((status&RT_RUNNING) == 0) return;

    if (status&RT_MODE_ERGO) {
        load_msecs += 10000; // jump forward 10 seconds
        main->notifySeek(load_msecs);
    }
    else displayWorkoutDistance += 1; // jump forward a kilometer in the workout

}

void TrainTool::Rewind()
{
    if ((status&RT_RUNNING) == 0) return;

    if (status&RT_MODE_ERGO) {
        load_msecs -=10000; // jump back 10 seconds
        if (load_msecs < 0) load_msecs = 0;
        main->notifySeek(load_msecs);
    } else {
        displayWorkoutDistance -=1; // jump back a kilometer
        if (displayWorkoutDistance < 0) displayWorkoutDistance = 0;
    }
}


// jump to next Lap marker (if there is one?)
void TrainTool::FFwdLap()
{
    if ((status&RT_RUNNING) == 0) return;

    double lapmarker;

    if (status&RT_MODE_ERGO) {
        lapmarker = ergFile->nextLap(load_msecs);
        if (lapmarker != -1) load_msecs = lapmarker; // jump forward to lapmarker
        main->notifySeek(load_msecs);
    } else {
        lapmarker = ergFile->nextLap(displayWorkoutDistance*1000);
        if (lapmarker != -1) displayWorkoutDistance = lapmarker/1000; // jump forward to lapmarker
    }
}

// higher load/gradient
void TrainTool::Higher()
{
    if ((status&RT_RUNNING) == 0) return;

    if (main->currentErgFile()) {
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
void TrainTool::Lower()
{
    if ((status&RT_RUNNING) == 0) return;

    if (main->currentErgFile()) {
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

void TrainTool::setLabels()
{
    if (main->currentErgFile()) {

        intensitySlider->show();

        if (main->currentErgFile()->format == CRS) {

            stress->setText(QString("Elevation %1").arg(main->currentErgFile()->ELE, 0, 'f', 0));
            intensity->setText(QString("Grade %1 %").arg(main->currentErgFile()->GRADE, 0, 'f', 1));

        } else {

            stress->setText(QString("TSS %1").arg(main->currentErgFile()->TSS, 0, 'f', 0));
            intensity->setText(QString("IF %1").arg(main->currentErgFile()->IF, 0, 'f', 3));
        }

    } else {

        intensitySlider->hide();
        stress->setText("");
        intensity->setText("");
    }
}

void TrainTool::adjustIntensity()
{
    if (!main->currentErgFile()) return; // no workout selected

    // block signals temporarily
    main->blockSignals(true);

    // work through the ergFile from NOW
    // adjusting back from last intensity setting
    // and increasing to new intensity setting

    double from = double(lastAppliedIntensity) / 100.00;
    double to = double(intensitySlider->value()) / 100.00;
    lastAppliedIntensity = intensitySlider->value();

    long starttime = main->getNow();

    bool insertedNow = main->getNow() ? false : true; // don't add if at start

    // what about gradient courses?
    ErgFilePoint last;
    for(int i = 0; i < main->currentErgFile()->Points.count(); i++) {

        if (main->currentErgFile()->Points.at(i).x >= starttime) {

            if (insertedNow == false) {

                if (i) {
                    // add a point to adjust from
                    ErgFilePoint add;
                    add.x = main->getNow();
                    add.val = last.val / from * to;

                    // recalibrate altitude if gradient changing
                    if (main->currentErgFile()->format == CRS) add.y = last.y + ((add.x-last.x) * (add.val/100));
                    else add.y = add.val;

                    main->currentErgFile()->Points.insert(i, add);

                    last = add;
                    i++; // move on to next point (i.e. where we were!)

                }
                insertedNow = true;
            }

            ErgFilePoint *p = &main->currentErgFile()->Points[i];

            // recalibrate altitude if in CRS mode
            p->val = p->val / from * to;
            if (main->currentErgFile()->format == CRS) {
                if (i) p->y = last.y + ((p->x-last.x) * (last.val/100));
            }
            else p->y = p->val;
        }

        // remember last
        last = main->currentErgFile()->Points.at(i);
    }

    // recalculate metrics
    main->currentErgFile()->calculateMetrics();
    setLabels();

    // unblock signals now we are done
    main->blockSignals(false);

    // force replot
    main->notifySetNow(main->getNow());
}

MultiDeviceDialog::MultiDeviceDialog(MainWindow *, TrainTool *traintool) : traintool(traintool)
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
TrainTool::deviceTreeMenuPopup(const QPoint &pos)
{
    QMenu menu(deviceTree);
    QAction *addDevice = new QAction(tr("Add Device"), deviceTree);
    connect(addDevice, SIGNAL(triggered(void)), main, SLOT(addDevice()));
    menu.addAction(addDevice);

    if (deviceTree->selectedItems().size() == 1) {
        QAction *delDevice = new QAction(tr("Delete Device"), deviceTree);
        connect(delDevice, SIGNAL(triggered(void)), this, SLOT(deleteDevice()));
        menu.addAction(delDevice);
    }

    menu.exec(deviceTree->mapToGlobal(pos));
}

void
TrainTool::deleteDevice()
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
    main->notifyConfigChanged();
}

// we have been told to select this video (usually because
// the user just dragndropped it in)
void
TrainTool::selectVideo(QString fullpath)
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
TrainTool::selectWorkout(QString fullpath)
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
