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
#include "Context.h"
#include "Athlete.h"
#include "RideFile.h"
#include "RideItem.h"
#include "RideMetric.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Units.h"
#include "Zones.h"
#include "PaceZones.h"
#include "PDModel.h"
#include "MetricAggregator.h"
#include "DBAccess.h"
#include <QtGui>
#include <QtXml/QtXml>
#include <math.h>

RideSummaryWindow::RideSummaryWindow(Context *context, bool ridesummary) :
     GcChartWindow(context), context(context), ridesummary(ridesummary), useCustom(false), useToToday(false), filtered(false), bestsCache(NULL), force(false)
{
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
        searchBox = new SearchFilterBox(this, context);
        connect(searchBox, SIGNAL(searchClear()), this, SLOT(clearFilter()));
        connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(setFilter(QStringList)));
        cl->addRow(new QLabel(tr("Filter")), searchBox);
        cl->addWidget(new QLabel("")); //spacing
#endif

        cl->addRow(new QLabel(tr("Date range")), dateSetting);

        // ecp model
        ecp = new ExtendedCriticalPower(context);
    }

    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->setSpacing(0);
    vlayout->setContentsMargins(10,10,10,10);
    rideSummary = new QWebView(this);
    rideSummary->setContentsMargins(0,0,0,0);
    rideSummary->page()->view()->setContentsMargins(0,0,0,0);
    rideSummary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    rideSummary->setAcceptDrops(false);

    vlayout->addWidget(rideSummary);

    if (ridesummary) {

        connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideItemChanged()));
        connect(context->athlete, SIGNAL(zonesChanged()), this, SLOT(refresh()));
        connect(context, SIGNAL(intervalsChanged()), this, SLOT(refresh()));
        connect(context, SIGNAL(compareIntervalsStateChanged(bool)), this, SLOT(compareChanged()));
        connect(context, SIGNAL(compareIntervalsChanged()), this, SLOT(compareChanged()));

    } else {

        connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
        connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh()));
        connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh()));
        connect(context, SIGNAL(filterChanged()), this, SLOT(refresh()));
        connect(context, SIGNAL(homeFilterChanged()), this, SLOT(refresh()));
        connect(context, SIGNAL(compareDateRangesStateChanged(bool)), this, SLOT(compareChanged()));
        connect(context, SIGNAL(compareDateRangesChanged()), this, SLOT(compareChanged()));

        // date settings
        connect(dateSetting, SIGNAL(useCustomRange(DateRange)), this, SLOT(useCustomRange(DateRange)));
        connect(dateSetting, SIGNAL(useThruToday()), this, SLOT(useThruToday()));
        connect(dateSetting, SIGNAL(useStandardRange()), this, SLOT(useStandardRange()));

    }
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(this, SIGNAL(doRefresh()), this, SLOT(refresh()));
    connect(context->athlete->metricDB, SIGNAL(modelProgress(int,int)), this, SLOT(modelProgress(int,int)));

    setChartLayout(vlayout);
    configChanged(); // set colors
}

RideSummaryWindow::~RideSummaryWindow()
{
    // cancel background thread if needed
    future.cancel();
    future.waitForFinished();
}

void
RideSummaryWindow::configChanged()
{
    setProperty("color", GColor(CPLOTBACKGROUND)); // called on config change

    QFont defaultFont;
    defaultFont.fromString(appsettings->value(NULL, GC_FONT_DEFAULT, QFont().toString()).toString());
    defaultFont.setPointSize(appsettings->value(NULL, GC_FONT_DEFAULT_SIZE, 10).toInt());

#ifdef Q_OS_MAC
    rideSummary->settings()->setFontSize(QWebSettings::DefaultFontSize, defaultFont.pointSize()+1);
#else
    rideSummary->settings()->setFontSize(QWebSettings::DefaultFontSize, defaultFont.pointSize()+2);
#endif
    rideSummary->settings()->setFontFamily(QWebSettings::StandardFont, defaultFont.family());


    force = true;
    refresh();
    force = false;
}

#ifdef GC_HAVE_LUCENE
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
#endif

void 
RideSummaryWindow::modelProgress(int year, int month)
{
    // ignore if not visible!
    if (!amVisible()) return;

    QString string;

    if (!year && !month) {
        string = tr("<h3>Model</h3>");
    } else {

#if 0 // too much info, just do years ...
        QString dotstring;
        if (month < 5) dotstring = ".  ";
        else if (month < 9) dotstring = ".. ";
        else dotstring = "...";
#endif

        string = QString(tr("<h3>Modeling<br>%1</h3>")).arg(year);
    }
    rideSummary->page()->mainFrame()->evaluateJavaScript(
        QString("var div = document.getElementById(\"modhead\"); div.innerHTML = '%1'; ").arg(string));;

}

void
RideSummaryWindow::compareChanged()
{
    // don't do much for now -- just refresh
    // we get called if the items to compare changed
    // or if compare mode is switched off / on
    refresh();
    GcWindow::repaint();
}

void
RideSummaryWindow::rideSelected()
{
    // so long as you can see me and I'm not in compare mode...
    if (!isCompare() && isVisible()) refresh();
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
    if (!isCompare() && isVisible()) refresh();
    refresh();
}

void
RideSummaryWindow::refresh()
{
    if (!force && !amVisible()) return; // only if you can see me!

    if (isCompare()) { // COMPARE MODE

        if (ridesummary) {

            // offsets into compare array taking into account checks..
            int checkcount=0;
            int versus=0;

            for(int i=0; i<context->compareIntervals.count(); i++) {
                if (context->compareIntervals.at(i).isChecked()) {
                    checkcount++;
                    versus=i;
                }
            }

            // comparing intervals
            if (checkcount == 2) {

                setSubTitle(QString(tr("%2 on %1  vs  %4 on %3"))
                            .arg(context->compareIntervals.at(0).data->startTime().toString(tr("dd MMM yy")))
                            .arg(context->compareIntervals.at(0).name)
                            .arg(context->compareIntervals.at(versus).data->startTime().toString(tr("dd MMM yy")))
                            .arg(context->compareIntervals.at(versus).name));
            } else if (context->compareIntervals.count() > 2) {
                setSubTitle(QString(tr("%2 on %1  vs  %3 others"))
                            .arg(context->compareIntervals.at(0).data->startTime().toString(tr("dd MMM yy")))
                            .arg(context->compareIntervals.at(0).name)
                            .arg(checkcount-1));
            } else {
                setSubTitle(tr("Compare")); // fallback to this
            }

        } else {

            // offsets into compare array taking into account checks..
            int checkcount=0;
            int versus=0;

            for(int i=0; i<context->compareDateRanges.count(); i++) {
                if (context->compareDateRanges.at(i).isChecked()) {
                    checkcount++;
                    versus=i;
                }
            }

            // comparing seasons
            if (checkcount == 2) {

                setSubTitle(QString(tr("%1  vs  %2"))
                            .arg(context->compareDateRanges.at(0).name)
                            .arg(context->compareDateRanges.at(versus).name));

            } else if (checkcount > 2) {
                setSubTitle(QString(tr("%1  vs  %2 others"))
                            .arg(context->compareDateRanges.at(0).name)
                            .arg(checkcount-1));
            } else {
                setSubTitle(tr("Compare")); // fallback to this
            }
        }
        rideSummary->page()->mainFrame()->setHtml(htmlCompareSummary());

    } else { // NOT COMPARE MODE - NORMAL MODE

        // if we're summarising a ride but have no ride to summarise
        if (ridesummary && !myRideItem) {
            setSubTitle(tr("Summary"));
	        rideSummary->page()->mainFrame()->setHtml(GCColor::css());
            return;
        }

        setUpdatesEnabled(false); // don't flicker as caches read

        if (ridesummary) {
            RideItem *rideItem = myRideItem;
            setSubTitle(rideItem->dateTime.toString(tr("dddd MMMM d, yyyy, hh:mm")));
        } else {

            if (myDateRange.name != "") setSubTitle(myDateRange.name);
            else {
            setSubTitle(myDateRange.from.toString(tr("dddd MMMM d yyyy")) +
                        " - " +
                        myDateRange.to.toString(tr("dddd MMMM d yyyy")));
            }
        }
        rideSummary->page()->mainFrame()->setHtml(htmlSummary());

        setUpdatesEnabled(true); // ready to update now
    }
}

QString
RideSummaryWindow::htmlSummary()
{
    QString summary("");
    QColor bgColor = GColor(CPLOTBACKGROUND);
    //QColor fgColor = GCColor::invertColor(bgColor);
    QColor altColor = GCColor::alternateColor(bgColor);

    RideItem *rideItem = myRideItem;
    RideFile *ride;

    if (!rideItem && !ridesummary) return ""; // nothing selected!
    else ride = rideItem ? rideItem->ride() : NULL;

    if (!ride && !ridesummary) return ""; // didn't parse!

    bool useMetricUnits = context->athlete->useMetricUnits;

    // ride summary and there were ridefile read errors?
    if (ridesummary && !ride) {
        summary = tr("<p>Couldn't read file \"");
        summary += rideItem->fileName + "\":";
        QListIterator<QString> i(rideItem->errors());
        while (i.hasNext())
            summary += "<br>" + i.next();
        return summary;
    }

    // set those colors
    summary = GCColor::css();
    summary += "<center>";

    // device summary for ride summary, otherwise how many activities?
    if (ridesummary) summary += ("<p><h3>" + tr("Device Type: ") + ride->deviceType() + "</h3><p>");

    // All the metrics we will display
    static const QStringList columnNames = QStringList() << tr("Totals") << tr("Averages") << tr("Maximums") << tr("Metrics");
    static const QStringList totalColumn = QStringList()
        << "workout_time"
        << "time_riding"
        << "total_distance"
        << "ride_count"
        << "total_work"
        << "skiba_wprime_exp"
        << "elevation_gain";

    static const QStringList rtotalColumn = QStringList()
        << "workout_time"
        << "total_distance"
        << "ride_count"
        << "total_work"
        << "skiba_wprime_exp"
        << "elevation_gain";

    QStringList averageColumn = QStringList() // not const as modified below..
        << "athlete_weight"
        << "average_speed"
        << "average_power"
        << "average_hr"
        << "average_cad";

    QStringList maximumColumn = QStringList() // not const as modified below..
        << "max_speed"
        << "max_power"
        << "max_heartrate"
        << "max_cadence"
        << "skiba_wprime_max";

    // show average and max temp if it is available (in ride summary mode)
    if ((ridesummary && (ride->areDataPresent()->temp || ride->getTag("Temperature", "-") != "-")) ||
       (!ridesummary && SummaryMetrics::getAggregated(context, "average_temp", data, QStringList(), false, true) != "-")) {
        averageColumn << "average_temp";
        maximumColumn << "max_temp";
    }

    // if o2 data is available show the average and max
    if ((ridesummary && ride->areDataPresent()->smo2) || 
       (!ridesummary && SummaryMetrics::getAggregated(context, "average_smo2", data, QStringList(), false, true) != "-")) {
        averageColumn << "average_smo2";
        maximumColumn << "max_smo2";
    }

    // users determine the metrics to display
    QString s = appsettings->value(this, GC_SETTINGS_SUMMARY_METRICS, GC_SETTINGS_SUMMARY_METRICS_DEFAULT).toString();
    if (s == "") s = GC_SETTINGS_SUMMARY_METRICS_DEFAULT;
    QStringList metricColumn = s.split(",");

    s = appsettings->value(this, GC_SETTINGS_BESTS_METRICS, GC_SETTINGS_BESTS_METRICS_DEFAULT).toString();
    if (s == "") s = GC_SETTINGS_BESTS_METRICS_DEFAULT;
    QStringList bestsColumn = s.split(",");

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

    static const QStringList paceTimeInZones = QStringList()
        << "time_in_zone_P1"
        << "time_in_zone_P2"
        << "time_in_zone_P3"
        << "time_in_zone_P4"
        << "time_in_zone_P5"
        << "time_in_zone_P6"
        << "time_in_zone_P7"
        << "time_in_zone_P8"
        << "time_in_zone_P9"
        << "time_in_zone_P10";

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
            worklist += paceTimeInZones;
            worklist += timeInZonesHR;
            // computeMetrics expects unique keys, no duplicates
            worklist.removeDuplicates();

            // go calculate them then...
            QHash<QString, RideMetricPtr> computed = RideMetric::computeMetrics(context, ride, context->athlete->zones(), context->athlete->hrZones(), worklist);
            for(int i = 0; i < worklist.count(); ++i) {
                if (worklist[i] != "") {
                    RideMetricPtr m = computed.value(worklist[i]);
                    if (m) metrics.setForSymbol(worklist[i], m->value(true));
                    else metrics.setForSymbol(worklist[i], 0.00);
                }
            }
        } else {

            // just use the metricDB versions, nice 'n fast
            metrics = context->athlete->metricDB->getRideMetrics(rideItem->fileName);
        }
    }


    //
    // 3 top columns - total, average, maximums and metrics for entire ride
    //
    summary += "<table border=0 cellspacing=10><tr>";
    for (int i = 0; i < columnNames.count(); ++i) {
        summary += "<td align=\"center\" valign=\"top\" width=\"%1%%\"><table>"
            "<tr><td align=\"center\" colspan=2><h3>%2</h3></td></tr>";
        summary = summary.arg(90 / (columnNames.count() + (ridesummary ? 0 : 1)));
        summary = summary.arg(columnNames[i]);

        QStringList metricsList;
        switch (i) {
            case 0: metricsList = totalColumn; break;
            case 1: metricsList = averageColumn; break;
            case 2: metricsList = maximumColumn; break;
            default: 
            case 3: metricsList = metricColumn; break;
        }
        for (int j = 0; j< metricsList.count(); ++j) {
            QString symbol = metricsList[j];

             if (symbol == "") continue;

             // don't need a count of 1 on a ride summary !
             if (symbol == "ride_count" && ridesummary) continue;

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
                 else {
                      QStringList filterList = filters;
                      if (context->ishomefiltered) {
                          if (filtered) {
                              foreach (QString file, filters) {
                                  if (context->homeFilters.contains(file)) 
                                      filterList << file;
                              }
                          } else {
                              filterList = context->homeFilters;
                          }
                      }
                      s = s.arg(SummaryMetrics::getAggregated(context, symbol, data, filterList, context->ishomefiltered || filtered, useMetricUnits));            }

             } else {
                 if (m->units(useMetricUnits) != "") s = s.arg(" (" + m->units(useMetricUnits) + ")");
                 else s = s.arg("");

                 // temperature is a special case, if it is not present fall back to metadata tag
                 // if that is not present then just display '-'

                 // when summarising a ride temperature is -255 when not present, when aggregating its 0.0
                 if ((symbol == "average_temp" || symbol == "max_temp") && ridesummary 
                     && metrics.getForSymbol(symbol) == RideFile::NoTemp) {

                    s = s.arg(ride->getTag("Temperature", "-"));

                 } else if (m->internalName() == "Pace" || m->internalName() == "xPace") { // pace is mm:ss

                    double pace;
                    bool metricPace = appsettings->value(this, GC_PACE, true).toBool();

                    if (ridesummary) pace  = metrics.getForSymbol(symbol) * (metricPace ? 1 : m->conversion()) + (metricPace ? 0 : m->conversionSum());
                    else {
                      QStringList filterList = filters;
                      if (context->ishomefiltered) {
                          if (filtered) {
                              foreach (QString file, filters) {
                                  if (context->homeFilters.contains(file)) 
                                      filterList << file;
                              }
                          } else {
                              filterList = context->homeFilters;
                          }
                      }
                      pace = SummaryMetrics::getAggregated(context, symbol, data, filterList, context->ishomefiltered || filtered, metricPace).toDouble();
                    }

                    s = s.arg(QTime(0,0,0,0).addSecs(pace*60).toString("mm:ss"));

                 } else {

                    // get the value - from metrics or from data array
                    if (ridesummary) {
                            QString v = QString("%1").arg(metrics.getForSymbol(symbol) * (useMetricUnits ? 1 : m->conversion())
                                + (useMetricUnits ? 0 : m->conversionSum()), 0, 'f', m->precision());

                            // W' over 100% is not a good thing!
                            if (symbol == "skiba_wprime_max" && metrics.getForSymbol(symbol) > 100) {
                                v = QString("<font color=\"red\">%1<font color=\"black\">").arg(v);
                            }
                            s = s.arg(v);
        
                    } else {
                      QStringList filterList = filters;
                      if (context->ishomefiltered) {
                          if (filtered) {
                              foreach (QString file, filters) {
                                  if (context->homeFilters.contains(file)) 
                                      filterList << file;
                              }
                          } else {
                              filterList = context->homeFilters;
                          }
                      }
                      s = s.arg(SummaryMetrics::getAggregated(context, symbol, data, filterList, context->ishomefiltered || filtered, useMetricUnits));
                   }
                 }
            }

            summary += s;
        }
        summary += "</table></td>";
    }

    // get the PDEStimates for this athlete
    if (context->athlete->PDEstimates.count() == 0) {

        // ugh .. refresh in background
        WPrimeStringWPK = CPStringWPK = FTPStringWPK = PMaxStringWPK = 
        WPrimeString = CPString = FTPString = PMaxString = "-";
        future = QtConcurrent::run(this, &RideSummaryWindow::getPDEstimates);

    } else {

        // set from cached values
        getPDEstimates();
    }

    // MODEL 
    // lets get a table going
    summary += "<td align=\"center\" valign=\"top\" width=\"%1%%\"><table>"
        "<tr><td align=\"center\" colspan=2><div id=\"modhead\">%2</div></td></tr>";

    summary = summary.arg(90 / columnNames.count()+1);
    summary = summary.arg(tr("<h3>Model</h3>"));

    // W;
    summary += QString("<tr><td>%1:</td><td align=\"right\">%2 kJ</td></tr>")
            .arg(tr("W'"))
            .arg(WPrimeString);
    summary += QString("<tr><td></td><td align=\"right\">%1 J/kg</td></tr>")
            .arg(WPrimeStringWPK);

    // spacer
    summary += "<tr style=\"height: 3px;\"></tr>";

    // CP;
    summary += QString("<tr><td>%1:</td><td align=\"right\">%2 %3</td></tr>")
            .arg(tr("CP"))
            .arg(CPString)
            .arg(tr("watts"));
    summary += QString("<tr><td></td><td align=\"right\">%1 %2</td></tr>")
            .arg(CPStringWPK)
            .arg(tr("w/kg"));

    // spacer
    summary += "<tr style=\"height: 3px;\"></tr>";

#if 0 // clutters it up and adds almost nothing
    // FTP/MMP60;
    summary += QString("<tr><td>%1:</td><td align=\"right\">%2</td></tr>")
            .arg(tr("FTP (watts)"))
            .arg(FTPString);
    summary += QString("<tr><td>%1:</td><td align=\"right\">%2</td></tr>")
            .arg(tr("FTP (w/kg)"))
            .arg(FTPStringWPK);
#endif

    // Pmax;
    summary += QString("<tr><td>%1:</td><td align=\"right\">%2 %3</td></tr>")
            .arg(tr("P-max"))
            .arg(PMaxString)
            .arg(tr("watts"));
    summary += QString("<tr><td></td><td align=\"right\">%1 %2</td></tr>")
            .arg(PMaxStringWPK)
            .arg(tr("w/kg"));

    summary += "</table></td>";
    summary += "</tr></table>";

    //
    // Bests for the period
    //
    if (!ridesummary) {
        summary += tr("<h3>Athlete Bests</h3>\n");

        // best headings
        summary += "<table border=0 cellspacing=10 width=\"90%%\"><tr>";
        for (int i = 0; i < bestsColumn.count(); ++i) {
        summary += "<td align=\"center\" valign=\"top\"><table width=\"100%%\">"
            "<tr><td align=\"center\" colspan=3><h4>%1<h4></td></tr>";

            //summary = summary.arg(90 / bestsColumn.count());
            const RideMetric *m = factory.rideMetric(bestsColumn[i]);
            summary = summary.arg(m->name());

            // get top n
            QStringList filterList = filters;
            if (context->ishomefiltered) {
                if (filtered) {
                    foreach (QString file, filters) {
                        if (context->homeFilters.contains(file)) 
                            filterList << file;
                    }
                } else {
                    filterList = context->homeFilters;
                }
            }
            QList<SummaryBest> bests = SummaryMetrics::getBests(context, bestsColumn[i], 10, data, filterList, context->ishomefiltered || filtered, useMetricUnits);

            int pos=1;
            foreach(SummaryBest best, bests) {

                // alternating shading
                if (pos%2) summary += "<tr bgcolor='" + altColor.name() + "'";
                else summary += "<tr";

                if (best.date == QDate::currentDate()) {
                    // its today -- highlight it !
                    summary += " id=\"sharp\" ";
                }
                summary += " >";

                summary += QString("<td align=\"center\">%1.</td><td align=\"center\">%2</td><td align=\"center\">%3</td></tr>")
                           .arg(pos++)
                           .arg(best.value)
                           .arg(best.date.toString(tr("d MMM yyyy")));
            }

            // close that column
            summary += "</table></td>";
        }
        // close the table
        summary += "</tr></table>";

    }

    //
    // Time In Zones
    //
    int numzones = 0;
    int range = -1;

    if (ridesummary && rideItem && rideItem->ride() && rideItem->ride()->isRun()) {

        if (context->athlete->paceZones()) {

            // no power zones on a run, should show pace here ...
            // ... when we have pace zones implemented
            range = context->athlete->paceZones()->whichRange(rideItem->dateTime.date());
            if (range > -1) {

                numzones = context->athlete->paceZones()->numZones(range);

                if (numzones > 0) {

                    // we have a valid range and it has at least one zone - lets go
                    QVector<double> time_in_zone(numzones);
                    for (int i = 0; i < numzones; ++i) {

                        // if using metrics or data
                        if (ridesummary) time_in_zone[i] = metrics.getForSymbol(paceTimeInZones[i]);
                        else { // *** THIS IS NOT RELEVANT YET -- NO SUMMARISING FOR SEASONS ***
                            QStringList filterList = filters;
                            if (context->ishomefiltered) {
                                if (filtered) {
                                    foreach (QString file, filters) {
                                        if (context->homeFilters.contains(file)) filterList << file;
                                    }
                                } else {
                                    filterList = context->homeFilters;
                                }
                            }
                            time_in_zone[i] = SummaryMetrics::getAggregated(context, paceTimeInZones[i], data, filterList, context->ishomefiltered || filtered, useMetricUnits, true).toDouble();
                        }
                    }
        
                    summary += tr("<h3>Pace Zones</h3>");
                    summary += context->athlete->paceZones()->summarize(range, time_in_zone, altColor); //aggregating
                }
            }
        }

    } else {

        if (ridesummary && rideItem && context->athlete->zones()) {

            // get zones to use via ride for ridesummary
            range = context->athlete->zones()->whichRange(rideItem->dateTime.date());
            if (range > -1) numzones = context->athlete->zones()->numZones(range);

        // or for end of daterange plotted for daterange summary
        } else if (context->athlete->zones()) {

            // get from end if period
            range = context->athlete->zones()->whichRange(myDateRange.to);
            if (range > -1) numzones = context->athlete->zones()->numZones(range);

        }

        if (range > -1 && numzones > 0) {
            QVector<double> time_in_zone(numzones);
            for (int i = 0; i < numzones; ++i) {

                // if using metrics or data
                if (ridesummary) time_in_zone[i] = metrics.getForSymbol(timeInZones[i]);
                else {
                    QStringList filterList = filters;
                    if (context->ishomefiltered) {
                        if (filtered) {
                            foreach (QString file, filters) {
                                if (context->homeFilters.contains(file)) 
                                    filterList << file;
                            }
                        } else {
                            filterList = context->homeFilters;
                        }
                    }
                    time_in_zone[i] = SummaryMetrics::getAggregated(context, timeInZones[i], data, filterList, context->ishomefiltered || filtered, useMetricUnits, true).toDouble();
                }
            }
            summary += tr("<h3>Power Zones</h3>");
            summary += context->athlete->zones()->summarize(range, time_in_zone, altColor); //aggregating
        }
    }

    //
    // Time In Zones HR
    //
    int numhrzones = 0;
    int hrrange = -1;

    // get zones to use via ride for ridesummary
    if (ridesummary && rideItem && context->athlete->hrZones()) {

        // get zones to use via ride for ridesummary
        hrrange = context->athlete->hrZones()->whichRange(rideItem->dateTime.date());
        if (hrrange > -1) numhrzones = context->athlete->hrZones()->numZones(hrrange);

    // or for end of daterange plotted for daterange summary
    } else if (context->athlete->hrZones()) {

        // get from end if period
        hrrange = context->athlete->hrZones()->whichRange(myDateRange.to);
        if (hrrange > -1) numhrzones = context->athlete->hrZones()->numZones(hrrange);

    }

    if (hrrange > -1 && numhrzones > 0) {

        QVector<double> time_in_zone(numhrzones);
        for (int i = 0; i < numhrzones; ++i) {

            // if using metrics or data
            if (ridesummary) time_in_zone[i] = metrics.getForSymbol(timeInZonesHR[i]);
            else {
                QStringList filterList = filters;
                if (context->ishomefiltered) {
                    if (filtered) {
                        foreach (QString file, filters) {
                            if (context->homeFilters.contains(file)) 
                                filterList << file;
                        }
                    } else {
                        filterList = context->homeFilters;
                    }
                }
                time_in_zone[i] = SummaryMetrics::getAggregated(context, timeInZonesHR[i], data, filterList, context->ishomefiltered || filtered, useMetricUnits, true).toDouble();
            }
        }

        summary += tr("<h3>Heart Rate Zones</h3>");
        summary += context->athlete->hrZones()->summarize(hrrange, time_in_zone, altColor); //aggregating
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
                RideFile f(ride);
                for (int i = ride->intervalBegin(interval); i>= 0 &&i < ride->dataPoints().size(); ++i) {
                    const RideFilePoint *p = ride->dataPoints()[i];
                    if (p->secs > interval.stop)
                        break;
                    f.appendPoint(p->secs, p->cad, p->hr, p->km, p->kph, p->nm,
                                p->watts, p->alt, p->lon, p->lat, p->headwind,
                                p->slope, p->temp, p->lrbalance, p->lte, p->rte, p->lps, p->rps, 
                                p->smo2, p->thb, p->rvert, p->rcad, p->rcontact, 0);

                    // derived data
                    RideFilePoint *l = f.dataPoints().last();
                    l->np = p->np;
                    l->xp = p->xp;
                    l->apower = p->apower;
                }
                if (f.dataPoints().size() == 0) {
                    // Interval empty, do not compute any metrics
                    continue;
                }

                QHash<QString,RideMetricPtr> metrics =
                    RideMetric::computeMetrics(context, &f, context->athlete->zones(), context->athlete->hrZones(), intervalMetrics);
                bool metricPace = appsettings->value(this, GC_PACE, true).toBool();

                if (firstRow) {
                    summary += "<tr>";
                    summary += "<td align=\"center\" valign=\"bottom\">"+tr("Interval Name")+"</td>";
                    foreach (QString symbol, intervalMetrics) {
                        RideMetricPtr m = metrics.value(symbol);
                        if (!m) continue;
                        summary += "<td align=\"center\" valign=\"bottom\">" + m->name();
                        if (m->internalName() == "Pace" || m->internalName() == "xPace") { // pace is mm:ss

                            summary += " (" + m->units(metricPace) + ")";
                        
                        } else if (m->units(useMetricUnits) == "seconds" || m->units(useMetricUnits) == tr("seconds")) {
                            ; // don't do anything

                        } else if (m->units(useMetricUnits).size() > 0) {

                            summary += " (" + m->units(useMetricUnits) + ")";

                        }

                        summary += "</td>";
                    }
                    summary += "</tr>";
                    firstRow = false;
                }
                if (even)
                    summary += "<tr>";
                else {
                    summary += "<tr bgcolor='" + altColor.name() + "'>";
                }
                even = !even;
                summary += "<td align=\"center\">" + interval.name + "</td>";
                foreach (QString symbol, intervalMetrics) {
                    RideMetricPtr m = metrics.value(symbol);
                    if (!m) continue;
                    QString s("<td align=\"center\">%1</td>");
                    if (m->units(useMetricUnits) == "seconds" || m->units(useMetricUnits) == tr("seconds"))
                        summary += s.arg(time_to_string(m->value(useMetricUnits)));
                    else if (m->internalName() == "Pace" || m->internalName() == "xPace") { // pace is mm:ss

                        double pace  = m->value(metricPace);
                        summary += s.arg(QTime(0,0,0,0).addSecs(pace*60).toString("mm:ss"));

                    } else {
                        summary += s.arg(m->value(useMetricUnits), 0, 'f', m->precision());
                    }
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

        // if we are filtered we need to count the number of activities
        // we have after filtering has been applied, otherwise it is just
        // the number of entries
        int activities = 0;
        int runs = 0;
        int rides = 0;
        int totalruns = 0;
        int totalrides = 0;
        if (context->ishomefiltered || context->isfiltered || filtered) {

            foreach (SummaryMetrics activity, data) {
                if (activity.isRun()) {
                    totalruns++;
                } else {
                    totalrides++;
                }
                if (filtered && !filters.contains(activity.getFileName())) continue;
                if (context->isfiltered && !context->filters.contains(activity.getFileName())) continue;
                if (context->ishomefiltered && !context->homeFilters.contains(activity.getFileName())) continue;
                if (activity.isRun()) {
                    runs++;
                } else {
                    rides++;
                }
                activities++;
            }

        } else {
            activities = data.count();
            foreach (SummaryMetrics activity, data) {
               if (activity.isRun()) {
                    runs++;
                } else {
                    rides++;
                }
            }

        }

        // some people have a LOT of metrics, so we only show so many since
        // you quickly run out of screen space, but if they have > 4 we can
        // take out elevation and work from the totals/
        // But only show a maximum of 7 metrics
        int totalCols;
        if (metricColumn.count() > 4) totalCols = 2;
        else totalCols = rtotalColumn.count();
        int metricCols = metricColumn.count() > 7 ? 7 : metricColumn.count();

        //Rides first
        if (context->ishomefiltered || context->isfiltered || filtered) {

            // "n of x activities" shown in header of list when filtered
            summary += ("<p><h3>" +
                        QString(tr("%1 of %2")).arg(rides).arg(totalrides)
                                           + (totalrides == 1 ? tr(" ride") : tr(" rides")) +
                        "</h3><p>");
        } else {

            // just "n activities" shown in header of list when not filtered
            summary += ("<p><h3>" +
                        QString("%1").arg(rides) + (rides == 1 ? tr(" ride") : tr(" rides")) +
                        "</h3><p>");
        }
        
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
            if (units == "seconds" || units == tr("seconds")) units = "";
            summary += QString("<td align=\"center\">%1</td>").arg(units);
        }
        for (j = 0; j< metricCols; ++j) {
            QString symbol = metricColumn[j];
            const RideMetric *m = factory.rideMetric(symbol);

            QString units = m->units(useMetricUnits);
            if (units == "seconds" || units == tr("seconds")) units = "";
            summary += QString("<td align=\"center\">%1</td>").arg(units);
        }
        summary += "</tr>";

        // activities 1 per row
        bool even = false;
        foreach (SummaryMetrics rideMetrics, data) {

            // apply the filter if there is one active
            if (filtered && !filters.contains(rideMetrics.getFileName())) continue;
            if (context->isfiltered && !context->filters.contains(rideMetrics.getFileName())) continue;
            if (context->ishomefiltered && !context->homeFilters.contains(rideMetrics.getFileName())) continue;

            if (rideMetrics.isRun()) continue;

            if (even) summary += "<tr>";
            else {
                    summary += "<tr bgcolor='" + altColor.name() + "'>";
            }
            even = !even;

            // date of ride
            summary += QString("<td align=\"center\">%1</td>")
                       .arg(rideMetrics.getRideDate().date().toString(tr("dd MMM yyyy")));

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

        //Now runs
        if (context->ishomefiltered || context->isfiltered || filtered) {

            // "n of x activities" shown in header of list when filtered
            summary += ("<p><h3>" +
                        QString(tr("%1 of %2")).arg(runs).arg(totalruns)
                                           + (totalruns == 1 ? tr(" run") : tr(" runs")) +
                        "</h3><p>");
        } else {

            // just "n activities" shown in header of list when not filtered
            summary += ("<p><h3>" +
                        QString("%1").arg(runs) + (runs == 1 ? tr(" run") : tr(" runs")) +
                        "</h3><p>");
        }

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
            if (units == "seconds" || units == tr("seconds")) units = "";
            summary += QString("<td align=\"center\">%1</td>").arg(units);
        }
        for (j = 0; j< metricCols; ++j) {
            QString symbol = metricColumn[j];
            const RideMetric *m = factory.rideMetric(symbol);

            QString units = m->units(useMetricUnits);
            if (units == "seconds" || units == tr("seconds")) units = "";
            summary += QString("<td align=\"center\">%1</td>").arg(units);
        }
        summary += "</tr>";

        // activities 1 per row
        even = false;
        foreach (SummaryMetrics rideMetrics, data) {

            // apply the filter if there is one active
            if (filtered && !filters.contains(rideMetrics.getFileName())) continue;
            if (context->isfiltered && !context->filters.contains(rideMetrics.getFileName())) continue;
            if (context->ishomefiltered && !context->homeFilters.contains(rideMetrics.getFileName())) continue;

            if (!rideMetrics.isRun()) continue;

            if (even) summary += "<tr>";
            else {
                    summary += "<tr bgcolor='" + altColor.name() + "'>";
            }
            even = !even;

            // date of ride
            summary += QString("<td align=\"center\">%1</td>")
                       .arg(rideMetrics.getRideDate().date().toString(tr("dd MMM yyyy")));

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

    // summarise errors reading file if it was a ride summary
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

    summary += tr("<br>FTP, TSS, NP and IF are trademarks of Peaksware LLC</center>");
    return summary;
}

void
RideSummaryWindow::getPDEstimates()
{
    bool ping = false;
    // refresh if needed
    if (context->athlete->PDEstimates.count() == 0) {
        ping = true;
        context->athlete->metricDB->refreshCPModelMetrics(true); //true = bg ...
    }

    for (int i=0; i<2; i++) {

        // two times, once to get wpk versions
        // second time to get normal
        bool wpk = (i==0);

        double lowWPrime=0, highWPrime=0,
               lowCP=0, highCP=0,
               lowFTP=0, highFTP=0,
               lowPMax=0, highPMax=0;

        // loop through and get ...
        foreach(PDEstimate est, context->athlete->PDEstimates) {

            // filter is set above
            if (est.wpk != wpk) continue;

            // We only use the extended model for now
            if (est.model != "Ext" ) continue;

            // summarising a season or date range
            if (!ridesummary && (est.from > myDateRange.to || est.to < myDateRange.from)) continue;

            // it definitely wasnt during our period
            if (!ridesummary && !((est.from >= myDateRange.from && est.from <= myDateRange.to) || (est.to >= myDateRange.from && est.to <= myDateRange.to) ||
                (est.from <= myDateRange.from && est.to >= myDateRange.to))) continue;

    
            // summarising a ride
            if (ridesummary && (!myRideItem || (myRideItem->dateTime.date() < est.from || myRideItem->dateTime.date() > est.to))) continue;

            // set low
            if (!lowWPrime || est.WPrime < lowWPrime) lowWPrime = est.WPrime;
            if (!lowCP || est.CP < lowCP) lowCP = est.CP;
            if (!lowFTP || est.FTP < lowFTP) lowFTP = est.FTP;
            if (!lowPMax || est.PMax < lowPMax) lowPMax = est.PMax;

            // set high
            if (!highWPrime || est.WPrime > highWPrime) highWPrime = est.WPrime;
            if (!highCP || est.CP > highCP) highCP = est.CP;
            if (!highFTP || est.FTP > highFTP) highFTP = est.FTP;
            if (!highPMax || est.PMax > highPMax) highPMax = est.PMax;
        }

        // ok, so lets set the string to either
        // N/A => when its not available
        // low - high => when its a range
        // val => when low = high
        double divisor = wpk ? 1.0f : 1000.00f;

        if (!lowWPrime && !highWPrime) WPrimeString = tr("N/A");
        else if (lowWPrime != highWPrime) WPrimeString = QString ("%1 - %2").arg(lowWPrime/divisor, 0, 'f',  wpk ? 0 : 1).arg(highWPrime/divisor, 0, 'f', wpk ? 0 : 1);
        else WPrimeString = QString("%1").arg(highWPrime/divisor, 0, 'f', wpk ? 0 : 1);

        if (!lowCP && !highCP) CPString = tr("N/A");
        else if (lowCP != highCP) CPString = QString ("%1 - %2").arg(lowCP, 0, 'f', wpk ? 2 : 0)
                                                                .arg(highCP, 0, 'f', wpk ? 2 : 0);
        else CPString = QString("%1").arg(highCP, 0, 'f', wpk ? 2 : 0);

        if (!lowFTP && !highFTP) FTPString = tr("N/A");
        else if (lowFTP != highFTP) FTPString = QString ("%1 - %2").arg(lowFTP, 0, 'f', wpk ? 2 : 0)
                                                                   .arg(highFTP, 0, 'f', wpk ? 2 : 0);
        else FTPString = QString("%1").arg(highFTP, 0, 'f', wpk ? 2 : 0);

        if (!lowPMax && !highPMax) PMaxString = tr("N/A");
        else if (lowPMax != highPMax) PMaxString = QString ("%1 - %2").arg(lowPMax, 0, 'f', wpk ? 2 : 0)
                                                                      .arg(highPMax, 0, 'f', wpk ? 2 : 0);
        else PMaxString = QString("%1").arg(highPMax, 0, 'f', wpk ? 2 : 0);

        // actually we just set the wPK versions:
        if (wpk) {
            WPrimeStringWPK = WPrimeString;
            CPStringWPK = CPString;
            FTPStringWPK = FTPString;
            PMaxStringWPK = PMaxString;
        }
    }

    // now refresh !
    if (ping) emit doRefresh();
}

QString
RideSummaryWindow::htmlCompareSummary() const
{
    QString summary;

    QColor bgColor = GColor(CPLOTBACKGROUND);
    //QColor fgColor = GCColor::invertColor(bgColor);
    QColor altColor = GCColor::alternateColor(bgColor);

    // SETUP ALL THE METRICS WE WILL SHOW

    // All the metrics we will display -- same as non compare mode FOR NOW
    static const QStringList columnNames = QStringList() << tr("Totals") << tr("Averages") << tr("Maximums") << tr("Metrics*");
    static const QStringList totalColumn = QStringList()
        << "workout_time"
        << "time_riding"
        << "total_distance"
        << "ride_count"
        << "total_work"
        << "skiba_wprime_exp"
        << "elevation_gain";

    static const QStringList rtotalColumn = QStringList()
        << "workout_time"
        << "total_distance"
        << "total_work"
        << "elevation_gain";

    QStringList averageColumn = QStringList() // not const as modified below..
        << "athlete_weight"
        << "average_speed"
        << "average_power"
        << "average_hr"
        << "average_cad";

    QStringList maximumColumn = QStringList() // not const as modified below..
        << "max_speed"
        << "max_power"
        << "max_heartrate"
        << "max_cadence"
        << "skiba_wprime_max";

#if 0 // XXX do /any/ of them have temperature -or- do they /all/ need to ???
    // show average and max temp if it is available (in ride summary mode)
    if (ridesummary && (ride->areDataPresent()->temp || ride->getTag("Temperature", "-") != "-")) {
        averageColumn << "average_temp";
        maximumColumn << "max_temp";
    }
#endif

    // users determine the metrics to display
    QString s = appsettings->value(this, GC_SETTINGS_SUMMARY_METRICS, GC_SETTINGS_SUMMARY_METRICS_DEFAULT).toString();
    if (s == "") s = GC_SETTINGS_SUMMARY_METRICS_DEFAULT;
    QStringList metricColumn = s.split(",");

    s = appsettings->value(this, GC_SETTINGS_BESTS_METRICS, GC_SETTINGS_BESTS_METRICS_DEFAULT).toString();
    if (s == "") s = GC_SETTINGS_BESTS_METRICS_DEFAULT;
    QStringList bestsColumn = s.split(",");

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

    if (ridesummary) {

        //
        // SUMMARISING INTERVALS SO ALWAYS COMPUTE METRICS ON DEMAND
        //
        QList<SummaryMetrics> intervalMetrics;

        QStringList worklist;
        worklist += totalColumn;
        worklist += averageColumn;
        worklist += maximumColumn;
        worklist += metricColumn;
        worklist += timeInZones;
        worklist += timeInZonesHR;
        // computeMetrics expects unique keys, no duplicates
        worklist.removeDuplicates();

        // go calculate them then...
        RideMetricFactory &factory = RideMetricFactory::instance();
        for (int j=0; j<context->compareIntervals.count(); j++) {

            SummaryMetrics metrics;

            // calculate using the source context of course!
            QHash<QString, RideMetricPtr> computed = RideMetric::computeMetrics(
                                                     context->compareIntervals.at(j).sourceContext,
                                                     context->compareIntervals.at(j).data,
                                                     context->compareIntervals.at(j).sourceContext->athlete->zones(),
                                                     context->compareIntervals.at(j).sourceContext->athlete->hrZones(), 
                                                     worklist);

            for(int i = 0; i < worklist.count(); ++i) {
                if (worklist[i] != "") {
                    RideMetricPtr m = computed.value(worklist[i]);
                    if (m) metrics.setForSymbol(worklist[i], m->value(true));
                    else metrics.setForSymbol(worklist[i], 0.00);
                }
            }
            intervalMetrics << metrics;
        }

        // LETS FORMAT THE HTML
        summary = GCColor::css();
        summary += "<center>";

        //
        // TOTALS, AVERAGES, MAX, METRICS
        //
        for (int v = 0; v < columnNames.count(); ++v) {

            QString columnName;
            QStringList metricsList;

            switch (v) { // slightly different order when summarising for compare...
                         // we want the metrics towards the top of the screen as they
                         // are more important than the maximums (generally anyway)

                case 0: metricsList = totalColumn; columnName = columnNames[0]; break;
                case 1: metricsList = metricColumn; columnName = columnNames[3]; break;
                case 2: metricsList = averageColumn; columnName = columnNames[1]; break;
                default: 
                case 3: metricsList = maximumColumn; columnName = columnNames[2]; break;
            }

            //
            // Repeat for each 'column' (now separate paragraphs)
            //
            summary += "<h3>" + columnName + "</h3>";

            // table of metrics
            summary += "<table align=\"center\" width=\"80%\" border=\"0\">";

            // first row is a row of headings
            summary += "<tr>";
            summary += "<td><b></b></td>"; // removed the text as its blinking obvious.. but left code in
                                             // case we ever come back here or use it for other things.
            summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

            foreach (QString symbol, metricsList) {
                const RideMetric *m = factory.rideMetric(symbol);

                QString name, units;
                if (!(m->units(context->athlete->useMetricUnits) == "seconds" || m->units(context->athlete->useMetricUnits) == tr("seconds")))
                        units = m->units(context->athlete->useMetricUnits);
                if (units != "") name = QString("%1 (%2)").arg(m->name()).arg(units);
                else name = QString("%1").arg(m->name());

                name = name.replace(QRegExp(tr("^(Average|Max) ")), ""); // average/max on average/max is dumb
                summary += "<td align=\"center\" colspan=2><b>" +  name + "</b></td>";
                summary += "<td>&nbsp;</td>"; // spacing
            }
            summary += "</tr>";

            // then one row for each interval
            int counter = 0;
            int rows = 0;
            foreach (SummaryMetrics metrics, intervalMetrics) {

                // skip if isn't checked
                if (!context->compareIntervals[counter].isChecked()) {
                    counter++;
                    continue;
                }

                // alternating shading
                if (rows%2) summary += "<tr bgcolor='" + altColor.name() + "'>";
                else summary += "<tr>";

                summary += "<td>" + context->compareIntervals[counter].name + "</td>";
                summary += "<td bgcolor='" + bgColor.name() +"'>&nbsp;</td>"; // spacing

                foreach (QString symbol, metricsList) {

                    // the values ...
                    const RideMetric *m = factory.rideMetric(symbol);

                    // get value and convert if needed (use local context for units)
                    double value = metrics.getForSymbol(symbol) 
                                * (context->athlete->useMetricUnits ? 1 : m->conversion()) 
                                + (context->athlete->useMetricUnits ? 0 : m->conversionSum());

                    // use right precision
                    QString strValue = QString("%1").arg(value, 0, 'f', m->precision());

                    // or maybe its a duration (worry about local lang or translated)
                    if (m->units(true) == "seconds" || m->units(true) == tr("seconds"))
                        strValue = time_to_string((int)value);

                    summary += "<td align=\"center\">" + strValue + "</td>";

                    // delta to first entry
                    if (counter) {

                        // calculate me vs the original
                        double value0 = intervalMetrics[0].getForSymbol(symbol) 
                                * (context->athlete->useMetricUnits ? 1 : m->conversion()) 
                                + (context->athlete->useMetricUnits ? 0 : m->conversionSum());

                        value -= value0; // delta

                        // use right precision
                        QString strValue = QString("%1%2").arg(value >= 0 ? "+" : "") // - sign added anyway
                                                        .arg(value, 0, 'f', m->precision());

                        // or maybe its a duration (worry about local lang or translated)
                        if (m->units(true) == "seconds" || m->units(true) == tr("seconds"))
                            strValue = QString(value >= 0 ? "+" : "-" ) + time_to_string(fabs(value));

                        summary += "<td align=\"center\">" + strValue + "</td>";
                    } else {
                        summary += "<td align=\"center\"></td>";
                    }
                    summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

                }
                summary += "</tr>";
                rows ++; counter++;
            }
            summary += "</table>";
        }

        //
        // TIME IN POWER ZONES
        //
        if (context->athlete->zones()) { // use my zones

            // get from end if period
            int rangeidx = context->athlete->zones()->whichRange(QDate::currentDate()); // use current zone names et al
            if (rangeidx > -1) {

                // get the list of zones
                ZoneRange range = const_cast<Zones*>(context->athlete->zones())->getZoneRange(rangeidx);
                QList<ZoneInfo> zones = range.zones;

                // we've got a range and a count of zones so all is well
                // we need to throw up a table of time in zone for each interval
                summary += tr("<h3>Power Zones</h3>");
                summary += "<table align=\"center\" width=\"80%\" border=\"0\">";

                // lets get some headings
                summary += "<tr><td></td>"; // ne need to have a heading for the interval name
                summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

                foreach (ZoneInfo zone, zones) {
                    summary += QString("<td colspan=\"2\" align=\"center\"><b>%1 (%2)</b></td>").arg(zone.desc).arg(zone.name);
                    summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing
                }
                summary += "</tr>";

                // now the summary
                int counter = 0;
                int rows = 0;
                foreach (SummaryMetrics metrics, intervalMetrics) {

                    // only ones that are checked
                    if (!context->compareIntervals[counter].isChecked()) {
                        counter++;
                        continue;
                    }

                    if (rows%2) summary += "<tr bgcolor='" + altColor.name() + "'>";
                    else summary += "<tr>";

                    summary += "<td>" + context->compareIntervals[counter].name + "</td>";
                    summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

                    int idx=0;
                    foreach (ZoneInfo zone, zones) {

                        int timeZone = metrics.getForSymbol(timeInZones[idx]);
                        int dt = timeZone - intervalMetrics[0].getForSymbol(timeInZones[idx]);
                        idx++;

                        // time and then +time
                        summary += QString("<td align=\"center\">%1</td>").arg(time_to_string(timeZone));

                        if (counter) summary += QString("<td align=\"center\">%1%2</td>")
                                                .arg(dt>0 ? "+" : "-")
                                                .arg(time_to_string(fabs(dt)));

                        else summary += "<td></td>";
                        summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

                    }
                    summary += "</tr>";
                    rows++; counter++;
                }

                // done
                summary += "</table>";
            }
        }

        //
        // TIME IN HR ZONES
        //
        if (context->athlete->hrZones()) { // use my zones

            // get from end if period
            int rangeidx = context->athlete->hrZones()->whichRange(QDate::currentDate()); // use current zone names et al
            if (rangeidx > -1) {

                // get the list of zones
                HrZoneRange range = const_cast<HrZones*>(context->athlete->hrZones())->getHrZoneRange(rangeidx);
                QList<HrZoneInfo> zones = range.zones;

                // we've got a range and a count of zones so all is well
                // we need to throw up a table of time in zone for each interval
                summary += tr("<h3>Heartrate Zones</h3>");
                summary += "<table align=\"center\" width=\"80%\" border=\"0\">";

                // lets get some headings
                summary += "<tr><td></td>"; // ne need to have a heading for the interval name
                summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

                foreach (HrZoneInfo zone, zones) {
                    summary += QString("<td colspan=\"2\" align=\"center\"><b>%1 (%2)</b></td>").arg(zone.desc).arg(zone.name);
                    summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing
                }
                summary += "</tr>";

                // now the summary
                int counter = 0;
                int rows = 0;
                foreach (SummaryMetrics metrics, intervalMetrics) {

                    // skip if not checked
                    if (!context->compareIntervals[counter].isChecked()) {
                        counter++;
                        continue;
                    }

                    if (rows%2) summary += "<tr bgcolor='" + altColor.name() + "'>";
                    else summary += "<tr>";

                    summary += "<td>" + context->compareIntervals[counter].name + "</td>";
                    summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

                    int idx=0;
                    foreach (HrZoneInfo zone, zones) {

                        int timeZone = metrics.getForSymbol(timeInZonesHR[idx]);
                        int dt = timeZone - intervalMetrics[0].getForSymbol(timeInZonesHR[idx]);
                        idx++;

                        // time and then +time
                        summary += QString("<td align=\"center\">%1</td>").arg(time_to_string(timeZone));

                        if (counter) summary += QString("<td align=\"center\">%1%2</td>")
                                                .arg(dt>0 ? "+" : "-")
                                                .arg(time_to_string(fabs(dt)));

                        else summary += "<td></td>";

                        summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

                    }
                    summary += "</tr>";
                    rows++; counter++;
                }

                // done
                summary += "</table>";
            }
        }


    } else { // DATE RANGE COMPARE

        // LETS FORMAT THE HTML
        summary = GCColor::css();
        summary += "<center>";

        // get metric details here ...
        RideMetricFactory &factory = RideMetricFactory::instance();

        //
        // TOTALS, AVERAGES, MAX, METRICS
        //
        for (int v = 0; v < columnNames.count(); ++v) {

            QString columnName;
            QStringList metricsList;

            switch (v) { // slightly different order when summarising for compare...
                         // we want the metrics towards the top of the screen as they
                         // are more important than the maximums (generally anyway)

                case 0: metricsList = totalColumn;
                        columnName = columnNames[0]; 
                        break;
                case 1: metricsList = metricColumn; columnName = columnNames[3]; break;
                case 2: metricsList = averageColumn; columnName = columnNames[1]; break;
                default: 
                case 3: metricsList = maximumColumn; columnName = columnNames[2]; break;
            }

            //
            // Repeat for each 'column' (now separate paragraphs)
            //
            summary += "<h3>" + columnName + "</h3>";

            // table of metrics
            summary += "<table align=\"center\" width=\"80%\" border=\"0\">";

            // first row is a row of headings
            summary += "<tr>";
            summary += "<td><b></b></td>"; // removed the text as its blinking obvious.. but left code in
                                             // case we ever come back here or use it for other things.
            summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

            foreach (QString symbol, metricsList) {
                const RideMetric *m = factory.rideMetric(symbol);

                QString name, units;
                if (!(m->units(context->athlete->useMetricUnits) == "seconds" || m->units(context->athlete->useMetricUnits) == tr("seconds")))
                    units = m->units(context->athlete->useMetricUnits);
                if (units != "") name = QString("%1 (%2)").arg(m->name()).arg(units);
                else name = QString("%1").arg(m->name());

                name = name.replace(QRegExp(tr("^(Average|Max) ")), ""); // average/max on average/max is dumb
                summary += "<td align=\"center\" colspan=2><b>" +  name + "</b></td>";
                summary += "<td>&nbsp;</td>"; // spacing
            }
            summary += "</tr>";

            // then one row for each interval
            int counter = 0;
            foreach (CompareDateRange dr, context->compareDateRanges) {

                // skip if not checked
                if (!dr.isChecked()) continue;

                // alternating shading
                if (counter%2) summary += "<tr bgcolor='" + altColor.name() + "'>";
                else summary += "<tr>";

                summary += "<td>" + dr.name + "</td>";
                summary += "<td bgcolor='" + bgColor.name() +"'>&nbsp;</td>"; // spacing

                foreach (QString symbol, metricsList) {

                    // the values ...
                    const RideMetric *m = factory.rideMetric(symbol);

                    // get value and convert if needed (use local context for units)
                    double value = SummaryMetrics::getAggregated(context, symbol, dr.metrics, QStringList(), false,
                                                  context->athlete->useMetricUnits, true).toDouble();

                    // use right precision
                    QString strValue = QString("%1").arg(value, 0, 'f', m->precision());

                    // or maybe its a duration (worry about local lang or translated)
                    if (m->units(true) == "seconds" || m->units(true) == tr("seconds"))
                        strValue = time_to_string((int)value);

                    summary += "<td align=\"center\">" + strValue + "</td>";

                    // delta to first entry
                    if (counter) {

                        // calculate me vs the original
                        double value0 = SummaryMetrics::getAggregated(context, symbol, 
                                                  context->compareDateRanges[0].metrics, QStringList(), false,
                                                  context->athlete->useMetricUnits, true).toDouble();

                        value -= value0; // delta

                        // use right precision
                        QString strValue = QString("%1%2").arg(value >= 0 ? "+" : "") // - sign added anyway
                                                        .arg(value, 0, 'f', m->precision());

                        // or maybe its a duration (worry about local lang or translated)
                        if (m->units(true) == "seconds" || m->units(true) == tr("seconds"))
                            strValue = QString(value >= 0 ? "+" : "-" ) + time_to_string(fabs(value));

                        summary += "<td align=\"center\">" + strValue + "</td>";
                    } else {
                        summary += "<td align=\"center\"></td>";
                    }
                    summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

                }
                summary += "</tr>";
                counter++;
            }
            summary += "</table>";
        }

        //
        // TIME IN POWER ZONES
        //
        if (context->athlete->zones()) { // use my zones

            // get from end if period
            int rangeidx = context->athlete->zones()->whichRange(QDate::currentDate()); // use current zone names et al
            if (rangeidx > -1) {

                // get the list of zones
                ZoneRange range = const_cast<Zones*>(context->athlete->zones())->getZoneRange(rangeidx);
                QList<ZoneInfo> zones = range.zones;

                // we've got a range and a count of zones so all is well
                // we need to throw up a table of time in zone for each interval
                summary += tr("<h3>Power Zones</h3>");
                summary += "<table align=\"center\" width=\"80%\" border=\"0\">";

                // lets get some headings
                summary += "<tr><td></td>"; // ne need to have a heading for the interval name
                summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

                foreach (ZoneInfo zone, zones) {
                    summary += QString("<td colspan=\"2\" align=\"center\"><b>%1 (%2)</b></td>").arg(zone.desc).arg(zone.name);
                    summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing
                }
                summary += "</tr>";

                // now the summary
                int counter = 0;
                foreach (CompareDateRange dr, context->compareDateRanges) {

                    // skip if not checked
                    if (!dr.isChecked()) continue;

                    if (counter%2) summary += "<tr bgcolor='" + altColor.name() + "'>";
                    else summary += "<tr>";

                    summary += "<td>" + dr.name + "</td>";
                    summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

                    int idx=0;
                    foreach (ZoneInfo zone, zones) {

                        int timeZone = SummaryMetrics::getAggregated(context, timeInZones[idx], dr.metrics, QStringList(), false,
                                                                     context->athlete->useMetricUnits, true).toInt();

                        int dt = timeZone - SummaryMetrics::getAggregated(context, timeInZones[idx], 
                                                                          context->compareDateRanges[0].metrics, QStringList(), false,
                                                                          context->athlete->useMetricUnits, true).toInt();
                        idx++;

                        // time and then +time
                        summary += QString("<td align=\"center\">%1</td>").arg(time_to_string(timeZone));

                        if (counter) summary += QString("<td align=\"center\">%1%2</td>")
                                                .arg(dt>0 ? "+" : "-")
                                                .arg(time_to_string(fabs(dt)));

                        else summary += "<td></td>";
                        summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

                    }
                    summary += "</tr>";
                    counter++;
                }

                // done
                summary += "</table>";
            }
        }

        //
        // TIME IN HR ZONES
        //
        if (context->athlete->hrZones()) { // use my zones

            // get from end if period
            int rangeidx = context->athlete->hrZones()->whichRange(QDate::currentDate()); // use current zone names et al
            if (rangeidx > -1) {

                // get the list of zones
                HrZoneRange range = const_cast<HrZones*>(context->athlete->hrZones())->getHrZoneRange(rangeidx);
                QList<HrZoneInfo> zones = range.zones;

                // we've got a range and a count of zones so all is well
                // we need to throw up a table of time in zone for each interval
                summary += tr("<h3>Heartrate Zones</h3>");
                summary += "<table align=\"center\" width=\"80%\" border=\"0\">";

                // lets get some headings
                summary += "<tr><td></td>"; // ne need to have a heading for the interval name
                summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

                foreach (HrZoneInfo zone, zones) {
                    summary += QString("<td colspan=\"2\" align=\"center\"><b>%1 (%2)</b></td>").arg(zone.desc).arg(zone.name);
                    summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing
                }
                summary += "</tr>";

                // now the summary
                int counter = 0;
                foreach (CompareDateRange dr, context->compareDateRanges) {

                    // skip if not checked
                    if (!dr.isChecked()) continue;

                    if (counter%2) summary += "<tr bgcolor='" + altColor.name() + "'>";
                    else summary += "<tr>";

                    summary += "<td>" + dr.name + "</td>";
                    summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

                    int idx=0;
                    foreach (HrZoneInfo zone, zones) {

                        int timeZone = SummaryMetrics::getAggregated(context, timeInZonesHR[idx], dr.metrics, QStringList(), false,
                                                                     context->athlete->useMetricUnits, true).toInt();

                        int dt = timeZone - SummaryMetrics::getAggregated(context, timeInZonesHR[idx], 
                                                                          context->compareDateRanges[0].metrics, QStringList(), false,
                                                                          context->athlete->useMetricUnits, true).toInt();
                        idx++;

                        // time and then +time
                        summary += QString("<td align=\"center\">%1</td>").arg(time_to_string(timeZone));

                        if (counter) summary += QString("<td align=\"center\">%1%2</td>")
                                                .arg(dt>0 ? "+" : "-")
                                                .arg(time_to_string(fabs(dt)));

                        else summary += "<td></td>";
                        summary += "<td bgcolor='" + bgColor.name() + "'>&nbsp;</td>"; // spacing

                    }
                    summary += "</tr>";
                    counter++;
                }

                // done
                summary += "</table>";
            }
        }

    }

    // add the usual disclaimers etc at the bottom
    summary += "<br><hr width=\"80%\">";

    // The extra <center> works around a bug in QT 4.3.1,
    // which will otherwise put the following above the <hr>.
    summary += tr("<br>BikeScore is a trademark of Dr. Philip "
        "Friere Skiba, PhysFarm Training Systems LLC");

    summary += tr("<br>FTP, TSS, NP and IF are trademarks of Peaksware LLC</center>");
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

    // wipe bests coz we need to refresh
    if (!ridesummary && bestsCache) {
        delete bestsCache;
        bestsCache=NULL;
    }

    if (useCustom) {
        data = context->athlete->metricDB->getAllMetricsFor(custom);
    } else if (useToToday) {

        DateRange use = myDateRange;
        QDate today = QDate::currentDate();
        if (use.to > today) use.to = today;
        data = context->athlete->metricDB->getAllMetricsFor(use);

    } else data = context->athlete->metricDB->getAllMetricsFor(myDateRange);

    refresh();
}
