/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
 * Copyright (c) 2009 Dan Connelly (@djconnel)
 * Copyright (c) 2014 Damien Grauser (Damien.Grauser@pev-geneve.ch)
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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
#include "PDModel.h"
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
class LogTimeScaleDraw;

#include "PDModel.h" // for all the models

class CPPlot : public QwtPlot
{
    Q_OBJECT
    G_OBJECT

    public:

        CPPlot(CriticalPowerWindow *parent, Context *, bool rangemode);

        // setters
        void setRide(RideItem *rideItem);
        void setDateRange(const QDate &start, const QDate &end, bool stale=false);
        void setShowPercent(bool x);
        void setShowPowerIndex(bool x);
        void setShowTest(bool x);
        void setShowBest(bool x);
        void setFilterBest(bool x);
        void setShowHeat(bool x);
        void setShowEffort(bool x);
        void setShowPP(bool x);
        void setShowHeatByDate(bool x);
        void setShowDelta(bool delta, bool percent);
        void setShadeMode(int x);
        void setShadeIntervals(int x);
        void setVeloCP(int x) { veloCP = x; }
        void setDateCP(int x) { dateCP = x; }
        void setDateCV(double x) { dateCV = x; }
        void setSport(QString s) { sport = s; }
        void setSeries(CriticalPowerWindow::CriticalSeriesType);
        void setPlotType(int index);
        void showXAxisLinear(bool x);
        void setModel(int sanI1, int sanI2, int anI1, int anI2,
                      int aeI1, int aeI2, int laeI1, int laeI2, int model, int variant, int fit, int fitdata, bool modelDecay);

        // getters
        QVector<double> getBests();
        QVector<QDate> getBestDates();
        const QwtPlotCurve *getThisCurve() const { return rideCurve; }
        const QwtPlotCurve *getModelCurve() const { return modelCurve; }
        const QwtPlotCurve *getEffortCurve() const { return effortCurve; }

        // when rides saved/deleted/added CPWindow
        // needs to know what range we have plotted
        // to decide if it needs refreshing
        QDate startDate;
        QDate endDate;

        // saving the data
        void exportBests(QString filename);

    public slots:

        // colors/appearance changed
        void configChanged(qint32);

        // the picker hovered over a point on the curve
        void pointHover(QwtPlotCurve *curve, int index);

        // filter being applied
        void perspectiveFilterChanged();
        void clearFilter();
        void setFilter(QStringList);

        // during a refresh we get a chance to replot
        void refreshUpdate(QDate);
        void refreshEnd();

    private:

        CriticalPowerWindow *parent;

        // calculate / data setting
        void calculateForDateRanges(QList<CompareDateRange> compareDateRanges);
        void calculateForIntervals(QList<CompareInterval> compareIntervals);

        // plotters
        void plotRide(RideItem *);
        void plotBests(RideItem *);
        void plotTests(RideItem *);
        void plotEfforts();
        void plotModel();
        void plotPowerProfile();
        void plotLinearWorkModel();
        void plotModel(QVector<double> vector, QColor plotColor, PDModel *baseline); // for compare date range models
        void updateModelHelper();   // overlay window with parameter estimates from fit
        void plotCentile(RideItem *);
        void plotCache(QVector<double> vector, QColor plotColor);

        void initModel();

        // utility
        void clearCurves();
        //QStringList filterForSeason(QStringList cpints, QDate startDate, QDate endDate);
        void setAxisTitle(int axis, QString label);
        void refreshReferenceLines(RideItem*);
        QString kphToString(double kph);
        QString kmToString(double km);

        // Models and Extended Models
        int model, modelVariant;
        int fit, fitdata;
        bool modelDecay;
        double sanI1, sanI2, anI1, anI2, aeI1, aeI2, laeI1, laeI2;

        // Data and State
        Context *context;
        RideFileCache *bestsCache;
        int veloCP;
        int dateCP;
        double dateCV;
        QString sport;
        QTime lastupdate;

        // settings
        RideFile::SeriesType rideSeries;
        CriticalPowerWindow::CriticalSeriesType criticalSeries;
        QStringList files;
        bool isFiltered;
        int shadeMode;
        bool shadeIntervals;
        bool rangemode;
        bool showTest;
        bool showBest;
        bool filterBest;
        bool showPowerIndex;
        bool showPercent;
        bool showHeat;
        bool showEffort;
        bool showPP;
        bool showHeatByDate;
        bool showDelta; // only in compare mode
        bool showDeltaPercent; // only in compare mode
        double shadingCP; // the CP value we use to draw the shade
        int plotType;
        bool xAxisLinearOnSpeed;

        // Curves
        QList<QwtPlotCurve*> bestsCurves;
        QList<QwtPlotCurve*> centileCurves;
        QList<QwtPlotCurve*> intervalCurves;
        QList<QwtPlotCurve*> profileCurves;

        QList<QwtPlotCurve*> modelCurves;
        QList<QwtPlotIntervalCurve*> modelIntCurves;
        QList<CpPlotCurve*> modelCPCurves;

        QwtPlotCurve *rideCurve;
        QwtPlotCurve *modelCurve;

        QwtPlotCurve *effortCurve;
        QwtPlotCurve *heatCurve;
        CpPlotCurve *heatAgeCurve;

        QwtPlotCurve *workModelCurve;

        // other plot objects
        QList<QwtPlotMarker*> referenceLines;
        QList<QwtPlotMarker*> allZoneLabels;
        QList<QwtPlotMarker*> cherries;
        QList<QwtPlotMarker*> performanceTests;

        LogTimeScaleDraw *ltsd;
        QwtScaleDraw *sd;

        // tooltip / zooming
        LTMCanvasPicker *canvasPicker;
        penTooltip *zoomer;

        // the model
        PDModel *pdModel;

        // filtered data
        QVector<double> filtertime, filterpower;

        // performance tests data
        QVector<double> testtime, testpower;

        // remember the ymax we computed
        double ymax;
};
#endif // _GC_CPPlot_h
