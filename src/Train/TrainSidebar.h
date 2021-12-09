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

#ifndef _GC_TrainSidebar_h
#define _GC_TrainSidebar_h 1
#include "GoldenCheetah.h"

#include "Context.h"
#include "RealtimeData.h"
#include "RealtimePlot.h"
#include "DeviceConfiguration.h"
#include "DeviceTypes.h"
#include "ErgFile.h"
#include "VideoSyncFile.h"
#include "ErgFilePlot.h"
#include "GcSideBarItem.h"
#include "RemoteControl.h"
#include "AthleteTab.h"
#include "PhysicsUtility.h"

// standard stuff
#include <QDir>
#include <QDialog>
#include <QtGui>
#include <QCheckBox>
#include <QLabel>
#include <QDebug>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QFormLayout>
#include <QSqlTableModel>

#include "cmath" // for round()
#include "Units.h" // for MILES_PER_KM

#include "PhysicsUtility.h"
#include "BicycleSim.h"


// Status settings
#define RT_MODE_ERGO        0x0001        // load generation modes
#define RT_MODE_SPIN        0x0002        // spinscan like modes
#define RT_MODE_SLOPE       0x0002        // same as spinscan but not so CT specific
#define RT_MODE_CALIBRATE   0x0004        // calibrate

#define RT_RUNNING      0x0100        // is running now
#define RT_PAUSED       0x0200        // is paused
#define RT_RECORDING    0x0400        // is recording to disk
#define RT_WORKOUT      0x0800        // is running a workout
#define RT_STREAMING    0x1000        // is streaming to a remote peer
#define RT_CONNECTED    0x2000        // is connected to devices
#define RT_CALIBRATING  0x4000        // is calibrating

// msecs constants for timers
#define REFRESHRATE    200 // screen refresh in milliseconds
#define STREAMRATE     200 // rate at which we stream updates to remote peer
#define SAMPLERATE     1000 // disk update in milliseconds
#define LOADRATE       1000 // rate at which load is adjusted

// device treeview node types
#define HEAD_TYPE    6666
#define SERVER_TYPE  5555
#define WORKOUT_TYPE 4444

class RealtimeController;
class ComputrainerController;
class ANTlocalController;
class NullController;
class RealtimePlot;
class RealtimeData;
class MultiDeviceDialog;
class TrainBottom;
class DeviceTreeView;

class TrainSidebar : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    public:

        TrainSidebar(Context *context);
        Context *context;
        void setTrainView(TrainView*x) { trainView=x; }

        QStringList listWorkoutFiles(const QDir &) const;

        QList<int> devices(); // convenience function for iterating over active devices

        int selectedDeviceNumber();

        // set labels when ergfile selected etc
        void setLabels();

        // was realtimewindow,merged into tool
        // update charts/dials and manage controller
        void updateData(RealtimeData &);      // to update telemetry by push devices
        void setStreamController();     // based upon selected device

        // this
        QTabWidget  *trainTabs;

        void setStatusFlags(int flags);   // helpers that update context etc when changing state
        void clearStatusFlags(int flags);

        // where to get telemetry from Device.at(x)
        int bpmTelemetry;   // Heartrate
        int wattsTelemetry; // Power (and AltPower)
        int rpmTelemetry;   // Cadence
        int kphTelemetry;   // Speed (and Distance)

        RemoteControl *remote;      // remote control settings
        int currentStatus() {return status;}

    signals:
        void deviceSelected();
        void start();
        void pause();
        void stop();
        void intensityChanged(int value);
        void statusChanged(int status);
        void setNotification(QString msg, int timeout);
        void clearNotification(void);

    private slots:
        void deviceTreeWidgetSelectionChanged();
        void workoutTreeWidgetSelectionChanged();
        void videosyncTreeWidgetSelectionChanged();
        void mediaTreeWidgetSelectionChanged();

        void deviceTreeMenuPopup(const QPoint &);
        void deleteDevice();
        void moveDevices(int, int);

        void devicePopup();
        void workoutPopup();
        void videosyncPopup();
        void mediaPopup();

        void refresh(); // when TrainDB is updated...

        void selectVideo(QString fullpath);
        void selectVideoSync(QString fullpath);
        void selectWorkout(QString fullpath);

        void removeInvalidVideoSync();
        void removeInvalidWorkout();

        void viewChanged(int index);

        int  getCalibrationIndex(void);

    public slots:
        void configChanged(qint32);
        void deleteWorkouts(); // deletes selected workouts
        void deleteVideos(); // deletes selected workouts
        void deleteVideoSyncs(); // deletes selected VideoSync

        void Start();       // when start button is pressed
        void Pause();       // when Paude is pressed
        void Stop(int status=0);        // when controller wants to stop
        void Connect();     // connect to train devices
        void Disconnect();  // disconnect train devices
        void toggleConnect();

        void Calibrate();   // toggle calibration mode
        void FFwd();        // jump forward when in a workout
        void Rewind();      // jump backwards when in a workout
        void FFwdLap();     // jump forward to next Lap marker
        void RewindLap();   // jump backwards to previous Lap marker
        void Higher();      // set load/gradient higher
        void Lower();       // set load/gradient higher
        void newLap();      // start new Lap!
        void resetLapTimer(); //reset the lap timer
        void resetTextAudioEmitTracking();
        void steerScroll(int scrollAmount);   // Scroll the train display

        void toggleCalibration();
        void updateCalibration();

        // Timed actions
        void guiUpdate();           // refreshes the telemetry
        void diskUpdate();          // writes to CSV file
        void loadUpdate();          // sets Load on CT like devices

        // When no config has been setup
        void warnnoConfig();

        // User adjusted intensity
        void adjustIntensity(int value);     // Intensity of workout user adjusted

        // slot for receiving remote control commands
        void remoteControl(uint16_t);

        // HRV R-R data being saved away
        void rrData(uint16_t  rrtime, uint8_t heartrateBeats, uint8_t instantHeartrate);

        // VO2 measurement data to save
        void vo2Data(double rf, double rmv, double vo2, double vco2, double tv, double feo2);

    protected:

        friend class ::MultiDeviceDialog;

        // watch keyboard events.
        bool eventFilter(QObject *object, QEvent *e);

        GcSplitter   *trainSplitter;
        GcSplitterItem *deviceItem,
                       *workoutItem,
                       *videosyncItem,
                       *mediaItem;

        QSqlTableModel *videoModel;
        QSqlTableModel *videosyncModel;
        QSqlTableModel *workoutModel;

        DeviceTreeView *deviceTree;
        QTreeView *workoutTree;
        QTreeView *videosyncTree;
        QTreeView *mediaTree;
        QSortFilterProxyModel *sortModel;  // sorting workout list
        QSortFilterProxyModel *vsortModel; // sorting video list
        QSortFilterProxyModel *vssortModel; // sorting videosync list

        int lastAppliedIntensity;// remember how we scaled last time

        int FTP; // current FTP / CP
        int WPRIME; // current W'

        QList<DeviceConfiguration> Devices;
        QList<int> activeDevices;

        // updated with a RealtimeData object either from
        // update() - from a push device (quarqd ANT+)
        // Device->getRealtimeData() - from a pull device (Computrainer)
        double displayPower, displayHeartRate, displayCadence, displaySpeed;
        double displayLRBalance, displayLTE, displayRTE, displayLPS, displayRPS;
        double displaySMO2, displayTHB, displayO2HB, displayHHB;
        double displayDistance, displayWorkoutDistance;
        double displayLapDistance, displayLapDistanceRemaining;
        double displayLatitude, displayLongitude, displayAltitude; // geolocation
        long load;
        double slope;
        int displayWorkoutLap;     // which Lap in the workout are we at?
        bool lapAudioEnabled;
        bool lapAudioThisLap;
        double textPositionEmitted;
        bool useSimulatedSpeed;

        void maintainLapDistanceState();

        // for non-zero average calcs
        int pwrcount, cadcount, hrcount, spdcount, lodcount, grdcount; // for NZ average calc
        int status;
        int displaymode;

        QFile *recordFile;      // where we record!
        int lastRecordSecs;     // to avoid duplicates
        QFile *rrFile;          // r-r records, if any received.
        QFile *vo2File;         // vo2 records, if any received.

        // ErgFile wrapper to support stateful location queries.
        ErgFileQueryAdapter        ergFileQueryAdapter;

        VideoSyncFile             *videosyncFile;     // videosync file

        bool     startCalibration, restartCalibration, finishCalibration;
        int      calibrationDeviceIndex;
        uint8_t  calibrationType, calibrationState;
        uint16_t calibrationSpindownTime, calibrationZeroOffset, calibrationSlope;
        double   calibrationTargetSpeed, calibrationCurrentSpeed;
        double   calibrationCadence, calibrationTorque;

        long total_msecs,
             lap_msecs,
             load_msecs;
        QTime load_period;

        uint session_elapsed_msec, lap_elapsed_msec, secs_to_start;
        QTime session_time, lap_time;

        QTimer      *gui_timer,     // refresh the gui
                    *load_timer,    // change the load on the device
                    *start_timer,   // delayed start
                    *disk_timer;    // write to .CSV file

        bool autoConnect;
        bool pendingConfigChange;

        Bicycle bicycle;

    public:
        int mode;
        QString mediafile, workoutfile;
        // everyone else wants this
        TrainView *trainView;
        QCheckBox   *recordSelector;
        QSharedPointer<QFileSystemWatcher> watcher;
        bool calibrating;
        double wbalr, wbal;
};

class MultiDeviceDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

    public:
        MultiDeviceDialog(Context *, TrainSidebar *);

    public slots:
        void applyClicked();
        void cancelClicked();

    private:

        Context *context;
        TrainSidebar *traintool;
        QComboBox  *bpmSelect,          // heartrate
                   *wattsSelect,        // power
                   *rpmSelect,          // cadence
                   *kphSelect;          // speed

        QPushButton *applyButton, *cancelButton;
};

class DeviceTreeView : public QTreeWidget
{
    Q_OBJECT

    public:
        DeviceTreeView();

    signals:
        void itemMoved(int previous, int actual);

    protected:
        void dropEvent(QDropEvent* event);
};
#endif // _GC_TrainSidebar_h
