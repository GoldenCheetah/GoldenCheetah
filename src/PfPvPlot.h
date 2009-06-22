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
class PfPvPlotZoneLabel;

class PfPvPlot : public QwtPlot
{
    Q_OBJECT

    public:

        PfPvPlot();
	void refreshZoneItems();
        void setData(RideItem *_rideItem);

	int getCP();
	void setCP(int cp);
	int getCAD();
	void setCAD(int cadence);
	double getCL();
	void setCL(double cranklen);
	void recalc();

	RideItem *rideItem;

        bool shadeZones() const { return shade_zones; }

    public slots:
    
    signals:
    
        void changedCP( const QString& );
        void changedCAD( const QString& );
        void changedCL( const QString& );

        void setShadeZones(bool value);

    protected:
        QwtPlotCurve *curve;
	QwtPlotCurve *cpCurve;
	QList <QwtPlotCurve *> zoneCurves;
	QList <PfPvPlotZoneLabel *> zoneLabels;
	QwtPlotMarker *mX;
	QwtPlotMarker *mY;

	static QwtArray<double> contour_xvalues;   // values used in CP and contour plots: djconnel
	
	int cp_;
	int cad_;
	double cl_;
	bool shade_zones;    // whether to shade zones, added 27Apr2009 djconnel
};

#endif // _GC_QaPlot_h

