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
#include "Colors.h"

#include <qwt_plot.h>
#include <qwt_axis_id.h>
#include <qwt_series_data.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_dict.h>
#include <qwt_plot_marker.h>
#include <qwt_point_3d.h>
#include <qwt_compat.h>
#include <QtGui>

#include <QTableWidget>
#include <QStackedWidget>
#include <QTextEdit>

#include "RideFile.h"

class QwtPlotCurve;
class QwtPlotIntervalCurve;
class QwtPlotGrid;
class QwtPlotMarker;
class RideItem;
class AllPlotBackground;
class AllPlotZoneLabel;
class AllPlotWindow;
class AllPlot;
struct RideFilePoint;
class IntervalItem;
class IntervalPlotData;
class Context;
class LTMToolTip;
class LTMCanvasPicker;
class QwtAxisId;

class CurveColors
{
    public:
        CurveColors(QwtPlot *plot) : plot(plot) {
            saveState();
        }

        void restoreState() {

            // make all the curves have the right pen
            QHashIterator<QwtPlotCurve *, bool> c(state);
            while (c.hasNext()) {
                c.next();
                c.key()->setVisible(c.value());
            }
        }

        void saveState() {
            state.clear();

            // get a list of plots and colors
            foreach(QwtPlotItem *item, plot->itemList(QwtPlotItem::Rtti_PlotCurve)) {
                state.insert(static_cast<QwtPlotCurve*>(item), 
                             static_cast<QwtPlotCurve*>(item)->isVisible());
            }
        }

        void isolate(QwtPlotCurve *curve) {

            // make the curve colored but all others go dull
            QHashIterator<QwtPlotCurve *, bool> c(state);
            while (c.hasNext()) {
                c.next();
                if (c.key() == curve) {
                    c.key()->setVisible(c.value());
                } else {

                    // hide others
                    c.key()->setVisible(false);
                }
            }
        }

        void isolateAxis(QwtAxisId id) {
            QHashIterator<QwtPlotCurve *, bool> c(state);
            while (c.hasNext()) {
                c.next();

                // isolate on axis hover
                if (c.key()->yAxis() == id) {
                    c.key()->setVisible(c.value());
                } else {
                    // hide others
                    c.key()->setVisible(false);
                }
            }
        }

    private:
        QwtPlot *plot;
        QHash<QwtPlotCurve *, bool> state;
};

class AllPlot;
class AllPlotObject : public QObject
{
    Q_OBJECT;

    // one set for every ride being plotted, which
    // as standard is just one, its only when we start
    // compare mode that we get more...

    public:

    AllPlotObject(AllPlot*); // construct associate with a plot
    ~AllPlotObject(); // delete and disassociate from a plot

    void setVisible(bool); // show or hide objects
    void setColor(QColor color); // set ALL curves the same color
    void hideUnwanted(); // hide curves we are not interested in
                         // using setVisible ...

    QwtPlotGrid *grid;
    QVector<QwtPlotMarker*> d_mrk;
    QVector<QwtPlotMarker*> cal_mrk;
    QwtPlotMarker curveTitle;
    QwtPlotMarker *allMarker1;
    QwtPlotMarker *allMarker2;
    // reference lines
    QVector<QwtPlotCurve*> referenceLines;
    QVector<QwtPlotCurve*> tmpReferenceLines;

    QwtPlotCurve *wattsCurve;
    QwtPlotCurve *npCurve;
    QwtPlotCurve *xpCurve;
    QwtPlotCurve *apCurve;
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
    QwtPlotCurve *mCurve;

    // source data
    QVector<double> hrArray;
    QVector<double> wattsArray;
    QVector<double> npArray;
    QVector<double> xpArray;
    QVector<double> apArray;
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
    QVector<double> smoothNP;
    QVector<double> smoothAP;
    QVector<double> smoothXP;
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

    // highlighting intervals
    QwtPlotCurve *intervalHighlighterCurve;  // highlight selected intervals on the Plot

    // the plot we work for
    AllPlot *plot;

    // some handy stuff
    double maxSECS, maxKM;
};

class AllPlot : public QwtPlot
{
    Q_OBJECT
    G_OBJECT


    public:

        // you can declare which series to plot, none means do them all
        // wanttext is to say if plot markers should have text
        AllPlot(AllPlotWindow *parent, Context *context, 
                RideFile::SeriesType series = RideFile::none, RideFile::SeriesType secSeries = RideFile::none, bool wanttext = true);
        ~AllPlot();

        bool eventFilter(QObject *object, QEvent *e);

        // set the curve data e.g. when a ride is selected
        void setDataFromRide(RideItem *_rideItem);
        void setDataFromRideFile(RideFile *ride, AllPlotObject *object); // when plotting lots of rides on fullPlot
        void setDataFromPlot(AllPlot *plot, int startidx, int stopidx);
        void setDataFromPlot(AllPlot *plot); // used for single series plotting
        void setDataFromPlots(QList<AllPlot*>); // user for single series comparing
        void setDataFromObject(AllPlotObject *object, AllPlot *reference); // for allplot when one per ride in a stack
                                                                           // reference is for settings et al

        // convert from time/distance to index in *smoothed* datapoints
        int timeIndex(double) const;
        int distanceIndex(double) const;

        // plot redraw functions
        bool shadeZones() const;
        void refreshZoneLabels();
        void refreshIntervalMarkers();
        void refreshCalibrationMarkers();
        void refreshReferenceLines();
        void refreshReferenceLinesForAllPlots();
        void setAxisTitle(QwtAxisId axis, QString label);

        // refresh data / plot parameters
        void recalc(AllPlotObject *objects);
        void setYMax();
        void setXTitle();
        void setHighlightIntervals(bool);

        void plotTmpReference(int axis, int x, int y);
        void confirmTmpReference(double value, int axis, bool allowDelete);
        QwtPlotCurve* plotReferenceLine(const RideFilePoint *referencePoint);

        // remembering state etc
        CurveColors *curveColors;

    public slots:

        void setShowPower(int id);
        void setShowNP(bool show);
        void setShowXP(bool show);
        void setShowAP(bool show);
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
        void setWantAxis(bool x) { wantaxis = x;}
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
        bool showNP;
        bool showXP;
        bool showAP;
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
        AllPlotObject *standard;
        QList<QwtPlotCurve*> compares; // when plotting single series in compare mode

        QList <AllPlotZoneLabel *> zoneLabels;

        // array / smooth state
        int smooth;
        bool bydist;

        // scope of plot (none means all, or just for a specific series
        RideFile::SeriesType scope;
        RideFile::SeriesType secondaryScope;

    protected:
        friend class ::AllPlotObject;
        Context *context;

    private:

        AllPlot *referencePlot;
        AllPlotWindow *parent;
        bool wanttext, wantaxis;
        bool isolation;
        LTMToolTip *tooltip;
        LTMCanvasPicker *_canvasPicker; // allow point selection/hover

        static void nextStep( int& step );
};

#endif // _GC_AllPlot_h

