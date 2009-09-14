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
#include <QtGui>
#include <qpainter.h>

class QPen;
class QwtPlotCurve;
class QwtPlotGrid;
class QwtPlotMarker;
class RideItem;
class RideFile;
class AllPlot;
class AllPlotBackground;
class AllPlotZoneLabel;

class AllPlot : public QwtPlot
{
    Q_OBJECT

    private:

	AllPlotBackground *bg;
        QSettings *settings;
        QVariant unit;

    public:

        QwtPlotCurve *wattsCurve;
        QwtPlotCurve *hrCurve;
        QwtPlotCurve *speedCurve;
        QwtPlotCurve *cadCurve;
        QwtPlotCurve *altCurve;
        QVector<QwtPlotMarker*> d_mrk;
	QList <AllPlotZoneLabel *> zoneLabels;

        AllPlot();

        int smoothing() const { return smooth; }

        bool byDistance() const { return bydist; }

	bool shadeZones() const;
	void refreshZoneLabels();

        void setData(RideItem *_rideItem);

	RideItem *rideItem;

    public slots:

        void showPower(int state);
        void showHr(int state);
        void showSpeed(int state);
        void showCad(int state);
        void showAlt(int state);
        void showGrid(int state);
        void setSmoothing(int value);
        void setByDistance(int value);

    protected:

        QwtPlotGrid *grid;

        double *hrArray;
        double *wattsArray;
        double *speedArray;
        double *cadArray;
        double *timeArray;
        double *distanceArray;
        double *altArray;
        int arrayLength;
        int *interArray;

        int smooth;

        bool bydist;

        void recalc();
        void setYMax();
        void setXTitle();

	bool shade_zones;     // whether power should be shaded
	bool useMetricUnits;  // whether metric units are used (or imperial)
};

#endif // _GC_AllPlot_h

