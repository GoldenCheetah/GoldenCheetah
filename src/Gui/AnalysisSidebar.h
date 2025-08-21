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

#ifndef _GC_AnalysisSidebar_h
#define _GC_AnalysisSidebar_h 1
#include "GoldenCheetah.h"

#include "Context.h"
#include "GcSideBarItem.h"
#include "RideNavigator.h"
#include "PlanSidebar.h" // for GcMultiCalendar
#include "RideItem.h"
#include "IntervalTreeView.h"
#include "IntervalSummaryWindow.h"
#include "CloudService.h"

#include <QTreeWidgetItem>
#include <QTreeWidget>
#include <QWidget>

class AnalysisSidebar : public QWidget
{
    Q_OBJECT
    G_OBJECT

    public:

        AnalysisSidebar(Context *context);
        void close();
        void setWidth(int x) { rideNavigator->setWidth(x); }
        RideNavigator *rideNavigator;

    signals:

    public slots:

        // config etc
        void configChanged(qint32);
        void setRide(RideItem*);
        void intervalsUpdate(RideItem*);
        void intervalItemSelectionChanged(IntervalItem*);

        void filterChanged();
        void setFilter(QStringList);
        void clearFilter();

        // analysis menu
        void analysisPopup();
        void showActivityMenu(const QPoint &pos);

        // interval selection
        void itemSelectionChanged();

        // interval menu
        void intervalPopup();
        void showIntervalMenu(const QPoint &pos);

        // interval functions
        void addIntervals();
        void editInterval(); // from right click
        void deleteRoute(); // stop tracking this route
        void editRoute(); // change route name
        void zoomInterval(); // from right click
        void sortIntervals(); // from menu popup
        void renameIntervalsSelected(void); // from menu popup and right click -- rename a series
        void editIntervalSelected(); // from menu popup
        void deleteIntervalSelected(void); // from menu popup and and right click
        void clickZoomInterval(QTreeWidgetItem*); // from treeview
        void zoomIntervalSelected(void); // from menu popup
        void zoomOut();
        void createRouteIntervalSelected(void); // from menu popup
        void frontInterval();
        void backInterval();
        void perfTestIntervalSelected();
        void createPerfTestIntervalSelected();

    private:

        Context *context;
        GcSplitter *splitter;

        QWidget *activityHistory;
        GcSplitterItem *activityItem;
        QSignalMapper *groupByMapper;

        GcSplitterItem *calendarItem;
        GcMultiCalendar *calendarWidget;

        GcSplitterItem *intervalItem;
        QSplitter *intervalSplitter;
        IntervalSummaryWindow *intervalSummaryWindow;
        IntervalItem *activeInterval; // currently active for context menu popup

        IntervalTreeView *intervalTree; // the interval tree
        QMap<RideFileInterval::intervaltype, QTreeWidgetItem*> trees;

        CloudServiceAutoDownloadWidget *autowidget;
};

#endif // _GC_AnalysisSidebar_h
