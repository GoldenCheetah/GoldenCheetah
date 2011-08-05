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
#include "HrZones.h"
#include <QtGui>
#include <QtXml/QtXml>
#include <assert.h>
#include <math.h>

RideSummaryWindow::RideSummaryWindow(MainWindow *mainWindow) :
    QWidget(mainWindow), mainWindow(mainWindow)
{
    QVBoxLayout *vlayout = new QVBoxLayout;
    rideSummary = new QWebView(this);
    rideSummary->setContentsMargins(0,0,0,0);
    rideSummary->page()->view()->setContentsMargins(0,0,0,0);
    rideSummary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    rideSummary->setAcceptDrops(false);

    QFont defaultFont; // mainwindow sets up the defaults.. we need to apply
    rideSummary->settings()->setFontSize(QWebSettings::DefaultFontSize, defaultFont.pointSize()+1);
    rideSummary->settings()->setFontFamily(QWebSettings::StandardFont, defaultFont.family());

    vlayout->addWidget(rideSummary);

    connect(mainWindow, SIGNAL(rideSelected()), this, SLOT(refresh()));
    connect(mainWindow, SIGNAL(zonesChanged()), this, SLOT(refresh()));
    connect(mainWindow, SIGNAL(intervalsChanged()), this, SLOT(refresh()));
    setLayout(vlayout);
}

void
RideSummaryWindow::refresh()
{
    // XXX: activeTab is never equaly to RideSummaryWindow right now because
    // it's wrapped in the summarySplitter in MainWindow.
    if (!mainWindow->rideItem()) {
	    rideSummary->page()->mainFrame()->setHtml("");
        return;
    }
    rideSummary->page()->mainFrame()->setHtml(htmlSummary());
}

QString
RideSummaryWindow::htmlSummary() const
{
    QString summary("");

    RideItem *rideItem = mainWindow->rideItem();
    RideFile *ride = rideItem->ride();

    // ridefile read errors?
    if (!ride) {
        summary = tr("<p>Couldn't read file \"");
        summary += rideItem->fileName + "\":";
        QListIterator<QString> i(mainWindow->rideItem()->errors());
        while (i.hasNext())
            summary += "<br>" + i.next();
        return summary;
    }

    summary = ("<p><center><h2>"
               + rideItem->dateTime.toString(tr("dddd MMMM d, yyyy, h:mm AP"))
               + "</h2><h3>" + tr("Device Type: ") + ride->deviceType() + "</h3>");

    rideItem->computeMetrics();

    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    QVariant unit = settings->value(GC_UNIT);

    summary += "<p>";

    bool metricUnits = (unit.toString() == "Metric");

    const int columns = 3;
    QString columnNames[] = { tr("Totals"), tr("Averages"), tr("Metrics*") };
    const char *totalColumn[] = {
        "workout_time",
        "time_riding",
        "total_distance",
        "total_work",
        "elevation_gain",
        NULL
    };

    const char *averageColumn[] = {
        "average_speed",
        "average_power",
        "average_hr",
        "average_cad",
        NULL
    };

    QString s;

    if (settings->contains(GC_SETTINGS_SUMMARY_METRICS))
        s = settings->value(GC_SETTINGS_SUMMARY_METRICS).toString();
    else
        s = GC_SETTINGS_SUMMARY_METRICS_DEFAULT;
    QStringList metricColumnList = s.split(",");

    char **metricColumnTmp;
    // Copy QStringList to char **
    metricColumnTmp = new char*[metricColumnList.size() + 1];
    for (int i = 0; i < metricColumnList.size(); i++) {
        metricColumnTmp[i] = new char[strlen(metricColumnList.at(i).toStdString().c_str())+1];
        memcpy(metricColumnTmp[i], metricColumnList.at(i).toStdString().c_str(), strlen(metricColumnList.at(i).toStdString().c_str())+1);
    }
    metricColumnTmp[metricColumnList.size()] = NULL;
    char const **metricColumn = (const char**)metricColumnTmp;

    /*const char *metricColumn[] = {
        "skiba_xpower",
        "skiba_relative_intensity",
        "skiba_bike_score",
        "daniels_points",
        "daniels_equivalent_power",
        "trimp_points",
        "aerobic_decoupling",
        NULL
    };*/

    summary += "<table border=0 cellspacing=10><tr>";
    for (int i = 0; i < columns; ++i) {
        summary += "<td align=\"center\" valign=\"top\" width=\"%1%\"><table>"
            "<tr><td align=\"center\" colspan=2><h3>%2</h3></td></tr>";
        summary = summary.arg(90 / columns);
        summary = summary.arg(columnNames[i]);
        const char **metricsList;
        switch (i) {
            case 0: metricsList = totalColumn; break;
            case 1: metricsList = averageColumn; break;
            case 2: metricsList = metricColumn; break;
            default: assert(false);
        }
        for (int j = 0;; ++j) {
            const char *symbol = metricsList[j];
            if (!symbol) break;

            RideMetricPtr m = rideItem->metrics.value(symbol);

            if (m) { // only if the metric still exists (ride metric forwards compatibility)
                QString name = m->name().replace(QRegExp(tr("^Average ")), "");
                if (m->units(metricUnits) == "seconds" || m->units(metricUnits) == tr("seconds")) {
                    QString s("<tr><td>%1:</td><td "
                            "align=\"right\">%2</td></tr>");
                    s = s.arg(name);
                    s = s.arg(time_to_string(m->value(metricUnits)));
                    summary += s;

                } else {
                    QString s = "<tr><td>" + name;
                    if (m->units(metricUnits) != "")
                        s += " (" + m->units(metricUnits) + ")";
                    s += ":</td><td align=\"right\">%1</td></tr>";
                    if (m->precision() == 0)
                        s = s.arg((unsigned) round(m->value(metricUnits)));
                    else
                        s = s.arg(m->value(metricUnits), 0, 'f', m->precision());
                    summary += s;
                }
            }
        }
        summary += "</table></td>";
    }
    summary += "</tr></table>";

    if (rideItem->numZones() > 0) {
        QVector<double> time_in_zone(rideItem->numZones());
        for (int i = 0; i < rideItem->numZones(); ++i)
            time_in_zone[i] = rideItem->timeInZone(i);
        summary += tr("<h2>Power Zones</h2>");
        summary += mainWindow->zones()->summarize(rideItem->zoneRange(), time_in_zone);
    }

    if (rideItem->numHrZones() > 0) {
        QVector<double> time_in_hr_zone(rideItem->numHrZones());
        for (int i = 0; i < rideItem->numHrZones(); ++i)
            time_in_hr_zone[i] = rideItem->timeInHrZone(i);
        summary += tr("<h2>Hr Zones</h2>");
        summary += mainWindow->hrZones()->summarize(rideItem->hrZoneRange(), time_in_hr_zone);
    }

    if (ride->intervals().size() > 0) {
        bool firstRow = true;
        QString s;
        boost::shared_ptr<QSettings> settings = GetApplicationSettings();
        if (settings->contains(GC_SETTINGS_INTERVAL_METRICS))
            s = settings->value(GC_SETTINGS_INTERVAL_METRICS).toString();
        else
            s = GC_SETTINGS_INTERVAL_METRICS_DEFAULT;
        QStringList intervalMetrics = s.split(",");
        summary += "<p><h2>"+tr("Intervals")+"</h2>\n<p>\n";
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
                              p->watts, p->alt, p->lon, p->lat, p->headwind, 0);
            }
            if (f.dataPoints().size() == 0) {
                // Interval empty, do not compute any metrics
                continue;
            }

            QHash<QString,RideMetricPtr> metrics =
                RideMetric::computeMetrics(&f, mainWindow->zones(), mainWindow->hrZones(), intervalMetrics);
            if (firstRow) {
                summary += "<tr>";
                summary += "<td align=\"center\" valign=\"bottom\">Interval Name</td>";
                foreach (QString symbol, intervalMetrics) {
                    RideMetricPtr m = metrics.value(symbol);
                    if (!m) continue;
                    summary += "<td align=\"center\" valign=\"bottom\">" + m->name();
                    if (m->units(metricUnits) == "seconds" || m->units(metricUnits) == tr("seconds"))
                        ; // don't do anything
                    else if (m->units(metricUnits).size() > 0)
                        summary += " (" + m->units(metricUnits) + ")";
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
                if (m->units(metricUnits) == "seconds" || m->units(metricUnits) == tr("seconds"))
                    summary += s.arg(time_to_string(m->value(metricUnits)));
                else
                    summary += s.arg(m->value(metricUnits), 0, 'f', m->precision());
            }
            summary += "</tr>";
        }
        summary += "</table>";
    }

    if (!rideItem->errors().empty()) {
        summary += tr("<p><h2>Errors reading file:</h2><ul>");
        QStringListIterator i(rideItem->errors());
        while(i.hasNext())
            summary += " <li>" + i.next();
        summary += "</ul>";
    }
    summary += "<br><hr width=\"80%\">";

    // The extra <center> works around a bug in QT 4.3.1,
    // which will otherwise put the following above the <hr>.
    summary += "<br>BikeScore is a trademark of Dr. Philip "
        "Friere Skiba, PhysFarm Training Systems LLC</center>";

    return summary;
}

