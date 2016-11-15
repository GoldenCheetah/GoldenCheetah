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

#include "XDataTableModel.h"

XDataTableModel::XDataTableModel(RideFile *ride, QString xdata) : ride(ride), xdata(xdata), series(NULL)
{
    setRide(ride);
}

void
XDataTableModel::setRide(RideFile *newride)
{
    // QPointer helps us check if the current ride has been deleted before trying to disconnect
    static QPointer<RideFileCommand> connection = NULL;
    if (connection) {
        disconnect(connection, SIGNAL(beginCommand(bool,RideCommand*)), this, SLOT(beginCommand(bool,RideCommand*)));
        disconnect(connection, SIGNAL(endCommand(bool,RideCommand*)), this, SLOT(endCommand(bool,RideCommand*)));
    }

    ride = newride;
    tooltips.clear(); // remove the tooltips -- rideEditor will set them (this is fugly, but efficient)

    if (ride) {

        // set series
        series = ride->xdata(xdata);

        // Trap commands
        connection = ride->command;
        connect(ride->command, SIGNAL(beginCommand(bool,RideCommand*)), this, SLOT(beginCommand(bool,RideCommand*)));
        connect(ride->command, SIGNAL(endCommand(bool,RideCommand*)), this, SLOT(endCommand(bool,RideCommand*)));
        connect(ride, SIGNAL(deleted()), this, SLOT(deleted()));

    } else {
        series = NULL;
    }

    // refresh
    emit layoutChanged();
}

void
XDataTableModel::deleted()
{
    // we don't need to disconnect since they're free'd up by QT
    ride = NULL;
    series = NULL;
    beginResetModel();
    endResetModel();
    dataChanged(createIndex(0,0), createIndex(90,999999));
}

Qt::ItemFlags
XDataTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEditable;
    else {
        return (Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }
}

QVariant
XDataTableModel::data(const QModelIndex & index, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();

    if (index.row() >= series->datapoints.count() || index.column() >= (series->valuename.count()+2))
        return QVariant();
    else {
        switch(index.column()) {
        case 0: // time
           return series->datapoints[index.row()]->secs;
        case 1: // distance
           return series->datapoints[index.row()]->km;
        default:
        return series->datapoints[index.row()]->number[index.column()-2];
        }
    }
}

QVariant
XDataTableModel::headerData(int section, Qt::Orientation orient, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();

    if (orient == Qt::Horizontal) {
        if (section >= series->valuename.count()+2)
            return QVariant();
        else {
            switch(section) {
            case 0: return tr("Time");
            case 1: return tr("Distance");
            default:
            return series->valuename[section-2];
            }
        }
    } else {
        return QString("%1").arg(section+1);
    }
}

int
XDataTableModel::rowCount(const QModelIndex &) const
{
    if (series) return series->datapoints.count();
    else return 0;
}

int
XDataTableModel::columnCount(const QModelIndex &) const
{
    if (series) return series->valuename.count()+2;
    else return 0;
}

bool
XDataTableModel::setData(const QModelIndex & index, const QVariant &value, int role)
{
    if (index.row() >= series->datapoints.count() || index.column() >= (series->valuename.count()+2))
        return false;
    else if (role == Qt::EditRole) {
        ride->command->setXDataPointValue(xdata, index.row(), index.column(), value.toDouble());
        return true;
    } else return false;
}

bool
XDataTableModel::setHeaderData(int , Qt::Orientation , const QVariant & , int)
{
    return false;
}

bool
XDataTableModel::insertRow(int row, const QModelIndex &parent)
{
    return insertRows(row, 1, parent);
}

bool
XDataTableModel::insertRows(int row, int count, const QModelIndex &)
{
    if (row >= series->datapoints.count()) return false;
    else {
        while (count--) {
            XDataPoint *p = new XDataPoint;
            ride->command->insertXDataPoint(xdata, row, p);
        }
        return true;
    }
    return false;
}

bool
XDataTableModel::appendRows(QVector<XDataPoint*>newRows)
{
    ride->command->appendXDataPoints(xdata, newRows);
    return true;
}

bool
XDataTableModel::removeRows(int row, int count, const QModelIndex &)
{
    if ((row + count) > series->datapoints.count()) return false;
    ride->command->deleteXDataPoints(xdata, row, count);
    return true;
}

bool
XDataTableModel::insertColumn(QString name)
{
    ride->command->addXDataSeries(xdata, name, "");
    return true;
}

bool
XDataTableModel::insertColumns(int, int, const QModelIndex &)
{
    // WE DON'T SUPPORT THIS
    // use insertColumn(RideFile::SeriesType) instead.
    return false;
}

bool
XDataTableModel::removeColumn(int index)
{
    QString name = headerData(index, Qt::Horizontal).toString();
    ride->command->removeXDataSeries(xdata, name);
    return true;
}

bool
XDataTableModel::removeColumns (int , int , const QModelIndex &)
{
    // WE DON'T SUPPORT THIS
    // use removeColumn(RideFile::SeriesType) instead.
    return false;
}

void
XDataTableModel::setValue(int row, int column, double value)
{
    ride->command->setXDataPointValue(xdata, row, column, value);
}

double
XDataTableModel::getValue(int row, int column)
{
    return series->datapoints[row]->number[column];
}

void
XDataTableModel::forceRedraw()
{
    // tell the view to redraw everything
    if (ride)
        dataChanged(createIndex(0,0), createIndex(series->datapoints.count(), series->valuename.count()+2));
}

//
// RideCommand has made changes...
//
void
XDataTableModel::beginCommand(bool undo, RideCommand *cmd)
{
    switch (cmd->type) {

        case RideCommand::SetXDataPointValue:
            break;

        case RideCommand::InsertXDataPoint:
        {
            InsertXDataPointCommand *dp = (InsertXDataPointCommand *)cmd;
            if (!undo) beginInsertRows(QModelIndex(), dp->row, dp->row);
            else beginRemoveRows(QModelIndex(), dp->row, dp->row);
            break;
        }

        case RideCommand::DeleteXDataPoints:
        {
            DeleteXDataPointsCommand *dp = (DeleteXDataPointsCommand *)cmd;
            if (undo) beginInsertRows(QModelIndex(), dp->row, dp->row);
            else beginRemoveRows(QModelIndex(), dp->row, dp->row);
            break;
        }

        case RideCommand::AppendXDataPoints:
        {
            AppendXDataPointsCommand *ap = (AppendXDataPointsCommand *)cmd;
            if (!undo) beginInsertRows(QModelIndex(), ap->row, ap->row + ap->count - 1);
            else beginRemoveRows(QModelIndex(), ap->row, ap->row + ap->count - 1);
            break;
        }
        default:
            break;
    }
}

void
XDataTableModel::endCommand(bool undo, RideCommand *cmd)
{
    switch (cmd->type) {

        case RideCommand::SetXDataPointValue:
        {
            SetXDataPointValueCommand *spv = (SetXDataPointValueCommand*)cmd;
            QModelIndex cell(index(spv->row,spv->col));
            dataChanged(cell, cell);
            break;
        }
        case RideCommand::InsertXDataPoint:
            if (!undo) endInsertRows();
            else endRemoveRows();
            break;

        case RideCommand::DeleteXDataPoints:
            if (undo) endInsertRows();
            else endRemoveRows();
            break;

        case RideCommand::AppendXDataPoints:
            if (undo) endRemoveRows();
            else endInsertRows();
            break;

        case RideCommand::AddXDataSeries:
        case RideCommand::RemoveXDataSeries:
            //XXXsetHeadings();
            emit layoutChanged();
            break;

        default:
            break;
    }
}

// Tooltips are kept in a QMap, since they SHOULD be sparse
static QString xsstring(int x, RideFile::SeriesType series)
{
    return QString("%1:%2").arg((int)x).arg(static_cast<int>(series));
}

void
XDataTableModel::setToolTip(int row, RideFile::SeriesType series, QString text)
{
    QString key = xsstring(row, series);

    // if text is blank we are removing it
    if (text == "") tooltips.remove(key);

    // if text is non-blank we are changing it
    if (text != "") tooltips.insert(key, text);
}

QString
XDataTableModel::toolTip(int row, RideFile::SeriesType series) const
{
    QString key = xsstring(row, series);
    return tooltips.value(key, "");
}
