/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#include "GenericSelectTool.h"

#include "Colors.h"
#include "AbstractView.h"
#include "RideFileCommand.h"
#include "Utils.h"

#include <limits>

GenericSelectTool::GenericSelectTool(GenericPlot *host) : QObject(host), QGraphicsItem(NULL), host(host)
{
    // start inactive and rectangle
    state = INACTIVE;
    mode = RECTANGLE;
    setVisible(true); // always visible - paints on axis
    setZValue(100); // always on top.
    hoverpoint = GPointF();
    hoverseries = NULL;
    hoveraxis = NULL;
    rect = QRectF(0,0,0,0);
    connect(&drag, SIGNAL(timeout()), this, SLOT(dragStart()));
}

// the selection tool is painted only when it is active, but will be
// a lassoo or rectangle shape
void GenericSelectTool::paint(QPainter*painter, const QStyleOptionGraphicsItem *, QWidget*)
{
    // don't paint outside the canvas
    painter->save();

    // min max texts
    QFont stGiles; // hoho - Chart Font St. Giles ... ok you have to be British to get this joke
    stGiles.fromString(appsettings->value(NULL, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    stGiles.setPointSizeF(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt()*host->scale_);
    QFontMetrics fm(stGiles); // adjust position to align centre

    //
    // HOVERING OVER AN AXIS
    //
    if (hoveraxis) {
        QRectF fr = host->axisRect.value(hoveraxis, QRectF());
        if (fr != QRectF()) {
            QColor color = GColor(CPLOTMARKER);
            color.setAlphaF(0.2); // almost hidden if not moving/sizing
            painter->fillRect(mapRectFromScene(fr),QBrush(color));
        }

        // straight line is drawn below when running
        // through the series, as all axis info is
        // available at that point.
    }

    // clip to plot area by default
    painter->setClipRect(mapRectFromScene(host->qchart->plotArea()));

    switch (mode) {
    case CIRCLE:
        {

        }
        break;

    case XRANGE:
        {
            //
            // LINE CHART HOVERED POINTS ON DATA SERIES
            //
            foreach(SeriesPoint p, hoverpoints) {
                QPointF pos = mapFromScene(host->qchart->mapToPosition(p.xy,p.series));
                QColor invert = GCColor::invertColor(GColor(CPLOTBACKGROUND));
                painter->setBrush(invert);
                painter->setPen(invert);
                QRectF circle(0,0,gl_linemarker*dpiXFactor*host->scale_,gl_linemarker*dpiYFactor*host->scale_);
                circle.moveCenter(pos);
                painter->drawEllipse(circle);
                painter->setBrush(Qt::NoBrush);
            }
        }
        //
        // DROPS THROUGH INTO RECTANGLE FOR SELECTION RECTANGLE
        // AND SERIES VALUES IN AXIS FOR CURRENT CURSOR POSITION
        //
        // fall through
    case RECTANGLE:
        {
            if (state == ACTIVE || state == INACTIVE) {

                if (host->charttype == GC_CHART_LINE || host->charttype == GC_CHART_SCATTER) {


                    painter->setFont(stGiles);

                    //
                    // SCATTER PLOT HOVERED POINT
                    //
                    if (hoverpoint != GPointF()) {
                        // draw a circle using marker color
                        QColor invert = GCColor::invertColor(GColor(CPLOTBACKGROUND));
                        painter->setBrush(invert);
                        painter->setPen(invert);
                        QRectF circle(0,0,gl_scattermarker*dpiXFactor*host->scale_,gl_scattermarker*dpiYFactor*host->scale_);
                        circle.moveCenter(hoverpoint);
                        painter->drawEllipse(circle);
                        painter->setBrush(Qt::NoBrush);

                    }

                    // current position for each series
                    foreach(QAbstractSeries *series, host->qchart->series()) {

                        if (series->isVisible() == false) continue; // ignore invisble curves

                        //
                        // PAINT Y-VALUE IN AXIS (AND DEFERRED PAINT OF AXIS HOVER LINE)
                        //
                        // convert screen position to value for series
                        QPointF v = host->qchart->mapToValue(spos,series);
                        QPointF posyp = mapFromScene(host->qchart->mapToPosition(QPointF(0, v.y()),series));

                        // get the scene rect for the y axis
                        // we are a little careful since axisRect
                        QAbstractAxis *yaxis = NULL;
                        QAbstractAxis *xaxis = NULL;
                        foreach (QAbstractAxis *axis, series->attachedAxes()) {
                            if (axis->orientation() == Qt::Vertical) {
                                yaxis=axis;
                            } else {
                                xaxis=axis;
                            }
                        }
                        if (yaxis) {
                            QRectF ar = host->axisRect.value(yaxis, QRectF());
                            painter->setClipRect(mapRectFromScene(ar));

                            // we have the scene rect for the y axis, so lets use the x value
                            if (ar != QRectF()) {
                                posyp.setX(mapFromScene(ar.topLeft()).x()); // move onto axis
                            }
                        }

                        // get y-axis color
                        QColor axisColor=Qt::gray;
                        if (yaxis) axisColor = yaxis->labelsColor();
                        else if (series->type() == QAbstractSeries::SeriesTypeScatter) axisColor = static_cast<QScatterSeries*>(series)->color();
                        else if (series->type() == QAbstractSeries::SeriesTypeLine) axisColor = static_cast<QLineSeries*>(series)->color();
                        else if (series->type() == QAbstractSeries::SeriesTypeArea) axisColor = static_cast<QAreaSeries*>(series)->color();
                        painter->setPen(QPen(axisColor));

                        // y value
                        QString label=QString("%1").arg(v.y(),0,'f',0); // no decimal places XXX fixup on series info
                        label = Utils::removeDP(label); // remove unneccessary decimal places
                        painter->drawText(posyp+QPointF(0,fm.tightBoundingRect(label).height()/2.0), label);

                        // axis hover line (plot area only)
                        if (yaxis && yaxis == hoveraxis) {
                            QPointF left = mapFromScene(host->qchart->mapToPosition(QPointF(0, v.y()),series));
                            left.setX(mapFromScene(host->qchart->plotArea().x(),0).x());
                            QPointF right=left;
                            right.setX(mapFromScene(host->qchart->plotArea().bottomRight()).x());
                            painter->setClipRect(mapRectFromScene(host->qchart->plotArea()));
                            painter->drawLine(left, right);
                        }

                        //
                        // PAINT X VALUE BOTTOM OF PLOT AREA
                        //
                        QPointF posxp = mapFromScene(host->qchart->mapToPosition(QPointF(v.x(),0),series));
                        posxp.setY(mapFromScene(host->qchart->plotArea().bottomLeft()).y()); // bottom of plot, 0 is not always origin

                        // x value
                        painter->setPen(QPen(GColor(CPLOTMARKER)));
                        // datetime?
                        if (xaxis && xaxis->type() == QAbstractAxis::AxisTypeDateTime)
                            label=QDateTime::fromMSecsSinceEpoch(v.x()).toString(static_cast<QDateTimeAxis*>(xaxis)->format());
                        else
                            label=QString("%1").arg(v.x(),0,'f',0); // no decimal places XXX fixup on series info
                        label = Utils::removeDP(label); // remove unneccessary decimal places
                        painter->setClipRect(mapRectFromScene(host->qchart->plotArea()));
                        painter->drawText(posxp-(QPointF(fm.tightBoundingRect(label).width()/2.0,4)), label);
                    }
                }
            }


            if (state != INACTIVE) {

                //
                // SELECTION AREA AND STATS/SLOPE ETC
                //

                // there is a rectangle to draw on the screen
                QRectF r=QRectF(4,4,rect.width()-8,rect.height()-8);
                QColor color = GColor(CPLOTMARKER);
                color.setAlphaF((state == ACTIVE && !isUnderMouse()) ? 0.05 : 0.2); // almost hidden if not moving/sizing
                painter->fillRect(r,QBrush(color));

                // now paint the statistics
                foreach(GenericCalculator calc, stats) {

                    // slope and intercept?
                    if (calc.count<2) continue;

                    if (calc.series->isVisible() == false) continue;

                    if (calc.xaxis != NULL) {

                        if (calc.xaxis->type() == QAbstractAxis::AxisTypeValue ||
                            calc.xaxis->type() == QAbstractAxis::AxisTypeDateTime) {

                            // slope calcs way over the top for a line chart
                            // where there are multiple series being plotted
                            if (host->charttype == GC_CHART_SCATTER) {
                                QString lr=QString("y = %1 x + %2 \nr2=%3, see=%4").arg(calc.m,0,'f',3).arg(calc.b, 0,'f', 3)
                                                                                   .arg(calc.r2,0,'f',3).arg(calc.see,0,'f',3);
                                QPen line(calc.color);
                                painter->setPen(line);
                                painter->drawText(QPointF(0,0), lr);
                            }

                            // get start and stop points for slope line
                            QPointF startp, stopp;
                            if (calc.xaxis->type() == QAbstractAxis::AxisTypeValue ||
                                (calc.xaxis->type() == QAbstractAxis::AxisTypeDateTime &&
                                 static_cast<QDateTimeAxis*>(calc.xaxis)->format() == GenericPlot::gl_dateformat)) {

                                // continuous or date don't need transforming
                                double startx = static_cast<QValueAxis*>(calc.xaxis)->min();
                                double stopx = static_cast<QValueAxis*>(calc.xaxis)->max();
                                startp = mapFromScene(host->qchart->mapToPosition(QPointF(startx,calc.b+(startx*calc.m)),calc.series));
                                stopp = mapFromScene(host->qchart->mapToPosition(QPointF(stopx,calc.b+(stopx*calc.m)),calc.series));

                            } else {

                                // time formats need transforming
                                double startx = static_cast<QDateTimeAxis*>(calc.xaxis)->min().toMSecsSinceEpoch();
                                double stopx = static_cast<QDateTimeAxis*>(calc.xaxis)->max().toMSecsSinceEpoch();
                                double startsecs = calc.midnight.secsTo(static_cast<QDateTimeAxis*>(calc.xaxis)->min());
                                double stopsecs= calc.midnight.secsTo(static_cast<QDateTimeAxis*>(calc.xaxis)->max());
                                startp = mapFromScene(host->qchart->mapToPosition(QPointF(startx,calc.b+(startsecs*calc.m)),calc.series));
                                stopp = mapFromScene(host->qchart->mapToPosition(QPointF(stopx,calc.b+(stopsecs*calc.m)),calc.series));

                            }

                            // draw slope line
                            QColor col=GColor(CPLOTMARKER);
                            col.setAlphaF(1);
                            QPen line(col);
                            line.setStyle(Qt::SolidLine);
                            line.setWidthF(0.5 * dpiXFactor*host->scale_);
                            painter->setPen(line);
                            if (host->charttype == GC_CHART_LINE) painter->setClipRect(r); // too jarring on a line plot
                            else painter->setClipRect(mapRectFromScene(host->qchart->plotArea())); // need context for a scatter plot
                            painter->drawLine(startp, stopp);
                        }

                        // reset clip to the plot area
                        painter->setClipRect(mapRectFromScene(host->qchart->plotArea()));

                        // scene coordinate for min/max (remember we get clipped)
                        QPointF minxp = mapFromScene(host->qchart->mapToPosition(QPointF(calc.x.min,0),calc.series));
                        QPointF maxxp = mapFromScene(host->qchart->mapToPosition(QPointF(calc.x.max,0),calc.series));
                        QPointF minyp = mapFromScene(host->qchart->mapToPosition(QPointF(0, calc.y.min),calc.series));
                        QPointF maxyp = mapFromScene(host->qchart->mapToPosition(QPointF(0, calc.y.max),calc.series));
                        QPointF avgyp = mapFromScene(host->qchart->mapToPosition(QPointF(0, calc.y.mean),calc.series));
                        QPointF avgxp = mapFromScene(host->qchart->mapToPosition(QPointF(calc.x.mean, 0),calc.series));

                        // bottom of plotarea, remember Y-axis range doesn't always start from 0
                        minxp.setY(mapFromScene(host->qchart->plotArea().bottomLeft()).y());
                        maxxp.setY(mapFromScene(host->qchart->plotArea().bottomLeft()).y());


                        if (host->charttype == GC_CHART_SCATTER) avgxp.setY(mapFromScene(host->qchart->plotArea().bottomLeft()).y()-4);
                        else avgxp.setY(mapFromScene(host->qchart->plotArea().topLeft()).y()+fm.tightBoundingRect("XXX").height()+4);

                        QColor linecol=GColor(CPLOTMARKER);
                        linecol.setAlphaF(0.25);
                        QPen gridpen(linecol);
                        gridpen.setStyle(Qt::DashLine);
                        gridpen.setWidthF(1 *dpiXFactor);
                        painter->setPen(gridpen);

                        // min max texts
                        QFont stGiles; // hoho - Chart Font St. Giles ... ok you have to be British to get this joke
                        stGiles.fromString(appsettings->value(NULL, GC_FONT_CHARTLABELS, QFont().toString()).toString());
                        stGiles.setPointSizeF(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt()*host->scale_);
                        painter->setFont(stGiles);

                        QPen markerpen(GColor(CPLOTMARKER));
                        painter->setPen(markerpen);
                        QString label;
                        QString datetimeformat;

                        // ok, if its datetime lets get the format...
                        if (calc.xaxis->type() == QAbstractAxis::AxisTypeDateTime)  datetimeformat = static_cast<QDateTimeAxis*>(calc.xaxis)->format();

                        // max
                        if (datetimeformat!="") label=QDateTime::fromMSecsSinceEpoch(calc.x.max).toString(datetimeformat);
                        else label=QString("%1").arg(calc.x.max);
                        painter->drawText(maxxp-QPointF(0,4), label);

                        // min
                        if (datetimeformat!="") label=QDateTime::fromMSecsSinceEpoch(calc.x.min).toString(datetimeformat);
                        else label=QString("%1").arg(calc.x.min);
                        painter->drawText(minxp-QPointF(0,4), label);

                        // scatter sees mean, line/timeseries more likely to want dx
                        if (calc.xaxis->type() == QAbstractAxis::AxisTypeDateTime) {

                            if (datetimeformat == GenericPlot::gl_dateformat) {
                                // number of days, not an average in selection
                                int days = QDateTime::fromMSecsSinceEpoch(calc.x.min).daysTo(QDateTime::fromMSecsSinceEpoch(calc.x.max));
                                label=QString("%1 days").arg(days);
                             } else {
                                // selection in h:m:s format
                                int secs = QDateTime::fromMSecsSinceEpoch(calc.x.min).secsTo(QDateTime::fromMSecsSinceEpoch(calc.x.max));
                                label=time_to_string(secs, true);
                             }

                        } else {
                            if (host->charttype == GC_CHART_SCATTER)  label=QString("%1").arg(calc.x.mean);
                            else label=QString("%1").arg(calc.x.max - calc.x.min);
                        }

                        // paint centered
                        painter->drawText(avgxp-QPointF(fm.tightBoundingRect(label).width()/2.0,0), label);

                        //
                        // MIN AVG MAX Y VALUE ON PLOT AREA
                        //
                        if (host->charttype == GC_CHART_SCATTER) {

                            // too noisy on a line chart, where there are
                            // typically multiple data series being plotted
                            // arguably this is too busy for a scatter chart too...
                            markerpen = QPen(calc.color);
                            painter->setPen(markerpen);
                            label=QString("%1").arg(calc.y.max);
                            painter->drawText(maxyp, label);
                            label=QString("%1").arg(calc.y.min);
                            painter->drawText(minyp, label);
                            label=QString("%1").arg(calc.y.mean);
                            painter->drawText(avgyp, label);
                        }
                    }

                }
            }
        }
        break;

    case LASSOO:
        {

        }
        break;
    }
    painter->restore();
}

// rect is updated during selection etc in the event handler
QRectF GenericSelectTool::boundingRect() const { return rect; }

// trap events and redirect to plot event handler
bool GenericSelectTool::sceneEventFilter(QGraphicsItem *watched, QEvent *event) { return host->eventHandler(0, watched,event); }

bool
GenericSelectTool::reset()
{
    state = INACTIVE;
    host->setCursor(Qt::ArrowCursor);
    start=QPointF(0,0);
    finish=QPointF(0,0);
    rect = QRectF(0,0,0,0);
    hoverpoint = GPointF();
    hoverseries = NULL;
    hoverpoints.clear();
    hoveraxis = NULL;
    hidden.clear();
    resetSelections();
    rectchanged=true;
    update();
    return true;
}

// handle mouse events in selector
bool
GenericSelectTool::seriesClicked()
{
    // clicked on a point in the series
    return clicked(QPointF()); // ignore mostly
}

bool
GenericSelectTool::clicked(QPointF pos)
{
    bool updatescene = false;

    // click on a point to click-thru
    if (hoverpoint.index != -1) { // scatter plot
        emit seriesClicked(hoverseries, hoverpoint);

        // not sure need to do this....
        hoverpoints.clear();
        hoverpoint=GPointF();
        return false;

    } else if (pos == QPointF()) { // series clicked and not hovering
        return false;

    }

    if (mode == XRANGE || mode == RECTANGLE) {

        if (hoveraxis) return false;


        if (state==ACTIVE && sceneBoundingRect().contains(pos)) {

            // are we moving?
            state = MOVING;
            start = pos;
            startingpos = this->pos();
            rectchanged = true;
            update(rect);
            updatescene = true;

        } else {

            // initial sizing - or click hold to drag?
            state = SIZING;
            start = pos;
            finish = QPointF(0,0);

            if (mode == RECTANGLE) {
                rect = QRectF(-5,-5,5,5);
                setPos(start);
            } else if (mode == XRANGE) {
                rect = QRectF(0,0,5,host->qchart->plotArea().height());
                setPos(QPointF(start.x(),host->qchart->plotArea().y()));
            }

            rectchanged = true;
            updatescene = true;
            update(rect);
        }
    }

    // time 400ms to drag - after lots of playing
    // this seems a reasonable time period that isnt
    // too long but doesn't conflict with straight
    // forward click to select.
    // its probably no coincidence that this is also
    // the Doherty Threshold for UX
    drag.setInterval(400);
    drag.setSingleShot(true);
    drag.start();

    return updatescene;
}

bool
GenericSelectTool::released(QPointF pos)
{
    if (mode == RECTANGLE || mode == XRANGE) {

        // ignore if in axis, likely adding line.
        if (hoveraxis) {

            // we clicked to add a line, not to start
            // a selection etc (that must be done from
            // the plot area)

            // use first series that uses the axis
            QAbstractSeries *yseries=NULL;
            foreach(QAbstractSeries *series, host->qchart->series()) {
                foreach(QAbstractAxis *axis, series->attachedAxes()) {
                    if (axis->orientation() == Qt::Vertical && axis == hoveraxis) {
                        yseries = series;
                        break;
                    }
                }
                if (yseries) break;
            }

            if (yseries) {

                // convert value to series value
                QPointF v = host->qchart->mapToValue(pos,yseries);

                // add to chart
                host->addAnnotation(GenericPlot::LINE, yseries, v.y());
            }

            return true; // don't drop through into selection logic
        }

        // width and heights can be negative if dragged in reverse
        if (state == DRAGGING ||
           (rect.width() < 10 && rect.width() > -10 && (mode == XRANGE || (rect.height() < 10 && rect.height() > -10)))) {

            reset();
            rectchanged = true;
            return true;

        } else if (state == SIZING || state == MOVING) {

            // finishing move/resize
            state = ACTIVE;
            hoverpoint=GPointF();
            hoverpoints.clear();
            rectchanged = true;
            update(rect);
            return true;
        }
    }
    return false;
}

void
GenericSelectTool::dragStart()
{
    if (mode == RECTANGLE || mode == XRANGE) {
        // check still right state for it?
        if (state == SIZING || state == MOVING) {

            reset(); // reset the selection, it would be active from the initial click

            start = spos;
            host->setCursor(Qt::ClosedHandCursor);
            state = DRAGGING;

            updateScene(); // force as we are a timeout not scene event
        }
    }
}

// for std::lower_bound search of x QPointF value
struct CompareQPointFX {
    bool operator()(const QPointF p1, const QPointF p2) {
        return p1.x() < p2.x();
    }
};

bool
GenericSelectTool::moved(QPointF pos)
{
    // user hovers over points, but can select using a rectangle
    if (state == SIZING) {

        // cancel the timer to trigger drag
        drag.stop();

        // work out where we got to
        finish = pos;

        // reshape - rect might have negative sizes if sized backwards
        if (mode == RECTANGLE) rect = QRectF(QPointF(0,0), finish-start);
        else rect.setWidth (finish.x()-start.x());
        rectchanged = true;

        update(rect);
        return true;

    } else if (state == MOVING) {

        // cancel the timer to trigger drag
        drag.stop();

        QPointF delta = pos - start;
        if (mode == RECTANGLE) setPos(this->startingpos + delta);
        else if (mode == XRANGE) setPos(this->startingpos + QPointF(delta.x(),0));
        rectchanged = true;
        update(rect);
        return true;

    } else if (state == DRAGGING) {

        //QPointF delta = pos - start;
        // move axis to reflect new pos...
        return true;

    } else {

        // HOVERING around.. the big one...

        // first off, are we hovering over axes, as might need to isolate
        // or are we isolating and need to reset back again
        if (isolateAxes(spos))  {
            update(rect);
            return true;
        }

        if (mode == RECTANGLE) {
            // remember screen pos of cursor for tracking values
            // when painting on the axis/plot area
            spos = pos;

            // not moving or sizing so just hovering
            // look for nearest point for each series
            // this needs to be super quick as mouse
            // movements are very fast, so we use a
            // quadtree to find the nearest points
            GPointF hoverv; // value                    // series x,y co-ord used in signal (and legend later)
            hoverpoint = GPointF(); // screen coordinates
            QAbstractSeries *originalhoverseries = hoverseries;
            hoverseries = NULL;
            foreach(QAbstractSeries *series, host->qchart->series()) {

                if (series->isVisible() == false) continue; // ignore invisble curves

                Quadtree *tree= host->quadtrees.value(series,NULL);
                if (tree != NULL) {

                    // lets convert cursor pos to value pos to find nearest
                    double pixels = 10 * dpiXFactor; // within 10 pixels
                    QRectF srect(pos-QPointF(pixels,pixels), pos+QPointF(pixels,pixels));
                    QRectF vrect(host->qchart->mapToValue(srect.topLeft(),series), host->qchart->mapToValue(srect.bottomRight(),series));
                    //QPointF vpos = host->qchart->mapToValue(pos, series);

                    // find candidates all close by using paint co-ords
                    QList<GPointF> tohere;
                    tree->candidates(vrect, tohere);

                    QPointF cursorpos=mapFromScene(pos);
                    foreach(GPointF p, tohere) {
                        QPointF scpos = mapFromScene(host->qchart->mapToPosition(p, series));
                        if (hoverpoint == GPointF()) {
                            hoverpoint = GPointF(scpos.x(), scpos.y(), p.index);
                            hoverseries = series;
                            hoverv = p;
                        } else if ((cursorpos-scpos).manhattanLength() < (cursorpos-hoverpoint).manhattanLength()) {
                            hoverpoint=GPointF(scpos.x(), scpos.y(), p.index);
                            hoverseries = series;
                            hoverv = p;
                        }
                    }

                    //if (tohere.count())  fprintf(stderr, "HOVER %d candidates nearby\n", tohere.count()); fflush(stderr);
                }

            }

            // hoverpoint changed - either a new series selected, a new point, or no point at all
            if (originalhoverseries != hoverseries || hoverv != GPointF()) {
                if (hoverseries != originalhoverseries && originalhoverseries != NULL) emit (unhover(originalhoverseries->name())); // old hover changed
                if (hoverseries != NULL)  emit hover(hoverv, hoverseries->name(), hoverseries); // new hover changed
            }

            // we need to clear x-axis if we aren't hovering on anything at all
            if (hoverv == GPointF()) {
                emit unhoverx();
            }

            // for mouse moves..
            update(rect);
            return true;

        } else if (mode == XRANGE) {

            // user hovers with a vertical line, but can select a range on the x axis
            // lets get x axis value (any old series will do as they should have a common x axis
            spos = pos;
            QMap<QAbstractSeries*,GPointF> vals; // what values were found
            double nearestx=-9999;
            foreach(QAbstractSeries *series, host->qchart->series()) {

                if (ignore.contains(series)) continue;

                if (series->isVisible() == false) {
                    // but is there a decoration visible?
                    QAbstractSeries *dec = host->decorations.value(series);
                    if (dec==NULL || dec->isVisible()== false) continue;
                }

                // get x value to search
                double xvalue=host->qchart->mapToValue(spos,series).x();

                // pointsVector
                if (series->type() == QAbstractSeries::SeriesTypeLine || series->type() == QAbstractSeries::SeriesTypeArea) {

                    // we take a copy, would love to avoid this.
                    QVector<QPointF> p = series->type() == QAbstractSeries::SeriesTypeLine ? static_cast<QLineSeries*>(series)->pointsVector() :
                                                                              static_cast<QAreaSeries*>(series)->upperSeries()->pointsVector();

                    // value we want
                    QPointF x= QPointF(xvalue,0);

                    // lower_bound to value near x
                    QVector<QPointF>::const_iterator i = std::lower_bound(p.begin(), p.end(), x, CompareQPointFX());

                    // collect them away
                    vals.insert(series, GPointF(i->x(), i->y(), i-p.begin()));

                    // nearest x?
                    if (i->x() != 0 && (nearestx == -9999 || (std::fabs(i->x()-xvalue)) < std::fabs((nearestx-xvalue)))) nearestx = i->x();
                }

            }

            // run over what we found, updating paint points and signal (for legend)
            hoverpoints.clear();
            QMapIterator<QAbstractSeries*, GPointF> i(vals);
            while (i.hasNext()) {
                i.next();
                if (i.value().x() == nearestx) {
                    SeriesPoint add;
                    add.series = i.key();
                    add.xy = i.value();
                    emit hover(i.value(), i.key()->name(), i.key());
                    if (add.xy.y()) hoverpoints << add; // ignore zeroes
                } else emit unhover(i.key()->name());
            }
            if (vals.count()) return true;
        }
    }
    return false;
}

bool
GenericSelectTool::wheel(int delta)
{

    if (mode == RECTANGLE) {
        // mouse wheel resizes selection rect if it is active
        if (state == ACTIVE) {
            if (delta < 0) {
                rect.setSize(rect.size() * 0.9);
            } else {
                rect.setSize(rect.size() * 1.1);
            }
            rectchanged = true;
            return true;
        }
    }
    return false;
}

bool
GenericSelectTool::isolateAxes(QPointF scenepos)
{
    QAbstractAxis *over=NULL;

    // check if mouse in axes
    QMapIterator<QAbstractAxis*, QRectF>i(host->axisRect);
    while (i.hasNext()) {
        i.next();
        if (i.value().contains(scenepos)) {
            over = i.key();
            break;
        }
    }

    // just debug for now
    if (over && over != hoveraxis) {

        // clear up from old hover?
        foreach(QAbstractSeries *series, hidden) series->setVisible(true);
        hidden.clear();

        // hide all visible curves that are not attached to this axis
        foreach(QAbstractSeries *series, host->qchart->series()) {
            if (series->isVisible() == false) continue; // ignore already hidden curves

            // is it on this axis?
            bool ours=false;
            foreach(QAbstractAxis *axis, series->attachedAxes())
                if (axis==over)
                    ours=true;

            // hide if not one of ours
            if (!ours) {
                hidden.append(series);
                series->setVisible(false);
            }
        }

        // now set hoveraxis to the one we're over
        hoveraxis = over;
        return true;

    } else if (over == NULL && hoveraxis) {

        // clear up from old hover?
        foreach(QAbstractSeries *series, hidden) series->setVisible(true);
        hidden.clear();

        // we are no longer hovering over an axis
        hoveraxis = NULL;
        return true;
    }
    return false;

}

double
GenericSelectTool::miny(QAbstractSeries*series)
{
    return host->qchart->mapToValue(pos()+QPointF(0,rect.height()), series).y();
}

double
GenericSelectTool::maxy(QAbstractSeries*series)
{
    return host->qchart->mapToValue(pos(), series).y();
}

double
GenericSelectTool::minx(QAbstractSeries*series)
{
    return host->qchart->mapToValue(pos(), series).x();
}

double
GenericSelectTool::maxx(QAbstractSeries*series)
{
    return host->qchart->mapToValue(pos()+QPointF(rect.width(),0), series).x();
}

void
GenericCalculator::initialise()
{
    count=0;
    m = b = lrsumxy = lrsumx2 = x.lrsum = y.lrsum = r2 = see =
    x.max = x.min = x.sum = x.mean =
    y.max = y.min = y.sum = y.mean = 0;
    xaxis=yaxis=NULL;
    midnight=QDateTime(QDate::currentDate(), QTime(0,0,0));

}

void
GenericCalculator::addPoint(QPointF point)
{
    // translate points if they've been adjusted to be time values for the
    // Qt chart time axis, i.e from MS since Epoch to seconds - this is because
    // we need original values for linear regress, all other calcs are fine
    // at this point
    QPointF lr=point;

    // X
    if (xaxis && xaxis->type() == QAbstractAxis::AxisTypeDateTime &&
        static_cast<QDateTimeAxis*>(xaxis)->format() == GenericPlot::gl_timeformat)
        lr.setX(midnight.secsTo(QDateTime::fromMSecsSinceEpoch(point.x())));

    // Y
    if (yaxis && yaxis->type() == QAbstractAxis::AxisTypeDateTime &&
        static_cast<QDateTimeAxis*>(yaxis)->format() == GenericPlot::gl_timeformat)
        lr.setY(midnight.secsTo(QDateTime::fromMSecsSinceEpoch(point.y())));

    // max min set from values we've seen
    if (count >0) {
        if (point.x() < x.min) x.min = point.x();
        if (point.x() > x.max) x.max = point.x();
        if (point.y() < y.min) y.min = point.y();
        if (point.y() > y.max) y.max = point.y();
    } else {
        // use only value we've seen
        x.min = x.max = point.x();
        y.min = y.max = point.y();
    }

    // basic
    count ++;
    x.sum += point.x();
    y.sum += point.y();
    x.mean = x.sum / double(count);
    y.mean = y.sum / double(count);

    // lr
    lrsumx2 += lr.x() * lr.x();
    lrsumxy += lr.x() * lr.y();
    x.lrsum += lr.x();
    y.lrsum += lr.y();

    // keep a record
    actual << lr;
}

void
GenericCalculator::finalise()
{
    // add calcs for stuff cannot do on the fly
    if (actual.count() >=2) {
        m = (count * lrsumxy - x.lrsum * y.lrsum) / (count * lrsumx2 - (x.lrsum * x.lrsum));
        b = (y.lrsum - m * x.lrsum) / count;

        // r2
        double sumActualErrors=0, sumPredictErrors=0;
        see = 0;
        for(int i=0; i<actual.count(); i++) {
            sumActualErrors += pow(actual[i].y() - y.mean, 2);
            sumPredictErrors += pow(((actual[i].x()*m)+b) - y.mean, 2);
            see += pow(((actual[i].x() * m) + b) - actual[i].y(), 2);
        }
        r2 = sumPredictErrors / sumActualErrors;

        if (actual.count() >2) see = sqrt(see/(actual.count()-2));
        else see=0;

    } else {
        m = b = r2 = see = 0;
    }

    actual.clear();
}

void
GenericSelectTool::setSeriesVisible(QString name, bool visible)
{
    // a series got hidden or showm, so do whats needed.
    QString selectionName = QString("%1_select").arg(name);
    foreach(QAbstractSeries *x,  host->qchart->series())
        if (x->name() == selectionName)
            x->setVisible(visible);

    // as a special case we clear the hoverpoints.
    // they will get redisplayed when the cursor moves
    // but for now this is the quickes and simplest way
    // to avoid artefacts
    hoverpoints.clear();
    hoverpoint=GPointF();

    // hide/show and updatescenes may overlap/get out of sync
    // so we update scene to be absolutely sure the scene
    // reflects the current visible settings (and others later)
    rectchanged = true;
    updateScene();
}

// selector needs to update the chart for selections
void
GenericSelectTool::updateScene()
{
    // is the selection active?
    if (rectchanged) {

        // clear incidental state that gets reset when needed
        stats.clear();

        if (state != GenericSelectTool::INACTIVE && state != DRAGGING) {

            // selection tool is active so set curves gray
            // and create curves for highlighted points etc
            QList<QAbstractSeries*> originallist=host->qchart->series();
            foreach(QAbstractSeries *x, originallist) { // because we update it below (!)

                if (x->isVisible() == false || ignore.contains(x)) continue;

                // Run through all the curves, setting them gray
                // selecting the points that fall into the selection
                // and creating selection curves and stats for painting
                // onto the plot
                //
                // We duplicate code by curve type (but not chart type)
                // so we can e.g. put a scatter curve on a line chart
                // and vice versa later.

                switch(x->type()) {
                case QAbstractSeries::SeriesTypeLine: {
                    QLineSeries *line = x->type() == QAbstractSeries::SeriesTypeLine ? static_cast<QLineSeries*>(x) :
                                                                                       static_cast<QAreaSeries*>(x)->upperSeries() ;
                    // ignore empty series
                    if (line->count() < 1) continue;

                    // this will be used to plot selected points on the plot
                    QLineSeries *selection =NULL;

                    // the axes for the current series
                    QAbstractAxis *xaxis=NULL, *yaxis=NULL;
                    foreach (QAbstractAxis *ax, x->attachedAxes()) {
                        if (ax->orientation() == Qt::Vertical && yaxis==NULL) yaxis=ax;
                        if (ax->orientation() == Qt::Horizontal && xaxis==NULL) xaxis=ax;
                    }

                    if ((selection=static_cast<QLineSeries*>(selections.value(x, NULL))) == NULL) {

                        selection = new QLineSeries();
                        selection->setName(QString("%1_select").arg(line->name()));

                        // all of this curve cloning should be in a new method xxx todo
                        selection->setUseOpenGL(line->useOpenGL());
                        selection->setPen(line->pen());
                        if (line->useOpenGL())
                            selection->setColor(Qt::gray); // use opengl ignores changing colors
                        else {
                            selection->setColor(line->color());
                            if (x->type() == QAbstractSeries::SeriesTypeLine) static_cast<QLineSeries*>(x)->setColor(Qt::gray);
                            else if (x->type() == QAbstractSeries::SeriesTypeArea) static_cast<QAreaSeries*>(x)->setColor(Qt::gray);
                        }
                        selections.insert(x, selection);
                        ignore.append(selection);

                        // add after done all aesthetic for opengl snafus
                        host->qchart->addSeries(selection); // before adding data and axis

                        // only do when creating it.
                        if (yaxis) selection->attachAxis(yaxis);
                        if (xaxis) selection->attachAxis(xaxis);
                    }

                    // lets work out what range of values we need to be
                    // selecting is, reverse since possible to have a backwards
                    // rectangle in the selection tool
                    double minx=0,maxx=0;
                    minx =this->minx(x);
                    maxx =this->maxx(x);
                    if (maxx < minx) { double t=minx; minx=maxx; maxx=t; }

                    //fprintf(stderr, "xaxis range %f-%f, yaxis range %f-%f, [%s] %d points to check\n", minx,maxx,miny,maxy,scatter->name().toStdString().c_str(), scatter->count());

                    // add points to the selection curve and calculate as you go
                    QList<QPointF> points;
                    GenericCalculator calc;
                    calc.initialise();
                    calc.color = selection->color(); // should this go into constructor?! xxx todo
                    calc.xaxis = xaxis;
                    calc.yaxis = yaxis;
                    calc.series = line;
                    for(int i=0; i<line->count(); i++) {
                        QPointF point = line->at(i); // avoid deep copy
                        if (point.x() >= minx && point.x() <= maxx) {
                            if (!points.contains(point)) points << point; // avoid dupes
                            calc.addPoint(point);
                        }
                    }
                    calc.finalise();
                    stats.insert(line, calc);

                    selection->clear();
                    if (points.count()) selection->append(points);

                }
                break;
                case QAbstractSeries::SeriesTypeScatter: {

                    QScatterSeries *scatter = static_cast<QScatterSeries*>(x);

                    // ignore empty series
                    if (scatter->count() < 1) continue;

                    // this will be used to plot selected points on the plot
                    QScatterSeries *selection =NULL;

                    // the axes for the current series
                    QAbstractAxis *xaxis=NULL, *yaxis=NULL;
                    foreach (QAbstractAxis *ax, x->attachedAxes()) {
                        if (ax->orientation() == Qt::Vertical && yaxis==NULL) yaxis=ax;
                        if (ax->orientation() == Qt::Horizontal && xaxis==NULL) xaxis=ax;
                    }

                    if ((selection=static_cast<QScatterSeries*>(selections.value(x, NULL))) == NULL) {

                        selection = new QScatterSeries();

                        // all of this curve cloning should be in a new method xxx todo
                        host->qchart->addSeries(selection); // before adding data and axis
                        selection->setUseOpenGL(scatter->useOpenGL());
                        if (selection->useOpenGL())
                            selection->setColor(Qt::gray); // use opengl ignores changing colors
                        else {
                            selection->setColor(scatter->color());
                            static_cast<QScatterSeries*>(x)->setColor(Qt::gray);
                        }
                        selection->setMarkerSize(scatter->markerSize());
                        selection->setMarkerShape(scatter->markerShape());
                        selection->setPen(scatter->pen());
                        selection->setName(QString("%1_select").arg(scatter->name()));
                        selections.insert(x, selection);
                        ignore.append(selection);

                        // only do when creating it.
                        if (yaxis) selection->attachAxis(yaxis);
                        if (xaxis) selection->attachAxis(xaxis);
                    }

                    // lets work out what range of values we need to be
                    // selecting is, reverse since possible to have a backwards
                    // rectangle in the selection tool
                    double miny=0,maxy=0,minx=0,maxx=0;
                    miny =this->miny(x);
                    maxy =this->maxy(x);
                    if (maxy < miny) { double t=miny; miny=maxy; maxy=t; }

                    minx =this->minx(x);
                    maxx =this->maxx(x);
                    if (maxx < minx) { double t=minx; minx=maxx; maxx=t; }

                    //fprintf(stderr, "xaxis range %f-%f, yaxis range %f-%f, [%s] %d points to check\n", minx,maxx,miny,maxy,scatter->name().toStdString().c_str(), scatter->count());

                    // add points to the selection curve and calculate as you go
                    QList<QPointF> points;
                    GenericCalculator calc;
                    calc.initialise();
                    calc.color = selection->color(); // should this go into constructor?! xxx todo
                    calc.xaxis = xaxis;
                    calc.yaxis = yaxis;
                    calc.series = scatter;
                    for(int i=0; i<scatter->count(); i++) {
                        QPointF point = scatter->at(i); // avoid deep copy
                        if (point.y() >= miny && point.y() <= maxy &&
                            point.x() >= minx && point.x() <= maxx) {
                            if (!points.contains(point)) points << point; // avoid dupes
                            calc.addPoint(point);
                        }
                    }
                    calc.finalise();
                    stats.insert(scatter, calc);

                    selection->clear();
                    if (points.count()) selection->append(points);
                }
                break;
                default:
                    break;
                }
            }

            rectchanged = false;

        } else {

            resetSelections();
        }
    }

    // repaint everything
    host->chartview->scene()->update(0,0,host->chartview->scene()->width(),host->chartview->scene()->height());
 }

 void
 GenericSelectTool::resetSelections()
 {
    // selection tool isn't active so reset curves to original
    if (selections.count()) {

        foreach(QAbstractSeries *x, host->qchart->series()) {

            if (ignore.contains(x)) continue; // we still reset selections for invisible curves

            switch(x->type()) {

            case QAbstractSeries::SeriesTypeLine:
            {
                QLineSeries *selection=NULL;
                if ((selection=static_cast<QLineSeries*>(selections.value(x,NULL))) != NULL) {

                    // set greyed out original back to its proper color
                    // rememember when opengl is in use setColor is ignored
                    // so we didn't change it on selection, so no need to
                    // set it back to original in that case
                    if (!selection->useOpenGL()) {
                        if (x->type() == QAbstractSeries::SeriesTypeLine)  static_cast<QLineSeries*>(x)->setColor(static_cast<QLineSeries*>(selection)->color());
                        else if (x->type() == QAbstractSeries::SeriesTypeArea)  static_cast<QAreaSeries*>(x)->setColor(static_cast<QLineSeries*>(selection)->color());
                    }

                    // clear points, remove from axis and remove from chart
                    selection->clear();
                    foreach(QAbstractAxis *ax, selection->attachedAxes()) selection->detachAxis(ax);
                    host->qchart->removeSeries(selection);
                    delete selection;
                }
            }
            break;
            case QAbstractSeries::SeriesTypeScatter:
            {
                QScatterSeries *selection=NULL;
                if ((selection=static_cast<QScatterSeries*>(selections.value(x,NULL))) != NULL) {

                    // set greyed out original back to its proper color
                    // rememember when opengl is in use setColor is ignored
                    // so we didn't change it on selection, so no need to
                    // set it back to original in that case
                    if (!selection->useOpenGL())
                        static_cast<QScatterSeries*>(x)->setColor(static_cast<QScatterSeries*>(selection)->color());

                    // clear points, remove from axis and remove from chart
                    selection->clear();
                    foreach(QAbstractAxis *ax, selection->attachedAxes()) selection->detachAxis(ax);
                    host->qchart->removeSeries(selection);
                    delete selection;
                }
            }
            break;
            default: break;
            }
        }
        selections.clear();
    }
    ignore.clear();
    stats.clear();
}
