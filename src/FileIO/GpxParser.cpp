/*
 * Copyright (c) 2010 Greg Lonnon (greg.lonnon@gmail.com) copied from
 * TcxParser.cpp
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

#include "GpxParser.h"
#include "TimeUtils.h"
#include <cmath>

// use stc strtod to bypass Qt toDouble() issues
#include <stdlib.h>

GpxParser::GpxParser (RideFile* rideFile)
    : rideFile(rideFile)
{
    isGarminSmartRecording = appsettings->value(NULL, GC_GARMIN_SMARTRECORD,Qt::Checked);
    GarminHWM = appsettings->value(NULL, GC_GARMIN_HWMARK);
    if (GarminHWM.isNull() || GarminHWM.toInt() == 0)
        GarminHWM.setValue(25); // default to 25 seconds.

    cad = 0;
    distance = 0;
    lastLat = lastLon = 0;
    watts = 0;
    alt = 0;
    lon = 0;
    lat = 0;
    hr = 0;
    speed = std::numeric_limits<double>::infinity();
    temp = RideFile::NA;
    firstTime = true;
    metadata = false;

}

bool GpxParser::startElement( const QString&, const QString&,
    const QString& qName,
    const QXmlAttributes& qAttributes)
{
    buffer.clear();

    if(metadata)
        return true;

    if(qName == "metadata")
    {
        metadata = true;

    }
    else if(qName == "trkpt")
    {
        int i = qAttributes.index("lat");
        if(i >= 0)
        {
            lat = qAttributes.value(i).toDouble();
        }
        else
        {
            lat = lastLat;
        }
        i = qAttributes.index("lon");
        if( i >= 0)
        {
            lon = qAttributes.value(i).toDouble();
        }
        else
        {
            lon = lastLon;
        }

        // clear last speed value
        speed = std::numeric_limits<double>::infinity();
    }
    return true;
}

#define PI 3.14159265
inline double toRadians(double degrees)
{
    return degrees * 2 * PI / 360;

}

bool
        GpxParser::endElement( const QString&, const QString&, const QString& qName)
{
    if(qName == "metadata")
    {
        metadata = false;
    }
    else if(metadata == true)
    {
        return true;
    }
    else if (qName == "time")
    {

        time = convertToLocalTime(buffer);
        if(firstTime)
        {
            start_time = time;
            rideFile->setStartTime(time);
            firstTime = false;
        }
    }
    else if (qName == "ele")
    {
        alt = buffer.toDouble();  // metric
    }
    else if (qName == "gpxtpx:hr" || qName == "ns3:hr" || qName == "heartrate")
    {
        hr = buffer.toInt();
    }
    else if (qName == "gpxdata:hr")
    {
        hr = buffer.toDouble(); // on suunto ambit export file, there are sometimes double values
    }
    else if (qName == "gpxdata:temp" || qName == "gpxtpx:atemp" || qName == "ns3:atemp")
    {
        temp = buffer.toDouble();
    }
    else if (qName == "gpxdata:cadence" || qName == "gpxtpx:cad" || qName == "ns3:cad" || qName == "cadence")
    {
        cad = buffer.toDouble();
    }
    else if (qName == "power" || qName == "gpxdata:power" || qName.endsWith("PowerInWatts")) // from suunto ambit export file and UrbanBiker
    {
        watts = buffer.toDouble();
    }
    else if (qName == "gpxtpx:speed" || qName == "ns3:speed" || qName == "speed")
    {
        // gpx speed is in meters/s.  Convert to kph.
        speed = buffer.toDouble() * (60.0 * 60.0) / 1000.0;
    }


    else if (qName == "trkpt")
    {
        // Time from beginning of activity
        double secs = start_time.secsTo(time);

        if(lastLon == 0)
        {
            // update the "lasts" and find the next point
            last_time = time;
            lastLon = lon;
            lastLat = lat;
            lastSpeed = speed;
	    // first point
            rideFile->appendPoint(secs, cad, hr, 0, 0, 0, watts, alt, lon, lat, 0, 0.0, temp, 0.0, 
                                  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0);
            return true;
        }

        // compute the elapsed time and distance traveled since the
        // last recorded trackpoint
        // use msec in case there are msec in QDateTime
        double delta_t_ms = last_time.msecsTo(time);

        if (!std::isinf(speed))
        {
            // If speed is specified in the extensions, use it to calculate
            // distance traveled.  Note that gaps in speed data could be either
            // of two cases:
            //
            // 1) missing samples during movement -- interpolation appropriate
            // 2) paused collection while stopped -- zeroization appropriate
            //
            // There is no guaranteed way to differentiate between the two, so
            // here we use the Garmin Smart Recording threshold to switch from
            // (1) to (2).

            // speed adjusted for time gaps
            double speed_avg;

            // if Garmin Smart Recording is enabled and time elapsed is less
            // than the configured limit, use simple linear interpolation of the
            // speed.
            if (isGarminSmartRecording.toBool() &&
                delta_t_ms < GarminHWM.toInt() * 1000.0) {
                if (std::isinf(lastSpeed)) {
                    speed_avg = speed;
                }
                else {
                    speed_avg = (lastSpeed + speed) / 2.0;
                }
            }
            // otherwise, for deltas greater than two sampling periods, scale
            // the speed by the elapsed time.  This is equivalent to treating
            // speed as 0m/s for all but the first second, or "zeroing the gap".
            else {
                if (delta_t_ms >= (2 * GPX_SAMPLE_INTERVAL * 1000.0)) {
                    speed_avg = speed / (delta_t_ms / 1000.0);
                }
                else {
                    speed_avg = speed;
                }
            }

            // now calculate the distance traveled for this time and speed
            double delta_d = speed_avg * delta_t_ms / (60.0 * 60.0) / 1000.0; // speed in kph
            distance += delta_d; // distance in km
        }
        else
        {
            // we need to figure out the distance by using the lon,lat
            // using the haversine formula
            double r = 6371;
            double dlat = toRadians(lat -lastLat);  // convert to radians

            double dlon = toRadians(lon - lastLon);
            double a = sin(dlat /2) * sin(dlat/2) + cos(toRadians(lat)) * cos(toRadians(lastLat)) * sin(dlon/2) * sin(dlon /2);
            //double c = 2*asin(sqrt(fabs(a)));  // Alternate definition.
            double c = 4*atan2(sqrt(a),1+sqrt(1-fabs(a)));
            double delta_d = r * c;
            if(lastLat != 0)
                distance += delta_d;

            if (delta_d<0)
            {
                delta_d=0;
            }

            // compute speed for this trackpoint by dividing the distance
            // traveled by the elapsed time. The elapsed time will be 0.0
            // for the first trackpoint -- so set speed to 0.0 instead of
            // dividing by zero.
            if (delta_t_ms > 0.0)
            {
                speed= 1000.0 * delta_d / delta_t_ms * 3600.0;
            }
        }

        // Record trackpoint

	// for smart recording, the delta_t will not be constant
	// average all the calculations based on the previous
	// point.

	// assumption that the change in ride is linear...  :)
	RideFilePoint *prevPoint = rideFile->dataPoints().back();
	double deltaSecs = secs - prevPoint->secs;

	    // Smart Recording High Water Mark.
        if ((isGarminSmartRecording.toInt() == 0) ||
                (deltaSecs == 1) ||
                (deltaSecs >= GarminHWM.toInt()) ||
                (secs == 0)) {

                // no smart recording, or delta exceeds HW treshold, or no time elements; just insert the data
                rideFile->appendPoint(secs, cad, hr, distance, speed, 0,watts, alt, lon, lat, 0, 0.0, temp, 0.0, 
                                      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0);

	    } else {
       	        double deltaDist = distance - prevPoint->km;
		double deltaSpeed = speed - prevPoint->kph;
		double deltaAlt = alt - prevPoint->alt;
		double deltaLon = lon - prevPoint->lon;
		double deltaLat = lat - prevPoint->lat;

		// smart recording is on and delta is less than GarminHWM seconds.
		for(int i = 1; i <= deltaSecs; i++) {
		    double weight = i/ deltaSecs;
		    double kph = prevPoint->kph + (deltaSpeed *weight);
		    // need to make sure speed goes to zero
		    kph = kph > 0.35 ? kph : 0;
		    double lat = prevPoint->lat + (deltaLat * weight);
		    double lon = prevPoint->lon + (deltaLon * weight);
		    rideFile->appendPoint(
			    prevPoint->secs + (deltaSecs * weight),
                cad,
			    hr,
			    prevPoint->km + (deltaDist * weight),
			    kph,
			    0,
                watts,
			    prevPoint->alt + (deltaAlt * weight),
			    lon, // lon
			    lat, // lat
                0,
                0.0,
                temp,
                0,
                0.0, 0.0, 0.0, 0.0, // pedal torque/smoothness
                0.0, 0.0, // pedal platform offset
                0.0, 0.0, 0.0, 0.0, //pedal power phase
                0.0, 0.0, 0.0, 0.0, //pedal peak power phase
                0.0, 0.0, // SmO2 / tHb
                0.0, 0.0, 0.0, // running dynamics
                0.0, //tcore
			    0);
		}
		prevPoint = rideFile->dataPoints().back();
	}
	// update the "lasts" and find the next point
        last_time = time;
        lastLon = lon;
        lastLat = lat;
        lastSpeed = speed;
    }

    return true;
}

bool GpxParser::characters( const QString& str )
{
    buffer += str;
    return true;
}
