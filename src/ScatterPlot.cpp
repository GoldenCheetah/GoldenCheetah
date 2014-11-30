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
#include "Context.h"
#include "Context.h"
#include "Athlete.h"
#include "Settings.h"
#include "Zones.h"
#include "Colors.h"
#include "RideFile.h"
#include "Units.h" // for MILES_PER_KM

#include <QWidget>
#include <qwt_series_data.h>
#include <qwt_legend.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_canvas.h>
#include <qwt_scale_draw.h>
#include <qwt_symbol.h>

#define PI M_PI

// side 0=left, 1=right, usually 0
static double
pointType(const RideFilePoint *point, int type, int side, bool metric, double cranklength)
{
    // return the point value for the given type
    switch(type) {

        case MODEL_POWER : return point->watts;
        case MODEL_CADENCE : return point->cad;
        case MODEL_HEARTRATE : return point->hr;
        case MODEL_SPEED :
            if (metric == true){
                return point->kph;
            }else {
                return point->kph * MILES_PER_KM;
            }
        case MODEL_ALT :
            if (metric == true){
                return point->alt;
            }else {
                return point->alt * FEET_PER_METER;
            }
        case MODEL_TORQUE : return point->nm;
        case MODEL_TIME : return point->secs;
        case MODEL_DISTANCE :
            if (metric == true){
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
        case MODEL_HEADWIND : return point->headwind;
        case MODEL_SLOPE : return point->slope;
        case MODEL_TEMP : return point->temp;

        case MODEL_LRBALANCE : return !side ? point->lrbalance : (100 - point->lrbalance);
        case MODEL_TE : return side ? point->rte : point->lte;
        case MODEL_PS : return side ? point->rps : point->lps;

        case MODEL_RV : return point->rvert;
        case MODEL_RGCT : return point->rcontact;
        case MODEL_RCAD : return point->rcad;
        case MODEL_GEAR : return point->gear;
        case MODEL_SMO2 : return point->smo2;
        case MODEL_THB : return point->thb;
    }
    return 0; // ? unknown channel ?
}

QString ScatterPlot::describeType(int type, bool longer, bool metric)
{
    // return the point value for the given type
    if (longer == true) {
        switch(type) {

            case MODEL_POWER : return (tr("Power (watts)"));
            case MODEL_CADENCE : return (tr("Cadence (rpm)"));
            case MODEL_HEARTRATE : return (tr("Heartrate (bpm)"));
            case MODEL_SPEED :
                if (metric == true){
                     return (tr("Speed (kph)"));
                }else {
                     return (tr("Speed (mph)"));
                }
            case MODEL_ALT :
                  if (metric == true){
                      return (tr("Altitude (meters)"));
                  }else {
                      return (tr("Altitude (feet)"));
                  }
            case MODEL_TORQUE : return (tr("Torque (N)"));
            case MODEL_TIME : return (tr("Elapsed Time (secs)"));
            case MODEL_DISTANCE :
                if (metric == true){
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
            case MODEL_HEADWIND : return (tr("Headwind (kph)"));
            case MODEL_SLOPE : return (tr("Slope (gradient)"));
            case MODEL_TEMP : return (tr("Temperature (C)"));
            case MODEL_LRBALANCE : return (tr("L/R Balance"));
            case MODEL_TE : return (tr("Torque Efficiency"));
            case MODEL_PS : return (tr("Pedal Smoothness"));
            case MODEL_RV :  return (tr("Running Vertical Oscillation"));
            case MODEL_RGCT : return (tr("Running Ground Contact Time"));
            case MODEL_RCAD :  return (tr("Running Cadence"));
            case MODEL_GEAR :  return (tr("Gear Ratio"));
            case MODEL_SMO2 :  return (tr("Muscle Oxygen"));
            case MODEL_THB :  return (tr("Haemoglobin Mass"));
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
            case MODEL_HEADWIND : return (tr("Headwind"));
            case MODEL_SLOPE : return (tr("Slope"));
            case MODEL_TEMP : return (tr("Temp"));
            case MODEL_LRBALANCE : return (tr("Balance"));
            case MODEL_TE : return (tr("TE"));
            case MODEL_PS : return (tr("PS"));
            case MODEL_RV :  return (tr("RV"));
            case MODEL_RGCT : return (tr("GCT"));
            case MODEL_RCAD :  return (tr("Run Cad"));
            case MODEL_GEAR :  return (tr("Gear"));
            case MODEL_SMO2 :  return (tr("SmO2"));
            case MODEL_THB :  return (tr("tHb"));
        }
        return (tr("None")); // ? unknown channel ?
    }
}

ScatterPlot::ScatterPlot(Context *context) : context(context)
{
    setAutoDelete(false); // no don't delete on detach !
    curve = NULL;
    curve2 = NULL;
    hover = NULL;
    hover2 = NULL;
    grid = NULL;
    ride = NULL;
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

    setAxisMaxMinor(xBottom, 0);
    setAxisMaxMinor(yLeft, 0);

    QwtScaleDraw *sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtPlot::xBottom, sd);

    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    setAxisScaleDraw(QwtPlot::yLeft, sd);

    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(context, SIGNAL(intervalHover(RideFileInterval)), this, SLOT(intervalHover(RideFileInterval)));

    // lets watch the mouse move...
    new mouseTracker(this);

    configChanged(); // use latest colors etc
}

void ScatterPlot::setData (ScatterSettings *settings)
{

    // if there are no settings or incomplete settings
    // create a null data plot
    if (settings == NULL || settings->ride == NULL || settings->ride->ride() == NULL ||
        settings->x == 0 || settings->y == 0 ) {
        ride = NULL;
        return;
    } else {
        ride = settings->ride;
    }

    // clear the hover curve
    if (hover) {
        hover->detach();
        delete hover;
        hover = NULL;
    }
    // wipe away existing curves -- we decide what to do below
	if (curve) {
        curve->detach();
	    delete curve;
        curve = NULL;
    }
	if (curve2) {
        curve2->detach();
	    delete curve2;
        curve2 = NULL;
    }
    // clear out any interval curves
    foreach(QwtPlotCurve *c, intervalCurves) {
        c->detach();
        delete c;
    }
    intervalCurves.clear();

    // count the currently selected intervals
    QVector<int> intervals;
    QMap<int,int> displaySequence;

    for (int child=0; child<context->athlete->allIntervalItems()->childCount(); child++) {
        IntervalItem *current = dynamic_cast<IntervalItem *>(context->athlete->allIntervalItems()->child(child));
        if ((current != NULL) && current->isSelected()) {
            intervals.append(child);
            displaySequence.insert(current->displaySequence, intervals.count()-1);
        }
    }

    // what are the settings
    xseries = settings->x;
    yseries = settings->y;

    // for setting boundaries of axis
    double minX, maxX;
    double minY, maxY;
    maxY = maxX = -65535;
    minY = minX = 65535;

    // get application settings
    cranklength = appsettings->value(this, GC_CRANKLENGTH, 175.00).toDouble() / 1000.0;

    // how many curves do we need ?
    if (xseries == MODEL_LRBALANCE || xseries == MODEL_TE || xseries == MODEL_PS ||
        yseries == MODEL_LRBALANCE || yseries == MODEL_TE || yseries == MODEL_PS) {

        // left and right side needed for each ride/interval
        curves = 2;
    } else {

        // just one curve per ride/interval
        curves = 1;
    }

    for (int side = 0; side < curves; side++) {

        if (context->isCompareIntervals == false) {
            //
            // GET ALL DATA AND MIN / MAX
            //
            int points=0;
            QVector<double> x;
            QVector<double> y;

            foreach(const RideFilePoint *point, settings->ride->ride()->dataPoints()) {

                double xv = pointType(point, settings->x, side, context->athlete->useMetricUnits, cranklength);
                double yv = pointType(point, settings->y, side, context->athlete->useMetricUnits, cranklength);

                // skip zeroes? - special logic for Model Gear, since there value between 0.01 and 1 happen and are relevant
                if ((settings->x != MODEL_GEAR && settings->y != MODEL_GEAR)
                     && settings->ignore && (int(xv) == 0 || int(yv) == 0)) continue;
                if ((settings->x == MODEL_GEAR)
                     && settings->ignore && (xv == 0.0f || int(yv) == 0)) continue;
                if ((settings->y == MODEL_GEAR)
                     && settings->ignore && (int(xv) == 0 || yv == 0.0f)) continue;

                // add it
                x <<xv;
                y <<yv;
                points++;

                if (yv > maxY) maxY = yv;
                if (yv < minY) minY = yv;
                if (xv > maxX) maxX = xv;
                if (xv < minX) minX = xv;
            }

            //
            // CREATE INTERVAL CURVES (and use framing data from above if needed)
            //
            if (intervals.count() > 0) {

                // interval data in here
                QVector<QVector<double> > xvals(intervals.count()); // array of curve x arrays
                QVector<QVector<double> > yvals(intervals.count()); // array of curve x arrays
                QVector<int> points(intervals.count());             // points in eac curve

                // extract interval data
                foreach(const RideFilePoint *point, settings->ride->ride()->dataPoints()) {

                    double x = pointType(point, settings->x, side, context->athlete->useMetricUnits, cranklength);
                    double y = pointType(point, settings->y, side, context->athlete->useMetricUnits, cranklength);

                    if (!(settings->ignore && (x == 0 && y ==0))) {

                        // which interval is it in?
                        for (int idx=0; idx<intervals.count(); idx++) {

                            IntervalItem *current = dynamic_cast<IntervalItem *>(context->athlete->allIntervalItems()->child(intervals[idx]));

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

                    QColor intervalColor;
                    intervalColor.setHsv((255/context->athlete->allIntervalItems()->childCount()) * (intervals[idx]), 255,255);
                    // left / right are darker lighter
                    if (side) intervalColor = intervalColor.lighter(50);

                    QwtSymbol *sym = new QwtSymbol;
                    sym->setStyle(QwtSymbol::Ellipse);
                    sym->setSize(4);
                    sym->setBrush(QBrush(intervalColor));
                    sym->setPen(QPen(intervalColor));

                    QwtPlotCurve *ic = new QwtPlotCurve();

                    ic->setSymbol(sym);
                    ic->setStyle(QwtPlotCurve::Dots);
                    ic->setRenderHint(QwtPlotItem::RenderAntialiased);
                    ic->setSamples(xvals[idx].constData(), yvals[idx].constData(), points[idx]);
                    ic->attach(this);

                    intervalCurves.append(ic);
                }
            }

            // setup the framing curve
            if (intervals.count() == 0 || settings->frame) {

                if (side) {

                    QwtSymbol *sym = new QwtSymbol;
                    sym->setStyle(QwtSymbol::Ellipse);
                    sym->setSize(4);
                    sym->setPen(QPen(Qt::cyan));
                    sym->setBrush(QBrush(Qt::cyan));

                    curve2 = new QwtPlotCurve();
                    curve2->setSymbol(sym);
                    curve2->setStyle(QwtPlotCurve::Dots);
                    curve2->setRenderHint(QwtPlotItem::RenderAntialiased);
                    curve2->setSamples(x.constData(), y.constData(), points);
                    curve2->setZ(-1);
                    curve2->attach(this);

                } else {

                    QwtSymbol *sym = new QwtSymbol;
                    sym->setStyle(QwtSymbol::Ellipse);
                    sym->setSize(4);
                    sym->setPen(QPen(Qt::red));
                    sym->setBrush(QBrush(Qt::red));

                    curve = new QwtPlotCurve();
                    curve->setSymbol(sym);
                    curve->setStyle(QwtPlotCurve::Dots);
                    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
                    curve->setSamples(x.constData(), y.constData(), points);
                    curve->setZ(-1);
                    curve->attach(this);
                }
            }
        }
        else {

            if (context->compareIntervals.count() > 0) {

                //
                // CREATE COMPARE CURVES
                //

                for (int i=0; i< context->compareIntervals.count(); i++) {
                    CompareInterval interval = context->compareIntervals.at(i);

                    if (interval.checked == false)
                        continue;

                    // interval data in here
                    QVector<double> xval;
                    QVector<double> yval;
                    int nbPoints = 0;

                    // extract interval data
                    foreach(const RideFilePoint *point, interval.data->dataPoints()) {

                        double x = pointType(point, settings->x, side, context->athlete->useMetricUnits, cranklength);
                        double y = pointType(point, settings->y, side, context->athlete->useMetricUnits, cranklength);

                        if (y > maxY) maxY = y;
                        if (y < minY) minY = y;
                        if (x > maxX) maxX = x;
                        if (x < minX) minX = x;

                        if (!(settings->ignore && (x == 0 && y ==0))) {
                            xval.append(x);
                            yval.append(y);
                            nbPoints++;
                        }
                    }

                    QColor intervalColor = interval.color;
                    // left / right are darker lighter
                    if (side) intervalColor = intervalColor.lighter(50);

                    QwtSymbol *sym = new QwtSymbol;
                    sym->setStyle(QwtSymbol::Ellipse);
                    sym->setSize(4);
                    sym->setBrush(QBrush(intervalColor));
                    sym->setPen(QPen(intervalColor));

                    QwtPlotCurve *ic = new QwtPlotCurve();

                    ic->setSymbol(sym);
                    ic->setStyle(QwtPlotCurve::Dots);
                    ic->setRenderHint(QwtPlotItem::RenderAntialiased);
                    ic->setSamples(xval.constData(), yval.constData(), nbPoints);
                    ic->attach(this);

                    intervalCurves.append(ic);
                }
            }
        }
    }

    // redraw grid 
    if (grid) {
        grid->detach();
        delete grid;
    }

    if (settings->gridlines) {

        QPen gridPen(GColor(CPLOTGRID));
        grid = new QwtPlotGrid();
        grid->setPen(gridPen);
        grid->enableX(true);
        grid->enableY(true);
        grid->setZ(-10);
        grid->attach(this);
    } else {
        grid = NULL;
    }

    // axis titles
    setAxisTitle(yLeft, describeType(settings->y, true, context->athlete->useMetricUnits));
    setAxisTitle(xBottom, describeType(settings->x, true, context->athlete->useMetricUnits));

    // axis scale
    if (settings->y == MODEL_AEPF) setAxisScale(yLeft, 0, 600);
    else setAxisScale(yLeft, minY, maxY);
    if (settings->x == MODEL_CPV) setAxisScale(xBottom, 0, 3);
    else setAxisScale(xBottom, minX, maxX);

    // gear
    if (settings->x == MODEL_GEAR) setAxisScale(xBottom, 0, 7);
    if (settings->y == MODEL_GEAR) setAxisScale(yLeft, 0, 7);


    // and those interval markers
    refreshIntervalMarkers(settings);

    replot();
}


void ScatterPlot::mouseMoved()
{
    if (!isVisible()) return;

    if (ride && ride->ride() && ride->ride()->intervals().count() >= intervalMarkers.count()) {

        // where is the mouse ?
        QPoint pos = QCursor::pos();

        // are we hovering "close" to an interval marker ?
        int index = 0;
        foreach (QwtPlotMarker *is, intervalMarkers) {

            QPoint mpos = canvas()->mapToGlobal(QPoint(transform(xBottom, is->value().x()), transform(yLeft, is->value().y())));

            int dx = mpos.x() - pos.x();
            int dy = mpos.y() - pos.y();

            if ((dx > -6 && dx < 6) && (dy > -6 && dy < 6))
                context->notifyIntervalHover(ride->ride()->intervals()[index]);

            index++;
        }
    }
    return;
}

void
ScatterPlot::intervalHover(RideFileInterval ri)
{
    if (!isVisible()) return;
    if (context->isCompareIntervals) return;
    if (!ride) return;
    if (!ride->ride()) return;

    // zap the old one
    if (hover) {
        hover->detach();
        delete hover;
        hover = NULL;
    }
    if (hover2) {
        hover2->detach();
        delete hover2;
        hover2 = NULL;
    }

    // how many curves do we need ?
    if (xseries == MODEL_LRBALANCE || xseries == MODEL_TE || xseries == MODEL_PS ||
        yseries == MODEL_LRBALANCE || yseries == MODEL_TE || yseries == MODEL_PS) {

        // left and right side needed for each ride/interval
        curves = 2;
    } else {

        // just one curve per ride/interval
        curves = 1;
    }

    for (int side = 0; side < curves; side++) {

        // collect the data
        QVector<double> xArray, yArray;
        foreach(const RideFilePoint *p1, ride->ride()->dataPoints()) {

            double y = pointType(p1, xseries, side, context->athlete->useMetricUnits, cranklength);
            double x = pointType(p1, yseries, side, context->athlete->useMetricUnits, cranklength);

            if (p1->secs < ri.start || p1->secs > ri.stop) continue;

            xArray << x;
            yArray << y;
        }

        // which interval is it or how many ?
        int count = 0;
        int ours = 0;
        foreach(RideFileInterval p, ride->ride()->intervals()) {
            if (p.start == ri.start && p.stop == ri.stop) ours = count;
            count++;
        }

        // any data ?
        if (xArray.size()) {
            QwtSymbol *sym = new QwtSymbol;
            sym->setStyle(QwtSymbol::Ellipse);
            sym->setSize(4);
            QColor color;
            color.setHsv(ours * 255/count, 255,255);
            color.setAlpha(128);

            // lighter for rhs
            if (side) color = color.lighter(50);

            sym->setPen(QPen(color));
            sym->setBrush(QBrush(color));
   
            if (side) { 
                hover2 = new QwtPlotCurve();
                hover2->setSymbol(sym);
                hover2->setStyle(QwtPlotCurve::Dots);
                hover2->setRenderHint(QwtPlotItem::RenderAntialiased);
                hover2->setSamples(yArray, xArray);
                hover2->attach(this);
            } else {
                hover = new QwtPlotCurve();
                hover->setSymbol(sym);
                hover->setStyle(QwtPlotCurve::Dots);
                hover->setRenderHint(QwtPlotItem::RenderAntialiased);
                hover->setSamples(yArray, xArray);
                hover->attach(this);
            }
        }
    }

    replot(); // refresh
}

// for accumulating averages when refreshing interval markers
class saccum { public: saccum() : count(0), x(0), y(0) {} int count; double x; double y; };

void
ScatterPlot::refreshIntervalMarkers(ScatterSettings *settings)
{
    // zap what we got ...
    foreach(QwtPlotMarker *is, intervalMarkers) {
        is->detach();
        delete is;
    }
    intervalMarkers.clear();

    if (context->isCompareIntervals) return;

    // do we have a ride with intervals to refresh ?
    int count=0;

    if (settings->ride && settings->ride->ride() && settings->ride->ride()->dataPoints().count() && (count = settings->ride->ride()->intervals().count())) {

        // accumulating...
        QVector<saccum> intervalAccumulator(count);

        foreach (RideFilePoint *point, settings->ride->ride()->dataPoints()) {

            double x0 = pointType(point, settings->x, 0, context->athlete->useMetricUnits, cranklength);
            double y0 = pointType(point, settings->y, 0, context->athlete->useMetricUnits, cranklength);
            double x1 = pointType(point, settings->x, 1, context->athlete->useMetricUnits, cranklength);
            double y1 = pointType(point, settings->y, 1, context->athlete->useMetricUnits, cranklength);

            // average of left and right (even if both the same)
            double x = (x0 + x1) / 2.00f;
            double y = (y0 + y1) / 2.00f;

            // accumulate values for each interval here ....
            for(int i=0; i < count; i++) {

                RideFileInterval v = settings->ride->ride()->intervals()[i];

                // in our interval ?
                if (point->secs >= v.start && point->secs <= v.stop) {
                    intervalAccumulator[i].x += x;
                    intervalAccumulator[i].y += y;
                    intervalAccumulator[i].count++;
                }
            }
        }

        // ok, so now add markers for the average position for each interval
        int index = 0;
        foreach (saccum a, intervalAccumulator) {

            QColor color;
            color.setHsv(index * 255/count, 255,255);
            double x = a.x/double(a.count);
            double y = a.y/double(a.count);

            QwtSymbol *sym = new QwtSymbol;
            sym->setStyle(QwtSymbol::Diamond);
            sym->setSize(8);
            sym->setPen(QPen(GColor(CPLOTMARKER)));
            sym->setBrush(QBrush(color));

            QwtPlotMarker *p = new QwtPlotMarker();
            p->setValue(x, y);
            p->setYAxis(yLeft);
            p->setZ(100);
            p->setSymbol(sym);
            p->attach(this);
            intervalMarkers << p;

            index++;
        }
    }
}

void
ScatterPlot::configChanged()
{
    // setColors bg
    setCanvasBackground(GColor(CPLOTBACKGROUND));

    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));

    axisWidget(QwtPlot::xBottom)->setPalette(palette);
    axisWidget(QwtPlot::yLeft)->setPalette(palette);

    replot();
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
