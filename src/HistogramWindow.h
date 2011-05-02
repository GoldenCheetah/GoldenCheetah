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

#include <QtGui>

class MainWindow;
class PowerHist;
class RideItem;
class RideFileCache;

class HistogramWindow : public GcWindow
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
    Q_PROPERTY(bool scope READ scope WRITE setScope USER true)

    public:

        HistogramWindow(MainWindow *mainWindow);

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
        int scope() const { return seasonCombo->currentIndex(); }
        void setScope(int x) { seasonCombo->setCurrentIndex(x); }

    public slots:

        void rideSelected();
        void intervalSelected();
        void zonesChanged();

    protected slots:

        void setBinWidthFromSlider();
        void setBinWidthFromLineEdit();
        void seasonSelected(int season);
        void updateChart();

    private:

        QList<Season> seasons;
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
        QComboBox *seasonCombo;         // Plot for Date range or current ride

        QList<RideFile::SeriesType> seriesList;
        void addSeasons();
        void addSeries();

        int powerRange, hrRange;

        RideFileCache *source;
        bool interval;
};

#endif // _GC_HistogramWindow_h
