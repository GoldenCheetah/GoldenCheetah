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

#include "PowerHist.h"
#include "RideFile.h"
#include "Settings.h"

#include <assert.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_legend.h>
#include <qwt_data.h>

PowerHist::PowerHist() : 
    array(NULL), binw(20)
{
    setCanvasBackground(Qt::white);

    setAxisTitle(xBottom, "Power (watts)");
    setAxisTitle(yLeft, "Cumulative Time (minutes)");
    
    curve = new QwtPlotCurve("Power");
    curve->setStyle(QwtPlotCurve::Steps);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    curve->setPen(QPen(Qt::red));
    curve->attach(this);

    grid = new QwtPlotGrid();
    grid->enableX(false);
    QPen gridPen;
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);
    grid->attach(this);
}

void
PowerHist::recalc()
{
    if (!array)
        return;
    int count = (int) ceil((arrayLength - 1) / binw);
    double *smoothWatts = new double[count+1];
    double *smoothTime  = new double[count+1];
    int i;
    for (i = 0; i < count; ++i) {
        int low = i * binw;
        int high = low + binw;
        smoothWatts[i] = low;
        smoothTime[i]  = 0.0;
        while (low < high)
            smoothTime[i] += array[low++] / 60.0;
    }
    smoothTime[i] = 0.0;
    smoothWatts[i] = i * binw;
    curve->setData(smoothWatts, smoothTime, count+1);
    setAxisScale(xBottom, 0.0, smoothWatts[count]);
    setYMax();
    replot();
    delete [] smoothWatts;
    delete [] smoothTime;
}

void
PowerHist::setYMax() 
{
    setAxisScale(yLeft, 0.0, curve->maxYValue() * 1.1);
}

void
PowerHist::setData(RideFile *ride)
{
    setTitle(ride->startTime().toString(GC_DATETIME_FORMAT));
    QMap<double,double> powerHist;
    QListIterator<RideFilePoint*> j(ride->dataPoints());
    while (j.hasNext()) {
        const RideFilePoint *p1 = j.next();
        if (powerHist.contains(p1->watts))
            powerHist[p1->watts] += ride->recIntSecs();
        else
            powerHist[p1->watts] = ride->recIntSecs();
    }
    assert(powerHist.keys().first() >= 0);
    int maxPower = (int) round(powerHist.keys().last());
    delete [] array;
    arrayLength = maxPower + 1;
    array = new double[arrayLength];
    for (int i = 0; i < arrayLength; ++i)
        array[i] = 0.0;
    QMapIterator<double,double> i(powerHist);
    while (i.hasNext()) {
        i.next();
        array[(int) round(i.key())] += i.value();
    }
    recalc();
}

void
PowerHist::setBinWidth(int value)
{
    binw = value;
    recalc();
}

