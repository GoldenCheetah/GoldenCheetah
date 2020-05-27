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

#include <QWebEngineView>
#include <QWebEngineSettings>

#include <QFormLayout>
#include <QtConcurrent>

#include <QPointer>
#include "RideFileCache.h"
#include "ExtendedCriticalPower.h"
#include "SearchFilterBox.h"
#include "Specification.h"

class RideSummaryWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(QString filter READ filter WRITE setFilter USER true)
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
        ~RideSummaryWindow();

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
        bool isFiltered() const { if (!ridesummary) return (filtered || context->ishomefiltered || context->isfiltered);
                                  else return false; }
        // filter
        QString filter() const { return ridesummary ? "" : searchBox->filter(); }
        void setFilter(QString x) { if (!ridesummary) searchBox->setFilter(x); }

        bool isCompare() const { return ((ridesummary && context->isCompareIntervals)
                                      || (!ridesummary && context->isCompareDateRanges)); }

        void getPDEstimates(bool run);

    protected slots:

        void refresh();
        void refresh(QDate);
        void rideSelected();
        void dateRangeChanged(DateRange);
        void rideItemChanged();
        void metadataChanged();
        void intervalsChanged();

        // date settings
        void useCustomRange(DateRange);
        void useStandardRange();
        void useThruToday();

        void clearFilter();
        void setFilter(QStringList);

        // compare mode started or items to compare changed
        void compareChanged();

        // config changed
        void configChanged(qint32);

    signals:

        void doRefresh();

    protected:

        QString htmlSummary();        // summary of a ride or a date range
        QString htmlCompareSummary() const; // comparing intervals or seasons
	static QString addTooltip(QString name, QString tooltip); // adds html tooltip

        Context *context;
        QWebEngineView *rideSummary;

        RideItem *_connected;
        bool justloaded;
        bool firstload;
        bool ridesummary; // do we summarise ride or daterange?

        Specification specification;
        DateRange current;

        DateSettingsEdit *dateSetting;
        bool useCustom;
        bool useToToday;
        DateRange custom;
        SearchFilterBox *searchBox;
        QStringList filters; // empty when no lucene
        bool filtered; // are we using a filter?

        RideFileCache *bestsCache;

        QString WPrimeString, CPString, FTPString, PMaxString;
        QString WPrimeStringWPK, CPStringWPK, FTPStringWPK, PMaxStringWPK;

        bool force; // to force a replot
        QTime lastupdate;

        QFuture<void> future; // used by QtConcurrent
};

#endif // _GC_RideSummaryWindow_h

