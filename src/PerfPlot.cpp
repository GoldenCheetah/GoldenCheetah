/*
 * Copyright (c) 2009 Eric Murray (ericm@lne.com)
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


#include <assert.h>
#include <QDebug>
#include <qwt_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include "RideItem.h"
#include "RideFile.h"
#include "PerfPlot.h"
#include "StressCalculator.h"
#include "Colors.h"

PerfPlot::PerfPlot() : STScurve(NULL), LTScurve(NULL), SBcurve(NULL), DAYcurve(NULL)
{
    setInstanceName("PM Plot");

    insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setTitle(tr("Performance Manager"));
    setAxisTitle(yLeft, "Exponentially Weighted Average Stress");
    setAxisTitle(xBottom, "Time (days)");
    setAxisTitle(yRight, "Daily Stress");
    enableAxis(yRight, true);

    grid = new QwtPlotGrid();
    grid->attach(this);

    configUpdate();
}

void
PerfPlot::configUpdate()
{
    // set the colors et al
    setCanvasBackground(GColor(CPLOTBACKGROUND));
    grid->enableX(false);
    QPen gridPen(GColor(CPLOTGRID));
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);
}

void PerfPlot::setStressCalculator(StressCalculator *sc) {
    _sc = sc;
    days = _sc->n();
    startDate = _sc->getStartDate();
    xmin =  0;
    xmax = _sc->n();
}

void PerfPlot::plot() {

    int  num, tics;
    tics = 42;

    setAxisScale(yLeft, _sc->min(), _sc->max());
    num = xmax - xmin;

    /*
       fprintf(stderr,"PerfPlot::plot: xmin = %d xmax = %d num = %d\n",
       xmin, xmax, num);
       */

    // set axis scale
    if (num < 15) {
	tics = 1;
    } else if (num < 71) {
	tics  = 7;
    } else if (num < 141) {
	tics  = 14;
    } else if (num < 364) {
	tics  = 27;
    }
    setAxisScale(xBottom, xmin, xmax,tics);

    setAxisScaleDraw(QwtPlot::xBottom, new TimeScaleDraw(startDate));

    double width = appsettings->value(this, GC_LINEWIDTH, 20.0).toDouble();

    if (DAYcurve) {
	DAYcurve->detach();
	delete DAYcurve;
    }
    DAYcurve = new QwtPlotCurve(tr("Daily"));
    if (appsettings->value(this, GC_ANTIALIAS, false).toBool()==true)
        DAYcurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen daypen = QPen(GColor(CDAILYSTRESS));
    daypen.setWidth(width);
    DAYcurve->setPen(daypen);
    DAYcurve->setStyle(QwtPlotCurve::Sticks);

    // XXX Note: the days are offset by 1 without the -1 in the
    //           line below  --------------+
    //                                     |
    //                                     V
    DAYcurve->setData(_sc->getDays()+xmin -1 ,_sc->getDAYvalues()+xmin,num);
    DAYcurve->setYAxis(yRight);
    DAYcurve->attach(this);

    if (STScurve) {
	STScurve->detach();
	delete STScurve;
    }
    STScurve = new QwtPlotCurve(appsettings->value(this, GC_STS_NAME,tr("Short Term Stress")).toString());
    if (appsettings->value(this, GC_ANTIALIAS, false).toBool()==true)
        STScurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen stspen = QPen(GColor(CSTS));
    stspen.setWidth(width);
    STScurve->setPen(stspen);
    STScurve->setData(_sc->getDays()+xmin,_sc->getSTSvalues()+xmin,num);
    STScurve->attach(this);


    if (LTScurve) {
	LTScurve->detach();
	delete LTScurve;
    }
    LTScurve = new QwtPlotCurve(appsettings->value(this, GC_LTS_NAME,tr("Long Term Stress")).toString());
    if (appsettings->value(this, GC_ANTIALIAS, false).toBool()==true)
        LTScurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen ltspen = QPen(GColor(CLTS));
    ltspen.setWidth(width);
    LTScurve->setPen(ltspen);
    LTScurve->setData(_sc->getDays()+xmin,_sc->getLTSvalues()+xmin,num);
    LTScurve->attach(this);


    if (SBcurve) {
	SBcurve->detach();
	delete SBcurve;
    }
    SBcurve = new QwtPlotCurve(appsettings->value(this, GC_SB_NAME,tr("Stress Balance")).toString());
    if (appsettings->value(this, GC_ANTIALIAS, false).toBool()==true)
        SBcurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen sbpen = QPen(GColor(CSB));
    sbpen.setWidth(width);
    SBcurve->setPen(sbpen);
    SBcurve->setData(_sc->getDays()+xmin,_sc->getSBvalues()+xmin,num);
    SBcurve->attach(this);

    replot();

}


void PerfPlot::resize(int newmin, int newmax)
{
    if (newmin < 0)
        newmin = 0;
    if (newmax >= _sc->n())
        newmax = _sc->n();

    if (newmin >= 0 && newmin <= _sc->n() && newmin < xmax)
	xmin = newmin;
    if (newmax >= 0 && newmax <= _sc->n() && newmax > xmin)
	xmax = newmax;

    plot();

}



