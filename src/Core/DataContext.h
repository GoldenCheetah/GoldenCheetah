#ifndef _GC_DataContext_h
#define _GC_DataContext_h 1

#include <QObject>
#include <QList>
#include <QString>
#include <QDate>
#include "ContextConstants.h"
#include "TimeUtils.h"
#include "CompareInterval.h"
#include "CompareDateRange.h"

class RideItem;
class IntervalItem;
class ErgFile;
class ErgFileBase;
class VideoSyncFile;
class Athlete;
class Season;
class RealtimeData;

class DataContext : public QObject
{
    Q_OBJECT

    public:
        DataContext(QObject *parent = nullptr);
        virtual ~DataContext();

        // data accessors
        RideItem *rideItem() const { return ride; }
        const RideItem *currentRideItem() { return ride; }
        DateRange currentDateRange() { return dr_; }
        Season const *currentSeason() { return season; }
        ErgFile *currentErgFile() { return workout; }
        VideoSyncFile *currentVideoSyncFile() { return videosync; }
        long getNow() { return now; }


        // Data Members
        Athlete *athlete;
        RideItem *ride;  // the currently selected ride
        DateRange dr_;
        Season const *season = nullptr;
        ErgFile *workout; // the currently selected workout file
        VideoSyncFile *videosync; // the currently selected videosync file
        QString videoFilename;
        long now; // point in time during train session

        // train mode state
        bool isRunning;
        bool isPaused;

        // comparing things
        bool isCompareIntervals;
        QList<CompareInterval> compareIntervals;

        bool isCompareDateRanges;
        QList<CompareDateRange> compareDateRanges;

    public slots:
        // Data-related slots
        void notifyRideSelected(RideItem*x) { ride=x; rideSelected(x); }
        void notifyRideAdded(RideItem *x) { ride=x; rideAdded(x); }
        void notifyRideDeleted(RideItem *x) { ride=x; rideDeleted(x); }
        void notifyRideChanged(RideItem *x) { rideChanged(x); }
        void notifyRideSaved(RideItem *x) { rideSaved(x); }
        
        void notifyDateRangeChanged(DateRange x) { dr_=x; emit dateRangeSelected(x); }
        void notifySeasonChanged(Season const *season) {
            bool changed = this->season != season;
            this->season = season;
            emit seasonSelected(season, changed);
        }

        void notifyIntervalsUpdate(RideItem *x) { emit intervalsUpdate(x); }
        void notifyIntervalsChanged() { emit intervalsChanged(); }
        void notifyRideClean() { rideClean(ride); }
        void notifyRideDirty() { rideDirty(ride); }
        void notifyMetadataFlush() { metadataFlush(); }

        // Realtime/Train slots
        void notifyTelemetryUpdate(const RealtimeData &rtData);
        void notifyErgFileSelected(ErgFile *x);
        void notifyVideoSyncFileSelected(VideoSyncFile *x) { videosync=x; videoSyncFileSelected(x); }
        void notifyMediaSelected( QString x) { videoFilename = x; mediaSelected(x); }
        void notifySelectVideo(QString x) { selectMedia(x); }
        void notifySelectWorkout(QString x) { selectWorkout(x); }
        void notifySelectWorkout(int idx ) { selectWorkout(idx); }
        void notifySelectVideoSync(QString x) { selectVideoSync(x); }
        void notifySetNow(long x) { now = x; setNow(x); }
        void notifyNewLap() { emit newLap(); }
        void notifyStart() { emit start(); }
        void notifyUnPause() { emit unpause(); }
        void notifyPause() { emit pause(); }
        void notifyStop() { emit stop(); }
        void notifySeek(long x) { emit seek(x); }
        void notifyIntensityChanged(int intensity) { emit intensityChanged(intensity); };

        void notifyWorkoutsChanged() { emit workoutsChanged(); }

        void notifyCompareIntervals(bool state);
        void notifyCompareIntervalsChanged();
        void notifyCompareDateRanges(bool state);
        void notifyCompareDateRangesChanged();

    signals:
        // specific data signals
        void rideSelected(RideItem*);
        void rideAdded(RideItem *);
        void rideDeleted(RideItem *);
        void rideChanged(RideItem *);
        void rideSaved(RideItem*);
        
        void dateRangeSelected(DateRange);
        void seasonSelected(Season const *season, bool changed);

        void workoutsChanged();
        void intervalsChanged();
        void intervalsUpdate(RideItem*);
        void rideDirty(RideItem*);
        void rideClean(RideItem*);
        void metadataFlush();

        // Realtime signals
        void telemetryUpdate(const RealtimeData &rtData);
        void ergFileSelected(ErgFile *);
        void ergFileSelected(ErgFileBase *);
        void videoSyncFileSelected(VideoSyncFile *);
        void mediaSelected(QString);
        void selectWorkout(QString);
        void selectWorkout(int idx);
        void selectMedia(QString);
        void selectVideoSync(QString);
        void setNow(long);
        void seek(long);
        void newLap();
        void start();
        void unpause();
        void pause();
        void stop();
        void intensityChanged(int intensity);

        void compareIntervalsStateChanged(bool);
        void compareIntervalsChanged();
        void compareDateRangesStateChanged(bool);
        void compareDateRangesChanged();
};

#endif // _GC_DataContext_h
