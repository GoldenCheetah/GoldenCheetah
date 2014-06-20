/*
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net),
 *                    J.T Conklin (jtc@acorntoolworks.com)
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

#include "TcxParser.h"
#include "TimeUtils.h"

// use stc strtod to bypass Qt toDouble() issues
#include <stdlib.h>

TcxParser::TcxParser (RideFile* rideFile, QList<RideFile*> *rides) : rideFile(rideFile), rides(rides)
{
    isGarminSmartRecording = appsettings->value(NULL, GC_GARMIN_SMARTRECORD,Qt::Checked);
    GarminHWM = appsettings->value(NULL, GC_GARMIN_HWMARK);
    if (GarminHWM.isNull() || GarminHWM.toInt() == 0) GarminHWM.setValue(25); // default to 25 seconds.
    first = true;

    // First initialisation for altitude (not initialised for each point)
    alt= 0;
}

bool
TcxParser::startElement( const QString&, const QString&, const QString& qName, const QXmlAttributes& qAttributes)
{
    buffer.clear();

    if (qName == "Activity") {

        lap = 0;

        if (first == true) first = false;
        else {

            rideFile = new RideFile();
            rideFile->setRecIntSecs(1.0);
            rideFile->setDeviceType("Garmin");
            rideFile->setFileFormat("Garmin Training Centre (tcx)");
        }

        // if caller is looking for rides...
        if (rides) rides->append(rideFile);

    } else if (qName == "Lap") {

    // Use the time of the first lap as the time of the activity.
        if (lap == 0) {

            start_time = convertToLocalTime(qAttributes.value("StartTime"));
            rideFile->setStartTime(start_time);

            last_distance = 0.0;
            last_time = start_time;
        }
        lap++;

    } else if (qName == "Trackpoint") {

        power = 0.0;
        cadence = 0.0;
        speed = 0.0;
        headwind = 0.0;
        torque = 0;
        hr = 0.0;
        lat = 0.0;
        lon = 0.0;
        badgps = false;
        //alt = 0.0; // TCX from FIT files have not alt point for each trackpoint
        distance = -1;  // nh - we set this to -1 so we can detect if there was a distance in the trackpoint.
        secs = 0;

    }

    return true;
}

bool
TcxParser::endElement( const QString&, const QString&, const QString& qName)
{
    if (qName == "Time") {
        time = convertToLocalTime(buffer);
        secs = start_time.secsTo(time);

    } else if (qName == "DistanceMeters") { distance = buffer.toDouble() / 1000; }
    else if (qName == "Watts" || qName == "ns3:Watts") { power = buffer.toDouble(); }
    else if (qName == "Speed" || qName == "ns3:Speed") { speed = buffer.toDouble() * 3.6; }
    else if (qName == "Value") { hr = buffer.toDouble(); }
    else if (qName == "Cadence") { cadence = buffer.toDouble(); }
    else if (qName == "AltitudeMeters") {
        // on Suunto TCX files there are lots of 0 values between valid ones, skip these
        if (buffer.toDouble() != 0) {
            alt = buffer.toDouble();
        }
    } else if (qName == "LongitudeDegrees") {

        char *p; 
        setlocale(LC_NUMERIC,"C"); // strtod is locale dependent!
        lon = strtod(buffer.toLatin1(), &p);
        setlocale(LC_NUMERIC,"");

    } else if (qName == "LatitudeDegrees") {
        char *p;
        setlocale(LC_NUMERIC,"C"); // strtod is locale dependent!
        lat = strtod(buffer.toLatin1(), &p);
        setlocale(LC_NUMERIC,"");

    } else if (qName == "Trackpoint") {

        // Some TCX files have Speed, some have Distance
        // Lets derive Speed from Distance or vice-versa
        // If we have neither Speed nor Distance then we
        // add a point with 0 for speed and distance
        if (speed == 0 || distance < 0) {

            // compute the elapsed time and distance traveled since the
            // last recorded trackpoint
            double delta_t = last_time.secsTo(time);

            // Derive speed from distance
            if (speed == 0 && distance >0) {

                double delta_d = distance - last_distance;
                if (delta_d<0) delta_d=0;
                if (delta_t > 0.0) speed=delta_d / delta_t * 3600.0;

            } else if (distance < 0) { // otherwise derive distance from speed

                double delta_d = delta_t * speed / 3600.0;
                distance = last_distance + delta_d;

            }
        }

        // Record trackpoint

        if (lat == 0 && lon == 0) badgps = true;

        // for smart recording, the delta_t will not be constant
        // average all the calculations based on the previous
        // point.

        if(rideFile->dataPoints().empty()) {

            // first point
            rideFile->appendPoint(secs, cadence, hr, distance, speed, torque,
                                  power, alt, lon, lat, headwind, 0.0, RideFile::NoTemp, 0.0, 
                                  0.0,0.0,0.0,0.0,0.0,0.0,lap);

        } else {

            // assumption that the change in ride is linear...  :)
            RideFilePoint *prevPoint = rideFile->dataPoints().back();
            double deltaSecs = secs - prevPoint->secs;
            double deltaCad = cadence - prevPoint->cad;
            double deltaHr = hr - prevPoint->hr;
            double deltaDist = distance - prevPoint->km;
            double deltaSpeed = speed - prevPoint->kph;
            double deltaTorque = torque - prevPoint->nm;
            double deltaPower = power - prevPoint->watts;
            double deltaAlt = alt - prevPoint->alt;
            double deltaLon = lon - prevPoint->lon;
            double deltaLat = lat - prevPoint->lat;

            if (prevPoint->lat == 0 && prevPoint->lon == 0) badgps = true;
            // Smart Recording High Water Mark.
            if ((isGarminSmartRecording.toInt() == 0) || (deltaSecs == 1) || (deltaSecs >= GarminHWM.toInt())) {

                // no smart recording, or delta exceeds HW treshold, just insert the data
                rideFile->appendPoint(secs, cadence, hr, distance, speed, torque, power,
                                      alt, lon, lat, headwind, 0.0, RideFile::NoTemp, 0.0, 
                                      0.0,0.0,0.0,0.0,0.0,0.0,lap);

            } else {

                // smart recording is on and delta is less than GarminHWM seconds.
                for(int i = 1; i <= deltaSecs; i++) {
                    double weight = i/ deltaSecs;
                    double kph = prevPoint->kph + (deltaSpeed *weight);
                    // need to make sure speed goes to zero
                    kph = kph > 0.35 ? kph : 0;
                    double cad = prevPoint->cad + (deltaCad * weight);
                    cad = cad > 0.35 ? cad : 0;
                    //double lat = prevPoint->lat + (deltaLat * weight);
                    //double lon = prevPoint->lon + (deltaLon * weight);

                    rideFile->appendPoint(prevPoint->secs + (deltaSecs * weight),
                                          prevPoint->cad  + (deltaCad * weight),
                                          prevPoint->hr +   (deltaHr * weight),
                                          prevPoint->km + (deltaDist * weight),
                                          kph,
                                          prevPoint->nm + (deltaTorque * weight),
                                          prevPoint->watts + (deltaPower * weight),
                                          prevPoint->alt + (deltaAlt * weight),
                                          badgps ? 0 : prevPoint->lon + (deltaLon * weight), // lon
                                          badgps ? 0 : prevPoint->lat + (deltaLat * weight), // lat
                                          headwind, // headwind
                                          0.0,
                                          RideFile::NoTemp,
                                          0.0,
                                          0.0,0.0,0.0,0.0,
                                          0.0,0.0,
                                          lap);
                }
                prevPoint = rideFile->dataPoints().back();
            }
        }
        last_distance = distance;
        last_time = time;
    }
    return true;
}

bool TcxParser::characters( const QString& str )
{
    buffer += str;
    return true;
}
