/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_CPPlot_h
#define _GC_CPPlot_h 1
#include "GoldenCheetah.h"

#include "CriticalPowerWindow.h"
#include "RideFileCache.h"
#include "ExtendedCriticalPower.h"
#include "CpPlotCurve.h"
#include "PowerHist.h" // for penTooltip

#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_spectrocurve.h>
#include <qwt_point_3d.h>
#include <qwt_compat.h>

#include <QtGui>
#include <QMessageBox>

class QwtPlotCurve;
class QwtPlotGrid;
class QwtPlotMarker;
class RideItem;
class Zones;
class Context;
class LTMCanvasPicker;
class CriticalPowerWindow;

class CPPlot : public QwtPlot
{
    Q_OBJECT
    G_OBJECT

    public:

        CPPlot(QWidget *parent, Context *, bool rangemode);

        // setters
        void setDateRange(const QDate &start, const QDate &end);
        void setShowPercent(bool x);
        void setShowHeat(bool x);
        void setShowHeatByDate(bool x);
        void setShadeMode(int x);
        void setShadeIntervals(int x);
        void setDateCP(int x) { dateCP = x; }
        void setSeries(CriticalPowerWindow::CriticalSeriesType);
        void setPlotType(int index);
        void setModel(int sanI1, int sanI2, int anI1, int anI2, 
                      int aeI1, int aeI2, int laeI1, int laeI2, int model);

    public slots:

        // colors/appearance changed
        void configChanged();

        // the picker hovered over a point on the curve
        void pointHover(QwtPlotCurve *curve, int index);

        // filter being applied
        void clearFilter();
        void setFilter(QStringList);

    protected:

        friend class ::CriticalPowerWindow;

        // when rides saved/deleted/added CPWindow
        // needs to know what range we have plotted
        // to decide if it needs refreshing
        QDate startDate;
        QDate endDate;

    private:

        // getters
        QVector<double> getBests() { return bestsCache->meanMaxArray(rideSeries); }
        QVector<QDate> getBestDates() { return bestsCache->meanMaxDates(rideSeries); }
        const QwtPlotCurve *getThisCurve() const { return rideCurve; }
        const QwtPlotCurve *getModelCurve() const { return modelCurve; }

        // calculate / data setting
        void setRide(RideItem *rideItem);
        void calculateForDateRanges(QList<CompareDateRange> compareDateRanges);
        void calculateForIntervals(QList<CompareInterval> compareIntervals);
        void deriveCPParameters();
        void deriveExtendedCPParameters();

        // plotters
        void plotRide(RideItem *); // done :)
        void plotBests(); // done :) (refresh issues)
        void plotModel(); // done :) (refresh issues)
        void plotCentile(RideItem *);
        void plotInterval(QVector<double> vector, QColor plotColor);

        // utility
        void clearCurves();
        QStringList filterForSeason(QStringList cpints, QDate startDate, QDate endDate);
        void setAxisTitle(int axis, QString label);
        void refreshReferenceLines(RideItem*);

        // Models and Extended Models
        int model;
        double sanI1, sanI2, anI1, anI2, aeI1, aeI2, laeI1, laeI2;
        double cp, tau, t0; // CP model parameters

        Model_eCP athleteModeleCP2;
        Model_eCP athleteModeleCP4;
        Model_eCP athleteModeleCP5;
        Model_eCP athleteModeleCP6;
        Model_eCP level14ModeleCP5;
        Model_eCP level15ModeleCP5;
        ExtendedCriticalPower *ecp;

        // Data and State
        Context *context;
        RideFileCache *rideCache, *bestsCache;
        int dateCP;

        // settings
        RideFile::SeriesType rideSeries;
        CriticalPowerWindow::CriticalSeriesType criticalSeries;
        QStringList files;
        bool isFiltered;
        int shadeMode;
        bool shadeIntervals;
        bool rangemode;
        bool showPercent;
        bool showHeat;
        bool showHeatByDate;
        double shadingCP; // the CP value we use to draw the shade
        int plotType;

        // Curves
        QList<QwtPlotCurve*> bestsCurves;
        QList<QwtPlotCurve*> centileCurves;
        QList<QwtPlotCurve*> intervalCurves;

        QwtPlotMarker *curveTitle;
        QwtPlotCurve *modelCurve, *extendedModelCurve2, *extendedModelCurve4, 
                     *extendedModelCurve5, *extendedModelCurve6;
        QwtPlotCurve *heatCurve;
        CpPlotCurve *heatAgeCurve;
        QwtPlotIntervalCurve *extendedModelCurve_WSecond, *extendedModelCurve_WPrime,
                             *extendedModelCurve_CP, *extendedModelCurve_WPrime_CP;
        QwtPlotCurve *level14Curve5, *level15Curve5;
        QwtPlotCurve *rideCurve;

        // other plot objects
        QList<QwtPlotMarker*> referenceLines;
        QList<QwtPlotMarker*> allZoneLabels;
        QwtPlotGrid *grid;

        // tooltip / zooming
        LTMCanvasPicker *canvasPicker;
        penTooltip *zoomer;
};
#endif // _GC_CPPlot_h
