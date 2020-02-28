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

#include "Colors.h"
#include "TabView.h"
#include "RideFileCommand.h"
#include "Utils.h"

#include <limits>

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

    // watch mouse hover etc on the chartview and scene
    chartview->setMouseTracking(true);
    chartview->scene()->installEventFilter(this);

    // add selector
    selector = new GenericSelectTool(this);
    chartview->scene()->addItem(selector);

    // the legend at the top for now
    legend = new GenericLegend(context, this);

    // filter ESC so we can stop scripts
    installEventFilter(this);

    // add all widgets to the view
    mainLayout->addWidget(legend);
    mainLayout->addWidget(chartview);

    // watch for colors changing
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // get notifications when values change
    connect(selector, SIGNAL(hover(QPointF,QString,QAbstractSeries*)), legend, SLOT(hover(QPointF,QString,QAbstractSeries*)));
    connect(selector, SIGNAL(unhover(QString)), legend, SLOT(unhover(QString)));
    connect(selector, SIGNAL(unhoverx()), legend, SLOT(unhoverx()));
    connect(legend, SIGNAL(clicked(QString,bool)), this, SLOT(setSeriesVisible(QString,bool)));

    // config changed...
    configChanged(0);
}

// source 0=scene, 1=widget
bool
GenericPlot::eventHandler(int, void *, QEvent *e)
{
    static bool block=false;

    // don't want to get interuppted
    if (block) return false;
    else block=true;

    // lets get some basic info first
    // where is the cursor?
    //QPoint wpos=cursor().pos();
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
        //fprintf(stderr,"POS: %g:%g \n", spos.x(), spos.y());
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
        //fprintf(stderr,"POS: %f:%f\n", spos.x(), spos.y());
        //fprintf(stderr,"%s: mouse MOVE for obj=%u\n", source ? "widget" : "scene", (void*)obj); fflush(stderr);
        {
            // see if selection tool cares about new mouse position
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
        //fprintf(stderr,"%s: HOVER scene item for obj=%u\n", source ? "widget" : "scene", (void*)obj); fflush(stderr);
    }
    break;

    // tooltip, paused for a moment..
    case QEvent::GraphicsSceneHoverLeave: {

        // debug info
        //fprintf(stderr,"%s: UNHOVER scene item for obj=%u\n", source ? "widget" : "scene", (void*)obj); fflush(stderr);
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

void
GenericPlot::setSeriesVisible(QString name, bool visible)
{
    // find the curve
    QAbstractSeries *series = curves.value(name, NULL);

    // does it exist and did it change?
    if (series && series->isVisible() != visible) {

        // show/hide
        series->setVisible(visible);

        // tell selector we hid/show a series so it can respond.
        selector->setSeriesVisible(name, visible);
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

    foreach(Quadtree *tree, quadtrees) delete tree;
    quadtrees.clear();

    foreach(GenericAxisInfo *axisinfo, axisinfos) delete axisinfo;
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

    // what kind of selector do we use?
    if (charttype==GC_CHART_LINE) selector->setMode(GenericSelectTool::XRANGE);
    else selector->setMode(GenericSelectTool::RECTANGLE);

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
            GenericCalculator calc; // watching as we add
            for (int i=0; i<xseries.size() && i<yseries.size(); i++) {
                add->append(xseries.at(i), yseries.at(i));

                // tell axis about the data
                xaxis->point(xseries.at(i), yseries.at(i));
                yaxis->point(xseries.at(i), yseries.at(i));

                // calculate stuff
                calc.addPoint(QPointF(xseries.at(i), yseries.at(i)));

            }

            // set the quadtree up - now we know the ranges...
            Quadtree *tree = new Quadtree(QPointF(calc.x.min, calc.y.min), QPointF(calc.x.max, calc.y.max));
            for (int i=0; i<xseries.size() && i<yseries.size(); i++)
                if (xseries.at(i) != 0 && yseries.at(i) != 0) // 0,0 is common and lets ignore (usually means no data)
                    tree->insert(QPointF(xseries.at(i), yseries.at(i)));

            if (tree->nodes.count()) quadtrees.insert(add, tree);

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
            yaxis->type = GenericAxisInfo::CONTINUOUS;
            xaxis->type = GenericAxisInfo::CATEGORY;

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

    // Create axes - for everyone except pie charts that don't have any
    if (charttype != GC_CHART_PIE) {
        // create desired axis
        foreach(GenericAxisInfo *axisinfo, axisinfos) {
//fprintf(stderr, "Axis: %s, orient:%s, type:%d\n",axisinfo->name.toStdString().c_str(),axisinfo->orientation==Qt::Vertical?"vertical":"horizontal",(int)axisinfo->type);
//fflush(stderr);
            QAbstractAxis *add=NULL;
            switch (axisinfo->type) {
            case GenericAxisInfo::DATERANGE: // TODO
            case GenericAxisInfo::TIME:      // TODO
            case GenericAxisInfo::CONTINUOUS:
                {
                    QValueAxis *vaxis= new QValueAxis(qchart);
                    add=vaxis; // gets added later

                    vaxis->setMin(axisinfo->min());
                    vaxis->setMax(axisinfo->max());

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

    if (charttype == GC_CHART_SCATTER || charttype == GC_CHART_LINE) {

        bool havexaxis=false;
        foreach(QAbstractSeries *series, qchart->series()) {
            // manufactured series to show X axis values
            // pick the first X-axis we find and add it
            // using the axis units
            if (havexaxis == false) {

                // look for first series with a horizontal axis (we will use this later)
                foreach(QAbstractAxis *axis, series->attachedAxes()) {
                    if (axis->orientation() == Qt::Horizontal && axis->type()==QAbstractAxis::AxisTypeValue) {
                        legend->addX(static_cast<QValueAxis*>(axis)->titleText());
                        havexaxis=true;
                        break;
                    }
                }
            }

            // add to the legend xxx might make some hidden?
            legend->addSeries(series->name(), series);

        }

        legend->show();
    }

    if (charttype ==GC_CHART_PIE || charttype== GC_CHART_BAR) { //XXX bar chart legend?
        // pie, never want a legend
        legend->hide();
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
    if (min != -1)  axis->minx = axis->miny = min;
    if (max != -1)  axis->maxx = axis->maxy = max;

    // type
    if (type != -1) axis->type = static_cast<GenericAxisInfo::AxisInfoType>(type);

    // color
    if (labelcolor != "") axis->labelcolor=QColor(labelcolor);
    if (color != "") axis->axiscolor=QColor(color);

    // log .. hmmm
    axis->log = log;

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
    default: return GColor(CPLOTMARKER);
    }
}
