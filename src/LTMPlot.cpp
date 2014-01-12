/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "Athlete.h"
#include "Context.h"
#include "LTMPlot.h"
#include "LTMTool.h"
#include "LTMTrend.h"
#include "LTMTrend2.h"
#include "LTMOutliers.h"
#include "LTMWindow.h"
#include "MetricAggregator.h"
#include "SummaryMetrics.h"
#include "RideMetric.h"
#include "Settings.h"
#include "Colors.h"

#include "StressCalculator.h" // for LTS/STS calculation

#include <QSettings>

#include <qwt_series_data.h>
#include <qwt_scale_widget.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_canvas.h>
#include <qwt_curve_fitter.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>

#include <math.h> // for isinf() isnan()

LTMPlot::LTMPlot(LTMWindow *parent, Context *context) : 
    bg(NULL), parent(parent), context(context), highlighter(NULL)
{
    // don't do this ..
    setAutoReplot(false);
    setAutoFillBackground(true);

    // setup my axes
    // for now we limit to 4 on left and 4 on right
    setAxesCount(QwtAxis::yLeft, 4);
    setAxesCount(QwtAxis::yRight, 4);
    setAxesCount(QwtAxis::xBottom, 1);
    setAxesCount(QwtAxis::xTop, 0);

    for (int i=0; i<4; i++) {

        // lefts
        QwtAxisId left(QwtAxis::yLeft, i);
        supportedAxes << left;
        
        QwtScaleDraw *sd = new QwtScaleDraw;
        sd->setTickLength(QwtScaleDiv::MajorTick, 3);
        sd->enableComponent(QwtScaleDraw::Ticks, false);
        sd->enableComponent(QwtScaleDraw::Backbone, false);

        setAxisScaleDraw(left, sd);
        setAxisMaxMinor(left, 0);
        setAxisVisible(left, false);

        QwtAxisId right(QwtAxis::yRight, i);
        supportedAxes << right;

        // lefts
        sd = new QwtScaleDraw;
        sd->setTickLength(QwtScaleDiv::MajorTick, 3);
        sd->enableComponent(QwtScaleDraw::Ticks, false);
        sd->enableComponent(QwtScaleDraw::Backbone, false);
        setAxisScaleDraw(right, sd);
        setAxisMaxMinor(right, 0);
        setAxisVisible(right, false);
    }

    // get application settings
    insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setAxisTitle(QwtAxis::xBottom, tr("Date"));
    enableAxis(QwtAxis::xBottom, true);
    setAxisVisible(QwtAxis::xBottom, true);
    setAxisVisible(QwtAxis::xTop, false);
    setAxisMaxMinor(QwtPlot::xBottom,-1);
    setAxisScaleDraw(QwtPlot::xBottom, new LTMScaleDraw(QDateTime::currentDateTime(), 0, LTM_DAY));

    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

    grid = new QwtPlotGrid();
    grid->enableX(false);
    grid->attach(this);

    settings = NULL;
    cogganPMC = skibaPMC = NULL; // cache when replotting a PMC

    configUpdate(); // set basic colors

    connect(context, SIGNAL(configChanged()), this, SLOT(configUpdate()));
}

LTMPlot::~LTMPlot()
{
}

void
LTMPlot::configUpdate()
{
    // set basic plot colors
    setCanvasBackground(GColor(CPLOTBACKGROUND));
    QPen gridPen(GColor(CPLOTGRID));
    //gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);

    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    setPalette(palette);

    foreach (QwtAxisId x, supportedAxes) {
        axisWidget(x)->setPalette(palette);
        axisWidget(x)->setPalette(palette);
    }
    axisWidget(QwtPlot::xBottom)->setPalette(palette);
    this->legend()->setPalette(palette);
}

void
LTMPlot::setAxisTitle(QwtAxisId axis, QString label)
{
    // setup the default fonts
    QFont stGiles; // hoho - Chart Font St. Giles ... ok you have to be British to get this joke
    stGiles.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    stGiles.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());

    QwtText title(label);
    title.setFont(stGiles);
    QwtPlot::setAxisFont(axis, stGiles);
    QwtPlot::setAxisTitle(axis, title);
}

void
LTMPlot::setData(LTMSettings *set)
{
    QTime timer;
    timer.start();

    //qDebug()<<"Starting.."<<timer.elapsed();

    // wipe away last cached stress calculator
    if (cogganPMC) { delete cogganPMC; cogganPMC=NULL; }
    if (skibaPMC) { delete cogganPMC; cogganPMC=NULL; }

    settings = set;

    // For each metric in chart, translate units and name if default uname
    //XXX BROKEN XXX LTMTool::translateMetrics(context, settings);

    // crop dates to at least within a year of the data available, but only if we have some data
    if (settings->data != NULL && (*settings->data).count() != 0) {
        // if dates are null we need to set them from the available data

        // end
        if (settings->end == QDateTime() ||
            settings->end > (*settings->data).last().getRideDate().addDays(365)) {
            if (settings->end < QDateTime::currentDateTime()) {
                settings->end = QDateTime::currentDateTime();
            } else {
                settings->end = (*settings->data).last().getRideDate();
            }
        }

        // start
        if (settings->start == QDateTime() ||
            settings->start < (*settings->data).first().getRideDate().addDays(-365)) {
            settings->start = (*settings->data).first().getRideDate();
        }
    }

    //setTitle(settings->title);
    if (settings->groupBy != LTM_TOD)
        setAxisTitle(xBottom, tr("Date"));
    else
        setAxisTitle(xBottom, tr("Time of Day"));
    enableAxis(QwtAxis::xBottom, true);
    setAxisVisible(QwtAxis::xBottom, true);
    setAxisVisible(QwtAxis::xTop, false);

    // wipe existing curves/axes details
    QHashIterator<QString, QwtPlotCurve*> c(curves);
    while (c.hasNext()) {
        c.next();
        QString symbol = c.key();
        QwtPlotCurve *current = c.value();
        //current->detach(); // the destructor does this for you
        delete current;
    }
    curves.clear();
    if (highlighter) {
        highlighter->detach();
        delete highlighter;
        highlighter = NULL;
    }

    // disable all y axes until we have populated
    for (int i=0; i<8; i++) {
        setAxisVisible(supportedAxes[i], false);
        enableAxis(supportedAxes[i].id, false);
    }
    axes.clear();

    // reset all min/max Y values
    for (int i=0; i<10; i++) minY[i]=0, maxY[i]=0;

    // no data to display so that all folks
    if (settings->data == NULL || (*settings->data).count() == 0) {

        // tidy up the bottom axis
        maxX = groupForDate(settings->end.date(), settings->groupBy) -
                groupForDate(settings->start.date(), settings->groupBy);

        setAxisScale(xBottom, 0, maxX);
        setAxisScaleDraw(QwtPlot::xBottom, new LTMScaleDraw(settings->start,
                groupForDate(settings->start.date(), settings->groupBy), settings->groupBy));
        enableAxis(QwtAxis::xBottom, true);
        setAxisVisible(QwtAxis::xBottom, true);
        setAxisVisible(QwtAxis::xTop, false);

        // remove the shading if it exists
        refreshZoneLabels(-1);

        // remove the old markers
        refreshMarkers(settings->start.date(), settings->end.date(), settings->groupBy);

        replot();
        return;
    }

    //qDebug()<<"Wiped previous.."<<timer.elapsed();

    // count the bars since we format them side by side and need
    // to now how to offset them from each other
    // unset stacking if not a bar chart too since we don't support
    // that yet, but would be good to add in the future (stacked
    // area plot).
    int barnum=0;
    int bars = 0;
    int stacknum = -1;
    // index through rather than foreach so we can modify
    for (int v=0; v<settings->metrics.count(); v++) {
        if (settings->metrics[v].curveStyle == QwtPlotCurve::Steps) {
            if (settings->metrics[v].stack && stacknum < 0) stacknum = bars++; // starts from 1 not zero
            else if (settings->metrics[v].stack == false) bars++;
        } else if (settings->metrics[v].stack == true)
            settings->metrics[v].stack = false; // we only support stack on bar charts
    }

    // aggregate the stack curves - backwards since
    // we plot forwards overlaying to create the illusion
    // of a stack, when in fact its just bars of descending
    // order (with values aggregated)

    // free stack memory
    foreach(QVector<double>*p, stackX) delete p;
    foreach(QVector<double>*q, stackY) delete q;
    stackX.clear();
    stackY.clear();
    stacks.clear();

    int r=0;
    foreach (MetricDetail metricDetail, settings->metrics) {
        if (metricDetail.stack == true) {

            // register this data
            QVector<double> *xdata = new QVector<double>();
            QVector<double> *ydata = new QVector<double>();
            stackX.append(xdata);
            stackY.append(ydata);

            int count;
            if (settings->groupBy != LTM_TOD)
                createCurveData(settings, metricDetail, *xdata, *ydata, count);
            else
                createTODCurveData(settings, metricDetail, *xdata, *ydata, count);

            // we add in the last curve for X axis values
            if (r) {
                aggregateCurves(*stackY[r], *stackY[r-1]);
            }
            r++;
        }
    }

    //qDebug()<<"Created curve data.."<<timer.elapsed();

    // setup the curves
    double width = appsettings->value(this, GC_LINEWIDTH, 2.0).toDouble();
    bool donestack = false;

    // now we iterate over the metric details AGAIN
    // but this time in reverse and only plot the
    // stacked values. This is because we overcome the
    // lack of a stacked plot in QWT by painting decreasing
    // bars, with the values aggregated previously
    // so if we plot L1 time in zone 1hr and L2 time in zone 1hr
    // it plots as L2 time in zone 2hr and then paints over that
    // with a L1 time in zone of 1hr.
    //
    // The tooltip has to unpick the aggregation to ensure
    // that it subtracts other data series in the stack from
    // the value plotted... all nasty but heck, it works
    int stackcounter = stackX.size()-1;
    for (int m=settings->metrics.count()-1; m>=0; m--) {

        int count=0;
        MetricDetail metricDetail = settings->metrics[m];

        if (metricDetail.stack == false) continue;

        QVector<double> xdata, ydata;

        // use the aggregated values
        xdata = *stackX[stackcounter];
        ydata = *stackY[stackcounter];
        stackcounter--;
        count = xdata.size()-2;

        // no data to plot!
        if (count <= 0) continue;

        // Create a curve
        QwtPlotCurve *current = new QwtPlotCurve(metricDetail.uname);
        if (metricDetail.type == METRIC_BEST)
            curves.insert(metricDetail.bestSymbol, current);
        else
            curves.insert(metricDetail.symbol, current);
        stacks.insert(current, stackcounter+1);
        if (appsettings->value(this, GC_ANTIALIAS, false).toBool() == true)
            current->setRenderHint(QwtPlotItem::RenderAntialiased);
        QPen cpen = QPen(metricDetail.penColor);
        cpen.setWidth(width);
        current->setPen(cpen);
        current->setStyle(metricDetail.curveStyle);

        // choose the axis
        QwtAxisId axisid = chooseYAxis(metricDetail.uunits);
        current->setYAxis(axisid);

        // left and right offset for bars
        double left = 0;
        double right = 0;

        if (metricDetail.curveStyle == QwtPlotCurve::Steps) {

            int barn = metricDetail.stack ? stacknum : barnum;

            double space = double(0.9) / bars;
            double gap = space * 0.10;
            double width = space * 0.90;
            left = (space * barn) + (gap / 2) + 0.1;
            right = left + width;

            if (metricDetail.stack && donestack == false) {
                barnum++;
                donestack = true;
            } else if (metricDetail.stack == false) barnum++;
        }

        if (metricDetail.curveStyle == QwtPlotCurve::Steps) {
            
            // fill the bars
            QColor brushColor = metricDetail.penColor;
            if (metricDetail.stack == true) {
                brushColor.setAlpha(255);
                QBrush brush = QBrush(brushColor);
                current->setBrush(brush);
            } else {
                brushColor.setAlpha(64); // now side by side, less transparency required
                QColor brushColor1 = brushColor.darker();

                QLinearGradient linearGradient(0, 0, 0, height());
                linearGradient.setColorAt(0.0, brushColor1);
                linearGradient.setColorAt(1.0, brushColor);
                linearGradient.setSpread(QGradient::PadSpread);
                current->setBrush(linearGradient);
            }

            current->setPen(QPen(Qt::NoPen));
            current->setCurveAttribute(QwtPlotCurve::Inverted, true);

            QwtSymbol *sym = new QwtSymbol;
            sym->setStyle(QwtSymbol::NoSymbol);
            current->setSymbol(sym);

            // fudge for date ranges, not for time of day graph
            // and fudge qwt'S lack of a decent bar chart
            // add a zero point at the head and tail so the
            // histogram columns look nice.
            // and shift all the x-values left by 0.5 so that
            // they centre over x-axis labels
            int i=0;
            for (i=0; i<count; i++) xdata[i] -= 0.5;
            // now add a final 0 value to get the last
            // column drawn - no resize neccessary
            // since it is always sized for 1 + maxnumber of entries
            xdata[i] = xdata[i-1] + 1;
            ydata[i] = 0;
            count++;

            QVector<double> xaxis (xdata.size() * 4);
            QVector<double> yaxis (ydata.size() * 4);

            // samples to time
            for (int i=0, offset=0; i<xdata.size(); i++) {

                double x = (double) xdata[i];
                double y = (double) ydata[i];

                xaxis[offset] = x +left;
                yaxis[offset] = metricDetail.baseline; // use baseline not 0, default is 0
                offset++;
                xaxis[offset] = x+left;
                yaxis[offset] = y;
                offset++;
                xaxis[offset] = x+right;
                yaxis[offset] = y;
                offset++;
                xaxis[offset] = x +right;
                yaxis[offset] = metricDetail.baseline;; // use baseline not 0, default is 0
                offset++;
            }
            xdata = xaxis;
            ydata = yaxis;
            count *= 4;
            // END OF FUDGE

        }

        // set the data series
        current->setSamples(xdata.data(),ydata.data(), count + 1);
        current->setBaseline(metricDetail.baseline);

        // update stack data so we can index off them
        // in tooltip
        *stackX[stackcounter+1] = xdata;
        *stackY[stackcounter+1] = ydata;
        
        // update min/max Y values for the chosen axis
        if (current->maxYValue() > maxY[supportedAxes.indexOf(axisid)]) maxY[supportedAxes.indexOf(axisid)] = current->maxYValue();
        if (current->minYValue() < minY[supportedAxes.indexOf(axisid)]) minY[supportedAxes.indexOf(axisid)] = current->minYValue();

        current->attach(this);

    } // end of reverse for stacked plots

    //qDebug()<<"First plotting iteration.."<<timer.elapsed();

    // do all curves excepts stacks in order
    // we skip stacked entries because they
    // are painted in reverse order in a
    // loop before this one.
    stackcounter= 0;
    foreach (MetricDetail metricDetail, settings->metrics) {

        if (metricDetail.stack == true) continue;

        QVector<double> xdata, ydata;

        int count;
        if (settings->groupBy != LTM_TOD)
            createCurveData(settings, metricDetail, xdata, ydata, count);
        else
            createTODCurveData(settings, metricDetail, xdata, ydata, count);

        //qDebug()<<"Create curve data.."<<timer.elapsed();

        // Create a curve
        QwtPlotCurve *current = new QwtPlotCurve(metricDetail.uname);
        if (metricDetail.type == METRIC_BEST)
            curves.insert(metricDetail.bestSymbol, current);
        else
            curves.insert(metricDetail.symbol, current);
        if (appsettings->value(this, GC_ANTIALIAS, false).toBool() == true)
            current->setRenderHint(QwtPlotItem::RenderAntialiased);
        QPen cpen = QPen(metricDetail.penColor);
        cpen.setWidth(width);
        current->setPen(cpen);
        current->setStyle(metricDetail.curveStyle);

        // choose the axis
        QwtAxisId axisid = chooseYAxis(metricDetail.uunits);
        current->setYAxis(axisid);

        // left and right offset for bars
        double left = 0;
        double right = 0;
        double middle = 0;
        if (metricDetail.curveStyle == QwtPlotCurve::Steps) {

            // we still worry about stacked bars, since we
            // need to take into account the space it will
            // consume when plotted in the second iteration
            // below this one
            int barn = metricDetail.stack ? stacknum : barnum;

            double space = double(0.9) / bars;
            double gap = space * 0.10;
            double width = space * 0.90;
            left = (space * barn) + (gap / 2) + 0.1;
            right = left + width;
            middle = ((left+right) / double(2)) - 0.5;
            if (metricDetail.stack && donestack == false) {
                barnum++;
                donestack = true;
            } else if (metricDetail.stack == false) barnum++;
        }

        // trend - clone the data for the curve and add a curvefitted
        if (metricDetail.trendtype) {

            // linear regress
            if (metricDetail.trendtype == 1 && count > 2) {

                // override class variable as doing it temporarily for trend line only
                double maxX = 0.5 + groupForDate(settings->end.date(), settings->groupBy) -
                    groupForDate(settings->start.date(), settings->groupBy);

                QString trendName = QString(tr("%1 trend")).arg(metricDetail.uname);
                QString trendSymbol = QString("%1_trend")
                                       .arg(metricDetail.type == METRIC_BEST ? 
                                       metricDetail.bestSymbol : metricDetail.symbol);

                QwtPlotCurve *trend = new QwtPlotCurve(trendName);

                // cosmetics
                QPen cpen = QPen(metricDetail.penColor.darker(200));
                cpen.setWidth(2); // double thickness for trend lines
                cpen.setStyle(Qt::SolidLine);
                trend->setPen(cpen);
                if (appsettings->value(this, GC_ANTIALIAS, false).toBool()==true)
                    trend->setRenderHint(QwtPlotItem::RenderAntialiased);
                trend->setBaseline(0);
                trend->setYAxis(axisid);
                trend->setStyle(QwtPlotCurve::Lines);

                // perform linear regression
                LTMTrend regress(xdata.data(), ydata.data(), count);
                double xtrend[2], ytrend[2];
                xtrend[0] = 0.0; 
                ytrend[0] = regress.getYforX(0.0);
                // point 2 is at far right of chart, not the last point
                // since we may be forecasting...
                xtrend[1] = maxX;
                ytrend[1] = regress.getYforX(maxX);
                trend->setSamples(xtrend,ytrend, 2);

                trend->attach(this);
                curves.insert(trendSymbol, trend);

            }

            // quadratic lsm regression
            if (metricDetail.trendtype == 2 && count > 3) {
                QString trendName = QString(tr("%1 trend")).arg(metricDetail.uname);
                QString trendSymbol = QString("%1_trend")
                                       .arg(metricDetail.type == METRIC_BEST ? 
                                       metricDetail.bestSymbol : metricDetail.symbol);

                QwtPlotCurve *trend = new QwtPlotCurve(trendName);

                // cosmetics
                QPen cpen = QPen(metricDetail.penColor.darker(200));
                cpen.setWidth(2); // double thickness for trend lines
                cpen.setStyle(Qt::SolidLine);
                trend->setPen(cpen);
                if (appsettings->value(this, GC_ANTIALIAS, false).toBool()==true)
                    trend->setRenderHint(QwtPlotItem::RenderAntialiased);
                trend->setBaseline(0);
                trend->setYAxis(axisid);
                trend->setStyle(QwtPlotCurve::Lines);

                // perform quadratic curve fit to data
                LTMTrend2 regress(xdata.data(), ydata.data(), count);

                QVector<double> xtrend;
                QVector<double> ytrend;

                double inc = (regress.maxx - regress.minx) / 100;
                for (double i=regress.minx; i<=(regress.maxx+inc); i+= inc) {
                    xtrend << i;
                    ytrend << regress.yForX(i);
                }

                // point 2 is at far right of chart, not the last point
                // since we may be forecasting...
                trend->setSamples(xtrend.data(),ytrend.data(), xtrend.count());

                trend->attach(this);
                curves.insert(trendSymbol, trend);
            }
        }

        // highlight outliers
        if (metricDetail.topOut > 0 && metricDetail.topOut < count && count > 10) {

            LTMOutliers outliers(xdata.data(), ydata.data(), count, 10);

            // the top 5 outliers
            QVector<double> hxdata, hydata;
            hxdata.resize(metricDetail.topOut);
            hydata.resize(metricDetail.topOut);

            // QMap orders the list so start at the top and work
            // backwards
            for (int i=0; i<metricDetail.topOut; i++) {
                hxdata[i] = outliers.getXForRank(i) + middle;
                hydata[i] = outliers.getYForRank(i);
            }

            // lets setup a curve with this data then!
            QString outName;
            if (metricDetail.topOut > 1)
                outName = QString(tr("%1 Top %2 Outliers"))
                          .arg(metricDetail.uname)
                          .arg(metricDetail.topOut);
            else
                outName = QString(tr("%1 Outlier")).arg(metricDetail.uname);

            QString outSymbol = QString("%1_outlier").arg(metricDetail.type == METRIC_BEST ?
                                                          metricDetail.bestSymbol : metricDetail.symbol);
            QwtPlotCurve *out = new QwtPlotCurve(outName);
            curves.insert(outSymbol, out);

            out->setRenderHint(QwtPlotItem::RenderAntialiased);
            out->setStyle(QwtPlotCurve::Dots);

            // we might have hidden the symbols for this curve
            // if its set to none then default to a rectangle
            QwtSymbol *sym = new QwtSymbol;
            if (metricDetail.symbolStyle == QwtSymbol::NoSymbol)
                sym->setStyle(QwtSymbol::Rect);
            else
                sym->setStyle(metricDetail.symbolStyle);
            sym->setSize(20);
            QColor lighter = metricDetail.penColor;
            lighter.setAlpha(50);
            sym->setPen(metricDetail.penColor);
            sym->setBrush(lighter);

            out->setSymbol(sym);
            out->setSamples(hxdata.data(),hydata.data(), metricDetail.topOut);
            out->setBaseline(0);
            out->setYAxis(axisid);
            out->attach(this);
        }

        // highlight top N values
        if (metricDetail.topN > 0) {

            QMap<double, int> sortedList;

            // copy the yvalues, retaining the offset
            for(int i=0; i<ydata.count(); i++)
                sortedList.insert(ydata[i], i);

            // copy the top N values
            QVector<double> hxdata, hydata;
            hxdata.resize(metricDetail.topN);
            hydata.resize(metricDetail.topN);

            // QMap orders the list so start at the top and work
            // backwards
            QMapIterator<double, int> i(sortedList);
            i.toBack();
            int counter = 0;
            while (i.hasPrevious() && counter < metricDetail.topN) {
                i.previous();
                if (ydata[i.value()]) {
                    hxdata[counter] = xdata[i.value()] + middle;
                    hydata[counter] = ydata[i.value()];
                    counter++;
                }
            }

            // lets setup a curve with this data then!
            QString topName;
            if (counter > 1)
                topName = QString(tr("%1 Best %2"))
                          .arg(metricDetail.uname)
                          .arg(counter); // starts from zero
            else
                topName = QString(tr("Best %1")).arg(metricDetail.uname);

            QString topSymbol = QString("%1_topN")
                                .arg(metricDetail.type == METRIC_BEST ? 
                                     metricDetail.bestSymbol : metricDetail.symbol);
            QwtPlotCurve *top = new QwtPlotCurve(topName);
            curves.insert(topSymbol, top);

            top->setRenderHint(QwtPlotItem::RenderAntialiased);
            top->setStyle(QwtPlotCurve::Dots);

            // we might have hidden the symbols for this curve
            // if its set to none then default to a rectangle
            QwtSymbol *sym = new QwtSymbol;
            if (metricDetail.symbolStyle == QwtSymbol::NoSymbol)
                sym->setStyle(QwtSymbol::Rect);
            else
                sym->setStyle(metricDetail.symbolStyle);
            sym->setSize(12);
            QColor lighter = metricDetail.penColor;
            lighter.setAlpha(200);
            sym->setPen(metricDetail.penColor);
            sym->setBrush(lighter);

            top->setSymbol(sym);
            top->setSamples(hxdata.data(),hydata.data(), counter);
            top->setBaseline(0);
            top->setYAxis(axisid);
            top->attach(this);
        }
        if (metricDetail.curveStyle == QwtPlotCurve::Steps) {
            
            // fill the bars
            QColor brushColor = metricDetail.penColor;
            brushColor.setAlpha(64); // now side by side, less transparency required
            QColor brushColor1 = metricDetail.penColor.darker();
            QLinearGradient linearGradient(0, 0, 0, height());
            linearGradient.setColorAt(0.0, brushColor1);
            linearGradient.setColorAt(1.0, brushColor);
            linearGradient.setSpread(QGradient::PadSpread);
            current->setBrush(linearGradient);
            current->setPen(QPen(Qt::NoPen));
            current->setCurveAttribute(QwtPlotCurve::Inverted, true);

            QwtSymbol *sym = new QwtSymbol;
            sym->setStyle(QwtSymbol::NoSymbol);
            current->setSymbol(sym);

            // fudge for date ranges, not for time of day graph
            // fudge qwt'S lack of a decent bar chart
            // add a zero point at the head and tail so the
            // histogram columns look nice.
            // and shift all the x-values left by 0.5 so that
            // they centre over x-axis labels
            count = xdata.size()-2;

            int i=0;
            for (i=0; i<count; i++) xdata[i] -= 0.5;
            // now add a final 0 value to get the last
            // column drawn - no resize neccessary
            // since it is always sized for 1 + maxnumber of entries
            xdata[i] = xdata[i-1] + 1;
            ydata[i] = 0;
            count++;

            QVector<double> xaxis (xdata.size() * 4);
            QVector<double> yaxis (ydata.size() * 4);

            // samples to time
            for (int i=0, offset=0; i<xdata.size(); i++) {

                double x = (double) xdata[i];
                double y = (double) ydata[i];

                xaxis[offset] = x +left;
                yaxis[offset] = metricDetail.baseline;; // use baseline not 0, default is 0
                offset++;
                xaxis[offset] = x+left;
                yaxis[offset] = y;
                offset++;
                xaxis[offset] = x+right;
                yaxis[offset] = y;
                offset++;
                xaxis[offset] = x +right;
                yaxis[offset] = metricDetail.baseline;; // use baseline not 0, default is 0
                offset++;
            }
            xdata = xaxis;
            ydata = yaxis;
            count *= 4;
            // END OF FUDGE

        } else if (metricDetail.curveStyle == QwtPlotCurve::Lines) {

            QPen cpen = QPen(metricDetail.penColor);
            cpen.setWidth(width);
            QwtSymbol *sym = new QwtSymbol;
            sym->setSize(6);
            sym->setStyle(metricDetail.symbolStyle);
            sym->setPen(QPen(metricDetail.penColor));
            sym->setBrush(QBrush(metricDetail.penColor));
            current->setSymbol(sym);
            current->setPen(cpen);

            // fill below the line
            if (metricDetail.fillCurve) {
                QColor fillColor = metricDetail.penColor;
                fillColor.setAlpha(60);
                current->setBrush(fillColor);
            }


        } else if (metricDetail.curveStyle == QwtPlotCurve::Dots) {

            QwtSymbol *sym = new QwtSymbol;
            sym->setSize(6);
            sym->setStyle(metricDetail.symbolStyle);
            sym->setPen(QPen(metricDetail.penColor));
            sym->setBrush(QBrush(metricDetail.penColor));
            current->setSymbol(sym);

        } else if (metricDetail.curveStyle == QwtPlotCurve::Sticks) {

            QwtSymbol *sym = new QwtSymbol;
            sym->setSize(4);
            sym->setStyle(metricDetail.symbolStyle);
            sym->setPen(QPen(metricDetail.penColor));
            sym->setBrush(QBrush(Qt::white));
            current->setSymbol(sym);

        }

        // smoothing
        if (metricDetail.smooth == true) {
            current->setCurveAttribute(QwtPlotCurve::Fitted, true);
        }

        // set the data series
        current->setSamples(xdata.data(),ydata.data(), count + 1);
        current->setBaseline(metricDetail.baseline);

        //qDebug()<<"Set Curve Data.."<<timer.elapsed();

        // update min/max Y values for the chosen axis
        if (current->maxYValue() > maxY[supportedAxes.indexOf(axisid)]) maxY[supportedAxes.indexOf(axisid)] = current->maxYValue();
        if (current->minYValue() < minY[supportedAxes.indexOf(axisid)]) minY[supportedAxes.indexOf(axisid)] = current->minYValue();

        current->attach(this);

    }

    //qDebug()<<"Second plotting iteration.."<<timer.elapsed();


    if (settings->groupBy != LTM_TOD) {

        // make start date always fall on a Monday
        if (settings->groupBy == LTM_WEEK) {
            int dow = settings->start.date().dayOfWeek(); // 1-7, where 1=monday
            settings->start.date().addDays(dow-1*-1);
        }

        // setup the xaxis at the bottom
        int tics;
        maxX = 0.5 + groupForDate(settings->end.date(), settings->groupBy) -
                groupForDate(settings->start.date(), settings->groupBy);
        if (maxX < 14) {
            tics = 1;
        } else {
            tics = 1 + maxX/10;
        }
        setAxisScale(xBottom, -0.5, maxX, tics);
        setAxisScaleDraw(QwtPlot::xBottom, new LTMScaleDraw(settings->start,
                    groupForDate(settings->start.date(), settings->groupBy), settings->groupBy));

    } else {
        setAxisScale(xBottom, 0, 24, 2);
        setAxisScaleDraw(QwtPlot::xBottom, new LTMScaleDraw(settings->start,
                    groupForDate(settings->start.date(), settings->groupBy), settings->groupBy));
    }
    enableAxis(QwtAxis::xBottom, true);
    setAxisVisible(QwtAxis::xBottom, true);
    setAxisVisible(QwtAxis::xTop, false);

    // run through the Y axis
    for (int i=0; i<8; i++) {
        // set the scale on the axis
        if (i != xBottom && i != xTop) {
            maxY[i] *= 1.1; // add 10% headroom
            setAxisScale(supportedAxes[i], minY[i], maxY[i]);
        }
    }

    QString format = axisTitle(yLeft).text();
    parent->toolTip()->setAxes(xBottom, yLeft);
    parent->toolTip()->setFormat(format);

    // draw zone labels axisid of -1 means delete whats there
    // cause no watts are being displayed
    if (settings->shadeZones == true) {
        QwtAxisId axisid = axes.value("watts", QwtAxisId(-1,-1));
        if (axisid == QwtAxisId(-1,-1)) axisid = axes.value(tr("watts"), QwtAxisId(-1,-1)); // Try translated version
        refreshZoneLabels(axisid);
    } else {
        refreshZoneLabels(QwtAxisId(-1,-1)); // turn em off
    }

    // show legend?
    if (settings->legend == false) {
        this->legend()->hide();
        QHashIterator<QString, QwtPlotCurve*> c(curves);
        while (c.hasNext()) {
            c.next();
            c.value()->setItemAttribute(QwtPlotItem::Legend, false);
        }
        updateLegend();
    }

    // markers
    if (settings->groupBy != LTM_TOD)
        refreshMarkers(settings->start.date(), settings->end.date(), settings->groupBy);

    //qDebug()<<"Final tidy.."<<timer.elapsed();

    // plot
    replot();

    //qDebug()<<"Replot and done.."<<timer.elapsed();

}

void
LTMPlot::createTODCurveData(LTMSettings *settings, MetricDetail metricDetail, QVector<double>&x,QVector<double>&y,int&n)
{
    y.clear();
    x.clear();

    x.resize((24+3));
    y.resize((24+3));
    n = (24);

    for (int i=0; i<(24); i++) x[i]=i;

    foreach (SummaryMetrics rideMetrics, *(settings->data)) {

        // filter out unwanted rides
        if (context->isfiltered && !context->filters.contains(rideMetrics.getFileName())) continue;

        double value = rideMetrics.getForSymbol(metricDetail.symbol);

        // check values are bounded to stop QWT going berserk
        if (isnan(value) || isinf(value)) value = 0;

        // Special computed metrics (LTS/STS) have a null metric pointer
        if (metricDetail.metric) {
            // convert from stored metric value to imperial
            if (context->athlete->useMetricUnits == false) {
                value *= metricDetail.metric->conversion();
                value += metricDetail.metric->conversionSum();
            }

            // convert seconds to hours
            if (metricDetail.metric->units(true) == "seconds" ||
                metricDetail.metric->units(true) == tr("seconds")) value /= 3600;
        }

        int array = rideMetrics.getRideDate().time().hour();
        int type = metricDetail.metric ? metricDetail.metric->type() : RideMetric::Average;

        if (metricDetail.uunits == "Ramp" ||
            metricDetail.uunits == tr("Ramp")) type = RideMetric::Total;

        switch (type) {
        case RideMetric::Total:
            y[array] += value;
            break;
        case RideMetric::Average:
            {
            // average should be calculated taking into account
            // the duration of the ride, otherwise high value but
            // short rides will skew the overall average
            y[array] = value; //XXX average is broken
            break;
            }
        case RideMetric::Low:
            if (value < y[array]) y[array] = value;
            break;
        case RideMetric::Peak:
            if (value > y[array]) y[array] = value;
            break;
        }
    }
}

void
LTMPlot::createCurveData(LTMSettings *settings, MetricDetail metricDetail, QVector<double>&x,QVector<double>&y,int&n)
{
    QList<SummaryMetrics> *data = NULL;

    // resize the curve array to maximum possible size
    int maxdays = groupForDate(settings->end.date(), settings->groupBy)
                    - groupForDate(settings->start.date(), settings->groupBy);
    x.resize(maxdays+3); // one for start from zero plus two for 0 value added at head and tail
    y.resize(maxdays+3); // one for start from zero plus two for 0 value added at head and tail

    // Get metric data, either from metricDB for RideFile metrics
    // or from StressCalculator for PM type metrics
    QList<SummaryMetrics> PMCdata;
    if (metricDetail.type == METRIC_DB || metricDetail.type == METRIC_META) {
        data = settings->data;
    } else if (metricDetail.type == METRIC_MEASURE) {
        data = settings->measures;
    } else if (metricDetail.type == METRIC_PM) {
        createPMCCurveData(settings, metricDetail, PMCdata);
        data = &PMCdata;
    } else if (metricDetail.type == METRIC_BEST) {
        data = settings->bests;
    }

    n=-1;
    int lastDay=0;
    unsigned long secondsPerGroupBy=0;
    bool wantZero = (metricDetail.curveStyle == QwtPlotCurve::Steps);
    foreach (SummaryMetrics rideMetrics, *data) { 

        // filter out unwanted rides but not for PMC type metrics
        // because that needs to be done in the stress calculator
        if (metricDetail.type != METRIC_PM && context->isfiltered && 
            !context->filters.contains(rideMetrics.getFileName())) continue;

        // day we are on
        int currentDay = groupForDate(rideMetrics.getRideDate().date(), settings->groupBy);

        // value for day -- measures are stored differently
        double value;
        if (metricDetail.type == METRIC_MEASURE)
            value = rideMetrics.getText(metricDetail.symbol, "0.0").toDouble();
        else if (metricDetail.type == METRIC_BEST)
            value = rideMetrics.getForSymbol(metricDetail.bestSymbol);
        else
            value = rideMetrics.getForSymbol(metricDetail.symbol);

        // check values are bounded to stop QWT going berserk
        if (isnan(value) || isinf(value)) value = 0;

        // Special computed metrics (LTS/STS) have a null metric pointer
        if (metricDetail.type != METRIC_BEST && metricDetail.metric) {
            // convert from stored metric value to imperial
            if (context->athlete->useMetricUnits == false) {
                value *= metricDetail.metric->conversion();
                value += metricDetail.metric->conversionSum();
            }

            // convert seconds to hours
            if (metricDetail.metric->units(true) == "seconds" ||
                metricDetail.metric->units(true) == tr("seconds")) value /= 3600;
        }

        if (value || wantZero) {
            unsigned long seconds = rideMetrics.getForSymbol("workout_time");
            if (metricDetail.type == METRIC_BEST || metricDetail.type == METRIC_MEASURE) seconds = 1;
            if (currentDay > lastDay) {
                if (lastDay && wantZero) {
                    while (lastDay<currentDay) {
                        lastDay++;
                        n++;
                        x[n]=lastDay - groupForDate(settings->start.date(), settings->groupBy);
                        y[n]=0;
                    }
                } else {
                    n++;
                }
                y[n] = value;
                x[n] = currentDay - groupForDate(settings->start.date(), settings->groupBy);
                secondsPerGroupBy = seconds; // reset for new group
            } else {
                // sum totals, average averages and choose best for Peaks
                int type = metricDetail.metric ? metricDetail.metric->type() : RideMetric::Average;

                if (metricDetail.uunits == "Ramp" ||
                    metricDetail.uunits == tr("Ramp")) type = RideMetric::Total;

                if (metricDetail.type == METRIC_BEST) type = RideMetric::Peak;

                switch (type) {
                case RideMetric::Total:
                    y[n] += value;
                    break;
                case RideMetric::Average:
                    {
                    // average should be calculated taking into account
                    // the duration of the ride, otherwise high value but
                    // short rides will skew the overall average
                    y[n] = ((y[n]*secondsPerGroupBy)+(seconds*value)) / (secondsPerGroupBy+seconds);
                    break;
                    }
                case RideMetric::Low:
                    if (value < y[n]) y[n] = value;
                    break;
                case RideMetric::Peak:
                    if (value > y[n]) y[n] = value;
                    break;
                }
                secondsPerGroupBy += seconds; // increment for same group
            }
            lastDay = currentDay;
        }
    }
}

void
LTMPlot::createPMCCurveData(LTMSettings *settings, MetricDetail metricDetail,
                            QList<SummaryMetrics> &customData)
{
    QDate earliest, latest; // rides
    QString scoreType;

    // create a custom set of summary metric data!
    if (metricDetail.symbol.startsWith("skiba")) {
        scoreType = "skiba_bike_score";
    } else if (metricDetail.symbol.startsWith("coggan")) {
        scoreType = "coggan_tss";
    } else if (metricDetail.symbol.startsWith("daniels")) {
        scoreType = "daniels_points";
    } else if (metricDetail.symbol.startsWith("trimp")) {
        scoreType = "trimp_points";
    } else if (metricDetail.symbol.startsWith("work")) {
        scoreType = "total_work";
    } else if (metricDetail.symbol.startsWith("distance")) {
        scoreType = "total_distance";
    }

    // create the Stress Calculation List
    // FOR ALL RIDE FILES
	StressCalculator *sc ;

    if (scoreType == "coggan_tss" && cogganPMC) {
        sc = cogganPMC;
    } else if (scoreType == "skiba_bike_score" && skibaPMC) {
        sc = skibaPMC;
    } else {
        sc = new StressCalculator(
                context->athlete->cyclist,
		        settings->start,
		        settings->end,
		        (appsettings->value(this, GC_STS_DAYS,7)).toInt(),
                (appsettings->value(this, GC_LTS_DAYS,42)).toInt());

        sc->calculateStress(context, context->athlete->home.absolutePath(), scoreType, settings->ltmTool->isFiltered(), settings->ltmTool->filters());

    }

    // pick out any data that is in the date range selected
    // convert to SummaryMetric Format used on the plot
    for (int i=0; i< sc->n(); i++) {

        SummaryMetrics add = SummaryMetrics();
        add.setRideDate(settings->start.addDays(i));
        if (scoreType == "skiba_bike_score") {
            add.setForSymbol("skiba_lts", sc->getLTSvalues()[i]);
            add.setForSymbol("skiba_sts", sc->getSTSvalues()[i]);
            add.setForSymbol("skiba_sb",  sc->getSBvalues()[i]);
            add.setForSymbol("skiba_sr", sc->getSRvalues()[i]);
            add.setForSymbol("skiba_lr", sc->getLRvalues()[i]);
        } else if (scoreType == "coggan_tss") {
            add.setForSymbol("coggan_ctl", sc->getLTSvalues()[i]);
            add.setForSymbol("coggan_atl", sc->getSTSvalues()[i]);
            add.setForSymbol("coggan_tsb",  sc->getSBvalues()[i]);
            add.setForSymbol("coggan_sr", sc->getSRvalues()[i]);
            add.setForSymbol("coggan_lr", sc->getLRvalues()[i]);
        } else if (scoreType == "daniels_points") {
            add.setForSymbol("daniels_lts", sc->getLTSvalues()[i]);
            add.setForSymbol("daniels_sts", sc->getSTSvalues()[i]);
            add.setForSymbol("daniels_sb",  sc->getSBvalues()[i]);
            add.setForSymbol("daniels_sr", sc->getSRvalues()[i]);
            add.setForSymbol("daniels_lr", sc->getLRvalues()[i]);
        } else if (scoreType == "trimp_points") {
            add.setForSymbol("trimp_lts", sc->getLTSvalues()[i]);
            add.setForSymbol("trimp_sts", sc->getSTSvalues()[i]);
            add.setForSymbol("trimp_sb",  sc->getSBvalues()[i]);
            add.setForSymbol("trimp_sr", sc->getSRvalues()[i]);
            add.setForSymbol("trimp_lr", sc->getLRvalues()[i]);
        } else if (scoreType == "total_work") {
            add.setForSymbol("work_lts", sc->getLTSvalues()[i]);
            add.setForSymbol("work_sts", sc->getSTSvalues()[i]);
            add.setForSymbol("work_sb",  sc->getSBvalues()[i]);
            add.setForSymbol("work_sr", sc->getSRvalues()[i]);
            add.setForSymbol("work_lr", sc->getLRvalues()[i]);
        } else if (scoreType == "total_distance") {
            add.setForSymbol("distance_lts", sc->getLTSvalues()[i]);
            add.setForSymbol("distance_sts", sc->getSTSvalues()[i]);
            add.setForSymbol("distance_sb",  sc->getSBvalues()[i]);
            add.setForSymbol("distance_sr", sc->getSRvalues()[i]);
            add.setForSymbol("distance_lr", sc->getLRvalues()[i]);
        }
        add.setForSymbol("workout_time", 1.0); // averaging is per day
        customData << add;

    }

    if (scoreType == "coggan_tss") {
        cogganPMC = sc;
    } else if (scoreType == "skiba_bike_score") {
        skibaPMC = sc;
    } else {
        delete sc;
    }
}

QwtAxisId
LTMPlot::chooseYAxis(QString units)
{
    QwtAxisId chosen(-1,-1);
    // return the YAxis to use
    if ((chosen = axes.value(units, QwtAxisId(-1,-1))) != QwtAxisId(-1,-1)) return chosen;
    else if (axes.count() < 8) {
        chosen = supportedAxes[axes.count()];
        if (units == "seconds" || units == tr("seconds")) setAxisTitle(chosen, tr("hours")); // we convert seconds to hours
        else setAxisTitle(chosen, units);
        enableAxis(chosen.id, true);
        setAxisVisible(chosen, true);
        axes.insert(units, chosen);
        return chosen;
    } else {
        // eek!
        return QwtAxis::yLeft; // just re-use the current yLeft axis
    }
}

int
LTMPlot::groupForDate(QDate date, int groupby)
{
    switch(groupby) {
    case LTM_WEEK:
        {
        // must start from 1 not zero!
        return 1 + ((date.toJulianDay() - settings->start.date().toJulianDay()) / 7);
        }
    case LTM_MONTH: return (date.year()*12) + date.month();
    case LTM_YEAR:  return date.year();
    case LTM_DAY:
    default:
        return date.toJulianDay();

    }
}

void
LTMPlot::pointHover(QwtPlotCurve *curve, int index)
{
    if (index >= 0 && curve != highlighter) {

        int stacknum = stacks.value(curve, -1);

        const RideMetricFactory &factory = RideMetricFactory::instance();
        double value;
        QString units;
        int precision = 0;
        QString datestr;

        LTMScaleDraw *lsd = new LTMScaleDraw(settings->start, groupForDate(settings->start.date(), settings->groupBy), settings->groupBy);
        QwtText startText = lsd->label((int)(curve->sample(index).x()+0.5));

        if (settings->groupBy != LTM_WEEK)
            datestr = startText.text();
        else
            datestr = QString(tr("Week Commencing %1")).arg(startText.text());

        datestr = datestr.replace('\n', ' ');

        // we reference the metric definitions of name and
        // units to decide on the level of precision required
        QHashIterator<QString, QwtPlotCurve*> c(curves);
        while (c.hasNext()) {
            c.next();
            if (c.value() == curve) {
                const RideMetric *metric =factory.rideMetric(c.key());
                units = metric ? metric->units(context->athlete->useMetricUnits) : "";
                precision = metric ? metric->precision() : 1;

                // BikeScore, RI and Daniels Points have no units
                if (units == "" && metric != NULL) {
                    QTextEdit processHTML(factory.rideMetric(c.key())->name());
                    units  = processHTML.toPlainText();
                }
                break;
            }
        }

        // the point value
        value = curve->sample(index).y();

        // de-aggregate stacked values
        if (stacknum > 0) {
            value = stackY[stacknum]->at(index) - stackY[stacknum-1]->at(index); // de-aggregate
        }

        // convert seconds to hours for the LTM plot
        if (units == "seconds" || units == tr("seconds")) {
            units = "hours"; // we translate from seconds to hours
            value = ceil(value*10.0)/10.0;
            precision = 1; // need more precision now
        }

        // output the tooltip
        QString text = QString("%1\n%2\n%3 %4")
                        .arg(datestr)
                        .arg(curve->title().text())
                        .arg(value, 0, 'f', precision)
                        .arg(this->axisTitle(curve->yAxis()).text());

        // set that text up
        parent->toolTip()->setText(text);
    } else {
        // no point
        parent->toolTip()->setText("");
    }
}

void
LTMPlot::pointClicked(QwtPlotCurve *curve, int index)
{
    if (index >= 0 && curve != highlighter) {
        // setup the popup
        parent->pointClicked(curve, index);
    }
}

// aggregate curve data, adds w to a and
// updates a directly. arrays MUST be of
// equal dimensions
void
LTMPlot::aggregateCurves(QVector<double> &a, QVector<double>&w)
{
    if (a.size() != w.size()) return; // ignore silently

    // add them in!
    for(int i=0; i<a.size(); i++) a[i] += w[i];
}

/*----------------------------------------------------------------------
 * Draw Power Zone Shading on Background (here to end of source file)
 *
 * THANKS TO DAMIEN GRAUSER FOR GETTING THIS WORKING TO SHOW
 * ZONE SHADING OVER TIME. WHEN CP CHANGES THE ZONE SHADING AND
 * LABELLING CHANGES TOO. NEAT.
 *--------------------------------------------------------------------*/
class LTMPlotBackground: public QwtPlotItem
{
    private:
        LTMPlot *parent;

    public:

        LTMPlotBackground(LTMPlot *_parent, QwtAxisId axisid)
        {
            //setAxis(QwtPlot::xBottom, axisid);
            setXAxis(axisid);
            setZ(0.0);
            parent = _parent;
        }

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    virtual void draw(QPainter *painter,
                      const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                      const QRectF &rect) const
    {
        const Zones *zones       = parent->parent->context->athlete->zones();
        int zone_range_size     = parent->parent->context->athlete->zones()->getRangeSize();

        if (zone_range_size >= 0) { //parent->shadeZones() &&
            for (int i = 0; i < zone_range_size; i ++) {
            int zone_range = i;
            int left = xMap.transform(parent->groupForDate(zones->getStartDate(zone_range), parent->settings->groupBy)- parent->groupForDate(parent->settings->start.date(), parent->settings->groupBy));

            /* The +50 pixels is for a QWT bug? cover the little gap on the right? */
            int right = xMap.transform(parent->maxX + 0.5) + 50;

            if (right<0)
                right= xMap.transform(parent->groupForDate(parent->settings->end.date(), parent->settings->groupBy) - parent->groupForDate(parent->settings->start.date(), parent->settings->groupBy));

            QList <int> zone_lows = zones->getZoneLows(zone_range);
            int num_zones = zone_lows.size();
            if (num_zones > 0) {
                for (int z = 0; z < num_zones; z ++) {
                    QRectF r = rect;
                    r.setLeft(left);
                    r.setRight(right);

                    QColor shading_color = zoneColor(z, num_zones);
                    shading_color.setHsv(
                        shading_color.hue(),
                        shading_color.saturation() / 4,
                        shading_color.value()
                        );
                    r.setBottom(yMap.transform(zone_lows[z]));
                    if (z + 1 < num_zones)
                        r.setTop(yMap.transform(zone_lows[z + 1]));
                    if (r.top() <= r.bottom())
                        painter->fillRect(r, shading_color);
                }
            }
            }
        }
    }
};


// Zone labels are drawn if power zone bands are enabled, automatically
// at the center of the plot
class LTMPlotZoneLabel: public QwtPlotItem
{
    private:
        LTMPlot *parent;
        int zone_number;
        double watts;
        QwtText text;

    public:
        LTMPlotZoneLabel(LTMPlot *_parent, int _zone_number, QwtAxisId axisid, LTMSettings *settings)
        {
            parent = _parent;
            zone_number = _zone_number;

            const Zones *zones       = parent->parent->context->athlete->zones();
            int zone_range     = zones->whichRange(settings->start.addDays((settings->end.date().toJulianDay()-settings->start.date().toJulianDay())/2).date());

            // which axis has watts?
            setXAxis(axisid);

            // create new zone labels if we're shading
            if (zone_range >= 0) { //parent->shadeZones()
                QList <int> zone_lows = zones->getZoneLows(zone_range);
                QList <QString> zone_names = zones->getZoneNames(zone_range);
                int num_zones = zone_lows.size();
                if (zone_names.size() != num_zones) return;
                if (zone_number < num_zones) {
                    watts =
                        (
                            (zone_number + 1 < num_zones) ?
                            0.5 * (zone_lows[zone_number] + zone_lows[zone_number + 1]) :
                            (
                                (zone_number > 0) ?
                                (1.5 * zone_lows[zone_number] - 0.5 * zone_lows[zone_number - 1]) :
                                2.0 * zone_lows[zone_number]
                            )
                        );
                    text = QwtText(zone_names[zone_number]);
                    text.setFont(QFont("Helvetica",20, QFont::Bold));
                    QColor text_color = zoneColor(zone_number, num_zones);
                    text_color.setAlpha(64);
                    text.setColor(text_color);
                }
            }
            setZ(1.0 + zone_number / 100.0);
        }

        virtual int rtti() const
        {
            return QwtPlotItem::Rtti_PlotUserItem;
        }

        void draw(QPainter *painter,
                  const QwtScaleMap &, const QwtScaleMap &yMap,
                  const QRectF &rect) const
        {
            if (true) {//parent->shadeZones()
                int x = (rect.left() + rect.right()) / 2;
                int y = yMap.transform(watts);

                // the following code based on source for QwtPlotMarker::draw()
                QRect tr(QPoint(0, 0), text.textSize(painter->font()).toSize());
                tr.moveCenter(QPoint(x, y));
                text.draw(painter, tr);
            }
        }
};

void
LTMPlot::refreshMarkers(QDate from, QDate to, int groupby)
{
    // clear old markers - if there are any
    foreach(QwtPlotMarker *m, markers) {
        m->detach();
        delete m;
    }
    markers.clear();

    double baseday = groupForDate(from, groupby);

    // seasons and season events
    if (settings->events) {
        foreach (Season s, context->athlete->seasons->seasons) {

            if (s.type != Season::temporary && s.name != settings->title && s.getStart() >= from && s.getStart() < to) {

                QwtPlotMarker *mrk = new QwtPlotMarker;
                markers.append(mrk);
                mrk->attach(this);
                mrk->setLineStyle(QwtPlotMarker::VLine);
                mrk->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
                mrk->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));

                QwtText text(s.getName());
                text.setFont(QFont("Helvetica", 10, QFont::Bold));
                text.setColor(GColor(CPLOTMARKER));
                mrk->setValue(double(groupForDate(s.getStart(), groupby)) - baseday, 0.0);
                mrk->setLabel(text);
            }

            foreach (SeasonEvent event, s.events) {

                if (event.date > from && event.date < to) {

                    // and the events...
                    QwtPlotMarker *mrk = new QwtPlotMarker;
                    markers.append(mrk);
                    mrk->attach(this);
                    mrk->setLineStyle(QwtPlotMarker::VLine);
                    mrk->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
                    mrk->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));

                    QwtText text(event.name);
                    text.setFont(QFont("Helvetica", 10, QFont::Bold));
                    text.setColor(GColor(CPLOTMARKER));
                    mrk->setValue(double(groupForDate(event.date, groupby)) - baseday, 10.0);
                    mrk->setLabel(text);
                }
            }
        }
    }
    return;
}

void
LTMPlot::refreshZoneLabels(QwtAxisId axisid)
{
    foreach(LTMPlotZoneLabel *label, zoneLabels) {
        label->detach();
        delete label;
    }
    zoneLabels.clear();

    if (bg) {
        bg->detach();
        delete bg;
        bg = NULL;
    }
    if (axisid == QwtAxisId(-1,-1)) return; // our job is done - no zones to plot

    const Zones *zones       = context->athlete->zones();

    if (zones == NULL || zones->getRangeSize()==0) return; // no zones to plot

    int zone_range     = 0; // first range

    // generate labels for existing zones
    if (zone_range >= 0) {
        int num_zones = zones->numZones(zone_range);
        for (int z = 0; z < num_zones; z ++) {
            LTMPlotZoneLabel *label = new LTMPlotZoneLabel(this, z, axisid, settings);
            label->attach(this);
            zoneLabels.append(label);
        }
    }
    bg = new LTMPlotBackground(this, axisid);
    bg->attach(this);
}
