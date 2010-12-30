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

#ifndef _GC_RideNavigator_h
#define _GC_RideNavigator_h 1
#include "GoldenCheetah.h"

#include "MainWindow.h"
#include "MetricAggregator.h"
#include "RideMetadata.h"
#include "DBAccess.h"
#include "MainWindow.h"
#include "Settings.h"

#include <QSqlTableModel>
#include <QTableView>

class NavigatorCellDelegate;
class GroupByModel;
class DiaryWindow;

//
// The RideNavigator
//
// A list of rides displayed using a QTreeView on top of
// a QSQLTableModel which reads from the "metrics" table
// via the DBAccess database connection
//
class RideNavigator : public QWidget
{
    Q_OBJECT
    G_OBJECT


    friend class ::NavigatorCellDelegate;
    friend class ::GroupByModel;
    friend class ::DiaryWindow;

    public:
        RideNavigator(MainWindow *);
        ~RideNavigator();

        void borderMenu(const QPoint &pos);

        // so the cell delegate can access
        QTreeView *tableView; // the view

    signals:

    public slots:
        void refresh();

        // working with columns
        void columnsChanged();
        void removeColumn();
        void showColumnChooser();

        // user selected a group by column
        void setGroupBy();

        // user double clicked or pressed enter on ride
        void selectRide(const QModelIndex &index);

        // user selection here or on mainwindow's ride list
        void cursorRide();
        void selectRow();
        bool eventFilter(QObject *object, QEvent *e);
        void rideTreeSelectionChanged(); // watch main window

        // drop column headings from column chooser
        void dragEnterEvent(QDragEnterEvent *event);
        void dropEvent(QDropEvent *event);

    protected:
        QSqlTableModel *sqlModel; // the sql table
        GroupByModel *groupByModel; // for group by
        QSortFilterProxyModel *sortModel; // for sort/filter

        // keep track of the headers
        QList<QString> logicalHeadings;
        QList<QString> visualHeadings;

        QMap<QString, const RideMetric *> columnMetrics;

    private:
        MainWindow *main;
        bool active;
        int groupBy;
        int currentColumn;
};

//
// Used to paint the cells in the ride navigator
//
class NavigatorCellDelegate : public QItemDelegate
{
    Q_OBJECT
    G_OBJECT


public:
    NavigatorCellDelegate(RideNavigator *, QObject *parent = 0);

    // These are all null since we don't allow editing
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    // override stanard painter to use color config to paint background
    // and perform correct level of rounding for each column before displaying
    // it will also override the values returned from metricDB with in-core values
    // when the ride has been modified but not saved (i.e. data is dirty)
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private slots:

    void commitAndCloseEditor();

private:
    RideNavigator *rideNavigator;

};

//
// Column Chooser
//
// Used by the ride navigator, provides an array
// of pushbuttons representing each available field
// and lets the user drag them onto the RideNavigator
// to include into the column list. Similarly, the
// available columns can be dragged off the ride
// navigator and dropped back in the array
//
class ColumnChooser : public QWidget
{
    Q_OBJECT
    G_OBJECT


public:
    ColumnChooser(QList<QString>&columnHeadings);

public slots:
    void buttonClicked(QString name);

private:
    QGridLayout *buttons;
    QSignalMapper *clicked;
};
#endif
