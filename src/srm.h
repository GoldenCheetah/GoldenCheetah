/* 
 * $Id: RawFile.h,v 1.3 2006/08/11 19:58:07 srhea Exp $
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

#ifndef _srm_h
#define _srm_h

#include <QDataStream>
#include <QDate>
#include <QFile>
#include <QStringList>

struct SrmDataPoint 
{
    int cad, hr, watts, interval;
    double kph, km, secs;
};

struct SrmData 
{
    QDateTime startTime;
    double recint;
    QList<SrmDataPoint*> dataPoints;
    ~SrmData() {
        QListIterator<SrmDataPoint*> i(dataPoints);
        while (i.hasNext()) 
            delete i.next();
    }
};

bool readSrmFile(QFile &file, SrmData &data, QStringList &errorStrings);

#endif // _srm_h

