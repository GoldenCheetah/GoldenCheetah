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

QImage Wahoo::logo() const
{
    printd("Logo requested.\n");
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
    CloudService::settings.insert(Local1, GC_WAHOO_REFRESH_TOKEN);
    CloudService::settings.insert(Local2, GC_WAHOO_LAST_REFRESH);
}

bool
Wahoo::open(QStringList &errors)
{
    printd("\nWahoo::open.\n");
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

    // Prevent uneccessary refresh requests. Refresh tokens are valid 2h
    // 120 sec will be enough, so the limit is 7000 sec from the last refresh
    QDateTime lastRefresh =  getSetting(GC_WAHOO_LAST_REFRESH).toDateTime();

    // Convert 2h in seconds and subtract 120
    constexpr int maxTokenAgeSec = 7200 - 120;
    auto expires = lastRefresh.addSecs(maxTokenAgeSec);
    if (QDateTime::currentDateTime().toLocalTime() < expires) {return true; }

    int expiresIn{0};
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
    printd("\nWahoo::close.\n");
    return true;
}

QList<CloudServiceEntry*>
Wahoo::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    printd("\nWahoo::readdir path: (%s), from: (%s), to: (%s)\n",
        qPrintable(path), qPrintable(from.toString()), qPrintable(to.toString()));

    QList<CloudServiceEntry*> results;
    const QString token = getToken();
    QList<WahooWorkoutEntry> entries = client_.fetchWorkouts(token, errors, from, to);
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
    printd("\nGet remotename: %s, remoteid: %s\n", qPrintable(remotename), qPrintable(remoteid));
    
    const QString token = getToken();
    QStringList errors;
    QNetworkReply *reply = client_.startFileDownload(token, remoteid, errors);
    if (reply == nullptr || !errors.isEmpty()) {
        printd("\nNetworkResponse error in Wahoo::readFile: %s\n", qPrintable(errors.join(". ")));
        return false;
    }

    // insert into reply map and buffers
    CloudService::mapReply(reply, remotename);
    buffers.insert(reply, data);

    auto completedFunc = QObject::connect(reply, &QNetworkReply::finished, this, &Wahoo::readFileCompleted);
    auto readyFunc = QObject::connect(reply, &QNetworkReply::readyRead, this, &Wahoo::readyRead);

    return true;
}

bool
Wahoo::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    printd("\nWahoo::writeFile called for %s\n", qPrintable(remotename));

    const QString token = getToken();
    printd("\nGot token %s\n", qPrintable(token));

    QNetworkReply *reply = client_.sendFile(token, data, remotename, ride->id());
    if (reply == nullptr) {
        printd("\nGot network reply with nullptr, exiting writeFile.\n");
        return false; 
    }

    CloudService::mapReply(reply, remotename);
    connect(reply, &QNetworkReply::finished, this, &Wahoo::writeFileCompleted);

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
    printd("\nWahoo::readFileCompleted.\n");
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    if (reply != nullptr && reply->error() == QNetworkReply::NoError) {
        printd("\nreceived %lld bytes\n", static_cast<long long>(buffers.value(reply)->size()));

        // This calls CloudService::readComplete,
        // which deletes the QByteArray *data
        notifyReadComplete(buffers.value(reply), CloudService::replyName(reply), tr("Completed."));
    }

    // reply is no longer needed
    reply->deleteLater();
}

void
Wahoo::writeFileCompleted()
{
    printd("\nWahoo writeFileCompleted.\n");
    auto *reply = static_cast<QNetworkReply*>(QObject::sender());
    
    const QString token = getToken();
    printd("\nGot token %s\n", qPrintable(token));
    QString result = client_.checkUpload(reply, token);
    
    CloudService::notifyWriteComplete(replyName(reply), result);
    printd("\nReturning writeFileCompleted with status: %s\n", qPrintable(result));
}

void
Wahoo::onSslErrors(QNetworkReply *reply, const QList<QSslError>&errors)
{
    // Redirect error handling to CloudService
    CloudService::sslErrors(context->mainWindow, reply, errors);
}

QString
Wahoo::getToken()
{
    printd("\nWahoo::getToken\n");
    QStringList errors;
    open(errors);
    if (!errors.empty()) {
        printd("\nAuthorization failed. %s\n", qPrintable(errors.join(".")));
        return "";
    }

    return getSetting(GC_WAHOO_TOKEN, "").toString();
}

static bool addWahoo() {
    CloudServiceFactory::instance().addService(new Wahoo(NULL));
    return true;
}

static bool add = addWahoo();
