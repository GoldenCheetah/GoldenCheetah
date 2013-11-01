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
#include "GoldenCheetah.h"

#include <qwt_plot.h>
#include <qwt_series_data.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_marker.h>
#include <qwt_point_3d.h>
#include <qwt_compat.h>
#include <QtGui>

class QwtPlotCurve;
class QwtPlotIntervalCurve;
class QwtPlotGrid;
class QwtPlotMarker;
class RideItem;
class AllPlotBackground;
class AllPlotZoneLabel;
class AllPlotWindow;
class AllPlot;
class IntervalItem;
class IntervalPlotData;
class Context;
class LTMToolTip;
class LTMCanvasPicker;

class AllPlot : public QwtPlot
{
    Q_OBJECT
    G_OBJECT


    public:

        AllPlot(AllPlotWindow *parent, Context *context);

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
        void refreshCalibrationMarkers();
        void refreshReferenceLines();
        void setAxisTitle(int axis, QString label);

        // refresh data / plot parameters
        void recalc();
        void setYMax();
        void setXTitle();

    public slots:

        void setShowPower(int id);
        void setShowHr(bool show);
        void setShowSpeed(bool show);
        void setShowCad(bool show);
        void setShowAlt(bool show);
        void setShowTemp(bool show);
        void setShowWind(bool show);
        void setShowW(bool show);
        void setShowTorque(bool show);
        void setShowBalance(bool show);
        void setShowGrid(bool show);
        void setPaintBrush(int state);
        void setShadeZones(bool x) { shade_zones=x; }
        void setSmoothing(int value);
        void setByDistance(int value);
        void configChanged();

        // for tooltip
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

        // controls
        bool shade_zones;
        int showPowerState;
        bool showHr;
        bool showSpeed;
        bool showCad;
        bool showAlt;
        bool showTemp;
        bool showWind;
        bool showW;
        bool showTorque;
        bool showBalance;

        // plot objects
        QwtPlotGrid *grid;
        QVector<QwtPlotMarker*> d_mrk;
        QVector<QwtPlotMarker*> cal_mrk;
        QwtPlotMarker curveTitle;
        QwtPlotMarker *allMarker1;
        QwtPlotMarker *allMarker2;
        QwtPlotCurve *wattsCurve;
        QwtPlotCurve *hrCurve;
        QwtPlotCurve *speedCurve;
        QwtPlotCurve *cadCurve;
        QwtPlotCurve *altCurve;
        QwtPlotCurve *tempCurve;
        QwtPlotIntervalCurve *windCurve;
        QwtPlotCurve *torqueCurve;
        QwtPlotCurve *balanceLCurve;
        QwtPlotCurve *balanceRCurve;
        QwtPlotCurve *wCurve;
        QwtPlotCurve *intervalHighlighterCurve;  // highlight selected intervals on the Plot
        QList <AllPlotZoneLabel *> zoneLabels;
        QVector<QwtPlotCurve*> referenceLines;

        // source data
        QVector<double> hrArray;
        QVector<double> wattsArray;
        QVector<double> speedArray;
        QVector<double> cadArray;
        QVector<double> timeArray;
        QVector<double> distanceArray;
        QVector<double> altArray;
        QVector<double> tempArray;
        QVector<double> windArray;
        QVector<double> torqueArray;
        QVector<double> balanceArray;

        // smoothed data
        QVector<double> smoothWatts;
        QVector<double> smoothHr;
        QVector<double> smoothSpeed;
        QVector<double> smoothCad;
        QVector<double> smoothTime;
        QVector<double> smoothDistance;
        QVector<double> smoothAltitude;
        QVector<double> smoothTemp;
        QVector<double> smoothWind;
        QVector<double> smoothTorque;
        QVector<double> smoothBalanceL;
        QVector<double> smoothBalanceR;
        QVector<QwtIntervalSample> smoothRelSpeed;

        // array / smooth state
        int arrayLength;
        int smooth;
        bool bydist;

    private:
        Context *context;

        AllPlot *referencePlot;
        AllPlotWindow *parent;
        LTMToolTip *tooltip;
        LTMCanvasPicker *_canvasPicker; // allow point selection/hover

        static void nextStep( int& step );
};

#endif // _GC_AllPlot_h

