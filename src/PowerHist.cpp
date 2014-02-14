/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
 *               2011 Mark Liversedge (liversedge@gmail.com)
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

#include "PowerHist.h"
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "RideFile.h"
#include "RideFileCache.h"
#include "SummaryMetrics.h"
#include "Settings.h"
#include "Zones.h"
#include "HrZones.h"
#include "Colors.h"

#include "ZoneScaleDraw.h"

#include <qpainter.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_zoomer.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>
#include <qwt_text.h>
#include <qwt_legend.h>
#include <qwt_series_data.h>

#include "LTMCanvasPicker.h" // for tooltip

PowerHist::PowerHist(Context *context, bool rangemode) :
    minX(0),
    maxX(0),
    rangemode(rangemode),
    rideItem(NULL),
    context(context),
    series(RideFile::watts),
    lny(false),
    shade(false),
    zoned(false),
    cpzoned(false),
    binw(3),
    withz(true),
    dt(1),
    absolutetime(true),
    cache(NULL),
    source(Ride)
{
    binw = appsettings->value(this, GC_HIST_BIN_WIDTH, 5).toInt();
    if (appsettings->value(this, GC_SHADEZONES, true).toBool() == true)
        shade = true;
    else
        shade = false;

    // create a background object for shading
    bg = new PowerHistBackground(this);
    bg->attach(this);

    hrbg = new HrHistBackground(this);
    hrbg->attach(this);

    setCanvasBackground(Qt::white);
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

    setParameterAxisTitle();
    setAxisTitle(yLeft, absolutetime ? tr("Time (minutes)") : tr("Time (percent)"));

    curve = new QwtPlotCurve("");
    curve->setStyle(QwtPlotCurve::Steps);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    curve->attach(this);

    curveSelected = new QwtPlotCurve("");
    curveSelected->setStyle(QwtPlotCurve::Steps);
    curveSelected->setRenderHint(QwtPlotItem::RenderAntialiased);
    curveSelected->attach(this);

    grid = new QwtPlotGrid();
    grid->enableX(false);
    grid->attach(this);

    zoneLabels = QList<PowerHistZoneLabel *>();
    hrzoneLabels = QList<HrHistZoneLabel *>();

    zoomer = new penTooltip(this->canvas());
    canvasPicker = new LTMCanvasPicker(this);
    connect(canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)), this, SLOT(pointHover(QwtPlotCurve*, int)));

    // usually hidden, but shown for compare mode
    //XXX insertLegend(new QwtLegend(), QwtPlot::BottomLegend);

    setAxisMaxMinor(xBottom, 0);
    setAxisMaxMinor(yLeft, 0);

    configChanged();
}

void
PowerHist::configChanged()
{
    // plot background
    setCanvasBackground(GColor(CPLOTBACKGROUND));

    // curve
    QPen pen;
    QColor brush_color;

    if (source == Metric) {

        pen.setColor(metricColor.darker(200));
        brush_color = metricColor;

    } else {

        switch (series) {
        case RideFile::watts:
        case RideFile::aPower:
        case RideFile::wattsKg:
            pen.setColor(GColor(CPOWER).darker(200));
            brush_color = GColor(CPOWER);
            break;
        case RideFile::nm:
            pen.setColor(GColor(CTORQUE).darker(200));
            brush_color = GColor(CTORQUE);
            break;
        case RideFile::kph:
            pen.setColor(GColor(CSPEED).darker(200));
            brush_color = GColor(CSPEED);
            break;
        case RideFile::cad:
            pen.setColor(GColor(CCADENCE).darker(200));
            brush_color = GColor(CCADENCE);
            break;
        default:
        case RideFile::hr:
            pen.setColor(GColor(CHEARTRATE).darker(200));
            brush_color = GColor(CHEARTRATE);
            break;
        }
    }

    double width = appsettings->value(this, GC_LINEWIDTH, 2.0).toDouble();
    if (appsettings->value(this, GC_ANTIALIAS, false).toBool()==true) {
        curve->setRenderHint(QwtPlotItem::RenderAntialiased);
        curveSelected->setRenderHint(QwtPlotItem::RenderAntialiased);
    }

    // use a linear gradient
    brush_color.setAlpha(GColor(CPLOTBACKGROUND) == QColor(Qt::white) ? 64 : 200);
    QColor brush_color1 = brush_color.darker();
    QLinearGradient linearGradient(0, 0, 0, height());
    linearGradient.setColorAt(0.0, brush_color);
    linearGradient.setColorAt(1.0, brush_color1);
    linearGradient.setSpread(QGradient::PadSpread);
    curve->setBrush(linearGradient);   // fill below the line

    if (zoned == false || (zoned == true && (series != RideFile::watts && series != RideFile::wattsKg && series != RideFile::hr))) {
        pen.setWidth(width);
        curve->setPen(pen);

    } else {
        pen.setWidth(width);
        curve->setPen(QPen(Qt::NoPen));
    }

    // intervalselection
    QPen ivl(GColor(CINTERVALHIGHLIGHTER).darker(200));
    ivl.setWidth(width);
    curveSelected->setPen(ivl);
    QColor ivlbrush = GColor(CINTERVALHIGHLIGHTER);
    ivlbrush.setAlpha(GColor(CPLOTBACKGROUND) == QColor(Qt::white) ? 64 : 200);
    curveSelected->setBrush(ivlbrush);   // fill below the line

    // grid
    QPen gridPen(GColor(CPLOTGRID));
    //gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);

    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    setPalette(palette);

    axisWidget(QwtPlot::xBottom)->setPalette(palette);
    axisWidget(QwtPlot::yLeft)->setPalette(palette);

    setAutoFillBackground(true);
}

void
PowerHist::hideStandard(bool hide)
{
    bg->setVisible(!hide);
    hrbg->setVisible(!hide);
    curve->setVisible(!hide);
    curveSelected->setVisible(!hide);

    if (!hide) {

        // wipe the compare data 
        compareData.clear();

        // we want normal so zap any compare curves
        foreach(QwtPlotCurve *x, compareCurves) {
            x->detach();
            delete x;
        }
        compareCurves.clear();
    }
}

PowerHist::~PowerHist() {
    delete bg;
    delete hrbg;
    delete curve;
    delete curveSelected;
    delete grid;
}

void
PowerHist::refreshZoneLabels()
{
    // delete any existing power zone labels
    if (zoneLabels.size()) {
        QListIterator<PowerHistZoneLabel *> i(zoneLabels);
        while (i.hasNext()) {
            PowerHistZoneLabel *label = i.next();
            label->detach();
            delete label;
        }
    }
    zoneLabels.clear();

    if (!rideItem) return;

    if (series == RideFile::watts || series == RideFile::wattsKg) {
        const Zones *zones = rideItem->zones;
        int zone_range = rideItem->zoneRange();

        // generate labels for existing zones
        if (zone_range >= 0) {
            int num_zones = zones->numZones(zone_range);
            for (int z = 0; z < num_zones; z ++) {
                PowerHistZoneLabel *label = new PowerHistZoneLabel(this, z);
                label->attach(this);
                zoneLabels.append(label);
            }
        }
    }
}

void
PowerHist::refreshHRZoneLabels()
{
    // delete any existing power zone labels
    if (hrzoneLabels.size()) {
        QListIterator<HrHistZoneLabel *> i(hrzoneLabels);
        while (i.hasNext()) {
            HrHistZoneLabel *label = i.next();
            label->detach();
            delete label;
        }
    }
    hrzoneLabels.clear();

    if (!rideItem) return;

    if (series == RideFile::hr) {
        const HrZones *zones       = context->athlete->hrZones();
        int zone_range     = rideItem->hrZoneRange();

        // generate labels for existing zones
        if (zone_range >= 0) {
            int num_zones = zones->numZones(zone_range);
            for (int z = 0; z < num_zones; z ++) {
                HrHistZoneLabel *label = new HrHistZoneLabel(this, z);
                label->attach(this);
                hrzoneLabels.append(label);
            }
        }
    }
}

void
PowerHist::recalcCompare()
{
    // Set curves .. they will always have been created 
    //               in setDataFromCompareIntervals, but no samples set

    // don't bother if not comparing for our mode or we're not visible
    if (!isVisible() ||
        ((rangemode && !context->isCompareDateRanges)
        || (!rangemode && !context->isCompareIntervals))) return;

    // loop through intervals or dateranges, depending upon mode
    // counting columns
    double ncols = 0;
    for(int i=0; (rangemode && i < context->compareDateRanges.count()) ||
                 (!rangemode && i < context->compareIntervals.count()) ; i++) {

        if (rangemode && context->compareDateRanges[i].isChecked()) ncols++;
        if (!rangemode && context->compareIntervals[i].isChecked()) ncols++;
    }
    int acol = 0;
    int maxX = 0;

    // loop again but now setting up data
    for(int intervalNumber=0; (rangemode && intervalNumber < context->compareDateRanges.count()) ||
                 (!rangemode && intervalNumber < context->compareIntervals.count()) ; intervalNumber++) {

        HistData &cid = compareData[intervalNumber];
        QwtPlotCurve *curve = compareCurves[intervalNumber];
        QVector<unsigned int> *array = NULL;
        int arrayLength = 0;

        if (source == Metric) {

            // we use the metricArray
            array = &cid.metricArray;
            arrayLength = cid.metricArray.size();

        } else if (series == RideFile::watts && zoned == false) {

            array = &cid.wattsArray;
            arrayLength = cid.wattsArray.size();

        } else if ((series == RideFile::watts || series == RideFile::wattsKg) && zoned == true) {

            array = &cid.wattsZoneArray;
            arrayLength = cid.wattsZoneArray.size();

        } else if (series == RideFile::aPower && zoned == false) {

            array = &cid.aPowerArray;
            arrayLength = cid.aPowerArray.size();

        } else if (series == RideFile::wattsKg && zoned == false) {

            array = &cid.wattsKgArray;
            arrayLength = cid.wattsKgArray.size();

        } else if (series == RideFile::nm) {

            array = &cid.nmArray;
            arrayLength = cid.nmArray.size();

        } else if (series == RideFile::hr && zoned == false) {

            array = &cid.hrArray;
            arrayLength = cid.hrArray.size();

        } else if (series == RideFile::hr && zoned == true) {

            array = &cid.hrZoneArray;
            arrayLength = cid.hrZoneArray.size();

        } else if (series == RideFile::kph) {

            array = &cid.kphArray;
            arrayLength = cid.kphArray.size();

        } else if (series == RideFile::cad) {
            array = &cid.cadArray;
            arrayLength = cid.cadArray.size();
        }

        // UNUSEDRideFile::SeriesType baseSeries = (series == RideFile::wattsKg) ? RideFile::watts : series;

        // null curve please -- we have no data!
        if (!array || arrayLength == 0) {
            // create empty curves when no data
            const double zero = 0;
            curve->setSamples(&zero, &zero, 0);
            continue;
        }

        if (zoned == false || (zoned == true && (series != RideFile::watts && series != RideFile::wattsKg 
                                                                           && series != RideFile::hr))) {

            // NOT ZONED
            // we add a bin on the end since the last "incomplete" bin
            // will be dropped otherwise
            int count = int(ceil((arrayLength - 1) / (binw)))+1;

            // allocate space for data, plus beginning and ending point
            QVector<double> parameterValue(count+2, 0.0);
            QVector<double> totalTime(count+2, 0.0);
            int i;
            for (i = 1; i <= count; ++i) {
                double high = i * round(binw/delta);
                double low = high - round(binw/delta);
                if (low==0 && !withz) low++;
                parameterValue[i] = high*delta;
                totalTime[i]  = 1e-9;  // nonzero to accomodate log plot
                while (low < high && low<arrayLength) {
                    totalTime[i] += dt * (*array)[low++];
                }
            }

            totalTime[i] = 1e-9;       // nonzero to accomodate log plot
            parameterValue[i] = i * delta * binw;
            totalTime[0] = 1e-9;
            parameterValue[0] = 0;

            // convert vectors from absolute time to percentage
            // if the user has selected that
            if (!absolutetime) {
                percentify(totalTime, 1);
            }

            curve->setSamples(parameterValue.data(), totalTime.data(), count + 2);

            QwtScaleDraw *sd = new QwtScaleDraw;
            sd->setTickLength(QwtScaleDiv::MajorTick, 3);
            setAxisScaleDraw(QwtPlot::xBottom, sd);

            // HR typically starts at 80 or so, rather than zero
            // lets crop the chart so we can focus on the data
            // if we're working with HR data...
            minX=0;
            if (!withz && series == RideFile::hr) {
                for (int i=1; i<cid.hrArray.size(); i++) {
                    if (cid.hrArray[i] > 0.1) {
                        minX = i;
                        break;
                    }
                }
            }

            // only set X-axis to largest value with significant value
            int truncate = count;
            while (truncate > 0) {
                if (!absolutetime && totalTime[truncate] >= 0.1) break;
                if (absolutetime && totalTime[truncate] >= 0.1) break;
                truncate--;
            }
            if (parameterValue[truncate] > maxX) maxX = parameterValue[truncate];

            // we only do zone labels when using absolute values
            refreshZoneLabels();
            refreshHRZoneLabels();
    
        } else { // ZONED

            // 0.625 = golden ratio for gaps betwen group of cols
            // 0.9 = 10% space between each col in group

            double width = (0.625 / ncols) * 0.90f;
            double jump = acol * (0.625 / ncols);
            
            // we're not binning instead we are prettyfing the columnar
            // display in much the same way as the weekly summary workds
            // Each zone column will have 4 points
            QVector<double> xaxis (array->size() * 4);
            QVector<double> yaxis (array->size() * 4);

            // samples to time
            for (int i=0, offset=0; i<array->size(); i++) {

                double x = double(i) - (0.625f * 0.90f / 2.0f);
                double y = dt * (double)(*array)[i];

                xaxis[offset] = x +jump;
                yaxis[offset] = 0;
                offset++;
                xaxis[offset] = x +jump;
                yaxis[offset] = y;
                offset++;
                xaxis[offset] = x +jump +width;
                yaxis[offset] = y;
                offset++;
                xaxis[offset] = x +jump +width;
                yaxis[offset] = 0;
                offset++;
            }

            if (!absolutetime) {
                percentify(yaxis, 2);
            }

            // set those curves
            curve->setPen(QPen(Qt::NoPen));
            curve->setSamples(xaxis.data(), yaxis.data(), xaxis.size());

            //
            // POWER ZONES
            //
            const Zones *zones;
            int zone_range = -1;
            zones = context->athlete->zones();

            if (zones) {
                if (context->compareIntervals.count())
                    zone_range = zones->whichRange(context->compareIntervals[0].data->startTime().date());
                if (zone_range == -1) zone_range = zones->whichRange(QDate::currentDate());

            }
            if (zones && zone_range != -1) {
                if ((series == RideFile::watts || series == RideFile::wattsKg)) {
                    setAxisScaleDraw(QwtPlot::xBottom, new ZoneScaleDraw(zones, zone_range));
                    setAxisScale(QwtPlot::xBottom, -0.99, zones->numZones(zone_range), 1);
                }
            }

            //
            // HR ZONES
            //
            const HrZones *hrzones;
            int hrzone_range = -1;
            hrzones = context->athlete->hrZones();

            if (hrzones) {
                if (context->compareIntervals.count())
                    hrzone_range = hrzones->whichRange(context->compareIntervals[0].data->startTime().date());
                if (hrzone_range == -1) hrzone_range = hrzones->whichRange(QDate::currentDate());

            }
            if (hrzones && hrzone_range != -1) {
                if (series == RideFile::hr) {
                    setAxisScaleDraw(QwtPlot::xBottom, new HrZoneScaleDraw(hrzones, hrzone_range));
                    setAxisScale(QwtPlot::xBottom, -0.99, hrzones->numZones(hrzone_range), 1);
                }
            }

            setAxisMaxMinor(QwtPlot::xBottom, 0);

            // keep track of columns visible -- depending upon mode
            if (!rangemode && context->compareIntervals[intervalNumber].isChecked()) acol++;
            if (rangemode && context->compareDateRanges[intervalNumber].isChecked()) acol++;
        }
    }

    // set axis etc
    if (zoned == false || (zoned == true && (series != RideFile::watts && series != RideFile::wattsKg 
                                                                           && series != RideFile::hr))) {
        //normal
        setAxisScale(xBottom, minX, maxX);
    } else {
        // zoned
    }
    setYMax(); 
    updatePlot();
}

void
PowerHist::recalc(bool force)
{
    if ((!rangemode && context->isCompareIntervals) ||
       (rangemode && context->isCompareDateRanges)) {
        recalcCompare();
        return;
    }

    QVector<unsigned int> *array = NULL;
    QVector<unsigned int> *selectedArray = NULL;
    int arrayLength = 0;

    // lets make sure we need to recalculate
    if (force == false &&
        LASTsource == source &&
        LASTcache == cache &&
        LASTrideItem == rideItem &&
        LASTseries == series &&
        LASTshade == shade &&
        LASTuseMetricUnits == context->athlete->useMetricUnits &&
        LASTlny == lny &&
        LASTzoned == zoned &&
        LASTcpzoned == cpzoned &&
        LASTbinw == binw &&
        LASTwithz == withz &&
        LASTdt == dt &&
        LASTabsolutetime == absolutetime) {
        return; // nothing has changed

    } else {

        // remember for next time
        LASTsource = source;
        LASTcache = cache;
        LASTrideItem = rideItem;
        LASTseries = series;
        LASTshade = shade;
        LASTuseMetricUnits = context->athlete->useMetricUnits;
        LASTlny = lny;
        LASTzoned = zoned;
        LASTcpzoned = cpzoned;
        LASTbinw = binw;
        LASTwithz = withz;
        LASTdt = dt;
        LASTabsolutetime = absolutetime;
    }


    if (source == Ride && !rideItem) { 
        return;
    }

    // make sure the interval length is set if not plotting metrics
    if (source != Metric && dt <= 0) {
        return;
    }

    if (source == Metric) {

        // we use the metricArray
        array = &standard.metricArray;
        arrayLength = standard.metricArray.size();
        selectedArray = NULL;

    } else if (series == RideFile::watts && zoned == false) {

        array = &standard.wattsArray;
        arrayLength = standard.wattsArray.size();
        selectedArray = &standard.wattsSelectedArray;

    } else if ((series == RideFile::watts || series == RideFile::wattsKg) && zoned == true) {
        if (cpzoned) {
            array = &standard.wattsCPZoneArray;
            arrayLength = standard.wattsCPZoneArray.size();
            selectedArray = &standard.wattsCPZoneSelectedArray;
        } else {
            array = &standard.wattsZoneArray;
            arrayLength = standard.wattsZoneArray.size();
            selectedArray = &standard.wattsZoneSelectedArray;
        }

    } else if (series == RideFile::aPower && zoned == false) {

        array = &standard.aPowerArray;
        arrayLength = standard.aPowerArray.size();
        selectedArray = &standard.aPowerSelectedArray;

    } else if (series == RideFile::wattsKg && zoned == false) {

        array = &standard.wattsKgArray;
        arrayLength = standard.wattsKgArray.size();
        selectedArray = &standard.wattsKgSelectedArray;

    } else if (series == RideFile::nm) {

        array = &standard.nmArray;
        arrayLength = standard.nmArray.size();
        selectedArray = &standard.nmSelectedArray;

    } else if (series == RideFile::hr && zoned == false) {

        array = &standard.hrArray;
        arrayLength = standard.hrArray.size();
        selectedArray = &standard.hrSelectedArray;

    } else if (series == RideFile::hr && zoned == true) {

        array = &standard.hrZoneArray;
        arrayLength = standard.hrZoneArray.size();
        selectedArray = &standard.hrZoneSelectedArray;

    } else if (series == RideFile::kph) {

        array = &standard.kphArray;
        arrayLength = standard.kphArray.size();
        selectedArray = &standard.kphSelectedArray;

    } else if (series == RideFile::cad) {
        array = &standard.cadArray;
        arrayLength = standard.cadArray.size();
        selectedArray = &standard.cadSelectedArray;
    }

    RideFile::SeriesType baseSeries = (series == RideFile::wattsKg) ? RideFile::watts : series;

    // null curve please -- we have no data!
    if (!array || arrayLength == 0 || (source == Ride && !rideItem->ride()->isDataPresent(baseSeries))) {
        // create empty curves when no data
        const double zero = 0;
        curve->setSamples(&zero, &zero, 0);
        curveSelected->setSamples(&zero, &zero, 0);
        updatePlot();
        return;
    }

    // binning of data when not zoned - we can't zone for series besides
    // watts and hr so ignore zoning for those data series
    if (zoned == false || (zoned == true && (series != RideFile::watts && series != RideFile::wattsKg && series != RideFile::hr))) {

        // we add a bin on the end since the last "incomplete" bin
        // will be dropped otherwise
        int count = int(ceil((arrayLength - 1) / (binw)))+1;

        // allocate space for data, plus beginning and ending point
        QVector<double> parameterValue(count+2, 0.0);
        QVector<double> totalTime(count+2, 0.0);
        QVector<double> totalTimeSelected(count+2, 0.0);
        int i;
        for (i = 1; i <= count; ++i) {
            double high = i * round(binw/delta);
            double low = high - round(binw/delta);
            if (low==0 && !withz) low++;
            parameterValue[i] = high*delta;
            totalTime[i]  = 1e-9;  // nonzero to accomodate log plot
            totalTimeSelected[i] = 1e-9;  // nonzero to accomodate log plot
            while (low < high && low<arrayLength) {
                if (selectedArray && (*selectedArray).size()>low)
                    totalTimeSelected[i] += dt * (*selectedArray)[low];
                totalTime[i] += dt * (*array)[low++];
            }
        }
        totalTime[i] = 1e-9;       // nonzero to accomodate log plot
        totalTimeSelected[i] = 1e-9;       // nonzero to accomodate log plot
        parameterValue[i] = i * delta * binw;
        totalTime[0] = 1e-9;
        totalTimeSelected[0] = 1e-9;
        parameterValue[0] = 0;

        // convert vectors from absolute time to percentage
        // if the user has selected that
        if (!absolutetime) {
            percentify(totalTime, 1);
            percentify(totalTimeSelected, 1);
        }

        curve->setSamples(parameterValue.data(), totalTime.data(), count + 2);
        curveSelected->setSamples(parameterValue.data(), totalTimeSelected.data(), count + 2);

        QwtScaleDraw *sd = new QwtScaleDraw;
        sd->setTickLength(QwtScaleDiv::MajorTick, 3);
        setAxisScaleDraw(QwtPlot::xBottom, sd);

        // HR typically starts at 80 or so, rather than zero
        // lets crop the chart so we can focus on the data
        // if we're working with HR data...
        minX=0;
        if (!withz && series == RideFile::hr) {
            for (int i=1; i<standard.hrArray.size(); i++) {
                if (standard.hrArray[i] > 0.1) {
                    minX = i;
                    break;
                }
            }
        }
        setAxisScale(xBottom, minX, parameterValue[count + 1]);

        // we only do zone labels when using absolute values
        refreshZoneLabels();
        refreshHRZoneLabels();

    } else {

        // we're not binning instead we are prettyfing the columnar
        // display in much the same way as the weekly summary workds
        // Each zone column will have 4 points
        QVector<double> xaxis (array->size() * 4);
        QVector<double> yaxis (array->size() * 4);
        QVector<double> selectedxaxis (selectedArray->size() * 4);
        QVector<double> selectedyaxis (selectedArray->size() * 4);

        // samples to time
        for (int i=0, offset=0; i<array->size(); i++) {

            double x = (double) i - (0.625f / 2.0f);
            double y = dt * (double)(*array)[i];

            xaxis[offset] = x;
            yaxis[offset] = 0;
            offset++;
            xaxis[offset] = x;
            yaxis[offset] = y;
            offset++;
            xaxis[offset] = x+0.625;
            yaxis[offset] = y;
            offset++;
            xaxis[offset] = x +0.625;
            yaxis[offset] = 0;
            offset++;
        }

        for (int i=0, offset=0; i<selectedArray->size(); i++) {
            double x = (double)i - 0.5;
            double y = dt * (double)(*selectedArray)[i];

            selectedxaxis[offset] = x +0.05;
            selectedyaxis[offset] = 0;
            offset++;
            selectedxaxis[offset] = x+0.05;
            selectedyaxis[offset] = y;
            offset++;
            selectedxaxis[offset] = x+0.95;
            selectedyaxis[offset] = y;
            offset++;
            selectedxaxis[offset] = x +0.95;
            selectedyaxis[offset] = 0;
            offset++;
        }

        if (!absolutetime) {
            percentify(yaxis, 2);
            percentify(selectedyaxis, 2);
        }

        // set those curves
        curve->setSamples(xaxis.data(), yaxis.data(), xaxis.size());
        curveSelected->setSamples(selectedxaxis.data(), selectedyaxis.data(), selectedxaxis.size());

        // zone scale draw
        if ((series == RideFile::watts || series == RideFile::wattsKg) && zoned && rideItem && rideItem->zones) {

            if (cpzoned) {
                setAxisScaleDraw(QwtPlot::xBottom, new PolarisedZoneScaleDraw());
                setAxisScale(QwtPlot::xBottom, -0.99, 3, 1);
            } else {
                setAxisScaleDraw(QwtPlot::xBottom, new ZoneScaleDraw(rideItem->zones, rideItem->zoneRange()));
                if (rideItem->zoneRange() >= 0)
                    setAxisScale(QwtPlot::xBottom, -0.99, rideItem->zones->numZones(rideItem->zoneRange()), 1);
                else
                    setAxisScale(QwtPlot::xBottom, -0.99, 0, 1);
            }
        }

        // hr scale draw
        int hrRange;
        if (series == RideFile::hr && zoned && rideItem && context->athlete->hrZones() &&
            (hrRange=context->athlete->hrZones()->whichRange(rideItem->dateTime.date())) != -1) {
            setAxisScaleDraw(QwtPlot::xBottom, new HrZoneScaleDraw(context->athlete->hrZones(), hrRange));

            if (hrRange >= 0)
                setAxisScale(QwtPlot::xBottom, -0.99, context->athlete->hrZones()->numZones(hrRange), 1);
            else
                setAxisScale(QwtPlot::xBottom, -0.99, 0, 1);
        }

        // watts zoned for a time range
        if (source == Cache && zoned && (series == RideFile::watts || series == RideFile::wattsKg) && context->athlete->zones()) {
            if (cpzoned) {
                setAxisScaleDraw(QwtPlot::xBottom, new PolarisedZoneScaleDraw());
                setAxisScale(QwtPlot::xBottom, -0.99, 3, 1);
            } else {
                setAxisScaleDraw(QwtPlot::xBottom, new ZoneScaleDraw(context->athlete->zones(), 0));
                if (context->athlete->zones()->getRangeSize())
                    setAxisScale(QwtPlot::xBottom, -0.99, context->athlete->zones()->numZones(0), 1); // use zones from first defined range
            }
        }

        // hr zoned for a time range
        if (source == Cache && zoned && series == RideFile::hr && context->athlete->hrZones()) {
            setAxisScaleDraw(QwtPlot::xBottom, new HrZoneScaleDraw(context->athlete->hrZones(), 0));
            if (context->athlete->hrZones()->getRangeSize())
                setAxisScale(QwtPlot::xBottom, -0.99, context->athlete->hrZones()->numZones(0), 1); // use zones from first defined range
        }

        setAxisMaxMinor(QwtPlot::xBottom, 0);
    }

    setYMax();
    configChanged(); // setup the curve colors to appropriate values
    updatePlot();
}

void
PowerHist::setYMax()
{
    double MaxY=0;

    if (!rangemode && context->isCompareIntervals) {
        int i=0;
        foreach (QwtPlotCurve *p, compareCurves) {

            // if its not visible don't set for it
            if (context->compareIntervals[i].isChecked()) {
                double my = p->maxYValue();
                if (my > MaxY) MaxY = my;
            }
            i++;
        }

    } else if (rangemode && context->isCompareDateRanges) {

        int i=0;
        foreach (QwtPlotCurve *p, compareCurves) {

            // if its not visible don't set for it
            if (context->compareDateRanges[i].isChecked()) {
                double my = p->maxYValue();
                if (my > MaxY) MaxY = my;
            }
            i++;
        }

    } else {

        MaxY = curve->maxYValue();
        if (MaxY < curveSelected->maxYValue()) MaxY = curveSelected->maxYValue();

    }

    static const double tmin = 1.0/60;
    setAxisScale(yLeft, (lny ? tmin : 0.0), MaxY * 1.1);

    QwtScaleDraw *sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    setAxisScaleDraw(QwtPlot::yLeft, sd);
}

static void
longFromDouble(QVector<unsigned int>&here, QVector<double>&there)
{
    int highest = 0;
    here.resize(there.size());
    for (int i=0; i<here.size(); i++) {
        here[i] = there[i];
        if (here[i] != 0) highest = i;
    }
    here.resize(highest);
}

void
PowerHist::setData(RideFileCache *cache)
{
    source = Cache;
    this->cache = cache;
    dt = 1.0f / 60.0f; // rideFileCache is normalised to 1secs

    // we set with this data already?
    if (cache == LASTcache && source == LASTsource) return;

    // Now go set all those tedious arrays from
    // the ride cache
    standard.wattsArray.resize(0);
    standard.wattsZoneArray.resize(10);
    standard.wattsCPZoneArray.resize(4);
    standard.wattsKgArray.resize(0);
    standard.aPowerArray.resize(0);
    standard.nmArray.resize(0);
    standard.hrArray.resize(0);
    standard.hrZoneArray.resize(10);
    standard.kphArray.resize(0);
    standard.cadArray.resize(0);

    // we do not use the selected array since it is
    // not meaningful to overlay interval selection
    // with long term data
    standard.wattsSelectedArray.resize(0);
    standard.wattsZoneSelectedArray.resize(0);
    standard.wattsKgSelectedArray.resize(0);
    standard.aPowerSelectedArray.resize(0);
    standard.nmSelectedArray.resize(0);
    standard.hrSelectedArray.resize(0);
    standard.hrZoneSelectedArray.resize(0);
    standard.kphSelectedArray.resize(0);
    standard.cadSelectedArray.resize(0);

    longFromDouble(standard.wattsArray, cache->distributionArray(RideFile::watts));
    longFromDouble(standard.wattsKgArray, cache->distributionArray(RideFile::wattsKg));
    longFromDouble(standard.aPowerArray, cache->distributionArray(RideFile::aPower));
    longFromDouble(standard.hrArray, cache->distributionArray(RideFile::hr));
    longFromDouble(standard.nmArray, cache->distributionArray(RideFile::nm));
    longFromDouble(standard.cadArray, cache->distributionArray(RideFile::cad));
    longFromDouble(standard.kphArray, cache->distributionArray(RideFile::kph));

    if (!context->athlete->useMetricUnits) {
        double torque_factor = (context->athlete->useMetricUnits ? 1.0 : 0.73756215);
        double speed_factor  = (context->athlete->useMetricUnits ? 1.0 : 0.62137119);

        for(int i=0; i<standard.nmArray.size(); i++) standard.nmArray[i] = standard.nmArray[i] * torque_factor;
        for(int i=0; i<standard.kphArray.size(); i++) standard.kphArray[i] = standard.kphArray[i] * speed_factor;
    }

    // zone array
    for (int i=0; i<10; i++) {
        standard.wattsZoneArray[i] = cache->wattsZoneArray()[i];
        standard.hrZoneArray[i] = cache->hrZoneArray()[i];
    }

    // polarised zones
    standard.wattsCPZoneArray[0] = cache->wattsCPZoneArray()[1];
    if (withz) {
        standard.wattsCPZoneArray[0] += cache->wattsCPZoneArray()[0]; // add in zero watts
    }
    standard.wattsCPZoneArray[1] = cache->wattsCPZoneArray()[2];
    standard.wattsCPZoneArray[2] = cache->wattsCPZoneArray()[3];

    curveSelected->hide();
}

void
PowerHist::setDataFromCompare()
{
    // not for metric plots sonny
    if (source == Metric) return;

    double width = appsettings->value(this, GC_LINEWIDTH, 2.0).toDouble();

    // set all the curves based upon whats in the compare intervals array
    // first lets clear the old data
    compareData.clear();

    // and remove the old curves
    foreach(QwtPlotCurve *x, compareCurves) {
        x->detach();
        delete x;
    }
    compareCurves.clear();

    // now lets setup a HistData for each CompareInterval
    for(int intervalNumber=0; (rangemode && intervalNumber < context->compareDateRanges.count()) ||
                 (!rangemode && intervalNumber < context->compareIntervals.count()) ; intervalNumber++) {

        // get the bits we need from either the interval or daterange compare object
        RideFileCache *s = rangemode ? context->compareDateRanges[intervalNumber].rideFileCache() :
                                       context->compareIntervals[intervalNumber].rideFileCache();

        QColor color = rangemode ? context->compareDateRanges[intervalNumber].color :
                                       context->compareIntervals[intervalNumber].color;

        bool ischecked = rangemode ? context->compareDateRanges[intervalNumber].isChecked() :
                                       context->compareIntervals[intervalNumber].isChecked();

        // set the data even if not checked
        HistData add;

        // Now go set all those tedious arrays from
        // the ride cache
        add.wattsArray.resize(0);
        add.wattsZoneArray.resize(10);
        add.wattsKgArray.resize(0);
        add.aPowerArray.resize(0);
        add.nmArray.resize(0);
        add.hrArray.resize(0);
        add.hrZoneArray.resize(10);
        add.kphArray.resize(0);
        add.cadArray.resize(0);

        longFromDouble(add.wattsArray, s->distributionArray(RideFile::watts));
        longFromDouble(add.wattsKgArray, s->distributionArray(RideFile::wattsKg));
        longFromDouble(add.aPowerArray, s->distributionArray(RideFile::aPower));
        longFromDouble(add.hrArray, s->distributionArray(RideFile::hr));
        longFromDouble(add.nmArray, s->distributionArray(RideFile::nm));
        longFromDouble(add.cadArray, s->distributionArray(RideFile::cad));
        longFromDouble(add.kphArray, s->distributionArray(RideFile::kph));

        // convert for metric imperial types
        if (!context->athlete->useMetricUnits) {
            double torque_factor = (context->athlete->useMetricUnits ? 1.0 : 0.73756215);
            double speed_factor  = (context->athlete->useMetricUnits ? 1.0 : 0.62137119);

            for(int i=0; i<add.nmArray.size(); i++) add.nmArray[i] = add.nmArray[i] * torque_factor;
            for(int i=0; i<add.kphArray.size(); i++) add.kphArray[i] = add.kphArray[i] * speed_factor;
        }

        // zone array
        for (int i=0; i<10; i++) {
            add.wattsZoneArray[i] = s->wattsZoneArray()[i];
            add.hrZoneArray[i] = s->hrZoneArray()[i];
        }

        // add to the list
        compareData << add;

        // now add a curve for recalc to play with
        QwtPlotCurve *newCurve = new QwtPlotCurve("");
        newCurve->setStyle(QwtPlotCurve::Steps);

        if (appsettings->value(this, GC_ANTIALIAS, false).toBool()==true)
            newCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

        // curve has no brush .. too confusing...
        QPen pen;
        pen.setColor(color);
        pen.setWidth(width);
        newCurve->setPen(pen);

        QColor brush_color = color;
        brush_color.setAlpha(GColor(CPLOTBACKGROUND) == QColor(Qt::white) ? 120 : 200);
        QColor brush_color1 = brush_color.darker();
        //QLinearGradient linearGradient(0, 0, 0, height());
        //linearGradient.setColorAt(0.0, brush_color);
        //linearGradient.setColorAt(1.0, brush_color1);
        //linearGradient.setSpread(QGradient::PadSpread);
        newCurve->setBrush(brush_color1);   // fill below the line

        // hide and show, but always attach
        newCurve->setVisible(ischecked);
        newCurve->attach(this);

        // we do want a legend in compare mode
        //XXX newCurve->setItemAttribute(QwtPlotItem::Legend, true);

        // add to our collection
        compareCurves << newCurve;
    }

    // show legend in compare mode
    //XXX legend()->show();
    //XXX updateLegend();
}

void
PowerHist::setComparePens()
{
    // no compare? don't bother
    if ((!rangemode && !context->isCompareIntervals) ||
        (rangemode && !context->isCompareDateRanges)) return;

    double width = appsettings->value(this, GC_LINEWIDTH, 2.0).toDouble();
    for (int i=0; (!rangemode && i<context->compareIntervals.count()) ||
                  (rangemode && i<context->compareDateRanges.count()); i++) {

        if (zoned == false || (zoned == true && (series != RideFile::watts && series != RideFile::wattsKg 
                                                                           && series != RideFile::hr))) {

            // NOT ZONED
            if (compareCurves.count() > i) {

                // set pen back
                QPen pen;
                pen.setColor(rangemode ? context->compareDateRanges[i].color :
                                         context->compareIntervals[i].color);
                pen.setWidth(width);
                compareCurves[i]->setPen(pen);
            }

        } else {

            if (compareCurves.count() > i) {

                // set pen back
                compareCurves[i]->setPen(QPen(Qt::NoPen));
            }
        }
    }
}

// set the metric arrays up for each date range using the
// summary metrics in the Context::CompareDateRange array
void
PowerHist::setDataFromCompare(QString totalMetric, QString distMetric)
{

    // set the data for each compare date range using
    // the metric results in the compare data range
    // and create the HistData and curves associated
    double width = appsettings->value(this, GC_LINEWIDTH, 2.0).toDouble();

    // remove old data and curves
    compareData.clear();
    foreach(QwtPlotCurve *x, compareCurves) {
        x->detach();
        delete x;
    }
    compareCurves.clear();

    // now lets setup a HistData for each CompareDate Range
    foreach(CompareDateRange cd, context->compareDateRanges) {

        // set the data even if not checked
        HistData add;

        // set it from the metric
        setData(cd.metrics,  totalMetric, distMetric, false, QStringList(), &add);

        // add to the list
        compareData << add;

        // now add a curve for recalc to play with
        QwtPlotCurve *newCurve = new QwtPlotCurve("");
        newCurve->setStyle(QwtPlotCurve::Steps);

        if (appsettings->value(this, GC_ANTIALIAS, false).toBool()==true)
            newCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

        // curve has no brush .. too confusing...
        QPen pen;
        pen.setColor(cd.color);
        pen.setWidth(width);
        newCurve->setPen(pen);

        QColor brush_color = cd.color;
        brush_color.setAlpha(GColor(CPLOTBACKGROUND) == QColor(Qt::white) ? 120 : 200);
        QColor brush_color1 = brush_color.darker();
        //QLinearGradient linearGradient(0, 0, 0, height());
        //linearGradient.setColorAt(0.0, brush_color);
        //linearGradient.setColorAt(1.0, brush_color1);
        //linearGradient.setSpread(QGradient::PadSpread);
        newCurve->setBrush(brush_color1);   // fill below the line

        // hide and show, but always attach
        newCurve->setVisible(cd.isChecked());
        newCurve->attach(this);

        // we do want a legend in compare mode
        //XXX newCurve->setItemAttribute(QwtPlotItem::Legend, true);

        // add to our collection
        compareCurves << newCurve;
    }
}

void 
PowerHist::setData(QList<SummaryMetrics>&results, QString totalMetric, QString distMetric,
                     bool isFiltered, QStringList files, HistData *data)
{
    // what metrics are we plotting?
    source = Metric;
    const RideMetricFactory &factory = RideMetricFactory::instance();
    const RideMetric *m = factory.rideMetric(distMetric);
    const RideMetric *tm = factory.rideMetric(totalMetric);
    if (m == NULL || tm == NULL) return;

    // metricX, metricY
    metricX = distMetric;
    metricY = totalMetric;

    // how big should the array be?
    double multiplier = pow(10, m->precision());
    double max = 0, min = 0;

    // LOOP THRU VALUES -- REPEATED WITH CUT AND PASTE BELOW
    // SO PLEASE MAKE SAME CHANGES TWICE (SORRY)
    foreach(SummaryMetrics x, results) { 

        // skip filtered values
        if (isFiltered && !files.contains(x.getFileName())) continue;

        // and global filter too
        if (context->isfiltered && !context->filters.contains(x.getFileName())) continue;

        // get computed value
        double v = x.getForSymbol(distMetric, context->athlete->useMetricUnits);

        // ignore no temp files
        if ((distMetric == "average_temp" || distMetric == "max_temp") && v == RideFile::noTemp) continue;

        // clean up dodgy values
        if (isnan(v) || isinf(v)) v = 0;

        // seconds to minutes
        if (m->units(context->athlete->useMetricUnits) == tr("seconds")) v /= 60;

        // apply multiplier
        v *= multiplier;

        if (v>max) max = v;
        if (v<min) min = v;
    }

    // lets truncate the data if there are very high
    // or very low max/min values, to ensure we don't exhaust memory
    if (max > 100000) max = 100000;
    if (min < -100000) min = -100000;

    // now run thru the data again, but this time
    // populate the metricArray
    // we add 1 to account for possible rounding up
    data->metricArray.resize(1 + (int)(max)-(int)(min));
    data->metricArray.fill(0);

    // LOOP THRU VALUES -- REPEATED WITH CUT AND PASTE ABOVE
    // SO PLEASE MAKE SAME CHANGES TWICE (SORRY)
    foreach(SummaryMetrics x, results) { 

        // skip filtered values
        if (isFiltered && !files.contains(x.getFileName())) continue;

        // and global filter too
        if (context->isfiltered && !context->filters.contains(x.getFileName())) continue;

        // get computed value
        double v = x.getForSymbol(distMetric, context->athlete->useMetricUnits);

        // ignore no temp files
        if ((distMetric == "average_temp" || distMetric == "max_temp") && v == RideFile::noTemp) continue;

        // clean up dodgy values
        if (isnan(v) || isinf(v)) v = 0;

        // seconds to minutes
        if (m->units(context->athlete->useMetricUnits) == tr("seconds")) v /= 60;

        // apply multiplier
        v *= multiplier;

        // ignore out of bounds data
        if ((int)(v)<min || (int)(v)>max) continue;

        // increment value, are intitialised to zero above
        // there will be some loss of precision due to totalising
        // a double in an int, but frankly that should be minimal
        // since most values of note are integer based anyway.
        double t = x.getForSymbol(totalMetric, context->athlete->useMetricUnits);

        // totalise in minutes
        if (tm->units(context->athlete->useMetricUnits) == tr("seconds")) t /= 60;

        // sum up
        data->metricArray[(int)(v)-min] += t;
    }

    // we certainly don't want the interval curve when plotting
    // metrics across rides!
    curveSelected->hide();

    // now set all the plot paramaters to match the data
    source = Metric;
    zoned = false;
    rideItem = NULL;
    lny = false;
    shade = false;
    withz = false;
    dt = 1;
    absolutetime = true;

    // and the plot itself
    QString yunits = tm->units(context->athlete->useMetricUnits);
    if (yunits == tr("seconds")) yunits = tr("minutes");
    QString xunits = m->units(context->athlete->useMetricUnits);
    if (xunits == tr("seconds")) xunits = tr("minutes");

    if (tm->units(context->athlete->useMetricUnits) != "")
        setAxisTitle(yLeft, QString(tr("Total %1 (%2)")).arg(tm->name()).arg(yunits));
    else
        setAxisTitle(yLeft, QString(tr("Total %1")).arg(tm->name()));
    
    if (m->units(context->athlete->useMetricUnits) != "")
        setAxisTitle(xBottom, QString(tr("%1 of Ride (%2)")).arg(m->name()).arg(xunits));
    else
        setAxisTitle(xBottom, QString(tr("%1 of Ride")).arg(m->name()));

    // dont show legend in metric mode
    //XXX legend()->hide();
    //XXX updateLegend();
}

void
PowerHist::updatePlot()
{
    replot();
    zoomer->setZoomBase();
}

void
PowerHist::setData(RideItem *_rideItem, bool force)
{
    // predefined deltas for each series
    static const double wattsDelta = 1.0;
    static const double wattsKgDelta = 0.01;
    static const double nmDelta    = 0.1;
    static const double hrDelta    = 1.0;
    static const double kphDelta   = 0.1;
    static const double cadDelta   = 1.0;

    source = Ride;

    // we set with this data already
    if (force == false && _rideItem == LASTrideItem && source == LASTsource) return;

    rideItem = _rideItem;
    if (!rideItem) return;

    RideFile *ride = rideItem->ride();

    bool hasData = ((series == RideFile::watts || series == RideFile::wattsKg) && ride->areDataPresent()->watts) ||
                   (series == RideFile::nm && ride->areDataPresent()->nm) ||
                   (series == RideFile::kph && ride->areDataPresent()->kph) ||
                   (series == RideFile::cad && ride->areDataPresent()->cad) ||
                   (series == RideFile::aPower && ride->areDataPresent()->apower) ||
                   (series == RideFile::hr && ride->areDataPresent()->hr);

    if (ride && hasData) {
        //setTitle(ride->startTime().toString(GC_DATETIME_FORMAT));

        static const int maxSize = 4096;

        // recording interval in minutes
        dt = ride->recIntSecs() / 60.0;

        standard.wattsArray.resize(0);
        standard.wattsZoneArray.resize(0);
        standard.wattsCPZoneArray.resize(0);
        standard.wattsKgArray.resize(0);
        standard.aPowerArray.resize(0);
        standard.nmArray.resize(0);
        standard.hrArray.resize(0);
        standard.hrZoneArray.resize(0);
        standard.kphArray.resize(0);
        standard.cadArray.resize(0);

        standard.wattsSelectedArray.resize(0);
        standard.wattsZoneSelectedArray.resize(0);
        standard.wattsKgSelectedArray.resize(0);
        standard.aPowerSelectedArray.resize(0);
        standard.nmSelectedArray.resize(0);
        standard.hrSelectedArray.resize(0);
        standard.hrZoneSelectedArray.resize(0);
        standard.kphSelectedArray.resize(0);
        standard.cadSelectedArray.resize(0);

        // unit conversion factor for imperial units for selected parameters
        double torque_factor = (context->athlete->useMetricUnits ? 1.0 : 0.73756215);
        double speed_factor  = (context->athlete->useMetricUnits ? 1.0 : 0.62137119);

        // cp and zones
        int CP = 0;
        const Zones *zones = rideItem->zones;
        int zoneRange = zones ? zones->whichRange(ride->startTime().date()) : -1;
        if (zoneRange != -1) CP=zones->getCP(zoneRange);
    
        foreach(const RideFilePoint *p1, ride->dataPoints()) {
            bool selected = isSelected(p1, ride->recIntSecs());

            // watts array
            int wattsIndex = int(floor(p1->watts / wattsDelta));
            if (wattsIndex >= 0 && wattsIndex < maxSize) {
                if (wattsIndex >= standard.wattsArray.size())
                    standard.wattsArray.resize(wattsIndex + 1);
                standard.wattsArray[wattsIndex]++;

                if (selected) {
                    if (wattsIndex >= standard.wattsSelectedArray.size())
                        standard.wattsSelectedArray.resize(wattsIndex + 1);
                    standard.wattsSelectedArray[wattsIndex]++;
                }
            }

            // watts zoned array

            // Only calculate zones if we have a valid range and check zeroes
            if (zoneRange > -1 && (withz || (!withz && p1->watts))) {

                // get the zone
                wattsIndex = zones->whichZone(zoneRange, p1->watts);

                // cp zoned
                if (standard.wattsCPZoneArray.size() < 4) standard.wattsCPZoneArray.resize(4);
                if (p1->watts < 1 && withz) // moderate zero watts
                    standard.wattsCPZoneArray[0] += ride->recIntSecs();
                else if (p1->watts >=1 && wattsIndex < 2) // moderate
                    standard.wattsCPZoneArray[0] += ride->recIntSecs();
                else if (p1->watts >=1 && p1->watts < CP) // heavy
                    standard.wattsCPZoneArray[1] += ride->recIntSecs();
                else if (p1->watts > CP) // severe
                    standard.wattsCPZoneArray[2] += ride->recIntSecs();


                // zoned
                if (wattsIndex >= 0 && wattsIndex < maxSize) {
                    if (wattsIndex >= standard.wattsZoneArray.size())
                        standard.wattsZoneArray.resize(wattsIndex + 1);
                    standard.wattsZoneArray[wattsIndex]++;

                    if (selected) {
                        if (wattsIndex >= standard.wattsZoneSelectedArray.size())
                            standard.wattsZoneSelectedArray.resize(wattsIndex + 1);
                        standard.wattsZoneSelectedArray[wattsIndex]++;
                    }
                }
            }

            // aPower array
            int aPowerIndex = int(floor(p1->apower / wattsDelta));
            if (aPowerIndex >= 0 && aPowerIndex < maxSize) {
                if (aPowerIndex >= standard.aPowerArray.size())
                    standard.aPowerArray.resize(aPowerIndex + 1);
                standard.aPowerArray[aPowerIndex]++;

                if (selected) {
                    if (aPowerIndex >= standard.aPowerSelectedArray.size())
                        standard.aPowerSelectedArray.resize(aPowerIndex + 1);
                    standard.aPowerSelectedArray[aPowerIndex]++;
                }
            }

            // wattsKg array
            int wattsKgIndex = int(floor(p1->watts / ride->getWeight() / wattsKgDelta));
            if (wattsKgIndex >= 0 && wattsKgIndex < maxSize) {
                if (wattsKgIndex >= standard.wattsKgArray.size())
                    standard.wattsKgArray.resize(wattsKgIndex + 1);
                standard.wattsKgArray[wattsKgIndex]++;

                if (selected) {
                    if (wattsKgIndex >= standard.wattsKgSelectedArray.size())
                        standard.wattsKgSelectedArray.resize(wattsKgIndex + 1);
                    standard.wattsKgSelectedArray[wattsKgIndex]++;
                }
            }

            int nmIndex = int(floor(p1->nm * torque_factor / nmDelta));
            if (nmIndex >= 0 && nmIndex < maxSize) {
                if (nmIndex >= standard.nmArray.size())
                    standard.nmArray.resize(nmIndex + 1);
                standard.nmArray[nmIndex]++;

                if (selected) {
                    if (nmIndex >= standard.nmSelectedArray.size())
                        standard.nmSelectedArray.resize(nmIndex + 1);
                    standard.nmSelectedArray[nmIndex]++;
                }
            }

            int hrIndex = int(floor(p1->hr / hrDelta));
            if (hrIndex >= 0 && hrIndex < maxSize) {
                if (hrIndex >= standard.hrArray.size())
                    standard.hrArray.resize(hrIndex + 1);
                standard.hrArray[hrIndex]++;

                if (selected) {
                    if (hrIndex >= standard.hrSelectedArray.size())
                        standard.hrSelectedArray.resize(hrIndex + 1);
                    standard.hrSelectedArray[hrIndex]++;
                }
            }

            // hr zoned array
            int hrZoneRange = context->athlete->hrZones() ? context->athlete->hrZones()->whichRange(ride->startTime().date()) : -1;

            // Only calculate zones if we have a valid range
            if (hrZoneRange > -1 && (withz || (!withz && p1->hr))) {
                hrIndex = context->athlete->hrZones()->whichZone(hrZoneRange, p1->hr);

                if (hrIndex >= 0 && hrIndex < maxSize) {
                    if (hrIndex >= standard.hrZoneArray.size())
                        standard.hrZoneArray.resize(hrIndex + 1);
                    standard.hrZoneArray[hrIndex]++;

                    if (selected) {
                        if (hrIndex >= standard.hrZoneSelectedArray.size())
                            standard.hrZoneSelectedArray.resize(hrIndex + 1);
                        standard.hrZoneSelectedArray[hrIndex]++;
                    }
                }
            }

            int kphIndex = int(floor(p1->kph * speed_factor / kphDelta));
            if (kphIndex >= 0 && kphIndex < maxSize) {
                if (kphIndex >= standard.kphArray.size())
                    standard.kphArray.resize(kphIndex + 1);
                standard.kphArray[kphIndex]++;

                if (selected) {
                    if (kphIndex >= standard.kphSelectedArray.size())
                        standard.kphSelectedArray.resize(kphIndex + 1);
                    standard.kphSelectedArray[kphIndex]++;
                }
            }

            int cadIndex = int(floor(p1->cad / cadDelta));
            if (cadIndex >= 0 && cadIndex < maxSize) {
                if (cadIndex >= standard.cadArray.size())
                    standard.cadArray.resize(cadIndex + 1);
                standard.cadArray[cadIndex]++;

                if (selected) {
                    if (cadIndex >= standard.cadSelectedArray.size())
                        standard.cadSelectedArray.resize(cadIndex + 1);
                    standard.cadSelectedArray[cadIndex]++;
                }
            }
        }

    } else {

        // create empty curves when no data
        const double zero = 0;
        curve->setSamples(&zero, &zero, 0);
        curveSelected->setSamples(&zero, &zero, 0);
        updatePlot();
    }
    curveSelected->show();
    zoomer->setZoomBase();

    // dont show legend in metric mode
    //XXX legend()->hide();
    //XXX updateLegend();
}

void
PowerHist::setBinWidth(double value)
{
    if (!value) value = 1; // binwidth must be nonzero
    binw = value;
}

void
PowerHist::setCPZoned(bool value)
{
    cpzoned = value;
    setComparePens();
}

void
PowerHist::setZoned(bool value)
{
    zoned = value;
    setComparePens();
}

void
PowerHist::setWithZeros(bool value)
{
    withz = value;
}

void
PowerHist::setlnY(bool value)
{
    // note: setAxisScaleEngine deletes the old ScaleEngine, so specifying
    // "new" in the argument list is not a leak

    lny=value;
    if (lny && !zoned) {

        setAxisScaleEngine(yLeft, new QwtLogScaleEngine);
        curve->setBaseline(1e-6);
        curveSelected->setBaseline(1e-6);

    } else {

        setAxisScaleEngine(yLeft, new QwtLinearScaleEngine);
        curve->setBaseline(0);
        curveSelected->setBaseline(0);

    }
    setYMax();
    updatePlot();
}

void
PowerHist::setSumY(bool value)
{
    absolutetime = value;
    setParameterAxisTitle();
}

void
PowerHist::setParameterAxisTitle()
{
    QString axislabel;
    switch (series) {

        case RideFile::watts:
            if (zoned) axislabel = tr("Power zone");
            else axislabel = tr("Power (watts)");
            break;

        case RideFile::wattsKg:
            if (zoned) axislabel = tr("Power zone");
            else axislabel = tr("Power (watts/kg)");
            break;

        case RideFile::hr:
            if (zoned) axislabel = tr("Heartrate zone");
            else axislabel = tr("Heartrate (bpm)");
            break;

        case RideFile::aPower:
            axislabel = tr("aPower (watts)");
            break;

        case RideFile::cad:
            axislabel = tr("Cadence (rpm)");
            break;

        case RideFile::kph:
            axislabel = QString(tr("Speed (%1)")).arg(context->athlete->useMetricUnits ? tr("kph") : tr("mph"));
            break;

        case RideFile::nm:
            axislabel = QString(tr("Torque (%1)")).arg(context->athlete->useMetricUnits ? tr("N-m") : tr("ft-lbf"));
            break;

        default:
            axislabel = QString(tr("Unknown data series"));
            break;
    }
    setAxisTitle(xBottom, axislabel);
    setAxisTitle(yLeft, absolutetime ? tr("Time (minutes)") : tr("Time (percent)"));
}

void
PowerHist::setAxisTitle(int axis, QString label)
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
PowerHist::setSeries(RideFile::SeriesType x)
{
    // user selected a different series to plot
    series = x;
    configChanged(); // set colors
    setParameterAxisTitle();
}

bool PowerHist::shadeZones() const
{
    return (rideItem && rideItem->ride() && (series == RideFile::aPower || series == RideFile::watts || series == RideFile::wattsKg) && !zoned && shade == true);
}

bool PowerHist::shadeHRZones() const
{
    return (rideItem && rideItem->ride() && series == RideFile::hr && !zoned && shade == true);
}

bool PowerHist::isSelected(const RideFilePoint *p, double sample) {
    if (context->athlete->allIntervalItems() != NULL) {
        for (int i=0; i<context->athlete->allIntervalItems()->childCount(); i++) {
            IntervalItem *current = dynamic_cast<IntervalItem*>(context->athlete->allIntervalItems()->child(i));
            if (current != NULL) {
                if (current->isSelected() && p->secs+sample>current->start && p->secs<current->stop) {
                    return true;
                }
            }
        }
    }
    return false;
}

void
PowerHist::pointHover(QwtPlotCurve *curve, int index)
{
    if (index >= 0) {

        double xvalue = curve->sample(index).x();
        double yvalue = curve->sample(index).y();
        QString text;

        if (zoned && yvalue > 0) {
            // output the tooltip
            text = QString("%1 %2").arg(yvalue, 0, 'f', 1).arg(absolutetime ? tr("minutes") : tr("%"));

            // set that text up
            zoomer->setText(text);
            return;

        } else if (yvalue > 0) {

            if (source != Metric) {
                // output the tooltip
                text = QString("%1 %2\n%3 %4")
                            .arg(xvalue, 0, 'f', digits)
                            .arg(this->axisTitle(curve->xAxis()).text())
                            .arg(yvalue, 0, 'f', 1)
                            .arg(absolutetime ? tr("minutes") : tr("%"));
            } else {
                text = QString("%1 %2\n%3 %4")
                            .arg(xvalue, 0, 'f', digits)
                            .arg(this->axisTitle(curve->xAxis()).text())
                            .arg(yvalue, 0, 'f', 1)
                            .arg(this->axisTitle(curve->yAxis()).text());
            }

            // set that text up
            zoomer->setText(text);
            return;
        }
    }
    // no point
    zoomer->setText("");
}

// because we need to effectively draw bars when showing
// time in zone (i.e. for every zone there are 2 points for each
// zone - top left and top right) we need to multiply the percentage
// values by 2 to take this into account
void
PowerHist::percentify(QVector<double> &array, double factor)
{
    double total=0;
    foreach (double current, array) total += current;

    if (total > 0)
        for (int i=0; i< array.size(); i++)
            if (array[i] > 0.01) // greater than 0.8s (i.e. not a double storage issue)
                array[i] = factor * (array[i] / total) * (double)100.00;
}

