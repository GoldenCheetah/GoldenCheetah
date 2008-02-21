/* 
 * $Id: RideItem.cpp,v 1.3 2006/07/09 15:30:34 srhea Exp $
 *
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#include "RideItem.h"
#include "RideMetric.h"
#include "RawFile.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Zones.h"
#include <assert.h>
#include <math.h>
#include <QtXml/QtXml>

RideItem::RideItem(QTreeWidgetItem *parent, int type,
                   QString path, QString fileName, const QDateTime &dateTime, 
                   const Zones *zones, QString notesFileName) : 
    QTreeWidgetItem(parent, type), path(path), fileName(fileName), 
    dateTime(dateTime), zones(zones), notesFileName(notesFileName)
{
    setText(0, dateTime.toString("ddd"));
    setText(1, dateTime.toString("MMM d, yyyy"));
    setText(2, dateTime.toString("h:mm AP"));
    setTextAlignment(1, Qt::AlignRight);
    setTextAlignment(2, Qt::AlignRight);

    time_in_zone = NULL;
    num_zones = -1;
    zone_range = -1;
}

RideItem::~RideItem()
{
    MetricIter i(metrics);
    while (i.hasNext()) {
        i.next();
        delete i.value();
    }
}

static void summarize(QString &intervals,
                      unsigned last_interval,
                      double mile_start, double mile_end,
                      double &int_watts_sum,
                      double &int_hr_sum,
                      double &int_cad_sum,
                      double &int_mph_sum,
                      double &int_secs_hr,
                      double &int_max_power,
                      double int_dur) 
{
    double dur = int_dur;
    double len = mile_end - mile_start;
    double minutes = (int) (dur/60.0);
    double seconds = dur - (60 * minutes);
    double watts_avg = int_watts_sum / dur;
    double hr_avg = int_hr_sum / int_secs_hr;
    double cad_avg = int_cad_sum / dur;
    double mph_avg = int_mph_sum / dur;
    double energy = int_watts_sum / 1000.0; // watts_avg / 1000.0 * dur;

    intervals += "<tr><td align=\"center\">%1</td>";
    intervals += "<td align=\"center\">%2:%3</td>";
    intervals += "<td align=\"center\">%4</td>";
    intervals += "<td align=\"center\">%5</td>";
    intervals += "<td align=\"center\">%6</td>";
    intervals += "<td align=\"center\">%7</td>";
    intervals += "<td align=\"center\">%8</td>";
    intervals += "<td align=\"center\">%9</td>";
    intervals += "<td align=\"center\">%10</td>";
    intervals = intervals.arg(last_interval);
    intervals = intervals.arg(minutes, 0, 'f', 0);
    intervals = intervals.arg(seconds, 2, 'f', 0, QLatin1Char('0'));
    intervals = intervals.arg(len, 0, 'f', 1);
    intervals = intervals.arg(energy, 0, 'f', 0);
    intervals = intervals.arg(int_max_power, 0, 'f', 0);
    intervals = intervals.arg(watts_avg, 0, 'f', 0);
    intervals = intervals.arg(hr_avg, 0, 'f', 0);
    intervals = intervals.arg(cad_avg, 0, 'f', 0);
    QSettings settings(GC_SETTINGS_CO, GC_SETTINGS_APP);
    QVariant unit = settings.value(GC_UNIT);
    if(unit.toString() == "Metric")
        intervals = intervals.arg(mph_avg * 1.60934, 0, 'f', 1);
    else
        intervals = intervals.arg(mph_avg, 0, 'f', 1);

    int_watts_sum = 0.0;
    int_hr_sum = 0.0;
    int_cad_sum = 0.0;
    int_mph_sum = 0.0;
    int_max_power = 0.0;
}

int RideItem::zoneRange() 
{
    if (summary.isEmpty())
        htmlSummary();
    return zone_range;
}

int RideItem::numZones() 
{
    if (summary.isEmpty())
        htmlSummary();
    assert(zone_range >= 0);
    return num_zones;
}

double RideItem::timeInZone(int zone)
{
    if (summary.isEmpty())
        htmlSummary();
    if (!raw)
        return 0.0;
    assert(zone_range >= 0);
    assert(zone < num_zones);
    return time_in_zone[zone];
}

static const char *metricsXml =  
    "<metrics>\n"
    "  <metric_group name=\"Totals\">\n"
    "    <metric name=\"workout_time\" display_name=\"Workout time\"\n"
    "            precision=\"0\"/>\n"    
    "    <metric name=\"time_riding\" display_name=\"Time riding\"\n"
    "            precision=\"0\"/>\n"    
    "    <metric name=\"total_distance\" display_name=\"Distance\"\n"
    "            precision=\"1\"/>\n"    
    "    <metric name=\"total_work\" display_name=\"Work\"\n"
    "            precision=\"0\"/>\n"
    "  </metric_group>\n"
    "  <metric_group name=\"Averages\">\n"
    "    <metric name=\"average_speed\" display_name=\"Speed\"\n"
    "            precision=\"1\"/>\n"
    "    <metric name=\"average_power\" display_name=\"Power\"\n"
    "            precision=\"0\"/>\n"
    "    <metric name=\"average_hr\" display_name=\"Heart rate\"\n"
    "            precision=\"0\"/>\n"
    "    <metric name=\"average_cad\" display_name=\"Cadence\"\n"
    "            precision=\"0\"/>\n"
    "  </metric_group>\n"
    "  <metric_group name=\"BikeScore(TM)\" note=\"BikeScore is a trademark "
    "      of Dr. Philip Friere Skiba, PhysFarm Training Systems LLC\">\n"
    "    <metric name=\"skiba_xpower\" display_name=\"xPower\"\n"
    "            precision=\"0\"/>\n"
    "    <metric name=\"skiba_relative_intensity\"\n"
    "            display_name=\"Relative Intensity\" precision=\"3\"/>\n"
    "    <metric name=\"skiba_bike_score\" display_name=\"BikeScore\"\n"
    "            precision=\"0\"/>\n"
    "  </metric_group>\n"
    "</metrics>\n";

QString 
RideItem::htmlSummary()
{
    if (summary.isEmpty()) {
        QFile file(path + "/" + fileName);
        QStringList errors;
        raw = RawFile::readFile(file, errors);
        if (!raw) {
            summary = ("<p>Couldn't read file \"" + 
                       file.fileName() + "\":");
            QListIterator<QString> i(errors);
            while (i.hasNext())
                summary += "<br>" + i.next();
            return summary;
        }
        summary = ("<p><center><h2>" 
                   + dateTime.toString("dddd MMMM d, yyyy, h:mm AP") 
                   + "</h2><p><h2>Summary</h2>");
        if (raw == NULL) {
            summary += "<p>Error: Can't read file.";
            return summary;
        }

        if (zones) {
            zone_range = zones->whichRange(dateTime.date());
            if (zone_range >= 0) {
                num_zones = zones->numZones(zone_range);
                time_in_zone = new double[num_zones];
                for (int i = 0; i < num_zones; ++i)
                    time_in_zone[i] = 0.0;
            }
        }
        QSettings settings(GC_SETTINGS_CO, GC_SETTINGS_APP);
        QVariant unit = settings.value(GC_UNIT);
 
        const RideMetricFactory &factory = RideMetricFactory::instance();
        QSet<QString> todo;
        for (int i = 0; i < factory.metricCount(); ++i)
            todo.insert(factory.metricName(i));

        while (!todo.empty()) {
            QMutableSetIterator<QString> i(todo);
later:
            while (i.hasNext()) {
                const QString &name = i.next();
                const QVector<QString> &deps = factory.dependencies(name);
                for (int j = 0; j < deps.size(); ++j)
                    if (!metrics.contains(deps[j]))
                        goto later;
                RideMetric *metric = factory.newMetric(name);
                metric->compute(raw, zones, zone_range, metrics);
                metrics.insert(name, metric);
                i.remove();
            }
        }

        double secs_watts = 0.0;
       
        QString intervals = "";
        int interval_count = 0;
        unsigned last_interval = UINT_MAX;
        double int_watts_sum = 0.0;
        double int_hr_sum = 0.0;
        double int_cad_sum = 0.0;
        double int_mph_sum = 0.0;
        double int_secs_hr = 0.0;
        double int_max_power = 0.0;

        double time_start, time_end, mile_start, mile_end, int_dur;

        QListIterator<RawFilePoint*> i(raw->points);
        while (i.hasNext()) {
            RawFilePoint *point = i.next();

            double secs_delta = raw->rec_int_ms / 1000.0;

            if (point->interval != last_interval) {

                if (last_interval != UINT_MAX) {
                    summarize(intervals, last_interval,
                              mile_start, mile_end, int_watts_sum, 
                              int_hr_sum, int_cad_sum, int_mph_sum, 
                              int_secs_hr, int_max_power, int_dur);
                }
                interval_count++;
                last_interval = point->interval;
                time_start = point->secs;
                mile_start = point->miles;
                int_secs_hr = secs_delta;
                int_dur = 0.0;
            }

            if ((point->mph > 0.0) || (point->cad > 0.0)) {
                int_dur += secs_delta;
            }
            if (point->watts >= 0.0) {
                secs_watts += secs_delta;
                int_watts_sum += point->watts * secs_delta;
                if (point->watts > int_max_power)
                    int_max_power = point->watts;
                if (zones) {
                    int zone = zones->whichZone(zone_range, point->watts);
                    if (zone >= 0)
                        time_in_zone[zone] += secs_delta;
                }
            }
            if (point->hr > 0) {
                int_hr_sum += point->hr * secs_delta;
                int_secs_hr += secs_delta;
            }
            if (point->cad > 0)
                int_cad_sum += point->cad * secs_delta;
            if (point->mph >= 0)
                int_mph_sum += point->mph * secs_delta;

            mile_end = point->miles;
            time_end = point->secs + secs_delta;
        }
        summarize(intervals, last_interval,
                  mile_start, mile_end, int_watts_sum, 
                  int_hr_sum, int_cad_sum, int_mph_sum, 
                  int_secs_hr, int_max_power, int_dur);

        summary += "<p>";

        bool metricUnits = (unit.toString() == "Metric");

        QDomDocument doc;
        {
            QString err;
            int errLine, errCol;
            if (!doc.setContent(QString(metricsXml), &err, &errLine, &errCol)){
                fprintf(stderr, "error: %s, line %d, col %d\n",
                        err.toAscii().constData(), errLine, errCol);
                assert(false);
            }
        }

        QString noteString = "";
        QString stars;
        QDomNodeList groups = doc.elementsByTagName("metric_group");
        for (int groupNum = 0; groupNum < groups.size(); ++groupNum) {
            QDomElement group = groups.at(groupNum).toElement();
            assert(!group.isNull());
            QString groupName = group.attribute("name");
            QString groupNote = group.attribute("note");
            assert(groupName.length() > 0);
            if (groupNum % 2 == 0)
                summary += "<table border=0 cellspacing=10><tr>";
            summary += "<td align=\"center\" width=\"45%\"><table>"
                "<tr><td align=\"center\" colspan=2><h2>%1</h2></td></tr>";
            if (groupNote.length() > 0) {
                stars += "*";
                summary = summary.arg(groupName + stars);
                noteString += "<br>" + stars + " " + groupNote;
            }
            else {
                summary = summary.arg(groupName);
            }
            QDomNodeList metricsList = group.childNodes();
            for (int i = 0; i < metricsList.size(); ++i) {
                QDomElement metric = metricsList.at(i).toElement();
                QString name = metric.attribute("name");
                QString displayName = metric.attribute("display_name");
                int precision = metric.attribute("precision", "0").toInt();
                assert(name.length() > 0);
                assert(displayName.length() > 0);
                const RideMetric *m = metrics.value(name);
                assert(m);
                if (m->units(metricUnits) == "seconds") {
                    QString s("<tr><td>%1:</td><td "
                              "align=\"right\">%2</td></tr>");
                    s = s.arg(displayName);
                    s = s.arg(time_to_string(m->value(metricUnits)));
                    summary += s;
                }
                else {
                    QString s = "<tr><td>" + displayName;
                    if (m->units(metricUnits) != "")
                        s += " (" + m->units(metricUnits) + ")";
                    s += ":</td><td align=\"right\">%1</td></tr>";
                    if (precision == 0)
                        s = s.arg((unsigned) round(m->value(metricUnits)));
                    else
                        s = s.arg(m->value(metricUnits), 0, 'f', precision);
                    summary += s;
                }
            }
            summary += "</table></td>";
            if ((groupNum % 2 == 1) || (groupNum == groups.size() - 1))
                summary += "</tr></table>";
        }

        if (zones) {
            summary += "<h2>Power Zones</h2>";
            summary += zones->summarize(zone_range, time_in_zone, num_zones);
        }

        // TODO: Ergomo uses non-consecutive interval numbers.
        // Seems to use 0 when not in an interval
        // and an integer < 30 when in an interval.
        // We'll need to create a counter for the intervals
        // rather than relying on the final data point's interval number.
        if (interval_count > 1) { 
            summary += "<p><h2>Intervals</h2>\n<p>\n";
            summary += "<table align=\"center\" width=\"90%\" ";
            summary += "cellspacing=0 border=0><tr>";
            summary += "<td align=\"center\">Interval</td>";
            summary += "<td align=\"center\"></td>";
            summary += "<td align=\"center\">Distance</td>";
            summary += "<td align=\"center\">Work</td>";
            summary += "<td align=\"center\">Max Power</td>";
            summary += "<td align=\"center\">Avg Power</td>";
            summary += "<td align=\"center\">Avg HR</td>";
            summary += "<td align=\"center\">Avg Cadence</td>";
            summary += "<td align=\"center\">Avg Speed</td>";
            summary += "</tr><tr>";
            summary += "<td align=\"center\">Number</td>";
            summary += "<td align=\"center\">Duration</td>";
            if(unit.toString() == "Metric")
                summary += "<td align=\"center\">(km)</td>";
            else
                summary += "<td align=\"center\">(miles)</td>";
            summary += "<td align=\"center\">(kJ)</td>";
            summary += "<td align=\"center\">(watts)</td>";
            summary += "<td align=\"center\">(watts)</td>";
            summary += "<td align=\"center\">(bpm)</td>";
            summary += "<td align=\"center\">(rpm)</td>";
            if(unit.toString() == "Metric")
                summary += "<td align=\"center\">(km/h)</td>";
            else
                summary += "<td align=\"center\">(mph)</td>";
            summary += "</tr>";
            summary += intervals;
            summary += "</table>";
        }

        if (!errors.empty()) {
            summary += "<p><h2>Errors reading file:</h2><ul>";
            QStringListIterator i(errors); 
            while(i.hasNext())
                summary += " <li>" + i.next();
            summary += "</ul>";
        }
        if (noteString.length() > 0)
            summary += "<br><hr width=\"80%\">" + noteString;
        summary += "</center>";
    }

    return summary;
}

