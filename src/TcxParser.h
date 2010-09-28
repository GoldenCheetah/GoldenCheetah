/*
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

class TcxParser : public QXmlDefaultHandler
{
public:
    TcxParser(RideFile* rideFile);

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
    double	lastDistance;
    double	distance;

    int		lap;
    double	power;
    double	cadence;
    double	hr;
    double      lastAltitude;
    double      alt;
    double      lat;
    double      lon;
    double      headwind;
};

#endif // _TcxParser_h

