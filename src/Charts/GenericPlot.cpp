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
    qchart->legend()->setVisible(true); // no legends
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

    // add watcher
    watcher = new SelectionTool(this);
    chartview->scene()->addItem(watcher);

    // filter ESC so we can stop scripts
    installEventFilter(this);

    // watch for colors changing
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // config changed...
    configChanged(0);
}

SelectionTool::SelectionTool(GenericPlot *host) : QGraphicsItem(NULL), host(host) {}
void SelectionTool::paint(QPainter*, const QStyleOptionGraphicsItem *, QWidget*) {}
QRectF SelectionTool::boundingRect() const { return QRectF(0,0,0,0); }
bool SelectionTool::sceneEventFilter(QGraphicsItem *watched, QEvent *event) { return host->sceneEventFilter(watched,event); }

bool
GenericPlot::sceneEventFilter(QGraphicsItem *, QEvent *)
{
    // events on items in qt chart
    //fprintf(stderr,"scene event, obj=%u event=%d\n", (void*)watched, event->type());
    //fflush(stderr);
    return false;
}

bool
GenericPlot::eventFilter(QObject *, QEvent *e)
{
    if (e->type() != QEvent::MouseMove && e->type() != QEvent::GraphicsSceneMouseMove) {
        //fprintf(stderr,"chart event, obj=%u event=%d\n", (void*)obj, e->type());
        //fflush(stderr);
    }

    // on resize event scale the display
    if (e->type() == QEvent::Resize) {
        //canvas->fitInView(canvas->sceneRect(), Qt::KeepAspectRatio);
    }

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
            delete existing;
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
                if (axisinfo->name != "x" && axisinfo->name != "y")  // equivalent to being blank
                    add->setTitleText(axisinfo->name);
                add->setLinePenColor(axisinfo->axiscolor);
                add->setLabelsColor(axisinfo->labelcolor);
                add->setTitleBrush(QBrush(axisinfo->labelcolor));

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

    // install event filters on thes scene objects to
    foreach(QGraphicsItem *item, chartview->scene()->items())
        item->installSceneEventFilter(watcher); // XXX create sceneitem to help us here!
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
