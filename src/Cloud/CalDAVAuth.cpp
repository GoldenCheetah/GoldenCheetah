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

#include "OAuthPKCE.h"
#include "Context.h"
#include "Secrets.h"

#include <QDateTime>

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


static bool
refreshOAuthAccessToken
(CloudService *cloudService, Context *context, QString *errorOut)
{
    static const QString tokenUrl = "https://oauth2.googleapis.com/token";
    static const QString clientId = GC_GOOGLECAL_CLIENT_ID;
    static const QString clientSecret = GC_GOOGLECAL_CLIENT_SECRET;

    QString refreshToken = CalDAVAuth::setting(cloudService, CloudService::Local3);
    if (refreshToken.isEmpty()) {
        if (errorOut) {
            *errorOut = QObject::tr("Not signed in - use Authorise to sign in to Google Calendar");
        }
        return false;
    }

    QString newAccess, newRefresh, error;
    int expiresIn = 0;
    if (! OAuthPKCE::refreshAccessToken(tokenUrl, clientId, refreshToken, newAccess, newRefresh, expiresIn, error, clientSecret)) {
        if (errorOut) {
            *errorOut = error;
        }
        return false;
    }

    QString accessKey = cloudService->settings.value(CloudService::OAuthToken, "");
    QString refreshKey = cloudService->settings.value(CloudService::Local3, "");
    QString lastRefreshKey = cloudService->settings.value(CloudService::Local4, "");

    if (! accessKey.isEmpty() && ! newAccess.isEmpty()) {
        cloudService->setSetting(accessKey, newAccess);
    }
    if (! refreshKey.isEmpty() && ! newRefresh.isEmpty()) {
        cloudService->setSetting(refreshKey, newRefresh);
    }
    if (! lastRefreshKey.isEmpty()) {
        cloudService->setSetting(lastRefreshKey, QDateTime::currentDateTime());
    }

    if (context) {
        CloudServiceFactory::instance().saveSettings(cloudService, context);
    }

    return true;
}


bool
CalDAVAuth::isConfigured
(CloudService *cloudService, Context *context)
{
    if (! cloudService) {
        return false;
    }

    if (cloudService->capabilities() & CloudService::OAuth) {
        bool ok = refreshOAuthAccessToken(cloudService, context, nullptr);
        QString calendarUrl = setting(cloudService, CloudService::Local1);
        return ok && ! calendarUrl.isEmpty();
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

    if (cloudService->capabilities() & CloudService::OAuth) {
        QString accessToken = setting(cloudService, CloudService::OAuthToken);
        if (accessToken.isEmpty()) {
            return;
        }
        request.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());
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
