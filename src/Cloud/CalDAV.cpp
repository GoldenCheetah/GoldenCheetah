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

#include "Secrets.h"

#include "CalDAV.h"
#include "MainWindow.h"
#include "Athlete.h"

CalDAV::CalDAV(Context *context) : context(context), mode(None)
{
    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestReply(QNetworkReply*)));

    connect(nam, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), this,
            SLOT(userpass(QNetworkReply*,QAuthenticator*)));
    connect(nam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this,
            SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));

    googleCalDAVurl = "https://apidata.googleusercontent.com/caldav/v2/%1/events/";

    getConfig();
}


bool
CalDAV::getConfig() {

    int t = appsettings->cvalue(context->athlete->cyclist, GC_DVCALDAVTYPE, "0").toInt();
    if (t == 0) {
        calDavType = Standard;
    } else {
        calDavType = Google;
    };

    if (calDavType == Standard) {
        url = appsettings->cvalue(context->athlete->cyclist, GC_DVURL, "").toString();
    } else { // calDavType = GOOGLE
        calID = appsettings->cvalue(context->athlete->cyclist, GC_DVGOOGLE_CALID, "").toString();
        url =  googleCalDAVurl.arg(calID);
    }

    // check if we have an useful URL (not space and not the Google Default without CalID
    if ((url == "" && calDavType == Standard) || (calID == "" && calDavType == Google)) {
        return false;
    }

    return true;
}


//
// GET event directory listing
//

bool
CalDAV::download(bool ignoreErrors)
{
    ignoreDownloadErrors = ignoreErrors;
    mode = Events;

    if (!getConfig()) {
        if (!ignoreDownloadErrors) {
             QMessageBox::warning(context->mainWindow, tr("Missing Preferences"), tr("CalID or CalDAV Url is missing in preferences"));
        }
        mode = None;
        return false;
    }

    if (calDavType == Standard) {
        return doDownload();
    } else { // calDavType = GOOGLE
        // after having the token the function defined in "mode" will be executed
        requestGoogleAccessTokenToExecute();
    }
    return true;
}

bool
CalDAV::doDownload()
{

    QNetworkRequest request = QNetworkRequest(QUrl(url));

    QByteArray *queryText = new QByteArray( "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
                                            "<C:calendar-query xmlns:D=\"DAV:\""
                                            "                 xmlns:C=\"urn:ietf:params:xml:ns:caldav\">"
                                            " <D:prop>"
                                            "   <D:getetag/>"
                                            "   <C:calendar-data/>"
                                            " </D:prop>"
                                            " <C:filter>"
                                            "   <C:comp-filter name=\"VCALENDAR\">"
                                            "     <C:comp-filter name=\"VEVENT\">"
                                            "       <C:time-range end=\"20200101T000000Z\" start=\"20000101T000000Z\"/>"
                                            "     </C:comp-filter>"
                                            "   </C:comp-filter>"
                                            " </C:filter>"
                                            "</C:calendar-query>\r\n");

    request.setRawHeader("Depth", "0");
    request.setRawHeader("Content-Type", "application/xml; charset=\"utf-8\"");
    request.setRawHeader("Content-Length", (QString("%1").arg(queryText->size())).toLatin1());
    if (calDavType == Google && googleCalendarAccessToken != "") {
        request.setRawHeader("Authorization", "Bearer "+googleCalendarAccessToken );
    }


    QBuffer *query = new QBuffer(queryText);

    mode = Events;
    QNetworkReply *reply = nam->sendCustomRequest(request, "REPORT", query);
    if (reply->error() != QNetworkReply::NoError) {
        if (!ignoreDownloadErrors) {
            QMessageBox::warning(context->mainWindow, tr("CalDAV REPORT url error"), reply->errorString());
        }
        mode = None;
        return false;
    }
    return true;
}

//
// Get OPTIONS available
//
//bool
//CalDAV::options()
//{
//    QString url = appsettings->cvalue(context->athlete->cyclist, GC_DVURL, "").toString();
//    if (url == "") return false; // not configured

//    QNetworkRequest request = QNetworkRequest(QUrl(url));


//    QByteArray *queryText = new QByteArray("<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
//                                           "<D:options xmlns:D=\"DAV:\">"
//                                           "  <C:calendar-home-set xmlns:C=\"urn:ietf:params:xml:ns:caldav\"/>"
//                                           "</D:options>");

//    request.setRawHeader("Depth", "0");
//    request.setRawHeader("Content-Type", "text/xml; charset=\"utf-8\"");
//    request.setRawHeader("Content-Length", (QString("%1").arg(queryText->size())).toLatin1());

//    QBuffer *query = new QBuffer(queryText);

//    mode = Options;
//    QNetworkReply *reply = nam->sendCustomRequest(request, "OPTIONS", query);
//    if (reply->error() != QNetworkReply::NoError) {
//        QMessageBox::warning(context->mainWindow, tr("CalDAV OPTIONS url error"), reply->errorString());
//        mode = None;
//        return false;
//    }
//    return true;
//}

//
// Get URI Properties via PROPFIND
//
//bool
//CalDAV::propfind()
//{
//    QString url = appsettings->cvalue(context->athlete->cyclist, GC_DVURL, "").toString();
//    if (url == "") return false; // not configured

//    QNetworkRequest request = QNetworkRequest(QUrl(url));


//    QByteArray *queryText = new QByteArray( "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
//                                            "<D:propfind xmlns:D=\"DAV:\""
//                                            "                 xmlns:C=\"urn:ietf:params:xml:ns:caldav\">"
//                                            "  <D:prop>"
//                                            "    <D:displayname/>"
//                                            "    <C:calendar-timezone/> "
//                                            "    <C:supported-calendar-component-set/> "
//                                            "  </D:prop>"
//                                            "</D:propfind>\r\n");

//    request.setRawHeader("Content-Type", "text/xml; charset=\"utf-8\"");
//    request.setRawHeader("Content-Length", (QString("%1").arg(queryText->size())).toLatin1());
//    request.setRawHeader("Depth", "0");

//    QBuffer *query = new QBuffer(queryText);

//    mode = PropFind;
//    QNetworkReply *reply = nam->sendCustomRequest(request, "PROPFIND" , query);
//    if (reply->error() != QNetworkReply::NoError) {
//        QMessageBox::warning(context->mainWindow, tr("CalDAV OPTIONS url error"), reply->errorString());
//        mode = None;
//        return false;
//    }
//    return true;
//}


////
//// REPORT of "all" VEVENTS
////
//bool
//CalDAV::report()
//{
//    QString url = appsettings->cvalue(context->athlete->cyclist, GC_DVURL, "").toString();
//    if (url == "") return false; // not configured

//    QNetworkRequest request = QNetworkRequest(QUrl(url));
//    QByteArray *queryText = new QByteArray("<x1:calendar-query xmlns:x1=\"urn:ietf:params:xml:ns:caldav\">"
//                     "<x0:prop xmlns:x0=\"DAV:\">"
//                     "<x0:getetag/>"
//                     "<x1:calendar-data/>"
//                     "</x0:prop>"
//                     "<x1:filter>"
//                     "<x1:comp-filter name=\"VCALENDAR\">"
//                     "<x1:comp-filter name=\"VEVENT\">"
//                     "<x1:time-range end=\"21001231\" start=\"20000101T000000Z\"/>"
//                     "</x1:comp-filter>"
//                     "</x1:comp-filter>"
//                     "</x1:filter>"
//                     "</x1:calendar-query>");

//    QBuffer *query = new QBuffer(queryText);

//    mode = Report;
//    QNetworkReply *reply = nam->sendCustomRequest(request, "REPORT", query);
//    if (reply->error() != QNetworkReply::NoError) {
//        QMessageBox::warning(context->mainWindow, tr("CalDAV REPORT url error"), reply->errorString());
//        mode = None;
//        return false;
//    }
//    return true;
//}

// utility function to create a VCALENDAR from a single RideItem
static
icalcomponent *createEvent(RideItem *rideItem)
{
    // calendar
    icalcomponent *root = icalcomponent_new(ICAL_VCALENDAR_COMPONENT);

    // calendar version
    icalproperty *version = icalproperty_new_version("2.0");
    icalcomponent_add_property(root, version);


    icalcomponent *event = icalcomponent_new(ICAL_VEVENT_COMPONENT);

    //
    // Unique ID
    //
    QString id = rideItem->ride()->id();
    if (id == "") {
        id = QUuid::createUuid().toString() + "@" + "goldencheetah.org";
        rideItem->ride()->setId(id);
        rideItem->notifyRideMetadataChanged();
        rideItem->setDirty(true); // need to save this!
    }
    icalproperty *uid = icalproperty_new_uid(id.toLatin1());
    icalcomponent_add_property(event, uid);

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
    //atime.is_utc = 1; // this is UTC is_utc is redundant but kept for completeness
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
    // google doesn't support attachments yet. There is a labs option to use google docs
    // but it is only available to Google Apps customers.

    // put the event into root
    icalcomponent_add_component(root, event);
    return root;
}

// utility function to create a VCALENDAR from a single SeasonEvent
static
icalcomponent *createEvent(SeasonEvent *seasonEvent)
{
    // calendar
    icalcomponent *root = icalcomponent_new(ICAL_VCALENDAR_COMPONENT);

    // calendar version
    icalproperty *version = icalproperty_new_version("2.0");
    icalcomponent_add_property(root, version);


    icalcomponent *event = icalcomponent_new(ICAL_VEVENT_COMPONENT);

    //
    // Unique ID
    //
    QString id = seasonEvent->id;
    if (id == "") {
        id = QUuid::createUuid().toString() + "@" + "goldencheetah.org";
        seasonEvent->id = id;
    }
    icalproperty *uid = icalproperty_new_uid(id.toLatin1());
    icalcomponent_add_property(event, uid);

    //
    // START DATE
    //
    struct icaltimetype atime;
    atime.year = seasonEvent->date.year();
    atime.month = seasonEvent->date.month();
    atime.day = seasonEvent->date.day();
    atime.hour = 0;
    atime.minute = 0;
    atime.second = 0;
    //atime.is_utc = 1; // this is UTC is_utc is redundant but kept for completeness
    atime.is_date = 1; // this is a date
    atime.is_daylight = 0; // no daylight savings - its UTC
    atime.zone = icaltimezone_get_utc_timezone(); // set UTC timezone
    icalproperty *dtstart = icalproperty_new_dtstart(atime);
    icalcomponent_add_property(event, dtstart);

    //
    // PRIORITY
    //
    if (seasonEvent->priority > 0) {
        icalproperty* priority = icalproperty_new_priority(seasonEvent->priority);
        icalcomponent_add_property(event, priority);
    }


    // set title & description
    icalcomponent_set_summary(event, seasonEvent->name.toLatin1());
    icalcomponent_set_description(event, seasonEvent->description.toLatin1());

    // put the event into root
    icalcomponent_add_component(root, event);
    return root;
}

// extract <calendar-data> entries and concatenate
// into a single string. This is from a query response
// where the VEVENTS are embedded within an XML document
static QString extractComponents(QString document)
{
    QString returning = "";

    // parse the document and extract the multistatus node (there is only one of those)
    QDomDocument doc;
    if (document == "" || doc.setContent(document) == false) return "";
    QDomNode multistatus = doc.documentElement();
    if (multistatus.isNull())  return "";

    // Google Calendar retains the namespace prefix in the results
    // Apple MobileMe doesn't. This means the element names will
    // possibly need a prefix...
    // Google CalDav API V2 does not deliver C: but caldav: as a prefix
    // so try both - just in case - to get the data

    QString Dprefix = "";
    QString Cprefix = "";
    QString Cprefix_Google = "";
    if (multistatus.nodeName().startsWith("D:")) {
        Dprefix = "D:";
        Cprefix = "C:";
        Cprefix_Google = "caldav:";
    }

    // read all the responses within the multistatus
    for (QDomNode response = multistatus.firstChildElement(Dprefix + "response");
         response.nodeName() == (Dprefix + "response"); response = response.nextSiblingElement(Dprefix + "response")) {

        // skate over the nest of crap to get at the calendar-data
        QDomNode propstat = response.firstChildElement(Dprefix + "propstat");
        QDomNode prop = propstat.firstChildElement(Dprefix + "prop");
        QDomNode calendardata = prop.firstChildElement(Cprefix + "calendar-data");
        if (calendardata.isNull()) {
            calendardata = prop.firstChildElement(Cprefix_Google + "calendar-data");
        };

        // extract the calendar entry - top and tail the other crap
        QString text = calendardata.toElement().text();
        int start = text.indexOf("BEGIN:VEVENT");
        int stop = text.indexOf("END:VEVENT");

        if (start == -1 || stop == -1) continue;

        returning += text.mid(start, stop-start+10) + "\n";
    }
    return returning;
}

//
// PUT a ride item
//

bool
CalDAV::upload(RideItem *rideItem)
{
    // is this a valid ride?
    if (!rideItem || !rideItem->ride()) return false;

    fileName = rideItem->fileName;
    // create the ICal event
    icalcomponent *vcard = createEvent(rideItem);
    QByteArray vcardtext(icalcomponent_as_ical_string(vcard));
    icalcomponent_free(vcard);

    return upload(vcardtext);
}

//
// PUT a SeasonEvent
//

bool
CalDAV::upload(SeasonEvent *seasonEvent)
{
    fileName = seasonEvent->id;
    // create the ICal event
    icalcomponent *vcard = createEvent(seasonEvent);
    QByteArray vcardtxt(icalcomponent_as_ical_string(vcard));
    icalcomponent_free(vcard);

    return upload(vcardtxt);
}

bool
CalDAV::upload(QByteArray vcardtxt)
{
    if (!getConfig()) {
        QMessageBox::warning(context->mainWindow, tr("Missing Preferences"), tr("CalID or CalDAV Url is missing in preferences"));
        return false;
    }
    mode = Put;
    vcardtext = vcardtxt;
    if (calDavType == Standard) {
        return doUpload();
    } else { // calDavType = GOOGLE
        // after having the token the function defined in "mode" will be executed
        requestGoogleAccessTokenToExecute();
    }
    return true;
}


bool
CalDAV::doUpload()
{
    // if URL does not end with "/" - just  add it (for convenience)
    if (!url.endsWith("/")) {
        url += "/";
    }
    // lets upload to calendar
    url += fileName;
    url += ".ics";

    // form the request
    QNetworkRequest request = QNetworkRequest(QUrl(url));
    request.setRawHeader("Content-Type", "text/calendar");
    request.setRawHeader("Content-Length", "xxxx");
    if (calDavType == Google && googleCalendarAccessToken != "") {
        request.setRawHeader("Authorization", "Bearer "+googleCalendarAccessToken );
    }

    mode = Put;
    QNetworkReply *reply = nam->put(request, vcardtext);
    if (reply->error() != QNetworkReply::NoError) {
        mode = None;
        QMessageBox::warning(context->mainWindow, tr("CalDAV Calendar url error"), reply->errorString());
        return false;
    }
    return true;
}

//
// All queries/commands respond here
//
void
CalDAV::requestReply(QNetworkReply *reply)
{
    QString response = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        if (!(mode == Events && ignoreDownloadErrors)) {
            QMessageBox::warning(context->mainWindow, tr("CalDAV Calendar API reply error"), reply->errorString());
        }
        mode = None;
        return; // silently
    }

    switch (mode) {
    case Report:
    case Events:
        context->athlete->rideCalendar->refreshRemote(extractComponents(response));
        mode = None;
        break;
    default:
    case Options:
    case PropFind:
        //nothing at the moment
        mode = None;
        break;
    case Put:
        //refresh local calendar
        mode = None;
        download(false);
        break;
    }
}

//
// Provide user credentials, called when receive a 401
//
void
CalDAV::userpass(QNetworkReply*,QAuthenticator*a)
{
    QString user = appsettings->cvalue(context->athlete->cyclist, GC_DVUSER, "").toString();
    QString pass = appsettings->cvalue(context->athlete->cyclist, GC_DVPASS, "").toString();
    a->setUser(user);
    a->setPassword(pass);
}

//
// Trap SSL errors
//
void
CalDAV::sslErrors(QNetworkReply* reply ,QList<QSslError> errors)
{
    QString errorString = "";
    foreach (const QSslError e, errors ) {
        if (!errorString.isEmpty())
            errorString += ", ";
        errorString += e.errorString();
    }
    if (!(mode == Events && ignoreDownloadErrors)) {
        QMessageBox::warning(context->mainWindow, tr("HTTP"), tr("SSL error(s) has occurred: %1").arg(errorString));
        mode = None;
        reply->ignoreSslErrors();
    }
}



//
// gets Google Calendar Access Token (from Refresh Token)
//
void
CalDAV::requestGoogleAccessTokenToExecute() {

    QString refresh_token = appsettings->cvalue(context->athlete->cyclist, GC_GOOGLE_CALENDAR_REFRESH_TOKEN, "").toString();
    if (refresh_token == "") {
        if (!(mode == Events && ignoreDownloadErrors)) {
            QMessageBox::warning(context->mainWindow, tr("Missing Preferences"), tr("Authorization for Google CalDAV is missing in preferences"));
        }
        mode = None;
        return;
    }

    // get a valid access token
    QByteArray data;
    QUrlQuery params;
    QString urlstr = "https://www.googleapis.com/oauth2/v3/token?";
    params.addQueryItem("client_id", GC_GOOGLE_CALENDAR_CLIENT_ID);
#ifdef GC_GOOGLE_CALENDAR_CLIENT_SECRET
    params.addQueryItem("client_secret", GC_GOOGLE_CALENDAR_CLIENT_SECRET);
#endif
    params.addQueryItem("refresh_token", refresh_token);
    params.addQueryItem("grant_type", "refresh_token");

    data.append(params.query(QUrl::FullyEncoded));

    // get a new Access Token (since they are just temporarily valid)
    QUrl url = QUrl(urlstr);
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    // now get the final token
    googleNetworkAccessManager = new QNetworkAccessManager(this);
    connect(googleNetworkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(googleNetworkRequestFinished(QNetworkReply*)));
    googleNetworkAccessManager->post(request, data);

}

//
// extract the token and call the requested CALDAV function
//
void
CalDAV::googleNetworkRequestFinished(QNetworkReply* reply)   {

    googleCalendarAccessToken = "";
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray payload = reply->readAll(); // JSON
        QByteArray token_type = "\"access_token\":";
        int token_length = 15;

        // get the token
        int at = payload.indexOf(token_type);
        if (at >=0 ) {
            int from = at + token_length; // first char after ":"
            int next = payload.indexOf("\"", from);
            from = next + 1;
            int to = payload.indexOf("\"", from);
            googleCalendarAccessToken = payload.mid(from, to-from);

        } else { // something failed

            if (!(mode == Events && ignoreDownloadErrors)) {
                QMessageBox::warning(context->mainWindow, tr("Authorization Error"), tr("Error requesting access token"));
            };
            mode = None;
            return;
        }
    }

    // now we have a token and can do the requested jobs
    if (mode == Put) doUpload();
    if (mode == Events) doDownload();
}

