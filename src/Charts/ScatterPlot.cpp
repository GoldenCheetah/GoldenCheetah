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
#include "Statistic.h"
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
#include <algorithm>

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

        case MODEL_LRBALANCE : return !side || point->lrbalance == RideFile::NA  ? point->lrbalance : (100 - point->lrbalance);
        case MODEL_TE : return side ? point->rte : point->lte;
        case MODEL_PS : return side ? point->rps : point->lps;

        case MODEL_RV : return point->rvert;
        case MODEL_RGCT : return point->rcontact;
        case MODEL_RCAD : return point->rcad;
        case MODEL_GEAR : return point->gear;
        case MODEL_SMO2 : return point->smo2;
        case MODEL_THB : return point->thb;
        case MODEL_HHB : return point->hhb;
        case MODEL_O2HB : return point->o2hb;
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
            case MODEL_CPV : return (tr("Circumferential Pedal Velocity (m/s)"));
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
            case MODEL_HHB :  return (tr("Deoxygenated Haemoglobin"));
            case MODEL_O2HB :  return (tr("Oxygenated Haemoglobin"));
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
            case MODEL_O2HB :  return (tr("O2Hb"));
            case MODEL_HHB :  return (tr("HHb"));
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

    setAxisMaxMinor(QwtAxis::XBottom, 0);
    setAxisMaxMinor(QwtAxis::YLeft, 0);

    QwtScaleDraw *sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtAxis::XBottom, sd);

    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    setAxisScaleDraw(QwtAxis::YLeft, sd);

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(intervalHover(IntervalItem*)), this, SLOT(intervalHover(IntervalItem*)));

    // lets watch the mouse move...
    new mouseTracker(this);

    configChanged(CONFIG_APPEARANCE | CONFIG_GENERAL); // use latest wheelsize/cranklength and colors
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

    // clear out any label
    foreach(QwtPlotMarker *c, labels) {
        c->detach();
        delete c;
    }
    labels.clear();

    // count the currently selected intervals
    QVector<int> intervals;
    QMap<int,int> displaySequence;

    for (int child=0; child<settings->ride->intervals().count(); child++) {
        IntervalItem *current = settings->ride->intervals().at(child);
        if (current->selected) {
            intervals.append(child);
            displaySequence.insert(current->displaySequence, child);
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
    cranklength = appsettings->cvalue(context->athlete->cyclist, GC_CRANKLENGTH, 175.00).toDouble() / 1000.0;

    // how many curves do we need ?
    if (xseries == MODEL_TE || xseries == MODEL_PS ||
        yseries == MODEL_TE || yseries == MODEL_PS) {

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

                double xv = pointType(point, settings->x, side, GlobalContext::context()->useMetricUnits, cranklength);
                double yv = pointType(point, settings->y, side, GlobalContext::context()->useMetricUnits, cranklength);

                // skip values ? Like zeroes...
                if (skipValues(xv, yv, settings))
                    continue;

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
                QVector<QVector<double> > xvals(settings->ride->intervals().count()); // array of curve x arrays
                QVector<QVector<double> > yvals(settings->ride->intervals().count()); // array of curve x arrays
                QVector<int> points(settings->ride->intervals().count());             // points in eac curve

                // extract interval data
                foreach(const RideFilePoint *point, settings->ride->ride()->dataPoints()) {

                    double x = pointType(point, settings->x, side, GlobalContext::context()->useMetricUnits, cranklength);
                    double y = pointType(point, settings->y, side, GlobalContext::context()->useMetricUnits, cranklength);

                    if (!(settings->ignore && (x == 0 && y ==0))) {

                        // which interval is it in?
                        for (int idx=0; idx<settings->ride->intervals().count(); idx++) {

                            IntervalItem *current = settings->ride->intervals().at(idx);
                            if (current->selected && point->secs+settings->ride->ride()->recIntSecs() > current->start && point->secs< current->stop) {
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
                    intervalColor.setHsv((255/settings->ride->intervals().count()) * idx, 255,255);
                    // left / right are darker lighter
                    if (side) intervalColor = intervalColor.lighter(50);

                    QwtSymbol *sym = new QwtSymbol;
                    sym->setStyle(QwtSymbol::Ellipse);
                    sym->setSize(4*dpiXFactor);
                    sym->setBrush(QBrush(intervalColor));
                    sym->setPen(QPen(intervalColor));

                    QwtPlotCurve *ic = new QwtPlotCurve();

                    ic->setSymbol(sym);
                    ic->setStyle(QwtPlotCurve::Dots);
                    ic->setRenderHint(QwtPlotItem::RenderAntialiased);
                    ic->setSamples(xvals[idx].constData(), yvals[idx].constData(), points[idx]);
                    ic->attach(this);
                    intervalCurves.append(ic);

                    // show as a line too, if not framed
#if 0 // a bit ugly tbh
                    if (!settings->frame) {
                        QwtPlotCurve *icl = new QwtPlotCurve();
                        icl->setStyle(QwtPlotCurve::Lines);
                        icl->setPen(QPen(intervalColor, Qt::SolidLine));
                        icl->setRenderHint(QwtPlotItem::RenderAntialiased);
                        icl->setSamples(xvals[idx].constData(), yvals[idx].constData(), points[idx]);
                        icl->attach(this);
                        intervalCurves.append(icl);
                    }
#endif
                }
            }

            // setup the framing curve
            if (intervals.count() == 0 || settings->frame) {
                smooth(x, y, points, settings->smoothing);

                if (side) {

                    QwtSymbol *sym = new QwtSymbol;
                    sym->setStyle(QwtSymbol::Ellipse);
                    sym->setSize(4*dpiXFactor);
                    sym->setPen(QPen(Qt::cyan));
                    sym->setBrush(QBrush(Qt::cyan));

                    curve2 = new QwtPlotCurve();
                    curve2->setSymbol(sym);
                    curve2->setStyle(QwtPlotCurve::Dots);
                    curve2->setRenderHint(QwtPlotItem::RenderAntialiased);
                    curve2->setSamples(x.constData(), y.constData(), points);
                    curve2->setZ(-1);
                    curve2->attach(this);

                    if (settings->trendLine>0)  {
                        addTrendLine(x, y, points, Qt::cyan);
                    }

                } else {

                    QwtSymbol *sym = new QwtSymbol;
                    sym->setStyle(QwtSymbol::Ellipse);
                    sym->setSize(4*dpiXFactor);
                    sym->setPen(QPen(Qt::red));
                    sym->setBrush(QBrush(Qt::red));

                    curve = new QwtPlotCurve();
                    curve->setSymbol(sym);
                    curve->setStyle(QwtPlotCurve::Dots);
                    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
                    curve->setSamples(x.constData(), y.constData(), points);
                    curve->setZ(-1);
                    curve->attach(this);

                    if (settings->trendLine>0)  {
                        addTrendLine(x, y, points, Qt::red);
                    }
                }
            }
        }
        else {

            if (context->compareIntervals.count() > 0) {

                //
                // CREATE COMPARE CURVES
                //

                int i=0;
                if (settings->compareMode > 0)
                    i++;
                for (; i< context->compareIntervals.count(); i++) {
                    CompareInterval interval = context->compareIntervals.at(i);

                    if (interval.checked == false)
                        continue;

                    // interval data in here
                    QVector<double> xval;
                    QVector<double> yval;
                    int nbPoints = 0;

                    // extract interval data
                    foreach(const RideFilePoint *point, interval.data->dataPoints()) {

                        double x;
                        double y;
                        if (settings->compareMode == 1) {
                            const RideFilePoint *refPoint = context->compareIntervals.at(0).data->dataPoints().at(context->compareIntervals.at(0).data->timeIndex(point->secs));
                            x = pointType(refPoint, settings->x, side, GlobalContext::context()->useMetricUnits, cranklength);
                            y = pointType(point, settings->y, side, GlobalContext::context()->useMetricUnits, cranklength);
                        }
                        else if (settings->compareMode == 2) {
                            const RideFilePoint *refPoint = context->compareIntervals.at(0).data->dataPoints().at(context->compareIntervals.at(0).data->timeIndex(point->secs));
                            x = pointType(point, settings->x, side, GlobalContext::context()->useMetricUnits, cranklength);
                            y = pointType(refPoint, settings->y, side, GlobalContext::context()->useMetricUnits, cranklength);
                        }
                        else {
                            x = pointType(point, settings->x, side, GlobalContext::context()->useMetricUnits, cranklength);
                            y = pointType(point, settings->y, side, GlobalContext::context()->useMetricUnits, cranklength);
                        }


                        // skip values ?
                        if (skipValues(x, y, settings))
                            continue;

                        if (y > maxY) maxY = y;
                        if (y < minY) minY = y;
                        if (x > maxX) maxX = x;
                        if (x < minX) minX = x;

                        xval.append(x);
                        yval.append(y);
                        nbPoints++;
                    }

                    smooth(xval, yval, nbPoints, settings->smoothing);

                    QColor intervalColor = interval.color;
                    // left / right are darker lighter
                    if (side) intervalColor = intervalColor.lighter(50);

                    QwtSymbol *sym = new QwtSymbol;
                    sym->setStyle(QwtSymbol::Ellipse);
                    sym->setSize(4*dpiXFactor);
                    sym->setBrush(QBrush(intervalColor));
                    sym->setPen(QPen(intervalColor));

                    QwtPlotCurve *ic = new QwtPlotCurve();

                    ic->setSymbol(sym);
                    ic->setStyle(QwtPlotCurve::Dots);
                    ic->setRenderHint(QwtPlotItem::RenderAntialiased);
                    ic->setSamples(xval.constData(), yval.constData(), nbPoints);
                    ic->attach(this);

                    intervalCurves.append(ic);

                    if (settings->trendLine>0)  {
                        addTrendLine(xval, yval, nbPoints, intervalColor);
                    }
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
    setAxisTitle(QwtAxis::YLeft, describeType(settings->y, true, GlobalContext::context()->useMetricUnits));
    setAxisTitle(QwtAxis::XBottom, describeType(settings->x, true, GlobalContext::context()->useMetricUnits));

    // axis scale
    if (settings->y == MODEL_AEPF) setAxisScale(QwtAxis::YLeft, 0, std::min(maxY, 2500.0));
    else setAxisScale(QwtAxis::YLeft, minY, maxY);
    if (settings->x == MODEL_CPV) setAxisScale(QwtAxis::XBottom, 0, std::max(maxX, 3.0));
    else setAxisScale(QwtAxis::XBottom, minX, maxX);

    // gear
    if (settings->x == MODEL_GEAR) setAxisScale(QwtAxis::XBottom, 0, 7);
    if (settings->y == MODEL_GEAR) setAxisScale(QwtAxis::YLeft, 0, 7);


    // and those interval markers
    refreshIntervalMarkers(settings);

    replot();
}


void ScatterPlot::mouseMoved()
{
    if (!isVisible()) return;

    if (ride && ride->ride() && ride->intervals().count() >= intervalMarkers.count()) {

        // where is the mouse ?
        QPoint pos = QCursor::pos();

        // are we hovering "close" to an interval marker ?
        int index = 0;
        foreach (QwtPlotMarker *is, intervalMarkers) {

            QPoint mpos = canvas()->mapToGlobal(QPoint(transform(QwtAxis::XBottom, is->value().x()), transform(QwtAxis::YLeft, is->value().y())));

            int dx = mpos.x() - pos.x();
            int dy = mpos.y() - pos.y();

            if ((dx > -6 && dx < 6) && (dy > -6 && dy < 6))
                context->notifyIntervalHover(ride->intervals()[index]);

            index++;
        }
    }
    return;
}

void
ScatterPlot::intervalHover(IntervalItem *ri)
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

    // null so just clear hover
    if (!ri) return;

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

            double y = pointType(p1, xseries, side, GlobalContext::context()->useMetricUnits, cranklength);
            double x = pointType(p1, yseries, side, GlobalContext::context()->useMetricUnits, cranklength);

            if (p1->secs < ri->start || p1->secs > ri->stop) continue;

            xArray << x;
            yArray << y;
        }

        // which interval is it or how many ?
        int count = 0;
        int ours = 0;
        foreach(IntervalItem *p, ride->intervals()) {
            if (p->start == ri->start && p->stop == ri->stop) ours = count;
            count++;
        }

        // any data ?
        if (count > 0 && xArray.size()) {
            QwtSymbol *sym = new QwtSymbol;
            sym->setStyle(QwtSymbol::Ellipse);
            sym->setSize(4*dpiXFactor);
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

    if (settings->ride && settings->ride->ride() && settings->ride->ride()->dataPoints().count() && (count = settings->ride->intervals().count())) {

        // accumulating...
        QVector<saccum> intervalAccumulator(count);

        foreach (RideFilePoint *point, settings->ride->ride()->dataPoints()) {

            double x0 = pointType(point, settings->x, 0, GlobalContext::context()->useMetricUnits, cranklength);
            double y0 = pointType(point, settings->y, 0, GlobalContext::context()->useMetricUnits, cranklength);
            double x1 = pointType(point, settings->x, 1, GlobalContext::context()->useMetricUnits, cranklength);
            double y1 = pointType(point, settings->y, 1, GlobalContext::context()->useMetricUnits, cranklength);

            // average of left and right (even if both the same)
            double x = (x0 + x1) / 2.00f;
            double y = (y0 + y1) / 2.00f;

            // accumulate values for each interval here ....
            for(int i=0; i < count; i++) {

                IntervalItem *v = settings->ride->intervals()[i];

                // in our interval ?
                if (point->secs >= v->start && point->secs <= v->stop) {
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
            sym->setSize(8*dpiXFactor);
            sym->setPen(QPen(GColor(CPLOTMARKER)));
            sym->setBrush(QBrush(color));

            QwtPlotMarker *p = new QwtPlotMarker();
            p->setValue(x, y);
            p->setYAxis(QwtAxis::YLeft);
            p->setZ(100);
            p->setSymbol(sym);
            p->attach(this);
            intervalMarkers << p;

            index++;
        }
    }
}

void
ScatterPlot::configChanged(qint32)
{
    // setColors bg
    setCanvasBackground(GColor(CPLOTBACKGROUND));

    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));

    axisWidget(QwtAxis::XBottom)->setPalette(palette);
    axisWidget(QwtAxis::YLeft)->setPalette(palette);

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

void
ScatterPlot::addTrendLine(QVector<double> xval, QVector<double> yval, int nbPoints, QColor intervalColor)
{
    QwtPlotCurve *trend = new QwtPlotCurve();

    // cosmetics
    QPen cpen = QPen(intervalColor.darker(200));
    cpen.setWidth(2); // double thickness for trend lines
    cpen.setStyle(Qt::SolidLine);
    trend->setPen(cpen);
    if (appsettings->value(this, GC_ANTIALIAS, true).toBool()==true)
        trend->setRenderHint(QwtPlotItem::RenderAntialiased);
    trend->setBaseline(0);
    trend->setYAxis(QwtAxis::YLeft);
    trend->setStyle(QwtPlotCurve::Lines);

    // perform linear regression
    Statistic regress(xval.data(), yval.data(), nbPoints);
    double xtrend[2], ytrend[2];
    xtrend[0] = 0.0;
    ytrend[0] = regress.getYforX(0.0);
    // point 2 is at far right of chart, not the last point
    // since we may be forecasting...
    xtrend[1] = regress.maxX;
    ytrend[1] = regress.getYforX(regress.maxX);
    trend->setSamples(xtrend,ytrend, 2);

    trend->attach(this);

    intervalCurves.append(trend);

    // make that mark -- always above with topN
    QwtPlotMarker *label = new QwtPlotMarker();
    QString labelString = regress.label();
    QwtText text(labelString);
    text.setColor(intervalColor.darker(200));
    label->setLabel(text);
    label->setValue(xtrend[1]*0.8, ytrend[1]*0.9);
    label->setYAxis(QwtAxis::YLeft);
    label->setSpacing(6 *dpiXFactor); // not px but by yaxis value !? mad.
    label->setLabelAlignment(Qt::AlignTop | Qt::AlignCenter);

    // and attach
    label->attach(this);
    labels << label;
}

void
ScatterPlot::smooth(QVector<double> &xval, QVector<double> &yval, int count, int applySmooth)
{
    if (applySmooth<2 || count < applySmooth)
        return;

    QVector<double> smoothX(count);
    QVector<double> smoothY(count);

    for (int i=0; i<count; i++) {
        smoothX[i] = xval.value(i);
        smoothY[i] = yval.value(i);
    }

    // initialise rolling average
    double rXtot = 0;
    double rYtot = 0;

    for (int i=applySmooth; i>0 && count-i >=0; i--) {
        rXtot += smoothX[count-i];
        rYtot += smoothY[count-i];
    }

    // now run backwards setting the rolling average
    for (int i=count-1; i>=applySmooth; i--) {
        int hereX = smoothX[i];
        smoothX[i] = rXtot / applySmooth;
        rXtot -= hereX;
        rXtot += smoothX[i-applySmooth];

        int hereY = smoothY[i];
        smoothY[i] = rYtot / applySmooth;
        rYtot -= hereY;
        rYtot += smoothY[i-applySmooth];
    }

    // Finish with smaller rolling average
    for (int i=applySmooth-1; i>0; i--) {
        int hereX = smoothX[i];
        smoothX[i] = rXtot / (i+2);
        rXtot -= hereX;

        int hereY = smoothY[i];
        smoothY[i] = rYtot / (i+2);
        rYtot -= hereY;
    }

    xval = smoothX;
    yval = smoothY;
}

void
ScatterPlot::resample(QVector<double> &xval, QVector<double> &yval, int &count, double recInterval, int applySmooth)
{
    if (recInterval >= applySmooth)
        return;


    int newcount=0;
    QVector<double> newxval;
    QVector<double> newyval;

    if (applySmooth > recInterval) {
        double totalX = 0.0;
        double totalY = 0.0;

        int j=0;

        for (int i = 0; i < count; i++) {
            j++;

            if (j<applySmooth) {
                totalX += xval.at(i)*recInterval;
                totalY += yval.at(i)*recInterval;
            }
            else {
                double part = recInterval-j+applySmooth;
                totalX += xval.at(i)*part;
                totalY += yval.at(i)*part;

                double smoothX = totalX/applySmooth;
                double smoothY = totalY/applySmooth;

                newxval.append(smoothX);
                newyval.append(smoothY);
                newcount++;

                j=0;
                totalX = xval.at(i)*(recInterval-part);
                totalY = yval.at(i)*(recInterval-part);
            }
        }
    }

    count = newcount;
    xval = newxval;
    yval = newyval;
}

bool
ScatterPlot::skipValues(double xv, double yv, ScatterSettings *settings) {

    // skip zeroes? - special logic for Model Gear/CPV, since there value between 0.01 and 1 happen and are relevant
    if ((settings->x != MODEL_GEAR && settings->y != MODEL_GEAR && settings->x != MODEL_CPV && settings->y != MODEL_CPV)
         && settings->ignore && (int(xv) == 0 || int(yv) == 0)) return true;

    // Model Gear/CPV
    if ((settings->x == MODEL_GEAR || settings->x == MODEL_CPV)
         && settings->ignore && (xv == 0.0f || int(yv) == 0)) return true;
    if ((settings->y == MODEL_GEAR || settings->y == MODEL_GEAR)
         && settings->ignore && (int(xv) == 0 || yv == 0.0f)) return true;

    // Temp 0 values are relevant
    if ((settings->x == MODEL_TEMP && xv == RideFile::NA) || (settings->y == MODEL_TEMP && yv == RideFile::NA)) return true;

    // LR Balance : if skip 0% skip also 100%
    if ((settings->x == MODEL_LRBALANCE && settings->ignore && xv == 100) || (settings->y == MODEL_LRBALANCE && settings->ignore && yv == 100)) return true;

    if ((settings->x == MODEL_LRBALANCE && xv == RideFile::NA) || (settings->y == MODEL_LRBALANCE && yv == RideFile::NA)) return true;

    return false;
}
