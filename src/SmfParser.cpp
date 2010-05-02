/*
 * Copyright (c) 2010 Frank Zschockelt (rox_sigma@freakysoft.de
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

#include <QString>

#include "SmfParser.h"
#include "TimeUtils.h"
#include "Units.h"

SmfParser::SmfParser (RideFile* rideFile)
   : rideFile(rideFile)
{
}

bool
SmfParser::startElement( const QString&, const QString&,
			 const QString& qName,
			 const QXmlAttributes& qAttributes)
{
    buffer.clear();
    (void)qName;
    (void)qAttributes;

    return TRUE;
}

bool
SmfParser::endElement( const QString&, const QString&, const QString& qName)
{
    if (qName == "Datum")
    {
        start_time.setDate(QDate::fromString(buffer, "dd.MM.yy").addYears(100));
    }
    else if (qName == "Einheit")
    {
        imperial = (buffer == "mph");
    }
    else if (qName == "Uhrzeit")
    {
        start_time.setTime(QTime::fromString(buffer, "hh:mm"));
	rideFile->setStartTime(start_time);
    }
    else if (qName == "DurchschnittHR")
    {
        QMap<QString, QString> avg_hr;
        avg_hr.insert("value", buffer);
        rideFile->metricOverrides.insert("average_hr", avg_hr);
    }
    else if (qName == "MaximalHR")
    {
        QMap<QString, QString> max_hr;
        max_hr.insert("value", buffer);
        rideFile->metricOverrides.insert("max_heartrate", max_hr);
    }
    else if (qName == "MinimalTemp")
    {
        //min_temperature
    }
    else if (qName == "MaximalTemp")
    {
        //max_temperature
    }
    else if (qName == "Kalorien")
    {
        QMap<QString, QString> work;
        work.insert("value", QString("%1").arg(buffer.toDouble() / 0.239));
        rideFile->metricOverrides.insert("total_work", work);
    }
    else if (qName == "Strecke")
    {
        double dist = buffer.toDouble();
        QMap<QString, QString> distance;
        if (imperial)
            dist *= KM_PER_MILE;
        distance.insert("value", QString("%1").arg(dist));
        rideFile->metricOverrides.insert("total_distance", distance);
    }
    else if (qName == "Fahrzeit")
    {
        QStringList durationParts;
        QMap<QString,QString> trm;
        ulong time_in_sec;
        durationParts = buffer.split(':');
        time_in_sec = durationParts[0].toULong() * 60; //hours
        time_in_sec+= durationParts[1].toULong(); //minutes
        time_in_sec*= 60;
        time_in_sec+= durationParts[2].toULong(); //seconds
        trm.insert("value", QString("%1").arg(time_in_sec));

        rideFile->metricOverrides.insert("time_riding", trm);
        rideFile->setRecIntSecs(time_in_sec);
    }
    else if (qName == "DurchGeschwindigkeit")
    {
        double avg = buffer.toDouble();
        QMap<QString, QString> avg_speed;
        if (imperial)
            avg *= KM_PER_MILE;
        avg_speed.insert("value", QString("%1").arg(avg));
        rideFile->metricOverrides.insert("average_speed", avg_speed);
    }
    else if (qName == "MaxGeschwindigkeit")
    {
        double max = buffer.toDouble();
        QMap<QString, QString> max_speed;
        if (imperial)
            max *= KM_PER_MILE;
        max_speed.insert("value", QString("%1").arg(max));
        rideFile->metricOverrides.insert("max_speed", max_speed);
    }
    else if (qName == "DurchTrittfrequenz")
    {
        QMap<QString, QString> avg_cad;
        avg_cad.insert("value", buffer);
        rideFile->metricOverrides.insert("average_cad", avg_cad);
    }
    else if (qName == "MaxTrittfrequenz")
    {
        QMap<QString, QString> max_cad;
        max_cad.insert("value", buffer);
        rideFile->metricOverrides.insert("max_cad", max_cad);
    }
    else if (qName == "HoehenMeterBergauf")
    {
        double g = buffer.toDouble();
        QMap<QString, QString> gain;
        if (imperial)
            g *= METERS_PER_FOOT;
        gain.insert("value", QString("%1").arg(g));
        rideFile->metricOverrides.insert("elevation_gain", gain);
    }
    return TRUE;
}

bool SmfParser::characters( const QString& str )
{
    buffer += str;
    return TRUE;
}
