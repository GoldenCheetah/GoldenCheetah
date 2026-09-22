/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2026 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include <QBuffer>
#include <QDomDocument>
#include <QByteArray>
#include <QEventLoop>

static QStringList extractResourceNames(const QString &document);
static icalcomponent *createEvent(const CalDAV::CalEntry &calEntry);


//////////////////////////////////////////////////////////////////////////////
// CalDAV::CalEntry

QString
CalDAV::CalEntry::getScopedId
() const
{
    QString calId;
    if (id.isEmpty()) {
        return id;
    } else if (type == EntryType::Season) {
        calId = "season-" + id;
    } else if (type == EntryType::Phase) {
        calId = "phase-" + id;
    } else if (type == EntryType::Event) {
        calId = "event-" + id;
    } else if (type == EntryType::PlannedActivity) {
        calId = "plannedActivity-" + id;
    } else if (type == EntryType::ActualActivity) {
        calId = "actualActivity-" + id;
    } else {
        calId = "UNKNOWN-" + id;
    }
    return calId + idSuffix;
}


bool
CalDAV::CalEntry::getIdParts
(QString input, EntryType * const entryType, QString * const idPart, QString * const originalId)
{
    *originalId = QUrl::fromPercentEncoding(input.toUtf8());
    if (! originalId->endsWith(CalDAV::CalEntry::idSuffix)) {
        return false;
    }
    QString work = originalId->chopped(idSuffix.length());
    int index = work.indexOf('-');
    if (index != -1) {
        QString typePart = work.sliced(0, index);
        if (typePart == "season") {
            *entryType = EntryType::Season;
        } else if (typePart == "phase") {
            *entryType = EntryType::Phase;
        } else if (typePart == "event") {
            *entryType = EntryType::Event;
        } else if (typePart == "plannedActivity") {
            *entryType = EntryType::PlannedActivity;
        } else if (typePart == "actualActivity") {
            *entryType = EntryType::ActualActivity;
        } else {
            return false;
        }
        *idPart = work.sliced(index + 1);
        return true;
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////////
// CalDAV

CalDAV::CalDAV
(Context *context, CloudService *cloudService)
: context(context), cloudService(cloudService)
{
    nam = new QNetworkAccessManager(this);
    connect(nam, &QNetworkAccessManager::finished, this, &CalDAV::requestReply);
    connect(nam, &QNetworkAccessManager::sslErrors, this, &CalDAV::sslErrors);
}


bool
CalDAV::isConfigured
() const
{
    return CalDAVAuth::isConfigured(cloudService);
}


bool
CalDAV::uploadBlocking
(const CalEntry &calEntry, QString *errorOut)
{
    QLocale locale;
    QEventLoop loop;
    bool ok = false;
    QString err;
    const QString calId = calEntry.getScopedId();

    QMetaObject::Connection conn = connect(this, &CalDAV::uploadFinished, this, [&](QString id, bool success, QString errorString) {
            if (id != calId) {
                return;
            }
            ok = success;
            err = errorString;
            loop.quit();
        }, Qt::QueuedConnection);
    upload(calEntry);

    loop.exec();
    disconnect(conn);

    if (errorOut) {
        *errorOut = err;
    }
    return ok;
}


bool
CalDAV::removeBlocking
(QString id, QString *errorOut)
{
    QEventLoop loop;
    bool ok = false;
    QString err;

    QMetaObject::Connection conn = connect(this, &CalDAV::uploadFinished, this, [&](QString replyId, bool success, QString errorString) {
            if (replyId != id) {
                return;
            }
            ok = success;
            err = errorString;
            loop.quit();
        }, Qt::QueuedConnection);

    remove(id);

    loop.exec();
    disconnect(conn);

    if (errorOut) {
        *errorOut = err;
    }
    return ok;
}


bool
CalDAV::listBlocking
(QStringList *namesOut, QString *errorOut)
{
    QEventLoop loop;
    bool ok = false;
    QString err;
    QStringList names;

    QMetaObject::Connection conn = connect(this, &CalDAV::listFinished, this, [&](QStringList resourceNames, bool success, QString errorString) {
            names = resourceNames;
            ok = success;
            err = errorString;
            loop.quit();
        }, Qt::QueuedConnection);

    list();

    loop.exec();
    disconnect(conn);

    if (namesOut) {
        *namesOut = names;
    }
    if (errorOut) {
        *errorOut = err;
    }
    return ok;
}


QString
CalDAV::collectionUrl
() const
{
    return CalDAVAuth::collectionUrl(cloudService);
}


QString
CalDAV::resourceUrl
(QString id) const
{
    return collectionUrl() + id + ".ics";
}


void
CalDAV::applyAuth
(QNetworkRequest &request) const
{
    CalDAVAuth::applyAuth(cloudService, request);
}


//
// utility function to create a VCALENDAR from a single (planned) RideItem
//

static icalcomponent*
createEvent
(const CalDAV::CalEntry &calEntry)
{
    // calendar
    icalcomponent *root = icalcomponent_new(ICAL_VCALENDAR_COMPONENT);

    // calendar version
    icalproperty *prodid = icalproperty_new_prodid("-//GoldenCheetah//GoldenCheetah Calendar//EN");
    icalcomponent_add_property(root, prodid);

    icalproperty *version = icalproperty_new_version("2.0");
    icalcomponent_add_property(root, version);

    icalcomponent *event = icalcomponent_new(ICAL_VEVENT_COMPONENT);

    icalproperty *uid = icalproperty_new_uid(calEntry.getScopedId().toLatin1());
    icalcomponent_add_property(event, uid);

    // DTSTAMP is always a UTC DATE-TIME.
    // It is NOT the event's start time.
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();

    struct icaltimetype stamp = icaltime_null_time();
    stamp.year = nowUtc.date().year();
    stamp.month = nowUtc.date().month();
    stamp.day = nowUtc.date().day();
    stamp.hour = nowUtc.time().hour();
    stamp.minute = nowUtc.time().minute();
    stamp.second = nowUtc.time().second();
    stamp.is_date = 0;
    stamp.is_daylight = 0;
    stamp.zone = icaltimezone_get_utc_timezone();

    icalcomponent_add_property(event, icalproperty_new_dtstamp(stamp));

    if (calEntry.allDay) {
        // ALL-DAY EVENT - Date only; DTEND is exclusive
        const QDate startDate = calEntry.start.date();
        const QDate endDate = calEntry.end.addDays(1).date();

        struct icaltimetype start = icaltime_null_time();
        start.year = startDate.year();
        start.month = startDate.month();
        start.day = startDate.day();
        start.is_date = 1;
        icalcomponent_add_property(event, icalproperty_new_dtstart(start));

        struct icaltimetype end = icaltime_null_time();
        end.year = endDate.year();
        end.month = endDate.month();
        end.day = endDate.day();
        end.is_date = 1;
        icalcomponent_add_property(event, icalproperty_new_dtend(end));
    } else {
        // TIMED EVENT - Store as UTC DATE-TIME values.
        const QDateTime startUtc = calEntry.start.toUTC();
        const QDateTime endUtc = calEntry.end.toUTC();

        struct icaltimetype start = icaltime_null_time();
        start.year = startUtc.date().year();
        start.month = startUtc.date().month();
        start.day = startUtc.date().day();
        start.hour = startUtc.time().hour();
        start.minute = startUtc.time().minute();
        start.second = startUtc.time().second();
        start.is_date = 0;
        start.is_daylight = 0;
        start.zone = icaltimezone_get_utc_timezone();
        icalcomponent_add_property(event, icalproperty_new_dtstart(start));

        struct icaltimetype end = icaltime_null_time();
        end.year = endUtc.date().year();
        end.month = endUtc.date().month();
        end.day = endUtc.date().day();
        end.hour = endUtc.time().hour();
        end.minute = endUtc.time().minute();
        end.second = endUtc.time().second();
        end.is_date = 0;
        end.is_daylight = 0;
        end.zone = icaltimezone_get_utc_timezone();
        icalcomponent_add_property(event, icalproperty_new_dtend(end));
    }

    icalcomponent_set_summary(event, calEntry.title.toUtf8().constData());
    icalcomponent_set_description(event, calEntry.description.toUtf8().constData());

    icalproperty *categories = icalproperty_new_categories("GC-PLANNED-WORKOUT");
    icalcomponent_add_property(event, categories);
    icalproperty *transp = icalproperty_new_transp(ICAL_TRANSP_OPAQUE);
    icalcomponent_add_property(event, transp);

    // put the event into root
    icalcomponent_add_component(root, event);
    return root;
}


static QStringList
extractResourceNames
(const QString &document)
{
    QStringList names;

    QDomDocument doc;
    if (document.isEmpty() || ! doc.setContent(document)) {
        return names;
    }

    QDomNode multistatus = doc.documentElement();
    if (multistatus.isNull()) {
        return names;
    }

    QString prefix;
    if (multistatus.nodeName().contains(":")) {
        prefix = multistatus.nodeName().section(':', 0, 0) + ":";
    }

    for (QDomNode response = multistatus.firstChildElement(prefix + "response"); ! response.isNull() && response.nodeName() == (prefix + "response"); response = response.nextSiblingElement(prefix + "response")) {
        QDomNode href = response.firstChildElement(prefix + "href");
        if (href.isNull()) {
            continue;
        }

        QString path = href.toElement().text();
        QString name = path.section('/', -1, -1, QString::SectionSkipEmpty);
        if (name.endsWith(".ics", Qt::CaseInsensitive)) {
            name.chop(4);
        }
        if (! name.isEmpty()) {
            names << name;
        }
    }

    return names;
}


bool
CalDAV::upload
(const CalEntry &calEntry)
{
    QString calId = calEntry.getScopedId();
    if (calId.isEmpty()) {
        emit uploadFinished(QString(), false, tr("Missing id"));
        return false;
    }

    icalcomponent *vcard = createEvent(calEntry);
    QByteArray vcardtext(icalcomponent_as_ical_string(vcard));
    icalcomponent_free(vcard);

    return put(calId, vcardtext);
}


bool
CalDAV::remove
(const CalEntry &calEntry)
{
    QString calId = calEntry.getScopedId();
    if (calId.isEmpty()) {
        return false;
    }
    return remove(calId);
}


bool
CalDAV::put
(QString id, QByteArray vcardtext)
{
    if (! isConfigured()) {
        emit uploadFinished(id, false, tr("CalDAV URL, username or password is missing in preferences"));
        return false;
    }

    QNetworkRequest request = QNetworkRequest(QUrl(resourceUrl(id)));
    request.setRawHeader("Content-Type", "text/calendar; charset=\"utf-8\"");
    request.setRawHeader("Content-Length", QString::number(vcardtext.size()).toLatin1());
    applyAuth(request);

    QNetworkReply *reply = nam->put(request, vcardtext);
    inFlight.insert(reply, { Op::Put, id });
    return true;
}


bool
CalDAV::remove
(QString id)
{
    if (! isConfigured()) {
        emit uploadFinished(id, false, tr("CalDAV URL, username or password is missing in preferences"));
        return false;
    }

    QNetworkRequest request = QNetworkRequest(QUrl(resourceUrl(id)));
    applyAuth(request);

    QNetworkReply *reply = nam->deleteResource(request);
    inFlight.insert(reply, { Op::Delete, id });
    return true;
}


bool
CalDAV::list()
{
    if (! isConfigured()) {
        emit listFinished(QStringList(), false, tr("CalDAV URL, username or password is missing in preferences"));
        return false;
    }

    QByteArray body = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
                       "<D:propfind xmlns:D=\"DAV:\">"
                       " <D:prop>"
                       "   <D:getetag/>"
                       " </D:prop>"
                       "</D:propfind>\r\n";

    QNetworkRequest request = QNetworkRequest(QUrl(collectionUrl()));
    request.setRawHeader("Depth", "1");
    request.setRawHeader("Content-Type", "application/xml; charset=\"utf-8\"");
    request.setRawHeader("Content-Length", QString::number(body.size()).toLatin1());
    applyAuth(request);

    QNetworkReply *reply = nam->sendCustomRequest(request, "PROPFIND", body);
    inFlight.insert(reply, { Op::List, QString() });
    return true;
}


void
CalDAV::requestReply
(QNetworkReply *reply)
{
    RequestContext ctx = inFlight.take(reply);
    QString response = QString::fromUtf8(reply->readAll());
    bool ok = (reply->error() == QNetworkReply::NoError);
    QString err = ok ? QString() : reply->errorString();

    switch (ctx.op) {
    case Op::Put:
    case Op::Delete:
        emit uploadFinished(ctx.id, ok, err);
        break;
    case Op::List:
        if (ok) {
            emit listFinished(extractResourceNames(response), true, QString());
        } else {
            emit listFinished(QStringList(), false, err);
        }
        break;
    }

    reply->deleteLater();
}


void
CalDAV::sslErrors
(QNetworkReply *reply, QList<QSslError> errors)
{
    CloudService::sslErrors(context->mainWindow, reply, errors);
}
