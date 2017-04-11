/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

#include "SportTracks.h"
#include "Athlete.h"
#include "Settings.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef SPORTTRACKS_DEBUG
// TODO(gille): This should be a command line flag.
#define SPORTTRACKS_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (SPORTTRACKS_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (SPORTTRACKS_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

SportTracks::SportTracks(Context *context) : CloudService(context), context(context), root_(NULL) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = none; // gzip
    downloadCompression = none;
    filetype = FIT;
    useMetric = true; // distance and duration metadata

    // config
    settings.insert(OAuthToken, GC_SPORTTRACKS_TOKEN);
    settings.insert(Local1, GC_SPORTTRACKS_REFRESH_TOKEN);
    settings.insert(Local2, GC_SPORTTRACKS_LAST_REFRESH);
}

SportTracks::~SportTracks() {
    if (context) delete nam;
}

void
SportTracks::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

// open by connecting and getting a basic list of folders available
bool
SportTracks::open(QStringList &errors)
{
    printd("SportTracks::open\n");

    // do we have a token
    QString token = getSetting(GC_SPORTTRACKS_TOKEN, "").toString();
    if (token == "") {
        errors << "You must authorise with SportTracks first";
        return false;
    }
    return true;
}

bool
SportTracks::close()
{
    printd("SportTracks::close\n");
    // nothing to do for now
    return true;
}

QList<CloudServiceEntry*>
SportTracks::readdir(QString path, QStringList &errors, QDateTime, QDateTime)
{
    printd("SportTracks::readdir(%s)\n", path.toStdString().c_str());

    QList<CloudServiceEntry*> returning;

    // do we have a token
    QString token = getSetting(GC_SPORTTRACKS_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with SportTracks first");
        return returning;
    }

    // all good ?
    return returning;
}

// read a file at location (relative to home) into passed array
bool
SportTracks::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("SportTracks::readFile(%s)\n", remotename.toStdString().c_str());
#if 0
    // this must be performed asyncronously and call made
    // to notifyReadComplete(QByteArray &data, QString remotename, QString message) when done

    // do we have a token ?
    QString token = getSetting(GC_SPORTTRACKS_TOKEN, "").toString();
    if (token == "") return false;

    // lets connect and get basic info on the root directory
    QString url = QString("%1/rest/files/download/%2")
          .arg(getSetting(GC_SPORTTRACKS_URL, "https://whats.todaysplan.com.au").toString())
          .arg(remoteid);

    printd("url:%s\n", url.toStdString().c_str());

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    // put the file
    QNetworkReply *reply = nam->get(request);

    // remember
    mapReply(reply,remotename);
    buffers.insert(reply,data);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(readFileCompleted()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
#endif
    return true;
}

void
SportTracks::readyRead()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

void
SportTracks::readFileCompleted()
{
    printd("SportTracks::readFileCompleted\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s\n", buffers.value(reply)->toStdString().c_str());

    notifyReadComplete(buffers.value(reply), replyName(reply), tr("Completed."));
}

bool
SportTracks::writeFile(QByteArray &data, QString remotename, RideFile *)
{
    printd("SportTracks::writeFile(%s)\n", remotename.toStdString().c_str());
#if 0
    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done

    // do we have a token ?
    QString token = getSetting(GC_SPORTTRACKS_TOKEN, "").toString();
    if (token == "") return false;

    // lets connect and get basic info on the root directory
    QString url = QString("%1/rest/files/upload")
          .arg(getSetting(GC_SPORTTRACKS_URL, "https://whats.todaysplan.com.au").toString());

    printd("URL used: %s\n", url.toStdString().c_str());

    QNetworkRequest request = QNetworkRequest(url);

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QString boundary = QVariant(qrand()).toString()+QVariant(qrand()).toString()+QVariant(qrand()).toString();
    multiPart->setBoundary(boundary.toLatin1());

    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    QHttpPart jsonPart;
    jsonPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"json\""));

    QString userId = getSetting(GC_SPORTTRACKS_ATHLETE_ID, "").toString();

    QString json;
    if (userId.length()>0) {
        json  = QString("{ filename: \"%1\", userId: %2 }").arg(remotename).arg(userId);
    } else {
        json  = QString("{ filename: \"%1\" }").arg(remotename);
    }
    printd("request: %s\n", json.toStdString().c_str());
    jsonPart.setBody(json.toLatin1());

    QHttpPart attachmentPart;
    attachmentPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"attachment\"; type=\"text/xml\""));
    attachmentPart.setBody(data);

    multiPart->append(jsonPart);
    multiPart->append(attachmentPart);

    // post the file
    QNetworkReply *reply;

    reply = nam->post(request, multiPart);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply,remotename);
#endif
    return true;
}

void
SportTracks::writeFileCompleted()
{
    printd("SportTracks::writeFileCompleted()\n");
#if 0
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    QByteArray r = reply->readAll();
    printd("reply:%s\n", r.toStdString().c_str());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    if (reply->error() == QNetworkReply::NoError) {
        QString name = replyName(static_cast<QNetworkReply*>(QObject::sender()));

        QJsonObject result = document.object();//["result"].toObject();
        replyActivity.insert(name, result);
        //rideSend(name);

        notifyWriteComplete( name, tr("Completed."));

    } else {

        notifyWriteComplete( replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Network Error - Upload failed."));
    }
#endif

}
// development put on hold - AccessLink API compatibility issues with Desktop applications
static bool addSportTracks() {
    CloudServiceFactory::instance().addService(new SportTracks(NULL));
    return true;
}

static bool add = addSportTracks();
