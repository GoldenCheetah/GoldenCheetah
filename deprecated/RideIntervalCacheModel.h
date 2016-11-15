/*
 * Copyright (c) 2015 Damien Grauser (Damien.Grauser@gmail.com)
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

#ifndef _GC_RideIntervalCacheModel_h
#define _GC_RideIntervalCacheModel_h 1

#include "GoldenCheetah.h"

// data definitions!
#include "RideMetadata.h"
#include "IntervalItem.h"
#include "RideItem.h"
#include "RideMetric.h"

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QVariant>

class Context;

class RideIntervalCacheModel : public QAbstractTableModel
{
    Q_OBJECT

    public:
        RideIntervalCacheModel(Context *, RideCache *);

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

        // catch intervalCache refreshes
        void refreshUpdate(QDate); 
        void refreshStart();
        void refreshEnd();

        // and updates to interval items
        void itemChanged(IntervalItem *item);
        void itemAdded(IntervalItem *item);

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
