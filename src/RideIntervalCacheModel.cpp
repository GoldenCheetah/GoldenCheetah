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

#include "RideIntervalCacheModel.h"

#include "GoldenCheetah.h"
#include "Athlete.h"
#include "Context.h"
#include "RideCache.h"

RideIntervalCacheModel::RideIntervalCacheModel(Context *context, RideCache *cache) : QAbstractTableModel(cache), context(context), rideCache(cache)
{
    factory = &RideMetricFactory::instance();
    configChanged(CONFIG_FIELDS | CONFIG_NOTECOLOR);

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(refreshStart()), this, SLOT(refreshStart()));
    connect(context, SIGNAL(refreshEnd()), this, SLOT(refreshEnd()));
    connect(context, SIGNAL(refreshUpdate(QDate)), this, SLOT(refreshUpdate(QDate)));

    //connect(context, SIGNAL(intervalAdded(IntervalItem*)), this, SLOT(itemAdded(IntervalItem*)));
    //connect(RideIntervalCache, SIGNAL(itemChanged(IntervalItem*)), this, SLOT(itemChanged(IntervalItem*)));
}

// must reimplement these
int 
RideIntervalCacheModel::rowCount(const QModelIndex &parent) const
{
    return 3;
    //return parent.isValid() ? 0 : RideIntervalCache->count();
}


int 
RideIntervalCacheModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : columns_;
}

Qt::ItemFlags
RideIntervalCacheModel::flags(const QModelIndex &index) const
{
     if (!index.isValid()) return Qt::ItemIsEnabled;
     return QAbstractTableModel::flags(index);
}

QVariant 
RideIntervalCacheModel::data(const QModelIndex &index, int role) const
{
    qDebug() << "data" << index.column();

    Q_UNUSED(role);

    return "test";
    /*if (!index.isValid() || index.row() < 0 || index.row() >= RideCache->count() ||
        index.column() < 0 || index.column() >= columns_) return QVariant();

    IntervalItem *item = RideCache->intervals()[index.row()];

    switch (index.column()) {
        case 0 : return item->ride->getTag("Filename","");
        case 1 : return item->ride->startTime();
        case 2 : return item->start;
        case 3 : return item->stop;
        //case 4 : return item->color.name();
        //case 5 : return item->isRun;

        case 4 : return item->color;
        case 5 : return 5;
        case 6 : return item->groupName;
        case 7 : return item->stop-item->start; // duration

        default:
        {
            // from here we're either a metric or meta
            // lets work that out ...
            if (index.column()-7 < factory->metricCount()) {

                // is a metric
                int i=index.column()-7;

                // unpack metric value into ridemetric and use it to get a stringified
                // version using the right metric/imperial conversion
                RideMetric *m = const_cast<RideMetric*>(factory->rideMetric(factory->metricName(i)));
                m->setValue(RideIntervalCache->intervals().at(index.row())->rideSegment()->metrics_[m->index()]);
                return m->toString(context->athlete->useMetricUnits);

            } else {

                // is a metadata
                //int i = index.column() -7 - factory->metricCount();
                //return item->getText(metadata[i].name, "");
            }
        }
    }*/
}

void
RideIntervalCacheModel::itemChanged(IntervalItem *item)
{
    // ok so lets signal that
    /*int row = RideIntervalCache->intervals().indexOf(item);
    if (row >= 0 && row <= RideIntervalCache->count()) {
        emit dataChanged(createIndex(row,0), createIndex(row,columns_-1));
    }*/
}

void RideIntervalCacheModel::beginReset() { beginResetModel(); }
void RideIntervalCacheModel::endReset() { endResetModel(); }

void 
RideIntervalCacheModel::itemAdded(IntervalItem*)
{
    // nothing to do now as the cache
    // told us to begin/end reset model
    // whilst it updated the ride list
}

void
RideIntervalCacheModel::startRemove(int index)
{
    beginRemoveRows(QModelIndex(), index, index);
}

void
RideIntervalCacheModel::endRemove(int)
{
    endRemoveRows();
}

bool 
RideIntervalCacheModel::setHeaderData (int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (orientation == Qt::Horizontal && role == Qt::EditRole) {
        headings_[section] = value.toString();
        emit headerDataChanged (Qt::Horizontal, section, section);
        return true;
    }
    return false;
}

QVariant 
RideIntervalCacheModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return headings_[section];
    }

    // nought (importanty including 0 for Qt::SizeHintRole)
    return QVariant();
}

// when updating metadata config
void 
RideIntervalCacheModel::configChanged(qint32)
{
    // we are resetting
    beginResetModel();

    // get field config
    metadata = context->athlete->rideMetadata()->getFields();

    // set new column count
    // 0    QString path;
    // 1    QString fileName;
    // 2    QDateTime dateTime;
    // 3    QString present;
    // 4    QColor color;
    // 5    bool isRun;

    columns_ = 5 + factory->metricCount() + metadata.count();
    headings_.clear();

    for (int section=0; section<columns_; section++) {

        switch (section) {
            case 0 : headings_<< QString("path"); break;
            case 1 : headings_<< QString("filename"); break;
            case 2 : headings_<< QString("ride_date"); break;
            case 3 : headings_<< QString("Data"); break;
            case 4 : headings_<< QString("color"); break;
            case 5 : headings_<< QString("isRun"); break;

            default:
            {
                // from here we're either a metric or meta
                // lets work that out ...
                if (section-5 < factory->metricCount()) {

                    // is a metric
                    int i=section-5;
                    headings_<< QString("X%1").arg(factory->metricName(i));

                } else {

                    // is a metadata
                    int i= section -5 - factory->metricCount();
                    headings_<< QString("Z%1").arg(context->specialFields.makeTechName(metadata[i].name));
                }
            }
            break;
        }
    }

    headerDataChanged (Qt::Horizontal, 0, columns_-1);

    // all good
    endResetModel();
}

// catch RideIntervalCache refreshes
void 
RideIntervalCacheModel::refreshUpdate(QDate)
{
}

void 
RideIntervalCacheModel::refreshStart()
{
}

void 
RideIntervalCacheModel::refreshEnd()
{
}
