/*
 * Copyright (c) 2010 Frank Zschockelt (rox_sigma@freakysoft.de)
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

#ifndef _SmfParser_h
#define _SmfParser_h

#include "RideFile.h"
#include <QString>
#include <QDateTime>
#include <QXmlDefaultHandler>

class SmfParser : public QXmlDefaultHandler
{
public:
    SmfParser(RideFile* rideFile);

    bool startElement( const QString&, const QString&, const QString&,
		       const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );

    bool characters( const QString& );

private:

    RideFile*	rideFile;

    QString	buffer;

    bool	imperial;
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

#endif // _SmfParser_h

