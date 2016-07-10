/*
 * Copyright (c) 2016 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_XDataTableModel_h
#define _GC_XDataTableModel_h 1
#include "GoldenCheetah.h"

#include "RideFile.h"
#include "RideFileCommand.h"
#include "Context.h"
#include <QAbstractTableModel>

//
// Provides a QAbstractTableModel interface to a ridefile and can be used as a
// model in a QTableView to view RideFile datapoints. Used by the RideEditor
//
// All modifications are made via the RideFileCommand interface to ensure the
// command pattern is honored for undo/redo etc.
//

class XDataEditor;
class XDataTableModel : public QAbstractTableModel
{
    Q_OBJECT
    G_OBJECT

    friend class ::XDataEditor;

    public:

        XDataTableModel(RideFile *ride = 0, QString xdata = "");

        // when we choose new ride
        void setRide(RideFile *ride);

        // used by the view - mandatory implementation
        Qt::ItemFlags flags(const QModelIndex &index) const;
        QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex & parent = QModelIndex()) const;
        bool setData(const QModelIndex & index, const QVariant &value, int role = Qt::EditRole);
        bool setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role = Qt::EditRole);
        bool insertRow(int row, const QModelIndex & parent = QModelIndex());
        bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex());
        bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
        bool appendRows(QVector<XDataPoint*>newRows);

        // DO NOT USE THESE -- THEY ARE NULL --
        bool insertColumns (int column, int count, const QModelIndex & parent = QModelIndex());
        bool removeColumns (int column, int count, const QModelIndex & parent = QModelIndex());
        // USE THESE INSTEAD
        bool insertColumn(QString name);
        bool removeColumn(int index);

        // get/set value by column number
        void setValue(int row, int column, double value);
        double getValue(int row, int column);

        // get/set tooltip
        void setToolTip(int row, RideFile::SeriesType series, QString value);
        QString toolTip(int row, RideFile::SeriesType series) const;

    public slots:
        // RideCommand signals trapped here
        void beginCommand(bool undo, RideCommand *);
        void endCommand(bool undo, RideCommand *);

        // Import wizard frees the ridefile memory to
        // stop exhausting memory, but we need to know
        // coz we reference the ride file and not the 
        // ride item. long story.
        void deleted();

        // force redraw - used by anomaly detection and find
        void forceRedraw();

    protected:

    private:
        RideFile *ride;
        QString xdata;
        XDataSeries *series;
        QMap <QString,QString> tooltips;
};
#endif // _GC_XDataTableModel_h
