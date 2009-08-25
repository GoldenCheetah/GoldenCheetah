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

#include "TcxParser.h"
#include "TimeUtils.h"

TcxParser::TcxParser (RideFile* rideFile)
   : rideFile(rideFile)
{
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
	alt = 0.0;
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
    else if (qName == "Watts")
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
    else if (qName == "Altitude")
    {
        alt = buffer.toDouble();
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

            // Work around bug in 705 firmware where cadence and
            // power values repeat when stopped.
            if (delta_d == 0.0)
            {
                power = 0.0;
                cadence = 0.0;
            }

            // Record trackpoint
            rideFile->appendPoint(secs, cadence, hr, distance,
                                   speed, torque, power, alt, lap);

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
