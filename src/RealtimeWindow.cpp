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

#include "MainWindow.h"
#include "RealtimeWindow.h"
#include "RealtimeData.h"
#include "RealtimePlot.h"
#include "math.h" // for round()
#include "Units.h" // for MILES_PER_KM
#include <QDebug>

// Three current realtime device types supported are:
#include "RealtimeController.h"
#include "ComputrainerController.h"
#include "ANTplusController.h"
#include "SimpleNetworkController.h"
#include "ErgFile.h"

#include "TrainTool.h"

void
RealtimeWindow::configUpdate()
{

    // config has been updated! so re-read
    // wipe out all the current entries
    deviceSelector->clear();
    streamSelector->clear();

    // metric or imperial changed?
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    QVariant unit = settings->value(GC_UNIT);
    useMetricUnits = (unit.toString() == "Metric");

    // set labels accordingly
    distanceLabel->setText(useMetricUnits ? tr("Distance (KM)") : tr("Distance (Miles)"));
    speedLabel->setText(useMetricUnits ? tr("km/h") : tr("MPH"));
    avgspeedLabel->setText(useMetricUnits ? tr("Avg km/h") : tr("Avg MPH"));

    // get configured devices
    DeviceConfigurations all;

    Devices.clear();
    Devices = all.getList();

    streamSelector->addItem("No streaming", -1);
    for (int i=0; i<Devices.count(); i++) {
        // add streamers
        if (Devices.at(i).type == DEV_GSERVER)
            streamSelector->addItem(Devices.at(i).name, i);

        // add data sources
        deviceSelector->addItem(Devices.at(i).name, i);
    }

    deviceSelector->setCurrentIndex(0);
    streamSelector->setCurrentIndex(0);

    // reconnect - otherwise after no config they still get an error
    if (Devices.count() > 0) {
        disconnect(startButton, SIGNAL(clicked()), this, SLOT(warnnoConfig()));
        disconnect(pauseButton, SIGNAL(clicked()), this, SLOT(warnnoConfig()));
        disconnect(stopButton, SIGNAL(clicked()), this, SLOT(warnnoConfig()));
        connect(startButton, SIGNAL(clicked()), this, SLOT(Start()));
        connect(pauseButton, SIGNAL(clicked()), this, SLOT(Pause()));
        connect(stopButton, SIGNAL(clicked()), this, SLOT(Stop()));
    }
}

RealtimeWindow::RealtimeWindow(MainWindow *parent, TrainTool *trainTool, const QDir &home)  : QWidget(parent)
{

    // set home
    this->home = home;
    this->trainTool = trainTool;
    main = parent;
    deviceController = NULL;
    streamController = NULL;
    ergFile = NULL;

    // metric or imperial?
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    QVariant unit = settings->value(GC_UNIT);
    useMetricUnits = (unit.toString() == "Metric");

    // main layout for the window
    main_layout = new QVBoxLayout(this);
    timer_layout = new QGridLayout();

    // BUTTONS AND LCDS
    button_layout = new QHBoxLayout();
    option_layout = new QHBoxLayout();
    controls_layout = new QVBoxLayout();

    deviceSelector = new QComboBox(this);
    streamSelector = new QComboBox(this);

    // get configured devices
    DeviceConfigurations all;
    Devices = all.getList();

    streamSelector->addItem("No streaming", -1);
    for (int i=0; i<Devices.count(); i++) {

        // add streamers
        if ( Devices.at(i).type == DEV_GSERVER ||
             Devices.at(i).type == DEV_GCLIENT )
            streamSelector->addItem(Devices.at(i).name, i);

        // add data sources
        deviceSelector->addItem(Devices.at(i).name, i);
    }

    deviceSelector->setCurrentIndex(0);
    streamSelector->setCurrentIndex(0);

    recordSelector = new QCheckBox(this);
    recordSelector->setText(tr("Save"));
    recordSelector->setChecked(Qt::Checked);

    startButton = new QPushButton(tr("Start"), this);
    startButton->setMaximumHeight(100);
    pauseButton = new QPushButton(tr("Pause"), this);
    pauseButton->setMaximumHeight(100);
    stopButton = new QPushButton(tr("Stop"), this);
    stopButton->setMaximumHeight(100);

    button_layout->addWidget(startButton);
    button_layout->addWidget(pauseButton);
    button_layout->addWidget(stopButton);
    option_layout->addWidget(deviceSelector);
    option_layout->addSpacing(10);
    option_layout->addWidget(recordSelector);
    option_layout->addSpacing(10);
    option_layout->addWidget(streamSelector);

    // XXX NETWORK STREAMING DISABLED IN THIS RELEASE SO HIDE THE COMBO
    streamSelector->hide();

    option_layout->addSpacing(10);
    controls_layout->addItem(option_layout);
    controls_layout->addItem(button_layout);

    // handle config changes
    connect(main, SIGNAL(configChanged()), this, SLOT(configUpdate()));

    connect(deviceSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(SelectDevice(int)));
    connect(recordSelector, SIGNAL(clicked()), this, SLOT(SelectRecord()));
    connect(streamSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(SelectStream(int)));
    connect(trainTool, SIGNAL(workoutSelected()), this, SLOT(SelectWorkout()));

    if (Devices.count() > 0) {
        connect(startButton, SIGNAL(clicked()), this, SLOT(Start()));
        connect(pauseButton, SIGNAL(clicked()), this, SLOT(Pause()));
        connect(stopButton, SIGNAL(clicked()), this, SLOT(Stop()));
    } else {
        connect(startButton, SIGNAL(clicked()), this, SLOT(warnnoConfig()));
        connect(pauseButton, SIGNAL(clicked()), this, SLOT(warnnoConfig()));
        connect(stopButton, SIGNAL(clicked()), this, SLOT(warnnoConfig()));
    }
    powerLabel = new QLabel(tr("WATTS"), this);
    powerLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    heartrateLabel = new QLabel(tr("BPM"), this);
    heartrateLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    speedLabel = new QLabel(useMetricUnits ? tr("km/h") : tr("MPH"), this);
    speedLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    cadenceLabel = new QLabel(tr("RPM"), this);
    cadenceLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    lapLabel = new QLabel(tr("Lap/Interval"), this);
    lapLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    loadLabel = new QLabel(tr("Load WATTS"), this);
    loadLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    distanceLabel = new QLabel(useMetricUnits ? tr("Distance (KM)") : tr("Distance (Miles)"), this);
    distanceLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    avgpowerLabel = new QLabel(tr("Avg WATTS"), this);
    avgpowerLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    avgheartrateLabel = new QLabel(tr("Avg BPM"), this);
    avgheartrateLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    avgspeedLabel = new QLabel(useMetricUnits ? tr("Avg km/h") : tr("Avg MPH"), this);
    avgspeedLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    avgcadenceLabel = new QLabel(tr("Avg RPM"), this);
    avgcadenceLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    avgloadLabel = new QLabel(tr("Avg Load WATTS"), this);
    avgloadLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    laptimeLabel = new QLabel(tr("LAP TIME"), this);
    laptimeLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    timeLabel = new QLabel(tr("TIME"), this);
    timeLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    powerLCD = new QLCDNumber(this); powerLCD->setSegmentStyle(QLCDNumber::Filled);
    heartrateLCD = new QLCDNumber(this); heartrateLCD->setSegmentStyle(QLCDNumber::Filled);
    speedLCD = new QLCDNumber(this); speedLCD->setSegmentStyle(QLCDNumber::Filled);
    cadenceLCD = new QLCDNumber(this); cadenceLCD->setSegmentStyle(QLCDNumber::Filled);
    lapLCD = new QLCDNumber(this); lapLCD->setSegmentStyle(QLCDNumber::Filled);
    loadLCD = new QLCDNumber(this); loadLCD->setSegmentStyle(QLCDNumber::Filled);
    distanceLCD = new QLCDNumber(this); distanceLCD->setSegmentStyle(QLCDNumber::Filled);

    avgpowerLCD = new QLCDNumber(this); avgpowerLCD->setSegmentStyle(QLCDNumber::Filled);
    avgheartrateLCD = new QLCDNumber(this); avgheartrateLCD->setSegmentStyle(QLCDNumber::Filled);
    avgspeedLCD = new QLCDNumber(this); avgspeedLCD->setSegmentStyle(QLCDNumber::Filled);
    avgcadenceLCD = new QLCDNumber(this); avgcadenceLCD->setSegmentStyle(QLCDNumber::Filled);
    avgloadLCD = new QLCDNumber(this); avgloadLCD->setSegmentStyle(QLCDNumber::Filled);

    laptimeLCD = new QLCDNumber(this); laptimeLCD->setSegmentStyle(QLCDNumber::Filled);
    laptimeLCD->setNumDigits(9);
    timeLCD = new QLCDNumber(this); timeLCD->setSegmentStyle(QLCDNumber::Filled);
    timeLCD->setNumDigits(9);

    gridLayout = new QGridLayout();
    gridLayout->addWidget(powerLabel, 1, 0);
    gridLayout->addWidget(cadenceLabel, 1, 1);
    gridLayout->addWidget(heartrateLabel, 1, 2);
    gridLayout->addWidget(speedLabel, 1, 3);
    gridLayout->addWidget(distanceLabel, 1, 4);
    gridLayout->addWidget(lapLabel, 1, 5);
    gridLayout->addWidget(powerLCD, 2, 0);
    gridLayout->addWidget(cadenceLCD, 2, 1);
    gridLayout->addWidget(heartrateLCD, 2, 2);
    gridLayout->addWidget(speedLCD, 2, 3);
    gridLayout->addWidget(distanceLCD, 2, 4);
    gridLayout->addWidget(lapLCD, 2, 5);
    gridLayout->addWidget(avgpowerLabel, 3, 0);
    gridLayout->addWidget(avgcadenceLabel, 3, 1);
    gridLayout->addWidget(avgheartrateLabel, 3, 2);
    gridLayout->addWidget(avgspeedLabel, 3, 3);
    gridLayout->addWidget(avgloadLabel, 3, 4);
    gridLayout->addWidget(loadLabel, 3, 5);
    gridLayout->addWidget(loadLCD, 4, 5);
    gridLayout->addWidget(avgpowerLCD, 4, 0);
    gridLayout->addWidget(avgcadenceLCD, 4, 1);
    gridLayout->addWidget(avgheartrateLCD, 4, 2);
    gridLayout->addWidget(avgspeedLCD, 4, 3);
    gridLayout->addWidget(avgloadLCD, 4, 4);
    gridLayout->setRowStretch(2, 4);
    gridLayout->setRowStretch(4, 3);

    // timers etc
    timer_layout->addWidget(timeLabel, 0, 0);
    timer_layout->addWidget(laptimeLabel, 0, 3);
    timer_layout->addWidget(timeLCD, 1, 0);
    timer_layout->addItem(controls_layout, 1,1);
    timer_layout->addWidget(laptimeLCD, 1, 3);
    timer_layout->setRowStretch(0, 1);
    timer_layout->setRowStretch(1, 4);

    // REALTIME PLOT
    rtPlot = new RealtimePlot();
    connect(main, SIGNAL(configChanged()), rtPlot, SLOT(configChanged()));

    // COURSE PLOT
    ergPlot = new ErgFilePlot(0);
    ergPlot->setVisible(false);

    // LAYOUT

    main_layout->addWidget(ergPlot);
    main_layout->addItem(timer_layout);
    main_layout->addItem(gridLayout);
    main_layout->addWidget(rtPlot);

    displaymode=2;
    main_layout->setStretch(0,1);
    main_layout->setStretch(1,1);
    main_layout->setStretch(2,2);
    main_layout->setStretch(3, displaymode);


    // now the GUI is setup lets sort our control variables
    gui_timer = new QTimer(this);
    disk_timer = new QTimer(this);
    stream_timer = new QTimer(this);
    load_timer = new QTimer(this);

    session_time = QTime();
    session_elapsed_msec = 0;
    lap_time = QTime();
    lap_elapsed_msec = 0;

    recordFile = NULL;
    status = 0;
    status |= RT_RECORDING;         // recording is on by default! - add others here
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
    avgPower= avgHeartRate= avgSpeed= avgCadence= avgLoad= 0;

    connect(gui_timer, SIGNAL(timeout()), this, SLOT(guiUpdate()));
    connect(disk_timer, SIGNAL(timeout()), this, SLOT(diskUpdate()));
    connect(stream_timer, SIGNAL(timeout()), this, SLOT(streamUpdate()));
    connect(load_timer, SIGNAL(timeout()), this, SLOT(loadUpdate()));

    // setup the controller based upon currently selected device
    setDeviceController();
    rtPlot->replot();
}

void RealtimeWindow::setDeviceController()
{
    int deviceno = this->deviceSelector->currentIndex();

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
        } else if (Devices.at(deviceno).type == DEV_GSERVER) {
            deviceController = new SimpleNetworkController(this, &temp);
        }
    }

}

void RealtimeWindow::setStreamController()
{
    int deviceno = streamSelector->itemData(streamSelector->currentIndex()).toInt();

    // zap the current one
    if (streamController != NULL) {
        delete streamController;
        streamController = NULL;
    }

    if (Devices.count() > 0) {
        DeviceConfiguration temp = Devices.at(deviceno);
        if (Devices.at(deviceno).type == DEV_ANTPLUS) {
            streamController = new ANTplusController(this, &temp);
        } else if (Devices.at(deviceno).type == DEV_CT) {
            streamController = new ComputrainerController(this, &temp);
        } else if (Devices.at(deviceno).type == DEV_GSERVER) {
            streamController = new SimpleNetworkController(this, &temp);
        }
    }
}

void RealtimeWindow::SelectDevice(int index)
{
    if (index != -1)
        setDeviceController();
}

void RealtimeWindow::Start()       // when start button is pressed
{
    if (status&RT_RUNNING) {
        newLap();
    } else {
        status |=RT_RUNNING;
        deviceController->start();          // start device
        load_period.restart();
        startButton->setText(tr("Lap/Interval"));
        recordSelector->setEnabled(false);
        streamSelector->setEnabled(false);
        deviceSelector->setEnabled(false);

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

        // stream
        if (status & RT_STREAMING) {
            streamController->start();
            stream_timer->start(STREAMRATE);
        }

        gui_timer->start(REFRESHRATE);      // start recording

    }
}

void RealtimeWindow::Pause()        // pause capture to recalibrate
{
    // we're not running fool!
    if ((status&RT_RUNNING) == 0) return;

    if (status&RT_PAUSED) {
        session_time.start();
        lap_time.start();
        status &=~RT_PAUSED;
        deviceController->restart();
        pauseButton->setText(tr("Pause"));
        gui_timer->start(REFRESHRATE);
        if (status & RT_STREAMING) stream_timer->start(STREAMRATE);
        if (status & RT_RECORDING) disk_timer->start(SAMPLERATE);
        load_period.restart();
        if (status & RT_WORKOUT) load_timer->start(LOADRATE);
    } else {
        session_elapsed_msec += session_time.elapsed();
        lap_elapsed_msec += lap_time.elapsed();
        deviceController->pause();
        pauseButton->setText(tr("Un-Pause"));
        status |=RT_PAUSED;
        gui_timer->stop();
        if (status & RT_STREAMING) stream_timer->stop();
        if (status & RT_RECORDING) disk_timer->stop();
        if (status & RT_WORKOUT) load_timer->stop();
        load_msecs += load_period.restart();
    }
}

void RealtimeWindow::Stop(int deviceStatus)        // when stop button is pressed
{
    if ((status&RT_RUNNING) == 0) return;

    status &= ~RT_RUNNING;
    startButton->setText(tr("Start"));
    deviceController->stop();
    gui_timer->stop();

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
        streamController->stop();
    }

    if (status & RT_WORKOUT) {
        load_timer->stop();
        load_msecs = 0;
        ergPlot->setNow(load_msecs);
        ergPlot->replot();
    }

    // Re-enable gui elements
    recordSelector->setEnabled(true);
    streamSelector->setEnabled(true);
    deviceSelector->setEnabled(true);

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
    avgPower= avgHeartRate= avgSpeed= avgCadence= avgLoad= 0;
    displayWorkoutDistance = displayDistance = 0;
    guiUpdate();

    return;
}


// Called by push devices (e.g. ANT+)
void RealtimeWindow::updateData(RealtimeData &rtData)
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

void RealtimeWindow::guiUpdate()           // refreshes the telemetry
{
    RealtimeData rtData;

    // get latest telemetry from device (if it is a pull device e.g. Computrainer //
    if (status&RT_RUNNING && deviceController->doesPull() == true) {
        deviceController->getRealtimeData(rtData);
        displayPower = rtData.getWatts();
        displayCadence = rtData.getCadence();
        displayHeartRate = rtData.getHr();
        displaySpeed = rtData.getSpeed();
        displayLoad = rtData.getLoad();
    }

    // Distance assumes current speed for the last second. from km/h to km/sec
    displayDistance += displaySpeed / (5 * 3600); // XXX assumes 200ms refreshrate
    displayWorkoutDistance += displaySpeed / (5 * 3600); // XXX assumes 200ms refreshrate

    total_msecs = session_elapsed_msec + session_time.elapsed();
    lap_msecs = lap_elapsed_msec + lap_time.elapsed();

    // update those LCDs!
    timeLCD->display(QString("%1:%2:%3.%4").arg(total_msecs/3600000)
                                           .arg((total_msecs%3600000)/60000,2,10,QLatin1Char('0'))
                                           .arg((total_msecs%60000)/1000,2,10,QLatin1Char('0'))
                                           .arg((total_msecs%1000)/100));

    laptimeLCD->display(QString("%1:%2:%3.%4").arg(lap_msecs/3600000,2)
                                              .arg((lap_msecs%3600000)/60000,2,10,QLatin1Char('0'))
                                              .arg((lap_msecs%60000)/1000,2,10,QLatin1Char('0'))
                                              .arg((lap_msecs%1000)/100));

    // Cadence, HR and Power needs to be rounded to 0 decimal places
    powerLCD->display(round(displayPower));
    double val = round(displaySpeed * (useMetricUnits ? 1.0 : MILES_PER_KM) * 10.00)/10.00;
    speedLCD->display(QString::number(val, 'f', 1)); // always show 1 decimal point
    cadenceLCD->display(round(displayCadence));
    heartrateLCD->display(round(displayHeartRate));
    lapLCD->display(displayWorkoutLap+displayLap);

    // load or gradient depending on mode we are running
    if (status&RT_MODE_ERGO)
        loadLCD->display(displayLoad);
    else
    {
        val = round(displayGradient*10)/10.00;
        loadLCD->display(QString::number(val, 'f', 1)); // always show 1 decimal point
    }

    // distance
    val = round(displayDistance*(useMetricUnits ? 1.0 : MILES_PER_KM) *10.00) /10.00;
    distanceLCD->display(QString::number(val, 'f', 1)); // always show 1 decimal point

    // NZ Averages.....
    if (displayPower) { //NZAP is bogus - make it configurable!!!
        pwrcount++; if (pwrcount ==1) avgPower = displayPower;
        avgPower = ((avgPower * (double)pwrcount) + displayPower) /(double) (pwrcount+1);
    }
    if (displayCadence) {
        cadcount++; if (cadcount ==1) avgCadence = displayCadence;
        avgCadence = ((avgCadence * (double)cadcount) + displayCadence) /(double) (cadcount+1);
    }
    if (displayHeartRate) {
        hrcount++; if (hrcount ==1) avgHeartRate = displayHeartRate;
        avgHeartRate = ((avgHeartRate * (double)hrcount) + displayHeartRate) /(double) (hrcount+1);
    }
    if (displaySpeed) {
        spdcount++; if (spdcount ==1) avgSpeed = displaySpeed;
        avgSpeed = ((avgSpeed * (double)spdcount) + displaySpeed) /(double) (spdcount+1);
    }
    if (displayLoad && status&RT_MODE_ERGO) {
        lodcount++; if (lodcount ==1) avgLoad = displayLoad;
        avgLoad = ((avgLoad * (double)lodcount) + displayLoad) /(double) (lodcount+1);
        avgloadLCD->display((int)avgLoad);
    }
    if (status&RT_MODE_SPIN) {
        grdcount++; if (grdcount ==1) avgGradient = displayGradient;
        avgGradient = ((avgGradient * (double)grdcount) + displayGradient) /(double) (grdcount+1);
        avgloadLCD->display((int)avgGradient);
    }

    avgpowerLCD->display((int)avgPower);
    val = round(avgSpeed * (useMetricUnits ? 1.0 : MILES_PER_KM) * 10.00)/10.00;
    avgspeedLCD->display(QString::number(val, 'f', 1)); // always show 1 decimal point
    avgcadenceLCD->display((int)avgCadence);
    avgheartrateLCD->display((int)avgHeartRate);


    // now that plot....
    rtPlot->pwrData.addData(displayPower); // add new data point
    rtPlot->cadData.addData(displayCadence); // add new data point
    rtPlot->spdData.addData(displaySpeed); // add new data point
    rtPlot->hrData.addData(displayHeartRate); // add new data point
    //rtPlot->lodData.addData(displayLoad); // add new Load point
    rtPlot->replot();                // redraw

    this->update();

}

// can be called from the controller - when user presses "Lap" button
void RealtimeWindow::newLap()
{
    displayLap++;

    pwrcount  = 0;
    cadcount  = 0;
    hrcount   = 0;
    spdcount  = 0;

    lap_time.restart();
    lap_elapsed_msec = 0;

    // set avg to current values to ensure averages represent from now onwards
    // and not from beginning of workout
    avgPower = displayPower;
    avgCadence = displayCadence;
    avgHeartRate = displayHeartRate;
    avgSpeed = displaySpeed;
    avgLoad = displayLoad;
}

// can be called from the controller
void RealtimeWindow::nextDisplayMode()
{
    if (displaymode == 8) displaymode=2;
    else displaymode += 1;
    main_layout->setStretch(3,displaymode);
}

void RealtimeWindow::warnnoConfig()
{
    QMessageBox::warning(this, tr("No Devices Configured"), "Please configure a device in Preferences.");
}

//----------------------------------------------------------------------
// STREAMING FUNCTION
//----------------------------------------------------------------------
void
RealtimeWindow::SelectStream(int index)
{

    if (index > 0) {
        status |= RT_STREAMING;
        setStreamController();
    } else {
        status &= ~RT_STREAMING;
    }
}

void
RealtimeWindow::streamUpdate()
{
    RealtimeData rtData;

    // get current telemetry...
    rtData.setWatts(displayPower);
    rtData.setCadence(displayCadence);
    rtData.setHr(displayHeartRate);
    rtData.setSpeed(displaySpeed);
    rtData.setLoad(displayLoad);
    rtData.setTime(0);

    // send over the wire...
    streamController->pushRealtimeData(rtData);
}

//----------------------------------------------------------------------
// DISK UPDATE FUNCTIONS
//----------------------------------------------------------------------
void
RealtimeWindow::SelectRecord()
{
    if (recordSelector->isChecked()) {
        status |= RT_RECORDING;
    } else {
        status &= ~RT_RECORDING;
    }
}

void RealtimeWindow::diskUpdate()
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
}

//----------------------------------------------------------------------
// WORKOUT MODE
//----------------------------------------------------------------------

void
RealtimeWindow::SelectWorkout()
{
    int mode;

    // wip away the current selected workout
    if (ergFile) {
        delete ergFile;
        ergFile = NULL;
    }

    // which one is selected?
    if (trainTool->currentWorkout() == NULL || trainTool->currentWorkout()->type() != WORKOUT_TYPE) return;

    // is it the auto mode?
    int index = trainTool->workoutItems()->indexOfChild((QTreeWidgetItem *)trainTool->currentWorkout());
    if (index == 0) {
        // ergo mode
        mode = ERG;
        status &= ~RT_WORKOUT;
        ergPlot->setVisible(false);
    } else if (index == 1) {
        // slope mode
        mode = CRS;
        status &= ~RT_WORKOUT;
        ergPlot->setVisible(false);
    } else {
        // workout mode
        boost::shared_ptr<QSettings> settings = GetApplicationSettings();
        QVariant workoutDir = settings->value(GC_WORKOUTDIR);
        QString fileName = workoutDir.toString() + "/" + trainTool->currentWorkout()->text(0); // filename

        // Get users CP for relative watts calculations
        QDate today = QDate::currentDate();
        double Cp=285;                       // default to 285 if zones are not set
        int range = main->zones()->whichRange(today);
        if (range != -1) Cp = main->zones()->getCP(range);

        ergFile = new ErgFile(fileName, mode, Cp);
        if (ergFile->isValid()) {

            status |= RT_WORKOUT;

            // success! we have a load file
            // setup the course profile in the
            // display!
            ergPlot->setData(ergFile);
            ergPlot->setVisible(true);
            ergPlot->replot();
        }
    }

    // set the device to the right mode
    if (mode == ERG || mode == MRC) {
        status |= RT_MODE_ERGO;
        status &= ~RT_MODE_SPIN;
        if (deviceController != NULL) deviceController->setMode(RT_MODE_ERGO);
        // set the labels on the gui
        loadLabel->setText(tr("Load WATTS"));
        avgloadLabel->setText(tr("Avg Load WATTS"));
    } else { // SLOPE MODE
        status |= RT_MODE_SPIN;
        status &= ~RT_MODE_ERGO;
        if (deviceController != NULL) deviceController->setMode(RT_MODE_SPIN);
        // set the labels on the gui
        loadLabel->setText(tr("Gradient PERCENT"));
        avgloadLabel->setText(tr("Avg Gradient PERCENT"));
    }
}

void RealtimeWindow::loadUpdate()
{
    long load;
    double gradient;
    // the period between loadUpdate calls is not constant, and not exactly LOADRATE,
    // therefore, use a QTime timer to measure the load period
    load_msecs += load_period.restart();

    if (status&RT_MODE_ERGO) {
        load = ergFile->wattsAt(load_msecs, displayWorkoutLap);

        // we got to the end!
        if (load == -100) {
            Stop(DEVICE_OK);
        } else {
            displayLoad = load;
            deviceController->setLoad(displayLoad);
            ergPlot->setNow(load_msecs);
        }
    } else {
        gradient = ergFile->gradientAt(displayWorkoutDistance*1000, displayWorkoutLap);

        // we got to the end!
        if (gradient == -100) {
            Stop(DEVICE_OK);
        } else {
            displayGradient = gradient;
            deviceController->setGradient(displayGradient);
            ergPlot->setNow(displayWorkoutDistance * 1000); // now is in meters we keep it in kilometers
        }
    }
}

void RealtimeWindow::FFwd()
{
    if (status&RT_MODE_ERGO) load_msecs += 10000; // jump forward 10 seconds
    else displayWorkoutDistance += 1; // jump forward a kilometer in the workout
}

void RealtimeWindow::Rewind()
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
void RealtimeWindow::FFwdLap()
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
void RealtimeWindow::Higher()
{
    if (status&RT_MODE_ERGO) displayLoad += 5;
    else displayGradient += 0.1;

    if (displayLoad >1500) displayLoad = 1500;
    if (displayGradient >15) displayGradient = 15;

    if (status&RT_MODE_ERGO) deviceController->setLoad(displayLoad);
    else deviceController->setGradient(displayGradient);
}

// higher load/gradient
void RealtimeWindow::Lower()
{
    if (status&RT_MODE_ERGO) displayLoad -= 5;
    else displayGradient -= 0.1;

    if (displayLoad <0) displayLoad = 0;
    if (displayGradient <-10) displayGradient = -10;

    if (status&RT_MODE_ERGO) deviceController->setLoad(displayLoad);
    else deviceController->setGradient(displayGradient);
}
