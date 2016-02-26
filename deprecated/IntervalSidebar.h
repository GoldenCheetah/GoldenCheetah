/*
 * Copyright (c) 2014 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#ifndef _GC_IntervalSidebar_h
#define _GC_IntervalSidebar_h 1
#include "GoldenCheetah.h"

#include "Context.h"
#include "GcSideBarItem.h"
#include "IntervalNavigator.h"
#include "DiarySidebar.h" // for GcMultiCalendar
#include "RideItem.h"
#include "IntervalTreeView.h"
#include "IntervalSummaryWindow.h"

class IntervalSidebar : public QWidget
{
    Q_OBJECT
    G_OBJECT

    public:

        IntervalSidebar(Context *context);
        void close();
        void setWidth(int x) { routeNavigator->setWidth(x); }
        IntervalNavigator *routeNavigator;
        IntervalNavigator *bestNavigator;

    signals:

    public slots:

        // config etc
        void configChanged(qint32);

        void filterChanged();
        void setFilter(QStringList);
        void clearFilter();

        // analysis menu
        void routePopup();
        void showRouteMenu(const QPoint &pos);
        void bestPopup();
        void showBestMenu(const QPoint &pos);

        // interval menu
        /*void intervalPopup();
        void showIntervalMenu(const QPoint &pos);*/

        // interval functions
        /*void addIntervals();
        void addIntervalForPowerPeaksForSecs(RideFile *ride, int windowSizeSecs, QString name);
        void findPowerPeaks();
        void editInterval(); // from right click
        void deleteInterval(); // from right click
        void renameInterval(); // from right click
        void zoomInterval(); // from right click
        void sortIntervals(); // from menu popup
        void renameIntervalSelected(void); // from menu popup
        void renameIntervalsSelected(void); // from menu popup -- rename a series
        void editIntervalSelected(); // from menu popup
        void deleteIntervalSelected(void); // from menu popup
        void zoomIntervalSelected(void); // from menu popup
        void zoomOut();
        void createRouteIntervalSelected(void); // from menu popup
        void frontInterval();
        void backInterval();*/

    private:

        Context *context;
        GcSplitter *splitter;

        QSignalMapper *groupByMapper;

        GcSplitterItem *routeItem;
        GcSplitterItem *bestItem;

        QSplitter *intervalSplitter;
        IntervalSummaryWindow *intervalSummaryWindow;
        IntervalItem *activeInterval; // currently active for context menu popup
};

#endif // _GC_IntervalSidebar_h
