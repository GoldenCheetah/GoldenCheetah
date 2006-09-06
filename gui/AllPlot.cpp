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
    hrArray(NULL), wattsArray(NULL), timeArray(NULL), smooth(30)
{
    insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setCanvasBackground(Qt::white);

    setAxisTitle(xBottom, "Time (minutes)");
    setAxisTitle(yLeft, "Power / HR");
    
    wattsCurve = new QwtPlotCurve("Power");
    wattsCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    wattsCurve->setPen(QPen(Qt::red));
    wattsCurve->attach(this);

    hrCurve = new QwtPlotCurve("Heart Rate");
    hrCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    hrCurve->setPen(QPen(Qt::blue));
    hrCurve->attach(this);

    grid = new QwtPlotGrid();
    grid->enableX(false);
    QPen gridPen;
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);
    grid->attach(this);
}

struct DataPoint {
    double time, hr, watts;
    DataPoint(double t, double h, double w) : time(t), hr(h), watts(w) {}
};

void
AllPlot::recalc()
{
    int rideTimeSecs = (int) ceil(timeArray[arrayLength - 1]);
    double ymax = 0.0;
    double totalWatts = 0.0;
    double totalHr = 0.0;
    QList<DataPoint*> list;
    int i = 0;
    double *smoothWatts = new double[rideTimeSecs + 1];
    double *smoothHr    = new double[rideTimeSecs + 1];
    double *smoothTime  = new double[rideTimeSecs + 1];
    for (int secs = 0; secs < smooth; ++secs) {
        smoothWatts[secs] = 0.0;
        smoothHr[secs]    = 0.0;
        smoothTime[secs]  = secs / 60.0;
    }
    for (int secs = smooth; secs <= rideTimeSecs; ++secs) {
        while ((i < arrayLength) && (timeArray[i] <= secs)) {
            DataPoint *dp = 
                new DataPoint(timeArray[i], hrArray[i], wattsArray[i]);
            totalWatts += wattsArray[i];
            totalHr    += hrArray[i];
            list.append(dp);
            ++i;
        }
        while (!list.empty() && (list.front()->time < secs - smooth)) {
            DataPoint *dp = list.front();
            list.removeFirst();
            totalWatts -= dp->watts;
            totalHr    -= dp->hr;
            delete dp;
        }
        // TODO: this is wrong.  We should do a weighted average over the
        // seconds represented by each point...
        if (list.empty()) {
            smoothWatts[secs] = 0.0;
            smoothHr[secs]    = 0.0;
        }
        else {
            smoothWatts[secs] = totalWatts / list.size();
            if (smoothWatts[secs] > ymax)
                ymax = smoothWatts[secs];
            smoothHr[secs]    = totalHr / list.size();
            if (smoothHr[secs] > ymax)
                ymax = smoothHr[secs];
        }
        smoothTime[secs]  = secs / 60.0;
    }
    wattsCurve->setData(smoothTime, smoothWatts, rideTimeSecs + 1);
    hrCurve->setData(smoothTime, smoothHr, rideTimeSecs + 1);
    setAxisScale(xBottom, 0.0, smoothTime[rideTimeSecs]);
    setAxisScale(yLeft, 0.0, ymax + 30);
    replot();
    delete [] smoothWatts;
    delete [] smoothHr;
    delete [] smoothTime;
}

void
AllPlot::setData(RawFile *raw)
{
    delete [] hrArray;
    delete [] wattsArray;
    delete [] timeArray;
    setTitle(raw->startTime.toString(GC_DATETIME_FORMAT));
    hrArray    = new double[raw->points.size()];
    wattsArray = new double[raw->points.size()];
    timeArray  = new double[raw->points.size()];
    arrayLength = 0;
    QListIterator<RawFilePoint*> i(raw->points); 
    while (i.hasNext()) {
        RawFilePoint *point = i.next();
        timeArray[arrayLength]  = point->secs;
        hrArray[arrayLength]    = max(0, point->hr);
        wattsArray[arrayLength] = max(0, point->watts);
        ++arrayLength;
    }
    recalc();
}

void
AllPlot::showPower(int state) 
{
    assert(state != Qt::PartiallyChecked);
    wattsCurve->setVisible(state == Qt::Checked);
    replot();
}

void
AllPlot::showHr(int state) 
{
    assert(state != Qt::PartiallyChecked);
    hrCurve->setVisible(state == Qt::Checked);
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

