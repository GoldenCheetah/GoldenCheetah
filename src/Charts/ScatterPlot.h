/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_ScatterPlot_h
#define _GC_ScatterPlot_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QTimer>
#include <QCheckBox>

#include "Context.h"
#include "IntervalItem.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "RideFile.h"
#include "Units.h"
#include "cmath"

#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>
#include <qwt_symbol.h>
#include <qwt_text.h>

#define MODEL_NONE          0
#define MODEL_POWER         1
#define MODEL_CADENCE       2
#define MODEL_HEARTRATE     3
#define MODEL_SPEED         4
#define MODEL_ALT           5
#define MODEL_TORQUE        6
#define MODEL_TIME          7
#define MODEL_DISTANCE      8
#define MODEL_INTERVAL      9
#define MODEL_LAT           10
#define MODEL_LONG          11
#define MODEL_XYTIME        12
#define MODEL_POWERZONE     13
#define MODEL_CPV           14
#define MODEL_AEPF          15
#define MODEL_HEADWIND      16
#define MODEL_SLOPE         17
#define MODEL_TEMP          18
#define MODEL_LRBALANCE     19
#define MODEL_TE            20
#define MODEL_PS            21
#define MODEL_RV            22
#define MODEL_RCAD          23
#define MODEL_RGCT          24
#define MODEL_GEAR          25
#define MODEL_SMO2          26
#define MODEL_THB           27
#define MODEL_HHB           28
#define MODEL_O2HB          29

// the data provider for the plot
class ScatterSettings;

// the core surface plot
class ScatterPlot : public QwtPlot
{
    Q_OBJECT
    G_OBJECT


    public:
        ScatterPlot(Context *);
        void setData(ScatterSettings *);
        void refreshIntervalMarkers(ScatterSettings *);
        void setAxisTitle(int axis, QString label);

    public slots:
        void intervalHover(IntervalItem*);
        void mouseMoved();
        void configChanged(qint32);

    protected:

        // passed from Context *
        Context *context;
        double cranklength;

        QList <QwtPlotCurve *> intervalCurves; // each curve on plot
        QList <QwtPlotMarker *> intervalMarkers; // each marker on plot
        QList <QwtPlotMarker *> labels; // each label on plot

        QwtPlotCurve *curve, *curve2; // when we are plotting l/r curve=left curve2=right
        QwtPlotCurve *hover, *hover2; // similarly for hover
        QwtPlotGrid *grid;

        int curves; // 1 per interval/ride but set to 2 when we have an L/R to plot

    private:
        static QString describeType(int type, bool longer, bool metric);

        void addTrendLine(QVector<double> xval, QVector<double> yval, int nbPoints, QColor intervalColor);

        void smooth(QVector<double> &xval, QVector<double> &yval, int count, int applySmooth);
        void resample(QVector<double> &xval, QVector<double> &yval, int &count, double recInterval, int applySmooth);

        bool skipValues(double xv, double yv, ScatterSettings *settings);

        // save the settings
        RideItem *ride; // what we plotting?
        int xseries,yseries;  // which channels to use
};

// qwt breaks QWidget::mouseMoveEvent, so we have to use their dogshit
// quite whats wrong with the QT event handling is beyond me.
class mouseTracker: public QwtPlotPicker
{
public:

    mouseTracker(ScatterPlot *me) : QwtPlotPicker(me->canvas()), me(me) {
        setTrackerMode(AlwaysOn);
    }

protected:

    virtual QwtText trackerText(const QPoint &) const {
        me->mouseMoved();
        return QwtText("");
    }

private:
    ScatterPlot *me;
};
#endif // _GC_ScatterPlot_h
