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

#ifndef _GC_RideCacheModel_h
#define _GC_RideCacheModel_h 1

#include "GoldenCheetah.h"

// data definitions!
#include "RideMetadata.h"
#include "RideCache.h"
#include "RideItem.h"
#include "RideMetric.h"

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QVariant>

class Context;

class RideCacheModel : public QAbstractTableModel
{
    Q_OBJECT

    public:
        RideCacheModel(Context *, RideCache *);

        // must reimplement these
        int rowCount(const QModelIndex &parent = QModelIndex()) const; 
        int columnCount(const QModelIndex &parent = QModelIndex()) const;
        Qt::ItemFlags flags(const QModelIndex &index) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        bool setHeaderData (int section, Qt::Orientation orientation, const QVariant &value, int role =Qt::EditRole);
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    public slots:

         // when updating metadata config
        void configChanged(qint32);

        // catch ridecache refreshes
        void refreshUpdate(QDate); 
        void refreshStart();
        void refreshEnd();

        // and updates to ride items
        void itemChanged(RideItem *item);
        void itemAdded(RideItem *item);

        // model reset on add (if needed)
        void beginReset();
        void endReset();

        // start / end remove
        void startRemove(int);
        void endRemove(int);

    private:
        Context *context;
        RideCache *rideCache;
        RideMetricFactory *factory;

        int columns_; // column count, based upon metric + meta config
        QStringList headings_;

        // the fields as defined
        QList<FieldDefinition> metadata;
};

#endif
