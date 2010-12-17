/*
 * Copyright (c) 2010 Greg Lonnon (greg.lonnon@gmail.com) copied from
 * TcxParser.cpp
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net),
 *		      J.T Conklin (jtc@acorntoolworks.com)
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

#ifndef _TcxParser_h
#define _TcxParser_h

#include "RideFile.h"
#include <QString>
#include <QDateTime>
#include <QXmlDefaultHandler>
#include "Settings.h"

class GpxParser : public QXmlDefaultHandler
{
public:
    GpxParser(RideFile* rideFile);

    bool startElement( const QString&, const QString&, const QString&,
		       const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );

    bool characters( const QString& );

private:

    RideFile*	rideFile;

    QString	buffer;
    QVariant    isGarminSmartRecording;
    QVariant    GarminHWM;

    QDateTime	start_time;
    QDateTime	last_time;
    QDateTime	time;
    double	distance;
    double      lastLat, lastLon;

    double      alt;
    double      lat;
    double      lon;

    // set to false after the first time element is seen (not in metadata)
    bool firstTime;
    // throw away the metadata, it doesn't look useful
    bool metadata;

};

#endif // _TcxParser_h

