/* 
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net)
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

#include "SrmRideFile.h"
#include "srm.h"

#define PI 3.14159265
        
static int srmFileReaderRegistered = 
    RideFileFactory::instance().registerReader("srm", new SrmFileReader());
 
RideFile *SrmFileReader::openRideFile(QFile &file, QStringList &errors) const 
{
    SrmData srmData;
    if (!readSrmFile(file, srmData, errors))
        return NULL;
    RideFile *rideFile = new RideFile(srmData.startTime, srmData.recint);
    QListIterator<SrmDataPoint*> i(srmData.dataPoints);
    while (i.hasNext()) {
        SrmDataPoint *p = i.next();
        double nm = p->watts / 2.0 / PI / p->cad * 60.0;
        rideFile->appendPoint(p->secs, p->cad, p->hr, p->km, 
                              p->kph, nm, p->watts, p->interval);
    }
    return rideFile;
}

