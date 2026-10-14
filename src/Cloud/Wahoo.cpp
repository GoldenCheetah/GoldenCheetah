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
#define WAHOO_DEBUG true
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

QJsonObject
network2Json (const QByteArray &replyData, QStringList &errors)
{
    // Convert network response to JSON objects
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(replyData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        errors << QObject::tr("JSON parser error: %1").arg(parseError.errorString());
        return QJsonObject{};
    }

    const QJsonObject jsonRoot = document.object();
    if (jsonRoot.isEmpty()) {
        errors << QObject::tr("JSON root object empty.");
    }
    return jsonRoot;
}

QByteArray
Wahoo::getNetworkResponse(QUrl url, QStringList &errors, QUrlQuery params)
{
    QString token = getSetting(GC_WAHOO_TOKEN, "").toString();
    if (token.isEmpty()) {
        errors << tr("You must authorize with Wahoo first.");
        return QByteArray{};
    }

    url.setQuery(params);
    QNetworkRequest request(url);

    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setRawHeader("Accept", "application/json;charset=UTF-8");
    if (nam == nullptr) {
        errors << tr("getNetworkResponse with QNetworkAccessManager nam uninitialized.");
        return QByteArray{};
    }

    using NetworkReply = QScopedPointer<QNetworkReply, QScopedPointerDeleteLater>;
    NetworkReply reply (nam->get(request));
    if (reply == nullptr) {
        errors << QObject::tr("Error getting network request pointer.");
        return QByteArray{};
    }

    QEventLoop loop;
    auto discard = QObject::connect(reply.get(), &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        errors << QObject::tr("Network error reading Wahoo workouts. ") << reply->errorString();;
        return QByteArray{};
    }

    QByteArray replyData = reply->readAll();
    return replyData;
}

QList<CloudServiceEntry*>
Wahoo::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    printd("Wahoo::readdir path: (%s), from: (%s), to: (%s)\n",
        qPrintable(path), qPrintable(from.toString()), qPrintable(to.toString()));

    QList<CloudServiceEntry*> results;
    // Get workouts

    // How many workouts at a time to fetch
    const int pageSize {30};

    // The position of the first workout to start looking at
    int offset {0};

    // How many workouts the api has
    // This will be set to the correct number
    // when the json is available
    int resultCount {INT_MAX};

    while (offset < resultCount) {
        QUrl url{WAHOO_URL + "/v1/workouts"};

        QUrlQuery params;
        params.addQueryItem("page", QString("%1").arg(offset/pageSize+1));
        params.addQueryItem("per_page", QString("%1").arg(pageSize));

        const QByteArray replyData = getNetworkResponse(url, errors, params);
        const QJsonObject root = network2Json(replyData, errors);
        if (!errors.empty()) return results;

        const QJsonValue total = root.value("total");
        if (total.type() != QJsonValue::Double) {
            errors << tr("Cannot parse total value of JSON root.");
            return results;
        }

        // toInt is a qint64, which is a signed type
        resultCount = total.toInt();
        if (resultCount <= 0) {
            errors << tr("Invalid value for total value of Wahoo API response: ");
            errors << total.toVariant().toString();
            return results;
        }

        const QJsonArray workouts = root.value("workouts").toArray();
        if (workouts.isEmpty()) {
            errors << tr("Cannot find workouts in JSON data.");
            printd("Network response: %s\n", qPrintable(replyData));
            return results;
        }

        for (const QJsonValue &workoutValue : workouts) {
            QJsonObject workout = workoutValue.toObject();
            QString dateStr = workout.value("starts").toString();
            QDateTime startDate = QDateTime::fromString(dateStr, Qt::ISODate).toLocalTime();

            // Activities are sorted by date from newest to oldest
            // Only look at activities between from and to
            // readdir is called from CloudServiceAutoDownload
            // with from = 30 back from now and to = todday
            if (!startDate.isValid()) continue;
            if (startDate < from) return results;
            if (startDate > to) continue;

            // By calling CloudService::newCloudServiceEntry
            // the CloudServiceEntry ptr will be added to CloudService::list_
            // CloudService will clean up the entries in its list_ object in its destructor
            // so this class doesn't have to call delete on *add
            CloudServiceEntry *add = CloudService::newCloudServiceEntry();

            add->id = workout["id"].toVariant().toString();
            add->label = workout["name"].toString();
            add->isDir = false;

            QJsonObject summary = workout["workout_summary"].toObject();

            // distance in km
            add->distance = summary["distance_accum"].toVariant().toDouble() / 1000.0;
            // total duration in secs
            add->duration = summary["duration_total_accum"].toVariant().toDouble();

            if (startDate.isValid()) {
                add->name = startDate.toString("yyyy_MM_dd_HH_mm_ss") + ".fit";
                results << add;
                printd("\nAdding name %s\n \twith id: %s \n", qPrintable(add->name), qPrintable(add->id));
                printd("\t distance: %f \n \t duration: %ld", add->distance, add->duration);
            }
        }
        // next page
        offset += pageSize;
    }
    return results;
}

bool
Wahoo::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("Get remotename: %s, remoteid: %s", qPrintable(remotename), qPrintable(remoteid));
    QStringList errors;

    // Get the requested file, first the workout_summary JSON object
    QString requestUrl = QString(WAHOO_URL + "/v1/workouts/%1/workout_summary").arg(remoteid);

    const QByteArray replyData = getNetworkResponse(requestUrl, errors);
    if (!errors.isEmpty()) {
        qWarning() << "NetworkResponse error in Wahoo::readFile: " << errors;
        printd("Tried to fetch %s.\n", qPrintable(requestUrl));
        return false;
    }

    const QJsonObject workoutSummary = network2Json(replyData, errors);

    // The workout_summary has a file object, which has a url value which points to the file
    const QJsonValue file = workoutSummary.value("file").toObject().value("url");
    const QString url = file.toString();
    if (url.isEmpty()) {
        qWarning() << "Wahoo API did not return a file URL.";
        printd("Wahoo API response: %s\n", qPrintable(replyData));
        return false;
    }
    printd("Download url: %s", qPrintable(url));

    // Download file
    QNetworkRequest request(url);

    if (nam == nullptr) {
        qWarning() << "Wahoo::readFile called with nam == nullptr,"
                << "which means its constructor was called with a nullptr context.";
        return false;
    }
    // Network reply is needed until readFileCompleted,
    // don't delete it in this function
    QNetworkReply *reply = nam->get(request);

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
        CloudService::notifyReadComplete(buffers.value(reply), CloudService::replyName(reply), tr("Completed."));
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
