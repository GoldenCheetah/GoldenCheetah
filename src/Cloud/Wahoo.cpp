/*
* Copyright (c) 2026 Simon-Friedrich Böttger (simonboettger (at) googlemail.com)
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

#include "Wahoo.h"
#include "OAuthPKCE.h"
#include "Athlete.h"
#include "Settings.h"
#include "Secrets.h"

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QScopedPointer>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QHttpMultiPart>
#include <QUrlQuery>
#include <QEventLoop>

#ifndef WAHOO_DEBUG
#define WAHOO_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (WAHOO_DEBUG) {                                 \
        printf("[%s:%d %s] \n" fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (WAHOO_DEBUG) {                                       \
            printf("[%s:%d %s] \n" fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

const static QString WAHOO_URL = "https://api.wahooligan.com";

QImage Wahoo::logo() const
{
    printd("Logo requested.");
    return QImage("images/services/wahoo.png");
}

Wahoo::Wahoo(Context *context) : CloudService(context), context(context)
{
    if (context) {
        nam = new QNetworkAccessManager(this);
        auto discard = QObject::connect(nam, &QNetworkAccessManager::sslErrors, this, &Wahoo::onSslErrors);
        client_.setNam(nam);
    }

    // Set CloudService properties

    // Wahoo does not use compression
    downloadCompression = none;
    uploadCompression = none;

    // Wahoo uses FIT files
    filetype = uploadType::FIT;

    // Wahoo knows distance and duration metadata
    useMetric = true;

    // Add settings to save our tokens
    // First argument is the enum CloudServiceSetting (i.e. the "type")
    // The second is the name of the setting
    CloudService::settings.insert(OAuthToken, GC_WAHOO_TOKEN);
    CloudService::settings.insert(OAuthToken, GC_WAHOO_REFRESH_TOKEN);
    CloudService::settings.insert(Local1, GC_WAHOO_LAST_REFRESH);
}

bool
Wahoo::open(QStringList &errors)
{
    printd("Wahoo::open.\n");
    // Get last refresh token
    QString refreshToken = getSetting(GC_WAHOO_REFRESH_TOKEN, "").toString();
    if (refreshToken.isEmpty()) {
        errors << tr("No authorization token configured.");
        return false;
    }

    // Refresh tokens
    QString newAccess;
    QString newRefresh;
    QString err;
    int expiresIn;
    if (!OAuthPKCE::refreshAccessToken(
            WAHOO_URL + "/oauth/token",
            GC_WAHOO_CLIENT_ID,
            refreshToken, newAccess, newRefresh, expiresIn, err)
        ) {
        errors << err;
        return false;
    }

    // Save the new tokens
    if (!newAccess.isEmpty()) setSetting(GC_WAHOO_TOKEN, newAccess);
    if (!newRefresh.isEmpty()) setSetting(GC_WAHOO_REFRESH_TOKEN, newRefresh);
    setSetting(GC_WAHOO_LAST_REFRESH, QDateTime::currentDateTime());

    CloudServiceFactory::instance().saveSettings(this, context);
    return true;
}

bool
Wahoo::close()
{
    printd("Wahoo::close.\n");
    return true;
}

QList<CloudServiceEntry*>
Wahoo::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    printd("Wahoo::readdir path: (%s), from: (%s), to: (%s)\n",
        qPrintable(path), qPrintable(from.toString()), qPrintable(to.toString()));

    QList<CloudServiceEntry*> results;
    QString token = getSetting(GC_WAHOO_TOKEN, "").toString();

    auto entries = client_.fetchWorkouts(token, errors, from, to);
    if (!errors.isEmpty()) return results;

    for (const auto &entry : entries) {
        CloudServiceEntry *add = newCloudServiceEntry();
        add->id = entry.id;
        add->label = entry.label;
        add->isDir = entry.isDir;
        add->distance = entry.distance;
        add->duration = entry.duration;
        add->name = entry.name;
        results << add;
    }

    return results;
}

bool
Wahoo::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("Get remotename: %s, remoteid: %s", qPrintable(remotename), qPrintable(remoteid));
    QStringList errors;
    QString token = getSetting(GC_WAHOO_TOKEN, "").toString();

    QNetworkReply *reply = client_.startFileDownload(token, remoteid, errors);
    if (reply == nullptr || !errors.isEmpty()) {
        qWarning() << "NetworkResponse error in Wahoo::readFile: " << errors;
        return false;
    }

    // insert into reply map and buffers
    CloudService::mapReply(reply, remotename);
    buffers.insert(reply, data);

    auto completedFunc = QObject::connect(reply, &QNetworkReply::finished, this, &Wahoo::readFileCompleted);
    auto readyFunc = QObject::connect(reply, &QNetworkReply::readyRead, this, &Wahoo::readyRead);

    return true;
}

void
Wahoo::readyRead()
{
    // reply* cannot be deleted in this function because the network
    // reply is also needed in Wahoo::readFileCompleted
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    if (reply != nullptr && reply->error() == QNetworkReply::NoError) {
        buffers.value(reply)->append(reply->readAll());
    }
}

void
Wahoo::readFileCompleted()
{
    printd("Wahoo::readFileCompleted.");
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    if (reply != nullptr && reply->error() == QNetworkReply::NoError) {
        printd("received %lld bytes\n", static_cast<long long>(buffers.value(reply)->size()));

        // This calls CloudService::readComplete,
        // which deletes the QByteArray *data
        notifyReadComplete(buffers.value(reply), CloudService::replyName(reply), tr("Completed."));
    }

    // reply is no longer needed
    reply->deleteLater();
}

void
Wahoo::onSslErrors(QNetworkReply *reply, const QList<QSslError>&errors)
{
    // Redirect error handling to CloudService
    CloudService::sslErrors(context->mainWindow, reply, errors);
}

static bool addWahoo() {
    CloudServiceFactory::instance().addService(new Wahoo(NULL));
    return true;
}

static bool add = addWahoo();
