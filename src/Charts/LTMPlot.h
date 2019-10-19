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

#ifndef _GC_LTMPlot_h
#define _GC_LTMPlot_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_draw.h>
#include <qwt_axis_id.h>
#include "IndendPlotMarker.h"

#include "LTMTool.h"
#include "AllPlot.h" // for curve colors widget
#include "LTMSettings.h"
#include "LTMCanvasPicker.h"

#include "Context.h"

class LTMPlotBackground;
class LTMWindow;
class LTMPlotZoneLabel;
class LTMScaleDraw;
class CompareScaleDraw;
class StressCalculator;
class LTMToolTip;

class LTMPlot : public QwtPlot
{
    Q_OBJECT
    G_OBJECT


    public:
        LTMPlot(LTMWindow *, Context *context, int postition=0); // position in a stack
        ~LTMPlot();
        void setData(LTMSettings *);
        void setCompareData(LTMSettings *);
        void setAxisTitle(QwtAxisId axis, QString label);
        static bool isMinutes(QString units);

    public slots:
        void pointHover(QwtPlotCurve*, int);
        void pointClicked(QwtPlotCurve*, int); // point clicked
        void configChanged(qint32);
        bool eventFilter(QObject *, QEvent *);
        virtual void replot();

    protected:
        friend class ::LTMPlotBackground;
        friend class ::LTMPlotZoneLabel;
        friend class ::LTMWindow;

        LTMPlotBackground *bg;
        QList <LTMPlotZoneLabel *> zoneLabels;
        QList <PDModel*>models;

        LTMWindow *parent;
        double minY[10], maxY[10], maxX;      // for all possible 10 curves

        // just to make sure all plots have a common x axis in a stack
        int getMaxX();
        void setMaxX(int x);

        // qwt picker
        LTMToolTip *picker;
        LTMCanvasPicker *_canvasPicker; // allow point selection/hover
        CurveColors *curveColors;

    private:
        Context *context;
        LTMSettings *settings;

        // date range selection
        int selection, seasonid;
        QString name;
        QDate start, end;
        QwtPlotCurve *highlighter;

        // keeping track of axes
        QHash<QString, QwtPlotCurve*> curves; // metric symbol with curve object
        QHash<QString, QwtAxisId> axes;             // units and associated axis
        QList<QObject*> axesObject;
        QList<QwtAxisId> axesId;

        QList<QwtPlotMarker*> labels;                // labels
        LTMScaleDraw *scale;
        QwtPlotGrid *grid;
        QDate firstDate,
              lastDate;

        QHash< QwtPlotCurve *, int> stacks; // map curve to stack #
        QList<QwtIndPlotMarker*> markers; // seasons and events
        QVector< QVector<double>* > stackX;
        QVector< QVector<double>* > stackY;

        int groupForDate(QDate , int);
        void createCurveData(Context *,LTMSettings *, MetricDetail, QVector<double>&, QVector<double>&, int&, bool=false);

        // create curve data from Banister
        void createBanisterData(Context *,LTMSettings *, MetricDetail, QVector<double>&, QVector<double>&, int&, bool=false);

        // create curve data from PMCData
        void createPMCData(Context *,LTMSettings *, MetricDetail, QVector<double>&, QVector<double>&, int&, bool=false);

        // create curve data from estimate
        void createEstimateData(Context *,LTMSettings *, MetricDetail, QVector<double>&, QVector<double>&, int&, bool=false);
        void flushAggregateEstimateData(QVector<double> &x, QVector<double> &y,
                                        QVector<double> &xCount, QVector<double> &yTotal, int &n);

        // create curve data from metadata or metric (from ridecache)
        void createMetricData(Context *,LTMSettings *, MetricDetail, QVector<double>&, QVector<double>&, int&, bool=false);
        void createFormulaData(Context *,LTMSettings *, MetricDetail, QVector<double>&, QVector<double>&, int&, bool=false);

        // create curve data from bests (from ridefile cache)
        void createBestsData(Context *,LTMSettings *, MetricDetail, QVector<double>&, QVector<double>&, int&, bool=false);

        // create curve data from Measure (Body, Hrv)
        void createMeasureData(Context *,LTMSettings *, MetricDetail, QVector<double>&, QVector<double>&, int&, bool=false);

        // create curve data from Measure (Body, Hrv)
        void createPerformanceData(Context *,LTMSettings *, MetricDetail, QVector<double>&, QVector<double>&, int&, bool=false);

        // create a curve based upon TOD
        void createTODCurveData(Context *,LTMSettings *, MetricDetail, QVector<double>&, QVector<double>&, int&, bool=false);

        // create an aggregate
        void aggregateCurves(QVector<double> &a, QVector<double>&w); // aggregate a with w, updates a

        QwtAxisId chooseYAxis(QString);
        void refreshZoneLabels(QwtAxisId);
        void refreshMarkers(LTMSettings *, QDate from, QDate to, int groupby, QColor color);

        QList<QwtAxisId> supportedAxes;
        bool isolation;
        int position, MAXX;
};

class CompareScaleDraw: public QwtScaleDraw
{
    public:
        CompareScaleDraw() {}

    virtual QwtText label(double v) const {

        return QwtText(QString("%1%2").arg(v ? "+" : "").arg(int(v)));
    }
};

// Produce Labels for X-Axis
class LTMScaleDraw: public QwtScaleDraw
{
    public:
    LTMScaleDraw(const QDateTime &base, int startGroup, int groupBy) :
        baseTime(base), groupBy(groupBy), startGroup(startGroup) {
        setTickLength(QwtScaleDiv::MajorTick, 3);
    }

    // used by LTMPopup
    void dateRange(double v, QDate &start, QDate &end) {
        int group = startGroup + (int) v;

        switch (groupBy) {
        case LTM_DAY:
            end = baseTime.addDays((int)v).date();
            start = baseTime.addDays((int)v).date();
            break;

        case LTM_WEEK:
            {
            QDate week = baseTime.date().addDays((int)v*7);
            start = week;
            end = week.addDays(6); // 0-6
            }
            break;

        case LTM_MONTH:
            { // month is count of months since year 0 starting from month 0
                int year=group/12;
                int month=group%12;
                if (!month) { year--; month=12; }
                start = QDate(year, month, 1);
                end = start.addMonths(1).addDays(-1);
            }
            break;

        case LTM_YEAR:
            start = QDate(group, 1, 1);
            end = QDate(group, 12, 31);
            break;

        case LTM_TOD:
            break;

        }
    }

    virtual QwtText label(double v) const {
        int group = startGroup + (int) v;
        QString label;
        QDateTime upTime;

        switch (groupBy) {
        case LTM_DAY:
            upTime = baseTime.addDays((int)v);
            label = upTime.toString(QObject::tr("MMM dd\nyyyy")); /* 2 line display of date - keep \n in translation */
            break;

        case LTM_WEEK:
            {
            QDate week = baseTime.date().addDays((int)v*7);
            label = week.toString(QObject::tr("MMM dd\nyyyy")); /* 2 line display of date - keep \n in translation */
            }
            break;

        case LTM_MONTH:
            { // month is count of months since year 0 starting from month 0
                int year=group/12;
                int month=group%12;
                if (!month) { year--; month=12; }
                label = QString("%1\n%2").arg(QDate::shortMonthName(month)).arg(year);
            }
            break;

        case LTM_YEAR:
            label = QString("%1").arg(group);
            break;

        case LTM_TOD:
            label = QString("%1:00").arg((int)v);
            break;

        case LTM_ALL:
            label = QString(QObject::tr("All"));
            break;

        }
        return label;
    }

    QDate toDate(double v)
    {
        int group = startGroup + (int) v;
        switch (groupBy) {

        default: // meaningless but keeps the compiler happy
        case LTM_DAY:
            return baseTime.addDays((int)v).date();
            break;

        case LTM_WEEK:
            return baseTime.date().addDays((int)v*7);
            break;

        case LTM_MONTH:
        {
            int year=group/12;
            int month=group%12;
            if (!month) { year--; month=12; }
            return QDate(year, month, 1);
            break;
        }
        case LTM_YEAR:
            return QDate(group, 1, 1);
            break;

        case LTM_TOD:
            return QDate::currentDate();
            break;
        }
    }

    private:
        QDateTime baseTime;
        int groupBy, startGroup;
};

#endif // _GC_LTMPlot_h

