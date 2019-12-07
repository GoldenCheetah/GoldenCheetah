/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
 *           (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_RideItem_h
#define _GC_RideItem_h 1
#include "GoldenCheetah.h"

#include "RideMetric.h"
#include "BodyMeasures.h"
#include "HrvMeasures.h"

#include <QString>
#include <QMap>
#include <QVector>

class RideFile;
class RideFileCache;
class RideCache;
class RideCacheModel;
class IntervalItem;
class IntervalSummaryWindow;
class Context;
class UserData;
class ComparePane;

Q_DECLARE_METATYPE(RideItem*)

class RideItem : public QObject
{

    Q_OBJECT
    G_OBJECT


    protected:

        friend class ::RideCache;
        friend class ::RideCacheModel;
        friend class ::IntervalItem;
        friend class ::IntervalSummaryWindow;
        friend class ::UserData;
        friend class ::ComparePane;

        // ridefile
        RideFile *ride_;
        RideFileCache *fileCache_;

        // precomputed metrics & user overrides
        QVector<double> metrics_;
        QVector<double> count_;

        // std deviation metrics need these to aggregate
        QMap<int, double> stdmean_;
        QMap<int, double> stdvariance_;

        // metadata (used by navigator)
        QMap<QString,QString> metadata_;

        // xdata series definitions
        QMap<QString,QStringList>xdata_;

        // got any intervals
        QList<IntervalItem*> intervals_;
        QStringList errors_;

        // userdata cache
        QMap<QString, QVector<double> > userCache;

        unsigned long metaCRC();

    public slots:
        void modified();
        void reverted();
        void saved();
        void notifyRideDataChanged();
        void notifyRideMetadataChanged();

    signals:
        void rideDataChanged();
        void rideMetadataChanged();

    public:

        Context *context; // to notify widgets when date/time changes
        bool isdirty;     // ride data has changed and needs saving
        bool isstale;     // metric data is out of date and needs recomputing
        bool isedit;      // is being edited at the moment
        bool skipsave;    // on exit we don't save the state to force rebuild at startup

        // set from another, e.g. during load of rideDB.json
        void setFrom(RideItem&, bool temp=false);

        // record of any overrides, used by formula "isset" function
        QStringList overrides_;

        // set metric values e.g. when working with intervals
        void setFrom(QHash<QString, RideMetricPtr>);

        // add interval e.g. during load of rideDB.json
        void addInterval(IntervalItem interval);
        void clearIntervals() { intervals_.clear(); } // does NOT delete them

        // new Interval created and needs to be reflected in ridefile
        IntervalItem * newInterval(QString name, double start, double stop, double startKM, double stopKM, QColor color, bool test);

        // access the metric value
        double getForSymbol(QString name, bool useMetricUnits=true);
        double getCountForSymbol(QString name);

        // access the stdmean and stdvariance value
        double getStdMeanForSymbol(QString name);
        double getStdVarianceForSymbol(QString name);

        // as a well formatted string
        QString getStringForSymbol(QString name, bool useMetricUnits=true);

        // access the metadata
        QString getText(QString name, QString fallback) { return metadata_.value(name, fallback); }
        bool hasText(QString name) { return metadata_.contains(name); }

        // get at the first class data
        QString path;
        QString fileName;
        QDateTime dateTime;
        QString present;
        QColor color;
        bool planned;
        bool isRun,isSwim;
        bool samples; // has samples data

        // which range to use?
        int zoneRange, hrZoneRange, paceZoneRange;

        // context the item was updated to
        unsigned long fingerprint; // zones
        unsigned long metacrc, crc, timestamp; // file content
        int dbversion; // metric version
        int udbversion; // user metric version
        double weight; // what weight was used ?

        // access to the cached data !
        BodyMeasure weightData;
        RideFile *ride(bool open=true);
        RideFileCache *fileCache();
        QVector<double> &metrics() { return metrics_; }
        QVector<double> &counts() { return count_; }
        QMap <int, double>&stdmeans() { return stdmean_; }
        QMap <int, double>&stdvariances() { return stdvariance_; }
        const QStringList errors() { return errors_; }
        double getWeight(int type=0);
        double getHrvMeasure(int type=HrvMeasure::RMSSD);
        unsigned short getHrvFingerprint();

        // when retrieving interval lists we can provide criteria too
        QList<IntervalItem*> &intervals()  { return intervals_; }
        QList<IntervalItem*> intervalsSelected() const;
        QList<IntervalItem*> intervals(RideFileInterval::intervaltype) const;
        QList<IntervalItem*> intervalsSelected(RideFileInterval::intervaltype) const;
        bool removeInterval(IntervalItem *x);
        void moveInterval(int from, int to);

        // metadata
        QMap<QString, QString> &metadata() { return metadata_; }

        // xdata definitions maps QString<xdata>, QStringList<xdataseries>
        QMap<QString,QStringList> &xdata() { return xdata_; }

        // hunt down the xdata series by matching, returns true or false on match
        // and will set mname and mseries to the value that matched
        bool xdataMatch(QString name, QString series, QString &mname, QString &mseries);

        // ride() will open the ride if it isn't already when open=true
        // if we pass false then it will just return ride_ so we can
        // traverse currently open rides when config changes
        void close();
        bool isOpen();

        // create and destroy
        RideItem();
        RideItem(RideFile *ride, Context *context);
        RideItem(QString path, QString fileName, QDateTime &dateTime, Context *context, bool planned);
        RideItem(RideFile *ride, QDateTime &dateTime, Context *context);

        ~RideItem();

        // state
        void setDirty(bool);
        bool isDirty() { return isdirty; }
        bool checkStale(); // check if we need to refresh
        bool isStale() { return isstale; }

        // refresh when stale
        void refresh();

        // get/set
        void setRide(RideFile *);
        void setFileName(QString, QString);
        void setStartTime(QDateTime);

        // sorting
        bool operator<(const RideItem &right) const { return dateTime < right.dateTime; }
        bool operator>(const RideItem &right) const { return dateTime < right.dateTime; }

    private:
        void updateIntervals();
};

#endif // _GC_RideItem_h
