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

#ifndef _GC_CriticalPowerWindow_h
#define _GC_CriticalPowerWindow_h 1
#include "GoldenCheetah.h"
#include "MainWindow.h" // for isfiltered and filters

#include <QtGui>
#include "Season.h"
#include "RideFile.h"
#ifdef GC_HAVE_LUCENE
#include "SearchFilterBox.h"
#endif

class CpintPlot;
class Context;
class RideItem;
class QwtPlotPicker;

class CriticalPowerWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    // properties can be saved/restored/set by the layout manager

#ifdef GC_HAVE_LUCENE
    Q_PROPERTY(QString filter READ filter WRITE setFilter USER true)
#endif
    Q_PROPERTY(int mode READ mode WRITE setMode USER true)

    // for retro compatibility
    Q_PROPERTY(QString season READ season WRITE setSeason USER true)

    Q_PROPERTY(QDate fromDate READ fromDate WRITE setFromDate USER true)
    Q_PROPERTY(QDate toDate READ toDate WRITE setToDate USER true)
    Q_PROPERTY(QDate startDate READ startDate WRITE setStartDate USER true)
    Q_PROPERTY(int lastN READ lastN WRITE setLastN USER true)
    Q_PROPERTY(int lastNX READ lastNX WRITE setLastNX USER true)
    Q_PROPERTY(int prevN READ prevN WRITE setPrevN USER true)
    Q_PROPERTY(int shading READ shading WRITE setShading USER true)
    Q_PROPERTY(int useSelected READ useSelected WRITE setUseSelected USER true) // !! must be last property !!

    public:

        CriticalPowerWindow(const QDir &home, Context *context, bool range = false);

        // reveal
        bool hasReveal() { return false; }

        void deleteCpiFile(QString filename);

        // set/get properties
        // ---------------------------------------------------
        int mode() const { return seriesCombo->currentIndex(); }
        void setMode(int x) { seriesCombo->setCurrentIndex(x); }

#ifdef GC_HAVE_LUCENE
        // filter
        bool isFiltered() const { return (searchBox->isFiltered() || context->mainWindow->isfiltered); }
        QString filter() const { return searchBox->filter(); }
        void setFilter(QString x) { searchBox->setFilter(x); }
#endif

        // for retro compatibility
        QString season() const { return cComboSeason->itemText(cComboSeason->currentIndex()); }
        void setSeason(QString x) { 
            int index = cComboSeason->findText(x);
            if (index != -1) {
                cComboSeason->setCurrentIndex(index);
                seasonSelected(index);
            }
        }

        RideFile::SeriesType series() { 
            return static_cast<RideFile::SeriesType>
                (seriesCombo->itemData(seriesCombo->currentIndex()).toInt());
        }

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

        int shading() { return shadeCombo->currentIndex(); }
        void setShading(int x) { return shadeCombo->setCurrentIndex(x); }

    protected slots:
        void forceReplot();
        void newRideAdded(RideItem*);
        void cpintTimeValueEntered();
        void pickerMoved(const QPoint &pos);
        void rideSelected();
        void seasonSelected(int season);
        void shadingSelected(int shading);
        void setSeries(int index);
        void resetSeasons();
        void filterChanged();
        void dateRangeChanged(DateRange);

        void useCustomRange(DateRange);
        void useStandardRange();
        void useThruToday();

        void refreshRideSaved();

    private:
        void updateCpint(double minutes);

        QString _dateRange;

    protected:

        QDir home;
        CpintPlot *cpintPlot;
        Context *context;
        QLabel *cpintTimeValue;
        QLabel *cpintTodayValue;
        QLabel *cpintAllValue;
        QLabel *cpintCPValue;
        QComboBox *seriesCombo;
        QComboBox *cComboSeason;
        QComboBox *shadeCombo;
        QwtPlotPicker *picker;
        void addSeries();
        Seasons *seasons;
        QList<Season> seasonsList;
        RideItem *currentRide;
        QList<RideFile::SeriesType> seriesList;
#ifdef GC_HAVE_LUCENE
        SearchFilterBox *searchBox;
#endif

        bool rangemode;
        bool isfiltered;
        QDate cfrom, cto;
        bool stale;
        bool useCustom;
        bool useToToday;
        DateRange custom;

        DateSettingsEdit *dateSetting;
};

#endif // _GC_CriticalPowerWindow_h

