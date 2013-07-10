/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

/********************* WARNING !!! *********************
 *
 * PLEASE DO NOT EDIT THIS SOURCE FILE IF YOU WANT TO
 * CHANGE THE WAY ROWS ARE GROUPED. THERE IS A FUNCTION
 * IN QxtScheduleView.cpp CALLED groupFromValue() WHICH
 * IS CALLED TO GET THE GROUP NAME FOR A COLUMN HEADING
 * VALUE COMBINATION. THIS IS CALLED FROM whichGroup()
 * BELOW.
 *
 * Of course, if there is a bug in this ProxyModel you
 * are welcome to fix it!
 * But do take care. Specifically with the index()
 * parent() mapToSource() and mapFromSource() functions.
 *
 *******************************************************/

#ifndef _GC_QxtScheduleViewProxy_h
#define _GC_QxtScheduleViewProxy_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include "qxtscheduleview.h"
#include "Context.h"
#include "RideMetadata.h"
#include "Colors.h"
#include "Settings.h"

// Proxy model for doing groupBy
class QxtScheduleViewProxy : public QAbstractProxyModel
{
    Q_OBJECT
    G_OBJECT


private:
    QxtScheduleView *rideNavigator;
    QList<FieldDefinition> *fieldDefinitions;
    QList<QString> columns; // what columns in the sql model
    Context *context;
    int filenameIndex, durationIndex, dateIndex, summaryIndex;

public:

    QxtScheduleViewProxy(QWidget *parent, QList<FieldDefinition> *fields, Context *context) : QAbstractProxyModel(parent), fieldDefinitions(fields), mainWindow(main) {
        setParent(parent);
    }
    ~QxtScheduleViewProxy() {}

    void setSourceModel(QAbstractItemModel *model) {
        QAbstractProxyModel::setSourceModel(model);
        summaryIndex = filenameIndex = dateIndex = durationIndex = -1;
        columns.clear();
        for (int i=0; i<sourceModel()->columnCount(); i++) {
            QString column = sourceModel()->headerData (i, Qt::Horizontal, Qt::DisplayRole).toString();
            columns << column;
            if (column == tr("Duration")) durationIndex = i;
            if (column == tr("Date")) dateIndex = i;
            if (column == tr("Filename")) filenameIndex = i;
            if (column == tr("Calendar Text")) summaryIndex = i;
        }
    }

    QModelIndex index(int row, int column, const QModelIndex &/*parent*/ = QModelIndex()) const {
        return createIndex(row,column,(void *)NULL);
    }

    QModelIndex parent(const QModelIndex &index) const {
        // parent should be encoded in the index if we supplied it, if
        // we didn't then return a duffer
        if (index == QModelIndex() || index.internalPointer() == NULL) {
            return QModelIndex();
        } else if (index.column()) {
            return QModelIndex();
        }  else {
            return *static_cast<QModelIndex*>(index.internalPointer());
        }
    }

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const {
        return sourceModel()->index(proxyIndex.row(), proxyIndex.column(), QModelIndex());
    }

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const {

        return createIndex(sourceIndex.row(), sourceIndex.column(), (void *)NULL); // accomodate virtual column
    }

    // we override the standard version to make our virtual column zero
    // selectable. If we don't do that then the arrow keys don't work
    // since there are no valid rows to cursor up or down to.
    Qt::ItemFlags flags (const QModelIndex &/*index*/) const {
        return 0; //Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    // we'll add some roles to access specific bits of info to!
    enum UserRoles {
        FilenameRole = Qxt::UserRole + 1
    };

    QVariant data(const QModelIndex &proxyIndex, int role = Qt::DisplayRole) const {

        if (!proxyIndex.isValid()) return QVariant();

        QVariant returning;

        switch (role) {
        // do something interesting!
        case Qxt::ItemStartTimeRole:
            if (dateIndex >= 0)
                return sourceModel()->data(sourceModel()->index(proxyIndex.row(), dateIndex), Qt::DisplayRole).toDateTime().toTime_t();
            else
                return 0;
            break;
        case Qxt::ItemDurationRole:
            if (durationIndex >= 0) {
                int duration = sourceModel()->data(sourceModel()->index(proxyIndex.row(), durationIndex), Qt::DisplayRole).toInt();
                if (duration) return duration;
                else return 1; // zeros cause QxtSchedule to crash!
            } else
                return 3600; // Just default to an hour
            break;
        case Qt::BackgroundRole:
            if (mainWindow->rideItem() && dateIndex >= 0 &&
                sourceModel()->data(sourceModel()->index(proxyIndex.row(), dateIndex), Qt::DisplayRole).toDateTime() == mainWindow->rideItem()->dateTime)
                return GColor(CCALCURRENT);
            else
                return GColor(CCALACTUAL);
            break;
        case Qt::ForegroundRole:
            return Qt::black;
            break;
        case Qt::FontRole:
            {
                QFont font;
                font.fromString(appsettings->value(this, GC_FONT_CALENDAR, QFont().toString()).toString());
                font.setPointSize(appsettings->value(this, GC_FONT_CALENDAR_SIZE, 12).toInt());
                return font;
            }
            break;
        case Qxt::OutlineRole:
            if (mainWindow->rideItem() && dateIndex >= 0 &&
                sourceModel()->data(sourceModel()->index(proxyIndex.row(), dateIndex), Qt::DisplayRole).toDateTime() == mainWindow->rideItem()->dateTime)
                return 3;
            else
                return 1;
            break;

        case FilenameRole:
            if (filenameIndex >= 0)
                return sourceModel()->data(sourceModel()->index(proxyIndex.row(), filenameIndex), Qt::DisplayRole).toString();
            else
                return "";
            break;
        case Qt::EditRole:
        case Qt::DisplayRole:
            return sourceModel()->data(sourceModel()->index(proxyIndex.row(), summaryIndex), Qt::DisplayRole).toString();
            break;
        }
        return QVariant();
    }

    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const {
        return sourceModel()->headerData(section, orientation, role);
    }

    bool setHeaderData (int section, Qt::Orientation orientation, const QVariant & value, int role = Qt::EditRole) {
        return sourceModel()->setHeaderData(section, orientation, value, role);
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const {
        return sourceModel()->columnCount(parent);
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const {
        return sourceModel()->rowCount(parent);
    }

    // does this index have children?
    bool hasChildren(const QModelIndex &index) const {
        return sourceModel()->hasChildren(index);
    }
};
#endif
