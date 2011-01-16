/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "CalDAV.h"

CalDAV::CalDAV(MainWindow *main) : main(main), mode(None)
{
    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(authenticateResponse(QNetworkReply*)));

    connect(nam, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), this,
        SLOT(userpass(QNetworkReply*,QAuthenticator*)));
//    connect(nam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this,
//        SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));
}

bool
CalDAV::authenticate()
{
    QString url = appsettings->cvalue(main->cyclist, GC_DVURL, "").toString();
    if (url == "") return false; // not configured

    QNetworkRequest request = QNetworkRequest(QUrl(url));
    mode = Events;
    QNetworkReply *reply = nam->get(request);
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(main, tr("CalDAV Calendar url error"), reply->errorString());
        mode = None;
        return false;
    }
    return true;
}

void
CalDAV::authenticateResponse(QNetworkReply *reply)
{
    switch (mode) {
    case Events:
        main->rideCalendar->refreshRemote(reply->readAll());
        break;
    default:
    case Put:
        break;
    }
    mode = None;
}

void
CalDAV::userpass(QNetworkReply*,QAuthenticator*a)
{
    QString user = appsettings->cvalue(main->cyclist, GC_DVUSER, "").toString();
    QString pass = appsettings->cvalue(main->cyclist, GC_DVPASS, "").toString();
    a->setUser(user);
    a->setPassword(pass);
}

void
CalDAV::sslErrors(QNetworkReply*,QList<QSslError>&)
{
}

// convert a rideItem into a vcard ICal Event
static
icalcomponent *createEvent(RideItem *rideItem)
{
    icalcomponent *root = icalcomponent_new(ICAL_VCALENDAR_COMPONENT);
    icalcomponent *event = icalcomponent_new(ICAL_VEVENT_COMPONENT);

    //
    // START DATE
    //
    struct icaltimetype atime;
    QDateTime utc = rideItem->dateTime.toUTC();
    atime.year = utc.date().year();
    atime.month = utc.date().month();
    atime.day = utc.date().day();
    atime.hour = utc.time().hour();
    atime.minute = utc.time().minute();
    atime.second = utc.time().second();
    atime.is_utc = 1; // this is UTC //XXX is_utc is redundant need to investiage further
    atime.is_date = 0; // this is a date AND time
    atime.is_daylight = 0; // no daylight savings - its UTC
    atime.zone = icaltimezone_get_utc_timezone(); // set UTC timezone
    icalproperty *dtstart = icalproperty_new_dtstart(atime);
    icalcomponent_add_property(event, dtstart);

    //
    // DURATION
    //

    // override values?
    QMap<QString,QString> lookup;
    lookup = rideItem->ride()->metricOverrides.value("workout_time");
    int secs = lookup.value("value", "0.0").toDouble();

    // from last - first timestamp?
    if (!rideItem->ride()->dataPoints().isEmpty() && rideItem->ride()->dataPoints().last() != NULL) {
        if (!secs) secs = rideItem->ride()->dataPoints().last()->secs;
    }

    // ok, got secs so now create in vcard
    struct icaldurationtype dur;
    dur.is_neg = 0;
    dur.days = dur.weeks = 0;
    dur.hours = secs/3600;
    dur.minutes = secs%3600/60;
    dur.seconds = secs%60;
    icalcomponent_set_duration(event, dur);

    // set title & description
    QString title = rideItem->ride()->getTag("Title", ""); // *new* 'special' metadata field
    if (title == "") title = rideItem->ride()->getTag("Sport", "") + " Workout";
    icalcomponent_set_summary(event, title.toLatin1());

    // set description using standard stuff
    icalcomponent_set_description(event, rideItem->ride()->getTag("Calendar Text", "").toLatin1());

    // attach ridefile
    // XXX todo -- google doesn't suport
    //             attachments properly .. yet
    //             there is a labs option to use
    //             google docs.. will check hotmail...

    // put the event into root
    icalcomponent_add_component(root, event);
    return root;
}

bool
CalDAV::upload(RideItem *rideItem)
{
    // is this a valid ride?
    if (!rideItem || !rideItem->ride()) return false;

    QString url = appsettings->cvalue(main->cyclist, GC_DVURL, "").toString();
    if (url == "") return false; // not configured

    // lets upload to calendar
    url += rideItem->fileName;
    url += ".ics";

    // form the request
    QNetworkRequest request = QNetworkRequest(QUrl(url));
    request.setRawHeader("Content-Type", "text/calendar");
    request.setRawHeader("Content-Length", "xxxx");

    // create the ICal event
    icalcomponent *vcard = createEvent(rideItem);
    QByteArray vcardtext(icalcomponent_as_ical_string(vcard));
    icalcomponent_free(vcard);

    mode = Put;
    QNetworkReply *reply = nam->put(request, vcardtext);
    if (reply->error() != QNetworkReply::NoError) {
        mode = None;
        QMessageBox::warning(main, tr("CalDAV Calendar url error"), reply->errorString());
        return false;
    }
    return true;
}

