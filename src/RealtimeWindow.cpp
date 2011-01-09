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

// user selections for device/server/workout here
#include "TrainTool.h"

// Three current realtime device types supported are:
#include "ComputrainerController.h"
#include "ANTplusController.h"
#include "NullController.h"
#include "ErgFile.h"

// And connect to server is via a GoldenServer object
#include "GoldenClient.h"


void
RealtimeWindow::configUpdate()
{

    FTP=285; // default to 285 if zones are not set
    int range = main->zones()->whichRange(QDate::currentDate());
    if (range != -1) FTP = main->zones()->getCP(range);

    // metric or imperial changed?
    QVariant unit = appsettings->value(this, GC_UNIT);
    useMetricUnits = (unit.toString() == "Metric");

    // set labels accordingly
    distanceLabel->setText(useMetricUnits ? tr("Distance (KM)") : tr("Distance (Miles)"));
    speedLabel->setText(useMetricUnits ? tr("KPH") : tr("MPH"));
    avgspeedLabel->setText(useMetricUnits ? tr("Avg KPH") : tr("Avg MPH"));

    // devices
    DeviceConfigurations all;
    Devices.clear();
    Devices = all.getList();
}

RealtimeWindow::RealtimeWindow(MainWindow *parent, TrainTool *trainTool, const QDir &home)  : GcWindow(parent)
{

    setInstanceName("Solo Ride Window");
    setControls(NULL);

    // set home
    this->home = home;
    this->trainTool = trainTool;
    main = parent;
    deviceController = NULL;
    streamController = NULL;
    ergFile = NULL;

    // metric or imperial?
    QVariant unit = appsettings->value(this, GC_UNIT);
    useMetricUnits = (unit.toString() == "Metric");

    // main layout for the window
    main_layout = new QVBoxLayout(this);
    timer_layout = new QGridLayout();

    // BUTTONS AND LCDS
    button_layout = new QHBoxLayout();
    option_layout = new QHBoxLayout();
    controls_layout = new QVBoxLayout();

    // handle config changes
    connect(main, SIGNAL(configChanged()), this, SLOT(configUpdate()));

    connect(trainTool, SIGNAL(workoutSelected()), this, SLOT(SelectWorkout()));

    // connect train tool buttons!
    connect(trainTool, SIGNAL(start()), this, SLOT(Start()));
    connect(trainTool, SIGNAL(pause()), this, SLOT(Pause()));
    connect(trainTool, SIGNAL(stop()), this, SLOT(Stop()));

    powerLabel = new QLabel(tr("WATTS"), this);
    powerLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    heartrateLabel = new QLabel(tr("BPM"), this);
    heartrateLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    speedLabel = new QLabel(useMetricUnits ? tr("KPH") : tr("MPH"), this);
    speedLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    cadenceLabel = new QLabel(tr("RPM"), this);
    cadenceLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    lapLabel = new QLabel(tr("Lap/Interval"), this);
    lapLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    loadLabel = new QLabel(tr("Load WATTS"), this);
    loadLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    distanceLabel = new QLabel(useMetricUnits ? tr("Distance (KM)") : tr("Distance (Miles)"), this);
    distanceLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    kjouleLabel = new QLabel(tr("kJoules"),this);
    kjouleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);


    avgpowerLabel = new QLabel(tr("Avg WATTS"), this);
    avgpowerLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    avgheartrateLabel = new QLabel(tr("Avg BPM"), this);
    avgheartrateLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    avgspeedLabel = new QLabel(useMetricUnits ? tr("Avg KPH") : tr("Avg MPH"), this);
    avgspeedLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    avgcadenceLabel = new QLabel(tr("Avg RPM"), this);
    avgcadenceLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    xpowerLabel = new QLabel(tr("xPower"), this);
    xpowerLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    bikescoreLabel = new QLabel(tr("BikeScore"),this);
    bikescoreLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

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
    distanceLCD->setNumDigits(9); // just to be same size as timers!
    kjouleLCD = new QLCDNumber(this); distanceLCD->setSegmentStyle(QLCDNumber::Filled);
    bikescoreLCD = new QLCDNumber(this); distanceLCD->setSegmentStyle(QLCDNumber::Filled);

    avgpowerLCD = new QLCDNumber(this); avgpowerLCD->setSegmentStyle(QLCDNumber::Filled);
    avgheartrateLCD = new QLCDNumber(this); avgheartrateLCD->setSegmentStyle(QLCDNumber::Filled);
    avgspeedLCD = new QLCDNumber(this); avgspeedLCD->setSegmentStyle(QLCDNumber::Filled);
    avgcadenceLCD = new QLCDNumber(this); avgcadenceLCD->setSegmentStyle(QLCDNumber::Filled);
    xpowerLCD = new QLCDNumber(this); xpowerLCD->setSegmentStyle(QLCDNumber::Filled);

    laptimeLCD = new QLCDNumber(this); laptimeLCD->setSegmentStyle(QLCDNumber::Filled);
    laptimeLCD->setNumDigits(9);
    timeLCD = new QLCDNumber(this); timeLCD->setSegmentStyle(QLCDNumber::Filled);
    timeLCD->setNumDigits(9);

    // 4 x 6 grid.
    int row = 0;
    int colm = 0;
    gridLayout = new QGridLayout();

    gridLayout->addWidget(powerLabel, row, colm);
    gridLayout->addWidget(powerLCD, row+1, colm);

    colm++;
    gridLayout->addWidget(cadenceLabel, row, colm);
    gridLayout->addWidget(cadenceLCD, row+1, colm);

    colm++;
    gridLayout->addWidget(heartrateLabel, row, colm);
    gridLayout->addWidget(heartrateLCD, row+1, colm);

    colm++;
    gridLayout->addWidget(speedLabel, row, colm);
    gridLayout->addWidget(speedLCD, row+1, colm);

    colm++;
    gridLayout->addWidget(lapLabel, row, colm);
    gridLayout->addWidget(lapLCD, row+1, colm);

    colm++;
    gridLayout->addWidget(kjouleLabel,row,colm);
    gridLayout->addWidget(kjouleLCD, row +1,colm);

    // end of first row of data
    colm = 0;
    row += 2;
    gridLayout->addWidget(avgpowerLabel, row, colm);
    gridLayout->addWidget(avgpowerLCD, row+1, colm);

    colm++;
    gridLayout->addWidget(avgcadenceLabel, row, colm);
    gridLayout->addWidget(avgcadenceLCD, row+1, colm);

    colm++;
    gridLayout->addWidget(avgheartrateLabel, row, colm);
    gridLayout->addWidget(avgheartrateLCD, row+1, colm);

    colm++;
    gridLayout->addWidget(avgspeedLabel, row, colm);
    gridLayout->addWidget(avgspeedLCD, row+1, colm);

    colm++;
    gridLayout->addWidget(xpowerLabel, row, colm);
    gridLayout->addWidget(xpowerLCD, row+1, colm);

    colm++;
    gridLayout->addWidget(bikescoreLabel, row, colm);
    gridLayout->addWidget(bikescoreLCD, row+1, colm);

    colm++;
    gridLayout->addWidget(loadLabel, row, colm);
    gridLayout->addWidget(loadLCD, row+1, colm);

    gridLayout->setRowStretch(1, 3);
    gridLayout->setRowStretch(3, 3);

    // timers etc
    timer_layout->addWidget(timeLabel, 0, 0);
    timer_layout->addWidget(distanceLabel, 0, 1);
    timer_layout->addWidget(laptimeLabel, 0, 2);
    timer_layout->addWidget(timeLCD, 1, 0);
    timer_layout->addWidget(distanceLCD, 1, 1);
    timer_layout->addWidget(laptimeLCD, 1, 2);
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
    metrics_timer = new QTimer(this);

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
    avgPower= avgHeartRate= avgSpeed= avgCadence= avgLoad= 0;

    rideFile = boost::shared_ptr<RideFile>(new RideFile(QDateTime::currentDateTime(),1));

    connect(gui_timer, SIGNAL(timeout()), this, SLOT(guiUpdate()));
    connect(disk_timer, SIGNAL(timeout()), this, SLOT(diskUpdate()));
    connect(stream_timer, SIGNAL(timeout()), this, SLOT(streamUpdate()));
    connect(load_timer, SIGNAL(timeout()), this, SLOT(loadUpdate()));
    connect(metrics_timer, SIGNAL(timeout()), this, SLOT(metricsUpdate()));


    configUpdate(); // apply current config

    rtPlot->replot();
}

void RealtimeWindow::setDeviceController()
{
    int deviceno = trainTool->selectedDeviceNumber();

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
        }
    }
}

// open a connection to the GoldenServer via a GoldenClient
void RealtimeWindow::setStreamController()
{
    int deviceno = trainTool->selectedServerNumber();

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

void RealtimeWindow::Start()       // when start button is pressed
{

    if (status&RT_RUNNING) {
        newLap();
    } else {

        // open the controller if it is selected
        setDeviceController();
        if (deviceController == NULL) return;
        else deviceController->start();          // start device

        // we're away!
        status |=RT_RUNNING;

        // should we be streaming too?
        setStreamController();
        if (streamController != NULL) status |= RT_STREAMING;

        trainTool->setStartText(tr("Lap/Interval"));
        //recordSelector->setEnabled(false);

        if (status & RT_WORKOUT) {
            load_timer->start(LOADRATE);      // start recording
        }
        if (trainTool->recordSelector->isChecked()) {
            status |= RT_RECORDING;

            // setup file
            QDate date = QDate().currentDate();
            QTime time = QTime().currentTime();
            QDateTime now(date, time);
            QChar zero = QLatin1Char ( '0' );
            QString filename = QString ( "%1_%2_%3_%4_%5_%6.csv" )
                               .arg ( now.date().year(), 4, 10, zero )
                               .arg ( now.date().month(), 2, 10, zero )
                               .arg ( now.date().day(), 2, 10, zero )
                               .arg ( now.time().hour(), 2, 10, zero )
                               .arg ( now.time().minute(), 2, 10, zero )
                               .arg ( now.time().second(), 2, 10, zero );

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
        } else {
            status &= ~RT_RECORDING;
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

void RealtimeWindow::Pause()        // pause capture to recalibrate
{
    if (deviceController == NULL) return;

    // we're not running fool!
    if ((status&RT_RUNNING) == 0) return;

    if (status&RT_PAUSED) {
        status &=~RT_PAUSED;
        deviceController->restart();
        trainTool->setPauseText(tr("Pause"));
        gui_timer->start(REFRESHRATE);
        metrics_timer->start(METRICSRATE);
        if (status & RT_STREAMING) stream_timer->start(STREAMRATE);
        if (status & RT_RECORDING) disk_timer->start(SAMPLERATE);
        if (status & RT_WORKOUT) load_timer->start(LOADRATE);
    } else {
        deviceController->pause();
        trainTool->setPauseText(tr("Un-Pause"));
        status |=RT_PAUSED;
        gui_timer->stop();
        metrics_timer->stop();
        if (status & RT_STREAMING) stream_timer->stop();
        if (status & RT_RECORDING) disk_timer->stop();
        if (status & RT_WORKOUT) load_timer->stop();
    }
}

void RealtimeWindow::Stop(int deviceStatus)        // when stop button is pressed
{
    if (deviceController == NULL) return;

    if ((status&RT_RUNNING) == 0) return;

    status &= ~RT_RUNNING;
    trainTool->setStartText(tr("Start"));

    // wipe connection
    deviceController->stop();
    delete deviceController;
    deviceController = NULL;

    gui_timer->stop();
    metrics_timer->stop();

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
        ergPlot->setNow(load_msecs);
        ergPlot->replot();
    }

    // Re-enable gui elements
    //recordSelector->setEnabled(true);

    // reset counters etc
    pwrcount = 0;
    cadcount = 0;
    hrcount = 0;
    spdcount = 0;
    lodcount = 0;
    displayWorkoutLap = displayLap =0;
    lap_msecs = 0;
    total_msecs = 0;
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

    if (deviceController == NULL) return;

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
    displaySpeed *=(useMetricUnits ? 1.0 : MILES_PER_KM);
    speedLCD->display(QString::number(displaySpeed,'f', 1));
    cadenceLCD->display(round(displayCadence));
    heartrateLCD->display(round(displayHeartRate));
    lapLCD->display(displayWorkoutLap+displayLap);

    // load or gradient depending on mode we are running
    if (status&RT_MODE_ERGO) loadLCD->display(displayLoad);
    else loadLCD->display(round(displayGradient*10)/10.00);

    // distance
    distanceLCD->display(QString::number(displayDistance*(useMetricUnits ? 1.0 : MILES_PER_KM),'f',1));

    // NZ Averages.....
    if (displayPower) { //NZAP is bogus - make it configurable!!!
        pwrcount++; if (pwrcount ==1) avgPower = displayPower;
        avgPower = ((avgPower * (double)pwrcount) + displayPower) /(double) (pwrcount+1);
        kjoules = avgPower * total_msecs /(1000*1000);
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
#if 0
    if (displayLoad && status&RT_MODE_ERGO) {
        lodcount++; if (lodcount ==1) avgLoad = displayLoad;
        avgLoad = ((avgLoad * (double)lodcount) + displayLoad) /(double) (lodcount+1);
        //avgloadLCD->display((int)avgLoad);
    }
#endif
    if (status&RT_MODE_SPIN) {
        grdcount++; if (grdcount ==1) avgGradient = displayGradient;
        avgGradient = ((avgGradient * (double)grdcount) + displayGradient) /(double) (grdcount+1);
     //   avgloadLCD->display((int)avgGradient);
    }

    avgpowerLCD->display((int)avgPower);
    avgspeedLCD->display(QString::number(avgSpeed,'f', 1));

    avgcadenceLCD->display((int)avgCadence);
    avgheartrateLCD->display((int)avgHeartRate);
    kjouleLCD->display(round(kjoules));
    bikescoreLCD->display(round(bikescore));
    xpowerLCD->display(round(xpower));

    // now that plot....
    rtPlot->pwrData.addData(displayPower); // add new data point
    rtPlot->cadData.addData(displayCadence); // add new data point
    rtPlot->spdData.addData(displaySpeed); // add new data point
    rtPlot->hrData.addData(displayHeartRate); // add new data point
    //rtPlot->lodData.addData(displayLoad); // add new Load point
    rtPlot->replot();                // redraw

    this->update();

    // add time
    total_msecs += REFRESHRATE;
    lap_msecs += REFRESHRATE;

}

// can be called from the controller - when user presses "Lap" button
void RealtimeWindow::newLap()
{
    displayLap++;

    lap_msecs = 0;
    pwrcount  = 0;
    cadcount  = 0;
    hrcount   = 0;
    spdcount  = 0;

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
#if 0
RealtimeWindow::SelectStream(int index)
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
RealtimeWindow::streamUpdate()
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
        trainTool->notifyRaceStandings(current);

        // has the race finished?
        if (current.race_finished == true) {
            Stop(); // all over dude
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
void RealtimeWindow::diskUpdate()
{
    double  Minutes;

    long Torq = 0, Altitude = 0;
    QTextStream recordFileStream(recordFile);

    // convert from milliseconds to minutes
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

void RealtimeWindow::metricsUpdate()
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
        QVariant workoutDir = appsettings->value(this, GC_WORKOUTDIR);
        QString fileName = workoutDir.toString() + "/" + trainTool->currentWorkout()->text(0); // filename

        // Get users CP for relative watts calculations
        QDate today = QDate::currentDate();

        ergFile = new ErgFile(fileName, mode, FTP);
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
        loadLabel->setText("Load WATTS");
    } else { // SLOPE MODE
        status |= RT_MODE_SPIN;
        status &= ~RT_MODE_ERGO;
        if (deviceController != NULL) deviceController->setMode(RT_MODE_SPIN);
        // set the labels on the gui
        loadLabel->setText("Gradient PERCENT");
    }
}

void RealtimeWindow::loadUpdate()
{
    long load;
    double gradient;
    load_msecs += LOADRATE;

    if (deviceController == NULL) return;

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
    if (deviceController == NULL) return;

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
    if (deviceController == NULL) return;

    if (status&RT_MODE_ERGO) displayLoad -= 5;
    else displayGradient -= 0.1;

    if (displayLoad <0) displayLoad = 0;
    if (displayGradient <-10) displayGradient = -10;

    if (status&RT_MODE_ERGO) deviceController->setLoad(displayLoad);
    else deviceController->setGradient(displayGradient);
}
