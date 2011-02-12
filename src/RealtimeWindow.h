/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_RealtimeWindow_h
#define _GC_RealtimeWindow_h 1

#include <QtGui>
#include <QTimer>
#include "MainWindow.h"
#include "DeviceConfiguration.h"
#include "DeviceTypes.h"
#include "ErgFile.h"
#include "ErgFilePlot.h"

class TrainTool;
class TrainTabs;

// Status settings
#define RT_MODE_ERGO    0x0001        // load generation modes
#define RT_MODE_SPIN    0x0002        // spinscan like modes
#define RT_RUNNING      0x0100        // is running now
#define RT_PAUSED       0x0200        // is paused
#define RT_RECORDING    0x0400        // is recording to disk
#define RT_WORKOUT      0x0800        // is running a workout
#define RT_STREAMING    0x1000        // is streaming to a remote peer

#define REFRESHRATE    200 // screen refresh in milliseconds
#define STREAMRATE     200 // rate at which we stream updates to remote peer
#define SAMPLERATE     1000 // disk update in milliseconds
#define LOADRATE       1000 // rate at which load is adjusted


class RealtimeController;
class RealtimePlot;
class RealtimeData;

class RealtimeWindow : public QWidget
{
    Q_OBJECT

    public:

        RealtimeController *deviceController;   // read from
        RealtimeController *streamController;   // send out to

        RealtimeWindow(MainWindow *, TrainTool *, const QDir &);

        void updateData(RealtimeData &);      // to update telemetry by push devices
        void newLap();                      // start new Lap!
        void nextDisplayMode();     // show next display mode
        void setDeviceController();     // based upon selected device
        void setStreamController();     // based upon selected device

    public slots:

        void Start();       // when start button is pressed
        void Pause();       // when Paude is pressed
        void Stop(int status=0);        // when stop button is pressed

        void FFwd();        // jump forward when in a workout
        void Rewind();      // jump backwards when in a workout
        void FFwdLap();     // jump forward to next Lap marker
        void Higher();      // set load/gradient higher
        void Lower();       // set load/gradient higher

        void SelectDevice(int); // when combobox chooses device
        void SelectRecord();    // when checkbox chooses record mode
        void SelectStream(int);    // when remote server to stream to is selected
        void SelectWorkout();    // to select a Workout to use

        // Timed actions
        void guiUpdate();           // refreshes the telemetry
        void diskUpdate();          // writes to CSV file
        void streamUpdate();        // writes to remote Peer
        void loadUpdate();          // sets Load on CT like devices

        // Handle config updates
        void configUpdate();            // called when config changes

        // When no config has been setup
        void warnnoConfig();

    protected:


        // passed from MainWindow
        QDir home;
        MainWindow *main;
        TrainTool *trainTool;

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

        // for non-zero average calcs
        int pwrcount, cadcount, hrcount, spdcount, lodcount, grdcount; // for NZ average calc
        int status;
        int displaymode;

        QFile *recordFile;      // where we record!
        ErgFile *ergFile;       // workout file

        long total_msecs,
             lap_msecs,
             load_msecs;
        QTime load_period;

        uint session_elapsed_msec, lap_elapsed_msec;
        QTime session_time, lap_time;

        // GUI WIDGETS
        // layout
        RealtimePlot *rtPlot;
        ErgFilePlot  *ergPlot;

        QVBoxLayout *main_layout;

        // labels
        QLabel *powerLabel,
                    *heartrateLabel,
                    *speedLabel,
                    *cadenceLabel,
                    *loadLabel,
                    *lapLabel,
                    *laptimeLabel,
                    *timeLabel,
                    *distanceLabel;

        double avgPower, avgHeartRate, avgSpeed, avgCadence, avgLoad, avgGradient;
        QLabel *avgpowerLabel,
                    *avgheartrateLabel,
                    *avgspeedLabel,
                    *avgcadenceLabel,
                    *avgloadLabel;

        QHBoxLayout *button_layout,
                    *option_layout;
        QGridLayout *timer_layout;
        QVBoxLayout *controls_layout;
        QCheckBox   *recordSelector;
        QComboBox   *deviceSelector,
                    *streamSelector;
        QPushButton *startButton,
                    *pauseButton,
                    *stopButton;

        QGridLayout *gridLayout;

        // the LCDs
        QLCDNumber *powerLCD,
                    *heartrateLCD,
                    *speedLCD,
                    *cadenceLCD,
                    *loadLCD,
                    *lapLCD,
                    *laptimeLCD,
                    *timeLCD,
                    *distanceLCD;

        QLCDNumber *avgpowerLCD,
                    *avgheartrateLCD,
                    *avgspeedLCD,
                    *avgcadenceLCD,
                    *avgloadLCD;

        QTimer      *gui_timer,     // refresh the gui
                    *stream_timer,  // send telemetry to server
                    *load_timer,    // change the load on the device
                    *disk_timer;    // write to .CSV file

};

#endif // _GC_RealtimeWindow_h

