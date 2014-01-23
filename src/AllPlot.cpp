/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "AllPlot.h"
#include "Context.h"
#include "Athlete.h"
#include "AllPlotWindow.h"
#include "ReferenceLineDialog.h"
#include "RideFile.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "Settings.h"
#include "Units.h"
#include "Zones.h"
#include "Colors.h"
#include "WPrime.h"

#include <qwt_plot_curve.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_intervalcurve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_div.h>
#include <qwt_scale_widget.h>
#include <qwt_compat.h>
#include <qwt_text.h>
#include <qwt_legend.h>
#include <qwt_series_data.h>
#include <QMultiMap>

#include <string.h> // for memcpy

class IntervalPlotData : public QwtSeriesData<QPointF>
{
    public:
    IntervalPlotData(AllPlot *allPlot, Context *context) :
        allPlot(allPlot), context(context) {}
    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    //virtual QwtData *copy() const ;
    void init() ;
    IntervalItem *intervalNum(int n) const;
    int intervalCount() const;
    AllPlot *allPlot;
    Context *context;

    virtual QPointF sample(size_t i) const;
    virtual QRectF boundingRect() const;
};

// define a background class to handle shading of power zones
// draws power zone bands IF zones are defined and the option
// to draw bonds has been selected
class AllPlotBackground: public QwtPlotItem
{
    private:
        AllPlot *parent;

    public:
        AllPlotBackground(AllPlot *_parent)
        {
            setZ(0.0);
            parent = _parent;
        }

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    virtual void draw(QPainter *painter,
                      const QwtScaleMap &, const QwtScaleMap &yMap,
                      const QRectF &rect) const
    {
        RideItem *rideItem = parent->rideItem;

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

        } else if (rideItem) {

            zones = rideItem->zones;
            zone_range = rideItem->zoneRange();

        } else {

            return; // nulls

        }

        if (parent->shadeZones() && (zone_range >= 0)) {
            QList <int> zone_lows = zones->getZoneLows(zone_range);
            int num_zones = zone_lows.size();
            if (num_zones > 0) {
                for (int z = 0; z < num_zones; z ++) {
                    QRect r = rect.toRect();

                    QColor shading_color = zoneColor(z, num_zones);
                    shading_color.setHsv(
                        shading_color.hue(),
                        shading_color.saturation() / 4,
                        shading_color.value()
                        );
                    r.setBottom(yMap.transform(zone_lows[z]));
                    if (z + 1 < num_zones)
                        r.setTop(yMap.transform(zone_lows[z + 1]));
                    if (r.top() <= r.bottom())
                        painter->fillRect(r, shading_color);
                }
            }
        } else {
        }
    }
};

// Zone labels are drawn if power zone bands are enabled, automatically
// at the center of the plot
class AllPlotZoneLabel: public QwtPlotItem
{
    private:
        AllPlot *parent;
        int zone_number;
        double watts;
        QwtText text;

    public:
        AllPlotZoneLabel(AllPlot *_parent, int _zone_number)
        {
            parent = _parent;
            zone_number = _zone_number;

            RideItem *rideItem = parent->rideItem;

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

            } else if (rideItem) {

                zones = rideItem->zones;
                zone_range = rideItem->zoneRange();

            } else {

                return; // nulls

            }

            // create new zone labels if we're shading
            if (parent->shadeZones() && (zone_range >= 0)) {
                QList <int> zone_lows = zones->getZoneLows(zone_range);
                QList <QString> zone_names = zones->getZoneNames(zone_range);
                int num_zones = zone_lows.size();
                if (zone_names.size() != num_zones) return;
                if (zone_number < num_zones) {
                    watts =
                        (
                            (zone_number + 1 < num_zones) ?
                            0.5 * (zone_lows[zone_number] + zone_lows[zone_number + 1]) :
                            (
                                (zone_number > 0) ?
                                (1.5 * zone_lows[zone_number] - 0.5 * zone_lows[zone_number - 1]) :
                                2.0 * zone_lows[zone_number]
                            )
                        );

                    text = QwtText(zone_names[zone_number]);
                    if (_parent->referencePlot == NULL) {
                        text.setFont(QFont("Helvetica",24, QFont::Bold));
                    } else {
                        text.setFont(QFont("Helvetica",12, QFont::Bold));
                    }

                    QColor text_color = zoneColor(zone_number, num_zones);
                    text_color.setAlpha(64);
                    text.setColor(text_color);
                }
            }

            setZ(1.0 + zone_number / 100.0);
        }
        virtual int rtti() const
        {
            return QwtPlotItem::Rtti_PlotUserItem;
        }

        void draw(QPainter *painter,
                  const QwtScaleMap &, const QwtScaleMap &yMap,
                  const QRectF &rect) const
        {
            if (parent->shadeZones()) {
                int x = (rect.left() + rect.right()) / 2;
                int y = yMap.transform(watts);

                // the following code based on source for QwtPlotMarker::draw()
                QRect tr(QPoint(0, 0), text.textSize(painter->font()).toSize());
                tr.moveCenter(QPoint(x, y));
                text.draw(painter, tr);
            }
        }
};

class TimeScaleDraw: public QwtScaleDraw
{

    public:

    TimeScaleDraw(bool *bydist) : QwtScaleDraw(), bydist(bydist) {}

    virtual QwtText label(double v) const
    {
        if (*bydist) {
            return QString("%1").arg(v);
        } else {
            QTime t = QTime().addSecs(v*60.00);
            if (scaleMap().sDist() > 5)
                return t.toString("hh:mm");
            return t.toString("hh:mm:ss");
        }
    }
    private:
    bool *bydist;

};

static inline double
max(double a, double b) { if (a > b) return a; else return b; }

AllPlotObject::AllPlotObject(AllPlot *plot) : plot(plot)
{
    maxKM = maxSECS = 0;

    wattsCurve = new QwtPlotCurve(tr("Power"));
    wattsCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    wattsCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 0));

    npCurve = new QwtPlotCurve(tr("NP"));
    npCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    npCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 0));

    xpCurve = new QwtPlotCurve(tr("xPower"));
    xpCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    xpCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 0));

    apCurve = new QwtPlotCurve(tr("aPower"));
    apCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    apCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 0));

    hrCurve = new QwtPlotCurve(tr("Heart Rate"));
    hrCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    hrCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));

    speedCurve = new QwtPlotCurve(tr("Speed"));
    speedCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    speedCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 0));

    cadCurve = new QwtPlotCurve(tr("Cadence"));
    cadCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    cadCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));

    altCurve = new QwtPlotCurve(tr("Altitude"));
    altCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    // standard->altCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    altCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 1));
    altCurve->setZ(-10); // always at the back.

    tempCurve = new QwtPlotCurve(tr("Temperature"));
    tempCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    if (plot->context->athlete->useMetricUnits)
        tempCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 0));
    else
        tempCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1)); // with cadence

    windCurve = new QwtPlotIntervalCurve(tr("Wind"));
    windCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 0));

    torqueCurve = new QwtPlotCurve(tr("Torque"));
    torqueCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    torqueCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 0));

    balanceLCurve = new QwtPlotCurve(tr("Left Balance"));
    balanceLCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    balanceLCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));

    balanceRCurve = new QwtPlotCurve(tr("Right Balance"));
    balanceRCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    balanceRCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));

    wCurve = new QwtPlotCurve(tr("W' Balance (j)"));
    wCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    wCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 2));

    mCurve = new QwtPlotCurve(tr("Matches"));
    mCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    mCurve->setStyle(QwtPlotCurve::Dots);
    mCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 2));

    curveTitle.attach(plot);
    curveTitle.setLabelAlignment(Qt::AlignRight);

    intervalHighlighterCurve = new QwtPlotCurve();
    intervalHighlighterCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 0));
    intervalHighlighterCurve->attach(plot);

    // setup that standard->grid
    grid = new QwtPlotGrid();
    grid->enableX(true);
    grid->enableY(true);
    grid->attach(plot);

}

// we tend to only do this for the compare objects
void
AllPlotObject::setColor(QColor color)
{
    QList<QwtPlotCurve*> worklist;
    worklist << mCurve << wCurve << wattsCurve << npCurve << xpCurve << speedCurve
             << apCurve << cadCurve << tempCurve << hrCurve << torqueCurve << balanceLCurve
             << balanceRCurve << altCurve;

    // work through getting progresively lighter
    QPen pen;
    pen.setWidth(1.0);
    int alpha = 200;
    bool antialias = appsettings->value(this, GC_ANTIALIAS, false).toBool();
    foreach(QwtPlotCurve *c, worklist) {

        pen.setColor(color);
        color.setAlpha(alpha);

        c->setPen(pen);
        if (antialias) c->setRenderHint(QwtPlotItem::RenderAntialiased);

        // lighten up for the next guy
        color = color.darker(110);
        alpha -= 10;
    }

    // has to be different...
    windCurve->setPen(pen);
    if (antialias)windCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

    // and alt needs a feint brush
    altCurve->setBrush(QBrush(altCurve->pen().color().lighter(150)));

}

// wipe those curves
AllPlotObject::~AllPlotObject()
{
    grid->detach(); delete grid;
    mCurve->detach(); delete mCurve;
    wCurve->detach(); delete wCurve;
    wattsCurve->detach(); delete wattsCurve;
    npCurve->detach(); delete npCurve;
    xpCurve->detach(); delete xpCurve;
    apCurve->detach(); delete apCurve;
    hrCurve->detach(); delete hrCurve;
    speedCurve->detach(); delete speedCurve;
    cadCurve->detach(); delete cadCurve;
    altCurve->detach(); delete altCurve;
    tempCurve->detach(); delete tempCurve;
    windCurve->detach(); delete windCurve;
    torqueCurve->detach(); delete torqueCurve;
    balanceLCurve->detach(); delete balanceLCurve;
    balanceRCurve->detach(); delete balanceRCurve;
}

void
AllPlotObject::setVisible(bool show)
{
    if (show == false) {

        grid->detach(); 
        mCurve->detach();
        wCurve->detach();
        wattsCurve->detach();
        npCurve->detach();
        xpCurve->detach();
        apCurve->detach();
        hrCurve->detach();
        speedCurve->detach();
        cadCurve->detach();
        altCurve->detach();
        tempCurve->detach();
        windCurve->detach();
        torqueCurve->detach();
        balanceLCurve->detach();
        balanceRCurve->detach();
        intervalHighlighterCurve->detach();

        // marks, calibrations and reference lines
        foreach(QwtPlotMarker *mrk, d_mrk) {
            mrk->detach();
        }
        foreach(QwtPlotCurve *referenceLine, referenceLines) {
            referenceLine->detach();
        }

    } else {


        altCurve->attach(plot); // always do first as it hasa brush
        grid->attach(plot);
        mCurve->attach(plot);
        wCurve->attach(plot);
        wattsCurve->attach(plot);
        npCurve->attach(plot);
        xpCurve->attach(plot);
        apCurve->attach(plot);
        hrCurve->attach(plot);
        speedCurve->attach(plot);
        cadCurve->attach(plot);
        tempCurve->attach(plot);
        windCurve->attach(plot);
        torqueCurve->attach(plot);
        balanceLCurve->attach(plot);
        balanceRCurve->attach(plot);

        intervalHighlighterCurve->attach(plot);

        // marks, calibrations and reference lines
        foreach(QwtPlotMarker *mrk, d_mrk) {
            mrk->attach(plot);
        }
        foreach(QwtPlotCurve *referenceLine, referenceLines) {
            referenceLine->attach(plot);
        }

    }
}

void
AllPlotObject::hideUnwanted()
{
    if (plot->showPowerState>1) wattsCurve->detach();
    if (!plot->showNP) npCurve->detach();
    if (!plot->showXP) xpCurve->detach();
    if (!plot->showAP) apCurve->detach();
    if (!plot->showW) wCurve->detach();
    if (!plot->showW) mCurve->detach();
    if (!plot->showHr) hrCurve->detach();
    if (!plot->showSpeed) speedCurve->detach();
    if (!plot->showCad) cadCurve->detach();
    if (!plot->showAlt) altCurve->detach();
    if (!plot->showTemp) tempCurve->detach();
    if (!plot->showWind) windCurve->detach();
    if (!plot->showTorque) torqueCurve->detach();
    if (!plot->showBalance) balanceLCurve->detach();
    if (!plot->showBalance) balanceRCurve->detach();
}

AllPlot::AllPlot(AllPlotWindow *parent, Context *context, RideFile::SeriesType scope, RideFile::SeriesType secScope, bool wanttext):
    QwtPlot(parent),
    rideItem(NULL),
    shade_zones(true),
    showPowerState(3),
    showNP(false),
    showXP(false),
    showAP(false),
    showHr(true),
    showSpeed(true),
    showCad(true),
    showAlt(true),
    showTemp(true),
    showWind(true),
    showTorque(true),
    showBalance(true),
    bydist(false),
    scope(scope),
    secondaryScope(secScope),
    context(context),
    parent(parent),
    wanttext(wanttext)
{

    if (appsettings->value(this, GC_SHADEZONES, true).toBool()==false)
        shade_zones = false;

    smooth = 1;
    wantaxis = true;
    setAutoDelete(false); // no - we are managing it via the AllPlotObjects now
    referencePlot = NULL;

    // create a background object for shading
    bg = new AllPlotBackground(this);
    bg->attach(this);

    //insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

    // set the axes that we use..
    setAxesCount(QwtAxis::yLeft, 2);
    setAxesCount(QwtAxis::yRight, 3);
    setAxesCount(QwtAxis::xBottom, 1);

    setXTitle();

    standard = new AllPlotObject(this);

    standard->intervalHighlighterCurve->setSamples(new IntervalPlotData(this, context));

    setAxisMaxMinor(xBottom, 0);
    enableAxis(xBottom, true);
    setAxisVisible(xBottom, true);
    setAxisMaxMinor(yLeft, 0);
    setAxisMaxMinor(QwtAxisId(QwtAxis::yLeft, 1), 0);
    setAxisMaxMinor(yRight, 0);
    setAxisMaxMinor(QwtAxisId(QwtAxis::yRight, 1), 0);

    axisWidget(QwtPlot::yLeft)->installEventFilter(this);

    configChanged(); // set colors
}

AllPlot::~AllPlot()
{
    // wipe compare curves if there are any
    foreach(QwtPlotCurve *compare, compares) {
        compare->detach(); delete compare;
    }
    compares.clear();

    // wipe the standard stuff
    delete standard;
}

void
AllPlot::configChanged()
{
    double width = appsettings->value(this, GC_LINEWIDTH, 2.0).toDouble();

    if (appsettings->value(this, GC_ANTIALIAS, false).toBool() == true) {
        standard->wattsCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->npCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->xpCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->apCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->wCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->mCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->hrCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->speedCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->cadCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->altCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->tempCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->windCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->torqueCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->balanceLCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->balanceRCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        standard->intervalHighlighterCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    }

    setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
    QPen wattsPen = QPen(GColor(CPOWER));
    wattsPen.setWidth(width);
    standard->wattsCurve->setPen(wattsPen);
    QPen npPen = QPen(GColor(CNPOWER));
    npPen.setWidth(width);
    standard->npCurve->setPen(npPen);
    QPen xpPen = QPen(GColor(CXPOWER));
    xpPen.setWidth(width);
    standard->xpCurve->setPen(xpPen);
    QPen apPen = QPen(GColor(CAPOWER));
    apPen.setWidth(width);
    standard->apCurve->setPen(apPen);
    QPen hrPen = QPen(GColor(CHEARTRATE));
    hrPen.setWidth(width);
    standard->hrCurve->setPen(hrPen);
    QPen speedPen = QPen(GColor(CSPEED));
    speedPen.setWidth(width);
    standard->speedCurve->setPen(speedPen);
    QPen cadPen = QPen(GColor(CCADENCE));
    cadPen.setWidth(width);
    standard->cadCurve->setPen(cadPen);
    QPen altPen(GColor(CALTITUDE));
    altPen.setWidth(width);
    standard->altCurve->setPen(altPen);
    QColor brush_color = GColor(CALTITUDEBRUSH);
    brush_color.setAlpha(200);
    standard->altCurve->setBrush(brush_color);   // fill below the line
    QPen tempPen = QPen(GColor(CTEMP));
    tempPen.setWidth(width);
    standard->tempCurve->setPen(tempPen);
    //QPen windPen = QPen(GColor(CWINDSPEED));
    //windPen.setWidth(width);
    standard->windCurve->setPen(QPen(Qt::NoPen));
    QColor wbrush_color = GColor(CWINDSPEED);
    wbrush_color.setAlpha(200);
    standard->windCurve->setBrush(wbrush_color);   // fill below the line
    QPen torquePen = QPen(GColor(CTORQUE));
    torquePen.setWidth(width);
    standard->torqueCurve->setPen(torquePen);
    QPen balanceLPen = QPen(GColor(CBALANCERIGHT));
    balanceLPen.setWidth(width);
    standard->balanceLCurve->setPen(balanceLPen);
    QColor brbrush_color = GColor(CBALANCERIGHT);
    brbrush_color.setAlpha(200);
    standard->balanceLCurve->setBrush(brbrush_color);   // fill below the line
    QPen balanceRPen = QPen(GColor(CBALANCELEFT));
    balanceRPen.setWidth(width);
    standard->balanceRCurve->setPen(balanceRPen);
    QColor blbrush_color = GColor(CBALANCELEFT);
    blbrush_color.setAlpha(200);
    standard->balanceRCurve->setBrush(blbrush_color);   // fill below the line
    QPen wPen = QPen(GColor(CWBAL)); 
    wPen.setWidth(2); // thicken
    standard->wCurve->setPen(wPen);
    QwtSymbol *sym = new QwtSymbol;
    sym->setStyle(QwtSymbol::Rect);
    sym->setPen(QPen(QColor(255,127,0))); // orange like a match, will make configurable later
    sym->setSize(4);
    standard->mCurve->setSymbol(sym);
    QPen ihlPen = QPen(GColor(CINTERVALHIGHLIGHTER));
    ihlPen.setWidth(width);
    standard->intervalHighlighterCurve->setPen(ihlPen);
    QColor ihlbrush = QColor(GColor(CINTERVALHIGHLIGHTER));
    ihlbrush.setAlpha(128);
    standard->intervalHighlighterCurve->setBrush(ihlbrush);   // fill below the line
    //this->legend()->remove(intervalHighlighterCurve); // don't show in legend
    QPen gridPen(GColor(CPLOTGRID));
    gridPen.setStyle(Qt::DotLine);
    standard->grid->setPen(gridPen);

    // curve brushes
    if (parent->isPaintBrush()) {
        QColor p;
        p = standard->wattsCurve->pen().color();
        p.setAlpha(64);
        standard->wattsCurve->setBrush(QBrush(p));

        p = standard->npCurve->pen().color();
        p.setAlpha(64);
        standard->npCurve->setBrush(QBrush(p));

        p = standard->xpCurve->pen().color();
        p.setAlpha(64);
        standard->xpCurve->setBrush(QBrush(p));

        p = standard->apCurve->pen().color();
        p.setAlpha(64);
        standard->apCurve->setBrush(QBrush(p));

        p = standard->wCurve->pen().color();
        p.setAlpha(64);
        standard->wCurve->setBrush(QBrush(p));

        p = standard->hrCurve->pen().color();
        p.setAlpha(64);
        standard->hrCurve->setBrush(QBrush(p));

        p = standard->speedCurve->pen().color();
        p.setAlpha(64);
        standard->speedCurve->setBrush(QBrush(p));

        p = standard->cadCurve->pen().color();
        p.setAlpha(64);
        standard->cadCurve->setBrush(QBrush(p));

        p = standard->torqueCurve->pen().color();
        p.setAlpha(64);
        standard->torqueCurve->setBrush(QBrush(p));

        p = standard->tempCurve->pen().color();
        p.setAlpha(64);
        standard->tempCurve->setBrush(QBrush(p));

        /*p = standard->balanceLCurve->pen().color();
        p.setAlpha(64);
        standard->balanceLCurve->setBrush(QBrush(p));

        p = standard->balanceRCurve->pen().color();
        p.setAlpha(64);
        standard->balanceRCurve->setBrush(QBrush(p));*/
    } else {
        standard->wattsCurve->setBrush(Qt::NoBrush);
        standard->npCurve->setBrush(Qt::NoBrush);
        standard->xpCurve->setBrush(Qt::NoBrush);
        standard->apCurve->setBrush(Qt::NoBrush);
        standard->wCurve->setBrush(Qt::NoBrush);
        standard->hrCurve->setBrush(Qt::NoBrush);
        standard->speedCurve->setBrush(Qt::NoBrush);
        standard->cadCurve->setBrush(Qt::NoBrush);
        standard->torqueCurve->setBrush(Qt::NoBrush);
        standard->tempCurve->setBrush(Qt::NoBrush);
        //standard->balanceLCurve->setBrush(Qt::NoBrush);
        //standard->balanceRCurve->setBrush(Qt::NoBrush);
    }

    QPalette pal;

    // tick draw
    TimeScaleDraw *tsd = new TimeScaleDraw(&this->bydist) ;
    tsd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtPlot::xBottom, tsd);
    pal.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    pal.setColor(QPalette::Text, GColor(CPLOTMARKER));
    axisWidget(QwtPlot::xBottom)->setPalette(pal);
    enableAxis(xBottom, true);
    setAxisVisible(xBottom, true);

    QwtScaleDraw *sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    setAxisScaleDraw(QwtPlot::yLeft, sd);
    pal.setColor(QPalette::WindowText, GColor(CPOWER));
    pal.setColor(QPalette::Text, GColor(CPOWER));
    axisWidget(QwtPlot::yLeft)->setPalette(pal);

    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    setAxisScaleDraw(QwtAxisId(QwtAxis::yLeft, 1), sd);
    pal.setColor(QPalette::WindowText, GColor(CHEARTRATE));
    pal.setColor(QPalette::Text, GColor(CHEARTRATE));
    axisWidget(QwtAxisId(QwtAxis::yLeft, 1))->setPalette(pal);

    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    setAxisScaleDraw(QwtPlot::yRight, sd);
    pal.setColor(QPalette::WindowText, GColor(CSPEED));
    pal.setColor(QPalette::Text, GColor(CSPEED));
    axisWidget(QwtPlot::yRight)->setPalette(pal);

    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    setAxisScaleDraw(QwtAxisId(QwtAxis::yRight, 1), sd);
    pal.setColor(QPalette::WindowText, GColor(CALTITUDE));
    pal.setColor(QPalette::Text, GColor(CALTITUDE));
    axisWidget(QwtAxisId(QwtAxis::yRight, 1))->setPalette(pal);

    sd = new QwtScaleDraw;
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    sd->setLabelRotation(90);// in the 000s
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtAxisId(QwtAxis::yRight, 2), sd);
    pal.setColor(QPalette::WindowText, GColor(CWBAL));
    pal.setColor(QPalette::Text, GColor(CWBAL));
    axisWidget(QwtAxisId(QwtAxis::yRight, 2))->setPalette(pal);
}

void 
AllPlot::setHighlightIntervals(bool state)
{
    if (state) standard->intervalHighlighterCurve->attach(this);
    else standard->intervalHighlighterCurve->detach();
}

struct DataPoint {
    double time, hr, watts, np, ap, xp, speed, cad, alt, temp, wind, torque, lrbalance;
    DataPoint(double t, double h, double w, double n, double l, double x, double s, double c, double a, double te, double wi, double tq, double lrb) :
        time(t), hr(h), watts(w), np(n), ap(l), xp(x), speed(s), cad(c), alt(a), temp(te), wind(wi), torque(tq), lrbalance(lrb) {}
};

bool AllPlot::shadeZones() const
{
    return shade_zones;
}

void
AllPlot::setAxisTitle(QwtAxisId axis, QString label)
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

void AllPlot::refreshZoneLabels()
{
    foreach(AllPlotZoneLabel *label, zoneLabels) {
        label->detach();
        delete label;
    }
    zoneLabels.clear();

    if (rideItem) {
        int zone_range = rideItem->zoneRange();
        const Zones *zones = rideItem->zones;

        // generate labels for existing zones
        if (zone_range >= 0) {
            int num_zones = zones->numZones(zone_range);
            for (int z = 0; z < num_zones; z ++) {
                AllPlotZoneLabel *label = new AllPlotZoneLabel(this, z);
                label->attach(this);
                zoneLabels.append(label);
            }
        }
    }
}


void
AllPlot::recalc(AllPlotObject *objects)
{
    if (referencePlot !=NULL){
        return;
    }

    if (objects->timeArray.empty())
        return;

    int rideTimeSecs = (int) ceil(objects->timeArray[objects->timeArray.count()-1]);
    if (rideTimeSecs > 7*24*60*60) {
        QwtArray<double> data;
        QVector<QwtIntervalSample> intData;

        objects->wCurve->setSamples(data,data);
        objects->mCurve->setSamples(data,data);
        if (!objects->npArray.empty())
            objects->npCurve->setSamples(data, data);
        if (!objects->xpArray.empty())
            objects->xpCurve->setSamples(data, data);
        if (!objects->apArray.empty())
            objects->apCurve->setSamples(data, data);
        if (!objects->wattsArray.empty())
            objects->wattsCurve->setSamples(data, data);
        if (!objects->hrArray.empty())
            objects->hrCurve->setSamples(data, data);
        if (!objects->speedArray.empty())
            objects->speedCurve->setSamples(data, data);
        if (!objects->cadArray.empty())
            objects->cadCurve->setSamples(data, data);
        if (!objects->altArray.empty())
            objects->altCurve->setSamples(data, data);
        if (!objects->tempArray.empty())
            objects->tempCurve->setSamples(data, data);
        if (!objects->windArray.empty())
            objects->windCurve->setSamples(new QwtIntervalSeriesData(intData));
        if (!objects->torqueArray.empty())
            objects->torqueCurve->setSamples(data, data);
        if (!objects->balanceArray.empty())
            objects->balanceLCurve->setSamples(data, data);
        if (!objects->balanceArray.empty())
            objects->balanceRCurve->setSamples(data, data);

        return;
    }

    // we should only smooth the curves if objects->smoothed rate is greater than sample rate
    if (smooth > 0) {

        double totalWatts = 0.0;
        double totalNP = 0.0;
        double totalXP = 0.0;
        double totalAP = 0.0;
        double totalHr = 0.0;
        double totalSpeed = 0.0;
        double totalCad = 0.0;
        double totalDist = 0.0;
        double totalAlt = 0.0;
        double totalTemp = 0.0;
        double totalWind = 0.0;
        double totalTorque = 0.0;
        double totalBalance = 0.0;

        QList<DataPoint> list;

        objects->smoothWatts.resize(rideTimeSecs + 1); //(rideTimeSecs + 1);
        objects->smoothNP.resize(rideTimeSecs + 1); //(rideTimeSecs + 1);
        objects->smoothXP.resize(rideTimeSecs + 1); //(rideTimeSecs + 1);
        objects->smoothAP.resize(rideTimeSecs + 1); //(rideTimeSecs + 1);
        objects->smoothHr.resize(rideTimeSecs + 1);
        objects->smoothSpeed.resize(rideTimeSecs + 1);
        objects->smoothCad.resize(rideTimeSecs + 1);
        objects->smoothTime.resize(rideTimeSecs + 1);
        objects->smoothDistance.resize(rideTimeSecs + 1);
        objects->smoothAltitude.resize(rideTimeSecs + 1);
        objects->smoothTemp.resize(rideTimeSecs + 1);
        objects->smoothWind.resize(rideTimeSecs + 1);
        objects->smoothRelSpeed.resize(rideTimeSecs + 1);
        objects->smoothTorque.resize(rideTimeSecs + 1);
        objects->smoothBalanceL.resize(rideTimeSecs + 1);
        objects->smoothBalanceR.resize(rideTimeSecs + 1);

        for (int secs = 0; ((secs < smooth)
                            && (secs < rideTimeSecs)); ++secs) {
            objects->smoothWatts[secs] = 0.0;
            objects->smoothNP[secs] = 0.0;
            objects->smoothXP[secs] = 0.0;
            objects->smoothAP[secs] = 0.0;
            objects->smoothHr[secs]    = 0.0;
            objects->smoothSpeed[secs] = 0.0;
            objects->smoothCad[secs]   = 0.0;
            objects->smoothTime[secs]  = secs / 60.0;
            objects->smoothDistance[secs]  = 0.0;
            objects->smoothAltitude[secs]  = 0.0;
            objects->smoothTemp[secs]  = 0.0;
            objects->smoothWind[secs]  = 0.0;
            objects->smoothRelSpeed[secs] = QwtIntervalSample();
            objects->smoothTorque[secs]  = 0.0;
            objects->smoothBalanceL[secs]  = 50;
            objects->smoothBalanceR[secs]  = 50;
        }

        int i = 0;
        for (int secs = smooth; secs <= rideTimeSecs; ++secs) {
            while ((i < objects->timeArray.count()) && (objects->timeArray[i] <= secs)) {
                DataPoint dp(objects->timeArray[i],
                             (!objects->hrArray.empty() ? objects->hrArray[i] : 0),
                             (!objects->wattsArray.empty() ? objects->wattsArray[i] : 0),
                             (!objects->npArray.empty() ? objects->npArray[i] : 0),
                             (!objects->apArray.empty() ? objects->apArray[i] : 0),
                             (!objects->xpArray.empty() ? objects->xpArray[i] : 0),
                             (!objects->speedArray.empty() ? objects->speedArray[i] : 0),
                             (!objects->cadArray.empty() ? objects->cadArray[i] : 0),
                             (!objects->altArray.empty() ? objects->altArray[i] : 0),
                             (!objects->tempArray.empty() ? objects->tempArray[i] : 0),
                             (!objects->windArray.empty() ? objects->windArray[i] : 0),
                             (!objects->torqueArray.empty() ? objects->torqueArray[i] : 0),
                             (!objects->balanceArray.empty() ? objects->balanceArray[i] : 0));
                if (!objects->wattsArray.empty())
                    totalWatts += objects->wattsArray[i];
                if (!objects->npArray.empty())
                    totalNP += objects->npArray[i];
                if (!objects->xpArray.empty())
                    totalXP += objects->xpArray[i];
                if (!objects->apArray.empty())
                    totalAP += objects->apArray[i];
                if (!objects->hrArray.empty())
                    totalHr    += objects->hrArray[i];
                if (!objects->speedArray.empty())
                    totalSpeed += objects->speedArray[i];
                if (!objects->cadArray.empty())
                    totalCad   += objects->cadArray[i];
                if (!objects->altArray.empty())
                    totalAlt   += objects->altArray[i];
                if (!objects->tempArray.empty() ) {
                    if (objects->tempArray[i] == RideFile::noTemp) {
                        dp.temp = (i>0 && !list.empty()?list.back().temp:0.0);
                        totalTemp   += dp.temp;
                    }
                    else {
                        totalTemp   += objects->tempArray[i];
                    }
                }
                if (!objects->windArray.empty())
                    totalWind   += objects->windArray[i];
                if (!objects->torqueArray.empty())
                    totalTorque   += objects->torqueArray[i];
                if (!objects->balanceArray.empty())
                    totalBalance   += (objects->balanceArray[i]>0?objects->balanceArray[i]:50);

                totalDist   = objects->distanceArray[i];
                list.append(dp);
                ++i;
            }
            while (!list.empty() && (list.front().time < secs - smooth)) {
                DataPoint &dp = list.front();
                totalWatts -= dp.watts;
                totalNP -= dp.np;
                totalAP -= dp.ap;
                totalXP -= dp.xp;
                totalHr    -= dp.hr;
                totalSpeed -= dp.speed;
                totalCad   -= dp.cad;
                totalAlt   -= dp.alt;
                totalTemp   -= dp.temp;
                totalWind   -= dp.wind;
                totalTorque   -= dp.torque;
                totalBalance   -= (dp.lrbalance>0?dp.lrbalance:50);
                list.removeFirst();
            }
            // TODO: this is wrong.  We should do a weighted average over the
            // seconds represented by each point...
            if (list.empty()) {
                objects->smoothWatts[secs] = 0.0;
                objects->smoothNP[secs] = 0.0;
                objects->smoothXP[secs] = 0.0;
                objects->smoothAP[secs] = 0.0;
                objects->smoothHr[secs]    = 0.0;
                objects->smoothSpeed[secs] = 0.0;
                objects->smoothCad[secs]   = 0.0;
                objects->smoothAltitude[secs]   = objects->smoothAltitude[secs - 1];
                objects->smoothTemp[secs]   = 0.0;
                objects->smoothWind[secs] = 0.0;
                objects->smoothRelSpeed[secs] =  QwtIntervalSample();
                objects->smoothTorque[secs] = 0.0;
                objects->smoothBalanceL[secs] = 50;
                objects->smoothBalanceR[secs] = 50;
            }
            else {
                objects->smoothWatts[secs]    = totalWatts / list.size();
                objects->smoothNP[secs]    = totalNP / list.size();
                objects->smoothXP[secs]    = totalXP / list.size();
                objects->smoothAP[secs]    = totalAP / list.size();
                objects->smoothHr[secs]       = totalHr / list.size();
                objects->smoothSpeed[secs]    = totalSpeed / list.size();
                objects->smoothCad[secs]      = totalCad / list.size();
                objects->smoothAltitude[secs]      = totalAlt / list.size();
                objects->smoothTemp[secs]      = totalTemp / list.size();
                objects->smoothWind[secs]    = totalWind / list.size();
                objects->smoothRelSpeed[secs] =  QwtIntervalSample( bydist ? totalDist : secs / 60.0, QwtInterval(qMin(totalWind / list.size(), totalSpeed / list.size()), qMax(totalWind / list.size(), totalSpeed / list.size()) ) );
                objects->smoothTorque[secs]    = totalTorque / list.size();

                double balance = totalBalance / list.size();
                if (balance == 0) {
                    objects->smoothBalanceL[secs]    = 50;
                    objects->smoothBalanceR[secs]    = 50;
                } else if (balance >= 50) {
                    objects->smoothBalanceL[secs]    = balance;
                    objects->smoothBalanceR[secs]    = 50;
                }
                else {
                    objects->smoothBalanceL[secs]    = 50;
                    objects->smoothBalanceR[secs]    = balance;
                }
            }
            objects->smoothDistance[secs] = totalDist;
            objects->smoothTime[secs]  = secs / 60.0;
        }

    } else {

        // no standard->smoothing .. just raw data
        objects->smoothWatts.resize(0);
        objects->smoothNP.resize(0);
        objects->smoothXP.resize(0);
        objects->smoothAP.resize(0);
        objects->smoothHr.resize(0);
        objects->smoothSpeed.resize(0);
        objects->smoothCad.resize(0);
        objects->smoothTime.resize(0);
        objects->smoothDistance.resize(0);
        objects->smoothAltitude.resize(0);
        objects->smoothTemp.resize(0);
        objects->smoothWind.resize(0);
        objects->smoothRelSpeed.resize(0);
        objects->smoothTorque.resize(0);
        objects->smoothBalanceL.resize(0);
        objects->smoothBalanceR.resize(0);

        foreach (RideFilePoint *dp, rideItem->ride()->dataPoints()) {
            objects->smoothWatts.append(dp->watts);
            objects->smoothNP.append(dp->np);
            objects->smoothXP.append(dp->xp);
            objects->smoothAP.append(dp->apower);
            objects->smoothHr.append(dp->hr);
            objects->smoothSpeed.append(context->athlete->useMetricUnits ? dp->kph : dp->kph * MILES_PER_KM);
            objects->smoothCad.append(dp->cad);
            objects->smoothTime.append(dp->secs/60);
            objects->smoothDistance.append(context->athlete->useMetricUnits ? dp->km : dp->km * MILES_PER_KM);
            objects->smoothAltitude.append(context->athlete->useMetricUnits ? dp->alt : dp->alt * FEET_PER_METER);
            if (dp->temp == RideFile::noTemp && !objects->smoothTemp.empty())
                dp->temp = objects->smoothTemp.last();
            objects->smoothTemp.append(context->athlete->useMetricUnits ? dp->temp : dp->temp * FAHRENHEIT_PER_CENTIGRADE + FAHRENHEIT_ADD_CENTIGRADE);
            objects->smoothWind.append(context->athlete->useMetricUnits ? dp->headwind : dp->headwind * MILES_PER_KM);
            objects->smoothTorque.append(dp->nm);

            if (dp->lrbalance == 0) {
                objects->smoothBalanceL.append(50);
                objects->smoothBalanceR.append(50);
            }
            else if (dp->lrbalance >= 50) {
                objects->smoothBalanceL.append(dp->lrbalance);
                objects->smoothBalanceR.append(50);
            }
            else {
                objects->smoothBalanceL.append(50);
                objects->smoothBalanceR.append(dp->lrbalance);
            }

            double head = dp->headwind * (context->athlete->useMetricUnits ? 1.0f : MILES_PER_KM);
            double speed = dp->kph * (context->athlete->useMetricUnits ? 1.0f : MILES_PER_KM);
            objects->smoothRelSpeed.append(QwtIntervalSample( bydist ? objects->smoothDistance.last() : objects->smoothTime.last(), QwtInterval(qMin(head, speed) , qMax(head, speed) ) ));

        }
    }

    QVector<double> &xaxis = bydist ? objects->smoothDistance : objects->smoothTime;
    int startingIndex = qMin(smooth, xaxis.count());
    int totalPoints = xaxis.count() - startingIndex;

    // set curves - we set the intervalHighlighter to whichver is available

    //W' curve set to whatever data we have
    if (rideItem && rideItem->ride()) {
        objects->wCurve->setSamples(rideItem->ride()->wprimeData()->xdata().data(), rideItem->ride()->wprimeData()->ydata().data(), rideItem->ride()->wprimeData()->xdata().count());
        objects->mCurve->setSamples(rideItem->ride()->wprimeData()->mxdata().data(), rideItem->ride()->wprimeData()->mydata().data(), rideItem->ride()->wprimeData()->mxdata().count());
    }

    if (!objects->wattsArray.empty()) {
        objects->wattsCurve->setSamples(xaxis.data() + startingIndex, objects->smoothWatts.data() + startingIndex, totalPoints);
        standard->intervalHighlighterCurve->setYAxis(yLeft);
    }

    if (!objects->npArray.empty()) {
        objects->npCurve->setSamples(xaxis.data() + startingIndex, objects->smoothNP.data() + startingIndex, totalPoints);
        standard->intervalHighlighterCurve->setYAxis(yLeft);
    }

    if (!objects->xpArray.empty()) {
        objects->xpCurve->setSamples(xaxis.data() + startingIndex, objects->smoothXP.data() + startingIndex, totalPoints);
        standard->intervalHighlighterCurve->setYAxis(yLeft);
    }

    if (!objects->apArray.empty()) {
        objects->apCurve->setSamples(xaxis.data() + startingIndex, objects->smoothAP.data() + startingIndex, totalPoints);
        standard->intervalHighlighterCurve->setYAxis(yLeft);
    }

    if (!objects->hrArray.empty()) {
        objects->hrCurve->setSamples(xaxis.data() + startingIndex, objects->smoothHr.data() + startingIndex, totalPoints);
        standard->intervalHighlighterCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));
    }

    if (!objects->speedArray.empty()) {
        objects->speedCurve->setSamples(xaxis.data() + startingIndex, objects->smoothSpeed.data() + startingIndex, totalPoints);
        standard->intervalHighlighterCurve->setYAxis(yRight);
    }

    if (!objects->cadArray.empty()) {
        objects->cadCurve->setSamples(xaxis.data() + startingIndex, objects->smoothCad.data() + startingIndex, totalPoints);
        standard->intervalHighlighterCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));
    }

    if (!objects->altArray.empty()) {
        objects->altCurve->setSamples(xaxis.data() + startingIndex, objects->smoothAltitude.data() + startingIndex, totalPoints);
        standard->intervalHighlighterCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 1));
    }

    if (!objects->tempArray.empty()) {
        objects->tempCurve->setSamples(xaxis.data() + startingIndex, objects->smoothTemp.data() + startingIndex, totalPoints);
        if (context->athlete->useMetricUnits)
            standard->intervalHighlighterCurve->setYAxis(yRight);
        else
            standard->intervalHighlighterCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));
    }


    if (!objects->windArray.empty()) {
        objects->windCurve->setSamples(new QwtIntervalSeriesData(objects->smoothRelSpeed));
        standard->intervalHighlighterCurve->setYAxis(yRight);
    }

    if (!objects->torqueArray.empty()) {
        objects->torqueCurve->setSamples(xaxis.data() + startingIndex, objects->smoothTorque.data() + startingIndex, totalPoints);
        standard->intervalHighlighterCurve->setYAxis(yRight);
    }

    if (!objects->balanceArray.empty()) {
        objects->balanceLCurve->setSamples(xaxis.data() + startingIndex, objects->smoothBalanceL.data() + startingIndex, totalPoints);
        standard->intervalHighlighterCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));
        objects->balanceRCurve->setSamples(xaxis.data() + startingIndex, objects->smoothBalanceR.data() + startingIndex, totalPoints);
        standard->intervalHighlighterCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));
    }

    setYMax();

    if (!context->isCompareIntervals) {
        refreshReferenceLines();
        refreshIntervalMarkers();
        refreshCalibrationMarkers();
        refreshZoneLabels();
    }

    replot();
}

void
AllPlot::refreshIntervalMarkers()
{
    foreach(QwtPlotMarker *mrk, standard->d_mrk) {
        mrk->detach();
        delete mrk;
    }
    standard->d_mrk.clear();
    QRegExp wkoAuto("^(Peak *[0-9]*(s|min)|Entire workout|Find #[0-9]*) *\\([^)]*\\)$");
    if (rideItem->ride()) {
        foreach(const RideFileInterval &interval, rideItem->ride()->intervals()) {
            // skip WKO autogenerated peak intervals
            if (wkoAuto.exactMatch(interval.name))
                continue;
            QwtPlotMarker *mrk = new QwtPlotMarker;
            standard->d_mrk.append(mrk);
            mrk->attach(this);
            mrk->setLineStyle(QwtPlotMarker::VLine);
            mrk->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
            mrk->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));

            // put matches on second line down
            QString name(interval.name);
            if (interval.name.startsWith(tr("Match"))) name = QString("\n%1").arg(interval.name);

            QwtText text(wanttext ? name : "");
            text.setFont(QFont("Helvetica", 10, QFont::Bold));
            if (interval.name.startsWith(tr("Match"))) 
                text.setColor(GColor(CWBAL));
            else
                text.setColor(GColor(CPLOTMARKER));
            if (!bydist)
                mrk->setValue(interval.start / 60.0, 0.0);
            else
                mrk->setValue((context->athlete->useMetricUnits ? 1 : MILES_PER_KM) *
                                rideItem->ride()->timeToDistance(interval.start), 0.0);
            mrk->setLabel(text);
        }
    }
}

void
AllPlot::refreshCalibrationMarkers()
{
    foreach(QwtPlotMarker *mrk, standard->cal_mrk) {
        mrk->detach();
        delete mrk;
    }
    standard->cal_mrk.clear();

    if (rideItem->ride()) {
        foreach(const RideFileCalibration &calibration, rideItem->ride()->calibrations()) {
            QwtPlotMarker *mrk = new QwtPlotMarker;
            standard->cal_mrk.append(mrk);
            mrk->attach(this);
            mrk->setLineStyle(QwtPlotMarker::VLine);
            mrk->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
            mrk->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));
            QwtText text(wanttext ? ("\n\n"+calibration.name) : "");
            text.setFont(QFont("Helvetica", 9, QFont::Bold));
            text.setColor(GColor(CPLOTMARKER));
            if (!bydist)
                mrk->setValue(calibration.start / 60.0, 0.0);
            else
                mrk->setValue((context->athlete->useMetricUnits ? 1 : MILES_PER_KM) *
                                rideItem->ride()->timeToDistance(calibration.start), 0.0);
            mrk->setLabel(text);
        }
    }
}

void
AllPlot::refreshReferenceLines()
{
    // not supported in compare mode
    if (context->isCompareIntervals) return;

    // only on power based charts
    if (scope != RideFile::none && scope != RideFile::watts &&
        scope != RideFile::NP && scope != RideFile::aPower && scope != RideFile::xPower) return;

    foreach(QwtPlotCurve *referenceLine, standard->referenceLines) {
        referenceLine->detach();
        delete referenceLine;
    }
    standard->referenceLines.clear();

    if (rideItem && rideItem->ride()) {
        foreach(const RideFilePoint *referencePoint, rideItem->ride()->referencePoints()) {
            QwtPlotCurve *referenceLine = plotReferenceLine(referencePoint);
            if (referenceLine) standard->referenceLines.append(referenceLine);
        }
    }
}

QwtPlotCurve*
AllPlot::plotReferenceLine(const RideFilePoint *referencePoint)
{
    // not supported in compare mode
    if (context->isCompareIntervals) return NULL;

    // only on power based charts
    if (scope != RideFile::none && scope != RideFile::watts &&
        scope != RideFile::NP && scope != RideFile::aPower && scope != RideFile::xPower) return NULL;

    QwtPlotCurve *referenceLine = NULL;

    QVector<double> xaxis;
    QVector<double> yaxis;

    xaxis << axisScaleDiv(QwtPlot::xBottom).lowerBound();
    xaxis << axisScaleDiv(QwtPlot::xBottom).upperBound();

    if (referencePoint->watts != 0)  {
        referenceLine = new QwtPlotCurve(tr("Power Ref"));
        referenceLine->setYAxis(yLeft);
        QPen wattsPen = QPen(GColor(CPOWER));
        wattsPen.setWidth(1);
        wattsPen.setStyle(Qt::DashLine);
        referenceLine->setPen(wattsPen);

        yaxis.append(referencePoint->watts);
        yaxis.append(referencePoint->watts);
    } else if (referencePoint->hr != 0)  {
        referenceLine = new QwtPlotCurve(tr("Heart Rate Ref"));
        referenceLine->setYAxis(yLeft);
        QPen hrPen = QPen(GColor(CHEARTRATE));
        hrPen.setWidth(1);
        hrPen.setStyle(Qt::DashLine);
        referenceLine->setPen(hrPen);

        yaxis.append(referencePoint->hr);
        yaxis.append(referencePoint->hr);
    } else if (referencePoint->cad != 0)  {
        referenceLine = new QwtPlotCurve(tr("Cadence Ref"));
        referenceLine->setYAxis(yLeft);
        QPen cadPen = QPen(GColor(CCADENCE));
        cadPen.setWidth(1);
        cadPen.setStyle(Qt::DashLine);
        referenceLine->setPen(cadPen);

        yaxis.append(referencePoint->cad);
        yaxis.append(referencePoint->cad);
    }

    if (referenceLine) {
        referenceLine->setSamples(xaxis,yaxis);
        referenceLine->attach(this);
        referenceLine->setVisible(true);
    }

    return referenceLine;
}


void
AllPlot::setYMax()
{
    // set axis scales
    if (standard->wCurve->isVisible() && rideItem && rideItem->ride()) {

        setAxisTitle(QwtAxisId(QwtAxis::yRight, 2), tr("W' Balance (j)"));
        setAxisScale(QwtAxisId(QwtAxis::yRight, 2),rideItem->ride()->wprimeData()->minY-1000,rideItem->ride()->wprimeData()->maxY+1000);
        setAxisLabelAlignment(QwtAxisId(QwtAxis::yRight, 2),Qt::AlignVCenter);
    }

    if (standard->wattsCurve->isVisible()) {
        double maxY = (referencePlot == NULL) ? (1.05 * standard->wattsCurve->maxYValue()) :
                                             (1.05 * referencePlot->standard->wattsCurve->maxYValue());

        int axisHeight = qRound( plotLayout()->canvasRect().height() );
        QFontMetrics labelWidthMetric = QFontMetrics( QwtPlot::axisFont(yLeft) );
        int labelWidth = labelWidthMetric.width( (maxY > 1000) ? " 8,888 " : " 888 " );

        int step = 100;
        while( ( qCeil(maxY / step) * labelWidth ) > axisHeight )
        {
            nextStep(step);
        }

        QwtValueList xytick[QwtScaleDiv::NTickTypes];
        for (int i=0;i<maxY && i<2000;i+=step)
            xytick[QwtScaleDiv::MajorTick]<<i;

        setAxisTitle(yLeft, tr("Watts"));
        setAxisScaleDiv(QwtPlot::yLeft,QwtScaleDiv(0.0,maxY,xytick));
        //setAxisLabelAlignment(yLeft,Qt::AlignVCenter);
    }
    if (standard->hrCurve->isVisible() || standard->cadCurve->isVisible() || (!context->athlete->useMetricUnits && standard->tempCurve->isVisible()) || standard->balanceLCurve->isVisible()) {
        double ymin = 0;
        double ymax = 0;

        QStringList labels;
        if (standard->hrCurve->isVisible()) {
            labels << tr("BPM");
            if (referencePlot == NULL)
                ymax = standard->hrCurve->maxYValue();
            else
                ymax = referencePlot->standard->hrCurve->maxYValue();
        }
        if (standard->cadCurve->isVisible()) {
            labels << tr("RPM");
            if (referencePlot == NULL)
                ymax = qMax(ymax, standard->cadCurve->maxYValue());
            else
                ymax = qMax(ymax, referencePlot->standard->cadCurve->maxYValue());
        }
        if (standard->tempCurve->isVisible() && !context->athlete->useMetricUnits) {

            labels << QString::fromUtf8("°F");

            if (referencePlot == NULL) {
                ymin = qMin(ymin, standard->tempCurve->minYValue());
                ymax = qMax(ymax, standard->tempCurve->maxYValue());
            }
            else {
                ymin = qMin(ymin, referencePlot->standard->tempCurve->minYValue());
                ymax = qMax(ymax, referencePlot->standard->tempCurve->maxYValue());
            }
        }
        if (standard->balanceLCurve->isVisible()) {
            labels << tr("% left");
            if (referencePlot == NULL)
                ymax = qMax(ymax, standard->balanceLCurve->maxYValue());
            else
                ymax = qMax(ymax, referencePlot->standard->balanceLCurve->maxYValue());

            standard->balanceLCurve->setBaseline(50);
            standard->balanceRCurve->setBaseline(50);
        }

        int axisHeight = qRound( plotLayout()->canvasRect().height() );
        QFontMetrics labelWidthMetric = QFontMetrics( QwtPlot::axisFont(yLeft) );
        int labelWidth = labelWidthMetric.width( "888 " );

        ymax *= 1.05;
        int step = 10;
        while( ( qCeil(ymax / step) * labelWidth ) > axisHeight )
        {
            nextStep(step);
        }

        QwtValueList xytick[QwtScaleDiv::NTickTypes];
        for (int i=0;i<ymax;i+=step)
            xytick[QwtScaleDiv::MajorTick]<<i;

        setAxisTitle(QwtAxisId(QwtAxis::yLeft, 1), labels.join(" / "));
        setAxisScaleDiv(QwtAxisId(QwtAxis::yLeft, 1),QwtScaleDiv(ymin, ymax, xytick));
        //setAxisLabelAlignment(yLeft2,Qt::AlignVCenter);
    }
    if (standard->speedCurve->isVisible() || (context->athlete->useMetricUnits && standard->tempCurve->isVisible()) || standard->torqueCurve->isVisible()) {
        double ymin = 0;
        double ymax = 0;

        QStringList labels;

        if (standard->speedCurve->isVisible()) {
            labels << (context->athlete->useMetricUnits ? tr("KPH") : tr("MPH"));

            if (referencePlot == NULL)
                ymax = standard->speedCurve->maxYValue();
            else
                ymax = referencePlot->standard->speedCurve->maxYValue();
        }
        if (standard->tempCurve->isVisible() && context->athlete->useMetricUnits) {

            labels << QString::fromUtf8("°C");

            if (referencePlot == NULL) {
                ymin = qMin(ymin, standard->tempCurve->minYValue());
                ymax = qMax(ymax, standard->tempCurve->maxYValue());
            }
            else {
                ymin = qMin(ymin, referencePlot->standard->tempCurve->minYValue());
                ymax = qMax(ymax, referencePlot->standard->tempCurve->maxYValue());
            }
        }
        if (standard->torqueCurve->isVisible()) {
            labels << (context->athlete->useMetricUnits ? tr("Nm") : tr("ftLb"));

            if (referencePlot == NULL)
                ymax = qMax(ymax, standard->torqueCurve->maxYValue());
            else
                ymax = qMax(ymax, referencePlot->standard->torqueCurve->maxYValue());
        }
        setAxisTitle(yRight, labels.join(" / "));
        setAxisScale(yRight, ymin, 1.05 * ymax);
        //setAxisLabelAlignment(yRight,Qt::AlignVCenter);
    }
    if (standard->altCurve->isVisible()) {
        setAxisTitle(QwtAxisId(QwtAxis::yRight, 1), context->athlete->useMetricUnits ? tr("Meters") : tr("Feet"));
        double ymin,ymax;

        if (referencePlot == NULL) {
            ymin = standard->altCurve->minYValue();
            ymax = qMax(ymin + 100, 1.05 * standard->altCurve->maxYValue());
        } else {
            ymin = referencePlot->standard->altCurve->minYValue();
            ymax = qMax(ymin + 100, 1.05 * referencePlot->standard->altCurve->maxYValue());
        }
        ymin = (ymin < 0 ? -100 : 0) + ( qRound(ymin) / 100 ) * 100;

        int axisHeight = qRound( plotLayout()->canvasRect().height() );
        QFontMetrics labelWidthMetric = QFontMetrics( QwtPlot::axisFont(yLeft) );
        int labelWidth = labelWidthMetric.width( (ymax > 1000) ? " 8888 " : " 888 " );

        int step = 10;
        while( ( qCeil( (ymax - ymin ) / step) * labelWidth ) > axisHeight )
        {
            nextStep(step);
        }

        QwtValueList xytick[QwtScaleDiv::NTickTypes];
        for (int i=ymin;i<ymax;i+=step)
            xytick[QwtScaleDiv::MajorTick]<<i;

        //setAxisScale(QwtAxisId(QwtAxis::yRight, 2), ymin, ymax);
        setAxisScaleDiv(QwtAxisId(QwtAxis::yRight, 1),QwtScaleDiv(ymin,ymax,xytick));
        //setAxisLabelAlignment(QwtAxisId(QwtAxis::yRight, 2),Qt::AlignVCenter);
        standard->altCurve->setBaseline(ymin);
    }

    if (wantaxis) {

        setAxisVisible(yLeft, standard->wattsCurve->isVisible() || standard->npCurve->isVisible() || standard->xpCurve->isVisible() || standard->apCurve->isVisible());
        setAxisVisible(QwtAxisId(QwtAxis::yLeft, 1), standard->hrCurve->isVisible() || standard->cadCurve->isVisible());
        setAxisVisible(yRight, standard->speedCurve->isVisible());
        setAxisVisible(QwtAxisId(QwtAxis::yRight, 1), standard->altCurve->isVisible());
        setAxisVisible(QwtAxisId(QwtAxis::yRight, 2), standard->wCurve->isVisible());
        setAxisVisible(xBottom, true);

    } else {

        setAxisVisible(yLeft, false);
        setAxisVisible(QwtAxisId(QwtAxis::yLeft,1), false);
        setAxisVisible(QwtAxisId(QwtAxis::yLeft,2), false);
        setAxisVisible(yRight, false);
        setAxisVisible(QwtAxisId(QwtPlot::yRight,1), false);
        setAxisVisible(QwtAxisId(QwtAxis::yRight,2), false);
        setAxisVisible(QwtAxisId(QwtAxis::yRight,3), false);
        setAxisVisible(xBottom, false);

    }
}

void
AllPlot::setXTitle()
{
    if (bydist)
        setAxisTitle(xBottom, context->athlete->useMetricUnits ? "KM" : "Miles");
    else
        setAxisTitle(xBottom, tr("")); // time is bloody obvious, less noise
    enableAxis(xBottom, true);
    setAxisVisible(xBottom, true);
}

void
AllPlot::setDataFromPlot(AllPlot *plot, int startidx, int stopidx)
{
    if (plot == NULL) {
        rideItem = NULL;
        return;
    }

    referencePlot = plot;

    // You got to give me some data first!
    if (!plot->standard->distanceArray.count() || !plot->standard->timeArray.count()) return;

    // reference the plot for data and state
    rideItem = plot->rideItem;
    bydist = plot->bydist;

    //arrayLength = stopidx-startidx;

    if (bydist) {
        startidx = plot->distanceIndex(plot->standard->distanceArray[startidx]);
        stopidx  = plot->distanceIndex(plot->standard->distanceArray[(stopidx>=plot->standard->distanceArray.size()?plot->standard->distanceArray.size()-1:stopidx)])-1;
    } else {
        startidx = plot->timeIndex(plot->standard->timeArray[startidx]/60);
        stopidx  = plot->timeIndex(plot->standard->timeArray[(stopidx>=plot->standard->timeArray.size()?plot->standard->timeArray.size()-1:stopidx)]/60)-1;
    }

    // center the curve title
    standard->curveTitle.setYValue(30);
    standard->curveTitle.setXValue(2);

    // make sure indexes are still valid
    if (startidx > stopidx || startidx < 0 || stopidx < 0) return;

    double *smoothW = &plot->standard->smoothWatts[startidx];
    double *smoothN = &plot->standard->smoothNP[startidx];
    double *smoothX = &plot->standard->smoothXP[startidx];
    double *smoothL = &plot->standard->smoothAP[startidx];
    double *smoothT = &plot->standard->smoothTime[startidx];
    double *smoothHR = &plot->standard->smoothHr[startidx];
    double *smoothS = &plot->standard->smoothSpeed[startidx];
    double *smoothC = &plot->standard->smoothCad[startidx];
    double *smoothA = &plot->standard->smoothAltitude[startidx];
    double *smoothD = &plot->standard->smoothDistance[startidx];
    double *smoothTE = &plot->standard->smoothTemp[startidx];
    //double *standard->smoothWND = &plot->standard->smoothWind[startidx];
    double *smoothNM = &plot->standard->smoothTorque[startidx];
    double *smoothBALL = &plot->standard->smoothBalanceL[startidx];
    double *smoothBALR = &plot->standard->smoothBalanceR[startidx];

    QwtIntervalSample *smoothRS = &plot->standard->smoothRelSpeed[startidx];

    double *xaxis = bydist ? smoothD : smoothT;

    // attach appropriate curves
    //if (this->legend()) this->legend()->hide();
    if (showW && rideItem->ride()->wprimeData()->TAU > 0) {

        // matches cost
        double burnt=0;
        int count=0;
        foreach(struct Match match, rideItem->ride()->wprimeData()->matches)
            if (match.cost > 2000) { //XXX how to decide the threshold for a match?
                burnt += match.cost;
                count++;
            }

        QString warn;
        if (rideItem->ride()->wprimeData()->minY < 0) {
            warn = QString("Minimum CP=%1").arg(rideItem->ride()->wprimeData()->PCP());
        }

        QwtText text(QString("Tau=%1, CP=%2, W'=%3, %4 matches >2kJ (%5 kJ) %6").arg(rideItem->ride()->wprimeData()->TAU)
                                                    .arg(rideItem->ride()->wprimeData()->CP)
                                                    .arg(rideItem->ride()->wprimeData()->WPRIME)
                                                    .arg(count)
                                                    .arg(burnt/1000.00, 0, 'f', 1)
                                                    .arg(warn));

        text.setFont(QFont("Helvetica", 10, QFont::Bold));
        text.setColor(GColor(CWBAL));
        standard->curveTitle.setLabel(text);
    } else {
        standard->curveTitle.setLabel(QwtText(""));
    }

    standard->wCurve->detach();
    standard->mCurve->detach();
    standard->wattsCurve->detach();
    standard->npCurve->detach();
    standard->xpCurve->detach();
    standard->apCurve->detach();
    standard->hrCurve->detach();
    standard->speedCurve->detach();
    standard->cadCurve->detach();
    standard->altCurve->detach();
    standard->tempCurve->detach();
    standard->windCurve->detach();
    standard->torqueCurve->detach();
    standard->balanceLCurve->detach();
    standard->balanceRCurve->detach();

    standard->wattsCurve->setVisible(rideItem->ride()->areDataPresent()->watts && showPowerState < 2);
    standard->npCurve->setVisible(rideItem->ride()->areDataPresent()->np && showNP);
    standard->xpCurve->setVisible(rideItem->ride()->areDataPresent()->xp && showXP);
    standard->apCurve->setVisible(rideItem->ride()->areDataPresent()->apower && showAP);
    standard->wCurve->setVisible(rideItem->ride()->areDataPresent()->watts && showPowerState<2 && showW);
    standard->mCurve->setVisible(rideItem->ride()->areDataPresent()->watts && showPowerState<2 && showW);
    standard->hrCurve->setVisible(rideItem->ride()->areDataPresent()->hr && showHr);
    standard->speedCurve->setVisible(rideItem->ride()->areDataPresent()->kph && showSpeed);
    standard->cadCurve->setVisible(rideItem->ride()->areDataPresent()->cad && showCad);
    standard->altCurve->setVisible(rideItem->ride()->areDataPresent()->alt && showAlt);
    standard->tempCurve->setVisible(rideItem->ride()->areDataPresent()->temp && showTemp);
    standard->windCurve->setVisible(rideItem->ride()->areDataPresent()->headwind && showWind);
    standard->torqueCurve->setVisible(rideItem->ride()->areDataPresent()->nm && showTorque);
    standard->balanceLCurve->setVisible(rideItem->ride()->areDataPresent()->lrbalance && showBalance);
    standard->balanceRCurve->setVisible(rideItem->ride()->areDataPresent()->lrbalance && showBalance);

    standard->wCurve->setSamples(rideItem->ride()->wprimeData()->xdata().data(),rideItem->ride()->wprimeData()->ydata().data(),rideItem->ride()->wprimeData()->xdata().count());
    standard->mCurve->setSamples(rideItem->ride()->wprimeData()->mxdata().data(),rideItem->ride()->wprimeData()->mydata().data(),rideItem->ride()->wprimeData()->mxdata().count());
    standard->wattsCurve->setSamples(xaxis,smoothW,stopidx-startidx);
    standard->npCurve->setSamples(xaxis,smoothN,stopidx-startidx);
    standard->xpCurve->setSamples(xaxis,smoothX,stopidx-startidx);
    standard->apCurve->setSamples(xaxis,smoothL,stopidx-startidx);
    standard->hrCurve->setSamples(xaxis, smoothHR,stopidx-startidx);
    standard->speedCurve->setSamples(xaxis, smoothS, stopidx-startidx);
    standard->cadCurve->setSamples(xaxis, smoothC, stopidx-startidx);
    standard->altCurve->setSamples(xaxis, smoothA, stopidx-startidx);
    standard->tempCurve->setSamples(xaxis, smoothTE, stopidx-startidx);

    QVector<QwtIntervalSample> tmpWND(stopidx-startidx);
    memcpy(tmpWND.data(), smoothRS, (stopidx-startidx) * sizeof(QwtIntervalSample));
    standard->windCurve->setSamples(new QwtIntervalSeriesData(tmpWND));
    standard->torqueCurve->setSamples(xaxis, smoothNM, stopidx-startidx);
    standard->balanceLCurve->setSamples(xaxis, smoothBALL, stopidx-startidx);
    standard->balanceRCurve->setSamples(xaxis, smoothBALR, stopidx-startidx);

    /*QVector<double> _time(stopidx-startidx);
    qMemCopy( _time.data(), xaxis, (stopidx-startidx) * sizeof( double ) );

    QVector<QwtIntervalSample> tmpWND(stopidx-startidx);
    for (int i=0;i<_time.count();i++) {
        QwtIntervalSample inter = QwtIntervalSample(_time.at(i), 20,50);
        tmpWND.append(inter); // plot->standard->smoothRelSpeed.at(i)
    }*/

    QwtSymbol *sym = new QwtSymbol;
    sym->setPen(QPen(GColor(CPLOTMARKER)));
    if (stopidx-startidx < 150) {
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(3);
    } else {
        sym->setStyle(QwtSymbol::NoSymbol);
        sym->setSize(0);
    }
    standard->wCurve->setSymbol(sym);

    sym = new QwtSymbol;
    sym->setPen(QPen(GColor(CPLOTMARKER)));
    if (stopidx-startidx < 150) {
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(3);
    } else {
        sym->setStyle(QwtSymbol::NoSymbol);
        sym->setSize(0);
    }
    standard->wattsCurve->setSymbol(sym);

    sym = new QwtSymbol;
    sym->setPen(QPen(GColor(CPLOTMARKER)));
    if (stopidx-startidx < 150) {
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(3);
    } else {
        sym->setStyle(QwtSymbol::NoSymbol);
        sym->setSize(0);
    }
    standard->npCurve->setSymbol(sym);

    sym = new QwtSymbol;
    sym->setPen(QPen(GColor(CPLOTMARKER)));
    if (stopidx-startidx < 150) {
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(3);
    } else {
        sym->setStyle(QwtSymbol::NoSymbol);
        sym->setSize(0);
    }
    standard->xpCurve->setSymbol(sym);

    sym = new QwtSymbol;
    sym->setPen(QPen(GColor(CPLOTMARKER)));
    if (stopidx-startidx < 150) {
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(3);
    } else {
        sym->setStyle(QwtSymbol::NoSymbol);
        sym->setSize(0);
    }
    standard->apCurve->setSymbol(sym);

    sym = new QwtSymbol;
    sym->setPen(QPen(GColor(CPLOTMARKER)));
    if (stopidx-startidx < 150) {
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(3);
    } else {
        sym->setStyle(QwtSymbol::NoSymbol);
        sym->setSize(0);
    }
    standard->hrCurve->setSymbol(sym);

    sym = new QwtSymbol;
    sym->setPen(QPen(GColor(CPLOTMARKER)));
    if (stopidx-startidx < 150) {
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(3);
    } else {
        sym->setStyle(QwtSymbol::NoSymbol);
        sym->setSize(0);
    }
    standard->speedCurve->setSymbol(sym);

    sym = new QwtSymbol;
    sym->setPen(QPen(GColor(CPLOTMARKER)));
    if (stopidx-startidx < 150) {
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(3);
    } else {
        sym->setStyle(QwtSymbol::NoSymbol);
        sym->setSize(0);
    }
    standard->cadCurve->setSymbol(sym);

    sym = new QwtSymbol;
    sym->setPen(QPen(GColor(CPLOTMARKER)));
    if (stopidx-startidx < 150) {
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(3);
    } else {
        sym->setStyle(QwtSymbol::NoSymbol);
        sym->setSize(0);
    }
    standard->altCurve->setSymbol(sym);

    sym = new QwtSymbol;
    sym->setPen(QPen(GColor(CPLOTMARKER)));
    if (stopidx-startidx < 150) {
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(3);
    } else {
        sym->setStyle(QwtSymbol::NoSymbol);
        sym->setSize(0);
    }
    standard->tempCurve->setSymbol(sym);

    sym = new QwtSymbol;
    sym->setPen(QPen(GColor(CPLOTMARKER)));
    if (stopidx-startidx < 150) {
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(3);
    } else {
        sym->setStyle(QwtSymbol::NoSymbol);
        sym->setSize(0);
    }
    standard->torqueCurve->setSymbol(sym);

    sym = new QwtSymbol;
    sym->setPen(QPen(GColor(CPLOTMARKER)));
    if (stopidx-startidx < 150) {
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(3);
    } else {
        sym->setStyle(QwtSymbol::NoSymbol);
        sym->setSize(0);
    }
    standard->balanceLCurve->setSymbol(sym);

    sym = new QwtSymbol;
    sym->setPen(QPen(GColor(CPLOTMARKER)));
    if (stopidx-startidx < 150) {
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(3);
    } else {
        sym->setStyle(QwtSymbol::NoSymbol);
        sym->setSize(0);
    }
    standard->balanceRCurve->setSymbol(sym);

    setYMax();

    setAxisScale(xBottom, xaxis[0], xaxis[stopidx-startidx-1]);
    enableAxis(xBottom, true);
    setAxisVisible(xBottom, true);

    if (!plot->standard->smoothAltitude.empty()) {
        standard->altCurve->attach(this);
        standard->intervalHighlighterCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 1));
    }
    if (rideItem->ride()->wprimeData()->xdata().count()) {
        standard->wCurve->attach(this);
        standard->mCurve->attach(this);
    }
    if (!plot->standard->smoothWatts.empty()) {
        standard->wattsCurve->attach(this);
        standard->intervalHighlighterCurve->setYAxis(yLeft);
    }
    if (!plot->standard->smoothNP.empty()) {
        standard->npCurve->attach(this);
        standard->intervalHighlighterCurve->setYAxis(yLeft);
    }
    if (!plot->standard->smoothXP.empty()) {
        standard->xpCurve->attach(this);
        standard->intervalHighlighterCurve->setYAxis(yLeft);
    }
    if (!plot->standard->smoothAP.empty()) {
        standard->apCurve->attach(this);
        standard->intervalHighlighterCurve->setYAxis(yLeft);
    }
    if (!plot->standard->smoothHr.empty()) {
        standard->hrCurve->attach(this);
        standard->intervalHighlighterCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));
    }
    if (!plot->standard->smoothSpeed.empty()) {
        standard->speedCurve->attach(this);
        standard->intervalHighlighterCurve->setYAxis(yRight);
    }
    if (!plot->standard->smoothCad.empty()) {
        standard->cadCurve->attach(this);
        standard->intervalHighlighterCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));
    }
    if (!plot->standard->smoothTemp.empty()) {
        standard->tempCurve->attach(this);
        standard->intervalHighlighterCurve->setYAxis(yRight);
    }
    if (!plot->standard->smoothWind.empty()) {
        standard->windCurve->attach(this);
        standard->intervalHighlighterCurve->setYAxis(yRight);
    }
    if (!plot->standard->smoothTorque.empty()) {
        standard->torqueCurve->attach(this);
        standard->intervalHighlighterCurve->setYAxis(yRight);
    }
    if (!plot->standard->smoothBalanceL.empty()) {
        standard->balanceLCurve->attach(this);
        standard->balanceRCurve->attach(this);
        standard->intervalHighlighterCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));
    }


    refreshReferenceLines();
    refreshIntervalMarkers();
    refreshCalibrationMarkers();
    refreshZoneLabels();

    //if (this->legend()) this->legend()->show();
    //replot();
}

void
AllPlot::setDataFromPlot(AllPlot *plot)
{
    if (plot == NULL) {
        rideItem = NULL;
        return;
    }

    referencePlot = plot;

    // reference the plot for data and state
    rideItem = plot->rideItem;
    bydist = plot->bydist;

    // remove all curves from the plot
    standard->wCurve->detach();
    standard->mCurve->detach();
    standard->wattsCurve->detach();
    standard->npCurve->detach();
    standard->xpCurve->detach();
    standard->apCurve->detach();
    standard->hrCurve->detach();
    standard->speedCurve->detach();
    standard->cadCurve->detach();
    standard->altCurve->detach();
    standard->tempCurve->detach();
    standard->windCurve->detach();
    standard->torqueCurve->detach();
    standard->balanceLCurve->detach();
    standard->balanceRCurve->detach();

    standard->wCurve->setVisible(false);
    standard->mCurve->setVisible(false);
    standard->wattsCurve->setVisible(false);
    standard->npCurve->setVisible(false);
    standard->xpCurve->setVisible(false);
    standard->apCurve->setVisible(false);
    standard->hrCurve->setVisible(false);
    standard->speedCurve->setVisible(false);
    standard->cadCurve->setVisible(false);
    standard->altCurve->setVisible(false);
    standard->tempCurve->setVisible(false);
    standard->windCurve->setVisible(false);
    standard->torqueCurve->setVisible(false);
    standard->balanceLCurve->setVisible(false);
    standard->balanceRCurve->setVisible(false);

    QwtPlotCurve *ourCurve = NULL, *thereCurve = NULL;
    QwtPlotCurve *ourCurve2 = NULL, *thereCurve2 = NULL;
    QwtPlotIntervalCurve *ourICurve = NULL, *thereICurve = NULL;
    QString title;

    // which curve are we interested in ?
    switch (scope) {

    case RideFile::cad:
        {
        ourCurve = standard->cadCurve;
        thereCurve = referencePlot->standard->cadCurve;
        title = tr("Cadence");
        }
        break;

    case RideFile::hr:
        {
        ourCurve = standard->hrCurve;
        thereCurve = referencePlot->standard->hrCurve;
        title = tr("Heartrate");
        }
        break;

    case RideFile::kph:
        {
        ourCurve = standard->speedCurve;
        thereCurve = referencePlot->standard->speedCurve;
        if (secondaryScope == RideFile::headwind) {
            ourICurve = standard->windCurve;
            thereICurve = referencePlot->standard->windCurve;
        }
        title = tr("Speed");
        }
        break;

    case RideFile::nm:
        {
        ourCurve = standard->torqueCurve;
        thereCurve = referencePlot->standard->torqueCurve;
        title = tr("Torque");
        }
        break;

    case RideFile::watts:
        {
        ourCurve = standard->wattsCurve;
        thereCurve = referencePlot->standard->wattsCurve;
        title = tr("Power");
        }
        break;

    case RideFile::wprime:
        {
        ourCurve = standard->wCurve;
        ourCurve2 = standard->mCurve;
        thereCurve = referencePlot->standard->wCurve;
        thereCurve2 = referencePlot->standard->mCurve;
        title = tr("W'bal");
        }
        break;

    case RideFile::alt:
        {
        ourCurve = standard->altCurve;
        thereCurve = referencePlot->standard->altCurve;
        title = tr("Altitude");
        }
        break;

    case RideFile::headwind:
        {
        ourICurve = standard->windCurve;
        thereICurve = referencePlot->standard->windCurve;
        title = tr("Headwind");
        }
        break;

    case RideFile::temp:
        {
        ourCurve = standard->tempCurve;
        thereCurve = referencePlot->standard->tempCurve;
        title = tr("Temperature");
        }
        break;

    case RideFile::NP:
        {
        ourCurve = standard->npCurve;
        thereCurve = referencePlot->standard->npCurve;
        title = tr("NP");
        }
        break;

    case RideFile::xPower:
        {
        ourCurve = standard->xpCurve;
        thereCurve = referencePlot->standard->xpCurve;
        title = tr("xPower");
        }
        break;

    case RideFile::lrbalance:
        {
        ourCurve = standard->balanceLCurve;
        ourCurve2 = standard->balanceRCurve;
        thereCurve = referencePlot->standard->balanceLCurve;
        thereCurve2 = referencePlot->standard->balanceRCurve;
        title = tr("L/R Balance");
        }
        break;

    case RideFile::aPower:
        {
        ourCurve = standard->apCurve;
        thereCurve = referencePlot->standard->apCurve;
        title = tr("aPower");
        }
        break;

    default:
    case RideFile::interval:
    case RideFile::slope:
    case RideFile::vam:
    case RideFile::wattsKg:
    case RideFile::km:
    case RideFile::lon:
    case RideFile::lat:
    case RideFile::none:
        break;
    }

    // lets clone !
    if ((ourCurve && thereCurve) || (ourICurve && thereICurve)) {

        if (ourCurve && thereCurve) {
            // no way to get values, so we run through them
            ourCurve->setVisible(true);
            ourCurve->attach(this);

            // lets clone the data
            QVector<QPointF> array;
            for (size_t i=0; i<thereCurve->data()->size(); i++) array << thereCurve->data()->sample(i);

            ourCurve->setSamples(array);
            ourCurve->setYAxis(yLeft);
            ourCurve->setBaseline(thereCurve->baseline());

            // symbol when zoomed in super close
            if (array.size() < 150) {
                QwtSymbol *sym = new QwtSymbol;
                sym->setPen(QPen(GColor(CPLOTMARKER)));
                sym->setStyle(QwtSymbol::Ellipse);
                sym->setSize(3);
                ourCurve->setSymbol(sym);
            } else {
                QwtSymbol *sym = new QwtSymbol;
                sym->setStyle(QwtSymbol::NoSymbol);
                sym->setSize(0);
                ourCurve->setSymbol(sym);
            }
        }

        if (ourCurve2 && thereCurve2) {

            ourCurve2->setVisible(true);
            ourCurve2->attach(this);

            // lets clone the data
            QVector<QPointF> array;
            for (size_t i=0; i<thereCurve2->data()->size(); i++) array << thereCurve2->data()->sample(i);

            ourCurve2->setSamples(array);
            ourCurve2->setYAxis(yLeft);
            ourCurve2->setBaseline(thereCurve2->baseline());

            // symbol when zoomed in super close
            if (array.size() < 150) {
                QwtSymbol *sym = new QwtSymbol;
                sym->setPen(QPen(GColor(CPLOTMARKER)));
                sym->setStyle(QwtSymbol::Ellipse);
                sym->setSize(3);
                ourCurve2->setSymbol(sym);
            } else {
                QwtSymbol *sym = new QwtSymbol;
                sym->setStyle(QwtSymbol::NoSymbol);
                sym->setSize(0);
                ourCurve2->setSymbol(sym);
            }
        }

        if (ourICurve && thereICurve) {

            ourICurve->setVisible(true);
            ourICurve->attach(this);

            // lets clone the data
            QVector<QwtIntervalSample> array;
            for (size_t i=0; i<thereICurve->data()->size(); i++) array << thereICurve->data()->sample(i);

            ourICurve->setSamples(array);
            ourICurve->setYAxis(yLeft);
        }

        // x-axis
        if (thereCurve)
            setAxisScale(QwtPlot::xBottom, thereCurve->minXValue(), thereCurve->maxXValue());
        else if (thereICurve)
            setAxisScale(QwtPlot::xBottom, thereICurve->boundingRect().left(), thereICurve->boundingRect().right());

        enableAxis(QwtPlot::xBottom, true);
        setAxisVisible(QwtPlot::xBottom, true);
        setXTitle();

        // y-axis yLeft
        setAxisVisible(yLeft, true);
        if (thereCurve)
            setAxisScale(QwtPlot::yLeft, thereCurve->minYValue(), 1.1f * thereCurve->maxYValue());
        if (thereICurve)
            setAxisScale(QwtPlot::yLeft, thereICurve->boundingRect().top(), 1.1f * thereICurve->boundingRect().bottom());


        QwtScaleDraw *sd = new QwtScaleDraw;
        sd->setTickLength(QwtScaleDiv::MajorTick, 3);
        sd->enableComponent(QwtScaleDraw::Ticks, false);
        sd->enableComponent(QwtScaleDraw::Backbone, false);
        setAxisScaleDraw(QwtPlot::yLeft, sd);

        // title and colour
        setAxisTitle(yLeft, title);
        QPalette pal;
        if (thereCurve) {
            pal.setColor(QPalette::WindowText, thereCurve->pen().color());
            pal.setColor(QPalette::Text, thereCurve->pen().color());
        } else if (thereICurve) {
            pal.setColor(QPalette::WindowText, thereICurve->pen().color());
            pal.setColor(QPalette::Text, thereICurve->pen().color());
        }
        axisWidget(QwtPlot::yLeft)->setPalette(pal);

        // hide other y axes
        setAxisVisible(QwtAxisId(QwtAxis::yLeft, 1), false);
        setAxisVisible(yRight, false);
        setAxisVisible(QwtAxisId(QwtAxis::yRight, 1), false);
        setAxisVisible(QwtAxisId(QwtAxis::yRight, 2), false);

        // plot standard->grid
        standard->grid->setVisible(referencePlot->standard->grid->isVisible());

        // plot markers etc
        refreshIntervalMarkers();
        refreshCalibrationMarkers();
        refreshReferenceLines();

        // always draw against yLeft in series mode
        standard->intervalHighlighterCurve->setYAxis(yLeft);
        if (thereCurve)
            standard->intervalHighlighterCurve->setBaseline(thereCurve->minYValue());
        else if (thereICurve)
            standard->intervalHighlighterCurve->setBaseline(thereICurve->boundingRect().top());
#if 0
        refreshZoneLabels();
#endif
    }
}

void
AllPlot::setDataFromPlots(QList<AllPlot *> plots)
{
    // remove all curves from the plot
    standard->wCurve->detach();
    standard->mCurve->detach();
    standard->wattsCurve->detach();
    standard->npCurve->detach();
    standard->xpCurve->detach();
    standard->apCurve->detach();
    standard->hrCurve->detach();
    standard->speedCurve->detach();
    standard->cadCurve->detach();
    standard->altCurve->detach();
    standard->tempCurve->detach();
    standard->windCurve->detach();
    standard->torqueCurve->detach();
    standard->balanceLCurve->detach();
    standard->balanceRCurve->detach();

    standard->wCurve->setVisible(false);
    standard->mCurve->setVisible(false);
    standard->wattsCurve->setVisible(false);
    standard->npCurve->setVisible(false);
    standard->xpCurve->setVisible(false);
    standard->apCurve->setVisible(false);
    standard->hrCurve->setVisible(false);
    standard->speedCurve->setVisible(false);
    standard->cadCurve->setVisible(false);
    standard->altCurve->setVisible(false);
    standard->tempCurve->setVisible(false);
    standard->windCurve->setVisible(false);
    standard->torqueCurve->setVisible(false);
    standard->balanceLCurve->setVisible(false);
    standard->balanceRCurve->setVisible(false);

    // clear previous curves
    foreach(QwtPlotCurve *prior, compares) {
        prior->detach();
        delete prior;
    }
    compares.clear();

    double MAXY = -100;

    // add all the curves
    int index=0;
    foreach (AllPlot *plot, plots) {

        if (context->compareIntervals[index].isChecked() == false) {
            index++;
            continue; // ignore if not shown
        }

        referencePlot = plot;

        QwtPlotCurve *ourCurve = NULL, *thereCurve = NULL;
        QwtPlotCurve *ourCurve2 = NULL, *thereCurve2 = NULL;
        QwtPlotIntervalCurve *ourICurve = NULL, *thereICurve = NULL;
        QString title;

        // which curve are we interested in ?
        switch (scope) {

            case RideFile::cad:
                {
                ourCurve = new QwtPlotCurve(tr("Cadence"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->cadCurve;
                title = tr("Cadence");
                }
                break;

            case RideFile::hr:
                {
                ourCurve = new QwtPlotCurve(tr("Heart Rate"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->hrCurve;
                title = tr("Heartrate");
                }
                break;

            case RideFile::kph:
                {
                ourCurve = new QwtPlotCurve(tr("Speed"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->speedCurve;
                if (secondaryScope == RideFile::headwind) {
                    ourICurve = standard->windCurve;
                    thereICurve = referencePlot->standard->windCurve;
                }
                title = tr("Speed");
                }
                break;

            case RideFile::nm:
                {
                ourCurve = new QwtPlotCurve(tr("Torque"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->torqueCurve;
                title = tr("Torque");
                }
                break;

            case RideFile::watts:
                {
                ourCurve = new QwtPlotCurve(tr("Power"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->wattsCurve;
                title = tr("Power");
                }
                break;

            case RideFile::wprime:
                {
                ourCurve = new QwtPlotCurve(tr("W' Balance (j)"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                ourCurve2 = new QwtPlotCurve(tr("Matches"));
                ourCurve2->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                ourCurve2->setStyle(QwtPlotCurve::Dots);
                ourCurve2->setYAxis(QwtAxisId(QwtAxis::yRight, 2));

                thereCurve = referencePlot->standard->wCurve;
                thereCurve2 = referencePlot->standard->mCurve;
                title = tr("W'bal");
                }
                break;

            case RideFile::alt:
                {
                ourCurve = new QwtPlotCurve(tr("Altitude"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                ourCurve->setZ(-10); // always at the back.
                thereCurve = referencePlot->standard->altCurve;
                title = tr("Altitude");
                }
                break;

            case RideFile::headwind:
                {
                ourICurve = new QwtPlotIntervalCurve(tr("Headwind"));
                thereICurve = referencePlot->standard->windCurve;
                title = tr("Headwind");
                }
                break;

            case RideFile::temp:
                {
                ourCurve = new QwtPlotCurve(tr("Temperature"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->tempCurve;
                title = tr("Temperature");
                }
                break;

            case RideFile::NP:
                {
                ourCurve = new QwtPlotCurve(tr("NP"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->npCurve;
                title = tr("NP");
                }
                break;

            case RideFile::xPower:
                {
                ourCurve = new QwtPlotCurve(tr("xPower"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->xpCurve;
                title = tr("xPower");
                }
                break;

            case RideFile::lrbalance:
                {
                ourCurve = new QwtPlotCurve(tr("Left Balance"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                ourCurve2 = new QwtPlotCurve(tr("Right Balance"));
                ourCurve2->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->balanceLCurve;
                thereCurve2 = referencePlot->standard->balanceRCurve;
                title = tr("L/R Balance");
                }
                break;

            case RideFile::aPower:
                {
                ourCurve = new QwtPlotCurve(tr("aPower"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->apCurve;
                title = tr("aPower");
                }
                break;

            default:
            case RideFile::interval:
            case RideFile::slope:
            case RideFile::vam:
            case RideFile::wattsKg:
            case RideFile::km:
            case RideFile::lon:
            case RideFile::lat:
            case RideFile::none:
                break;
            }

            bool antialias = appsettings->value(this, GC_ANTIALIAS, false).toBool();

            // lets clone !
            if ((ourCurve && thereCurve) || (ourICurve && thereICurve)) {

                if (ourCurve && thereCurve) {

                    // remember for next time...
                    compares << ourCurve;

                    // colours etc
                    if (antialias) ourCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
                    QPen pen = thereCurve->pen();
                    pen.setColor(context->compareIntervals[index].color);
                    ourCurve->setPen(pen);
                    ourCurve->setVisible(true);
                    ourCurve->attach(this);

                    // lets clone the data
                    QVector<QPointF> array;
                    for (size_t i=0; i<thereCurve->data()->size(); i++) array << thereCurve->data()->sample(i);

                    ourCurve->setSamples(array);
                    ourCurve->setYAxis(yLeft);
                    ourCurve->setBaseline(thereCurve->baseline());

                    if (ourCurve->maxYValue() > MAXY) MAXY = ourCurve->maxYValue();

                    // symbol when zoomed in super close
                    if (array.size() < 150) {
                        QwtSymbol *sym = new QwtSymbol;
                        sym->setPen(QPen(GColor(CPLOTMARKER)));
                        sym->setStyle(QwtSymbol::Ellipse);
                        sym->setSize(3);
                        ourCurve->setSymbol(sym);
                    } else {
                        QwtSymbol *sym = new QwtSymbol;
                        sym->setStyle(QwtSymbol::NoSymbol);
                        sym->setSize(0);
                        ourCurve->setSymbol(sym);
                    }
                }

                if (ourCurve2 && thereCurve2) {

                    // remember for next time...
                    compares << ourCurve2;

                    ourCurve2->setVisible(true);
                    ourCurve2->attach(this);
                    if (antialias) ourCurve2->setRenderHint(QwtPlotItem::RenderAntialiased);
                    QPen pen = thereCurve2->pen();
                    pen.setColor(context->compareIntervals[index].color);
                    ourCurve2->setPen(pen);

                    // lets clone the data
                    QVector<QPointF> array;
                    for (size_t i=0; i<thereCurve2->data()->size(); i++) array << thereCurve2->data()->sample(i);

                    ourCurve2->setSamples(array);
                    ourCurve2->setYAxis(yLeft);
                    ourCurve2->setBaseline(thereCurve2->baseline());

                    if (ourCurve2->maxYValue() > MAXY) MAXY = ourCurve2->maxYValue();

                    // symbol when zoomed in super close
                    if (array.size() < 150) {
                        QwtSymbol *sym = new QwtSymbol;
                        sym->setPen(QPen(GColor(CPLOTMARKER)));
                        sym->setStyle(QwtSymbol::Ellipse);
                        sym->setSize(3);
                        ourCurve2->setSymbol(sym);
                    } else {
                        QwtSymbol *sym = new QwtSymbol;
                        sym->setStyle(QwtSymbol::NoSymbol);
                        sym->setSize(0);
                        ourCurve2->setSymbol(sym);
                    }
                }

                if (ourICurve && thereICurve) {

                    ourICurve->setVisible(true);
                    ourICurve->attach(this);
                    QPen pen = thereICurve->pen();
                    pen.setColor(context->compareIntervals[index].color);
                    ourICurve->setPen(pen);
                    if (antialias) ourICurve->setRenderHint(QwtPlotItem::RenderAntialiased);

                    // lets clone the data
                    QVector<QwtIntervalSample> array;
                    for (size_t i=0; i<thereICurve->data()->size(); i++) array << thereICurve->data()->sample(i);

                    ourICurve->setSamples(array);
                    ourICurve->setYAxis(yLeft);

                    //XXXX ???? DUNNO ?????
                    //XXXX FIX LATER XXXX if (ourICurve->maxYValue() > MAXY) MAXY = ourICurve->maxYValue();
                }

        }

        // move on -- this is used to reference into the compareIntervals
        //            array to get the colors predominantly!
        index++;
    }

    // x-axis
    enableAxis(QwtPlot::xBottom, true);
    setAxisVisible(QwtPlot::xBottom, true);
    setAxisVisible(yLeft, true);

    // prettify the chart at the end
    QwtScaleDraw *sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    setAxisScaleDraw(QwtPlot::yLeft, sd);

    // set the y-axis for largest value we saw +10%
    setAxisScale(QwtPlot::yLeft, 0, MAXY * 1.10f);

    // hide other y axes
    setAxisVisible(QwtAxisId(QwtAxis::yLeft, 1), false);
    setAxisVisible(yRight, false);
    setAxisVisible(QwtAxisId(QwtAxis::yRight, 1), false);
    setAxisVisible(QwtAxisId(QwtAxis::yRight, 2), false);

    // refresh zone background (if needed)
    if (shade_zones) {
        bg->attach(this);
        refreshZoneLabels();
    } else
        bg->detach();
#if 0

    // plot standard->grid
    standard->grid->setVisible(referencePlot->standard->grid->isVisible());

    // plot markers etc
    refreshIntervalMarkers();
    refreshCalibrationMarkers();
    refreshReferenceLines();

    // always draw against yLeft in series mode
    intervalHighlighterCurve->setYAxis(yLeft);
    if (thereCurve)
        intervalHighlighterCurve->setBaseline(thereCurve->minYValue());
    else if (thereICurve)
        intervalHighlighterCurve->setBaseline(thereICurve->boundingRect().top());
#if 0
#endif
#endif
}

// used to setup array of allplots where there is one for
// each interval in compare mode
void 
AllPlot::setDataFromObject(AllPlotObject *object, AllPlot *reference)
{
    referencePlot = reference;
    bydist = reference->bydist;

    // remove all curves from the plot
    standard->wCurve->detach();
    standard->mCurve->detach();
    standard->wattsCurve->detach();
    standard->npCurve->detach();
    standard->xpCurve->detach();
    standard->apCurve->detach();
    standard->hrCurve->detach();
    standard->speedCurve->detach();
    standard->cadCurve->detach();
    standard->altCurve->detach();
    standard->tempCurve->detach();
    standard->windCurve->detach();
    standard->torqueCurve->detach();
    standard->balanceLCurve->detach();
    standard->balanceRCurve->detach();
    standard->intervalHighlighterCurve->detach();

    standard->wCurve->setVisible(false);
    standard->mCurve->setVisible(false);
    standard->wattsCurve->setVisible(false);
    standard->npCurve->setVisible(false);
    standard->xpCurve->setVisible(false);
    standard->apCurve->setVisible(false);
    standard->hrCurve->setVisible(false);
    standard->speedCurve->setVisible(false);
    standard->cadCurve->setVisible(false);
    standard->altCurve->setVisible(false);
    standard->tempCurve->setVisible(false);
    standard->windCurve->setVisible(false);
    standard->torqueCurve->setVisible(false);
    standard->balanceLCurve->setVisible(false);
    standard->balanceRCurve->setVisible(false);
    standard->intervalHighlighterCurve->setVisible(false);

    // NOW SET OUR CURVES USING THEIR DATA ...
    QVector<double> &xaxis = referencePlot->bydist ? object->smoothDistance : object->smoothTime;
    int totalPoints = xaxis.count();

    //W' curve set to whatever data we have
    //object->wCurve->setSamples(rideItem->ride()->wprimeData()->xdata().data(), rideItem->ride()->wprimeData()->ydata().data(), rideItem->ride()->wprimeData()->xdata().count());
    //object->mCurve->setSamples(rideItem->ride()->wprimeData()->mxdata().data(), rideItem->ride()->wprimeData()->mydata().data(), rideItem->ride()->wprimeData()->mxdata().count());

    if (!object->wattsArray.empty()) {
        standard->wattsCurve->setSamples(xaxis.data(), object->smoothWatts.data(), totalPoints);
        standard->wattsCurve->attach(this);
        standard->wattsCurve->setVisible(true);
    }

    if (!object->npArray.empty()) {
        standard->npCurve->setSamples(xaxis.data(), object->smoothNP.data(), totalPoints);
        standard->npCurve->attach(this);
        standard->npCurve->setVisible(true);
    }

    if (!object->xpArray.empty()) {
        standard->xpCurve->setSamples(xaxis.data(), object->smoothXP.data(), totalPoints);
        standard->xpCurve->attach(this);
        standard->xpCurve->setVisible(true);
    }

    if (!object->apArray.empty()) {
        standard->apCurve->setSamples(xaxis.data(), object->smoothAP.data(), totalPoints);
        standard->apCurve->attach(this);
        standard->apCurve->setVisible(true);
    }

    if (!object->hrArray.empty()) {
        standard->hrCurve->setSamples(xaxis.data(), object->smoothHr.data(), totalPoints);
        standard->hrCurve->attach(this);
        standard->hrCurve->setVisible(true);
    }

    if (!object->speedArray.empty()) {
        standard->speedCurve->setSamples(xaxis.data(), object->smoothSpeed.data(), totalPoints);
        standard->speedCurve->attach(this);
        standard->speedCurve->setVisible(true);
    }

    if (!object->cadArray.empty()) {
        standard->cadCurve->setSamples(xaxis.data(), object->smoothCad.data(), totalPoints);
        standard->cadCurve->attach(this);
        standard->cadCurve->setVisible(true);
    }

    if (!object->altArray.empty()) {
        standard->altCurve->setSamples(xaxis.data(), object->smoothAltitude.data(), totalPoints);
        standard->altCurve->attach(this);
        standard->altCurve->setVisible(true);
    }

    if (!object->tempArray.empty()) {
        standard->tempCurve->setSamples(xaxis.data(), object->smoothTemp.data(), totalPoints);
        standard->tempCurve->attach(this);
        standard->tempCurve->setVisible(true);
    }


    if (!object->windArray.empty()) {
        standard->windCurve->setSamples(new QwtIntervalSeriesData(object->smoothRelSpeed));
        standard->windCurve->attach(this);
        standard->windCurve->setVisible(true);
    }

    if (!object->torqueArray.empty()) {
        standard->torqueCurve->setSamples(xaxis.data(), object->smoothTorque.data(), totalPoints);
        standard->torqueCurve->attach(this);
        standard->torqueCurve->setVisible(true);
    }

    if (!object->balanceArray.empty()) {
        standard->balanceLCurve->setSamples(xaxis.data(), object->smoothBalanceL.data(), totalPoints);
        standard->balanceRCurve->setSamples(xaxis.data(), object->smoothBalanceR.data(), totalPoints);
        standard->balanceLCurve->attach(this);
        standard->balanceLCurve->setVisible(true);
        standard->balanceRCurve->attach(this);
        standard->balanceRCurve->setVisible(true);
    }

    // to the max / min
    standard->grid->detach();

    // honour user preferences
    standard->wCurve->setVisible(referencePlot->showPowerState < 2 && referencePlot->showW);
    standard->mCurve->setVisible(referencePlot->showPowerState < 2 && referencePlot->showW);
    standard->wattsCurve->setVisible(referencePlot->showPowerState < 2);
    standard->npCurve->setVisible(referencePlot->showNP);
    standard->xpCurve->setVisible(referencePlot->showXP);
    standard->apCurve->setVisible(referencePlot->showAP);
    standard->hrCurve->setVisible(referencePlot->showHr);
    standard->speedCurve->setVisible(referencePlot->showSpeed);
    standard->cadCurve->setVisible(referencePlot->showCad);
    standard->altCurve->setVisible(referencePlot->showAlt);
    standard->tempCurve->setVisible(referencePlot->showTemp);
    standard->windCurve->setVisible(referencePlot->showWind);
    standard->torqueCurve->setVisible(referencePlot->showWind);
    standard->balanceLCurve->setVisible(referencePlot->showBalance);
    standard->balanceRCurve->setVisible(referencePlot->showBalance);

    // set xaxis -- but not min/max as we get called during smoothing
    //              and massively quicker to reuse data and replot
    setXTitle();
    enableAxis(xBottom, true);
    setAxisVisible(xBottom, true);

    // set the y-axis scales now
    referencePlot = NULL;
    setYMax();

    // refresh zone background (if needed)
    if (shade_zones) {
        bg->attach(this);
        refreshZoneLabels();
    } else
        bg->detach();

    replot();
}

void
AllPlot::setDataFromRide(RideItem *_rideItem)
{
    rideItem = _rideItem;
    if (_rideItem == NULL) return;

    // we don't have a reference plot
    referencePlot = NULL;

    // bsically clear out
    standard->wattsArray.clear();
    standard->curveTitle.setLabel(QwtText(QString(""), QwtText::PlainText)); // default to no title

    setDataFromRideFile(rideItem->ride(), standard);
}

void
AllPlot::setDataFromRideFile(RideFile *ride, AllPlotObject *here)
{
    if (ride && ride->dataPoints().size()) {
        const RideFileDataPresent *dataPresent = ride->areDataPresent();
        int npoints = ride->dataPoints().size();
        here->wattsArray.resize(dataPresent->watts ? npoints : 0);
        here->npArray.resize(dataPresent->np ? npoints : 0);
        here->xpArray.resize(dataPresent->xp ? npoints : 0);
        here->apArray.resize(dataPresent->apower ? npoints : 0);
        here->hrArray.resize(dataPresent->hr ? npoints : 0);
        here->speedArray.resize(dataPresent->kph ? npoints : 0);
        here->cadArray.resize(dataPresent->cad ? npoints : 0);
        here->altArray.resize(dataPresent->alt ? npoints : 0);
        here->tempArray.resize(dataPresent->temp ? npoints : 0);
        here->windArray.resize(dataPresent->headwind ? npoints : 0);
        here->torqueArray.resize(dataPresent->nm ? npoints : 0);
        here->balanceArray.resize(dataPresent->lrbalance ? npoints : 0);
        here->timeArray.resize(npoints);
        here->distanceArray.resize(npoints);

        // attach appropriate curves
        here->wCurve->detach();
        here->mCurve->detach();
        here->wattsCurve->detach();
        here->npCurve->detach();
        here->xpCurve->detach();
        here->apCurve->detach();
        here->hrCurve->detach();
        here->speedCurve->detach();
        here->cadCurve->detach();
        here->altCurve->detach();
        here->tempCurve->detach();
        here->windCurve->detach();
        here->torqueCurve->detach();
        here->balanceLCurve->detach();
        here->balanceRCurve->detach();

        if (!here->altArray.empty()) here->altCurve->attach(this);
        if (!here->wattsArray.empty()) here->wattsCurve->attach(this);
        if (!here->npArray.empty()) here->npCurve->attach(this);
        if (!here->xpArray.empty()) here->xpCurve->attach(this);
        if (!here->apArray.empty()) here->apCurve->attach(this);
        if (ride && !ride->wprimeData()->ydata().empty()) {
            here->wCurve->attach(this);
            here->mCurve->attach(this);
        }
        if (!here->hrArray.empty()) here->hrCurve->attach(this);
        if (!here->speedArray.empty()) here->speedCurve->attach(this);
        if (!here->cadArray.empty()) here->cadCurve->attach(this);
        if (!here->tempArray.empty()) here->tempCurve->attach(this);
        if (!here->windArray.empty()) here->windCurve->attach(this);
        if (!here->torqueArray.empty()) here->torqueCurve->attach(this);
        if (!here->balanceArray.empty()) {
            here->balanceLCurve->attach(this);
            here->balanceRCurve->attach(this);
        }

        here->wCurve->setVisible(dataPresent->watts && showPowerState < 2 && showW);
        here->mCurve->setVisible(dataPresent->watts && showPowerState < 2 && showW);
        here->wattsCurve->setVisible(dataPresent->watts && showPowerState < 2);
        here->npCurve->setVisible(dataPresent->np && showNP);
        here->xpCurve->setVisible(dataPresent->xp && showXP);
        here->apCurve->setVisible(dataPresent->apower && showAP);
        here->hrCurve->setVisible(dataPresent->hr && showHr);
        here->speedCurve->setVisible(dataPresent->kph && showSpeed);
        here->cadCurve->setVisible(dataPresent->cad && showCad);
        here->altCurve->setVisible(dataPresent->alt && showAlt);
        here->tempCurve->setVisible(dataPresent->temp && showTemp);
        here->windCurve->setVisible(dataPresent->headwind && showWind);
        here->torqueCurve->setVisible(dataPresent->nm && showWind);
        here->balanceLCurve->setVisible(dataPresent->lrbalance && showBalance);
        here->balanceRCurve->setVisible(dataPresent->lrbalance && showBalance);

        int arrayLength = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {

            // we round the time to nearest 100th of a second
            // before adding to the array, to avoid situation
            // where 'high precision' time slice is an artefact
            // of double precision or slight timing anomalies
            // e.g. where realtime gives timestamps like
            // 940.002 followed by 940.998 and were previouslt
            // both rounded to 940s
            //
            // NOTE: this rounding mechanism is identical to that
            //       used by the Ride Editor.
            double secs = floor(point->secs);
            double msecs = round((point->secs - secs) * 100) * 10;

            here->timeArray[arrayLength]  = secs + msecs/1000;
            if (!here->wattsArray.empty()) here->wattsArray[arrayLength] = max(0, point->watts);
            if (!here->npArray.empty()) here->npArray[arrayLength] = max(0, point->np);
            if (!here->xpArray.empty()) here->xpArray[arrayLength] = max(0, point->xp);
            if (!here->apArray.empty()) here->apArray[arrayLength] = max(0, point->apower);

            if (!here->hrArray.empty())
                here->hrArray[arrayLength]    = max(0, point->hr);
            if (!here->speedArray.empty())
                here->speedArray[arrayLength] = max(0,
                                              (context->athlete->useMetricUnits
                                               ? point->kph
                                               : point->kph * MILES_PER_KM));
            if (!here->cadArray.empty())
                here->cadArray[arrayLength]   = max(0, point->cad);
            if (!here->altArray.empty())
                here->altArray[arrayLength]   = (context->athlete->useMetricUnits
                                           ? point->alt
                                           : point->alt * FEET_PER_METER);
            if (!here->tempArray.empty())
                here->tempArray[arrayLength]   = point->temp;

            if (!here->windArray.empty())
                here->windArray[arrayLength] = max(0,
                                             (context->athlete->useMetricUnits
                                              ? point->headwind
                                              : point->headwind * MILES_PER_KM));

            if (!here->balanceArray.empty())
                here->balanceArray[arrayLength]   = point->lrbalance;

            here->distanceArray[arrayLength] = max(0,
                                             (context->athlete->useMetricUnits
                                              ? point->km
                                              : point->km * MILES_PER_KM));

            if (!here->torqueArray.empty())
                here->torqueArray[arrayLength] = max(0,
                                              (context->athlete->useMetricUnits
                                               ? point->nm
                                               : point->nm * FEET_LB_PER_NM));
            ++arrayLength;
        }
        recalc(here);
    }
    else {
        //setTitle("no data");

        here->wCurve->detach();
        here->mCurve->detach();
        here->wattsCurve->detach();
        here->npCurve->detach();
        here->xpCurve->detach();
        here->apCurve->detach();
        here->hrCurve->detach();
        here->speedCurve->detach();
        here->cadCurve->detach();
        here->altCurve->detach();
        here->tempCurve->detach();
        here->windCurve->detach();
        here->torqueCurve->detach();
        here->balanceLCurve->detach();
        here->balanceRCurve->detach();

        foreach(QwtPlotMarker *mrk, here->d_mrk)
            delete mrk;
        here->d_mrk.clear();

        foreach(QwtPlotMarker *mrk, here->cal_mrk)
            delete mrk;
        here->cal_mrk.clear();

        foreach(QwtPlotCurve *referenceLine, here->referenceLines) {
            referenceLine->detach();
            delete referenceLine;
        }
        here->referenceLines.clear();
    }

    // record the max x value
    if (here->timeArray.count() && here->distanceArray.count()) {
        int maxSECS = here->timeArray[here->timeArray.count()-1];
        int maxKM = here->distanceArray[here->distanceArray.count()-1];
        if (maxKM > here->maxKM) here->maxKM = maxKM;
        if (maxSECS > here->maxSECS) here->maxSECS = maxSECS;
    }

}

void
AllPlot::setShowPower(int id)
{
    if (showPowerState == id) return;

    showPowerState = id;
    standard->wattsCurve->setVisible(id < 2);
    shade_zones = (id == 0);
    setYMax();
    if (shade_zones) {
        bg->attach(this);
        refreshZoneLabels();
    } else
        bg->detach();
}

void
AllPlot::setShowNP(bool show)
{
    showNP = show;
    standard->npCurve->setVisible(show);
    setYMax();
    replot();
}

void
AllPlot::setShowXP(bool show)
{
    showXP = show;
    standard->xpCurve->setVisible(show);
    setYMax();
    replot();
}

void
AllPlot::setShowAP(bool show)
{
    showAP = show;
    standard->apCurve->setVisible(show);
    setYMax();
    replot();
}

void
AllPlot::setShowHr(bool show)
{
    showHr = show;
    standard->hrCurve->setVisible(show);
    setYMax();
    replot();
}

void
AllPlot::setShowSpeed(bool show)
{
    showSpeed = show;
    standard->speedCurve->setVisible(show);
    setYMax();
    replot();
}

void
AllPlot::setShowCad(bool show)
{
    showCad = show;
    standard->cadCurve->setVisible(show);
    setYMax();
    replot();
}

void
AllPlot::setShowAlt(bool show)
{
    showAlt = show;
    standard->altCurve->setVisible(show);
    setYMax();
    replot();
}

void
AllPlot::setShowTemp(bool show)
{
    showTemp = show;
    standard->tempCurve->setVisible(show);
    setYMax();
    replot();
}

void
AllPlot::setShowWind(bool show)
{
    showWind = show;
    standard->windCurve->setVisible(show);
    setYMax();
    replot();
}

void
AllPlot::setShowW(bool show)
{
    showW = show;
    standard->wCurve->setVisible(show);
    standard->mCurve->setVisible(show);
    if (!showW || (rideItem && rideItem->ride() && rideItem->ride()->wprimeData()->TAU <= 0)) {
        standard->curveTitle.setLabel(QwtText(""));
    }
    setYMax();
    replot();
}

void
AllPlot::setShowTorque(bool show)
{
    showTorque = show;
    standard->torqueCurve->setVisible(show);
    setYMax();
    replot();
}

void
AllPlot::setShowBalance(bool show)
{
    showBalance = show;
    standard->balanceLCurve->setVisible(show);
    standard->balanceRCurve->setVisible(show);
    setYMax();
    replot();
}

void
AllPlot::setShowGrid(bool show)
{
    standard->grid->setVisible(show);
    replot();
}

void
AllPlot::setPaintBrush(int state)
{
    if (state) {

        QColor p;
        p = standard->wCurve->pen().color();
        p.setAlpha(64);
        standard->wCurve->setBrush(QBrush(p));

        p = standard->wattsCurve->pen().color();
        p.setAlpha(64);
        standard->wattsCurve->setBrush(QBrush(p));

        p = standard->npCurve->pen().color();
        p.setAlpha(64);
        standard->npCurve->setBrush(QBrush(p));

        p = standard->xpCurve->pen().color();
        p.setAlpha(64);
        standard->xpCurve->setBrush(QBrush(p));

        p = standard->apCurve->pen().color();
        p.setAlpha(64);
        standard->apCurve->setBrush(QBrush(p));

        p = standard->hrCurve->pen().color();
        p.setAlpha(64);
        standard->hrCurve->setBrush(QBrush(p));

        p = standard->speedCurve->pen().color();
        p.setAlpha(64);
        standard->speedCurve->setBrush(QBrush(p));

        p = standard->cadCurve->pen().color();
        p.setAlpha(64);
        standard->cadCurve->setBrush(QBrush(p));

        p = standard->tempCurve->pen().color();
        p.setAlpha(64);
        standard->tempCurve->setBrush(QBrush(p));

        p = standard->torqueCurve->pen().color();
        p.setAlpha(64);
        standard->torqueCurve->setBrush(QBrush(p));

        /*p = standard->balanceLCurve->pen().color();
        p.setAlpha(64);
        standard->balanceLCurve->setBrush(QBrush(p));

        p = standard->balanceRCurve->pen().color();
        p.setAlpha(64);
        standard->balanceRCurve->setBrush(QBrush(p));*/
    } else {
        standard->wCurve->setBrush(Qt::NoBrush);
        standard->wattsCurve->setBrush(Qt::NoBrush);
        standard->npCurve->setBrush(Qt::NoBrush);
        standard->xpCurve->setBrush(Qt::NoBrush);
        standard->apCurve->setBrush(Qt::NoBrush);
        standard->hrCurve->setBrush(Qt::NoBrush);
        standard->speedCurve->setBrush(Qt::NoBrush);
        standard->cadCurve->setBrush(Qt::NoBrush);
        standard->tempCurve->setBrush(Qt::NoBrush);
        standard->torqueCurve->setBrush(Qt::NoBrush);
        //standard->balanceLCurve->setBrush(Qt::NoBrush);
        //standard->balanceRCurve->setBrush(Qt::NoBrush);
    }
    replot();
}

void
AllPlot::setSmoothing(int value)
{
    smooth = value;
    recalc(standard);
}

void
AllPlot::setByDistance(int id)
{
    bydist = (id == 1);
    setXTitle();
    recalc(standard);
}

struct ComparePoints {
    bool operator()(const double p1, const double p2) {
        return p1 < p2;
    }
};

int
AllPlot::timeIndex(double min) const
{
    // return index offset for specified time
    QVector<double>::const_iterator i = std::lower_bound(
            standard->smoothTime.begin(), standard->smoothTime.end(), min, ComparePoints());
    if (i == standard->smoothTime.end())
        return standard->smoothTime.size();
    return i - standard->smoothTime.begin();
}



int
AllPlot::distanceIndex(double km) const
{
    // return index offset for specified distance in km
	QVector<double>::const_iterator i = std::lower_bound(
            standard->smoothDistance.begin(), standard->smoothDistance.end(), km, ComparePoints());
    if (i == standard->smoothDistance.end())
        return standard->smoothDistance.size();
    return i - standard->smoothDistance.begin();
}
/*----------------------------------------------------------------------
 * Interval plotting
 *--------------------------------------------------------------------*/


/*
 * HELPER FUNCTIONS:
 *    intervalNum - returns a pointer to the nth selected interval
 *    intervalCount - returns the number of highlighted intervals
 */

// note this is operating on the children of allIntervals and not the
// intervalWidget (QTreeWidget) -- this is why we do not use the
// selectedItems() member. N starts a one not zero.
IntervalItem *IntervalPlotData::intervalNum(int n) const
{
    int highlighted=0;
    const QTreeWidgetItem *allIntervals = context->athlete->allIntervalItems();
    for (int i=0; i<allIntervals->childCount(); i++) {
        IntervalItem *current = (IntervalItem *)allIntervals->child(i);

        if (current != NULL) {
            if (current->isSelected() == true) ++highlighted;
        } else {
            return NULL;
        }
        if (highlighted == n) return current;
    }
    return NULL;
}

// how many intervals selected?
int IntervalPlotData::intervalCount() const
{
    int highlighted;
    highlighted = 0;
    if (context->athlete->allIntervalItems() == NULL) return 0; // not inited yet!

    const QTreeWidgetItem *allIntervals = context->athlete->allIntervalItems();
    for (int i=0; i<allIntervals->childCount(); i++) {
        IntervalItem *current = (IntervalItem *)allIntervals->child(i);
        if (current != NULL) {
            if (current->isSelected() == true) {
                ++highlighted;
            }
        }
    }
    return highlighted;
}

/*
 * INTERVAL HIGHLIGHTING CURVE
 * IntervalPlotData - implements the qwtdata interface where
 *                    x,y return point co-ordinates and
 *                    size returns the number of points
 */

// The interval curve data is derived from the intervals that have
// been selected in the Context leftlayout for each selected
// interval we return 4 data points; bottomleft, topleft, topright
// and bottom right.
//
// the points correspond to:
// bottom left = interval start, 0 watts
// top left = interval start, maxwatts
// top right = interval stop, maxwatts
// bottom right = interval stop, 0 watts
//
double IntervalPlotData::x(size_t i) const
{
    // for each interval there are four points, which interval is this for?
    int interval = i ? i/4 : 0;
    interval += 1; // interval numbers start at 1 not ZERO in the utility functions

    double multiplier = context->athlete->useMetricUnits ? 1 : MILES_PER_KM;

    // get the interval
    IntervalItem *current = intervalNum(interval);
    if (current == NULL) return 0; // out of bounds !?

    // which point are we returning?
    switch (i%4) {
    case 0 : return allPlot->bydist ? multiplier * current->startKM : current->start/60; // bottom left
    case 1 : return allPlot->bydist ? multiplier * current->startKM : current->start/60; // top left
    case 2 : return allPlot->bydist ? multiplier * current->stopKM : current->stop/60; // bottom right
    case 3 : return allPlot->bydist ? multiplier * current->stopKM : current->stop/60; // bottom right
    }
    return 0; // shouldn't get here, but keeps compiler happy
}


double IntervalPlotData::y(size_t i) const
{
    // which point are we returning?
    switch (i%4) {
    case 0 : return -100; // bottom left
    case 1 : return 5000; // top left - set to out of bound value
    case 2 : return 5000; // top right - set to out of bound value
    case 3 : return -100;  // bottom right
    }
    return 0;
}


size_t IntervalPlotData::size() const { return intervalCount()*4; }

QPointF IntervalPlotData::sample(size_t i) const {
    return QPointF(x(i), y(i));
}

QRectF IntervalPlotData::boundingRect() const
{
    return QRectF(-100, 5000, 5100, 5100);
}

void
AllPlot::pointHover(QwtPlotCurve *curve, int index)
{
    if (index >= 0 && curve != standard->intervalHighlighterCurve) {

        double yvalue = curve->sample(index).y();
        double xvalue = curve->sample(index).x();

        QString xstring;
        if (bydist) {
            xstring = QString("%1").arg(xvalue);
        } else {
            QTime t = QTime().addSecs(xvalue*60.00);
            xstring = t.toString("hh:mm:ss");
        }

        // output the tooltip
        QString text = QString("%1 %2\n%3 %4")
                        .arg(yvalue, 0, 'f', 0)
                        .arg(this->axisTitle(curve->yAxis()).text())
                        .arg(xstring)
                        .arg(this->axisTitle(curve->xAxis()).text());

        // set that text up
        tooltip->setText(text);

    } else {

        // no point
        tooltip->setText("");
    }
}

void
AllPlot::nextStep( int& step )
{
    if( step < 50 )
    {
        step += 10;
    }
    else if( step == 50 )
    {
        step = 100;
    }
    else if( step >= 100 && step < 1000 )
    {
        step += 100;
    }
    else if( step >= 1000 && step < 5000)
    {
        step += 500;
    }
    else
    {
        step += 1000;
    }
}

bool
AllPlot::eventFilter(QObject *obj, QEvent *event)
{
    int axis = -1;
    if (obj == axisWidget(QwtPlot::yLeft))
        axis=QwtPlot::yLeft;

    if (axis>-1 && event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *m = static_cast<QMouseEvent*>(event);
        confirmTmpReference(invTransform(axis, m->y()),axis, true); // do show delete stuff
    }
    if (axis>-1 && event->type() == QEvent::MouseMove) {
        QMouseEvent *m = static_cast<QMouseEvent*>(event);
        plotTmpReference(axis, m->x()-axisWidget(axis)->width(), m->y());
    }
    if (axis>-1 && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *m = static_cast<QMouseEvent*>(event);
        if (m->x()>axisWidget(axis)->width()) {
            confirmTmpReference(invTransform(axis, m->y()),axis,false); // don't show delete stuff
        }
        else  {
            plotTmpReference(axis, 0, 0); //unplot
        }
    }
    return false;
}

void
AllPlot::plotTmpReference(int axis, int x, int y)
{
    // not supported in compare mode
    if (context->isCompareIntervals) return;

    // only on power based charts
    if (scope != RideFile::none && scope != RideFile::watts &&
        scope != RideFile::NP && scope != RideFile::aPower && scope != RideFile::xPower) return;

    if (x>0) {

        RideFilePoint *referencePoint = new RideFilePoint();
        referencePoint->watts = invTransform(axis, y);

        foreach(QwtPlotCurve *curve, standard->tmpReferenceLines) {
            if (curve) {
                curve->detach();
                delete curve;
            }
        }
        standard->tmpReferenceLines.clear();

        // only plot if they are relevant to the plot.
        QwtPlotCurve *referenceLine = parent->allPlot->plotReferenceLine(referencePoint);
        if (referenceLine) {
            standard->tmpReferenceLines.append(referenceLine);
            parent->allPlot->replot();
        }

        // now do the series plots
        foreach(AllPlot *plot, parent->seriesPlots) {
            QwtPlotCurve *referenceLine = plot->plotReferenceLine(referencePoint);
            if (referenceLine) {
                standard->tmpReferenceLines.append(referenceLine);
                plot->replot();
            }
        }

        // now the stack plots
        foreach(AllPlot *plot, parent->allPlots) {
            QwtPlotCurve *referenceLine = plot->plotReferenceLine(referencePoint);
            if (referenceLine) {
                standard->tmpReferenceLines.append(referenceLine);
                plot->replot();
            }
        }


    } else  {

        // wipe any we don't want
        foreach(QwtPlotCurve *curve, standard->tmpReferenceLines) {
            if (curve) {
                curve->detach();
                delete curve;
            }
        }
        standard->tmpReferenceLines.clear();
        parent->allPlot->replot();
        foreach(AllPlot *plot, parent->seriesPlots) {
            plot->replot();
        }
        parent->allPlot->replot();
        foreach(AllPlot *plot, parent->allPlots) {
            plot->replot();
        }
    }
}

void
AllPlot::refreshReferenceLinesForAllPlots()
{
    // not supported in compare mode
    if (context->isCompareIntervals) return;

    parent->allPlot->refreshReferenceLines();
    foreach(AllPlot *plot, parent->allPlots) {
        plot->refreshReferenceLines();
    }
    foreach(AllPlot *plot, parent->seriesPlots) {
        plot->refreshReferenceLines();
    }
}

void
AllPlot::confirmTmpReference(double value, int axis, bool allowDelete)
{
    // not supported in compare mode
    if (context->isCompareIntervals) return;

    ReferenceLineDialog *p = new ReferenceLineDialog(this, context, allowDelete);
    p->setWindowModality(Qt::ApplicationModal); // don't allow select other ride or it all goes wrong!
    p->setValueForAxis(value, axis);
    p->move(QCursor::pos()-QPoint(40,40));
    p->exec();
}
