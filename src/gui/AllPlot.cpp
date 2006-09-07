/* 
 * $Id: AllPlot.cpp,v 1.2 2006/07/12 02:13:57 srhea Exp $
 *
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
#include "RawFile.h"
#include "Settings.h"

#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_legend.h>
#include <qwt_data.h>

static const inline double
max(double a, double b) { if (a > b) return a; else return b; }

AllPlot::AllPlot() : 
    hrArray(NULL), wattsArray(NULL), 
    speedArray(NULL), cadArray(NULL), 
    timeArray(NULL), smooth(30)
{
    insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setCanvasBackground(Qt::white);

    setAxisTitle(xBottom, "Time (minutes)");
    
    wattsCurve = new QwtPlotCurve("Power");
    wattsCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    wattsCurve->setPen(QPen(Qt::red));
    wattsCurve->attach(this);

    hrCurve = new QwtPlotCurve("Heart Rate");
    hrCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    hrCurve->setPen(QPen(Qt::blue));
    hrCurve->attach(this);

    speedCurve = new QwtPlotCurve("Speed");
    speedCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    speedCurve->setPen(QPen(Qt::green));
    speedCurve->attach(this);

    cadCurve = new QwtPlotCurve("Cadence");
    cadCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    cadCurve->setPen(QPen(Qt::cyan));
    cadCurve->attach(this);

    grid = new QwtPlotGrid();
    grid->enableX(false);
    QPen gridPen;
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);
    grid->attach(this);
}

struct DataPoint {
    double time, hr, watts, speed, cad;
    DataPoint(double t, double h, double w, double s, double c) : 
        time(t), hr(h), watts(w), speed(s), cad(c) {}
};

void
AllPlot::recalc()
{
    int rideTimeSecs = (int) ceil(timeArray[arrayLength - 1]);
    double totalWatts = 0.0;
    double totalHr = 0.0;
    double totalSpeed = 0.0;
    double totalCad = 0.0;
    QList<DataPoint*> list;
    int i = 0;
    double *smoothWatts = new double[rideTimeSecs + 1];
    double *smoothHr    = new double[rideTimeSecs + 1];
    double *smoothSpeed = new double[rideTimeSecs + 1];
    double *smoothCad   = new double[rideTimeSecs + 1];
    double *smoothTime  = new double[rideTimeSecs + 1];
    for (int secs = 0; secs < smooth; ++secs) {
        smoothWatts[secs] = 0.0;
        smoothHr[secs]    = 0.0;
        smoothSpeed[secs] = 0.0;
        smoothCad[secs]   = 0.0;
        smoothTime[secs]  = secs / 60.0;
    }
    for (int secs = smooth; secs <= rideTimeSecs; ++secs) {
        while ((i < arrayLength) && (timeArray[i] <= secs)) {
            DataPoint *dp = 
                new DataPoint(timeArray[i], hrArray[i], wattsArray[i], 
                              speedArray[i], cadArray[i]);
            totalWatts += wattsArray[i];
            totalHr    += hrArray[i];
            totalSpeed += speedArray[i];
            totalCad   += cadArray[i];
            list.append(dp);
            ++i;
        }
        while (!list.empty() && (list.front()->time < secs - smooth)) {
            DataPoint *dp = list.front();
            list.removeFirst();
            totalWatts -= dp->watts;
            totalHr    -= dp->hr;
            totalSpeed -= dp->speed;
            totalCad   -= dp->cad;
            delete dp;
        }
        // TODO: this is wrong.  We should do a weighted average over the
        // seconds represented by each point...
        if (list.empty()) {
            smoothWatts[secs] = 0.0;
            smoothHr[secs]    = 0.0;
            smoothSpeed[secs] = 0.0;
            smoothCad[secs]   = 0.0;
        }
        else {
            smoothWatts[secs] = totalWatts / list.size();
            smoothHr[secs]    = totalHr / list.size();
            smoothSpeed[secs]    = totalSpeed / list.size();
            smoothCad[secs]    = totalCad / list.size();
        }
        smoothTime[secs]  = secs / 60.0;
    }
    wattsCurve->setData(smoothTime, smoothWatts, rideTimeSecs + 1);
    hrCurve->setData(smoothTime, smoothHr, rideTimeSecs + 1);
    speedCurve->setData(smoothTime, smoothSpeed, rideTimeSecs + 1);
    cadCurve->setData(smoothTime, smoothCad, rideTimeSecs + 1);
    setAxisScale(xBottom, 0.0, smoothTime[rideTimeSecs]);
    setYMax();

    replot();
    delete [] smoothWatts;
    delete [] smoothHr;
    delete [] smoothSpeed;
    delete [] smoothCad;
    delete [] smoothTime;
}

void
AllPlot::setYMax() 
{
    double ymax = 0;
    QString ylabel = "";
    if (wattsCurve->isVisible()) {
        ymax = max(ymax, wattsCurve->maxYValue());
        ylabel += QString((ylabel == "") ? "" : " / ") + "Watts";
    }
    if (hrCurve->isVisible()) {
        ymax = max(ymax, hrCurve->maxYValue());
        ylabel += QString((ylabel == "") ? "" : " / ") + "BPM";
    }
    if (speedCurve->isVisible()) {
        ymax = max(ymax, speedCurve->maxYValue());
        ylabel += QString((ylabel == "") ? "" : " / ") + "MPH";
    }
    if (cadCurve->isVisible()) {
        ymax = max(ymax, cadCurve->maxYValue());
        ylabel += QString((ylabel == "") ? "" : " / ") + "RPM";
    }
    setAxisScale(yLeft, 0.0, ymax * 1.1);
    setAxisTitle(yLeft, ylabel);
}

void
AllPlot::setData(RawFile *raw)
{
    delete [] wattsArray;
    delete [] hrArray;
    delete [] speedArray;
    delete [] cadArray;
    delete [] timeArray;
    setTitle(raw->startTime.toString(GC_DATETIME_FORMAT));
    wattsArray = new double[raw->points.size()];
    hrArray    = new double[raw->points.size()];
    speedArray    = new double[raw->points.size()];
    cadArray    = new double[raw->points.size()];
    timeArray  = new double[raw->points.size()];
    arrayLength = 0;
    QListIterator<RawFilePoint*> i(raw->points); 
    while (i.hasNext()) {
        RawFilePoint *point = i.next();
        timeArray[arrayLength]  = point->secs;
        wattsArray[arrayLength] = max(0, point->watts);
        hrArray[arrayLength]    = max(0, point->hr);
        speedArray[arrayLength] = max(0, point->mph);
        cadArray[arrayLength]   = max(0, point->cad);
        ++arrayLength;
    }
    recalc();
}

void
AllPlot::showPower(int state) 
{
    assert(state != Qt::PartiallyChecked);
    wattsCurve->setVisible(state == Qt::Checked);
    setYMax();
    replot();
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

