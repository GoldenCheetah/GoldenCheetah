/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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

#include "WeeklySummaryWindow.h"
#include "DaysScaleDraw.h"
#include "MainWindow.h"
#include "RideItem.h"
#include "RideMetric.h"
#include "SummaryMetrics.h"
#include "MetricAggregator.h"
#include "Zones.h"
#include "Units.h" // for MILES_PER_KM
#include <QtGui>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_grid.h>
#include <assert.h>

WeeklySummaryWindow::WeeklySummaryWindow(bool useMetricUnits,
                                         MainWindow *mainWindow) :
    GcWindow(mainWindow), mainWindow(mainWindow),
    useMetricUnits(useMetricUnits)
{
    setInstanceName("Weekly Summary Window");
    setControls(NULL);

    QGridLayout *glayout = new QGridLayout;

    // set up the weekly distance / duration plot:
    weeklyPlot = new QwtPlot();
    weeklyPlot->enableAxis(QwtAxis::YRight, true);
    weeklyPlot->setAxisMaxMinor(QwtAxis::XBottom,0);
    weeklyPlot->setAxisScaleDraw(QwtAxis::XBottom, new DaysScaleDraw());
    QFont weeklyPlotAxisFont = weeklyPlot->axisFont(QwtAxis::YLeft);
    weeklyPlotAxisFont.setPointSize(weeklyPlotAxisFont.pointSize() * 0.9f);
    weeklyPlot->setAxisFont(QwtAxis::XBottom, weeklyPlotAxisFont);
    weeklyPlot->setAxisFont(QwtAxis::YLeft, weeklyPlotAxisFont);
    weeklyPlot->setAxisFont(QwtAxis::YRight, weeklyPlotAxisFont);
    weeklyPlot->canvas()->setFrameStyle(QFrame::NoFrame);
    weeklyPlot->setCanvasBackground(Qt::white);

    weeklyDistCurve = new QwtPlotCurve();
    weeklyDistCurve->setStyle(QwtPlotCurve::Steps);
    QPen pen(Qt::SolidLine);
    weeklyDistCurve->setPen(pen);
    weeklyDistCurve->setBrush(Qt::red);
    weeklyDistCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    weeklyDistCurve->setCurveAttribute(QwtPlotCurve::Inverted, true); // inverted, right-to-left
    weeklyDistCurve->attach(weeklyPlot);

    weeklyDurationCurve = new QwtPlotCurve();
    weeklyDurationCurve->setStyle(QwtPlotCurve::Steps);
    weeklyDurationCurve->setPen(pen);
    weeklyDurationCurve->setBrush(QColor(255,200,0,255));
    weeklyDurationCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    weeklyDurationCurve->setCurveAttribute(QwtPlotCurve::Inverted, true); // inverted, right-to-left
    weeklyDurationCurve->setYAxis(QwtAxis::YRight);
    weeklyDurationCurve->attach(weeklyPlot);

    // set up the weekly bike score plot:
    weeklyBSPlot = new QwtPlot();
    weeklyBSPlot->enableAxis(QwtAxis::YRight, true);
    weeklyBSPlot->setAxisMaxMinor(QwtAxis::XBottom,0);
    weeklyBSPlot->setAxisScaleDraw(QwtAxis::XBottom, new DaysScaleDraw());
    QwtText textLabel = QwtText();
    weeklyBSPlot->setAxisFont(QwtAxis::XBottom, weeklyPlotAxisFont);
    weeklyBSPlot->setAxisFont(QwtAxis::YLeft, weeklyPlotAxisFont);
    weeklyBSPlot->setAxisFont(QwtAxis::YRight, weeklyPlotAxisFont);
    weeklyBSPlot->canvas()->setFrameStyle(QFrame::NoFrame);
    weeklyBSPlot->setCanvasBackground(Qt::white);

    weeklyBSCurve = new QwtPlotCurve();
    weeklyBSCurve->setStyle(QwtPlotCurve::Steps);
    weeklyBSCurve->setPen(pen);
    weeklyBSCurve->setBrush(Qt::blue);
    weeklyBSCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    weeklyBSCurve->setCurveAttribute(QwtPlotCurve::Inverted, true); // inverted, right-to-left
    weeklyBSCurve->attach(weeklyBSPlot);

    weeklyRICurve = new QwtPlotCurve();
    weeklyRICurve->setStyle(QwtPlotCurve::Steps);
    weeklyRICurve->setPen(pen);
    weeklyRICurve->setBrush(Qt::green);
    weeklyRICurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    weeklyRICurve->setCurveAttribute(QwtPlotCurve::Inverted, true); // inverted, right-to-left
    weeklyRICurve->setYAxis(QwtAxis::YRight);
    weeklyRICurve->attach(weeklyBSPlot);

    // set baseline curves to obscure linewidth variations along baseline
    pen.setWidth(2);
    weeklyBaselineCurve   = new QwtPlotCurve();
    weeklyBaselineCurve->setPen(pen);
    weeklyBaselineCurve->attach(weeklyPlot);
    weeklyBSBaselineCurve = new QwtPlotCurve();
    weeklyBSBaselineCurve->setPen(pen);
    weeklyBSBaselineCurve->attach(weeklyBSPlot);

    QwtPlotGrid *grid = new QwtPlotGrid();
    grid->enableX(false);
    QPen gridPen;
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);
    grid->attach(weeklyPlot);

    QwtPlotGrid *grid1 = new QwtPlotGrid();
    grid1->enableX(false);
    gridPen.setStyle(Qt::DotLine);
    grid1->setPen(gridPen);
    grid1->attach(weeklyBSPlot);

    weeklySummary = new QTextEdit;
    weeklySummary->setReadOnly(true);
    weeklySummary->setStyleSheet("QTextEdit { border: 0px}");

    QFont defaultFont; // mainwindow sets up the defaults.. we need to apply
    defaultFont.setPointSize(defaultFont.pointSize()-2);
    weeklySummary->setFont(defaultFont);

    glayout->addWidget(weeklySummary,0,0,1,-1); // row, col, rowspan, colspan. -1 == fill to edge
    glayout->addWidget(weeklyPlot,1,0);
    glayout->addWidget(weeklyBSPlot,1,1);

    glayout->setRowStretch(0, 3);           // stretch factor of summary
    glayout->setRowStretch(1, 2);           // stretch factor of weekly plots
    glayout->setColumnStretch(0, 1);        // stretch first column
    glayout->setColumnStretch(1, 1);        // stretch second column
    glayout->setRowMinimumHeight(0, 180);   // minimum height of weekly summary
    glayout->setRowMinimumHeight(1, 120);   // minimum height of weekly plots

    setLayout(glayout);

    //connect(mainWindow, SIGNAL(rideSelected()), this, SLOT(refresh()));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(refresh()));
    connect(mainWindow, SIGNAL(zonesChanged()), this, SLOT(refresh()));
    connect(mainWindow, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh()));
    connect(mainWindow, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh()));
}

void
WeeklySummaryWindow::refresh()
{
    // not active, or null ride item then do nothing
    if (!amVisible()) return;
    const RideItem *ride = myRideItem;
    if (!ride) return;

    // work through the rides
    const QTreeWidgetItem *allRides = mainWindow->allRideItems();

    // start / end of week
    QDate wstart = ride->dateTime.date();
    wstart = wstart.addDays(Qt::Monday - wstart.dayOfWeek());
    assert(wstart.dayOfWeek() == Qt::Monday);

    // weekly values
    QVector<double> time_in_zone; // max 10 zones supported
    QVector<double> time_in_hrzone; // max 10 zones supported

    double weeklyRides=0;
    double weeklySeconds=0;
    double weeklyDistance=0;
    double weeklyW=0;
    double weeklyBS=0;
    double weeklyCS=0;
    double weeklyRI=0;

    // daily values
    double dailyRides[7];
    double dailySeconds[7];
    double dailyDistance[7];
    double dailyW[7];
    double dailyBS[7];
    double dailyXP[7];
    double dailyRI[7];

    // init daily values to zero
    for(int i=0; i<7; i++)
        dailyRides[i] =
        dailySeconds[i] =
        dailyDistance[i] =
        dailyW[i] =
        dailyBS[i] =
        dailyXP[i] =
        dailyRI[i] = 0;

    // how many zones did we see?
    int num_zones = -1;
    int zone_range = -1;

    int num_hrzones = -1;
    int hrzone_range = -1;

    for (int i = 0; i < allRides->childCount(); ++i) {

	QTreeWidgetItem *twi = allRides->child(i);
        RideItem *item = static_cast<RideItem*>(twi);

	int day;

        if (((day = wstart.daysTo(item->dateTime.date())) >= 0) && (day < 7)) {

            // one more ride for this day
            dailyRides[day]++;
            weeklyRides++;

            // get metrics from metricDB (always in metric units)
            SummaryMetrics metrics = mainWindow->metricDB->getRideMetrics(item->fileName);

            // increment daily values
            dailySeconds[day] += metrics.getForSymbol("time_riding");
            dailyDistance[day] += metrics.getForSymbol("total_distance") * (useMetricUnits ? 1 : MILES_PER_KM);
            dailyW[day] += metrics.getForSymbol("total_work");
            dailyBS[day] += metrics.getForSymbol("skiba_bike_score");
            dailyXP[day] += metrics.getForSymbol("skiba_power");

            // average RI for the day
            if (dailyRides[day] > 1)
                dailyRI[day] = ((dailyRI[day] * (dailyRides[day]-1))
                                + metrics.getForSymbol("skiba_relative_intensity")) / dailyRides[day];
            else
                dailyRI[day] = metrics.getForSymbol("skiba_relative_intensity");

            // increment weekly values
            weeklySeconds += metrics.getForSymbol("time_riding");
            weeklyDistance += metrics.getForSymbol("total_distance") * (useMetricUnits ? 1 : MILES_PER_KM);
            weeklyCS += metrics.getForSymbol("daniels_points");
            weeklyW += metrics.getForSymbol("total_work");
            weeklyBS += metrics.getForSymbol("skiba_bike_score");

            // average RI for the week
            if (weeklyRides > 1)
                weeklyRI = ((weeklyRI * (weeklyRides-1))
                                + metrics.getForSymbol("skiba_relative_intensity")) / weeklyRides;
            else
                weeklyRI = metrics.getForSymbol("skiba_relative_intensity");

            // compute time in zones
            // Power
            if (num_zones < item->numZones()) {
                num_zones = item->numZones();
                time_in_zone.resize(num_zones);
            }
            zone_range = item->zoneRange();

            for (int j=0; j<item->numZones(); j++) {
                QString symbol = QString("time_in_zone_L%1").arg(j+1);
                time_in_zone[j] += metrics.getForSymbol(symbol);
	        }
            // HR
            if (num_hrzones < item->numHrZones()) {
                num_hrzones = item->numHrZones();
                time_in_hrzone.resize(num_hrzones);
            }
            hrzone_range = item->hrZoneRange();

            for (int j=0; j<item->numHrZones(); j++) {
                QString symbol = QString("time_in_zone_H%1").arg(j+1);
                time_in_hrzone[j] += metrics.getForSymbol(symbol);
	        }
        }
    }

    // break duration into hours, mins and seconds
    int seconds = round(weeklySeconds);
    int minutes = seconds / 60;
    seconds %= 60;
    int hours = minutes / 60;
    minutes %= 60;

    QString summary;
    summary = tr(
	   "<center>"
	   "<h3>Week of %1 through %2</h3>"
	   "<h3>Summary</h3>"
	   "<p>"
	   "<table align=\"center\" width=\"60%\" border=0>"
	   "<tr><td>Total time riding:</td>"
	   "    <td align=\"right\">%3:%4:%5</td></tr>"
	   "<tr><td>Total distance (%6):</td>"
	   "    <td align=\"right\">%7</td></tr>"
	   "<tr><td>Total work (kJ):</td>"
	   "    <td align=\"right\">%8</td></tr>"
	   "<tr><td>Daily Average work (kJ):</td>"
	   "    <td align=\"right\">%9</td></tr>")
	.arg(wstart.toString(tr("MM/dd/yyyy")))
	.arg(wstart.addDays(6).toString(tr("MM/dd/yyyy")))
	.arg(hours)
	.arg(minutes, 2, 10, QLatin1Char('0'))
	.arg(seconds, 2, 10, QLatin1Char('0'))
	.arg(useMetricUnits ? "km" : "miles")
	.arg(weeklyDistance, 0, 'f', 1)
	.arg((unsigned) round(weeklyW))
	.arg((unsigned) round(weeklyW/ 7));

    bool useBikeScore = (weeklyBS > 0);

    if (num_zones != -1) {
	if (useBikeScore)
	    summary += tr("<tr><td>Total BikeScore:</td>"
		   "    <td align=\"right\">%1</td></tr>"
		   "<tr><td>Total Daniels Points:</td>"
		   "    <td align=\"right\">%2</td></tr>"
		   "<tr><td>Net Relative Intensity:</td>"
		   "    <td align=\"right\">%3</td></tr>")
		.arg((unsigned) round(weeklyBS))
                .arg(weeklyCS, 0, 'f', 1)
		.arg(weeklyRI, 0, 'f', 3);

        if (zone_range >= 0) {
            summary += tr( "</table>" "<h3>Power Zones</h3>");
            summary += mainWindow->zones()->summarize(zone_range, time_in_zone);
        } else {
            summary += "No zones configured - zone summary not available.";
        }
        if (hrzone_range >= 0) {
            summary += tr( "</table>" "<h3>Heart Rate Zones</h3>");
            summary += mainWindow->hrZones()->summarize(hrzone_range, time_in_hrzone);
        }
    }

    summary += "</center>";

    // set the axis labels of the weekly plots
    QwtText textLabel = QwtText(useMetricUnits ? "km" : "miles");
    QFont weeklyPlotAxisTitleFont = textLabel.font();
    weeklyPlotAxisTitleFont.setPointSize(10);
    weeklyPlotAxisTitleFont.setBold(true);
    textLabel.setFont(weeklyPlotAxisTitleFont);
    weeklyPlot->setAxisTitle(QwtAxis::YLeft, textLabel);
    textLabel.setText("Minutes");
    weeklyPlot->setAxisTitle(QwtAxis::YRight, textLabel);
    textLabel.setText(useBikeScore ? "BikeScore" : "kJoules");
    weeklyBSPlot->setAxisTitle(QwtAxis::YLeft, textLabel);
    textLabel.setText(useBikeScore ? "Intensity" : "xPower");
    weeklyBSPlot->setAxisTitle(QwtAxis::YRight, textLabel);

    // for the daily distance/duration and bikescore plots:
    // first point: establish zero position
    // points 2N, 2N+1: Nth day (N from 1 to 7), up then down
    // 16th point: move to draw baseline off right of plot

    double xdist[16];
    double xdur[16];
    double xbsorw[16];
    double xriorxp[16];

    double ydist[16];   // daily distance
    double ydur[16];    // daily total time
    double ybsorw[16];     // daily minutes
    double yriorxp[16];     // daily relative intensity

    // data for a "baseline" curve to draw a baseline
    double xbaseline[] = {0, 8};
    double ybaseline[] = {0, 0};
    weeklyBaselineCurve->setData(xbaseline, ybaseline, 2);
    weeklyBSBaselineCurve->setData(xbaseline, ybaseline, 2);

    const double bar_width = 0.3;

    int i = 0;
    xdist[i] =
	xdur[i] =
	xbsorw[i] =
	xriorxp[i] =
	0;

    ydist[i] =
	ydur[i] =
	ybsorw[i] =
	yriorxp[i] =
	0;

    for(int day = 0; day < 7; day++){
	double x;

	i++;
        xdist[i]         = x = day + 1 - bar_width;
        xdist[i + 1]     = x += bar_width;
        xdur[i]          = x;
        xdur[i + 1]      = x += bar_width;

        xbsorw[i]        = x = day + 1 - bar_width;
        xbsorw[i + 1]    = x += bar_width;
        xriorxp[i]       = x;
	xriorxp[i + 1]   = x += bar_width;

	ydist[i]         = dailyDistance[day];
	ydur[i]          = dailySeconds[day] / 60;
	ybsorw[i]        = useBikeScore ? dailyBS[day] : dailyW[day] / 1000;
	yriorxp[i]       = useBikeScore ? dailyRI[day] : dailyXP[day];

	i++;
	ydist[i] = 0;
	ydur[i]  = 0;
	ybsorw[i]   = 0;
	yriorxp[i]   = 0;
    }

    // sweep a baseline off the right of the plot
    i++;
    xdist[i] =
	xdur[i] =
	xbsorw[i] =
	xriorxp[i] =
	8;

    ydist[i] =
	ydur[i] =
	ybsorw[i] =
	yriorxp[i] =
	0;

    // Distance/Duration plot:
    weeklyDistCurve->setData(xdist, ydist, 16);
    weeklyPlot->setAxisScale(QwtAxis::YLeft, 0, weeklyDistCurve->maxYValue()*1.1, 0);
    weeklyPlot->setAxisScale(QwtAxis::XBottom, 0.5, 7.5, 0);
    weeklyPlot->setAxisTitle(QwtAxis::YLeft, useMetricUnits ? "Kilometers" : "Miles");

    weeklyDurationCurve->setData(xdur, ydur, 16);
    weeklyPlot->setAxisScale(QwtAxis::YRight, 0, weeklyDurationCurve->maxYValue()*1.1, 0);
    weeklyPlot->replot();

    // BikeScore/Relative Intensity plot
    weeklyBSCurve->setData(xbsorw, ybsorw, 16);
    weeklyBSPlot->setAxisScale(QwtAxis::YLeft, 0, weeklyBSCurve->maxYValue()*1.1, 0);
    weeklyBSPlot->setAxisScale(QwtAxis::XBottom, 0.5, 7.5, 0);

    // set axis minimum for relative intensity
    double RImin = -1;
    for(int i = 1; i < 16; i += 2)
        if (yriorxp[i] > 0 && ((RImin < 0) || (yriorxp[i] < RImin)))
	    RImin = yriorxp[i];
    if (RImin < 0)
	RImin = 0;
    RImin *= 0.8;
    for (int i = 0; i < 16; i ++)
	if (yriorxp[i] < RImin)
	    yriorxp[i] = RImin;
    weeklyRICurve->setBaseline(RImin);
    weeklyRICurve->setData(xriorxp, yriorxp, 16);
    weeklyBSPlot->setAxisScale(QwtAxis::YRight, RImin, weeklyRICurve->maxYValue()*1.1, 0);

    weeklyBSPlot->replot();

    weeklySummary->setHtml(summary);
}
