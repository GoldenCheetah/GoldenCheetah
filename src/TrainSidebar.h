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

class TrainSidebar : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    public:

        TrainSidebar(Context *context);
        Context *context;

        QStringList listWorkoutFiles(const QDir &) const;

        QList<int> devices(); // convenience function for iterating over active devices

        const QTreeWidgetItem *currentWorkout() { return workout; }
        const QTreeWidgetItem *currentMedia() { return media; }
        const QTreeWidgetItem *workoutItems() { return allWorkouts; }

        int selectedDeviceNumber();

        // set labels when ergfile selected etc
        void setLabels();

        // was realtimewindow,merged into tool
        // update charts/dials and manage controller
        void updateData(RealtimeData &);      // to update telemetry by push devices
        void nextDisplayMode();     // show next display mode
        void setStreamController();     // based upon selected device

        // this
        QTabWidget  *trainTabs;

        // get the panel
        QWidget *getToolbarButtons() { return toolbarButtons; }

        // where to get telemetry from Device.at(x)
        int bpmTelemetry;   // Heartrate
        int wattsTelemetry; // Power (and AltPower)
        int rpmTelemetry;   // Cadence
        int kphTelemetry;   // Speed (and Distance)

    signals:

        void deviceSelected();
        void start();
        void pause();
        void stop();

    private slots:
        void deviceTreeWidgetSelectionChanged();
        void workoutTreeWidgetSelectionChanged();
        void videosyncTreeWidgetSelectionChanged();
        void mediaTreeWidgetSelectionChanged();

        void deviceTreeMenuPopup(const QPoint &);
        void deleteDevice();

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


    public slots:
        void configChanged(qint32);
        void deleteWorkouts(); // deletes selected workouts
        void deleteVideos(); // deletes selected workouts
        void deleteVideoSyncs(); // deletes selected VideoSync

        void Start();       // when start button is pressed
        void Pause();       // when Paude is pressed
        void Stop(int status=0);        // when controller wants to stop

        void Calibrate();   // toggle calibration mode
        void FFwd();        // jump forward when in a workout
        void Rewind();      // jump backwards when in a workout
        void FFwdLap();     // jump forward to next Lap marker
        void Higher();      // set load/gradient higher
        void Lower();       // set load/gradient higher
        void newLap();      // start new Lap!
        void resetLapTimer(); //reset the lap timer

        // Timed actions
        void guiUpdate();           // refreshes the telemetry
        void diskUpdate();          // writes to CSV file
        void loadUpdate();          // sets Load on CT like devices

        // When no config has been setup
        void warnnoConfig();

        // User adjusted intensity
        void adjustIntensity();     // Intensity of workout user adjusted

    protected:

        friend class ::MultiDeviceDialog;

        GcSplitter   *trainSplitter;
        GcSplitterItem *deviceItem,
                       *workoutItem,
                       *videosyncItem,
                       *mediaItem;

        QWidget *toolbarButtons;

        QSqlTableModel *videoModel;
        QSqlTableModel *videosyncModel;
        QSqlTableModel *workoutModel;

        QTreeWidget *deviceTree;
        QTreeView *workoutTree;
        QTreeView *videosyncTree;
        QTreeView *mediaTree;
        QSortFilterProxyModel *sortModel;  // sorting workout list
        QSortFilterProxyModel *vsortModel; // sorting video list
        QSortFilterProxyModel *vssortModel; // sorting videosync list

        QTreeWidgetItem *allWorkouts;
        QTreeWidgetItem *workout;
        QTreeWidgetItem *videosync;
        QTreeWidgetItem *media;

        // Panel buttons
        QPushButton *play;
        QLabel *stress, *intensity;
        QSlider *intensitySlider;
        int lastAppliedIntensity;// remember how we scaled last time

        int FTP; // current FTP / CP
        int WPRIME; // current W'

        QList<DeviceConfiguration> Devices;

        // updated with a RealtimeData object either from
        // update() - from a push device (quarqd ANT+)
        // Device->getRealtimeData() - from a pull device (Computrainer)
        double displayPower, displayHeartRate, displayCadence, displaySpeed;
        double displayLRBalance, displayLTE, displayRTE, displayLPS, displayRPS;
        double displayDistance, displayWorkoutDistance;
        long load;
        double slope;
        int displayLap;            // user increment for Lap
        int displayWorkoutLap;     // which Lap in the workout are we at?

        // for non-zero average calcs
        int pwrcount, cadcount, hrcount, spdcount, lodcount, grdcount; // for NZ average calc
        int status;
        int displaymode;

        QFile *recordFile;      // where we record!
        ErgFile *ergFile;       // workout file
        VideoSyncFile *videosyncFile;       // videosync file

        long total_msecs,
             lap_msecs,
             load_msecs;
        QTime load_period;

        uint session_elapsed_msec, lap_elapsed_msec;
        QTime session_time, lap_time;

        QTimer      *gui_timer,     // refresh the gui
                    *load_timer,    // change the load on the device
                    *disk_timer;    // write to .CSV file

    public:
        int mode;
        // everyone else wants this
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
#endif // _GC_TrainSidebar_h

