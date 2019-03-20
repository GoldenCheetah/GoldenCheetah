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
#include "AllPlotSlopeCurve.h"
#include "ReferenceLineDialog.h"
#include "ExhaustionDialog.h"
#include "RideFile.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "Settings.h"
#include "Units.h"
#include "Zones.h"
#include "Colors.h"
#include "WPrime.h"
#include "IndendPlotMarker.h"
#include "Utils.h"

#include <qwt_plot_curve.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_intervalcurve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_div.h>
#include <qwt_scale_widget.h>
#include <qwt_text.h>
#include <qwt_legend.h>
#include <qwt_series_data.h>
#include <qwt_scale_engine.h>

#include "qwt_plot_gapped_curve.h"

#include <QMultiMap>

#include <string.h> // for memcpy

class IntervalPlotData : public QwtSeriesData<QPointF>
{
    public:
    IntervalPlotData(AllPlot *allPlot, Context *context, AllPlotWindow *window) :
        allPlot(allPlot), context(context), window(window) {}
    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    //virtual QwtData *copy() const ;
    void init() ;
    IntervalItem *intervalNum(int n) const;
    int intervalCount() const;
    AllPlot *allPlot;
    Context *context;
    AllPlotWindow *window;

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
            setZ(-100.0);
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

            zones = parent->context->athlete->zones(rideItem ? rideItem->isRun : false);
            if (!zones) return;

            // use first compare interval date
            if (parent->context->compareIntervals.count())
                zone_range = zones->whichRange(parent->context->compareIntervals[0].data->startTime().date());

            // still not set 
            if (zone_range == -1)
                zone_range = zones->whichRange(QDate::currentDate());

        } else if (rideItem && parent->context->athlete->zones(rideItem->isRun)) {

            zones = parent->context->athlete->zones(rideItem->isRun);
            zone_range = parent->context->athlete->zones(rideItem->isRun)->whichRange(rideItem->dateTime.date());

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

                zones = parent->context->athlete->zones(rideItem ? rideItem->isRun : false);
                if (!zones) return;

                // use first compare interval date
                if (parent->context->compareIntervals.count())
                    zone_range = zones->whichRange(parent->context->compareIntervals[0].data->startTime().date());

                // still not set 
                if (zone_range == -1)
                    zone_range = zones->whichRange(QDate::currentDate());

            } else if (rideItem && parent->context->athlete->zones(rideItem->isRun)) {

                zones = parent->context->athlete->zones(rideItem->isRun);
                zone_range = parent->context->athlete->zones(rideItem->isRun)->whichRange(rideItem->dateTime.date());

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

            setZ(-99.00 + zone_number / 100.0);
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

class TimeScaleDraw: public ScaleScaleDraw
{

    public:

    TimeScaleDraw(bool *bydist, int *timeoffset) : ScaleScaleDraw(), bydist(bydist), timeoffset(timeoffset) {}

    virtual QwtText label(double v) const
    {
        if (*bydist) {
            return QString("%1").arg(v);
        } else {
            QTime t = QTime(0,0,0,0).addSecs((*timeoffset+v)*60.00);
            if (scaleMap().sDist() > 5)
                return t.toString("hh:mm");
            return t.toString("hh:mm:ss");
        }
    }
    private:
    bool *bydist;
    int *timeoffset;

};

static inline double
max(double a, double b) { if (a > b) return a; else return b; }

AllPlotObject::AllPlotObject(AllPlot *plot, QList<UserData*> user) : plot(plot)
{
    maxKM = maxSECS = 0;

    // user data
    setUserData(user);

    wattsCurve = (QwtPlotCurve*)new QwtPlotGappedCurve(tr("Power"), 3); // > 3s is a power gap
    wattsCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    wattsCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 0));

    antissCurve = new QwtPlotCurve(tr("anTISS"));
    antissCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    antissCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 3));

    atissCurve = new QwtPlotCurve(tr("aTISS"));
    atissCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    atissCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 3));

    npCurve = new QwtPlotCurve(tr("IsoPower"));
    npCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    npCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 0));

    rvCurve = new QwtPlotCurve(tr("Vertical Oscillation"));
    rvCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    rvCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 0));

    rcadCurve = new QwtPlotCurve(tr("Run Cadence"));
    rcadCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    rcadCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 0));

    rgctCurve = new QwtPlotCurve(tr("GCT"));
    rgctCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    rgctCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 0));

    gearCurve = new QwtPlotCurve(tr("Gear Ratio"));
    gearCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    gearCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 0));
    gearCurve->setStyle(QwtPlotCurve::Steps);
    gearCurve->setCurveAttribute(QwtPlotCurve::Inverted);

    smo2Curve = new QwtPlotCurve(tr("SmO2"));
    smo2Curve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    smo2Curve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));

    thbCurve = new QwtPlotCurve(tr("tHb"));
    thbCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    thbCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 0));

    o2hbCurve = new QwtPlotCurve(tr("O2Hb"));
    o2hbCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    o2hbCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 0));

    hhbCurve = new QwtPlotCurve(tr("HHb"));
    hhbCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    hhbCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 0));

    xpCurve = new QwtPlotCurve(tr("xPower"));
    xpCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    xpCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 0));

    apCurve = new QwtPlotCurve(tr("aPower"));
    apCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    apCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 0));

    hrCurve = new QwtPlotCurve(tr("Heart Rate"));
    hrCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    hrCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));

    hrvCurve = new QwtPlotCurve(tr("R-R Rate"));
    hrvCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    hrvCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));

    tcoreCurve = new QwtPlotCurve(tr("Core Temp"));
    tcoreCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    tcoreCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 1));

    accelCurve = new QwtPlotCurve(tr("Acceleration"));
    accelCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    accelCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 0));

    wattsDCurve = new QwtPlotCurve(tr("Power Delta"));
    wattsDCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    wattsDCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 0));

    cadDCurve = new QwtPlotCurve(tr("Cadence Delta"));
    cadDCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    cadDCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 0));

    nmDCurve = new QwtPlotCurve(tr("Torque Delta"));
    nmDCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    nmDCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 0));

    hrDCurve = new QwtPlotCurve(tr("Heartrate Delta"));
    hrDCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    hrDCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 0));

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

    altSlopeCurve = new AllPlotSlopeCurve(tr("Alt/Slope"));
    altSlopeCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    altSlopeCurve->setStyle(AllPlotSlopeCurve::SlopeDist1);
    altSlopeCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 1));
    altSlopeCurve->setZ(-5); // always at the back.

    slopeCurve = new QwtPlotCurve(tr("Slope"));
    slopeCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    slopeCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 3));


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
    balanceLCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 3));

    balanceRCurve = new QwtPlotCurve(tr("Right Balance"));
    balanceRCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    balanceRCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 3));

    lteCurve = new QwtPlotCurve(tr("Left Torque Efficiency"));
    lteCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    lteCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 3));

    rteCurve = new QwtPlotCurve(tr("Right Torque Efficiency"));
    rteCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    rteCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 3));

    lpsCurve = new QwtPlotCurve(tr("Left Pedal Smoothness"));
    lpsCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    lpsCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 3));

    rpsCurve = new QwtPlotCurve(tr("Right Pedal Smoothness"));
    rpsCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    rpsCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 3));

    lpcoCurve = new QwtPlotCurve(tr("Left Pedal Center Offset"));
    lpcoCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    lpcoCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 3));

    rpcoCurve = new QwtPlotCurve(tr("Right Pedal Center Offset"));
    rpcoCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    rpcoCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 3));

    lppCurve = new QwtPlotIntervalCurve(tr("Left Pedal Power Phase"));
    lppCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 3));

    rppCurve = new QwtPlotIntervalCurve(tr("Right Pedal Power Phase"));
    rppCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 3));

    lpppCurve = new QwtPlotIntervalCurve(tr("Left Peak Pedal Power Phase"));
    lpppCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 3));

    rpppCurve = new QwtPlotIntervalCurve(tr("Right Peak Pedal Power Phase"));
    rpppCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 3));

    wCurve = new QwtPlotCurve(tr("W' Balance (kJ)"));
    wCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    wCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 2));

    mCurve = new QwtPlotCurve(tr("Matches"));
    mCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    mCurve->setStyle(QwtPlotCurve::Dots);
    mCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 2));

    curveTitle.attach(plot);
    curveTitle.setLabelAlignment(Qt::AlignRight);

    intervalHighlighterCurve = new QwtPlotCurve();
    intervalHighlighterCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 2));
    intervalHighlighterCurve->setBaseline(-20); // go below axis
    intervalHighlighterCurve->setZ(-20); // behind alt but infront of zones
    intervalHighlighterCurve->attach(plot);
    intervalHoverCurve = new QwtPlotCurve();
    intervalHoverCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 2));
    intervalHoverCurve->setBaseline(-20); // go below axis
    intervalHoverCurve->setZ(-20); // behind alt but infront of zones
    intervalHoverCurve->attach(plot);

    // setup that standard->grid
    grid = new QwtPlotGrid();
    grid->enableX(false); // not needed
    grid->enableY(true);
    grid->attach(plot);

}

QVector<QwtZone>
AllPlotObject::parseZoneString(QString zstring)
{
    QVector<QwtZone> returning;

    // split zstring into ; delimetered tokens
    foreach(QString entry, zstring.split(";")) {

        // need value, colorname
        QStringList toks = entry.split(",");
        if (toks.count()!=2) continue;
        bool ok;
        double lim=toks[0].toDouble(&ok);
        if (!ok) continue;
        QColor col;
        col.setNamedColor(toks[1]);
        if (!col.isValid()) continue;

        // well that worked!
        returning << QwtZone(lim,col);

    }
    return returning;
}

void 
AllPlotObject::setUserData(QList<UserData*>user)
{
    // wipe away current
    // user objects
    for(int k=0; k<U.count(); k++) {

        // wipe any curves
        U[k].curve->detach(); delete U[k].curve;
    }
    U.clear();

    // setup the U array
    int k=0;
    bool antialias = appsettings->value(this, GC_ANTIALIAS, true).toBool();

    foreach(UserData *userdata, user) {

        UserObject add;

        // create curve
        add.name = userdata->name;
        add.units = userdata->units;
        add.curve = new QwtPlotGappedCurve(userdata->name, 3);
        //add.curve->setNAValue(RideFile::NA);
        add.curve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
        add.curve->setYAxis(QwtAxisId(QwtAxis::yRight, 4 + k)); // for now.
        add.curve->attach(plot);

        // default the color
        add.color = userdata->color;
        add.color.setAlpha(200);

        // set zones
        QVector<QwtZone> zones = parseZoneString(userdata->zstring);
        if (zones.count()>0) add.curve->setZones(zones);

        QPen pen;
        pen.setWidth(1.0);
        pen.setColor(userdata->color);
        add.curve->setPen(pen);

        if (plot->fill || zones.count()>0) {
            QColor p = add.color;
            p.setAlpha(64);
            add.curve->setBrush(QBrush(p));
        } else {
            add.curve->setBrush(Qt::NoBrush);
        }

        // antialias
        if (antialias) add.curve->setRenderHint(QwtPlotItem::RenderAntialiased);

        // register
        U << add;

        k++; // keep count
    }

    // reset the y axis to accomodate us
    plot->setAxesCount(QwtAxis::yRight, 4 + U.count());
    QPalette pal = plot->palette();
    for(int k=0; k < U.count(); k++) {

        // set axis
        plot->setAxisMaxMinor(QwtAxisId(QwtAxis::yRight, 4 + k), 0);
        plot->axisWidget(QwtAxisId(QwtAxis::yRight, 4 + k))->installEventFilter(plot);

        // scale and color for axis
        ScaleScaleDraw *sd = new ScaleScaleDraw;
        sd->setTickLength(QwtScaleDiv::MajorTick, 3);
        sd->enableComponent(ScaleScaleDraw::Ticks, false);
        sd->enableComponent(ScaleScaleDraw::Backbone, false);
        plot->setAxisScaleDraw(QwtAxisId(QwtAxis::yRight, 4 + k), sd);
        pal.setColor(QPalette::WindowText, U[k].color);
        pal.setColor(QPalette::Text, U[k].color);
        plot->axisWidget(QwtAxisId(QwtAxis::yRight, 4 + k))->setPalette(pal);

        // title
        plot->setAxisTitle(QwtAxisId(QwtAxis::yRight, 4 + k), U[k].units);

        // and hide if plot is scoped for only one series, for now
        if (plot->scope != RideFile::none) {
            plot->axisWidget(QwtAxisId(QwtAxis::yRight, 4 + k))->setVisible(false);
            U[k].curve->setVisible(false);
            U[k].curve->detach();
        }
    }
    plot->curveColors->saveState();
}

// we tend to only do this for the compare objects
void
AllPlotObject::setColor(QColor color)
{
    QList<QwtPlotCurve*> worklist;
    worklist << mCurve << wCurve << wattsCurve << atissCurve << antissCurve << npCurve << xpCurve << speedCurve << accelCurve
             << wattsDCurve << cadDCurve << nmDCurve << hrDCurve
             << apCurve << cadCurve << tempCurve << hrCurve << hrvCurve <<tcoreCurve << torqueCurve << balanceLCurve
             << balanceRCurve << lteCurve << rteCurve << lpsCurve << rpsCurve
             << lpcoCurve << rpcoCurve
             << altCurve << slopeCurve << altSlopeCurve
             << rvCurve << rcadCurve << rgctCurve << gearCurve 
             << smo2Curve << thbCurve << o2hbCurve << hhbCurve;


    // work through getting progressively lighter
    QPen pen;
    pen.setWidth(1.0);
    int alpha = 200;
    bool antialias = appsettings->value(this, GC_ANTIALIAS, true).toBool();
    foreach(QwtPlotCurve *c, worklist) {

        pen.setColor(color);
        color.setAlpha(alpha);

        c->setPen(pen);
        if (antialias) c->setRenderHint(QwtPlotItem::RenderAntialiased);

        // lighten up for the next guy
        color = color.darker(110);
        if (alpha > 10) alpha -= 10;
    }

    // has to be different...
    windCurve->setPen(pen);
    if (antialias)windCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    lppCurve->setPen(pen);
    if (antialias)lppCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    rppCurve->setPen(pen);
    if (antialias)rppCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    lpppCurve->setPen(pen);
    if (antialias)lpppCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    rpppCurve->setPen(pen);
    if (antialias)rpppCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

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
    atissCurve->detach(); delete atissCurve;
    antissCurve->detach(); delete antissCurve;
    npCurve->detach(); delete npCurve;
    rcadCurve->detach(); delete rcadCurve;
    rvCurve->detach(); delete rvCurve;
    rgctCurve->detach(); delete rgctCurve;
    gearCurve->detach(); delete gearCurve;
    smo2Curve->detach(); delete smo2Curve;
    thbCurve->detach(); delete thbCurve;
    o2hbCurve->detach(); delete o2hbCurve;
    hhbCurve->detach(); delete hhbCurve;
    xpCurve->detach(); delete xpCurve;
    apCurve->detach(); delete apCurve;
    hrCurve->detach(); delete hrCurve;
    hrvCurve->detach(); delete hrvCurve;
    tcoreCurve->detach(); delete tcoreCurve;
    speedCurve->detach(); delete speedCurve;
    accelCurve->detach(); delete accelCurve;
    wattsDCurve->detach(); delete wattsDCurve;
    cadDCurve->detach(); delete cadDCurve;
    nmDCurve->detach(); delete nmDCurve;
    hrDCurve->detach(); delete hrDCurve;
    cadCurve->detach(); delete cadCurve;
    altCurve->detach(); delete altCurve;
    slopeCurve->detach(); delete slopeCurve;
    altSlopeCurve->detach(); delete altSlopeCurve;
    tempCurve->detach(); delete tempCurve;
    windCurve->detach(); delete windCurve;
    torqueCurve->detach(); delete torqueCurve;
    balanceLCurve->detach(); delete balanceLCurve;
    balanceRCurve->detach(); delete balanceRCurve;
    lteCurve->detach(); delete lteCurve;
    rteCurve->detach(); delete rteCurve;
    lpsCurve->detach(); delete lpsCurve;
    rpsCurve->detach(); delete rpsCurve;
    lpcoCurve->detach(); delete lpcoCurve;
    rpcoCurve->detach(); delete rpcoCurve;
    lppCurve->detach(); delete lppCurve;
    rppCurve->detach(); delete rppCurve;
    lpppCurve->detach(); delete lpppCurve;
    rpppCurve->detach(); delete rpppCurve;

    // user objects
    foreach(UserObject u, U) {

        // wipe any curves
        u.curve->detach(); delete u.curve;
    }
    U.clear();
}

void
AllPlotObject::setVisible(bool show)
{
    if (show == false) {

        foreach(UserObject u, U) u.curve->detach();
        grid->detach(); 
        mCurve->detach();
        wCurve->detach();
        wattsCurve->detach();
        npCurve->detach();
        rcadCurve->detach();
        rvCurve->detach();
        rgctCurve->detach();
        gearCurve->detach();
        smo2Curve->detach();
        thbCurve->detach();
        o2hbCurve->detach();
        hhbCurve->detach();
        atissCurve->detach();
        antissCurve->detach();
        xpCurve->detach();
        apCurve->detach();
        hrCurve->detach();
        hrvCurve->detach();
        tcoreCurve->detach();
        speedCurve->detach();
        accelCurve->detach();
        wattsDCurve->detach();
        cadDCurve->detach();
        nmDCurve->detach();
        hrDCurve->detach();
        cadCurve->detach();
        altCurve->detach();
        slopeCurve->detach();
        altSlopeCurve->detach();
        tempCurve->detach();
        windCurve->detach();
        torqueCurve->detach();
        lteCurve->detach();
        rteCurve->detach();
        lpsCurve->detach();
        rpsCurve->detach();
        lpcoCurve->detach();
        rpcoCurve->detach();
        lppCurve->detach();
        rppCurve->detach();
        lpppCurve->detach();
        rpppCurve->detach();
        balanceLCurve->detach();
        balanceRCurve->detach();
        intervalHighlighterCurve->detach();
        intervalHoverCurve->detach();

        // marks, calibrations and reference lines
        foreach(QwtPlotMarker *mrk, d_mrk) {
            mrk->detach();
        }
        foreach(QwtPlotCurve *referenceLine, referenceLines) {
            referenceLine->detach();
        }

    } else {


        foreach(UserObject u, U) u.curve->attach(plot);
        altCurve->attach(plot); // always do first as it hasa brush
        grid->attach(plot);
        mCurve->attach(plot);
        wCurve->attach(plot);
        wattsCurve->attach(plot);
        slopeCurve->attach(plot);
        altSlopeCurve->attach(plot);
        npCurve->attach(plot);
        rvCurve->attach(plot);
        rcadCurve->attach(plot);
        rgctCurve->attach(plot);
        gearCurve->attach(plot);
        smo2Curve->attach(plot);
        thbCurve->attach(plot);
        o2hbCurve->attach(plot);
        hhbCurve->attach(plot);
        atissCurve->attach(plot);
        antissCurve->attach(plot);
        xpCurve->attach(plot);
        apCurve->attach(plot);
        hrCurve->attach(plot);
        hrvCurve->attach(plot);
        tcoreCurve->attach(plot);
        speedCurve->attach(plot);
        accelCurve->attach(plot);
        wattsDCurve->attach(plot);
        cadDCurve->attach(plot);
        nmDCurve->attach(plot);
        hrDCurve->attach(plot);
        cadCurve->attach(plot);
        tempCurve->attach(plot);
        windCurve->attach(plot);
        torqueCurve->attach(plot);
        lteCurve->attach(plot);
        rteCurve->attach(plot);
        lpsCurve->attach(plot);
        rpsCurve->attach(plot);
        lpcoCurve->attach(plot);
        rpcoCurve->attach(plot);
        lppCurve->attach(plot);
        rppCurve->attach(plot);
        lpppCurve->attach(plot);
        rpppCurve->attach(plot);
        balanceLCurve->attach(plot);
        balanceRCurve->attach(plot);

        intervalHighlighterCurve->attach(plot);
        intervalHoverCurve->attach(plot);

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
    if (!plot->showRV) rvCurve->detach();
    if (!plot->showRCad) rcadCurve->detach();
    if (!plot->showRGCT) rgctCurve->detach();
    if (!plot->showGear) gearCurve->detach();
    if (!plot->showSmO2) smo2Curve->detach();
    if (!plot->showtHb) thbCurve->detach();
    if (!plot->showO2Hb) o2hbCurve->detach();
    if (!plot->showHHb) hhbCurve->detach();
    if (!plot->showATISS) atissCurve->detach();
    if (!plot->showANTISS) antissCurve->detach();
    if (!plot->showXP) xpCurve->detach();
    if (!plot->showAP) apCurve->detach();
    if (!plot->showW) wCurve->detach();
    if (!plot->showW) mCurve->detach();
    if (!plot->showHr) hrCurve->detach();
    if (!plot->showHRV) hrvCurve->detach();
    if (!plot->showTcore) tcoreCurve->detach();
    if (!plot->showSpeed) speedCurve->detach();
    if (!plot->showAccel) accelCurve->detach();
    if (!plot->showPowerD) wattsDCurve->detach();
    if (!plot->showCadD) cadDCurve->detach();
    if (!plot->showTorqueD) nmDCurve->detach();
    if (!plot->showHrD) hrDCurve->detach();
    if (!plot->showCad) cadCurve->detach();
    if (!plot->showAlt) altCurve->detach();
    if (!plot->showSlope) slopeCurve->detach();
    if (plot->showAltSlopeState == 0) altSlopeCurve->detach();
    if (!plot->showTemp) tempCurve->detach();
    if (!plot->showWind) windCurve->detach();
    if (!plot->showTorque) torqueCurve->detach();
    if (!plot->showTE) {
            lteCurve->detach();
            rteCurve->detach();
    }
    if (!plot->showPS) {
            lpsCurve->detach();
            rpsCurve->detach();
    }
    if (!plot->showPCO) {
            lpcoCurve->detach();
            rpcoCurve->detach();
    }
    if (!plot->showDC) {
            lppCurve->detach();
            rppCurve->detach();
    }
    if (!plot->showPPP) {
            lpppCurve->detach();
            rpppCurve->detach();
    }
    if (!plot->showBalance) {
            balanceLCurve->detach();
            balanceRCurve->detach();
    }
}

AllPlot::AllPlot(QWidget *parent, AllPlotWindow *window, Context *context, RideFile::SeriesType scope, RideFile::SeriesType secScope, bool wanttext):
    QwtPlot(parent),
    rideItem(NULL),
    shade_zones(true),
    showPowerState(3),
    showAltSlopeState(0),
    showATISS(false),
    showANTISS(false),
    showNP(false),
    showXP(false),
    showAP(false),
    showHr(true),
    showHRV(true),
    showTcore(false),
    showSpeed(true),
    showAccel(false),
    showPowerD(false),
    showCadD(false),
    showTorqueD(false),
    showHrD(false),
    showCad(true),
    showAlt(true),
    showSlope(false),
    showTemp(true),
    showWind(true),
    showTorque(true),
    showBalance(true),
    showRV(true),
    showRGCT(true),
    showRCad(true),
    showSmO2(true),
    showtHb(true),
    showO2Hb(true),
    showHHb(true),
    showGear(true),
    bydist(false),
    bytimeofday(false),
    timeoffset(0),
    scope(scope),
    secondaryScope(secScope),
    context(context),
    parent(parent),
    window(window),
    wanttext(wanttext),
    isolation(false)
{

    if (appsettings->value(this, GC_SHADEZONES, true).toBool()==false)
        shade_zones = false;

    smooth = 1;
    standard = NULL; // until its created
    wantxaxis = wantaxis = true;
    setAutoDelete(false); // no - we are managing it via the AllPlotObjects now
    referencePlot = NULL;
    tooltip = NULL;
    _canvasPicker = NULL;

    // curve color object
    curveColors = new CurveColors(this, true);

    // create a background object for shading
    bg = new AllPlotBackground(this);
    bg->attach(this);

    //insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

    // set the axes that we use.. yLeft 3 is ALWAYS the highlighter axes and never visible
    // yLeft 4 is balance stuff
    setAxesCount(QwtAxis::yLeft, 4);
    setAxesCount(QwtAxis::yRight, 4 + (window ? window->userDataSeries.count() : 0));
    setAxesCount(QwtAxis::xBottom, 1);

    setXTitle();

    standard = new AllPlotObject(this, window ? window->userDataSeries : QList<UserData*>());

    standard->intervalHighlighterCurve->setSamples(new IntervalPlotData(this, context, window));

    setAxisMaxMinor(xBottom, 0);
    enableAxis(xBottom, true);
    setAxisVisible(xBottom, true);
    axisWidget(QwtAxisId(QwtAxis::xBottom))->installEventFilter(this);

    // highlighter
    ScaleScaleDraw *sd = new ScaleScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 2);
    sd->enableComponent(ScaleScaleDraw::Ticks, false);
    sd->enableComponent(ScaleScaleDraw::Backbone, false);
    setAxisScaleDraw(QwtAxisId(QwtAxis::yLeft, 2), sd);

    QPalette pal = palette();
    pal.setBrush(QPalette::Background, QBrush(GColor(CRIDEPLOTBACKGROUND)));
    pal.setColor(QPalette::WindowText, QColor(Qt::gray));
    pal.setColor(QPalette::Text, QColor(Qt::gray));
    axisWidget(QwtAxisId(QwtAxis::yLeft, 2))->setPalette(pal);
    setAxisScale(QwtAxisId(QwtAxis::yLeft, 2), 0, 100);
    setAxisVisible(QwtAxisId(QwtAxis::yLeft, 2), false); // hide interval axis

    setAxisMaxMinor(yLeft, 0);
    setAxisMaxMinor(QwtAxisId(QwtAxis::yLeft, 1), 0);
    setAxisMaxMinor(QwtAxisId(QwtAxis::yLeft, 3), 0);
    setAxisMaxMinor(yRight, 0);
    setAxisMaxMinor(QwtAxisId(QwtAxis::yRight, 1), 0);
    setAxisMaxMinor(QwtAxisId(QwtAxis::yRight, 2), 0);
    setAxisMaxMinor(QwtAxisId(QwtAxis::yRight, 3), 0);

    axisWidget(QwtPlot::yLeft)->installEventFilter(this);
    axisWidget(QwtPlot::yRight)->installEventFilter(this);
    axisWidget(QwtAxisId(QwtAxis::yLeft, 1))->installEventFilter(this);
    axisWidget(QwtAxisId(QwtAxis::yLeft, 3))->installEventFilter(this);
    axisWidget(QwtAxisId(QwtAxis::yRight, 1))->installEventFilter(this);
    axisWidget(QwtAxisId(QwtAxis::yRight, 2))->installEventFilter(this);
    axisWidget(QwtAxisId(QwtAxis::yRight, 3))->installEventFilter(this);

    for(int k=0; k < standard->U.count(); k++) {
        setAxisMaxMinor(QwtAxisId(QwtAxis::yRight, 4 + k), 0);
        axisWidget(QwtAxisId(QwtAxis::yRight, 4 + k))->installEventFilter(this);
    }

    configChanged(CONFIG_APPEARANCE); // set colors
}

AllPlot::~AllPlot()
{
    // wipe compare curves if there are any
    foreach(QwtPlotCurve *compare, compares) {
        compare->detach(); delete compare;
    }
    compares.clear();

    // wipe the standard stuff
    if (tooltip) delete tooltip;
    if (_canvasPicker) delete _canvasPicker;
    delete standard;

}

void
AllPlot::configChanged(qint32 what)
{

    if (what & CONFIG_APPEARANCE) {

        double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();
        labelFont.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
        labelFont.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());

        if (appsettings->value(this, GC_ANTIALIAS, true).toBool() == true) {
            standard->wattsCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->atissCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->antissCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->npCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->rvCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->rcadCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->rgctCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->gearCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->smo2Curve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->thbCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->o2hbCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->hhbCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->xpCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->apCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->wCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->mCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->hrCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->hrvCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->tcoreCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->speedCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->accelCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->wattsDCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->cadDCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->nmDCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->hrDCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->cadCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->altCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->slopeCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->altSlopeCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->tempCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->windCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->torqueCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->lteCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->rteCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->lpsCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->rpsCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->lpcoCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->rpcoCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->lppCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->rppCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->lpppCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->rpppCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->balanceLCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->balanceRCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->intervalHighlighterCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            standard->intervalHoverCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        }

        setAltSlopePlotStyle(standard->altSlopeCurve);

        setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
        QPen wattsPen = QPen(GColor(CPOWER));
        wattsPen.setWidth(width);
        standard->wattsCurve->setPen(wattsPen);
        standard->wattsDCurve->setPen(wattsPen);
        QPen npPen = QPen(GColor(CNPOWER));
        npPen.setWidth(width);
        standard->npCurve->setPen(npPen);
        QPen rvPen = QPen(GColor(CRV));
        rvPen.setWidth(width);
        standard->rvCurve->setPen(rvPen);
        QPen rcadPen = QPen(GColor(CRCAD));
        rcadPen.setWidth(width);
        standard->rcadCurve->setPen(rcadPen);
        QPen rgctPen = QPen(GColor(CRGCT));
        rgctPen.setWidth(width);
        standard->rgctCurve->setPen(rgctPen);
        QPen gearPen = QPen(GColor(CGEAR));
        gearPen.setWidth(width);
        standard->gearCurve->setPen(gearPen);
        QPen smo2Pen = QPen(GColor(CSMO2));
        smo2Pen.setWidth(width);
        standard->smo2Curve->setPen(smo2Pen);
        QPen thbPen = QPen(GColor(CTHB));
        thbPen.setWidth(width);
        standard->thbCurve->setPen(thbPen);
        QPen o2hbPen = QPen(GColor(CO2HB));
        o2hbPen.setWidth(width);
        standard->o2hbCurve->setPen(o2hbPen);
        QPen hhbPen = QPen(GColor(CHHB));
        hhbPen.setWidth(width);
        standard->hhbCurve->setPen(hhbPen);

        QPen antissPen = QPen(GColor(CANTISS));
        antissPen.setWidth(width);
        standard->antissCurve->setPen(antissPen);
        QPen atissPen = QPen(GColor(CATISS));
        atissPen.setWidth(width);
        standard->atissCurve->setPen(atissPen);
        QPen xpPen = QPen(GColor(CXPOWER));
        xpPen.setWidth(width);
        standard->xpCurve->setPen(xpPen);
        QPen apPen = QPen(GColor(CAPOWER));
        apPen.setWidth(width);
        standard->apCurve->setPen(apPen);
        QPen hrPen = QPen(GColor(CHEARTRATE));
        hrPen.setWidth(width);
        standard->hrCurve->setPen(hrPen);
        standard->hrvCurve->setPen(hrPen);
        QPen tcorePen = QPen(GColor(CCORETEMP));
        tcorePen.setWidth(width);
        standard->tcoreCurve->setPen(tcorePen);
        standard->hrDCurve->setPen(hrPen);
        QPen speedPen = QPen(GColor(CSPEED));
        speedPen.setWidth(width);
        standard->speedCurve->setPen(speedPen);
        QPen accelPen = QPen(GColor(CACCELERATION));
        accelPen.setWidth(width);
        standard->accelCurve->setPen(accelPen);
        QPen cadPen = QPen(GColor(CCADENCE));
        cadPen.setWidth(width);
        standard->cadCurve->setPen(cadPen);
        standard->cadDCurve->setPen(cadPen);
        QPen slopePen(GColor(CSLOPE));
        slopePen.setWidth(width);
        standard->slopeCurve->setPen(slopePen);
        QPen altPen(GColor(CALTITUDE));
        altPen.setWidth(width);
        standard->altCurve->setPen(altPen);
        QColor brush_color = GColor(CALTITUDEBRUSH);
        brush_color.setAlpha(200);
        standard->altCurve->setBrush(brush_color);   // fill below the line
        QPen altSlopePen(GCColor::invertColor(GColor(CPLOTBACKGROUND)));
        altSlopePen.setWidth(width);
        standard->altSlopeCurve->setPen(altSlopePen);
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
        standard->nmDCurve->setPen(torquePen);
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
        QPen ltePen = QPen(GColor(CLTE));
        ltePen.setWidth(width);
        standard->lteCurve->setPen(ltePen);
        QPen rtePen = QPen(GColor(CRTE));
        rtePen.setWidth(width);
        standard->rteCurve->setPen(rtePen);
        QPen lpsPen = QPen(GColor(CLPS));
        lpsPen.setWidth(width);
        standard->lpsCurve->setPen(lpsPen);
        QPen rpsPen = QPen(GColor(CRPS));
        rpsPen.setWidth(width);
        standard->rpsCurve->setPen(rpsPen);
        QPen lpcoPen = QPen(GColor(CLPS));
        lpcoPen.setWidth(width);
        standard->lpcoCurve->setPen(lpcoPen);
        QPen rpcoPen = QPen(GColor(CRPS));
        rpcoPen.setWidth(width);
        standard->rpcoCurve->setPen(rpcoPen);
        QPen ldcPen = QPen(GColor(CLPS));
        ldcPen.setWidth(width);
        standard->lppCurve->setPen(ldcPen);
        QPen rdcPen = QPen(GColor(CRPS));
        rdcPen.setWidth(width);
        standard->rppCurve->setPen(rdcPen);
        QPen lpppPen = QPen(GColor(CLPS));
        lpppPen.setWidth(width);
        standard->lpppCurve->setPen(lpppPen);
        QPen rpppPen = QPen(GColor(CRPS));
        rpppPen.setWidth(width);
        standard->rpppCurve->setPen(rpppPen);
        QPen wPen = QPen(GColor(CWBAL)); 
        wPen.setWidth(width); // don't thicken
        standard->wCurve->setPen(wPen);
        QwtSymbol *sym = new QwtSymbol;
        sym->setStyle(QwtSymbol::Rect);
        sym->setPen(QPen(QColor(255,127,0))); // orange like a match, will make configurable later
        sym->setSize(4 *dpiXFactor);
        standard->mCurve->setSymbol(sym);
        QPen ihlPen = QPen(GColor(CINTERVALHIGHLIGHTER));
        ihlPen.setWidth(width);
        standard->intervalHighlighterCurve->setPen(QPen(Qt::NoPen));
        standard->intervalHoverCurve->setPen(QPen(Qt::NoPen));
        QColor ihlbrush = QColor(GColor(CINTERVALHIGHLIGHTER));
        ihlbrush.setAlpha(128);
        standard->intervalHighlighterCurve->setBrush(ihlbrush);   // fill below the line
        QColor hbrush = QColor(Qt::lightGray);
        hbrush.setAlpha(64);
        standard->intervalHoverCurve->setBrush(hbrush);   // fill below the line
        //this->legend()->remove(intervalHighlighterCurve); // don't show in legend
        QPen gridPen(GColor(CPLOTGRID));
        //gridPen.setStyle(Qt::DotLine); // solid line is nicer
        standard->grid->setPen(gridPen);

        // curve brushes
        if (fill) {

            for(int k=0; k<standard->U.count(); k++) {
                QColor p = standard->U[k].color;
                p.setAlpha(64);
                standard->U[k].curve->setBrush(QBrush(p));
            }

            QColor p;
            p = standard->wattsCurve->pen().color();
            p.setAlpha(64);
            standard->wattsCurve->setBrush(QBrush(p));

            p = standard->atissCurve->pen().color();
            p.setAlpha(64);
            standard->atissCurve->setBrush(QBrush(p));

            p = standard->antissCurve->pen().color();
            p.setAlpha(64);
            standard->antissCurve->setBrush(QBrush(p));

            p = standard->npCurve->pen().color();
            p.setAlpha(64);
            standard->npCurve->setBrush(QBrush(p));

            p = standard->rvCurve->pen().color();
            p.setAlpha(64);
            standard->rvCurve->setBrush(QBrush(p));

            p = standard->rcadCurve->pen().color();
            p.setAlpha(64);
            standard->rcadCurve->setBrush(QBrush(p));

            p = standard->rgctCurve->pen().color();
            p.setAlpha(64);
            standard->rgctCurve->setBrush(QBrush(p));

            p = standard->gearCurve->pen().color();
            p.setAlpha(64);
            standard->gearCurve->setBrush(QBrush(p));

            p = standard->smo2Curve->pen().color();
            p.setAlpha(64);
            standard->smo2Curve->setBrush(QBrush(p));

            p = standard->thbCurve->pen().color();
            p.setAlpha(64);
            standard->thbCurve->setBrush(QBrush(p));

            p = standard->o2hbCurve->pen().color();
            p.setAlpha(64);
            standard->o2hbCurve->setBrush(QBrush(p));

            p = standard->hhbCurve->pen().color();
            p.setAlpha(64);
            standard->hhbCurve->setBrush(QBrush(p));

            p = standard->xpCurve->pen().color();
            p.setAlpha(64);
            standard->xpCurve->setBrush(QBrush(p));

            p = standard->apCurve->pen().color();
            p.setAlpha(64);
            standard->apCurve->setBrush(QBrush(p));

            p = standard->wCurve->pen().color();
            p.setAlpha(64);
            standard->wCurve->setBrush(QBrush(p));

            p = standard->tcoreCurve->pen().color();
            p.setAlpha(64);
            standard->tcoreCurve->setBrush(QBrush(p));

            p = standard->hrCurve->pen().color();
            p.setAlpha(64);
            standard->hrCurve->setBrush(QBrush(p));

            p = standard->hrvCurve->pen().color();
            p.setAlpha(64);
            standard->hrvCurve->setBrush(QBrush(p));

            p = standard->accelCurve->pen().color();
            p.setAlpha(64);
            standard->accelCurve->setBrush(QBrush(p));

            p = standard->wattsDCurve->pen().color();
            p.setAlpha(64);
            standard->wattsDCurve->setBrush(QBrush(p));

            p = standard->cadDCurve->pen().color();
            p.setAlpha(64);
            standard->cadDCurve->setBrush(QBrush(p));

            p = standard->nmDCurve->pen().color();
            p.setAlpha(64);
            standard->nmDCurve->setBrush(QBrush(p));

            p = standard->hrDCurve->pen().color();
            p.setAlpha(64);
            standard->hrDCurve->setBrush(QBrush(p));

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

            p = standard->lteCurve->pen().color();
            p.setAlpha(64);
            standard->lteCurve->setBrush(QBrush(p));

            p = standard->rteCurve->pen().color();
            p.setAlpha(64);
            standard->rteCurve->setBrush(QBrush(p));

            p = standard->lpsCurve->pen().color();
            p.setAlpha(64);
            standard->lpsCurve->setBrush(QBrush(p));

            p = standard->rpsCurve->pen().color();
            p.setAlpha(64);
            standard->rpsCurve->setBrush(QBrush(p));

            p = standard->lpcoCurve->pen().color();
            p.setAlpha(64);
            standard->lpcoCurve->setBrush(QBrush(p));

            p = standard->rpcoCurve->pen().color();
            p.setAlpha(64);
            standard->rpcoCurve->setBrush(QBrush(p));

            p = standard->lppCurve->pen().color();
            p.setAlpha(64);
            standard->lppCurve->setBrush(QBrush(p));

            p = standard->rppCurve->pen().color();
            p.setAlpha(64);
            standard->rppCurve->setBrush(QBrush(p));

            p = standard->lpppCurve->pen().color();
            p.setAlpha(64);
            standard->lpppCurve->setBrush(QBrush(p));

            p = standard->rpppCurve->pen().color();
            p.setAlpha(64);
            standard->rpppCurve->setBrush(QBrush(p));

            p = standard->slopeCurve->pen().color();
            p.setAlpha(64);
            standard->slopeCurve->setBrush(QBrush(p));

            /*p = standard->altSlopeCurve->pen().color();
            p.setAlpha(64);
            standard->altSlopeCurve->setBrush(QBrush(p));

            p = standard->balanceLCurve->pen().color();
            p.setAlpha(64);
            standard->balanceLCurve->setBrush(QBrush(p));

            p = standard->balanceRCurve->pen().color();
            p.setAlpha(64);
            standard->balanceRCurve->setBrush(QBrush(p));*/
        } else {
            for(int k=0; k<standard->U.count(); k++) {
                standard->U[k].curve->setBrush(Qt::NoBrush);
            }
            standard->wattsCurve->setBrush(Qt::NoBrush);
            standard->atissCurve->setBrush(Qt::NoBrush);
            standard->antissCurve->setBrush(Qt::NoBrush);
            standard->rvCurve->setBrush(Qt::NoBrush);
            standard->rcadCurve->setBrush(Qt::NoBrush);
            standard->rgctCurve->setBrush(Qt::NoBrush);
            standard->gearCurve->setBrush(Qt::NoBrush);
            standard->smo2Curve->setBrush(Qt::NoBrush);
            standard->thbCurve->setBrush(Qt::NoBrush);
            standard->o2hbCurve->setBrush(Qt::NoBrush);
            standard->hhbCurve->setBrush(Qt::NoBrush);
            standard->npCurve->setBrush(Qt::NoBrush);
            standard->xpCurve->setBrush(Qt::NoBrush);
            standard->apCurve->setBrush(Qt::NoBrush);
            standard->wCurve->setBrush(Qt::NoBrush);
            standard->hrCurve->setBrush(Qt::NoBrush);
            standard->hrvCurve->setBrush(Qt::NoBrush);
            standard->tcoreCurve->setBrush(Qt::NoBrush);
            standard->speedCurve->setBrush(Qt::NoBrush);
            standard->accelCurve->setBrush(Qt::NoBrush);
            standard->wattsDCurve->setBrush(Qt::NoBrush);
            standard->cadDCurve->setBrush(Qt::NoBrush);
            standard->nmDCurve->setBrush(Qt::NoBrush);
            standard->hrDCurve->setBrush(Qt::NoBrush);
            standard->cadCurve->setBrush(Qt::NoBrush);
            standard->torqueCurve->setBrush(Qt::NoBrush);
            standard->tempCurve->setBrush(Qt::NoBrush);
            standard->lteCurve->setBrush(Qt::NoBrush);
            standard->rteCurve->setBrush(Qt::NoBrush);
            standard->lpsCurve->setBrush(Qt::NoBrush);
            standard->rpsCurve->setBrush(Qt::NoBrush);
            standard->lpcoCurve->setBrush(Qt::NoBrush);
            standard->rpcoCurve->setBrush(Qt::NoBrush);
            standard->lppCurve->setBrush(Qt::NoBrush);
            standard->rppCurve->setBrush(Qt::NoBrush);
            standard->lpppCurve->setBrush(Qt::NoBrush);
            standard->rpppCurve->setBrush(Qt::NoBrush);
            standard->slopeCurve->setBrush(Qt::NoBrush);
            //standard->altSlopeCurve->setBrush((Qt::NoBrush));
            //standard->balanceLCurve->setBrush(Qt::NoBrush);
            //standard->balanceRCurve->setBrush(Qt::NoBrush);
        }

        QPalette pal = palette();
        pal.setBrush(QPalette::Background, QBrush(GColor(CRIDEPLOTBACKGROUND)));
        setPalette(pal);

        // tick draw
        TimeScaleDraw *tsd = new TimeScaleDraw(&this->bydist, &timeoffset) ;
        tsd->setTickLength(QwtScaleDiv::MajorTick, 3);
        setAxisScaleDraw(QwtPlot::xBottom, tsd);
        pal.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
        pal.setColor(QPalette::Text, GColor(CPLOTMARKER));
        axisWidget(QwtPlot::xBottom)->setPalette(pal);
        enableAxis(xBottom, true);
        setAxisVisible(xBottom, true);

        ScaleScaleDraw *sd = new ScaleScaleDraw;
        sd->setTickLength(QwtScaleDiv::MajorTick, 3);
        sd->enableComponent(ScaleScaleDraw::Ticks, false);
        sd->enableComponent(ScaleScaleDraw::Backbone, false);
        setAxisScaleDraw(QwtPlot::yLeft, sd);
        pal.setColor(QPalette::WindowText, GColor(CPOWER));
        pal.setColor(QPalette::Text, GColor(CPOWER));
        axisWidget(QwtPlot::yLeft)->setPalette(pal);

        // some axis show multiple things so color them 
        // to match up if only one curve is selected; 
        // e.g. left, 1 typically has HR, Cadence
        // on the same curve but can also have SmO2 and Temp
        // since it gets set a few places we do it with
        // a special method
        setLeftOnePalette();
        setRightPalette(); 

        sd = new ScaleScaleDraw;
        sd->setTickLength(QwtScaleDiv::MajorTick, 3);
        sd->enableComponent(ScaleScaleDraw::Ticks, false);
        sd->enableComponent(ScaleScaleDraw::Backbone, false);
        setAxisScaleDraw(QwtAxisId(QwtAxis::yLeft, 3), sd);
        pal.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
        pal.setColor(QPalette::Text, GColor(CPLOTMARKER));
        axisWidget(QwtAxisId(QwtAxis::yLeft, 3))->setPalette(pal);

        sd = new ScaleScaleDraw;
        sd->setTickLength(QwtScaleDiv::MajorTick, 3);
        sd->enableComponent(ScaleScaleDraw::Ticks, false);
        sd->enableComponent(ScaleScaleDraw::Backbone, false);
        setAxisScaleDraw(QwtAxisId(QwtAxis::yRight, 1), sd);
        pal.setColor(QPalette::WindowText, GColor(CALTITUDE));
        pal.setColor(QPalette::Text, GColor(CALTITUDE));
        axisWidget(QwtAxisId(QwtAxis::yRight, 1))->setPalette(pal);

        sd = new ScaleScaleDraw;
        sd->enableComponent(ScaleScaleDraw::Ticks, false);
        sd->enableComponent(ScaleScaleDraw::Backbone, false);
        sd->setTickLength(QwtScaleDiv::MajorTick, 3);
        sd->setFactor(0.001f); // in kJ
        setAxisScaleDraw(QwtAxisId(QwtAxis::yRight, 2), sd);
        pal.setColor(QPalette::WindowText, GColor(CWBAL));
        pal.setColor(QPalette::Text, GColor(CWBAL));
        axisWidget(QwtAxisId(QwtAxis::yRight, 2))->setPalette(pal);

        sd = new ScaleScaleDraw;
        sd->enableComponent(ScaleScaleDraw::Ticks, false);
        sd->enableComponent(ScaleScaleDraw::Backbone, false);
        sd->setTickLength(QwtScaleDiv::MajorTick, 3);
        setAxisScaleDraw(QwtAxisId(QwtAxis::yRight, 3), sd);
        pal.setColor(QPalette::WindowText, GColor(CATISS));
        pal.setColor(QPalette::Text, GColor(CATISS));
        axisWidget(QwtAxisId(QwtAxis::yRight, 3))->setPalette(pal);

        curveColors->saveState();

    }
}

void
AllPlot::setLeftOnePalette()
{
    // always use the last, so BPM overrides
    // Cadence then Temp then SmO2 ...
    QColor single = QColor(Qt::red);
    if (standard->smo2Curve->isVisible()) {
        single = GColor(CSMO2);
    }
    if (standard->tempCurve->isVisible() && !context->athlete->useMetricUnits) {
        single = GColor(CTEMP);
    }
    if (standard->cadCurve->isVisible()) {
        single = GColor(CCADENCE);
    }
    if (standard->hrCurve->isVisible()) {
        single = GColor(CHEARTRATE);
    }

    if (standard->hrvCurve->isVisible()) {
        single = GColor(CHEARTRATE);
    }

    if (standard->tcoreCurve->isVisible()) {
        single = GColor(CCORETEMP);
    }

    // lets go
    ScaleScaleDraw *sd = new ScaleScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(ScaleScaleDraw::Ticks, false);
    sd->enableComponent(ScaleScaleDraw::Backbone, false);
    setAxisScaleDraw(QwtAxisId(QwtAxis::yLeft, 1), sd);

    QPalette pal = palette();
    pal.setBrush(QPalette::Background, QBrush(GColor(CRIDEPLOTBACKGROUND)));
    pal.setColor(QPalette::WindowText, single);
    pal.setColor(QPalette::Text, single);

    // now work it out ....
    axisWidget(QwtAxisId(QwtAxis::yLeft, 1))->setPalette(pal);
}

void
AllPlot::setRightPalette()
{
    // always use the last, so BPM overrides
    // Cadence then Temp then SmO2 ...
    QColor single = QColor(Qt::green);
    if (standard->speedCurve->isVisible()) {
        single = GColor(CSPEED);
    }
    if (standard->tempCurve->isVisible() && context->athlete->useMetricUnits) {
        single = GColor(CTEMP);
    }
    if (standard->o2hbCurve->isVisible()) {
        single = GColor(CO2HB);
    }
    if (standard->hhbCurve->isVisible()) {
        single = GColor(CHHB);
    }
    if (standard->thbCurve->isVisible()) {
        single = GColor(CTHB);
    }
    if (standard->torqueCurve->isVisible()) {
        single = GColor(CTORQUE);
    }

    // lets go
    ScaleScaleDraw *sd = new ScaleScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(ScaleScaleDraw::Ticks, false);
    sd->enableComponent(ScaleScaleDraw::Backbone, false);
    sd->setDecimals(2);
    setAxisScaleDraw(QwtAxisId(QwtAxis::yRight, 0), sd);

    QPalette pal = palette();
    pal.setBrush(QPalette::Background, QBrush(GColor(CRIDEPLOTBACKGROUND)));
    pal.setColor(QPalette::WindowText, single);
    pal.setColor(QPalette::Text, single);

    // now work it out ....
    axisWidget(QwtAxisId(QwtAxis::yRight, 0))->setPalette(pal);
}

void 
AllPlot::setHighlightIntervals(bool state)
{
    if (state) {
        standard->intervalHighlighterCurve->attach(this);
        standard->intervalHoverCurve->attach(this);
    } else {
        standard->intervalHighlighterCurve->detach();
        standard->intervalHoverCurve->detach();
    }
}

struct DataPoint {

    double time, hr, watts, atiss, antiss, np, rv, rcad, rgct,
           smo2, thb, o2hb, hhb, ap, xp, speed, cad, 
           alt, temp, wind, torque, lrbalance, lte, rte, lps, rps,
           lpco, rpco, lppb, rppb, lppe, rppe, lpppb, rpppb, lpppe, rpppe,
           kphd, wattsd, cadd, nmd, hrd, slope, tcore;

    // user values
    QList<double>user;

    DataPoint(double t, double h, double w, double at, double an, double n, double rv, double rcad, double rgct,
              double smo2, double thb, double o2hb, double hhb, double l, double x, double s, double c,
              double a, double te, double wi, double tq, double lrb, double lte, double rte, double lps, double rps,
              double lpco, double rpco, double lppb, double rppb, double lppe, double rppe, double lpppb, double rpppb, double lpppe, double rpppe,
              double kphd, double wattsd, double cadd, double nmd, double hrd, double sl, double tcore, QList<double>user) :

              time(t), hr(h), watts(w), atiss(at), antiss(an), np(n), rv(rv), rcad(rcad), rgct(rgct),
              smo2(smo2), thb(thb), o2hb(o2hb), hhb(hhb), ap(l), xp(x), speed(s), cad(c),
              alt(a), temp(te), wind(wi), torque(tq), lrbalance(lrb), lte(lte), rte(rte), lps(lps), rps(rps),
              lpco(lpco), rpco(rpco), lppb(lppb), rppb(rppb), lppe(lppe), rppe(rppe), lpppb(lpppb), rpppb(rpppb), lpppe(lpppe), rpppe(rpppe),
              kphd(kphd), wattsd(wattsd), cadd(cadd), nmd(nmd), hrd(hrd), slope(sl), tcore(tcore), user(user) {}
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

    if (rideItem && context->athlete->zones(rideItem->isRun)) {

        int zone_range = context->athlete->zones(rideItem->isRun)->whichRange(rideItem->dateTime.date());

        // generate labels for existing zones
        if (zone_range >= 0) {
            int num_zones = context->athlete->zones(rideItem->isRun)->numZones(zone_range);
            for (int z = 0; z < num_zones; z ++) {
                AllPlotZoneLabel *label = new AllPlotZoneLabel(this, z);
                label->attach(this);
                zoneLabels.append(label);
            }
        }
    }
}

void
AllPlot::setMatchLabels(AllPlotObject *objects)
{
    // clear anyway
    foreach(QwtPlotMarker *p, objects->matchLabels) {
        p->detach();
        delete p;
    }
    objects->matchLabels.clear();

    // add new ones, but only if showW
    if (showW && objects->mCurve) {

        bool below = false;
        // zip through the matches and add a label
        for (size_t i=0; i<objects->mCurve->data()->size(); i++) {
            // mCurve->data()->sample(i);

            // Qwt uses its own text objects
            QwtText text(QString("%1").arg(objects->mCurve->data()->sample(i).y()/1000.00f, 4, 'f', 1));
            text.setFont(labelFont);
            text.setColor(QColor(255,127,0)); // supposed to be configurable !

            // make that mark -- always above with topN
            QwtPlotMarker *label = new QwtPlotMarker();
            label->setLabel(text);
            label->setValue(objects->mCurve->data()->sample(i).x(), objects->mCurve->data()->sample(i).y());
            label->setYAxis(objects->mCurve->yAxis());
            label->setSpacing(6 *dpiXFactor); // not px but by yaxis value !? mad.
            label->setLabelAlignment(below ? (Qt::AlignBottom | Qt::AlignCenter) :
                                             (Qt::AlignTop | Qt::AlignCenter));

            // and attach
            label->attach(this);
            objects->matchLabels << label;

            // toggle top / bottom
            below = !below;
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

    // skip null rides
    if (!rideItem || !rideItem->ride()) return;


    int rideTimeSecs = (int) ceil(objects->timeArray[objects->timeArray.count()-1]);
    if (rideTimeSecs > SECONDS_IN_A_WEEK) {

        // clear all the curves
        QwtArray<double> data;
        QVector<QwtIntervalSample> intData;

        objects->wCurve->setSamples(data,data);
        objects->mCurve->setSamples(data,data);
        setMatchLabels(objects);

        if (!objects->atissArray.empty()) objects->atissCurve->setSamples(data, data);
        if (!objects->antissArray.empty()) objects->antissCurve->setSamples(data, data);
        if (!objects->npArray.empty()) objects->npCurve->setSamples(data, data);
        if (!objects->rvArray.empty()) objects->rvCurve->setSamples(data, data);
        if (!objects->rcadArray.empty()) objects->rcadCurve->setSamples(data, data);
        if (!objects->rgctArray.empty()) objects->rgctCurve->setSamples(data, data);
        if (!objects->gearArray.empty()) objects->gearCurve->setSamples(data, data);
        if (!objects->smo2Array.empty()) objects->smo2Curve->setSamples(data, data);
        if (!objects->thbArray.empty()) objects->thbCurve->setSamples(data, data);
        if (!objects->o2hbArray.empty()) objects->o2hbCurve->setSamples(data, data);
        if (!objects->hhbArray.empty()) objects->hhbCurve->setSamples(data, data);
        if (!objects->xpArray.empty()) objects->xpCurve->setSamples(data, data);
        if (!objects->apArray.empty()) objects->apCurve->setSamples(data, data);
        if (!objects->wattsArray.empty()) objects->wattsCurve->setSamples(data, data);
        if (!objects->hrArray.empty()) objects->hrCurve->setSamples(data, data);
        if (!objects->tcoreArray.empty()) objects->tcoreCurve->setSamples(data, data);
        if (!objects->speedArray.empty()) objects->speedCurve->setSamples(data, data);

        // user data
        foreach(UserObject obj, objects->U) {
            if (!obj.array.empty()) obj.curve->setSamples(data,data);
        }

        // deltas
        if (!objects->accelArray.empty()) objects->accelCurve->setSamples(data, data);
        if (!objects->wattsDArray.empty()) objects->wattsDCurve->setSamples(data, data);
        if (!objects->cadDArray.empty()) objects->cadDCurve->setSamples(data, data);
        if (!objects->nmDArray.empty()) objects->nmDCurve->setSamples(data, data);
        if (!objects->hrDArray.empty()) objects->hrDCurve->setSamples(data, data);

        if (!objects->cadArray.empty()) objects->cadCurve->setSamples(data, data);
        if (!objects->altArray.empty()) {
            objects->altCurve->setSamples(data, data);
            objects->altSlopeCurve->setSamples(data, data);
        }
        if (!objects->slopeArray.empty()) objects->slopeCurve->setSamples(data, data);
        if (!objects->tempArray.empty()) objects->tempCurve->setSamples(data, data);
        if (!objects->windArray.empty()) objects->windCurve->setSamples(new QwtIntervalSeriesData(intData));
        if (!objects->torqueArray.empty()) objects->torqueCurve->setSamples(data, data);

        // left/right data
        if (!objects->balanceArray.empty()) {
            objects->balanceLCurve->setSamples(data, data);
            objects->balanceRCurve->setSamples(data, data);
        }
        if (!objects->lteArray.empty()) objects->lteCurve->setSamples(data, data);
        if (!objects->rteArray.empty()) objects->rteCurve->setSamples(data, data);
        if (!objects->lpsArray.empty()) objects->lpsCurve->setSamples(data, data);
        if (!objects->rpsArray.empty()) objects->rpsCurve->setSamples(data, data);
        if (!objects->lpcoArray.empty()) objects->lpcoCurve->setSamples(data, data);
        if (!objects->rpcoArray.empty()) objects->rpcoCurve->setSamples(data, data);
        if (!objects->lppbArray.empty()) objects->lppCurve->setSamples(new QwtIntervalSeriesData(intData));
        if (!objects->rppbArray.empty()) objects->rppCurve->setSamples(new QwtIntervalSeriesData(intData));;
        if (!objects->lpppbArray.empty()) objects->lpppCurve->setSamples(new QwtIntervalSeriesData(intData));
        if (!objects->rpppbArray.empty()) objects->rpppCurve->setSamples(new QwtIntervalSeriesData(intData));

        return;
    }

    // if recintsecs is longer than the smoothing, or equal to the smoothing there is no point in even trying
    int applysmooth = smooth <= rideItem->ride()->recIntSecs() ? 0 : smooth;

    // compare mode breaks
    if (context->isCompareIntervals && applysmooth == 0) applysmooth = 1;
    
    // we should only smooth the curves if objects->smoothed rate is greater than sample rate

    // user data totals
    QVector<double> utotals;
    utotals.resize(objects->U.count());
    utotals.fill(0);

    // Offset for timeOfDay
    if (context->isCompareIntervals || !bytimeofday)
        timeoffset = 0;
    else
        timeoffset = QTime(0, 0).secsTo(rideItem->ride()->startTime().time()) / 60.0;

    if (applysmooth > 0) {

        double totalWatts = 0.0;
        double totalNP = 0.0;
        double totalRCad = 0.0;
        double totalRV = 0.0;
        double totalRGCT = 0.0;
        double totalSmO2 = 0.0;
        double totaltHb = 0.0;
        double totalO2Hb = 0.0;
        double totalHHb = 0.0;
        double totalATISS = 0.0;
        double totalANTISS = 0.0;
        double totalXP = 0.0;
        double totalAP = 0.0;
        double totalHr = 0.0;
        double totalTcore = 0.0;
        double totalSpeed = 0.0;
        double totalAccel = 0.0;
        double totalWattsD = 0.0;
        double totalCadD = 0.0;
        double totalNmD = 0.0;
        double totalHrD = 0.0;
        double totalCad = 0.0;
        double totalDist = 0.0;
        double totalAlt = 0.0;
        double totalSlope = 0.0;
        double totalTemp = 0.0;
        double totalWind = 0.0;
        double totalTorque = 0.0;
        double totalBalance = 0.0;
        double totalLTE = 0.0;
        double totalRTE = 0.0;
        double totalLPS = 0.0;
        double totalRPS = 0.0;
        double totalLPCO = 0.0;
        double totalRPCO = 0.0;
        double totalLPPB = 0.0;
        double totalRPPB = 0.0;
        double totalLPPE = 0.0;
        double totalRPPE = 0.0;
        double totalLPPPB = 0.0;
        double totalRPPPB = 0.0;
        double totalLPPPE = 0.0;
        double totalRPPPE = 0.0;
        double currentGearRatio = 0.0;


        QList<DataPoint> list;

        objects->smoothWatts.resize(rideTimeSecs + 1);
        objects->smoothNP.resize(rideTimeSecs + 1);
        objects->smoothGear.resize(rideTimeSecs + 1);
        objects->smoothRV.resize(rideTimeSecs + 1);
        objects->smoothRCad.resize(rideTimeSecs + 1);
        objects->smoothRGCT.resize(rideTimeSecs + 1);
        objects->smoothSmO2.resize(rideTimeSecs + 1);
        objects->smoothtHb.resize(rideTimeSecs + 1);
        objects->smoothO2Hb.resize(rideTimeSecs + 1);
        objects->smoothHHb.resize(rideTimeSecs + 1);
        objects->smoothAT.resize(rideTimeSecs + 1);
        objects->smoothANT.resize(rideTimeSecs + 1);
        objects->smoothXP.resize(rideTimeSecs + 1);
        objects->smoothAP.resize(rideTimeSecs + 1);
        objects->smoothHr.resize(rideTimeSecs + 1);
        objects->smoothTcore.resize(rideTimeSecs + 1);
        objects->smoothSpeed.resize(rideTimeSecs + 1);
        objects->smoothAccel.resize(rideTimeSecs + 1);
        objects->smoothWattsD.resize(rideTimeSecs + 1);
        objects->smoothCadD.resize(rideTimeSecs + 1);
        objects->smoothNmD.resize(rideTimeSecs + 1);
        objects->smoothHrD.resize(rideTimeSecs + 1);
        objects->smoothCad.resize(rideTimeSecs + 1);
        objects->smoothTime.resize(rideTimeSecs + 1);
        objects->smoothDistance.resize(rideTimeSecs + 1);
        objects->smoothAltitude.resize(rideTimeSecs + 1);
        objects->smoothSlope.resize(rideTimeSecs + 1);
        objects->smoothTemp.resize(rideTimeSecs + 1);
        objects->smoothWind.resize(rideTimeSecs + 1);
        objects->smoothRelSpeed.resize(rideTimeSecs + 1);
        objects->smoothTorque.resize(rideTimeSecs + 1);
        objects->smoothBalanceL.resize(rideTimeSecs + 1);
        objects->smoothBalanceR.resize(rideTimeSecs + 1);
        objects->smoothLTE.resize(rideTimeSecs + 1);
        objects->smoothRTE.resize(rideTimeSecs + 1);
        objects->smoothLPS.resize(rideTimeSecs + 1);
        objects->smoothRPS.resize(rideTimeSecs + 1);
        objects->smoothLPCO.resize(rideTimeSecs + 1);
        objects->smoothRPCO.resize(rideTimeSecs + 1);
        objects->smoothLPP.resize(rideTimeSecs + 1);
        objects->smoothRPP.resize(rideTimeSecs + 1);
        objects->smoothLPPP.resize(rideTimeSecs + 1);
        objects->smoothRPPP.resize(rideTimeSecs + 1);
        for(int k=0; k<objects->U.count(); k++) {
            objects->U[k].smooth.resize(rideTimeSecs + 1);
        }

        // do the smoothing by calculating the average of the "applysmooth" values left
        // of the current data point - for points in time smaller than "applysmooth"
        // only the available datapoints left are used to build the average
        int i = 0;
        for (int secs = 0; secs <= rideTimeSecs; ++secs) {
            while ((i < objects->timeArray.count()) && (objects->timeArray[i] <= secs)) {

                // collect user data if its there
                QList<double> udata;
                for (int k=0; k<objects->U.count(); k++) udata << (objects->U[k].array.size() > i ? objects->U[k].array[i] : 0);

                // add to list
                DataPoint dp(objects->timeArray[i],
                             (!objects->hrArray.empty() ? objects->hrArray[i] : 0),
                             (!objects->wattsArray.empty() ? objects->wattsArray[i] : 0),
                             (!objects->atissArray.empty() ? objects->atissArray[i] : 0),
                             (!objects->antissArray.empty() ? objects->antissArray[i] : 0),
                             (!objects->npArray.empty() ? objects->npArray[i] : 0),
                             (!objects->rvArray.empty() ? objects->rvArray[i] : 0),
                             (!objects->rcadArray.empty() ? objects->rcadArray[i] : 0),
                             (!objects->rgctArray.empty() ? objects->rgctArray[i] : 0),
                             (!objects->smo2Array.empty() ? objects->smo2Array[i] : 0),
                             (!objects->thbArray.empty() ? objects->thbArray[i] : 0),
                             (!objects->o2hbArray.empty() ? objects->o2hbArray[i] : 0),
                             (!objects->hhbArray.empty() ? objects->hhbArray[i] : 0),
                             (!objects->apArray.empty() ? objects->apArray[i] : 0),
                             (!objects->xpArray.empty() ? objects->xpArray[i] : 0),
                             (!objects->speedArray.empty() ? objects->speedArray[i] : 0),
                             (!objects->cadArray.empty() ? objects->cadArray[i] : 0),
                             (!objects->altArray.empty() ? objects->altArray[i] : 0),
                             (!objects->tempArray.empty() ? objects->tempArray[i] : 0),
                             (!objects->windArray.empty() ? objects->windArray[i] : 0),
                             (!objects->torqueArray.empty() ? objects->torqueArray[i] : 0),
                             (!objects->balanceArray.empty() ? objects->balanceArray[i] : 0),
                             (!objects->lteArray.empty() ? objects->lteArray[i] : 0),
                             (!objects->rteArray.empty() ? objects->rteArray[i] : 0),
                             (!objects->lpsArray.empty() ? objects->lpsArray[i] : 0),
                             (!objects->rpsArray.empty() ? objects->rpsArray[i] : 0),
                             (!objects->lpcoArray.empty() ? objects->lpcoArray[i] : 0),
                             (!objects->rpcoArray.empty() ? objects->rpcoArray[i] : 0),
                             (!objects->lppbArray.empty() ? objects->lppbArray[i] : 0),
                             (!objects->rppbArray.empty() ? objects->rppbArray[i] : 0),
                             (!objects->lppeArray.empty() ? objects->lppeArray[i] : 0),
                             (!objects->rppeArray.empty() ? objects->rppeArray[i] : 0),
                             (!objects->lpppbArray.empty() ? objects->lpppbArray[i] : 0),
                             (!objects->rpppbArray.empty() ? objects->rpppbArray[i] : 0),
                             (!objects->lpppeArray.empty() ? objects->lpppeArray[i] : 0),
                             (!objects->rpppeArray.empty() ? objects->rpppeArray[i] : 0),
                             (!objects->accelArray.empty() ? objects->accelArray[i] : 0),
                             (!objects->wattsDArray.empty() ? objects->wattsDArray[i] : 0),
                             (!objects->cadDArray.empty() ? objects->cadDArray[i] : 0),
                             (!objects->nmDArray.empty() ? objects->nmDArray[i] : 0),
                             (!objects->hrDArray.empty() ? objects->hrDArray[i] : 0),
                             (!objects->slopeArray.empty() ? objects->slopeArray[i] : 0),
                             (!objects->tcoreArray.empty() ? objects->tcoreArray[i] : 0),
                            udata);

                // apend to list
                list.append(dp);

                // maintain totals
                if (!objects->wattsArray.empty()) totalWatts += objects->wattsArray[i];
                if (!objects->npArray.empty()) totalNP += objects->npArray[i];
                if (!objects->rvArray.empty()) totalRV += objects->rvArray[i];
                if (!objects->rcadArray.empty()) totalRCad += objects->rcadArray[i];
                if (!objects->rgctArray.empty()) totalRGCT += objects->rgctArray[i];
                if (!objects->smo2Array.empty()) totalSmO2 += objects->smo2Array[i];
                if (!objects->thbArray.empty()) totaltHb += objects->thbArray[i];
                if (!objects->o2hbArray.empty()) totalO2Hb += objects->o2hbArray[i];
                if (!objects->hhbArray.empty()) totalHHb += objects->hhbArray[i];
                if (!objects->atissArray.empty()) totalATISS += objects->atissArray[i];
                if (!objects->antissArray.empty()) totalANTISS += objects->antissArray[i];
                if (!objects->xpArray.empty()) totalXP += objects->xpArray[i];
                if (!objects->apArray.empty()) totalAP += objects->apArray[i];
                if (!objects->tcoreArray.empty()) totalTcore    += objects->tcoreArray[i];
                if (!objects->hrArray.empty()) totalHr    += objects->hrArray[i];
                if (!objects->accelArray.empty()) totalAccel += objects->accelArray[i];
                if (!objects->wattsDArray.empty()) totalWattsD += objects->wattsDArray[i];
                if (!objects->cadDArray.empty()) totalCadD += objects->cadDArray[i];
                if (!objects->nmDArray.empty()) totalNmD += objects->nmDArray[i];
                if (!objects->hrDArray.empty()) totalHrD += objects->hrDArray[i];
                if (!objects->speedArray.empty()) totalSpeed += objects->speedArray[i];
                if (!objects->cadArray.empty()) totalCad   += objects->cadArray[i];
                if (!objects->altArray.empty()) totalAlt   += objects->altArray[i];
                if (!objects->slopeArray.empty()) totalSlope   += objects->slopeArray[i];
                if (!objects->windArray.empty()) totalWind   += objects->windArray[i];
                if (!objects->torqueArray.empty()) totalTorque   += objects->torqueArray[i];
                if (!objects->tempArray.empty() ) {
                    if (objects->tempArray[i] == RideFile::NA) {
                        dp.temp = (i>0 && !list.empty()?list.back().temp:0.0);
                        totalTemp   += dp.temp;
                    }
                    else {
                        totalTemp   += objects->tempArray[i];
                    }
                }
                if (!objects->balanceArray.empty()) totalBalance   += (objects->balanceArray[i]>0?objects->balanceArray[i]:50);
                if (!objects->lteArray.empty()) totalLTE   += (objects->lteArray[i]>0?objects->lteArray[i]:0);
                if (!objects->rteArray.empty()) totalRTE   += (objects->rteArray[i]>0?objects->rteArray[i]:0);
                if (!objects->lpsArray.empty()) totalLPS   += (objects->lpsArray[i]>0?objects->lpsArray[i]:0);
                if (!objects->rpsArray.empty()) totalRPS   += (objects->rpsArray[i]>0?objects->rpsArray[i]:0);
                if (!objects->lpcoArray.empty()) totalLPCO   += objects->lpcoArray[i];
                if (!objects->rpcoArray.empty()) totalRPCO   += objects->rpcoArray[i];
                if (!objects->lppbArray.empty()) totalLPPB   += (objects->lppbArray[i]>0?objects->lppbArray[i]:0);
                if (!objects->rppbArray.empty()) totalRPPB   += (objects->rppbArray[i]>0?objects->rppbArray[i]:0);
                if (!objects->lppeArray.empty()) totalLPPE   += (objects->lppeArray[i]>0?objects->lppeArray[i]:0);
                if (!objects->rppeArray.empty()) totalRPPE   += (objects->rppeArray[i]>0?objects->rppeArray[i]:0);
                if (!objects->lpppbArray.empty()) totalLPPPB   += (objects->lpppbArray[i]>0?objects->lpppbArray[i]:0);
                if (!objects->rpppbArray.empty()) totalRPPPB   += (objects->rpppbArray[i]>0?objects->rpppbArray[i]:0);
                if (!objects->lpppeArray.empty()) totalLPPPE   += (objects->lpppeArray[i]>0?objects->lpppeArray[i]:0);
                if (!objects->rpppeArray.empty()) totalRPPPE   += (objects->rpppeArray[i]>0?objects->rpppeArray[i]:0);

                // set values which must not be smoothed
                if (!objects->gearArray.empty()) currentGearRatio = (objects->gearArray[i]>0?objects->gearArray[i]:0);
                totalDist   = objects->distanceArray[i];

                // totalise user data
                for(int k=0; k<utotals.count(); k++) utotals[k] += udata[k];

                ++i;
            }

            // remove data from before smoothing duration (er, really?)
            while (!list.empty() && (list.front().time < secs - applysmooth)) {
                DataPoint &dp = list.front();
                totalWatts -= dp.watts;
                totalNP -= dp.np;
                totalRV -= dp.rv;
                totalRCad -= dp.rcad;
                totalRGCT -= dp.rgct;
                totalSmO2 -= dp.smo2;
                totaltHb -= dp.thb;
                totalO2Hb -= dp.o2hb;
                totalHHb -= dp.hhb;
                totalATISS -= dp.atiss;
                totalANTISS -= dp.antiss;
                totalAP -= dp.ap;
                totalXP -= dp.xp;
                totalHr    -= dp.hr;
                totalTcore    -= dp.tcore;
                totalSpeed -= dp.speed;
                totalAccel -= dp.kphd;
                totalWattsD -= dp.wattsd;
                totalCadD -= dp.cadd;
                totalNmD -= dp.nmd;
                totalHrD -= dp.hrd;
                totalCad   -= dp.cad;
                totalAlt   -= dp.alt;
                totalSlope -= dp.slope;
                totalTemp   -= dp.temp;
                totalWind   -= dp.wind;
                totalTorque   -= dp.torque;
                totalLTE   -= dp.lte;
                totalRTE   -= dp.rte;
                totalLPS   -= dp.lps;
                totalRPS   -= dp.rps;
                totalLPCO  -= dp.lpco;
                totalRPCO  -= dp.rpco;
                totalLPPB   -= dp.lppb;
                totalRPPB   -= dp.rppb;
                totalLPPE   -= dp.lppe;
                totalRPPE   -= dp.rppe;
                totalLPPPB  -= dp.lpppb;
                totalRPPPB  -= dp.rpppb;
                totalLPPPE  -= dp.lpppe;
                totalRPPPE  -= dp.rpppe;
                totalBalance   -= (dp.lrbalance>0?dp.lrbalance:50);
                for(int k=0; k<utotals.count(); k++) utotals[k] -= dp.user[k];

                list.removeFirst();
            }

            // TODO: this is wrong.  We should do a weighted average over the
            // seconds represented by each point...
            if (list.empty()) {

                for (int k=0; k<objects->U.count(); k++) objects->U[k].smooth[secs] = 0.0;
                objects->smoothWatts[secs] = 0.0;
                objects->smoothNP[secs] = 0.0;
                objects->smoothRV[secs] = 0.0;
                objects->smoothRCad[secs] = 0.0;
                objects->smoothRGCT[secs] = 0.0;
                objects->smoothSmO2[secs] = 0.0;
                objects->smoothtHb[secs] = 0.0;
                objects->smoothO2Hb[secs] = 0.0;
                objects->smoothHHb[secs] = 0.0;
                objects->smoothAT[secs] = 0.0;
                objects->smoothANT[secs] = 0.0;
                objects->smoothXP[secs] = 0.0;
                objects->smoothAP[secs] = 0.0;
                objects->smoothHr[secs]    = 0.0;
                objects->smoothTcore[secs]    = 0.0;
                objects->smoothSpeed[secs] = 0.0;
                objects->smoothAccel[secs] = 0.0;
                objects->smoothWattsD[secs] = 0.0;
                objects->smoothCadD[secs] = 0.0;
                objects->smoothNmD[secs] = 0.0;
                objects->smoothHrD[secs] = 0.0;
                objects->smoothCad[secs]   = 0.0;
                objects->smoothAltitude[secs]   = ((secs > 0) ? objects->smoothAltitude[secs - 1] : objects->altArray[secs] ) ;
                objects->smoothSlope[secs]   =  0.0;
                objects->smoothTemp[secs]   = 0.0;
                objects->smoothWind[secs] = 0.0;
                objects->smoothRelSpeed[secs] =  QwtIntervalSample();
                objects->smoothTorque[secs] = 0.0;
                objects->smoothLTE[secs] = 0.0;
                objects->smoothRTE[secs] = 0.0;
                objects->smoothLPS[secs] = 0.0;
                objects->smoothRPS[secs] = 0.0;
                objects->smoothLPCO[secs] = 0.0;
                objects->smoothRPCO[secs] = 0.0;
                objects->smoothLPP[secs] = QwtIntervalSample();
                objects->smoothRPP[secs] = QwtIntervalSample();
                objects->smoothLPPP[secs] = QwtIntervalSample();
                objects->smoothRPPP[secs] = QwtIntervalSample();
                objects->smoothBalanceL[secs] = 50;
                objects->smoothBalanceR[secs] = 50;

            } else {

                for(int k=0; k<utotals.count(); k++) objects->U[k].smooth[secs] = utotals[k] / list.size();
                objects->smoothWatts[secs]    = totalWatts / list.size();
                objects->smoothNP[secs]    = totalNP / list.size();
                objects->smoothRV[secs]    = totalRV / list.size();
                objects->smoothRCad[secs]    = totalRCad / list.size();
                objects->smoothRGCT[secs]    = totalRGCT / list.size();
                objects->smoothSmO2[secs]    = totalSmO2 / list.size();
                objects->smoothtHb[secs]    = totaltHb / list.size();
                objects->smoothO2Hb[secs]    = totalO2Hb / list.size();
                objects->smoothHHb[secs]    = totalHHb / list.size();
                objects->smoothAT[secs]    = totalATISS / list.size();
                objects->smoothANT[secs]    = totalANTISS / list.size();
                objects->smoothXP[secs]    = totalXP / list.size();
                objects->smoothAP[secs]    = totalAP / list.size();
                objects->smoothHr[secs]       = totalHr / list.size();
                objects->smoothTcore[secs]       = totalTcore / list.size();
                objects->smoothSpeed[secs]    = totalSpeed / list.size();
                objects->smoothAccel[secs]    = totalAccel / double(list.size());
                objects->smoothWattsD[secs]    = totalWattsD / double(list.size());
                objects->smoothCadD[secs]    = totalCadD / double(list.size());
                objects->smoothNmD[secs]    = totalNmD / double(list.size());
                objects->smoothHrD[secs]    = totalHrD / double(list.size());
                objects->smoothCad[secs]      = totalCad / list.size();
                objects->smoothAltitude[secs]      = totalAlt / list.size();
                objects->smoothSlope[secs]      = totalSlope / double(list.size());
                objects->smoothTemp[secs]      = totalTemp / list.size();
                objects->smoothWind[secs]    = totalWind / list.size();
                objects->smoothTorque[secs]    = totalTorque / list.size();
                objects->smoothRelSpeed[secs] =  QwtIntervalSample(bydist ? totalDist : secs / 60.0, 
                                                                   QwtInterval(qMin(totalWind / list.size(),
                                                                   totalSpeed / list.size()), 
                                                                   qMax(totalWind / list.size(), 
                                                                   totalSpeed / list.size())));

                // left /right pedal data
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
                objects->smoothLTE[secs]    = totalLTE / list.size();
                objects->smoothRTE[secs]    = totalRTE / list.size();
                objects->smoothLPS[secs]    = totalLPS / list.size();
                objects->smoothRPS[secs]    = totalRPS / list.size();
                objects->smoothLPCO[secs]   = totalLPCO / list.size();
                objects->smoothRPCO[secs]   = totalRPCO / list.size();
                objects->smoothLPP[secs]    = QwtIntervalSample( bydist ? totalDist : secs / 60.0, QwtInterval(totalLPPB / list.size(), totalLPPE / list.size() ) );
                objects->smoothRPP[secs]    = QwtIntervalSample( bydist ? totalDist : secs / 60.0, QwtInterval(totalRPPB / list.size(), totalRPPE / list.size() ) );
                objects->smoothLPPP[secs]   = QwtIntervalSample( bydist ? totalDist : secs / 60.0, QwtInterval(totalLPPPB / list.size(), totalLPPPE / list.size() ) );
                objects->smoothRPPP[secs]   = QwtIntervalSample( bydist ? totalDist : secs / 60.0, QwtInterval(totalRPPPB / list.size(), totalRPPPE / list.size() ) );
            }
            objects->smoothGear[secs] = currentGearRatio;
            objects->smoothDistance[secs] = totalDist;
            objects->smoothTime[secs]  =  secs / 60.0;
        }

    } else {

        // no standard->smoothing .. just raw data
        for (int k=0; k<objects->U.count(); k++) objects->U[k].smooth.resize(0);
        objects->smoothWatts.resize(0);
        objects->smoothNP.resize(0);
        objects->smoothGear.resize(0);
        objects->smoothRV.resize(0);
        objects->smoothRCad.resize(0);
        objects->smoothRGCT.resize(0);
        objects->smoothSmO2.resize(0);
        objects->smoothtHb.resize(0);
        objects->smoothO2Hb.resize(0);
        objects->smoothHHb.resize(0);
        objects->smoothAT.resize(0);
        objects->smoothANT.resize(0);
        objects->smoothXP.resize(0);
        objects->smoothAP.resize(0);
        objects->smoothHr.resize(0);
        objects->smoothTcore.resize(0);
        objects->smoothSpeed.resize(0);
        objects->smoothAccel.resize(0);
        objects->smoothWattsD.resize(0);
        objects->smoothCadD.resize(0);
        objects->smoothNmD.resize(0);
        objects->smoothHrD.resize(0);
        objects->smoothCad.resize(0);
        objects->smoothTime.resize(0);
        objects->smoothDistance.resize(0);
        objects->smoothAltitude.resize(0);
        objects->smoothSlope.resize(0);
        objects->smoothTemp.resize(0);
        objects->smoothWind.resize(0);
        objects->smoothRelSpeed.resize(0);
        objects->smoothTorque.resize(0);
        objects->smoothLTE.resize(0);
        objects->smoothRTE.resize(0);
        objects->smoothLPS.resize(0);
        objects->smoothRPS.resize(0);
        objects->smoothLPCO.resize(0);
        objects->smoothRPCO.resize(0);
        objects->smoothLPP.resize(0);
        objects->smoothRPP.resize(0);
        objects->smoothLPPP.resize(0);
        objects->smoothRPPP.resize(0);
        objects->smoothBalanceL.resize(0);
        objects->smoothBalanceR.resize(0);

        // fill with raw data
        for (int k=0; k<objects->U.count(); k++) objects->U[k].smooth = objects->U[k].array;
        foreach (RideFilePoint *dp, rideItem->ride()->dataPoints()) {
            objects->smoothWatts.append(dp->watts);
            objects->smoothNP.append(dp->np);
            objects->smoothRV.append(dp->rvert);
            objects->smoothRCad.append(dp->rcad);
            objects->smoothRGCT.append(dp->rcontact);
            objects->smoothGear.append(dp->gear);
            objects->smoothSmO2.append(dp->smo2);
            objects->smoothtHb.append(dp->thb);
            objects->smoothO2Hb.append(dp->o2hb);
            objects->smoothHHb.append(dp->hhb);
            objects->smoothAT.append(dp->atiss);
            objects->smoothANT.append(dp->antiss);
            objects->smoothXP.append(dp->xp);
            objects->smoothAP.append(dp->apower);
            objects->smoothHr.append(dp->hr);
            objects->smoothTcore.append(dp->tcore);
            objects->smoothSpeed.append(context->athlete->useMetricUnits ? dp->kph : dp->kph * MILES_PER_KM);
            objects->smoothAccel.append(dp->kphd);
            objects->smoothWattsD.append(dp->wattsd);
            objects->smoothCadD.append(dp->cadd);
            objects->smoothNmD.append(dp->nmd);
            objects->smoothHrD.append(dp->hrd);
            objects->smoothCad.append(dp->cad);
            objects->smoothTime.append(dp->secs/60);
            objects->smoothDistance.append(context->athlete->useMetricUnits ? dp->km : dp->km * MILES_PER_KM);
            objects->smoothAltitude.append(context->athlete->useMetricUnits ? dp->alt : dp->alt * FEET_PER_METER);
            objects->smoothSlope.append(dp->slope);
            if (dp->temp == RideFile::NA && !objects->smoothTemp.empty())
                dp->temp = objects->smoothTemp.last();
            objects->smoothTemp.append(context->athlete->useMetricUnits ? dp->temp : dp->temp * FAHRENHEIT_PER_CENTIGRADE + FAHRENHEIT_ADD_CENTIGRADE);
            objects->smoothWind.append(context->athlete->useMetricUnits ? dp->headwind : dp->headwind * MILES_PER_KM);
            objects->smoothTorque.append(dp->nm);

            if (dp->lrbalance == RideFile::NA || (dp->lrbalance == 0)) {
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
            objects->smoothLTE.append(dp->lte);
            objects->smoothRTE.append(dp->rte);
            objects->smoothLPS.append(dp->lps);
            objects->smoothRPS.append(dp->rps);
            objects->smoothLPCO.append(dp->lpco);
            objects->smoothRPCO.append(dp->rpco);
            objects->smoothLPP.append(QwtIntervalSample( bydist ? objects->smoothDistance.last() : objects->smoothTime.last(), QwtInterval(dp->lppb , dp->rppe ) ));
            objects->smoothRPP.append(QwtIntervalSample( bydist ? objects->smoothDistance.last() : objects->smoothTime.last(), QwtInterval(dp->rppb , dp->rppe ) ));
            objects->smoothLPPP.append(QwtIntervalSample( bydist ? objects->smoothDistance.last() : objects->smoothTime.last(), QwtInterval(dp->lpppb , dp->lpppe ) ));
            objects->smoothRPPP.append(QwtIntervalSample( bydist ? objects->smoothDistance.last() : objects->smoothTime.last(), QwtInterval(dp->rpppb , dp->rpppe ) ));


            double head = dp->headwind * (context->athlete->useMetricUnits ? 1.0f : MILES_PER_KM);
            double speed = dp->kph * (context->athlete->useMetricUnits ? 1.0f : MILES_PER_KM);
            objects->smoothRelSpeed.append(QwtIntervalSample( bydist ? objects->smoothDistance.last() : objects->smoothTime.last(), QwtInterval(qMin(head, speed) , qMax(head, speed) ) ));

        }
    }

    QVector<double> &xaxis = bydist ? objects->smoothDistance : objects->smoothTime;
    int startingIndex = qMin(smooth, xaxis.count());
    int totalPoints = xaxis.count() - startingIndex;

    // Build vectors for HRV
    if (rideItem->ride()->areDataPresent()->hrv)
        {
            objects->smoothHrv.resize(0);
            objects->smoothHrv_time.resize(0);

            XDataSeries *series = rideItem->ride()->xdata("HRV");

            if (series)
                {
                    foreach(XDataPoint *p, series->datapoints)
                        {
                            if (applysmooth==0 || p->number[1]>0)
                                {
                                    objects->smoothHrv_time.append(p->secs/60.0);
                                    objects->smoothHrv.append(p->number[0]/1000.0);
                                }
                        }
                }
            objects->hrvCurve->setSamples(objects->smoothHrv_time.data(),
                                          objects->smoothHrv.data(),
                                          objects->smoothHrv.count());
        }
    // set curves - we set the intervalHighlighter to whichver is available

    //W' curve set to whatever data we have
    if (!objects->wprime.empty()) {
        objects->wCurve->setSamples(bydist ? objects->wprimeDist.data() : objects->wprimeTime.data(), 
                                    objects->wprime.data(), objects->wprime.count());
        objects->mCurve->setSamples(bydist ? objects->matchDist.data() : objects->matchTime.data(), 
                                    objects->match.data(), objects->match.count());
        setMatchLabels(objects);
    }

    // set curve.
    for(int k=0; k<objects->U.count(); k++) {
        if (!objects->U[k].array.empty()) {
            objects->U[k].curve->setSamples(xaxis.data() + startingIndex, objects->U[k].smooth.data() + startingIndex, totalPoints);
        }
    }

    if (!objects->wattsArray.empty()) {
        objects->wattsCurve->setSamples(xaxis.data() + startingIndex, objects->smoothWatts.data() + startingIndex, totalPoints);
    }

    if (!objects->antissArray.empty()) {
        objects->antissCurve->setSamples(xaxis.data() + startingIndex, objects->smoothANT.data() + startingIndex, totalPoints);
    }

    if (!objects->atissArray.empty()) {
        objects->atissCurve->setSamples(xaxis.data() + startingIndex, objects->smoothAT.data() + startingIndex, totalPoints);
    }

    if (!objects->rvArray.empty()) {
        objects->rvCurve->setSamples(xaxis.data() + startingIndex, objects->smoothRV.data() + startingIndex, totalPoints);
    }

    if (!objects->rcadArray.empty()) {
        objects->rcadCurve->setSamples(xaxis.data() + startingIndex, objects->smoothRCad.data() + startingIndex, totalPoints);
    }

    if (!objects->rgctArray.empty()) {
        objects->rgctCurve->setSamples(xaxis.data() + startingIndex, objects->smoothRGCT.data() + startingIndex, totalPoints);
    }

    if (!objects->gearArray.empty()) {
        objects->gearCurve->setSamples(xaxis.data() + startingIndex, objects->smoothGear.data() + startingIndex, totalPoints);
    }

    if (!objects->smo2Array.empty()) {
        objects->smo2Curve->setSamples(xaxis.data() + startingIndex, objects->smoothSmO2.data() + startingIndex, totalPoints);
    }

    if (!objects->thbArray.empty()) {
        objects->thbCurve->setSamples(xaxis.data() + startingIndex, objects->smoothtHb.data() + startingIndex, totalPoints);
    }

    if (!objects->o2hbArray.empty()) {
        objects->o2hbCurve->setSamples(xaxis.data() + startingIndex, objects->smoothO2Hb.data() + startingIndex, totalPoints);
    }

    if (!objects->hhbArray.empty()) {
        objects->hhbCurve->setSamples(xaxis.data() + startingIndex, objects->smoothHHb.data() + startingIndex, totalPoints);
    }

    if (!objects->npArray.empty()) {
        objects->npCurve->setSamples(xaxis.data() + startingIndex, objects->smoothNP.data() + startingIndex, totalPoints);
    }

    if (!objects->xpArray.empty()) {
        objects->xpCurve->setSamples(xaxis.data() + startingIndex, objects->smoothXP.data() + startingIndex, totalPoints);
    }

    if (!objects->apArray.empty()) {
        objects->apCurve->setSamples(xaxis.data() + startingIndex, objects->smoothAP.data() + startingIndex, totalPoints);
    }

    if (!objects->hrArray.empty()) {
        objects->hrCurve->setSamples(xaxis.data() + startingIndex, objects->smoothHr.data() + startingIndex, totalPoints);
    }

    if (!objects->tcoreArray.empty()) {
        objects->tcoreCurve->setSamples(xaxis.data() + startingIndex, objects->smoothTcore.data() + startingIndex, totalPoints);
    }

    if (!objects->speedArray.empty()) {
        objects->speedCurve->setSamples(xaxis.data() + startingIndex, objects->smoothSpeed.data() + startingIndex, totalPoints);
    }

    if (!objects->accelArray.empty()) {
        objects->accelCurve->setSamples(xaxis.data() + startingIndex, objects->smoothAccel.data() + startingIndex, totalPoints);
    }

    if (!objects->wattsDArray.empty()) {
        objects->wattsDCurve->setSamples(xaxis.data() + startingIndex, objects->smoothWattsD.data() + startingIndex, totalPoints);
    }

    if (!objects->cadDArray.empty()) {
        objects->cadDCurve->setSamples(xaxis.data() + startingIndex, objects->smoothCadD.data() + startingIndex, totalPoints);
    }

    if (!objects->nmDArray.empty()) {
        objects->nmDCurve->setSamples(xaxis.data() + startingIndex, objects->smoothNmD.data() + startingIndex, totalPoints);
    }

    if (!objects->hrDArray.empty()) {
        objects->hrDCurve->setSamples(xaxis.data() + startingIndex, objects->smoothHrD.data() + startingIndex, totalPoints);
    }

    if (!objects->cadArray.empty()) {
        objects->cadCurve->setSamples(xaxis.data() + startingIndex, objects->smoothCad.data() + startingIndex, totalPoints);
    }

    if (!objects->altArray.empty()) {
        objects->altCurve->setSamples(xaxis.data() + startingIndex, objects->smoothAltitude.data() + startingIndex, totalPoints);
        objects->altSlopeCurve->setSamples(xaxis.data() + startingIndex, objects->smoothAltitude.data() + startingIndex, totalPoints);
    }
    if (!objects->slopeArray.empty()) {
        objects->slopeCurve->setSamples(xaxis.data() + startingIndex, objects->smoothSlope.data() + startingIndex, totalPoints);
    }

    if (!objects->tempArray.empty()) {
        objects->tempCurve->setSamples(xaxis.data() + startingIndex, objects->smoothTemp.data() + startingIndex, totalPoints);
    }


    if (!objects->windArray.empty()) {
        objects->windCurve->setSamples(new QwtIntervalSeriesData(objects->smoothRelSpeed));
    }

    if (!objects->torqueArray.empty()) {
        objects->torqueCurve->setSamples(xaxis.data() + startingIndex, objects->smoothTorque.data() + startingIndex, totalPoints);
    }

    // left/right pedals
    if (!objects->balanceArray.empty()) {
        objects->balanceLCurve->setSamples(xaxis.data() + startingIndex, 
                                           objects->smoothBalanceL.data() + startingIndex, totalPoints);
        objects->balanceRCurve->setSamples(xaxis.data() + startingIndex, 
                                           objects->smoothBalanceR.data() + startingIndex, totalPoints);
    }
    if (!objects->lteArray.empty()) objects->lteCurve->setSamples(xaxis.data() + startingIndex, 
                                             objects->smoothLTE.data() + startingIndex, totalPoints);
    if (!objects->rteArray.empty()) objects->rteCurve->setSamples(xaxis.data() + startingIndex, 
                                             objects->smoothRTE.data() + startingIndex, totalPoints);
    if (!objects->lpsArray.empty()) objects->lpsCurve->setSamples(xaxis.data() + startingIndex, 
                                             objects->smoothLPS.data() + startingIndex, totalPoints);
    if (!objects->rpsArray.empty()) objects->rpsCurve->setSamples(xaxis.data() + startingIndex, 
                                             objects->smoothRPS.data() + startingIndex, totalPoints);

    if (!objects->lpcoArray.empty()) objects->lpcoCurve->setSamples(xaxis.data() + startingIndex,
                                             objects->smoothLPCO.data() + startingIndex, totalPoints);
    if (!objects->rpcoArray.empty()) objects->rpcoCurve->setSamples(xaxis.data() + startingIndex,
                                             objects->smoothRPCO.data() + startingIndex, totalPoints);
    if (!objects->lppbArray.empty()) {
        objects->lppCurve->setSamples(new QwtIntervalSeriesData(objects->smoothLPP));
    }
    if (!objects->rppbArray.empty()) {
        objects->rppCurve->setSamples(new QwtIntervalSeriesData(objects->smoothRPP));
    }
    if (!objects->lpppbArray.empty()) {
        objects->lpppCurve->setSamples(new QwtIntervalSeriesData(objects->smoothLPPP));
    }
    if (!objects->rpppbArray.empty()) {
        objects->rpppCurve->setSamples(new QwtIntervalSeriesData(objects->smoothRPPP));
    }

    setAltSlopePlotStyle(objects->altSlopeCurve);
    setYMax();

    if (!context->isCompareIntervals) {
        refreshReferenceLines();
        refreshExhaustions();
        refreshIntervalMarkers();
        refreshCalibrationMarkers();
        refreshZoneLabels();
    }

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();

    replot();
}

void
AllPlot::refreshIntervalMarkers()
{
    QwtIndPlotMarker::resetDrawnLabels();

    foreach(QwtPlotMarker *mrk, standard->d_mrk) {
        mrk->detach();
        delete mrk;
    }
    standard->d_mrk.clear();
    if (rideItem && rideItem->ride()) {
        foreach(IntervalItem *interval, rideItem->intervals()) {

            bool nolabel = false;

            // no label, but do add
            if (interval->type != RideFileInterval::USER) nolabel = true;

            QwtPlotMarker *mrk = new QwtIndPlotMarker;
            standard->d_mrk.append(mrk);
            mrk->attach(this);
            mrk->setLineStyle(QwtPlotMarker::VLine);
            mrk->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);

            if (nolabel) mrk->setLinePen(QPen(QColor(127,127,127,64), 0, Qt::DashLine));
            else mrk->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashLine));

            // put matches on second line down
            QString name(interval->name);
            if (interval->name.startsWith(tr("Match"))) name = QString("\n%1").arg(interval->name);

            QwtText text(wanttext && !nolabel ? name : "");

            if (!nolabel) {
                text.setFont(QFont("Helvetica", 10, QFont::Bold));
                if (interval->name.startsWith(tr("Match"))) 
                    text.setColor(GColor(CWBAL));
                else
                    text.setColor(GColor(CPLOTMARKER));
            }

            if (!bydist) {
                mrk->setValue(interval->start / 60.0, 0.0);
            } else
                mrk->setValue((context->athlete->useMetricUnits ? 1 : MILES_PER_KM) *
                                interval->startKM, 0.0);
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

    // only on power based charts
    if (scope != RideFile::none && scope != RideFile::watts && scope != RideFile::aTISS && scope != RideFile::anTISS &&
        scope != RideFile::IsoPower && scope != RideFile::aPower && scope != RideFile::xPower) return;

    QColor color = GColor(CPOWER);
    color.setAlpha(15); // almost invisible !

    if (rideItem && rideItem->ride()) {
        foreach(const RideFileCalibration *calibration, rideItem->ride()->calibrations()) {
            QwtPlotMarker *mrk = new QwtPlotMarker;
            standard->cal_mrk.append(mrk);
            mrk->attach(this);
            mrk->setLineStyle(QwtPlotMarker::VLine);
            mrk->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
            mrk->setLinePen(QPen(color, 0, Qt::SolidLine));
            if (!bydist)
                mrk->setValue(calibration->start / 60.0, 0.0);
            else
                mrk->setValue((context->athlete->useMetricUnits ? 1 : MILES_PER_KM) *
                                rideItem->ride()->timeToDistance(calibration->start), 0.0);

            //Lots of markers can clutter things, so avoid texts for now
            //QwtText text(false ? ("\n\n"+calibration.name) : "");
            //text.setFont(QFont("Helvetica", 9, QFont::Bold));
            //text.setColor(Qt::gray);
            //mrk->setLabel(text);
        }
    }
}

void
AllPlot::refreshReferenceLines()
{
    // not supported in compare mode
    if (context->isCompareIntervals) return;

    // power or hr
    if (scope != RideFile::none && scope != RideFile::watts && scope != RideFile::aTISS && scope != RideFile::hr &&
        scope != RideFile::anTISS && scope != RideFile::IsoPower && scope != RideFile::aPower && scope != RideFile::xPower) return;

    foreach(QwtPlotCurve *referenceLine, standard->referenceLines) {
        curveColors->remove(referenceLine);
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

void
AllPlot::refreshExhaustions()
{
    // not supported in compare mode
    if (context->isCompareIntervals) return;

    foreach(QwtPlotMarker *marker, standard->exhaustionLines) {
        marker->detach();
        delete marker;
    }
    standard->exhaustionLines.clear();

    if (rideItem && rideItem->ride()) {
        foreach(const RideFilePoint *referencePoint, rideItem->ride()->referencePoints()) {
            if (referencePoint->secs) {
                QwtPlotMarker *marker = plotExhaustionLine(referencePoint->secs);
                if (marker) standard->exhaustionLines.append(marker);
            }
        }
    }
}

QwtPlotCurve*
AllPlot::plotReferenceLine(const RideFilePoint *referencePoint)
{
    // not supported in compare mode
    if (context->isCompareIntervals) return NULL;

    // power on power plots
    if (referencePoint->watts != 0 && scope != RideFile::none && scope != RideFile::watts && scope != RideFile::aTISS && 
        scope != RideFile::anTISS && scope != RideFile::IsoPower && scope != RideFile::aPower && scope != RideFile::xPower) return NULL;

    // hr on hr plots
    if (referencePoint->hr !=0 && scope != RideFile::none && scope != RideFile::hr) return NULL;

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
        if (scope == RideFile::none) referenceLine->setYAxis(QwtAxisId(QwtAxis::yLeft,1));
        else if (scope == RideFile::hr) referenceLine->setYAxis(QwtAxisId(QwtAxis::yLeft));
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
        curveColors->insert(referenceLine);
        referenceLine->setSamples(xaxis,yaxis);
        referenceLine->attach(this);
        referenceLine->setVisible(true);
    }

    return referenceLine;
}

void
AllPlot::replot() {
        QwtIndPlotMarker::resetDrawnLabels();
        QwtPlot::replot();
    }

void
AllPlot::setAxisScaleDiv(const QwtAxisId& axis, double min, double max, double step) {
    int maxMajorTicks = 0;
    int maxMinorTicks = 0;
    QwtPlot::setAxisScaleDiv(axis, 
        QwtLinearScaleEngine().divideScale(
            min, max, maxMajorTicks, maxMinorTicks, step)
    );
}

void
AllPlot::setYMax()
{
    // first lets show or hide, otherwise all the efforts to set scales
    // etc are ignored because the axis is not visible
    if (wantaxis) {

        // hide user data axis
        for(int k=0; k<standard->U.count(); k++) {
            setAxisVisible(QwtAxisId(QwtAxis::yRight, 4+k), standard->U[k].curve->isVisible());
        }

        setAxisVisible(yLeft, standard->wattsCurve->isVisible() || 
                              standard->atissCurve->isVisible() || 
                              standard->antissCurve->isVisible() || 
                              standard->npCurve->isVisible() || 
                              standard->rvCurve->isVisible() || 
                              standard->rcadCurve->isVisible() || 
                              standard->rgctCurve->isVisible() || 
                              standard->gearCurve->isVisible() || 
                              standard->xpCurve->isVisible() || 
                              standard->apCurve->isVisible());

        setAxisVisible(QwtAxisId(QwtAxis::yLeft, 1), standard->hrCurve->isVisible() || standard->tcoreCurve->isVisible() ||
                                                     standard->cadCurve->isVisible() || standard->smo2Curve->isVisible() || standard->hrvCurve->isVisible());
        setAxisVisible(QwtAxisId(QwtAxis::yLeft, 2), false);
        setAxisVisible(QwtAxisId(QwtAxis::yLeft, 3), standard->balanceLCurve->isVisible() ||
                                                     standard->lteCurve->isVisible() ||
                                                     standard->lpsCurve->isVisible()  ||
                                                     standard->slopeCurve->isVisible() );
        setAxisVisible(yRight, standard->speedCurve->isVisible() || standard->torqueCurve->isVisible() || 
                               standard->thbCurve->isVisible() || standard->o2hbCurve->isVisible() || standard->hhbCurve->isVisible());

        setAxisVisible(QwtAxisId(QwtAxis::yRight, 1), standard->altCurve->isVisible() ||
                                                      standard->altSlopeCurve->isVisible());
        setAxisVisible(QwtAxisId(QwtAxis::yRight, 2), standard->wCurve->isVisible());
        setAxisVisible(QwtAxisId(QwtAxis::yRight, 3), standard->atissCurve->isVisible() || standard->antissCurve->isVisible());

    } else {

        setAxisVisible(yLeft, false);
        setAxisVisible(QwtAxisId(QwtAxis::yLeft,1), false);
        setAxisVisible(QwtAxisId(QwtAxis::yLeft,2), false);
        setAxisVisible(QwtAxisId(QwtAxis::yLeft,3), false);
        setAxisVisible(yRight, false);
        setAxisVisible(QwtAxisId(QwtPlot::yRight,1), false);
        setAxisVisible(QwtAxisId(QwtAxis::yRight,2), false);
        setAxisVisible(QwtAxisId(QwtAxis::yRight,3), false);
        // hide user data axis
        for(int k=0; k<standard->U.count(); k++) {
            setAxisVisible(QwtAxisId(QwtAxis::yRight, 4+k), false);
        }
    }

    // might want xaxis
    if (wantxaxis) setAxisVisible(xBottom, true);
    else setAxisVisible(xBottom, false);

    // set axis scales
    // QwtAxis::yRight, 3
    if (((showATISS && standard->atissCurve->isVisible()) || (showANTISS && standard->antissCurve->isVisible()))
         && rideItem && rideItem->ride()) {

        setAxisTitle(QwtAxisId(QwtAxis::yRight, 3), tr("TISS"));
        setAxisScale(QwtAxisId(QwtAxis::yRight, 3),0, qMax(standard->atissCurve->maxYValue(), standard->atissCurve->maxYValue()) * 1.05);
        setAxisLabelAlignment(QwtAxisId(QwtAxis::yRight, 3),Qt::AlignVCenter);
    }

    // QwtAxis::yRight, 2
    if (showW && standard->wCurve->isVisible() && rideItem && rideItem->ride()) {

        setAxisTitle(QwtAxisId(QwtAxis::yRight, 2), tr("W' Balance (kJ)"));
        setAxisScale(QwtAxisId(QwtAxis::yRight, 2), qMin(int(standard->wCurve->minYValue()-1000), 0),
                                                    qMax(int(standard->wCurve->maxYValue()+1000), 0));

        setAxisLabelAlignment(QwtAxisId(QwtAxis::yRight, 2),Qt::AlignVCenter);
    }

    // QwtAxis::yLeft
    if (standard->wattsCurve->isVisible()) {
        double maxY = (referencePlot == NULL) ? (1.05 * standard->wattsCurve->maxYValue()) :
                                             (1.05 * referencePlot->standard->wattsCurve->maxYValue());

        int axisHeight = qRound( plotLayout()->canvasRect().height() );
        int step = 100;

        // axisHeight will be zero before first show, so only do this if its non-zero!
        if (axisHeight) {
            QFontMetrics labelWidthMetric = QFontMetrics( QwtPlot::axisFont(yLeft) );
            int labelWidth = labelWidthMetric.width( (maxY > 1000) ? " 8,888 " : " 888 " );

            if (axisHeight > labelWidth*2) //Avoid insane iterations
                while( ( qCeil(maxY / step) * labelWidth ) > axisHeight ) nextStep(step);
        }

        setAxisTitle(yLeft, tr("Watts"));
        AllPlot::setAxisScaleDiv(QwtPlot::yLeft, 0, maxY, step);
        axisWidget(yLeft)->update();
    }

    // QwtAxis::yLeft, 1
    if (standard->hrCurve->isVisible() ||standard->hrvCurve->isVisible() || standard->tcoreCurve->isVisible() ||
        standard->cadCurve->isVisible() || standard->smo2Curve->isVisible() ||
       (!context->athlete->useMetricUnits && standard->tempCurve->isVisible())) {

        double ymin = 0;
        double ymax = 0;

        QStringList labels;
        if (standard->hrvCurve->isVisible()) {
            labels << tr("RR");
        ymax = standard->hrvCurve->maxYValue();
        }
        if (standard->hrCurve->isVisible()) {
            labels << tr("BPM");
            if (referencePlot == NULL)
                ymax = standard->hrCurve->maxYValue();
            else
                ymax = referencePlot->standard->hrCurve->maxYValue();
        }
        if (standard->tcoreCurve->isVisible()) {
            labels << tr("Core Temperature");
            if (referencePlot == NULL)
                ymax = qMax(ymax, standard->tcoreCurve->maxYValue());
            else
                ymax = qMax(ymax, referencePlot->standard->tcoreCurve->maxYValue());
        }
        if (standard->smo2Curve->isVisible()) {
            labels << tr("SmO2");
            if (referencePlot == NULL)
                ymax = qMax(ymax, standard->smo2Curve->maxYValue());
            else
                ymax = qMax(ymax, referencePlot->standard->smo2Curve->maxYValue());
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

        int axisHeight = qRound( plotLayout()->canvasRect().height() );
        int step = 10;

        if (axisHeight) {
            QFontMetrics labelWidthMetric = QFontMetrics( QwtPlot::axisFont(yLeft) );
            int labelWidth = labelWidthMetric.width( "888 " );
            ymax *= 1.05;

            if (axisHeight>labelWidth*2) //Avoid insane iterations
                while((qCeil(ymax / step) * labelWidth) > axisHeight) nextStep(step);
        }

        setAxisTitle(QwtAxisId(QwtAxis::yLeft, 1), labels.join(" / "));

        if (labels.count() == 1 && labels[0] == tr("Core Temperature")) {

            double ymin=36.5f;
            if (ymax < 39.0f) ymax = 39.0f;

            if (standard->tcoreCurve->isVisible() && standard->tcoreCurve->minYValue() < ymin)
                ymin = standard->tcoreCurve->minYValue();

            double step = 0.00f;
            if (ymin < 100.00f) step = (ymax - ymin) / 4;

            // we just have Core Temp ...
            setAxisScale(QwtAxisId(QwtAxis::yLeft, 1),ymin<100.0f?ymin:0, ymax, step);

        } else {
            AllPlot::setAxisScaleDiv(QwtAxisId(QwtAxis::yLeft, 1), ymin, ymax, step);
        }
    }

    // QwtAxis::yLeft, 3
    if ((standard->balanceLCurve->isVisible() || standard->lteCurve->isVisible() ||
        standard->lpsCurve->isVisible()) || standard->slopeCurve->isVisible()){

        QStringList labels;
        double ymin = 0;
        double ymax = 0;

        if (standard->balanceLCurve->isVisible() || standard->lteCurve->isVisible() ||
            standard->lpsCurve->isVisible()) {
          labels << tr("Percent");
          ymin = 0;
          ymax = 100;
        };
        if (standard->slopeCurve->isVisible()) {
          labels << tr("Slope");
          ymin = qMin(standard->slopeCurve->minYValue() * 1.1, ymin);
          ymax = qMax(standard->slopeCurve->maxYValue() * 1.1, ymax);
        };

        // Set range from the curves
        setAxisTitle(QwtAxisId(QwtAxis::yLeft, 3), labels.join(" / "));
        setAxisScale(QwtAxisId(QwtAxis::yLeft, 3), ymin, ymax);

        // not sure about this .. should be done on creation (?)
        standard->balanceLCurve->setBaseline(50);
        standard->balanceRCurve->setBaseline(50);
    }

    // QwtAxis::yRight, 0
    if (standard->speedCurve->isVisible() || standard->thbCurve->isVisible() || 
        standard->o2hbCurve->isVisible() || standard->hhbCurve->isVisible() ||
        (context->athlete->useMetricUnits && standard->tempCurve->isVisible()) || 
         standard->torqueCurve->isVisible()) {

        double ymin = -10;
        double ymax = 0;

        QStringList labels;

        // axis scale draw precision
        static_cast<ScaleScaleDraw*>(axisScaleDraw(QwtAxisId(QwtAxis::yRight, 0)))->setDecimals(2);

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
        if (standard->thbCurve->isVisible() || standard->o2hbCurve->isVisible() || standard->hhbCurve->isVisible()) {
            labels << tr("Hb");

            if (referencePlot == NULL)
                ymax = qMax(ymax, standard->thbCurve->maxYValue());
            else
                ymax = qMax(ymax, referencePlot->standard->thbCurve->maxYValue());
        }
        if (standard->torqueCurve->isVisible()) {
            labels << (context->athlete->useMetricUnits ? tr("Nm") : tr("ftLb"));

            if (referencePlot == NULL)
                ymax = qMax(ymax, standard->torqueCurve->maxYValue());
            else
                ymax = qMax(ymax, referencePlot->standard->torqueCurve->maxYValue());
        }

        int axisHeight = qRound( plotLayout()->canvasRect().height() );
        int step = 10;

        if (axisHeight) {
            QFontMetrics labelWidthMetric = QFontMetrics( QwtPlot::axisFont(yRight) );
            int labelWidth = labelWidthMetric.width( "888 " );
            ymax *= 1.05;

            if (axisHeight>labelWidth*2) //Avoid insane iterations
                while((qCeil(ymax / step) * labelWidth) > axisHeight) nextStep(step);
        }

        setAxisTitle(QwtAxisId(QwtAxis::yRight, 0), labels.join(" / "));

        // we just have Hb ?
        if (labels.count() == 1 && labels[0] == tr("Hb")) {

            double ymin=100;
            ymax /= 1.05;
            ymax += .10f;

            if (standard->thbCurve->isVisible() && standard->thbCurve->minYValue() < ymin)
                ymin = standard->thbCurve->minYValue();
            if (standard->o2hbCurve->isVisible() && standard->o2hbCurve->minYValue() < ymin)
                ymin = standard->o2hbCurve->minYValue();
            if (standard->hhbCurve->isVisible() && standard->hhbCurve->minYValue() < ymin)
                ymin = standard->hhbCurve->minYValue();

            double step = 0.00f;
            if (ymin < 100.00f) step = (ymax - ymin) / 5;

            // we just have Hb ...
            setAxisScale(QwtAxisId(QwtAxis::yRight, 0),ymin<100.0f?ymin:0, ymax, step);

        } else {
            AllPlot::setAxisScaleDiv(QwtAxisId(QwtAxis::yRight, 0), ymin, ymax, step);
        }
    }

    // QwtAxis::yRight, 1
    if (standard->altCurve->isVisible() || standard->altSlopeCurve->isVisible())  {
        setAxisTitle(QwtAxisId(QwtAxis::yRight, 1), context->athlete->useMetricUnits ? tr("Meters") : tr("Feet"));
        double ymin,ymax;

        if (referencePlot == NULL) {
            ymin = standard->altCurve->minYValue();
            ymax = qMax(500.000, 1.05 * standard->altCurve->maxYValue());
        } else {
            ymin = referencePlot->standard->altCurve->minYValue();
            ymax = qMax(500.000, 1.05 * referencePlot->standard->altCurve->maxYValue());
        }
        ymin = (ymin < 0 ? -100 : 0) + ( qRound(ymin) / 100 ) * 100;

        int axisHeight = qRound( plotLayout()->canvasRect().height() );
        int step = 100;

        if (axisHeight) {
            QFontMetrics labelWidthMetric = QFontMetrics( QwtPlot::axisFont(yLeft) );
            int labelWidth = labelWidthMetric.width( (ymax > 1000) ? " 8888 " : " 888 " );

            if (axisHeight>labelWidth*2) //Avoid insane iterations
                while( ( qCeil( (ymax - ymin ) / step) * labelWidth ) > axisHeight ) nextStep(step);
        }

        AllPlot::setAxisScaleDiv(QwtAxisId(QwtAxis::yRight, 1), ymin, ymax, step);
        standard->altCurve->setBaseline(ymin);
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

// we do this a lot so trying to save a bit of cut and paste woe
static void setSymbol(QwtPlotCurve *curve, int points)
{
    QwtSymbol *sym = new QwtSymbol;
    sym->setPen(QPen(GColor(CPLOTMARKER)));
    if (points < 150) {
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(3 *dpiXFactor);
    } else {
        sym->setStyle(QwtSymbol::NoSymbol);
        sym->setSize(0 *dpiXFactor);
    }
    curve->setSymbol(sym);
}

void
AllPlot::setDataFromPlot(AllPlot *plot, int startidx, int stopidx)
{
    if (plot == NULL) {
        rideItem = NULL;
        return;
    }

    referencePlot = plot;

    isolation = false;
    curveColors->saveState();

    // You got to give me some data first!
    if (!plot->standard->distanceArray.count() || !plot->standard->timeArray.count()) return;

    // reference the plot for data and state
    rideItem = plot->rideItem;
    bydist = plot->bydist;
    bytimeofday = plot->bytimeofday;
    timeoffset = plot->timeoffset;

    //arrayLength = stopidx-startidx;

    if (bydist) {
        startidx = plot->distanceIndex(plot->standard->distanceArray[startidx]);
        stopidx  = plot->distanceIndex(plot->standard->distanceArray[(stopidx>=plot->standard->distanceArray.size()?plot->standard->distanceArray.size()-1:stopidx)]);
    } else {
        startidx = plot->timeIndex(plot->standard->timeArray[startidx]/60);
        stopidx  = plot->timeIndex(plot->standard->timeArray[(stopidx>=plot->standard->timeArray.size()?plot->standard->timeArray.size()-1:stopidx)]/60);
    }

    // center the curve title
    standard->curveTitle.setYValue(30);
    standard->curveTitle.setXValue(2);

    // make sure indexes are still valid
    if (startidx > stopidx || startidx < 0 || stopidx < 0) return;

    // user data smoothing
    QList<double*> smoothU;
    for(int k=0; k<plot->standard->U.count(); k++)
        smoothU << &plot->standard->U[k].smooth[startidx];

    double *smoothW = &plot->standard->smoothWatts[startidx];
    double *smoothN = &plot->standard->smoothNP[startidx];
    double *smoothRV = &plot->standard->smoothRV[startidx];
    double *smoothRCad = &plot->standard->smoothRCad[startidx];
    double *smoothRGCT = &plot->standard->smoothRGCT[startidx];
    double *smoothGear = &plot->standard->smoothGear[startidx];
    double *smoothSmO2 = &plot->standard->smoothSmO2[startidx];
    double *smoothtHb = &plot->standard->smoothtHb[startidx];
    double *smoothO2Hb = &plot->standard->smoothO2Hb[startidx];
    double *smoothHHb = &plot->standard->smoothHHb[startidx];
    double *smoothAT = &plot->standard->smoothAT[startidx];
    double *smoothANT = &plot->standard->smoothANT[startidx];
    double *smoothX = &plot->standard->smoothXP[startidx];
    double *smoothL = &plot->standard->smoothAP[startidx];
    double *smoothT = &plot->standard->smoothTime[startidx];
    double *smoothHR = &plot->standard->smoothHr[startidx];
    double *smoothTCORE = &plot->standard->smoothTcore[startidx];
    double *smoothS = &plot->standard->smoothSpeed[startidx];
    double *smoothC = &plot->standard->smoothCad[startidx];
    double *smoothA = &plot->standard->smoothAltitude[startidx];
    double *smoothSL = &plot->standard->smoothSlope[startidx];
    double *smoothD = &plot->standard->smoothDistance[startidx];
    double *smoothTE = &plot->standard->smoothTemp[startidx];
    //double *standard->smoothWND = &plot->standard->smoothWind[startidx];
    double *smoothNM = &plot->standard->smoothTorque[startidx];

    // left/right
    double *smoothBALL = &plot->standard->smoothBalanceL[startidx];
    double *smoothBALR = &plot->standard->smoothBalanceR[startidx];
    double *smoothLTE = &plot->standard->smoothLTE[startidx];
    double *smoothRTE = &plot->standard->smoothRTE[startidx];
    double *smoothLPS = &plot->standard->smoothLPS[startidx];
    double *smoothRPS = &plot->standard->smoothRPS[startidx];
    double *smoothLPCO = &plot->standard->smoothLPCO[startidx];
    double *smoothRPCO = &plot->standard->smoothRPCO[startidx];
    QwtIntervalSample *smoothLPP = &plot->standard->smoothLPP[startidx];
    QwtIntervalSample *smoothRPP = &plot->standard->smoothRPP[startidx];
    QwtIntervalSample *smoothLPPP = &plot->standard->smoothLPPP[startidx];
    QwtIntervalSample *smoothRPPP = &plot->standard->smoothRPPP[startidx];

    // deltas
    double *smoothAC = &plot->standard->smoothAccel[startidx];
    double *smoothWD = &plot->standard->smoothWattsD[startidx];
    double *smoothCD = &plot->standard->smoothCadD[startidx];
    double *smoothND = &plot->standard->smoothNmD[startidx];
    double *smoothHD = &plot->standard->smoothHrD[startidx];

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
            int minCP = rideItem->ride()->wprimeData()->PCP();
            if (minCP)
                warn = QString(tr("** Minimum CP=%1 **")).arg(rideItem->ride()->wprimeData()->PCP());
            else
                warn = QString(tr("** Check W' is set correctly **"));
        }

        QString matchesText;  // consider Singular/Plural in Text / Zero is in most languages handled like Plural
        if (count == 1) {
          matchesText = tr("Tau=%1, CP=%2, W'=%3, %4 match >2kJ (%5 kJ) %6");
        }
        else {
          matchesText = tr("Tau=%1, CP=%2, W'=%3, %4 matches >2kJ (%5 kJ) %6");
        }

        QwtText text(matchesText.arg(rideItem->ride()->wprimeData()->TAU)
                                                    .arg(rideItem->ride()->wprimeData()->CP)
                                                    .arg(rideItem->ride()->wprimeData()->WPRIME)
                                                    .arg(count)
                                                    .arg(burnt/1000.00, 0, 'f', 1)
                                                    .arg(warn));

        QFont font; // default;
        font.setWeight(QFont::Bold);
        text.setFont(font);
        text.setColor(GColor(CWBAL));
        standard->curveTitle.setLabel(text);
    } else {
        standard->curveTitle.setLabel(QwtText(""));
    }

    for(int k=0; k<standard->U.count(); k++) standard->U[k].curve->detach();
    standard->wCurve->detach();
    standard->mCurve->detach();
    standard->wattsCurve->detach();
    standard->atissCurve->detach();
    standard->antissCurve->detach();
    standard->npCurve->detach();
    standard->rvCurve->detach();
    standard->rcadCurve->detach();
    standard->rgctCurve->detach();
    standard->gearCurve->detach();
    standard->smo2Curve->detach();
    standard->thbCurve->detach();
    standard->o2hbCurve->detach();
    standard->hhbCurve->detach();
    standard->xpCurve->detach();
    standard->apCurve->detach();
    standard->hrCurve->detach();
    standard->hrvCurve->detach();
    standard->tcoreCurve->detach();
    standard->speedCurve->detach();
    standard->accelCurve->detach(); 
    standard->wattsDCurve->detach(); 
    standard->cadDCurve->detach(); 
    standard->nmDCurve->detach(); 
    standard->hrDCurve->detach(); 
    standard->cadCurve->detach();
    standard->altCurve->detach();
    standard->altSlopeCurve->detach();
    standard->slopeCurve->detach();
    standard->tempCurve->detach();
    standard->windCurve->detach();
    standard->torqueCurve->detach();
    standard->lteCurve->detach();
    standard->rteCurve->detach();
    standard->lpsCurve->detach();
    standard->rpsCurve->detach();
    standard->balanceLCurve->detach();
    standard->balanceRCurve->detach();
    standard->lpcoCurve->detach();
    standard->rpcoCurve->detach();
    standard->lppCurve->detach();
    standard->rppCurve->detach();
    standard->lpppCurve->detach();
    standard->rpppCurve->detach();

    for(int k=0; k<standard->U.count(); k++) standard->U[k].curve->setVisible(true);
    standard->wattsCurve->setVisible(rideItem->ride()->areDataPresent()->watts && showPowerState < 2);
    standard->atissCurve->setVisible(rideItem->ride()->areDataPresent()->watts && showATISS);
    standard->antissCurve->setVisible(rideItem->ride()->areDataPresent()->watts && showANTISS);
    standard->npCurve->setVisible(rideItem->ride()->areDataPresent()->np && showNP);
    standard->rvCurve->setVisible(rideItem->ride()->areDataPresent()->rvert && showRV);
    standard->rcadCurve->setVisible(rideItem->ride()->areDataPresent()->rcad && showRCad);
    standard->gearCurve->setVisible(rideItem->ride()->areDataPresent()->gear && showGear);
    standard->smo2Curve->setVisible(rideItem->ride()->areDataPresent()->smo2 && showSmO2);
    standard->thbCurve->setVisible(rideItem->ride()->areDataPresent()->thb && showtHb);
    standard->o2hbCurve->setVisible(rideItem->ride()->areDataPresent()->o2hb && showO2Hb);
    standard->hhbCurve->setVisible(rideItem->ride()->areDataPresent()->hhb && showHHb);
    standard->rgctCurve->setVisible(rideItem->ride()->areDataPresent()->rcontact && showRGCT);
    standard->xpCurve->setVisible(rideItem->ride()->areDataPresent()->xp && showXP);
    standard->apCurve->setVisible(rideItem->ride()->areDataPresent()->apower && showAP);
    standard->wCurve->setVisible(rideItem->ride()->areDataPresent()->watts && showW);
    standard->mCurve->setVisible(rideItem->ride()->areDataPresent()->watts && showW);
    standard->hrCurve->setVisible(rideItem->ride()->areDataPresent()->hr && showHr);
    standard->hrvCurve->setVisible(rideItem->ride()->areDataPresent()->hrv && showHRV);
    standard->tcoreCurve->setVisible(rideItem->ride()->areDataPresent()->hr && showTcore);
    standard->speedCurve->setVisible(rideItem->ride()->areDataPresent()->kph && showSpeed);
    standard->accelCurve->setVisible(rideItem->ride()->areDataPresent()->kph && showAccel);
    standard->wattsDCurve->setVisible(rideItem->ride()->areDataPresent()->watts && showPowerD);
    standard->cadDCurve->setVisible(rideItem->ride()->areDataPresent()->cad && showCadD);
    standard->nmDCurve->setVisible(rideItem->ride()->areDataPresent()->nm && showTorqueD);
    standard->hrDCurve->setVisible(rideItem->ride()->areDataPresent()->hr && showHrD);
    standard->cadCurve->setVisible(rideItem->ride()->areDataPresent()->cad && showCad);
    standard->altCurve->setVisible(rideItem->ride()->areDataPresent()->alt && showAlt);
    standard->altSlopeCurve->setVisible(rideItem->ride()->areDataPresent()->alt && showAltSlopeState > 0);
    standard->slopeCurve->setVisible(rideItem->ride()->areDataPresent()->slope && showSlope);
    standard->tempCurve->setVisible(rideItem->ride()->areDataPresent()->temp && showTemp);
    standard->windCurve->setVisible(rideItem->ride()->areDataPresent()->headwind && showWind);
    standard->torqueCurve->setVisible(rideItem->ride()->areDataPresent()->nm && showTorque);
    standard->lteCurve->setVisible(rideItem->ride()->areDataPresent()->lte && showTE);
    standard->rteCurve->setVisible(rideItem->ride()->areDataPresent()->rte && showTE);
    standard->lpsCurve->setVisible(rideItem->ride()->areDataPresent()->lps && showPS);
    standard->rpsCurve->setVisible(rideItem->ride()->areDataPresent()->rps && showPS);
    standard->balanceLCurve->setVisible(rideItem->ride()->areDataPresent()->lrbalance && showBalance);
    standard->balanceRCurve->setVisible(rideItem->ride()->areDataPresent()->lrbalance && showBalance);
    standard->lpcoCurve->setVisible(rideItem->ride()->areDataPresent()->lpco && showPCO);
    standard->rpcoCurve->setVisible(rideItem->ride()->areDataPresent()->rpco && showPCO);
    standard->lppCurve->setVisible(rideItem->ride()->areDataPresent()->lppb && showDC);
    standard->rppCurve->setVisible(rideItem->ride()->areDataPresent()->rppb && showDC);
    standard->lpppCurve->setVisible(rideItem->ride()->areDataPresent()->lpppb && showPPP);
    standard->rpppCurve->setVisible(rideItem->ride()->areDataPresent()->rpppb && showPPP);

    if (showW) {
        standard->wCurve->setSamples(bydist ? plot->standard->wprimeDist.data() : plot->standard->wprimeTime.data(), 
                                    plot->standard->wprime.data(), plot->standard->wprime.count());
        standard->mCurve->setSamples(bydist ? plot->standard->matchDist.data() : plot->standard->matchTime.data(), 
                                    plot->standard->match.data(), plot->standard->match.count());
        setMatchLabels(standard);
    }
    int points = stopidx - startidx + 1; // e.g. 10 to 12 is 3 points 10,11,12, so not 12-10 !
    for(int k=0; k<standard->U.count(); k++) standard->U[k].curve->setSamples(xaxis,smoothU[k], points);
    standard->hrvCurve->setSamples(plot->standard->smoothHrv_time.data(),
                   plot->standard->smoothHrv.data(),
                   plot->standard->smoothHrv.count());
    standard->wattsCurve->setSamples(xaxis,smoothW,points);
    standard->atissCurve->setSamples(xaxis,smoothAT,points);
    standard->antissCurve->setSamples(xaxis,smoothANT,points);
    standard->npCurve->setSamples(xaxis,smoothN,points);
    standard->rvCurve->setSamples(xaxis,smoothRV,points);
    standard->rcadCurve->setSamples(xaxis,smoothRCad,points);
    standard->rgctCurve->setSamples(xaxis,smoothRGCT,points);
    standard->gearCurve->setSamples(xaxis,smoothGear,points);
    standard->smo2Curve->setSamples(xaxis,smoothSmO2,points);
    standard->thbCurve->setSamples(xaxis,smoothtHb,points);
    standard->o2hbCurve->setSamples(xaxis,smoothO2Hb,points);
    standard->hhbCurve->setSamples(xaxis,smoothHHb,points);
    standard->xpCurve->setSamples(xaxis,smoothX,points);
    standard->apCurve->setSamples(xaxis,smoothL,points);
    standard->hrCurve->setSamples(xaxis, smoothHR,points);
    standard->tcoreCurve->setSamples(xaxis, smoothTCORE,points);
    standard->speedCurve->setSamples(xaxis, smoothS, points);
    standard->accelCurve->setSamples(xaxis, smoothAC, points);
    standard->wattsDCurve->setSamples(xaxis, smoothWD, points);
    standard->cadDCurve->setSamples(xaxis, smoothCD, points);
    standard->nmDCurve->setSamples(xaxis, smoothND, points);
    standard->hrDCurve->setSamples(xaxis, smoothHD, points);
    standard->cadCurve->setSamples(xaxis, smoothC, points);
    standard->altCurve->setSamples(xaxis, smoothA, points);
    standard->altSlopeCurve->setSamples(xaxis, smoothA, points);
    standard->slopeCurve->setSamples(xaxis, smoothSL, points);
    standard->tempCurve->setSamples(xaxis, smoothTE, points);

    QVector<QwtIntervalSample> tmpWND(points);
    memcpy(tmpWND.data(), smoothRS, (points) * sizeof(QwtIntervalSample));
    standard->windCurve->setSamples(new QwtIntervalSeriesData(tmpWND));
    standard->torqueCurve->setSamples(xaxis, smoothNM, points);
    standard->balanceLCurve->setSamples(xaxis, smoothBALL, points);
    standard->balanceRCurve->setSamples(xaxis, smoothBALR, points);
    standard->lteCurve->setSamples(xaxis, smoothLTE, points);
    standard->rteCurve->setSamples(xaxis, smoothRTE, points);
    standard->lpsCurve->setSamples(xaxis, smoothLPS, points);
    standard->rpsCurve->setSamples(xaxis, smoothRPS, points);
    standard->lpcoCurve->setSamples(xaxis, smoothLPCO, points);
    standard->rpcoCurve->setSamples(xaxis, smoothRPCO, points);

    QVector<QwtIntervalSample> tmpLDC(points);
    memcpy(tmpLDC.data(), smoothLPP, (points) * sizeof(QwtIntervalSample));
    standard->lppCurve->setSamples(new QwtIntervalSeriesData(tmpLDC));
    QVector<QwtIntervalSample> tmpRDC(points);
    memcpy(tmpRDC.data(), smoothRPP, (points) * sizeof(QwtIntervalSample));
    standard->rppCurve->setSamples(new QwtIntervalSeriesData(tmpRDC));
    QVector<QwtIntervalSample> tmpLPPP(points);
    memcpy(tmpLPPP.data(), smoothLPPP, (points) * sizeof(QwtIntervalSample));
    standard->lpppCurve->setSamples(new QwtIntervalSeriesData(tmpLPPP));
    QVector<QwtIntervalSample> tmpRPPP(points);
    memcpy(tmpRPPP.data(), smoothRPPP, (points) * sizeof(QwtIntervalSample));
    standard->rpppCurve->setSamples(new QwtIntervalSeriesData(tmpRPPP));

    /*QVector<double> _time(stopidx-startidx);
    qMemCopy( _time.data(), xaxis, (stopidx-startidx) * sizeof( double ) );

    QVector<QwtIntervalSample> tmpWND(stopidx-startidx);
    for (int i=0;i<_time.count();i++) {
        QwtIntervalSample inter = QwtIntervalSample(_time.at(i), 20,50);
        tmpWND.append(inter); // plot->standard->smoothRelSpeed.at(i)
    }*/

    for(int k=0; k<standard->U.count(); k++) setSymbol(standard->U[k].curve, points);
    setSymbol(standard->wCurve, points);
    setSymbol(standard->wattsCurve, points);
    setSymbol(standard->antissCurve, points);
    setSymbol(standard->atissCurve, points);
    setSymbol(standard->npCurve, points);
    setSymbol(standard->rvCurve, points);
    setSymbol(standard->rcadCurve, points);
    setSymbol(standard->rgctCurve, points);
    setSymbol(standard->gearCurve, points);
    setSymbol(standard->smo2Curve, points);
    setSymbol(standard->thbCurve, points);
    setSymbol(standard->o2hbCurve, points);
    setSymbol(standard->hhbCurve, points);
    setSymbol(standard->xpCurve, points);
    setSymbol(standard->apCurve, points);
    setSymbol(standard->hrCurve, points);
    setSymbol(standard->tcoreCurve, points);
    setSymbol(standard->speedCurve, points);
    setSymbol(standard->accelCurve, points);
    setSymbol(standard->wattsDCurve, points);
    setSymbol(standard->cadDCurve, points);
    setSymbol(standard->nmDCurve, points);
    setSymbol(standard->hrDCurve, points);
    setSymbol(standard->cadCurve, points);
    setSymbol(standard->altCurve, points);
    setSymbol(standard->altSlopeCurve, points);
    setSymbol(standard->slopeCurve, points);
    setSymbol(standard->tempCurve, points);
    setSymbol(standard->torqueCurve, points);
    setSymbol(standard->balanceLCurve, points);
    setSymbol(standard->balanceRCurve, points);
    setSymbol(standard->lteCurve, points);
    setSymbol(standard->rteCurve, points);
    setSymbol(standard->lpsCurve, points);
    setSymbol(standard->rpsCurve, points);
    setSymbol(standard->lpcoCurve, points);
    setSymbol(standard->rpcoCurve, points);

    // show if got data
    for(int k=0; k<standard->U.count(); k++) {
        if (!plot->standard->U[k].smooth.empty()) {
            standard->U[k].curve->attach(this);
        }
    }

    if (!plot->standard->smoothAltitude.empty()) {
        standard->altCurve->attach(this);
        standard->altSlopeCurve->attach(this);
    }
    if (!plot->standard->smoothSlope.empty()) {
        standard->slopeCurve->attach(this);
    }
    if (showW && plot->standard->wprime.count()) {
        standard->wCurve->attach(this);
        standard->mCurve->attach(this);
    }
    if (!plot->standard->smoothWatts.empty()) {
        standard->wattsCurve->attach(this);
    }
    if (!plot->standard->smoothANT.empty()) {
        standard->antissCurve->attach(this);
    }
    if (!plot->standard->smoothAT.empty()) {
        standard->atissCurve->attach(this);
    }
    if (!plot->standard->smoothNP.empty()) {
        standard->npCurve->attach(this);
    }
    if (!plot->standard->smoothRV.empty()) {
        standard->rvCurve->attach(this);
    }
    if (!plot->standard->smoothRCad.empty()) {
        standard->rcadCurve->attach(this);
    }
    if (!plot->standard->smoothRGCT.empty()) {
        standard->rgctCurve->attach(this);
    }
    if (!plot->standard->smoothGear.empty()) {
        standard->gearCurve->attach(this);
    }
    if (!plot->standard->smoothSmO2.empty()) {
        standard->smo2Curve->attach(this);
    }
    if (!plot->standard->smoothtHb.empty()) {
        standard->thbCurve->attach(this);
    }
    if (!plot->standard->smoothO2Hb.empty()) {
        standard->o2hbCurve->attach(this);
    }
    if (!plot->standard->smoothHHb.empty()) {
        standard->hhbCurve->attach(this);
    }
    if (!plot->standard->smoothXP.empty()) {
        standard->xpCurve->attach(this);
    }
    if (!plot->standard->smoothAP.empty()) {
        standard->apCurve->attach(this);
    }
    if (!plot->standard->smoothTcore.empty()) {
        standard->tcoreCurve->attach(this);
    }
    if (!plot->standard->smoothHr.empty()) {
        standard->hrCurve->attach(this);
    }
    if (!plot->standard->smoothAccel.empty()) {
        standard->accelCurve->attach(this);
    }
    if (!plot->standard->smoothWattsD.empty()) {
        standard->wattsDCurve->attach(this);
    }
    if (!plot->standard->smoothCadD.empty()) {
        standard->cadDCurve->attach(this);
    }
    if (!plot->standard->smoothNmD.empty()) {
        standard->nmDCurve->attach(this);
    }
    if (!plot->standard->smoothHrD.empty()) {
        standard->hrDCurve->attach(this);
    }
    if (!plot->standard->smoothSpeed.empty()) {
        standard->speedCurve->attach(this);
    }
    if (!plot->standard->smoothCad.empty()) {
        standard->cadCurve->attach(this);
    }
    if (!plot->standard->smoothTemp.empty()) {
        standard->tempCurve->attach(this);
    }
    if (!plot->standard->smoothWind.empty()) {
        standard->windCurve->attach(this);
    }
    if (!plot->standard->smoothTorque.empty()) {
        standard->torqueCurve->attach(this);
    }
    if (!plot->standard->smoothBalanceL.empty()) {
        standard->balanceLCurve->attach(this);
        standard->balanceRCurve->attach(this);
    }
    if (!plot->standard->smoothLTE.empty()) {
        standard->lteCurve->attach(this);
        standard->rteCurve->attach(this);
    }
    if (!plot->standard->smoothLPS.empty()) {
        standard->lpsCurve->attach(this);
        standard->rpsCurve->attach(this);
    }
    if (!plot->standard->smoothLPCO.empty()) {
        standard->lpcoCurve->attach(this);
        standard->rpcoCurve->attach(this);
    }
    if (!plot->standard->smoothLPP.empty()) {
        standard->lppCurve->attach(this);
        standard->rppCurve->attach(this);
    }
    if (!plot->standard->smoothLPPP.empty()) {
        standard->lpppCurve->attach(this);
        standard->rpppCurve->attach(this);
    }

    setAltSlopePlotStyle(standard->altSlopeCurve);
    setYMax();

    setAxisScale(xBottom, xaxis[0], xaxis[stopidx-startidx]);
    enableAxis(xBottom, true);
    setAxisVisible(xBottom, true);

    refreshReferenceLines();
    refreshExhaustions();
    refreshIntervalMarkers();
    refreshCalibrationMarkers();
    refreshZoneLabels();

    // set all the colors ?
    configChanged(CONFIG_APPEARANCE);

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();

    replot();
}

void
AllPlot::setDataFromPlot(AllPlot *plot)
{
    if (plot == NULL) {
        rideItem = NULL;
        return;
    }

    referencePlot = plot;

    isolation = false;
    curveColors->saveState();

    // reference the plot for data and state
    rideItem = plot->rideItem;
    bydist = plot->bydist;
    bytimeofday = plot->bytimeofday;
    timeoffset = plot->timeoffset;

    // remove all curves from the plot
    for(int k=0; k<standard->U.count(); k++) standard->U[k].curve->detach();
    standard->wCurve->detach();
    standard->mCurve->detach();
    standard->wattsCurve->detach();
    standard->atissCurve->detach();
    standard->antissCurve->detach();
    standard->npCurve->detach();
    standard->rvCurve->detach();
    standard->rcadCurve->detach();
    standard->rgctCurve->detach();
    standard->gearCurve->detach();
    standard->smo2Curve->detach();
    standard->thbCurve->detach();
    standard->o2hbCurve->detach();
    standard->hhbCurve->detach();
    standard->xpCurve->detach();
    standard->apCurve->detach();
    standard->hrCurve->detach();
    standard->hrvCurve->detach();
    standard->tcoreCurve->detach();
    standard->speedCurve->detach();
    standard->accelCurve->detach();
    standard->wattsDCurve->detach();
    standard->cadDCurve->detach();
    standard->nmDCurve->detach();
    standard->hrDCurve->detach();
    standard->cadCurve->detach();
    standard->altCurve->detach();
    standard->altSlopeCurve->detach();
    standard->slopeCurve->detach();
    standard->tempCurve->detach();
    standard->windCurve->detach();
    standard->torqueCurve->detach();
    standard->balanceLCurve->detach();
    standard->balanceRCurve->detach();
    standard->lteCurve->detach();
    standard->rteCurve->detach();
    standard->lpsCurve->detach();
    standard->rpsCurve->detach();
    standard->lpcoCurve->detach();
    standard->rpcoCurve->detach();
    standard->lppCurve->detach();
    standard->rppCurve->detach();
    standard->lpppCurve->detach();
    standard->rpppCurve->detach();

    for(int k=0; k<standard->U.count(); k++) standard->U[k].curve->setVisible(false);
    standard->wCurve->setVisible(false);
    standard->mCurve->setVisible(false);
    standard->wattsCurve->setVisible(false);
    standard->atissCurve->setVisible(false);
    standard->antissCurve->setVisible(false);
    standard->npCurve->setVisible(false);
    standard->rvCurve->setVisible(false);
    standard->rcadCurve->setVisible(false);
    standard->rgctCurve->setVisible(false);
    standard->gearCurve->setVisible(false);
    standard->smo2Curve->setVisible(false);
    standard->thbCurve->setVisible(false);
    standard->o2hbCurve->setVisible(false);
    standard->hhbCurve->setVisible(false);
    standard->xpCurve->setVisible(false);
    standard->apCurve->setVisible(false);
    standard->hrCurve->setVisible(false);
    standard->hrvCurve->setVisible(false);
    standard->tcoreCurve->setVisible(false);
    standard->speedCurve->setVisible(false);
    standard->accelCurve->setVisible(false);
    standard->wattsDCurve->setVisible(false);
    standard->cadDCurve->setVisible(false);
    standard->nmDCurve->setVisible(false);
    standard->hrDCurve->setVisible(false);
    standard->cadCurve->setVisible(false);
    standard->altCurve->setVisible(false);
    standard->altSlopeCurve->setVisible(false);
    standard->slopeCurve->setVisible(false);
    standard->tempCurve->setVisible(false);
    standard->windCurve->setVisible(false);
    standard->torqueCurve->setVisible(false);
    standard->balanceLCurve->setVisible(false);
    standard->balanceRCurve->setVisible(false);
    standard->lteCurve->setVisible(false);
    standard->rteCurve->setVisible(false);
    standard->lpsCurve->setVisible(false);
    standard->rpsCurve->setVisible(false);
    standard->lpcoCurve->setVisible(false);
    standard->rpcoCurve->setVisible(false);
    standard->lppCurve->setVisible(false);
    standard->rppCurve->setVisible(false);
    standard->lpppCurve->setVisible(false);
    standard->rpppCurve->setVisible(false);

    QwtPlotCurve *ourCurve = NULL, *thereCurve = NULL;
    QwtPlotCurve *ourCurve2 = NULL, *thereCurve2 = NULL;
    AllPlotSlopeCurve *ourASCurve = NULL, *thereASCurve = NULL;
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

    case RideFile::tcore:
        {
        ourCurve = standard->tcoreCurve;
        thereCurve = referencePlot->standard->tcoreCurve;
        title = tr("Core Temp");
        }
        break;

    case RideFile::hr:
        {
        ourCurve = standard->hrCurve;
        thereCurve = referencePlot->standard->hrCurve;
        title = tr("Heartrate");
        }
        break;

    case RideFile::hrv:
        {
        ourCurve = standard->hrvCurve;
        thereCurve = referencePlot->standard->hrvCurve;
        title = tr("R-R");
        }
        break;

    case RideFile::kphd:
        {
        ourCurve = standard->accelCurve;
        thereCurve = referencePlot->standard->accelCurve;
        title = tr("Acceleration");
        }
        break;

    case RideFile::wattsd:
        {
        ourCurve = standard->wattsDCurve;
        thereCurve = referencePlot->standard->wattsDCurve;
        title = tr("Power Delta");
        }
        break;

    case RideFile::cadd:
        {
        ourCurve = standard->cadDCurve;
        thereCurve = referencePlot->standard->cadDCurve;
        title = tr("Cadence Delta");
        }
        break;

    case RideFile::nmd:
        {
        ourCurve = standard->nmDCurve;
        thereCurve = referencePlot->standard->nmDCurve;
        title = tr("Torque Delta");
        }
        break;

    case RideFile::hrd:
        {
        ourCurve = standard->hrDCurve;
        thereCurve = referencePlot->standard->hrDCurve;
        title = tr("Heartrate Delta");
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
        if (secondaryScope == RideFile::none) {
          ourCurve = standard->altCurve;
          thereCurve = referencePlot->standard->altCurve;
          title = tr("Altitude");
        } else {
            ourASCurve = standard->altSlopeCurve;
            thereASCurve = referencePlot->standard->altSlopeCurve;
            title = tr("Alt/Slope");
        }
        }
        break;

    case RideFile::slope:
        {
        ourCurve = standard->slopeCurve;
        thereCurve = referencePlot->standard->slopeCurve;
        title = tr("Slope");
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

    case RideFile::anTISS:
        {
        ourCurve = standard->antissCurve;
        thereCurve = referencePlot->standard->antissCurve;
        title = tr("Anaerobic TISS");
        }
        break;

    case RideFile::aTISS:
        {
        ourCurve = standard->atissCurve;
        thereCurve = referencePlot->standard->atissCurve;
        title = tr("Aerobic TISS");
        }
        break;

    case RideFile::IsoPower:
        {
        ourCurve = standard->npCurve;
        thereCurve = referencePlot->standard->npCurve;
        title = tr("IsoPower");
        }
        break;

    case RideFile::rvert:
        {
        ourCurve = standard->rvCurve;
        thereCurve = referencePlot->standard->rvCurve;
        title = tr("Vertical Oscillation");
        }
        break;

    case RideFile::rcad:
        {
        ourCurve = standard->rcadCurve;
        thereCurve = referencePlot->standard->rcadCurve;
        title = tr("Run Cadence");
        }
        break;

    case RideFile::rcontact:
        {
        ourCurve = standard->rgctCurve;
        thereCurve = referencePlot->standard->rgctCurve;
        title = tr("GCT");
        }
        break;

    case RideFile::gear:
        {
        ourCurve = standard->gearCurve;
        thereCurve = referencePlot->standard->gearCurve;
        title = tr("Gear Ratio");
        }
        break;

    case RideFile::smo2:
        {
        ourCurve = standard->smo2Curve;
        thereCurve = referencePlot->standard->smo2Curve;
        title = tr("SmO2");
        }
        break;

    case RideFile::thb:
        {
        ourCurve = standard->thbCurve;
        thereCurve = referencePlot->standard->thbCurve;
        title = tr("tHb");
        }
        break;

    case RideFile::o2hb:
        {
        ourCurve = standard->o2hbCurve;
        thereCurve = referencePlot->standard->o2hbCurve;
        title = tr("O2Hb");
        }
        break;

    case RideFile::hhb:
        {
        ourCurve = standard->hhbCurve;
        thereCurve = referencePlot->standard->hhbCurve;
        title = tr("HHb");
        }
        break;

    case RideFile::xPower:
        {
        ourCurve = standard->xpCurve;
        thereCurve = referencePlot->standard->xpCurve;
        title = tr("xPower");
        }
        break;

    case RideFile::lps:
        {
        ourCurve = standard->lpsCurve;
        thereCurve = referencePlot->standard->lpsCurve;
        title = tr("Left Pedal Smoothness");
        }
        break;

    case RideFile::rps:
        {
        ourCurve = standard->rpsCurve;
        thereCurve = referencePlot->standard->rpsCurve;
        title = tr("Right Pedal Smoothness");
        }
        break;

    case RideFile::lte:
        {
        ourCurve = standard->lteCurve;
        thereCurve = referencePlot->standard->lteCurve;
        title = tr("Left Torque Efficiency");
        }
        break;

    case RideFile::rte:
        {
        ourCurve = standard->rteCurve;
        thereCurve = referencePlot->standard->rteCurve;
        title = tr("Right Torque Efficiency");
        }
        break;

    case RideFile::lpco:
    case RideFile::rpco:
        {
        ourCurve = standard->lpcoCurve;
        ourCurve2 = standard->rpcoCurve;
        thereCurve = referencePlot->standard->lpcoCurve;
        thereCurve2 = referencePlot->standard->rpcoCurve;
        title = tr("Left/Right Pedal Center Offset");
        }
        break;

    case RideFile::lppb:
        {
        ourICurve = standard->lppCurve;
        thereICurve = referencePlot->standard->lppCurve;
        title = tr("Left Power Phase");
        }
        break;

    case RideFile::rppb:
        {
        ourICurve = standard->rppCurve;
        thereICurve = referencePlot->standard->rppCurve;
        title = tr("Right Power Phase");
        }
        break;

    case RideFile::lpppb:
        {
        ourICurve = standard->lpppCurve;
        thereICurve = referencePlot->standard->lpppCurve;
        title = tr("Left Peak Power Phase");
        }
        break;

    case RideFile::rpppb:
        {
        ourICurve = standard->rpppCurve;
        thereICurve = referencePlot->standard->rpppCurve;
        title = tr("Right Peak Power Phase");
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
        if (scope > RideFile::none) {

            // user defined series
            int index = scope - (RideFile::none+1);
            ourCurve = standard->U[index].curve;
            thereCurve = referencePlot->standard->U[index].curve;
            title = referencePlot->standard->U[index].name;
        }
    case RideFile::interval:
    case RideFile::vam:
    case RideFile::wattsKg:
    case RideFile::km:
    case RideFile::lon:
    case RideFile::lat:
    case RideFile::none:
        break;
    }

    // lets clone !
    if ((ourCurve && thereCurve) || (ourICurve && thereICurve) || (ourASCurve && thereASCurve)) {

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
            ourCurve->setStyle(thereCurve->style());

            // symbol when zoomed in super close
            if (array.size() < 150) {
                QwtSymbol *sym = new QwtSymbol;
                sym->setPen(QPen(GColor(CPLOTMARKER)));
                sym->setStyle(QwtSymbol::Ellipse);
                sym->setSize(3 *dpiXFactor);
                ourCurve->setSymbol(sym);
            } else {
                QwtSymbol *sym = new QwtSymbol;
                sym->setStyle(QwtSymbol::NoSymbol);
                sym->setSize(0 *dpiXFactor);
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
                sym->setSize(3 *dpiXFactor);
                ourCurve2->setSymbol(sym);
            } else {
                QwtSymbol *sym = new QwtSymbol;
                sym->setStyle(QwtSymbol::NoSymbol);
                sym->setSize(0 *dpiXFactor);
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

        if (ourASCurve && thereASCurve) {
            // no way to get values, so we run through them
            ourASCurve->setVisible(true);
            ourASCurve->attach(this);

            // lets clone the data
            QVector<QPointF> array;
            for (size_t i=0; i<thereASCurve->data()->size(); i++) array << thereASCurve->data()->sample(i);

            ourASCurve->setSamples(array);
            ourASCurve->setYAxis(yLeft);
            ourASCurve->setBaseline(thereASCurve->baseline());
            ourASCurve->setStyle(thereASCurve->style());

            QwtSymbol *sym = new QwtSymbol;
            sym->setStyle(QwtSymbol::NoSymbol);
            sym->setSize(0 *dpiXFactor);
            ourASCurve->setSymbol(sym);
        }

        // x-axis
        if (thereCurve || thereASCurve)
            setAxisScale(QwtPlot::xBottom, referencePlot->axisScaleDiv(xBottom).lowerBound(),
                                           referencePlot->axisScaleDiv(xBottom).upperBound());
        else if (thereICurve)
            setAxisScale(QwtPlot::xBottom, thereICurve->boundingRect().left(), thereICurve->boundingRect().right());

        enableAxis(QwtPlot::xBottom, true);
        setAxisVisible(QwtPlot::xBottom, true);
        setXTitle();

        // y-axis yLeft
        setAxisVisible(yLeft, true);
        if (scope == RideFile::thb && thereCurve) {

            // minimum non-zero value... worst case its zero !
            double minNZ = 0.00f;
            for (size_t i=0; i<thereCurve->data()->size(); i++) {
                if (!minNZ) minNZ = thereCurve->data()->sample(i).y();
                else if (thereCurve->data()->sample(i).y()<minNZ) minNZ = thereCurve->data()->sample(i).y();
            }
            setAxisScale(QwtPlot::yLeft, minNZ, thereCurve->maxYValue() + 0.10f);

        } else if (scope == RideFile::wprime) {

            // always zero or lower (don't truncate)
            double min = thereCurve->minYValue();
            setAxisScale(QwtPlot::yLeft, min > 0 ? 0 : min * 1.1f, 1.1f * thereCurve->maxYValue());

        } else if (scope == RideFile::tcore) {

            // always zero or lower (don't truncate)
            double min = qMin(36.5f, float(thereCurve->minYValue()));
            double max = qMax(39.0f, float(thereCurve->maxYValue()+0.5f));
            setAxisScale(QwtPlot::yLeft, min, max);

        } else if (scope == RideFile::lrbalance) {

            setAxisScale(QwtPlot::yLeft, 0, 100); // 100 %

        } else if (scope == RideFile::lpco || scope == RideFile::rpco) {

            // get the min/max values for both curves
            double min = thereCurve->minYValue();
            double max = thereCurve->maxYValue();
            if (thereCurve2) {
                min = qMin(min, thereCurve2->minYValue());
                max = qMax(max, thereCurve2->maxYValue());
            }
            setAxisScale(QwtPlot::yLeft, min, max); // not more than 15 degrees

        } else if (scope == RideFile::lppb || scope == RideFile::rppb) {

            setAxisScale(QwtPlot::yLeft, 0, 360); // 360 degrees

        } else {
            if (thereCurve)
                setAxisScale(QwtPlot::yLeft, thereCurve->minYValue(), 1.1f * thereCurve->maxYValue());
            if (thereICurve)
                setAxisScale(QwtPlot::yLeft, thereICurve->boundingRect().top(), 1.1f * thereICurve->boundingRect().bottom());
            if (thereASCurve)
                setAxisScale(QwtPlot::yLeft, thereASCurve->minYValue(), 1.1f * thereASCurve->maxYValue());
        }


        ScaleScaleDraw *sd = new ScaleScaleDraw;
        sd->setTickLength(QwtScaleDiv::MajorTick, 3);
        sd->enableComponent(ScaleScaleDraw::Ticks, false);
        sd->enableComponent(ScaleScaleDraw::Backbone, false);
        sd->setMinimumExtent(24);
        sd->setSpacing(0);

        if (scope == RideFile::wprime) sd->setFactor(0.001f); // Kj
        if (scope == RideFile::thb || scope == RideFile::smo2 || scope == RideFile::hrv
            || scope == RideFile::o2hb || scope == RideFile::hhb) // Hb
            sd->setDecimals(2);
        if (scope == RideFile::tcore) sd->setDecimals(1);

        setAxisScaleDraw(QwtPlot::yLeft, sd);

        // title and colour
        setAxisTitle(yLeft, title);
        QPalette pal = palette();
        if (thereCurve) {
            pal.setColor(QPalette::WindowText, thereCurve->pen().color());
            pal.setColor(QPalette::Text, thereCurve->pen().color());
        } else if (thereICurve) {
            pal.setColor(QPalette::WindowText, thereICurve->pen().color());
            pal.setColor(QPalette::Text, thereICurve->pen().color());
        } else if (thereASCurve) {
            pal.setColor(QPalette::WindowText, thereASCurve->pen().color());
            pal.setColor(QPalette::Text, thereASCurve->pen().color());
        }
        axisWidget(QwtPlot::yLeft)->setPalette(pal);

        // hide other y axes
        setAxisVisible(QwtAxisId(QwtAxis::yLeft, 1), false);
        setAxisVisible(QwtAxisId(QwtAxis::yLeft, 3), false);
        setAxisVisible(yRight, false);
        setAxisVisible(QwtAxisId(QwtAxis::yRight, 1), false);
        setAxisVisible(QwtAxisId(QwtAxis::yRight, 2), false);
        setAxisVisible(QwtAxisId(QwtAxis::yRight, 3), false);

        // hide user axes
        for(int k=0; k<standard->U.count(); k++)
            setAxisVisible(QwtAxisId(QwtAxis::yRight, 4 + k), false);

        // plot standard->grid
        standard->grid->setVisible(referencePlot->standard->grid->isVisible());

        // plot markers etc
        refreshIntervalMarkers();
        refreshCalibrationMarkers();
        refreshReferenceLines();
        refreshExhaustions();

#if 0
        refreshZoneLabels();
#endif
    }

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
}

void
AllPlot::setDataFromPlots(QList<AllPlot *> plots)
{
    isolation = false;
    curveColors->saveState();

    // remove all curves from the plot
    for(int k=0; k<standard->U.count(); k++) standard->U[k].curve->detach();
    standard->wCurve->detach();
    standard->mCurve->detach();
    standard->wattsCurve->detach();
    standard->atissCurve->detach();
    standard->antissCurve->detach();
    standard->npCurve->detach();
    standard->rvCurve->detach();
    standard->rcadCurve->detach();
    standard->rgctCurve->detach();
    standard->gearCurve->detach();
    standard->smo2Curve->detach();
    standard->thbCurve->detach();
    standard->o2hbCurve->detach();
    standard->hhbCurve->detach();
    standard->xpCurve->detach();
    standard->apCurve->detach();
    standard->hrCurve->detach();
    standard->hrvCurve->detach();
    standard->tcoreCurve->detach();
    standard->speedCurve->detach();
    standard->accelCurve->detach();
    standard->wattsDCurve->detach();
    standard->cadDCurve->detach();
    standard->nmDCurve->detach();
    standard->hrDCurve->detach();
    standard->cadCurve->detach();
    standard->altCurve->detach();
    standard->altSlopeCurve->detach();
    standard->slopeCurve->detach();
    standard->tempCurve->detach();
    standard->windCurve->detach();
    standard->torqueCurve->detach();
    standard->balanceLCurve->detach();
    standard->balanceRCurve->detach();
    standard->lteCurve->detach();
    standard->rteCurve->detach();
    standard->lpsCurve->detach();
    standard->rpsCurve->detach();
    standard->lpcoCurve->detach();
    standard->rpcoCurve->detach();
    standard->lppCurve->detach();
    standard->rppCurve->detach();
    standard->lpppCurve->detach();
    standard->rpppCurve->detach();

    for(int k=0; k<standard->U.count(); k++) standard->U[k].curve->setVisible(false);
    standard->wCurve->setVisible(false);
    standard->mCurve->setVisible(false);
    standard->wattsCurve->setVisible(false);
    standard->atissCurve->setVisible(false);
    standard->antissCurve->setVisible(false);
    standard->npCurve->setVisible(false);
    standard->rvCurve->setVisible(false);
    standard->rcadCurve->setVisible(false);
    standard->rgctCurve->setVisible(false);
    standard->gearCurve->setVisible(false);
    standard->smo2Curve->setVisible(false);
    standard->thbCurve->setVisible(false);
    standard->o2hbCurve->setVisible(false);
    standard->hhbCurve->setVisible(false);
    standard->xpCurve->setVisible(false);
    standard->apCurve->setVisible(false);
    standard->hrCurve->setVisible(false);
    standard->hrvCurve->setVisible(false);
    standard->tcoreCurve->setVisible(false);
    standard->speedCurve->setVisible(false);
    standard->accelCurve->setVisible(false);
    standard->wattsDCurve->setVisible(false);
    standard->cadDCurve->setVisible(false);
    standard->nmDCurve->setVisible(false);
    standard->hrDCurve->setVisible(false);
    standard->cadCurve->setVisible(false);
    standard->altCurve->setVisible(false);
    standard->altSlopeCurve->setVisible(false);
    standard->slopeCurve->setVisible(false);
    standard->tempCurve->setVisible(false);
    standard->windCurve->setVisible(false);
    standard->torqueCurve->setVisible(false);
    standard->balanceLCurve->setVisible(false);
    standard->balanceRCurve->setVisible(false);
    standard->lteCurve->setVisible(false);
    standard->rteCurve->setVisible(false);
    standard->lpsCurve->setVisible(false);
    standard->rpsCurve->setVisible(false);
    standard->lpcoCurve->setVisible(false);
    standard->rpcoCurve->setVisible(false);
    standard->lppCurve->setVisible(false);
    standard->rppCurve->setVisible(false);
    standard->lpppCurve->setVisible(false);
    standard->rpppCurve->setVisible(false);

    // clear previous curves
    foreach(QwtPlotCurve *prior, compares) {
        prior->detach();
        delete prior;
    }
    compares.clear();

    double MAXY = -100;
    double MINY = 0;

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
        AllPlotSlopeCurve *ourASCurve = NULL, *thereASCurve = NULL;
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

            case RideFile::tcore:
                {
                ourCurve = new QwtPlotCurve(tr("Core Temp"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->tcoreCurve;
                title = tr("Core Temp");
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

            case RideFile::hrv:
                {
                ourCurve = new QwtPlotCurve(tr("R-R"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->hrvCurve;
                title = tr("R-R");
                }
                break;

            case RideFile::kphd:
                {
                ourCurve = new QwtPlotCurve(tr("Acceleration"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->accelCurve;
                title = tr("Acceleration");
                }
                break;

            case RideFile::wattsd:
                {
                ourCurve = new QwtPlotCurve(tr("Power Delta"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->wattsDCurve;
                title = tr("Power Delta");
                }
                break;

            case RideFile::cadd:
                {
                ourCurve = new QwtPlotCurve(tr("Cadence Delta"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->cadDCurve;
                title = tr("Cadence Delta");
                }
                break;

            case RideFile::nmd:
                {
                ourCurve = new QwtPlotCurve(tr("Torque Delta"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->nmDCurve;
                title = tr("Torque Delta");
                }
                break;

            case RideFile::hrd:
                {
                ourCurve = new QwtPlotCurve(tr("Heartrate Delta"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->hrDCurve;
                title = tr("Heartrate Delta");
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
                ourCurve = (QwtPlotCurve*)(new QwtPlotGappedCurve(tr("Power"), 3));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->wattsCurve;
                title = tr("Power");
                }
                break;

            case RideFile::wprime:
                {
                ourCurve = new QwtPlotCurve(tr("W' Balance (kJ)"));
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
               if (secondaryScope != RideFile::slope) {
                   ourCurve = new QwtPlotCurve(tr("Altitude"));
                   ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                   ourCurve->setZ(-10); // always at the back.
                   thereCurve = referencePlot->standard->altCurve;
                   title = tr("Altitude");
                   } else {
                   ourASCurve = new AllPlotSlopeCurve(tr("Alt/Slope"));
                   ourASCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                   ourASCurve->setZ(-5); //
                   thereASCurve = referencePlot->standard->altSlopeCurve;
                   title = tr("Alt/Slope");
                   }
               }
               break;

            case RideFile::slope:
                {
                ourCurve = new QwtPlotCurve(tr("Slope"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->slopeCurve;
                title = tr("Slope");
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

            case RideFile::anTISS:
                {
                ourCurve = new QwtPlotCurve(tr("Anaerobic TISS"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->antissCurve;
                title = tr("Anaerobic TISS");
                }
                break;

            case RideFile::aTISS:
                {
                ourCurve = new QwtPlotCurve(tr("Aerobic TISS"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->atissCurve;
                title = tr("Aerobic TISS");
                }
                break;

            case RideFile::IsoPower:
                {
                ourCurve = new QwtPlotCurve(tr("IsoPower"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->npCurve;
                title = tr("IsoPower");
                }
                break;

            case RideFile::rvert:
                {
                ourCurve = new QwtPlotCurve(tr("Vertical Oscillation"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->rvCurve;
                title = tr("Vertical Oscillation");
                }
                break;

            case RideFile::rcad:
                {
                ourCurve = new QwtPlotCurve(tr("Run Cadence"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->rcadCurve;
                title = tr("Run Cadence");
                }
                break;

            case RideFile::rcontact:
                {
                ourCurve = new QwtPlotCurve(tr("GCT"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->rgctCurve;
                title = tr("GCT");
                }
                break;

            case RideFile::gear:
                {
                ourCurve = new QwtPlotCurve(tr("Gear Ratio"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->gearCurve;
                title = tr("Gear Ratio");
                }
                break;

            case RideFile::smo2:
                {
                ourCurve = new QwtPlotCurve(tr("SmO2"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->smo2Curve;
                title = tr("SmO2");
                }
                break;

            case RideFile::thb:
                {
                ourCurve = new QwtPlotCurve(tr("tHb"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->thbCurve;
                title = tr("tHb");
                }
                break;

            case RideFile::o2hb:
                {
                ourCurve = new QwtPlotCurve(tr("O2Hb"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->o2hbCurve;
                title = tr("O2Hb");
                }
                break;

            case RideFile::hhb:
                {
                ourCurve = new QwtPlotCurve(tr("HHb"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->hhbCurve;
                title = tr("HHb");
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

            case RideFile::lps:
                {
                ourCurve = new QwtPlotCurve(tr("Left Pedal Smoothness"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->lpsCurve;
                title = tr("Left Pedal Smoothness");
                }
                break;

            case RideFile::rps:
                {
                ourCurve = new QwtPlotCurve(tr("Right Pedal Smoothness"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->rpsCurve;
                title = tr("Right Pedal Smoothness");
                }
                break;

            case RideFile::lte:
                {
                ourCurve = new QwtPlotCurve(tr("Left Torque Efficiency"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->lteCurve;
                title = tr("Left Torque Efficiency");
                }
                break;

            case RideFile::rte:
                {
                ourCurve = new QwtPlotCurve(tr("Right Torque Efficiency"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->rteCurve;
                title = tr("Right Torque Efficiency");
                }
                break;

            case RideFile::rpco:
            case RideFile::lpco:
                {
                ourCurve = new QwtPlotCurve(tr("Left Pedal Center Offset"));
                ourCurve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve = referencePlot->standard->lpcoCurve;
                ourCurve2 = new QwtPlotCurve(tr("Right Pedal Center Offset"));
                ourCurve2->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
                thereCurve2 = referencePlot->standard->rpcoCurve;
                title = tr("Left/Right Pedal Center Offset");
                }
                break;

            case RideFile::lppb:
            case RideFile::lppe:
                {
                ourICurve = new QwtPlotIntervalCurve(tr("Left Power Phase"));
                thereICurve = referencePlot->standard->lppCurve;
                title = tr("Left Power Phase");
                }
                break;

            case RideFile::rppb:
            case RideFile::rppe:
                {
                ourICurve = new QwtPlotIntervalCurve(tr("Right Power Phase"));
                thereICurve = referencePlot->standard->rppCurve;
                title = tr("Right Power Phase");
                }
                break;

            case RideFile::lpppb:
            case RideFile::lpppe:
                {
                ourICurve = new QwtPlotIntervalCurve(tr("Left Peak Power Phase"));
                thereICurve = referencePlot->standard->lpppCurve;
                title = tr("Left Peak Power Phase");
                }
                break;

            case RideFile::rpppb:
            case RideFile::rpppe:
                {
                ourICurve = new QwtPlotIntervalCurve(tr("Right Peak Power Phase"));
                thereICurve = referencePlot->standard->rpppCurve;
                title = tr("Right Peak Power Phase");
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
                if (scope > RideFile::none) {

                    // user defined series
                    int index = scope - (RideFile::none+1);
                    ourCurve = static_cast<QwtPlotCurve*>(new QwtPlotGappedCurve(referencePlot->standard->U[index].name, 3));
                    thereCurve = referencePlot->standard->U[index].curve;
                    title = referencePlot->standard->U[index].name;
                }
            case RideFile::interval:
            case RideFile::vam:
            case RideFile::wattsKg:
            case RideFile::km:
            case RideFile::lon:
            case RideFile::lat:
            case RideFile::none:
                break;
            }

            bool antialias = appsettings->value(this, GC_ANTIALIAS, true).toBool();

            // lets clone !
            if ((ourCurve && thereCurve) || (ourICurve && thereICurve) || (ourASCurve && thereASCurve)) {

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
                    QVector<double> x,y;
                    for (size_t i=0; i<thereCurve->data()->size(); i++) {
                        x << thereCurve->data()->sample(i).x();
                        y << thereCurve->data()->sample(i).y();
                    }

                    ourCurve->setSamples(x,y);
                    ourCurve->setYAxis(yLeft);
                    ourCurve->setBaseline(thereCurve->baseline());

                    if (ourCurve->maxYValue() > MAXY) MAXY = ourCurve->maxYValue();
                    if (ourCurve->minYValue() < MINY) MINY = ourCurve->minYValue();

                    // symbol when zoomed in super close
                    if (x.size() < 150) {
                        QwtSymbol *sym = new QwtSymbol;
                        sym->setPen(QPen(GColor(CPLOTMARKER)));
                        sym->setStyle(QwtSymbol::Ellipse);
                        sym->setSize(3 *dpiXFactor);
                        ourCurve->setSymbol(sym);
                    } else {
                        QwtSymbol *sym = new QwtSymbol;
                        sym->setStyle(QwtSymbol::NoSymbol);
                        sym->setSize(0 *dpiXFactor);
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
                    if (ourCurve2->minYValue() < MINY) MINY = ourCurve2->minYValue();

                    // symbol when zoomed in super close
                    if (array.size() < 150) {
                        QwtSymbol *sym = new QwtSymbol;
                        sym->setPen(QPen(GColor(CPLOTMARKER)));
                        sym->setStyle(QwtSymbol::Ellipse);
                        sym->setSize(3 *dpiXFactor);
                        ourCurve2->setSymbol(sym);
                    } else {
                        QwtSymbol *sym = new QwtSymbol;
                        sym->setStyle(QwtSymbol::NoSymbol);
                        sym->setSize(0 *dpiXFactor);
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

                if (ourASCurve && thereASCurve) {

                    // remember for next time...
                    compares << ourASCurve;

                    // colours etc
                    if (antialias) ourASCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
                    QPen pen = thereASCurve->pen();
                    pen.setColor(context->compareIntervals[index].color);
                    ourASCurve->setPen(pen);
                    ourASCurve->setVisible(true);
                    ourASCurve->attach(this);

                    // lets clone the data
                    QVector<QPointF> array;
                    for (size_t i=0; i<thereASCurve->data()->size(); i++) array << thereASCurve->data()->sample(i);

                    ourASCurve->setSamples(array);
                    ourASCurve->setYAxis(yLeft);
                    ourASCurve->setBaseline(thereASCurve->baseline());
                    setAltSlopePlotStyle (ourASCurve);

                    if (ourASCurve->maxYValue() > MAXY) MAXY = ourASCurve->maxYValue();
                    if (ourASCurve->minYValue() < MINY) MINY = ourASCurve->minYValue();

                    QwtSymbol *sym = new QwtSymbol;
                    sym->setStyle(QwtSymbol::NoSymbol);
                    sym->setSize(0 *dpiXFactor);
                    ourASCurve->setSymbol(sym);

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
    ScaleScaleDraw *sd = new ScaleScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(ScaleScaleDraw::Ticks, false);
    sd->enableComponent(ScaleScaleDraw::Backbone, false);
    if (scope == RideFile::wprime) sd->setFactor(0.001f); // Kj
    setAxisScaleDraw(QwtPlot::yLeft, sd);

    // set the y-axis for largest value we saw +10%
    setAxisScale(QwtPlot::yLeft, MINY * 1.10f , MAXY * 1.10f);

    // hide other y axes
    setAxisVisible(QwtAxisId(QwtAxis::yLeft, 1), false);
    setAxisVisible(QwtAxisId(QwtAxis::yLeft, 3), false);
    setAxisVisible(yRight, false);
    setAxisVisible(QwtAxisId(QwtAxis::yRight, 1), false);
    setAxisVisible(QwtAxisId(QwtAxis::yRight, 2), false);
    setAxisVisible(QwtAxisId(QwtAxis::yRight, 3), false);

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
    refreshExhaustions();

    // always draw against yLeft in series mode
    intervalHighlighterCurve->setYAxis(yLeft);
    if (thereCurve)
        intervalHighlighterCurve->setBaseline(thereCurve->minYValue());
    else if (thereICurve)
        intervalHighlighterCurve->setBaseline(thereICurve->boundingRect().top());
#if 0
#endif
#endif

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
}

// used to setup array of allplots where there is one for
// each interval in compare mode
void 
AllPlot::setDataFromObject(AllPlotObject *object, AllPlot *reference)
{
    referencePlot = reference;
    bydist = reference->bydist;
    bytimeofday = reference->bytimeofday;
    timeoffset = reference->timeoffset;

    // remove all curves from the plot
    for(int k=0; k<standard->U.count(); k++) standard->U[k].curve->detach();
    standard->wCurve->detach();
    standard->mCurve->detach();
    standard->wattsCurve->detach();
    standard->atissCurve->detach();
    standard->antissCurve->detach();
    standard->npCurve->detach();
    standard->rvCurve->detach();
    standard->rcadCurve->detach();
    standard->rgctCurve->detach();
    standard->gearCurve->detach();
    standard->smo2Curve->detach();
    standard->thbCurve->detach();
    standard->o2hbCurve->detach();
    standard->hhbCurve->detach();
    standard->xpCurve->detach();
    standard->apCurve->detach();
    standard->hrCurve->detach();
    standard->hrvCurve->detach();
    standard->tcoreCurve->detach();
    standard->speedCurve->detach();
    standard->accelCurve->detach();
    standard->wattsDCurve->detach();
    standard->cadDCurve->detach();
    standard->nmDCurve->detach();
    standard->hrDCurve->detach();
    standard->cadCurve->detach();
    standard->altCurve->detach();
    standard->altSlopeCurve->detach();
    standard->slopeCurve->detach();
    standard->tempCurve->detach();
    standard->windCurve->detach();
    standard->torqueCurve->detach();
    standard->balanceLCurve->detach();
    standard->balanceRCurve->detach();
    standard->lteCurve->detach();
    standard->rteCurve->detach();
    standard->lpsCurve->detach();
    standard->rpsCurve->detach();
    standard->intervalHighlighterCurve->detach();
    standard->intervalHoverCurve->detach();

    for(int k=0; k<standard->U.count(); k++) standard->U[k].curve->setVisible(false);
    standard->wCurve->setVisible(false);
    standard->mCurve->setVisible(false);
    standard->wattsCurve->setVisible(false);
    standard->atissCurve->setVisible(false);
    standard->antissCurve->setVisible(false);
    standard->npCurve->setVisible(false);
    standard->rvCurve->setVisible(false);
    standard->rcadCurve->setVisible(false);
    standard->rgctCurve->setVisible(false);
    standard->gearCurve->setVisible(false);
    standard->smo2Curve->setVisible(false);
    standard->thbCurve->setVisible(false);
    standard->o2hbCurve->setVisible(false);
    standard->hhbCurve->setVisible(false);
    standard->xpCurve->setVisible(false);
    standard->apCurve->setVisible(false);
    standard->hrCurve->setVisible(false);
    standard->hrvCurve->setVisible(false);
    standard->tcoreCurve->setVisible(false);
    standard->speedCurve->setVisible(false);
    standard->accelCurve->setVisible(false);
    standard->wattsDCurve->setVisible(false);
    standard->cadDCurve->setVisible(false);
    standard->nmDCurve->setVisible(false);
    standard->hrDCurve->setVisible(false);
    standard->cadCurve->setVisible(false);
    standard->altCurve->setVisible(false);
    standard->altSlopeCurve->setVisible(false);
    standard->slopeCurve->setVisible(false);
    standard->tempCurve->setVisible(false);
    standard->windCurve->setVisible(false);
    standard->torqueCurve->setVisible(false);
    standard->balanceLCurve->setVisible(false);
    standard->balanceRCurve->setVisible(false);
    standard->lteCurve->setVisible(false);
    standard->rteCurve->setVisible(false);
    standard->lpsCurve->setVisible(false);
    standard->rpsCurve->setVisible(false);
    standard->intervalHighlighterCurve->setVisible(false);
    standard->intervalHoverCurve->setVisible(false);

    // NOW SET OUR CURVES USING THEIR DATA ...
    QVector<double> &xaxis = referencePlot->bydist ? object->smoothDistance : object->smoothTime;

    int totalPoints = xaxis.count();

    //W' curve set to whatever data we have
    if (!object->wprime.empty()) {
        standard->wCurve->setSamples(bydist ? object->wprimeDist.data() : object->wprimeTime.data(), 
                                    object->wprime.data(), object->wprime.count());
        standard->mCurve->setSamples(bydist ? object->matchDist.data() : object->matchTime.data(), 
                                    object->match.data(), object->match.count());
        setMatchLabels(standard);
    }

    // show user data curves
    for(int k=0; k< standard->U.count() && k<object->U.count(); k++) {

        if (!object->U[k].smooth.empty()) {

            standard->U[k].curve->setSamples(xaxis.data(), object->U[k].smooth.data(), totalPoints);
            standard->U[k].curve->attach(this);
            standard->U[k].curve->setVisible(true);
        }
    }

    if (!object->wattsArray.empty()) {
        standard->wattsCurve->setSamples(xaxis.data(), object->smoothWatts.data(), totalPoints);
        standard->wattsCurve->attach(this);
        standard->wattsCurve->setVisible(true);
    }

    if (!object->antissArray.empty()) {
        standard->antissCurve->setSamples(xaxis.data(), object->smoothANT.data(), totalPoints);
        standard->antissCurve->attach(this);
        standard->antissCurve->setVisible(true);
    }

    if (!object->atissArray.empty()) {
        standard->atissCurve->setSamples(xaxis.data(), object->smoothAT.data(), totalPoints);
        standard->atissCurve->attach(this);
        standard->atissCurve->setVisible(true);
    }

    if (!object->npArray.empty()) {
        standard->npCurve->setSamples(xaxis.data(), object->smoothNP.data(), totalPoints);
        standard->npCurve->attach(this);
        standard->npCurve->setVisible(true);
    }

    if (!object->rvArray.empty()) {
        standard->rvCurve->setSamples(xaxis.data(), object->smoothRV.data(), totalPoints);
        standard->rvCurve->attach(this);
        standard->rvCurve->setVisible(true);
    }

    if (!object->rcadArray.empty()) {
        standard->rcadCurve->setSamples(xaxis.data(), object->smoothRCad.data(), totalPoints);
        standard->rcadCurve->attach(this);
        standard->rcadCurve->setVisible(true);
    }

    if (!object->rgctArray.empty()) {
        standard->rgctCurve->setSamples(xaxis.data(), object->smoothRGCT.data(), totalPoints);
        standard->rgctCurve->attach(this);
        standard->rgctCurve->setVisible(true);
    }

    if (!object->gearArray.empty()) {
        standard->gearCurve->setSamples(xaxis.data(), object->smoothGear.data(), totalPoints);
        standard->gearCurve->attach(this);
        standard->gearCurve->setVisible(true);
    }

    if (!object->smo2Array.empty()) {
        standard->smo2Curve->setSamples(xaxis.data(), object->smoothSmO2.data(), totalPoints);
        standard->smo2Curve->attach(this);
        standard->smo2Curve->setVisible(true);
    }

    if (!object->thbArray.empty()) {
        standard->thbCurve->setSamples(xaxis.data(), object->smoothtHb.data(), totalPoints);
        standard->thbCurve->attach(this);
        standard->thbCurve->setVisible(true);
    }

    if (!object->o2hbArray.empty()) {
        standard->o2hbCurve->setSamples(xaxis.data(), object->smoothO2Hb.data(), totalPoints);
        standard->o2hbCurve->attach(this);
        standard->o2hbCurve->setVisible(true);
    }

    if (!object->hhbArray.empty()) {
        standard->hhbCurve->setSamples(xaxis.data(), object->smoothHHb.data(), totalPoints);
        standard->hhbCurve->attach(this);
        standard->hhbCurve->setVisible(true);
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

    if (!object->tcoreArray.empty()) {
        standard->tcoreCurve->setSamples(xaxis.data(), object->smoothTcore.data(), totalPoints);
        standard->tcoreCurve->attach(this);
        standard->tcoreCurve->setVisible(true);
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

    if (!object->accelArray.empty()) {
        standard->accelCurve->setSamples(xaxis.data(), object->smoothAccel.data(), totalPoints);
        standard->accelCurve->attach(this);
        standard->accelCurve->setVisible(true);
    }

    if (!object->wattsDArray.empty()) {
        standard->wattsDCurve->setSamples(xaxis.data(), object->smoothWattsD.data(), totalPoints);
        standard->wattsDCurve->attach(this);
        standard->wattsDCurve->setVisible(true);
    }

    if (!object->cadDArray.empty()) {
        standard->cadDCurve->setSamples(xaxis.data(), object->smoothCadD.data(), totalPoints);
        standard->cadDCurve->attach(this);
        standard->cadDCurve->setVisible(true);
    }

    if (!object->nmDArray.empty()) {
        standard->nmDCurve->setSamples(xaxis.data(), object->smoothNmD.data(), totalPoints);
        standard->nmDCurve->attach(this);
        standard->nmDCurve->setVisible(true);
    }

    if (!object->hrDArray.empty()) {
        standard->hrDCurve->setSamples(xaxis.data(), object->smoothHrD.data(), totalPoints);
        standard->hrDCurve->attach(this);
        standard->hrDCurve->setVisible(true);
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
        standard->altSlopeCurve->setSamples(xaxis.data(), object->smoothAltitude.data(), totalPoints);
        standard->altSlopeCurve->attach(this);
        standard->altSlopeCurve->setVisible(true);
    }

    if (!object->slopeArray.empty()) {
        standard->slopeCurve->setSamples(xaxis.data(), object->smoothSlope.data(), totalPoints);
        standard->slopeCurve->attach(this);
        standard->slopeCurve->setVisible(true);
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

    if (!object->lteArray.empty()) {
        standard->lteCurve->setSamples(xaxis.data(), object->smoothLTE.data(), totalPoints);
        standard->rteCurve->setSamples(xaxis.data(), object->smoothRTE.data(), totalPoints);
        standard->lteCurve->attach(this);
        standard->lteCurve->setVisible(true);
        standard->rteCurve->attach(this);
        standard->rteCurve->setVisible(true);
    }

    if (!object->lpsArray.empty()) {
        standard->lpsCurve->setSamples(xaxis.data(), object->smoothLPS.data(), totalPoints);
        standard->rpsCurve->setSamples(xaxis.data(), object->smoothRPS.data(), totalPoints);
        standard->lpsCurve->attach(this);
        standard->lpsCurve->setVisible(true);
        standard->rpsCurve->attach(this);
        standard->rpsCurve->setVisible(true);
    }

    if (!object->lpcoArray.empty()) {
        standard->lpcoCurve->setSamples(xaxis.data(), object->smoothLPCO.data(), totalPoints);
        standard->rpcoCurve->setSamples(xaxis.data(), object->smoothRPCO.data(), totalPoints);
        standard->lpcoCurve->attach(this);
        standard->lpcoCurve->setVisible(true);
        standard->rpcoCurve->attach(this);
        standard->rpcoCurve->setVisible(true);
    }

    if (!object->lppbArray.empty()) {
        standard->lppCurve->setSamples(new QwtIntervalSeriesData(object->smoothLPP));
        standard->lppCurve->attach(this);
        standard->lppCurve->setVisible(true);
    }

    if (!object->rppbArray.empty()) {
        standard->rppCurve->setSamples(new QwtIntervalSeriesData(object->smoothRPP));
        standard->rppCurve->attach(this);
        standard->rppCurve->setVisible(true);
    }

    if (!object->lpppbArray.empty()) {
        standard->lpppCurve->setSamples(new QwtIntervalSeriesData(object->smoothLPPP));
        standard->lpppCurve->attach(this);
        standard->lpppCurve->setVisible(true);
    }

    if (!object->rpppbArray.empty()) {
        standard->rpppCurve->setSamples(new QwtIntervalSeriesData(object->smoothRPPP));
        standard->rpppCurve->attach(this);
        standard->rpppCurve->setVisible(true);
    }

    // to the max / min
    standard->grid->detach();

    // honour user preferences
    standard->wCurve->setVisible(referencePlot->showW);
    standard->mCurve->setVisible(referencePlot->showW);
    standard->wattsCurve->setVisible(referencePlot->showPowerState < 2);
    standard->npCurve->setVisible(referencePlot->showNP);
    standard->rvCurve->setVisible(referencePlot->showRV);
    standard->rcadCurve->setVisible(referencePlot->showRCad);
    standard->rgctCurve->setVisible(referencePlot->showRGCT);
    standard->gearCurve->setVisible(referencePlot->showGear);
    standard->smo2Curve->setVisible(referencePlot->showSmO2);
    standard->thbCurve->setVisible(referencePlot->showtHb);
    standard->o2hbCurve->setVisible(referencePlot->showO2Hb);
    standard->hhbCurve->setVisible(referencePlot->showHHb);
    standard->atissCurve->setVisible(referencePlot->showATISS);
    standard->antissCurve->setVisible(referencePlot->showANTISS);
    standard->xpCurve->setVisible(referencePlot->showXP);
    standard->apCurve->setVisible(referencePlot->showAP);
    standard->hrCurve->setVisible(referencePlot->showHr);
    standard->hrvCurve->setVisible(referencePlot->showHRV);
    standard->tcoreCurve->setVisible(referencePlot->showTcore);
    standard->speedCurve->setVisible(referencePlot->showSpeed);
    standard->accelCurve->setVisible(referencePlot->showAccel);
    standard->wattsDCurve->setVisible(referencePlot->showPowerD);
    standard->cadDCurve->setVisible(referencePlot->showCadD);
    standard->nmDCurve->setVisible(referencePlot->showTorqueD);
    standard->hrDCurve->setVisible(referencePlot->showHrD);
    standard->cadCurve->setVisible(referencePlot->showCad);
    standard->altCurve->setVisible(referencePlot->showAlt);
    standard->altSlopeCurve->setVisible(referencePlot->showAltSlopeState > 0);
    standard->slopeCurve->setVisible(referencePlot->showSlope);
    standard->tempCurve->setVisible(referencePlot->showTemp);
    standard->windCurve->setVisible(referencePlot->showWind);
    standard->torqueCurve->setVisible(referencePlot->showWind);
    standard->balanceLCurve->setVisible(referencePlot->showBalance);
    standard->balanceRCurve->setVisible(referencePlot->showBalance);
    standard->lteCurve->setVisible(referencePlot->showTE);
    standard->rteCurve->setVisible(referencePlot->showTE);
    standard->lpsCurve->setVisible(referencePlot->showPS);
    standard->rpsCurve->setVisible(referencePlot->showPS);
    standard->lpcoCurve->setVisible(referencePlot->showPCO);
    standard->rpcoCurve->setVisible(referencePlot->showPCO);
    standard->lppCurve->setVisible(referencePlot->showDC);
    standard->rppCurve->setVisible(referencePlot->showDC);
    standard->lpppCurve->setVisible(referencePlot->showPPP);
    standard->rpppCurve->setVisible(referencePlot->showPPP);

    // set xaxis -- but not min/max as we get called during smoothing
    //              and massively quicker to reuse data and replot
    setXTitle();
    enableAxis(xBottom, true);
    setAxisVisible(xBottom, true);

    // set the y-axis scales now
    referencePlot = NULL;
    setAltSlopePlotStyle(standard->altSlopeCurve);
    setYMax();

    // refresh zone background (if needed)
    if (shade_zones) {
        bg->attach(this);
        refreshZoneLabels();
    } else
        bg->detach();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();

    replot();
}

void
AllPlot::setDataFromRide(RideItem *_rideItem, QList<UserData*>user)
{
    rideItem = _rideItem;
    if (_rideItem == NULL) return;

    // we don't have a reference plot
    referencePlot = NULL;

    // basically clear out
    //standard->wattsArray.clear();
    //standard->curveTitle.setLabel(QwtText(QString(""), QwtText::PlainText)); // default to no title

    setDataFromRideFile(rideItem->ride(), standard, user);

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
}

void
AllPlot::setDataFromRideFile(RideFile *ride, AllPlotObject *here, QList<UserData*>user)
{
    if (ride && ride->dataPoints().size()) {
        const RideFileDataPresent *dataPresent = ride->areDataPresent();
        int npoints = ride->dataPoints().size();

        // fetch w' bal data
        here->match = ride->wprimeData()->mydata();
        here->matchTime = ride->wprimeData()->mxdata(false);
        here->matchDist = ride->wprimeData()->mxdata(true);
        here->wprime = ride->wprimeData()->ydata();
        here->wprimeTime = ride->wprimeData()->xdata(false);
        here->wprimeDist = ride->wprimeData()->xdata(true);

        here->wattsArray.resize(dataPresent->watts ? npoints : 0);
        here->atissArray.resize(dataPresent->watts ? npoints : 0);
        here->antissArray.resize(dataPresent->watts ? npoints : 0);
        here->npArray.resize(dataPresent->np ? npoints : 0);
        here->rcadArray.resize(dataPresent->rcad ? npoints : 0);
        here->rvArray.resize(dataPresent->rvert ? npoints : 0);
        here->rgctArray.resize(dataPresent->rcontact ? npoints : 0);
        here->smo2Array.resize(dataPresent->smo2 ? npoints : 0);
        here->thbArray.resize(dataPresent->thb ? npoints : 0);
        here->o2hbArray.resize(dataPresent->o2hb ? npoints : 0);
        here->hhbArray.resize(dataPresent->hhb ? npoints : 0);
        here->gearArray.resize(dataPresent->gear ? npoints : 0);
        here->xpArray.resize(dataPresent->xp ? npoints : 0);
        here->apArray.resize(dataPresent->apower ? npoints : 0);
        here->hrArray.resize(dataPresent->hr ? npoints : 0);
        here->tcoreArray.resize(dataPresent->hr ? npoints : 0);
        here->speedArray.resize(dataPresent->kph ? npoints : 0);
        here->accelArray.resize(dataPresent->kph ? npoints : 0);
        here->wattsDArray.resize(dataPresent->watts ? npoints : 0);
        here->cadDArray.resize(dataPresent->cad ? npoints : 0);
        here->nmDArray.resize(dataPresent->nm ? npoints : 0);
        here->hrDArray.resize(dataPresent->hr ? npoints : 0);
        here->cadArray.resize(dataPresent->cad ? npoints : 0);
        here->altArray.resize(dataPresent->alt ? npoints : 0);
        here->slopeArray.resize(dataPresent->slope ? npoints : 0);
        here->tempArray.resize(dataPresent->temp ? npoints : 0);
        here->windArray.resize(dataPresent->headwind ? npoints : 0);
        here->torqueArray.resize(dataPresent->nm ? npoints : 0);
        here->balanceArray.resize(dataPresent->lrbalance ? npoints : 0);
        here->lteArray.resize(dataPresent->lte ? npoints : 0);
        here->rteArray.resize(dataPresent->rte ? npoints : 0);
        here->lpsArray.resize(dataPresent->lps ? npoints : 0);
        here->rpsArray.resize(dataPresent->rps ? npoints : 0);
        here->lpcoArray.resize(dataPresent->lpco ? npoints : 0);
        here->rpcoArray.resize(dataPresent->rpco ? npoints : 0);
        here->lppbArray.resize(dataPresent->lppb ? npoints : 0);
        here->rppbArray.resize(dataPresent->rppb ? npoints : 0);
        here->lppeArray.resize(dataPresent->lppe ? npoints : 0);
        here->rppeArray.resize(dataPresent->rppe ? npoints : 0);
        here->lpppbArray.resize(dataPresent->lpppb ? npoints : 0);
        here->rpppbArray.resize(dataPresent->rpppb ? npoints : 0);
        here->lpppeArray.resize(dataPresent->lpppe ? npoints : 0);
        here->rpppeArray.resize(dataPresent->rpppe ? npoints : 0);
        here->timeArray.resize(npoints);
        here->distanceArray.resize(npoints);
        for(int k=0; k<here->U.count(); k++) here->U[k].array.resize(npoints);

        // attach appropriate curves
        here->wCurve->detach();
        here->mCurve->detach();
        here->wattsCurve->detach();
        here->atissCurve->detach();
        here->antissCurve->detach();
        here->npCurve->detach();
        here->rcadCurve->detach();
        here->rvCurve->detach();
        here->rgctCurve->detach();
        here->gearCurve->detach();
        here->smo2Curve->detach();
        here->thbCurve->detach();
        here->o2hbCurve->detach();
        here->hhbCurve->detach();
        here->xpCurve->detach();
        here->apCurve->detach();
        here->hrCurve->detach();
        here->hrvCurve->detach();
        here->tcoreCurve->detach();
        here->speedCurve->detach();
        here->accelCurve->detach();
        here->wattsDCurve->detach();
        here->cadDCurve->detach();
        here->nmDCurve->detach();
        here->hrDCurve->detach();
        here->cadCurve->detach();
        here->altCurve->detach();
        here->altSlopeCurve->detach();
        here->slopeCurve->detach();
        here->tempCurve->detach();
        here->windCurve->detach();
        here->torqueCurve->detach();
        here->balanceLCurve->detach();
        here->balanceRCurve->detach();
        here->lteCurve->detach();
        here->rteCurve->detach();
        here->lpsCurve->detach();
        here->rpsCurve->detach();
        here->lpcoCurve->detach();
        here->rpcoCurve->detach();
        here->lppCurve->detach();
        here->rppCurve->detach();
        here->lpppCurve->detach();
        here->rpppCurve->detach();

        if (!here->altArray.empty()) {
            here->altCurve->attach(this);
            here->altSlopeCurve->attach(this);
        }
        if (!here->slopeArray.empty()) here->slopeCurve->attach(this);
        if (!here->wattsArray.empty()) here->wattsCurve->attach(this);
        if (!here->atissArray.empty()) here->atissCurve->attach(this);
        if (!here->antissArray.empty()) here->antissCurve->attach(this);
        if (!here->npArray.empty()) here->npCurve->attach(this);
        if (!here->rvArray.empty()) here->rvCurve->attach(this);
        if (!here->rcadArray.empty()) here->rcadCurve->attach(this);
        if (!here->rgctArray.empty()) here->rgctCurve->attach(this);
        if (!here->gearArray.empty()) here->gearCurve->attach(this);
        if (!here->smo2Array.empty()) here->smo2Curve->attach(this);
        if (!here->thbArray.empty()) here->thbCurve->attach(this);
        if (!here->o2hbArray.empty()) here->o2hbCurve->attach(this);
        if (!here->hhbArray.empty()) here->hhbCurve->attach(this);
        if (!here->xpArray.empty()) here->xpCurve->attach(this);
        if (!here->apArray.empty()) here->apCurve->attach(this);
        if (showW && ride && !here->wprime.empty()) {
            here->wCurve->attach(this);
            here->mCurve->attach(this);
        }
        if (!here->hrArray.empty()) here->hrCurve->attach(this);
        if (dataPresent->hrv) here->hrvCurve->attach(this);
        if (!here->tcoreArray.empty()) here->tcoreCurve->attach(this);
        if (!here->speedArray.empty()) here->speedCurve->attach(this);

        // deltas
        if (!here->accelArray.empty()) here->accelCurve->attach(this);
        if (!here->wattsDArray.empty()) here->wattsDCurve->attach(this);
        if (!here->cadDArray.empty()) here->cadDCurve->attach(this);
        if (!here->nmDArray.empty()) here->nmDCurve->attach(this);
        if (!here->hrDArray.empty()) here->hrDCurve->attach(this);

        if (!here->cadArray.empty()) here->cadCurve->attach(this);
        if (!here->tempArray.empty()) here->tempCurve->attach(this);
        if (!here->windArray.empty()) here->windCurve->attach(this);
        if (!here->torqueArray.empty()) here->torqueCurve->attach(this);
        if (!here->lteArray.empty()) {
            here->lteCurve->attach(this);
            here->rteCurve->attach(this);
        }
        if (!here->lpsArray.empty()) {
            here->lpsCurve->attach(this);
            here->rpsCurve->attach(this);
        }
        if (!here->balanceArray.empty()) {
            here->balanceLCurve->attach(this);
            here->balanceRCurve->attach(this);
        }
        if (!here->lpcoArray.empty()) {
            here->lpcoCurve->attach(this);
            here->rpcoCurve->attach(this);
        }
        if (!here->lppbArray.empty()) {
            here->lppCurve->attach(this);
            here->rppCurve->attach(this);
        }
        if (!here->lpppbArray.empty()) {
            here->lpppCurve->attach(this);
            here->rpppCurve->attach(this);
        }

        here->wCurve->setVisible(dataPresent->watts && showW);
        here->mCurve->setVisible(dataPresent->watts && showW);
        here->wattsCurve->setVisible(dataPresent->watts && showPowerState < 2);
        here->atissCurve->setVisible(dataPresent->watts && showATISS);
        here->antissCurve->setVisible(dataPresent->watts && showANTISS);
        here->npCurve->setVisible(dataPresent->np && showNP);
        here->rcadCurve->setVisible(dataPresent->rcad && showRCad);
        here->rvCurve->setVisible(dataPresent->rvert && showRV);
        here->rgctCurve->setVisible(dataPresent->rcontact && showRGCT);
        here->gearCurve->setVisible(dataPresent->gear && showGear);
        here->smo2Curve->setVisible(dataPresent->smo2 && showSmO2);
        here->thbCurve->setVisible(dataPresent->thb && showtHb);
        here->o2hbCurve->setVisible(dataPresent->o2hb && showO2Hb);
        here->hhbCurve->setVisible(dataPresent->hhb && showHHb);
        here->xpCurve->setVisible(dataPresent->xp && showXP);
        here->apCurve->setVisible(dataPresent->apower && showAP);
        here->hrCurve->setVisible(dataPresent->hr && showHr);
        here->hrvCurve->setVisible(dataPresent->hrv && showHRV);
        here->tcoreCurve->setVisible(dataPresent->hr && showTcore);
        here->speedCurve->setVisible(dataPresent->kph && showSpeed);
        here->cadCurve->setVisible(dataPresent->cad && showCad);
        here->altCurve->setVisible(dataPresent->alt && showAlt);
        here->altSlopeCurve->setVisible(dataPresent->alt && showAltSlopeState > 0);
        here->slopeCurve->setVisible(dataPresent->slope && showSlope);
        here->tempCurve->setVisible(dataPresent->temp && showTemp);
        here->windCurve->setVisible(dataPresent->headwind && showWind);
        here->torqueCurve->setVisible(dataPresent->nm && showWind);
        here->lteCurve->setVisible(dataPresent->lte && showTE);
        here->rteCurve->setVisible(dataPresent->rte && showTE);
        here->lpsCurve->setVisible(dataPresent->lps && showPS);
        here->rpsCurve->setVisible(dataPresent->rps && showPS);
        here->balanceLCurve->setVisible(dataPresent->lrbalance && showBalance);
        here->balanceRCurve->setVisible(dataPresent->lrbalance && showBalance);
        here->lpcoCurve->setVisible(dataPresent->lpco && showPCO);
        here->rpcoCurve->setVisible(dataPresent->rpco && showPCO);
        here->lppCurve->setVisible(dataPresent->lppb && showDC);
        here->rppCurve->setVisible(dataPresent->rppb && showDC);
        here->lpppCurve->setVisible(dataPresent->lpppb && showPPP);
        here->rpppCurve->setVisible(dataPresent->rpppb && showPPP);

        // deltas
        here->accelCurve->setVisible(dataPresent->kph && showAccel);
        here->wattsDCurve->setVisible(dataPresent->watts && showPowerD);
        here->cadDCurve->setVisible(dataPresent->cad && showCadD);
        here->nmDCurve->setVisible(dataPresent->nm && showTorqueD);
        here->hrDCurve->setVisible(dataPresent->hr && showHrD);

        int arrayLength = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {

            // we round the time to nearest 100th of a second
            // before adding to the array, to avoid situation
            // where 'high precision' time slice is an artefact
            // of double precision or slight timing anomalies
            // e.g. where realtime gives timestamps like
            // 940.002 followed by 940.998 and were previously
            // both rounded to 940s
            //
            // NOTE: this rounding mechanism is identical to that
            //       used by the Ride Editor.
            double secs = floor(point->secs);
            double msecs = round((point->secs - secs) * 100) * 10;

            here->timeArray[arrayLength]  = secs + msecs/1000;

            for(int k=0; k<here->U.count() && k<user.count(); k++) {
                here->U[k].array[arrayLength] = user[k]->vector[arrayLength];
            }
            if (!here->wattsArray.empty()) here->wattsArray[arrayLength] = max(0, point->watts);
            if (!here->atissArray.empty()) here->atissArray[arrayLength] = max(0, point->atiss);
            if (!here->antissArray.empty()) here->antissArray[arrayLength] = max(0, point->antiss);
            if (!here->npArray.empty()) here->npArray[arrayLength] = max(0, point->np);
            if (!here->rvArray.empty()) here->rvArray[arrayLength] = max(0, point->rvert);
            if (!here->rcadArray.empty()) here->rcadArray[arrayLength] = max(0, point->rcad);
            if (!here->rgctArray.empty()) here->rgctArray[arrayLength] = max(0, point->rcontact);
            if (!here->gearArray.empty()) here->gearArray[arrayLength] = max(0, point->gear);
            if (!here->smo2Array.empty()) here->smo2Array[arrayLength] = max(0, point->smo2);
            if (!here->thbArray.empty()) here->thbArray[arrayLength] = max(0, point->thb);
            if (!here->o2hbArray.empty()) here->o2hbArray[arrayLength] = max(0, point->o2hb);
            if (!here->hhbArray.empty()) here->hhbArray[arrayLength] = max(0, point->hhb);
            if (!here->xpArray.empty()) here->xpArray[arrayLength] = max(0, point->xp);
            if (!here->apArray.empty()) here->apArray[arrayLength] = max(0, point->apower);
            if (!here->hrArray.empty()) here->hrArray[arrayLength]    = max(0, point->hr);
            if (!here->tcoreArray.empty()) here->tcoreArray[arrayLength]    = max(0, point->tcore);

            // delta series
            if (!here->accelArray.empty()) here->accelArray[arrayLength] = point->kphd;
            if (!here->wattsDArray.empty()) here->wattsDArray[arrayLength] = point->wattsd;
            if (!here->cadDArray.empty()) here->cadDArray[arrayLength] = point->cadd;
            if (!here->nmDArray.empty()) here->nmDArray[arrayLength] = point->nmd;
            if (!here->hrDArray.empty()) here->hrDArray[arrayLength] = point->hrd;

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

            if (!here->slopeArray.empty()) here->slopeArray[arrayLength] = point->slope;

            if (!here->tempArray.empty())
                here->tempArray[arrayLength]   = context->athlete->useMetricUnits ? point->temp
                                                 : point->temp * FAHRENHEIT_PER_CENTIGRADE + FAHRENHEIT_ADD_CENTIGRADE;

            if (!here->windArray.empty())
                here->windArray[arrayLength] = max(0,
                                             (context->athlete->useMetricUnits
                                              ? point->headwind
                                              : point->headwind * MILES_PER_KM));

            // pedal data
            if (!here->balanceArray.empty()) here->balanceArray[arrayLength] = point->lrbalance;
            if (!here->lteArray.empty()) here->lteArray[arrayLength] = point->lte;
            if (!here->rteArray.empty()) here->rteArray[arrayLength] = point->rte;
            if (!here->lpsArray.empty()) here->lpsArray[arrayLength] = point->lps;
            if (!here->rpsArray.empty()) here->rpsArray[arrayLength] = point->rps;
            if (!here->lpcoArray.empty()) here->lpcoArray[arrayLength] = point->lpco;
            if (!here->rpcoArray.empty()) here->rpcoArray[arrayLength] = point->rpco;
            if (!here->lppbArray.empty()) here->lppbArray[arrayLength] = point->lppb;
            if (!here->rppbArray.empty()) here->rppbArray[arrayLength] = point->rppb;
            if (!here->lppeArray.empty()) here->lppeArray[arrayLength] = point->lppe;
            if (!here->rppeArray.empty()) here->rppeArray[arrayLength] = point->rppe;
            if (!here->lpppbArray.empty()) here->lpppbArray[arrayLength] = point->lpppb;
            if (!here->rpppbArray.empty()) here->rpppbArray[arrayLength] = point->rpppb;
            if (!here->lpppeArray.empty()) here->lpppeArray[arrayLength] = point->lpppe;
            if (!here->rpppeArray.empty()) here->rpppeArray[arrayLength] = point->rpppe;

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
        here->atissCurve->detach();
        here->antissCurve->detach();
        here->npCurve->detach();
        here->rvCurve->detach();
        here->rcadCurve->detach();
        here->rgctCurve->detach();
        here->gearCurve->detach();
        here->smo2Curve->detach();
        here->thbCurve->detach();
        here->o2hbCurve->detach();
        here->hhbCurve->detach();
        here->xpCurve->detach();
        here->apCurve->detach();
        here->hrCurve->detach();
        here->hrvCurve->detach();
        here->tcoreCurve->detach();
        here->speedCurve->detach();
        here->accelCurve->detach();
        here->wattsDCurve->detach();
        here->cadDCurve->detach();
        here->nmDCurve->detach();
        here->hrDCurve->detach();
        here->cadCurve->detach();
        here->altCurve->detach();
        here->altSlopeCurve->detach();
        here->slopeCurve->detach();
        here->tempCurve->detach();
        here->windCurve->detach();
        here->torqueCurve->detach();
        here->lteCurve->detach();
        here->rteCurve->detach();
        here->lpsCurve->detach();
        here->rpsCurve->detach();
        here->balanceLCurve->detach();
        here->balanceRCurve->detach();
        here->lpcoCurve->detach();
        here->rpcoCurve->detach();
        here->lppCurve->detach();
        here->rppCurve->detach();
        here->lpppCurve->detach();
        here->rpppCurve->detach();

        foreach(QwtPlotMarker *mrk, here->d_mrk)
            delete mrk;
        here->d_mrk.clear();

        foreach(QwtPlotMarker *mrk, here->cal_mrk)
            delete mrk;
        here->cal_mrk.clear();

        foreach(QwtPlotCurve *referenceLine, here->referenceLines) {
            curveColors->remove(referenceLine);
            referenceLine->detach();
            delete referenceLine;
        }
        here->referenceLines.clear();
    }

    // record the max x value
    if (here->timeArray.count() && here->distanceArray.count()) {
        double maxSECS = here->timeArray[here->timeArray.count()-1];
        double maxKM = here->distanceArray[here->distanceArray.count()-1];
        if (maxKM > here->maxKM) here->maxKM = maxKM;
        if (maxSECS > here->maxSECS) here->maxSECS = maxSECS;
    }

    setAltSlopePlotStyle(here->altSlopeCurve);

    // set the axis
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
}

void
AllPlot::setShow(RideFile::SeriesType type, bool state)
{
    switch(type) {

    case RideFile::none:
        setShowAccel(false);
        setShowPowerD(false);
        setShowCadD(false);
        setShowTorqueD(false);
        setShowHrD(false);
        setShowHRV(false);
        setShowPower(0);
        setShowAltSlope(0);
        setShowSlope(false);
        setShowNP(false);
        setShowATISS(false);
        setShowANTISS(false);
        setShowXP(false);
        setShowAP(false);
        setShowHr(false);
        setShowTcore(false);
        setShowSpeed(false);
        setShowCad(false);
        setShowAlt(false);
        setShowTemp(false);
        setShowWind(false);
        setShowRV(false);
        setShowRGCT(false);
        setShowRCad(false);
        setShowSmO2(false);
        setShowtHb(false);
        setShowO2Hb(false);
        setShowHHb(false);
        setShowGear(false);
        setShowW(false);
        setShowTorque(false);
        setShowBalance(false);
        setShowTE(false);
        setShowPS(false);
        setShowPCO(false);
        setShowDC(false);
        setShowPPP(false);
        break;

    case RideFile::secs: 
        break;
    case RideFile::cad: 
        setShowCad(state);
        break;
    case RideFile::tcore: 
        setShowTcore(state);
        break;
    case RideFile::hr: 
        setShowHr(state);
        break;
    case RideFile::hrv:
        setShowHRV(state);
        break;
    case RideFile::km:
        break;
    case RideFile::kph: 
        setShowSpeed(state);
        break;
    case RideFile::kphd: 
        setShowAccel(state);
        break;
    case RideFile::wattsd: 
        setShowPowerD(state);
        break;
    case RideFile::cadd: 
        setShowCadD(state);
        break;
    case RideFile::nmd: 
        setShowTorqueD(state);
        break;
    case RideFile::hrd: 
        setShowHrD(state);
        break;
    case RideFile::nm: 
        setShowTorque(state);
        break;
    case RideFile::watts: 
        setShowPower(state ? 0 : 2);
        break;
    case RideFile::xPower: 
        setShowXP(state);
        break;
    case RideFile::aPower: 
        setShowAP(state);
        break;
    case RideFile::aTISS: 
        setShowATISS(state);
        break;
    case RideFile::anTISS: 
        setShowANTISS(state);
        break;
    case RideFile::IsoPower: 
        setShowNP(state);
        break;
    case RideFile::alt: 
        setShowAlt(state);
        break;
    case RideFile::lon: 
        break;
    case RideFile::lat: 
        break;
    case RideFile::headwind: 
        setShowWind(state);
        break;
    case RideFile::slope: 
        setShowSlope(state);
        break;
    case RideFile::temp: 
        setShowTemp(state);
        break;
    case RideFile::lrbalance: 
        setShowBalance(state);
        break;
    case RideFile::lte: 
    case RideFile::rte: 
        setShowTE(state);
        break;
    case RideFile::lps: 
    case RideFile::rps: 
        setShowPS(state);
        break;
    case RideFile::lpco:
    case RideFile::rpco:
        setShowPCO(state);
        break;
    case RideFile::lppb:
    case RideFile::rppb:
    case RideFile::lppe:
    case RideFile::rppe:
        setShowDC(state);
        break;
    case RideFile::lpppb:
    case RideFile::rpppb:
    case RideFile::lpppe:
    case RideFile::rpppe:
        setShowPPP(state);
        break;
    case RideFile::interval: 
        break;
    case RideFile::vam: 
        break;
    case RideFile::wattsKg: 
        break;
    case RideFile::wprime: 
        setShowW(state);
        break;
    case RideFile::smo2: 
        setShowSmO2(state);
        break;
    case RideFile::thb: 
        setShowtHb(state);
        break;
    case RideFile::o2hb: 
        setShowO2Hb(state);
        break;
    case RideFile::hhb: 
        setShowHHb(state);
        break;
    case RideFile::rvert: 
        setShowRV(state);
        break;
    case RideFile::rcad: 
        setShowRCad(state);
        break;
    case RideFile::rcontact: 
        setShowRGCT(state);
        break;
    case RideFile::gear: 
        setShowGear(state);
        break;
    default:
    case RideFile::wbal:
    case RideFile::index:
        break;
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

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
}


void
AllPlot::setShowNP(bool show)
{
    showNP = show;
    standard->npCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowRV(bool show)
{
    showRV = show;
    standard->rvCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowRCad(bool show)
{
    showRCad = show;
    standard->rcadCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowRGCT(bool show)
{
    showRGCT = show;
    standard->rgctCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowGear(bool show)
{
    showGear = show;
    standard->gearCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowSmO2(bool show)
{
    showSmO2 = show;
    standard->smo2Curve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowtHb(bool show)
{
    showtHb = show;
    standard->thbCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowO2Hb(bool show)
{
    showO2Hb = show;
    standard->o2hbCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowHHb(bool show)
{
    showHHb = show;
    standard->hhbCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowANTISS(bool show)
{
    showANTISS = show;
    standard->antissCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowATISS(bool show)
{
    showATISS = show;
    standard->atissCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowXP(bool show)
{
    showXP = show;
    standard->xpCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowAP(bool show)
{
    showAP = show;
    standard->apCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowTcore(bool show)
{
    showTcore = show;
    standard->tcoreCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowHr(bool show)
{
    showHr = show;
    standard->hrCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowHRV(bool show)
{
    showHRV = show;
    standard->hrvCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowSpeed(bool show)
{
    showSpeed = show;
    standard->speedCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowAccel(bool show)
{
    showAccel = show;
    standard->accelCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowPowerD(bool show)
{
    showPowerD = show;
    standard->wattsDCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowCadD(bool show)
{
    showCadD = show;
    standard->cadDCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowTorqueD(bool show)
{
    showTorqueD = show;
    standard->nmDCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowHrD(bool show)
{
    showHrD = show;
    standard->hrDCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowCad(bool show)
{
    showCad = show;
    standard->cadCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowAlt(bool show)
{
    showAlt = show;
    standard->altCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowSlope(bool show)
{
    showSlope = show;
    standard->slopeCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowAltSlope(int id)
{
    if (showAltSlopeState == id) return;

    showAltSlopeState = id;
    standard->altSlopeCurve->setVisible(id > 0);
    setAltSlopePlotStyle(standard->altSlopeCurve);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowTemp(bool show)
{
    showTemp = show;
    standard->tempCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowWind(bool show)
{
    showWind = show;
    standard->windCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
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

    // clear labels ?
    if (show == false) {
        foreach(QwtPlotMarker *p, standard->matchLabels) {
            p->detach();
            delete p;
        }
        standard->matchLabels.clear();
    }

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowTorque(bool show)
{
    showTorque = show;
    standard->torqueCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowBalance(bool show)
{
    showBalance = show;
    standard->balanceLCurve->setVisible(show);
    standard->balanceRCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowTE(bool show)
{
    showTE = show;
    standard->lteCurve->setVisible(show); 
    standard->rteCurve->setVisible(show); 
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowPS(bool show)
{
    showPS = show;
    standard->lpsCurve->setVisible(show);
    standard->rpsCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowDC(bool show)
{
    showDC = show;
    standard->lppCurve->setVisible(show);
    standard->rppCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowPPP(bool show)
{
    showPPP = show;
    standard->lpppCurve->setVisible(show);
    standard->rpppCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowPCO(bool show)
{
    showPCO = show;
    standard->lpcoCurve->setVisible(show);
    standard->rpcoCurve->setVisible(show);
    setYMax();

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setShowGrid(bool show)
{
    standard->grid->setVisible(show);

    // remember the curves and colors
    isolation = false;
    curveColors->saveState();
    replot();
}

void
AllPlot::setPaintBrush(int state)
{
    fill = state;
    if (state) {

        for(int k=0; k<standard->U.count(); k++) {
            QColor p = standard->U[k].color;
            p.setAlpha(64);
            standard->U[k].curve->setBrush(QBrush(p));
        }

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
        standard->atissCurve->setBrush(QBrush(p));
        standard->antissCurve->setBrush(QBrush(p));


        p = standard->rvCurve->pen().color();
        p.setAlpha(64);
        standard->rvCurve->setBrush(QBrush(p));
        p = standard->rgctCurve->pen().color();
        p.setAlpha(64);
        standard->rgctCurve->setBrush(QBrush(p));
        p = standard->rcadCurve->pen().color();
        p.setAlpha(64);
        standard->rcadCurve->setBrush(QBrush(p));
        p = standard->gearCurve->pen().color();
        p.setAlpha(64);
        standard->gearCurve->setBrush(QBrush(p));
        p = standard->smo2Curve->pen().color();
        p.setAlpha(64);
        standard->smo2Curve->setBrush(QBrush(p));
        p = standard->thbCurve->pen().color();
        p.setAlpha(64);
        standard->thbCurve->setBrush(QBrush(p));
        p = standard->o2hbCurve->pen().color();
        p.setAlpha(64);
        standard->o2hbCurve->setBrush(QBrush(p));
        p = standard->hhbCurve->pen().color();
        p.setAlpha(64);
        standard->hhbCurve->setBrush(QBrush(p));


        p = standard->xpCurve->pen().color();
        p.setAlpha(64);
        standard->xpCurve->setBrush(QBrush(p));

        p = standard->apCurve->pen().color();
        p.setAlpha(64);
        standard->apCurve->setBrush(QBrush(p));

        p = standard->tcoreCurve->pen().color();
        p.setAlpha(64);
        standard->tcoreCurve->setBrush(QBrush(p));

        p = standard->hrCurve->pen().color();
        p.setAlpha(64);
        standard->hrCurve->setBrush(QBrush(p));

        p = standard->hrvCurve->pen().color();
        p.setAlpha(64);
        standard->hrvCurve->setBrush(QBrush(p));

        p = standard->accelCurve->pen().color();
        p.setAlpha(64);
        standard->accelCurve->setBrush(QBrush(p));

        p = standard->wattsDCurve->pen().color();
        p.setAlpha(64);
        standard->wattsDCurve->setBrush(QBrush(p));

        p = standard->cadDCurve->pen().color();
        p.setAlpha(64);
        standard->cadDCurve->setBrush(QBrush(p));

        p = standard->nmDCurve->pen().color();
        p.setAlpha(64);
        standard->nmDCurve->setBrush(QBrush(p));

        p = standard->hrDCurve->pen().color();
        p.setAlpha(64);
        standard->hrDCurve->setBrush(QBrush(p));

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

        p = standard->lteCurve->pen().color();
        p.setAlpha(64);
        standard->lteCurve->setBrush(QBrush(p));
        p = standard->rteCurve->pen().color();
        p.setAlpha(64);
        standard->rteCurve->setBrush(QBrush(p));
        p = standard->lpsCurve->pen().color();
        p.setAlpha(64);
        standard->lpsCurve->setBrush(QBrush(p));
        p = standard->rpsCurve->pen().color();
        p.setAlpha(64);
        standard->rpsCurve->setBrush(QBrush(p));
        p = standard->lpcoCurve->pen().color();
        p.setAlpha(64);
        standard->lpcoCurve->setBrush(QBrush(p));
        p = standard->rpcoCurve->pen().color();
        p.setAlpha(64);
        standard->rpcoCurve->setBrush(QBrush(p));
        p = standard->lppCurve->pen().color();
        p.setAlpha(64);
        standard->lppCurve->setBrush(QBrush(p));
        p = standard->rppCurve->pen().color();
        p.setAlpha(64);
        standard->rppCurve->setBrush(QBrush(p));
        p = standard->lpppCurve->pen().color();
        p.setAlpha(64);
        standard->lpppCurve->setBrush(QBrush(p));

        p = standard->rpppCurve->pen().color();
        p.setAlpha(64);
        standard->rpppCurve->setBrush(QBrush(p));

        p = standard->slopeCurve->pen().color();
        p.setAlpha(64);
        standard->slopeCurve->setBrush(QBrush(p));

        /*p = standard->altSlopeCurve->pen().color();
        p.setAlpha(64);
        standard->altSlopeCurve->setBrush(QBrush(p));

        p = standard->balanceLCurve->pen().color();
        p.setAlpha(64);
        standard->balanceLCurve->setBrush(QBrush(p));

        p = standard->balanceRCurve->pen().color();
        p.setAlpha(64);
        standard->balanceRCurve->setBrush(QBrush(p));*/
    } else {
        for(int k=0; k<standard->U.count(); k++) {
            standard->U[k].curve->setBrush(Qt::NoBrush);
        }

        standard->wCurve->setBrush(Qt::NoBrush);
        standard->wattsCurve->setBrush(Qt::NoBrush);
        standard->npCurve->setBrush(Qt::NoBrush);
        standard->rvCurve->setBrush(Qt::NoBrush);
        standard->rgctCurve->setBrush(Qt::NoBrush);
        standard->rcadCurve->setBrush(Qt::NoBrush);
        standard->gearCurve->setBrush(Qt::NoBrush);
        standard->smo2Curve->setBrush(Qt::NoBrush);
        standard->thbCurve->setBrush(Qt::NoBrush);
        standard->o2hbCurve->setBrush(Qt::NoBrush);
        standard->hhbCurve->setBrush(Qt::NoBrush);
        standard->atissCurve->setBrush(Qt::NoBrush);
        standard->antissCurve->setBrush(Qt::NoBrush);
        standard->xpCurve->setBrush(Qt::NoBrush);
        standard->apCurve->setBrush(Qt::NoBrush);
        standard->hrCurve->setBrush(Qt::NoBrush);
        standard->hrvCurve->setBrush(Qt::NoBrush);
        standard->tcoreCurve->setBrush(Qt::NoBrush);
        standard->speedCurve->setBrush(Qt::NoBrush);
        standard->accelCurve->setBrush(Qt::NoBrush);
        standard->wattsDCurve->setBrush(Qt::NoBrush);
        standard->cadDCurve->setBrush(Qt::NoBrush);
        standard->hrDCurve->setBrush(Qt::NoBrush);
        standard->nmDCurve->setBrush(Qt::NoBrush);
        standard->cadCurve->setBrush(Qt::NoBrush);
        standard->tempCurve->setBrush(Qt::NoBrush);
        standard->torqueCurve->setBrush(Qt::NoBrush);
        standard->lteCurve->setBrush(Qt::NoBrush);
        standard->rteCurve->setBrush(Qt::NoBrush);
        standard->lpsCurve->setBrush(Qt::NoBrush);
        standard->rpsCurve->setBrush(Qt::NoBrush);
        standard->lpcoCurve->setBrush(Qt::NoBrush);
        standard->rpcoCurve->setBrush(Qt::NoBrush);
        standard->lppCurve->setBrush(Qt::NoBrush);
        standard->rppCurve->setBrush(Qt::NoBrush);
        standard->lpppCurve->setBrush(Qt::NoBrush);
        standard->rpppCurve->setBrush(Qt::NoBrush);
        standard->slopeCurve->setBrush(Qt::NoBrush);
        //standard->altSlopeCurve->setBrush(Qt::NoBrush);
        //standard->balanceLCurve->setBrush(Qt::NoBrush);
        //standard->balanceRCurve->setBrush(Qt::NoBrush);
    }
    replot();
}

void
AllPlot::setSmoothing(int value)
{
    smooth = value;

    // if anything is going on, lets stop it now!
    // ACTUALLY its quite handy to play with smooting!
    isolation = false;
    curveColors->restoreState();

    recalc(standard);
}

void
AllPlot::setByDistance(int id)
{
    bydist = (id == 1);
    bytimeofday = (id == 2);

    setXTitle();

    // if anything is going on, lets stop it now!
    isolation = false;
    curveColors->restoreState();

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
    RideItem *rideItem = window->current;
    if (!rideItem) return NULL;

    int highlighted=0;
    foreach(IntervalItem *p, rideItem->intervals()) {
        if (p->selected) highlighted++;
        if (highlighted == n) return p;
    }
    return NULL;
}

// how many intervals selected?
int IntervalPlotData::intervalCount() const
{
    RideItem *rideItem = window->current;
    if (!rideItem) return 0;

    int highlighted=0;
    foreach(IntervalItem *p, rideItem->intervals())
        if (p->selected) highlighted++;
            
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

    // overlap at right ?
    double right = allPlot->bydist ? multiplier * current->stopKM : current->stop/60;


    if (i%4 == 2 || i%4 == 3) {
        for (int n=1; n<=intervalCount(); n++) {
            IntervalItem *other = intervalNum(n);
            if (other != current) {
                if (other->start < current->stop && other->stop > current->stop) {
                    if (other->start < current->start) {
                        double _right = allPlot->bydist ? multiplier * current->startKM : current->start/60;
                        if (_right<right)
                            right = _right;
                    } else  {
                        double _right = allPlot->bydist ? multiplier * other->startKM : other->start/60;
                        if (_right<right)
                            right = _right;
                    }
                }
            }
        }
    }


    // which point are we returning?
    switch (i%4) {
        case 0 : return allPlot->bydist ? multiplier * current->startKM : current->start/60; // bottom left
        case 1 : return allPlot->bydist ? multiplier * current->startKM : current->start/60; // top left
        case 2 : return right; // top right
        case 3 : return right; // bottom right
    }
    return 0; // shouldn't get here, but keeps compiler happy
}


double IntervalPlotData::y(size_t i) const
{
    // which point are we returning?
    switch (i%4) {
        case 0 : return -20; // bottom left
        case 1 : return 100; // top left - set to out of bound value
        case 2 : return 100; // top right - set to out of bound value
        case 3 : return -20;  // bottom right
        }
    return 0;
}


size_t IntervalPlotData::size() const { return intervalCount()*4; }

QPointF IntervalPlotData::sample(size_t i) const {
    return QPointF(x(i), y(i));
}

QRectF IntervalPlotData::boundingRect() const
{
    return QRectF(0, 5000, 5100, 5100);
}

void
AllPlot::pointHover(QwtPlotCurve *curve, int index)
{
    double X=0.0f;

    if (index >= 0 && curve != standard->intervalHighlighterCurve && 
                      curve != standard->intervalHoverCurve && curve->isVisible()) {

        int precision = 1; //< default to tens precision

        const double xvalue = curve->sample(index).x();
        double yvalue = curve->sample(index).y();

        X = xvalue;

        QString xstring;
        if (bydist) {
            xstring = QString("%1").arg(xvalue);
        } else {
            QTime t = QTime(0,0,0).addSecs((timeoffset+xvalue)*60.00);
            xstring = t.toString("hh:mm:ss");
        }

        // for speed curve add pace with units according to settings
        // only when the activity is a run.
        QString paceStr;
        
        if (curve->title() == tr("Speed") && rideItem) {
            precision = 2;
            if (rideItem->isRun) {
                bool metricPace = appsettings->value(this, GC_PACE, true).toBool();
                QString paceunit = metricPace ? tr("min/km") : tr("min/mile");
                paceStr = tr("\n%1 %2").arg(context->athlete->useMetricUnits ? kphToPace(yvalue, metricPace, false) : mphToPace(yvalue, metricPace, false)).arg(paceunit);
            } else if (rideItem->isSwim) {
                bool metricPace = appsettings->value(this, GC_SWIMPACE, true).toBool();
                QString paceunit = metricPace ? tr("min/100m") : tr("min/100yd");
                paceStr = tr("\n%1 %2").arg(context->athlete->useMetricUnits ? kphToPace(yvalue, metricPace, true) : mphToPace(yvalue, metricPace, true)).arg(paceunit);
            }
        } else if (curve->title() == tr("W'")) {
            // need to scale for W' bal
            yvalue /= 1000.0f;
        } else if (curve->title() == tr("Hb")) {
            precision = 2;
        } else if (curve->title() == tr("R-R")) {
            precision = 3;
        } else if (curve->title() == tr("Gear Ratio")) {
            precision = 2;
        }
        
        // output the tooltip
        QString text = QString("%1\n%2 %3%4\n%5 %6")
                        .arg(this->axisTitle(curve->yAxis()).text())
                        .arg(yvalue, 0, 'f', precision)
                        .arg("") // TODO: determine units to describe mph, bpm, rpm, etc.
                        .arg(paceStr)
                        .arg(xstring)
                        .arg(this->axisTitle(curve->xAxis()).text())
                        ;

        // set that text up
        tooltip->setText(text);

        // isolate me -- maybe do this via the legend ?
        //curveColors->isolate(curve);
        //replot();

    } else {

        // no point
        tooltip->setText("");

        // ok now we highlight intervals
        QPoint cursor = QCursor::pos();
        X = tooltip->invTransform(canvas()->mapFromGlobal(cursor)).x();

        // get colors back -- maybe do this via the legend?
        //curveColors->restoreState();
        //replot();
    }

    // we don't want hoveing or we have intervals selected so no need to mouse over
    if (!window->showHover->isChecked() || (rideItem && rideItem->intervalsSelected().count())) return;

    if (!context->isCompareIntervals && rideItem && rideItem->ride()) {

        // convert from distance to time
        if (bydist) X = rideItem->ride()->distanceToTime(X) / 60.00f;

        QVector<double>xdata, ydata;
        IntervalItem *chosen = NULL;

        if (rideItem->ride()->dataPoints().count() > 1) {

            // set duration to length of ride, and keep the value to compare
            int rideduration = rideItem->ride()->dataPoints().last()->secs -
                                  rideItem->ride()->dataPoints().first()->secs;

            int duration = rideduration;

            // loop through intervals and select FIRST we are in
            foreach(IntervalItem *i, rideItem->intervals()) {

                // ignore peaks and all, they are really distracting
                if (i->type == RideFileInterval::ALL || i->type == RideFileInterval::PEAKPOWER)
                    continue;

                if (i->start < (X*60.00f) && i->stop > (X*60.00f)) {
                    if ((i->stop-i->start) < duration) {
                        duration = i->stop - i->start;
                        chosen = i;
                    }
                }
            }

            // we already chose it!
            if (chosen == NULL || chosen == hovered) return;

            if (duration < rideduration) {

                // hover curve color aligns to the type of interval we are highlighting
                QColor hbrush = chosen->color;
                hbrush.setAlpha(64);
                standard->intervalHoverCurve->setBrush(hbrush);   // fill below the line

                // we chose one?
                if (bydist) {

                    double multiplier = context->athlete->useMetricUnits ? 1 : MILES_PER_KM;
                    double start = multiplier * chosen->startKM;
                    double stop = multiplier * chosen->stopKM;

                    xdata << start;
                    ydata << -20;
                    xdata << start;
                    ydata << 100;
                    xdata << stop;
                    ydata << 100;
                    xdata << stop;
                    ydata << -20;

                } else {

                    xdata << chosen->start / 60.00f;
                    ydata << -20;
                    xdata << chosen->start / 60.00f;
                    ydata << 100;
                    xdata << chosen->stop / 60.00f;
                    ydata << 100;
                    xdata << chosen->stop / 60.00f;
                    ydata << -20;
                }
            }
        }

        standard->intervalHoverCurve->setSamples(xdata,ydata);
        replot();

        // remember for next time!
        hovered = chosen;

        // tell the charts -- and block signals whilst they occur
        blockSignals(true);
        context->notifyIntervalHover(hovered);
        blockSignals(false);

    }
}

void
AllPlot::intervalHover(IntervalItem *chosen)
{
    // no point!
    if (!isVisible() || chosen == hovered) return;

    // don't highlight the all or all the peak intervals
    if (chosen && chosen->type == RideFileInterval::ALL) return;

    QVector<double>xdata, ydata;
    if (chosen) {

        // hover curve color aligns to the type of interval we are highlighting
        QColor hbrush = chosen->color;
        hbrush.setAlpha(64);
        standard->intervalHoverCurve->setBrush(hbrush);   // fill below the line

        if (bydist) {
            double multiplier = context->athlete->useMetricUnits ? 1 : MILES_PER_KM;
            double start = multiplier * chosen->startKM;
            double stop = multiplier * chosen->stopKM;

            xdata << start;
            ydata << -20;
            xdata << start;
            ydata << 100;
            xdata << stop;
            ydata << 100;
            xdata << stop;
            ydata << -20;
        } else {
            xdata << chosen->start / 60.00f;
            ydata << -20;
            xdata << chosen->start / 60.00f;
            ydata << 100;
            xdata << chosen->stop / 60.00f;
            ydata << 100;
            xdata << chosen->stop / 60.00f;
            ydata << -20;
        }
    }

    // update state
    hovered = chosen;

    standard->intervalHoverCurve->setSamples(xdata,ydata); 
    replot();
}

bool
AllPlot::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        emit resized();
    }

    // REFERENCE LINE FOR POWER
    if ((showPowerState<2 && scope == RideFile::none) || scope == RideFile::watts || scope == RideFile::aTISS || 
        scope == RideFile::anTISS || scope == RideFile::IsoPower || scope == RideFile::aPower || scope == RideFile::xPower) {

        // which axis ?
        int axis = -1;
        if (obj == axisWidget(QwtPlot::yLeft)) axis=QwtPlot::yLeft;
        else if (obj == axisWidget(QwtPlot::xBottom)) axis=QwtPlot::xBottom;

        if (axis == QwtPlot::yLeft && event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *m = static_cast<QMouseEvent*>(event);
            confirmTmpReference(invTransform(axis, m->y()),axis, RideFile::watts, true); // do show delete stuff
            return false;
        }
        if (axis == QwtPlot::yLeft && event->type() == QEvent::MouseMove) {
            QMouseEvent *m = static_cast<QMouseEvent*>(event);
            plotTmpReference(axis, m->x()-axisWidget(axis)->width(), m->y(), RideFile::watts);
            return false;
        }
        if (axis == QwtPlot::yLeft && event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *m = static_cast<QMouseEvent*>(event);
            if (m->x()>axisWidget(axis)->width()) {
                confirmTmpReference(invTransform(axis, m->y()),axis, RideFile::watts, false); // don't show delete stuff
                return false;
            } else  if (standard->tmpReferenceLines.count()) {
                plotTmpReference(axis, 0, 0, RideFile::watts); //unplot
                return true;
            }
        }
    }

    // REFERENCE LINE FOR POWER
    if ((showHr && scope == RideFile::none) || scope == RideFile::hr) {

        QwtAxisId axis(-1,-1);

        // is it an HR Axis?
        if (scope == RideFile::none && obj == axisWidget(QwtAxisId(QwtAxis::yLeft, 1))) axis=QwtAxisId(QwtAxis::yLeft,1);
        else if (scope == RideFile::hr && obj == axisWidget(QwtAxisId(QwtAxis::yLeft))) axis=QwtAxisId(QwtAxis::yLeft);

        if (axis != QwtAxisId(-1,-1) && event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *m = static_cast<QMouseEvent*>(event);
            confirmTmpReference(invTransform(axis, m->y()),axis.id, RideFile::hr, true); // do show delete stuff
            return false;
        }
        if (axis != QwtAxisId(-1,-1) && event->type() == QEvent::MouseMove) {
            QMouseEvent *m = static_cast<QMouseEvent*>(event);
            plotTmpReference(axis, m->x()-axisWidget(axis)->width(), m->y(), RideFile::hr);
            return false;
        }
        if (axis != QwtAxisId(-1,-1) && event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *m = static_cast<QMouseEvent*>(event);
            if (m->x()>axisWidget(axis)->width()) {
                confirmTmpReference(invTransform(axis, m->y()),axis.id, RideFile::hr, false); // don't show delete stuff
                return false;
            } else  if (standard->tmpReferenceLines.count()) {
                plotTmpReference(axis.id, 0, 0, RideFile::hr); //unplot
                return true;
            }
        }
    }

    // TO EXHAUSTION REFERENCE (INTERVAL TYPE "EXHAUSTION")
    if (obj == axisWidget(QwtPlot::xBottom)) {

        // if the user clicks whilst on the axis then moves the cursor
        // we will get the mouse move events as that happens, which
        // means we don't need to trap mouse click events -- since we
        // don't get mouse move events until the mouse is pressed (!)
        QMouseEvent *m = static_cast<QMouseEvent*>(event);

        if (event->type() == QEvent::MouseButtonDblClick) {

            confirmTmpExhaustion(m->x(), true); // do show delete stuff
            return false;

        } else if (event->type() == QEvent::MouseMove) {

            // plot a temporary marker
            plotTmpExhaustion(m->x());
            return true;

        } else if (event->type() == QEvent::MouseButtonRelease) {

            // only respond if we are on the plot, which is above so
            // therefore has a y-value that is negative
            if (m->y() < 0) {

                // confirm and add to references + intervals
                confirmTmpExhaustion(m->x());
                return true;

            } else {

                // unplot when released on axis (must release on the chart area)
                plotTmpExhaustion(0);
                return true;
            }
        }

        return false;
    }

    // is it for other objects ?
    QList<QObject*> axes;
    QList<QwtAxisId> axesId;

    axes << axisWidget(QwtPlot::yLeft);
    axesId << QwtPlot::yLeft;

    axes << axisWidget(QwtAxisId(QwtAxis::yLeft, 1));
    axesId << QwtAxisId(QwtAxis::yLeft, 1);

    axes << axisWidget(QwtAxisId(QwtAxis::yLeft, 3));
    axesId << QwtAxisId(QwtAxis::yLeft, 3);

    axes << axisWidget(QwtPlot::yRight);
    axesId << QwtPlot::yRight;

    axes << axisWidget(QwtAxisId(QwtAxis::yRight, 1));
    axesId << QwtAxisId(QwtAxis::yRight, 1);

    axes << axisWidget(QwtAxisId(QwtAxis::yRight, 2));
    axesId << QwtAxisId(QwtAxis::yRight, 2);

    axes << axisWidget(QwtAxisId(QwtAxis::yRight, 3));
    axesId << QwtAxisId(QwtAxis::yRight, 3);

    // user axis
    if (standard) {
        for(int k=0; k<standard->U.count(); k++) {
            axes << axisWidget(QwtAxisId(QwtAxis::yRight, 4 + k));
            axesId << QwtAxisId(QwtAxis::yRight, 4 + k);
        }
    }

    if (axes.contains(obj)) {

        QwtAxisId id = axesId.at(axes.indexOf(obj));

        // this is an axes widget
        //qDebug()<<this<<"event on="<<id<< static_cast<QwtScaleWidget*>(obj)->title().text() <<"event="<<event->type();

        // isolate / restore on mouse enter leave
        if (!isolation && event->type() == QEvent::Enter) {

            // isolate curve on hover
            curveColors->isolateAxis(id);
            replot();

        } else if (!isolation && event->type() == QEvent::Leave) {

            // return to normal when leave
            curveColors->restoreState();
            replot();

        } else if (event->type() == QEvent::MouseButtonRelease) {

            // click on any axis to toggle isolation
            // if isolation is on, just turns it off
            // if isolation is off, turns it on for the axis clicked
            if (isolation) {
                isolation = false;
                curveColors->restoreState();
                replot();
            } else {
                isolation = true;
                curveColors->isolateAxis(id, true); // with scale adjust
                replot();
            }
        }
    }

    // turn off hover when mouse leaves
    if (event->type() == QEvent::Leave) context->notifyIntervalHover(NULL);

    return false;
}

void
AllPlot::plotTmpExhaustion(double mx)
{
    // we need to have a ride to work with
    if (!rideItem || !rideItem->ride()) return;

    double secs = -1;
    if (mx > 0) {
        double px = invTransform(QwtAxisId(QwtPlot::xBottom), mx);
        if (bydist == true) secs = rideItem->ride()->distanceToTime(px);
        else secs = px * 60.00f;
    }

    //
    // clear whatever is there
    //
    foreach(QwtPlotMarker *marker, standard->tmpExhaustionLines) {
        if (marker) {
            //curveColors->remove(curve); // ignored if not already there
            marker->detach();
            delete marker;
        }
    }
    standard->tmpExhaustionLines.clear();

    foreach(AllPlot *plot, window->seriesPlots) {
        plot->replot();
        foreach(QwtPlotMarker *marker, plot->standard->tmpExhaustionLines) {
            if (marker) {
                marker->detach();
                delete marker;
            }
        }
        plot->standard->tmpExhaustionLines.clear();
    }
    foreach(AllPlot *plot, window->allPlots) {
        plot->replot();
        foreach(QwtPlotMarker *marker, plot->standard->tmpExhaustionLines) {
            if (marker) {
                marker->detach();
                delete marker;
            }
        }
        plot->standard->tmpExhaustionLines.clear();
    }

    //
    // add a new one if its valid
    //
    if (secs > 0) {

        // only plot if they are relevant to the plot.
        QwtPlotMarker *exhaustionLine = window->allPlot->plotExhaustionLine(secs);
        if (exhaustionLine) {
            standard->tmpExhaustionLines.append(exhaustionLine);
            window->allPlot->replot();
        }

        foreach(AllPlot *plot, window->seriesPlots) {
            QwtPlotMarker *exhaustionLine = plot->plotExhaustionLine(secs);
            if (exhaustionLine) {
                plot->standard->tmpExhaustionLines.append(exhaustionLine);
                plot->replot();
            }
        }

        foreach(AllPlot *plot, window->allPlots) {
            QwtPlotMarker *marker = plot->plotExhaustionLine(secs);
            if (marker) {
                plot->standard->tmpExhaustionLines.append(marker);
                plot->replot();
            }
        }
    }
}

void
AllPlot::confirmTmpExhaustion(double mx, bool allowDelete)
{
    // not supported in compare mode
    if (window == NULL || context->isCompareIntervals) return;

    // we need to have a ride to work with
    if (!rideItem || !rideItem->ride()) return;

    double px = invTransform(QwtAxisId(QwtPlot::xBottom), mx);
    double secs = -1;

    if (bydist == true) secs = rideItem->ride()->distanceToTime(px);
    else secs = px * 60.00f;
    ExhaustionDialog *p = new ExhaustionDialog(this, context, secs, allowDelete);
    p->setWindowModality(Qt::ApplicationModal); // don't allow select other ride or it all goes wrong!
    p->setValue(secs);
    p->move(QCursor::pos()-QPoint(40,40));
    p->exec();
}

QwtPlotMarker*
AllPlot::plotExhaustionLine(double secs)
{
    // not supported in compare mode
    if (secs <= 0 || context->isCompareIntervals) return NULL;

    QwtPlotMarker *marker = new QwtPlotMarker(""); //name is TODO
    marker->setLinePen(QPen(GColor(CWBAL),5.0f));
    marker->setLineStyle(QwtPlotMarker::VLine);
    marker->setXValue(bydist ? rideItem->ride()->timeToDistance(secs) : secs/60.0f);
    marker->setZ(-15); // to the back
    marker->attach(this);
    return marker;
}

void
AllPlot::plotTmpReference(QwtAxisId axis, int x, int y, RideFile::SeriesType serie)
{
    // only if on allplotwindow
    if (window==NULL) return;

    // not supported in compare mode
    if (context->isCompareIntervals) return;

    // power on power plots
    if (serie == RideFile::watts && scope != RideFile::none && scope != RideFile::watts && scope != RideFile::aTISS && 
        scope != RideFile::anTISS && scope != RideFile::IsoPower && scope != RideFile::aPower && scope != RideFile::xPower) return;

    // hr on hr plots
    if (serie == RideFile::hr && scope != RideFile::none && scope != RideFile::hr) return;

    if (x>0) {

        RideFilePoint *referencePoint = new RideFilePoint();

        if (serie == RideFile::watts)
            referencePoint->watts = invTransform(axis, y);
        if (serie == RideFile::hr)
            referencePoint->hr = invTransform(axis, y);

        foreach(QwtPlotCurve *curve, standard->tmpReferenceLines) {
            if (curve) {
                curveColors->remove(curve); // ignored if not already there
                curve->detach();
                delete curve;
            }
        }
        standard->tmpReferenceLines.clear();

        // only plot if they are relevant to the plot.
        QwtPlotCurve *referenceLine = window->allPlot->plotReferenceLine(referencePoint);
        if (referenceLine) {
            standard->tmpReferenceLines.append(referenceLine);
            window->allPlot->replot();
        }

        // now do the series plots
        foreach(AllPlot *plot, window->seriesPlots) {
            plot->replot();
            foreach(QwtPlotCurve *curve, plot->standard->tmpReferenceLines) {
                if (curve) {
                    plot->curveColors->remove(curve); // ignored if not already there
                    curve->detach();
                    delete curve;
                }
            }
            plot->standard->tmpReferenceLines.clear();
        }
        foreach(AllPlot *plot, window->seriesPlots) {
            QwtPlotCurve *referenceLine = plot->plotReferenceLine(referencePoint);
            if (referenceLine) {
                plot->standard->tmpReferenceLines.append(referenceLine);
                plot->replot();
            }
        }

        // now the stack plots
        foreach(AllPlot *plot, window->allPlots) {
            plot->replot();
            foreach(QwtPlotCurve *curve, plot->standard->tmpReferenceLines) {
                if (curve) {
                    plot->curveColors->remove(curve); // ignored if not already there
                    curve->detach();
                    delete curve;
                }
            }
            plot->standard->tmpReferenceLines.clear();
        }
        foreach(AllPlot *plot, window->allPlots) {
            QwtPlotCurve *referenceLine = plot->plotReferenceLine(referencePoint);
            if (referenceLine) {
                plot->standard->tmpReferenceLines.append(referenceLine);
                plot->replot();
            }
        }


    } else  {

        // wipe any we don't want
        foreach(QwtPlotCurve *curve, standard->tmpReferenceLines) {
            if (curve) {
                curveColors->remove(curve); // ignored if not already there
                curve->detach();
                delete curve;
            }
        }
        standard->tmpReferenceLines.clear();
        window->allPlot->replot();
        foreach(AllPlot *plot, window->seriesPlots) {
            plot->replot();
            foreach(QwtPlotCurve *curve, plot->standard->tmpReferenceLines) {
                if (curve) {
                    plot->curveColors->remove(curve); // ignored if not already there
                    curve->detach();
                    delete curve;
                }
                plot->standard->tmpReferenceLines.clear();
            }
        }
        window->allPlot->replot();
        foreach(AllPlot *plot, window->allPlots) {
            plot->replot();
            foreach(QwtPlotCurve *curve, plot->standard->tmpReferenceLines) {
                if (curve) {
                    plot->curveColors->remove(curve); // ignored if not already there
                    curve->detach();
                    delete curve;
                }
            }
            plot->standard->tmpReferenceLines.clear();
        }
    }
}

void
AllPlot::refreshReferenceLinesForAllPlots()
{
    // not supported in compare mode
    if (window == NULL || context->isCompareIntervals) return;

    window->allPlot->refreshReferenceLines();
    foreach(AllPlot *plot, window->allPlots) {
        plot->refreshReferenceLines();
    }
    foreach(AllPlot *plot, window->seriesPlots) {
        plot->refreshReferenceLines();
    }
}

void
AllPlot::refreshExhaustionsForAllPlots()
{
    // not supported in compare mode
    if (window == NULL || context->isCompareIntervals) return;

    window->allPlot->refreshExhaustions();
    foreach(AllPlot *plot, window->allPlots) {
        plot->refreshExhaustions();
    }
    foreach(AllPlot *plot, window->seriesPlots) {
        plot->refreshExhaustions();
    }
}

void
AllPlot::confirmTmpReference(double value, int axis, RideFile::SeriesType serie, bool allowDelete)
{
    // not supported in compare mode
    if (window == NULL || context->isCompareIntervals) return;

    ReferenceLineDialog *p = new ReferenceLineDialog(this, context, serie, allowDelete);
    p->setWindowModality(Qt::ApplicationModal); // don't allow select other ride or it all goes wrong!
    p->setValueForAxis(value, axis);
    p->move(QCursor::pos()-QPoint(40,40));
    p->exec();
}


void
AllPlot::setAltSlopePlotStyle (AllPlotSlopeCurve *curve){

    if (bydist) {
        switch (showAltSlopeState) {
        case 0: {curve->setStyle(AllPlotSlopeCurve::SlopeDist1); break;}
        case 1: {curve->setStyle(AllPlotSlopeCurve::SlopeDist1); break;}
        case 2: {curve->setStyle(AllPlotSlopeCurve::SlopeDist2); break;}
        case 3: {curve->setStyle(AllPlotSlopeCurve::SlopeDist3); break;}
        }

    } else {
        switch (showAltSlopeState) {
        case 0: {curve->setStyle(AllPlotSlopeCurve::SlopeTime1); break;}
        case 1: {curve->setStyle(AllPlotSlopeCurve::SlopeTime1); break;}
        case 2: {curve->setStyle(AllPlotSlopeCurve::SlopeTime2); break;}
        case 3: {curve->setStyle(AllPlotSlopeCurve::SlopeTime3); break;}
        }
    }
}
