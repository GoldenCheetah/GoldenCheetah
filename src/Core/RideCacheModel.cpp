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

#include "GoldenCheetah.h"
#include "Athlete.h"
#include "Context.h"
#include "RideCache.h"
#include "AthleteTab.h"
#include "AbstractView.h"
#include "SpecialFields.h"

#include "RideCacheModel.h"

RideCacheModel::RideCacheModel(Context *context, RideCache *cache) : QAbstractTableModel(cache), context(context), rideCache(cache)
{
    factory = &RideMetricFactory::instance();
    configChanged(CONFIG_FIELDS | CONFIG_NOTECOLOR);

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(refreshStart()), this, SLOT(refreshStart()));
    connect(context, SIGNAL(refreshEnd()), this, SLOT(refreshEnd()));
    connect(context, SIGNAL(refreshUpdate(QDate)), this, SLOT(refreshUpdate(QDate)));
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(itemAdded(RideItem*)));
    connect(rideCache, SIGNAL(itemChanged(RideItem*)), this, SLOT(itemChanged(RideItem*)));
}

// must reimplement these
int 
RideCacheModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : rideCache->count();
}


int 
RideCacheModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : columns_;
}

Qt::ItemFlags
RideCacheModel::flags(const QModelIndex &index) const
{
     if (!index.isValid()) return Qt::ItemIsEnabled;
     return QAbstractTableModel::flags(index);
}

QVariant 
RideCacheModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role);

    if (!index.isValid() || index.row() < 0 || index.row() >= rideCache->count() ||
        index.column() < 0 || index.column() >= columns_) return QVariant();

    const RideItem *item = rideCache->rides().at(index.row());

    switch (index.column()) {
        case 0 : return item->path;
        case 1 : return item->fileName;
        case 2 : return item->dateTime;
        case 3 : return item->present;
        case 4 : return item->color.name();
        case 5 : return item->planned;

        default:
        {
            // from here we're either a metric or meta
            // lets work that out ...
            if (index.column()-5 < factory->metricCount()) {

                // is a metric
                int i=index.column()-5;

                // unpack metric value into ridemetric and use it to get a stringified
                // version using the right metric/imperial conversion
                RideMetric *m = const_cast<RideMetric*>(factory->rideMetric(factory->metricName(i)));

                // bit of a kludge, but will return times as QTime,
                // stuff with no decimal places as a number,
                // but not if high precision, which means
                // metrics with high precision don't sort this is crap XXX
                if (m->isTime()) {
                    return QTime(0,0,0).addSecs(rideCache->rides().at(index.row())->metrics_[m->index()]);
                } else if (m->units(true) != "km" && m->precision() > 0) {
                    m->setValue(rideCache->rides().at(index.row())->metrics_[m->index()]);
                    return m->toString(GlobalContext::context()->useMetricUnits); // string
                } else {

                    // make low precision numbers sort, including distance which we picked
                    // up as a special case. not sure about pace ....
                    double value = rideCache->rides().at(index.row())->metrics_[m->index()];

                    // convert to imperial if needed
                    if (GlobalContext::context()->useMetricUnits == false) 
                        value = (value * m->conversion()) + m->conversionSum();

                    return round(value);
                }

            } else {

                // is a metadata
                int i = index.column() -5 - factory->metricCount();
                return item->getText(metadata[i].name, "");
            }
        }
    }
}

void
RideCacheModel::itemChanged(RideItem *item)
{
    // ok so lets signal that
    int row = rideCache->rides().indexOf(item);
    if (row >= 0 && row <= rideCache->count()) {
        emit dataChanged(createIndex(row,0), createIndex(row,columns_-1));
    }
    //XXX hack to get the navigator to redraw
    context->tab->view(1)->sidebar()->update();
}

void RideCacheModel::beginReset() { beginResetModel(); }
void RideCacheModel::endReset() { endResetModel(); }

void 
RideCacheModel::itemAdded(RideItem*)
{
    // nothing to do now as the cache
    // told us to begin/end reset model
    // whilst it updated the ride list
}

void
RideCacheModel::startRemove(int index)
{
    beginRemoveRows(QModelIndex(), index, index);
}

void
RideCacheModel::endRemove(int)
{
    endRemoveRows();
}

bool 
RideCacheModel::setHeaderData (int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (orientation == Qt::Horizontal && role == Qt::EditRole) {
        headings_[section] = value.toString();
        emit headerDataChanged (Qt::Horizontal, section, section);
        return true;
    }
    return false;
}

QVariant 
RideCacheModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return headings_[section];
    }

    // nought (importanty including 0 for Qt::SizeHintRole)
    return QVariant();
}

// when updating metadata config
void 
RideCacheModel::configChanged(qint32)
{
    // we are resetting
    beginResetModel();

    // get field config
    metadata = GlobalContext::context()->rideMetadata->getFields();

    // set new column count
    // 0    QString path;
    // 1    QString fileName;
    // 2    QDateTime dateTime;
    // 3    QString present;
    // 4    QColor color;
    // 5    bool planned;

    columns_ = 5 + factory->metricCount() + metadata.count();
    headings_.clear();

    for (int section=0; section<columns_; section++) {

        switch (section) {
            case 0 : headings_<< QString("path"); break;
            case 1 : headings_<< QString("filename"); break;
            case 2 : headings_<< QString("ride_date"); break;
            case 3 : headings_<< QString("Data"); break;
            case 4 : headings_<< QString("color"); break;
            case 5 : headings_<< QString("planned"); break;

            default:
            {
                // from here we're either a metric or meta
                // lets work that out ...
                if (section-5 < factory->metricCount()) {

                    // is a metric
                    int i=section-5;
                    headings_<< QString("%1").arg(factory->metricName(i));

                } else {

                    // is a metadata
                    int i= section -5 - factory->metricCount();
                    headings_<< QString("%1").arg(SpecialFields::getInstance().makeTechName(metadata[i].name));
                }
            }
            break;
        }
    }

    headerDataChanged (Qt::Horizontal, 0, columns_-1);

    // all good
    endResetModel();
}

// catch ridecache refreshes
void 
RideCacheModel::refreshUpdate(QDate)
{
}

void 
RideCacheModel::refreshStart()
{
}

void 
RideCacheModel::refreshEnd()
{
}
