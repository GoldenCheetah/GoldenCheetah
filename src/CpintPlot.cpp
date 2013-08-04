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

#include "Athlete.h"
#include "Zones.h"
#include "Colors.h"
#include "CpintPlot.h"
#include <unistd.h>
#include <QDebug>
#include <qwt_series_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_engine.h>
#include "RideItem.h"
#include "LogTimeScaleDraw.h"
#include "LogTimeScaleEngine.h"
#include "RideFile.h"
#include "Season.h"
#include "Settings.h"
#include "LTMCanvasPicker.h"
#include "TimeUtils.h"

#include <algorithm> // for std::lower_bound

#define USE_T0_IN_CP_MODEL 0 // added djconnel 08Apr2009: allow 3-parameter CP model

CpintPlot::CpintPlot(Context *context, QString p, const Zones *zones) :
    path(p),
    thisCurve(NULL),
    CPCurve(NULL),
    allCurve(NULL),
    zones(zones),
    series(RideFile::watts),
    context(context),
    current(NULL),
    bests(NULL),
    isFiltered(false),
    shadeMode(2)
{
    setInstanceName("CP Plot");

    setAxisTitle(xBottom, tr("Interval Length"));
    LogTimeScaleDraw *ld = new LogTimeScaleDraw;
    ld->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(xBottom, ld);
    setAxisScaleEngine(xBottom, new LogTimeScaleEngine);
    setAxisScale(xBottom, (double)0.017, (double)60);

    QwtScaleDraw *sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(yLeft, sd);
    setAxisTitle(yLeft, tr("Average Power (watts)"));
    setAxisMaxMinor(yLeft, 0);
    plotLayout()->setAlignCanvasToScales(true);

    grid = new QwtPlotGrid();
    grid->enableX(true);
    grid->attach(this);

    curveTitle.attach(this);
    curveTitle.setXValue(5);
    curveTitle.setYValue(20);
    curveTitle.setLabel(QwtText("", QwtText::PlainText)); // default to no title

    zoomer = new penTooltip(this->canvas());
    zoomer->setMousePattern(QwtEventPattern::MouseSelect1,
                            Qt::LeftButton, Qt::ShiftModifier);

    canvasPicker = new LTMCanvasPicker(this);
    canvas()->setFrameStyle(QFrame::NoFrame);
    connect(canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)), this, SLOT(pointHover(QwtPlotCurve*, int)));

    configChanged(); // apply colors
}

void
CpintPlot::configChanged()
{
    setCanvasBackground(GColor(CPLOTBACKGROUND));
    QPen gridPen(GColor(CPLOTGRID));
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);
}

void
CpintPlot::setAxisTitle(int axis, QString label)
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
CpintPlot::changeSeason(const QDate &start, const QDate &end)
{
    // wipe out current - calculate will reinstate
    startDate = (start == QDate()) ? QDate(1900, 1, 1) : start;
    endDate = (end == QDate()) ? QDate(3000, 12, 31) : end;

    if (CPCurve) {
        delete CPCurve;
        CPCurve = NULL;
        clear_CP_Curves();
    }
    if (bests) {
        delete bests;
        bests = NULL;
    }
}

void
CpintPlot::setSeries(RideFile::SeriesType x)
{
    series = x;

    // Log scale for all bar Energy
    setAxisScaleEngine(xBottom, new LogTimeScaleEngine);
    setAxisScaleDraw(xBottom, new LogTimeScaleDraw);
    setAxisTitle(xBottom, tr("Interval Length"));

    switch (series) {

        case RideFile::none:
            setAxisTitle(yLeft, tr("Total work (kJ)"));
            setAxisScaleEngine(xBottom, new QwtLinearScaleEngine);
            setAxisScaleDraw(xBottom, new QwtScaleDraw);
            setAxisTitle(xBottom, tr("Interval Length (minutes)"));
            break;

        case RideFile::cad:
            setAxisTitle(yLeft, tr("Average Cadence (rpm)"));
            break;

        case RideFile::hr:
            setAxisTitle(yLeft, tr("Average Heartrate (bpm)"));
            break;

        case RideFile::kph:
            setAxisTitle(yLeft, tr("Average Speed (kph)"));
            break;

        case RideFile::nm:
            setAxisTitle(yLeft, tr("Average Pedal Force (nm)"));
            break;

        case RideFile::NP:
            setAxisTitle(yLeft, tr("Normalized Power (watts)"));
            break;

        case RideFile::xPower:
            setAxisTitle(yLeft, tr("Skiba xPower (watts)"));
            break;

        case RideFile::wattsKg:
            if (context->athlete->useMetricUnits)
                setAxisTitle(yLeft, tr("Watts per kilo (watts/kg)"));
            else
                setAxisTitle(yLeft, tr("Watts per lb (watts/lb)"));
            break;

        case RideFile::vam:
            setAxisTitle(yLeft, tr("VAM (meters per hour)"));
            break;

        default:
        case RideFile::watts:
            setAxisTitle(yLeft, tr("Average Power (watts)"));
            break;

    }

    delete CPCurve;
    CPCurve = NULL;
    clear_CP_Curves();
    if (allCurve) {
        delete allCurve;
        allCurve = NULL;
    }
}

// extract critical power parameters which match the given curve
// model: maximal power = cp (1 + tau / [t + t0]), where t is the
// duration of the effort, and t, cp and tau are model parameters
// the basic critical power model is t0 = 0, but non-zero has
// been discussed in the literature
// it is assumed duration = index * seconds
void
CpintPlot::deriveCPParameters()
{
    // bounds on anaerobic interval in minutes
    const double t1 = USE_T0_IN_CP_MODEL ? 0.25 : 1;
    const double t2 = 6;

    // bounds on aerobic interval in minutes
    const double t3 = 10;
    const double t4 = 60;

    // bounds of these time valus in the data
    int i1, i2, i3, i4;

    // find the indexes associated with the bounds
    // the first point must be at least the minimum for the anaerobic interval, or quit
    for (i1 = 0; i1 < 60 * t1; i1++)
        if (i1 + 1 >= bests->meanMaxArray(series).size())
            return;
    // the second point is the maximum point suitable for anaerobicly dominated efforts.
    for (i2 = i1; i2 + 1 <= 60 * t2; i2++)
        if (i2 + 1 >= bests->meanMaxArray(series).size())
            return;
    // the third point is the beginning of the minimum duration for aerobic efforts
    for (i3 = i2; i3 < 60 * t3; i3++)
        if (i3 + 1 >= bests->meanMaxArray(series).size())
            return;
    for (i4 = i3; i4 + 1 <= 60 * t4; i4++)
        if (i4 + 1 >= bests->meanMaxArray(series).size())
            break;

    // initial estimate of tau
    if (tau == 0)
        tau = 1;

    // initial estimate of cp (if not already available)
    if (cp == 0)
        cp = 300;

    // initial estimate of t0: start small to maximize sensitivity to data
    t0 = 0;

    // lower bound on tau
    const double tau_min = 0.5;

    // convergence delta for tau
    const double tau_delta_max = 1e-4;
    const double t0_delta_max  = 1e-4;

    // previous loop value of tau and t0
    double tau_prev;
    double t0_prev;

    // maximum number of loops
    const int max_loops = 100;

    // loop to convergence
    int iteration = 0;
    do {
        if (iteration ++ > max_loops) {
            QMessageBox::warning(
                NULL, "Warning",
                QString("Maximum number of loops %d exceeded in cp model"
                        "extraction").arg(max_loops),
                QMessageBox::Ok,
                QMessageBox::NoButton);
            break;
        }

        // record the previous version of tau, for convergence
        tau_prev = tau;
        t0_prev  = t0;

        // estimate cp, given tau
        int i;
        cp = 0;
        for (i = i3; i <= i4; i++) {
            double cpn = bests->meanMaxArray(series)[i] / (1 + tau / (t0 + i / 60.0));
            if (cp < cpn)
                cp = cpn;
        }

        // if cp = 0; no valid data; give up
        if (cp == 0.0)
            return;

        // estimate tau, given cp
        tau = tau_min;
        for (i = i1; i <= i2; i++) {
            double taun = (bests->meanMaxArray(series)[i] / cp - 1) * (i / 60.0 + t0) - t0;
            if (tau < taun)
                tau = taun;
        }

        // update t0 if we're using that model
        #if USE_T0_IN_CP_MODEL
        t0 = tau / (bests->meanMaxArray(series)[1] / cp - 1) - 1 / 60.0;
        #endif

    } while ((fabs(tau - tau_prev) > tau_delta_max) ||
             (fabs(t0 - t0_prev) > t0_delta_max)
            );
}

void
CpintPlot::plot_CP_curve(CpintPlot *thisPlot,     // the plot we're currently displaying
                         double cp,
                         double tau,
                         double t0)
{
    if (CPCurve) {
        delete CPCurve;
        CPCurve = NULL;
    }

    // if there's no cp, then there's nothing to do
    if (cp <= 0)
        return;

    // populate curve data with a CP curve
    const int curve_points = 100;
    double tmin = USE_T0_IN_CP_MODEL ? 1.0/60 : tau;
    double tmax = 180.0;
    QVector<double> cp_curve_power(curve_points);
    QVector<double> cp_curve_time(curve_points);
    int i;

    for (i = 0; i < curve_points; i ++) {
        double x = (double) i / (curve_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        cp_curve_time[i] = t;
        if (series == RideFile::none) //this is ENERGY
            cp_curve_power[i] = (cp * t + cp * tau) * 60.0 / 1000.0;
        else
            cp_curve_power[i] = cp * (1 + tau / (t + t0));
    }

    // generate a plot
    QString curve_title;
#if USE_T0_IN_CP_MODEL
    curve_title.sprintf("CP=%.1f W; AWC/CP=%.2f m; t0=%.1f s", cp, tau, 60 * t0);
#else
    if (series == RideFile::wattsKg)
        curve_title.sprintf("CP=%.2f W/kg; AWC=%.2f kJ/kg", cp, cp * tau * 60.0 / 1000.0);
    else
        curve_title.sprintf("CP=%.0f W; AWC=%.0f kJ", cp, cp * tau * 60.0 / 1000.0);
#endif
    if (series == RideFile::watts || series == RideFile::wattsKg) curveTitle.setLabel(QwtText(curve_title, QwtText::PlainText));

    if (series == RideFile::wattsKg)
        curveTitle.setYValue(0.6);
    else
        curveTitle.setYValue(20);

    CPCurve = new QwtPlotCurve(curve_title);
    if (appsettings->value(this, GC_ANTIALIAS, false).toBool() == true)
        CPCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen pen(GColor(CCP));
    pen.setWidth(2.0);
    pen.setStyle(Qt::DashLine);
    CPCurve->setPen(pen);
    CPCurve->setData(cp_curve_time.data(), cp_curve_power.data(), curve_points);
    CPCurve->attach(thisPlot);
}

void
CpintPlot::clear_CP_Curves()
{
    // unattach any existing shading curves and reset the list
    if (allCurves.size()) {
        foreach (QwtPlotCurve *curve, allCurves)
            delete curve;
        allCurves.clear();
    }

    // now delete any labels
    if (allZoneLabels.size()) {
        foreach (QwtPlotMarker *label, allZoneLabels)
            delete label;
        allZoneLabels.clear();
    }
}

// plot the all curve, with shading according to the shade mode
void
CpintPlot::plot_allCurve(CpintPlot *thisPlot,
                         int n_values,
                         const double *power_values)
{
    clear_CP_Curves();

    QVector<double> energyBests(n_values);
    QVector<double> time_values(n_values);
    // generate an array of time values
    for (int t = 0; t < n_values; t++) {
        time_values[t] = (t + 1) / 60.0;
        energyBests[t] = power_values[t] * time_values[t] * 60.0 / 1000.0;
    }

    // lets work out how we are shading it
    switch(shadeMode) {
        case 0 : // not shading!!
            break;

        case 1 : // value for current date
                 // or average for date range if a range
            shadingCP = dateCP;
            break;

        default:
        case 2 : // derived value
            shadingCP = cp;
            break;
    }

    // generate zones from shading CP value
    if (shadingCP > 0) {
        QList <int> power_zone;
        int n_zones = zones->lowsFromCP(&power_zone, (int) int(shadingCP));
        int high = n_values - 1;
        int zone = 0;
        while (zone < n_zones && high > 0) {
            int low = high - 1;
            int nextZone = zone + 1;
            if (nextZone >= power_zone.size())
                low = 0;
            else {
                while ((low > 0) && (power_values[low] < power_zone[nextZone]))
                    --low;
            }

            QColor color = zoneColor(zone, n_zones);
            QString name = zones->getDefaultZoneName(zone);
            QwtPlotCurve *curve = new QwtPlotCurve(name);
            if (appsettings->value(this, GC_ANTIALIAS, false).toBool() == true)
                curve->setRenderHint(QwtPlotItem::RenderAntialiased);
            QPen pen(color.darker(200));
            pen.setWidth(2.0);
            curve->setPen(pen);
            curve->attach(thisPlot);

            // use a linear gradient
            if (shadeMode && shadingCP) { // 0 value means no shading please - and only if proper value for shadingCP
                color.setAlpha(64);
                QColor color1 = color.darker();
                QLinearGradient linearGradient(0, 0, 0, height());
                linearGradient.setColorAt(0.0, color);
                linearGradient.setColorAt(1.0, color1);
                linearGradient.setSpread(QGradient::PadSpread);
                curve->setBrush(linearGradient);   // fill below the line
            }

            if (series == RideFile::none) { // this is Energy mode 
                curve->setData(time_values.data() + low,
                               energyBests.data() + low, high - low + 1);
            } else {
                curve->setData(time_values.data() + low,
                               power_values + low, high - low + 1);
            }
            allCurves.append(curve);

            if (shadeMode && (series != RideFile::none || energyBests[high] > 100.0)) {
                QwtText text(name);
                text.setFont(QFont("Helvetica", 20, QFont::Bold));
                color.setAlpha(255);
                text.setColor(color);
                QwtPlotMarker *label_mark = new QwtPlotMarker();
                // place the text in the geometric mean in time, at a decent power
                double x, y;
                if (series == RideFile::none) {
                    x = (time_values[low] + time_values[high]) / 2;
                    y = (energyBests[low] + energyBests[high]) / 5;
                }
                else {
                    x = sqrt(time_values[low] * time_values[high]);
                    y = (power_values[low] + power_values[high]) / 5;
                }
                label_mark->setValue(x, y);
                label_mark->setLabel(text);
                label_mark->attach(thisPlot);
                allZoneLabels.append(label_mark);
            }

            high = low;
            ++zone;
        }
    }
    // no zones available: just plot the curve without zones
    else {
        QwtPlotCurve *curve = new QwtPlotCurve(tr("maximal power"));
        if (appsettings->value(this, GC_ANTIALIAS, false).toBool() == true)
            curve->setRenderHint(QwtPlotItem::RenderAntialiased);
        QPen pen(GColor(CCP));
        pen.setWidth(appsettings->value(this, GC_LINEWIDTH, 2.0).toDouble());
        curve->setPen(pen);
        QColor brush_color = GColor(CCP);
        brush_color.setAlpha(200);
        curve->setBrush(brush_color);   // brush fills below the line
        if (series == RideFile::none)
            curve->setData(time_values.data(), energyBests.data(), n_values);
        else
            curve->setData(time_values.data(), power_values, n_values);
        curve->attach(thisPlot);
        allCurves.append(curve);
    }

    // Energy mode is really only interesting in the range where energy is
    // linear in interval duration--up to about 1 hour.
    double xmax = (series == RideFile::none)  ? 60.0 : time_values[n_values - 1];

    if (series == RideFile::vam)
        thisPlot->setAxisScale(thisPlot->xBottom, (double) 4.993, (double)xmax);
    else
        thisPlot->setAxisScale(thisPlot->xBottom, (double) 0.017, (double)xmax);

    double ymax;
    if (series == RideFile::none) {
        int i = std::lower_bound(time_values.begin(), time_values.end(), 60.0) - time_values.begin();
        ymax = 10 * ceil(energyBests[i] / 10);
    }
    else {
        ymax = 100 * ceil(power_values[0] / 100);
        if (ymax == 100)
            ymax = 5 * ceil(power_values[0] / 5);
    }
    thisPlot->setAxisScale(thisPlot->yLeft, 0, ymax);
}

void
CpintPlot::calculate(RideItem *rideItem)
{
    if (!rideItem) return;

    QString fileName = rideItem->fileName;
    QDateTime dateTime = rideItem->dateTime;
    QDir dir(path);
    QFileInfo file(fileName);

    // zap any existing ridefilecache then get new one
    if (current) delete current;
    current = new RideFileCache(context, context->athlete->home.absolutePath() + "/" + fileName);

    // get aggregates - incase not initialised from date change
    if (bests == NULL) bests = new RideFileCache(context, startDate, endDate, isFiltered, files);

    //
    // PLOT MODEL CURVE (DERIVED)
    //
    if (series == RideFile::xPower || series == RideFile::NP || series == RideFile::watts  || series == RideFile::wattsKg || series == RideFile::none) {

        if (bests->meanMaxArray(series).size() > 1) {
            // calculate CP model from all-time best data
            cp  = tau = t0  = 0;
            deriveCPParameters();
        }

        //
        // CP curve only relevant for Energy or Watts (?)
        //
        if (series == RideFile::watts || series == RideFile::wattsKg || series == RideFile::none) {
            if (!CPCurve) plot_CP_curve(this, cp, tau, t0);
            else {
                // make sure color reflects latest config
                QPen pen(GColor(CCP));
                pen.setWidth(4.0);
                pen.setStyle(Qt::DashLine);
                CPCurve->setPen(pen);
            }
        }

        //
        // PLOT ZONE (RAINBOW) AGGREGATED CURVE
        //
        if (bests->meanMaxArray(series).size()) {
            int maxNonZero = 0;
            for (int i = 0; i < bests->meanMaxArray(series).size(); ++i) {
                if (bests->meanMaxArray(series)[i] > 0) maxNonZero = i;
            }
            plot_allCurve(this, maxNonZero, bests->meanMaxArray(series).constData() + 1);
        }
    } else {

        //
        // PLOT BESTS IN SERIES COLOR
        //
        if (allCurve) {
            delete allCurve;
            allCurve = NULL;
        }
        if (bests->meanMaxArray(series).size()) {

            int maxNonZero = 0;
            QVector<double> timeArray(bests->meanMaxArray(series).size());
            for (int i = 0; i < bests->meanMaxArray(series).size(); ++i) {
                timeArray[i] = i / 60.0;
                if (bests->meanMaxArray(series)[i] > 0) maxNonZero = i;
            }

            if (maxNonZero > 1) {

                allCurve = new QwtPlotCurve(dateTime.toString(tr("ddd MMM d, yyyy h:mm AP")));
                allCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

                QPen line;
                QColor fill;
                switch (series) {

                    case RideFile::kph:
                        line.setColor(GColor(CSPEED).darker(200));
                        fill = (GColor(CSPEED));
                        break;

                    case RideFile::cad:
                        line.setColor(GColor(CCADENCE).darker(200));
                        fill = (GColor(CCADENCE));
                        break;

                    case RideFile::nm:
                        line.setColor(GColor(CTORQUE).darker(200));
                        fill = (GColor(CTORQUE));
                        break;

                    case RideFile::hr:
                        line.setColor(GColor(CHEARTRATE).darker(200));
                        fill = (GColor(CHEARTRATE));
                        break;

                    case RideFile::vam:
                        line.setColor(GColor(CALTITUDE).darker(200));
                        fill = (GColor(CALTITUDE));
                        break;

                    default:
                    case RideFile::watts: // won't ever get here
                    case RideFile::NP:
                    case RideFile::xPower:
                        line.setColor(GColor(CPOWER).darker(200));
                        fill = (GColor(CPOWER));
                        break;
                }

                // wow, QVector really doesn't have a max/min method!
                double ymax = 0;
                double ymin = 100000;
                foreach(double v, current->meanMaxArray(series)) {
                    if (v > ymax) ymax = v;
                    if (v && v < ymin) ymin = v;
                }
                foreach(double v, bests->meanMaxArray(series)) {
                    if (v > ymax) ymax = v;
                    if (v&& v < ymin) ymin = v;
                }
                if (ymin == 100000) ymin = 0;

                // VAM is a bit special
                if (series == RideFile::vam) {
                    if (bests->meanMaxArray(series).size() > 300)
                        ymax = bests->meanMaxArray(series)[300];
                    else
                        ymax = 2000;
                }

                ymax *= 1.1; // bit of headroom
                ymin *= 0.9;

                // xmax is directly related to the size of the arrays
                double xmax = current->meanMaxArray(series).size();
                if (bests->meanMaxArray(series).size() > xmax)
                    xmax = bests->meanMaxArray(series).size();
                xmax /= 60; // its in minutes not seconds

                setAxisScale(yLeft, ymin, ymax);

                if (series == RideFile::vam)
                    setAxisScale(xBottom, 4.993, xmax);
                else
                    setAxisScale(xBottom, 0.017, xmax);

                allCurve->setPen(line);
                fill.setAlpha(64);
                // use a linear gradient
                fill.setAlpha(64);
                QColor fill1 = fill.darker();
                QLinearGradient linearGradient(0, 0, 0, height());
                linearGradient.setColorAt(0.0, fill);
                linearGradient.setColorAt(1.0, fill1);
                linearGradient.setSpread(QGradient::PadSpread);
                allCurve->setBrush(linearGradient);
                allCurve->attach(this);
                allCurve->setData(timeArray.data() + 1, bests->meanMaxArray(series).constData() + 1, maxNonZero - 1);
            }
        }
    }

    //
    // PLOT THIS RIDE CURVE
    //
    if (thisCurve) {
        delete thisCurve;
        thisCurve = NULL;
    }

    if (current->meanMaxArray(series).size()) {
        int maxNonZero = 0;
        QVector<double> timeArray(current->meanMaxArray(series).size());
        for (int i = 0; i < current->meanMaxArray(series).size(); ++i) {
            timeArray[i] = i / 60.0;
            if (current->meanMaxArray(series)[i] > 0) maxNonZero = i;
        }

        if (maxNonZero > 1) {

            thisCurve = new QwtPlotCurve(dateTime.toString(tr("ddd MMM d, yyyy h:mm AP")));
            thisCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            QPen black;
            black.setColor(Qt::black);
            black.setWidth(2.0);
            thisCurve->setPen(black);
            thisCurve->attach(this);

            if (series == RideFile::none) {

                // Calculate Energy
                QVector<double> energyArray(current->meanMaxArray(RideFile::watts).size());
                for (int i = 0; i <= maxNonZero; ++i) {
                    energyArray[i] = 
                    timeArray[i] * 
                    current->meanMaxArray(RideFile::watts)[i] * 60.0 / 1000.0;
                }
                thisCurve->setData(timeArray.data() + 1, energyArray.constData() + 1, maxNonZero - 1);

            } else {

                // normal
                thisCurve->setData(timeArray.data() + 1, 
                current->meanMaxArray(series).constData() + 1, maxNonZero - 1);
            }
        }
    }

    replot();
}

void
CpintPlot::showGrid(int state)
{
    grid->setVisible(state == Qt::Checked);
    replot();
}

void
CpintPlot::pointHover(QwtPlotCurve *curve, int index)
{
    if (index >= 0) {

        double xvalue = curve->sample(index).x();
        double yvalue = curve->sample(index).y();
        QString text, dateStr;

        // add when to tooltip if its all curve
        if (allCurves.contains(curve)) {
            int index = xvalue * 60;
            if (index >= 0 && getBests().count() > index) {
                QDate date = getBestDates()[index];
                dateStr = date.toString("\nddd, dd MMM yyyy");
            }
        }

            // output the tooltip
        text = QString("%1\n%3 %4%5")
            .arg(interval_to_str(60.0*xvalue))
            .arg(yvalue, 0, 'f', RideFile::decimalsFor(series))
            .arg(RideFile::unitName(series, context))
            .arg(dateStr);

        // set that text up
        zoomer->setText(text);
        return;
    }
    // no point
    zoomer->setText("");
}

void
CpintPlot::clearFilter()
{
    isFiltered = false;
    files.clear();
    delete bests;
    bests = NULL;
}

void
CpintPlot::setFilter(QStringList list)
{
    isFiltered = true;
    files = list;
    delete bests;
    bests = NULL;
}

void
CpintPlot::setShadeMode(int x)
{
    shadeMode = x;
}
