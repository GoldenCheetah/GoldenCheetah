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
#include "GoldenCheetah.h"

#include "RideFile.h"
#include <QString>
#include <QDateTime>
#include <QXmlDefaultHandler>
#include "Settings.h"
#include "locale.h" // for LC_LOCALE definition used in strtod

class TcxParser : public QXmlDefaultHandler
{

public:

    TcxParser(RideFile* rideFile, QList<RideFile*>*rides);

    bool startElement( const QString&, const QString&, const QString&, const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );
    bool characters( const QString& );

    RideFile*	rideFile;
    QList<RideFile*> *rides; // when parsed multiple rides

private:

    QString	buffer;
    QVariant isGarminSmartRecording;
    QVariant GarminHWM;

    QDateTime start_time;
    QDateTime last_time;
    QDateTime time;

    int lap;
    QDateTime lap_start_time;
    double lapSecs; // for pause intervals in pool swimming files
    enum { ltManual = 0, ltDistance = 1, ltLocation = 2, ltTime = 3, ltHeartRate = 4, ltLast = 5} lapTrigger;
    int lapCount[ltLast];

    double last_distance;
    double distance;
    enum { NotSwim, MayBeSwim, Swim } swim; // to detect pool swimming files
    double lastLength; // for pool swimming files
    XDataSeries *swimXdata; // length-by-length pool swim XData

    bool   first; // first ride found, when it may contain collections!
    bool   creator;
    bool   training; // detailed-sport-info

    double power;
    double cadence;
    double rcad;
    double hr;
    double speed;
    double torque;
    double alt;
    double lat;
    double lon;
    double headwind;
    double lrbalance;
    double lte;
    double rte;
    double lps;
    double rps;
    double secs;
    bool   badgps;
};

#endif // _TcxParser_h
