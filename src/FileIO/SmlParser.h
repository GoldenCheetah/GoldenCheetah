/*
 * Copyright (c) 2015 Alejandro Martinez (amtriathlon@gmail.com)
 *               Based on TcxParser.h
 *
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
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301	 USA
 */

#ifndef _SmlParser_h
#define _SmlParser_h
#include "GoldenCheetah.h"

#include "RideFile.h"
#include <QString>
#include <QDateTime>
#include <QXmlDefaultHandler>
#include "Settings.h"

class SmlParser : public QXmlDefaultHandler
{
public:
    SmlParser(RideFile* rideFile);

    bool startElement( const QString&, const QString&, const QString&,
                       const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );

    bool characters( const QString& );

private:

    RideFile*   rideFile;

    QString     buffer;

    double      lastTime;
    double      time;
    double      lastDistance;
    double      lastLength;
    double      lastLat, lastLon;

    double      alt;
    double      lat;
    double      lon;
    double      hr;
    double      temp;
    double      cad;
    double      speed;
    double      distance;
    double      watts;
    bool        periodic;
    bool        swimming;
    int         lap;
    int         strokes;

    // header processing state
    bool header;
};

#endif // _SmlParser_h

