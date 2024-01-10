/*
 * Copyright (c) 2014 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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



#include "AllPlotInterval.h"

#include "Colors.h"
#include "Units.h"
#include "Athlete.h"
#include "AllPlot.h"
#include "IntervalItem.h"

#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_intervalcurve.h>
#include <qwt_scale_div.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_map.h>

class AllPlotIntervalData : public QwtArraySeriesData<QwtIntervalSample>
{
    public:
    AllPlotIntervalData(AllPlotInterval *plot, Context *context, int level, int max, RideItem *rideItem, const IntervalItem *interval) :
        plot(plot), context(context), level(level), max(max), rideItem(rideItem), interval(interval) {}

    double x(size_t i) const ;
    double ymin(size_t) const ;
    double ymax(size_t i) const ;

    size_t size() const ;

    void init() ;
    IntervalItem *intervalNum(int n) const;
    int intervalCount() const;

    AllPlotInterval *plot;
    Context *context;
    int level;
    int max;

    RideItem *rideItem;
    const IntervalItem *interval;

    virtual QwtIntervalSample sample(size_t i) const;
    virtual QRectF boundingRect() const;
};

class TimeScaleDraw: public ScaleScaleDraw
{

    public:

    TimeScaleDraw(bool *bydist) : ScaleScaleDraw(), bydist(bydist) {}

    virtual QwtText label(double v) const
    {
        if (*bydist) {
            return QString("%1").arg(v);
        } else {
            QTime t = QTime(0,0,0,0).addSecs(v*60.00);
            if (scaleMap().sDist() > 5)
                return t.toString("hh:mm");
            return t.toString("hh:mm:ss");
        }
    }
    private:
    bool *bydist;

};

AllPlotInterval::AllPlotInterval(QWidget *parent, Context *context):
    QwtPlot(parent),
    bydist(false),
    context(context),
    rideItem(NULL),
    groupMatch(false)
{
    // tick draw
    //TimeScaleDraw *tsd = new TimeScaleDraw(&this->bydist) ;
    //tsd->setTickLength(QwtScaleDiv::MajorTick, 3);
    //setAxisScaleDraw(QwtAxis::XBottom, tsd);
    //pal.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    //pal.setColor(QPalette::Text, GColor(CPLOTMARKER));
    //axisWidget(QwtAxis::XBottom)->setPalette(pal);

    setAxisVisible(QwtAxis::XBottom, false);
    setAxisVisible(QwtAxis::XBottom, false);
    setAxisVisible(QwtAxis::YLeft, false);

    tooltip = new LTMToolTip(QwtAxis::XBottom, QwtAxis::YLeft,
                                   QwtPicker::NoRubberBand,
                                   QwtPicker::AlwaysOn,
                                   canvas(),
                                   "");

    canvasPicker = new AllPlotIntervalCanvasPicker(this);

    connect(context, SIGNAL(intervalHover(IntervalItem*)), this, SLOT(intervalHover(IntervalItem*)));
    connect(canvasPicker, SIGNAL(pointHover(QwtPlotIntervalCurve*, int)), this, SLOT(intervalCurveHover(QwtPlotIntervalCurve*)));
    connect(canvasPicker, SIGNAL(pointClicked(QwtPlotIntervalCurve*,int)), this, SLOT(intervalCurveClick(QwtPlotIntervalCurve*)));
    connect(canvasPicker, SIGNAL(pointDblClicked(QwtPlotIntervalCurve*,int)), this, SLOT(intervalCurveDblClick(QwtPlotIntervalCurve*)));

    configChanged(CONFIG_APPEARANCE);
}

void
AllPlotInterval::configChanged(qint32)
{
    QPalette pal = palette();
    pal.setBrush(QPalette::Window, QBrush(GColor(CRIDEPLOTBACKGROUND)));
    setPalette(pal);
    setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);
    update();
    replot();
}

void
AllPlotInterval::setByDistance(int id)
{
    bydist = (id == 1);
    if (rideItem == NULL) return;
    if (intervalLigns.count() == 0) return;

    refreshIntervals();
}

void
AllPlotInterval::setDataFromRide(RideItem *_rideItem)
{
    rideItem = _rideItem;
    if (rideItem == NULL) return;

    // set bottom scale to match the ride
    refreshIntervals();
}

void
AllPlotInterval::refreshIntervals()
{
    placeIntervals();
    refreshIntervalCurve();
    //refreshIntervalMarkers();
}

// Compare two RideFileInterval on duration.
bool intervalBiggerThan(const IntervalItem* i1, const IntervalItem* i2)
{
    return (i1->stop-i1->start) > (i2->stop-i2->start);
}

void
AllPlotInterval::sortIntervals(QList<IntervalItem*> &intervals, QList< QList<IntervalItem*> > &intervalsGroups)
{
    // Sort by duration
    std::sort(intervals.begin(), intervals.end(), intervalBiggerThan);

    QList<IntervalItem*> matchesGroup;

    for (int i=0; i<intervals.count(); i++) {
        IntervalItem* interval = intervals.at(i);

        if (groupMatch && interval->type == RideFileInterval::USER) {
            matchesGroup.append(interval);
            intervals.removeOne(interval);
            //intervals.move(i, place++);
        }
    }

    if (matchesGroup.count() > 0)
        intervalsGroups.append(matchesGroup);

}

void
AllPlotInterval::placeIntervals()
{
    if (!rideItem) return;

    QList<IntervalItem*> intervals = rideItem->intervals();
    QList< QList<IntervalItem*> > intervalsGroups;

    sortIntervals(intervals, intervalsGroups);

    intervalLigns.clear();

    if (intervalsGroups.count()>0)
        intervalLigns.append(intervalsGroups.at(0));
    else {
        QList<IntervalItem*> intervalsLign1;
        intervalLigns.append(intervalsLign1);
    }

    while (intervals.count()>0) {
        IntervalItem *interval = intervals.first();

        int lign = 0;
        bool placed = false;

        while (!placed) {
            bool place = true;

            /*if (interval.isPeak()) {
                intervals.removeFirst();
                placed = true;
            }*/

            foreach(const IntervalItem* placedinterval, intervalLigns.at(lign)) {
                if (interval->stop>placedinterval->start && interval->start<placedinterval->stop)
                    place = false;
            }
            if (place) {
                intervalLigns[lign].append(interval);
                intervals.removeFirst();
                placed = true;
            } else {
                lign++;
                if (intervalLigns.count()<=lign) {
                    QList<IntervalItem*> intervalsLign;
                    intervalLigns.append(intervalsLign);
                }
            }
         }
    }

    setFixedHeight((1+intervalLigns.count())*(10*dpiYFactor));
    setAxisScale(QwtAxis::YLeft, 0, 3000*intervalLigns.count());
}

void
AllPlotInterval::setColorForIntervalCurve(QwtPlotIntervalCurve *intervalCurve, const IntervalItem *interval, bool selected)
{
    QColor color = interval->color;
    QPen ihlPen = QPen(color);
    intervalCurve->setPen(ihlPen);
    QColor ihlbrush = QColor(color);
    if (!selected)
        ihlbrush.setAlpha(128);
    intervalCurve->setPen(ihlbrush);   // fill below the line
    intervalCurve->setBrush(ihlbrush);   // fill below the line
}

void
AllPlotInterval::refreshIntervalCurve()
{
    foreach(QwtPlotIntervalCurve *curve, curves.values()) {
        curve->detach();
        delete curve;
    }
    curves.clear();

    int level=0;
    foreach(const QList<IntervalItem*> &intervalsLign, intervalLigns) {

        foreach(IntervalItem *interval, intervalsLign) {
            QwtPlotIntervalCurve *intervalCurve = new QwtPlotIntervalCurve();
            intervalCurve->setYAxis(QwtAxis::YLeft);

            setColorForIntervalCurve(intervalCurve, interval, false);

            int max = 3000*intervalLigns.count();
            intervalCurve->setSamples(new AllPlotIntervalData(this, context, level, max, rideItem, interval));

            intervalCurve->attach(this);
            curves.insert(interval, intervalCurve);
        }
        level++;
    }
}

void
AllPlotInterval::intervalHover(IntervalItem *chosen)
{
    foreach(IntervalItem *interval, curves.keys()) {
        if (chosen == interval) {
            setColorForIntervalCurve(curves.value(interval), interval, true);
        } else  {
            setColorForIntervalCurve(curves.value(interval), interval, false);
        }
    }
    replot();
}

void
AllPlotInterval::intervalCurveHover(QwtPlotIntervalCurve *curve)
{
    if (curve != NULL) {
        IntervalItem *interval = curves.key(curve);
        //intervalHover(interval);

        // tell the charts -- and block signals whilst they occur
        tooltip->setText(interval->name);
        blockSignals(true);
        context->notifyIntervalHover(interval);
        blockSignals(false);

    } else {
        context->notifyIntervalHover(NULL); // clear
        tooltip->setText("");
    }
}

void
AllPlotInterval::intervalCurveClick(QwtPlotIntervalCurve *curve) {
    IntervalItem *interval = curves.key(curve);

    if (interval) {
        interval->selected = !interval->selected;
        context->notifyIntervalItemSelectionChanged(interval);
    }
}

void
AllPlotInterval::intervalCurveDblClick(QwtPlotIntervalCurve *curve) {
    IntervalItem *interval = curves.key(curve);
    if (interval) context->notifyIntervalZoom(interval);
}


/*
 *
 */

double
AllPlotIntervalData::x(size_t i) const
{
    //if (interval == NULL) return 0; // out of bounds !?
    double multiplier = GlobalContext::context()->useMetricUnits ? 1 : MILES_PER_KM;

    // which point are we returning?
    switch (i%4) {
        case 0 : return plot->bydist ? (multiplier * interval->startKM) : interval->start/60; // bottom left
        case 1 : return plot->bydist ? (multiplier * interval->startKM) : interval->start/60; // top left
        case 2 : return plot->bydist ? (multiplier * interval->stopKM) : interval->stop/60; // top right
        case 3 : return plot->bydist ? (multiplier * interval->stopKM) : interval->stop/60; // bottom right
    }
    return 0; // shouldn't get here, but keeps compiler happy
}


double
AllPlotIntervalData::ymin(size_t) const
{
    return max-2000-3000*(level);
}

double
AllPlotIntervalData::ymax(size_t i) const
{
    switch (i%4) {
        case 0 : return ymin(i); // bottom left
        case 1 : return max-3000*(level); // top left
        case 2 : return max-3000*(level); // top right
        case 3 : return ymin(i); // bottom right
    }
    return 0; // shouldn't get here, but keeps compiler happy
}


size_t
AllPlotIntervalData::size() const { return 4; }

QwtIntervalSample AllPlotIntervalData::sample(size_t i) const {
    return QwtIntervalSample( x(i), ymin(i), ymax(i) );
}

QRectF
AllPlotIntervalData::boundingRect() const
{
    return QRectF(0, 5000*dpiYFactor, 5100*dpiXFactor, 5100*dpiYFactor);
}

AllPlotIntervalCanvasPicker::AllPlotIntervalCanvasPicker(QwtPlot *plot):
    QObject(plot),
    d_selectedCurve(NULL),
    d_selectedPoint(-1)
{
    canvas = static_cast<QwtPlotCanvas*>(plot->canvas());
    canvas->installEventFilter(this);


    // We want the focus, but no focus rect. The
    canvas->setFocusPolicy(Qt::StrongFocus);
    canvas->setFocusIndicator(QwtPlotCanvas::ItemFocusIndicator);
}

bool
AllPlotIntervalCanvasPicker::event(QEvent *e)
{
    if ( e->type() == QEvent::User )
    {
        //showCursor(true);
        return true;
    }
    return QObject::event(e);
}

bool
AllPlotIntervalCanvasPicker::eventFilter(QObject *object, QEvent *e)
{
    // for our canvas ?
    if (object != canvas) return false;

    switch(e->type())
    {
        default:
            QApplication::postEvent(this, new QEvent(QEvent::User));
            break;
        case QEvent::MouseButtonDblClick:
            select(((QMouseEvent *)e)->pos(), true, true);
            break;
        case QEvent::MouseButtonPress:
            select(((QMouseEvent *)e)->pos(), true, false);
            break;
        case QEvent::MouseMove:
            select(((QMouseEvent *)e)->pos(), false, false);
            break;
    }
    return QObject::eventFilter(object, e);
}

// Select the point at a position. If there is no point
// deselect the selected point

void
AllPlotIntervalCanvasPicker::select(const QPoint &pos, bool clicked, bool dblClicked)
{
    QwtPlotIntervalCurve *curve = NULL;
    int index = -1;

    const QwtPlotItemList& itmList = plot()->itemList();
    for ( QwtPlotItemIterator it = itmList.begin();
        it != itmList.end(); ++it )
    {
        if ( (*it)->rtti() == QwtPlotItem::Rtti_PlotIntervalCurve )
        {
            QwtPlotIntervalCurve *c = (QwtPlotIntervalCurve*)(*it);

            double xmin = c->plot()->transform(c->xAxis(),c->sample(0).value);
            double xmax = c->plot()->transform(c->xAxis(),c->sample(2).value);
            double ymax = c->plot()->transform(c->yAxis(),c->sample(2).interval.minValue());
            double ymin = c->plot()->transform(c->yAxis(),c->sample(2).interval.maxValue());

            if ( pos.x()>=xmin &&
                 pos.x()<=xmax &&
                 pos.y()>=ymin &&
                 pos.y()<=ymax)
            {
                curve = c;
            }
        }
    }

    d_selectedCurve = NULL;
    d_selectedPoint = -1;

    if ( curve )
    {
        // picked one
        d_selectedCurve = curve;
        d_selectedPoint = index;

        if (dblClicked)
            pointDblClicked(curve, index); // emit
        else if (clicked)
            pointClicked(curve, index); // emit
        else
            pointHover(curve, index);  // emit
    } else {
        // didn't
        if (dblClicked)
            pointDblClicked(NULL, index); // emit
        else if (clicked)
            pointClicked(NULL, -1); // emit
        else
            pointHover(NULL, -1);  // emit

    }
}
