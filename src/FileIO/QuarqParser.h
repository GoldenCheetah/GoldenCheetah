/*
 * Copyright (c) 2009 Mark Rages (mark@quarq.us)
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

#ifndef _QuarqParser_h
#define _QuarqParser_h
#include "GoldenCheetah.h"

#include "RideFile.h"
#include <QString>
#include <QHash>
#include <QDateTime>
#include <QProcess>
#include <QXmlDefaultHandler>

class QuarqParser : public QXmlDefaultHandler
{
public:
    QuarqParser(RideFile* rideFile);

    bool startElement( const QString&, const QString&, const QString&,
		       const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );

    bool characters( const QString& );

private:

    void incrementTime( const double new_time ) ;

    RideFile*	rideFile;

    QString	buf;

    QString     version;

    QDateTime	time;
    double	km;

    double	watts;
    double	cad;
    double	hr;

    double      initial_seconds;
    double      seconds_from_start;

    double      kph;
    double      nm;

    QHash<QString, int> unknown_keys;

};

#endif // _QuarqParser_h

