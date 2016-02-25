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
 * 
 * Rox 10.0 support by Carlos Giraldo (carlitosgiraldo@gmail.com)
 */

#include <QString>

#include "SlfParser.h"
#include "TimeUtils.h"
#include "Units.h"

SlfParser::SlfParser (RideFile* rideFile)
   : rideFile(rideFile)
{
}

bool
SlfParser::startElement( const QString&, const QString&,
			 const QString& qName,
			 const QXmlAttributes& qAttributes)
{
    buffer.clear();
    (void)qName;
    (void)qAttributes;

    if (qName == "Activity")
    {
        secs = 0.0;
        distance = 0.0;
        lap = 0;
    } else if (qName == "Computer")
    {
        rideFile->setDeviceType(qAttributes.value("unit"));
    }
    else if (qName == "Log")
    {
        secs = 0.0;
        distance = 0.0;
        lap = 0;
    }
    else if (qName == "Eintrag") {
        hr = 0.0;
        alt = 0.0;
        speed = 0.0;
        pauseSec = 0.0;
        restSec = 0.0;
        if (qAttributes.value("wp").toULong() == 1)
        {
            lap++;
        }
    }
    else if (qName == "Pause")
    {
        pauseSec = qAttributes.value("zeit").toDouble();
    }
    else if (qName == "Rest")
    {
        restSec = qAttributes.value("zeit").toDouble();
    }
    // Rox 10 Entries
    else if (qName == "Entry")
    {
        double secs = qAttributes.value("trainingTimeAbsolute").toDouble()/100; 
        double cadence = qAttributes.value("cadence").toDouble(); 
        double hr = qAttributes.value("heartrate").toDouble();
        double distance = qAttributes.value("distanceAbsolute").toDouble()/1000;
        double speed = qAttributes.value("speed").toDouble()*3.6;
        double torque = 0.0;
        double power = qAttributes.value("power").toDouble();
        double alt = qAttributes.value("altitude").toDouble()/1000;
        double lon = qAttributes.value("longitude").toDouble();
        double lat = qAttributes.value("latitude").toDouble();
        double headwind = 0.0;
        double temp = qAttributes.value("temperature").toDouble();
        double slope = qAttributes.value("incline").toDouble();
        rideFile->appendPoint(secs, cadence, hr, distance, speed, torque, power, alt, lon, lat, headwind, slope, temp, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, lap);
    } 
    return true;
}

bool
SlfParser::endElement( const QString&, const QString&, const QString& qName)
{
    if (qName == "startDate")
    {
        // Fri May 1 13:55:10 GMT+0200 2015
        QLocale local(QLocale::English);
        QString date = buffer.mid(0,buffer.indexOf("GMT")) + buffer.right(4);
        rideFile->setStartTime(local.toDateTime(date, "ddd MMM d HH:mm:ss yyyy"));
    }
    else if (qName == "StartDatum")
    {
        start_time.setDate(QDate::fromString(buffer, "dd.MM.yy").addYears(100));
        rideFile->setStartTime(start_time);
    }
    else if (qName == "StartZeit")
    {
        start_time.setTime(QTime::fromString(buffer, "hh:mm:ss"));
    }
    else if (qName == "StoppDatum")
    {
        QMap<QString, QString> workout;
        stop_time.setDate(QDate::fromString(buffer, "dd.MM.yy").addYears(100));
        workout.insert("value", QString("%1").arg(start_time.secsTo(stop_time)));
        rideFile->metricOverrides.insert("workout_time", workout);
    }
    // ROX 10.0 format
    else if (qName == "trainingTime")
    {
        QMap<QString, QString> workout;
        workout.insert("value", QString("%1").arg(buffer.toDouble()/100));
        rideFile->metricOverrides.insert("workout_time", workout);
    }
    else if (qName == "StoppZeit")
    {
        stop_time.setTime(QTime::fromString(buffer, "hh:mm:ss"));
    }
    else if (qName == "RadGroesse")
    {
        wheelSize = buffer.toInt();
    }
    else if (qName == "Einheit")
    {
        imperial = (buffer == "mph");
    }
    else if (qName == "Kalorien")
    {
        QMap<QString, QString> work;
        work.insert("value", QString("%1").arg(buffer.toDouble() / 0.239));
        rideFile->metricOverrides.insert("total_work", work);
    }
    else if (qName == "SamplingRate")
    {
        //Seems like the sampling rate is rounded...
        samplingRate = buffer.toDouble() - 0.5;
        rideFile->setRecIntSecs(samplingRate);
    }
    // Rox 10.0 format
    else if (qName == "samplingRate")
    {
        samplingRate = buffer.toDouble();
        rideFile->setRecIntSecs(samplingRate);
    }
    else if (qName == "Speed")
    {
        speed = buffer.toDouble();
    }
    else if (qName == "Puls")
    {
        hr = buffer.toDouble();
    }
    else if (qName == "Hoehe")
    {
        alt = buffer.toDouble();
    }
    else if (qName == "RPLAbs")
    {
        rotations = buffer.toDouble();
        distance += (rotations * (wheelSize) / 1000 / 1000);
    }
    else if (qName == "Temp")
    {
        temperature = buffer.toDouble();
    }
    else if (qName == "Eintrag")
    {
        double cadence = 0.0;
        double torque = 0.0;
        double power = 0.0;
        double lon = 0.0;
        double lat = 0.0;
        double headwind = 0.0;
        if (pauseSec > 0.0)
        {
            secs += pauseSec - restSec;
        }
        else
        {
            if (imperial)
            {
                speed *= KM_PER_MILE;
                distance *= KM_PER_MILE;
                alt *= FEET_PER_METER;
            }
            rideFile->appendPoint(secs, cadence, hr, distance, speed, torque, power, alt, lon, lat, headwind, 0.0,
                                  RideFile::NA, RideFile::NA,
                                  0.0, 0.0, 0.0, 0.0,
                                  0.0, 0.0,
                                  0.0, 0.0, 0.0, 0.0,
                                  0.0, 0.0, 0.0, 0.0,
                                  0.0, 0.0,
                                  0.0, 0.0, 0.0, 0.0,
                                  lap);
            secs += samplingRate;
        }
    }
    return true;
}

bool SlfParser::characters( const QString& str )
{
    buffer += str;
    return true;
}
