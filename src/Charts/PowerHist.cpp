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
#include "RideCache.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "RideFile.h"
#include "WPrime.h"
#include "RideFileCache.h"
#include "Settings.h"
#include "Zones.h"
#include "HrZones.h"
#include "Colors.h"
#include "Units.h"

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
    LASTcache(nullptr),
    source(Ride),
    LASTsource(Ride)
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

    pacebg = new PaceHistBackground(this);
    pacebg->attach(this);

    setCanvasBackground(Qt::white);
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

    setParameterAxisTitle();
    setAxisTitle(QwtAxis::YLeft, absolutetime ? tr("Time (minutes)") : tr("Time (percent)"));

    curve = new QwtPlotCurve("");
    curve->setStyle(QwtPlotCurve::Steps);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    curve->setZ(20);
    curve->attach(this);

    curveSelected = new QwtPlotCurve("");
    curveSelected->setStyle(QwtPlotCurve::Steps);
    curveSelected->setRenderHint(QwtPlotItem::RenderAntialiased);
    curveSelected->setZ(50);
    curveSelected->attach(this);

    curveHover = new QwtPlotCurve("");
    curveHover->setStyle(QwtPlotCurve::Steps);
    curveHover->setRenderHint(QwtPlotItem::RenderAntialiased);
    curveHover->setZ(100);
    curveHover->attach(this);

    grid = new QwtPlotGrid();
    grid->enableX(false);
    grid->attach(this);

    zoneLabels = QList<PowerHistZoneLabel *>();
    hrzoneLabels = QList<HrHistZoneLabel *>();
    pacezoneLabels = QList<PaceHistZoneLabel *>();

    zoomer = new penTooltip(this->canvas());
    canvasPicker = new LTMCanvasPicker(this);
    connect(canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)), this, SLOT(pointHover(QwtPlotCurve*, int)));

    // usually hidden, but shown for compare mode
    //XXX insertLegend(new QwtLegend(), QwtPlot::BottomLegend);

    setAxisMaxMinor(QwtAxis::XBottom, 0);
    setAxisMaxMinor(QwtAxis::YLeft, 0);

    configChanged(CONFIG_APPEARANCE);
}

void
PowerHist::configChanged(qint32)
{
    // plot background
    if (rangemode) setCanvasBackground(GColor(CTRENDPLOTBACKGROUND));
    else setCanvasBackground(GColor(CPLOTBACKGROUND));

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
        case RideFile::smo2:
            pen.setColor(GColor(CSMO2).darker(200));
            brush_color = GColor(CSMO2);
            break;
        case RideFile::gear:
            pen.setColor(GColor(CGEAR).darker(200));
            brush_color = GColor(CGEAR);
            break;
        case RideFile::wbal:
            pen.setColor(GColor(CWBAL).darker(200));
            brush_color = GColor(CGEAR);
            break;
        default:
        case RideFile::hr:
            pen.setColor(GColor(CHEARTRATE).darker(200));
            brush_color = GColor(CHEARTRATE);
            break;
        }
    }

    double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();
    if (appsettings->value(this, GC_ANTIALIAS, true).toBool()==true) {
        curve->setRenderHint(QwtPlotItem::RenderAntialiased);
        curveSelected->setRenderHint(QwtPlotItem::RenderAntialiased);
        curveHover->setRenderHint(QwtPlotItem::RenderAntialiased);
    }

    curve->setBrush(brush_color);   // fill below the line

    if (!isZoningEnabled()) {
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
    if (rangemode) ivlbrush.setAlpha(GColor(CTRENDPLOTBACKGROUND) == QColor(Qt::white) ? 64 : 200);
    else ivlbrush.setAlpha(GColor(CPLOTBACKGROUND) == QColor(Qt::white) ? 64 : 200);
    curveSelected->setBrush(ivlbrush);   // fill below the line

    // hover curve
    QPen hvl(Qt::darkGray);
    hvl.setWidth(width);
    curveHover->setPen(hvl);
    QColor hvlbrush = QColor(Qt::darkGray);
    if (rangemode) hvlbrush.setAlpha((GColor(CTRENDPLOTBACKGROUND) == QColor(Qt::white) ? 64 : 200));
    else hvlbrush.setAlpha((GColor(CPLOTBACKGROUND) == QColor(Qt::white) ? 64 : 200));
    curveHover->setBrush(hvlbrush);   // fill below the line

    // grid
    QPen gridPen(GColor(CPLOTGRID));
    //gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);

    QPalette palette;
    if (rangemode) palette.setBrush(QPalette::Window, QBrush(GColor(CTRENDPLOTBACKGROUND)));
    else palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    setPalette(palette);

    axisWidget(QwtAxis::XBottom)->setPalette(palette);
    axisWidget(QwtAxis::YLeft)->setPalette(palette);

    setAutoFillBackground(true);
}

void
PowerHist::hideStandard(bool hide)
{
    bg->setVisible(!hide);
    hrbg->setVisible(!hide);
    pacebg->setVisible(!hide);
    curve->setVisible(!hide);
    curveSelected->setVisible(!hide);
    curveHover->setVisible(!hide);

    // clear if we are showing standard (not comparing) or if the comparisons got cleared
    if (!hide || (rangemode && context->compareDateRanges.count() == 0) || (!rangemode && context->compareIntervals.count() == 0) ) {

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
    delete pacebg;
    delete curve;
    delete curveSelected;
    delete curveHover;
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

        const Zones *zones = context->athlete->zones(rideItem->sport);
        int zone_range = zones->whichRange(rideItem->dateTime.date());

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
        const HrZones *zones = context->athlete->hrZones(rideItem->sport);
        int zone_range = zones->whichRange(rideItem->dateTime.date());

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
PowerHist::refreshPaceZoneLabels()
{
    // delete any existing power zone labels
    if (pacezoneLabels.size()) {
        QListIterator<PaceHistZoneLabel *> i(pacezoneLabels);
        while (i.hasNext()) {
            PaceHistZoneLabel *label = i.next();
            label->detach();
            delete label;
        }
    }
    pacezoneLabels.clear();

    if (!rideItem || !(rideItem->isRun || rideItem->isSwim)) return;

    if (series == RideFile::kph && context->athlete->paceZones(rideItem->isSwim)) {
        const PaceZones *zones = context->athlete->paceZones(rideItem->isSwim);
        int zone_range = zones->whichRange(rideItem->dateTime.date());

        // generate labels for existing zones
        if (zone_range >= 0) {
            int num_zones = zones->numZones(zone_range);
            for (int z = 0; z < num_zones; z ++) {
                PaceHistZoneLabel *label = new PaceHistZoneLabel(this, z);
                label->attach(this);
                pacezoneLabels.append(label);
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

    // zap any zone data labels
    foreach (QwtPlotMarker *label, zoneDataLabels) {
        label->detach();
        delete label;
    }
    zoneDataLabels.clear();

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

            if (cpzoned) {

                array = &cid.wattsCPZoneArray;
                arrayLength = cid.wattsCPZoneArray.size();

            } else {

                array = &cid.wattsZoneArray;
                arrayLength = cid.wattsZoneArray.size();
            }

        } else if (series == RideFile::wbal && zoned == true) {

            array = &cid.wbalZoneArray;
            arrayLength = cid.wbalZoneArray.size();

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

            if (cpzoned) {

                array = &cid.hrCPZoneArray;
                arrayLength = cid.hrCPZoneArray.size();

            } else {

                array = &cid.hrZoneArray;
                arrayLength = cid.hrZoneArray.size();

            }

        } else if (series == RideFile::kph && !(zoned == true && (!rideItem || rideItem->isRun || rideItem->isSwim))) {

            array = &cid.kphArray;
            arrayLength = cid.kphArray.size();

        } else if (series == RideFile::kph && zoned == true && (!rideItem || rideItem->isRun || rideItem->isSwim)) {

            if (cpzoned) {

                array = &cid.paceCPZoneArray;
                arrayLength = cid.paceCPZoneArray.size();

            } else {

                array = &cid.paceZoneArray;
                arrayLength = cid.paceZoneArray.size();

            }

        } else if (series == RideFile::smo2) {

            array = &cid.smo2Array;
            arrayLength = cid.smo2Array.size();

        } else if (series == RideFile::gear) {

            array = &cid.gearArray;
            arrayLength = cid.gearArray.size();


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

        if (!isZoningEnabled()) {

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
                totalTime[i]  = 1e-9;  // nonzero to accommodate log plot
                while (low < high && low<arrayLength) {
                    totalTime[i] += dt * (*array)[low++];
                }
            }

            totalTime[i] = 1e-9;       // nonzero to accommodate log plot
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
            setAxisScaleDraw(QwtAxis::XBottom, sd);

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
            if (series == RideFile::wbal) {
                if (cid.wbalArray.size() > maxX) maxX = cid.wbalArray.size();
            } else {
                int truncate = count;
                while (truncate > 0) {
                    if (!absolutetime && totalTime[truncate] >= 0.1) break;
                    if (absolutetime && totalTime[truncate] >= 0.1) break;
                    truncate--;
                }
                if (parameterValue[truncate] > maxX) maxX = parameterValue[truncate];
            }

            // we only do zone labels when using absolute values
            refreshZoneLabels();
            refreshHRZoneLabels();
            refreshPaceZoneLabels();

        } else { // ZONED

            QFont labelFont;
            labelFont.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
            labelFont.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());

            // 0.625 = golden ratio for gaps between group of cols
            // 0.9 = 10% space between each col in group

            double width = (0.625 / ncols) * 0.90f;
            double jump = acol * (0.625 / ncols);
            
            // we're not binning instead we are prettyfying the columnar
            // display in much the same way as the weekly summary workds
            // Each zone column will have 4 points
            QVector<double> xaxis (array->size() * 4);
            QVector<double> yaxis (array->size() * 4);

            // so we can calculate percentage for the labels
            double total=0;
            for (int i=0; i<array->size(); i++) total += dt * (double)(*array)[i];

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

                double yval = absolutetime ? y : (y /total * 100.00f);

                if (yval > 0) {

                    QColor color = rangemode ? context->compareDateRanges[intervalNumber].color.darker(200)
                                             : context->compareIntervals[intervalNumber].color.darker(200);

                    // is this one selected ?
                    if ((rangemode && context->compareDateRanges[intervalNumber].isChecked()) ||
                        (!rangemode && context->compareIntervals[intervalNumber].isChecked())) {

                        // now add a label above the bar
                        QwtPlotMarker *label = new QwtPlotMarker();
                        QwtText text(QString("%1%2").arg(round(yval)).arg(absolutetime ? "" : "%"), QwtText::PlainText);
                        text.setFont(labelFont);
                        text.setColor(color);
                        label->setLabel(text);
                        label->setValue(x+jump+(width/2.00f), yval);
                        label->setYAxis(QwtAxis::YLeft);
                        label->setSpacing(5 *dpiXFactor); // not px but by yaxis value !? mad.
                        label->setLabelAlignment(Qt::AlignTop | Qt::AlignCenter);
            
                        // and attach
                        label->attach(this);
                        zoneDataLabels << label;
                    }
                }
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
            if (series == RideFile::wattsKg || series == RideFile::watts) {

                const Zones *zones = context->athlete->zones("Bike");
                int zone_range = -1;

                if (zones) {
                    if (context->compareIntervals.count())
                        zone_range = zones->whichRange(context->compareIntervals[0].data->startTime().date());
                    if (zone_range == -1) zone_range = zones->whichRange(QDate::currentDate());
                }

                if (zones && zone_range != -1) {
                    if (cpzoned) {
                        setAxisScaleDraw(QwtAxis::XBottom, new PolarisedZoneScaleDraw(zones, zone_range, zoneLimited));
                        setAxisScale(QwtAxis::XBottom, -0.99, 3, 1);
                    } else {
                        setAxisScaleDraw(QwtAxis::XBottom, new ZoneScaleDraw(zones, zone_range, zoneLimited));
                        setAxisScale(QwtAxis::XBottom, -0.99, zones->numZones(zone_range), 1);
                    }
                }
            }

            //
            // HR ZONES
            //
            if (!cpzoned) {

                const HrZones *hrzones = context->athlete->hrZones("Bike");
                int hrzone_range = -1;

                if (hrzones) {
                    if (context->compareIntervals.count())
                        hrzone_range = hrzones->whichRange(context->compareIntervals[0].data->startTime().date());
                    if (hrzone_range == -1) hrzone_range = hrzones->whichRange(QDate::currentDate());

                }
                if (hrzones && hrzone_range != -1) {
                    if (series == RideFile::hr) {
                        setAxisScaleDraw(QwtAxis::XBottom, new HrZoneScaleDraw(hrzones, hrzone_range, zoneLimited));
                        setAxisScale(QwtAxis::XBottom, -0.99, hrzones->numZones(hrzone_range), 1);
                    }
                }

            }

            //
            // PACE ZONES using Run Zones for scale
            //
            if (!cpzoned) {

                const PaceZones *pacezones = context->athlete->paceZones(false);
                int pacezone_range = -1;

                if (pacezones) {
                    if (context->compareIntervals.count())
                        pacezone_range = pacezones->whichRange(context->compareIntervals[0].data->startTime().date());
                    if (pacezone_range == -1) pacezone_range = pacezones->whichRange(QDate::currentDate());

                }
                if (pacezones && pacezone_range != -1) {
                    if (series == RideFile::kph) {
                        setAxisScaleDraw(QwtAxis::XBottom, new PaceZoneScaleDraw(pacezones, pacezone_range, zoneLimited));
                        setAxisScale(QwtAxis::XBottom, -0.99, pacezones->numZones(pacezone_range), 1);
                    }
                }

            }

            //
            // W'bal zones
            //
            if (!cpzoned && series == RideFile::wbal) {

                const Zones *zones = context->athlete->zones("Bike");
                int zone_range = -1;

                if (zones) {
                    if (context->compareIntervals.count())
                        zone_range = zones->whichRange(context->compareIntervals[0].data->startTime().date());
                    if (zone_range == -1) zone_range = zones->whichRange(QDate::currentDate());

                }
                if (zones && zone_range != -1) {
                    setAxisScaleDraw(QwtAxis::XBottom, new WbalZoneScaleDraw(zones, zone_range, zoneLimited));
                    setAxisScale(QwtAxis::XBottom, -0.99, WPrime::zoneCount(), 1);
                }

            }

            setAxisMaxMinor(QwtAxis::XBottom, 0);

            // keep track of columns visible -- depending upon mode
            if (!rangemode && context->compareIntervals[intervalNumber].isChecked()) acol++;
            if (rangemode && context->compareDateRanges[intervalNumber].isChecked()) acol++;
        }
    }

    // set axis etc
    if (!isZoningEnabled()) {
        //normal
        setAxisScale(QwtAxis::XBottom, minX, maxX);
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

    // lets make sure we need to recalculate
    if (force == false &&
        LASTsource == source &&
        LASTcache == cache &&
        LASTrideItem == rideItem &&
        LASTseries == series &&
        LASTshade == shade &&
        LASTuseMetricUnits == GlobalContext::context()->useMetricUnits &&
        LASTlny == lny &&
        LASTzoned == zoned &&
        LASTcpzoned == cpzoned &&
        LASTzoneLimited == zoneLimited &&
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
        LASTuseMetricUnits = GlobalContext::context()->useMetricUnits;
        LASTlny = lny;
        LASTzoned = zoned;
        LASTcpzoned = cpzoned;
        LASTzoneLimited = zoneLimited;
        LASTbinw = binw;
        LASTwithz = withz;
        LASTdt = dt;
        LASTabsolutetime = absolutetime;
    }

    setParameterAxisTitle();

    if (source == Ride && !rideItem) { 
        return;
    }

    // make sure the interval length is set if not plotting metrics
    if (source != Metric && dt <= 0) {
        return;
    }

    // zap any zone data labels
    foreach (QwtPlotMarker *label, zoneDataLabels) {
        label->detach();
        delete label;
    }
    zoneDataLabels.clear();

    // bin the data
    QVector<double>x,y,sx,sy;
    binData(standard, x, y, sx, sy);

    if (!isZoningEnabled()) {

        // now draw curves / axis etc
        curve->setSamples(x, y);
        curveSelected->setSamples(sx, sy);

        QwtScaleDraw *sd = new QwtScaleDraw;
        sd->setTickLength(QwtScaleDiv::MajorTick, 3);
        setAxisScaleDraw(QwtAxis::XBottom, sd);

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
        setAxisScale(QwtAxis::XBottom, minX, x[x.size()-1]);

        // we only do zone labels when using absolute values
        refreshZoneLabels();
        refreshHRZoneLabels();
        refreshPaceZoneLabels();

    } else {

        QFont labelFont;
        labelFont.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
        labelFont.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 10).toInt());

        // zoned labels
        for(int i=1; i<x.count(); i += 4) {

            double yval = y[i];
            double xval = x[i];

            if (yval > 0) {

                // now add a label above the bar
                QwtPlotMarker *label = new QwtPlotMarker();
                QwtText text(QString("%1%2").arg(round(yval)).arg(absolutetime ? "" : "%"), QwtText::PlainText);
                text.setFont(labelFont);
                if (series == RideFile::watts)
                    text.setColor(GColor(CPOWER).darker(200));
                else if (series == RideFile::hr)
                    text.setColor(GColor(CHEARTRATE).darker(200));
                else
                    text.setColor(GColor(CSPEED).darker(200));
                label->setLabel(text);
                label->setValue(xval+0.312f, yval);
                label->setYAxis(QwtAxis::YLeft);
                label->setSpacing(5 *dpiXFactor); // not px but by yaxis value !? mad.
                label->setLabelAlignment(Qt::AlignTop | Qt::AlignCenter);
            
                // and attach
                label->attach(this);
                zoneDataLabels << label;
            }
        }

        // set those curves
        curve->setSamples(x, y);
        curveSelected->setSamples(sx, sy);

        // zone scale draw
        if ((series == RideFile::watts || series == RideFile::wattsKg) && zoned && rideItem && context->athlete->zones(rideItem->sport)) {

            int zone_range = context->athlete->zones(rideItem->sport)->whichRange(rideItem->dateTime.date());
            if (cpzoned) {
                setAxisScaleDraw(QwtAxis::XBottom, new PolarisedZoneScaleDraw(context->athlete->zones(rideItem->sport), zone_range, zoneLimited));
                setAxisScale(QwtAxis::XBottom, -0.99, 3, 1);
            } else {
                setAxisScaleDraw(QwtAxis::XBottom, new ZoneScaleDraw(context->athlete->zones(rideItem->sport), zone_range, zoneLimited));
                if (zone_range >= 0)
                    setAxisScale(QwtAxis::XBottom, -0.99, context->athlete->zones(rideItem->sport)->numZones(zone_range), 1);
                else
                    setAxisScale(QwtAxis::XBottom, -0.99, 0, 1);
            }
        }

        // hr scale draw
        int hrRange;
        if (series == RideFile::hr && zoned && rideItem && context->athlete->hrZones(rideItem->sport) &&
            (hrRange=context->athlete->hrZones(rideItem->sport)->whichRange(rideItem->dateTime.date())) != -1) {

            if (cpzoned) {
                setAxisScaleDraw(QwtAxis::XBottom, new HrPolarisedZoneScaleDraw(context->athlete->hrZones(rideItem->sport), hrRange, zoneLimited));
                setAxisScale(QwtAxis::XBottom, -0.99, 3, 1);
            } else {
                setAxisScaleDraw(QwtAxis::XBottom, new HrZoneScaleDraw(context->athlete->hrZones(rideItem->sport), hrRange, zoneLimited));
                if (hrRange >= 0)
                    setAxisScale(QwtAxis::XBottom, -0.99, context->athlete->hrZones(rideItem->sport)->numZones(hrRange), 1);
                else
                    setAxisScale(QwtAxis::XBottom, -0.99, 0, 1);
            }
        }

        // pace scale draw
        int paceRange;
        if (series == RideFile::kph && zoned && rideItem &&
            (rideItem->isRun || rideItem->isSwim) && context->athlete->paceZones(rideItem->isSwim) &&
            (paceRange=context->athlete->paceZones(rideItem->isSwim)->whichRange(rideItem->dateTime.date())) != -1) {

            if (cpzoned) {
                setAxisScaleDraw(QwtAxis::XBottom, new PacePolarisedZoneScaleDraw(context->athlete->paceZones(rideItem->isSwim), paceRange, zoneLimited));
                setAxisScale(QwtAxis::XBottom, -0.99, 3, 1);
            } else {
                setAxisScaleDraw(QwtAxis::XBottom, new PaceZoneScaleDraw(context->athlete->paceZones(rideItem->isSwim), paceRange, zoneLimited));

                if (paceRange >= 0)
                    setAxisScale(QwtAxis::XBottom, -0.99, context->athlete->paceZones(rideItem->isSwim)->numZones(paceRange), 1);
                else
                    setAxisScale(QwtAxis::XBottom, -0.99, 0, 1);
            }
        }

        // watts zoned for a time range
        if (source == Cache && zoned && (series == RideFile::watts || series == RideFile::wattsKg) && context->athlete->zones("Bike")) {
            if (cpzoned) {
                setAxisScaleDraw(QwtAxis::XBottom, new PolarisedZoneScaleDraw(context->athlete->zones("Bike"), 0, zoneLimited));
                setAxisScale(QwtAxis::XBottom, -0.99, 3, 1);
            } else {
                setAxisScaleDraw(QwtAxis::XBottom, new ZoneScaleDraw(context->athlete->zones("Bike"), 0, zoneLimited));
                if (context->athlete->zones("Bike")->getRangeSize())
                    setAxisScale(QwtAxis::XBottom, -0.99, context->athlete->zones("Bike")->numZones(0), 1); // use zones from first defined range
            }
        }

        // hr zoned for a time range
        if (source == Cache && zoned && series == RideFile::hr && context->athlete->hrZones("Bike")) {
            if (cpzoned) {
                setAxisScaleDraw(QwtAxis::XBottom, new HrPolarisedZoneScaleDraw(context->athlete->hrZones("Bike"), 0, zoneLimited));
                setAxisScale(QwtAxis::XBottom, -0.99, 3, 1);
            } else {
                setAxisScaleDraw(QwtAxis::XBottom, new HrZoneScaleDraw(context->athlete->hrZones("Bike"), 0, zoneLimited));
                if (context->athlete->hrZones("Bike")->getRangeSize())
                    setAxisScale(QwtAxis::XBottom, -0.99, context->athlete->hrZones("Bike")->numZones(0), 1); // use zones from first defined range
            }
        }

        // pace zoned for a time range using run zones for scale
        if (source == Cache && zoned && series == RideFile::kph && context->athlete->paceZones(false)) {
            if (cpzoned) {
                setAxisScaleDraw(QwtAxis::XBottom, new PacePolarisedZoneScaleDraw(context->athlete->paceZones(false), 0, zoneLimited));
                setAxisScale(QwtAxis::XBottom, -0.99, 3, 1);
            } else {
                setAxisScaleDraw(QwtAxis::XBottom, new PaceZoneScaleDraw(context->athlete->paceZones(false), 0, zoneLimited));
                if (context->athlete->paceZones(false)->getRangeSize())
                    setAxisScale(QwtAxis::XBottom, -0.99, context->athlete->paceZones(false)->numZones(0), 1); // use zones from first defined range
            }
        }

        // w'bal zoned
        if (zoned && series == RideFile::wbal && context->athlete->zones("Bike")) {
            setAxisScaleDraw(QwtAxis::XBottom, new WbalZoneScaleDraw(context->athlete->zones("Bike"), 0, zoneLimited));
            setAxisScale(QwtAxis::XBottom, -0.99, WPrime::zoneCount(), 1);
        }
        setAxisMaxMinor(QwtAxis::XBottom, 0);
    }

    setYMax();
    configChanged(CONFIG_APPEARANCE); // setup the curve colors to appropriate values
    updatePlot();
}

// bin the data into x/y for ride and selected intervals
void
PowerHist::binData(HistData &standard, QVector<double>&x, // x-axis for data
                                       QVector<double>&y, // y-axis for data
                                       QVector<double>&sx, // x-axis for selected data
                                       QVector<double>&sy) // y-axis for selected data
{
    QVector<unsigned int> *array = NULL;
    QVector<unsigned int> *selectedArray = NULL;
    int arrayLength = 0;

    if (source == Metric) {

        // we use the metricArray
        array = &standard.metricArray;
        arrayLength = standard.metricArray.size();
        selectedArray = NULL;

    } else if (series == RideFile::watts && zoned == false) {

        array = &standard.wattsArray;
        arrayLength = standard.wattsArray.size();
        selectedArray = &standard.wattsSelectedArray;

    } else if (series == RideFile::wbal && zoned == false) {

        array = &standard.wbalArray;
        arrayLength = standard.wbalArray.size();
        selectedArray = &standard.wbalSelectedArray;

    } else if (series == RideFile::wbal && zoned == true) {

            array = &standard.wbalZoneArray;
            arrayLength = standard.wbalZoneArray.size();
            selectedArray = &standard.wbalZoneSelectedArray;

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

        if (cpzoned) {
            array = &standard.hrCPZoneArray;
            arrayLength = standard.hrCPZoneArray.size();
            selectedArray = &standard.hrCPZoneSelectedArray;
        } else {
            array = &standard.hrZoneArray;
            arrayLength = standard.hrZoneArray.size();
            selectedArray = &standard.hrZoneSelectedArray;
        }

    } else if (series == RideFile::kph && !(zoned == true && (!rideItem || rideItem->isRun || rideItem->isSwim))) {

        array = &standard.kphArray;
        arrayLength = standard.kphArray.size();
        selectedArray = &standard.kphSelectedArray;

    } else if (series == RideFile::kph && zoned == true && (!rideItem || rideItem->isRun || rideItem->isSwim)) {

        if (cpzoned) {
            array = &standard.paceCPZoneArray;
            arrayLength = standard.paceCPZoneArray.size();
            selectedArray = &standard.paceCPZoneSelectedArray;
        } else {
            array = &standard.paceZoneArray;
            arrayLength = standard.paceZoneArray.size();
            selectedArray = &standard.paceZoneSelectedArray;
        }

    } else if (series == RideFile::smo2) {
        array = &standard.smo2Array;
        arrayLength = standard.smo2Array.size();
        selectedArray = &standard.smo2SelectedArray;

    } else if (series == RideFile::gear) {
        array = &standard.gearArray;
        arrayLength = standard.gearArray.size();
        selectedArray = &standard.gearSelectedArray;

    } else if (series == RideFile::cad) {
        array = &standard.cadArray;
        arrayLength = standard.cadArray.size();
        selectedArray = &standard.cadSelectedArray;
    }

    // binning of data when not zoned
    if (!isZoningEnabled()) {

        // we add a bin on the end since the last "incomplete" bin
        // will be dropped otherwise
        int count = qMax(0, int(ceil((arrayLength - 1) / (binw)))+1);

        // allocate space for data, plus beginning and ending point
        x.resize(count+2);
        y.resize(count+2);
        sy.resize(count+2);
        x.fill(0.0);
        y.fill(0.0);
        sx.fill(0.0);

        int i;
        for (i = 1; i <= count; ++i) {
            double high = i * round(binw/delta);
            double low = high - round(binw/delta);
            if (low==0 && !withz) low++;
            x[i] = high*delta;
            y[i]  = 1e-9;  // nonzero to accommodate log plot
            sy[i] = 1e-9;  // nonzero to accommodate log plot
            if (array) {
                while (low < high && low<arrayLength) {
                    if (selectedArray && (*selectedArray).size()>low)
                        sy[i] += dt * (*selectedArray)[low];
                    y[i] += dt * (*array)[low++];
                }
            }
        }
        y[i] = 1e-9;       // nonzero to accommodate log plot
        sy[i] = 1e-9;       // nonzero to accommodate log plot
        x[i] = i * delta * binw;
        y[0] = 1e-9;
        sy[0] = 1e-9;
        x[0] = 0;

        // convert vectors from absolute time to percentage
        // if the user has selected that
        if (!absolutetime) {
            percentify(y, 1);
            percentify(sy, 1);
        }

        // the selected parameter values are the same as the 
        // actual values
        sx = x;

    } else {

        // we're not binning instead we are prettyfing the columnar
        // display in much the same way as the weekly summary workds
        // Each zone column will have 4 points
        x.resize(array->size() * 4);
        y.resize(array->size() * 4);
        sx.resize(selectedArray->size() * 4);
        sy.resize(selectedArray->size() * 4);

        // so we can calculate percentage for the labels
        double total=0;
        for (int i=0; i<array->size(); i++) total += dt * (double)(*array)[i];

        // samples to time
        for (int i=0, offset=0; i<array->size(); i++) {

            double xn = (double) i - (0.625f / 2.0f);
            double yn = dt * (double)(*array)[i];

            x[offset] = xn;
            y[offset] = 0;
            offset++;
            x[offset] = xn;
            y[offset] = yn;
            offset++;
            x[offset] = xn+0.625;
            y[offset] = yn;
            offset++;
            x[offset] = xn +0.625;
            y[offset] = 0;
            offset++;
        }

        for (int i=0, offset=0; i<selectedArray->size(); i++) {
            double xn = (double)i - (0.625f / 2.0f);
            double yn = dt * (double)(*selectedArray)[i];

            sx[offset] = xn;
            sy[offset] = 0;
            offset++;
            sx[offset] = xn;
            sy[offset] = yn;
            offset++;
            sx[offset] = xn+0.625;
            sy[offset] = yn;
            offset++;
            sx[offset] = xn +0.625;
            sy[offset] = 0;
            offset++;

        }

        if (!absolutetime) {
            percentify(y, 2);
            percentify(sy, 2);
        }
    }
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
    setAxisScale(QwtAxis::YLeft, (lny ? tmin : 0.0), MaxY * 1.1);

    QwtScaleDraw *sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    setAxisScaleDraw(QwtAxis::YLeft, sd);
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
    standard.wattsCPZoneArray.resize(3);
    standard.hrZoneArray.resize(10);
    standard.hrCPZoneArray.resize(3);
    standard.wattsKgArray.resize(0);
    standard.aPowerArray.resize(0);
    standard.nmArray.resize(0);
    standard.hrArray.resize(0);
    standard.kphArray.resize(0);
    standard.paceZoneArray.resize(10);
    standard.paceCPZoneArray.resize(3);
    standard.gearArray.resize(0);
    standard.smo2Array.resize(0);
    standard.cadArray.resize(0);
    standard.wbalArray.resize(0);
    standard.wbalZoneArray.resize(4);

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
    standard.paceZoneSelectedArray.resize(0);
    standard.gearSelectedArray.resize(0);
    standard.smo2SelectedArray.resize(0);
    standard.cadSelectedArray.resize(0);
    standard.wbalSelectedArray.resize(0);
    standard.wbalZoneSelectedArray.resize(0);

    longFromDouble(standard.wattsArray, cache->distributionArray(RideFile::watts));
    longFromDouble(standard.wattsKgArray, cache->distributionArray(RideFile::wattsKg));
    longFromDouble(standard.aPowerArray, cache->distributionArray(RideFile::aPower));
    longFromDouble(standard.hrArray, cache->distributionArray(RideFile::hr));
    longFromDouble(standard.nmArray, cache->distributionArray(RideFile::nm));
    longFromDouble(standard.cadArray, cache->distributionArray(RideFile::cad));
    longFromDouble(standard.gearArray, cache->distributionArray(RideFile::gear));
    longFromDouble(standard.smo2Array, cache->distributionArray(RideFile::smo2));
    longFromDouble(standard.kphArray, cache->distributionArray(RideFile::kph));
    longFromDouble(standard.wbalArray, cache->distributionArray(RideFile::wbal));

    if (!GlobalContext::context()->useMetricUnits) {
        for(int i=1; i<standard.nmArray.size(); i++) {
            int nmIndex = round(i / FOOT_POUNDS_PER_METER);
            if (nmIndex < standard.nmArray.size())
                standard.nmArray[i] = standard.nmArray[nmIndex];
            else
                standard.nmArray[i] = 0;
        }
        for(int i=1; i<standard.kphArray.size(); i++) {
            int kphIndex = round(i / MILES_PER_KM);
            if (kphIndex < standard.kphArray.size())
                standard.kphArray[i] = standard.kphArray[kphIndex];
            else
                standard.kphArray[i] = 0;
        }
    }

    // zone array
    for (int i=0; i<10; i++) {
        standard.wattsZoneArray[i] = cache->wattsZoneArray()[i];
        standard.hrZoneArray[i] = cache->hrZoneArray()[i];
        standard.paceZoneArray[i] = cache->paceZoneArray()[i];
    }

    // polarised zones
    standard.wattsCPZoneArray[0] = cache->wattsCPZoneArray()[1];
    standard.hrCPZoneArray[0] = cache->hrCPZoneArray()[1];
    standard.paceCPZoneArray[0] = cache->paceCPZoneArray()[1];
    if (withz) {
        standard.wattsCPZoneArray[0] += cache->wattsCPZoneArray()[0]; // add in zero watts
        standard.hrCPZoneArray[0] += cache->hrCPZoneArray()[0]; // add in zero bpm
        standard.paceCPZoneArray[0] += cache->paceCPZoneArray()[0]; // add in zero kph
    }
    standard.wattsCPZoneArray[1] = cache->wattsCPZoneArray()[2];
    standard.hrCPZoneArray[1] = cache->hrCPZoneArray()[2];
    standard.paceCPZoneArray[1] = cache->paceCPZoneArray()[2];
    standard.wattsCPZoneArray[2] = cache->wattsCPZoneArray()[3];
    standard.hrCPZoneArray[2] = cache->hrCPZoneArray()[3];
    standard.paceCPZoneArray[2] = cache->paceCPZoneArray()[3];

    // w'bal zones
    for(int i=0; i<4; i++) standard.wbalZoneArray[i] = cache->wbalZoneArray()[i];

    curveSelected->hide();
    curveHover->hide();
}

void
PowerHist::intervalHover(IntervalItem *x)
{
    // telling me to hide
    if (x == NULL) {
        curveHover->hide();
        return;
    }

    if (!isVisible()) return; // noone can see us

    if ((rangemode && context->isCompareDateRanges)
        || (!rangemode && context->isCompareIntervals)) return; // not in compare mode

    if (rideItem && rideItem->ride()) {

        // set data
        HistData hoverData;
        setArraysFromRide(rideItem->ride(), hoverData, context->athlete->zones(rideItem->sport), x);

        // set curve
        QVector<double>x,y,sx,sy;
        binData(hoverData, x,y,sx,sy);

        curveHover->setSamples(sx,sy);
        curveHover->show();

        replot();
    }
}

void
PowerHist::setDataFromCompare()
{
    // not for metric plots sonny
    if (source == Metric) return;

    double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();

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
        add.wattsCPZoneArray.resize(3);
        add.wattsKgArray.resize(0);
        add.aPowerArray.resize(0);
        add.nmArray.resize(0);
        add.hrArray.resize(0);
        add.hrZoneArray.resize(10);
        add.hrCPZoneArray.resize(3);
        add.kphArray.resize(0);
        add.paceZoneArray.resize(10);
        add.paceCPZoneArray.resize(3);
        add.gearArray.resize(0);
        add.smo2Array.resize(0);
        add.cadArray.resize(0);
        add.wbalArray.resize(0);
        add.wbalZoneArray.resize(4);

        longFromDouble(add.wattsArray, s->distributionArray(RideFile::watts));
        longFromDouble(add.wattsKgArray, s->distributionArray(RideFile::wattsKg));
        longFromDouble(add.aPowerArray, s->distributionArray(RideFile::aPower));
        longFromDouble(add.hrArray, s->distributionArray(RideFile::hr));
        longFromDouble(add.nmArray, s->distributionArray(RideFile::nm));
        longFromDouble(add.gearArray, s->distributionArray(RideFile::gear));
        longFromDouble(add.smo2Array, s->distributionArray(RideFile::smo2));
        longFromDouble(add.cadArray, s->distributionArray(RideFile::cad));
        longFromDouble(add.kphArray, s->distributionArray(RideFile::kph));
        longFromDouble(add.wbalArray, s->distributionArray(RideFile::wbal));

        // always 0-100 percent minimum
        if (add.wbalArray.size() < 101) add.wbalArray.resize(101);

        // convert for metric imperial types
        if (!GlobalContext::context()->useMetricUnits) {
            for(int i=1; i<add.nmArray.size(); i++) {
                int nmIndex = round(i / FOOT_POUNDS_PER_METER);
                if (nmIndex < add.nmArray.size())
                    add.nmArray[i] = add.nmArray[nmIndex];
                else
                    add.nmArray[i] = 0;
            }
            for(int i=1; i<add.kphArray.size(); i++) {
                int kphIndex = round(i / MILES_PER_KM);
                if (kphIndex < add.kphArray.size())
                    add.kphArray[i] = add.kphArray[kphIndex];
                else
                    add.kphArray[i] = 0;
            }
        }

        // zone array
        for (int i=0; i<10; i++) {
            add.wattsZoneArray[i] = s->wattsZoneArray()[i];
            add.hrZoneArray[i] = s->hrZoneArray()[i];
            add.paceZoneArray[i] = s->paceZoneArray()[i];
        }
        // polarised zones
        add.wattsCPZoneArray[0] = s->wattsCPZoneArray()[1];
        add.hrCPZoneArray[0] = s->hrCPZoneArray()[1];
        add.paceCPZoneArray[0] = s->paceCPZoneArray()[1];
        if (withz) {
            add.wattsCPZoneArray[0] += s->wattsCPZoneArray()[0]; // add in zero watts
            add.hrCPZoneArray[0] += s->hrCPZoneArray()[0]; // add in zero bpm
            add.paceCPZoneArray[0] += s->paceCPZoneArray()[0]; // add in zero kph
        }
        add.wattsCPZoneArray[1] = s->wattsCPZoneArray()[2];
        add.hrCPZoneArray[1] = s->hrCPZoneArray()[2];
        add.paceCPZoneArray[1] = s->paceCPZoneArray()[2];
        add.wattsCPZoneArray[2] = s->wattsCPZoneArray()[3];
        add.hrCPZoneArray[2] = s->hrCPZoneArray()[3];
        add.paceCPZoneArray[2] = s->paceCPZoneArray()[3];

        // w'bal zones
        for(int i=0; i<4; i++) add.wbalZoneArray[i] = s->wbalZoneArray()[i];

        // add to the list
        compareData << add;

        // now add a curve for recalc to play with
        QwtPlotCurve *newCurve = new QwtPlotCurve("");
        newCurve->setStyle(QwtPlotCurve::Steps);

        if (appsettings->value(this, GC_ANTIALIAS, true).toBool()==true)
            newCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

        // curve has no brush .. too confusing...
        QPen pen;
        pen.setColor(color);
        pen.setWidth(width);
        newCurve->setPen(pen);
        newCurve->setBrush(color);   // fill below the line

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

    double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();
    for (int i=0; (!rangemode && i<context->compareIntervals.count()) ||
                  (rangemode && i<context->compareDateRanges.count()); i++) {

        if (!isZoningEnabled()) {

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
    double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();

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

        // set the specification
        FilterSet fs;
        fs.addFilter(context->isfiltered, context->filters);
        fs.addFilter(context->ishomefiltered, context->homeFilters);
        Specification spec;
        spec.setDateRange(DateRange(cd.start,cd.end));
        spec.setFilterSet(fs);

        // set it from the metric
        setData(spec,  totalMetric, distMetric, &add);

        // add to the list
        compareData << add;

        // now add a curve for recalc to play with
        QwtPlotCurve *newCurve = new QwtPlotCurve("");
        newCurve->setStyle(QwtPlotCurve::Steps);

        if (appsettings->value(this, GC_ANTIALIAS, true).toBool()==true)
            newCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

        // curve has no brush .. too confusing...
        QPen pen;
        pen.setColor(cd.color);
        pen.setWidth(width);
        newCurve->setPen(pen);
        newCurve->setBrush(cd.color);   // fill below the line

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
PowerHist::setData(Specification specification, QString totalMetric, QString distMetric, HistData *data)
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
    double multiplier = pow(10, validatePrecision(m->precision()));
    double max = 0, min = 0;

    // LOOP THRU VALUES -- REPEATED WITH CUT AND PASTE BELOW
    // SO PLEASE MAKE SAME CHANGES TWICE (SORRY)
    foreach(RideItem *x, context->athlete->rideCache->rides()) { 

        // not passed
        if (!specification.pass(x)) continue;

        // get computed value
        double v = x->getForSymbol(distMetric, GlobalContext::context()->useMetricUnits);

        // ignore no temp files
        if ((distMetric == "average_temp" || distMetric == "max_temp") && v == RideFile::NA) continue;

        // clean up dodgy values
        if (std::isnan(v) || std::isinf(v)) v = 0;

        // seconds to minutes
        if (m->units(GlobalContext::context()->useMetricUnits) == "seconds" ||
            m->units(GlobalContext::context()->useMetricUnits) == tr("seconds")) v /= 60;

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
    foreach(RideItem *x, context->athlete->rideCache->rides()) { 

        // does it match
        if (!specification.pass(x)) continue;

        // get computed value
        double v = x->getForSymbol(distMetric, GlobalContext::context()->useMetricUnits);

        // ignore no temp files
        if ((distMetric == "average_temp" || distMetric == "max_temp") && v == RideFile::NA) continue;

        // clean up dodgy values
        if (std::isnan(v) || std::isinf(v)) v = 0;

        // seconds to minutes
        if (m->units(GlobalContext::context()->useMetricUnits) == "seconds" ||
            m->units(GlobalContext::context()->useMetricUnits) == tr("seconds")) v /= 60;

        // apply multiplier
        v *= multiplier;

        // ignore out of bounds data
        if ((int)(v)<min || (int)(v)>max) continue;

        // increment value, are inititialised to zero above
        // there will be some loss of precision due to totalising
        // a double in an int, but frankly that should be minimal
        // since most values of note are integer based anyway.
        double t = x->getForSymbol(totalMetric, GlobalContext::context()->useMetricUnits);

        // totalise in minutes
        if (tm->units(GlobalContext::context()->useMetricUnits) == "seconds" ||
            tm->units(GlobalContext::context()->useMetricUnits) == tr("seconds")) t /= 60;

        // sum up
        data->metricArray[(int)(v)-min] += t;
    }

    // we certainly don't want the interval curve when plotting
    // metrics across rides!
    curveSelected->hide();

    // now set all the plot parameters to match the data
    source = Metric;
    zoned = false;
    rideItem = NULL;
    lny = false;
    shade = false;
    withz = false;
    dt = 1;
    absolutetime = true;

    // and the plot itself
    QString yunits = tm->units(GlobalContext::context()->useMetricUnits);
    if (yunits == "seconds" || yunits == tr("seconds")) yunits = tr("minutes");
    QString xunits = m->units(GlobalContext::context()->useMetricUnits);
    if (xunits == "seconds" || xunits == tr("seconds")) xunits = tr("minutes");

    if (tm->units(GlobalContext::context()->useMetricUnits) != "")
        setAxisTitle(QwtAxis::YLeft, QString(tr("Total %1 (%2)")).arg(tm->name()).arg(yunits));
    else
        setAxisTitle(QwtAxis::YLeft, QString(tr("Total %1")).arg(tm->name()));

    if (m->units(GlobalContext::context()->useMetricUnits) != "")
        setAxisTitle(QwtAxis::XBottom, QString(tr("%1 of Activity (%2)")).arg(m->name()).arg(xunits));
    else
        setAxisTitle(QwtAxis::XBottom, QString(tr("%1 of Activity")).arg(m->name()));

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
    source = Ride;

    // hide hover curve just in case its there
    // from previous ride
    curveHover->hide();

    // we set with this data already
    if (force == false && _rideItem == LASTrideItem && source == LASTsource) return;

    rideItem = _rideItem;
    if (!rideItem) return;

    RideFile *ride = rideItem->ride();

    bool hasData = ((series == RideFile::watts || series == RideFile::wattsKg) && ride->areDataPresent()->watts) ||
                   (series == RideFile::nm && ride->areDataPresent()->nm) ||
                   (series == RideFile::kph && ride->areDataPresent()->kph) ||
                   (series == RideFile::gear && ride->areDataPresent()->gear) ||
                   (series == RideFile::smo2 && ride->areDataPresent()->smo2) ||
                   (series == RideFile::cad && ride->areDataPresent()->cad) ||
                   (series == RideFile::aPower && ride->areDataPresent()->apower) ||
                   (series == RideFile::wbal && ride->areDataPresent()->watts) ||
                   (series == RideFile::hr && ride->areDataPresent()->hr);

    if (ride && hasData) {
        //setTitle(ride->startTime().toString(GC_DATETIME_FORMAT));
        setArraysFromRide(ride, standard, context->athlete->zones(rideItem->sport), NULL);

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
PowerHist::setArraysFromRide(RideFile *ride, HistData &standard, const Zones *zones, IntervalItem *hover)
{
    // predefined deltas for each series
    static const double wattsDelta = 1.0;
    static const double wattsKgDelta = 0.01;
    static const double nmDelta    = 0.1;
    static const double hrDelta    = 1.0;
    static const double kphDelta   = 0.1;
    static const double cadDelta   = 1.0;
    static const double gearDelta  = 0.01; //RideFileCache creates POW(10) * decimals section
    static const double smo2Delta  = 1;
    static const double wbalDelta  = 1;
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
    standard.hrCPZoneArray.resize(0);
    standard.kphArray.resize(0);
    standard.paceZoneArray.resize(0);
    standard.paceCPZoneArray.resize(0);
    standard.gearArray.resize(0);
    standard.smo2Array.resize(0);
    standard.cadArray.resize(0);
    standard.wbalArray.resize(0);
    standard.wbalZoneArray.resize(0);

    standard.wattsSelectedArray.resize(0);
    standard.wattsZoneSelectedArray.resize(0);
    standard.wattsKgSelectedArray.resize(0);
    standard.aPowerSelectedArray.resize(0);
    standard.nmSelectedArray.resize(0);
    standard.hrSelectedArray.resize(0);
    standard.hrZoneSelectedArray.resize(0);
    standard.kphSelectedArray.resize(0);
    standard.paceZoneSelectedArray.resize(0);
    standard.gearSelectedArray.resize(0);
    standard.smo2SelectedArray.resize(0);
    standard.cadSelectedArray.resize(0);
    standard.wbalSelectedArray.resize(0);
    standard.wbalZoneSelectedArray.resize(0);

    // unit conversion factor for imperial units for selected parameters
    double torque_factor = (GlobalContext::context()->useMetricUnits ? 1.0 : FOOT_POUNDS_PER_METER);
    double speed_factor  = (GlobalContext::context()->useMetricUnits ? 1.0 : MILES_PER_KM);

    // cp and zones
    int zoneRange = zones ? zones->whichRange(ride->startTime().date()) : -1;
    int CP = zoneRange != -1 ? zones->getCP(zoneRange) : 0;
    int AeTP = zoneRange != -1 ? zones->getAeT(zoneRange) : 0;
    double WPRIME = zoneRange != -1 ? zones->getWprime(zoneRange) : 22000;

    int hrZoneRange = context->athlete->hrZones(ride->sport()) ? context->athlete->hrZones(ride->sport())->whichRange(ride->startTime().date()) : -1;
    int LTHR = hrZoneRange != -1 ? context->athlete->hrZones(ride->sport())->getLT(hrZoneRange) : 0;
    int AeTHR = hrZoneRange != -1 ? context->athlete->hrZones(ride->sport())->getAeT(hrZoneRange) : 0;

    int paceZoneRange = context->athlete->paceZones(ride->isSwim()) ? context->athlete->paceZones(ride->isSwim())->whichRange(ride->startTime().date()) : -1;
    double CV = (paceZoneRange != -1) ? context->athlete->paceZones(ride->isSwim())->getCV(paceZoneRange) : 0.0;
    double AeTV = (paceZoneRange != -1) ? context->athlete->paceZones(ride->isSwim())->getAeT(paceZoneRange) : 0.0;

    // get it from the wprimeData()->ydata() if we're plotting
    // w'bal, otherwise its from the ride data
    if (series == RideFile::wbal) {

        // always do 0-100%
        standard.wbalArray.resize(101);
        standard.wbalSelectedArray.resize(101);

        // fill with zeroes
        standard.wbalZoneArray.resize(4);
        standard.wbalZoneSelectedArray.resize(4);

        // t is time in seconds
        for(int t=0; t< ride->wprimeData()->ydata().count(); t++) {

            // get the value
            double value = ride->wprimeData()->ydata()[t];

            // percent left
            double percent = 100.0f - ((double (value) / WPRIME) * 100.0f);
            if (percent < 0.0f) percent = 0.0f;
            if (percent > 100.0f) percent = 100.0f;

            // is this point selected in an interval ?
            bool selected = false; 
            if (hover && t >= hover->start && t <= hover->stop) selected = true; 
            if (!selected) selected = isSelected(t, ride->recIntSecs());

            // watts array
            int wbalIndex = int(floor(percent / wbalDelta));
            if (wbalIndex >= 0 && wbalIndex < maxSize) {
                if (wbalIndex >= standard.wbalArray.size()) {
                    standard.wbalArray.resize(wbalIndex + 1);
                    standard.wbalArray[wbalIndex]=0;
                }
                standard.wbalArray[wbalIndex]++;

                if (selected) {
                    if (wbalIndex >= standard.wbalSelectedArray.size()) {
                        standard.wbalSelectedArray.resize(wbalIndex + 1);
                        standard.wbalSelectedArray[wbalIndex] = 0;
                    }
                    standard.wbalSelectedArray[wbalIndex]++;
                }
            }

            // zones
            if (percent < 25.0f) standard.wbalZoneArray[0] += ride->recIntSecs();
            else if (percent < 50.0f) standard.wbalZoneArray[1] += ride->recIntSecs();
            else if (percent < 75.0f) standard.wbalZoneArray[2] += ride->recIntSecs();
            else if (percent >= 75.0f) standard.wbalZoneArray[3] += ride->recIntSecs();

            if (selected) {
                if (percent < 25.0f) standard.wbalZoneSelectedArray[0] += ride->recIntSecs();
                else if (percent < 50.0f) standard.wbalZoneSelectedArray[1] += ride->recIntSecs();
                else if (percent < 75.0f) standard.wbalZoneSelectedArray[2] += ride->recIntSecs();
                else if (percent >= 75.0f) standard.wbalZoneSelectedArray[3] += ride->recIntSecs();
            }
        }

    } else {

        foreach(const RideFilePoint *p1, ride->dataPoints()) {

            // selected if hovered -or- selected depending on
            // whether we were passed a blank or real RideFileInterval
            bool selected = false; 
            if (hover) {
                if (p1->secs >= hover->start && p1->secs <= hover->stop) { selected = true; }
            } else {
                selected = isSelected(p1, ride->recIntSecs());
            }

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

                // cp zoned
                if (standard.wattsCPZoneArray.size() < 3) {
                    standard.wattsCPZoneArray.resize(3);
                }

                if (p1->watts < 1 && withz) { // I zero watts
                    standard.wattsCPZoneArray[0] ++;
                } else if (p1->watts < AeTP) { // I
                    standard.wattsCPZoneArray[0] ++;
                } else if (p1->watts < CP) { // II
                    standard.wattsCPZoneArray[1] ++;
                } else { // III
                    standard.wattsCPZoneArray[2] ++;
                }

                // get the zone
                wattsIndex = zones->whichZone(zoneRange, p1->watts);

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
            // Only calculate zones if we have a valid range
            if (hrZoneRange > -1 && (withz || (!withz && p1->hr))) {
                // cp zoned
                if (standard.hrCPZoneArray.size() < 3) {
                    standard.hrCPZoneArray.resize(3);
                }

                if (p1->hr < 1 && withz) { // I zero bpm
                    standard.hrCPZoneArray[0] ++;
                } else if (p1->hr < AeTHR) { // I
                    standard.hrCPZoneArray[0] ++;
                } else if (p1->hr < LTHR) { // II
                    standard.hrCPZoneArray[1] ++;
                } else { // III
                    standard.hrCPZoneArray[2] ++;
                }

                hrIndex = context->athlete->hrZones(ride->sport())->whichZone(hrZoneRange, p1->hr);

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

            // pace zoned array
            // Only calculate zones if we have a running activity with a valid range
            if ((ride->isRun() || ride->isSwim()) && paceZoneRange > -1 && (withz || (!withz && p1->kph))) {
                // cp zoned
                if (standard.paceCPZoneArray.size() < 3) {
                    standard.paceCPZoneArray.resize(3);
                }

                if (p1->kph < 0.1 && withz) { // I zero kph
                    standard.paceCPZoneArray[0] ++;
                } else if (p1->kph < AeTV) { // I
                    standard.paceCPZoneArray[0] ++;
                } else if (p1->kph < CV) { // II
                    standard.paceCPZoneArray[1] ++;
                } else { // III
                    standard.paceCPZoneArray[2] ++;
                }

                kphIndex = context->athlete->paceZones(ride->isSwim())->whichZone(paceZoneRange, p1->kph);

                if (kphIndex >= 0 && kphIndex < maxSize) {
                    if (kphIndex >= standard.paceZoneArray.size())
                        standard.paceZoneArray.resize(kphIndex + 1);
                    standard.paceZoneArray[kphIndex]++;

                    if (selected) {
                        if (kphIndex >= standard.paceZoneSelectedArray.size())
                            standard.paceZoneSelectedArray.resize(kphIndex + 1);
                        standard.paceZoneSelectedArray[kphIndex]++;
                    }
                }
            }

            int smo2Index = int(floor(p1->smo2 / smo2Delta));
            if (smo2Index >= 0 && smo2Index < maxSize) {
                if (smo2Index >= standard.smo2Array.size())
                    standard.smo2Array.resize(smo2Index + 1);
                standard.smo2Array[smo2Index]++;

                if (selected) {
                    if (smo2Index >= standard.smo2SelectedArray.size())
                        standard.smo2SelectedArray.resize(smo2Index + 1);
                    standard.smo2SelectedArray[smo2Index]++;
                }
            }

            int gearIndex = int(floor(p1->gear / gearDelta));
            if (gearIndex >= 0 && gearIndex < maxSize) {
                if (gearIndex >= standard.gearArray.size())
                    standard.gearArray.resize(gearIndex + 1);
                standard.gearArray[gearIndex]++;

                if (selected) {
                    if (gearIndex >= standard.gearSelectedArray.size())
                        standard.gearSelectedArray.resize(gearIndex + 1);
                    standard.gearSelectedArray[gearIndex]++;
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
    }
}

void
PowerHist::setBinWidth(double value)
{
    if (!value) value = 1; // binwidth must be nonzero
    binw = value;

    // hide the hover curve as it will now be wrong
    if (!rangemode) curveHover->hide();
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
PowerHist::setZoneLimited(bool value)
{
    zoneLimited = value;
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

        setAxisScaleEngine(QwtAxis::YLeft, new QwtLogScaleEngine);
        curve->setBaseline(1e-6);
        curveSelected->setBaseline(1e-6);

    } else {

        setAxisScaleEngine(QwtAxis::YLeft, new QwtLinearScaleEngine);
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

        case RideFile::wbal:
            if (zoned) axislabel = tr("W'bal zone");
            else axislabel = tr("W'Bal Consumed (%)");
            break;

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
            if (zoned && (!rideItem || rideItem->isRun || rideItem->isSwim))
                axislabel = tr("Pace zone");
            else
                axislabel = QString(tr("Speed (%1)")).arg(GlobalContext::context()->useMetricUnits ? tr("kph") : tr("mph"));
            break;

        case RideFile::nm:
            axislabel = QString(tr("Torque (%1)")).arg(GlobalContext::context()->useMetricUnits ? tr("N-m") : tr("ft-lbf"));
            break;

        case RideFile::gear:
            axislabel = tr("Gear Ratio");
            break;

        case RideFile::smo2:
            axislabel = tr("SmO2");
            break;

        default:
            axislabel = QString(tr("Unknown data series"));
            break;
    }
    setAxisTitle(QwtAxis::XBottom, axislabel);
    setAxisTitle(QwtAxis::YLeft, absolutetime ? tr("Time (minutes)") : tr("Time (percent)"));
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
    configChanged(CONFIG_APPEARANCE); // set colors
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

bool PowerHist::shadePaceZones() const
{
    return (rideItem && rideItem->ride() && series == RideFile::kph && !zoned && shade == true);
}

bool PowerHist::isSelected(const RideFilePoint *p, double sample)
{
    if (!rideItem) {

        foreach (IntervalItem *interval, rideItem->intervalsSelected()) {
            if (interval->selected && p->secs+sample>interval->start && p->secs<interval->stop) 
                return true;
        }
    }
    return false;
}

bool PowerHist::isSelected(const double t, double sample)
{
    if (rideItem) {

        foreach (IntervalItem *interval, rideItem->intervalsSelected()) {
            if (interval->selected && t+sample>interval->start && t<interval->stop) 
                return true;
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
                // for speed series add pace with units according to settings
                // only when there is no ride (home) or the activity is a run/swim.
                QString runPaceStr, swimPaceStr;
                if (series == RideFile::kph && (!rideItem || rideItem->isRun)) {
                    bool metricPace = appsettings->value(this, GC_PACE, GlobalContext::context()->useMetricUnits).toBool();
                    QString paceunit = metricPace ? tr("min/km") : tr("min/mile");
                    runPaceStr = tr("\n%1 Pace (%2)").arg(GlobalContext::context()->useMetricUnits ? kphToPace(xvalue, metricPace, false) : mphToPace(xvalue, metricPace, false)).arg(paceunit);
                }
                if (series == RideFile::kph && (!rideItem || rideItem->isSwim)) {
                    bool metricPace = appsettings->value(this, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
                    QString paceunit = metricPace ? tr("min/100m") : tr("min/100yd");
                    swimPaceStr = tr("\n%1 Pace (%2)").arg(GlobalContext::context()->useMetricUnits ? kphToPace(xvalue, metricPace, true) : mphToPace(xvalue, metricPace, true)).arg(paceunit);
                }
                // output the tooltip
                text = QString("%1 %2%5%6\n%3 %4")
                            .arg(xvalue, 0, 'f', digits)
                            .arg(this->axisTitle(curve->xAxis()).text())
                            .arg(yvalue, 0, 'f', 1)
                            .arg(absolutetime ? tr("minutes") : tr("%"))
                            .arg(runPaceStr)
                            .arg(swimPaceStr);
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

// Conditions to enable zoning, we can't zone for series besides watts, hr and
// kph only for running activities, so ignore zoning for those data series
bool
PowerHist::isZoningEnabled()
{
    // zoning valid for power, w/kg, hr, wbal and also
    // for speed (aka Pace; but only for swims and runs)
    return (zoned == true &&
            (series == RideFile::watts || series == RideFile::wattsKg || 
             series == RideFile::hr || series == RideFile::wbal ||
            (series == RideFile::kph && (!rideItem || rideItem->isRun || rideItem->isSwim))));
}
