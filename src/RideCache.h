/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_RideCache_h
#define _GC_RideCache_h 1

#include "GoldenCheetah.h"
#include "MainWindow.h"
#include "RideFile.h"
#include "RideItem.h"

#include <QVector>
#include <QThread>

#include <QFuture>
#include <QFutureWatcher>
#if QT_VERSION > 0x050000
# include <QtConcurrent>
#else
# include <QtConcurrentRun>
#endif

class Context;
class RideCacheBackgroundRefresh;

class RideCache : public QObject
{
    Q_OBJECT

    public:

        RideCache(Context *context);
        ~RideCache();

        // query the cache
	    QList<QDateTime> getAllDates();

        // is running ?
        bool isRunning() { return future.isRunning(); }

        // the ride list
	    QVector<RideItem*>&rides() { return rides_; } 

        // add/remove a ride to the list
        void addRide(QString name, bool dosignal);
        void removeCurrentRide();

        // restore / dump cache to disk (json)
        void load();
        void save();

        // export metrics in CSV format
        void writeAsCSV(QString filename);

        // the background refresher !
        void refresh();
        double progress() { return progress_; }

    public slots:

        // user updated options/preferences
        void configChanged();

        // background refresh progress update
        void progressing(int);

        // cancel background processing because about to exit
        void cancel();

    protected:

        friend class ::RideCacheBackgroundRefresh;

        Context *context;
        QVector<RideItem*> rides_;
        bool exiting;
	    double progress_; // percent
        unsigned long fingerprint; // zone configuration fingerprint

        QFuture<void> future;
        QFutureWatcher<void> watcher;

};

#endif // _GC_RideCache_h
