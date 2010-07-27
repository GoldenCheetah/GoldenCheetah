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
class IntervalPlotData;
class MainWindow;
class LTMToolTip;
class LTMCanvasPicker;

class AllPlot : public QwtPlot
{
    Q_OBJECT

    public:

        AllPlot(AllPlotWindow *parent, MainWindow *mainWindow);

        // set the curve data e.g. when a ride is selected
        void setDataFromRide(RideItem *_rideItem);
        void setDataFromPlot(AllPlot *plot, int startidx, int stopidx);

        // convert from time/distance to index in *smoothed* datapoints
        int timeIndex(double) const;
        int distanceIndex(double) const;

        // plot redraw functions
        bool shadeZones() const;
        void refreshZoneLabels();
        void refreshIntervalMarkers();

        // refresh data / plot parameters
        void recalc();
        void setYMax();
        void setXTitle();

    public slots:

        void showPower(int state);
        void showHr(int state);
        void showSpeed(int state);
        void showCad(int state);
        void showAlt(int state);
        void showGrid(int state);
        void setShadeZones(bool x) { shade_zones=x; }
        void setSmoothing(int value);
        void setByDistance(int value);
        void configChanged();
        void pointHover(QwtPlotCurve*, int);

    protected:

        friend class ::AllPlotBackground;
        friend class ::AllPlotZoneLabel;
        friend class ::AllPlotWindow;
        friend class ::IntervalPlotData;

        // cached state
        RideItem *rideItem;
        AllPlotBackground *bg;
        QSettings *settings;
        QVariant unit;
        bool useMetricUnits;

        // controls
        bool shade_zones;
        int showPowerState;
        int showHrState;
        int showSpeedState;
        int showCadState;
        int showAltState;

        // plot objects
        QwtPlotGrid *grid;
        QVector<QwtPlotMarker*> d_mrk;
        QwtPlotMarker *allMarker1;
        QwtPlotMarker *allMarker2;
        QwtPlotCurve *wattsCurve;
        QwtPlotCurve *hrCurve;
        QwtPlotCurve *speedCurve;
        QwtPlotCurve *cadCurve;
        QwtPlotCurve *altCurve;
        QwtPlotCurve *intervalHighlighterCurve;  // highlight selected intervals on the Plot
        QList <AllPlotZoneLabel *> zoneLabels;

        // source data
        QVector<double> hrArray;
        QVector<double> wattsArray;
        QVector<double> speedArray;
        QVector<double> cadArray;
        QVector<double> timeArray;
        QVector<double> distanceArray;
        QVector<double> altArray;

        // smoothed data
        QVector<double> smoothWatts;
        QVector<double> smoothHr;
        QVector<double> smoothSpeed;
        QVector<double> smoothCad;
        QVector<double> smoothTime;
        QVector<double> smoothDistance;
        QVector<double> smoothAltitude;

        // array / smooth state
        int arrayLength;
        int smooth;
        bool bydist;

    private:
        AllPlot *referencePlot;
        AllPlotWindow *parent;
        LTMToolTip *tooltip;
        LTMCanvasPicker *_canvasPicker; // allow point selection/hover
};

#endif // _GC_AllPlot_h

