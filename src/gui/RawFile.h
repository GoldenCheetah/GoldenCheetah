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

#ifndef _GC_RawFile_h
#define _GC_RawFile_h 1

#include <QDateTime>
#include <QFile>
#include <QList>
#include <QMap>
#include <QStringList>

struct RawFilePoint 
{
    double secs, nm, mph, watts, miles;
    unsigned cad, hr, interval;
    RawFilePoint(double secs, double nm, double mph, double watts, 
                 double miles, unsigned cad, unsigned hr, unsigned interval) :
        secs(secs), nm(nm), mph(mph), watts(watts), miles(miles), 
        cad(cad), hr(hr), interval(interval) {}
};

class RawFile
{
    private:

        QFile file;

        RawFile(QString fileName) {
            file.setFileName(fileName);
        }

    public:

        QDateTime startTime;
        int rec_int;
        QList<RawFilePoint*> points;
        QMap<double,double> powerHist;
        static RawFile *readFile(QFile &file, QStringList &errors);
};

#endif // _GC_RawFile_h

