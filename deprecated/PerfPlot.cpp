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

#include "PerfPlot.h"

// handle x-axis names
class PPTimeScaleDraw: public QwtScaleDraw
{
    public:
	PPTimeScaleDraw(const QDateTime &base):
	    QwtScaleDraw(), baseTime(base) {
		format = "MMM d";
        QwtScaleDraw::invalidateCache();
	    }
	virtual QwtText label(double v) const
	{
	    QDateTime upTime = baseTime.addDays((int)v);
	    return upTime.toString(format);
	}
    void setBase(QDateTime base)
    {
        baseTime = base;
        invalidateCache();
    }

    private:
	QDateTime baseTime;
	QString format;

};

PerfPlot::PerfPlot() : STScurve(NULL), LTScurve(NULL), SBcurve(NULL), DAYcurve(NULL)
{
    xsd = new PPTimeScaleDraw(QDateTime());
    xsd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtAxis::XBottom, xsd);

    insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setAxisTitle(YLeft, tr("Exponentially Weighted Average Stress"));
    setAxisTitle(XBottom, tr("Time (days)"));
    setAxisTitle(YRight, tr("Daily Stress"));
    enableAxis(YRight, true);
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

    setAxisMaxMinor(XBottom, 0);
    setAxisMaxMinor(YLeft, 0);
    setAxisMaxMinor(YRight, 0);

    QwtScaleDraw *sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtAxis::YLeft, sd);

    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtAxis::YRight, sd);

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

    setAxisScale(YLeft, _sc->min(), _sc->max());
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
    setAxisScale(XBottom, xmin, xmax,tics);
    axisWidget(XBottom)->update();

    // set base
    xsd->setBase(startDate);

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

    // Note: the days are offset by 1 without the -1 in the
    //       line below  ------------------+
    //                                     |
    //                                     V
    DAYcurve->setSamples(_sc->getDays()+xmin -1 ,_sc->getDAYvalues()+xmin,num);
    DAYcurve->setYAxis(YRight);
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
    STScurve->setSamples(_sc->getDays()+xmin,_sc->getSTSvalues()+xmin,num);
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
    LTScurve->setSamples(_sc->getDays()+xmin,_sc->getLTSvalues()+xmin,num);
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
    SBcurve->setSamples(_sc->getDays()+xmin,_sc->getSBvalues()+xmin,num);
    SBcurve->attach(this);

    axisWidget(QwtAxis::XBottom)->update();
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

void
PerfPlot::setAxisTitle(int axis, QString label)
{
    // setup the default fonts
    QFont stGiles; // hoho - Chart Font St. Giles ... ok you have to be British to get this joke
    stGiles.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    stGiles.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());

    QwtText title(label);
    title.setFont(stGiles);
    QwtPlot::setAxisFont(axis, stGiles);
    QwtPlot::setAxisTitle(axis, title);
    axisWidget(axis)->update();
}
