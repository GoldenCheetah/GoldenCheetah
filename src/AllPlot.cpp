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

#include "AllPlot.h"
#include "MainWindow.h"
#include "AllPlotWindow.h"
#include "RideFile.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "Settings.h"
#include "Units.h"
#include "Zones.h"
#include "Colors.h"

#include <assert.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_div.h>
#include <qwt_scale_widget.h>
#include <qwt_valuelist.h>
#include <qwt_text.h>
#include <qwt_legend.h>
#include <qwt_data.h>
#include <QMultiMap>

class IntervalPlotData : public QwtData
{
    public:
    IntervalPlotData(AllPlot *allPlot, MainWindow *mainWindow) :
        allPlot(allPlot), mainWindow(mainWindow) {}
    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    virtual QwtData *copy() const ;
    void init() ;
    IntervalItem *intervalNum(int n) const;
    int intervalCount() const;
    AllPlot *allPlot;
    MainWindow *mainWindow;
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
                      const QRect &rect) const
    {
        RideItem *rideItem = parent->rideItem;

        if (! rideItem)
            return;

        const Zones *zones       = rideItem->zones;
        int zone_range     = rideItem->zoneRange();
        if (parent->shadeZones() && (zone_range >= 0)) {
            QList <int> zone_lows = zones->getZoneLows(zone_range);
            int num_zones = zone_lows.size();
            if (num_zones > 0) {
                for (int z = 0; z < num_zones; z ++) {
                    QRect r = rect;

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


            if (! rideItem)
                return;


            const Zones *zones       = rideItem->zones;
            int zone_range     = rideItem->zoneRange();

            // create new zone labels if we're shading
            if (parent->shadeZones() && (zone_range >= 0)) {
                QList <int> zone_lows = zones->getZoneLows(zone_range);
                QList <QString> zone_names = zones->getZoneNames(zone_range);
                int num_zones = zone_lows.size();
                assert(zone_names.size() == num_zones);
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
                  const QRect &rect) const
        {
            if (parent->shadeZones()) {
                int x = (rect.left() + rect.right()) / 2;
                int y = yMap.transform(watts);

                // the following code based on source for QwtPlotMarker::draw()
                QRect tr(QPoint(0, 0), text.textSize(painter->font()));
                tr.moveCenter(QPoint(x, y));
                text.draw(painter, tr);
            }
        }
};


static inline double
max(double a, double b) { if (a > b) return a; else return b; }

AllPlot::AllPlot(AllPlotWindow *parent, MainWindow *mainWindow):
    QwtPlot(parent),
    rideItem(NULL),
    unit(0),
    shade_zones(true),
    showPowerState(3),
    showHrState(Qt::Checked),
    showSpeedState(Qt::Checked),
    showCadState(Qt::Checked),
    showAltState(Qt::Checked),
    showTempState(Qt::Checked),
    bydist(false),
    parent(parent)
{
    setInstanceName("AllPlot");
    unit = appsettings->value(this, GC_UNIT);

    referencePlot = NULL;

    useMetricUnits = (unit.toString() == "Metric");
    if (appsettings->value(this, GC_SHADEZONES, true).toBool()==false)
        shade_zones = false;

    if (smooth < 1) smooth = 1;

    // create a background object for shading
    bg = new AllPlotBackground(this);
    bg->attach(this);

    //insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
    canvas()->setFrameStyle(QFrame::NoFrame);

    setXTitle();

    wattsCurve = new QwtPlotCurve(tr("Power"));
    wattsCurve->setYAxis(yLeft);

    hrCurve = new QwtPlotCurve(tr("Heart Rate"));
    hrCurve->setYAxis(yLeft2);

    speedCurve = new QwtPlotCurve(tr("Speed"));
    speedCurve->setYAxis(yRight);

    cadCurve = new QwtPlotCurve(tr("Cadence"));
    cadCurve->setYAxis(yLeft2);

    altCurve = new QwtPlotCurve(tr("Altitude"));
    // altCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    altCurve->setYAxis(yRight2);

    tempCurve = new QwtPlotCurve(tr("Temperature"));
    tempCurve->setYAxis(yRight);

    intervalHighlighterCurve = new QwtPlotCurve();
    intervalHighlighterCurve->setYAxis(yLeft);
    intervalHighlighterCurve->setData(IntervalPlotData(this, mainWindow));
    intervalHighlighterCurve->attach(this);
    //this->legend()->remove(intervalHighlighterCurve); // don't show in legend

    // setup that grid
    grid = new QwtPlotGrid();
    grid->enableX(true);
    grid->enableY(true);
    grid->attach(this);

    // get rid of nasty blank space on right of the plot
    plotLayout()->setAlignCanvasToScales(true);
    setAxisMaxMinor(xBottom, 0);
    setAxisMaxMinor(yLeft, 0);
    setAxisMaxMinor(yLeft2, 0);
    setAxisMaxMinor(yRight, 0);
    setAxisMaxMinor(yRight2, 0);

    configChanged(); // set colors
}

void
AllPlot::configChanged()
{

    double width = appsettings->value(this, GC_LINEWIDTH, 2.0).toDouble();

    if (appsettings->value(this, GC_ANTIALIAS, false).toBool() == true) {
        wattsCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        hrCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        speedCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        cadCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        altCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        tempCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        intervalHighlighterCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    }

    setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
    QPen wattsPen = QPen(GColor(CPOWER));
    wattsPen.setWidth(width);
    wattsCurve->setPen(wattsPen);
    QPen hrPen = QPen(GColor(CHEARTRATE));
    hrPen.setWidth(width);
    hrCurve->setPen(hrPen);
    QPen speedPen = QPen(GColor(CSPEED));
    speedPen.setWidth(width);
    speedCurve->setPen(speedPen);
    QPen cadPen = QPen(GColor(CCADENCE));
    cadPen.setWidth(width);
    cadCurve->setPen(cadPen);
    QPen altPen(GColor(CALTITUDE));
    altPen.setWidth(width);
    altCurve->setPen(altPen);
    QColor brush_color = GColor(CALTITUDEBRUSH);
    brush_color.setAlpha(200);
    altCurve->setBrush(brush_color);   // fill below the line
    QPen tempPen = QPen(GColor(CTEMP));
    tempPen.setStyle(Qt::DotLine);
    tempPen.setWidth(width);
    //tempCurve->setPen(tempPen);

    QwtSymbol sym;
    sym.setStyle(QwtSymbol::Ellipse);
    sym.setSize(width*10);
    sym.setPen(QPen(GColor(CTEMP)));
    sym.setBrush(QBrush(GColor(CTEMP)));
    tempCurve->setPen(tempPen); //Qt::NoPen
    tempCurve->setStyle(QwtPlotCurve::Dots);
    tempCurve->setSymbol(sym);

    QPen ihlPen = QPen(GColor(CINTERVALHIGHLIGHTER));
    ihlPen.setWidth(width);
    intervalHighlighterCurve->setPen(ihlPen);
    QColor ihlbrush = QColor(GColor(CINTERVALHIGHLIGHTER));
    ihlbrush.setAlpha(64);
    intervalHighlighterCurve->setBrush(ihlbrush);   // fill below the line
    //this->legend()->remove(intervalHighlighterCurve); // don't show in legend
    QPen gridPen(GColor(CPLOTGRID));
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);

    // curve brushes
    if (parent->isPaintBrush()) {
        QColor p;
        p = wattsCurve->pen().color();
        p.setAlpha(64);
        wattsCurve->setBrush(QBrush(p));

        p = hrCurve->pen().color();
        p.setAlpha(64);
        hrCurve->setBrush(QBrush(p));

        p = speedCurve->pen().color();
        p.setAlpha(64);
        speedCurve->setBrush(QBrush(p));

        p = cadCurve->pen().color();
        p.setAlpha(64);
        cadCurve->setBrush(QBrush(p));
    } else {
        wattsCurve->setBrush(Qt::NoBrush);
        hrCurve->setBrush(Qt::NoBrush);
        speedCurve->setBrush(Qt::NoBrush);
        cadCurve->setBrush(Qt::NoBrush);
    }

    QPalette pal;

    // tick draw
    QwtScaleDraw *sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtPlot::xBottom, sd);

    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtPlot::yLeft, sd);
    pal.setColor(QPalette::WindowText, GColor(CPOWER));
    pal.setColor(QPalette::Text, GColor(CPOWER));
    axisWidget(QwtPlot::yLeft)->setPalette(pal);

    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtPlot::yLeft2, sd);
    pal.setColor(QPalette::WindowText, GColor(CHEARTRATE));
    pal.setColor(QPalette::Text, GColor(CHEARTRATE));
    axisWidget(QwtPlot::yLeft2)->setPalette(pal);

    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtPlot::yRight, sd);
    pal.setColor(QPalette::WindowText, GColor(CSPEED));
    pal.setColor(QPalette::Text, GColor(CSPEED));
    axisWidget(QwtPlot::yRight)->setPalette(pal);

    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtPlot::yRight2, sd);
    pal.setColor(QPalette::WindowText, GColor(CALTITUDE));
    pal.setColor(QPalette::Text, GColor(CALTITUDE));
    axisWidget(QwtPlot::yRight2)->setPalette(pal);

}

struct DataPoint {
    double time, hr, watts, speed, cad, alt, temp;
    DataPoint(double t, double h, double w, double s, double c, double a, double te) :
        time(t), hr(h), watts(w), speed(s), cad(c), alt(a), temp(te) {}
};

bool AllPlot::shadeZones() const
{
    return shade_zones;
}

void
AllPlot::setAxisTitle(int axis, QString label)
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
AllPlot::recalc()
{
    if (referencePlot !=NULL){
        return;
    }

    if (timeArray.empty())
        return;

    int rideTimeSecs = (int) ceil(timeArray[arrayLength - 1]);
    if (rideTimeSecs > 7*24*60*60) {
        QwtArray<double> data;
        if (!wattsArray.empty())
            wattsCurve->setData(data, data);
        if (!hrArray.empty())
            hrCurve->setData(data, data);
        if (!speedArray.empty())
            speedCurve->setData(data, data);
        if (!cadArray.empty())
            cadCurve->setData(data, data);
        if (!altArray.empty())
            altCurve->setData(data, data);
        if (!tempArray.empty())
            tempCurve->setData(data, data);
        return;
    }
    // we should only smooth the curves if smoothed rate is greater than sample rate
    if (smooth > rideItem->ride()->recIntSecs()) {

        double totalWatts = 0.0;
        double totalHr = 0.0;
        double totalSpeed = 0.0;
        double totalCad = 0.0;
        double totalDist = 0.0;
        double totalAlt = 0.0;
        double totalTemp = 0.0;

        QList<DataPoint> list;

        smoothWatts.resize(rideTimeSecs + 1); //(rideTimeSecs + 1);
        smoothHr.resize(rideTimeSecs + 1);
        smoothSpeed.resize(rideTimeSecs + 1);
        smoothCad.resize(rideTimeSecs + 1);
        smoothTime.resize(rideTimeSecs + 1);
        smoothDistance.resize(rideTimeSecs + 1);
        smoothAltitude.resize(rideTimeSecs + 1);
        smoothTemp.resize(rideTimeSecs + 1);

        for (int secs = 0; ((secs < smooth)
                            && (secs < rideTimeSecs)); ++secs) {
            smoothWatts[secs] = 0.0;
            smoothHr[secs]    = 0.0;
            smoothSpeed[secs] = 0.0;
            smoothCad[secs]   = 0.0;
            smoothTime[secs]  = secs / 60.0;
            smoothDistance[secs]  = 0.0;
            smoothAltitude[secs]  = 0.0;
            smoothTemp[secs]  = 0.0;
        }

        int i = 0;
        for (int secs = smooth; secs <= rideTimeSecs; ++secs) {
            while ((i < arrayLength) && (timeArray[i] <= secs)) {
                DataPoint dp(timeArray[i],
                             (!hrArray.empty() ? hrArray[i] : 0),
                             (!wattsArray.empty() ? wattsArray[i] : 0),
                             (!speedArray.empty() ? speedArray[i] : 0),
                             (!cadArray.empty() ? cadArray[i] : 0),
                             (!altArray.empty() ? altArray[i] : 0),
                             (!tempArray.empty() ? tempArray[i] : 0));
                if (!wattsArray.empty())
                    totalWatts += wattsArray[i];
                if (!hrArray.empty())
                    totalHr    += hrArray[i];
                if (!speedArray.empty())
                    totalSpeed += speedArray[i];
                if (!cadArray.empty())
                    totalCad   += cadArray[i];
                if (!altArray.empty())
                    totalAlt   += altArray[i];
                if (!tempArray.empty())
                    totalTemp   += tempArray[i];
                totalDist   = distanceArray[i];
                list.append(dp);
                ++i;
            }
            while (!list.empty() && (list.front().time < secs - smooth)) {
                DataPoint &dp = list.front();
                totalWatts -= dp.watts;
                totalHr    -= dp.hr;
                totalSpeed -= dp.speed;
                totalCad   -= dp.cad;
                totalAlt   -= dp.alt;
                totalTemp   -= dp.temp;
                list.removeFirst();
            }
            // TODO: this is wrong.  We should do a weighted average over the
            // seconds represented by each point...
            if (list.empty()) {
                smoothWatts[secs] = 0.0;
                smoothHr[secs]    = 0.0;
                smoothSpeed[secs] = 0.0;
                smoothCad[secs]   = 0.0;
                smoothAltitude[secs]   = smoothAltitude[secs - 1];
                smoothTemp[secs]   = 0.0;
            }
            else {
                smoothWatts[secs]    = totalWatts / list.size();
                smoothHr[secs]       = totalHr / list.size();
                smoothSpeed[secs]    = totalSpeed / list.size();
                smoothCad[secs]      = totalCad / list.size();
                smoothAltitude[secs]      = totalAlt / list.size();
                smoothTemp[secs]      = totalTemp / list.size();
            }
            smoothDistance[secs] = totalDist;
            smoothTime[secs]  = secs / 60.0;
        }

    } else {

        // no smoothing .. just raw data
        smoothWatts.resize(0);
        smoothHr.resize(0);
        smoothSpeed.resize(0);
        smoothCad.resize(0);
        smoothTime.resize(0);
        smoothDistance.resize(0);
        smoothAltitude.resize(0);
        smoothTemp.resize(0);

        foreach (RideFilePoint *dp, rideItem->ride()->dataPoints()) {
            smoothWatts.append(dp->watts);
            smoothHr.append(dp->hr);
            smoothSpeed.append(useMetricUnits ? dp->kph : dp->kph * MILES_PER_KM);
            smoothCad.append(dp->cad);
            smoothTime.append(dp->secs/60);
            smoothDistance.append(useMetricUnits ? dp->km : dp->km * MILES_PER_KM);
            smoothAltitude.append(useMetricUnits ? dp->alt : dp->alt * FEET_PER_METER);
            smoothTemp.append(dp->temp);
        }
    }

    QVector<double> &xaxis = bydist ? smoothDistance : smoothTime;
    int startingIndex = qMin(smooth, xaxis.count());
    int totalPoints = xaxis.count() - startingIndex;

    // set curves - we set the intervalHighlighter to whichver is available
    if (!wattsArray.empty()) {

        wattsCurve->setData(xaxis.data() + startingIndex, smoothWatts.data() + startingIndex, totalPoints);
        intervalHighlighterCurve->setYAxis(yLeft);

    } if (!hrArray.empty()) {

        hrCurve->setData(xaxis.data() + startingIndex, smoothHr.data() + startingIndex, totalPoints);
        intervalHighlighterCurve->setYAxis(yLeft2);

    } if (!speedArray.empty()) {

        speedCurve->setData(xaxis.data() + startingIndex, smoothSpeed.data() + startingIndex, totalPoints);
        intervalHighlighterCurve->setYAxis(yRight);

    } if (!cadArray.empty()) {

        cadCurve->setData(xaxis.data() + startingIndex, smoothCad.data() + startingIndex, totalPoints);
        intervalHighlighterCurve->setYAxis(yLeft2);

    } if (!altArray.empty()) {

        altCurve->setData(xaxis.data() + startingIndex, smoothAltitude.data() + startingIndex, totalPoints);
        intervalHighlighterCurve->setYAxis(yRight2);

    } if (!tempArray.empty()) {

        tempCurve->setData(xaxis.data() + startingIndex, smoothTemp.data() + startingIndex, totalPoints);
        intervalHighlighterCurve->setYAxis(yRight);

    }

    setYMax();
    refreshIntervalMarkers();
    refreshZoneLabels();

    //replot();
}

void
AllPlot::refreshIntervalMarkers()
{
    foreach(QwtPlotMarker *mrk, d_mrk) {
        mrk->detach();
        delete mrk;
    }
    d_mrk.clear();
    QRegExp wkoAuto("^(Peak *[0-9]*(s|min)|Entire workout|Find #[0-9]*) *\\([^)]*\\)$");
    if (rideItem->ride()) {
        foreach(const RideFileInterval &interval, rideItem->ride()->intervals()) {
            // skip WKO autogenerated peak intervals
            if (wkoAuto.exactMatch(interval.name))
                continue;
            QwtPlotMarker *mrk = new QwtPlotMarker;
            d_mrk.append(mrk);
            mrk->attach(this);
            mrk->setLineStyle(QwtPlotMarker::VLine);
            mrk->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
            mrk->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));
            QwtText text(interval.name);
            text.setFont(QFont("Helvetica", 10, QFont::Bold));
            text.setColor(GColor(CPLOTMARKER));
            if (!bydist)
                mrk->setValue(interval.start / 60.0, 0.0);
            else
                mrk->setValue((useMetricUnits ? 1 : MILES_PER_KM) *
                                rideItem->ride()->timeToDistance(interval.start), 0.0);
            mrk->setLabel(text);
        }
    }
}

void
AllPlot::setYMax()
{
    if (wattsCurve->isVisible()) {
        double maxY = (referencePlot == NULL) ? (1.05 * wattsCurve->maxYValue()) :
                                             (1.05 * referencePlot->wattsCurve->maxYValue());
        // grid lines for 100 or 25
        QwtValueList xytick[QwtScaleDiv::NTickTypes];
        for (int i=0;i<maxY;i+=100)
            xytick[QwtScaleDiv::MajorTick]<<i;
#if 0
        for (int i=0;i<maxY;i+=25)
            xytick[QwtScaleDiv::MinorTick]<<i;
#endif

        setAxisTitle(yLeft, "Watts");
        //setAxisScale(yLeft, 0.0, maxY);
        setAxisScaleDiv(QwtPlot::yLeft,QwtScaleDiv(0.0,maxY,xytick));
        setAxisLabelRotation(yLeft,270);
        setAxisLabelAlignment(yLeft,Qt::AlignVCenter);
    }
    if (hrCurve->isVisible() || cadCurve->isVisible()) {
        double ymax = 0;
        QStringList labels;
        if (hrCurve->isVisible()) {
            labels << "BPM";
            if (referencePlot == NULL)
                ymax = hrCurve->maxYValue();
            else
                ymax = referencePlot->hrCurve->maxYValue();
        }
        if (cadCurve->isVisible()) {
            labels << "RPM";
            if (referencePlot == NULL)
                ymax = qMax(ymax, cadCurve->maxYValue());
            else
                ymax = qMax(ymax, referencePlot->cadCurve->maxYValue());
        }
        setAxisTitle(yLeft2, labels.join(" / "));
        setAxisScale(yLeft2, 0.0, 1.05 * ymax);
        setAxisLabelRotation(yLeft2,270);
        setAxisLabelAlignment(yLeft2,Qt::AlignVCenter);
    }
    if (speedCurve->isVisible() || tempCurve->isVisible()) {
        double ymax = 0;
        QStringList labels;

        if (speedCurve->isVisible()) {
            labels << (useMetricUnits ? tr("KPH") : tr("MPH"));

            if (referencePlot == NULL)
                ymax = speedCurve->maxYValue();
            else
                ymax = referencePlot->speedCurve->maxYValue();
        }
        if (tempCurve->isVisible()) {
            labels << QString::fromUtf8("°C");

            if (referencePlot == NULL)
                ymax = qMax(ymax, tempCurve->maxYValue());
            else
                ymax = qMax(ymax, referencePlot->tempCurve->maxYValue());
        }
        setAxisTitle(yRight, labels.join(" / "));
        setAxisScale(yRight, 0.0, 1.05 * ymax);
        setAxisLabelRotation(yRight,90);
        setAxisLabelAlignment(yRight,Qt::AlignVCenter);
    }
    if (altCurve->isVisible()) {
        setAxisTitle(yRight2, useMetricUnits ? tr("Meters") : tr("Feet"));
        double ymin,ymax;

        if (referencePlot == NULL) {
            ymin = altCurve->minYValue();
            ymax = qMax(ymin + 100, 1.05 * altCurve->maxYValue());
        } else {
            ymin = referencePlot->altCurve->minYValue();
            ymax = qMax(ymin + 100, 1.05 * referencePlot->altCurve->maxYValue());
        }
        setAxisScale(yRight2, ymin, ymax);
        setAxisLabelRotation(yRight2,90);
        setAxisLabelAlignment(yRight2,Qt::AlignVCenter);
        altCurve->setBaseline(ymin);
    }

    enableAxis(yLeft, wattsCurve->isVisible());
    enableAxis(yLeft2, hrCurve->isVisible() || cadCurve->isVisible());
    enableAxis(yRight, speedCurve->isVisible());
    enableAxis(yRight2, altCurve->isVisible());
}

void
AllPlot::setXTitle()
{
    if (bydist)
        setAxisTitle(xBottom, tr("Distance ")+QString(unit.toString() == "Metric"?"(km)":"(miles)"));
    else
        setAxisTitle(xBottom, tr("Time (minutes)"));
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
    if (!plot->distanceArray.count() || !plot->timeArray.count()) return;

    // reference the plot for data and state
    rideItem = plot->rideItem;
    bydist = plot->bydist;

    arrayLength = stopidx-startidx;

    if (bydist) {
        startidx = plot->distanceIndex(plot->distanceArray[startidx]);
        stopidx  = plot->distanceIndex(plot->distanceArray[(stopidx>=plot->distanceArray.size()?plot->distanceArray.size()-1:stopidx)])-1;
    } else {
        startidx = plot->timeIndex(plot->timeArray[startidx]/60);
        stopidx  = plot->timeIndex(plot->timeArray[(stopidx>=plot->timeArray.size()?plot->timeArray.size()-1:stopidx)]/60)-1;
    }

    // make sure indexes are still valid
    if (startidx > stopidx || startidx < 0 || stopidx < 0) return;

    double *smoothW = &plot->smoothWatts[startidx];
    double *smoothT = &plot->smoothTime[startidx];
    double *smoothHR = &plot->smoothHr[startidx];
    double *smoothS = &plot->smoothSpeed[startidx];
    double *smoothC = &plot->smoothCad[startidx];
    double *smoothA = &plot->smoothAltitude[startidx];
    double *smoothD = &plot->smoothDistance[startidx];
    double *smoothTE = &plot->smoothTemp[startidx];

    double *xaxis = bydist ? smoothD : smoothT;

    // attach appropriate curves
    //if (this->legend()) this->legend()->hide();

    wattsCurve->detach();
    hrCurve->detach();
    speedCurve->detach();
    cadCurve->detach();
    altCurve->detach();
    tempCurve->detach();

    wattsCurve->setVisible(rideItem->ride()->areDataPresent()->watts && showPowerState < 2);
    hrCurve->setVisible(rideItem->ride()->areDataPresent()->hr && showHrState == Qt::Checked);
    speedCurve->setVisible(rideItem->ride()->areDataPresent()->kph && showSpeedState == Qt::Checked);
    cadCurve->setVisible(rideItem->ride()->areDataPresent()->cad && showCadState == Qt::Checked);
    altCurve->setVisible(rideItem->ride()->areDataPresent()->alt && showAltState == Qt::Checked);
    tempCurve->setVisible(rideItem->ride()->areDataPresent()->temp && showTempState == Qt::Checked);

    wattsCurve->setData(xaxis,smoothW,stopidx-startidx);
    hrCurve->setData(xaxis, smoothHR,stopidx-startidx);
    speedCurve->setData(xaxis, smoothS, stopidx-startidx);
    cadCurve->setData(xaxis, smoothC, stopidx-startidx);
    altCurve->setData(xaxis, smoothA, stopidx-startidx);
    tempCurve->setData(xaxis, smoothTE, stopidx-startidx);

    QwtSymbol sym;
    sym.setPen(QPen(GColor(CPLOTMARKER)));
    if (stopidx-startidx < 150) {
        sym.setStyle(QwtSymbol::Ellipse);
        sym.setSize(3);
    } else {
        sym.setStyle(QwtSymbol::NoSymbol);
        sym.setSize(0);
    }
    wattsCurve->setSymbol(sym);
    hrCurve->setSymbol(sym);
    speedCurve->setSymbol(sym);
    cadCurve->setSymbol(sym);
    altCurve->setSymbol(sym);
    tempCurve->setSymbol(sym);

    setYMax();

    setAxisScale(xBottom, xaxis[0], xaxis[stopidx-startidx-1]);

    if (!plot->smoothAltitude.empty()) {
        altCurve->attach(this);
        intervalHighlighterCurve->setYAxis(yRight2);
    }
    if (!plot->smoothWatts.empty()) {
        wattsCurve->attach(this);
        intervalHighlighterCurve->setYAxis(yLeft);
    }
    if (!plot->smoothHr.empty()) {
        hrCurve->attach(this);
        intervalHighlighterCurve->setYAxis(yLeft2);
    }
    if (!plot->smoothSpeed.empty()) {
        speedCurve->attach(this);
        intervalHighlighterCurve->setYAxis(yRight);
    }
    if (!plot->smoothCad.empty()) {
        cadCurve->attach(this);
        intervalHighlighterCurve->setYAxis(yLeft2);
    }
    if (!plot->smoothTemp.empty()) {
        tempCurve->attach(this);
        intervalHighlighterCurve->setYAxis(yRight);
    }

    refreshIntervalMarkers();
    refreshZoneLabels();

    //if (this->legend()) this->legend()->show();
    //replot();
}

void
AllPlot::setDataFromRide(RideItem *_rideItem)
{
    rideItem = _rideItem;
    if (_rideItem == NULL) return;

    wattsArray.clear();

    RideFile *ride = rideItem->ride();
    if (ride && ride->deviceType() != QString("Manual CSV")) {

        const RideFileDataPresent *dataPresent = ride->areDataPresent();
        int npoints = ride->dataPoints().size();
        wattsArray.resize(dataPresent->watts ? npoints : 0);
        hrArray.resize(dataPresent->hr ? npoints : 0);
        speedArray.resize(dataPresent->kph ? npoints : 0);
        cadArray.resize(dataPresent->cad ? npoints : 0);
        altArray.resize(dataPresent->alt ? npoints : 0);
        timeArray.resize(npoints);
        distanceArray.resize(npoints);

        // attach appropriate curves
        wattsCurve->detach();
        hrCurve->detach();
        speedCurve->detach();
        cadCurve->detach();
        altCurve->detach();
        if (!altArray.empty()) altCurve->attach(this);
        if (!wattsArray.empty()) wattsCurve->attach(this);
        if (!hrArray.empty()) hrCurve->attach(this);
        if (!speedArray.empty()) speedCurve->attach(this);
        if (!cadArray.empty()) cadCurve->attach(this);

        wattsCurve->setVisible(dataPresent->watts && showPowerState < 2);
        hrCurve->setVisible(dataPresent->hr && showHrState == Qt::Checked);
        speedCurve->setVisible(dataPresent->kph && showSpeedState == Qt::Checked);
        cadCurve->setVisible(dataPresent->cad && showCadState == Qt::Checked);
        altCurve->setVisible(dataPresent->alt && showAltState == Qt::Checked);

        arrayLength = 0;
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

            timeArray[arrayLength]  = secs + msecs/1000;
            if (!wattsArray.empty())
                wattsArray[arrayLength] = max(0, point->watts);
            if (!hrArray.empty())
                hrArray[arrayLength]    = max(0, point->hr);
            if (!speedArray.empty())
                speedArray[arrayLength] = max(0,
                                              (useMetricUnits
                                               ? point->kph
                                               : point->kph * MILES_PER_KM));
            if (!cadArray.empty())
                cadArray[arrayLength]   = max(0, point->cad);
            if (!altArray.empty())
                altArray[arrayLength]   = (useMetricUnits
                                           ? point->alt
                                           : point->alt * FEET_PER_METER);
            distanceArray[arrayLength] = max(0,
                                             (useMetricUnits
                                              ? point->km
                                              : point->km * MILES_PER_KM));
            ++arrayLength;
        }
        recalc();
    }
    else {
        //setTitle("no data");

        wattsCurve->detach();
        hrCurve->detach();
        speedCurve->detach();
        cadCurve->detach();
        altCurve->detach();
        foreach(QwtPlotMarker *mrk, d_mrk)
            delete mrk;
        d_mrk.clear();
    }
}

void
AllPlot::showPower(int id)
{
    if (showPowerState == id) return;

    showPowerState = id;
    wattsCurve->setVisible(id < 2);
    shade_zones = (id == 0);
    setYMax();
    if (shade_zones) {
        bg->attach(this);
        refreshZoneLabels();
    } else
        bg->detach();
}

void
AllPlot::showHr(int state)
{
    showHrState = state;
    assert(state != Qt::PartiallyChecked);
    hrCurve->setVisible(state == Qt::Checked);
    setYMax();
    replot();
}

void
AllPlot::showSpeed(int state)
{
    showSpeedState = state;
    assert(state != Qt::PartiallyChecked);
    speedCurve->setVisible(state == Qt::Checked);
    setYMax();
    replot();
}

void
AllPlot::showCad(int state)
{
    showCadState = state;
    assert(state != Qt::PartiallyChecked);
    cadCurve->setVisible(state == Qt::Checked);
    setYMax();
    replot();
}

void
AllPlot::showAlt(int state)
{
    showAltState = state;
    assert(state != Qt::PartiallyChecked);
    altCurve->setVisible(state == Qt::Checked);
    setYMax();
    replot();
}

void
AllPlot::showTemp(int state)
{
    showTempState = state;
    assert(state != Qt::PartiallyChecked);
    tempCurve->setVisible(state == Qt::Checked);
    setYMax();
    replot();
}

void
AllPlot::showGrid(int state)
{
    assert(state != Qt::PartiallyChecked);
    grid->setVisible(state == Qt::Checked);
    replot();
}

void
AllPlot::setPaintBrush(int state)
{
    if (state) {

        QColor p;
        p = wattsCurve->pen().color();
        p.setAlpha(64);
        wattsCurve->setBrush(QBrush(p));

        p = hrCurve->pen().color();
        p.setAlpha(64);
        hrCurve->setBrush(QBrush(p));

        p = speedCurve->pen().color();
        p.setAlpha(64);
        speedCurve->setBrush(QBrush(p));

        p = cadCurve->pen().color();
        p.setAlpha(64);
        cadCurve->setBrush(QBrush(p));

    } else {
        wattsCurve->setBrush(Qt::NoBrush);
        hrCurve->setBrush(Qt::NoBrush);
        speedCurve->setBrush(Qt::NoBrush);
        cadCurve->setBrush(Qt::NoBrush);
    }
    replot();
}

void
AllPlot::setSmoothing(int value)
{
    smooth = value;
    recalc();
}

void
AllPlot::setByDistance(int id)
{
    bydist = (id == 1);
    setXTitle();
    recalc();
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
            smoothTime.begin(), smoothTime.end(), min, ComparePoints());
    if (i == smoothTime.end())
        return smoothTime.size();
    return i - smoothTime.begin();
}



int
AllPlot::distanceIndex(double km) const
{
    // return index offset for specified distance in km
	QVector<double>::const_iterator i = std::lower_bound(
            smoothDistance.begin(), smoothDistance.end(), km, ComparePoints());
    if (i == smoothDistance.end())
        return smoothDistance.size();
    return i - smoothDistance.begin();
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
    const QTreeWidgetItem *allIntervals = mainWindow->allIntervalItems();
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
    if (mainWindow->allIntervalItems() == NULL) return 0; // not inited yet!

    const QTreeWidgetItem *allIntervals = mainWindow->allIntervalItems();
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
// been selected in the MainWindow leftlayout for each selected
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

    double multiplier = allPlot->useMetricUnits ? 1 : MILES_PER_KM;

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
QwtData *IntervalPlotData::copy() const {
    return new IntervalPlotData(allPlot, mainWindow);
}

void
AllPlot::pointHover(QwtPlotCurve *curve, int index)
{
    if (index >= 0 && curve != intervalHighlighterCurve) {

        double yvalue = curve->y(index);
        double xvalue = curve->x(index);

        // output the tooltip
        QString text = QString("%1 %2\n%3 %4")
                        .arg(yvalue, 0, 'f', 0)
                        .arg(this->axisTitle(curve->yAxis()).text())
                        .arg(xvalue, 0, 'f', 2)
                        .arg(this->axisTitle(curve->xAxis()).text());

        // set that text up
        tooltip->setText(text);

    } else {

        // no point
        tooltip->setText("");
    }
}
