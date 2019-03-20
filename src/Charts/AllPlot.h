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
#include "AllPlotSlopeCurve.h"

#include <qwt_plot.h>
#include <qwt_axis_id.h>
#include <qwt_series_data.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_dict.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_intervalcurve.h>
#include <qwt_point_3d.h>
#include <qwt_scale_widget.h>
#include <qwt_compat.h>
#include <QtGui>
#include <QFont>

#include <QTableWidget>
#include <QStackedWidget>
#include <QTextEdit>

// span slider specials
#include <qxtspanslider.h>
#include <QStyleFactory>
#include <QStyle>

#include "UserData.h"
#include "RideFile.h"

class QwtPlotCurve;
class QwtPlotGappedCurve;
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

class CurveColors : public QObject
{
    Q_OBJECT;

    public:
        CurveColors(QwtPlot *plot, bool allplot=false) : isolated(false), allplot(allplot), plot(plot) {

            // span slider appears when curve isolated
            // to enable zooming in and out
            slider = new QxtSpanSlider(Qt::Vertical, plot);
            slider->setFocusPolicy(Qt::NoFocus);
            slider->setHandleMovementMode(QxtSpanSlider::NoOverlapping);
            slider->setMinimum(0);
            slider->setMaximum(100); // %age
            slider->setShowRail(false); // no rail please

#ifdef Q_OS_MAC
            // BUG in QMacStyle and painting of spanSlider
            // so we use a plain style to avoid it, but only
            // on a MAC, since win and linux are fine
#if QT_VERSION > 0x5000
            QStyle *style = QStyleFactory::create("fusion");
#else
            QStyle *style = QStyleFactory::create("Cleanlooks");
#endif
            slider->setStyle(style);
#endif
            slider->hide();
            connect(slider, SIGNAL(lowerPositionChanged(int)), this, SLOT(zoomChanged()));
            connect(slider, SIGNAL(upperPositionChanged(int)), this, SLOT(zoomChanged()));

            saveState();
        }

        ~CurveColors() {
            delete slider;
        }

        void restoreState() {

            // if a curve is hidden, don't show its labels we hack this
            // by hiding any labels with the same color, a bit of a
            // hack but works largely and avoids having to integrate
            // with the plot object to understand the relationship between
            // labels and curves (which is only really known by LTMPlot)
            QList<QColor> shown;

            // make all the curves visible (that should be)
            // and remember the colors so we can show/hide 
            // the associated labels
            QHashIterator<QwtPlotSeriesItem *, bool> c(state);
            while (c.hasNext()) {
                c.next();
                c.key()->setVisible(c.value());

                if (c.value()) {
                    if (c.key()->rtti() == QwtPlotItem::Rtti_PlotIntervalCurve) {
                        shown << static_cast<QwtPlotIntervalCurve*>(c.key())->pen().color();
                    } else if (c.key()->rtti() == QwtPlotItem::Rtti_PlotCurve) {
                        shown << static_cast<QwtPlotCurve*>(c.key())->pen().color();
                    }
                }
            }

            // show all labels
            QHashIterator<QwtPlotMarker *, QwtScaleWidget*> l(labels);
            while (l.hasNext()) {
                l.next();

                // we cannot hide via legend on allplot so always hide/show labels
                l.key()->setVisible(allplot || shown.contains(l.key()->label().color()));
            }

            // make all the axes have the right color
            QHashIterator<QwtScaleWidget *, QPalette> s(colors);
            while (s.hasNext()) {
                s.next();
                s.key()->setPalette(s.value());
            }

            slider->hide();
            isolated = false;
        }

        void saveState() {

            // don't save in this state!!
            if (isolated) restoreState();
            else slider->hide();

            state.clear();
            labels.clear();
            colors.clear();
            lims.clear();

            // Labels
            foreach(QwtPlotItem *item, plot->itemList(QwtPlotItem::Rtti_PlotMarker)) {

                // ignore event / interval markers / blanks
                if (static_cast<QwtPlotMarker*>(item)->lineStyle() == QwtPlotMarker::VLine ||
                    static_cast<QwtPlotMarker*>(item)->label().text() == "") continue;

                QwtScaleWidget *x = plot->axisWidget(static_cast<QwtPlotMarker*>(item)->yAxis());
                labels.insert(static_cast<QwtPlotMarker*>(item), x);
            }

            // curves
            foreach(QwtPlotItem *item, plot->itemList(QwtPlotItem::Rtti_PlotCurve)) {

                state.insert(static_cast<QwtPlotSeriesItem*>(item), 
                             static_cast<QwtPlotSeriesItem*>(item)->isVisible());

                QwtScaleWidget *x = plot->axisWidget(static_cast<QwtPlotSeriesItem*>(item)->yAxis());
                if (x) colors.insert(x, x->palette());

                QwtPlotCurve *curve = static_cast<QwtPlotCurve*>(item);

                QPointF lim;
                lim.setX(curve->minYValue());
                lim.setY(curve->maxYValue() * 1.1f);

                lims.insert(curve, lim);
            }

            // interval curves
            foreach(QwtPlotItem *item, plot->itemList(QwtPlotItem::Rtti_PlotIntervalCurve)) {

                state.insert(static_cast<QwtPlotSeriesItem*>(item), 
                             static_cast<QwtPlotSeriesItem*>(item)->isVisible());

                QwtScaleWidget *x = plot->axisWidget(static_cast<QwtPlotSeriesItem*>(item)->yAxis());
                if (x) colors.insert(x, x->palette());

            }
        }

        // remove curve if being zapped (e.g. reference line)
        void remove(QwtPlotSeriesItem *remove) {
            state.remove(remove);
        }

        void insert(QwtPlotSeriesItem *add) {
            state.insert(add, add->isVisible());
        }

        void isolate(QwtPlotSeriesItem *curve) {

            // make the curve colored but all others go dull
            QHashIterator<QwtPlotSeriesItem *, bool> c(state);
            while (c.hasNext()) {
                c.next();
                if (c.key() == curve) {
                    c.key()->setVisible(c.value());
                } else {

                    // hide others
                    c.key()->setVisible(false);
                }
            }

            isolated = true;
        }

        void isolateAxis(QwtAxisId id, bool showslider = false) {

            isolatedIDpos = id.pos;
            isolatedIDid = id.id;
            isolatedAxis = plot->axisWidget(id);
            isolatedDiv = plot->axisScaleDiv(id);

            // if a curve is hidden, don't show its labels we hack this
            // by hiding any labels with the same color, a bit of a
            // hack but works largely and avoids having to integrate
            // with the plot object to understand the relationship between
            // labels and curves (which is only really known by LTMPlot)
            QList<QColor> shown;

            // hide curves that are not ours
            QHashIterator<QwtPlotSeriesItem *, bool> c(state);
            while (c.hasNext()) {
                c.next();

                // isolate on axis hover (but leave huighlighters alone)
                if (c.key()->yAxis() == id || c.key()->yAxis() == QwtAxisId(QwtAxis::yLeft,2)) {

                    // show and remember color
                    c.key()->setVisible(c.value());
                    if (c.value()) {
                        if (c.key()->rtti() == QwtPlotItem::Rtti_PlotIntervalCurve) {
                            shown << static_cast<QwtPlotIntervalCurve*>(c.key())->pen().color();
                        } else if (c.key()->rtti() == QwtPlotItem::Rtti_PlotCurve) {
                            shown << static_cast<QwtPlotCurve*>(c.key())->pen().color();
                        }
                    }

                    if (showslider && c.key()->yAxis() == id) {

                        if (c.key()->yAxis().pos == QwtAxis::yLeft)
                            slider->move(plot->canvas()->pos().x(), 10);
                        else
                            slider->move(plot->canvas()->pos().x() +
                                         plot->canvas()->width() - slider->width(), 10);

                        slider->setLowerValue(0);
                        slider->setUpperValue(100);
                        slider->setFixedHeight(plot->canvas()->height()-20);
                        slider->show();
                    }

                } else {
                    // hide others
                    c.key()->setVisible(false);
                }
            }

            QwtScaleWidget *ours = plot->axisWidget(id);

            // dull axis that are not ours
            QHashIterator<QwtScaleWidget *, QPalette> s(colors);
            while (s.hasNext()) {
                s.next();

                // isolate on axis hover
                if (s.key() != ours) {
                    QPalette pal = s.value();
                    pal.setColor(QPalette::WindowText, QColor(Qt::gray));
                    pal.setColor(QPalette::Text, QColor(Qt::gray));
                    s.key()->setPalette(pal);
                }
            }

            // hide labels that are not ours
            QHashIterator<QwtPlotMarker *, QwtScaleWidget*> l(labels);
            while (l.hasNext()) {
                l.next();

                if (l.value() != ours) {
                    l.key()->setVisible(false);
                } else {
                    // we cannot hide via legend on allplot so always hide/show labels
                    l.key()->setVisible(allplot || shown.contains(l.key()->label().color()));
                }
            }

            isolated = true;
        }

        bool isolated, allplot;

    public slots:
        void zoomChanged() {
            // change ...
            if (!slider->isHidden()) {

                plot->setAxisScale(QwtAxisId(isolatedIDpos, isolatedIDid),
                isolatedDiv.upperBound() * slider->lowerValue() / 100.00f,
                isolatedDiv.upperBound() * slider->upperValue() / 100.00f);
                
                plot->replot();
            }
        }

    private:
        QxtSpanSlider *slider;
        QwtPlot *plot;
        QHash<QwtPlotSeriesItem *, bool> state;
        QHash<QwtPlotMarker *, QwtScaleWidget*> labels;
        QHash<QwtScaleWidget*, QPalette> colors;
        QHash<QwtPlotSeriesItem *, QPointF> lims;

        int isolatedIDpos, isolatedIDid;
        QwtScaleWidget *isolatedAxis;
        QwtScaleDiv isolatedDiv;

};

// Plotting user data
struct UserObject {
    QString name, units;
    QVector<double> array;
    QVector<double> smooth;
    QwtPlotGappedCurve    *curve;
    QVector<QwtZone> zones;
    QColor          color;
};

class AllPlot;
class MergeAdjust;
class AllPlotObject : public QObject
{
    Q_OBJECT;

    // one set for every ride being plotted, which
    // as standard is just one, its only when we start
    // compare mode that we get more...

    public:

    AllPlotObject(AllPlot*, QList<UserData*>); // construct associate with a plot
    ~AllPlotObject(); // delete and disassociate from a plot

    void setVisible(bool); // show or hide objects
    void setColor(QColor color); // set ALL curves the same color
    void hideUnwanted(); // hide curves we are not interested in
                         // using setVisible ...
    QVector<QwtZone> parseZoneString(QString zstring);

    QwtPlotGrid *grid;
    QVector<QwtPlotMarker*> d_mrk;
    QVector<QwtPlotMarker*> cal_mrk;
    QwtPlotMarker curveTitle;
    QwtPlotMarker *allMarker1;
    QwtPlotMarker *allMarker2;

    // reference lines
    QVector<QwtPlotCurve*> referenceLines;
    QVector<QwtPlotCurve*> tmpReferenceLines;

    // points of exhaustion
    QVector<QwtPlotMarker*> exhaustionLines;
    QVector<QwtPlotMarker*> tmpExhaustionLines;

    QwtPlotCurve *wattsCurve;
    QwtPlotCurve *slopeCurve;
    AllPlotSlopeCurve *altSlopeCurve;
    QwtPlotCurve *atissCurve;
    QwtPlotCurve *antissCurve;
    QwtPlotCurve *rvCurve;
    QwtPlotCurve *rcadCurve;
    QwtPlotCurve *rgctCurve;
    QwtPlotCurve *gearCurve;
    QwtPlotCurve *smo2Curve;
    QwtPlotCurve *thbCurve;
    QwtPlotCurve *o2hbCurve;
    QwtPlotCurve *hhbCurve;
    QwtPlotCurve *npCurve;
    QwtPlotCurve *xpCurve;
    QwtPlotCurve *apCurve;
    QwtPlotCurve *hrCurve;
    QwtPlotCurve *hrvCurve;
    QwtPlotCurve *tcoreCurve;
    QwtPlotCurve *speedCurve;
    QwtPlotCurve *accelCurve;
    QwtPlotCurve *wattsDCurve;
    QwtPlotCurve *cadDCurve;
    QwtPlotCurve *nmDCurve;
    QwtPlotCurve *hrDCurve;
    QwtPlotCurve *cadCurve;
    QwtPlotCurve *altCurve;
    QwtPlotCurve *tempCurve;
    QwtPlotIntervalCurve *windCurve;
    QwtPlotCurve *torqueCurve;
    QwtPlotCurve *balanceLCurve;
    QwtPlotCurve *balanceRCurve;
    QwtPlotCurve *wCurve;
    QwtPlotCurve *mCurve;
    QwtPlotCurve *lteCurve;
    QwtPlotCurve *rteCurve;
    QwtPlotCurve *lpsCurve;
    QwtPlotCurve *rpsCurve;
    QwtPlotCurve *lpcoCurve;
    QwtPlotCurve *rpcoCurve;
    QwtPlotIntervalCurve *lppCurve;
    QwtPlotIntervalCurve *rppCurve;
    QwtPlotIntervalCurve *lpppCurve;
    QwtPlotIntervalCurve *rpppCurve;

    // source data
    QVector<double> match;
    QVector<double> matchTime;
    QVector<double> matchDist;
    QVector<QwtPlotMarker*> matchLabels;
    QVector<double> wprime;
    QVector<double> wprimeTime;
    QVector<double> wprimeDist;

    QVector<double> hrArray;
  //    QVector<double> hrvArray;
    QVector<double> tcoreArray;
    QVector<double> wattsArray;
    QVector<double> atissArray;
    QVector<double> antissArray;
    QVector<double> rvArray;
    QVector<double> rcadArray;
    QVector<double> rgctArray;
    QVector<double> gearArray;
    QVector<double> smo2Array;
    QVector<double> thbArray;
    QVector<double> o2hbArray;
    QVector<double> hhbArray;
    QVector<double> npArray;
    QVector<double> xpArray;
    QVector<double> apArray;
    QVector<double> speedArray;
    QVector<double> accelArray;
    QVector<double> wattsDArray;
    QVector<double> cadDArray;
    QVector<double> nmDArray;
    QVector<double> hrDArray;
    QVector<double> cadArray;
    QVector<double> timeArray;
    QVector<double> distanceArray;
    QVector<double> altArray;
    QVector<double> slopeArray;
    QVector<double> tempArray;
    QVector<double> windArray;
    QVector<double> torqueArray;
    QVector<double> balanceArray;
    QVector<double> lteArray;
    QVector<double> rteArray;
    QVector<double> lpsArray;
    QVector<double> rpsArray;
    QVector<double> lpcoArray;
    QVector<double> rpcoArray;
    QVector<double> lppbArray;
    QVector<double> rppbArray;
    QVector<double> lppeArray;
    QVector<double> rppeArray;
    QVector<double> lpppbArray;
    QVector<double> rpppbArray;
    QVector<double> lpppeArray;
    QVector<double> rpppeArray;

    // smoothed data
    QVector<double> smoothWatts;
    QVector<double> smoothAT;
    QVector<double> smoothANT;
    QVector<double> smoothRV;
    QVector<double> smoothRCad;
    QVector<double> smoothRGCT;
    QVector<double> smoothGear;
    QVector<double> smoothSmO2;
    QVector<double> smoothtHb;
    QVector<double> smoothO2Hb;
    QVector<double> smoothHHb;
    QVector<double> smoothNP;
    QVector<double> smoothAP;
    QVector<double> smoothXP;
    QVector<double> smoothHr;
    QVector<double> smoothHrv;
    QVector<double> smoothHrv_time;
    QVector<double> smoothTcore;
    QVector<double> smoothSpeed;
    QVector<double> smoothAccel;
    QVector<double> smoothWattsD;
    QVector<double> smoothCadD;
    QVector<double> smoothNmD;
    QVector<double> smoothHrD;
    QVector<double> smoothCad;
    QVector<double> smoothTime;
    QVector<double> smoothDistance;
    QVector<double> smoothAltitude;
    QVector<double> smoothSlope;
    QVector<double> smoothTemp;
    QVector<double> smoothWind;
    QVector<double> smoothTorque;
    QVector<double> smoothBalanceL;
    QVector<double> smoothBalanceR;
    QVector<double> smoothLTE;
    QVector<double> smoothRTE;
    QVector<double> smoothLPS;
    QVector<double> smoothRPS;
    QVector<double> smoothLPCO;
    QVector<double> smoothRPCO;
    QVector<QwtIntervalSample> smoothLPP;
    QVector<QwtIntervalSample> smoothRPP;
    QVector<QwtIntervalSample> smoothLPPP;
    QVector<QwtIntervalSample> smoothRPPP;
    QVector<QwtIntervalSample> smoothRelSpeed;

    // setup as copy from user data
    void setUserData(QList<UserData*>); // reset below to reflect current
    QList<UserObject> U;

    // highlighting intervals
    QwtPlotCurve *intervalHighlighterCurve,  // highlight selected intervals on the Plot
                 *intervalHoverCurve;

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
        AllPlot(QWidget *parent, AllPlotWindow *window, Context *context, 
                RideFile::SeriesType series = RideFile::none, RideFile::SeriesType secSeries = RideFile::none, bool wanttext = true);
        ~AllPlot();

        bool eventFilter(QObject *object, QEvent *e);

        // set the curve data e.g. when a ride is selected
        void setDataFromRide(RideItem *_rideItem, QList<UserData*>);
        void setDataFromRideFile(RideFile *ride, AllPlotObject *object, QList<UserData*>); // when plotting lots of rides on fullPlot
        void setDataFromPlot(AllPlot *plot, int startidx, int stopidx);
        void setDataFromPlot(AllPlot *plot); // used for single series plotting
        void setDataFromPlots(QList<AllPlot*>); // user for single series comparing
        void setDataFromObject(AllPlotObject *object, AllPlot *reference); // for allplot when one per ride in a stack
                                                                           // reference is for settings et al
        void setMatchLabels(AllPlotObject *object); // set labels from object

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
        void refreshExhaustions();
        void refreshExhaustionsForAllPlots();
        void setAxisTitle(QwtAxisId axis, QString label);

        // refresh data / plot parameters
        void recalc(AllPlotObject *objects);
        void setYMax();
        void setLeftOnePalette(); // color of yLeft,1 axis
        void setRightPalette(); // color of yRight,0 axis
        void setXTitle();
        void setHighlightIntervals(bool);

        void plotTmpReference(QwtAxisId axis, int x, int y, RideFile::SeriesType serie);
        void confirmTmpReference(double value, int axis, RideFile::SeriesType serie, bool allowDelete);
        QwtPlotCurve* plotReferenceLine(const RideFilePoint *referencePoint);

        // marking part of a ride where athlete rode to exhaustion
        // user can confirm exact point and name to point
        void plotTmpExhaustion(double secs);
        QwtPlotMarker* plotExhaustionLine(double secs);
        void confirmTmpExhaustion(double secs, bool allowDelete=false);

        virtual void replot();

        // remembering state etc
        CurveColors *curveColors;

    public slots:

        void setShow(RideFile::SeriesType, bool);
        void setShowAccel(bool show);
        void setShowPowerD(bool show);
        void setShowCadD(bool show);
        void setShowTorqueD(bool show);
        void setShowHrD(bool show);
        void setShowPower(int id);
        void setShowAltSlope(int id);
        void setShowSlope(bool show);
        void setShowNP(bool show);
        void setShowATISS(bool show);
        void setShowANTISS(bool show);
        void setShowXP(bool show);
        void setShowAP(bool show);
        void setShowHr(bool show);
        void setShowHRV(bool show);
        void setShowTcore(bool show);
        void setShowSpeed(bool show);
        void setShowCad(bool show);
        void setShowAlt(bool show);
        void setShowTemp(bool show);
        void setShowWind(bool show);
        void setShowRV(bool show);
        void setShowRGCT(bool show);
        void setShowRCad(bool show);
        void setShowSmO2(bool show);
        void setShowtHb(bool show);
        void setShowO2Hb(bool show);
        void setShowHHb(bool show);
        void setShowGear(bool show);
        void setShowW(bool show);
        void setShowTorque(bool show);
        void setShowBalance(bool show);
        void setShowTE(bool show);
        void setShowPS(bool show);
        void setShowPCO(bool show);
        void setShowDC(bool show);
        void setShowPPP(bool show);
        void setShowGrid(bool show);
        void setPaintBrush(int state);
        void setShadeZones(bool x) { shade_zones=x; }
        void setSmoothing(int value);
        void setByDistance(int value);
        void setWantAxis(bool x, bool y=false) { wantaxis = x; wantxaxis = y;}
        void configChanged(qint32);

        // for tooltip
        void pointHover(QwtPlotCurve*, int);
        void intervalHover(IntervalItem *h);

    signals:
        void resized();

    protected:

        friend class ::AllPlotBackground;
        friend class ::AllPlotZoneLabel;
        friend class ::AllPlotWindow;
        friend class ::MergeAdjust;
        friend class ::IntervalPlotData;

        // cached state
        RideItem *rideItem;
        AllPlotBackground *bg;
        QSettings *settings;
        IntervalItem *hovered;

        // controls
        bool shade_zones;
        int showPowerState;
        int showAltSlopeState;
        bool showATISS;
        bool showANTISS;
        bool showNP;
        bool showXP;
        bool showAP;
        bool showHr;
        bool showHRV;
        bool showTcore;
        bool showSpeed;
        bool showAccel;
        bool showPowerD;
        bool showCadD;
        bool showTorqueD;
        bool showHrD;
        bool showCad;
        bool showAlt;
        bool showSlope;
        bool showTemp;
        bool showWind;
        bool showW;
        bool showTorque;
        bool showBalance;
        bool showTE;
        bool showPS;
        bool showPCO;
        bool showDC;
        bool showPPP;
        bool showRV;
        bool showRGCT;
        bool showRCad;
        bool showSmO2;
        bool showtHb;
        bool showO2Hb;
        bool showHHb;
        bool showGear;

        // plot objects
        AllPlotObject *standard;
        QList<QwtPlotCurve*> compares; // when plotting single series in compare mode

        QList <AllPlotZoneLabel *> zoneLabels;

        // array / smooth state
        int smooth;
        bool bydist;
        bool bytimeofday;
        bool fill;

        int timeoffset;

        // scope of plot (none means all, or just for a specific series
        RideFile::SeriesType scope;
        RideFile::SeriesType secondaryScope;

    protected:
        friend class ::AllPlotObject;
        Context *context;

    private:

        AllPlot *referencePlot;
        QWidget *parent;
        AllPlotWindow *window;
        bool wanttext, wantaxis, wantxaxis;
        bool isolation;
        LTMToolTip *tooltip;
        LTMCanvasPicker *_canvasPicker; // allow point selection/hover
        QFont labelFont;

        void setAltSlopePlotStyle (AllPlotSlopeCurve *curve);
        void setAxisScaleDiv(const QwtAxisId&, double, double, double);
        static inline void nextStep( int& step ) {
            if (step >= 5000)
            {
                step += 1000;
            } else if( step < 50 )
            {
                step += 10;
            }
            else if( step == 50 )
            {
                step = 100;
            }
            else if( step >= 100 && step < 1000 )
            {
                step += 100;
            }
            else //if( step >= 1000 && step < 5000)
            {
                step += 500;
            }
        }

};

// Produce Labels for X-Axis
class ScaleScaleDraw: public QwtScaleDraw
{
    public:

        ScaleScaleDraw(double factor=1.0f) : factor(factor), decimals(0) {}

        void setFactor(double f) { factor=f; }
        void setDecimals(int d) { decimals=d; }

        virtual QwtText label(double v) const {
            return QString("%1").arg(v * factor, 0, 'f', decimals);
        }

    private:

        double factor;
        int decimals;
};
#endif // _GC_AllPlot_h

