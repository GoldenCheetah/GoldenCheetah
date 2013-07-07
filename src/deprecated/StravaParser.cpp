/*
 * Copyright (c) 2013 Damien.Grauser (damien.grauser@pev-geneve.ch)
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
#include <QScriptEngine>

#include "StravaParser.h"
#include "TimeUtils.h"
#include "Units.h"

StravaParser::StravaParser (RideFile* rideFile, QFile* file)
    : rideFile(rideFile), file(file)
{


}

void
StravaParser::parse()
{
    QScriptValue sc;
    QScriptEngine se;

    if (!file->open(QIODevice::ReadOnly)) {
        delete rideFile;
        return;
    }

    QByteArray content = file->readAll();
    file->close();

    sc = se.evaluate("("+content+")");

    if (sc.isError()) {
        delete rideFile;
        return;
    }

    QScriptValue ride = sc.property("ride");

    if (!ride.isNull()) {
        rideFile->setRecIntSecs(1.0);
        rideFile->setDeviceType("Strava activity");
        rideFile->setFileFormat("Strava activity streams (strava)");

        QDateTime datetime = QDateTime::fromString(ride.property("startDateLocal").toString(), Qt::ISODate);
        rideFile->setStartTime(datetime);

        rideFile->setTag("Notes", ride.property("location").toString());

        QScriptValue streams = ride.property("streams");

        int length = streams.property("time").property("length").toInteger();

        bool hasHeartrate = (streams.property("heartrate").property("length").toInteger()>0);
        bool hasVelocity = (streams.property("velocity_smooth").property("length").toInteger()>0);
        bool hasDistance = (streams.property("distance").property("length").toInteger()>0);
        bool hasCadence = (streams.property("cadence").property("length").toInteger()>0);
        bool hasAltitude = (streams.property("altitude").property("length").toInteger()>0);
        bool hasLatLng = (streams.property("latlng").property("length").toInteger()>0);
        bool hasWatts = (streams.property("watts").property("length").toInteger()>0);
        bool hasTemp = (streams.property("temp").property("length").toInteger()>0);

        for(int i = 0; i < length; i++) {
            RideFilePoint point;

            point.secs = streams.property("time").property(i).toNumber();
            if (hasDistance)
                point.km = streams.property("distance").property(i).toNumber()/1000.0;
            if (hasVelocity)
                point.kph = streams.property("velocity_smooth").property(i).toNumber()*3.6;
            if (hasHeartrate)
                point.hr = streams.property("heartrate").property(i).toNumber();
            if (hasCadence)
                point.cad = streams.property("cadence").property(i).toNumber();
            if (hasAltitude)
                point.alt = streams.property("altitude").property(i).toNumber();
            if (hasLatLng) {
                point.lat = streams.property("latlng").property(i).property(0).toNumber();
                point.lon = streams.property("latlng").property(i).property(1).toNumber();
            }
            if (hasWatts)
                point.watts = streams.property("watts").property(i).toNumber();
            if (hasTemp)
                point.temp = streams.property("temp").property(i).toNumber();

            rideFile->appendPoint(point.secs, point.cad, point.hr, point.km,
                                   point.kph, point.nm, point.watts, point.alt,
                                   point.lon, point.lat, point.headwind,
                                   point.slope, point.temp, point.lrbalance, point.interval);
        }
    }
}

