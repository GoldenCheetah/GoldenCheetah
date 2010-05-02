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

#ifndef _SlfParser_h
#define _SlfParser_h

#include "RideFile.h"
#include <QString>
#include <QDateTime>
#include <QXmlDefaultHandler>

class SlfParser : public QXmlDefaultHandler
{
public:
    SlfParser(RideFile* rideFile);

    bool startElement( const QString&, const QString&, const QString&,
		       const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );

    bool characters( const QString& );

private:

    RideFile*	rideFile;

    QString	buffer;

    QDateTime	start_time;
    QDateTime	stop_time;
    bool	imperial;
    double	secs;
    int		wheelSize;
    double	samplingRate;

    double	lastDistance;
    double	distance;

    int		lap;
    double	hr;
    double      lastAltitude;
    double      alt;
    double	speed;
    double	rotations;
    double	temperature;
    double      pauseSec;
    double      restSec;
};

#endif // _SlfParser_h

