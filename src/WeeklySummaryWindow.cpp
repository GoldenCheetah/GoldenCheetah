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
#include "Zones.h"
#include "HrZones.h"
#include <QtGui>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <assert.h>

WeeklySummaryWindow::WeeklySummaryWindow(bool useMetricUnits,
                                         MainWindow *mainWindow) :
    QWidget(mainWindow), mainWindow(mainWindow),
    useMetricUnits(useMetricUnits)
{
    QGridLayout *glayout = new QGridLayout;

    // set up the weekly distance / duration plot:
    weeklyPlot = new QwtPlot();
    weeklyPlot->enableAxis(QwtPlot::yRight, true);
    weeklyPlot->setAxisMaxMinor(QwtPlot::xBottom,0);
    weeklyPlot->setAxisScaleDraw(QwtPlot::xBottom, new DaysScaleDraw());
    QFont weeklyPlotAxisFont = weeklyPlot->axisFont(QwtPlot::yLeft);
    weeklyPlotAxisFont.setPointSize(weeklyPlotAxisFont.pointSize() * 0.9f);
    weeklyPlot->setAxisFont(QwtPlot::xBottom, weeklyPlotAxisFont);
    weeklyPlot->setAxisFont(QwtPlot::yLeft, weeklyPlotAxisFont);
    weeklyPlot->setAxisFont(QwtPlot::yRight, weeklyPlotAxisFont);

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
    weeklyDurationCurve->setYAxis(QwtPlot::yRight);
    weeklyDurationCurve->attach(weeklyPlot);

    // set up the weekly bike score plot:
    weeklyBSPlot = new QwtPlot();
    weeklyBSPlot->enableAxis(QwtPlot::yRight, true);
    weeklyBSPlot->setAxisMaxMinor(QwtPlot::xBottom,0);
    weeklyBSPlot->setAxisScaleDraw(QwtPlot::xBottom, new DaysScaleDraw());
    QwtText textLabel = QwtText();
    weeklyBSPlot->setAxisFont(QwtPlot::xBottom, weeklyPlotAxisFont);
    weeklyBSPlot->setAxisFont(QwtPlot::yLeft, weeklyPlotAxisFont);
    weeklyBSPlot->setAxisFont(QwtPlot::yRight, weeklyPlotAxisFont);

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
    weeklyRICurve->setYAxis(QwtPlot::yRight);
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

    connect(mainWindow, SIGNAL(rideSelected()), this, SLOT(refresh()));
    connect(mainWindow, SIGNAL(zonesChanged()), this, SLOT(refresh()));
}

void
WeeklySummaryWindow::refresh()
{
    if (mainWindow->activeTab() != this)
        return;
    const RideItem *ride = mainWindow->rideItem();
    if (!ride)
        return;
    const QTreeWidgetItem *allRides = mainWindow->allRideItems();
    const Zones *zones = mainWindow->zones();
    const HrZones *hrZones = mainWindow->hrZones();
    QDate wstart = ride->dateTime.date();
    wstart = wstart.addDays(Qt::Monday - wstart.dayOfWeek());
    assert(wstart.dayOfWeek() == Qt::Monday);
    QDate wend = wstart.addDays(7);
    const RideMetricFactory &factory = RideMetricFactory::instance();
    QSharedPointer<RideMetric> weeklySeconds(factory.newMetric("time_riding"));
    assert(weeklySeconds);
    QSharedPointer<RideMetric> weeklyDistance(factory.newMetric("total_distance"));
    assert(weeklyDistance);
    QSharedPointer<RideMetric> weeklyWork(factory.newMetric("total_work"));
    assert(weeklyWork);
    QSharedPointer<RideMetric> weeklyBS(factory.newMetric("skiba_bike_score"));
    assert(weeklyBS);
    QSharedPointer<RideMetric> weeklyRelIntensity(factory.newMetric("skiba_relative_intensity"));
    assert(weeklyRelIntensity);
    QSharedPointer<RideMetric> weeklyCS(factory.newMetric("daniels_points"));
    assert(weeklyCS);

    QSharedPointer<RideMetric> dailySeconds[7];
    QSharedPointer<RideMetric> dailyDistance[7];
    QSharedPointer<RideMetric> dailyBS[7];
    QSharedPointer<RideMetric> dailyRI[7];
    QSharedPointer<RideMetric> dailyW[7];
    QSharedPointer<RideMetric> dailyXP[7];

    for (int i = 0; i < 7; i++) {
	dailySeconds[i] = QSharedPointer<RideMetric>(factory.newMetric("time_riding"));
	assert(dailySeconds[i]);
	dailyDistance[i] = QSharedPointer<RideMetric>(factory.newMetric("total_distance"));
	assert(dailyDistance[i]);
	dailyBS[i] = QSharedPointer<RideMetric>(factory.newMetric("skiba_bike_score"));
	assert(dailyBS[i]);
	dailyRI[i] = QSharedPointer<RideMetric>(factory.newMetric("skiba_relative_intensity"));
	assert(dailyRI[i]);
	dailyW[i] = QSharedPointer<RideMetric>(factory.newMetric("total_work"));
	assert(dailyW[i]);
	dailyXP[i] = QSharedPointer<RideMetric>(factory.newMetric("skiba_xpower"));
	assert(dailyXP[i]);
    }

    int zone_range = -1;
    int hrzone_range = -1;
    QVector<double> time_in_zone;
    QVector<double> time_in_hrzone; // max 10 zones supported
    int num_zones = -1;
    int num_hrzones = -1;
    bool zones_ok = true;

    for (int i = 0; i < allRides->childCount(); ++i) {
	RideItem *item = (RideItem*) allRides->child(i);
	int day;
        if (
	    (item->type() == RIDE_TYPE) &&
	    ((day = wstart.daysTo(item->dateTime.date())) >= 0) &&
	    (day < 7)
	    ) {

            item->computeMetrics(); // generates item->ride
            if (!item->ride())
                continue;

	    RideMetricPtr m;
	    if ((m = item->metrics.value(weeklySeconds->symbol()))) {
		weeklySeconds->aggregateWith(*m);
		dailySeconds[day]->aggregateWith(*m);
	    }

	    if ((m = item->metrics.value(weeklyDistance->symbol()))) {
		weeklyDistance->aggregateWith(*m);
		dailyDistance[day]->aggregateWith(*m);
	    }

	    if ((m = item->metrics.value(weeklyWork->symbol()))) {
		weeklyWork->aggregateWith(*m);
		dailyW[day]->aggregateWith(*m);
	    }

            if ((m = item->metrics.value(weeklyCS->symbol())))
                weeklyCS->aggregateWith(*m);

        if ((m = item->metrics.value(weeklyBS->symbol()))) {
		weeklyBS->aggregateWith(*m);
		dailyBS[day]->aggregateWith(*m);
	    }

	    if ((m = item->metrics.value(weeklyRelIntensity->symbol()))) {
		weeklyRelIntensity->aggregateWith(*m);
		dailyRI[day]->aggregateWith(*m);
	    }

	    if ((m = item->metrics.value("skiba_xpower")))
		dailyXP[day]->aggregateWith(*m);

	    // compute time in zones
	    if (zones) {
		if (zone_range == -1) {
		    zone_range = item->zoneRange();
		    num_zones = item->numZones();
                    time_in_zone.clear();
                    time_in_zone.resize(num_zones);
		}
		else if (item->zoneRange() != zone_range) {
		    zones_ok = false;
		}
		if (zone_range != -1) {
		    for (int j = 0; j < num_zones; ++j)
			time_in_zone[j] += item->timeInZone(j);
		}
	    }
	    if (hrZones) {
                // HR
                if (num_hrzones < item->numHrZones()) {
                    num_hrzones = item->numHrZones();
                    time_in_hrzone.resize(num_hrzones);
                }
                hrzone_range = item->hrZoneRange();

                for (int j=0; j<item->numHrZones(); j++)
                    time_in_hrzone[j] += item->timeInHrZone(j);
            }
            }
    }

    int seconds = ((int) round(weeklySeconds->value(true)));
    int minutes = seconds / 60;
    seconds %= 60;
    int hours = minutes / 60;
    minutes %= 60;

    QString summary;
    summary =
	tr(
	   "<center>"
	   "<h2>Week of %1 through %2</h2>"
	   "<h2>Summary</h2>"
	   "<p>"
	   "<table align=\"center\" width=\"60%\" border=0>"
	   "<tr><td>Total time riding:</td>"
	   "    <td align=\"right\">%3:%4:%5</td></tr>"
	   "<tr><td>Total distance (%6):</td>"
	   "    <td align=\"right\">%7</td></tr>"
	   "<tr><td>Total work (kJ):</td>"
	   "    <td align=\"right\">%8</td></tr>"
	   "<tr><td>Daily Average work (kJ):</td>"
	   "    <td align=\"right\">%9</td></tr>"
	   )
	.arg(wstart.toString(tr("MM/dd/yyyy")))
	.arg(wstart.addDays(6).toString(tr("MM/dd/yyyy")))
	.arg(hours)
	.arg(minutes, 2, 10, QLatin1Char('0'))
	.arg(seconds, 2, 10, QLatin1Char('0'))
	.arg(useMetricUnits ? "km" : "miles")
	.arg(weeklyDistance->value(useMetricUnits), 0, 'f', 1)
	.arg((unsigned) round(weeklyWork->value(useMetricUnits)))
	.arg((unsigned) round(weeklyWork->value(useMetricUnits) / 7));

    double weeklyBSValue = weeklyBS->value(useMetricUnits);
    bool useBikeScore = (zone_range != -1) && (weeklyBSValue > 0);

    if (zone_range != -1) {
	if (useBikeScore)
	    summary +=
		tr(
		   "<tr><td>Total BikeScore:</td>"
		   "    <td align=\"right\">%1</td></tr>"
		   "<tr><td>Total Daniels Points:</td>"
		   "    <td align=\"right\">%2</td></tr>"
		   "<tr><td>Net Relative Intensity:</td>"
		   "    <td align=\"right\">%3</td></tr>"
		   )
		.arg((unsigned) round(weeklyBSValue))
                .arg(weeklyCS->value(useMetricUnits), 0, 'f', 1)
		.arg(weeklyRelIntensity->value(useMetricUnits), 0, 'f', 3);

        summary +=
	    tr(
	       "</table>"
	       "<h2>Power Zones</h2>"
	       );
        if (!zones_ok)
            summary += "Error: Week spans more than one zone range.";
        else {
            summary += zones->summarize(zone_range, time_in_zone);
        }
    }
    if (hrzone_range >= 0) {
        summary += tr( "</table>" "<h2>Heart Rate Zones</h2>");
        summary += hrZones->summarize(hrzone_range, time_in_hrzone);
    }

    summary += "</center>";

    // set the axis labels of the weekly plots
    QwtText textLabel = QwtText(useMetricUnits ? tr("km") : tr("miles"));
    QFont weeklyPlotAxisTitleFont = textLabel.font();
    weeklyPlotAxisTitleFont.setPointSize(10);
    weeklyPlotAxisTitleFont.setBold(true);
    textLabel.setFont(weeklyPlotAxisTitleFont);
    weeklyPlot->setAxisTitle(QwtPlot::yLeft, textLabel);
    textLabel.setText(tr("Minutes"));
    weeklyPlot->setAxisTitle(QwtPlot::yRight, textLabel);
    textLabel.setText(useBikeScore ? tr("BikeScore") : tr("kJoules"));
    weeklyBSPlot->setAxisTitle(QwtPlot::yLeft, textLabel);
    textLabel.setText(useBikeScore ? tr("Intensity") : tr("xPower"));
    weeklyBSPlot->setAxisTitle(QwtPlot::yRight, textLabel);

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

	ydist[i]         = dailyDistance[day]->value(useMetricUnits);
	ydur[i]          = dailySeconds[day]->value(useMetricUnits) / 60;
	ybsorw[i]        = useBikeScore ? dailyBS[day]->value(useMetricUnits) : dailyW[day]->value(useMetricUnits) / 1000;
	yriorxp[i]       = useBikeScore ? dailyRI[day]->value(useMetricUnits) : dailyXP[day]->value(useMetricUnits);

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
    weeklyPlot->setAxisScale(QwtPlot::yLeft, 0, weeklyDistCurve->maxYValue()*1.1, 0);
    weeklyPlot->setAxisScale(QwtPlot::xBottom, 0.5, 7.5, 0);
    weeklyPlot->setAxisTitle(QwtPlot::yLeft, useMetricUnits ? tr("Kilometers") : tr("Miles"));

    weeklyDurationCurve->setData(xdur, ydur, 16);
    weeklyPlot->setAxisScale(QwtPlot::yRight, 0, weeklyDurationCurve->maxYValue()*1.1, 0);
    weeklyPlot->replot();

    // BikeScore/Relative Intensity plot
    weeklyBSCurve->setData(xbsorw, ybsorw, 16);
    weeklyBSPlot->setAxisScale(QwtPlot::yLeft, 0, weeklyBSCurve->maxYValue()*1.1, 0);
    weeklyBSPlot->setAxisScale(QwtPlot::xBottom, 0.5, 7.5, 0);

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
    weeklyBSPlot->setAxisScale(QwtPlot::yRight, RImin, weeklyRICurve->maxYValue()*1.1, 0);

    weeklyBSPlot->replot();

    weeklySummary->setHtml(summary);
}

