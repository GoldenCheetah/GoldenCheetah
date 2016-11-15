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

#include "StravaRideFile.h"
#include "StravaParser.h"

static int stravaFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "strava", "Strava activity streams", new StravaFileReader());

RideFile *StravaFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    (void) errors;
    RideFile *rideFile = new RideFile();
    rideFile->setDeviceType("Strava activity");
    rideFile->setFileFormat("Strava activity streams (strava)");

    StravaParser parser(rideFile, &file);
    parser.parse();

    return rideFile;
}
