/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2012 Rainer Clasen <bj@zuto.de>
 * Copyright (c) 2013 Damien.Grauser (damien.grauser@pev-geneve.ch)
 * Copyright (c) 2014 Nils Knieling copied from TtbDialog.cpp
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

#include "Velohero.h"
#include "Athlete.h"
#include "Settings.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef VELOHERO_DEBUG
#define VELOHERO_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (VELOHERO_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (VELOHERO_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

static const QString VELOHERO_URL( "https://app.velohero.com" );

Velohero::Velohero(Context *context) : CloudService(context), context(context), root_(NULL) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = none; // gzip
    filetype = CloudService::uploadType::PWX;
    useMetric = true; // distance and duration metadata

    //config
    settings.insert(Username, GC_VELOHEROUSER);
    settings.insert(Password, GC_VELOHEROPASS);
}

Velohero::~Velohero() {
    if (context) delete nam;
}

void
Velohero::onSslErrors(QNetworkReply *reply, const QList<QSslError>&errors)
{
    sslErrors(context->mainWindow, reply, errors);
}

bool
Velohero::open(QStringList &errors)
{
    printd("Velohero::open\n");

    QString username = getSetting(GC_VELOHEROUSER).toString();
    QString password = getSetting(GC_VELOHEROPASS).toString();

    QUrlQuery urlquery;
    urlquery.addQueryItem("view", "xml");
    urlquery.addQueryItem("user", username);
    urlquery.addQueryItem("pass", password);

    QUrl url (VELOHERO_URL + "/sso");
    url.setQuery(urlquery.query());
    QNetworkRequest request = QNetworkRequest(url);

    request.setRawHeader("Accept-Encoding", "identity");
    request.setRawHeader("Accept", "application/xml");
    request.setRawHeader("Accept-Charset", "utf-8");
    request.setRawHeader("User-Agent", "GoldenCheetah/1.0");

    QEventLoop loop;
    reply = nam->get(request);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    VeloHeroSessionParser handler;
    QXmlInputSource source(reply);

    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);

    if(!reader.parse(source)) {
        errors << tr("Failed to parse sessionID response.");
        return false;
    }

    if(handler.error.length() > 0) {
        errors << tr("failed to get new session: ") + handler.error;
        return false;
    }

    sessionId = handler.session;

    if(sessionId.length() == 0) {
        errors << tr("got empty session");
        return false;
    }

    // have a sessionid
    printd("Velohere:: open session id=%s\n", sessionId.toStdString().c_str());
    return true;
}

bool
Velohero::close()
{
    printd("Velohero::close\n");
    // nothing to do for now
    return true;
}

bool
Velohero::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    Q_UNUSED(ride);

    printd("Velohero::writeFile(%s)\n", remotename.toStdString().c_str());
    QHttpMultiPart *body = new QHttpMultiPart( QHttpMultiPart::FormDataType );

    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader,
    QVariant("form-data; name=\"upload_submit\""));
    textPart.setBody("hrm");
    body->append( textPart );

    int limit = 16777216; // 16MB
    if( data.size() >= limit ){
        return false;
    }

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
    QVariant("application/octet-stream"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
    QVariant("form-data; name=\"file\"; filename=\"gc-upload-velohero.pwx\""));
    filePart.setBody(data);
    body->append(filePart);

    QUrlQuery urlquery;
    urlquery.addQueryItem( "view", "xml" );
    urlquery.addQueryItem( "sso", sessionId );

    QUrl url (VELOHERO_URL + "/upload/file");
    url.setQuery(urlquery.query());
    QNetworkRequest request = QNetworkRequest(url);

    request.setRawHeader( "Accept-Encoding", "identity" );
    request.setRawHeader( "Accept", "application/xml" );
    request.setRawHeader( "Accept-Charset", "utf-8" );
    request.setRawHeader( "User-Agent", "GoldenCheetah/1.0" );

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
Velohero::writeFileCompleted()
{
    printd("Velohero::writeFileCompleted()\n");
    printd("reply:%s\n", reply->readAll().toStdString().c_str());

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    VeloHeroPostParser handler;
    QXmlInputSource source(reply);

    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);

    bool success=true;
    if(!reader.parse(source)){ success=false; }

    if(handler.error.length() > 0) { success=false; }

    QString exerciseId = handler.id;

    if(exerciseId.length() == 0){ success=false; }


    if (success == true && reply->error() == QNetworkReply::NoError) {
        notifyWriteComplete(replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Completed."));
    } else {
        notifyWriteComplete(replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Network Error - Upload failed."));
    }
}

static bool addVelohero() {
    CloudServiceFactory::instance().addService(new Velohero(NULL));
    return true;
}

static bool add = addVelohero();
