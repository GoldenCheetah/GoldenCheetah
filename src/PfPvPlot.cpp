/*
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net),
 *                    J.T Conklin (jtc@acorntoolworks.com)
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

#include "PfPvPlot.h"
#include "Athlete.h"
#include "Context.h"
#include "RideFile.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "Settings.h"
#include "Zones.h"
#include "Colors.h"

#include <math.h>
#include <qwt_series_data.h>
#include <qwt_legend.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_widget.h>
#include <qwt_symbol.h>
#include <set>

#define PI M_PI


// Zone labels are drawn if power zone bands are enabled, automatically
// at the center of the plot
class PfPvPlotZoneLabel: public QwtPlotItem
{

private:

    PfPvPlot *parent;
    int zone_number;
    double watts;
    QwtText text;

public:

    PfPvPlotZoneLabel(PfPvPlot *_parent, int _zone_number)
    {
        parent = _parent;
        RideItem *rideItem = parent->rideItem;
        zone_number = _zone_number;

        // get zone data from ride or athlete ...
        const Zones *zones;
        int zone_range = -1;

        if (parent->context->isCompareIntervals) {

            zones = parent->context->athlete->zones();
            if (!zones) return;

            // use first compare interval date
            if (parent->context->compareIntervals.count())
                zone_range = zones->whichRange(parent->context->compareIntervals[0].data->startTime().date());

            // still not set 
            if (zone_range == -1)
                zone_range = zones->whichRange(QDate::currentDate());

        } else if (rideItem && parent->context->athlete->zones()) {

            zones = parent->context->athlete->zones();
            zone_range = zones->whichRange(rideItem->dateTime.date());

        } else {

            return; // nulls

        }

        setZ(1.0 + zone_number / 100.0);

        // create new zone labels if we're shading
        if (zone_range >= 0) {

            // retrieve zone setup
            QList <int> zone_lows = zones->getZoneLows(zone_range);
            QList <QString> zone_names = zones->getZoneNames(zone_range);
            int num_zones = zone_lows.size();
            if(zone_names.size() != num_zones) return;

            if (zone_number < num_zones) {

                watts = ((zone_number + 1 < num_zones) ?  0.5 * (zone_lows[zone_number] + zone_lows[zone_number + 1]) : ( (zone_number > 0) ?  (1.5 * zone_lows[zone_number] - 0.5 * zone_lows[zone_number - 1]) : 2.0 * zone_lows[zone_number]));

                text = QwtText(zone_names[zone_number]);
                text.setFont(QFont("Helvetica",24, QFont::Bold));
                QColor text_color = zoneColor(zone_number, num_zones);
                text_color.setAlpha(64);
                text.setColor(text_color);
            }
        }
    }

    virtual int rtti() const {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    void draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &rect) const {

        if (parent->shadeZones() && (rect.width() > 0) && (rect.height() > 0)) {
            // draw the label along a plot diagonal:
            // 1. x*y = watts * dx/dv * dy/df
            // 2. x/width = y/height
            // =>
            // 1. x^2 = width/height * watts
            // 2. y^2 = height/width * watts
            double xscale = fabs(xMap.transform(parent->maxCPV) - xMap.transform(0)) / parent->maxCPV;
            double yscale = fabs(yMap.transform(parent->maxAEPF) - yMap.transform(0)) / parent->maxAEPF;
            if ((xscale > 0) && (yscale > 0)) {
                double w = watts * xscale * yscale;
                int x = xMap.transform(sqrt(w * rect.width() / rect.height()) / xscale);
                int y = yMap.transform(sqrt(w * rect.height() / rect.width()) / yscale);

                // the following code based on source for QwtPlotMarker::draw()
                QRect tr(QPoint(0, 0), text.textSize(painter->font()).toSize());
                tr.moveCenter(QPoint(x, y));
                text.draw(painter, tr);
            }
        }
    }
};


PfPvPlot::PfPvPlot(Context *context)
    : rideItem (NULL), context(context), hover(NULL), cp_ (0), cad_ (85), cl_ (0.175), shade_zones(true)
{
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

    setAutoFillBackground(true);
    setAxisTitle(yLeft, tr("Average Effective Pedal Force (N)"));
    setAxisScale(yLeft, 0, 600);
    setAxisTitle(xBottom, tr("Circumferential Pedal Velocity (m/s)"));
    setAxisScale(xBottom, 0, 3);
    setAxisMaxMinor(yLeft, 0);
    setAxisMaxMinor(xBottom, 0);
    QwtScaleDraw *sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(xBottom, sd);
    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    setAxisScaleDraw(yLeft, sd);

    mX = new QwtPlotMarker();
    mX->setLineStyle(QwtPlotMarker::VLine);
    mX->attach(this);

    mY = new QwtPlotMarker();
    mY->setLineStyle(QwtPlotMarker::HLine);
    mY->attach(this);

    cpCurve = new QwtPlotCurve();
    cpCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    cpCurve->attach(this);

    curve = new QwtPlotCurve();
    curve->attach(this);

    cl_ = appsettings->value(this, GC_CRANKLENGTH).toDouble() / 1000.0;

    // markup timeInQuadrant
    tiqMarker[0] = new QwtPlotMarker(); tiqMarker[0]->attach(this);
    tiqMarker[0]->setXValue(2.9);
    tiqMarker[0]->setYValue(580);

    tiqMarker[1] = new QwtPlotMarker(); tiqMarker[1]->attach(this);
    tiqMarker[1]->setXValue(0.1);
    tiqMarker[1]->setYValue(580);

    tiqMarker[2] = new QwtPlotMarker(); tiqMarker[2]->attach(this);
    tiqMarker[2]->setXValue(0.1);
    tiqMarker[2]->setYValue(10);

    tiqMarker[3] = new QwtPlotMarker(); tiqMarker[3]->attach(this);
    tiqMarker[3]->setXValue(2.9);
    tiqMarker[3]->setYValue(10);

    merge_intervals = false;
    frame_intervals = true;
    gear_ratio_display = false;

    // only default on first time through, after this the user may have adjusted
    if (appsettings->value(this, GC_SHADEZONES, true).toBool()==false) shade_zones = false;
    else shade_zones = true;

    configChanged();

    recalc();
}

void
PfPvPlot::configChanged()
{
    setCanvasBackground(GColor(CPLOTBACKGROUND));

    // frame with inverse of background
    QwtSymbol *sym = new QwtSymbol;
    sym->setStyle(QwtSymbol::Ellipse);
    sym->setSize(4);
    sym->setPen(QPen(Qt::red));
    sym->setBrush(QBrush(Qt::red));
    curve->setSymbol(sym);
    curve->setStyle(QwtPlotCurve::Dots);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);

    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    setPalette(palette);

    axisWidget(QwtPlot::xBottom)->setPalette(palette);
    axisWidget(QwtPlot::yLeft)->setPalette(palette);

    // use grid line color for mX, mY and CPcurve
    QPen marker = GColor(CPLOTMARKER);
    QPen cp = GColor(CCP);
    mX->setLinePen(marker);
    mY->setLinePen(marker);
    cpCurve->setPen(cp);

    setCL(appsettings->value(this, GC_CRANKLENGTH).toDouble() / 1000.0);

    replot();
}

void
PfPvPlot::setAxisTitle(int axis, QString label)
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
PfPvPlot::refreshZoneItems()
{
    // clear out any zone curves which are presently defined
    if (zoneCurves.size()) {

        QListIterator<QwtPlotCurve *> i(zoneCurves);
        while (i.hasNext()) {
            QwtPlotCurve *curve = i.next();
            curve->detach();
            //delete curve;
        }
    }
    zoneCurves.clear();

    // delete any existing power zone labels
    if (zoneLabels.size()) {

        QListIterator<PfPvPlotZoneLabel *> i(zoneLabels);
        while (i.hasNext()) {
            PfPvPlotZoneLabel *label = i.next();
            label->detach();
            delete label;
        }
    }
    zoneLabels.clear();

    // set zones from ride or athlete and date of
    // first item in the compare set
    const Zones *zones = context->athlete->zones();
    int zone_range = -1;

    // comparing does zones for items selected not current ride
    if (context->isCompareIntervals) {

        // no athlete zones anyway!
        if (!zones) return;

        // use first compare interval date
        if (context->compareIntervals.count()) {
            zone_range = zones->whichRange(context->compareIntervals[0].data->startTime().date());
        }

        // still not set 
        if (zone_range == -1) {
            zone_range = zones->whichRange(QDate::currentDate());
        }

    } else if (rideItem && zones) {

        zone_range = zones->whichRange(rideItem->dateTime.date());

    } else {

        return; // null ride and not compare etc
    }

    if (zone_range >= 0) {
        setCP(zones->getCP(zone_range));

        // populate the zone curves
        QList <int> zone_power = zones->getZoneLows(zone_range);
        QList <QString> zone_name = zones->getZoneNames(zone_range);
        int num_zones = zone_power.size();
        if (zone_name.size() != num_zones) return;

        if (num_zones > 0) {
            QPen *pen = new QPen();
            pen->setStyle(Qt::NoPen);

            QwtArray<double> yvalues;

            // generate x values
            for (int z = 0; z < num_zones; z ++) {

                QwtPlotCurve *curve = new QwtPlotCurve(zone_name[z]);

                curve->setPen(*pen);
                QColor brush_color = zoneColor(z, num_zones);
                brush_color.setHsv(brush_color.hue(), brush_color.saturation() / 4, brush_color.value());
                curve->setBrush(brush_color);   // fill below the line
                curve->setZ(1 - 1e-6 * zone_power[z]);

                // generate data for curve
                if (z < num_zones - 1) {
                    QwtArray <double> contour_yvalues;
                    int watts = zone_power[z + 1];
                    int dwatts = (double) watts;
                    for (int i = 0; i < contour_xvalues.size(); i ++) {
                        contour_yvalues.append( (1e6 * contour_xvalues[i] < watts) ?  1e6 : dwatts / contour_xvalues[i]);
                    }
                    curve->setSamples(contour_xvalues, contour_yvalues);

                } else {

                    // top zone has a curve at "infinite" power
                    QwtArray <double> contour_x;
                    QwtArray <double> contour_y;
                    contour_x.append(contour_xvalues[0]);
                    contour_x.append(contour_xvalues[contour_xvalues.size() - 1]);
                    contour_y.append(1e6);
                    contour_y.append(1e6);
                    curve->setSamples(contour_x, contour_y);
                }

                curve->setVisible(shade_zones);
                curve->attach(this);
                zoneCurves.append(curve);
            }

            delete pen;

            // generate labels for existing zones
            for (int z = 0; z < num_zones; z ++) {
                PfPvPlotZoneLabel *label = new PfPvPlotZoneLabel(this, z);
                label->setVisible(shade_zones);
                label->attach(this);
                zoneLabels.append(label);
            }
        }
    }
}


// how many intervals selected?
int PfPvPlot::intervalCount() const
{
    int highlighted;
    highlighted = 0;
    if (context->athlete->allIntervalItems() == NULL) return 0; // not inited yet!

    for (int i=0; i<context->athlete->allIntervalItems()->childCount(); i++) {
        IntervalItem *current = dynamic_cast<IntervalItem *>(context->athlete->allIntervalItems()->child(i));
        if (current != NULL) {
            if (current->isSelected() == true) {
                ++highlighted;
            }
        }
    }
    return highlighted;
}

void 
PfPvPlot::mouseTrack(double cad, double watts)
{
    if (watts <= 0 || cad <= 0) return;

    double aepf = (watts * 60.0) / (cad * cl_ * 2.0 * PI);
    double cpv = (cad * cl_ * 2.0 * PI) / 60.0;

    if (rideItem && rideItem->ride() && rideItem->ride()->intervals().count() >= intervalMarkers.count()) {
        // are we hovering "close" to an interval marker ?
        int index = 0;
        foreach (QwtPlotMarker *is, intervalMarkers) {

            double x = is->xValue() - cpv;
            double y = is->yValue() - aepf;

            if ((x > -0.05f && x < 0.05f) && (y > -3 && y < 3)) {
                context->notifyIntervalHover(rideItem->ride()->intervals()[index]);
            }
    
            index++;
        }
    }
}

// for accumulating averages when refreshing interval markers
class accum { public: accum() : count(0), aepf(0), cpv(0) {} int count; double aepf; double cpv; };

void
PfPvPlot::refreshIntervalMarkers()
{
    // zap what we got ...
    foreach(QwtPlotMarker *is, intervalMarkers) {
        is->detach();
        delete is;
    }
    intervalMarkers.clear();

    // do we have a ride with intervals to refresh ?
    int count=0;
    if (rideItem && rideItem->ride() && rideItem->ride()->dataPoints().count() && (count = rideItem->ride()->intervals().count())) {

        // accumulating...
        QVector<accum> intervalAccumulator(count);

        foreach (RideFilePoint *p1, rideItem->ride()->dataPoints()) {

            if (p1->cad && p1->watts) {

                // calculate the values
                double aepf = (p1->watts * 60.0) / (p1->cad * cl_ * 2.0 * PI);
                double cpv = (p1->cad * cl_ * 2.0 * PI) / 60.0;

                // accumulate values for each interval here ....
                for(int i=0; i < count; i++) {

                    RideFileInterval v = rideItem->ride()->intervals()[i];

                    // in our interval ?
                    if (p1->secs >= v.start && p1->secs <= v.stop) {
                        intervalAccumulator[i].aepf += aepf;
                        intervalAccumulator[i].cpv += cpv;
                        intervalAccumulator[i].count++;
                    }
                }
            }
        }

        // ok, so now add markers for the average position for each interval
        int index = 0;
        foreach (accum a, intervalAccumulator) {

            QColor color;
            color.setHsv(index * 255/count, 255,255);
            double cpv = a.cpv/double(a.count);
            double aepf = a.aepf/double(a.count);

            QwtSymbol *sym = new QwtSymbol;
            sym->setStyle(QwtSymbol::Diamond);
            sym->setSize(8);
            sym->setPen(QPen(GColor(CPLOTMARKER)));
            sym->setBrush(QBrush(color));

            QwtPlotMarker *p = new QwtPlotMarker();
            p->setValue(cpv, aepf);
            p->setYAxis(yLeft);
            p->setSymbol(sym);
            p->attach(this);
            intervalMarkers << p;

            index++;
        }
    }
}

void
PfPvPlot::setData(RideItem *_rideItem)
{
    if (context->isCompareIntervals) return;

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

    // clear the hover curve
    if (hover) {
        hover->detach();
        delete hover;
        hover = NULL;
    }

    rideItem = _rideItem;
    RideFile *ride = rideItem->ride();

    if (ride) {

        // quickly erase old data
        mainCurvesSetVisible(false);


        // due to the discrete power and cadence values returned by the
        // power meter, there will very likely be many duplicate values.
        // Rather than pass them all to the curve, use a set to strip
        // out duplicates.
        std::set<std::pair<double, double> > dataSet;

        const int num_gearRatio_Plots = 4;
        QVector<std::set<std::pair<double, double> > > gearSet(num_gearRatio_Plots);
        long tot_cad = 0;
        long tot_cad_points = 0;


        foreach(const RideFilePoint *p1, ride->dataPoints()) {

            if (p1->watts != 0 && p1->cad != 0) {

                double aepf = (p1->watts * 60.0) / (p1->cad * cl_ * 2.0 * PI);
                double cpv = (p1->cad * cl_ * 2.0 * PI) / 60.0;

                if (aepf <= 2500) { // > 2500 newtons is our out of bounds
                    dataSet.insert(std::make_pair<double, double>(aepf, cpv));
                    tot_cad += p1->cad;
                    tot_cad_points++;

                    // fill the dataSets for gearRatio display
                    if (p1->gear != 0) {
                        if (p1->gear > 0.0f && p1->gear <= 1.00f) {
                            gearSet[0].insert(std::make_pair<double, double>(aepf, cpv));
                        } else if (p1->gear > 1.00f && p1->gear <= 2.50f ) {
                            gearSet[1].insert(std::make_pair<double, double>(aepf, cpv));
                        } else if (p1->gear > 2.50f && p1->gear <= 4.00 ) {
                            gearSet[2].insert(std::make_pair<double, double>(aepf, cpv));
                        } else {  // > 4.0
                            gearSet[3].insert(std::make_pair<double, double>(aepf, cpv));
                        }
                    }
                 }
            }
        }

        setCAD(tot_cad_points ? tot_cad / tot_cad_points : 0);

        if (tot_cad_points == 0) {
            //setTitle(tr("no cadence"));
            refreshZoneItems();
            mainCurvesSetVisible(false);

        } else {
            // Now that we have the set of points, transform them into the
            // QwtArrays needed to set the curve's data.
            QwtArray<double> aepfArray;
            QwtArray<double> cpvArray;

            std::set<std::pair<double, double> >::const_iterator j(dataSet.begin());
            while (j != dataSet.end()) {
                const std::pair<double, double>& dataPoint = *j;

                aepfArray.push_back(dataPoint.first);
                cpvArray.push_back(dataPoint.second);

                ++j;
            }

            curve->setSamples(cpvArray, aepfArray);

            QwtSymbol *sym = new QwtSymbol;
            sym->setStyle(QwtSymbol::Ellipse);
            sym->setSize(4);
            sym->setPen(QPen(Qt::red));
            sym->setBrush(QBrush(Qt::red));
            curve->setSymbol(sym);
            curve->setStyle(QwtPlotCurve::Dots);
            curve->setRenderHint(QwtPlotItem::RenderAntialiased);

            // now show the data (zone shading would already be visible)
            refreshZoneItems();

            // averages for intervals
            refreshIntervalMarkers();

            // now the curves for gearRatio
            if (gearRatioCurves.size()) {
               QListIterator<QwtPlotCurve *> g(gearRatioCurves);
               while (g.hasNext()) {
                   QwtPlotCurve *curve = g.next();
                   curve->detach();
                   delete curve;
               }
            }
            gearRatioCurves.clear();

            // get the detailed data
            QVector<QwtArray<double> > aepfArrayGearRatio(num_gearRatio_Plots);
            QVector<QwtArray<double> > cpvArrayGearRatio(num_gearRatio_Plots);

            for (int i=0;i<num_gearRatio_Plots;i++) {
                std::set<std::pair<double, double> >::const_iterator m(gearSet[i].begin());
                while (m != gearSet[i].end()) {
                    const std::pair<double, double>& dataPoint = *m;

                    aepfArrayGearRatio[i].push_back(dataPoint.first);
                    cpvArrayGearRatio[i].push_back(dataPoint.second);

                    ++m;
                }
            }

            // create the plots
            for (int i=0;i<num_gearRatio_Plots;i++) {
                QwtPlotCurve *gearCurve;
                gearCurve = new QwtPlotCurve();
                QwtSymbol *sym = new QwtSymbol;
                sym->setStyle(QwtSymbol::Diamond);
                sym->setSize(3);
                sym->setBrush(QBrush(Qt::NoBrush));
                sym->setPen(QPen(Qt::red));
                switch (i) {
                  case 0: sym->setPen(QPen(Qt::red));  sym->setBrush(QBrush(Qt::red));  break;
                  case 1: sym->setPen(QPen(Qt::yellow)); sym->setBrush(QBrush(Qt::yellow));break;
                  case 2: sym->setPen(QPen(Qt::green));  sym->setBrush(QBrush(Qt::green));break;
                  case 3: sym->setPen(QPen(Qt::blue));  sym->setBrush(QBrush(Qt::blue)); break;
                    // default means something is wrong, but no err
                  default: sym->setPen(QPen(Qt::white)); sym->setBrush(QBrush(Qt::white));
                }
                gearCurve->setSymbol(sym);
                gearCurve->setStyle(QwtPlotCurve::Dots);
                gearCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
                gearCurve->setSamples(cpvArrayGearRatio[i], aepfArrayGearRatio[i]);
                gearCurve->attach(this);

                gearRatioCurves.append(gearCurve);

            }

        }
        mainCurvesSetVisible(true);

    } else {

        //setTitle("no data");
        refreshZoneItems();
        mainCurvesSetVisible(false);
    }

    replot();
}

void
PfPvPlot::intervalHover(RideFileInterval x)
{
    if (!isVisible()) return;
    if (context->isCompareIntervals) return;
    if (!rideItem) return;
    if (!rideItem->ride()) return;

    // zap the old one
    if (hover) {
        hover->detach();
        delete hover;
        hover = NULL;
    }

    // collect the data
    QVector<double> aepfArray, cpvArray;
    foreach(const RideFilePoint *p1, rideItem->ride()->dataPoints()) {

        if (p1->secs < x.start || p1->secs > x.stop) continue;

        if (p1->watts != 0 && p1->cad != 0) {
            double aepf = (p1->watts * 60.0) / (p1->cad * cl_ * 2.0 * PI);
            double cpv = (p1->cad * cl_ * 2.0 * PI) / 60.0;

            aepfArray << aepf;
            cpvArray << cpv;
        }
    }

    // which interval is it or how many ?
    int count = 0;
    int ours = 0;
    foreach(RideFileInterval p, rideItem->ride()->intervals()) {
        if (p.start == x.start && p.stop == x.stop) ours = count;
        count++;
    }

    // any data ?
    if (aepfArray.size()) {
        QwtSymbol *sym = new QwtSymbol;
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(4);
        QColor color;
        color.setHsv(ours * 255/count, 255,255);
        color.setAlpha(128);
        sym->setPen(QPen(color));
        sym->setBrush(QBrush(color));

        hover = new QwtPlotCurve();
        hover->setSymbol(sym);
        hover->setStyle(QwtPlotCurve::Dots);
        hover->setRenderHint(QwtPlotItem::RenderAntialiased);
        hover->setSamples(cpvArray, aepfArray);
        hover->attach(this);
    }

    replot(); // refresh
}

void
PfPvPlot::showIntervals(RideItem *_rideItem)
{
    if (context->isCompareIntervals) return;
    if (!rideItem) return;

    // clear out any interval curves which are presently defined
    if (intervalCurves.size()) {
       QListIterator<QwtPlotCurve *> i(intervalCurves);
       while (i.hasNext()) {
           QwtPlotCurve *curve = i.next();
           curve->detach();
           //delete curve;
       }
    }
    intervalCurves.clear();

    rideItem = _rideItem;

    RideFile *ride = rideItem->ride();

    if (ride) {
       int num_intervals=intervalCount();

       if (mergeIntervals()) num_intervals = 1;
       if (frameIntervals() || num_intervals==0) mainCurvesSetVisible(true);
       if (frameIntervals()==false && num_intervals) mainCurvesSetVisible(false);
       QVector<std::set<std::pair<double, double> > > dataSetInterval(num_intervals);

       long tot_cad = 0;
       long tot_cad_points = 0;

        foreach(const RideFilePoint *p1, ride->dataPoints()) {

            if (p1->watts != 0 && p1->cad != 0) {
                double aepf = (p1->watts * 60.0) / (p1->cad * cl_ * 2.0 * PI);
                double cpv = (p1->cad * cl_ * 2.0 * PI) / 60.0;

                for (int high=-1, t=0; t<context->athlete->allIntervalItems()->childCount(); t++) {

                    IntervalItem *current = dynamic_cast<IntervalItem *>(context->athlete->allIntervalItems()->child(t));

                    if ((current != NULL) && current->isSelected()) {
                        ++high;
                        if (p1->secs+ride->recIntSecs() > current->start && p1->secs< current->stop) {
                            if (mergeIntervals())
                                dataSetInterval[0].insert(std::make_pair<double, double>(aepf, cpv));
                            else
                                dataSetInterval[high].insert(std::make_pair<double, double>(aepf, cpv));
                        }
                    }
                }
                tot_cad += p1->cad;
                tot_cad_points++;
            }
        }

        if (tot_cad_points > 0) {

           // Now that we have the set of points, transform them into the
           // QwtArrays needed to set the curve's data.
           QVector<QwtArray<double> > aepfArrayInterval(num_intervals);
           QVector<QwtArray<double> > cpvArrayInterval(num_intervals);

           for (int i=0;i<num_intervals;i++) {
               std::set<std::pair<double, double> >::const_iterator l(dataSetInterval[i].begin());
               while (l != dataSetInterval[i].end()) {
                   const std::pair<double, double>& dataPoint = *l;

                   aepfArrayInterval[i].push_back(dataPoint.first);
                   cpvArrayInterval[i].push_back(dataPoint.second);

                   ++l;
               }
           }



           // ensure same colors are used for each interval selected
           int num_intervals_defined=0;
           QVector<int> intervalmap;
           if (context->athlete->allIntervalItems() != NULL) {

                num_intervals_defined = context->athlete->allIntervalItems()->childCount();

                for (int g=0; g<context->athlete->allIntervalItems()->childCount(); g++) {
                    IntervalItem *curr = dynamic_cast<IntervalItem *>(context->athlete->allIntervalItems()->child(g));
                    if (curr->isSelected()) intervalmap.append(g);
                }
           }

            // honor display sequencing
            QMap<int, int> intervalOrder;
            int count=0;

            if (mergeIntervals()) intervalOrder.insert(1,0);
            else {
                for (int i=0; i<context->athlete->allIntervalItems()->childCount(); i++) {

                    IntervalItem *current = dynamic_cast<IntervalItem *>(context->athlete->allIntervalItems()->child(i));

                    if (current != NULL && current->isSelected() == true) {
                            intervalOrder.insert(current->displaySequence, count++);
                    }
                }
            }

            QMapIterator<int, int> order(intervalOrder);
            while (order.hasNext()) {
                order.next();
                int z = order.value();

                QwtPlotCurve *curve;
                curve = new QwtPlotCurve();

                QColor intervalColor;
                if (mergeIntervals())
                    intervalColor = Qt::red;
                else
                    intervalColor.setHsv((intervalmap.count() > 0 ? intervalmap.at(z) : 1) * 255/num_intervals_defined, 255,255);

                QwtSymbol *sym = new QwtSymbol;
                sym->setStyle(QwtSymbol::Ellipse);
                sym->setSize(4);
                sym->setBrush(QBrush(Qt::NoBrush));

                QPen pen;
                pen.setColor(intervalColor);
                sym->setPen(pen);
                intervalColor.setAlpha(128);
                sym->setBrush(QBrush(intervalColor));

                curve->setSymbol(sym);
                curve->setStyle(QwtPlotCurve::Dots);
                curve->setRenderHint(QwtPlotItem::RenderAntialiased);
                curve->setSamples(cpvArrayInterval[z],aepfArrayInterval[z]);
                curve->attach(this);

                intervalCurves.append(curve);
            }
        }
    }
    refreshIntervalMarkers();
    replot();
}

void
PfPvPlot::recalcCompare()
{
    // do all the calcuations for compare mode
    // we do as a separate method to 'isolate' compare mode
    // updates to ensure we don't break all the charts !!
    maxAEPF = 600;
    maxCPV = 3;

    foreach(CompareInterval ci, context->compareIntervals) {

        if (!ci.isChecked()) continue;

        // calculate maximums
        foreach(const RideFilePoint *p1, ci.data->dataPoints()) {

            if (p1->watts != 0 && p1->cad != 0) {

                double aepf = (p1->watts * 60.0) / (p1->cad * cl_ * 2.0 * PI);
                double cpv = (p1->cad * cl_ * 2.0 * PI) / 60.0;

                if (aepf < 255 && aepf > maxAEPF) maxAEPF = aepf;
                if (cpv > maxCPV) maxCPV = cpv;
            }
        }
    }

    if (maxAEPF > 600) {

        setAxisScale(yLeft, 0, (maxAEPF < 2500) ? (maxAEPF * 1.1) : 2500); // a bit of headroom
        tiqMarker[0]->setYValue(maxAEPF);
        tiqMarker[1]->setYValue(maxAEPF);

    } else {

        maxAEPF = 600; // for background shading and CP curve
        setAxisScale(yLeft, 0, 600);
        tiqMarker[0]->setYValue(580);
        tiqMarker[1]->setYValue(580);
    }

    if (maxCPV > 3) {

        // round *UP* to next integer for axis to fill nicely
        maxCPV = round(maxCPV + 0.5);
        setAxisScale(xBottom, 0, maxCPV);
        tiqMarker[0]->setXValue(maxCPV - 0.5);
        tiqMarker[3]->setXValue(maxCPV - 0.5);

    } else {

        maxCPV = 3; // for background shading and CP curve
        setAxisScale(xBottom, 0, 3);
        tiqMarker[0]->setXValue(2.9);
        tiqMarker[3]->setXValue(2.9);
    }

    // initialize x values used for contours
    contour_xvalues.clear();
    for (double x = 0; x <= maxCPV; x += x / 20 + 0.02) contour_xvalues.append(x);
    contour_xvalues.append(maxCPV);

    double cpv = (cad_ * cl_ * 2.0 * PI) / 60.0;
    mX->setXValue(cpv);

    double aepf = (cp_ * 60.0) / (cad_ * cl_ * 2.0 * PI);
    mY->setYValue(aepf);

    // reset to zero in  case there are none
    timeInQuadrant[0]=
    timeInQuadrant[1]=
    timeInQuadrant[2]=
    timeInQuadrant[3]= 0.0;

    // watch out for null rides
    foreach(CompareInterval ci, context->compareIntervals) {

        if (!ci.isChecked()) continue;

        foreach(const RideFilePoint *p1, ci.data->dataPoints()) {
            if (p1->watts != 0 && p1->cad != 0) {

                double aepf_ = (p1->watts * 60.0) / (p1->cad * cl_ * 2.0 * PI);
                double cpv_ = (p1->cad * cl_ * 2.0 * PI) / 60.0;

                // classic QA quadrants I II III and IV
                if (aepf_ > aepf && cpv_ > cpv) timeInQuadrant[0] += ci.data->recIntSecs();
                else if (aepf_ > aepf && cpv_ <= cpv) timeInQuadrant[1] += ci.data->recIntSecs();
                else if (aepf_ <= aepf && cpv_ <= cpv) timeInQuadrant[2] += ci.data->recIntSecs();
                else if (aepf_ <= aepf && cpv_ > cpv) timeInQuadrant[3] += ci.data->recIntSecs();

            }
        }
    }

    double totaltime = timeInQuadrant[0] + timeInQuadrant[1] + timeInQuadrant[2] + timeInQuadrant[3] ;

    if (totaltime) {

        QwtText t0(QString("%1%").arg(timeInQuadrant[0] / totaltime * 100, 0, 'f', 1),QwtText::PlainText);
        t0.setColor(GColor(CPLOTMARKER));
        tiqMarker[0]->setLabel(t0);

        QwtText t1(QString("%1%").arg(timeInQuadrant[1] / totaltime * 100, 0, 'f', 1),QwtText::PlainText);
        t1.setColor(GColor(CPLOTMARKER));
        tiqMarker[1]->setLabel(t1);

        QwtText t2(QString("%1%").arg(timeInQuadrant[2] / totaltime * 100, 0, 'f', 1),QwtText::PlainText);
        t2.setColor(GColor(CPLOTMARKER));
        tiqMarker[2]->setLabel(t2);

        QwtText t3(QString("%1%").arg(timeInQuadrant[3] / totaltime * 100, 0, 'f', 1),QwtText::PlainText);
        t3.setColor(GColor(CPLOTMARKER));
        tiqMarker[3]->setLabel(t3);

    } else {

        tiqMarker[0]->setLabel(QwtText("",QwtText::PlainText));
        tiqMarker[1]->setLabel(QwtText("",QwtText::PlainText));
        tiqMarker[2]->setLabel(QwtText("",QwtText::PlainText));
        tiqMarker[3]->setLabel(QwtText("",QwtText::PlainText));

    }
}

void
PfPvPlot::recalc()
{
    // we're for a ride only..
    if (context->isCompareIntervals) {
        recalcCompare();
        return;
    }

    // adjust the scales if we have some big values
    // this can happen with track sprinters who put
    // out big numbers for power and cadence since
    // hey have a fixed gear and big quads!
    maxAEPF = 600;
    maxCPV = 3;

    RideFile *ride;
    if (rideItem && (ride=rideItem->ride())) {

        // calculate maximums
        foreach(const RideFilePoint *p1, ride->dataPoints()) {

            if (p1->watts != 0 && p1->cad != 0) {

                double aepf = (p1->watts * 60.0) / (p1->cad * cl_ * 2.0 * PI);
                double cpv = (p1->cad * cl_ * 2.0 * PI) / 60.0;

                if (aepf < 255 && aepf > maxAEPF) maxAEPF = aepf;
                if (cpv > maxCPV) maxCPV = cpv;
            }
        }
    }

    if (maxAEPF > 600) {

        setAxisScale(yLeft, 0, (maxAEPF < 2500) ? (maxAEPF * 1.1) : 2500); // a bit of headroom
        tiqMarker[0]->setYValue(maxAEPF);
        tiqMarker[1]->setYValue(maxAEPF);

    } else {

        maxAEPF = 600; // for background shading and CP curve
        setAxisScale(yLeft, 0, 600);
        tiqMarker[0]->setYValue(580);
        tiqMarker[1]->setYValue(580);
    }

    if (maxCPV > 3) {

        // round *UP* to next integer for axis to fill nicely
        maxCPV = round(maxCPV + 0.5);
        setAxisScale(xBottom, 0, maxCPV);
        tiqMarker[0]->setXValue(maxCPV - 0.5);
        tiqMarker[3]->setXValue(maxCPV - 0.5);

    } else {

        maxCPV = 3; // for background shading and CP curve
        setAxisScale(xBottom, 0, 3);
        tiqMarker[0]->setXValue(2.9);
        tiqMarker[3]->setXValue(2.9);
    }

    // initialize x values used for contours
    contour_xvalues.clear();
    for (double x = 0; x <= maxCPV; x += x / 20 + 0.02) contour_xvalues.append(x);
    contour_xvalues.append(maxCPV);

    double cpv = (cad_ * cl_ * 2.0 * PI) / 60.0;
    mX->setXValue(cpv);

    double aepf = (cp_ * 60.0) / (cad_ * cl_ * 2.0 * PI);
    mY->setYValue(aepf);

    // watch out for null rides
    if (rideItem && (ride=rideItem->ride())) {

        timeInQuadrant[0]=
        timeInQuadrant[1]=
        timeInQuadrant[2]=
        timeInQuadrant[3]= 0.0;

        foreach(const RideFilePoint *p1, ride->dataPoints()) {
            if (p1->watts != 0 && p1->cad != 0) {

                double aepf_ = (p1->watts * 60.0) / (p1->cad * cl_ * 2.0 * PI);
                double cpv_ = (p1->cad * cl_ * 2.0 * PI) / 60.0;

                // classic QA quadrants I II III and IV
                if (aepf_ > aepf && cpv_ > cpv) timeInQuadrant[0] += ride->recIntSecs();
                else if (aepf_ > aepf && cpv_ <= cpv) timeInQuadrant[1] += ride->recIntSecs();
                else if (aepf_ <= aepf && cpv_ <= cpv) timeInQuadrant[2] += ride->recIntSecs();
                else if (aepf_ <= aepf && cpv_ > cpv) timeInQuadrant[3] += ride->recIntSecs();

            }
        }
        double totaltime = timeInQuadrant[0] + timeInQuadrant[1] + timeInQuadrant[2] + timeInQuadrant[3] ;

        if (totaltime) {

            QwtText t0(QString("%1%").arg(timeInQuadrant[0] / totaltime * 100, 0, 'f', 1),QwtText::PlainText);
            t0.setColor(GColor(CPLOTMARKER));
            tiqMarker[0]->setLabel(t0);

            QwtText t1(QString("%1%").arg(timeInQuadrant[1] / totaltime * 100, 0, 'f', 1),QwtText::PlainText);
            t1.setColor(GColor(CPLOTMARKER));
            tiqMarker[1]->setLabel(t1);

            QwtText t2(QString("%1%").arg(timeInQuadrant[2] / totaltime * 100, 0, 'f', 1),QwtText::PlainText);
            t2.setColor(GColor(CPLOTMARKER));
            tiqMarker[2]->setLabel(t2);

            QwtText t3(QString("%1%").arg(timeInQuadrant[3] / totaltime * 100, 0, 'f', 1),QwtText::PlainText);
            t3.setColor(GColor(CPLOTMARKER));
            tiqMarker[3]->setLabel(t3);

        } else {

            tiqMarker[0]->setLabel(QwtText("",QwtText::PlainText));
            tiqMarker[1]->setLabel(QwtText("",QwtText::PlainText));
            tiqMarker[2]->setLabel(QwtText("",QwtText::PlainText));
            tiqMarker[3]->setLabel(QwtText("",QwtText::PlainText));

        }

    } else {

        tiqMarker[0]->setLabel(QwtText("",QwtText::PlainText));
        tiqMarker[1]->setLabel(QwtText("",QwtText::PlainText));
        tiqMarker[2]->setLabel(QwtText("",QwtText::PlainText));
        tiqMarker[3]->setLabel(QwtText("",QwtText::PlainText));

    }

    QwtArray<double> yvalues(contour_xvalues.size());

    if (cp_) {

        // reinitialise array
        for (int i = 0; i < contour_xvalues.size(); i ++)
            yvalues[i] = (cpv < cp_ / 1e6) ?  1e6 : cp_ / contour_xvalues[i];

        // generate curve at a given power
        cpCurve->setSamples(contour_xvalues, yvalues);

    } else {

        // an empty curve if no power (or zero power) is specified
        QwtArray<double> data;
        cpCurve->setSamples(data,data);
    }
}

int
PfPvPlot::getCP()
{
    return cp_;
}

void
PfPvPlot::setCP(int cp)
{
    cp_ = cp;
    recalc();
    emit changedCP( QString("%1").arg(cp) );
}

int
PfPvPlot::getCAD()
{
    return cad_;
}

void
PfPvPlot::setCAD(int cadence)
{
    cad_ = cadence;
    recalc();
    emit changedCAD( QString("%1").arg(cadence) );
}

double
PfPvPlot::getCL()
{
    return cl_;
}

void
PfPvPlot::setCL(double cranklen)
{
    cl_ = cranklen;
    recalc();
    emit changedCL( QString("%1").arg(cranklen) );
}
// process checkbox for zone shading
void
PfPvPlot::setShadeZones(bool value)
{
    shade_zones = value;

    // if there are defined zones and labels, set their visibility
    for (int i = 0; i < zoneCurves.size(); i ++)
    zoneCurves[i]->setVisible(shade_zones);
    for (int i = 0; i < zoneLabels.size(); i ++)
    zoneLabels[i]->setVisible(shade_zones);

    //replot();
}

void
PfPvPlot::setMergeIntervals(bool value)
{
    merge_intervals = value;
    showIntervals(rideItem);
}

void
PfPvPlot::setFrameIntervals(bool value)
{
    frame_intervals = value;
    showIntervals(rideItem);
}

void
PfPvPlot::setGearRatioDisplay(bool value)
{
    gear_ratio_display = value;
    if (context->isCompareIntervals) return;
    mainCurvesSetVisible(true);
    replot();
}

void
PfPvPlot::showCompareIntervals()
{
    // Remove interval curve
    if (intervalCurves.size()) {
       QListIterator<QwtPlotCurve *> i(intervalCurves);
       while (i.hasNext()) {
           QwtPlotCurve *curve = i.next();
           curve->detach();
           //delete curve;
       }
    }
    intervalCurves.clear();

    // quickly erase old data
    mainCurvesSetVisible(false);

    int num_intervals = context->compareIntervals.count();

    // If not compare mode or no intervals
    if (context->compareIntervals.count() == 0) return;

    recalcCompare();

    QVector<std::set<std::pair<double, double> > > dataSetInterval(num_intervals);
    long tot_cad = 0;
    long tot_cad_points = 0;

    // prepare aggregates
    for (int high=-1, i = 0; i < context->compareIntervals.size(); ++i) {
        CompareInterval interval = context->compareIntervals.at(i);

        if (interval.isChecked())  {
            ++high;

            foreach(const RideFilePoint *p1, interval.data->dataPoints()) {
                if (p1->watts != 0 && p1->cad != 0) {
                    double aepf = (p1->watts * 60.0) / (p1->cad * cl_ * 2.0 * PI);
                    double cpv = (p1->cad * cl_ * 2.0 * PI) / 60.0;

                    if (mergeIntervals())
                        dataSetInterval[0].insert(std::make_pair<double, double>(aepf, cpv));
                    else
                        dataSetInterval[high].insert(std::make_pair<double, double>(aepf, cpv));

                    tot_cad += p1->cad;
                    tot_cad_points++;
                }
            }
        }

    }

    if (tot_cad_points > 0) {

        // Now that we have the set of points, transform them into the
        // QwtArrays needed to set the curve's data.
        QVector<QwtArray<double> > aepfArrayInterval(num_intervals);
        QVector<QwtArray<double> > cpvArrayInterval(num_intervals);

        for (int i=0;i<num_intervals;i++) {
            std::set<std::pair<double, double> >::const_iterator l(dataSetInterval[i].begin());
            while (l != dataSetInterval[i].end()) {
                const std::pair<double, double>& dataPoint = *l;

                aepfArrayInterval[i].push_back(dataPoint.first);
                cpvArrayInterval[i].push_back(dataPoint.second);

                ++l;
            }
        }

        // ensure same colors are used for each interval selected
        QMap<int, CompareInterval> intervalmap;
        int count=0;

        if (num_intervals > 0) {
            for (int g=0; g<num_intervals; g++) {
                CompareInterval interval = context->compareIntervals.at(g);
                if (interval.checked) intervalmap.insert(count++, interval);
            }
        }

        QMapIterator<int, CompareInterval> order(intervalmap);
        while (order.hasNext()) {
            order.next();
            CompareInterval interval = order.value();

            QwtPlotCurve *_curve;
            _curve = new QwtPlotCurve();

            QColor intervalColor;
            if (mergeIntervals())
                intervalColor = Qt::red;
            else
                intervalColor = interval.color;

            QwtSymbol *sym = new QwtSymbol;
            sym->setStyle(QwtSymbol::Ellipse);
            sym->setSize(4);
            sym->setBrush(QBrush(Qt::NoBrush));

            QPen pen;
            pen.setColor(intervalColor);
            sym->setPen(pen);
            intervalColor.setAlpha(128);
            sym->setBrush(QBrush(intervalColor));

            _curve->setSymbol(sym);
            _curve->setStyle(QwtPlotCurve::Dots);
            _curve->setRenderHint(QwtPlotItem::RenderAntialiased);
            _curve->setSamples(cpvArrayInterval[order.key()],aepfArrayInterval[order.key()]);
            _curve->attach(this);

            intervalCurves.append(_curve);
        }
    }

    refreshZoneItems();
    replot();
}

void
PfPvPlot::mainCurvesSetVisible(bool visible) {

    if (gearRatioCurves.size()) {
        QListIterator<QwtPlotCurve *> i(gearRatioCurves);
        while (i.hasNext()) {
            QwtPlotCurve *curve = i.next();
            curve->setVisible(visible ? gear_ratio_display : false);
        }
    }

    curve->setVisible(visible ? !gear_ratio_display : false);

}
