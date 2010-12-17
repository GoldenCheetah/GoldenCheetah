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
#include <math.h>

// use stc strtod to bypass Qt toDouble() issues
#include <stdlib.h>

GpxParser::GpxParser (RideFile* rideFile)
    : rideFile(rideFile)
{
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    isGarminSmartRecording = settings->value(GC_GARMIN_SMARTRECORD,Qt::Checked);
    GarminHWM = settings->value(GC_GARMIN_HWMARK);
    if (GarminHWM.isNull() || GarminHWM.toInt() == 0)
        GarminHWM.setValue(25); // default to 25 seconds.

    distance = 0;
    lastLat = lastLon = 0;
    alt =0;
    lon = 0;
    lat = 0;
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
    }
    return TRUE;
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

    else if (qName == "trkpt")
    {
        if(lastLon == 0)
        {
            // update the "lasts" and find the next point
            last_time = time;
            lastLon = lon;
            lastLat = lat;
            return TRUE;
        }
        // we need to figure out the distance by using the lon,lat
        // using teh haversine formula
        double r = 6371;
        double dlat = toRadians(lat -lastLat);  // convert to radians

        double dlon = toRadians(lon - lastLon);
        double a = sin(dlat /2) * sin(dlat/2) + cos(lat) * cos(lastLat) * sin(dlon/2) * sin(dlon /2);
        double c = 2 * atan2(sqrt(a),sqrt(1-a));
        double delta_d = r * c;
        if(lastLat != 0)
            distance += delta_d;

        // compute the elapsed time and distance traveled since the
        // last recorded trackpoint
        double delta_t = last_time.secsTo(time);
        if (delta_d<0)
        {
            delta_d=0;
        }

        // compute speed for this trackpoint by dividing the distance
        // traveled by the elapsed time. The elapsed time will be 0.0
        // for the first trackpoint -- so set speed to 0.0 instead of
        // dividing by zero.
        double speed = 0.0;
        if (delta_t > 0.0)
        {
            speed=delta_d / delta_t * 3600.0;
        }

        // Time from beginning of activity
        double secs = start_time.secsTo(time);

        // Record trackpoint

	// for smart recording, the delta_t will not be constant
	// average all the calculations based on the previous
	// point.

	if(rideFile->dataPoints().empty()) {
	    // first point
	    rideFile->appendPoint(secs, 0, 0, distance,
				  speed, 0, 0, alt, lon, lat, 0, 0);
	}
	else {
	    // assumption that the change in ride is linear...  :)
	    RideFilePoint *prevPoint = rideFile->dataPoints().back();
	    double deltaSecs = secs - prevPoint->secs;
	    double deltaDist = distance - prevPoint->km;
	    double deltaSpeed = speed - prevPoint->kph;
	    double deltaAlt = alt - prevPoint->alt;
	    double deltaLon = lon - prevPoint->lon;
	    double deltaLat = lat - prevPoint->lat;

	    // Smart Recording High Water Mark.
	    if ((isGarminSmartRecording.toInt() == 0) || (deltaSecs == 1) || (deltaSecs >= GarminHWM.toInt())) {
		// no smart recording, or delta exceeds HW treshold, just insert the data
		rideFile->appendPoint(secs, 0, 0, distance,
				      speed, 0,0, alt, lon, lat, 0, 0);
	    }
	    else {
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
			    0,
			    0,
			    prevPoint->km + (deltaDist * weight),
			    kph,
			    0,
			    0,
			    prevPoint->alt + (deltaAlt * weight),
			    lon, // lon
			    lat, // lat
			    0,
			    0);
		}
		prevPoint = rideFile->dataPoints().back();
	    }
	}
	// update the "lasts" and find the next point
        last_time = time;
        lastLon = lon;
        lastLat = lat;
    }

    return TRUE;
}

bool GpxParser::characters( const QString& str )
{
    buffer += str;
    return TRUE;
}
