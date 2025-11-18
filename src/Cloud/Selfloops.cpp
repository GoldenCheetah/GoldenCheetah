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

#include "Selfloops.h"
#include "Athlete.h"
#include "Settings.h"
#include "mvjson.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef SELFLOOPS_DEBUG
#define SELFLOOPS_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (SELFLOOPS_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (SELFLOOPS_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

Selfloops::Selfloops(Context *context) : CloudService(context), context(context), root_(NULL) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = gzip; // gzip
    filetype = CloudService::uploadType::TCX;
    useMetric = true; // distance and duration metadata

    //config
    settings.insert(Username, GC_SELUSER);
    settings.insert(Password, GC_SELPASS);
}

Selfloops::~Selfloops() {
    if (context) delete nam;
}

void
Selfloops::onSslErrors(QNetworkReply *reply, const QList<QSslError>&errors)
{
    CloudDBCommon::sslErrors(reply, errors);
}

bool
Selfloops::open(QStringList &errors)
{
    printd("Selfloops::open\n");
    QString username = getSetting(GC_SELUSER).toString();
    if (username == "") {
        errors << tr("Account is not configured,");
        return false;
    }
    return true;
}

bool
Selfloops::close()
{
    printd("Selfloops::close\n");
    // nothing to do for now
    return true;
}

bool
Selfloops::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    Q_UNUSED(ride);

    printd("Selfloops::writeFile(%s)\n", remotename.toStdString().c_str());

    QUrl url = QUrl( "https://www.selfloops.com/restapi/public/activities/upload.json" );
    QNetworkRequest request = QNetworkRequest(url);

    QString boundary = QVariant(QRandomGenerator::global()->generate()).toString()+QVariant(QRandomGenerator::global()->generate()).toString()+QVariant(QRandomGenerator::global()->generate()).toString();

    QString username = getSetting(GC_SELUSER).toString();
    QString password = getSetting(GC_SELPASS).toString();

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::MixedType);
    multiPart->setBoundary(boundary.toLatin1());

    QHttpPart emailPart;
    emailPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"email\""));
    emailPart.setBody(username.toLatin1());

    QHttpPart passwordPart;
    passwordPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"pw\""));
    passwordPart.setBody(password.toLatin1());

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"tcxfile\"; filename=\"myfile.tcx.gz\"; type=\"application/x-gzip\""));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-gzip");
    filePart.setBody(data);

    multiPart->append(emailPart);
    multiPart->append(passwordPart);
    multiPart->append(filePart);

    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done
    reply = nam->post(request, multiPart);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply,remotename);
    return true;
}

void
Selfloops::writeFileCompleted()
{
    printd("Selfloops::writeFileCompleted()\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s\n", reply->readAll().toStdString().c_str());

    bool uploadSuccessful = false;
    int error;
    QString uploadError;

    try {

        // parse the response
        QString response = reply->readAll();
        MVJSONReader jsonResponse(string(response.toLatin1()));

        // get values
        error = jsonResponse.root->getFieldInt("error_code");
        uploadError = jsonResponse.root->getFieldString("message").c_str();
        //XXX selfloopsActivityId = jsonResponse.root->getFieldInt("activity_id");

    } catch(...) {

        // problem!
        error = 500;
        uploadError = "bad response or parser exception.";
        //XXX selfloopsActivityId = 0;
    }

    // set tag for upload id
    if (error>0 || reply->error() != QNetworkReply::NoError) uploadSuccessful=false;
    else {

        //qDebug() << "activity: " << selfloopsActivityId;

        //XXX ride->ride()->setTag("Selfloops activityId", QString("%1").arg(selfloopsActivityId));
        //XXX ride->setDirty(true);
        uploadSuccessful = true;
    }

    if (uploadSuccessful && reply->error() == QNetworkReply::NoError) {
        notifyWriteComplete( replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Completed."));
    } else {
        notifyWriteComplete( replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Network Error - Upload failed."));
    }
}

static bool addSelfloops() {
    CloudServiceFactory::instance().addService(new Selfloops(NULL));
    return true;
}

static bool add = addSelfloops();
