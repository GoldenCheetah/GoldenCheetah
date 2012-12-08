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

#include "MainWindow.h"
#include "IntervalItem.h"
#include "IntervalSummaryWindow.h"
#include "Settings.h"
#include "TimeUtils.h"

IntervalSummaryWindow::IntervalSummaryWindow(MainWindow *mainWindow) : mainWindow(mainWindow)
{
	setWindowTitle(tr("Interval Summary"));
	setReadOnly(true);
    setFrameStyle(QFrame::NoFrame);

#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    connect(mainWindow, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
}

IntervalSummaryWindow::~IntervalSummaryWindow() {
}

void IntervalSummaryWindow::intervalSelected()
{
    // if no ride available don't bother
    if (mainWindow->currentRideItem() == NULL || mainWindow->currentRide() == NULL) return;

	QString html;
    if (mainWindow->allIntervalItems() != NULL) {
        for (int i=0; i<mainWindow->allIntervalItems()->childCount(); i++) {
            IntervalItem *current = dynamic_cast<IntervalItem*>(mainWindow->allIntervalItems()->child(i));
            if (current != NULL) {
                if (current->isSelected()) {
                    calcInterval(current, html);
                }
            }
        }
    }
    if (html.length() == 0)
    	html = "<i>" + tr("select an interval for summary info") + "</i>";

	setHtml(html);
	return;
}

void IntervalSummaryWindow::calcInterval(IntervalItem* interval, QString& html)
{
	const RideFile* ride = mainWindow->currentRide();

    bool metricUnits = mainWindow->useMetricUnits;

    RideFile f(ride->startTime(), ride->recIntSecs());
    int start = ride->timeIndex(interval->start);
    int end = ride->timeIndex(interval->stop);
    for (int i = start; i < end; ++i) {
        const RideFilePoint *p = ride->dataPoints()[i];
        f.appendPoint(p->secs, p->cad, p->hr, p->km, p->kph, p->nm,
                      p->watts, p->alt, p->lon, p->lat, p->headwind, p->slope, p->temp, p->lrbalance, 0);
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
        RideMetric::computeMetrics(mainWindow, &f, mainWindow->zones(), mainWindow->hrZones(), intervalMetrics);

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
        else
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


