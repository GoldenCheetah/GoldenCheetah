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

#include "Context.h"
#include "Athlete.h"
#include "ChartSettings.h"
#include "ColorButton.h"
#include "Colors.h"
#include "PowerHist.h"
#include "RideFile.h"
#include "RideFileCache.h"
#include "RideItem.h"
#include "Settings.h"

#include "Zones.h"
#include "HrZones.h"

#include "Season.h"
#include "SeasonParser.h"

#include "SearchFilterBox.h"

#include "qxtstringspinbox.h"
#include <QtGui>
#include <QCheckBox>
#include <QFormLayout>
#include <QTextEdit>
#include <QHeaderView>

#include <assert.h>

class Context;
class PowerHist;
class RideItem;
class RideFileCache;
class ColorButton;

class HistogramWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int series READ series WRITE setSeries USER true)
    Q_PROPERTY(int percent READ percent WRITE setPercent USER true)
    Q_PROPERTY(bool logY READ logY WRITE setLogY USER true)
    Q_PROPERTY(bool zeroes READ zeroes WRITE setZeroes USER true)
    Q_PROPERTY(bool shade READ shade WRITE setShade USER true)
    Q_PROPERTY(bool zoned READ zoned WRITE setZoned USER true)
    Q_PROPERTY(bool cpZoned READ cpZoned WRITE setCPZoned USER true)
    Q_PROPERTY(bool zoneLimited READ zoneLimited WRITE setZoneLimited USER true)
    Q_PROPERTY(QString filter READ filter WRITE setFilter USER true)
    Q_PROPERTY(QDate fromDate READ fromDate WRITE setFromDate USER true)
    Q_PROPERTY(QDate toDate READ toDate WRITE setToDate USER true)
    Q_PROPERTY(QDate startDate READ startDate WRITE setStartDate USER true)
    Q_PROPERTY(int lastN READ lastN WRITE setLastN USER true)
    Q_PROPERTY(int lastNX READ lastNX WRITE setLastNX USER true)
    Q_PROPERTY(int prevN READ prevN WRITE setPrevN USER true)
    Q_PROPERTY(QString plotColor READ plotColor WRITE setPlotColor USER true)
    Q_PROPERTY(QString distmetric READ distMetric WRITE setDistMetric USER true)
    Q_PROPERTY(QString totalmetric READ totalMetric WRITE setTotalMetric USER true)
    Q_PROPERTY(bool dataMode READ dataMode WRITE setDataMode USER true)
    Q_PROPERTY(double bin READ bin WRITE setBin USER true)
    Q_PROPERTY(int useSelected READ useSelected WRITE setUseSelected USER true) // !! must be last property !!

    public:

        HistogramWindow(Context *context, bool rangemode = false);

        // reveal
        bool hasReveal() { return true; }

        // get/set properties
        int series() const { return seriesCombo->currentIndex(); }
        void setSeries(int x) { seriesCombo->setCurrentIndex(x); rSeriesSelector->setValue(x); }
        int percent() const { return showSumY->currentIndex(); }
        void setPercent(int x) { showSumY->setCurrentIndex(x); }
        double bin() const { return binWidthLineEdit->text().toDouble(); }
        void setBin(double x);
        bool logY() const { return showLnY->isChecked(); }
        void setLogY(bool x) { showLnY->setChecked(x); }
        bool zeroes() const { return showZeroes->isChecked(); }
        void setZeroes(bool x) { showZeroes->setChecked(x); }
        bool shade() const { return shadeZones->isChecked(); }
        void setShade(bool x) { shadeZones->setChecked(x); }
        bool cpZoned() const { return showInCPZones->isChecked(); }
        void setCPZoned(bool x) { return showInCPZones->setChecked(x); }
        bool zoned() const { return showInZones->isChecked(); }
        void setZoned(bool x) { return showInZones->setChecked(x); }
        bool zoneLimited() const { return showZoneLimits->isChecked(); }
        void setZoneLimited(bool x) { return showZoneLimits->setChecked(x); }
        bool isFiltered() const { if (rangemode) return (isfiltered || context->ishomefiltered || context->isfiltered);
                                  else return false; }
        QString filter() const { return searchBox->filter(); }
        void setFilter(QString x) { searchBox->setFilter(x); }

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
        bool dataMode() const;
        void setDataMode(bool);
        QString totalMetric() const;
        void setTotalMetric(QString);
        QString distMetric() const;
        void setDistMetric(QString);
        void setPlotColor(QString);
        QString plotColor() const;

        // for metric or data series
        double getDelta();
        int getDigits();

        // bin width editor
        void setBinEditors();

        // comparing things
        bool isCompare() const;

    public slots:

        void refreshUpdate(QDate);
        void rideSelected();
        void rideAddorRemove(RideItem*);
        void intervalSelected();
        void zonesChanged();
        void clearFilter();
        void setFilter(QStringList files);
        void perspectiveFilterChanged();

        // date settings
        void useCustomRange(DateRange);
        void useStandardRange();
        void useThruToday();
        void dateRangeChanged(DateRange);

        // we changed the series to plot
        void seriesChanged();
        void rSeriesSelectorChanged(int);

        // in rangemode we choose data series or metric
        void metricToggled(bool);
        void dataToggled(bool);
        void switchMode();

        void setZoned(int);
        void setCPZoned(int);
        void setZoneLimited(int);
        void setShade(int);

        // comparing things
        void compareChanged();

        // update on config
        void configChanged(qint32);

    protected slots:

        void setrBinWidthFromSlider();
        void setrBinWidthFromLineEdit();
        void setBinWidthFromSlider();
        void setBinWidthFromLineEdit();
        void forceReplot();
        void updateChart();

        void treeSelectionChanged();

    private:

        Context *context;
        PowerHist *powerHist;

        QSlider *binWidthSlider;        // seet Bin Width from a slider
        QLineEdit *binWidthLineEdit;    // set Bin Width from the line edit
        QCheckBox *showLnY;     // set show as Log(y)
        QCheckBox *showZeroes;   // Include zeroes
        QComboBox *showSumY;            // ??
        QCheckBox *shadeZones;      // Shade zone background
        QCheckBox *showInZones;       // Plot by Zone
        QCheckBox *showInCPZones;       // Plot by CP domain Moderate/Heavy/Severe Zone
        QCheckBox *showZoneLimits;      // Show Zone limits in labels
        QComboBox *seriesCombo;         // Which data series to plot

        // reveal controls
        QLabel *rWidth;
        QLineEdit *rBinEdit;    // set Bin Width from the line edit
        QSlider *rBinSlider;        // seet Bin Width from a slider
        QCheckBox *rShade, *rZones;
        QxtStringSpinBox *rSeriesSelector;

        QList<RideFile::SeriesType> seriesList;
        void addSeries();

        int powerRange, hrRange;

        bool stale;
        QDate cfrom, cto;
        RideFileCache *source;
        bool interval;

        SearchFilterBox *searchBox;
        bool isfiltered;
        QStringList files;

        bool active,  // active switching mode between data series and metric
             bactive; // active setting binwidth
        bool rangemode;
        bool compareStale;
        DateSettingsEdit *dateSetting;
        bool useCustom;
        bool useToToday;
        DateRange custom;
        int precision;

        // labels we need to remember so we can show/hide
        // when switching between data series and range mode
        QLabel *comboLabel, *metricLabel1, *metricLabel2, *showLabel,
               *blankLabel1, *blankLabel2,
               *blankLabel3, *blankLabel4, *blankLabel5, *blankLabel6,
               *blankLabel7, *blankLabel8, *colorLabel;

        // in range mode we can also plot a distribution chart
        // based upon metrics and not just data series
        QRadioButton *data, *metric;

        // total value (y-axis)
        QTreeWidget *totalMetricTree;
        void selectTotal(QString);

        // distribution value (y-axis)
        QTreeWidget *distMetricTree;
        void selectMetric(QString);

        // One shot timer.. delay before refreshing as user
        // scrolls up and down metric/total treewidget. This is
        // to have a slight lag before redrawing since it is expensive
        // and users are likely to move up and down with the arrow keys
        ColorButton *colorButton;
        DateRange last;
        QTime lastupdate;
};

#endif // _GC_HistogramWindow_h
