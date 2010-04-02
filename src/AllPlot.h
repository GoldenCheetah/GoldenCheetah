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
#include <qwt_data.h>
#include <QtGui>

class QwtPlotCurve;
class QwtPlotGrid;
class QwtPlotMarker;
class RideItem;
class AllPlotBackground;
class AllPlotZoneLabel;
class AllPlotWindow;
class AllPlot;
class IntervalItem;
class MainWindow;

class AllPlot : public QwtPlot
{
    Q_OBJECT

    public:

        AllPlot(QWidget *parent, MainWindow *mainWindow);

        int smoothing() const { return smooth; }

        bool byDistance() const { return bydist; }

	bool useMetricUnits;  // whether metric units are used (or imperial)

	bool shadeZones() const;
	void refreshZoneLabels();
	void refreshIntervalMarkers();

        void setDataI(RideItem *_rideItem);
        void setDataP(AllPlot *plot, int startidx, int stopidx);

        int timeIndex(double) const;          // get index offset for time in secs
        int distanceIndex(double) const;      // get index offset for distance in KM

        QwtPlotMarker *allMarker1;
        QwtPlotMarker *allMarker2;
        QwtPlotMarker *allMarker3;

    public slots:

        void showPower(int state);
        void showHr(int state);
        void showSpeed(int state);
        void showCad(int state);
        void showAlt(int state);
        void showGrid(int state);
        void setSmoothing(int value);
        void setByDistance(int value);
        void configChanged();

    protected:

        friend class ::AllPlotBackground;
        friend class ::AllPlotZoneLabel;
        friend class ::AllPlotWindow;

	AllPlotBackground *bg;
        QSettings *settings;
        QVariant unit;

        QwtPlotCurve *wattsCurve;
        QwtPlotCurve *hrCurve;
        QwtPlotCurve *speedCurve;
        QwtPlotCurve *cadCurve;
        QwtPlotCurve *altCurve;
        QwtPlotCurve *intervalHighlighterCurve;  // highlight selected intervals on the Plot
        QVector<QwtPlotMarker*> d_mrk;
	QList <AllPlotZoneLabel *> zoneLabels;

	RideItem *rideItem;

        QwtPlotGrid *grid;

        QVector<double> hrArray;
        QVector<double> wattsArray;
        QVector<double> speedArray;
        QVector<double> cadArray;
        QVector<double> timeArray;
        QVector<double> distanceArray;
        QVector<double> altArray;

        QVector<double> smoothWatts;
        QVector<double> smoothHr;
        QVector<double> smoothSpeed;
        QVector<double> smoothCad;
        QVector<double> smoothTime;
        QVector<double> smoothDistance;
        QVector<double> smoothAltitude;

        int arrayLength;

        int smooth;

        bool bydist;

        void recalc();
        void setYMax();
        void setXTitle();

	bool shade_zones;     // whether power should be shaded

        int showPowerState;
        int showHrState;
        int showSpeedState;
        int showCadState;
        int showAltState;

    private:
        AllPlot *referencePlot;
};

#endif // _GC_AllPlot_h

