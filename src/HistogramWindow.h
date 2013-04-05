/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
 *               2011 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_HistogramWindow_h
#define _GC_HistogramWindow_h 1
#include "GoldenCheetah.h"

#include "Season.h"
#include "SeasonParser.h"
#ifdef GC_HAVE_LUCENE
#include "SearchFilterBox.h"
#endif

#include <QtGui>

class MainWindow;
class PowerHist;
class RideItem;
class RideFileCache;

class HistogramWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int series READ series WRITE setSeries USER true)
    Q_PROPERTY(int percent READ percent WRITE setPercent USER true)
    Q_PROPERTY(double bin READ bin WRITE setBin USER true)
    Q_PROPERTY(bool logY READ logY WRITE setLogY USER true)
    Q_PROPERTY(bool zeroes READ zeroes WRITE setZeroes USER true)
    Q_PROPERTY(bool shade READ shade WRITE setShade USER true)
    Q_PROPERTY(bool zoned READ zoned WRITE setZoned USER true)
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

        HistogramWindow(MainWindow *mainWindow, bool rangemode = false);

        // reveal
        bool hasReveal() { return true; }

        // get/set properties
        int series() const { return seriesCombo->currentIndex(); }
        void setSeries(int x) { seriesCombo->setCurrentIndex(x); }
        int percent() const { return showSumY->currentIndex(); }
        void setPercent(int x) { showSumY->setCurrentIndex(x); }
        double bin() const { return binWidthSlider->value(); }
        void setBin(double x) { binWidthSlider->setValue(x); }
        bool logY() const { return showLnY->isChecked(); }
        void setLogY(bool x) { showLnY->setChecked(x); }
        bool zeroes() const { return showZeroes->isChecked(); }
        void setZeroes(bool x) { showZeroes->setChecked(x); }
        bool shade() const { return shadeZones->isChecked(); }
        void setShade(bool x) { shadeZones->setChecked(x); }
        bool zoned() const { return showInZones->isChecked(); }
        void setZoned(bool x) { return showInZones->setChecked(x); }
#ifdef GC_HAVE_LUCENE
        QString filter() const { return searchBox->filter(); }
        void setFilter(QString x) { searchBox->setFilter(x); }
#endif
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

    public slots:

        void rideSelected();
        void rideAddorRemove(RideItem*);
        void intervalSelected();
        void zonesChanged();
#ifdef GC_HAVE_LUCENE
        void clearFilter();
        void setFilter(QStringList files);
#endif
        // date settings
        void useCustomRange(DateRange);
        void useStandardRange();
        void useThruToday();
        void dateRangeChanged(DateRange);

        void setZoned(int);
        void setShade(int);

    protected slots:

        void setrBinWidthFromSlider();
        void setrBinWidthFromLineEdit();
        void setBinWidthFromSlider();
        void setBinWidthFromLineEdit();
        void forceReplot();
        void updateChart();

    private:

        void setHistTextValidator();
        void setHistBinWidthText();

        MainWindow *mainWindow;
        PowerHist *powerHist;

        QSlider *binWidthSlider;        // seet Bin Width from a slider
        QLineEdit *binWidthLineEdit;    // set Bin Width from the line edit
        QCheckBox *showLnY;     // set show as Log(y)
        QCheckBox *showZeroes;   // Include zeroes
        QComboBox *showSumY;            // ??
        QCheckBox *shadeZones;      // Shade zone background
        QCheckBox *showInZones;       // Plot by Zone
        QComboBox *seriesCombo;         // Which data series to plot

        // reveal controls
        QLabel *rWidth;
        QLineEdit *rBinEdit;    // set Bin Width from the line edit
        QSlider *rBinSlider;        // seet Bin Width from a slider
        QCheckBox *rShade, *rZones;

        QList<RideFile::SeriesType> seriesList;
        void addSeries();

        int powerRange, hrRange;

        bool stale;
        QDate cfrom, cto;
        RideFileCache *source;
        bool interval;
#ifdef GC_HAVE_LUCENE
        SearchFilterBox *searchBox;
        bool isFiltered;
        QStringList files;
#endif

        bool rangemode;
        DateSettingsEdit *dateSetting;
        bool useCustom;
        bool useToToday;
        DateRange custom;
};

#endif // _GC_HistogramWindow_h
