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

#include "GenericChart.h"

#include "Colors.h"
#include "AbstractView.h"
#include "RideFileCommand.h"
#include "Utils.h"

#include <limits>
#include <QScrollArea>

//
// The generic chart manages collections of genericplots
// if the stack option is selected we create a plot for
// each data series.
//
// If series do not have a common x-axis we create a
// separate plot, with series that share a common x-axis
// sharing the same plot
//
// The layout is horizontal or vertical in which case the
// generic chart will use vbox or hbox layouts. XXX TODO
//
// Charts have a minimum width and height that needs to be
// honoured too, since multiple series stacked can get
// cramped, so these are placed into a scroll area
//
GenericChart::GenericChart(QWidget *parent, Context *context) : QWidget(parent), context(context), item(NULL)
{
    // for scrollarea, since we see a little of it.
    QPalette palette;
    palette.setBrush(QPalette::Background, QBrush(GColor(CRIDEPLOTBACKGROUND)));

    // main layout for widget
    QVBoxLayout *main=new QVBoxLayout(this);
    main->setSpacing(0);
    main->setContentsMargins(0,0,0,0);

    // main layout used for charts
    mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
    lrLayout = new QHBoxLayout();
    mainLayout->addLayout(lrLayout);

    // stack for scrolling
    QWidget *stackWidget = new QWidget(this);
    stackWidget->setAutoFillBackground(false);
    stackWidget->setLayout(mainLayout);
    stackWidget->setPalette(palette);

    // put everything inside a scrollarea
    stackFrame = new QScrollArea(this);
#ifdef Q_OS_WIN
    QStyle *cde = QStyleFactory::create(OS_STYLE);
    stackFrame->setStyle(cde);
#endif
    stackFrame->setAutoFillBackground(false);
    stackFrame->setWidgetResizable(true);
    stackFrame->setFrameStyle(QFrame::NoFrame);
    stackFrame->setContentsMargins(0,0,0,0);
    stackFrame->setPalette(palette);
    main->addWidget(stackFrame);
    stackFrame->setWidget(stackWidget);

    // watch for color/themes change
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // default bgcolor
    bgcolor = GColor(CPLOTBACKGROUND);
    configChanged(0);
}

void
GenericChart::configChanged(qint32)
{
    setUpdatesEnabled(false);

    setProperty("color", bgcolor);
    QPalette palette;
    palette.setBrush(QPalette::Background, QBrush(bgcolor));
    setPalette(palette); // propagates to children

    // set style sheets
#ifndef Q_OS_MAC
    stackFrame->setStyleSheet(AbstractView::ourStyleSheet());
#endif
    setUpdatesEnabled(true);
}

void
GenericChart::setBackgroundColor(QColor bgcolor)
{
    if (bgcolor != this->bgcolor) {
        this->bgcolor = bgcolor;
        configChanged(0);
    }
}

void
GenericChart::setGraphicsItem(QGraphicsItem *item)
{
    this->item = item;
}

// set chart settings
bool
GenericChart::initialiseChart(QString title, int type, bool animate, int legendpos, bool stack, int orientation, double scale)
{
    // Remember the settings, we use them for every new plot
    this->title = title;
    this->type = type;
    this->animate = animate;
    this->legendpos = legendpos;
    this->stack = stack;
    this->orientation = orientation;
    this->scale = scale;

    // store info as its passed to us
    newSeries.clear();
    newAxes.clear();

    return true;
}

// add a curve, associating an axis
bool
GenericChart::addCurve(QString name, QVector<double> xseries, QVector<double> yseries, QVector<QString> fseries, QString xname, QString yname,
                      QStringList labels, QStringList colors, int line, int symbol, int size, QString color, int opacity, bool opengl,
                      bool legend, bool datalabels, bool fill)
{

    newSeries << GenericSeriesInfo(name, xseries, yseries, fseries, xname, yname, labels, colors, line, symbol, size, color, opacity, opengl, legend, datalabels, fill);
    return true;
}

// helper for python
bool
GenericChart::addCurve(QString name, QVector<double> xseries, QVector<double> yseries, QStringList fseries, QString xname, QString yname,
                      QStringList labels, QStringList colors, int line, int symbol, int size, QString color, int opacity, bool opengl,
                      bool legend, bool datalabels, bool fill)
{
    QVector<QString> flist;
    for(int i=0; i<fseries.count(); i++) flist << fseries.at(i);
    newSeries << GenericSeriesInfo(name, xseries, yseries, flist, xname, yname, labels, colors, line, symbol, size, color, opacity, opengl, legend, datalabels, fill);
    return true;
}

// add a label to a series
bool
GenericChart::annotateLabel(QString name, QStringList list)
{
    // add to the curve
    for(int i=0; i<newSeries.count(); i++)
        if (newSeries[i].name == name)
            newSeries[i].annotateLabels << list;

    return true;
}

// configure axis, after curves added
bool
GenericChart::configureAxis(QString name, bool visible, int align, double min, double max,
                      int type, QString labelcolor, QString color, bool log, QStringList categories)
{
    newAxes << GenericAxisInfo(name, visible, align, min, max, type, labelcolor, color, log, categories);
    return true;
}

// preprocess data before updating the qt charts
void
GenericChart::preprocessData()
{
    // any series on an axis that is time based
    // will need to have the values scaled from
    // seconds to MSSinceEpoch
    QDateTime midnight(QDate::currentDate(), QTime(0,0,0), Qt::LocalTime); // we always use LocalTime due to qt-bug 62285
    QDateTime earliest(QDate(1900,01,01), QTime(0,0,0), Qt::LocalTime);

    for(int ai=0; ai<newAxes.count(); ai++) {
        GenericAxisInfo &axis=newAxes[ai];

        if (axis.type == GenericAxisInfo::TIME) {

            // it's time based, so update series
            // associated with this axies
            bool usey = (axis.orientation == Qt::Vertical);

            // adjust the min/max set by the user to MS since epoch
            if (usey) {
                if (axis.miny != -1) axis.miny=midnight.addSecs(axis.miny).toMSecsSinceEpoch();
                if (axis.maxy != -1) axis.maxy=midnight.addSecs(axis.maxy).toMSecsSinceEpoch();
            } else {
                if (axis.minx != -1) axis.minx=midnight.addSecs(axis.minx).toMSecsSinceEpoch();
                if (axis.maxx != -1) axis.maxx=midnight.addSecs(axis.maxx).toMSecsSinceEpoch();
            }

            for(int si=0; si<newSeries.count(); si++) {
                GenericSeriesInfo &series=newSeries[si];
                if (!usey && series.xname == axis.name) {
                    // adjust x values
                    for(int i=0; i<series.xseries.count(); i++)
                        series.xseries[i] = midnight.addSecs(series.xseries[i]).toMSecsSinceEpoch();
                }
                if (usey && series.yname == axis.name) {
                    // adjust y values
                    for(int i=0; i<series.yseries.count(); i++)
                        series.yseries[i] = midnight.addSecs(series.yseries[i]).toMSecsSinceEpoch();
                }
            }

        } else if (axis.type == GenericAxisInfo::DATERANGE) {

            // it's time based, so update series
            // associated with this axies
            bool usey = (axis.orientation == Qt::Vertical);

            // adjust the min/max set by the user to MS since epoch
            if (usey) {
                if (axis.miny != -1) axis.miny=earliest.addDays(axis.miny).toMSecsSinceEpoch();
                if (axis.maxy != -1) axis.maxy=earliest.addDays(axis.maxy).toMSecsSinceEpoch();
            } else {
                if (axis.minx != -1) axis.minx=earliest.addDays(axis.minx).toMSecsSinceEpoch();
                if (axis.maxx != -1) axis.maxx=earliest.addDays(axis.maxx).toMSecsSinceEpoch();
            }

            for(int si=0; si<newSeries.count(); si++) {
                GenericSeriesInfo &series=newSeries[si];
                if (!usey && series.xname == axis.name) {
                    // adjust x values
                    for(int i=0; i<series.xseries.count(); i++)
                        series.xseries[i] = earliest.addDays(series.xseries[i]).toMSecsSinceEpoch();
                }
                if (usey && series.yname == axis.name) {
                    // adjust y values
                    for(int i=0; i<series.yseries.count(); i++)
                        series.yseries[i] = earliest.addDays(series.yseries[i]).toMSecsSinceEpoch();
                }
            }

        }
    }

}

// post processing clean up / add decorations / helpers etc
void
GenericChart::finaliseChart()
{
    setUpdatesEnabled(false); // lets not open our kimono here

    // now we know the axis/series and relationships we can
    // adapt the data provided, prior to updating the qt charts
    // the most obvious thing here is converting seconds to
    // qdatetime MS since midnight. But later we may also need
    // to perform scaling or filtering.
    preprocessData();

    // ok, so lets work out what we need, run through the curves
    // and allocate to a plot, where we have separate plots if
    // we have stacked set -or- for each xaxis, remembering to
    // add the axisinfo each time too.
    QList<GenericPlotInfo> newPlots;
    QStringList xaxes;
    for(int i=0; i<newSeries.count(); i++) {

        if (stack) {

            // super easy we just have a new plot for each series
            GenericPlotInfo add(newSeries[i].xname);
            add.series << newSeries[i];

            // xaxis info
            int ax=GenericAxisInfo::findAxis(newAxes, newSeries[i].xname);
            if (ax>=0) add.axes << newAxes[ax];

            // yaxis info
            ax=GenericAxisInfo::findAxis(newAxes, newSeries[i].yname);
            if (ax>=0) add.axes << newAxes[ax];

            // add to list
            newPlots << add;
            xaxes << newSeries[i].xname;

        } else {

            // otherwise add all series to the same plot
            // with a common x axis
            int index = xaxes.indexOf(newSeries[i].xname);
            if (index >=0) {
                // woop, we already got this xaxis
                newPlots[index].series << newSeries[i];

                // yaxis info
                int ax=GenericAxisInfo::findAxis(newAxes, newSeries[i].yname);
                if (ax>=0) newPlots[index].axes << newAxes[ax];

            } else {

                GenericPlotInfo add(newSeries[i].xname);
                add.series << newSeries[i];

                // xaxis info
                int ax=GenericAxisInfo::findAxis(newAxes, newSeries[i].xname);
                if (ax>=0) add.axes << newAxes[ax];

                // yaxis info
                ax=GenericAxisInfo::findAxis(newAxes, newSeries[i].yname);
                if (ax>=0) add.axes << newAxes[ax];

                // add to list
                newPlots << add;
                xaxes << newSeries[i].xname;
            }
        }
    }

    // lets find existing plots or create new ones
    // first we mark all existing plots as deleteme
    for(int i=0; i<currentPlots.count(); i++) currentPlots[i].state = GenericPlotInfo::deleteme;

    // source and target layouts
    QBoxLayout *source, *target;
    if (orientation == Qt::Vertical) {
        source = lrLayout;
        target = mainLayout;
    } else {
        source = mainLayout;
        target = lrLayout;
    }

    // match what we want with what we got
    for(int i=0; i<newPlots.count(); i++) {
        int index = GenericPlotInfo::findPlot(currentPlots, newPlots[i]);

        // don't reuse bar/pie charts, easier to rebuild
        if (type == GC_CHART_PIE || type == GC_CHART_BAR || type == GC_CHART_STACK || type == GC_CHART_PERCENT) index = -1;

        if (index <0) {
            // new one required
            newPlots[i].plot = new GenericPlot(this, context, item);

            target->addWidget(newPlots[i].plot);
            target->setStretchFactor(newPlots[i].plot, 10);// make them all the same
        } else {
            newPlots[i].plot = currentPlots[index].plot; // reuse
            currentPlots[index].state = GenericPlotInfo::matched; // don't deleteme !

            // make sure its in the right layout (might remove and add back to the same layout!)
            source->removeWidget(newPlots[i].plot);
            target->addWidget(newPlots[i].plot);
            target->setStretchFactor(newPlots[i].plot, 10);// make them all the same
        }
    }

    // delete all the deleteme plots
    for(int i=0; i<currentPlots.count(); i++) {
        if (currentPlots[i].state == GenericPlotInfo::deleteme) {
            delete currentPlots[i].plot; // will remove from layout too
        }
    }

    // update plot to stop the jarring effect as chart
    // is updated, especially when multiple series in
    // stack mode. I would expect this to make things
    // worse but for some reason, it solves the problem.
    // do not remove !!!
    QApplication::processEvents();

    // now initialise all the newPlots
    for(int i=0; i<newPlots.count(); i++) {

        // set initial parameters
        newPlots[i].plot->setBackgroundColor(bgcolor);
        newPlots[i].plot->initialiseChart(title, type, animate, legendpos, scale);

        // add curves
        QListIterator<GenericSeriesInfo>s(newPlots[i].series);
        while(s.hasNext()) {

            GenericSeriesInfo p=s.next();

            // add curve-- with additional axis info in advance since it finally is added to an actual chart
            newPlots[i].plot->addCurve(p.name, p.xseries, p.yseries, p.fseries, p.xname, p.yname, p.labels, p.colors, p.line,
                                       p.symbol, p.size, p.color, p.opacity, p.opengl, p.legend, p.datalabels, p.fill);

            // did we get some labels associated with the curve?
            QListIterator<QStringList>l(p.annotateLabels);
            while(l.hasNext()) {
                QStringList list=l.next();
                newPlots[i].plot->addAnnotation(GenericPlot::LABEL, list.join(" "), RGBColor(QColor(p.color)));
            }

        }

        // set axis
        QListIterator<GenericAxisInfo>a(newPlots[i].axes);
        while(a.hasNext()) {
            GenericAxisInfo p=a.next();
            newPlots[i].plot->configureAxis(p.name, p.visible, p.align,p.minx, p.maxx, p.type, p.labelcolorstring, p.axiscolorstring, p.log, p.categories);
        }

        newPlots[i].plot->finaliseChart();
        newPlots[i].state = GenericPlotInfo::active;
    }

    // set current and wipe rest
    currentPlots=newPlots;

    // and zap the state
    newAxes.clear();
    newSeries.clear();

    // and display
    setUpdatesEnabled(true);
}
