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
    //XXXsetEnabled(false); // stop the fucking thing grabbing keyboard focus FFS.
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
    connect(context, SIGNAL(intervalHover(IntervalItem*)), this, SLOT(intervalHover(IntervalItem*)));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(intervalSelected()));

    setHtml(GCColor::instance()->css() + "<body></body>");
}

IntervalSummaryWindow::~IntervalSummaryWindow() {
}

void IntervalSummaryWindow::intervalSelected()
{
    // if no ride available don't bother - just reset for color changes
    RideItem *rideItem = const_cast<RideItem*>(context->currentRideItem());

    if (rideItem == NULL || rideItem->intervalsSelected().count() == 0 || rideItem->ride() == NULL) {
        // no ride just update the colors
	    QString html = GCColor::instance()->css();
        html += "<body></body>";
	    setHtml(html);
	    return;
    }

    // summary is html
	QString html = GCColor::instance()->css();
    html += "<body>";

    // summarise all the intervals selected - this is painful!
    // now also summarises for entire ride EXCLUDING the intervals selected
    QString notincluding;
    if (rideItem->intervalsSelected().count()>1) html += summary(rideItem->intervalsSelected(), notincluding);

    // summary for each of the currently selected intervals
    foreach(IntervalItem *interval, rideItem->intervalsSelected()) html += summary(interval);

    // now add the excluding text
    html += notincluding;

    if (html == GCColor::instance()->css()+"<body>") html += "<i>" + tr("select an interval for summary info") + "</i>";

    html += "</body>";
	setHtml(html);
	return;
}

void
IntervalSummaryWindow::intervalHover(IntervalItem* x)
{
    // if we're not visible don't bother
    if (!isVisible()) return;

    // we already have summaries!
    if (x && x->rideItem()->intervalsSelected().count()) return;

    // its to clear, but if the current ride has selected intervals then we will ignore it
    RideItem *rideItem = const_cast<RideItem*>(context->currentRideItem());
    if (!x && rideItem && rideItem->intervalsSelected().count()) return;

    QString html = GCColor::instance()->css();
    html += "<body>";

    if (x == NULL) {
    	html += "<i>" + tr("select an interval for summary info") + "</i>";
    } else {
        html += summary(x);
    }
    html += "</body>";
    setHtml(html);
    return;
}

static bool contains(const RideFile*ride, QList<IntervalItem*> intervals, int index)
{
    foreach(IntervalItem *item, intervals) {
        int start = ride->timeIndex(item->start);
        int end = ride->timeIndex(item->stop);

        if (index >= start && index <= end) return true;
    }
    return false;
}

QString IntervalSummaryWindow::summary(QList<IntervalItem*> intervals, QString &notincluding)
{
    // need a current rideitem
    if (!context->currentRideItem()) return "";

    // We need to create a special ridefile just for the selected intervals
    // to calculate the aggregated metrics because intervals can OVERLAP!
    // so we can't just aggregate the pre-computed metrics as this will lead
    // to overstated totals and skewed averages.
	const RideFile* ride = context->ride ? context->ride->ride() : NULL;
    RideFile f(const_cast<RideFile*>(ride));
    RideFile notf(const_cast<RideFile*>(ride));

    // for concatenating intervals
    RideFilePoint *last = NULL;
    double timeOff=0;
    double distOff=0;

    RideFilePoint *notlast = NULL;
    double nottimeOff=0;
    double notdistOff=0;

    for (int i = 0; i < ride->dataPoints().count(); ++i) {

        // append points for selected intervals
        const RideFilePoint *p = ride->dataPoints()[i];
        if (contains(ride, intervals, i)) {

            // drag back time/distance for data not included below
            if (notlast) {
                nottimeOff = p->secs - notlast->secs;
                notdistOff = p->km - notlast->km;
            } else {
                nottimeOff = p->secs;
                notdistOff = p->km;
            }

            f.appendPoint(p->secs-timeOff, p->cad, p->hr, p->km-distOff, p->kph, p->nm,
                        p->watts, p->alt, p->lon, p->lat, p->headwind, p->slope, p->temp, p->lrbalance, 
                        p->lte, p->rte, p->lps, p->rps,
                        p->lpco, p->rpco,
                        p->lppb, p->rppb, p->lppe, p->rppe,
                        p->lpppb, p->rpppb, p->lpppe, p->rpppe,
                        p->smo2, p->thb,
                        p->rvert, p->rcad, p->rcontact, p->tcore, 0);

            // derived data
            last = f.dataPoints().last();
            last->np = p->np;
            last->xp = p->xp;
            last->apower = p->apower;

        } else {

            // drag back time/distance for data not included above
            if (last) {
                timeOff = p->secs - last->secs;
                distOff = p->km - last->km;
            } else {
                timeOff = p->secs;
                distOff = p->km;
            }

            notf.appendPoint(p->secs-nottimeOff, p->cad, p->hr, p->km-notdistOff, p->kph, p->nm,
                        p->watts, p->alt, p->lon, p->lat, p->headwind, p->slope, p->temp, p->lrbalance, 
                        p->lte, p->rte, p->lps, p->rps,
                        p->lpco, p->rpco,
                        p->lppb, p->rppb, p->lppe, p->rppe,
                        p->lpppb, p->rpppb, p->lpppe, p->rpppe,
                        p->smo2, p->thb,
                        p->rvert, p->rcad, p->rcontact, p->tcore, 0);

            // derived data
            notlast = notf.dataPoints().last();
            notlast->np = p->np;
            notlast->xp = p->xp;
            notlast->apower = p->apower;
        }
    }

    QString s;
    if (appsettings->contains(GC_SETTINGS_FAVOURITE_METRICS))
        s = appsettings->value(this, GC_SETTINGS_FAVOURITE_METRICS).toString();
    else
        s = GC_SETTINGS_FAVOURITE_METRICS_DEFAULT;
    QStringList intervalMetrics = s.split(",");
    const RideMetricFactory &factory = RideMetricFactory::instance();

    // build fake rideitem and compute metrics
    RideItem *fake;
    fake = new RideItem(&f, context);
    fake->setFrom(*const_cast<RideItem*>(context->currentRideItem()), true); // this wipes ride_ so put back
    fake->ride_ = &f;
    fake->getWeight();
    fake->intervals_.clear(); // don't accidentally wipe these!!!!
    fake->samples = f.dataPoints().count() > 0;
    QHash<QString,RideMetricPtr> metrics = RideMetric::computeMetrics(fake, Specification(), intervalMetrics);

    // build fake for not in intervals
    RideItem *notfake;
    notfake = new RideItem(&notf, context);
    notfake->setFrom(*const_cast<RideItem*>(context->currentRideItem()), true); // this wipes ride_ so put back
    notfake->ride_ = &notf;
    notfake->getWeight();
    fake->intervals_.clear(); // don't accidentally wipe these!!!!
    notfake->samples = notf.dataPoints().count() > 0;
    QHash<QString,RideMetricPtr> notmetrics = RideMetric::computeMetrics(notfake, Specification(), intervalMetrics);

    // create temp interval item to use by interval summary
    IntervalItem temp(NULL, "", 0, 0, 0, 0, 0, Qt::black, false, RideFileInterval::USER);

    // pack the metrics away and clean up if needed
    temp.metrics().fill(0, factory.metricCount());

    // NOTE INCLUDED
    // snaffle away all the computed values into the array
    QHashIterator<QString, RideMetricPtr> i(metrics);
    while (i.hasNext()) {
        i.next();
        temp.metrics()[i.value()->index()] = i.value()->value();
    }

    // clean any bad values
    for(int j=0; j<factory.metricCount(); j++)
        if (std::isinf(temp.metrics()[j]) || std::isnan(temp.metrics()[j]))
            temp.metrics()[j] = 0.00f;

    // set name
    temp.name = QString(tr("%1 selected intervals")).arg(intervals.count());
    temp.rideItem_ = fake;

    QString returning = summary(&temp);

    if (notf.dataPoints().count()) {
        // EXCLUDING
        // snaffle away all the computed values into the array
        // pack the metrics away and clean up if needed
        temp.metrics().fill(0, factory.metricCount());

        QHashIterator<QString, RideMetricPtr> i(notmetrics);
        while (i.hasNext()) {
            i.next();
            temp.metrics()[i.value()->index()] = i.value()->value();
        }

        // clean any bad values
        for(int j=0; j<factory.metricCount(); j++)
            if (std::isinf(temp.metrics()[j]) || std::isnan(temp.metrics()[j]))
                temp.metrics()[j] = 0.00f;

        // set name
        temp.name = QString(tr("Excluding %1 selected")).arg(intervals.count());
        temp.rideItem_ = notfake;

        // use standard method from above
        notincluding = summary(&temp);
    }

    // remove references temporary / fakes get wiped
    temp.rideItem_ = NULL;

    // zap references to real, and delete temporary ride item
    fake->ride_ = NULL;
    delete fake;
    
    notfake->ride_ = NULL;
    delete notfake;

    return returning;
}

QString IntervalSummaryWindow::summary(IntervalItem *interval)
{
    QString html;

    bool useMetricUnits = GlobalContext::context()->useMetricUnits;

    QString s;
    if (appsettings->contains(GC_SETTINGS_FAVOURITE_METRICS))
        s = appsettings->value(this, GC_SETTINGS_FAVOURITE_METRICS).toString();
    else
        s = GC_SETTINGS_FAVOURITE_METRICS_DEFAULT;
    QStringList intervalMetrics = s.split(",");

    html += "<b>" + interval->name + "</b>";
    html += "<table align=\"center\" width=\"90%\" ";
    html += "cellspacing=0 border=0>";

    RideMetricFactory &factory = RideMetricFactory::instance();
    foreach (QString symbol, intervalMetrics) {
        const RideMetric *m = factory.rideMetric(symbol);
        if (!m) continue;

        // skip metrics that are not relevant for this ride
        if (!interval->rideItem() || m->isRelevantForRide(interval->rideItem()) == false) continue;

        html += "<tr>";
        // left column (names)
        html += "<td align=\"right\" valign=\"bottom\">" + m->name() + "</td>";

        // right column (values)
        QString s("<td align=\"center\">%1</td>");
        html += s.arg(interval->getStringForSymbol(symbol, useMetricUnits));
        html += "<td align=\"left\" valign=\"bottom\">";
        if (m->units(useMetricUnits) == "seconds" ||
            m->units(useMetricUnits) == tr("seconds"))
            ; // don't do anything
        else if (m->units(useMetricUnits).size() > 0)
            html += m->units(useMetricUnits);
        html += "</td>";

        html += "</tr>";

    }
    html += "</table>";

    return html;
}
