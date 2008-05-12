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

#ifndef _GC_AllPlot_h
#define _GC_AllPlot_h 1

#include <qwt_plot.h>

class QwtPlotCurve;
class QwtPlotGrid;
class QwtPlotMarker;
class RideFile;

class AllPlot : public QwtPlot
{
    Q_OBJECT

    public:

        QwtPlotCurve *wattsCurve;
        QwtPlotCurve *hrCurve;
        QwtPlotCurve *speedCurve;
        QwtPlotCurve *cadCurve;
        QwtPlotMarker *d_mrk;

        AllPlot();

        int smoothing() const { return smooth; }

        void setData(RideFile *ride);

    public slots:

        void showPower(int state);
        void showHr(int state);
        void showSpeed(int state);
        void showCad(int state);
        void showGrid(int state);
        void setSmoothing(int value);

    protected:

        QwtPlotGrid *grid;

        double *hrArray;
        double *wattsArray;
        double *speedArray;
        double *cadArray;
        double *timeArray;
        int arrayLength;
        int *interArray;
        

        int smooth;

        void recalc();
        void setYMax();
};

#endif // _GC_AllPlot_h

