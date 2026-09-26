/*
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

#include "GoogleCalendarDiscovery.h"
#include "CalDAVAuth.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QObject>

bool
GoogleCalendarDiscovery::listCalendars
(CloudService *cloudService, QList<CalDAVDiscovery::CalendarInfo> *results, QString *errorOut)
{
    if (! results) {
        return false;
    }
    results->clear();

    if (! cloudService) {
        if (errorOut) {
            *errorOut = QObject::tr("No account configured");
        }
        return false;
    }

    QString accessToken = CalDAVAuth::setting(cloudService, CloudService::OAuthToken);
    if (accessToken.isEmpty()) {
        if (errorOut) {
            *errorOut = QObject::tr("Authorise with Google first");
        }
        return false;
    }

    QNetworkAccessManager nam;

    QUrl url("https://www.googleapis.com/calendar/v3/users/me/calendarList");
    QUrlQuery params;
    params.addQueryItem("fields", "items(id,summary)");
    params.addQueryItem("minAccessRole", "writer");
    url.setQuery(params);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());

    QEventLoop loop;
    QNetworkReply *reply = nam.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool ok = (reply->error() == QNetworkReply::NoError);
    QByteArray body = reply->readAll();

    if (! ok) {
        if (errorOut) {
            QJsonDocument errDoc = QJsonDocument::fromJson(body);
            QString detail = errDoc.object()["error"].toObject()["message"].toString();
            *errorOut = detail.isEmpty() ? reply->errorString() : detail;
        }
        reply->deleteLater();
        return false;
    }
    reply->deleteLater();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorOut) {
            *errorOut = parseError.errorString();
        }
        return false;
    }

    QJsonArray items = doc.object()["items"].toArray();
    for (const QJsonValue &v : items) {
        QJsonObject cal = v.toObject();
        QString calId = cal["id"].toString();
        if (calId.isEmpty()) {
            continue;
        }

        CalDAVDiscovery::CalendarInfo info;
        info.url = QString("https://apidata.googleusercontent.com/caldav/v2/%1/events/")
                          .arg(QString::fromUtf8(QUrl::toPercentEncoding(calId)));
        info.displayName = cal["summary"].toString();
        if (info.displayName.isEmpty()) {
            info.displayName = calId;
        }
        *results << info;
    }

    if (results->isEmpty()) {
        if (errorOut) {
            *errorOut = QObject::tr("No writable calendars found in this Google account");
        }
        return false;
    }
    return true;
}
