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
#include "RideFile.h"
#include "RideItem.h"
#include "Settings.h"
#include "Units.h"
#include "Zones.h"

#include <assert.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_text.h>
#include <qwt_legend.h>
#include <qwt_data.h>
#include <QMultiMap>


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

        Zones **zones      = rideItem->zones;
        int zone_range     = rideItem->zoneRange();

        if (parent->shadeZones() && zones && *zones && (zone_range >= 0)) {
            QList <int> zone_lows = (*zones)->getZoneLows(zone_range);
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


            Zones **zones      = rideItem->zones;
            int zone_range     = rideItem->zoneRange();

            // create new zone labels if we're shading
            if (parent->shadeZones() && zones && *zones && (zone_range >= 0)) {
                QList <int> zone_lows = (*zones)->getZoneLows(zone_range);
                QList <QString> zone_names = (*zones)->getZoneNames(zone_range);
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
                    text.setFont(QFont("Helvetica",24, QFont::Bold));
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

AllPlot::AllPlot(QWidget *parent):
    QwtPlot(parent),
    settings(NULL),
    unit(0),
    rideItem(NULL),
    smooth(30), bydist(false),
    shade_zones(false)
{
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    unit = settings->value(GC_UNIT);

    useMetricUnits = (unit.toString() == "Metric");

    // create a background object for shading
    bg = new AllPlotBackground(this);
    bg->attach(this);

    insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setCanvasBackground(Qt::white);

    setXTitle();

    wattsCurve = new QwtPlotCurve("Power");
    QPen wattsPen = QPen(Qt::red);
    wattsPen.setWidth(2);
    wattsCurve->setPen(wattsPen);

    hrCurve = new QwtPlotCurve("Heart Rate");
    QPen hrPen = QPen(Qt::blue);
    hrPen.setWidth(2);
    hrCurve->setPen(hrPen);
    hrCurve->setYAxis(yLeft2);

    speedCurve = new QwtPlotCurve("Speed");
    QPen speedPen = QPen(QColor(0, 204, 0));
    speedPen.setWidth(2);
    speedCurve->setPen(speedPen);
    speedCurve->setYAxis(yRight);

    cadCurve = new QwtPlotCurve("Cadence");
    QPen cadPen = QPen(QColor(0, 204, 204));
    cadPen.setWidth(2);
    cadCurve->setPen(cadPen);
    cadCurve->setYAxis(yLeft2);

    altCurve = new QwtPlotCurve("Altitude");
    // altCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen altPen(QColor(124, 91, 31));
    altPen.setWidth(1);
    altCurve->setPen(altPen);
    QColor brush_color = QColor(124, 91, 31);
    brush_color.setAlpha(64);
    altCurve->setBrush(brush_color);   // fill below the line
    altCurve->setYAxis(yRight2);

    grid = new QwtPlotGrid();
    grid->enableX(false);
    QPen gridPen;
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);
    grid->attach(this);

    zoneLabels = QList <AllPlotZoneLabel *>::QList();
}

struct DataPoint {
    double time, hr, watts, speed, cad, alt;
    DataPoint(double t, double h, double w, double s, double c, double a) :
        time(t), hr(h), watts(w), speed(s), cad(c), alt(a) {}
};

bool AllPlot::shadeZones() const
{
    return (shade_zones && !wattsArray.empty());
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
        Zones **zones  = rideItem->zones;

        // generate labels for existing zones
        if (zones && *zones && (zone_range >= 0)) {
            int num_zones = (*zones)->numZones(zone_range);
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
        return;
    }
    double totalWatts = 0.0;
    double totalHr = 0.0;
    double totalSpeed = 0.0;
    double totalCad = 0.0;
    double totalDist = 0.0;
    double totalAlt = 0.0;

    QList<DataPoint> list;

    QVector<double> smoothWatts(rideTimeSecs + 1);
    QVector<double> smoothHr(rideTimeSecs + 1);
    QVector<double> smoothSpeed(rideTimeSecs + 1);
    QVector<double> smoothCad(rideTimeSecs + 1);
    QVector<double> smoothTime(rideTimeSecs + 1);
    QVector<double> smoothDistance(rideTimeSecs + 1);
    QVector<double> smoothAltitude(rideTimeSecs + 1);

    for (int secs = 0; ((secs < smooth)
                        && (secs < rideTimeSecs)); ++secs) {
        smoothWatts[secs] = 0.0;
        smoothHr[secs]    = 0.0;
        smoothSpeed[secs] = 0.0;
        smoothCad[secs]   = 0.0;
        smoothTime[secs]  = secs / 60.0;
        smoothDistance[secs]  = 0.0;
        smoothAltitude[secs]  = 0.0;
    }

    int i = 0;
    for (int secs = smooth; secs <= rideTimeSecs; ++secs) {
        while ((i < arrayLength) && (timeArray[i] <= secs)) {
            DataPoint dp(timeArray[i],
                         (!hrArray.empty() ? hrArray[i] : 0),
                         (!wattsArray.empty() ? wattsArray[i] : 0),
                         (!speedArray.empty() ? speedArray[i] : 0),
                         (!cadArray.empty() ? cadArray[i] : 0),
                         (!altArray.empty() ? altArray[i] : 0));
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
        }
        else {
            smoothWatts[secs]    = totalWatts / list.size();
            smoothHr[secs]       = totalHr / list.size();
            smoothSpeed[secs]    = totalSpeed / list.size();
            smoothCad[secs]      = totalCad / list.size();
            smoothAltitude[secs]      = totalAlt / list.size();
        }
        smoothDistance[secs] = totalDist;
        smoothTime[secs]  = secs / 60.0;
    }

    QVector<double> &xaxis = bydist ? smoothDistance : smoothTime;
    int startingIndex = qMin(smooth, rideTimeSecs);
    int totalPoints = rideTimeSecs + 1 - startingIndex;
    // set curves
    if (!wattsArray.empty())
        wattsCurve->setData(xaxis.data() + startingIndex, smoothWatts.data() + startingIndex, totalPoints);
    if (!hrArray.empty())
        hrCurve->setData(xaxis.data() + startingIndex, smoothHr.data() + startingIndex, totalPoints);
    if (!speedArray.empty())
        speedCurve->setData(xaxis.data() + startingIndex, smoothSpeed.data() + startingIndex, totalPoints);
    if (!cadArray.empty())
        cadCurve->setData(xaxis.data() + startingIndex, smoothCad.data() + startingIndex, totalPoints);
    if (!altArray.empty())
        altCurve->setData(xaxis.data() + startingIndex, smoothAltitude.data() + startingIndex, totalPoints);

    setAxisScale(xBottom, 0.0, bydist ? totalDist : smoothTime[rideTimeSecs]);
    setYMax();

    refreshZoneLabels();

    foreach(QwtPlotMarker *mrk, d_mrk) {
        mrk->detach();
        delete mrk;
    }
    d_mrk.clear();
    if (rideItem->ride) {
        rideItem->ride->fillInIntervals();
        foreach(const RideFileInterval &interval, rideItem->ride->intervals()) {
            QwtPlotMarker *mrk = new QwtPlotMarker;
            d_mrk.append(mrk);
            mrk->attach(this);
            mrk->setLineStyle(QwtPlotMarker::VLine);
            mrk->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
            mrk->setLinePen(QPen(Qt::black, 0, Qt::DashDotLine));
            QwtText text(interval.name);
            text.setFont(QFont("Helvetica", 10, QFont::Bold));
            text.setColor(Qt::black);
            if (!bydist)
                mrk->setValue(interval.start / 60.0, 0.0);
            else
                mrk->setValue(smoothDistance[int(ceil(interval.start))], 0.0);
            mrk->setLabel(text);
        }
    }

    replot();
}

void
AllPlot::setYMax()
{
    if (wattsCurve->isVisible()) {
        setAxisTitle(yLeft, "Watts");
        setAxisScale(yLeft, 0.0, 1.05 * wattsCurve->maxYValue());
        setAxisLabelRotation(yLeft,270);
        setAxisLabelAlignment(yLeft,Qt::AlignVCenter);
    }
    if (hrCurve->isVisible() || cadCurve->isVisible()) {
        double ymax = 0;
        QStringList labels;
        if (hrCurve->isVisible()) {
            labels << "BPM";
            ymax = hrCurve->maxYValue();
        }
        if (cadCurve->isVisible()) {
            labels << "RPM";
            ymax = qMax(ymax, cadCurve->maxYValue());
        }
        setAxisTitle(yLeft2, labels.join(" / "));
        setAxisScale(yLeft2, 0.0, 1.05 * ymax);
        setAxisLabelRotation(yLeft2,270);
        setAxisLabelAlignment(yLeft2,Qt::AlignVCenter);
    }
    if (speedCurve->isVisible()) {
        setAxisTitle(yRight, (useMetricUnits ? "KPH" : "MPH"));
        setAxisScale(yRight, 0.0, 1.05 * speedCurve->maxYValue());
        setAxisLabelRotation(yRight,90);
        setAxisLabelAlignment(yRight,Qt::AlignVCenter);
    }
    if (altCurve->isVisible()) {
        setAxisTitle(yRight2, useMetricUnits ? "Meters" : "Feet");
        double ymin = altCurve->minYValue();
        double ymax = qMax(ymin + 100, 1.05 * altCurve->maxYValue());
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
        setAxisTitle(xBottom, "Distance "+QString(unit.toString() == "Metric"?"(km)":"(miles)"));
    else
        setAxisTitle(xBottom, "Time (minutes)");
}

void
AllPlot::setData(RideItem *_rideItem)
{
    rideItem = _rideItem;

    wattsArray.clear();

    RideFile *ride = rideItem->ride;
    if (ride) {
        setTitle(ride->startTime().toString(GC_DATETIME_FORMAT));

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
        if (!wattsArray.empty()) wattsCurve->attach(this);
        if (!hrArray.empty()) hrCurve->attach(this);
        if (!speedArray.empty()) speedCurve->attach(this);
        if (!cadArray.empty()) cadCurve->attach(this);
        if (!altArray.empty()) altCurve->attach(this);

        arrayLength = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            timeArray[arrayLength]  = point->secs;
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
        setTitle("no data");
        wattsCurve->detach();
        hrCurve->detach();
        speedCurve->detach();
        cadCurve->detach();
        altCurve->detach();
    }
}

void
AllPlot::showPower(int id)
{
    wattsCurve->setVisible(id < 2);
    shade_zones = (id == 0);
    setYMax();
    recalc();
}

void
AllPlot::showHr(int state)
{
    assert(state != Qt::PartiallyChecked);
    hrCurve->setVisible(state == Qt::Checked);
    setYMax();
    replot();
}

void
AllPlot::showSpeed(int state)
{
    assert(state != Qt::PartiallyChecked);
    speedCurve->setVisible(state == Qt::Checked);
    setYMax();
    replot();
}

void
AllPlot::showCad(int state)
{
    assert(state != Qt::PartiallyChecked);
    cadCurve->setVisible(state == Qt::Checked);
    setYMax();
    replot();
}

void
AllPlot::showAlt(int state)
{
    assert(state != Qt::PartiallyChecked);
    altCurve->setVisible(state == Qt::Checked);
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
