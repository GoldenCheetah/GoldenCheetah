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
#include <assert.h>
#include "TcxParser.h"

TcxParser::TcxParser (RideFile* rideFile)
   : rideFile_(rideFile)
{
}

bool
TcxParser::startElement( const QString&, const QString&,
			 const QString& qName,
			 const QXmlAttributes& qAttributes)
{
    buf_.clear();

    if (qName == "Activity") {
	lap_ = 0;
    } else if (qName == "Lap") {
	// Use the time of the first lap as the time of the activity.
	if (lap_ == 0) {
	    start_time_ = QDateTime::fromString(qAttributes.value("StartTime"),
						Qt::ISODate);
	    rideFile_->setStartTime(start_time_);
	    
	    last_km_ = 0.0;
	    last_time_ = start_time_;
	}

	++lap_;
    } else if (qName == "Trackpoint") {
	watts_ = 0.0;
	cad_ = 0.0;
	hr_ = 0.0;
        km_ = -1;  // nh - we set this to -1 so we can detect if there was a distance in the trackpoint.
    }

    return TRUE;
}

bool
TcxParser::endElement( const QString&, const QString&, const QString& qName)
{
    if (qName == "Time") {
	time_ = QDateTime::fromString(buf_, Qt::ISODate);
    }
    if (qName == "DistanceMeters") {
	km_    = buf_.toDouble() / 1000;
    } else if (qName == "Watts") {
	watts_ = buf_.toDouble();
    } else if (qName == "Value") {
	hr_    = buf_.toDouble();
    } else if (qName == "Cadence") {
	cad_   = buf_.toDouble();
    } else if (qName == "Trackpoint")
    {
        // nh - there are track points that don't have any distance info.  We need to ignore them
        if (km_>=0)
        {
            // compute the elapsed time and distance traveled since the
            // last recorded trackpoint
            double delta_t = last_time_.secsTo(time_);
            double delta_d = km_ - last_km_;
            assert(delta_d>=0);

            // compute speed for this trackpoint by dividing the distance
            // traveled by the elapsed time. The elapsed time will be 0.0
            // for the first trackpoint -- so set speed to 0.0 instead of
            // dividing by zero.
            double kph = ((delta_t != 0.0) ? ((delta_d / delta_t) * 3600.0) : 0.0);
            assert(kph>=0);

            // Don't know what to do about torque
            double nm  = 0.0;

            // Time from beginning of activity
            double secs = start_time_.secsTo(time_);

            // Work around bug in 705 firmware where cadence and
            // power values repeat when stopped.
            if (delta_d == 0.0) {
                watts_ = 0.0;
                cad_ = 0.0;
            }

            // Record trackpoint
            rideFile_->appendPoint(secs, cad_, hr_, km_,
                                   kph, nm, watts_, lap_);

            last_km_ = km_;
        }
        last_time_ = time_;
    }
	

    return TRUE;
}

bool
TcxParser::characters( const QString& str )
{
    buf_ += str;
    return TRUE;
}
