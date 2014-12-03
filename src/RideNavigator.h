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

#include "Context.h"
#include "MetricAggregator.h"
#include "RideMetadata.h"
#include "DBAccess.h"
#include "Context.h"
#include "Settings.h"
#include "Colors.h"

#include <QSqlTableModel>
#include <QTableView>
#include <QHeaderView>
#include <QScrollBar>
#include <QDragMoveEvent>
#include <QDragEnterEvent>

class NavigatorCellDelegate;
class GroupByModel;
class SearchFilter;
class SearchFilterBox;
class DiaryWindow;
class DiarySidebar;
class BUGFIXQSortFilterProxyModel;
class DataFilter;
class GcMiniCalendar;
class SearchBox;
class RideTreeView;

//
// The RideNavigator
//
// A list of rides displayed using a QTreeView on top of
// a QSQLTableModel which reads from the "metrics" table
// via the DBAccess database connection
//
class RideNavigator : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int sortByIndex READ sortByIndex WRITE setSortByIndex USER true)
    Q_PROPERTY(int sortByOrder READ sortByOrder WRITE setSortByOrder USER true)
    Q_PROPERTY(int groupBy READ groupBy WRITE setGroupBy USER true)
    Q_PROPERTY(QString columns READ columns WRITE setColumns USER true)
    Q_PROPERTY(QString widths READ widths WRITE setWidths USER true)

    friend class ::NavigatorCellDelegate;
    friend class ::GroupByModel;
    friend class ::DiaryWindow;
    friend class ::DiarySidebar;
    friend class ::GcMiniCalendar;
    friend class ::DataFilter;
    friend class ::SearchBox;

    public:
        RideNavigator(Context *, bool mainwindow = false);
        ~RideNavigator();

        void borderMenu(const QPoint &pos);

        // so the cell delegate can access
        RideTreeView *tableView; // the view

        Context *context;

    public slots:

        void configChanged();
        void refresh();

        void showEvent(QShowEvent *event);

        void resizeEvent(QResizeEvent*);
        void showTreeContextMenuPopup(const QPoint &);

        // working with columns
        void columnsChanged();
        void columnsResized(int, int, int);
        void removeColumn();
        void showColumnChooser();

        // user changed ride selection somewhere else
        void setRide(RideItem*);

        // user double clicked/hit return on a ride
        // not used at present, for the future
        void selectRide(const QModelIndex &index);

        // user changed sort order, so ride is off screen
        // bring it back in view
        void cursorRide();

        // user selection so line up
        void selectionChanged(QItemSelection);
        bool eventFilter(QObject *object, QEvent *e);

        // drop column headings from column chooser
        void dragEnterEvent(QDragEnterEvent *event);
        void dropEvent(QDropEvent *event);

        // how wide am I?
        void setWidth(int x);
        void setSortBy(int index, Qt::SortOrder);
        void setGroupByColumn();

        int sortByIndex() const { return _sortByIndex; }
        void setSortByIndex(int x) { _sortByIndex = x; }

        int sortByOrder() const { return _sortByOrder; }
        void setSortByOrder(int x) { _sortByOrder = x; }

        int groupBy() const { return _groupBy; }
        void setGroupBy(int x) { _groupBy = x; }
 
        QString columns() const { return _columns; }
        void setColumns(QString x) { _columns = x; }

        // These are used in the main sidebar to let the users
        // add remove columns etc without using right click
        QStringList columnNames() const;
        void setGroupByColumnName(QString); // set blank turns it off
        void noGroups() { currentColumn=-1; setGroupByColumn(); }

        QString widths() const { return _widths; }
        void setWidths (QString x) { _widths = x; resetView(); } // only reset once widths are set

        void resetView(); // when columns/width changes

        void searchStrings(QStringList);
        void clearSearch();

    protected:
        GroupByModel *groupByModel; // for group by
        BUGFIXQSortFilterProxyModel *sortModel; // for sort/filter

        // keep track of the headers
        QList<QString> logicalHeadings;
        QList<QString> visualHeadings;

        // this maps friendly names to metric names
        QMap <QString, QString> nameMap;
        QMap<QString, const RideMetric *> columnMetrics;

    private:
        bool active;
        bool mainwindow;
        bool init;
        int currentColumn;
        int pwidth;
        NavigatorCellDelegate *delegate;
        QVBoxLayout *mainLayout;
        RideItem *currentItem;

        // properties
        int _sortByIndex;
        int _sortByOrder;
        int _groupBy;
        QString _columns;
        QString _widths;

        // font metrics for display etc
        int fontHeight;
        QColor reverseColor; // used by delegate when 'use for color' set

        // search filter
        SearchFilter *searchFilter;

        // search filter box
        SearchFilterBox *searchFilterBox;

        // support functions
        void calcColumnsChanged(bool, int logicalIndex=0, int oldWidth=0, int newWidth=0);
        void setColumnWidth(int, bool, int logicalIndex=0, int oldWidth=0, int newWidth=0);
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

    // We increase the row height if there is a calendar text to display
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const ;

    // override stanard painter to use color config to paint background
    // and perform correct level of rounding for each column before displaying
    // it will also override the values returned from metricDB with in-core values
    // when the ride has been modified but not saved (i.e. data is dirty)
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void setWidth(int x) { pwidth=x; }

private slots:

    void commitAndCloseEditor();

private:
    RideNavigator *rideNavigator;
    int pwidth;

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

class RideTreeView : public QTreeView
{
    Q_OBJECT;

    public:
        RideTreeView();

    signals:
        void rowSelected(QItemSelection);

    public slots:
        void selectionChanged (const QItemSelection &selected, const QItemSelection&deselected) {
            rowSelected(selected);
            QTreeView::selectionChanged(selected,deselected);
        }

    protected:
         void dragEnterEvent(QDragEnterEvent *e) {
            e->accept();
        }
 
        void dragMoveEvent(QDragMoveEvent *e) {
            e->accept();
        }
};
#endif
