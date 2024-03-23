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
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _Gc_CalDAV_h
#define _Gc_CalDAV_h
#include "GoldenCheetah.h"

#include <QObject>

// send receive HTTPS requests
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslSocket>

// diag messages
#include <QMessageBox>

// set user and password
#include <QAuthenticator>
#include "Context.h"
#include "Athlete.h"

// work with calDAV protocol
// ...

// GC Settings and Ride Metrics
#include "Settings.h"

// update main ride calendar
#include "ICalendar.h"

// rideitem
#include "RideItem.h"
#include "RideFile.h"
#include "JsonRideFile.h"

// SeasonEvent
#include "Season.h"

// create a UUID
#include <QUuid>

class CalDAV : public QObject
{
    Q_OBJECT
    G_OBJECT

public:
    enum action { Options, PropFind, Put, Get, Events, Report, None };
    typedef enum action ActionType;

    enum type { Standard, Webcal };
    typedef enum type CalDAVType;

    CalDAV(Context *context);

public slots:

    // Query CalDAV server Options
    //bool options(); // not used

    // Query CalDAV server Options
    //bool propfind(); // not used

    // authentication (and refresh all events)
    // -- parameter ignoreErrors run the function in a "stealth" mode - ignoring any error message which might occur e.g. missing configuration
    bool download(bool ignoreErrors=false);

    // Query CalDAV server for events ...
    //bool report(); // not used

    // Upload ride as a VEVENT
    bool upload(RideItem *rideItem);
    // Upload SeasonEvent as a VEVENT
    bool upload(SeasonEvent *seasonEvent);
    // Upload a VEVENT
    bool upload(QByteArray vcardtext);

    // Catch NAM signals ...
    void requestReply(QNetworkReply *reply);
    void userpass(QNetworkReply*r,QAuthenticator*a);
    void sslErrors(QNetworkReply*,QList<QSslError>);

    // enable aynchronous up/download for Google
    // since access token is temporarily valid only, it needs refresh before access to Google CALDAV
    bool doDownload();
    bool doUpload();

    bool getConfig();

private:

    Context *context;
    QNetworkAccessManager *nam;
    CalDAVType calDavType;
    QString url; QString calID;
    QString googleCalDAVurl;
    bool ignoreDownloadErrors;

    ActionType mode;
    QString fileName;
    QByteArray vcardtext;

};
#endif
