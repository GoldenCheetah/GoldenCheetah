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
#include "ANTplusController.h"
#include "ANTlocalController.h"
#include "NullController.h"

#ifdef GC_HAVE_VLC
// Media selection helper
#include "VideoWindow.h"
#endif

#ifdef Q_OS_MAC
#include "QtMacVideoWindow.h"
#endif

TrainTool::TrainTool(MainWindow *parent, const QDir &home) : GcWindow(parent), home(home), main(parent)
{
    setInstanceName("Train Controls");

    QWidget *c = new QWidget;
    //c->setContentsMargins(0,0,0,0); // bit of space is useful
    QVBoxLayout *cl = new QVBoxLayout(c);
    setControls(c);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    cl->setSpacing(0);
    cl->setContentsMargins(0,0,0,0);

    //setLineWidth(1);
    //setMidLineWidth(0);
    //setFrameStyle(QFrame::Plain | QFrame::Sunken);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(3,3,3,3);
    setContentsMargins(0,0,0,0);

#if 0 // not in this release .. or for a while TBH
    serverTree = new QTreeWidget;
    serverTree->setFrameStyle(QFrame::NoFrame);
    serverTree->setColumnCount(1);
    serverTree->setSelectionMode(QAbstractItemView::SingleSelection);
    serverTree->header()->hide();
    serverTree->setAlternatingRowColors (false);
    serverTree->setIndentation(5);
    allServers = new QTreeWidgetItem(serverTree, HEAD_TYPE);
    allServers->setText(0, tr("Race Servers"));
    serverTree->expandItem(allServers);
#endif

#if defined Q_OS_MAC || defined GC_HAVE_VLC
    mediaTree = new QTreeWidget;
    mediaTree->setFrameStyle(QFrame::NoFrame);
    mediaTree->setColumnCount(1);
    mediaTree->setSelectionMode(QAbstractItemView::SingleSelection);
    mediaTree->header()->hide();
    mediaTree->setAlternatingRowColors (false);
    mediaTree->setIndentation(5);
    allMedia = new QTreeWidgetItem(mediaTree, HEAD_TYPE);
    allMedia->setText(0, tr("Video / Media"));
    mediaTree->expandItem(allMedia);
#endif

    deviceTree = new QTreeWidget;
    deviceTree->setFrameStyle(QFrame::NoFrame);
    deviceTree->setSelectionMode(QAbstractItemView::MultiSelection);
    deviceTree->setColumnCount(1);
    deviceTree->header()->hide();
    deviceTree->setAlternatingRowColors (false);
    deviceTree->setIndentation(5);
    allDevices = new QTreeWidgetItem(deviceTree, HEAD_TYPE);
    allDevices->setText(0, tr("Devices"));
    deviceTree->expandItem(allDevices);

    workoutTree = new QTreeWidget;
    workoutTree->setFrameStyle(QFrame::NoFrame);
    workoutTree->setColumnCount(1);
    workoutTree->setSelectionMode(QAbstractItemView::SingleSelection);
    workoutTree->header()->hide();
    workoutTree->setAlternatingRowColors (false);
    workoutTree->setIndentation(5);

    allWorkouts = new QTreeWidgetItem(workoutTree, HEAD_TYPE);
    allWorkouts->setText(0, tr("Workout Library"));
    workoutTree->expandItem(allWorkouts);

    buttonPanel = new QFrame;
    buttonPanel->setLineWidth(1);
    buttonPanel->setFrameStyle(QFrame::NoFrame);
    buttonPanel->setContentsMargins(0,0,0,0);

    QVBoxLayout *panel = new QVBoxLayout;
    panel->setSpacing(0);
    panel->setContentsMargins(0,0,0,0);

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->setSpacing(0);
    buttons->setContentsMargins(0,0,0,0);

    startButton = new QPushButton(tr("Start"), this);
    startButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pauseButton = new QPushButton(tr("Pause"), this);
    pauseButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    stopButton = new QPushButton(tr("Stop"), this);
    stopButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    recordSelector = new QCheckBox(this);
    recordSelector->setText(tr("Save workout data"));
    recordSelector->setChecked(Qt::Checked);
    recordSelector->hide(); // we don't let users change this for now

    buttons->addWidget(startButton);
    buttons->addWidget(pauseButton);
    buttons->addWidget(stopButton);
    panel->addLayout(buttons);
    //panel->addWidget(recordSelector);
    buttonPanel->setLayout(panel);
    mainLayout->addWidget(buttonPanel);

    trainSplitter = new QSplitter;
    trainSplitter->setHandleWidth(1);
    trainSplitter->setFrameStyle(QFrame::NoFrame);
    trainSplitter->setOrientation(Qt::Vertical);
    trainSplitter->setContentsMargins(0,0,0,0);
    trainSplitter->setLineWidth(0);
    trainSplitter->setMidLineWidth(0);

    cl->addWidget(trainSplitter);
    trainSplitter->addWidget(deviceTree);
    //trainSplitter->addWidget(serverTree);
    trainSplitter->addWidget(workoutTree);
#if defined Q_OS_MAC || defined GC_HAVE_VLC
    trainSplitter->addWidget(mediaTree);
#endif

    // handle config changes
    //connect(serverTree,SIGNAL(itemSelectionChanged()), this, SLOT(serverTreeWidgetSelectionChanged()));
    connect(deviceTree,SIGNAL(itemSelectionChanged()), this, SLOT(deviceTreeWidgetSelectionChanged()));
    connect(workoutTree,SIGNAL(itemSelectionChanged()), this, SLOT(workoutTreeWidgetSelectionChanged()));
#if defined Q_OS_MAC || defined GC_HAVE_VLC
    connect(mediaTree,SIGNAL(itemSelectionChanged()), this, SLOT(mediaTreeWidgetSelectionChanged()));
#endif
    connect(main, SIGNAL(configChanged()), this, SLOT(configChanged()));

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

    // set home
    main = parent;
    deviceController = NULL;
    streamController = NULL;
    ergFile = NULL;

    // metric or imperial?
    QVariant unit = appsettings->value(this, GC_UNIT);
    useMetricUnits = (unit.toString() == "Metric");

    // now the GUI is setup lets sort our control variables
    gui_timer = new QTimer(this);
    disk_timer = new QTimer(this);
    stream_timer = new QTimer(this);
    load_timer = new QTimer(this);
    metrics_timer = new QTimer(this);

    session_time = QTime();
    session_elapsed_msec = 0;
    lap_time = QTime();
    lap_elapsed_msec = 0;

    recordFile = NULL;
    status = 0;
    status |= RT_MODE_ERGO;         // ergo mode by default
    displayWorkoutLap = displayLap = 0;
    pwrcount = 0;
    cadcount = 0;
    hrcount = 0;
    spdcount = 0;
    lodcount = 0;
    load_msecs = total_msecs = lap_msecs = 0;
    displayWorkoutDistance = displayDistance = displayPower = displayHeartRate =
    displaySpeed = displayCadence = displayGradient = displayLoad = 0;
    manualOverride = false;

    rideFile = boost::shared_ptr<RideFile>(new RideFile(QDateTime::currentDateTime(),1));

    connect(gui_timer, SIGNAL(timeout()), this, SLOT(guiUpdate()));
    connect(disk_timer, SIGNAL(timeout()), this, SLOT(diskUpdate()));
    connect(stream_timer, SIGNAL(timeout()), this, SLOT(streamUpdate()));
    connect(load_timer, SIGNAL(timeout()), this, SLOT(loadUpdate()));
    connect(metrics_timer, SIGNAL(timeout()), this, SLOT(metricsUpdate()));

    configChanged(); // will reset the workout tree

}

void
TrainTool::configChanged()
{
    setProperty("color", GColor(CRIDEPLOTBACKGROUND));

    // DEVICES
    // zap whats there
    //QList<QTreeWidgetItem *> servers = allServers->takeChildren();
    //for (int i=0; i<servers.count(); i++) delete servers.at(i);
    QList<QTreeWidgetItem *> devices = allDevices->takeChildren();
    for (int i=0; i<devices.count(); i++) delete devices.at(i);

    DeviceConfigurations all;
    Devices.clear();
    Devices = all.getList();
    for (int i=0; i<Devices.count(); i++) {
        if (Devices.at(i).type == DEV_GSERVER) {
            //QTreeWidgetItem *server = new QTreeWidgetItem(allServers, i);
            //server->setText(0, Devices.at(i).name);
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

#if defined Q_OS_MAC || defined GC_HAVE_VLC
    // MEDIA
    QList<QTreeWidgetItem *> media = allMedia->takeChildren();
    for (int i=0; i<media.count(); i++) delete media.at(i);

    MediaHelper mediaHelper;
    foreach(QString video, mediaHelper.listMedia(QDir(workoutDir.toString()))) {

        // add a media line for the video (it might be a song though...)
        QTreeWidgetItem *media = new QTreeWidgetItem(allMedia, WORKOUT_TYPE);
        media->setText(0, video);
    }
#endif

    // Athlete
    FTP=285; // default to 285 if zones are not set
    int range = main->zones()->whichRange(QDate::currentDate());
    if (range != -1) FTP = main->zones()->getCP(range);

    // metric or imperial changed?
    QVariant unit = appsettings->value(this, GC_UNIT);
    useMetricUnits = (unit.toString() == "Metric");
}

/*----------------------------------------------------------------------
 * Buttons!
 *----------------------------------------------------------------------*/

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

    int mode;

    // wip away the current selected workout
    if (ergFile) {
        delete ergFile;
        ergFile = NULL;
    }

    // which one is selected?
    if (currentWorkout() == NULL || currentWorkout()->type() != WORKOUT_TYPE) {
        main->notifyErgFileSelected(NULL);
        return;
    }

    // is it the auto mode?
    int index = workoutItems()->indexOfChild((QTreeWidgetItem *)currentWorkout());
    if (index == 0) {
        // ergo mode
        main->notifyErgFileSelected(NULL);
        mode = ERG;
        status &= ~RT_WORKOUT;
        //ergPlot->setVisible(false);
    } else if (index == 1) {
        // slope mode
        main->notifyErgFileSelected(NULL);
        mode = CRS;
        status &= ~RT_WORKOUT;
        //ergPlot->setVisible(false);
    } else {
        // workout mode
        QVariant workoutDir = appsettings->value(this, GC_WORKOUTDIR);
        QString fileName = workoutDir.toString() + "/" + currentWorkout()->text(0); // filename

        // Get users CP for relative watts calculations
        QDate today = QDate::currentDate();

        ergFile = new ErgFile(fileName, mode, FTP);
        if (ergFile->isValid()) {

            status |= RT_WORKOUT;

            // success! we have a load file
            // setup the course profile in the
            // display!
            main->notifyErgFileSelected(ergFile);
        }
    }

    // set the device to the right mode
    if (mode == ERG || mode == MRC) {
        status |= RT_MODE_ERGO;
        status &= ~RT_MODE_SPIN;
        if (deviceController != NULL) deviceController->setMode(RT_MODE_ERGO);
    } else { // SLOPE MODE
        status |= RT_MODE_SPIN;
        status &= ~RT_MODE_ERGO;
        if (deviceController != NULL) deviceController->setMode(RT_MODE_SPIN);
    }
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

void
TrainTool::mediaTreeWidgetSelectionChanged()
{
    assert(mediaTree->selectedItems().size() <= 1);
    if (mediaTree->selectedItems().isEmpty())
        media = NULL;
    else {
        QTreeWidgetItem *which = mediaTree->selectedItems().first();
        if (which->type() != WORKOUT_TYPE)
            media = NULL;
        else
            media = which;
    }

    // which one is selected?
    if (currentMedia() == NULL || currentMedia()->type() != WORKOUT_TYPE) {
        main->notifyMediaSelected("");
        return;
    }

    QVariant workoutDir = appsettings->value(this, GC_WORKOUTDIR);
    QString fileName = workoutDir.toString() + "/" + currentMedia()->text(0); // filename
    main->notifyMediaSelected(fileName);
}

/*--------------------------------------------------------------------------------
 * Was realtime window, now local and manages controller and chart updates etc
 *------------------------------------------------------------------------------*/

void TrainTool::setDeviceController()
{
    int deviceno = selectedDeviceNumber();

    if (deviceno == -1) // not selected, maybe they are spectating
        return;

    // zap the current one
    if (deviceController != NULL) {
        delete deviceController;
        deviceController = NULL;
    }

    if (Devices.count() > 0) {
        DeviceConfiguration temp = Devices.at(deviceno);
        if (Devices.at(deviceno).type == DEV_ANTPLUS) {
            deviceController = new ANTplusController(this, &temp);
        } else if (Devices.at(deviceno).type == DEV_CT) {
            deviceController = new ComputrainerController(this, &temp);
        } else if (Devices.at(deviceno).type == DEV_NULL) {
            deviceController = new NullController(this, &temp);
        } else if (Devices.at(deviceno).type == DEV_ANTLOCAL) {
            deviceController = new ANTlocalController(this, &temp);
        }
    }
}

// open a connection to the GoldenServer via a GoldenClient
void TrainTool::setStreamController()
{
    int deviceno = selectedServerNumber();

    if (deviceno == -1) return;

    // zap the current one
    if (streamController != NULL) {
        delete streamController;
        streamController = NULL;
    }

    if (Devices.count() > 0) {
        DeviceConfiguration config = Devices.at(deviceno);
        streamController = new GoldenClient;

        // connect
        QStringList speclist = config.portSpec.split(":", QString::SkipEmptyParts);
        bool rc = streamController->connect(speclist[0], // host
                                  speclist[1].toInt(),   // port
                                  "9cf638294030cea7b1590a4ca32e7f58", // raceid
                                  appsettings->cvalue(main->cyclist, GC_NICKNAME).toString(), // name
                                  FTP, // CP60
                                  appsettings->cvalue(main->cyclist, GC_WEIGHT).toDouble()); // weight

        // no connection
        if (rc == false) {
            streamController->closeAndExit();
            streamController = NULL;
            status &= ~RT_STREAMING;
            QMessageBox msgBox;
            msgBox.setText(QString(tr("Cannot Connect to Server %1 on port %2").arg(speclist[0]).arg(speclist[1])));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();
        }
    }
}

void TrainTool::Start()       // when start button is pressed
{
    if (status&RT_RUNNING) {
        newLap();

        // tell the world
        main->notifyNewLap();

    } else {

        // open the controller if it is selected
        setDeviceController();
        if (deviceController == NULL) return;
        else deviceController->start();          // start device

        // tell the world
        main->notifyStart();

        // we're away!
        status |=RT_RUNNING;

        // should we be streaming too?
        //setStreamController();
        if (streamController != NULL) status |= RT_STREAMING;

        setStartText(tr("Lap"));

        load_period.restart();
        session_time.start();
        session_elapsed_msec = 0;
        lap_time.start();
        lap_elapsed_msec = 0;

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

        // create a new rideFile
        rideFile = boost::shared_ptr<RideFile>(new RideFile(QDateTime::currentDateTime(),1));


        // stream
        if (status & RT_STREAMING) {
            stream_timer->start(STREAMRATE);
        }

        gui_timer->start(REFRESHRATE);      // start recording
        metrics_timer->start(METRICSRATE);

    }
}

void TrainTool::Pause()        // pause capture to recalibrate
{
    if (deviceController == NULL) return;

    // we're not running fool!
    if ((status&RT_RUNNING) == 0) return;

    if (status&RT_PAUSED) {

        session_time.start();
        lap_time.start();
        status &=~RT_PAUSED;
        deviceController->restart();
        setPauseText(tr("Pause"));
        gui_timer->start(REFRESHRATE);
        metrics_timer->start(METRICSRATE);
        if (status & RT_STREAMING) stream_timer->start(STREAMRATE);
        if (status & RT_RECORDING) disk_timer->start(SAMPLERATE);
        load_period.restart();
        if (status & RT_WORKOUT) load_timer->start(LOADRATE);

        // tell the world
        main->notifyUnPause();

    } else {

        session_elapsed_msec += session_time.elapsed();
        lap_elapsed_msec += lap_time.elapsed();
        deviceController->pause();
        setPauseText(tr("Un-Pause"));
        status |=RT_PAUSED;
        gui_timer->stop();
        metrics_timer->stop();
        if (status & RT_STREAMING) stream_timer->stop();
        if (status & RT_RECORDING) disk_timer->stop();
        if (status & RT_WORKOUT) load_timer->stop();
        load_msecs += load_period.restart();

        // tell the world
        main->notifyPause();
    }
}

void TrainTool::Stop(int deviceStatus)        // when stop button is pressed
{
    if (deviceController == NULL) return;

    if ((status&RT_RUNNING) == 0) return;

    status &= ~RT_RUNNING;
    setStartText(tr("Start"));

    // wipe connection
    deviceController->stop();
    delete deviceController;
    deviceController = NULL;

    gui_timer->stop();
    metrics_timer->stop();

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

    if (status & RT_STREAMING) {
        stream_timer->stop();
        streamController->closeAndExit();
        delete streamController;
        streamController = NULL;
    }

    if (status & RT_WORKOUT) {
        load_timer->stop();
        load_msecs = 0;
    }

    main->notifySetNow(load_msecs);


    // tell the world
    main->notifyStop();

    // Re-enable gui elements
    //recordSelector->setEnabled(true);

    // reset counters etc
    pwrcount = 0;
    cadcount = 0;
    hrcount = 0;
    spdcount = 0;
    lodcount = 0;
    manualOverride = false;
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
        displayLoad = rtData.getLoad();
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

    if (deviceController == NULL) return;

    // get latest telemetry from device (if it is a pull device e.g. Computrainer //
    if (status&RT_RUNNING && deviceController->doesPull() == true) {

        deviceController->getRealtimeData(rtData);

        // Distance assumes current speed for the last second. from km/h to km/sec
        displayDistance += displaySpeed / (5 * 3600); // XXX assumes 200ms refreshrate
        displayWorkoutDistance += displaySpeed / (5 * 3600); // XXX assumes 200ms refreshrate
        rtData.setDistance(displayDistance);

        // time
        total_msecs = session_elapsed_msec + session_time.elapsed();
        lap_msecs = lap_elapsed_msec + lap_time.elapsed();
        rtData.setMsecs(total_msecs);
        rtData.setLapMsecs(lap_msecs);

        // metrics
        rtData.setJoules(kjoules);
        rtData.setBikeScore(bikescore);
        rtData.setXPower(xpower);

        // local stuff ...
        displayPower = rtData.getWatts();
        displayCadence = rtData.getCadence();
        displayHeartRate = rtData.getHr();
        displaySpeed = rtData.getSpeed();
        displayLoad = rtData.getLoad();

        // go update the displays...
        main->notifyTelemetryUpdate(rtData); // signal everyone to update telemetry

        // set now to current time when not using a workout
        // but limit to almost every second (account for
        // slight timing errors of 100ms or so)
        if (!(status&RT_WORKOUT) && rtData.getMsecs()%1000 < 100) {
            main->notifySetNow(rtData.getMsecs());
        }
    }
}

// can be called from the controller - when user presses "Lap" button
void TrainTool::newLap()
{
    displayLap++;

    pwrcount  = 0;
    cadcount  = 0;
    hrcount   = 0;
    spdcount  = 0;

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
// STREAMING FUNCTION
//----------------------------------------------------------------------
#if 0
TrainTool::SelectStream(int index)
{

    if (index > 0) {
        status |= RT_STREAMING;
        setStreamController();
    } else {
        status &= ~RT_STREAMING;
    }
}
#endif

void
TrainTool::streamUpdate()
{
    // send over the wire...
    if (streamController) {

        // send my data
        streamController->sendTelemetry(displayPower,
                                        displayCadence,
                                        displayDistance,
                                        displayHeartRate,
                                        displaySpeed);

        // get standings for everyone else
        RaceStatus current = streamController->getStandings();

        // send out to all the widgets...
        notifyRaceStandings(current);

        // has the race finished?
        if (current.race_finished == true) {
            Stop(0); // all over dude
            QMessageBox msgBox;
            msgBox.setText(tr("Race Over!"));
            msgBox.setIcon(QMessageBox::Information);
            msgBox.exec();
        }
    }
}

//----------------------------------------------------------------------
// DISK UPDATE FUNCTIONS
//----------------------------------------------------------------------
void TrainTool::diskUpdate()
{
    double  Minutes;

    long Torq = 0, Altitude = 0;
    QTextStream recordFileStream(recordFile);

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

    rideFile->appendPoint(total_msecs/1000,displayCadence,displayHeartRate,displayDistance,displaySpeed,0,
                          displayPower,Altitude,0,0,0,displayLap + displayWorkoutLap);
}

void TrainTool::metricsUpdate()
{
    // calculate bike score, xpower
    const RideMetricFactory &factory = RideMetricFactory::instance();
    const RideMetric *rm = factory.rideMetric("skiba_xpower");

    QStringList metrics;
    metrics.append("skiba_bike_score");
    metrics.append("skiba_xpower");
    QHash<QString,RideMetricPtr> results = rm->computeMetrics(
            this->main,&*rideFile,this->main->zones(),this->main->hrZones(),metrics);
    bikescore = results["skiba_bike_score"]->value(true);
    xpower = results["skiba_xpower"]->value(true);
}

//----------------------------------------------------------------------
// WORKOUT MODE
//----------------------------------------------------------------------

void TrainTool::loadUpdate()
{
    int curLap;
    long load;
    double gradient;
    // the period between loadUpdate calls is not constant, and not exactly LOADRATE,
    // therefore, use a QTime timer to measure the load period
    load_msecs += load_period.restart();

    if (deviceController == NULL) return;

    if (status&RT_MODE_ERGO) {
        load = ergFile->wattsAt(load_msecs, curLap);

        if(displayWorkoutLap != curLap)
        {
            // we are onto a new lap/interval, reset the override
            manualOverride = false;
        }
        if(manualOverride == false)
        {
            displayLoad = load;
        }
        displayWorkoutLap = curLap;

        // we got to the end!
        if (load == -100) {
            Stop(DEVICE_OK);
        } else {
            displayLoad = load;
            deviceController->setLoad(displayLoad);
            main->notifySetNow(load_msecs);
        }
    } else {
        gradient = ergFile->gradientAt(displayWorkoutDistance*1000, curLap);

        if(displayWorkoutLap != curLap)
        {
            // we are onto a new lap/interval, reset the override
            manualOverride = false;
        }
        if(manualOverride == false)
        {
            displayGradient = gradient;
        }
        displayWorkoutLap = curLap;

        // we got to the end!
        if (gradient == -100) {
            Stop(DEVICE_OK);
        } else {
            displayGradient = gradient;
            deviceController->setGradient(displayGradient);
            main->notifySetNow(displayWorkoutDistance * 1000);
        }
    }
}

void TrainTool::FFwd()
{
    if (status&RT_MODE_ERGO) load_msecs += 10000; // jump forward 10 seconds
    else displayWorkoutDistance += 1; // jump forward a kilometer in the workout
}

void TrainTool::Rewind()
{
    if (status&RT_MODE_ERGO) {
        load_msecs -=10000; // jump back 10 seconds
        if (load_msecs < 0) load_msecs = 0;
    } else {
        displayWorkoutDistance -=1; // jump back a kilometer
        if (displayWorkoutDistance < 0) displayWorkoutDistance = 0;
    }
}


// jump to next Lap marker (if there is one?)
void TrainTool::FFwdLap()
{
    double lapmarker;

    if (status&RT_MODE_ERGO) {
        lapmarker = ergFile->nextLap(load_msecs);
        if (lapmarker != -1) load_msecs = lapmarker; // jump forward to lapmarker
    } else {
        lapmarker = ergFile->nextLap(displayWorkoutDistance*1000);
        if (lapmarker != -1) displayWorkoutDistance = lapmarker/1000; // jump forward to lapmarker
    }
}

// higher load/gradient
void TrainTool::Higher()
{
    if (deviceController == NULL) return;

    manualOverride = true;

    if (status&RT_MODE_ERGO) displayLoad += 5;
    else displayGradient += 0.1;

    if (displayLoad >1500) displayLoad = 1500;
    if (displayGradient >15) displayGradient = 15;

    if (status&RT_MODE_ERGO) deviceController->setLoad(displayLoad);
    else deviceController->setGradient(displayGradient);
}

// higher load/gradient
void TrainTool::Lower()
{
    if (deviceController == NULL) return;

    manualOverride = true;

    if (status&RT_MODE_ERGO) displayLoad -= 5;
    else displayGradient -= 0.1;

    if (displayLoad <0) displayLoad = 0;
    if (displayGradient <-10) displayGradient = -10;

    if (status&RT_MODE_ERGO) deviceController->setLoad(displayLoad);
    else deviceController->setGradient(displayGradient);
}
