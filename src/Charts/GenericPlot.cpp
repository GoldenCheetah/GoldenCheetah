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

#include "GenericPlot.h"
#include "GenericChart.h"

#include "Colors.h"
#include "AbstractView.h"
#include "RideFileCommand.h"
#include "RideCache.h"
#include "Utils.h"

#include "ChartSpace.h"
#include "UserChartOverviewItem.h"

#include "Voronoi.h"
#include "GenericAnnotations.h"

#include "gsl/gsl_fit.h"
#include <limits>

// used to format dates/times on axes
QString GenericPlot::gl_dateformat = QString("dd MMM yy");
QString GenericPlot::gl_timeformat = QString("hh:mm:ss");

GenericPlot::GenericPlot(QWidget *parent, Context *context, QGraphicsItem *item) : QWidget(parent), context(context), item(item)
{
    setAutoFillBackground(true);

    // set a minimum height
    setMinimumHeight(gl_minheight *dpiXFactor);

    // intitialise state info
    charttype=0;
    chartview=NULL;
    barseries=NULL;
    stackbarseries=NULL;
    percentbarseries=NULL;
    bottom=left=true;

    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
    leftLayout = new QHBoxLayout();

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

    // watch mouse hover etc on the chartview and scene
    if (item == NULL) {
        chartview->setMouseTracking(true);
        chartview->scene()->installEventFilter(this);
        installEventFilter(this);
    } else {

        // we get events from the chartspace becuase
        // when we are embedded via QGraphicsProxyWidget
        // mouse events go missing.
        item->scene()->installEventFilter(this);
    }

    // add selector
    selector = new GenericSelectTool(this);
    chartview->scene()->addItem(selector);

    // add annotations controller (which attaches to the plot)
    annotationController = new GenericAnnotationController(this);

    // the legend at the top for now
    legend = new GenericLegend(context, this);

    // add all widgets to the view
    mainLayout->addWidget(legend);
    holder=mainLayout;
    mainLayout->addLayout(leftLayout);
    leftLayout->addWidget(chartview);

    // watch for colors changing
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // get notifications when values change
    connect(selector, SIGNAL(seriesClicked(QAbstractSeries*,GPointF)), this, SLOT(seriesClicked(QAbstractSeries*,GPointF)));
    connect(selector, SIGNAL(hover(GPointF,QString,QAbstractSeries*)), legend, SLOT(setValue(GPointF,QString)));
    connect(selector, SIGNAL(unhover(QString)), legend, SLOT(unhover(QString)));
    connect(selector, SIGNAL(unhoverx()), legend, SLOT(unhoverx()));
    connect(legend, SIGNAL(clicked(QString,bool)), this, SLOT(setSeriesVisible(QString,bool)));
    connect(qchart, SIGNAL(plotAreaChanged(QRectF)), this, SLOT(plotAreaChanged()));

    // config changed...
    configChanged(0);
}

void
GenericPlot::seriesClicked(QAbstractSeries*series, GPointF point)
{
    // user clicked on a point, do we need to click thru?
    QVector<QString> fseries = filenames.value(series, QVector<QString>());
    if (point.index >= 0 && point.index < fseries.count()) {
        // click thru
        RideItem *item = context->athlete->rideCache->getRide(fseries.at(point.index));
        if (item) context->notifyRideSelected(item);
    }
}

// we can intercept events from the QT Chart's chartview or a chartspace
bool GenericPlot::eventFilter(QObject *obj, QEvent *e)
{

    if (item == NULL)  return eventHandler(0, obj, e);

    // space is non null and not busy dragging and resizing etc
    if ((e->type() == QEvent::GraphicsSceneMousePress ||
         e->type() == QEvent::GraphicsSceneMouseRelease ||
         e->type() == QEvent::GraphicsSceneMouseMove) &&

        // the line below is naughty- it assumes we are embedded via a userchartoverview item- if we
        // embed R or Python charts on the overview in the future this line will cause a SEGV- I took the
        // decision that the performance improvement on screen when dragging and moving
        // overview items was worth the filthy code. if you just delete it (or comment it out)
        // the code will still be fine, its just here to optimise out unneccessary event processing.
        static_cast<UserChartOverviewItem*>(static_cast<QGraphicsProxyWidget*>(item)->parent())->chartspace()->state == ChartSpace::NONE &&

        item->sceneBoundingRect().contains(static_cast<QGraphicsSceneMouseEvent*>(e)->scenePos())) {

        // send, as-is, they will have to map co-ordinates
        return eventHandler(1, obj, e);
    }

    return false;
}

// source 0=chart scene, 1= chart space scene
bool
GenericPlot::eventHandler(int source, void *, QEvent *e)
{
    static bool block=false;

    // don't want to get interuppted
    if (block) return false;
    else block=true;

    // where is the cursor?
    QPointF spos;
    if ((e->type() == QEvent::GraphicsSceneMousePress ||
         e->type() == QEvent::GraphicsSceneMouseRelease ||
         e->type() == QEvent::GraphicsSceneMouseMove)) {

        // map to the proxy coordinates
        if (source == 0) { // chartview coordinates already
            spos = static_cast<QGraphicsSceneMouseEvent*>(e)->scenePos();
        } else {

            // map to proxy
            spos = item->mapFromScene(static_cast<QGraphicsSceneMouseEvent*>(e)->scenePos());

            // now add in our relative coordinates wthin the item (our topleft is relative to the proxy topleft)
            spos -= chartview->geometry().topLeft();
        }
    }

    // so we want to trigger a scene update?
    bool updatescene = false;

    //
    // HANDLE EVENTS AND UPDATE STATE
    //
    switch(e->type()) {

    // mouse clicked
    case QEvent::GraphicsSceneMousePress:
    {
        updatescene = selector->clicked(spos);
    }
    break;

    // mouse released
    case QEvent::GraphicsSceneMouseRelease:
    {
        updatescene = selector->released(spos);
    }
    break;

    // mouse move
    case QEvent::GraphicsSceneMouseMove:
    {
        updatescene = selector->moved(spos);
    }
    break;

    case QEvent::GraphicsSceneWheel:
    {
        QGraphicsSceneWheelEvent *w = static_cast<QGraphicsSceneWheelEvent*>(e);
        updatescene = selector->wheel(w->delta());
    }
    break;

    // resize
    case QEvent::Resize:
        break;

    // tooltip, paused for a moment..
    case QEvent::GraphicsSceneHoverEnter:
    case QEvent::GraphicsSceneHelp:
    case QEvent::GraphicsSceneHoverMove:
        break;

    // tooltip, paused for a moment..
    case QEvent::GraphicsSceneHoverLeave:
        break;

    // tooltip, paused for a moment..
    case QEvent::ToolTip:
        break;


    default:
        //fprintf(stderr,"%s: some event %d for obj=%u\n", source ? "widget" : "scene", e->type(), (void*)obj); fflush(stderr);
        break;
    }

    //
    // UPDATE SCENE TO REFLECT STATE
    //
    if (updatescene)   selector->updateScene(); // really only do selection right now.. more to come

    // all done.
    block = false;
    return false;
}

void
GenericPlot::configChanged(qint32)
{
    // tinted palette for headings etc
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(bgcolor_));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Base, GCColor::alternateColor(bgcolor_));
    setPalette(palette);

    // chart colors
    chartview->setBackgroundBrush(QBrush(bgcolor_));
    qchart->setBackgroundBrush(QBrush(bgcolor_));
    qchart->setBackgroundPen(QPen(GColor(CPLOTMARKER)));
}

void
GenericPlot::setBackgroundColor(QColor bgcolor)
{
    this->bgcolor_ = bgcolor;
    configChanged(0);
}

void
GenericPlot::setSeriesVisible(QString name, bool visible)
{
    // find the curve
    QAbstractSeries *series = curves.value(name, NULL);
    QAbstractSeries *dec = series ? decorations.value(series) : NULL;

    // does it exist and did it change?
    if (series && series->isVisible() != visible) {

        // show/hide along with any decoration
        series->setVisible(visible);
        if (dec) dec->setVisible(visible);

        // tell selector we hid/show a series so it can respond.
        selector->setSeriesVisible(name, visible);
    }
}

// annotations


void
GenericPlot::pieHover(QPieSlice *slice, bool state)
{
    if (havelegend.count() == 0) return;
    if (state == true)  legend->setValue(GPointF(0, round(slice->percentage()*1000)/10, -1), havelegend.first());
    else legend->unhover(havelegend.first());
}

// handle hover on barset
void GenericPlot::barsetHover(bool status, int index, QBarSet *)
{
    QString category;
    if (categories.count() > index) category = categories[index];

    foreach(QBarSet *barset, barsets) {
        if (status)  legend->setValue(GPointF(0, barset->at(index), -1), barset->label(), category);
        else {
            legend->unhover(barset->label());
            legend->unhoverx();
        }
    }
}

struct axisrect {
    bool operator< (axisrect right) const { return rect.x() < right.rect.x(); }
    axisrect(QAbstractAxis *ax, QRectF ar) : rect(ar), axis(ax) {}
    QRectF rect;
    QAbstractAxis *axis;
};

// for sorting rectangles
static bool myqRectLess(const QRectF left, const QRectF right)
{
    return (left.x() < right.x());
}

// resizing, so plot area changed and likely all the scene moved
void
GenericPlot::plotAreaChanged()
{
    // we need to recalculate the axis geometries
    // since the qchart methods do not make any of
    // this public we have to search through the
    // graphic items in the axis areas and associate
    // them with an axis. Fortunately all label items
    // have a common parentItem() and are actually
    // added to the scene as QGraphicsTextItem so we
    // find them safely and group them together.
    //
    // Sadly, there is not direct link between the parentItem
    // and the axisitem but the title text is available
    // so we use that.
    //

    // Step One: Hunt for labels and turn into bounding rectangles
    QList<axisrect> ar; // axis + title rect
    QList <QRectF> lr;  // consolidated label rects

    static Qt::AlignmentFlag sides[2]= { Qt::AlignLeft, Qt::AlignRight };
    for(int i=0; i<2; i++) {

        // create a rectangle that covers the area
        // in the scene where labels will be located
        QRectF zone;
        switch (sides[i]) {
        case Qt::AlignLeft:
                zone =QRectF(QPointF(0, -10),
                      QPointF(qchart->plotArea().x(), qchart->scene()->height()));
            break;
        case Qt::AlignRight:
                zone =QRectF(QPointF(qchart->plotArea().x()+qchart->plotArea().width(), -10),
                             QPointF(qchart->scene()->width(), qchart->scene()->height()));
            break;
        default:
            break;
        }

        // temporarily keep track of label rectangles
        QMap<QGraphicsItem*, QRectF> bounds;
        foreach(QGraphicsItem *item, qchart->scene()->items(zone, Qt::ItemSelectionMode::ContainsItemShape)) {
            if (item->type() != QGraphicsTextItem::Type) continue;

            // for some reason 5.10 or earlier has errant text items we should ignore
            if (static_cast<QGraphicsTextItem*>(item)->toPlainText() == "") continue;

            // is this an axis title?
            QAbstractAxis *found=NULL;
            foreach(QAbstractAxis *axis, qchart->axes()) {
                if (axis->titleText() == static_cast<QGraphicsTextItem*>(item)->toPlainText()) {
                    found=axis;
                    break;
                }
            }

            // update the maps
            if (found) {
                ar.append(axisrect(found, QRectF(item->scenePos().x(), item->scenePos().y(),
                                            item->boundingRect().width(), item->boundingRect().height())));
            } else {
                QRectF bound = bounds.value(item->parentItem(), QRectF());
                QRectF ir = QRectF(item->scenePos().x(), item->scenePos().y(),
                                   item->boundingRect().width(), item->boundingRect().height());
                if (bound == QRectF()) bound=ir;
                else bound = bound.united(ir);
                bounds.insert(item->parentItem(), bound);
            }
        }

        // take united label rects and make a list
        QMapIterator<QGraphicsItem*, QRectF> rr(bounds);
        while(rr.hasNext()) {
            rr.next();
            lr.append(rr.value());
        }
    }

    // Step two: Sort both lists by ascending x values- both will match 1:1 by index
    //           because we're only doing y-axis- label rects and title rects have
    //           same sort order left to right (x-axis) so we can then associate
    //           the label rect at position [i] with the axisrect at position [i]
    std::sort(ar.begin(), ar.end());
    std::sort(lr.begin(), lr.end(), myqRectLess);

    axisRect.clear(); // class member that tracks axis->scene rectangle
    for(int i=0; i< ar.count() && i <lr.count(); i++) {
        axisRect.insert(ar[i].axis, lr[i]);
    }
}

bool
GenericPlot::initialiseChart(QString title, int type, bool animate, int legpos, double scale)
{
    clearAnnotations();

    // if we changed the type, all series must go
    if (charttype != type) {
        qchart->removeAllSeries();
        curves.clear();
        filenames.clear();
        barseries=NULL;
        stackbarseries=NULL;
        percentbarseries=NULL;
    }


    foreach(QLabel *label, labels) delete label;
    labels.clear();

    foreach(Quadtree *tree, quadtrees) delete tree;
    quadtrees.clear();

    foreach(GenericAxisInfo *axisinfo, axisinfos) delete axisinfo;
    axisinfos.clear();

    left=true;
    bottom=true;
    barsets.clear();
    havelegend.clear();
    this->scale_ = scale;

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

    // lets move the legend, only support top or bottom for now
    holder->removeWidget(legend);
    switch (legpos) {
    case 0: // bottom
        legend->setOrientation(Qt::Horizontal);
        mainLayout->insertWidget(-1, legend); // at end
        holder=mainLayout;
        legend->show();
        break;
    case 1: // left
        legend->setOrientation(Qt::Vertical);
        leftLayout->insertWidget(0, legend); // at beginning
        holder=leftLayout;
        legend->show();
        break;
    case 2: // top
        legend->setOrientation(Qt::Horizontal);
        mainLayout->insertWidget(0, legend); // at front
        holder=mainLayout;
        legend->show();
        break;
    case 3: // right
        legend->setOrientation(Qt::Vertical);
        leftLayout->insertWidget(-1, legend); // at end
        holder=leftLayout;
        legend->show();
        break;
    case 4: // none
        legend->hide();
        break;
    }

    // what kind of selector do we use?
    if (charttype==GC_CHART_LINE) selector->setMode(GenericSelectTool::XRANGE);
    else selector->setMode(GenericSelectTool::RECTANGLE);

    return true;
}

// rendering to qt chart
bool
GenericPlot::addCurve(QString name, QVector<double> xseries, QVector<double> yseries, QVector<QString> fseries, QString xname, QString yname,
                      QStringList labels, QStringList colors,
                      int linestyle, int symbol, int size, QString color, int opacity, bool opengl, bool legend, bool datalabels, bool fill,
                      QList<GenericAnnotationInfo> annotations)
{

    // a curve can have a decoration associated with it
    // on a line chart the decoration is the symbols
    // on a scatter chart the decoration is the line
    QString dname = QString("d_%1").arg(name);

    // standard colors are encoded 1,1,x - where x is the index into the colorset
    QColor applyColor = RGBColor(QColor(color));

    // labels font
    QFont labelsfont;
    labelsfont.setPointSizeF(labelsfont.pointSize() * scale_);

    // if curve already exists, remove it
    if (charttype==GC_CHART_LINE || charttype==GC_CHART_SCATTER || charttype==GC_CHART_PIE) {
        QAbstractSeries *existing = curves.value(name);
        if (existing) {
            qchart->removeSeries(existing);
            delete existing;
            curves.remove(name);

            QAbstractSeries *decor = decorations.value(existing);
            if (decor) {
                qchart->removeSeries(decor);
                delete decor;
                decorations.remove(existing);
            }
        }
    }

    // want a legend?
    if (legend) havelegend << name;

    // lets find that axis - even blank ones
    GenericAxisInfo *xaxis, *yaxis;
    xaxis=axisinfos.value(xname);
    yaxis=axisinfos.value(yname);
    if (xaxis==NULL) {
        xaxis=new GenericAxisInfo(Qt::Horizontal, xname);

        // default alignment toggles
        xaxis->align = bottom ? Qt::AlignBottom : Qt::AlignTop;
        bottom = !bottom;

        // use marker color for x axes
        xaxis->labelcolor = xaxis->axiscolor = GColor(CPLOTMARKER);

        // add to list
        axisinfos.insert(xname, xaxis);

    }
    if (yaxis==NULL) {
        yaxis=new GenericAxisInfo(Qt::Vertical, yname);

        // default alignment toggles
        yaxis->align = left ? Qt::AlignLeft : Qt::AlignRight;
        left = !left;

        // yaxis color matches, but not done for xaxis above
        yaxis->labelcolor = yaxis->axiscolor = applyColor;

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

            // get setup for click thru
            connect(add, SIGNAL(clicked(QPointF)), selector, SLOT(seriesClicked())); // catch series clicks
            filenames.insert(add, fseries);

            // aesthetics
            add->setBrush(Qt::NoBrush);
            QPen pen(applyColor);
            pen.setStyle(static_cast<Qt::PenStyle>(linestyle));
            pen.setWidth(size * scale_);
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
            qchart->setDropShadowEnabled(false);

            // no line, we are invisible
            if (linestyle == 0) add->setVisible(false);
            else {
                if (datalabels) {
                    add->setPointLabelsFont(labelsfont);
                    add->setPointLabelsVisible(true);    // is false by default
                    add->setPointLabelsColor(applyColor);
                    add->setPointLabelsFormat("@yPoint");
                }
                // fill curve?
            }

            if (fill) {

                // for fill effect we create an area series
                QAreaSeries *area = new QAreaSeries(add);
                area->setName(name);

                QPen pen(applyColor);
                pen.setStyle(static_cast<Qt::PenStyle>(linestyle));
                pen.setWidth(size);
                area->setPen(pen);

                QColor col(applyColor);
                col.setAlpha(64);
                QBrush brush(col, Qt::SolidPattern);
                area->setBrush(brush);

                qchart->addSeries(area);
                curves.insert(name,area);
                xaxis->series.append(area);
                yaxis->series.append(area);

            } else {

                // normal line series
                qchart->addSeries(add);
                curves.insert(name,add);
                xaxis->series.append(add);
                yaxis->series.append(add);
            }


            // so do we need to decorate with a symbol?
            if (symbol > 0) {

                // set up the curves
                QScatterSeries *dec = new QScatterSeries();
                dec->setName(dname);

                // data
                for (int i=0; i<xseries.size() && i<yseries.size(); i++)
                    dec->append(xseries.at(i), yseries.at(i));

                // if no line, but we still want labels then show
                // for our data points
                if (linestyle == 0 && datalabels) {
                    add->setPointLabelsFont(labelsfont);
                    dec->setPointLabelsVisible(true);    // is false by default
                    dec->setPointLabelsColor(applyColor);
                    dec->setPointLabelsFormat("@yPoint");
                }

                // aesthetics
                if (symbol == 1) dec->setMarkerShape(QScatterSeries::MarkerShapeCircle);
                else if (symbol == 2)  dec->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
                dec->setMarkerSize(size*6*scale_);
                QColor col=applyColor;
                dec->setBrush(QBrush(col));
                dec->setPen(Qt::NoPen);
                dec->setOpacity(double(opacity) / 100.0); // 0-100% to 0.0-1.0 values

                // hardware support?
                chartview->setRenderHint(QPainter::Antialiasing);
                dec->setUseOpenGL(opengl); // for scatter or line only apparently
                qchart->setDropShadowEnabled(false);

                // chart
                qchart->addSeries(dec);

                // add to list of curves
                decorations.insert(add,dec);
                xaxis->decorations.append(dec);
                yaxis->decorations.append(dec);
            }
        }
        break;

    case GC_CHART_SCATTER:
        {
            // set up the curves
            QScatterSeries *add = new QScatterSeries();
            add->setName(name);

            // handle click thru
            connect(add, SIGNAL(clicked(QPointF)), selector, SLOT(seriesClicked())); // catch series clicks
            filenames.insert(add, fseries);

            // aesthetics
            if (symbol == 0) add->setVisible(false); // no marker !
            else if (symbol == 1) add->setMarkerShape(QScatterSeries::MarkerShapeCircle);
            else if (symbol == 2)  add->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
            add->setMarkerSize(size*scale_);
            QColor col=applyColor;
            add->setBrush(QBrush(col));
            add->setPen(Qt::NoPen);
            add->setOpacity(double(opacity) / 100.0); // 0-100% to 0.0-1.0 values

            // data
            GenericCalculator calc; // watching as we add
            for (int i=0; i<xseries.size() && i<yseries.size(); i++) {
                add->append(xseries.at(i), yseries.at(i));

                // tell axis about the data
                xaxis->point(xseries.at(i), yseries.at(i));
                yaxis->point(xseries.at(i), yseries.at(i));

                // calculate stuff
                calc.addPoint(QPointF(xseries.at(i), yseries.at(i)));

            }

            if (datalabels) {
                add->setPointLabelsFont(labelsfont);
                add->setPointLabelsVisible(true);    // is false by default
                add->setPointLabelsColor(applyColor);
                add->setPointLabelsFormat("@yPoint");
            }

            // set the quadtree up - now we know the ranges...
            Quadtree *tree = new Quadtree(QPointF(calc.x.min, calc.y.min), QPointF(calc.x.max, calc.y.max));
            for (int i=0; i<xseries.size() && i<yseries.size(); i++)
                if (xseries.at(i) != 0 && yseries.at(i) != 0) // 0,0 is common and lets ignore (usually means no data)
                    tree->insert(GPointF(xseries.at(i), yseries.at(i), i));

            if (tree->nodes.count() || tree->root->contents.count()) quadtrees.insert(add, tree);

            // hardware support?
            chartview->setRenderHint(QPainter::Antialiasing);
            add->setUseOpenGL(opengl); // for scatter or line only apparently
            qchart->setDropShadowEnabled(false);

            // chart
            qchart->addSeries(add);

            // add to list of curves
            curves.insert(name,add);
            xaxis->series.append(add);
            yaxis->series.append(add);

            if (linestyle > 0) {
                // set up the curves
                QLineSeries *dec = new QLineSeries();
                dec->setName(dname);

                // aesthetics
                dec->setBrush(Qt::NoBrush);
                QPen pen(applyColor);
                pen.setStyle(static_cast<Qt::PenStyle>(linestyle));
                pen.setWidth(size*scale_);
                dec->setPen(pen);
                dec->setOpacity(double(opacity) / 100.0); // 0-100% to 0.0-1.0 values

                // data
                for (int i=0; i<xseries.size() && i<yseries.size(); i++)
                    dec->append(xseries.at(i), yseries.at(i));

                // hardware support?
                chartview->setRenderHint(QPainter::Antialiasing);
                dec->setUseOpenGL(opengl); // for scatter or line only apparently
                qchart->setDropShadowEnabled(false);

                // chart
                qchart->addSeries(dec);

                // add to list of curves
                decorations.insert(add,dec);
                xaxis->decorations.append(dec);
                yaxis->decorations.append(dec);
            }
        }
        break;

    case GC_CHART_BAR:
    case GC_CHART_STACK:
    case GC_CHART_PERCENT:
        {
            // set up the barsets
            QBarSet *add= new QBarSet(name);

            // aesthetics
            add->setBrush(QBrush(applyColor));
            if (charttype == GC_CHART_STACK) {
                QPen outline(bgcolor_);
                outline.setWidth(4 * scale_); // space between blocks
                add->setPen(outline);
            } else {
                add->setPen(Qt::NoPen);
            }

            // data and min/max values- works fine for bar chart
            // but for stacked bar we need to get max of sum so
            // we do that in the finaliseChart section
            for (int i=0; i<yseries.size(); i++) {
                double value = yseries.at(i);
                *add << value;

                // don't try and calculate here in a stack chart- we need
                // all the barsets to have been defined first
                if (charttype != GC_CHART_STACK && charttype != GC_CHART_PERCENT) yaxis->point(i,value);
                xaxis->point(i,value);
            }

            // we are very particular regarding axis
            yaxis->type = GenericAxisInfo::CONTINUOUS;
            xaxis->type = GenericAxisInfo::CATEGORY;

            // shadows on bar charts
            qchart->setDropShadowEnabled(false);

            // add to list of barsets
            barsets << add;
        }
        break;

    case GC_CHART_PIE:
        {
            // set up the curves
            QPieSeries *add = new QPieSeries();
            havelegend << name.append(" %");
            connect(add, SIGNAL(hovered(QPieSlice*,bool)), this, SLOT(pieHover(QPieSlice*,bool)));
            add->setPieSize(0.7);
            add->setHoleSize(0.5);

            // setup the slices
            for(int i=0; i<yseries.size(); i++) {
                // get label?
                if (i>=labels.size())
                    add->append(QString("%1").arg(i), yseries.at(i));
                else {
                    if (labels.at(i) == "")add->append("(blank)", yseries.at(i));
                    else add->append(labels.at(i), yseries.at(i));
                }
            }

            // now do the colors
            int i=0;
            foreach(QPieSlice *slice, add->slices()) {

                slice->setExploded();
                slice->setLabelVisible();
                slice->setLabelBrush(QBrush(GColor(CPLOTMARKER)));
                slice->setPen(Qt::NoPen);
                if (i <colors.size()) slice->setBrush(QColor(colors.at(i)));
                else slice->setBrush(Qt::red);
                i++;
            }

            // shadows on pie
            qchart->setDropShadowEnabled(false);

            // set the pie chart
            qchart->addSeries(add);

            // add to list of curves
            curves.insert(name,add);
        }
        break;

    }

    // add all the annotations via a genericseriesinfo structure, which is a bit crap since we get
    // passed the individual parts from one of these, they we create one down here. Its because
    // the R and Python charts don't have this and we want to isolate annotations right at the top
    // in UserChart and then pass them all the way down to here
    if (annotations.count()) {

        // we keep a record of them, they get added in finalise (after the axes are all resolved)
        annotationinfos << GenericSeriesInfo(name, xseries, yseries, fseries, xname, yname, labels, colors,
                          linestyle, symbol, size, color, opacity, opengl, legend, datalabels, fill, RideMetric::Average, annotations);
    }

    return true;
}

// once python script has run polish the chart, fixup axes/ranges and so on.
void
GenericPlot::finaliseChart()
{
    if (!qchart) return;

    // clear ALL axes
    foreach(QAbstractAxis *axis, qchart->axes(Qt::Vertical)) {
        qchart->removeAxis(axis);
        delete axis;
    }
    foreach(QAbstractAxis *axis, qchart->axes(Qt::Horizontal)) {
        qchart->removeAxis(axis);
        delete axis;
    }

    // clear the legend
    legend->removeAllSeries();

    // basic aesthetics
    qchart->legend()->setMarkerShape(QLegend::MarkerShapeRectangle);
    qchart->setDropShadowEnabled(false);

    // no more than 1 category axis since barsets are all assigned.
    bool donecategory=false;

    // only now can we calculate the max height for a stack chart
    double maxystack=0;
    if (charttype == GC_CHART_STACK && barsets.count()) {

        bool havemore=true;
        for(int index=0; havemore; index++) {
            double maxcalc=0;
            // calculate height of stack from constituent parts
            foreach(QBarSet *bs, barsets) {
                if (bs->count() > index) maxcalc += bs->at(index);
                else {
                    havemore=false;
                    break;
                }
            }
            if (maxcalc > maxystack) maxystack = maxcalc;
        }
    }

    // Create axes - for everyone except pie charts that don't have any
    if (charttype != GC_CHART_PIE) {
        // create desired axis
        foreach(GenericAxisInfo *axisinfo, axisinfos) {
//fprintf(stderr, "Axis: %s, orient:%s, type:%d\n",axisinfo->name.toStdString().c_str(),axisinfo->orientation==Qt::Vertical?"vertical":"horizontal",(int)axisinfo->type);
//fflush(stderr);
            QAbstractAxis *add=NULL;
            switch (axisinfo->type) {
            case GenericAxisInfo::DATERANGE:
                {

                    QDateTimeAxis *vaxis = new QDateTimeAxis(qchart);
                    add=vaxis; // gets added later

                    vaxis->setMin(QDateTime::fromMSecsSinceEpoch(axisinfo->min()));
                    vaxis->setMax(QDateTime::fromMSecsSinceEpoch(axisinfo->max()));

                    // attach to the chart
                    qchart->addAxis(add, axisinfo->locate());
                    vaxis->setFormat(GenericPlot::gl_dateformat);
                }
                break;
            case GenericAxisInfo::TIME:
                {
                    QDateTimeAxis *vaxis = new QDateTimeAxis(qchart);
                    add=vaxis; // gets added later

                    vaxis->setMin(QDateTime::fromMSecsSinceEpoch(axisinfo->min()));
                    vaxis->setMax(QDateTime::fromMSecsSinceEpoch(axisinfo->max()));

                    // attach to the chart
                    qchart->addAxis(add, axisinfo->locate());
                    vaxis->setFormat(GenericPlot::gl_timeformat);
                }
                break;
            case GenericAxisInfo::CONTINUOUS:
                {
                    QAbstractAxis *vaxis=NULL;
                    if (axisinfo->log) vaxis= new QLogValueAxis(qchart);
                    else vaxis= new QValueAxis(qchart);
                    add=vaxis; // gets added later

                    // we finally get to set the max value for stacked charts
                    // but only of not already passed by user
                    if (charttype == GC_CHART_STACK && axisinfo->max() <maxystack) vaxis->setMax(maxystack);
                    else vaxis->setMax(axisinfo->max());
                    vaxis->setMin(axisinfo->min());

                    if (charttype == GC_CHART_PERCENT) {
                        vaxis->setMax(100);
                        vaxis->setMin(0);
                    }

                    // attach to the chart
                    qchart->addAxis(add, axisinfo->locate());
                }
                break;
            case GenericAxisInfo::CATEGORY:
                {
                    if (!donecategory) {

                        donecategory=true;

                        QBarCategoryAxis *caxis = new QBarCategoryAxis(qchart);
                        add=caxis;

                        // add the bar series
                        if (charttype == GC_CHART_STACK) {

                            if (!stackbarseries) {
                                stackbarseries = new QStackedBarSeries();
                                qchart->addSeries(stackbarseries);

                                // connect hover events
                                connect(stackbarseries, SIGNAL(hovered(bool,int,QBarSet*)), this, SLOT(barsetHover(bool,int,QBarSet*)));

                            } else stackbarseries->clear();

                            // add the new barsets
                            foreach (QBarSet *bs, barsets)
                                stackbarseries->append(bs);

                            // attach before addig barseries
                            qchart->addAxis(add, axisinfo->locate());

                            // attach to category axis
                            stackbarseries->attachAxis(caxis);

                        } else if (charttype == GC_CHART_PERCENT) {

                            if (!percentbarseries) {
                                percentbarseries = new QPercentBarSeries();
                                qchart->addSeries(percentbarseries);

                                // connect hover events
                                connect(percentbarseries, SIGNAL(hovered(bool,int,QBarSet*)), this, SLOT(barsetHover(bool,int,QBarSet*)));

                            } else percentbarseries->clear();

                            // add the new barsets
                            foreach (QBarSet *bs, barsets)
                                percentbarseries->append(bs);

                            // attach before addig barseries
                            qchart->addAxis(add, axisinfo->locate());

                            // attach to category axis
                            percentbarseries->attachAxis(caxis);

                        } else {

                            // Bar and Pie
                            if (!barseries) {
                                barseries = new QBarSeries();
                                qchart->addSeries(barseries);

                                // connect hover events
                                connect(barseries, SIGNAL(hovered(bool,int,QBarSet*)), this, SLOT(barsetHover(bool,int,QBarSet*)));

                            } else barseries->clear();

                            // add the new barsets
                            foreach (QBarSet *bs, barsets)
                                barseries->append(bs);

                            // attach before addig barseries
                            qchart->addAxis(add, axisinfo->locate());

                            // attach to category axis
                            barseries->attachAxis(caxis);
                        }

                        // category labels
                        for(int i=axisinfo->categories.count(); i<=axisinfo->maxx; i++)
                            axisinfo->categories << QString("%1").arg(i+1);
                        // set blank to "(blank)"
                        for(int i=0; i<axisinfo->categories.count(); i++)
                            if (axisinfo->categories.at(i) == "") axisinfo->categories[i]="(blank)";
                        caxis->setCategories(axisinfo->categories);
                        categories = axisinfo->categories;
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
                stGiles.setPointSizeF(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt() * scale_);
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

                foreach(QAbstractSeries *series, axisinfo->series)
                    series->attachAxis(add);
                foreach(QAbstractSeries *series, axisinfo->decorations)
                    series->attachAxis(add);
            }
        }
    }

    if (charttype == GC_CHART_SCATTER || charttype == GC_CHART_LINE
        || charttype == GC_CHART_BAR || charttype == GC_CHART_STACK || charttype == GC_CHART_PERCENT) {

        bool havexaxis=false;
        foreach(QAbstractSeries *series, qchart->series()) {
            // manufactured series to show X axis values
            // pick the first X-axis we find and add it
            // using the axis units
            if (havexaxis == false) {

                // look for first series with a horizontal axis (we will use this later)
                foreach(QAbstractAxis *axis, series->attachedAxes()) {
                    if (axis->orientation() == Qt::Horizontal) {
                        if (axis->type()==QAbstractAxis::AxisTypeValue)  legend->addX(static_cast<QValueAxis*>(axis)->titleText(), false, "");
                        else if (axis->type()==QAbstractAxis::AxisTypeLogValue)  legend->addX(static_cast<QLogValueAxis*>(axis)->titleText(), false, "");
                        else if (axis->type()==QAbstractAxis::AxisTypeDateTime)  {
                            QDateTimeAxis *dta = static_cast<QDateTimeAxis*>(axis);
                            legend->addX(dta->titleText(), true, dta->format());
                        }
                        havexaxis=true;
                        break;
                    }
                }
            }

            // add to the legend
            if (havelegend.contains(series->name())) legend->addSeries(series->name(), GenericPlot::seriesColor(series));

        }

    }

    if (charttype== GC_CHART_PIE) {
        foreach(QString name, havelegend)  legend->addSeries(name, GColor(CPLOTMARKER));
        legend->setClickable(false);
    }


    // barseries special case - Bar chart
    if (charttype==GC_CHART_BAR && barseries) {

        // need to attach barseries to the value axes
        foreach(QAbstractAxis *axis, qchart->axes(Qt::Vertical))
            barseries->attachAxis(axis);

        // add first X axis we find
        foreach(QAbstractAxis *axis, qchart->axes(Qt::Horizontal)) {
            legend->addX(static_cast<QCategoryAxis*>(axis)->titleText(), false, "");
            break;
        }

        // and legend
        foreach(QBarSet *set, barsets)
            legend->addSeries(set->label(), set->color());

        legend->setClickable(false);
    }
    // stacked bar uses stackbarseries, but otherwise very similar
    if (charttype==GC_CHART_STACK && stackbarseries) {

        // add first X axis we find
        foreach(QAbstractAxis *axis, qchart->axes(Qt::Horizontal)) {
            legend->addX(static_cast<QCategoryAxis*>(axis)->titleText(), false, "");
            break;
        }

        // need to attach stackbarseries to the value axes
        foreach(QAbstractAxis *axis, qchart->axes(Qt::Vertical))
            stackbarseries->attachAxis(axis);

        // and legend
        foreach(QBarSet *set, barsets)
            legend->addSeries(set->label(), set->color());

        legend->setClickable(false);
    }

    // percent bar uses stackbarseries, but otherwise very similar
    if (charttype==GC_CHART_PERCENT && percentbarseries) {

        // add first X axis we find
        foreach(QAbstractAxis *axis, qchart->axes(Qt::Horizontal)) {
            legend->addX(static_cast<QCategoryAxis*>(axis)->titleText(), false, "");
            break;
        }

        // need to attach stackbarseries to the value axes
        foreach(QAbstractAxis *axis, qchart->axes(Qt::Vertical))
            percentbarseries->attachAxis(axis);

        // and legend
        foreach(QBarSet *set, barsets)
            legend->addSeries(set->label(), set->color());

        legend->setClickable(false);
    }

    // install event filters on thes scene objects for Pie and Bar
    // charts only, since for line/scatter we select and interact via
    // collision detection (and don't want the double number of events).
    if (charttype == GC_CHART_STACK || charttype == GC_CHART_STACK || charttype == GC_CHART_BAR || charttype == GC_CHART_PIE) {

        // largely we just want the hover events coz they're handy
        foreach(QGraphicsItem *item, chartview->scene()->items())
            item->installSceneEventFilter(selector); // XXX create sceneitem to help us here!

    }

    // now finally we can add annotations to the top
    foreach(GenericSeriesInfo series, annotationinfos) plotAnnotations(series);

    plotAreaChanged(); // make sure get updated before paint
}

bool
GenericPlot::configureAxis(QString name, bool visible, int align, double min, double max,
                      int type, QString labelcolor, QString color, bool log, QStringList categories)
{
    GenericAxisInfo *axis = axisinfos.value(name);
    if (axis == NULL) return false;

    // lets update the settings then
    axis->visible = visible;

    // -1 if not passed
    if (align == 0) axis->align = Qt::AlignBottom;
    if (align == 1) axis->align = Qt::AlignLeft;
    if (align == 2) axis->align = Qt::AlignTop;
    if (align == 3) axis->align = Qt::AlignRight;

    // -1 if not passed
    if (min == -1) {

        // automatically set the start/stop values for this axis- based upon
        // the data in all the series attached to the axis
        bool usey = axis->orientation == Qt::Vertical;
        min=0;
        bool setmin=false;
        // min should be minimum value for all attached series
        foreach(QAbstractSeries *series, axis->series) {
            if (series->type() == QAbstractSeries::SeriesType::SeriesTypeScatter ||
                series->type() == QAbstractSeries::SeriesType::SeriesTypeBar ||
                series->type() == QAbstractSeries::SeriesType::SeriesTypeLine) {
                foreach(QPointF point, static_cast<QXYSeries*>(series)->pointsVector()) {
                    if (usey) {
                        if (setmin && point.y() < min) min=point.y();
                        else if (!setmin) { min=point.y(); setmin=true; }
                    } else {
                        if (setmin && point.x() < min) min=point.x();
                        else if (!setmin) { min=point.x(); setmin=true; }
                    }
                }
            }
        }
    }
    axis->minx = axis->miny = min;

    if (max == -1) {

        // automatically set the start/stop values for this axis- based upon
        // the data in all the series attached to the axis
        bool usey = axis->orientation == Qt::Vertical;
        max=0;
        bool setmax=false;

        if (charttype != GC_CHART_STACK && charttype != GC_CHART_PERCENT) { // we insist on stacked being oriented this way

            // min should be minimum value for all attached series
            foreach(QAbstractSeries *series, axis->series) {
                if (series->type() == QAbstractSeries::SeriesType::SeriesTypeScatter ||
                    series->type() == QAbstractSeries::SeriesType::SeriesTypeBar ||
                    series->type() == QAbstractSeries::SeriesType::SeriesTypeLine) {
                    foreach(QPointF point, static_cast<QXYSeries*>(series)->pointsVector()) {
                        if (usey) {
                            if (setmax && point.y() > max) max=point.y();
                            else if (!setmax) { max=point.y(); setmax=true; }
                        } else {
                            if (setmax && point.x() > max) max=point.x();
                            else if (!setmax) { max=point.x(); setmax=true; }
                        }
                    }
                }
            }
        }
    }
    axis->maxx = axis->maxy = max;

    // type
    if (type != -1) axis->type = static_cast<GenericAxisInfo::AxisInfoType>(type);
    else {
        if (axis->orientation == Qt::Horizontal) {
            // infer x-axis type from name, helps a little...
            if (!axis->name.compare(tr("date"), Qt::CaseInsensitive)) axis->type = GenericAxisInfo::DATERANGE;
            if (!axis->name.compare(tr("time"), Qt::CaseInsensitive)) axis->type = GenericAxisInfo::TIME;
        }
    }

    // color
    if (labelcolor != "") axis->labelcolor=RGBColor(QColor(labelcolor));
    if (color != "") axis->axiscolor=RGBColor(QColor(color));

    // log ..
    axis->log = log;
    if (min==0 && log) min = 1; // 0 are no-no on log axes
    if (max==0 && log) max = 1; // 0 are no-no on log axes

    // categories
    if (categories.count()) axis->categories = categories;

    return true;
}

QColor
GenericPlot::seriesColor(QAbstractSeries* series)
{
    switch (series->type()) {
    case QAbstractSeries::SeriesTypeScatter: return static_cast<QScatterSeries*>(series)->color(); break;
    case QAbstractSeries::SeriesTypeLine: return static_cast<QLineSeries*>(series)->color(); break;
    case QAbstractSeries::SeriesTypeArea: return static_cast<QAreaSeries*>(series)->color(); break;
    default: return GColor(CPLOTMARKER);
    }
}

// when we plot annotations we need the curve context, so we get that as a genericseriesinfo...
void
GenericPlot::plotAnnotations(GenericSeriesInfo &seriesinfo)
{

    foreach(GenericAnnotationInfo annotation, seriesinfo.annotations) {

        switch(annotation.type) {

        case GenericAnnotationInfo::Label:
        {
            QFont std;
            std.setPointSizeF(std.pointSizeF() * scale_);
            QFontMetrics fm(std);

            QLabel *add = new QLabel(this);
            QString string = annotation.labels.join(" ");
            add->setFont(std);
            add->setText(string);
            add->setStyleSheet(QString("color: %1").arg(RGBColor(QColor(seriesinfo.color)).name()));
            add->setFixedWidth(fm.boundingRect(string).width() + (25*dpiXFactor));
            add->setAlignment(Qt::AlignCenter);
            legend->addLabel(add);
            labels << add;
        }
        break;

        case GenericAnnotationInfo::LR:
        {
            QAbstractSeries *curve=curves.value(seriesinfo.name,NULL);

            if (curve == NULL || !curve->isVisible()) return; // big nope

            // what can we see?
            double minx=0, maxx=0, miny=0,maxy=0;
            if (curve) {
                foreach(QAbstractAxis *axis, curve->attachedAxes()) {
                    if (axis->orientation() == Qt::Horizontal) { minx = qtchartaxismin(axis); maxx = qtchartaxismax(axis); }
                    else { miny = qtchartaxismin(axis); maxy = qtchartaxismax(axis); }
                }
            }

            // fit
            GenericCalculator calc;
            calc.initialise();

            // pass all the points- as they are on the plot
            for (int i=0; i< seriesinfo.xseries.count() && i < seriesinfo.yseries.count(); i++) {
                QPointF p(seriesinfo.xseries.at(i), seriesinfo.yseries.at(i));
                if (p.x() < minx || p.x() > maxx || p.y() < miny || p.y() > maxy) continue;
                calc.addPoint(p);
            }
            calc.finalise();

            double slope = calc.m;
            double intercept = calc.b;

            // now again, but converting from the chart "units"
            // calc is smart and can work back from the axis type
            calc.initialise();
            if (curve) {
                foreach(QAbstractAxis *axis, curve->attachedAxes()) {
                    if (axis->orientation() == Qt::Horizontal) calc.xaxis=axis;
                    else calc.yaxis=axis;
                }
            }

            // pass all the points- but now they are converted according to axis
            for (int i=0; i< seriesinfo.xseries.count() && i < seriesinfo.yseries.count(); i++) {
                QPointF p(seriesinfo.xseries.at(i), seriesinfo.yseries.at(i));
                if (p.x() < minx || p.x() > maxx || p.y() < miny || p.y() > maxy) continue;
                calc.addPoint(p);
            }
            calc.finalise();

            // find the x-axis
            GenericLR *lr = new GenericLR(this->annotationController);
            annotationController->addAnnotation(lr);
            lr->setColor(QColor(annotation.color));
            lr->setStyle(annotation.linestyle);
            lr->setParms(slope, intercept); // in chart "units" from first pass
            lr->setText(QString("y= %2x + %1 R2= %3").arg(calc.b, 0, 'f', 3).arg(calc.m, 0, 'f', 3).arg(calc.r2, 0, 'f', 3));
            lr->setCurve(curves.value(seriesinfo.name,NULL));

            annotations << lr;

            // add a label for the stats
            QFont std;
            std.setPointSizeF(std.pointSizeF() * scale_);
            QFontMetrics fm(std);

            QLabel *add = new QLabel(this);
            add->setFont(std);
            add->setText(lr->text());
            add->setStyleSheet(QString("color: %1").arg(RGBColor(QColor(seriesinfo.color)).name()));
            add->setFixedWidth(fm.boundingRect(lr->text()).width() + (25*dpiXFactor));
            add->setAlignment(Qt::AlignCenter);
            legend->addLabel(add);
            labels << add;
        }
        break;

        case GenericAnnotationInfo::Voronoi:
        {
            if (annotation.vx.count() < 2) return;

            Voronoi v;
            for(int i=0; i<annotation.vx.count(); i++)  {
                QPointF point(annotation.vx[i],annotation.vy[i]);
                v.addSite(point);
            }
            v.run(QRectF());
#if 0
    // how many lines?
    fprintf(stderr, "voronoi diagram curve '%s' has %d lines\n", annotation.vname.toStdString().c_str(),v.lines().count()); fflush(stderr);

    foreach(QLineF line, v.lines()) {
        fprintf(stderr, "from %f,%f to %f,%f\n", line.p1().x(), line.p1().y(), line.p2().x(), line.p2().y());
    }
#endif

            // create a new diagram
            GenericLines *voronoidiagram = new GenericLines(this->annotationController);
            annotationController->addAnnotation(voronoidiagram);
            voronoidiagram->setCurve(curves.value(seriesinfo.name,NULL));
            voronoidiagram->setLines(v.lines());

            annotations << voronoidiagram;
        }
        break;


        case GenericAnnotationInfo::VLine:
        case GenericAnnotationInfo::HLine:
        {
            StraightLine *line = new StraightLine(this->annotationController);
            annotationController->addAnnotation(line);
            line->setCurve(curves.value(seriesinfo.name,NULL));
            line->setValue(annotation.value);
            line->setText(annotation.text);
            line->setOrientation(annotation.type == GenericAnnotationInfo::VLine ? Qt::Vertical : Qt::Horizontal);
            line->setStyle(annotation.linestyle);

            annotations << line;
        }
        break;
        }
    }
}

void
GenericPlot::clearAnnotations()
{
    // labels are a form of annotation, but are placed in the
    // legend not drawn on the plot
    foreach(QLabel *label, labels) {
        legend->removeLabel(label);
        delete label;
    }
    labels.clear();

    // annotations are painted onto the chart scene
    // we zap all here
    foreach(GenericAnnotation *annotation, annotations) {
        annotationController->removeAnnotation(annotation);
        delete annotation;
    }
    annotations.clear();
    annotationinfos.clear();
}
