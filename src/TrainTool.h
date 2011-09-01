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
#include "GoldenClient.h"
#include "RealtimeData.h"
#include "RealtimePlot.h"
#include "DeviceConfiguration.h"
#include "DeviceTypes.h"
#include "ErgFile.h"
#include "ErgFilePlot.h"

// standard stuff
#include <QDir>
#include <QtGui>
#include "math.h" // for round()
#include "Units.h" // for MILES_PER_KM
#include <QDebug>

// Status settings
#define RT_MODE_ERGO    0x0001        // load generation modes
#define RT_MODE_SPIN    0x0002        // spinscan like modes
#define RT_RUNNING      0x0100        // is running now
#define RT_PAUSED       0x0200        // is paused
#define RT_RECORDING    0x0400        // is recording to disk
#define RT_WORKOUT      0x0800        // is running a workout
#define RT_STREAMING    0x1000        // is streaming to a remote peer

// msecs constants for timers
#define REFRESHRATE    200 // screen refresh in milliseconds
#define STREAMRATE     200 // rate at which we stream updates to remote peer
#define SAMPLERATE     1000 // disk update in milliseconds
#define LOADRATE       1000 // rate at which load is adjusted
#define METRICSRATE    1000 // rate the metrics are updated

// device treeview node types
#define HEAD_TYPE    6666
#define SERVER_TYPE  5555
#define WORKOUT_TYPE 4444
#define DEVICE_TYPE  0

class RealtimeController;
class ComputrainerController;
class ANTplusController;
class ANTlocalController;
class NullController;
class RealtimePlot;
class RealtimeData;

class TrainTool : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    public:

        TrainTool(MainWindow *parent, const QDir &home);
        QStringList listWorkoutFiles(const QDir &) const;

        RealtimeController *deviceController;   // read from
        GoldenClient       *streamController;   // send out to

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

        // was realtimewindow,merged into tool
        // update charts/dials and manage controller
        void updateData(RealtimeData &);      // to update telemetry by push devices
        void newLap();                      // start new Lap!
        void nextDisplayMode();     // show next display mode
        void setDeviceController();     // based upon selected device
        void setStreamController();     // based upon selected device

        // this
        QTabWidget  *trainTabs;

    signals:

        void deviceSelected();
        void serverSelected();
        void raceStandings(RaceStatus);
        void start();
        void pause();
        void stop();

    private slots:
        void serverTreeWidgetSelectionChanged();
        void deviceTreeWidgetSelectionChanged();
        void workoutTreeWidgetSelectionChanged();
        void configChanged();

    public slots:

        void Start();       // when start button is pressed
        void Pause();       // when Paude is pressed
        void Stop(int status=0);        // when controller wants to stop

        void FFwd();        // jump forward when in a workout
        void Rewind();      // jump backwards when in a workout
        void FFwdLap();     // jump forward to next Lap marker
        void Higher();      // set load/gradient higher
        void Lower();       // set load/gradient higher

        // Timed actions
        void guiUpdate();           // refreshes the telemetry
        void diskUpdate();          // writes to CSV file
        void streamUpdate();        // writes to remote Peer
        void loadUpdate();          // sets Load on CT like devices
        void metricsUpdate();       // calculates the metrics

        // When no config has been setup
        void warnnoConfig();

    private:

        const QDir home;
        MainWindow *main;

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

        int FTP; // current FTP

        QList<DeviceConfiguration> Devices;
        bool useMetricUnits;

        // updated with a RealtimeData object either from
        // update() - from a push device (quarqd ANT+)
        // Device->getRealtimeData() - from a pull device (Computrainer)
        double displayPower, displayHeartRate, displayCadence,
               displayLoad, displayGradient, displaySpeed;
        double displayDistance, displayWorkoutDistance;
        int displayLap;            // user increment for Lap
        int displayWorkoutLap;     // which Lap in the workout are we at?
        double kjoules;
        double bikescore;
        double xpower;

        // for non-zero average calcs
        int pwrcount, cadcount, hrcount, spdcount, lodcount, grdcount; // for NZ average calc
        int status;
        int displaymode;

        QFile *recordFile;      // where we record!
        ErgFile *ergFile;       // workout file
        boost::shared_ptr<RideFile> rideFile;     // keeps track of the workout to figure out BikeScore

        long total_msecs,
             lap_msecs,
             load_msecs;
        QTime load_period;

        uint session_elapsed_msec, lap_elapsed_msec;
        QTime session_time, lap_time;

        QTimer      *gui_timer,     // refresh the gui
                    *stream_timer,  // send telemetry to server
                    *load_timer,    // change the load on the device
                    *disk_timer,    // write to .CSV file
                    *metrics_timer; // computational intense metrics

    public:
        // everyone else wants this
        QCheckBox   *recordSelector;
        boost::shared_ptr<QFileSystemWatcher> watcher;
};

#endif // _GC_TrainTool_h

