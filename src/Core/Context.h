/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_Context_h
#define _GC_Context_h 1

#include "TimeUtils.h" // for class DateRange
#include "RealtimeData.h" // for class RealtimeData
#include "SpecialFields.h" // for class RealtimeData
#include "CompareInterval.h" // what intervals are being compared?
#include "CompareDateRange.h" // what intervals are being compared?
#include "RideFile.h"

#ifdef GC_HAS_CLOUD_DB
#include "CloudDBChart.h"
#include "CloudDBUserMetric.h"
#endif

// when config changes we need to notify widgets what changed
// but there is so much config these days we need to be a little
// more specific, not too specific since we would have a million
// variations, but enough to let widgets ignore stuff and if they
// need to react to very specific changes they should manage that
// themselves.
//
// Context::notifyConfigChanged(x) and its signal configChanged(qint32)
// are used to pass a state value around that contains the different
// values or-ed together -- since it is possible to make changes
// to different config and save it all at once.

#define CONFIG_ATHLETE           0x1        // includes default weight, height etc
#define CONFIG_ZONES             0x2        // CP, FTP, useCPforFTP, zone config etc
#define CONFIG_GENERAL           0x4        // includes default weight, w'bal formula, directories
#define CONFIG_USERMETRICS       0x8        // user defined metrics
#define CONFIG_APPEARANCE        0x10
#define CONFIG_FIELDS            0x20       // metadata fields
#define CONFIG_NOTECOLOR         0x40       // ride coloring from "notes" fields
#define CONFIG_METRICS           0x100      // metrics to show for intervals, bests and summary
#define CONFIG_DEVICES           0x200
#define CONFIG_SEASONS           0x400      // includes seasons, events and LTS/STS seeded values
#define CONFIG_UNITS             0x800      // metric / imperial
#define CONFIG_PMC               0x1000     // PMC constants
#define CONFIG_WBAL              0x2000     // which w'bal formula to use ?
#define CONFIG_WORKOUTS          0x4000     // workout location / files
#define CONFIG_DISCOVERY         0x8000     // interval discovery
#define CONFIG_WORKOUTTAGMANAGER 0x10000    // workout tags

class RideItem;
class IntervalItem;
class ErgFile;
class VideoSyncFile;

class Context;
class Athlete;
class MainWindow;
class AthleteTab;
class NavigationModel;
class RideMetadata;
class ColorEngine;
class ModelFilter;
class QWebEngineProfile;


class GlobalContext : public QObject
{
    Q_OBJECT

    public:

        GlobalContext();
        static GlobalContext *context();
        void notifyConfigChanged(qint32);

        // metadata etc
        RideMetadata *rideMetadata;
        SpecialFields specialFields;
        ColorEngine *colorEngine;

        // metric units
        bool useMetricUnits;

    public slots:
        void readConfig(qint32);
        void userMetricsConfigChanged();

    signals:
        void configChanged(qint32); // for global widgets that aren't athlete specific

};

class RideNavigator;
class Context : public QObject
{
    Q_OBJECT;

    public:
        Context(MainWindow *mainWindow);
        ~Context();

        // check if valid (might be deleted)
        static bool isValid(Context *);

        // mainwindow state
        NavigationModel *nav;
        int viewIndex;
        bool showSidebar, showLowbar, showToolbar, showTabbar;
        int style;
        QString searchText;
        bool scopehighlighted;

        // ride item
        RideItem *rideItem() const { return ride; }
        const RideItem *currentRideItem() { return ride; }
        DateRange currentDateRange() { return dr_; }

        // current selections and widgetry
        MainWindow * const mainWindow;
        RideNavigator *rideNavigator;
        AthleteTab *tab;
        Athlete *athlete;
        RideItem *ride;  // the currently selected ride
        DateRange dr_;
        ErgFile *workout; // the currently selected workout file
        VideoSyncFile *videosync; // the currently selected videosync file
        QString videoFilename;
        long now; // point in time during train session

        // search filter
        bool isfiltered;
        bool ishomefiltered;
        QStringList filters; // searchBox filters
        QStringList homeFilters; // homewindow sidebar filters

        // train mode state
        bool isRunning;
        bool isPaused;

        // comparing things
        bool isCompareIntervals;
        QList<CompareInterval> compareIntervals;

        bool isCompareDateRanges;
        QList<CompareDateRange> compareDateRanges;

#ifdef GC_HAS_CLOUD_DB
        // CloudDB - common data
        CloudDBChartListDialog *cdbChartListDialog;
        CloudDBUserMetricListDialog *cdbUserMetricListDialog;
#endif

        // WebEngineProfile for this user
        QWebEngineProfile* webEngineProfile;

    public slots:

        // *********************************************
        // APPLICATION EVENTS
        // *********************************************
        void notifyConfigChanged(qint32); // Global and athlete specific changes communicated via this signal

        // athlete load/close
        void notifyLoadProgress(QString folder, double progress) { emit loadProgress(folder,progress); }
        void notifyLoadCompleted(QString folder, Context *context) { emit loadCompleted(folder,context); } // Athlete loaded
        void notifyAthleteClose(QString folder, Context *context) { emit athleteClose(folder,context); }
        void notifyLoadDone(QString folder, Context *context) { emit loadDone(folder, context); } // MainWindow finished

        // preset charts
        void notifyPresetsChanged() { emit presetsChanged(); }
        void notifyPresetSelected(int n) { emit presetSelected(n); }

        // filters
        void setHomeFilter(QStringList&f) { homeFilters=f; ishomefiltered=true; emit homeFilterChanged(); }
        void clearHomeFilter() { homeFilters.clear(); ishomefiltered=false; emit homeFilterChanged(); }

        void setFilter(QStringList&f) { filters=f; isfiltered=true; emit filterChanged(); }
        void clearFilter() { filters.clear(); isfiltered=false; emit filterChanged(); }

        void setWorkoutFilters(QList<ModelFilter*> &f) { emit workoutFiltersChanged(f); }
        void clearWorkoutFilters() { emit workoutFiltersRemoved(); }

        // user metrics - cascade
        void notifyUserMetricsChanged() { emit userMetricsChanged(); }

        // view changed
        void setIndex(int i) { viewIndex = i; emit viewChanged(i); }

        // realtime signals
        void notifyTelemetryUpdate(const RealtimeData &rtData) { telemetryUpdate(rtData); }
        void notifyErgFileSelected(ErgFile *x) { workout=x; ergFileSelected(x); ergFileSelected((ErgFileBase*)(x));}
        void notifyVideoSyncFileSelected(VideoSyncFile *x) { videosync=x; videoSyncFileSelected(x); }
        ErgFile *currentErgFile() { return workout; }
        VideoSyncFile *currentVideoSyncFile() { return videosync; }
        void notifyMediaSelected( QString x) { videoFilename = x; mediaSelected(x); }
        void notifySelectVideo(QString x) { selectMedia(x); }
        void notifySelectWorkout(QString x) { selectWorkout(x); }
        void notifySelectVideoSync(QString x) { selectVideoSync(x); }
        void notifySetNow(long x) { now = x; setNow(x); }
        long getNow() { return now; }
        void notifyNewLap() { emit newLap(); }
        void notifyStart() { emit start(); }
        void notifyUnPause() { emit unpause(); }
        void notifyPause() { emit pause(); }
        void notifyStop() { emit stop(); }
        void notifySeek(long x) { emit seek(x); }
        void notifyIntensityChanged(int intensity) { emit intensityChanged(intensity); };

        // date range selection
        void notifyDateRangeChanged(DateRange x) { dr_=x; emit dateRangeSelected(x); }
        void notifyWorkoutsChanged() { emit workoutsChanged(); }
        void notifyVideoSyncChanged() { emit VideoSyncChanged(); }

        void notifyRideSelected(RideItem*x) { ride=x; rideSelected(x); }
        void notifyRideAdded(RideItem *x) { ride=x; rideAdded(x); }
        void notifyRideDeleted(RideItem *x) { ride=x; rideDeleted(x); }
        void notifyRideChanged(RideItem *x) { rideChanged(x); }
        void notifyRideSaved(RideItem *x) { rideSaved(x); }

        void notifyIntervalZoom(IntervalItem*x) { emit intervalZoom(x); }
        void notifyZoomOut() { emit zoomOut(); }
        void notifyIntervalSelected() { intervalSelected(); }
        void notifyIntervalItemSelectionChanged(IntervalItem*x) { intervalItemSelectionChanged(x); }
        void notifyIntervalsUpdate(RideItem *x) { emit intervalsUpdate(x); }
        void notifyIntervalsChanged() { emit intervalsChanged(); }
        void notifyIntervalHover(IntervalItem *x) { emit intervalHover(x); }
        void notifyRideClean() { rideClean(ride); }
        void notifyRideDirty() { rideDirty(ride); }
        void notifyMetadataFlush() { metadataFlush(); }

        void notifyAutoDownloadStart() { emit autoDownloadStart(); }
        void notifyAutoDownloadEnd() { emit autoDownloadEnd(); }
        void notifyAutoDownloadProgress(QString s, double x, int i, int n) { emit autoDownloadProgress(s, x, i, n); }

        void notifyRefreshStart() { emit refreshStart(); }
        void notifyRefreshEnd() { emit refreshEnd(); }
        void notifyRefreshUpdate(QDate date) { emit refreshUpdate(date); }

        void notifyRMessage(QString x) { emit rMessage(x); }
        void notifyCompareIntervals(bool state);
        void notifyCompareIntervalsChanged();

        void notifyCompareDateRanges(bool state);
        void notifyCompareDateRangesChanged();

        void notifySteerScroll(int scrollAmount) { emit steerScroll(scrollAmount); }
        void notifyEstimatesRefreshed() { emit estimatesRefreshed(); }

    protected:

        // we need to act since the user metric config changed
        // and we need to notify other contexts !
        void userMetricsConfigChanged();

    signals:

        // loading an athlete
        void loadProgress(QString,double);
        void loadCompleted(QString, Context*);
        void loadDone(QString, Context*);
        void athleteClose(QString, Context*);

        // global filter changed
        void filterChanged();
        void homeFilterChanged();

        // workout filters
        void workoutFiltersChanged(QList<ModelFilter*>&);
        void workoutFiltersRemoved();

        void workoutsChanged(); // added or deleted a workout in train view
        void VideoSyncChanged(); // added or deleted a workout in train view
        void presetsChanged();
        void presetSelected(int);

        // user metrics
        void configChanged(qint32);
        void userMetricsChanged();

        // view changed
        void viewChanged(int);

        // refreshing stats
        void refreshStart();
        void refreshEnd();
        void refreshUpdate(QDate);
        void estimatesRefreshed();

        // cloud download
        void autoDownloadStart();
        void autoDownloadEnd();
        void autoDownloadProgress(QString, double, int, int);

        void dateRangeSelected(DateRange);
        void rideSelected(RideItem*);

        // we added/deleted/changed an item
        void rideAdded(RideItem *);
        void rideDeleted(RideItem *);
        void rideChanged(RideItem *);
        void rideSaved(RideItem*);

        void intervalSelected();
        void intervalsChanged();
        void intervalsUpdate(RideItem*);
        void intervalHover(IntervalItem*);
        void intervalZoom(IntervalItem*);
        void intervalItemSelectionChanged(IntervalItem*);
        void zoomOut();
        void metadataFlush();
        void rideDirty(RideItem*);
        void rideClean(RideItem*);

        // realtime
        void telemetryUpdate(RealtimeData rtData);
        void ergFileSelected(ErgFile *);
        void ergFileSelected(ErgFileBase *);
        void videoSyncFileSelected(VideoSyncFile *);
        void mediaSelected(QString);
        void selectWorkout(QString); // ask traintool to select this
        void selectMedia(QString); // ask traintool to select this
        void selectVideoSync(QString); // ask traintool to select this
        void setNow(long);
        void seek(long);
        void newLap();
        void start();
        void unpause();
        void pause();
        void stop();
        void intensityChanged(int intensity);

        // R messages
        void rMessage(QString);

        // comparing things
        void compareIntervalsStateChanged(bool);
        void compareIntervalsChanged();
        void compareDateRangesStateChanged(bool);
        void compareDateRangesChanged();

        // Trainer controls
        void steerScroll(int);
};
#endif // _GC_Context_h
