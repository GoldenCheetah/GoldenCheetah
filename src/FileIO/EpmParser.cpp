/*
 * Copyright (c) 2020 Alejandro Martinez (amtriathlon@gmail.com)
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

#include "EpmParser.h"
#include "TimeUtils.h"
#include <cmath>

EpmParser::EpmParser(RideFile* rideFile) : rideFile(rideFile)
{
    lastDist = 0;
    lastSecs = 0;
}

bool
EpmParser::startElement(const QString&, const QString&,
                        const QString& qName,
                        const QXmlAttributes& attrs)
{
    if (qName == "mapping") { // mappings

        double secs = attrs.value("frame").toDouble() / framerate;
        double dist = attrs.value("distance").toDouble() / 1000.0;
        // Old EPM files may have dpf instead of distance tag.
        if (dist == 0) {
            dist = lastDist + attrs.value("dpf").toDouble() * (secs - lastSecs) * framerate / 1000.0;
        }
        double speed = 3600.0*(dist - lastDist)/(secs - lastSecs);
        lastSecs = secs;
        lastDist = dist;
        rideFile->appendPoint(secs, 0, 0, dist, speed, 0, 0, 0,
                              0, 0, 0, 0.0, RideFile::NA, 0.0,
                              0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                              0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                              0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0);

    } else if (qName == "position") { // positions

        double dist = attrs.value("distance").toDouble() / 1000.0;
        double lat = attrs.value("lat").toDouble();
        double lon = attrs.value("lon").toDouble();
        double alt = attrs.value("height").toDouble();
        double secs = rideFile->distanceToTime(dist);
        rideFile->appendOrUpdatePoint(secs, 0, 0, 0, 0, 0, 0, alt,
                                      lon, lat, 0, 0.0, RideFile::NA, 0.0,
                                      0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                      0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                      0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                      0.0, 0.0, 0, false);
    }

    // and clear anything left in the buffer
    buffer.clear();
    return true;
}

bool
EpmParser::endElement(const QString&, const QString&, const QString& qName)
{
    // METADATA
    if(qName == "name") {
        rideFile->setTag("Route", buffer.trimmed());
    } else if(qName == "framerate") {
        framerate = buffer.trimmed().toDouble();
    }
    return true;
}

bool
EpmParser::characters(const QString& str)
{
    buffer += str;
    return true;
}
