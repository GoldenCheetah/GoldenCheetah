/*
 * Copyright (c) 2015 Alejandro Martinez (amtriathlon@gmail.com)
 *               Based on TcxParser.cpp
 *
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net),
 *                    J.T Conklin (jtc@acorntoolworks.com)
 *
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
#include <QDebug>

#include "SmlParser.h"
#include "TimeUtils.h"
#include <cmath>

#define SMLdebug false // Lap Swimming debug

SmlParser::SmlParser(RideFile* rideFile) : rideFile(rideFile)
{
    cad = 0;
    speed = 0;
    distance = 0;
    lastDistance = 0;
    lastTime = 0;
    lastLength = 0;
    lastLat = lastLon = 0;
    watts = 0;
    alt = 0;
    lon = 0;
    lat = 0;
    hr = 0;
    temp = RideFile::NA;
    periodic = false;
    swimming = false;
    header = false;
    lap = 0;
    strokes = 0;
    style = 0;

    swimXdata = new XDataSeries();
    swimXdata->name = "SWIM";
    swimXdata->valuename << "TYPE";
    swimXdata->valuename << "DURATION";
    swimXdata->valuename << "STROKES";
}

bool
SmlParser::startElement(const QString&, const QString&,
                        const QString& qName,
                        const QXmlAttributes&)
{
    buffer.clear();

    if(header)
        return true;

    if(qName == "Header")
    {
        header = true;
    }
    else if(qName == "Sample")
    {
        cad = 0;
        speed = 0;
        distance = 0;
        watts = 0;
        alt = 0;
        hr = 0;
        temp = RideFile::NA;
        periodic = false;
        swimming = false;
    }
    return true;
}

#define PI 3.14159265
inline double toRadians(double degrees)
{
    return degrees * PI / 180;

}
inline double toDegrees(double radians)
{
    return radians * 180.0 / PI;
}

bool
SmlParser::endElement(const QString&, const QString&, const QString& qName)
{
    if(qName == "Header")
    {
        header = false;
    }
    else if(header == true)
    {
        if (qName == "DateTime")
        {
            rideFile->setStartTime(convertToLocalTime(buffer));
        }
        else if (qName == "Activity")
        {
            if (buffer.contains("Biking", Qt::CaseInsensitive))
                rideFile->setTag("Sport", "Bike");
            else if (buffer.contains("Running", Qt::CaseInsensitive))
                rideFile->setTag("Sport", "Run");
            else if (buffer.contains("Swimming", Qt::CaseInsensitive))
                rideFile->setTag("Sport", "Swim");
        }
        else if (qName == "PoolLength")
        {
            rideFile->setTag("Pool Length", buffer);
            rideFile->setTag("Sport", "Swim"); // Just in case Activity was renamed
        }
        return true;
    }
    else if (qName == "Lap")
    {
        lap++;
    }
    else if (qName == "Time")
    {
        time = buffer.toDouble();
    }
    else if (qName == "Latitude")
    {
        lat = toDegrees(buffer.toDouble());  // lat comes in radians
    }
    else if (qName == "Longitude")
    {
        lon = toDegrees(buffer.toDouble());  // lat comes in radians
    }
    else if (qName == "Altitude")
    {
        alt = buffer.toDouble();  // metric
    }
    else if (qName == "HR")
    {
        hr = round(buffer.toDouble()*60.0); // HR comes per sec
    }
    else if (qName == "Temperature")
    {
        temp = buffer.toDouble()-273.0; // Temperature comes in Kelvin unit
    }
    else if (qName == "Cadence")
    {
        cad = round(buffer.toDouble()*60.0); // Cadence comes in per sec
    }
    else if (qName == "Speed")
    {
        speed = buffer.toDouble()*3.6; // Speed comes in m/s
    }
    else if (qName == "Distance")
    {
        distance = buffer.toDouble()/1000.0; // Distance comes in meters
    }
    else if (qName == "BikePower")
    {
        watts = buffer.toDouble();
    }
    else if (qName == "SampleType")
    {
        periodic = (buffer == "periodic");
        swimming = (buffer == "swimming");
    }
    else if (qName == "Type")
    {
        if (buffer == "Stroke") strokes++;
    }
    else if (qName == "PrevPoolLengthStyle")
    {
        // style is coded to be compatible with FIT files
        if (buffer == "Freestyle") style = 0;
        else if (buffer == "Backstroke") style = 1;
        else if (buffer == "Breaststroke") style = 2;
        else if (buffer == "Flystroke") style = 3;
        else if (buffer == "Drill") style = 4;
        else if (buffer == "Other") style = 5;
    }


    else if (qName == "Sample")
    {
        if(time == 0 && periodic)
        {
            // update the "lasts" and record the first point
            lastTime = time;
            lastLon = lon;
            lastLat = lat;
            rideFile->appendPoint(time, cad, hr, 0, 0, 0, watts, alt,
                                  lon, lat, 0, 0.0, temp, 0.0,
                                  0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                  0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, lap);
            return true;
        }

        if (distance > 0 && speed == 0)
        {
            // compute speed from distance since the last recorded sample
            double delta_t = time - lastTime;
            double delta_d = distance - lastDistance;
            if (delta_t > 0.0 && delta_d > 0.0)
                speed = 3600.0 * delta_d / delta_t;
        }
        if (distance == 0 && speed > 0)
        {
            // compute distance traveled since the last recorded sample
            double delta_t = time - lastTime;
            if (delta_t > 0.0)
                distance = lastDistance + speed * delta_t / 3600.0;
        }
        else if (distance == 0 && speed == 0)
        {
            // we need to figure out the distance by using the lon,lat
            // using the haversine formula
            double r = 6371;
            double dlat = toRadians(lat -lastLat);  // convert to radians

            double dlon = toRadians(lon - lastLon);
            double a = sin(dlat /2) * sin(dlat/2) + cos(toRadians(lat)) *
                       cos(toRadians(lastLat)) * sin(dlon/2) * sin(dlon /2);
            double c = 4*atan2(sqrt(a),1+sqrt(1-fabs(a)));
            double delta_d = r * c;
            if(lastLat != 0)
                distance = lastDistance + delta_d;

            // compute the elapsed time and distance traveled since the
            // last recorded trackpoint
            double delta_t = time - lastTime;
            if (delta_d < 0)
                delta_d = 0;

            // compute speed using distance traveled and elapsed time
            if (delta_t > 0.0)
                speed = 3600.0 * delta_d / delta_t;
        }

        // Record point on periodic samples only
        if (periodic && round(time) > round(lastTime)) {
            if (distance > lastDistance) lastDistance = distance;
            if (SMLdebug) qDebug() << "    Time" << time;
            rideFile->appendPoint(round(time), cad, hr, lastDistance, speed, 0,
                         watts, alt, lon, lat, 0, 0.0, temp, 0.0, 0.0,
                         0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                         0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, lap);
            // update the "lasts"
            lastTime = time;
        }

        lastLon = lon;
        lastLat = lat;

        // Update distance, speed and cadence for swimming lengths
        if (swimming && distance > 0.0 && round(time) > lastLength) {
            if (SMLdebug) qDebug() << "Time" << time << "Distance" << distance << "lastLength" << lastLength << "lastDistance" << lastDistance;
            // length-by-length Swim XData
            XDataPoint *p = new XDataPoint();
            p->secs = lastLength;
            p->km = lastDistance;
            p->number[0] = (distance > lastDistance) ? 1 + style : 0;
            p->number[1] = time - lastLength;
            p->number[2] = (distance > lastDistance) ? strokes : 0;
            swimXdata->datapoints.append(p);

            if (distance > lastDistance) {
                double deltaSecs = round(time) - lastLength;
                double deltaDist = (distance - lastDistance) / deltaSecs;
                double kph = 3600.0 * deltaDist;
                double cad = 60 * strokes / deltaSecs;
                for (int i = rideFile->timeIndex(lastLength);
                     i>= 0 && i < rideFile->dataPoints().size() &&
                     rideFile->dataPoints()[i]->secs <= round(time);
                     ++i) {
                    rideFile->dataPoints()[i]->kph = kph;
                    rideFile->dataPoints()[i]->cad = cad;
                    rideFile->dataPoints()[i]->km = lastDistance;
                    lastDistance += deltaDist;
                }
                lastDistance = distance;
                strokes = 0;
                if (kph > 0.0) rideFile->setDataPresent(rideFile->kph, true);
                if (cad > 0.0) rideFile->setDataPresent(rideFile->cad, true);
                if (SMLdebug) qDebug() << "    kph" << kph;
            }
            lastLength = round(time);
        }
    }

    else if (qName == "Data")
    {   // R-R data: store in XData, when no HR in samples backfill
        // using EWMA filtered R-R
        double secs = 0.0;
        double ewmaRR = -1.0;
        const double ewmaTC = 5.0;
        XDataSeries *hrvXdata = new XDataSeries();
        hrvXdata->name = "HRV";
        hrvXdata->valuename << "R-R";
        hrvXdata->unitname << "msecs";
        foreach (QString strRR, buffer.split(" ")) {
            double rr = strRR.toDouble() / 1000.0;
            if (ewmaRR < 0.0) ewmaRR = rr;
            else ewmaRR += (rr - ewmaRR)/ewmaTC;
            if (!rideFile->isDataPresent(rideFile->hr)) {
                for (int i = 0; secs + rr >= trunc(secs) + i + 1; i++) {
                    rideFile->dataPoints()[rideFile->timeIndex(secs + i)]->hr = round(60.0/ ewmaRR);
                }
            }
            secs += rr;
            XDataPoint *p = new XDataPoint();
            p->secs = secs;
            p->km = 0;
            p->number[0] = rr * 1000.0;
            hrvXdata->datapoints.append(p);
        }
        if (ewmaRR >= 0.0 && !rideFile->isDataPresent(rideFile->hr))
            rideFile->setDataPresent(rideFile->hr, true);
        if (hrvXdata->datapoints.count()>0)
            rideFile->addXData("HRV", hrvXdata);
        else
            delete hrvXdata;
    }

    else if (qName == "Samples")
    {
        if (SMLdebug) qDebug()<<"Swim XData records"<<swimXdata->datapoints.count();
        // Add length-by-length Swim XData, if present
        if (swimXdata->datapoints.count()>0)
            rideFile->addXData("SWIM", swimXdata);
        else
            delete swimXdata;
    }

    return true;
}

bool
SmlParser::characters(const QString& str)
{
    buffer += str;
    return true;
}
