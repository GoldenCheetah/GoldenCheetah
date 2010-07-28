/* 
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net)
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

#include "TimeUtils.h"
#include <math.h>
#include <QRegExpValidator>

QString time_to_string(double secs) 
{
    QString result;
    unsigned rounded = static_cast<unsigned>(round(secs));
    bool needs_colon = false;
    if (rounded >= 3600) {
        result += QString("%1").arg(rounded / 3600);
        rounded %= 3600;
        needs_colon = true;
    }
    if (needs_colon)
        result += ":";
    result += QString("%1").arg(rounded / 60, 2, 10, QLatin1Char('0'));
    rounded %= 60;
    result += ":";
    result += QString("%1").arg(rounded, 2, 10, QLatin1Char('0'));
    return result;
}

double str_to_interval(QString s)
{
    QRegExp rx("(\\d+\\s*h)?\\s*(\\d{1,2}\\s*m)?\\s*(\\d{1,2})(\\.\\d+)?\\s*s");
    rx.indexIn(s);
    QString hour = rx.cap(1);
    QString min = rx.cap(2);
    QString sec = rx.cap(3) + rx.cap(4);
    hour.chop(1);
    min.chop(1);
    return 3600.0 * hour.toUInt() + 60.0 * min.toUInt() + sec.toDouble();
}

QString interval_to_str(double secs) 
{
    if (secs < 60.0)
        return QString("%1s").arg(secs, 0, 'f', 2, QLatin1Char('0'));
    QString result;
    unsigned rounded = static_cast<unsigned>(round(secs));
    bool needs_colon = false;
    if (rounded >= 3600) {
        result += QString("%1h").arg(rounded / 3600);
        rounded %= 3600;
        needs_colon = true;
    }
    if (needs_colon || rounded >= 60) {
        if (needs_colon)
            result += " ";
        result += QString("%1m").arg(rounded / 60, 2, 10, QLatin1Char('0'));
        rounded %= 60;
        needs_colon = true;
    }
    if (needs_colon)
        result += " ";
    result += QString("%1s").arg(rounded, 2, 10, QLatin1Char('0'));
    return result;
}

QDateTime convertToLocalTime(QString timestamp)
{
    //check if the last character is Z designating the timezone to be UTC
    if (timestamp[timestamp.size()-1].toLower()=='z')
    {
        QDateTime datetime = QDateTime::fromString(timestamp, Qt::ISODate);
        datetime.setTimeSpec(Qt::UTC);
        datetime=datetime.toLocalTime();
        return datetime;
    }
    //otherwise assume the timestamp is already in local time and simply convert it
    //something to add here would be a handler for explicitly set timezones
    //ex: 2002-05-30T09:30:10+06:00
    //see http://www.w3schools.com/Schema/schema_dtypes_date.asp
    //for more on this date format
    else
    {
        return QDateTime::fromString(timestamp);
    }
}

