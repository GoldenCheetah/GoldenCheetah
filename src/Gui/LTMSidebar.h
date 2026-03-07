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

#ifndef _GC_LTMSidebar_h
#define _GC_LTMSidebar_h 1
#include "GoldenCheetah.h"

#include "Context.h"
#include "GcSideBarItem.h"
#include "Seasons.h"
#include "SeasonDialogs.h"
#include "RideMetric.h"
#include "LTMSettings.h"
#include "LTMChartParser.h"

#include "SearchFilterBox.h"

#include <QDir>
#include <QtGui>

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

        void updatePresetChartsOnShow(GcViewType viewType);

    signals:
        void dateRangeChanged(DateRange);

    public slots:

        // date range selection and editing
        void selectDateRange(DateRange);
        void dateRangeTreeWidgetSelectionChanged();
        void dateRangePopup(QPoint);
        void dateRangePopup();
        void dateRangeMoved(QTreeWidgetItem *, int, int);
        void addRange();
        void editRange();
        void deleteRange();

        void eventPopup(QPoint);
        void eventPopup();
        void editEvent();
        void deleteEvent();
        void addEvent();
        void eventMoved(QTreeWidgetItem *, int, int);

        void addPhase();

        // working with preset charts
        void presetsChanged();
        void presetSelectionChanged();
        void presetSelected(int index);
        void presetPopup(QPoint);
        void presetPopup();
        void presetMoved(QTreeWidgetItem *, int, int);
        void addPreset();
        void editPreset();
        void deletePreset();
        void resetPreset();
        void exportPreset();
        void importPreset();

        void filterTreeWidgetSelectionChanged();
        void resetFilters(); // rebuild the seasons list if it changes
        void filterPopup();
        void manageFilters();
        void deleteFilter();
        void autoFilterChanged(); // users turned on/off an item
        void autoFilterSelectionChanged(); // users selected a value 
        void filterNotify(); // merge/update when auto or query filter applied

        // config etc
        void configChanged(qint32);
        void resetSeasons(); // rebuild the seasons list if it changes
        void setAutoFilterMenu();
        void autoFilterRefresh(); // refresh the value lists

    protected slots:

        void chartVisibilityChanged();
        void filterVisibilityChanged();

    private:

        Context *context;
        bool active;

        Seasons *seasons;
        GcSplitterItem *seasonsWidget;
        SeasonTreeView *dateRangeTree;
        QTreeWidgetItem *allDateRanges;

        bool chartsWidgetVisible;
        GcSplitterItem *chartsWidget;
        ChartTreeView *chartTree;
        QTreeWidgetItem *allCharts;

        GcSplitterItem *eventsWidget;
        SeasonEventTreeView *eventTree;
        QTreeWidgetItem *allEvents;

        GcSplitterItem *filtersWidget;
        QTreeWidget *filterTree;
        QTreeWidgetItem *allFilters;

        QMenu *autoFilterMenu;
        QList<bool> autoFilterState;

        GcSplitter *splitter;
        GcSubSplitter *filterSplitter;

        // filter state
        bool isqueryfilter, isautofilter;
        QStringList autoFilterFiles, queryFilterFiles;

        void buildDateRangeMenu(QMenu &menu, QTreeWidgetItem *item, bool asGlobal) const;

};

#endif // _GC_LTMSidebar_h
