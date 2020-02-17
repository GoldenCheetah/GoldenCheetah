/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

#include "GenericPlot.h"

#include "Colors.h"
#include "TabView.h"
#include "RideFileCommand.h"

GenericPlot::GenericPlot(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    // intitialise state info
    charttype=0;
    chartview=NULL;
    barseries=NULL;
    bottom=left=true;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    // setup the chart
    qchart = new QChart();
    qchart->setBackgroundVisible(false); // draw on canvas
    qchart->legend()->setVisible(false); // no legends --> custom todo
    qchart->setTitle("No title set"); // none wanted
    qchart->setAnimationOptions(QChart::NoAnimation);
    qchart->setFont(QFont());

    // set theme, but for now use a std one TODO: map color scheme to chart theme
    qchart->setTheme(QChart::ChartThemeDark);

    chartview = new QChartView(qchart, this);
    chartview->setRenderHint(QPainter::Antialiasing);
    mainLayout->addWidget(chartview);

    // watch mouse hover etc on the chartview and scene
    chartview->setMouseTracking(true);
    chartview->scene()->installEventFilter(this);

    // add selector
    selector = new SelectionTool(this);
    chartview->scene()->addItem(selector);

    // filter ESC so we can stop scripts
    installEventFilter(this);

    // watch for colors changing
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // config changed...
    configChanged(0);
}

//
// Selecting points on the plot
//
SelectionTool::SelectionTool(GenericPlot *host) : QGraphicsItem(NULL), host(host)
{
    // start inactive and rectangle
    state = INACTIVE;
    mode = RECTANGLE;
    setVisible(false);
    rect = QRectF(0,0,0,0);
}

// the selection tool is painted only when it is active, but will be
// a lassoo or rectangle shape
void SelectionTool::paint(QPainter*painter, const QStyleOptionGraphicsItem *, QWidget*)
{
   if (state == INACTIVE) return; // do not paint when inactive
   switch (mode) {
   case CIRCLE:
        {

        }
        break;
   case RECTANGLE:
        {
            // paint inside!
            painter->save();
            QRectF r=QRectF(4,4,rect.width()-8,rect.height()-8);
            QColor color = GColor(CPLOTMARKER);
            color.setAlphaF(state == ACTIVE ? 0.05 : 0.2); // almost hidden if not moving/sizing
            painter->setClipRect(mapRectFromScene(host->qchart->plotArea()));
            painter->fillRect(r,QBrush(color));

            // now paint the statistics
            foreach(Calculator calc, stats) {
                // slope and intercept?
                if (calc.count<2) continue;
                QString lr=QString("y = %1 x + %2").arg(calc.m).arg(calc.b);
                QPen line(calc.color);
                painter->setPen(line);
                painter->drawText(QPointF(0,0), lr);

                // slope
                if (calc.xaxis != NULL) {

                    if (calc.xaxis->type() == QAbstractAxis::AxisTypeValue) { //XXX todo for log date etc?
                        double startx = static_cast<QValueAxis*>(calc.xaxis)->min();
                        double stopx = static_cast<QValueAxis*>(calc.xaxis)->max();
                        QPointF startp = mapFromScene(host->qchart->mapToPosition(QPointF(startx,calc.b),calc.series));
                        QPointF stopp = mapFromScene(host->qchart->mapToPosition(QPointF(stopx,calc.b+(stopx*calc.m)),calc.series));
                        QColor col=GColor(CPLOTMARKER);
                        col.setAlphaF(1);
                        QPen line(col);
                        line.setStyle(Qt::SolidLine);
                        line.setWidthF(2 * dpiXFactor);
                        painter->setPen(line);
                        painter->setClipRect(r);
                        painter->drawLine(startp, stopp);
                        painter->setClipRect(mapRectFromScene(host->qchart->plotArea()));
                    }

                    // scene coordinate for min/max (remember we get clipped)
                    QPointF minxp = mapFromScene(host->qchart->mapToPosition(QPointF(calc.x.min,0),calc.series));
                    QPointF minxpinf = mapFromScene(host->qchart->mapToPosition(QPointF(calc.x.min,calc.y.min),calc.series));

                    QPointF maxxp = mapFromScene(host->qchart->mapToPosition(QPointF(calc.x.max,0),calc.series));
                    QPointF maxxpinf = mapFromScene(host->qchart->mapToPosition(QPointF(calc.x.max,calc.y.max),calc.series));

                    QPointF minyp = mapFromScene(host->qchart->mapToPosition(QPointF(0, calc.y.min),calc.series));
                    QPointF minypinf = mapFromScene(host->qchart->mapToPosition(QPointF(calc.x.min, calc.y.min),calc.series));

                    QPointF maxyp = mapFromScene(host->qchart->mapToPosition(QPointF(0, calc.y.max),calc.series));
                    QPointF maxypinf = mapFromScene(host->qchart->mapToPosition(QPointF(calc.x.max, calc.y.max),calc.series));

                    QPointF avgyp = mapFromScene(host->qchart->mapToPosition(QPointF(0, calc.y.mean),calc.series));
                    QPointF avgxp = mapFromScene(host->qchart->mapToPosition(QPointF(calc.x.mean, 0),calc.series));
                    QPointF avgmid = mapFromScene(host->qchart->mapToPosition(QPointF(calc.x.mean, calc.y.mean),calc.series));

                    QColor linecol=GColor(CPLOTMARKER);
                    linecol.setAlphaF(0.25);
                    QPen gridpen(linecol);
                    gridpen.setStyle(Qt::DashLine);
                    gridpen.setWidthF(1 *dpiXFactor);
                    painter->setPen(gridpen);
#if 0 // way too busy on the chart
                    // min/max guides
                    painter->drawLine(minxp,minxpinf);
                    painter->drawLine(maxxp,maxxpinf);
                    painter->drawLine(minyp,minypinf);
                    painter->drawLine(maxyp,maxypinf);

                    linecol = QColor(Qt::red);
                    linecol.setAlphaF(0.25);
                    gridpen = QColor(linecol);
                    gridpen.setStyle(Qt::DashLine);
                    gridpen.setWidthF(1 *dpiXFactor);
                    painter->setPen(gridpen);

                    // avg guides
                    painter->drawLine(avgxp,avgmid);
                    painter->drawLine(avgyp,avgmid);
#endif

                    // min max texts
                    QFont stGiles; // hoho - Chart Font St. Giles ... ok you have to be British to get this joke
                    stGiles.fromString(appsettings->value(NULL, GC_FONT_CHARTLABELS, QFont().toString()).toString());
                    stGiles.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());
                    painter->setFont(stGiles);

                    QPen markerpen(GColor(CPLOTMARKER));
                    painter->setPen(markerpen);
                    QString label=QString("%1").arg(calc.x.max);
                    painter->drawText(maxxp-QPointF(0,4), label);
                    label=QString("%1").arg(calc.x.min);
                    painter->drawText(minxp-QPointF(0,4), label);
                    label=QString("%1").arg(calc.x.mean);
                    painter->drawText(avgxp-QPointF(0,4), label);

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
            painter->restore();
        }
        break;

   case LASSOO:
        {

        }
        break;
   }
}

// rect is updated during selection etc in the event handler
QRectF SelectionTool::boundingRect() const { return rect; }

// trap events and redirect to plot event handler
bool SelectionTool::sceneEventFilter(QGraphicsItem *watched, QEvent *event) { return host->eventHandler(0, watched,event); }
bool GenericPlot::eventFilter(QObject *obj, QEvent *e) { return eventHandler(1, obj, e); }

bool
SelectionTool::reset()
{
    state = INACTIVE;
    start=QPointF(0,0);
    finish=QPointF(0,0);
    rect = QRectF(0,0,0,0);
    setVisible(false);
    resetSelections();
    update();
    return true;
}

// handle mouse events in selector
bool
SelectionTool::clicked(QPointF pos)
{
    if (state==ACTIVE && sceneBoundingRect().contains(pos)) {

        // are we moving?
        state = MOVING;
        setZValue(100);
        start = pos;
        startingpos = this->pos();
        update(rect);
        return true;

    } else {

        // initial sizing
        state = SIZING;
        start = pos;
        finish = QPointF(0,0);
        rect = QRectF(-5,-5,5,5);
        setPos(start);
        // above when selecting
        setZValue(100);
        setVisible(true);
        update(rect);
        return true;

    }
    return false;
}

bool
SelectionTool::released(QPointF)
{
    // width and heights can be negative if dragged in reverse
    if (rect.width() < 10 && rect.width() > -10 && rect.height() < 10 && rect.height() > -10) {

        // tiny, as in click release - deactivate
        state = INACTIVE; // reset for any state
        rect = QRectF(0,0,0,0);
        setVisible(false);
        return true;

    } else if (state == SIZING || state == MOVING) {

        // finishing move/resize
        state = ACTIVE;
        setZValue(-100); // send to back after done
        update(rect);
        return true;
    }
    return false;
}

bool
SelectionTool::moved(QPointF pos)
{
    // only care if we are sizing
    if (state == SIZING) {
        finish = pos;

        // reshape - rect might have negative sizes if sized backwards
        rect = QRectF(QPointF(0,0), finish-start);
        update(rect);
        return true;

    } else if (state == MOVING) {

        QPointF delta = pos - start;
        setPos(this->startingpos + delta);
        update(rect);
        return true;
    }
    return false;
}

bool
SelectionTool::wheel(int delta)
{
    if (state == ACTIVE) {
        if (delta < 0) {
            rect.setSize(rect.size() * 0.9);
        } else {
            rect.setSize(rect.size() * 1.1);
        }
        return true;
    }
    return false;
}

double
SelectionTool::miny(QAbstractSeries*series)
{
    return host->qchart->mapToValue(pos()+QPointF(0,rect.height()), series).y();
}

double
SelectionTool::maxy(QAbstractSeries*series)
{
    return host->qchart->mapToValue(pos(), series).y();
}

double
SelectionTool::minx(QAbstractSeries*series)
{
    return host->qchart->mapToValue(pos(), series).x();
}

double
SelectionTool::maxx(QAbstractSeries*series)
{
    return host->qchart->mapToValue(pos()+QPointF(rect.width(),0), series).x();
}

void
Calculator::initialise()
{
    count=0;
    m = b = sumxy = sumx2 =
    x.max = x.min = x.sum = x.mean =
    y.max = y.min = y.sum = y.mean = 0;

}

void
Calculator::addPoint(QPointF point)
{

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

    count ++;
    x.sum += point.x();
    y.sum += point.y();
    x.mean = x.sum / double(count);
    y.mean = y.sum / double(count);
    sumx2 += point.x() * point.x();
    sumxy += point.x() * point.y();
}

void
Calculator::finalise()
{
    // add calcs for stuff cannot do on the fly
    if (count >=2) {
        m = (count * sumxy - x.sum * y.sum) / (count * sumx2 - (x.sum * x.sum));
        b = (y.sum - m * x.sum) / count;
    }
}

// source 0=scene, 1=widget
bool
GenericPlot::eventHandler(int source, void *obj, QEvent *e)
{
    static bool block=false;

    // don't want to get interuppted
    if (block) return false;
    else block=true;

    // lets get some basic info first
    // where is the cursor?
    QPoint wpos=cursor().pos();
    QPointF spos=QPointF();

    // so we want to trigger a scene update?
    bool updatescene = false;

    //
    // HANDLE EVENTS AND UPDATE STATE
    //
    switch(e->type()) {

    // mouse clicked
    case QEvent::GraphicsSceneMousePress:
        spos = static_cast<QGraphicsSceneMouseEvent*>(e)->scenePos();
    //case QEvent::MouseButtonPress:
        //fprintf(stderr,"POS: %g:%g | %d:%d\n", spos.x(), spos.y(), wpos.x(), wpos.y());
        //fprintf(stderr,"%s: mouse PRESSED for obj=%u\n", source ? "widget" : "scene", (void*)obj); fflush(stderr);
        {
            updatescene = selector->clicked(spos);
        }
    break;

    // mouse released
    case QEvent::GraphicsSceneMouseRelease:
        spos = static_cast<QGraphicsSceneMouseEvent*>(e)->scenePos();
    //case QEvent::MouseButtonRelease:
        //fprintf(stderr,"POS: %g:%g | %d:%d\n", spos.x(), spos.y(), wpos.x(), wpos.y());
        //fprintf(stderr,"%s: mouse RELEASED for obj=%u\n", source ? "widget" : "scene", (void*)obj); fflush(stderr);
        {
            updatescene = selector->released(spos);
        }
    break;

    // mouse move
    case QEvent::GraphicsSceneMouseMove:
        spos = static_cast<QGraphicsSceneMouseEvent*>(e)->scenePos();
    //case QEvent::MouseMove:
        //fprintf(stderr,"POS: %g:%g | %d:%d\n", spos.x(), spos.y(), wpos.x(), wpos.y());
        //fprintf(stderr,"%s: mouse MOVE for obj=%u\n", source ? "widget" : "scene", (void*)obj); fflush(stderr);
        {
            updatescene = selector->moved(spos);
        }
    break;

    case QEvent::GraphicsSceneWheel:
        {
            QGraphicsSceneWheelEvent *w = static_cast<QGraphicsSceneWheelEvent*>(e);
            //fprintf(stderr,"%s: mouse WHEEL [%d] for obj=%u\n", source ? "widget" : "scene", w->delta(),(void*)obj); fflush(stderr);
            updatescene = selector->wheel(w->delta());
        }
        break;

    // resize
    case QEvent::Resize: {
        //canvas->fitInView(canvas->sceneRect(), Qt::KeepAspectRatio);
    }
    break;

    // tooltip, paused for a moment..
    case QEvent::GraphicsSceneHoverEnter:
    case QEvent::GraphicsSceneHelp:
    case QEvent::GraphicsSceneHoverMove: {

        // debug info
        fprintf(stderr,"%s: HOVER scene item for obj=%u\n", source ? "widget" : "scene", (void*)obj); fflush(stderr);
    }
    break;

    // tooltip, paused for a moment..
    case QEvent::GraphicsSceneHoverLeave: {

        // debug info
        fprintf(stderr,"%s: UNHOVER scene item for obj=%u\n", source ? "widget" : "scene", (void*)obj); fflush(stderr);
    }
    break;

    // tooltip, paused for a moment..
    case QEvent::ToolTip: {

        // debug info
        //fprintf(stderr,"%s: tooltip for obj=%u\n", source ? "widget" : "scene", (void*)obj); fflush(stderr);
    }
    break;


    default:
        //fprintf(stderr,"%s: some event %d for obj=%u\n", source ? "widget" : "scene", e->type(), (void*)obj); fflush(stderr);
        break;
    }

    //
    // UPDATE SCENE TO REFLECT STATE
    //
    if (updatescene) {

        selector->updateScene(); // really only do selection right now.. more to come

        // repaint everything
        chartview->scene()->update(0,0,chartview->scene()->width(),chartview->scene()->height());
    }

    // all done.
    block = false;
    return false;
}

// selector needs to update the chart for selections
void
SelectionTool::updateScene()
{
    // is the selection active?
    if (state != SelectionTool::INACTIVE) {

        // selection tool is active so set curves gray
        // and create curves for highlighted points etc
        QList<QAbstractSeries*> originallist=host->qchart->series();
        foreach(QAbstractSeries *x, originallist) { // because we update it below (!)

            if (ignore.contains(x)) continue;

            switch(x->type()) {
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
                    selection->setColor(scatter->color());
                    selection->setMarkerSize(scatter->markerSize());
                    selection->setMarkerShape(scatter->markerShape());
                    selection->setPen(scatter->pen());
                    selection->setVisible(true);
                    selections.insert(x, selection);
                    ignore.append(selection);
                    static_cast<QScatterSeries*>(x)->setColor(Qt::gray);

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
                Calculator calc;
                calc.initialise();
                calc.color = selection->color(); // should this go into constructor?! xxx todo
                calc.xaxis = xaxis;
                calc.yaxis = yaxis;
                calc.series = scatter;
                for(int i=0; i<scatter->count(); i++) {
                    QPointF point = scatter->at(i); // avoid deep copy
                    if (point.y() >= miny && point.y() <= maxy &&
                        point.x() >= minx && point.x() <= maxx) {
                        points << point;
                        calc.addPoint(point);
                    }
                }
                calc.finalise();
                stats.insert(scatter, calc);

                //fprintf(stderr, "selected %d points\n", points.count());
                selection->clear();
                if (points.count()) selection->append(points);
            }
            break;
            }
        }

    } else {

        resetSelections();
    }
 }

 void
 SelectionTool::resetSelections()
 {
    // selection tool isn't active so reset curves to original
    if (selections.count()) {

        foreach(QAbstractSeries *x, host->qchart->series()) {

            if (ignore.contains(x)) continue;

            switch(x->type()) {

            case QAbstractSeries::SeriesTypeScatter:
                QScatterSeries *selection=NULL;
                if ((selection=static_cast<QScatterSeries*>(selections.value(x,NULL))) != NULL) {

                    // set greyed out original back to its proper color
                    static_cast<QScatterSeries*>(x)->setColor(static_cast<QScatterSeries*>(selection)->color());

                    // clear points, remove from axis and remove from chart
                    selection->clear();
                    foreach(QAbstractAxis *ax, selection->attachedAxes()) selection->detachAxis(ax);
                    host->qchart->removeSeries(selection);
                    delete selection;
                }
                break;
            }
        }
        selections.clear();
    }
    ignore.clear();
    stats.clear();
}

void
GenericPlot::configChanged(qint32)
{
    // tinted palette for headings etc
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);

    // chart colors
    chartview->setBackgroundBrush(QBrush(GColor(CPLOTBACKGROUND)));
    qchart->setBackgroundBrush(QBrush(GColor(CPLOTBACKGROUND)));
    qchart->setBackgroundPen(QPen(GColor(CPLOTMARKER)));
}

double
GenericPlot::min(QAbstractAxis *ax)
{
    if (ax == NULL) return 0;
    else {
        switch (ax->type()) {
            case QAbstractAxis::AxisTypeValue:
                return static_cast<QValueAxis*>(ax)->min();
                break;
        }
        return 0;
    }
}

double
GenericPlot::max(QAbstractAxis *ax)
{
    if (ax == NULL) return 0;
    else {
        switch (ax->type()) {
            case QAbstractAxis::AxisTypeValue:
                return static_cast<QValueAxis*>(ax)->max();
                break;
        }
        return 0;
    }
}

bool
GenericPlot::initialiseChart(QString title, int type, bool animate)
{
    // if we changed the type, all series must go
    if (charttype != type) {
        qchart->removeAllSeries();
        curves.clear();
        barseries=NULL;
    }

    // clear ALL axes
    foreach(QAbstractAxis *axis, qchart->axes(Qt::Vertical)) {
        qchart->removeAxis(axis);
        delete axis;
    }
    foreach(QAbstractAxis *axis, qchart->axes(Qt::Horizontal)) {
        qchart->removeAxis(axis);
        delete axis;
    }
    foreach(AxisInfo *axisinfo, axisinfos) delete axisinfo;
    axisinfos.clear();

    left=true;
    bottom=true;
    barsets.clear();

    // reset selections etc
    selector->reset();

    // remember type
    charttype=type;

    // title is allowed to be blank
    qchart->setTitle(title);

    // would avoid animations as they get very tiresome and are not
    // generally transition animations, so add very little value
    // by default they are disabled anyway
    qchart->setAnimationOptions(animate ? QChart::SeriesAnimations : QChart::NoAnimation);

    return true;
}

// rendering to qt chart
bool
GenericPlot::addCurve(QString name, QVector<double> xseries, QVector<double> yseries, QString xname, QString yname,
                      QStringList labels, QStringList colors,
                      int /*line TODO*/, int /*symbol TODO */, int size, QString color, int opacity, bool opengl)
{
    // if curve already exists, remove it
    if (charttype==GC_CHART_LINE || charttype==GC_CHART_SCATTER || charttype==GC_CHART_PIE) {
        QAbstractSeries *existing = curves.value(name);
        if (existing) {
            qchart->removeSeries(existing);
            delete existing; // XXX is this such a great idea.. causes a lot of flicker...
        }
    }

    // lets find that axis - even blank ones
    AxisInfo *xaxis, *yaxis;
    xaxis=axisinfos.value(xname);
    yaxis=axisinfos.value(yname);
    if (xaxis==NULL) {
        xaxis=new AxisInfo(Qt::Horizontal, xname);

        // default alignment toggles
        xaxis->align = bottom ? Qt::AlignBottom : Qt::AlignTop;
        bottom = !bottom;

        // use marker color for x axes
        xaxis->labelcolor = xaxis->axiscolor = GColor(CPLOTMARKER);

        // add to list
        axisinfos.insert(xname, xaxis);
    }
    if (yaxis==NULL) {
        yaxis=new AxisInfo(Qt::Vertical, yname);

        // default alignment toggles
        yaxis->align = left ? Qt::AlignLeft : Qt::AlignRight;
        left = !left;

        // yaxis color matches, but not done for xaxis above
        yaxis->labelcolor = yaxis->axiscolor = QColor(color);

        // add to list
        axisinfos.insert(yname, yaxis);
    }

    switch (charttype) {
    default:

    case GC_CHART_LINE:
        {
            // set up the curves
            QLineSeries *add = new QLineSeries();
            add->setName(name);

            // aesthetics
            add->setBrush(Qt::NoBrush);
            QPen pen(color);
            pen.setStyle(Qt::SolidLine);
            pen.setWidth(size);
            add->setPen(pen);
            add->setOpacity(double(opacity) / 100.0); // 0-100% to 0.0-1.0 values

            // data
            for (int i=0; i<xseries.size() && i<yseries.size(); i++) {
                add->append(xseries.at(i), yseries.at(i));

                // tell axis about the data
                xaxis->point(xseries.at(i), yseries.at(i));
                yaxis->point(xseries.at(i), yseries.at(i));
            }

            // hardware support?
            chartview->setRenderHint(QPainter::Antialiasing);
            add->setUseOpenGL(opengl); // for scatter or line only apparently

            // chart
            qchart->addSeries(add);

            // add to list of curves
            curves.insert(name,add);
            xaxis->series.append(add);
            yaxis->series.append(add);
        }
        break;

    case GC_CHART_SCATTER:
        {
            // set up the curves
            QScatterSeries *add = new QScatterSeries();
            add->setName(name);

            // aesthetics
            add->setMarkerShape(QScatterSeries::MarkerShapeCircle); //TODO: use 'symbol'
            add->setMarkerSize(size);
            QColor col=QColor(color);
            add->setBrush(QBrush(col));
            add->setPen(Qt::NoPen);
            add->setOpacity(double(opacity) / 100.0); // 0-100% to 0.0-1.0 values

            // data
            for (int i=0; i<xseries.size() && i<yseries.size(); i++) {
                add->append(xseries.at(i), yseries.at(i));

                // tell axis about the data
                xaxis->point(xseries.at(i), yseries.at(i));
                yaxis->point(xseries.at(i), yseries.at(i));

            }

            // hardware support?
            chartview->setRenderHint(QPainter::Antialiasing);
            add->setUseOpenGL(opengl); // for scatter or line only apparently

            // chart
            qchart->addSeries(add);
            qchart->legend()->setMarkerShape(QLegend::MarkerShapeRectangle);
            qchart->setDropShadowEnabled(false);

            // add to list of curves
            curves.insert(name,add);
            xaxis->series.append(add);
            yaxis->series.append(add);

        }
        break;

    case GC_CHART_BAR:
        {
            // set up the barsets
            QBarSet *add= new QBarSet(name);

            // aesthetics
            add->setBrush(QBrush(QColor(color)));
            add->setPen(Qt::NoPen);

            // data and min/max values
            for (int i=0; i<yseries.size(); i++) {
                double value = yseries.at(i);
                *add << value;
                yaxis->point(i,value);
                xaxis->point(i,value);
            }

            // we are very particular regarding axis
            yaxis->type = AxisInfo::CONTINUOUS;
            xaxis->type = AxisInfo::CATEGORY;

            // add to list of barsets
            barsets << add;
        }
        break;

    case GC_CHART_PIE:
        {
            // set up the curves
            QPieSeries *add = new QPieSeries();

            // setup the slices
            for(int i=0; i<yseries.size(); i++) {
                // get label?
                if (i>=labels.size())
                    add->append(QString("%1").arg(i), yseries.at(i));
                else
                    add->append(labels.at(i), yseries.at(i));
            }

            // now do the colors
            int i=0;
            foreach(QPieSlice *slice, add->slices()) {

                slice->setExploded();
                slice->setLabelVisible();
                slice->setPen(Qt::NoPen);
                if (i <colors.size()) slice->setBrush(QColor(colors.at(i)));
                else slice->setBrush(Qt::red);
                i++;
            }

            // set the pie chart
            qchart->addSeries(add);

            // add to list of curves
            curves.insert(name,add);
        }
        break;

    }
    return true;
}

// once python script has run polish the chart, fixup axes/ranges and so on.
void
GenericPlot::finaliseChart()
{
    if (!qchart) return;

    // basic aesthetics
    qchart->legend()->setMarkerShape(QLegend::MarkerShapeRectangle);
    qchart->setDropShadowEnabled(false);

    // no more than 1 category axis since barsets are all assigned.
    bool donecategory=false;

    // Create axes - for everyone except pie charts that don't have any
    if (charttype != GC_CHART_PIE) {
        // create desired axis
        foreach(AxisInfo *axisinfo, axisinfos) {
//fprintf(stderr, "Axis: %s, orient:%s, type:%d\n",axisinfo->name.toStdString().c_str(),axisinfo->orientation==Qt::Vertical?"vertical":"horizontal",(int)axisinfo->type);
//fflush(stderr);
            QAbstractAxis *add=NULL;
            switch (axisinfo->type) {
            case AxisInfo::DATERANGE: // TODO
            case AxisInfo::TIME:      // TODO
            case AxisInfo::CONTINUOUS:
                {
                    QValueAxis *vaxis= new QValueAxis(qchart);
                    add=vaxis; // gets added later

                    vaxis->setMin(axisinfo->min());
                    vaxis->setMax(axisinfo->max());

                    // attach to the chart
                    qchart->addAxis(add, axisinfo->locate());
                }
                break;
            case AxisInfo::CATEGORY:
                {
                    if (!donecategory) {

                        donecategory=true;

                        QBarCategoryAxis *caxis = new QBarCategoryAxis(qchart);
                        add=caxis;

                        // add the bar series
                        if (!barseries) { barseries = new QBarSeries(); qchart->addSeries(barseries); }
                        else barseries->clear();

                        // add the new barsets
                        foreach (QBarSet *bs, barsets)
                            barseries->append(bs);

                        // attach before addig barseries
                        qchart->addAxis(add, axisinfo->locate());

                        // attach to category axis
                        barseries->attachAxis(caxis);

                        // category labels
                        for(int i=axisinfo->categories.count(); i<=axisinfo->maxx; i++)
                            axisinfo->categories << QString("%1").arg(i+1);
                        caxis->setCategories(axisinfo->categories);
                    }
                }
                break;
            }

            // at this point the basic settngs have been done and the axis
            // is attached to the chart, so we can go ahead and apply common settings
            if (add) {

                // once we've done the basics, lets do the aesthetics
                QFont stGiles; // hoho - Chart Font St. Giles ... ok you have to be British to get this joke
                stGiles.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
                stGiles.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());
                add->setTitleFont(stGiles);
                add->setLabelsFont(stGiles);

                if (axisinfo->name != "x" && axisinfo->name != "y")  // equivalent to being blank
                    add->setTitleText(axisinfo->name);
                add->setLinePenColor(axisinfo->axiscolor);
                if (axisinfo->orientation == Qt::Vertical) // we never have y axis lines
                    add->setLineVisible(false);
                add->setLabelsColor(axisinfo->labelcolor);
                add->setTitleBrush(QBrush(axisinfo->labelcolor));

                // grid lines, just color for now xxx todo: ticks (sigh)
                add->setGridLineColor(GColor(CPLOTGRID));
                if (charttype != GC_CHART_SCATTER && add->orientation()==Qt::Horizontal) // no x grids unless a scatter
                    add->setGridLineVisible(false);

                // add the series that are associated with this
                foreach(QAbstractSeries *series, axisinfo->series)
                    series->attachAxis(add);
            }
        }
    }

    if (charttype ==GC_CHART_PIE) {
        // pie, never want a legend
        qchart->legend()->setVisible(false);
    }

    // barseries special case
    if (charttype==GC_CHART_BAR && barseries) {

        // need to attach barseries to the value axes
        foreach(QAbstractAxis *axis, qchart->axes(Qt::Vertical))
            barseries->attachAxis(axis);
    }

    // install event filters on thes scene objects for Pie and Bar
    // charts only, since for line/scatter we select and interact via
    // collision detection (and don't want the double number of events).
    if (charttype == GC_CHART_BAR || charttype == GC_CHART_PIE) {

        // largely we just want the hover events coz they're handy
        foreach(QGraphicsItem *item, chartview->scene()->items())
            item->installSceneEventFilter(selector); // XXX create sceneitem to help us here!

    }
}

bool
GenericPlot::configureAxis(QString name, bool visible, int align, double min, double max,
                      int type, QString labelcolor, QString color, bool log, QStringList categories)
{
    AxisInfo *axis = axisinfos.value(name);
    if (axis == NULL) return false;

    // lets update the settings then
    axis->visible = visible;

    // -1 if not passed
    if (align == 0) axis->align = Qt::AlignBottom;
    if (align == 1) axis->align = Qt::AlignLeft;
    if (align == 2) axis->align = Qt::AlignTop;
    if (align == 3) axis->align = Qt::AlignRight;

    // -1 if not passed
    if (min != -1)  axis->minx = axis->miny = min;
    if (max != -1)  axis->maxx = axis->maxy = max;

    // type
    if (type != -1) axis->type = static_cast<AxisInfo::AxisInfoType>(type);

    // color
    if (labelcolor != "") axis->labelcolor=QColor(labelcolor);
    if (color != "") axis->axiscolor=QColor(color);

    // log .. hmmm
    axis->log = log;

    // categories
    if (categories.count()) axis->categories = categories;
    return true;
}
