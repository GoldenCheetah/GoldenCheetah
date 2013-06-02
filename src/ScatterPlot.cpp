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

#include "ScatterPlot.h"
#include "ScatterWindow.h"
#include "IntervalItem.h"
#include "MainWindow.h"
#include "Settings.h"
#include "Zones.h"
#include "Colors.h"
#include "RideFile.h"
#include "Units.h" // for MILES_PER_KM

#include <QWidget>
#include <qwt_series_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_canvas.h>
#include <qwt_scale_draw.h>
#include <qwt_symbol.h>

#define PI M_PI

static double
pointType(const RideFilePoint *point, int type, bool useMetricUnits, double cranklength)
{
    // return the point value for the given type
    switch(type) {

        case MODEL_POWER : return point->watts;
        case MODEL_CADENCE : return point->cad;
        case MODEL_HEARTRATE : return point->hr;
        case MODEL_SPEED :
            if (useMetricUnits == true){
                return point->kph;
            }else {
                return point->kph * MILES_PER_KM;
            }
        case MODEL_ALT :
            if (useMetricUnits == true){
                return point->alt;
            }else {
                return point->alt * FEET_PER_METER;
            }
        case MODEL_TORQUE : return point->nm;
        case MODEL_TIME : return point->secs;
        case MODEL_DISTANCE :
            if (useMetricUnits == true){
                return point->km;
            }else {
                return point->km * MILES_PER_KM;
            }
        case MODEL_INTERVAL : return point->interval;
        case MODEL_LAT : return point->lat;
        case MODEL_LONG : return point->lon;
        case MODEL_AEPF :
            if (point->watts == 0 || point->cad == 0) return 0;
            else return ((point->watts * 60.0) / (point->cad * cranklength * 2.0 * PI));
        case MODEL_CPV :
            return ((point->cad * cranklength * 2.0 * PI) / 60.0);
        // these you need to do yourself cause there is some
        // logic needed and I'm just lookup table!
        case MODEL_XYTIME : return 1;
        case MODEL_POWERZONE : return point->watts;
    }
    return 0; // ? unknown channel ?
}

QString ScatterPlot::describeType(int type, bool longer, bool useMetricUnits)
{
    // return the point value for the given type
    if (longer == true) {
        switch(type) {

            case MODEL_POWER : return (tr("Power (watts)"));
            case MODEL_CADENCE : return (tr("Cadence (rpm)"));
            case MODEL_HEARTRATE : return (tr("Heartrate (bpm)"));
            case MODEL_SPEED :
                if (useMetricUnits == true){
                     return (tr("Speed (kph)"));
                }else {
                     return (tr("Speed (mph)"));
                }
            case MODEL_ALT :
                  if (useMetricUnits == true){
                      return (tr("Altitude (meters)"));
                  }else {
                      return (tr("Altitude (feet)"));
                  }
            case MODEL_TORQUE : return (tr("Torque (N)"));
            case MODEL_TIME : return (tr("Elapsed Time (secs)"));
            case MODEL_DISTANCE :
                if (useMetricUnits == true){
                    return (tr("Elapsed Distance (km)"));
                }else {
                    return (tr("Elapsed Distance (mi)"));
                }
            case MODEL_INTERVAL : return (tr("Interval Number"));
            case MODEL_LAT : return (tr("Latitude (degree offset)"));
            case MODEL_LONG : return (tr("Longitude (degree offset)"));
            case MODEL_CPV : return (tr("Circumferential Pedal Velocity (cm/s)"));
            case MODEL_AEPF : return (tr("Average Effective Pedal Force (N)"));

            // these you need to do yourself cause there is some
            // logic needed and I'm just lookup table!
            case MODEL_XYTIME : return (tr("Time at X/Y (%)"));
            case MODEL_POWERZONE : return (tr("Power Zone"));
        }
        return (tr("Unknown"));; // ? unknown channel ?
    } else {
        switch(type) {

            case MODEL_POWER : return (tr("Power"));
            case MODEL_CADENCE : return (tr("Cadence"));
            case MODEL_HEARTRATE : return (tr("Heartrate"));
            case MODEL_SPEED : return (tr("Speed"));
            case MODEL_ALT : return (tr("Altitude"));
            case MODEL_TORQUE : return (tr("Pedal Force"));
            case MODEL_TIME : return (tr("Time"));
            case MODEL_DISTANCE : return (tr("Distance"));
            case MODEL_INTERVAL : return (tr("Interval"));
            case MODEL_LAT : return (tr("Latitude"));
            case MODEL_LONG : return (tr("Longitude"));
            case MODEL_XYTIME : return (tr("Time at X/Y"));
            case MODEL_POWERZONE : return (tr("Zone"));
            case MODEL_CPV : return (tr("CPV"));
            case MODEL_AEPF : return (tr("AEPF"));
        }
        return (tr("None")); // ? unknown channel ?
    }
}

ScatterPlot::ScatterPlot(MainWindow *parent) : main(parent)
{
    setInstanceName("2D Plot");
    all = NULL;
    grid = NULL;
    canvas()->setFrameStyle(QFrame::NoFrame);

    setAxisMaxMinor(xBottom, 0);
    setAxisMaxMinor(yLeft, 0);

    QwtScaleDraw *sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtPlot::xBottom, sd);

    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtPlot::yLeft, sd);

    connect(main, SIGNAL(configChanged()), this, SLOT(configChanged()));
    configChanged(); // use latest colors etc
}

void ScatterPlot::setData (ScatterSettings *settings)
{
    // get application settings
    cranklength = appsettings->value(this, GC_CRANKLENGTH, 0.0).toDouble() / 1000.0;

    // if there are no settings or incomplete settings
    // create a null data plot
    if (settings == NULL || settings->ride == NULL || settings->ride->ride() == NULL ||
        settings->x == 0 || settings->y == 0 ) {
        return;
    }


    // if its not setup or no settings exist default to 175mm cranks
    if (cranklength == 0.0) cranklength = 0.175;

    //
    // Create Main Plot dataset - used to frame intervals
    //
    int points=0;

    x.clear();
    y.clear();
    x.resize(settings->ride->ride()->dataPoints().count());
    y.resize(settings->ride->ride()->dataPoints().count());

    double maxY = maxX = -65535;
    double minY = minX = 65535;

    foreach(const RideFilePoint *point, settings->ride->ride()->dataPoints()) {

        double xv = x[points] = pointType(point, settings->x, main->useMetricUnits, cranklength);
        double yv = y[points] = pointType(point, settings->y, main->useMetricUnits, cranklength);

        // skip zeroes?
        if (!(settings->ignore && (x[points] == 0 || y[points] == 0))) {
            points++;
            if (yv > maxY) maxY = yv;
            if (yv < minY) minY = yv;
            if (xv > maxX) maxX = xv;
            if (xv < minX) minX = xv;
        }
    }

    QwtSymbol sym;
    sym.setStyle(QwtSymbol::Ellipse);
    sym.setSize(6);
    sym.setPen(GCColor::invert(GColor(CPLOTBACKGROUND)));
    sym.setBrush(QBrush(Qt::NoBrush));
    QPen p;
    p.setColor(GColor(CPLOTSYMBOL));
    sym.setPen(p);

    // wipe away existing
	if (all) {
        all->detach();
	    delete all;
    }

    // setup the framing curve
    if (settings->frame) {
        all = new QwtPlotCurve();
        all->setSymbol(new QwtSymbol(sym));
        all->setStyle(QwtPlotCurve::Dots);
        all->setRenderHint(QwtPlotItem::RenderAntialiased);
	    all->setData(x.constData(), y.constData(), points);
        all->attach(this);
    } else {
        all = NULL;
    }

    QPen gridPen(GColor(CPLOTGRID));
    gridPen.setStyle(Qt::DotLine);

    if (grid) {
        grid->detach();
        delete grid;
    }

    if (settings->gridlines) {
        grid = new QwtPlotGrid();
        grid->setPen(gridPen);
        grid->enableX(true);
        grid->enableY(true);
        grid->attach(this);
    } else {
        grid = NULL;
    }

    setAxisTitle(yLeft, describeType(settings->y, true, useMetricUnits));
    setAxisTitle(xBottom, describeType(settings->x, true, useMetricUnits));

    // truncate PfPv values to make easier to read
    if (settings->y == MODEL_AEPF) setAxisScale(yLeft, 0, 600);
    else setAxisScale(yLeft, minY, maxY);
    if (settings->x == MODEL_CPV) setAxisScale(xBottom, 0, 3);
    else setAxisScale(xBottom, minX, maxX);

    //
    // Create Interval Plot dataset - used to frame intervals
    //

    // clear out any interval curves which are presently defined
    if (intervalCurves.size()) {
       QListIterator<QwtPlotCurve *> i(intervalCurves);
       while (i.hasNext()) {
           QwtPlotCurve *curve = i.next();
           curve->detach();
           delete curve;
       }
    }
    intervalCurves.clear();

    // which ones are highlighted then?
    QVector<int> intervals;
    QMap<int,int> displaySequence;

    for (int child=0; child<main->allIntervalItems()->childCount(); child++) {
        IntervalItem *current = dynamic_cast<IntervalItem *>(main->allIntervalItems()->child(child));
        if ((current != NULL) && current->isSelected()) {
            intervals.append(child);
            displaySequence.insert(current->displaySequence, intervals.count()-1);
        }
    }

    if (intervals.count() > 0) {

        // interval data in here
        QVector<QVector<double> > xvals(intervals.count()); // array of curve x arrays
        QVector<QVector<double> > yvals(intervals.count()); // array of curve x arrays
        QVector<int> points(intervals.count());             // points in eac curve

        // extract interval data
        foreach(const RideFilePoint *point, settings->ride->ride()->dataPoints()) {

            double x = pointType(point, settings->x, useMetricUnits, cranklength);
            double y = pointType(point, settings->y, useMetricUnits, cranklength);

            if (!(settings->ignore && (x == 0 && y ==0))) {

                // which interval is it in?
                for (int idx=0; idx<intervals.count(); idx++) {

                    IntervalItem *current = dynamic_cast<IntervalItem *>(main->allIntervalItems()->child(intervals[idx]));

                    if (point->secs+settings->ride->ride()->recIntSecs() > current->start && point->secs< current->stop) {
                        xvals[idx].append(x);
                        yvals[idx].append(y);
                        points[idx]++;
                    }
                }
            }
        }

        // now we have the interval data lets create the curves
        QMapIterator<int, int> order(displaySequence);
        while (order.hasNext()) {
            order.next();
            int idx = order.value();

            QPen pen;
            QColor intervalColor;
            intervalColor.setHsv((255/main->allIntervalItems()->childCount()) * (intervals[idx]), 255,255);
            pen.setColor(intervalColor);
            sym.setPen(pen);

            QwtPlotCurve *curve = new QwtPlotCurve();

            curve->setSymbol(new QwtSymbol(sym));
            curve->setStyle(QwtPlotCurve::Dots);
            curve->setRenderHint(QwtPlotItem::RenderAntialiased);
            curve->setData(xvals[idx].constData(), yvals[idx].constData(), points[idx]);
            curve->attach(this);

            intervalCurves.append(curve);
        }
    }

    replot();
}

void
ScatterPlot::configChanged()
{
    // setColors bg
    setCanvasBackground(GColor(CPLOTBACKGROUND));
}

void
ScatterPlot::showTime(ScatterSettings *settings, int offset, int secs)
{
    static QwtPlotCurve *time = NULL;

    int begin = offset-secs;
    if (begin<0) begin = 0;

    if (time == NULL) {
        time = new QwtPlotCurve();
        time->attach(this);
        replot();
    }

    // offset into data points...
    if (settings && settings->ride && settings->ride->ride()) {
        int startidx = settings->ride->ride()->timeIndex(begin);
        int stopidx = settings->ride->ride()->timeIndex(offset);

        QwtSymbol sym;
        sym.setStyle(QwtSymbol::Rect);
        sym.setSize(6);
        sym.setPen(GCColor::invert(GColor(CPLOTBACKGROUND)));
        sym.setBrush(QBrush(Qt::NoBrush));

        QPen pen;
        pen.setColor(Qt::red);
        sym.setPen(pen);

        time->setSymbol(new QwtSymbol(sym));
        time->setStyle(QwtPlotCurve::Dots);
        time->setRenderHint(QwtPlotItem::RenderAntialiased);
        time->setData(x.constData()+startidx, y.constData()+startidx, stopidx-startidx);

        // make it on top
        time->detach();
        time->attach(this);
        replot(); // ouch really expensive if lots on the plot...
    }
}

void
ScatterPlot::setAxisTitle(int axis, QString label)
{
    // setup the default fonts
    QFont stGiles; // hoho - Chart Font St. Giles ... ok you have to be British to get this joke
    stGiles.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    stGiles.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());

    QwtText title(label);
    title.setFont(stGiles);
    QwtPlot::setAxisFont(axis, stGiles);
    QwtPlot::setAxisTitle(axis, title);
}
