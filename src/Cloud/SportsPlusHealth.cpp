/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2015 Luca Rasina <luca.rasina@sphtechnology.ch>
 * Copyright (c) 2013 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#include "SportsPlusHealth.h"
#include "Athlete.h"
#include "Settings.h"
#include "mvjson.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef SPORTSPLUSHEALTH_DEBUG
#define SPORTSPLUSHEALTH_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (SPORTSPLUSHEALTH_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (SPORTSPLUSHEALTH_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

// api endpoint
const QString SPH_URL("http://www.sportplushealth.com/sport/en/api/1");

SportsPlusHealth::SportsPlusHealth(Context *context) : CloudService(context), context(context), root_(NULL) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = none; // gzip
    filetype = CloudService::uploadType::TCX;
    useMetric = true; // distance and duration metadata

    //config
    settings.insert(Username, GC_SPORTPLUSHEALTHUSER);
    settings.insert(Password, GC_SPORTPLUSHEALTHPASS);
}

SportsPlusHealth::~SportsPlusHealth() {
    if (context) delete nam;
}

void
SportsPlusHealth::onSslErrors(QNetworkReply *reply, const QList<QSslError>&errors)
{
    sslErrors(context->mainWindow, reply, errors);
}

bool
SportsPlusHealth::open(QStringList &errors)
{
    Q_UNUSED(errors);

    printd("SportsPlusHealth::open\n");
    return true;
}

bool
SportsPlusHealth::close()
{
    printd("SportsPlusHealth::close\n");
    // nothing to do for now
    return true;
}

bool
SportsPlusHealth::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    Q_UNUSED(ride);

    printd("SportsPlusHealth::writeFile(%s)\n", remotename.toStdString().c_str());

    // get credentials
    QString username = getSetting(GC_SPORTPLUSHEALTHUSER).toString();
    QString password = getSetting(GC_SPORTPLUSHEALTHPASS).toString();

    //Building the message content
    QHttpMultiPart *body = new QHttpMultiPart( QHttpMultiPart::FormDataType);

    //Including the optional session name
    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"session_name\""));
    textPart.setBody(QByteArray(remotename.toLatin1()));
    body->append(textPart);

    //Including the content data type
    QHttpPart dataTypePart;
    dataTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"format\""));
    dataTypePart.setBody("tcx");
    body->append(dataTypePart);

    //Including file in the request
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"datafile\"; filename=\"sph_file.tcx\"; type=\"text/xml\""));
    filePart.setBody(data);
    body->append(filePart);

    //Sending the authenticated post request to the API
    QUrl url(SPH_URL + "/" + username + "/importGC");
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Authorization", "Basic " + QByteArray(QString("%1:%2").arg(username).arg(password).toLatin1()).toBase64());

    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done
    reply = nam->post(request, body);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply,remotename);
    return true;
}

void
SportsPlusHealth::writeFileCompleted()
{
    printd("SportsPlusHealth::writeFileCompleted()\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    bool success=false;
    int errorcode=-1;

    printd("reply:%s\n", reply->readAll().toStdString().c_str());
    MVJSONReader jsonResponse(reply->readAll().toStdString().c_str());
    if(jsonResponse.root && jsonResponse.root->hasField("success") && jsonResponse.root->hasField("error_code")) {
        success = jsonResponse.root->getFieldBool("success");
        errorcode = jsonResponse.root->getFieldInt("error_code");
    } else {
        success = false;
        errorcode = -1;
    }

    if (success) {
        notifyWriteComplete(
            replyName(static_cast<QNetworkReply*>(QObject::sender())),
            tr("Completed."));
    } else {
        notifyWriteComplete(
            replyName(static_cast<QNetworkReply*>(QObject::sender())),
            QString(tr("Upload failed. (%1)")).arg(errorcode));
    }
}

static bool addSportsPlusHealth() {
    CloudServiceFactory::instance().addService(new SportsPlusHealth(NULL));
    return true;
}

static bool add = addSportsPlusHealth();
