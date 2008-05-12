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
#include "RideFile.h"
#include "Settings.h"

#include <assert.h>
#include <qwt_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <set>

#define PI	3.14159

PfPvPlot::PfPvPlot()
    : cp_ (300),
      cad_ (85),
      cl_ (0.175)
{
    setCanvasBackground(Qt::white);

    setAxisTitle(yLeft, "Average Effective Pedal Force (N)");
    setAxisScale(yLeft, 0, 600);
    setAxisTitle(xBottom, "Circumferential Pedal Velocity (m/s)");
    setAxisScale(xBottom, 0, 3);

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
    curve->setPen(QPen(Qt::red));
    curve->setStyle(QwtPlotCurve::Dots);
    curve->attach(this);

    recalc();
}

void
PfPvPlot::setData(RideFile *ride)
{
    setTitle(ride->startTime().toString(GC_DATETIME_FORMAT));

    // due to the discrete power and cadence values returned by the
    // power meter, there will very likely be many duplicate values.
    // Rather than pass them all to the curve, use a set to strip
    // out duplicates.
    std::set<std::pair<double, double> > dataSet;
    QListIterator<RideFilePoint*> i(ride->dataPoints());
    while (i.hasNext()) {
	const RideFilePoint *p1 = i.next();

	if (p1->watts != 0 && p1->cad != 0) {
	    double aepf = (p1->watts * 60.0) / (p1->cad * cl_ * 2.0 * PI);
	    double cpv = (p1->cad * cl_ * 2.0 * PI) / 60.0;

	    dataSet.insert(std::make_pair<double, double>(aepf, cpv));
	}
    }

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

    curve->setData(cpvArray, aepfArray);
    replot();
}

void
PfPvPlot::recalc()
{
    double cpv = (cad_ * cl_ * 2.0 * PI) / 60.0;
    mX->setXValue(cpv);
    
    double aepf = (cp_ * 60.0) / (cad_ * cl_ * 2.0 * PI);
    mY->setYValue(aepf);
    
    QwtArray<double> aepfArray;
    QwtArray<double> cpvArray;
    for (double cpv = 0.1; cpv <= 3.0; cpv += 0.05) {
	double aepf = cp_ / cpv;

	aepfArray.push_back(aepf);
	cpvArray.push_back(cpv);
    }
    
    cpCurve->setData(cpvArray, aepfArray);
    
    replot();
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
}
