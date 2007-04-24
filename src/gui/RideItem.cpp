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
#include "RawFile.h"
#include "Settings.h"
#include <math.h>

static QString
time_to_string(double secs) {
    QString result;
    unsigned rounded = (unsigned) round(secs);
    bool needs_colon = false;
    if (rounded >= 3600) {
        result += QString("%1").arg(rounded / 3600);
        rounded %= 3600;
        needs_colon = true;
    }
    if (needs_colon || rounded >= 60) {
        if (needs_colon)
            result += ":";
        result += QString("%1").arg(rounded / 60, 2, 10, QLatin1Char('0'));
        rounded %= 60;
        needs_colon = true;
    }
    if (needs_colon)
        result += ":";
    result += QString("%1").arg(rounded, 2, 10, QLatin1Char('0'));
    return result;
}

RideItem::RideItem(QTreeWidgetItem *parent, int type, QString path, 
                   QString fileName, const QDateTime &dateTime) : 
    QTreeWidgetItem(parent, type), path(path),
    fileName(fileName), dateTime(dateTime)
{
    setText(0, dateTime.toString("ddd"));
    setText(1, dateTime.toString("MMM d, yyyy"));
    setText(2, dateTime.toString("h:mm AP"));
    setTextAlignment(1, Qt::AlignRight);
    setTextAlignment(2, Qt::AlignRight);
}

static void summarize(QString &intervals,
                      unsigned last_interval,
                      double time_start, double time_end,
                      double mile_start, double mile_end,
                      double &int_watts_sum,
                      double &int_hr_sum,
                      double &int_cad_sum,
                      double &int_mph_sum) 
{
    double dur = round(time_end - time_start);
    double len = mile_end - mile_start;
    double minutes = (int) (dur/60.0);
    double seconds = dur - (60 * minutes);
    double watts_avg = int_watts_sum / dur;
    double hr_avg = int_hr_sum / dur;
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
    intervals = intervals.arg(last_interval);
    intervals = intervals.arg(minutes, 0, 'f', 0);
    intervals = intervals.arg(seconds, 2, 'f', 0, QLatin1Char('0'));
    intervals = intervals.arg(len, 0, 'f', 1);
    intervals = intervals.arg(energy, 0, 'f', 0);
    intervals = intervals.arg(watts_avg, 0, 'f', 0);
    intervals = intervals.arg(hr_avg, 0, 'f', 0);
    intervals = intervals.arg(cad_avg, 0, 'f', 0);
    intervals = intervals.arg(mph_avg, 0, 'f', 1);

    int_watts_sum = 0.0;
    int_hr_sum = 0.0;
    int_cad_sum = 0.0;
    int_mph_sum = 0.0;
}

double RideItem::secsMovingOrPedaling()
{
    if (summary.isEmpty())
        htmlSummary();
    return secs_moving_or_pedaling;
}

double RideItem::totalDistance()
{
    if (summary.isEmpty())
        htmlSummary();
    return total_distance;
}

double RideItem::totalWork()
{
    if (summary.isEmpty())
        htmlSummary();
    return total_work;
}

QString 
RideItem::htmlSummary()
{
    if (summary.isEmpty()) {
        QFile file(path + "/" + fileName);
        QStringList errors;
        raw = RawFile::readFile(file, errors);
        summary = ("<p><center><h2>" 
                   + dateTime.toString("dddd MMMM d, yyyy, h:mm AP") 
                   + "</h2><p><h2>Summary</h2>");
        if (raw == NULL) {
            summary += "<p>Error: Can't read file.";
            return summary;
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

        double time_start, time_end, mile_start, mile_end;

        QListIterator<RawFilePoint*> i(raw->points);
        while (i.hasNext()) {
            RawFilePoint *point = i.next();

            if (point->interval != last_interval) {

                if (last_interval != UINT_MAX) {
                    summarize(intervals, last_interval, time_start, 
                              time_end, mile_start, mile_end, int_watts_sum, 
                              int_hr_sum, int_cad_sum, int_mph_sum);
                }

                last_interval = point->interval;
                time_start = point->secs;
                mile_start = point->miles;
            }

            double secs_delta = raw->rec_int_ms / 1000.0;
            if ((point->mph > 0.0) || (point->cad > 0.0))
                secs_moving_or_pedaling += secs_delta;
            if (point->mph > 0.0)
                secs_moving += secs_delta;
            if (point->watts >= 0.0) {
                total_watts += point->watts * secs_delta;
                secs_watts += secs_delta;
                int_watts_sum += point->watts * secs_delta;
            }
            if (point->hr > 0) {
                total_hr += point->hr * secs_delta;
                secs_hr += secs_delta;
                int_hr_sum += point->hr * secs_delta;
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
            time_end = point->secs;
        }

        summarize(intervals, last_interval, time_start, 
                  time_end, mile_start, mile_end, int_watts_sum, 
                  int_hr_sum, int_cad_sum, int_mph_sum);

        avg_watts = (secs_watts == 0.0) ? 0.0
            : round(total_watts / secs_watts);

        total_distance = raw->points.back()->miles;
        total_work = total_watts / 1000.0;

        summary += "<p><table align=\"center\" width=\"60%\" border=0>";
        summary += "<tr><td>Total workout time:</td><td align=\"right\">" + 
            time_to_string(raw->points.back()->secs);
        summary += "<tr><td>Total time riding:</td><td align=\"right\">" + 
            time_to_string(secs_moving_or_pedaling) + "</td></tr>";
        summary += QString("<tr><td>Total distance (miles):</td>"
                           "<td align=\"right\">%1</td></tr>")
            .arg(total_distance, 0, 'f', 1);
        summary += QString("<tr><td>Total work (kJ):</td>"
                           "<td align=\"right\">%1</td></tr>")
            .arg((unsigned) round(total_work));
        summary += QString("<tr><td>Average speed (mph):</td>"
                           "<td align=\"right\">%1</td></tr>")
            .arg(((secs_moving == 0.0) ? 0.0
                  : raw->points.back()->miles / secs_moving * 3600.0), 
                 0, 'f', 1);
        summary += QString("<tr><td>Average power (watts):</td>"
                           "<td align=\"right\">%1</td></tr>")
            .arg((unsigned) avg_watts);
        summary +=QString("<tr><td>Average heart rate (bpm):</td>"
                          "<td align=\"right\">%1</td></tr>")
            .arg((unsigned) ((secs_hr == 0.0) ? 0.0
                             : round(total_hr / secs_hr)));
        summary += QString("<tr><td>Average cadence (rpm):</td>"
                           "<td align=\"right\">%1</td></tr>")
            .arg((unsigned) ((secs_cad == 0.0) ? 0.0
                             : round(total_cad / secs_cad)));
        summary += "</table>";

        if (last_interval > 0) {
            summary += "<p><h2>Intervals</h2>\n<p>\n";
            summary += "<table align=\"center\" width=\"90%\" ";
            summary += "cellspacing=0 border=0><tr>";
            summary += "<td align=\"center\">Interval</td>";
            summary += "<td align=\"center\"></td>";
            summary += "<td align=\"center\">Distance</td>";
            summary += "<td align=\"center\">Work</td>";
            summary += "<td align=\"center\">Avg Power</td>";
            summary += "<td align=\"center\">Avg HR</td>";
            summary += "<td align=\"center\">Avg Cadence</td>";
            summary += "<td align=\"center\">Avg Speed</td>";
            summary += "</tr><tr>";
            summary += "<td align=\"center\">Number</td>";
            summary += "<td align=\"center\">Duration</td>";
            summary += "<td align=\"center\">(miles)</td>";
            summary += "<td align=\"center\">(kJ)</td>";
            summary += "<td align=\"center\">(watts)</td>";
            summary += "<td align=\"center\">(bpm)</td>";
            summary += "<td align=\"center\">(rpm)</td>";
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


