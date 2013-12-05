/*

// named searchs
#include "NamedSearch.h"
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

#ifndef _GC_LTMSidebar_h
#define _GC_LTMSidebar_h 1
#include "GoldenCheetah.h"

#include "Context.h"
#include "GcSideBarItem.h"
#include "Season.h"
#include "RideMetric.h"
#include "LTMSettings.h"

#ifdef GC_HAVE_LUCENE
#include "SearchFilterBox.h"
#endif

#include <QDir>
#include <QtGui>

class QWebView;
class LTMSidebar : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        LTMSidebar(Context *context);

        //const Season *currentDateRange() { return dateRange; }
        //void selectDateRange(int);

        // allow others to create and update season structures
        int newSeason(QString, QDate, QDate, int);
        void updateSeason(int, QString, QDate, QDate, int);

    signals:
        void dateRangeChanged(DateRange);

    public slots:

        // date range selection and editing
        void dateRangeTreeWidgetSelectionChanged();
        void dateRangePopup(QPoint);
        void dateRangePopup();
        void dateRangeChanged(QTreeWidgetItem *, int);
        void dateRangeMoved(QTreeWidgetItem *, int, int);
        void addRange();
        void editRange();
        void deleteRange();

        void eventPopup(QPoint);
        void eventPopup();
        void editEvent();
        void deleteEvent();
        void addEvent();

        void filterTreeWidgetSelectionChanged();
        void resetFilters(); // rebuild the seasons list if it changes
        void filterPopup();
        void manageFilters();
        void deleteFilter();

        // config etc
        void configChanged();
        void resetSeasons(); // rebuild the seasons list if it changes

        // gui components
        void setSummary(DateRange);

    private:

        Context *context;
        bool active;
        QDate from, to; // so we don't repeat update...


        Seasons *seasons;
        GcSplitterItem *seasonsWidget;
        SeasonTreeView *dateRangeTree;
        QTreeWidgetItem *allDateRanges;

        GcSplitterItem *eventsWidget;
        QTreeWidget *eventTree;
        QTreeWidgetItem *allEvents;

        GcSplitterItem *filtersWidget;
        QTreeWidget *filterTree;
        QTreeWidgetItem *allFilters;

        QWebView *summary;

        GcSplitter *splitter;
};

#endif // _GC_LTMSidebar_h
