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

#ifndef _GC_CpintPlot_h
#define _GC_CpintPlot_h 1
#include "GoldenCheetah.h"

#include "RideFileCache.h"
#include "ExtendedCriticalPower.h"
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_marker.h>
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

class penTooltip: public QwtPlotZoomer
{
    public:
         penTooltip(QWidget *canvas):
             QwtPlotZoomer(canvas), tip("")
         {
                 // With some versions of Qt/Qwt, setting this to AlwaysOn
                 // causes an infinite recursion.
                 //setTrackerMode(AlwaysOn);
                 setTrackerMode(AlwaysOn);
         }

    virtual QwtText trackerText(const QPoint &/*pos*/) const
    {
        QColor bg = QColor(255,255, 170); // toolyip yellow
#if QT_VERSION >= 0x040300
        bg.setAlpha(200);
#endif
        QwtText text;
        QFont def;
        //def.setPointSize(8); // too small on low res displays (Mac)
        //double val = ceil(pos.y()*100) / 100; // round to 2 decimal place
        //text.setText(QString("%1 %2").arg(val).arg(format), QwtText::PlainText);
        text.setText(tip);
        text.setFont(def);
        text.setBackgroundBrush( QBrush( bg ));
        text.setRenderFlags(Qt::AlignLeft | Qt::AlignTop);
        return text;
    }
    void setFormat(QString fmt) { format = fmt; }
    void setText(QString txt) { tip = txt; }
    private:
    QString format;
    QString tip;
};

class CpintPlot : public QwtPlot
{
    Q_OBJECT
    G_OBJECT


    public:

        CpintPlot(Context *, QString path, const Zones *zones, bool rangemode);

        const QwtPlotCurve *getThisCurve() const { return thisCurve; }
        const QwtPlotCurve *getCPCurve() const { return CPCurve; }

        void setModel(int sanI1, int sanI2, int anI1, int anI2, int aeI1, int aeI2, int laeI1, int laeI2, int model);

        // model type & intervals
        int model;
        double sanI1, sanI2, anI1, anI2, aeI1, aeI2, laeI1, laeI2;

        double cp, tau, t0; // CP model parameters

        Model_eCP athleteModeleCP2;
        Model_eCP athleteModeleCP4;
        Model_eCP athleteModeleCP5;
        Model_eCP athleteModeleCP6;

        double shadingCP; // the CP value we use to draw the shade
        void deriveCPParameters();
        void deriveExtendedCPParameters();

        void changeSeason(const QDate &start, const QDate &end);
        void setAxisTitle(int axis, QString label);
        void setSeries(RideFile::SeriesType);

        QVector<double> getBests() { return bests->meanMaxArray(series); }
        QVector<QDate> getBestDates() { return bests->meanMaxDates(series); }

        QDate startDate;
        QDate endDate;

    public slots:

        void showGrid(int state);
        void calculate(RideItem *rideItem);
        void plot_CP_curve(CpintPlot *plot, double cp, double tau, double t0n);
        void plot_allCurve(CpintPlot *plot, int n_values, const double *power_values, QColor plotColor, bool forcePlotColor);
        void plot_interval(CpintPlot *plot, QVector<double> vector, QColor plotColor);
        void configChanged();
        void pointHover(QwtPlotCurve *curve, int index);
        void setShadeMode(int x);
        void setShadeIntervals(int x);
        void setDateCP(int x) { dateCP = x; }
        void clearFilter();
        void setFilter(QStringList);
        void setRidePlotStyle(int index);

        void calculateForDateRanges(QList<CompareDateRange> compareDateRanges);
        void calculateForIntervals(QList<CompareInterval> compareIntervals);

    protected:

        friend class ::CriticalPowerWindow;

        QString path;
        QwtPlotCurve *thisCurve;
        QwtPlotCurve *CPCurve, *extendedCPCurve2, *extendedCPCurve4, *extendedCPCurve5, *extendedCPCurve6;

        QwtPlotIntervalCurve *extendedCPCurve_WSecond, *extendedCPCurve_WPrime, *extendedCPCurve_CP, *extendedCPCurve_WPrime_CP;

        QList<QwtPlotCurve*> allCurves;
        QwtPlotCurve *allCurve; // bests but not zoned
        QwtPlotMarker *curveTitle;

        QList<QwtPlotMarker*> allZoneLabels;
        void clear_CP_Curves();
        QStringList filterForSeason(QStringList cpints, QDate startDate, QDate endDate);
        QwtPlotGrid *grid;
        const Zones *zones;
        int dateCP;
        RideFile::SeriesType series;
        Context *context;

        void refreshReferenceLines(RideItem*);
        QList<QwtPlotMarker*> referenceLines;

        ExtendedCriticalPower *ecp;

        RideFileCache *current, *bests;
        LTMCanvasPicker *canvasPicker;
        penTooltip *zoomer;

        QStringList files;
        bool isFiltered;
        int shadeMode;
        bool shadeIntervals;
        bool rangemode;

        int ridePlotStyle;
        void calculateCentile(RideItem *rideItem);


};

#endif // _GC_CpintPlot_h

