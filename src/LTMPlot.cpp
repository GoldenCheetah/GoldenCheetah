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

#include "LTMPlot.h"
#include "LTMTool.h"
#include "LTMTrend.h"
#include "LTMWindow.h"
#include "MetricAggregator.h"
#include "SummaryMetrics.h"
#include "RideMetric.h"
#include "Settings.h"
#include "Colors.h"

#include "StressCalculator.h" // for LTS/STS calculation

#include <QSettings>

#include <qwt_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_curve_fitter.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>

#include <math.h> // for isinf() isnan()
#include <boost/shared_ptr.hpp>

static int supported_axes[] = { QwtPlot::yLeft, QwtPlot::yRight, QwtPlot::yLeft1, QwtPlot::yRight1, QwtPlot::yLeft2, QwtPlot::yRight2, QwtPlot::yLeft3, QwtPlot::yRight3 };

LTMPlot::LTMPlot(LTMWindow *parent, MainWindow *main, QDir home) : bg(NULL), parent(parent), main(main),
                                                                   home(home), highlighter(NULL)
{
    // get application settings
    boost::shared_ptr<QSettings> appsettings = GetApplicationSettings();
    useMetricUnits = appsettings->value(GC_UNIT).toString() == "Metric";

    insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setAxisTitle(yLeft, tr(""));
    setAxisTitle(xBottom, tr("Date"));
    setAxisMaxMinor(QwtPlot::xBottom,-1);
    setAxisScaleDraw(QwtPlot::xBottom, new LTMScaleDraw(QDateTime::currentDateTime(), 0, LTM_DAY));

    grid = new QwtPlotGrid();
    grid->enableX(false);
    grid->attach(this);

    settings = NULL;

    configUpdate(); // set basic colors

    connect(main, SIGNAL(configChanged()), this, SLOT(configUpdate()));
}

LTMPlot::~LTMPlot()
{
}

void
LTMPlot::configUpdate()
{
    // get application settings
    boost::shared_ptr<QSettings> appsettings = GetApplicationSettings();
    useMetricUnits = appsettings->value(GC_UNIT).toString() == "Metric";

    // set basic plot colors
    setCanvasBackground(GColor(CPLOTBACKGROUND));
    QPen gridPen(GColor(CPLOTGRID));
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);
}

void
LTMPlot::setData(LTMSettings *set)
{
    settings = set;

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

    setTitle(settings->title);

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
    for (int i=0; i<8; i++) enableAxis(supported_axes[i], false);
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

        // remove the shading if it exists
        refreshZoneLabels(-1);

        replot();
        return;
    }

    // count the bars since we format them side by side and need
    // to now how to offset them from each other
    int barnum=0;
    int bars = 0;
    foreach(MetricDetail metricDetail, settings->metrics)
        if (metricDetail.curveStyle == QwtPlotCurve::Steps)
            bars++;

    // setup the curves
    int count;
    foreach (MetricDetail metricDetail, settings->metrics) {

        QVector<double> xdata, ydata;
        createCurveData(settings, metricDetail, xdata, ydata, count);

        // Create a curve
        QwtPlotCurve *current = new QwtPlotCurve(metricDetail.uname);
        curves.insert(metricDetail.symbol, current);
        current->setRenderHint(QwtPlotItem::RenderAntialiased);
        QPen cpen = QPen(metricDetail.penColor);
        cpen.setWidth(1.0);
        current->setPen(cpen);
        current->setStyle(metricDetail.curveStyle);

        QwtSymbol sym;
        sym.setStyle(metricDetail.symbolStyle);

        // choose the axis
        int axisid = chooseYAxis(metricDetail.uunits);
        current->setYAxis(axisid);

        // left and right offset for bars
        double left = 0;
        double right = 0;
        double middle = 0;
        if (metricDetail.curveStyle == QwtPlotCurve::Steps) {
            double space = double(0.8) / bars;
            double gap = space * 0.10;
            double width = space * 0.90;
            left = (space * barnum) + (gap / 2) + 0.1;
            right = left + width;
            middle = ((left+right) / double(2)) - 0.5;
            barnum++;
        }

        // trend - clone the data for the curve and add a curvefitted
        //         curve with no symbols and use a dashed pen
        // need more than 2 points for a trend line
        if (metricDetail.trend == true && count > 2) {

            QString trendName = QString(tr("%1 trend")).arg(metricDetail.uname);
            QString trendSymbol = QString("%1_trend").arg(metricDetail.symbol);
            QwtPlotCurve *trend = new QwtPlotCurve(trendName);

            // cosmetics
            QPen cpen = QPen(metricDetail.penColor.darker(200));
            cpen.setWidth(4.0);
            cpen.setStyle(Qt::DotLine);
            trend->setPen(cpen);
            trend->setRenderHint(QwtPlotItem::RenderAntialiased);
            trend->setBaseline(0);
            trend->setYAxis(axisid);
            trend->setStyle(QwtPlotCurve::Lines);

            // perform linear regression
            LTMTrend regress(xdata.data(), ydata.data(), count);
            double xtrend[2], ytrend[2];
            xtrend[0] = 0.0;
            ytrend[0] = regress.getYforX(0.0);
            xtrend[1] = xdata[count];
            ytrend[1] = regress.getYforX(xdata[count]);
            trend->setData(xtrend,ytrend, 2);

            trend->attach(this);
            curves.insert(trendSymbol, trend);
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

            QString topSymbol = QString("%1_topN").arg(metricDetail.symbol);
            QwtPlotCurve *top = new QwtPlotCurve(topName);
            curves.insert(topSymbol, top);

            top->setRenderHint(QwtPlotItem::RenderAntialiased);
            top->setStyle(QwtPlotCurve::Dots);

            // we might have hidden the symbols for this curve
            // if its set to none then default to a rectangle
            if (metricDetail.symbolStyle == QwtSymbol::NoSymbol)
                sym.setStyle(QwtSymbol::Rect);
            sym.setSize(12);
            QColor lighter = metricDetail.penColor;
            lighter.setAlpha(50);
            sym.setPen(metricDetail.penColor);
            sym.setBrush(lighter);

            top->setSymbol(sym);
            top->setData(hxdata.data(),hydata.data(), counter);
            top->setBaseline(0);
            top->setYAxis(axisid);
            top->attach(this);
        }

        if (metricDetail.curveStyle == QwtPlotCurve::Steps) {

            // fill the bars
            QColor brushColor = metricDetail.penColor;
            brushColor.setAlpha(200); // now side by side, less transparency required
            QBrush brush = QBrush(brushColor);
            current->setBrush(brush);
            current->setPen(cpen);
            current->setCurveAttribute(QwtPlotCurve::Inverted, true);

            // XXX Symbol for steps looks horrible
            sym.setStyle(QwtSymbol::NoSymbol);
            sym.setPen(QPen(metricDetail.penColor));
            sym.setBrush(QBrush(metricDetail.penColor));
            current->setSymbol(sym);

            // XXX FUDGE QWT's LACK OF A BAR CHART
            // add a zero point at the head and tail so the
            // histogram columns look nice.
            // and shift all the x-values left by 0.5 so that
            // they centre over x-axis labels
            int i=0;
            for (i=0; i<=count; i++) xdata[i] -= 0.5;
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
                yaxis[offset] = 0;
                offset++;
                xaxis[offset] = x+left;
                yaxis[offset] = y;
                offset++;
                xaxis[offset] = x+right;
                yaxis[offset] = y;
                offset++;
                xaxis[offset] = x +right;
                yaxis[offset] = 0;
                offset++;
            }
            xdata = xaxis;
            ydata = yaxis;
            count *= 4;
            // END OF FUDGE

        } else if (metricDetail.curveStyle == QwtPlotCurve::Lines) {

            QPen cpen = QPen(metricDetail.penColor);
            cpen.setWidth(2.0);
            sym.setSize(6);
            sym.setPen(QPen(metricDetail.penColor));
            sym.setBrush(QBrush(metricDetail.penColor));
            current->setSymbol(sym);
            current->setPen(cpen);


        } else if (metricDetail.curveStyle == QwtPlotCurve::Dots) {
            sym.setSize(6);
            sym.setPen(QPen(metricDetail.penColor));
            sym.setBrush(QBrush(metricDetail.penColor));
            current->setSymbol(sym);

        } else if (metricDetail.curveStyle == QwtPlotCurve::Sticks) {

            sym.setSize(4);
            sym.setPen(QPen(metricDetail.penColor));
            sym.setBrush(QBrush(Qt::white));
            current->setSymbol(sym);

        }

        // smoothing
        if (metricDetail.smooth == true) {
            current->setCurveAttribute(QwtPlotCurve::Fitted, true);
        }

        // set the data series
        current->setData(xdata.data(),ydata.data(), count+1);
        current->setBaseline(metricDetail.baseline);

        // update min/max Y values for the chosen axis
        if (current->maxYValue() > maxY[axisid]) maxY[axisid] = current->maxYValue();
        if (current->minYValue() < minY[axisid]) minY[axisid] = current->minYValue();

        current->attach(this);


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

    // run through the Y axis
    for (int i=0; i<10; i++) {
        // set the scale on the axis
        if (i != xBottom && i != xTop) {
            maxY[i] *= 1.1; // add 10% headroom
            setAxisScale(i, minY[i], maxY[i]);
        }
    }

    QString format = axisTitle(yLeft).text();
    parent->toolTip()->setAxis(xBottom, yLeft);
    parent->toolTip()->setFormat(format);

    // draw zone labels axisid of -1 means delete whats there
    // cause no watts are being displayed
    if (settings->shadeZones == true) {
        int axisid = axes.value("watts", -1);
#ifdef ENABLE_METRICS_TRANSLATION
        if (axisid == -1) axisid = axes.value(tr("watts"), -1); // Try translated version
#endif
        refreshZoneLabels(axisid);
    } else
        refreshZoneLabels(-1); // turn em off

    // plot
    replot();
}

void
LTMPlot::createCurveData(LTMSettings *settings, MetricDetail metricDetail, QVector<double>&x,QVector<double>&y,int&n)
{
    QList<SummaryMetrics> *data;

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
    } else if (metricDetail.type == METRIC_PM) {
        createPMCCurveData(settings, metricDetail, PMCdata);
        data = &PMCdata;
    }

    n=-1;
    int lastDay=0;
    unsigned long secondsPerGroupBy=0;
    bool wantZero = (metricDetail.curveStyle == QwtPlotCurve::Steps);
    foreach (SummaryMetrics rideMetrics, *data) {

        // day we are on
        int currentDay = groupForDate(rideMetrics.getRideDate().date(), settings->groupBy);

        // value for day
        double value = rideMetrics.getForSymbol(metricDetail.symbol);

        // check values are bounded to stop QWT going berserk
        if (isnan(value) || isinf(value)) value = 0;

        // Special computed metrics (LTS/STS) have a null metric pointer
        if (metricDetail.metric) {
            // convert from stored metric value to imperial
            if (useMetricUnits == false) value *= metricDetail.metric->conversion();

            // convert seconds to hours
            if (metricDetail.metric->units(true) == "seconds" || metricDetail.metric->units(true) == tr("seconds")) value /= 3600;
        }

        if (value || wantZero) {
            unsigned long seconds = rideMetrics.getForSymbol("workout_time");
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

                if (metricDetail.uunits == "Ramp" || metricDetail.uunits == tr("Ramp")) type = RideMetric::Total;

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
    boost::shared_ptr<QSettings> appsettings = GetApplicationSettings();
    QString scoreType;

    // create a custom set of summary metric data!
    if (metricDetail.name.startsWith("Skiba")) {
        scoreType = "skiba_bike_score";
    } else if (metricDetail.name.startsWith("Daniels")) {
        scoreType = "daniels_points";
    } else if (metricDetail.name.startsWith("TRIMP")) {
        scoreType = "trimp_points";
    }

    // create the Stress Calculation List
    // FOR ALL RIDE FILES
	StressCalculator *sc = new StressCalculator(
		    settings->start,
		    settings->end,
		    (appsettings->value(GC_INITIAL_STS)).toInt(),
		    (appsettings->value(GC_INITIAL_LTS)).toInt(),
		    (appsettings->value(GC_STS_DAYS,7)).toInt(),
		    (appsettings->value(GC_LTS_DAYS,42)).toInt());

    sc->calculateStress(main, home.absolutePath(), scoreType);

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
        }
        add.setForSymbol("workout_time", 1.0); // averaging is per day
        customData << add;

    }
    delete sc;
}

int
LTMPlot::chooseYAxis(QString units)
{
    int chosen;

    // return the YAxis to use
    if ((chosen = axes.value(units, -1)) != -1) return chosen;
    else if (axes.count() < 8) {
        chosen = supported_axes[axes.count()];
        if (units == "seconds" || units == tr("seconds")) setAxisTitle(chosen, tr("hours")); // we convert seconds to hours
        else setAxisTitle(chosen, units);
        enableAxis(chosen, true);
        axes.insert(units, chosen);
        return chosen;
    } else {
        // eek!
        return yLeft; // just re-use the current yLeft axis
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
        const RideMetricFactory &factory = RideMetricFactory::instance();
        double value;
        QString units;
        int precision;
        QString datestr;

        LTMScaleDraw *lsd = new LTMScaleDraw(settings->start, groupForDate(settings->start.date(), settings->groupBy), settings->groupBy);
        QwtText startText = lsd->label((int)(curve->x(index)+0.5));
        QwtText endText;
        endText   = lsd->label((int)(curve->x(index)+1.5));


        if (settings->groupBy != LTM_WEEK)
            datestr = startText.text();
        else
            datestr = QString("%1 - %2").arg(startText.text()).arg(endText.text());

        datestr = datestr.replace('\n', ' ');

        // we reference the metric definitions of name and
        // units to decide on the level of precision required
        QHashIterator<QString, QwtPlotCurve*> c(curves);
        while (c.hasNext()) {
            c.next();
            if (c.value() == curve) {
                const RideMetric *metric =factory.rideMetric(c.key());
                units = metric ? metric->units(useMetricUnits) : "";
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
        value = curve->y(index);

        // convert seconds to hours for the LTM plot
        if (units == "seconds" || units == tr("seconds")) {
            units = tr("hours"); // we translate from seconds to hours
            value = ceil(curve->y(index)*10.0)/10.0;
            precision = 1; // new more precision since converting to hours
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

// start of date range selection
void
LTMPlot::pickerAppended(QPoint pos)
{
    // ony work once we have a chart to do it on
    if (settings == NULL) return;

    // allow user to select a date range across the plot
    if (highlighter) {
        // detach and delete
        highlighter->detach();
        delete highlighter;
    }
    highlighter = new QwtPlotCurve("Date Selection");
    double curveDataX[4]; // simple 4 point line
    double curveDataY[4]; // simple 4 point line

    // get x
    int x = invTransform(xBottom, pos.x());

    // trying to select a range on anull plot
    if (maxY[yLeft] == 0) {
        enableAxis(yLeft, true);
        setAxisTitle(yLeft, tr("watts")); // as good as any
        setAxisScale(yLeft, 0, 1000);
        maxY[yLeft] = 1000;
    }

    // get min/max y
    curveDataX[0]=x;
    curveDataY[0]=minY[yLeft];
    curveDataX[1]=x;
    curveDataY[1]=maxY[yLeft];

    // no right then down - updated by pickerMoved
    curveDataX[2]=curveDataX[1];
    curveDataY[2]=curveDataY[1];
    curveDataX[3]=curveDataX[0];
    curveDataY[3]=curveDataY[3];

    // color
    QColor ccol(GColor(CINTERVALHIGHLIGHTER));
    ccol.setAlpha(64);
    QPen cpen = QPen(ccol);
    cpen.setWidth(1.0);
    QBrush cbrush = QBrush(ccol);
    highlighter->setPen(cpen);
    highlighter->setBrush(cbrush);
    highlighter->setStyle(QwtPlotCurve::Lines);

    highlighter->setData(curveDataX,curveDataY, 4);

    // axis etc
    highlighter->setYAxis(QwtPlot::yLeft);
    highlighter->attach(this);
    highlighter->show();

    // what is the start date?
    LTMScaleDraw *lsd = new LTMScaleDraw(settings->start,
                            groupForDate(settings->start.date(),
                            settings->groupBy),
                            settings->groupBy);
    start = lsd->toDate((int)x);
    end = start.addYears(10);
    name = QString("%1 - ").arg(start.toString("d MMM yy"));
    seasonid = settings->ltmTool->newSeason(name, start, end, Season::adhoc);

    replot();
}

// end of date range selection
void
LTMPlot::pickerMoved(QPoint pos)
{
    if (settings == NULL) return;

    // allow user to select a date range across the plot
    double curveDataX[4]; // simple 4 point line
    double curveDataY[4]; // simple 4 point line

    // get x
    int x = invTransform(xBottom, pos.x());

    // update to reflect new x position
    curveDataX[0]=highlighter->x(0);
    curveDataY[0]=highlighter->y(0);
    curveDataX[1]=highlighter->x(0);
    curveDataY[1]=highlighter->y(1);
    curveDataX[2]=x;
    curveDataY[2]=curveDataY[1];
    curveDataX[3]=x;
    curveDataY[3]=curveDataY[3];

    // what is the end date?
    LTMScaleDraw *lsd = new LTMScaleDraw(settings->start,
                            groupForDate(settings->start.date(),
                            settings->groupBy),
                            settings->groupBy);
    end = lsd->toDate((int)x);
    name = QString("%1 - %2").arg(start.toString("d MMM yy"))
                             .arg(end.toString("d MMM yy"));
    settings->ltmTool->updateSeason(seasonid, name, start, end, Season::adhoc);

    // update and replot highlighter
    highlighter->setData(curveDataX,curveDataY, 4);
    replot();
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

        LTMPlotBackground(LTMPlot *_parent, int axisid)
        {
            setAxis(QwtPlot::xBottom, axisid);
            setZ(0.0);
            parent = _parent;
        }

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    virtual void draw(QPainter *painter,
                      const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                      const QRect &rect) const
    {
        const Zones *zones       = parent->parent->main->zones();
        int zone_range_size     = parent->parent->main->zones()->getRangeSize();

        //fprintf(stderr, "size: %d\n",zone_range_size);
        if (zone_range_size >= 0) { //parent->shadeZones() &&
            for (int i = 0; i < zone_range_size; i ++) {
            int zone_range = i;
            //int zone_range = zones->whichRange(parent->settings->start.addDays((parent->settings->end.date().toJulianDay()-parent->settings->start.date().toJulianDay())/2).date()); // XXX Damien fixup

            int left = xMap.transform(parent->groupForDate(zones->getStartDate(zone_range), parent->settings->groupBy)- parent->groupForDate(parent->settings->start.date(), parent->settings->groupBy));

            //fprintf(stderr, "%d left: %d\n",i,left);
            //int right = xMap.transform(parent->groupForDate(zones->getEndDate(zone_range), parent->settings->groupBy)- parent->groupForDate(parent->settings->start.date(), parent->settings->groupBy));
            //fprintf(stderr, "%d right: %d\n",i,right);

            /* The +50 pixels is for a QWT bug? cover the little gap on the right? */
            int right = xMap.transform(parent->maxX + 0.5) + 50;

            if (right<0)
                right= xMap.transform(parent->groupForDate(parent->settings->end.date(), parent->settings->groupBy) - parent->groupForDate(parent->settings->start.date(), parent->settings->groupBy));

            QList <int> zone_lows = zones->getZoneLows(zone_range);
            int num_zones = zone_lows.size();
            if (num_zones > 0) {
                for (int z = 0; z < num_zones; z ++) {
                    QRect r = rect;
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
        LTMPlotZoneLabel(LTMPlot *_parent, int _zone_number, int axisid, LTMSettings *settings)
        {
            parent = _parent;
            zone_number = _zone_number;

            const Zones *zones       = parent->parent->main->zones();
            //int zone_range     = 0; //parent->parent->mainWindow->zoneRange();
            int zone_range     = zones->whichRange(settings->start.addDays((settings->end.date().toJulianDay()-settings->start.date().toJulianDay())/2).date()); // XXX Damien Fixup

            // which axis has watts?
            setAxis(QwtPlot::xBottom, axisid);

            // create new zone labels if we're shading
            if (zone_range >= 0) { //parent->shadeZones()
                QList <int> zone_lows = zones->getZoneLows(zone_range);
                QList <QString> zone_names = zones->getZoneNames(zone_range);
                int num_zones = zone_lows.size();
                assert(zone_names.size() == num_zones);
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
                    text.setFont(QFont("Helvetica",24, QFont::Bold));
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
                  const QRect &rect) const
        {
            if (true) {//parent->shadeZones()
                int x = (rect.left() + rect.right()) / 2;
                int y = yMap.transform(watts);

                // the following code based on source for QwtPlotMarker::draw()
                QRect tr(QPoint(0, 0), text.textSize(painter->font()));
                tr.moveCenter(QPoint(x, y));
                text.draw(painter, tr);
            }
        }
};

void
LTMPlot::refreshZoneLabels(int axisid)
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
    if (axisid == -1) return; // our job is done - no zones to plot

    const Zones *zones       = main->zones();

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

