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

#ifndef _Gc_CalDAVDiscovery_h
#define _Gc_CalDAVDiscovery_h

#include <QString>
#include <QList>
#include <QUrl>
#include <QByteArray>
#include <QNetworkAccessManager>

#include "CloudService.h"


// Partial implementation of RFC 6764 for calendar discovery
class CalDAVDiscovery {
public:
    struct CalendarInfo {
        QString url;
        QString displayName;
    };

    static bool discoverCalendars(CloudService *cloudService, QList<CalendarInfo> *results, QString *errorOut = nullptr);

private:
    static bool propfindBlocking(CloudService *cloudService, QNetworkAccessManager *manager, const QString &url, const QByteArray &depth, const QByteArray &body, QString *response, QString *errorOut);
    static bool wellKnownBootstrap(CloudService *cloudService, QNetworkAccessManager *manager, const QString &domain, QUrl *resolvedUrl, QString *errorOut);
};

#endif
