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

#include <QDebug>
#include <QtGui>
#include <qwt_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_grid.h>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_div.h>
#include <qwt_scale_widget.h>
#include <qwt_symbol.h>
#include "ErgFile.h"

#include "Settings.h"
#include "Colors.h"

#include "RealtimeData.h"


class ErgFileData : public QwtData
{
    public:
    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    virtual QwtData *copy() const ;
    void init() ;
};

class NowData : public QwtData
{
    public:
    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    virtual QwtData *copy() const ;
    void init() ;
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
    QwtArray<double> d_x;
    QwtArray<double> d_y;
};

class DistScaleDraw: public QwtScaleDraw
{
public:
    DistScaleDraw() { }

    // we do 100m for <= a kilo
    virtual QwtText label(double v) const { if (v<1000) return QString("%1").arg(v/1000, 0, 'g', 1);
                                            else return QString("%1").arg(round(v/1000)); }
};
class TimeScaleDraw: public QwtScaleDraw
{
public:
    TimeScaleDraw() { }

    virtual QwtText label(double v) const { return QString("%1").arg(round(v/60000)); }
};

class ErgFilePlot : public QwtPlot
{
    Q_OBJECT
    G_OBJECT


    public:

    ErgFilePlot(QList<ErgFilePoint> * = 0);
    QList<QwtPlotMarker *> Marks;

    void setData(ErgFile *); // set the course
    void reset(); // reset counters etc when plot changes
    void setNow(long); // set point we're add for progress pointer

    public slots:

    void performancePlot(RealtimeData);
    void start();

    private:

    bool bydist;

	QwtPlotGrid *grid;
	QwtPlotCurve *LodCurve;
	QwtPlotCurve *wattsCurve;
	QwtPlotCurve *hrCurve;
	QwtPlotCurve *cadCurve;
	QwtPlotCurve *speedCurve;
	QwtPlotCurve *NowCurve;

    CurveData *wattsData,
              *hrData,
              *cadData,
              *speedData;

    double counter;
    double wattssum,
           hrsum,
           cadsum,
           speedsum;

    ErgFileData lodData;
    NowData nowData;

    DistScaleDraw *distdraw;
    TimeScaleDraw *timedraw;

    ErgFilePlot();

};



#endif

