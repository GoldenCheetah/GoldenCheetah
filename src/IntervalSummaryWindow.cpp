/*
 * Copyright (c) 2011 Eric Brandt (eric.l.brandt@gmail.com)
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

#include "Context.h"
#include "Athlete.h"
#include "RideFile.h"
#include "RideItem.h"
#include "RideMetric.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "IntervalSummaryWindow.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Colors.h"
#include <QStyle>
#include <QStyleFactory>
#include <QScrollBar>

IntervalSummaryWindow::IntervalSummaryWindow(Context *context) : context(context)
{
    setWindowTitle(tr("Interval Summary"));
    setReadOnly(true);
    setFrameStyle(QFrame::NoFrame);
#ifdef Q_OS_WIN
    QStyle *cde = QStyleFactory::create(OS_STYLE);
    verticalScrollBar()->setStyle(cde);
#endif

#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    connect(context, SIGNAL(intervalsChanged()), this, SLOT(intervalSelected()));
    connect(context, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    connect(context, SIGNAL(intervalHover(RideFileInterval)), this, SLOT(intervalHover(RideFileInterval)));
    connect(context, SIGNAL(configChanged()), this, SLOT(intervalSelected()));

    setHtml(GCColor::css() + "<body></body>");
}

IntervalSummaryWindow::~IntervalSummaryWindow() {
}

void IntervalSummaryWindow::intervalSelected()
{
    // if no ride available don't bother - just reset for color changes
    RideItem *rideItem = const_cast<RideItem*>(context->currentRideItem());

    if (context->athlete->intervalTreeWidget()->selectedItems().count() == 0 || 
        rideItem == NULL || rideItem->ride() == NULL) {
        // no ride just update the colors
	    QString html = GCColor::css();
        html += "<body></body>";
	    setHtml(html);
	    return;
    }

	QString html = GCColor::css();
    html += "<body>";
    if (context->athlete->allIntervalItems() != NULL) {
        for (int i=0; i<context->athlete->allIntervalItems()->childCount(); i++) {
            IntervalItem *current = dynamic_cast<IntervalItem*>(context->athlete->allIntervalItems()->child(i));
            if (current != NULL) {
                if (current->isSelected()) {
                    calcInterval(current, html);
                }
            }
        }
    }
    if (html == GCColor::css()+"<body>") {
    	html += "<i>" + tr("select an interval for summary info") + "</i>";
    }

    html += "</body>";
	setHtml(html);
	return;
}

void
IntervalSummaryWindow::intervalHover(RideFileInterval x)
{
    // if we're not visible don't bother
    if (!isVisible()) return;

    // we already have summaries!
    if (context->athlete->intervalWidget->selectedItems().count()) return;

    QString html = GCColor::css();
    html += "<body>";

    if (x == RideFileInterval()) {
    	html += "<i>" + tr("select an interval for summary info") + "</i>";
    } else {
        calcInterval(x, html);
    }
    html += "</body>";
    setHtml(html);
    return;
}

void IntervalSummaryWindow::calcInterval(IntervalItem* interval, QString& html)
{
	const RideFile* ride = context->ride ? context->ride->ride() : NULL;

    bool metricUnits = context->athlete->useMetricUnits;

    RideFile f(const_cast<RideFile*>(ride));
    int start = ride->timeIndex(interval->start);
    int end = ride->timeIndex(interval->stop);
    for (int i = start; i <= end; ++i) {
        const RideFilePoint *p = ride->dataPoints()[i];
        f.appendPoint(p->secs, p->cad, p->hr, p->km, p->kph, p->nm,
                      p->watts, p->alt, p->lon, p->lat, p->headwind, p->slope, p->temp, p->lrbalance, 
                      p->lte, p->rte, p->lps, p->rps, p->smo2, p->thb, 
                      p->rvert, p->rcad, p->rcontact, 0);

        // derived data
        RideFilePoint *l = f.dataPoints().last();
        l->np = p->np;
        l->xp = p->xp;
        l->apower = p->apower;
    }
    if (f.dataPoints().size() == 0) {
        // Interval empty, do not compute any metrics
        html += "<i>" + tr("empty interval") + "</tr>";
    }

    QString s;
    if (appsettings->contains(GC_SETTINGS_INTERVAL_METRICS))
        s = appsettings->value(this, GC_SETTINGS_INTERVAL_METRICS).toString();
    else
        s = GC_SETTINGS_INTERVAL_METRICS_DEFAULT;
    QStringList intervalMetrics = s.split(",");

    QHash<QString,RideMetricPtr> metrics =
        RideMetric::computeMetrics(context, &f, context->athlete->zones(), context->athlete->hrZones(), intervalMetrics);

    html += "<b>" + interval->text(0) + "</b>";
    html += "<table align=\"center\" width=\"90%\" ";
    html += "cellspacing=0 border=0>";

    foreach (QString symbol, intervalMetrics) {
        RideMetricPtr m = metrics.value(symbol);
        if (!m) continue;

        html += "<tr>";
        // left column (names)
        html += "<td align=\"right\" valign=\"bottom\">" + m->name() + "</td>";

        // right column (values)
        QString s("<td align=\"center\">%1</td>");
        if (m->units(metricUnits) == "seconds" ||
            m->units(metricUnits) == tr("seconds"))
            html += s.arg(time_to_string(m->value(metricUnits)));
        else if (m->internalName() == "Pace") {
            bool metricPace = appsettings->value(this, GC_PACE, true).toBool();
            html += s.arg(QTime(0,0,0,0).addSecs(m->value(metricPace)*60).toString("mm:ss"));
        } else
            html += s.arg(m->value(metricUnits), 0, 'f', m->precision());

        html += "<td align=\"left\" valign=\"bottom\">";
        if (m->units(metricUnits) == "seconds" ||
            m->units(metricUnits) == tr("seconds"))
            ; // don't do anything
        else if (m->units(metricUnits).size() > 0)
            html += m->units(metricUnits);
        html += "</td>";

        html += "</tr>";

    }
    html += "</table>";
}

void IntervalSummaryWindow::calcInterval(RideFileInterval interval, QString& html)
{
	const RideFile* ride = context->ride ? context->ride->ride() : NULL;

    bool metricUnits = context->athlete->useMetricUnits;

    RideFile f(const_cast<RideFile*>(ride));
    int start = ride->timeIndex(interval.start);
    int end = ride->timeIndex(interval.stop);
    for (int i = start; i <= end; ++i) {
        const RideFilePoint *p = ride->dataPoints()[i];
        f.appendPoint(p->secs, p->cad, p->hr, p->km, p->kph, p->nm,
                      p->watts, p->alt, p->lon, p->lat, p->headwind, p->slope, p->temp, p->lrbalance, 
                      p->lte, p->rte, p->lps, p->rps, p->smo2, p->thb, 
                      p->rvert, p->rcad, p->rcontact, 0);

        // derived data
        RideFilePoint *l = f.dataPoints().last();
        l->np = p->np;
        l->xp = p->xp;
        l->apower = p->apower;
    }
    if (f.dataPoints().size() == 0) {
        // Interval empty, do not compute any metrics
        html += "<i>" + tr("empty interval") + "</tr>";
    }

    QString s;
    if (appsettings->contains(GC_SETTINGS_INTERVAL_METRICS))
        s = appsettings->value(this, GC_SETTINGS_INTERVAL_METRICS).toString();
    else
        s = GC_SETTINGS_INTERVAL_METRICS_DEFAULT;
    QStringList intervalMetrics = s.split(",");

    QHash<QString,RideMetricPtr> metrics =
        RideMetric::computeMetrics(context, &f, context->athlete->zones(), context->athlete->hrZones(), intervalMetrics);

    html += "<b>" + interval.name + "</b>";
    html += "<table align=\"center\" width=\"90%\" ";
    html += "cellspacing=0 border=0>";

    foreach (QString symbol, intervalMetrics) {
        RideMetricPtr m = metrics.value(symbol);
        if (!m) continue;

        html += "<tr>";
        // left column (names)
        html += "<td align=\"right\" valign=\"bottom\">" + m->name() + "</td>";

        // right column (values)
        QString s("<td align=\"center\">%1</td>");
        if (m->units(metricUnits) == "seconds" ||
            m->units(metricUnits) == tr("seconds"))
            html += s.arg(time_to_string(m->value(metricUnits)));
        else if (m->internalName() == "Pace") {
            bool metricPace = appsettings->value(this, GC_PACE, true).toBool();
            html += s.arg(QTime(0,0,0,0).addSecs(m->value(metricPace)*60).toString("mm:ss"));
        } else
            html += s.arg(m->value(metricUnits), 0, 'f', m->precision());

        html += "<td align=\"left\" valign=\"bottom\">";
        if (m->units(metricUnits) == "seconds" ||
            m->units(metricUnits) == tr("seconds"))
            ; // don't do anything
        else if (m->units(metricUnits).size() > 0)
            html += m->units(metricUnits);
        html += "</td>";

        html += "</tr>";

    }
    html += "</table>";
}
