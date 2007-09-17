/* 
 * $Id: RawFile.cpp,v 1.2 2006/08/11 19:58:07 srhea Exp $
 *
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#include "RawFile.h"
#include "RideFile.h"
#include <assert.h>
#include <math.h>

#define KM_TO_MILES 0.62137119

RawFile *RawFile::readFile(QFile &file, QStringList &errors)
{
    RideFile *rideFile = 
        CombinedFileReader::instance().openRideFile(file, errors);
    if (!rideFile)
        return NULL;
    RawFile *result = new RawFile(file.fileName());
    result->startTime = rideFile->startTime();
    result->rec_int_ms = (unsigned) round(rideFile->recIntSecs() * 1000.0);
    QListIterator<RideFilePoint*> i(rideFile->dataPoints());
    while (i.hasNext()) {
        const RideFilePoint *p1 = i.next();
        RawFilePoint *p2 = new RawFilePoint(
            p1->secs, p1->nm, p1->kph * KM_TO_MILES, p1->watts,
            p1->km * KM_TO_MILES, (int) p1->cad, (int) p1->hr, p1->interval);
        if (result->powerHist.contains(p2->watts))
            result->powerHist[p2->watts] += rideFile->recIntSecs();
        else
            result->powerHist[p2->watts] = rideFile->recIntSecs();
        result->points.append(p2);
    }
    return result;
}

