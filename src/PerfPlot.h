/*
 * Copyright (c) 2009 Eric Murray  (ericm@lne.com)
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

#ifndef _GC_PerfPlot_h
#define _GC_PerfPlot_h 1

#include "StressCalculator.h"
#include <QtGui>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_draw.h>
#include "Settings.h"




// handle x-axis names
class TimeScaleDraw: public QwtScaleDraw
{
    public:
	TimeScaleDraw(const QDateTime &base):
	    baseTime(base) {
		format = "MMM d";
	    }
	virtual QwtText label(double v) const
	{
	    QDateTime upTime = baseTime.addDays((int)v);
	    return upTime.toString(format);
	}
    private:
	QDateTime baseTime;
	QString format;

};


class PerfPlot : public QwtPlot
{
    Q_OBJECT

    private:
	QwtPlotGrid *grid;
	QwtPlotCurve *STScurve, *LTScurve, *SBcurve, *DAYcurve;
	int days;
	QDateTime startDate;
	StressCalculator *_sc;
	int xmin, xmax;

    public slots:
    void configUpdate();

    public:

    double getSTS(int i) { return STScurve->y(i - xmin); }
    double getLTS(int i) { return LTScurve->y(i - xmin); }
    double getSB(int i) { return SBcurve->y(i - xmin); }
    double getDAY(int i) { return DAYcurve->y(i - xmin); }
    int n(void) { return days; }
    int max(void) { return xmax; }
    int min(void) { return xmin; }

    QDateTime getStartDate(void) { return startDate; }
    QDateTime getEndDate(void) { return startDate.addDays(days); }


    PerfPlot();

    void setStressCalculator(StressCalculator *sc);

    void plot();
    void resize(int newmin, int newmax);

};


#endif // _GC_PerfPlot_h

