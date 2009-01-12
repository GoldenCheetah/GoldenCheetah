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

#ifndef _GC_QaPlot_h
#define _GC_QaPlot_h 1

#include <qwt_plot.h>

// forward references
class RideFile;
class RideItem;
class QwtPlotCurve;
class QwtPlotMarker;

class PfPvPlot : public QwtPlot
{
    Q_OBJECT

    public:

        PfPvPlot();
        void setData(RideItem *rideItem);

	int getCP();
	void setCP(int cp);
	int getCAD();
	void setCAD(int cadence);
	double getCL();
	void setCL(double cranklen);
	
    public slots:

    protected:
	void recalc();
	
        QwtPlotCurve *curve;
	QwtPlotCurve *cpCurve;
	QwtPlotMarker *mX;
	QwtPlotMarker *mY;
	
	int cp_;
	int cad_;
	double cl_;
};

#endif // _GC_QaPlot_h

