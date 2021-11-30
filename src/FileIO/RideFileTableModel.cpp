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

#include "RideFileTableModel.h"

RideFileTableModel::RideFileTableModel(RideFile *ride) : ride(ride)
{
    setRide(ride);
}

void
RideFileTableModel::setRide(RideFile *newride)
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

        // set the headings to reflect the data that is present
        setHeadings();

        // Trap commands
        connection = ride->command;
        connect(ride->command, SIGNAL(beginCommand(bool,RideCommand*)), this, SLOT(beginCommand(bool,RideCommand*)));
        connect(ride->command, SIGNAL(endCommand(bool,RideCommand*)), this, SLOT(endCommand(bool,RideCommand*)));
        connect(ride, SIGNAL(deleted()), this, SLOT(deleted()));

        // refresh
        emit layoutChanged();
    }
}

void
RideFileTableModel::deleted()
{
    // we don't need to disconnect since they're free'd up by QT
    ride = NULL;
    beginResetModel();
    endResetModel();
    dataChanged(createIndex(0,0), createIndex(90,999999));
}

void
RideFileTableModel::setHeadings(RideFile::SeriesType series)
{
        headings_.clear();
        headingsType.clear();

        // set the headings array
        if (series == RideFile::secs || ride->areDataPresent()->secs) {
            headings_ << tr("Time");
            headingsType << RideFile::secs;
        }
        if (series == RideFile::km || ride->areDataPresent()->km) {
            headings_ << tr("Distance");
            headingsType << RideFile::km;
        }
        if (series == RideFile::watts || ride->areDataPresent()->watts) {
            headings_ << tr("Power");
            headingsType << RideFile::watts;
        }
        if (series == RideFile::nm || ride->areDataPresent()->nm) {
            headings_ << tr("Torque");
            headingsType << RideFile::nm;
        }
        if (series == RideFile::cad || ride->areDataPresent()->cad) {
            headings_ << tr("Cadence");
            headingsType << RideFile::cad;
        }
        if (series == RideFile::hr || ride->areDataPresent()->hr) {
            headings_ << tr("Heartrate");
            headingsType << RideFile::hr;
        }
        if (series == RideFile::kph || ride->areDataPresent()->kph) {
            headings_ << tr("Speed");
            headingsType << RideFile::kph;
        }
        if (series == RideFile::alt || ride->areDataPresent()->alt) {
            headings_ << tr("Altitude");
            headingsType << RideFile::alt;
        }
        if (series == RideFile::lat || ride->areDataPresent()->lat) {
            headings_ << tr("Latitude");
            headingsType << RideFile::lat;
        }
        if (series == RideFile::lon || ride->areDataPresent()->lon) {
            headings_ << tr("Longitude");
            headingsType << RideFile::lon;
        }
        if (series == RideFile::headwind || ride->areDataPresent()->headwind) {
            headings_ << tr("Headwind");
            headingsType << RideFile::headwind;
        }
        if (series == RideFile::slope || ride->areDataPresent()->slope) {
            headings_ << tr("Slope");
            headingsType << RideFile::slope;
        }
        if (series == RideFile::temp || ride->areDataPresent()->temp) {
            headings_ << tr("Temperature");
            headingsType << RideFile::temp;
        }
        if (series == RideFile::lrbalance || ride->areDataPresent()->lrbalance) {
            headings_ << tr("Left/Right Balance");
            headingsType << RideFile::lrbalance;
        }
        if (series == RideFile::lte || ride->areDataPresent()->lte) {
            headings_ << tr("Left TE");
            headingsType << RideFile::lte;
        }
        if (series == RideFile::rte || ride->areDataPresent()->rte) {
            headings_ << tr("Right TE");
            headingsType << RideFile::rte;
        }
        if (series == RideFile::lps || ride->areDataPresent()->lps) {
            headings_ << tr("Left PS");
            headingsType << RideFile::lps;
        }
        if (series == RideFile::rps || ride->areDataPresent()->rps) {
            headings_ << tr("Right PS");
            headingsType << RideFile::rps;
        }
        if (series == RideFile::lpco || ride->areDataPresent()->lpco) {
            headings_ << tr("Left Platform Center Offset");
            headingsType << RideFile::lpco;
        }
        if (series == RideFile::rpco || ride->areDataPresent()->rpco) {
            headings_ << tr("Right Platform Center Offset");
            headingsType << RideFile::rpco;
        }
        if (series == RideFile::lppb || ride->areDataPresent()->lppb) {
            headings_ << tr("Left Power Phase Start");
            headingsType << RideFile::lppb;
        }
        if (series == RideFile::lppe || ride->areDataPresent()->lppe) {
            headings_ << tr("Left Power Phase End");
            headingsType << RideFile::lppe;
        }
        if (series == RideFile::rppb || ride->areDataPresent()->rppb) {
            headings_ << tr("Right Power Phase Start");
            headingsType << RideFile::rppb;
        }
        if (series == RideFile::rppe || ride->areDataPresent()->rppe) {
            headings_ << tr("Right Power Phase End");
            headingsType << RideFile::rppe;
        }
        if (series == RideFile::lpppb || ride->areDataPresent()->lpppb) {
            headings_ << tr("Left Peak Power Phase Start");
            headingsType << RideFile::lpppb;
        }
        if (series == RideFile::lpppe || ride->areDataPresent()->lpppe) {
            headings_ << tr("Left Peak Power Phase End");
            headingsType << RideFile::lpppe;
        }
        if (series == RideFile::rpppb || ride->areDataPresent()->rpppb) {
            headings_ << tr("Right Peak Power Phase Start");
            headingsType << RideFile::rpppb;
        }
        if (series == RideFile::rpppe || ride->areDataPresent()->rpppe) {
            headings_ << tr("Right Peak Power Phase End");
            headingsType << RideFile::rpppe;
        }
        if (series == RideFile::smo2 || ride->areDataPresent()->smo2) {
            headings_ << tr("SmO2");
            headingsType << RideFile::smo2;
        }
        if (series == RideFile::thb || ride->areDataPresent()->thb) {
            headings_ << tr("tHb");
            headingsType << RideFile::thb;
        }
        if (series == RideFile::rcad || ride->areDataPresent()->rcad) {
            headings_ << tr("Run Cadence");
            headingsType << RideFile::rcad;
        }
        if (series == RideFile::rvert || ride->areDataPresent()->rvert) {
            headings_ << tr("Vertical Oscillation");
            headingsType << RideFile::rvert;
        }
        if (series == RideFile::rcontact || ride->areDataPresent()->rcontact) {
            headings_ << tr("GCT");
            headingsType << RideFile::rcontact;
        }
        if (series == RideFile::interval || ride->areDataPresent()->interval) {
            headings_ << tr("Interval");
            headingsType << RideFile::interval;
        }

}

Qt::ItemFlags
RideFileTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant
RideFileTableModel::data(const QModelIndex & index, int role) const
{
    if (role == Qt::ToolTipRole) return toolTip(index.row(), columnType(index.column()));

    if (index.row() >= ride->dataPoints().count() || index.column() >= headings_.count())
        return QVariant();
    else {
        return ride->getPoint(index.row(), headingsType[index.column()]);
    }
}

QVariant
RideFileTableModel::headerData(int section, Qt::Orientation orient, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();

    if (orient == Qt::Horizontal) {
        if (section >= headings_.count())
            return QVariant();
        else {
            return headings_[section];
        }
    } else {
        return QString("%1").arg(section+1);
    }
}

int
RideFileTableModel::rowCount(const QModelIndex &) const
{
    if (ride) return ride->dataPoints().count();
    else return 0;
}

int
RideFileTableModel::columnCount(const QModelIndex &) const
{
    if (ride) return headingsType.count();
    else return 0;
}

bool
RideFileTableModel::setData(const QModelIndex & index, const QVariant &value, int role)
{
    if (index.row() >= ride->dataPoints().count() || index.column() >= headings_.count())
        return false;
    else if (role == Qt::EditRole) {
        ride->command->setPointValue(index.row(), headingsType[index.column()], value.toDouble());
        return true;
    } else return false;
}

bool
RideFileTableModel::setHeaderData(int section, Qt::Orientation , const QVariant & value, int)
{
    if (section >= headings_.count()) return false;
    else {
        headings_[section] = value.toString();
        return true;
    }
}

bool
RideFileTableModel::insertRow(int row, const QModelIndex &parent)
{
    return insertRows(row, 1, parent);
}

bool
RideFileTableModel::insertRows(int row, int count, const QModelIndex &)
{
    if (row >= ride->dataPoints().count()) return false;
    else {
        while (count--) {
            struct RideFilePoint *p = new RideFilePoint;
            ride->command->insertPoint(row, p);
        }
        return true;
    }
}

bool
RideFileTableModel::appendRows(QVector<RideFilePoint>newRows)
{
    ride->command->appendPoints(newRows);
    return true;
}

bool
RideFileTableModel::removeRows(int row, int count, const QModelIndex &)
{
    if ((row + count) > ride->dataPoints().count()) return false;
    ride->command->deletePoints(row, count);
    return true;
}

bool
RideFileTableModel::insertColumn(RideFile::SeriesType series)
{
    if (headingsType.contains(series)) return false; // already there

    ride->command->setDataPresent(series, true);
    return true;
}

bool
RideFileTableModel::insertColumns(int, int, const QModelIndex &)
{
    // WE DON'T SUPPORT THIS
    // use insertColumn(RideFile::SeriesType) instead.
    return false;
}

bool
RideFileTableModel::removeColumn(RideFile::SeriesType series)
{
    if (headingsType.contains(series))  {
        ride->command->setDataPresent(series, false);
        return true;
    } else
        return false; // its not there
}

bool
RideFileTableModel::removeColumns (int , int , const QModelIndex &)
{
    // WE DON'T SUPPORT THIS
    // use removeColumn(RideFile::SeriesType) instead.
    return false;
}

void
RideFileTableModel::setValue(int row, int column, double value)
{
    ride->command->setPointValue(row, headingsType[column], value);
}

double
RideFileTableModel::getValue(int row, int column)
{
    return ride->getPointValue(row, headingsType[column]);
}

void
RideFileTableModel::forceRedraw()
{
    // tell the view to redraw everything
    if (ride)
        dataChanged(createIndex(0,0), createIndex(headingsType.count(), ride->dataPoints().count()));
}

//
// RideCommand has made changes...
//
void
RideFileTableModel::beginCommand(bool undo, RideCommand *cmd)
{
    switch (cmd->type) {

        case RideCommand::SetPointValue:
            break;

        case RideCommand::InsertPoint:
        {
            InsertPointCommand *dp = (InsertPointCommand *)cmd;
            if (!undo) beginInsertRows(QModelIndex(), dp->row, dp->row);
            else beginRemoveRows(QModelIndex(), dp->row, dp->row);
            break;
        }

        case RideCommand::DeletePoint:
        {
            DeletePointCommand *dp = (DeletePointCommand *)cmd;
            if (undo) beginInsertRows(QModelIndex(), dp->row, dp->row);
            else beginRemoveRows(QModelIndex(), dp->row, dp->row);
            break;
        }

        case RideCommand::DeletePoints:
        {
            DeletePointsCommand *ds = (DeletePointsCommand *)cmd;
            if (undo) beginInsertRows(QModelIndex(), ds->row, ds->row + ds->count - 1);
            else beginRemoveRows(QModelIndex(), ds->row, ds->row + ds->count - 1);
            break;
        }

        case RideCommand::AppendPoints:
        {
            AppendPointsCommand *ap = (AppendPointsCommand *)cmd;
            if (!undo) beginInsertRows(QModelIndex(), ap->row, ap->row + ap->count - 1);
            else beginRemoveRows(QModelIndex(), ap->row, ap->row + ap->count - 1);
            break;
        }
        default:
            break;
    }
}

void
RideFileTableModel::endCommand(bool undo, RideCommand *cmd)
{
    switch (cmd->type) {

        case RideCommand::SetPointValue:
        {
            SetPointValueCommand *spv = (SetPointValueCommand*)cmd;
            QModelIndex cell(index(spv->row,headingsType.indexOf(spv->series)));
            dataChanged(cell, cell);
            break;
        }
        case RideCommand::InsertPoint:
            if (!undo) endInsertRows();
            else endRemoveRows();
            break;

        case RideCommand::DeletePoint:
        case RideCommand::DeletePoints:
            if (undo) endInsertRows();
            else endRemoveRows();
            break;

        case RideCommand::AppendPoints:
            if (undo) endRemoveRows();
            else endInsertRows();
            break;

        case RideCommand::SetDataPresent:
            setHeadings();
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
RideFileTableModel::setToolTip(int row, RideFile::SeriesType series, QString text)
{
    QString key = xsstring(row, series);

    // if text is blank we are removing it
    if (text == "") tooltips.remove(key);

    // if text is non-blank we are changing it
    if (text != "") tooltips.insert(key, text);
}

QString
RideFileTableModel::toolTip(int row, RideFile::SeriesType series) const
{
    QString key = xsstring(row, series);
    return tooltips.value(key, "");
}
