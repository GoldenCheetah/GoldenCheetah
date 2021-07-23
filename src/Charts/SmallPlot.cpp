/*
 * Copyright (c) 2011 Damien Grauser
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

#include "SmallPlot.h"
#include "RideFile.h"
#include "RideItem.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"

#include <qwt_plot_curve.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_text.h>
#include <qwt_legend.h>
#include <qwt_series_data.h>

static double inline max(double a, double b) { if (a > b) return a; else return b; }

SmallPlot::SmallPlot(QWidget *parent) : QwtPlot(parent), d_mrk(NULL), smooth(30)
{
    setCanvasBackground(GColor(CPLOTBACKGROUND));
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

    setXTitle();
    setAxesCount(QwtAxis::yLeft, 2);

    altCurve = new QwtPlotCurve(tr("Altitude"));
    altCurve->setPen(QPen(GColor(CALTITUDE)));
    QColor brush_color = GColor(CALTITUDEBRUSH);
    brush_color.setAlpha(180);
    altCurve->setBrush(brush_color);
    altCurve->setYAxis(QwtAxisId(QwtAxis::yLeft,1));
    altCurve->attach(this);

    wattsCurve = new QwtPlotCurve("Power");
    //timeCurves.resize(36);// wattsCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    wattsCurve->setYAxis(QwtAxisId(QwtAxis::yLeft,0));
    wattsCurve->setPen(QPen(GColor(CPOWER)));
    wattsCurve->attach(this);

    hrCurve = new QwtPlotCurve("Heart Rate");
    // hrCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    hrCurve->setYAxis(QwtAxisId(QwtAxis::yLeft,0));
    hrCurve->setPen(QPen(GColor(CHEARTRATE)));
    hrCurve->attach(this);

    // grid lines on such a small plot look AWFUL
    //grid = new QwtPlotGrid();
    //grid->enableX(false);
    //QPen gridPen;
    //gridPen.setStyle(Qt::DotLine);
    //grid->setPen(QPen(GColor(CPLOTGRID)));
    //grid->attach(this);

    //timeCurves.resize(36);
    //for (int i = 0; i < 36; ++i) {
        //QColor color = QColor(255,255,255);
        //color.setHsv(60+i*(360/36), 255,255,255);

        //QPen pen = QPen(color);
        //pen.setWidth(3);

        //timeCurves[i] = new QwtPlotCurve();
        //timeCurves[i]->setPen(pen);
        //timeCurves[i]->setStyle(QwtPlotCurve::Lines);
        //timeCurves[i]->setRenderHint(QwtPlotItem::RenderAntialiased);
        //timeCurves[i]->attach(this);
        //QwtLegend *legend = new QwtLegend;
        //legend->setVisible(false);
        //legend->setDisabled(true);
        //timeCurves[i]->updateLegend(legend);
     //}
}

struct DataPoint {
    double time, hr, alt, watts;
    int inter;
    DataPoint(double t, double h, double w, double a, int i) :
             time(t), hr(h), alt(a), watts(w), inter(i) {}
};

void
SmallPlot::recalc()
{
    if (!timeArray.size()) return;

    long rideTimeSecs = (long) ceil(timeArray[arrayLength - 1]);
    if (rideTimeSecs < 0 || rideTimeSecs > SECONDS_IN_A_WEEK) {
        QwtArray<double> data;
        wattsCurve->setSamples(data, data);
        hrCurve->setSamples(data, data);
        altCurve->setSamples(data, data);
        return;
    }

    double totalWatts = 0.0;
    double totalHr = 0.0;
    double totalAlt = 0.0;
    QList<DataPoint*> list;
    int i = 0;

    QVector<double> smoothWatts(rideTimeSecs + 1);
    QVector<double> smoothHr(rideTimeSecs + 1);
    QVector<double> smoothAlt(rideTimeSecs + 1);
    QVector<double> smoothTime(rideTimeSecs + 1);

    QList<double> interList; //Just store the time that it happened.
                             //Intervals are sequential.

    int lastInterval = 0; //Detect if we hit a new interval

    for (int secs = 0; ((secs < smooth) && (secs < rideTimeSecs)); ++secs) {
        smoothWatts[secs] = 0.0;
        smoothHr[secs]    = 0.0;
        smoothAlt[secs]    = 0.0;
    }
    for (int secs = smooth; secs <= rideTimeSecs; ++secs) {
        while ((i < arrayLength) && (timeArray[i] <= secs)) {
            DataPoint *dp =
                new DataPoint(timeArray[i], hrArray[i], wattsArray[i], altArray[i], interArray[i]);
            totalWatts += wattsArray[i];
            totalHr    += hrArray[i];
            totalAlt    += altArray[i];
            list.append(dp);
            //Figure out when and if we have a new interval..
            if(lastInterval != interArray[i]) {
                lastInterval = interArray[i];
                interList.append(secs/60.0);
            }
            ++i;
        }
        while (!list.empty() && (list.front()->time < secs - smooth)) {
            DataPoint *dp = list.front();
            list.removeFirst();
            totalWatts -= dp->watts;
            totalHr    -= dp->hr;
            totalAlt    -= dp->alt;
            delete dp;
        }
        // TODO: this is wrong.  We should do a weighted average over the
        // seconds represented by each point...
        if (list.empty()) {
            smoothWatts[secs] = 0.0;
            smoothHr[secs]    = 0.0;
            smoothAlt[secs]    = 0.0;
        }
        else {
            smoothWatts[secs]    = totalWatts / list.size();
            smoothHr[secs]       = totalHr / list.size();
            smoothAlt[secs]       = totalAlt / list.size();
        }
        smoothTime[secs]  = secs / 60.0;
    }
    wattsCurve->setSamples(smoothTime.constData(), smoothWatts.constData(), rideTimeSecs + 1);
    hrCurve->setSamples(smoothTime.constData(), smoothHr.constData(), rideTimeSecs + 1);
    altCurve->setSamples(smoothTime.constData(), smoothAlt.constData(), rideTimeSecs + 1);
    setAxisScale(xBottom, 0.0, smoothTime[rideTimeSecs]);

    setYMax();
    replot();
}

void
SmallPlot::setYMax()
{
    double ymax = 0;
    double y1max = 500;
    QString ylabel = "";
    QString y1label = "";
    if (wattsCurve->isVisible()) {
        ymax = max(ymax, wattsCurve->maxYValue());
        ylabel += QString((ylabel == "") ? "" : " / ") + tr("Watts");
    }
    if (hrCurve->isVisible()) {
        ymax = max(ymax, hrCurve->maxYValue());
        ylabel += QString((ylabel == "") ? "" : " / ") + tr("BPM");
    }
    if (altCurve->isVisible()) {
         y1max = max(y1max, altCurve->maxYValue());
         y1label = "m";
    }
    setAxisScale(QwtAxisId(QwtAxis::yLeft,0), 0.0, ymax * 1.1);
    setAxisTitle(QwtAxisId(QwtAxis::yLeft,0), ylabel);
    setAxisScale(QwtAxisId(QwtAxis::yLeft,1), 0.0, y1max * 1.1);
    setAxisTitle(QwtAxisId(QwtAxis::yLeft,1), y1label);
    setAxisVisible(QwtAxisId(QwtAxis::yLeft,0), false); // hide for a small plot
    setAxisVisible(QwtAxisId(QwtAxis::yLeft,1), false); // hide for a small plot
}

void
SmallPlot::setXTitle()
{
    setAxisTitle(xBottom, tr("Time (minutes)"));
    enableAxis(xBottom, true);
}

void
SmallPlot::setAxisTitle(QwtAxisId axis, QString label)
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
SmallPlot::setData(RideItem *rideItem)
{
    RideFile *ride = rideItem->ride();
    setData(ride);
}

void
SmallPlot::setData(RideFile *ride)
{

    wattsArray.resize(ride->dataPoints().size());
    hrArray.resize(ride->dataPoints().size());
    altArray.resize(ride->dataPoints().size());
    timeArray.resize(ride->dataPoints().size());
    interArray.resize(ride->dataPoints().size());

    arrayLength = 0;
    foreach (const RideFilePoint *point, ride->dataPoints()) {
        timeArray[arrayLength]  = point->secs;
        wattsArray[arrayLength] = max(0, point->watts);
        hrArray[arrayLength]    = max(0, point->hr);
        altArray[arrayLength]    = max(0, point->alt);
        interArray[arrayLength] = point->interval;
        ++arrayLength;
    }
    recalc();
}

void
SmallPlot::showPower(int state)
{
    wattsCurve->setVisible(state == Qt::Checked);
    setYMax();
    replot();
}

void
SmallPlot::showHr(int state)
{
    hrCurve->setVisible(state == Qt::Checked);
    setYMax();
    replot();
}

void
SmallPlot::setSmoothing(int value)
{
    smooth = value;
    recalc();
}
