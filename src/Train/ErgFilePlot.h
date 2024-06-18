/*
 * Copyright (c) 2009 Mark Liversedge  (liversedge@gmail.com)
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

#ifndef _GC_ErgFilePlot_h
#define _GC_ErgFilePlot_h 1
#include "GoldenCheetah.h"
#include "Context.h"

#include <QDebug>
#include <QtGui>
#include <qwt_series_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_grid.h>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_div.h>
#include <qwt_scale_map.h>
#include <qwt_scale_widget.h>
#include <qwt_symbol.h>
#include "ErgFile.h"
#include "WPrime.h"

#include "Settings.h"
#include "Colors.h"
#include "PowerHist.h" // for penTooltip

#include "RealtimeData.h"
#include <qwt_series_data.h>
#include <qwt_point_data.h>

#define DEFAULT_TAU 450

class ErgFileData : public QwtPointArrayData<double>
{
    public:
        ErgFileData (Context *context) : QwtPointArrayData(QVector<double>(), QVector<double>()), context(context) {}
        double x(size_t i) const;
        double y(size_t i) const;
        size_t size() const;
        void setByDist(bool bd) { bydist = bd; };
        bool byDist() const { return bydist; };

    private:
        Context *context;
        bool bydist = false;

        virtual QPointF sample(size_t i) const;
        virtual QRectF boundingRect() const;
};


class NowData : public QwtPointArrayData<double>
{
    public:
        NowData (Context *context) : QwtPointArrayData(QVector<double>(), QVector<double>()), context(context) {}
        double x(size_t i) const;
        double y(size_t i) const;
        size_t size() const;
        void setByDist(bool bd) { bydist = bd; };
        bool byDist() const { return bydist; };

        void init();

    private:
        Context *context;
        bool bydist = false;

        virtual QPointF sample(size_t i) const;
        //virtual QRectF boundingRect() const;
};


// incremental data, for each curve
class CurveData
{
    // A container class for growing data
    public:

        CurveData();

        void append(double *x, double *y, int count);
        void clear();

        int count() const;
        int size() const;
        const double *x() const;
        const double *y() const;

    private:
        int d_count;
        QVector<double> d_x;
        QVector<double> d_y;
};


class DistScaleDraw: public QwtScaleDraw
{
    public:
        DistScaleDraw() { }

        // we do 100m for <= a kilo
        virtual QwtText label(double v) const { if (v<1000) return QString("%1").arg(v/1000, 0, 'g', 1);
                                                else return QString("%1").arg(round(v/1000)); }
};


class HourTimeScaleDraw: public QwtScaleDraw
{
    public:
        HourTimeScaleDraw() { }

        virtual QwtText label(double v) const {
            v /= 1000;
            QTime t = QTime(0,0,0,0).addSecs(v);
            if (scaleMap().sDist() > 5)
                return t.toString("hh:mm");
            return t.toString("hh:mm:ss");
        }
};

class ErgFilePlot : public QwtPlot
{
    Q_OBJECT
    G_OBJECT

    public:
        ErgFilePlot(Context *context);

        QList<QwtPlotMarker *> Marks;

        void setData(ErgFile *); // set the course
        void reset(); // reset counters etc when plot changes
        void setNow(long); // set point we're add for progress pointer

        bool eventFilter(QObject *obj, QEvent *event);

    public slots:
        void performancePlot(RealtimeData);
        void configChanged(qint32);
        void start();
        void hover(const QPoint &point);
        void startWorkout();
        void stopWorkout();
        void selectCurves();
        void selectTooltip();

        int showColorZones() const;
        void setShowColorZones(int index);
        int showTooltip() const;
        void setShowTooltip(int index);

    private:
        WPrime calculator;
        Context *context;
        bool bydist;
        ErgFile *ergFile;
        QwtPlotMarker *CPMarker;

        int _showColorZones = 0;
        int _showTooltip = 0;

        QwtPlotGrid *grid;
        QList<QwtPlotCurve*> powerSectionCurves;
        QwtPlotCurve *LodCurve;
        QwtPlotCurve *wbalCurve;
        QwtPlotCurve *wbalCurvePredict;
        QwtPlotCurve *wattsCurve;
        QwtPlotCurve *hrCurve;
        QwtPlotCurve *cadCurve;
        QwtPlotCurve *speedCurve;
        QwtPlotCurve *NowCurve;
        QwtPlotCurve *powerHeadroom;

        CurveData *wattsData,
                  *hrData,
                  *cadData,
                  *wbalData,
                  *speedData;

        double counter;
        double wattssum,
               hrsum,
               cadsum,
               wbalsum,
               speedsum;

        ErgFileData *lodData;
        NowData *nowData;

        DistScaleDraw *distdraw;
        HourTimeScaleDraw *timedraw;

        QwtPlotPicker *picker;
        penTooltip *tooltip;
        bool workoutActive = false;

        void highlightSectionCurve(QwtPlotCurve const * const highlightedCurve);
        QString secsToString(int fullSecs) const;
};


#endif

