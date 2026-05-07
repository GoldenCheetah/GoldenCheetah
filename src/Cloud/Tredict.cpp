/*
 * Copyright (c) 2026 Felix Gertz (felix@tredict.com)
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

#include "Tredict.h"
#include "OAuthPKCE.h"
#include "Athlete.h"
#include "Settings.h"
#include "Secrets.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrlQuery>

#ifndef TREDICT_DEBUG
#define TREDICT_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (TREDICT_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (TREDICT_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

static const QString TREDICT_URL = "https://www.tredict.com";

Tredict::Tredict(Context *context) : CloudService(context), context(context), root_(NULL) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )),
                this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = none;
    downloadCompression = none;
    filetype = uploadType::FIT;
    useMetric = true;

    settings.insert(OAuthToken, GC_TREDICT_TOKEN);
    settings.insert(Local1, GC_TREDICT_REFRESH_TOKEN);
    settings.insert(Local2, GC_TREDICT_LAST_REFRESH);
}

Tredict::~Tredict() {
    if (context) delete nam;
}

QImage Tredict::logo() const
{
    return QImage(":images/services/tredict.png");
}

void
Tredict::onSslErrors(QNetworkReply *reply, const QList<QSslError>&errors)
{
    sslErrors(context->mainWindow, reply, errors);
}

bool
Tredict::open(QStringList &errors)
{
    printd("Tredict::open\n");
    QString rt = getSetting(GC_TREDICT_REFRESH_TOKEN, "").toString();
    if (rt.isEmpty()) {
        errors << tr("No authorization token configured.");
        return false;
    }

    printd("Refreshing access token.\n");

    QString newAccess, newRefresh, err;
    int expiresIn;
    if (!OAuthPKCE::refreshAccessToken(
            TREDICT_URL + "/user/oauth/v2/token",
            GC_TREDICT_CLIENT_ID,
            rt, newAccess, newRefresh, expiresIn, err)) {
        errors << err;
        return false;
    }

    if (!newAccess.isEmpty()) setSetting(GC_TREDICT_TOKEN, newAccess);
    if (!newRefresh.isEmpty()) setSetting(GC_TREDICT_REFRESH_TOKEN, newRefresh);
    setSetting(GC_TREDICT_LAST_REFRESH, QDateTime::currentDateTime());

    CloudServiceFactory::instance().saveSettings(this, context);
    return true;
}

bool
Tredict::close()
{
    printd("Tredict::close\n");
    return true;
}

QList<CloudServiceEntry*>
Tredict::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    printd("Tredict::readdir(%s)\n", path.toStdString().c_str());

    QList<CloudServiceEntry*> returning;

    QString token = getSetting(GC_TREDICT_TOKEN, "").toString();
    if (token.isEmpty()) {
        errors << tr("You must authorize with Tredict first.");
        return returning;
    }

    // API traverses newest-first, no endDate param -- client-side filter for `from`
    QString nextPageUrl = "";
    bool hasMore = true;

    while (hasMore) {
        QUrl url;
        if (nextPageUrl.isEmpty()) {
            url = QUrl(TREDICT_URL + "/api/oauth/v2/activityList");
            QUrlQuery params;
            params.addQueryItem("startDate", to.toUTC().toString(Qt::ISODate));
            params.addQueryItem("extendedSummary", "1");
            params.addQueryItem("pageSize", "500");
            url.setQuery(params);
        } else {
            url = QUrl(nextPageUrl);
            QUrlQuery params(url);
            if (!params.hasQueryItem("extendedSummary"))
                params.addQueryItem("extendedSummary", "1");
            url.setQuery(params);
        }

        printd("URL: %s\n", url.url().toStdString().c_str());

        QNetworkRequest request(url);
        request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
        request.setRawHeader("Accept", "application/json;charset=UTF-8");

        QNetworkReply *reply = nam->get(request);

        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "error" << reply->errorString();
            errors << tr("Network error reading Tredict data.");
            reply->deleteLater();
            break;
        }

        QByteArray r = reply->readAll();
        reply->deleteLater();
        printd("response: %s\n", r.toStdString().c_str());

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            errors << tr("JSON parser error: %1").arg(parseError.errorString());
            break;
        }

        QJsonObject root = document.object();
        QJsonObject embedded = root["_embedded"].toObject();
        QJsonArray activities = embedded["activityList"].toArray();

        bool reachedFrom = false;
        for (int i = 0; i < activities.size(); i++) {
            QJsonObject each = activities.at(i).toObject();

            QString dateStr = each["date"].toString();
            QDateTime dt = QDateTime::fromString(dateStr, Qt::ISODate);

            if (dt.isValid() && dt < from) {
                reachedFrom = true;
                break;
            }

            CloudServiceEntry *add = newCloudServiceEntry();

            add->id = each["id"].toString();
            add->label = each["title"].toString();
            if (add->label.isEmpty())
                add->label = each["sportType"].toString();
            add->isDir = false;

            QJsonObject summary = each["summary"].toObject();
            add->distance = summary["distance"].toDouble() / 1000.0;
            add->duration = summary["durationTotal"].toDouble();

            if (dt.isValid()) {
                add->name = dt.toString("yyyy_MM_dd_HH_mm_ss") + ".fit";
                returning << add;
            }

            printd("direntry: %s %s\n", add->id.toStdString().c_str(), add->name.toStdString().c_str());
        }

        if (reachedFrom) break;

        // _links.next.href contains the full URL for the next page
        QJsonObject links = root["_links"].toObject();
        QJsonObject next = links["next"].toObject();
        if (next.contains("href")) {
            nextPageUrl = next["href"].toString();
            hasMore = !nextPageUrl.isEmpty();
        } else {
            hasMore = false;
        }
    }

    return returning;
}

bool
Tredict::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("Tredict::readFile(%s)\n", remotename.toStdString().c_str());

    QString token = getSetting(GC_TREDICT_TOKEN, "").toString();
    if (token.isEmpty()) return false;

    QString url = QString("%1/api/oauth/v2/activity/file/%2").arg(TREDICT_URL).arg(remoteid);
    printd("url: %s\n", url.toStdString().c_str());

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setRawHeader("Accept", "application/json;charset=UTF-8");

    reply = nam->get(request);

    mapReply(reply, remotename);
    buffers.insert(reply, data);

    connect(reply, SIGNAL(finished()), this, SLOT(readFileCompleted()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    return true;
}

bool
Tredict::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    printd("Tredict::writeFile(%s)\n", remotename.toStdString().c_str());

    QString token = getSetting(GC_TREDICT_TOKEN, "").toString();
    if (token.isEmpty()) return false;

    QUrl url = QUrl(TREDICT_URL + "/api/oauth/v2/activity/upload");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setRawHeader("Accept", "application/json;charset=UTF-8");

    QString boundary = QVariant(QRandomGenerator::global()->generate()).toString() +
        QVariant(QRandomGenerator::global()->generate()).toString() +
        QVariant(QRandomGenerator::global()->generate()).toString();

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    multiPart->setBoundary(boundary.toLatin1());

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"file\"; filename=\"" + remotename + "\""));
    filePart.setBody(data);
    multiPart->append(filePart);

    QString workoutCode = ride->getTag("Workout Code", "").trimmed();
    QString sport = ride->getTag("Sport", "").trimmed();
    QString name;
    if (!workoutCode.isEmpty() && !sport.isEmpty())
        name = sport + " - " + workoutCode;
    else if (!workoutCode.isEmpty())
        name = workoutCode;
    else if (!sport.isEmpty())
        name = sport;

    if (!name.isEmpty()) {
        QHttpPart namePart;
        namePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                           QVariant("form-data; name=\"name\""));
        namePart.setBody(name.toUtf8());
        multiPart->append(namePart);
    }

    QString notes = ride->getTag("Notes", "").trimmed();
    if (!notes.isEmpty()) {
        QHttpPart notesPart;
        notesPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                            QVariant("form-data; name=\"notes\""));
        notesPart.setBody(notes.left(1024).toUtf8());
        multiPart->append(notesPart);
    }

    reply = nam->post(request, multiPart);

    mapReply(reply, remotename);
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));
    return true;
}

void
Tredict::writeFileCompleted()
{
    printd("Tredict::writeFileCompleted()\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    bool uploadSuccessful = false;
    QString uploadError = tr("Unknown error");

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray r = reply->readAll();
        printd("reply: %s\n", r.toStdString().c_str());

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(r, &parseError);

        if (parseError.error == QJsonParseError::NoError) {
            QJsonObject obj = doc.object();
            if (obj.contains("success")) {
                uploadSuccessful = true;
                uploadError = "";
            } else if (obj.contains("error")) {
                uploadError = obj["error"].toString();
            }
        }
    } else {
        uploadError = reply->errorString();
    }

    if (uploadSuccessful) {
        notifyWriteComplete(replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Completed."));
    } else {
        notifyWriteComplete(replyName(static_cast<QNetworkReply*>(QObject::sender())), uploadError);
    }
}

void
Tredict::readyRead()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

void
Tredict::readFileCompleted()
{
    printd("Tredict::readFileCompleted\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    // the data is raw FIT (or TCX), pass it through directly
    // GC's uncompressRide / RideFileFactory handles both formats
    printd("received %lld bytes\n", (long long)buffers.value(reply)->size());

    notifyReadComplete(buffers.value(reply), replyName(reply), tr("Completed."));
}

static bool addTredict() {
    CloudServiceFactory::instance().addService(new Tredict(NULL));
    return true;
}

static bool add = addTredict();
