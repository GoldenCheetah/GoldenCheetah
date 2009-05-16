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

#ifndef _GC_PowerHist_h
#define _GC_PowerHist_h 1

#include <qwt_plot.h>

class QwtPlotCurve;
class QwtPlotGrid;
class RideFile;

class PowerHist : public QwtPlot
{
    Q_OBJECT

    public:

        QwtPlotCurve *curve;

        PowerHist();
        ~PowerHist();

        int binWidth() const { return binw; }

        inline bool withZeros() const { return withz; }
        inline bool islnY() const { return lny; }

        void setData(RideFile *ride);

    public slots:

        void setlnY(bool value);
        void setWithZeros(bool value);

        void setBinWidth(int value);

    protected:

        QwtPlotGrid *grid;

        double *array;
        int arrayLength;

        bool withz;   // whether P=0 are omitted from hisogram
	bool lny;     // whether y-axis is a log scale

        int binw;

        void recalc();
        void setYMax();
};

#endif // _GC_PowerHist_h

