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

#include <QString>
#include <QMap>
#include <QVector>

class RideFile;
class RideFileCache;
class RideCache;
class Context;

Q_DECLARE_METATYPE(RideItem*)

class RideItem : public QObject
{

    Q_OBJECT
    G_OBJECT


    protected:

        friend class ::RideCache;

        // ridefile
        RideFile *ride_;
        RideFileCache *fileCache_;

        // precomputed metrics
        QVector<double> metrics_;

        // metadata (used by navigator)
        QMap<QString,QString> metadata_;

        QStringList errors_;
        Context *context; // to notify widgets when date/time changes

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

        bool isdirty;     // ride data has changed and needs saving
        bool isstale;     // metric data is out of date and needs recomputing
        bool isedit;      // is being edited at the moment

        // set from another, e.g. during load of rideDB.json
        void setFrom(RideItem&);

        // access the metric value
        double getForSymbol(QString name);

        // access the metadata
        QString getText(QString name, QString fallback) { return metadata_.value(name, fallback); }

        // get at the first class data
        QString path;
        QString fileName;
        QDateTime dateTime;
        QString present;
        QColor color;
        bool isRun;

        // context the item was updated to
        unsigned long fingerprint; // zones
        unsigned long crc, timestamp; // file content
        int dbversion; // metric version
        double weight; // what weight was used ?

        // access to the cached data !
        RideFile *ride(bool open=true);
        RideFileCache *fileCache();
        QVector<double> &metrics() { return metrics_; }
        QMap<QString, QString> &metadata() { return metadata_; }
        const QStringList errors() { return errors_; }
        double getWeight();

        // ride() will open the ride if it isn't already when open=true
        // if we pass false then it will just return ride_ so we can
        // traverse currently open rides when config changes
        void close();
        bool isOpen();

        // create and destroy
        RideItem();
        RideItem(RideFile *ride, Context *context);
        RideItem(QString path, QString fileName, QDateTime &dateTime, Context *context);
        RideItem(RideFile *ride, QDateTime &dateTime, Context *context);

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
};

#endif // _GC_RideItem_h
