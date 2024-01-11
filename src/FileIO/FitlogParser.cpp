/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#include "FitlogParser.h"
#include "TimeUtils.h"

// use stc strtod to bypass Qt toDouble() issues
#include <stdlib.h>
#include <cmath>

FitlogParser::FitlogParser (RideFile* rideFile, QList<RideFile*> *rides)
   : rideFile(rideFile), rides(rides)
{
  first = true;
  lap = 0;
  track_offset = 0;

}

bool
FitlogParser::startElement( const QString&, const QString&,
			 const QString& qName,
			 const QXmlAttributes& qAttributes)
{
    buffer.clear();

    if (qName == "Activity") {

        lap = 0;

        if (first == true) first = false;
        else {

            rideFile = new RideFile();
            rideFile->setRecIntSecs(1.0);
            //rideFile->setDeviceType("SportTracks");
            rideFile->setFileFormat("SportTracks (*.fitlog)");
        }

        rideFile->setStartTime(start_time = convertToLocalTime(qAttributes.value("StartTime")));

        // if caller is looking for rides...
        if (rides) rides->append(rideFile);

    } else if (qName == "Lap") {

        lap++;
        double start = start_time.secsTo(convertToLocalTime(qAttributes.value("StartTime")));
        double stop = start + qAttributes.value("DurationSeconds").toDouble();
        rideFile->addInterval(RideFileInterval::DEVICE, start, stop, QString("%1").arg(lap));

    } else if (qName == "Track") {

	    // Use the time of the first lap as the time of the activity.
        track_offset = start_time.secsTo(convertToLocalTime(qAttributes.value("StartTime")));

    } else if (qName == "Category") {

        rideFile->setTag("Sport", qAttributes.value("Name"));

    } else if (qName == "Metadata") {

        QString source = qAttributes.value("Source");
        if (source != "") rideFile->setDeviceType(source);

    } else if (qName == "Location") {

        QString location = qAttributes.value("Name");
        if (location != "") rideFile->setTag("Route", location);

    } else if (qName == "EquipmentItem") {

        QString equipment = qAttributes.value("Name");
        if (equipment != "") {
            QString prevEq = rideFile->getTag("Equipment", "");
            if (prevEq != "") equipment = prevEq + ", " + equipment;
            rideFile->setTag("Equipment", equipment);
        }

    } else if (qName == "pt") {

        // set point values to zero
        RideFilePoint point;

        // extract from the attributes
        for (int i=0; i<qAttributes.count(); i++) {
            QString m = qAttributes.qName(i);

            if (m == "tm") point.secs = track_offset + qAttributes.value(i).toInt();
            else if (m == "dist") point.km = qAttributes.value(i).toFloat() / 1000.00; // meters to km
            else if (m == "ele") point.alt = qAttributes.value(i).toFloat();
            else if (m == "hr") point.hr = qAttributes.value(i).toFloat();
            else if (m == "cadence") point.cad = qAttributes.value(i).toFloat();
            else if (m == "power") point.watts = qAttributes.value(i).toFloat();
            else if (m == "lat") point.lat = qAttributes.value(i).toFloat();
            else if (m == "lon") point.lon = qAttributes.value(i).toFloat();
        }

        // now add
        rideFile->appendPoint(point.secs,point.cad,point.hr,point.km,point.kph,point.nm,
                              point.watts,point.alt,point.lon,point.lat, point.headwind,
                              0.0, RideFile::NA, RideFile::NA,
                              0.0, 0.0, 0.0, 0.0,
                              0.0, 0.0,
                              0.0, 0.0, 0.0, 0.0,
                              0.0, 0.0, 0.0, 0.0,
                              0.0, 0.0,
                              0.0, 0.0, 0.0,// running dynamics
                              0.0, //tcore
                              point.interval);
    }
    return true;
}

bool
FitlogParser::endElement( const QString&, const QString&, const QString& qName)
{
    if (qName == "Activity") {

        // DERIVE DISTANCE FROM GPS
        if (!rideFile->areDataPresent()->km &&
             rideFile->areDataPresent()->lat &&
             rideFile->areDataPresent()->lon) {

            RideFilePoint last;
            bool first = true;
            double rdist = 0;

            foreach(RideFilePoint *point, rideFile->dataPoints()) {

                if (first == true) {
                    if (point->lat && point->lon) first = false;
                } else {

                    if (last.lat && last.lon && point->lat && point->lon) {

                        if (rideFile->areDataPresent()->alt)
                            rdist += distanceBetween(last.lat, last.lon, last.alt,
                                                     point->lat, point->lon, point->alt);
                        else
                            rdist += distanceBetween(last.lat, last.lon, point->lat, point->lon);
                    }
                    point->km = rdist; 
                }
                last = *point;
            }
            rideFile->setDataPresent(RideFile::km, (rdist > 0));
        }

        // DERIVE SPEED
        if (rideFile->areDataPresent()->km && !rideFile->areDataPresent()->kph) {

            RideFilePoint last;
            bool first = true;

            foreach(RideFilePoint *point, rideFile->dataPoints()) {

                if (first == true) {
                    first = false;
                } else {
                    double distdelta = point->km - last.km;
                    double timedelta = point->secs - last.secs;

                    if (timedelta) {
                        point->kph = (distdelta / timedelta) * 3600; // km/s to km/h

                    } else {
                        point->kph = 0;
                    }
                }
                last = *point;
            }
            rideFile->setDataPresent(RideFile::kph, true);
        }

        // DERIVE RECINTSECS
        QMap<double, int> ints;

        bool first = true;
        double last = 0;

        foreach(RideFilePoint *p, rideFile->dataPoints()) {

            if (first) {
                last = p->secs;
                first = false;
            } else {
                double delta = p->secs-last;
                last = p->secs;

                // lookup
                int count = ints.value(delta);
                count++;
                ints.insert(delta, count);
            }
        }

        // which is most popular?
        double populardelta=1.0;
        int count=0;
        QMapIterator<double, int> i(ints);
        while (i.hasNext()) {
            i.next();

            if (i.value() > count) {
                count = i.value();
                populardelta = i.key();
            }
        }
        rideFile->setRecIntSecs(populardelta);

    } else if (qName == "Name") {

        rideFile->setTag("Objective", buffer);

    } else if (qName == "Notes") {

        rideFile->setTag("Notes", buffer);
    }

    return true;
}
bool FitlogParser::characters( const QString& str )
{
    buffer += str;
    return true;
}

static const double EARTH_RADIUS = 6378.140; // in km
static const double PI = 3.14159265358979323846264;
static const double DEG2RAD = PI/180.00 ;

// calculate distance in kilometers between point(lat1,lon1) and point(lat2, lon2)
double
FitlogParser::distanceBetween(double lat1, double lon1, double lat2, double lon2)
{
    double distance;

    if ((lat1 == lat2) && (lon1 == lon2)) {

        // same position
        distance = 0.0;

    } else {

        lat1 *= DEG2RAD;
        lat2 *= DEG2RAD;
        lon1 *= DEG2RAD;
        lon2 *= DEG2RAD;

        distance = acos(cos(lat1)*cos(lon1)*cos(lat2)*cos(lon2) +
                        cos(lat1)*sin(lon1)*cos(lat2)*sin(lon2) +
                        sin(lat1)*sin(lat2)) * EARTH_RADIUS;

    }
    return distance;
}

double
FitlogParser::distanceBetween(double lat1, double lon1, double alt1,
                              double lat2, double lon2, double alt2)
{
    double distance = distanceBetween(lat1, lon1, lat2, lon2);

    // now take in slope
    double delta = alt2-alt1;
    delta *= delta < 0 ? -0.001 : 0.001; // make pos and convert to kms
    distance = sqrt(pow(distance, 2) + pow(delta, 2));

    return distance;
}
