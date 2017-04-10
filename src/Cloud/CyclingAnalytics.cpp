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

#include "CyclingAnalytics.h"
#include "Athlete.h"
#include "Settings.h"
#include "mvjson.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef CYCLINGANALYTICS_DEBUG
#define CYCLINGANALYTICS_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (CYCLINGANALYTICS_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (CYCLINGANALYTICS_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

CyclingAnalytics::CyclingAnalytics(Context *context) : CloudService(context), context(context), root_(NULL) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = none; // gzip
    filetype = uploadType::TCX;
    useMetric = true; // distance and duration metadata

    // config
    settings.insert(OAuthToken, GC_CYCLINGANALYTICS_TOKEN);
}

CyclingAnalytics::~CyclingAnalytics() {
    if (context) delete nam;
}

void
CyclingAnalytics::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

bool
CyclingAnalytics::open(QStringList &errors)
{
    printd("CyclingAnalytics::open\n");
    QString token = getSetting(GC_CYCLINGANALYTICS_TOKEN, "").toString();
    if (token == "") {
        errors << tr("Account is not configured.");
        return false;
    }
    return true;
}

bool
CyclingAnalytics::close()
{
    printd("CyclingAnalytics::close\n");
    // nothing to do for now
    return true;
}

bool
CyclingAnalytics::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    Q_UNUSED(ride);

    printd("CyclingAnalytics::writeFile(%s)\n", remotename.toStdString().c_str());

    QString token = getSetting(GC_CYCLINGANALYTICS_TOKEN, "").toString();
    QUrl url = QUrl( "https://www.cyclinganalytics.com/api/me/upload" );
    QNetworkRequest request = QNetworkRequest(url);

    QString boundary = QVariant(qrand()).toString()+QVariant(qrand()).toString()+QVariant(qrand()).toString();

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    multiPart->setBoundary(boundary.toLatin1());

    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    QHttpPart activityNamePart;
    activityNamePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"title\""));
    activityNamePart.setBody(remotename.toLatin1());

    QHttpPart dataTypePart;
    dataTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"format\""));
    dataTypePart.setBody("tcx");

    QHttpPart filenamePart;
    filenamePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"filename\""));
    filenamePart.setBody(remotename.toLatin1());

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"data\"; filename=\"file.tcx\"; type=\"text/xml\""));
    filePart.setBody(data);

    multiPart->append(activityNamePart);
    multiPart->append(filenamePart);
    multiPart->append(dataTypePart);
    multiPart->append(filePart);

    reply = nam->post(request, multiPart);
    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply,remotename);
    return true;
}

void
CyclingAnalytics::writeFileCompleted()
{
    printd("CyclingAnalytics::writeFileCompleted()\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s\n", reply->readAll().toStdString().c_str());

    // remember why we errored
    bool uploadSuccessful = false;
    QString uploadError;

    try {

        // parse the response
        QString response = reply->readAll();
        MVJSONReader jsonResponse(string(response.toLatin1()));

        // get values
        uploadError = jsonResponse.root->getFieldString("error").c_str();
        //XXXcyclingAnalyticsUploadId = jsonResponse.root->getFieldInt("upload_id");

    } catch(...) {

        // problem!
        uploadError = "bad response or parser exception.";
        //XXXcyclingAnalyticsUploadId = 0;
    }

    // if not there clean out
    if (uploadError.toLower() == "none" || uploadError.toLower() == "null") uploadError = "";

    // update id if uploaded successfully
    if (uploadError.length()>0 || reply->error() != QNetworkReply::NoError)  uploadSuccessful = false;
    else {

        // Success
        //XXX ride->ride()->setTag("CyclingAnalytics uploadId", QString("%1").arg(cyclingAnalyticsUploadId));
        //XXX ride->setDirty(true);
        uploadSuccessful = true;
    }

    if (uploadSuccessful && reply->error() == QNetworkReply::NoError) {
        notifyWriteComplete( replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Completed."));
    } else {
        notifyWriteComplete( replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Network Error - Upload failed."));
    }
}

static bool addCyclingAnalytics() {
    CloudServiceFactory::instance().addService(new CyclingAnalytics(NULL));
    return true;
}

static bool add = addCyclingAnalytics();
