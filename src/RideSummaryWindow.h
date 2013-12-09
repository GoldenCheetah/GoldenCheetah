/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_RideSummaryWindow_h
#define _GC_RideSummaryWindow_h 1
#include "GoldenCheetah.h"
#include "MainWindow.h" // for isfiltered and filters
#include "Context.h"

#include <QWidget>
#include <QWebView>
#include <QWebFrame>
#include <QFormLayout>

#include "SummaryMetrics.h"

#ifdef GC_HAVE_LUCENE
#include "SearchFilterBox.h"
#endif


class RideSummaryWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

#ifdef GC_HAVE_LUCENE
    Q_PROPERTY(QString filter READ filter WRITE setFilter USER true)
#endif
    Q_PROPERTY(QDate fromDate READ fromDate WRITE setFromDate USER true)
    Q_PROPERTY(QDate toDate READ toDate WRITE setToDate USER true)
    Q_PROPERTY(QDate startDate READ startDate WRITE setStartDate USER true)
    Q_PROPERTY(int lastN READ lastN WRITE setLastN USER true)
    Q_PROPERTY(int lastNX READ lastNX WRITE setLastNX USER true)
    Q_PROPERTY(int prevN READ prevN WRITE setPrevN USER true)
    Q_PROPERTY(int useSelected READ useSelected WRITE setUseSelected USER true) // !! must be last property !!

    public:

        // two modes - summarise ride or summarise date range
        RideSummaryWindow(Context *context, bool ridesummary = true);

        // properties
        int useSelected() { return dateSetting->mode(); }
        void setUseSelected(int x) { dateSetting->setMode(x); }
        QDate fromDate() { return dateSetting->fromDate(); }
        void setFromDate(QDate date)  { return dateSetting->setFromDate(date); }
        QDate toDate() { return dateSetting->toDate(); }
        void setToDate(QDate date)  { return dateSetting->setToDate(date); }
        QDate startDate() { return dateSetting->startDate(); }
        void setStartDate(QDate date)  { return dateSetting->setStartDate(date); }
        int lastN() { return dateSetting->lastN(); }
        void setLastN(int x) { dateSetting->setLastN(x); }
        int lastNX() { return dateSetting->lastNX(); }
        void setLastNX(int x) { dateSetting->setLastNX(x); }
        int prevN() { return dateSetting->prevN(); }
        void setPrevN(int x) { dateSetting->setPrevN(x); }
#ifdef GC_HAVE_LUCENE
        bool isFiltered() const { if (!ridesummary) return (filtered || context->ishomefiltered || context->isfiltered);
                                  else return false; }
        // filter
        QString filter() const { return ridesummary ? "" : searchBox->filter(); }
        void setFilter(QString x) { if (!ridesummary) searchBox->setFilter(x); }
#endif

    protected slots:

        void refresh();
        void rideSelected();
        void dateRangeChanged(DateRange);
        void rideItemChanged();
        void metadataChanged();

        // date settings
        void useCustomRange(DateRange);
        void useStandardRange();
        void useThruToday();

#ifdef GC_HAVE_LUCENE
        void clearFilter();
        void setFilter(QStringList);
#endif

    protected:

        QString htmlSummary() const;

        Context *context;
        QWebView *rideSummary;

        RideItem *_connected;
        bool ridesummary; // do we summarise ride or daterange?

        QList<SummaryMetrics> data; // when in date range mode
        DateRange current;

        DateSettingsEdit *dateSetting;
        bool useCustom;
        bool useToToday;
        DateRange custom;
#ifdef GC_HAVE_LUCENE
        SearchFilterBox *searchBox;
#endif
        QStringList filters; // empty when no lucene
        bool filtered; // are we using a filter?
};

#endif // _GC_RideSummaryWindow_h

