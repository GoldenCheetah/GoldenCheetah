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

TcxParser::TcxParser (RideFile* rideFile)
   : rideFile(rideFile)
{
  boost::shared_ptr<QSettings> settings = GetApplicationSettings();
  isGarminSmartRecording = settings->value(GC_GARMIN_SMARTRECORD,Qt::Checked);
  GarminHWM = settings->value(GC_GARMIN_HWMARK);
  if (GarminHWM.isNull() || GarminHWM.toInt() == 0)
      GarminHWM.setValue(25); // default to 25 seconds.

}

bool
TcxParser::startElement( const QString&, const QString&,
			 const QString& qName,
			 const QXmlAttributes& qAttributes)
{
    buffer.clear();

    if (qName == "Activity")
    {
        lap = 0;
    }
    else if (qName == "Lap")
    {
	// Use the time of the first lap as the time of the activity.
        if (lap == 0)
        {
            start_time = convertToLocalTime(qAttributes.value("StartTime"));
            rideFile->setStartTime(start_time);

            lastDistance = 0.0;
            last_time = start_time;
	}
        lap++;
    }
    else if (qName == "Trackpoint")
    {
        power = 0.0;
        cadence = 0.0;
        hr = 0.0;
        //alt = 0.0; // TCS from FIT files have not alt point for each trackpoint
        distance = -1;  // nh - we set this to -1 so we can detect if there was a distance in the trackpoint.
    }
    return TRUE;
}

bool
TcxParser::endElement( const QString&, const QString&, const QString& qName)
{
    if (qName == "Time")
    {
        time = convertToLocalTime(buffer);
    }
    else if (qName == "DistanceMeters")
    {
        distance = buffer.toDouble() / 1000;
    }
    else if (qName == "Watts" || qName == "ns3:Watts")
    {
        power = buffer.toDouble();
    }
    else if (qName == "Value")
    {
        hr = buffer.toDouble();
    }
    else if (qName == "Cadence")
    {
        cadence = buffer.toDouble();
    }
    else if (qName == "AltitudeMeters")
    {
        alt = buffer.toDouble();
    }
    else if (qName == "LongitudeDegrees")
    {
        char *p;
        lon = strtod(buffer.toLatin1(), &p);
    }
    else if (qName == "LatitudeDegrees")
    {
        char *p;
        lat = strtod(buffer.toLatin1(), &p);
    }
    else if (qName == "Trackpoint")
    {
        // nh - there are track points that don't have any distance info.  We need to ignore them
        if (distance>=0)
        {
            // compute the elapsed time and distance traveled since the
            // last recorded trackpoint
            double delta_t = last_time.secsTo(time);
            double delta_d = distance - lastDistance;
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

            // Don't know what to do about torque
            double torque  = 0.0;

            // Time from beginning of activity
            double secs = start_time.secsTo(time);

            // Record trackpoint

	    // for smart recording, the delta_t will not be constant
	    // average all the calculations based on the previous
	    // point.
            headwind = 0.0;
	    if(rideFile->dataPoints().empty()) {
	        // first point
	        rideFile->appendPoint(secs, cadence, hr, distance,
                                   speed, torque, power, alt, lon, lat, headwind, lap);
	    }
	    else {
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

	  // Smart Recording High Water Mark.
	  if ((isGarminSmartRecording.toInt() == 0) || (deltaSecs == 1) || (deltaSecs >= GarminHWM.toInt())) {
		  // no smart recording, or delta exceeds HW treshold, just insert the data
		  rideFile->appendPoint(secs, cadence, hr, distance,
                                   speed, torque, power, alt, lon, lat, headwind, lap);
	      }
	      else {
		// smart recording is on and delta is less than GarminHWM seconds.
		  for(int i = 1; i <= deltaSecs; i++) {
		      double weight = i/ deltaSecs;
		      double kph = prevPoint->kph + (deltaSpeed *weight);
		      // need to make sure speed goes to zero
		      kph = kph > 0.35 ? kph : 0;
		      double cad = prevPoint->cad + (deltaCad * weight);
		      cad = cad > 0.35 ? cad : 0;
              double lat = prevPoint->lat + (deltaLat * weight);
              double lon = prevPoint->lon + (deltaLon * weight);
		      rideFile->appendPoint(
					    prevPoint->secs + (deltaSecs * weight),
					    prevPoint->cad  + (deltaCad * weight),
					    prevPoint->hr +   (deltaHr * weight),
					    prevPoint->km + (deltaDist * weight),
					    kph,
					    prevPoint->nm + (deltaTorque * weight),
					    prevPoint->watts + (deltaPower * weight),
					    prevPoint->alt + (deltaAlt * weight),
                        lon, // lon
                        lat, // lat
                        headwind, // headwind
					    lap);
		  }
		  prevPoint = rideFile->dataPoints().back();
	      }
	    }
            lastDistance = distance;
        }
        last_time = time;
    }
    return TRUE;
}

bool TcxParser::characters( const QString& str )
{
    buffer += str;
    return TRUE;
}
