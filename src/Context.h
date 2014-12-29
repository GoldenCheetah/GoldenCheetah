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
#define CONFIG_ZONES             0x2 
#define CONFIG_GENERAL           0x4        // includes default weight, w'bal formula, directories
#define CONFIG_PASSWORDS         0x8
#define CONFIG_APPEARANCE        0x10
#define CONFIG_FIELDS            0x20       // metadata fields
#define CONFIG_NOTECOLOR         0x40       // ride coloring from "notes" fields
#define CONFIG_PROCESSING        0x80       // fix tools
#define CONFIG_METRICS           0x100
#define CONFIG_DEVICES           0x200
#define CONFIG_SEASONS           0x400      // includes events and PMC constants
#define CONFIG_UNITS             0x800      // metric / imperial

class RideItem;
class IntervalItem;
class ErgFile;

class Context;
class Athlete;
class MainWindow;
class Tab;

class Context : public QObject
{
    Q_OBJECT;

    public:
        Context(MainWindow *mainWindow);

        // mainwindow state
        int viewIndex;
        bool showSidebar, showLowbar, showToolbar, showTabbar;
        int style;
        QString searchText;
        bool scopehighlighted;

        // ride item
        RideItem *rideItem() const { return ride; }
        const RideFile *currentRide();
        const RideItem *currentRideItem() { return ride; }
        DateRange currentDateRange() { return dr_; }

        // current selections
        MainWindow * const mainWindow;
        Tab *tab;
        Athlete *athlete;
        RideItem *ride;  // the currently selected ride
        DateRange dr_;
        ErgFile *workout; // the currently selected workout file
        long now; // point in time during train session
        SpecialFields specialFields;

        // search filter
        bool isfiltered;
        bool ishomefiltered;
        QStringList filters; // searchBox filters
        QStringList homeFilters; // homewindow sidebar filters

        // comparing things
        bool isCompareIntervals;
        QList<CompareInterval> compareIntervals;

        bool isCompareDateRanges;
        QList<CompareDateRange> compareDateRanges;

    public slots:

        // *********************************************
        // APPLICATION EVENTS
        // *********************************************
        void notifyConfigChanged(qint32); // used by ConfigDialog to notify Context *
                                            // when config has changed - and to get a
                                            // signal emitted to notify its children

        // preset charts
        void notifyPresetsChanged() { emit presetsChanged(); }
        void notifyPresetSelected(int n) { emit presetSelected(n); }

        // filters
        void setHomeFilter(QStringList&f) { homeFilters=f; ishomefiltered=true; emit homeFilterChanged(); }
        void clearHomeFilter() { homeFilters.clear(); ishomefiltered=false; emit homeFilterChanged(); }

        void setFilter(QStringList&f) { filters=f; isfiltered=true; emit filterChanged(); }
        void clearFilter() { filters.clear(); isfiltered=false; emit filterChanged(); }

        // realtime signals
        void notifyTelemetryUpdate(const RealtimeData &rtData) { telemetryUpdate(rtData); }
        void notifyErgFileSelected(ErgFile *x) { workout=x; ergFileSelected(x); }
        ErgFile *currentErgFile() { return workout; }
        void notifyMediaSelected( QString x) { mediaSelected(x); }
        void notifySelectVideo(QString x) { selectMedia(x); }
        void notifySelectWorkout(QString x) { selectWorkout(x); }
        void notifySetNow(long x) { now = x; setNow(x); }
        long getNow() { return now; }
        void notifyNewLap() { emit newLap(); }
        void notifyStart() { emit start(); }
        void notifyUnPause() { emit unpause(); }
        void notifyPause() { emit pause(); }
        void notifyStop() { emit stop(); }
        void notifySeek(long x) { emit seek(x); }

        void notifyWorkoutsChanged() { emit workoutsChanged(); }

        void notifyRideSelected(RideItem*x) { ride=x; rideSelected(x); }
        void notifyRideAdded(RideItem *x) { ride=x; rideAdded(x); }
        void notifyRideDeleted(RideItem *x) { ride=x; rideDeleted(x); }
        void notifyRideChanged(RideItem *x) { rideChanged(x); }

        void notifyIntervalZoom(IntervalItem*x) { emit intervalZoom(x); }
        void notifyZoomOut() { emit zoomOut(); }
        void notifyIntervalSelected() { intervalSelected(); }
        void notifyIntervalsChanged() { emit intervalsChanged(); }
        void notifyIntervalHover(RideFileInterval x) { emit intervalHover(x); }
        void notifyRideClean() { rideClean(ride); }
        void notifyRideDirty() { rideDirty(ride); }
        void notifyMetadataFlush() { metadataFlush(); }

        void notifyRefreshStart() { emit refreshStart(); }
        void notifyRefreshEnd() { emit refreshEnd(); }
        void notifyRefreshUpdate(QDate date) { emit refreshUpdate(date); }

        void notifyCompareIntervals(bool state);
        void notifyCompareIntervalsChanged();

        void notifyCompareDateRanges(bool state);
        void notifyCompareDateRangesChanged();

    signals:

        // global filter changed
        void filterChanged();
        void homeFilterChanged();

        void configChanged(qint32);

        void workoutsChanged(); // added or deleted a workout in train view
        void presetsChanged();
        void presetSelected(int);


        // refreshing stats
        void refreshStart();
        void refreshEnd();
        void refreshUpdate(QDate);

        void rideSelected(RideItem*);

        // we added/deleted/changed an item
        void rideAdded(RideItem *);
        void rideDeleted(RideItem *);
        void rideChanged(RideItem *);

        void intervalSelected();
        void intervalsChanged();
        void intervalHover(RideFileInterval);
        void intervalZoom(IntervalItem*);
        void zoomOut();
        void metadataFlush();
        void rideDirty(RideItem*);
        void rideClean(RideItem*);

        // realtime
        void telemetryUpdate(RealtimeData rtData);
        void ergFileSelected(ErgFile *);
        void mediaSelected(QString);
        void selectWorkout(QString); // ask traintool to select this
        void selectMedia(QString); // ask traintool to select this
        void setNow(long);
        void seek(long);
        void newLap();
        void start();
        void unpause();
        void pause();
        void stop();

        // comparing things
        void compareIntervalsStateChanged(bool);
        void compareIntervalsChanged();
        void compareDateRangesStateChanged(bool);
        void compareDateRangesChanged();
};
#endif // _GC_Context_h
