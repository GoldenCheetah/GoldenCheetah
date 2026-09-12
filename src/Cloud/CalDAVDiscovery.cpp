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

#include "CalDAVDiscovery.h"
#include "CalDAVAuth.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDomDocument>
#include <QEventLoop>
#include <QObject>

static QDomElement findFirstByLocalName(const QDomNode &node, const QString &localName);
static bool looksLikeDomainOrEmail(const QString &rawInput, QString *domain);
static QList<CalDAVDiscovery::CalendarInfo> extractCalendars(const QString &document, const QString &baseUrl);


bool
CalDAVDiscovery::propfindBlocking
(CloudService *cloudService, QNetworkAccessManager *manager, const QString &url, const QByteArray &depth, const QByteArray &body, QString *response, QString *errorOut)
{
    QNetworkRequest request = QNetworkRequest(QUrl(url));
    request.setRawHeader("Depth", depth);
    request.setRawHeader("Content-Type", "application/xml; charset=\"utf-8\"");
    request.setRawHeader("Content-Length", QString::number(body.size()).toLatin1());
    CalDAVAuth::applyAuth(cloudService, request);

    QEventLoop loop;
    QNetworkReply *reply = manager->sendCustomRequest(request, "PROPFIND", body);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool ok = (reply->error() == QNetworkReply::NoError);
    if (response) {
        *response = QString::fromUtf8(reply->readAll());
    }
    if (! ok && errorOut) {
        *errorOut = reply->errorString();
    }
    reply->deleteLater();
    return ok;
}


bool
CalDAVDiscovery::wellKnownBootstrap
(CloudService *cloudService, QNetworkAccessManager *manager, const QString &domain, QUrl *resolvedUrl, QString *errorOut)
{
    QUrl wellKnownUrl(QString("https://%1/.well-known/caldav").arg(domain));

    QNetworkRequest request(wellKnownUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    CalDAVAuth::applyAuth(cloudService, request);

    QEventLoop loop;
    QNetworkReply *reply = manager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString location = QString::fromUtf8(reply->rawHeader("Location"));

    bool ok = false;
    if (   (   status == 301
            || status == 302
            || status == 303
            || status == 307
            || status == 308)
        && ! location.isEmpty()) {
        *resolvedUrl = wellKnownUrl.resolved(QUrl(location));
        ok = true;
    } else if (errorOut) {
        *errorOut = QObject::tr(".well-known/caldav did not redirect (status %1)").arg(status);
    }

    reply->deleteLater();
    return ok;
}


bool
CalDAVDiscovery::discoverCalendars
(CloudService *cloudService, QList<CalDAVDiscovery::CalendarInfo> *results, QString *errorOut)
{
    if (! results) {
        return false;
    }
    results->clear();

    QString serverUrl = CalDAVAuth::setting(cloudService, CloudService::URL);
    QString user = CalDAVAuth::setting(cloudService, CloudService::Username);
    QString pass = CalDAVAuth::setting(cloudService, CloudService::Password);
    if (serverUrl.isEmpty() || user.isEmpty() || pass.isEmpty()) {
        if (errorOut) {
            *errorOut = QObject::tr("Server URL, username or password is missing");
        }
        return false;
    }

    QNetworkAccessManager discoveryNam;
    QString stepError;

    QUrl resolvedUrl(serverUrl);
    if (! serverUrl.isEmpty() && ! serverUrl.endsWith("/") && ! resolvedUrl.host().isEmpty()) {
        resolvedUrl = QUrl(serverUrl + "/");
    }

    QString domain;
    if (looksLikeDomainOrEmail(serverUrl, &domain)) {
        QUrl bootstrapped;
        QString bootstrapError;
        if (wellKnownBootstrap(cloudService, &discoveryNam, domain, &bootstrapped, &bootstrapError)) {
            resolvedUrl = bootstrapped;
        } else if (resolvedUrl.host().isEmpty()) {
            resolvedUrl = QUrl(QString("https://%1/").arg(domain));
        }
    }

    QUrl principalUrl = resolvedUrl;

    QByteArray principalBody =
        "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
        "<D:propfind xmlns:D=\"DAV:\">"
        " <D:prop><D:current-user-principal/></D:prop>"
        "</D:propfind>\r\n";

    QString principalResponse;
    if (propfindBlocking(cloudService, &discoveryNam, resolvedUrl.toString(), "0", principalBody, &principalResponse, &stepError)) {
        QDomDocument principalDoc;
        if (principalDoc.setContent(principalResponse, QDomDocument::ParseOption::UseNamespaceProcessing)) {
            QDomElement principalProp = findFirstByLocalName(principalDoc.documentElement(), "current-user-principal");
            QDomElement principalHrefEl = principalProp.isNull() ? QDomElement() : findFirstByLocalName(principalProp, "href");
            if (! principalHrefEl.isNull()) {
                principalUrl = resolvedUrl.resolved(QUrl(principalHrefEl.text().trimmed()));
            }
        }
    }

    QUrl homeSetUrl = resolvedUrl;

    QByteArray homeSetBody =
        "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
        "<D:propfind xmlns:D=\"DAV:\" xmlns:C=\"urn:ietf:params:xml:ns:caldav\">"
        " <D:prop><C:calendar-home-set/></D:prop>"
        "</D:propfind>\r\n";

    QString homeSetResponse;
    if (propfindBlocking(cloudService, &discoveryNam, principalUrl.toString(), "0", homeSetBody, &homeSetResponse, &stepError)) {
        QDomDocument homeDoc;
        if (homeDoc.setContent(homeSetResponse, QDomDocument::ParseOption::UseNamespaceProcessing)) {
            QDomElement homeSetProp = findFirstByLocalName(homeDoc.documentElement(), "calendar-home-set");
            QDomElement homeHrefEl = homeSetProp.isNull() ? QDomElement() : findFirstByLocalName(homeSetProp, "href");
            if (! homeHrefEl.isNull()) {
                homeSetUrl = principalUrl.resolved(QUrl(homeHrefEl.text().trimmed()));
            }
        }
    }

    QByteArray listBody =
        "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
        "<D:propfind xmlns:D=\"DAV:\">"
        " <D:prop><D:displayname/><D:resourcetype/></D:prop>"
        "</D:propfind>\r\n";

    QString listResponse;
    if (propfindBlocking(cloudService, &discoveryNam, homeSetUrl.toString(), "1", listBody, &listResponse, &stepError)) {
        *results = extractCalendars(listResponse, homeSetUrl.toString());
    }

    if (! results->isEmpty()) {
        return true;
    }

    QString selfResponse;
    if (propfindBlocking(cloudService, &discoveryNam, resolvedUrl.toString(), "0", listBody, &selfResponse, &stepError)) {
        *results = extractCalendars(selfResponse, resolvedUrl.toString());
    }

    if (! results->isEmpty()) {
        return true;
    }

    if (errorOut) {
        *errorOut = stepError.isEmpty() ? QObject::tr("No calendars found - check the URL points at or above your CalDAV calendar home") : stepError;
    }
    return false;
}


static QDomElement
findFirstByLocalName
(const QDomNode &node, const QString &localName)
{
    for (QDomNode n = node.firstChild(); ! n.isNull(); n = n.nextSibling()) {
        QDomElement e = n.toElement();
        if (e.isNull()) {
            continue;
        }
        if (e.localName() == localName) {
            return e;
        }
        QDomElement found = findFirstByLocalName(e, localName);
        if (! found.isNull()) {
            return found;
        }
    }
    return QDomElement();
}


static bool
looksLikeDomainOrEmail
(const QString &rawInput, QString *domain)
{
    QString input = rawInput.trimmed();
    if (input.isEmpty()) {
        return false;
    }

    if (input.contains('@')) {
        *domain = input.section('@', -1, -1).trimmed();
        return ! domain->isEmpty();
    }

    QUrl url(input);
    if (url.scheme().isEmpty()) {
        *domain = input.section('/', 0, 0);
        return ! domain->isEmpty();
    }

    if ((url.path().isEmpty() || url.path() == "/") && ! url.host().isEmpty()) {
        *domain = url.host();
        return true;
    }

    return false;
}


static QList<CalDAVDiscovery::CalendarInfo>
extractCalendars
(const QString &document, const QString &baseUrl)
{
    QList<CalDAVDiscovery::CalendarInfo> results;

    QDomDocument doc;
    if (document.isEmpty() || ! doc.setContent(document, QDomDocument::ParseOption::UseNamespaceProcessing)) {
        return results;
    }

    QDomElement multistatus = doc.documentElement();
    for (QDomNode n = multistatus.firstChild(); ! n.isNull(); n = n.nextSibling()) {
        QDomElement response = n.toElement();
        if (response.isNull() || response.localName() != "response") {
            continue;
        }

        QDomElement hrefEl = findFirstByLocalName(response, "href");
        if (hrefEl.isNull()) {
            continue;
        }

        QDomElement resourcetype = findFirstByLocalName(response, "resourcetype");
        if (resourcetype.isNull()) {
            continue;
        }

        bool isCalendar = false;
        for (QDomNode c = resourcetype.firstChild(); ! c.isNull(); c = c.nextSibling()) {
            QDomElement ce = c.toElement();
            if (! ce.isNull() && ce.localName() == "calendar") {
                isCalendar = true;
                break;
            }
        }
        if (! isCalendar) {
            continue;
        }

        QDomElement nameEl = findFirstByLocalName(response, "displayname");
        QString href = hrefEl.text().trimmed();
        QString name = (! nameEl.isNull() && ! nameEl.text().trimmed().isEmpty()) ? nameEl.text().trimmed() : href.section('/', -2, -2, QString::SectionSkipEmpty);

        CalDAVDiscovery::CalendarInfo info;
        info.url = QUrl(baseUrl).resolved(QUrl(href)).toString();
        info.displayName = name;
        results << info;
    }

    return results;
}
