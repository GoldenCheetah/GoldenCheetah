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

QString 
RideItem::htmlSummary()
{
    if (summary.isEmpty()) {
        QFile file(path + "/" + fileName);
        QStringList errors;
        raw = RawFile::readFile(file, errors);
        summary = ("<center><h2>" 
                   + dateTime.toString("dddd MMMM d, yyyy, h:mm AP") 
                   + "</h2></center>");
        if (raw == NULL) {
            summary += "<p>Error: Can't read file.";
        }
        else if (errors.empty()) {
            double secs_moving = 0.0;
            double total_watts = 0.0;
            double secs_watts = 0.0;
            double secs_hr = 0.0;
            double total_hr = 0.0;
            double secs_cad = 0.0;
            double total_cad = 0.0;
            double last_secs = 0.0;
            QListIterator<RawFilePoint*> i(raw->points);
            while (i.hasNext()) {
                RawFilePoint *point = i.next();
                double secs_delta = point->secs - last_secs;
                if (point->mph > 0.0)
                    secs_moving += secs_delta;
                if (point->watts >= 0.0) {
                    total_watts += point->watts * secs_delta;
                    secs_watts += secs_delta;
                }
                if (point->hr > 0) {
                    total_hr += point->hr * secs_delta;
                    secs_hr += secs_delta;
                }
                if (point->cad > 0) {
                    total_cad += point->cad * secs_delta;
                    secs_cad += secs_delta;
                }
                last_secs = point->secs;
            }
            summary += "<br><table width=\"80%\" "
                "align=\"center\" border=0>";
            summary += "<tr><td>Total workout time:</td><td align=\"right\">" + 
                time_to_string(raw->points.back()->secs);
            summary += "<tr><td>Total time riding:</td><td align=\"right\">" + 
                time_to_string(secs_moving) + "</td></tr>";
            summary += QString("<tr><td>Total distance:</td>"
                               "<td align=\"right\">%1</td></tr>")
                .arg(raw->points.back()->miles, 0, 'f', 1);
            summary += QString("<tr><td>Average speed:</td>"
                               "<td align=\"right\">%1</td></tr>")
                .arg(raw->points.back()->miles / secs_moving * 3600.0, 
                     0, 'f', 1);
            summary += QString("<tr><td>Average power:</td>"
                               "<td align=\"right\">%1</td></tr>")
                .arg((unsigned) round(total_watts / secs_watts));
            summary +=QString("<tr><td>Average heart rate:</td>"
                              "<td align=\"right\">%1</td></tr>")
                .arg((unsigned) round(total_hr / secs_hr));
            summary += QString("<tr><td>Average cadence:</td>"
                               "<td align=\"right\">%1</td></tr>")
                .arg((unsigned) ((secs_cad == 0) ? 0
                                 : round(total_cad / secs_cad)));
            summary += "</table>";
        }
        else {
            summary += "<br>Errors reading file:<ul>";
            QStringListIterator i(errors); 
            while(i.hasNext())
                summary += " <li>" + i.next();
            summary += "</ul>";
        }
    }
    return summary;
}


