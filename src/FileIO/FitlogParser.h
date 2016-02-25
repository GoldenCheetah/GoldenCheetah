/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _FitlogParser_h
#define _FitlogParser_h
#include "GoldenCheetah.h"

#include "RideFile.h"
#include <QString>
#include <QDateTime>
#include <QXmlDefaultHandler>
#include "Settings.h"

class FitlogParser : public QXmlDefaultHandler
{
public:
    FitlogParser(RideFile* rideFile, QList<RideFile*>*rides);

    bool startElement( const QString&, const QString&, const QString&,
		       const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );
    bool characters( const QString& );

    // for deriving distance from GPS
    double distanceBetween(double lat1, double lon1, double lat2, double lon2);
    double distanceBetween(double lat1, double lon1, double alt1,
                           double lat2, double lon2, double alt2);

    RideFile*	rideFile;
    QList<RideFile*> *rides; // when parsed multiple rides

private:

    QString	buffer;

    QDateTime start_time; // when the ride started
    int lap;              // lap number
    int	track_offset;     // offset for point.secs

    bool first; // first ride found, when it may contain collections!
};

#endif // _FitlogParser_h
