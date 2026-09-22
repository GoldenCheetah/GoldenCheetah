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

#include "CalDAVAuth.h"


QString
CalDAVAuth::setting
(CloudService *cloudService, CloudService::CloudServiceSetting key)
{
    if (! cloudService) {
        return QString();
    }
    return cloudService->getSetting(cloudService->settings.value(key)).toString();
}


QString
CalDAVAuth::collectionUrl
(CloudService *cloudService)
{
    QString url = setting(cloudService, CloudService::Local1);
    if (! url.isEmpty() && ! url.endsWith("/")) {
        url += "/";
    }
    return url;
}


bool
CalDAVAuth::isConfigured
(CloudService *cloudService)
{
    if (! cloudService) {
        return false;
    }

    QString url = setting(cloudService, CloudService::Local1);
    if (url.isEmpty()) {
        url = setting(cloudService, CloudService::URL);
    }
    QString user = setting(cloudService, CloudService::Username);
    QString pass = setting(cloudService, CloudService::Password);
    return ! url.isEmpty() && ! user.isEmpty() && ! pass.isEmpty();
}


void
CalDAVAuth::applyAuth
(CloudService *cloudService, QNetworkRequest &request)
{
    if (! cloudService) {
        return;
    }

    QString user = setting(cloudService, CloudService::Username);
    QString pass = setting(cloudService, CloudService::Password);
    if (user.isEmpty()) {
        return;
    }

    QByteArray token = QString("%1:%2").arg(user, pass).toUtf8().toBase64();
    request.setRawHeader("Authorization", "Basic " + token);
}
