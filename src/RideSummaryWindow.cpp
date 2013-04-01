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

#include "RideSummaryWindow.h"
#include "MainWindow.h"
#include "RideFile.h"
#include "RideItem.h"
#include "RideMetric.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Units.h"
#include "Zones.h"
#include "MetricAggregator.h"
#include "DBAccess.h"
#include <QtGui>
#include <QtXml/QtXml>
#include <assert.h>
#include <math.h>

RideSummaryWindow::RideSummaryWindow(MainWindow *mainWindow, bool ridesummary) :
     GcChartWindow(mainWindow), mainWindow(mainWindow), ridesummary(ridesummary), useCustom(false), useToToday(false), filtered(false)
{
    setInstanceName("Ride Summary Window");
    setRideItem(NULL);

    // allow user to select date range if in summary mode
    dateSetting = new DateSettingsEdit(this);
    if (ridesummary) {

        setControls(NULL);
        dateSetting->hide(); // not needed, but holds property values

    } else {

        QWidget *c = new QWidget;
        c->setContentsMargins(0,0,0,0);
        QFormLayout *cl = new QFormLayout(c);
        cl->setContentsMargins(0,0,0,0);
        cl->setSpacing(0);
        setControls(c);

#ifdef GC_HAVE_LUCENE
        // filter / searchbox
        searchBox = new SearchFilterBox(this, mainWindow);
        connect(searchBox, SIGNAL(searchClear()), this, SLOT(clearFilter()));
        connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(setFilter(QStringList)));
        cl->addRow(new QLabel(tr("Filter")), searchBox);
        cl->addWidget(new QLabel("")); //spacing
#endif

        cl->addRow(new QLabel(tr("Date range")), dateSetting);
    }

    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->setSpacing(0);
    vlayout->setContentsMargins(10,10,10,10);
    rideSummary = new QWebView(this);
    rideSummary->setContentsMargins(0,0,0,0);
    rideSummary->page()->view()->setContentsMargins(0,0,0,0);
    rideSummary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    rideSummary->setAcceptDrops(false);

    QFont defaultFont; // mainwindow sets up the defaults.. we need to apply
    rideSummary->settings()->setFontSize(QWebSettings::DefaultFontSize, defaultFont.pointSize()+1);
    rideSummary->settings()->setFontFamily(QWebSettings::StandardFont, defaultFont.family());

    vlayout->addWidget(rideSummary);

    if (ridesummary) {

        connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideItemChanged()));
        connect(mainWindow, SIGNAL(zonesChanged()), this, SLOT(refresh()));
        connect(mainWindow, SIGNAL(intervalsChanged()), this, SLOT(refresh()));

    } else {

        connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
        connect(mainWindow, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh()));
        connect(mainWindow, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh()));

        // date settings
        connect(dateSetting, SIGNAL(useCustomRange(DateRange)), this, SLOT(useCustomRange(DateRange)));
        connect(dateSetting, SIGNAL(useThruToday()), this, SLOT(useThruToday()));
        connect(dateSetting, SIGNAL(useStandardRange()), this, SLOT(useStandardRange()));

    }
    setChartLayout(vlayout);
}

void
RideSummaryWindow::clearFilter()
{
    filters.clear();
    filtered = false;
    refresh();
}

void
RideSummaryWindow::setFilter(QStringList list)
{
    filters = list;
    filtered = true;
    refresh();
}

void
RideSummaryWindow::rideSelected()
{
    refresh();
}

void
RideSummaryWindow::rideItemChanged()
{
    // disconnect from previous
    static QPointer<RideItem> _connected = NULL;
    if (_connected) {
        disconnect(_connected, SIGNAL(rideMetadataChanged()), this, SLOT(metadataChanged()));
    }
    _connected=myRideItem;
    if (_connected) { // in case it was set to null!
        connect (_connected, SIGNAL(rideMetadataChanged()), this, SLOT(metadataChanged()));
        // and now refresh
        setIsBlank(false);
        refresh();
    } else {
        setIsBlank(true);
    }
}

void
RideSummaryWindow::metadataChanged()
{
    refresh();
}

void
RideSummaryWindow::refresh()
{
    // if we're summarising a ride but have no ride to summarise
    if (ridesummary && !myRideItem) {
	    rideSummary->page()->mainFrame()->setHtml("");
        return;
    }

    if (ridesummary) {
        RideItem *rideItem = myRideItem;
        setSubTitle(rideItem->dateTime.toString(tr("dddd MMMM d, yyyy, h:mm AP")));
    } else {

        if (myDateRange.name != "") setSubTitle(myDateRange.name);
        else {
        setSubTitle(myDateRange.from.toString("dddd MMMM d yyyy") +
                    " - " +
                    myDateRange.to.toString("dddd MMMM d yyyy"));
        }
    }
    rideSummary->page()->mainFrame()->setHtml(htmlSummary());
}


QString
RideSummaryWindow::htmlSummary() const
{
    QString summary("");

    RideItem *rideItem = myRideItem;
    RideFile *ride;

    if (!rideItem && !ridesummary) return ""; // nothing selected!
    else ride = rideItem->ride();

    if (!ride && !ridesummary) return ""; // didn't parse!

    bool useMetricUnits = mainWindow->useMetricUnits;

    // ride summary and there were ridefile read errors?
    if (ridesummary && !ride) {
        summary = tr("<p>Couldn't read file \"");
        summary += rideItem->fileName + "\":";
        QListIterator<QString> i(rideItem->errors());
        while (i.hasNext())
            summary += "<br>" + i.next();
        return summary;
    }

    // always centered
    summary = "<center>";

    // device summary for ride summary, otherwise how many activities?
    if (ridesummary) summary += ("<p><h3>" + tr("Device Type: ") + ride->deviceType() + "</h3><p>");

    // All the metrics we will display
    static const QStringList columnNames = QStringList() << tr("Totals") << tr("Averages") << tr("Maximums") << tr("Metrics*");
    static const QStringList totalColumn = QStringList()
        << "workout_time"
        << "time_riding"
        << "total_distance"
        << "total_work"
        << "elevation_gain";

    static const QStringList rtotalColumn = QStringList()
        << "workout_time"
        << "total_distance"
        << "total_work"
        << "elevation_gain";

    QStringList averageColumn = QStringList() // not const as modified below..
        << "average_speed"
        << "average_power"
        << "average_hr"
        << "average_cad";

    QStringList maximumColumn = QStringList() // not const as modified below..
        << "max_speed"
        << "max_power"
        << "max_heartrate"
        << "max_cadence";

    // show average and max temp if it is available (in ride summary mode)
    if (ridesummary && (ride->areDataPresent()->temp || ride->getTag("Temperature", "-") != "-")) {
        averageColumn << "average_temp";
        maximumColumn << "max_temp";
    }

    // users determine the metrics to display
    QString s = appsettings->value(this, GC_SETTINGS_SUMMARY_METRICS, GC_SETTINGS_SUMMARY_METRICS_DEFAULT).toString();
    if (s == "") s = GC_SETTINGS_SUMMARY_METRICS_DEFAULT;
    QStringList metricColumn = s.split(",");

    static const QStringList timeInZones = QStringList()
        << "time_in_zone_L1"
        << "time_in_zone_L2"
        << "time_in_zone_L3"
        << "time_in_zone_L4"
        << "time_in_zone_L5"
        << "time_in_zone_L6"
        << "time_in_zone_L7"
        << "time_in_zone_L8"
        << "time_in_zone_L9"
        << "time_in_zone_L10";

    static const QStringList timeInZonesHR = QStringList()
        << "time_in_zone_H1"
        << "time_in_zone_H2"
        << "time_in_zone_H3"
        << "time_in_zone_H4"
        << "time_in_zone_H5"
        << "time_in_zone_H6"
        << "time_in_zone_H7"
        << "time_in_zone_H8";

    // Use pre-computed and saved metric values if the ride has not
    // been edited. Otherwise we need to re-compute every time.
    // this is only for ride summary, when showing for a date range
    // we already have a summary metrics array
    SummaryMetrics metrics;
    RideMetricFactory &factory = RideMetricFactory::instance();

    if (ridesummary) {
        if (rideItem->isDirty()) {
            // make a list if the metrics we want computed
            // instead of calculating them all, just do the
            // ones we display
            QStringList worklist;
            worklist += totalColumn;
            worklist += averageColumn;
            worklist += maximumColumn;
            worklist += metricColumn;
            worklist += timeInZones;
            worklist += timeInZonesHR;

            // go calculate them then...
            QHash<QString, RideMetricPtr> computed = RideMetric::computeMetrics(mainWindow, ride, mainWindow->zones(), mainWindow->hrZones(), worklist);
            for(int i = 0; i < worklist.count(); ++i) {
                if (worklist[i] != "") {
                    RideMetricPtr m = computed.value(worklist[i]);
                    if (m) metrics.setForSymbol(worklist[i], m->value(true));
                    else metrics.setForSymbol(worklist[i], 0.00);
                }
            }
        } else {

            // just use the metricDB versions, nice 'n fast
            metrics = mainWindow->metricDB->getRideMetrics(rideItem->fileName);
        }
    }


    //
    // 3 top columns - total, average, maximums and metrics for entire ride
    //
    summary += "<table border=0 cellspacing=10><tr>";
    for (int i = 0; i < columnNames.count(); ++i) {
        summary += "<td align=\"center\" valign=\"top\" width=\"%1%\"><table>"
            "<tr><td align=\"center\" colspan=2><h3>%2</h3></td></tr>";
        summary = summary.arg(90 / columnNames.count());
        summary = summary.arg(columnNames[i]);

        QStringList metricsList;
        switch (i) {
            case 0: metricsList = totalColumn; break;
            case 1: metricsList = averageColumn; break;
            case 2: metricsList = maximumColumn; break;
            case 3: metricsList = metricColumn; break;
            default: assert(false);
        }
        for (int j = 0; j< metricsList.count(); ++j) {
            QString symbol = metricsList[j];

             if (symbol == "") continue;

             const RideMetric *m = factory.rideMetric(symbol);
             if (!m) break;
 
             // HTML table row
             QString s("<tr><td>%1%2:</td><td align=\"right\">%3</td></tr>");
 
             // Maximum Max and Average Average looks nasty, remove from name for display
             s = s.arg(m->name().replace(QRegExp(tr("^(Average|Max) ")), ""));
 
             // Add units (if needed)  and value (with right precision)
             if (m->units(useMetricUnits) == "seconds" || m->units(useMetricUnits) == tr("seconds")) {
                 s = s.arg(""); // no units

                 // get the value - from metrics or from data array
                 if (ridesummary) s = s.arg(time_to_string(metrics.getForSymbol(symbol)));
                 else s = s.arg(SummaryMetrics::getAggregated(symbol, data, filters, filtered, useMetricUnits));

             } else {
                 if (m->units(useMetricUnits) != "") s = s.arg(" (" + m->units(useMetricUnits) + ")");
                 else s = s.arg("");

                 // temperature is a special case, if it is not present fall back to metadata tag
                 // if that is not present then just display '-'
                 if ((symbol == "average_temp" || symbol == "max_temp") && metrics.getForSymbol(symbol) == RideFile::noTemp)

                    s = s.arg(ride->getTag("Temperature", "-"));

                 else if (m->internalName() == "Pace") { // pace is mm:ss

                    double pace;
                    if (ridesummary) pace  = metrics.getForSymbol(symbol) * (useMetricUnits ? 1 : m->conversion()) + (useMetricUnits ? 0 : m->conversionSum());
                    else pace = SummaryMetrics::getAggregated(symbol, data, filters, filtered, useMetricUnits).toDouble();

                    s = s.arg(QTime(0,0,0,0).addSecs(pace*60).toString("mm:ss"));

                 } else {

                    // get the value - from metrics or from data array
                    if (ridesummary) s = s.arg(metrics.getForSymbol(symbol) * (useMetricUnits ? 1 : m->conversion())
                                               + (useMetricUnits ? 0 : m->conversionSum()), 0, 'f', m->precision());
                    else s = s.arg(SummaryMetrics::getAggregated(symbol, data, filters, filtered, useMetricUnits));
                 }
            }

            summary += s;
        }
        summary += "</table></td>";
    }
    summary += "</tr></table>";

    //
    // Time In Zones
    //
    if (rideItem->numZones() > 0) {
        QVector<double> time_in_zone(rideItem->numZones());
        for (int i = 0; i < rideItem->numZones(); ++i) {

            // if using metrics or data
            if (ridesummary) time_in_zone[i] = metrics.getForSymbol(timeInZones[i]);
            else time_in_zone[i] = SummaryMetrics::getAggregated(timeInZones[i], data, filters, filtered, useMetricUnits, true).toDouble();
        }
        summary += tr("<h3>Power Zones</h3>");
        summary += mainWindow->zones()->summarize(rideItem->zoneRange(), time_in_zone); //aggregating
    }

    //
    // Time In Zones HR
    //
    if (rideItem->numHrZones() > 0) {
        QVector<double> time_in_zone(rideItem->numHrZones());
        for (int i = 0; i < rideItem->numHrZones(); ++i) {

            // if using metrics or data
            if (ridesummary) time_in_zone[i] = metrics.getForSymbol(timeInZonesHR[i]);
            else time_in_zone[i] = SummaryMetrics::getAggregated(timeInZonesHR[i], data, filters, filtered, useMetricUnits, true).toDouble();
        }

        summary += tr("<h3>Heart Rate Zones</h3>");
        summary += mainWindow->hrZones()->summarize(rideItem->hrZoneRange(), time_in_zone); //aggregating
    }

    // Only get interval summary for a ride summary
    if (ridesummary) {

        //
        // Interval Summary (recalculated on every refresh since they are not cached at present)
        //
        if (ride->intervals().size() > 0) {
            bool firstRow = true;
            QString s;
            if (appsettings->contains(GC_SETTINGS_INTERVAL_METRICS))
                s = appsettings->value(this, GC_SETTINGS_INTERVAL_METRICS).toString();
            else
                s = GC_SETTINGS_INTERVAL_METRICS_DEFAULT;
            QStringList intervalMetrics = s.split(",");
            summary += "<p><h3>"+tr("Intervals")+"</h3>\n<p>\n";
            summary += "<table align=\"center\" width=\"90%\" ";
            summary += "cellspacing=0 border=0>";
            bool even = false;
            foreach (RideFileInterval interval, ride->intervals()) {
                RideFile f(ride->startTime(), ride->recIntSecs());
                for (int i = ride->intervalBegin(interval); i < ride->dataPoints().size(); ++i) {
                    const RideFilePoint *p = ride->dataPoints()[i];
                    if (p->secs >= interval.stop)
                        break;
                    f.appendPoint(p->secs, p->cad, p->hr, p->km, p->kph, p->nm,
                                p->watts, p->alt, p->lon, p->lat, p->headwind,
                                p->slope, p->temp, p->lrbalance, 0);
                }
                if (f.dataPoints().size() == 0) {
                    // Interval empty, do not compute any metrics
                    continue;
                }

                QHash<QString,RideMetricPtr> metrics =
                    RideMetric::computeMetrics(mainWindow, &f, mainWindow->zones(), mainWindow->hrZones(), intervalMetrics);
                if (firstRow) {
                    summary += "<tr>";
                    summary += "<td align=\"center\" valign=\"bottom\">Interval Name</td>";
                    foreach (QString symbol, intervalMetrics) {
                        RideMetricPtr m = metrics.value(symbol);
                        if (!m) continue;
                        summary += "<td align=\"center\" valign=\"bottom\">" + m->name();
                        if (m->units(useMetricUnits) == "seconds" || m->units(useMetricUnits) == tr("seconds"))
                            ; // don't do anything
                        else if (m->units(useMetricUnits).size() > 0)
                            summary += " (" + m->units(useMetricUnits) + ")";
                        summary += "</td>";
                    }
                    summary += "</tr>";
                    firstRow = false;
                }
                if (even)
                    summary += "<tr>";
                else {
                    QColor color = QApplication::palette().alternateBase().color();
                    color = QColor::fromHsv(color.hue(), color.saturation() * 2, color.value());
                    summary += "<tr bgcolor='" + color.name() + "'>";
                }
                even = !even;
                summary += "<td align=\"center\">" + interval.name + "</td>";
                foreach (QString symbol, intervalMetrics) {
                    RideMetricPtr m = metrics.value(symbol);
                    if (!m) continue;
                    QString s("<td align=\"center\">%1</td>");
                    if (m->units(useMetricUnits) == "seconds" || m->units(useMetricUnits) == tr("seconds"))
                        summary += s.arg(time_to_string(m->value(useMetricUnits)));
                    else
                        summary += s.arg(m->value(useMetricUnits), 0, 'f', m->precision());
                }
                summary += "</tr>";
            }
            summary += "</table>";
        }
    }

    //
    // If summarising a date range show metrics for each ride in the date range
    //
    if (!ridesummary) {

        int j;

        // some people have a LOT of metrics, so we only show so many since
        // you quickly run out of screen space, but if they have > 4 we can
        // take out elevation and work from the totals/
        // But only show a maximum of 7 metrics
        int totalCols;
        if (metricColumn.count() > 4) totalCols = 2;
        else totalCols = rtotalColumn.count();
        int metricCols = metricColumn.count() > 7 ? 7 : metricColumn.count();

        summary += ("<p><h3>" + 
                    QString("%1").arg(data.count()) + (data.count() == 1 ? tr(" activity") : tr(" activities")) +
                    "</h3><p>");
        
        // table of activities
        summary += "<table align=\"center\" width=\"80%\" border=\"0\">";

        // header row 1 - name
        summary += "<tr>";
        summary += tr("<td align=\"center\">Date</td>");
        for (j = 0; j< totalCols; ++j) {
            QString symbol = rtotalColumn[j];
            const RideMetric *m = factory.rideMetric(symbol);

            summary += QString("<td align=\"center\">%1</td>").arg(m->name());
        }
        for (j = 0; j< metricCols; ++j) {
            QString symbol = metricColumn[j];
            const RideMetric *m = factory.rideMetric(symbol);

            summary += QString("<td align=\"center\">%1</td>").arg(m->name());
        }
        summary += "</tr>";

        // header row 2 - units
        summary += "<tr>";
        summary += tr("<td align=\"center\"></td>"); // date no units
        for (j = 0; j< totalCols; ++j) {
            QString symbol = rtotalColumn[j];
            const RideMetric *m = factory.rideMetric(symbol);

            QString units = m->units(useMetricUnits);
            if (units == tr("seconds")) units = "";
            summary += QString("<td align=\"center\">%1</td>").arg(units);
        }
        for (j = 0; j< metricCols; ++j) {
            QString symbol = metricColumn[j];
            const RideMetric *m = factory.rideMetric(symbol);

            QString units = m->units(useMetricUnits);
            if (units == tr("seconds")) units = "";
            summary += QString("<td align=\"center\">%1</td>").arg(units);
        }
        summary += "</tr>";

        // activities 1 per row
        bool even = false;
        foreach (SummaryMetrics rideMetrics, data) {

            if (even) summary += "<tr>";
            else {
                    QColor color = QApplication::palette().alternateBase().color();
                    color = QColor::fromHsv(color.hue(), color.saturation() * 2, color.value());
                    summary += "<tr bgcolor='" + color.name() + "'>";
            }
            even = !even;

            // date of ride
            summary += QString("<td align=\"center\">%1</td>")
                       .arg(rideMetrics.getRideDate().date().toString("dd MMM yyyy"));

            for (j = 0; j< totalCols; ++j) {
                QString symbol = rtotalColumn[j];

                // get this value
                QString value = rideMetrics.getStringForSymbol(symbol,useMetricUnits);
                summary += QString("<td align=\"center\">%1</td>").arg(value);
            }
            for (j = 0; j< metricCols; ++j) {
                QString symbol = metricColumn[j];

                // get this value
                QString value = rideMetrics.getStringForSymbol(symbol,useMetricUnits);
                summary += QString("<td align=\"center\">%1</td>").arg(value);
            }
            summary += "</tr>";
        }
        summary += "</table><br>";
    }

    // sumarise errors reading file if it was a ride summary
    if (ridesummary && !rideItem->errors().empty()) {

        summary += tr("<p><h2>Errors reading file:</h2><ul>");
        QStringListIterator i(rideItem->errors());
        while(i.hasNext())
            summary += " <li>" + i.next();
        summary += "</ul>";
    }
    summary += "<br><hr width=\"80%\">";

    // The extra <center> works around a bug in QT 4.3.1,
    // which will otherwise put the following above the <hr>.
    summary += tr("<br>BikeScore is a trademark of Dr. Philip "
        "Friere Skiba, PhysFarm Training Systems LLC");

    summary += tr("<br>TSS, NP and IF are trademarks of Peaksware LLC</center>");
    return summary;
}

void
RideSummaryWindow::useCustomRange(DateRange range)
{
    // plot using the supplied range
    useCustom = true;
    useToToday = false;
    custom = range;
    dateRangeChanged(custom);
}

void
RideSummaryWindow::useStandardRange()
{
    useToToday = useCustom = false;
    dateRangeChanged(myDateRange);
}

void
RideSummaryWindow::useThruToday()
{
    // plot using the supplied range
    useCustom = false;
    useToToday = true;
    custom = myDateRange;
    if (custom.to > QDate::currentDate()) custom.to = QDate::currentDate();
    dateRangeChanged(custom);
}
void RideSummaryWindow::dateRangeChanged(DateRange dr)
{
    if (!amVisible()) return;

    // range didnt change ignore it...
    if (dr.from == current.from && dr.to == current.to) return;
    else current = dr;

    if (useCustom) {
        data = mainWindow->metricDB->getAllMetricsFor(custom);
    } else if (useToToday) {

        DateRange use = myDateRange;
        QDate today = QDate::currentDate();
        if (use.to > today) use.to = today;
        data = mainWindow->metricDB->getAllMetricsFor(use);

    } else data = mainWindow->metricDB->getAllMetricsFor(myDateRange);

    refresh();
}
