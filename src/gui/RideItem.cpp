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

double RideItem::secsMovingOrPedaling()
{
    if (summary.isEmpty())
        htmlSummary();
    return secs_moving_or_pedaling;
}

double RideItem::totalWork()
{
    if (summary.isEmpty())
        htmlSummary();
    return total_work;
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
        RideMetricFactory::RideMetricIter metricIter(factory.metrics());
        while (metricIter.hasNext()) {
            metricIter.next();
            QString name = metricIter.key();
            RideMetric *metric = factory.newMetric(name);
            metric->compute(raw, zones, zone_range);
            metrics.insert(name, metric);
        }

        secs_moving_or_pedaling = 0.0;
        double secs_moving = 0.0;
        double total_watts = 0.0;
        double secs_watts = 0.0;
        double avg_watts = 0.0;
        double secs_hr = 0.0;
        double total_hr = 0.0;
        double secs_cad = 0.0;
        double total_cad = 0.0;
       
        QString intervals = "";
        unsigned last_interval = UINT_MAX;
        double int_watts_sum = 0.0;
        double int_hr_sum = 0.0;
        double int_cad_sum = 0.0;
        double int_mph_sum = 0.0;
        double int_secs_hr = 0.0;
        double int_max_power = 0.0;

        // for computing "Bike Score"
        // we're interested in a 25 second moving average
        double moving_avg_len = 25000 / raw->rec_int_ms;
        double moving_avg_watts[int(ceil(moving_avg_len))];
        double fourthPower_watts = 0.0;
        double fourth_counter = 0.0;
        double int_fourth_watts = 0.0;
        double int_fourth_counter = 0.0;
        double xpower = 0.0;
        double relative_intensity = 0.0;
        double bike_score = 0.0;

        double time_start, time_end, mile_start, mile_end, int_dur;

        int j = 0; // an index for the moving average function

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

                last_interval = point->interval;
                time_start = point->secs;
                mile_start = point->miles;
                int_secs_hr = secs_delta;
                int_dur = 0.0;
            }

            if ((point->mph > 0.0) || (point->cad > 0.0)) {
                secs_moving_or_pedaling += secs_delta;
                int_dur += secs_delta;
            }
            if (point->mph > 0.0)
                secs_moving += secs_delta;
            if (point->watts >= 0.0) {
                total_watts += point->watts * secs_delta;
                secs_watts += secs_delta;

                // 25 second moving average
                // populate the array in the first 25 seconds
                if (secs_watts <= 25)
                    moving_avg_watts[j] = point->watts;
                else {
                    // add the most recent value, and calculate average
                    moving_avg_watts[j] = point->watts;
                    double mov_avg = 0.0;
                    for(int jj = 0;jj < moving_avg_len; jj++)
                        mov_avg += moving_avg_watts[jj];
                    double current_fourth = pow(mov_avg / moving_avg_len, 4.0);
                    fourthPower_watts += current_fourth;
                    fourth_counter++;
                    int_fourth_watts += current_fourth;
                    int_fourth_counter++;
                }
                j = (j + 1) % ((int)(moving_avg_len));

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
                total_hr += point->hr * secs_delta;
                secs_hr += secs_delta;
                int_hr_sum += point->hr * secs_delta;
                int_secs_hr += secs_delta;
            }
            if (point->cad > 0) {
                total_cad += point->cad * secs_delta;
                secs_cad += secs_delta;
                int_cad_sum += point->cad * secs_delta;
            }
            if (point->mph >= 0) {
                int_mph_sum += point->mph * secs_delta;
            }

            mile_end = point->miles;
            time_end = point->secs + secs_delta;
        }
        summarize(intervals, last_interval,
                  mile_start, mile_end, int_watts_sum, 
                  int_hr_sum, int_cad_sum, int_mph_sum, 
                  int_secs_hr, int_max_power, int_dur);

        avg_watts = (secs_watts == 0.0) ? 0.0
            : round(total_watts / secs_watts);

        if (zones) {
           xpower =  pow(int_fourth_watts/int_fourth_counter,0.25);
           relative_intensity = xpower / ((float) zones->getFTP(zone_range));
           bike_score = ((raw->points.back()->secs / 60) / 60) 
               * (pow(relative_intensity, 2)) * 100.0;
        }
               
        total_work = total_watts / 1000.0;
                
        summary += "<p>";
        summary += "<table align=\"center\" width=\"90%\" border=0>";
        summary += "<tr><td align=\"center\"><h2>Totals</h2></td>";
        summary += "<td align=\"center\"><h2>Averages</h2></td></tr>";
        summary += "<tr><td>";
        summary += "<table align=\"center\" width=\"70%\" border=0>";
        summary += "<tr><td>Workout time:</td><td align=\"right\">" + 
            time_to_string(raw->points.back()->secs);
        summary += "<tr><td>Time riding:</td><td align=\"right\">" + 
            time_to_string(secs_moving_or_pedaling) + "</td></tr>";

        bool metric = (unit.toString() == "Metric");

        assert(metrics.contains("total_distance"));
        const RideMetric *totalDistanceMetric = metrics.value("total_distance");
        summary += ("<tr><td>Distance (" 
                    + totalDistanceMetric->units(metric) 
                    + "):</td><td align=\"right\">%1</td></tr>")
                   .arg(totalDistanceMetric->value(metric), 0, 'f', 1);
 
        summary += QString("<tr><td>Work (kJ):</td>"
                           "<td align=\"right\">%1</td></tr>")
            .arg((unsigned) round(total_work));
        summary += "</table></td><td>";
        summary += "<table align=\"center\" width=\"70%\" border=0>";

        if (unit.toString() == "Metric") {
             summary += QString("<tr><td>Speed (km/h):</td>"
                             "<td align=\"right\">%1</td></tr>")
             .arg(((secs_moving == 0.0) ? 0.0
                   : ((raw->points.back()->miles * 1.60934) 
                      / secs_moving * 3600.0)), 
                  0, 'f', 1);
        }
        else {
             summary += QString("<tr><td>Speed (mph):</td>"
                            "<td align=\"right\">%1</td></tr>")
             .arg(((secs_moving == 0.0) ? 0.0
                   : raw->points.back()->miles / secs_moving * 3600.0), 
                  0, 'f', 1);
        }
        summary += QString("<tr><td>Power (watts):</td>"
                           "<td align=\"right\">%1</td></tr>")
            .arg((unsigned) avg_watts);
        summary +=QString("<tr><td>Heart rate (bpm):</td>"
                          "<td align=\"right\">%1</td></tr>")
            .arg((unsigned) ((secs_hr == 0.0) ? 0.0
                             : round(total_hr / secs_hr)));
        summary += QString("<tr><td>Cadence (rpm):</td>"
                           "<td align=\"right\">%1</td></tr>")
            .arg((unsigned) ((secs_cad == 0.0) ? 0.0
                             : round(total_cad / secs_cad)));
        summary += "</table></td></tr>";
        summary += "</table>";
 
        if (xpower != 0) {
            summary += "<h2>Dr. Skiba's BikeScore</h2>";
            summary += "<table align=\"center\" width=\"40%\" ";
            summary += "cellspacing=0 border=0>";
            summary += QString("<tr><td>xPower:</td>"
                               "<td align=\"left\">%1</td></tr>")
                .arg((unsigned) xpower);
            summary += QString("<tr><td>Relative Intensity:</td>"
                               "<td align=\"left\">%1</td></tr>")
                .arg(relative_intensity, 0, 'f', 3);
            summary += QString("<tr><td>Bike Score: </td>"
                               "<td align=\"left\">%1</td></tr>")
                .arg(bike_score, 0, 'f', 2);
            summary += "</table>";
        }

        if (zones) {
            summary += "<h2>Power Zones</h2>";
            summary += zones->summarize(zone_range, time_in_zone, num_zones);
        }

        if (last_interval > 0) {
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
        summary += "</center>";
    }

    return summary;
}

