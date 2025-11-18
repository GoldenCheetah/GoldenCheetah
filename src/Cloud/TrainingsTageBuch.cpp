/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2013 Damien.Grauser (damien.grauser@pev-geneve.ch)
 * Copyright (c) 2012 Rainer Clasen <bj@zuto.de>
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

#include "TrainingsTageBuch.h"
#include "Athlete.h"
#include "Settings.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef TRAININGSTAGEBUCH_DEBUG
#define TRAININGSTAGEBUCH_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (TRAININGSTAGEBUCH_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (TRAININGSTAGEBUCH_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

const QString TTB_URL( "http://trainingstagebuch.org" );

TrainingsTageBuch::TrainingsTageBuch(Context *context) : CloudService(context), context(context), root_(NULL) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = none; // gzip
    filetype = CloudService::uploadType::PWX;
    useMetric = true; // distance and duration metadata

    //config
    settings.insert(Username, GC_TTBUSER);
    settings.insert(Password, GC_TTBPASS);
}

TrainingsTageBuch::~TrainingsTageBuch() {
    if (context) delete nam;
}

void
TrainingsTageBuch::onSslErrors(QNetworkReply *reply, const QList<QSslError>&errors)
{
    CloudDBCommon::sslErrors(reply, errors);
}

bool
TrainingsTageBuch::open(QStringList &errors)
{
    // get a session token, then get the settings for the account
    printd("TrainingStageBuch::open\n");

    // GET ACCOUNT SETTINGS
    QString username = getSetting(GC_TTBUSER).toString();
    QString password = getSetting(GC_TTBPASS).toString();

    QUrlQuery urlquery;
    urlquery.addQueryItem( "view", "xml" );
    urlquery.addQueryItem( "user", username );
    urlquery.addQueryItem( "pass", password );

    QUrl url (TTB_URL + "/settings/list");
    url.setQuery(urlquery.query());
    QNetworkRequest request = QNetworkRequest(url);

    request.setRawHeader( "Accept-Encoding", "identity" );
    request.setRawHeader( "Accept", "application/xml" );
    request.setRawHeader( "Accept-Charset", "utf-8" );

    // block waiting for response...
    QEventLoop loop;
    reply = nam->get(request);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    TTBSettingsParser handler;
    QXmlInputSource source(reply);

    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);

    if(! reader.parse(source) ){
        errors << (tr("failed to parse Settings response: ")+handler.errorString());
        return false;
    }

    if( handler.error.length() > 0 ){
        errors << (tr("failed to get settings: ") +handler.error);
        return false;
    }

    sessionId = handler.session;
    proMember = handler.pro;

    // if we got a session id, no need to go further.
    if(sessionId.length() > 0) return true;

    // GET SESSION TOKEN

    urlquery = QUrlQuery();
    urlquery.addQueryItem( "view", "xml" );
    urlquery.addQueryItem( "user", username );
    urlquery.addQueryItem( "pass", password );

    url = QUrl(TTB_URL + "/login/sso");
    url.setQuery(urlquery.query());
    request = QNetworkRequest(url);

    request.setRawHeader( "Accept-Encoding", "identity" );
    request.setRawHeader( "Accept", "application/xml" );
    request.setRawHeader( "Accept-Charset", "utf-8" );

    // block waiting for response
    reply = nam->get(request);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    TTBSessionParser shandler;
    QXmlInputSource ssource(reply);

    reader.setContentHandler(&shandler);

    if(! reader.parse(ssource)) {
        errors << (tr("failed to parse Session response: ")+shandler.errorString());
        return false;
    }

    if(handler.error.length() > 0){
        errors << (tr("failed to get new session: ") +shandler.error );
        return false;
    }

    sessionId = shandler.session;

    if(sessionId.length() == 0){
        errors << (tr("got empty session"));
        return false;
    }

    // SUCCESS
    return true;
}

bool
TrainingsTageBuch::close()
{
    printd("TrainingStageBuch::close\n");
    // nothing to do for now
    return true;
}

bool
TrainingsTageBuch::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    Q_UNUSED(ride);

    printd("TrainingStageBuch::writeFile(%s)\n", remotename.toStdString().c_str());

    QHttpMultiPart *body = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader,
    QVariant("form-data; name=\"upload_submit\""));
    textPart.setBody("hrm");
    body->append(textPart);

    int limit = proMember ? 8 * 1024 * 1024 : 4 * 1024 * 1024;
    if(data.size() >= limit ){
        return false;
    }

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
    QVariant("application/octet-stream"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
    QVariant("form-data; name=\"file\"; filename=\"gc-upload-ttb.pwx\""));
    filePart.setBody(data);
    body->append(filePart);

    QUrlQuery urlquery;
    urlquery.addQueryItem( "view", "xml" );
    urlquery.addQueryItem( "sso", sessionId );


    QUrl url (TTB_URL + "/file/upload");
    url.setQuery(urlquery.query());
    QNetworkRequest request = QNetworkRequest(url);

    request.setRawHeader( "Accept-Encoding", "identity" );
    request.setRawHeader( "Accept", "application/xml" );
    request.setRawHeader( "Accept-Charset", "utf-8" );

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
TrainingsTageBuch::writeFileCompleted()
{
    printd("TrainingStageBuch::writeFileCompleted()\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s\n", reply->readAll().toStdString().c_str());

    TTBUploadParser handler;
    QXmlInputSource source(reply);

    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);

    bool success = true;
    if(! reader.parse(source)) {
        success = false;
    }

    if(success && handler.error.length() > 0){
        success = false;
    }

    if(success && handler.id.length() == 0 ){
        success = false;
    }

    if (success && reply->error() == QNetworkReply::NoError) {
        notifyWriteComplete(replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Completed."));
    } else {
        notifyWriteComplete( replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Error - Upload failed."));
    }
}

static bool addTrainingStageBuch() {
    CloudServiceFactory::instance().addService(new TrainingsTageBuch(NULL));
    return true;
}

static bool add = addTrainingStageBuch();
