/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

#include "Strava.h"
#include "Athlete.h"
#include "Settings.h"
#include "mvjson.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef STRAVA_DEBUG
#define STRAVA_DEBUG true
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (STRAVA_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (STRAVA_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

Strava::Strava(Context *context) : CloudService(context), context(context), root_(NULL) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = gzip; // gzip
    filetype = uploadType::TCX;
    useMetric = true; // distance and duration metadata

    // config
    settings.insert(OAuthToken, GC_STRAVA_TOKEN);
}

Strava::~Strava() {
    if (context) delete nam;
}

void
Strava::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

bool
Strava::open(QStringList &errors)
{
    printd("Strava::open\n");
    QString token = getSetting(GC_STRAVA_TOKEN, "").toString();
    if (token == "") {
        errors << tr("No authorisation token configured.");
        return false;
    }
    return true;
}

bool
Strava::close()
{
    printd("Strava::close\n");
    // nothing to do for now
    return true;
}

QList<CloudServiceEntry*>
Strava::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    printd("Strava::readdir(%s)\n", path.toStdString().c_str());

    QList<CloudServiceEntry*> returning;

    // do we have a token
    QString token = getSetting(GC_STRAVA_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with Strava first");
        return returning;
    }

    QString urlstr = "https://www.strava.com/api/v3/athlete/activities?";

#if QT_VERSION > 0x050000
    QUrlQuery params;
#else
    QUrl params;
#endif

    // use toMSecsSinceEpoch for compatibility with QT4
    params.addQueryItem("before", QString::number(to.toMSecsSinceEpoch()/1000.0f, 'f', 0));
    params.addQueryItem("after", QString::number(from.toMSecsSinceEpoch()/1000.0f, 'f', 0));

    QUrl url = QUrl( urlstr + params.toString() );
    printd("URL used: %s\n", url.url().toStdString().c_str());

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    QNetworkReply *reply = nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "error" << reply->errorString();
        errors << tr("Network Problem reading Strava data");
        return returning;
    }
    // did we get a good response ?
    QByteArray r = reply->readAll();
    printd("response: %s\n", r.toStdString().c_str());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {

        // results ?
        QJsonArray results = document.array();

        // lets look at that then
        for(int i=0; i<results.size(); i++) {
            QJsonObject each = results.at(i).toObject();
            CloudServiceEntry *add = newCloudServiceEntry();


            //Strava has full path, we just want the file name
            add->label = QFileInfo(each["name"].toString()).fileName();
            add->id = QString("%1").arg(each["id"].toInt());
            add->isDir = false;
            add->distance = each["distance"].toDouble()/1000.0;
            add->duration = each["elapsed_time"].toInt();
            add->name = QDateTime::fromString(each["start_date"].toString(), Qt::ISODate).toString("yyyy_MM_dd_HH_mm_ss")+".json";

            printd("direntry: %s %s\n", add->id.toStdString().c_str(), add->name.toStdString().c_str());

            returning << add;
        }
    }
    // all good ?
    return returning;
}

// read a file at location (relative to home) into passed array
bool
Strava::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("Strava::readFile(%s)\n", remotename.toStdString().c_str());

    // do we have a token ?
    QString token = getSetting(GC_STRAVA_TOKEN, "").toString();
    if (token == "") return false;

    // lets connect and get basic info on the root directory
    QString url = QString("https://www.strava.com/api/v3/activities/%1")
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
    return true;
}

bool
Strava::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    Q_UNUSED(ride);

    printd("Strava::writeFile(%s)\n", remotename.toStdString().c_str());

    QString token = getSetting(GC_STRAVA_TOKEN, "").toString();

    // access the original file for ride start
    QDateTime rideDateTime = ride->startTime();

    // trap network response from access manager

    QUrl url = QUrl( "https://www.strava.com/api/v3/uploads" ); // The V3 API doc said "https://api.strava.com" but it is not working yet
    QNetworkRequest request = QNetworkRequest(url);

    //QString boundary = QString::number(qrand() * (90000000000) / (RAND_MAX + 1) + 10000000000, 16);
    QString boundary = QVariant(qrand()).toString() +
        QVariant(qrand()).toString() + QVariant(qrand()).toString();

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    multiPart->setBoundary(boundary.toLatin1());

    QHttpPart accessTokenPart;
    accessTokenPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                              QVariant("form-data; name=\"access_token\""));
    accessTokenPart.setBody(token.toLatin1());

    QHttpPart activityTypePart;
    activityTypePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                               QVariant("form-data; name=\"activity_type\""));
    if (ride->isRun())
      activityTypePart.setBody("run");
    else if (ride->isSwim())
      activityTypePart.setBody("swim");
    else
      activityTypePart.setBody("ride");

    QHttpPart activityNamePart;
    activityNamePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"activity_name\""));
    activityNamePart.setBody(remotename.toLatin1());

    QHttpPart dataTypePart;
    dataTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"data_type\""));
    dataTypePart.setBody("tcx.gz");

    QHttpPart externalIdPart;
    externalIdPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"external_id\""));
    externalIdPart.setBody("Ride");

    //XXXQHttpPart privatePart;
    //XXXprivatePart.setHeader(QNetworkRequest::ContentDispositionHeader,
    //XXX                      QVariant("form-data; name=\"private\""));
    //XXXprivatePart.setBody(parent->privateChk->isChecked() ? "1" : "0");

    //XXXQHttpPart commutePart;
    //XXXcommutePart.setHeader(QNetworkRequest::ContentDispositionHeader,
    //XXX                      QVariant("form-data; name=\"commute\""));
    //XXXcommutePart.setBody(parent->commuteChk->isChecked() ? "1" : "0");
    //XXXQHttpPart trainerPart;
    //XXXtrainerPart.setHeader(QNetworkRequest::ContentDispositionHeader,
    //XXX                      QVariant("form-data; name=\"trainer\""));
    //XXXtrainerPart.setBody(parent->trainerChk->isChecked() ? "1" : "0");

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/xml"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"file.tcx.gz\"; type=\"text/xml\""));
    filePart.setBody(data);

    multiPart->append(accessTokenPart);
    multiPart->append(activityTypePart);
    multiPart->append(activityNamePart);
    multiPart->append(dataTypePart);
    multiPart->append(externalIdPart);
    //XXXmultiPart->append(privatePart);
    //XXXmultiPart->append(commutePart);
    //XXXmultiPart->append(trainerPart);
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
Strava::writeFileCompleted()
{
    printd("Strava::writeFileCompleted()\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s\n", reply->readAll().toStdString().c_str());

    bool uploadSuccessful = false;
    QString response = reply->readLine();
    QString uploadError="invalid response or parser error";

    try {

        // parse !
        MVJSONReader jsonResponse(string(response.toLatin1()));

        // get error field
        if (jsonResponse.root) {
            if (jsonResponse.root->hasField("error")) {
                uploadError = jsonResponse.root->getFieldString("error").c_str();
            } else {
                uploadError = ""; // no error
            }

            // get upload_id, but if not available use id
            //XXX if (jsonResponse.root->hasField("upload_id")) {
            //XXX     stravaUploadId = jsonResponse.root->getFieldInt("upload_id");
            //XXX } else if (jsonResponse.root->hasField("id")) {
            //XXX     stravaUploadId = jsonResponse.root->getFieldInt("id");
            //XXX } else {
            //XXX     stravaUploadId = 0;
            //XXX }
        } else {
            uploadError = "no connection";
        }

    } catch(...) { // not really sure what exceptions to expect so do them all (bad, sorry)
        uploadError=tr("invalid response or parser exception.");
    }

    if (uploadError.toLower() == "none" || uploadError.toLower() == "null") uploadError = "";

    // if successful update ID
    if (uploadError.length()>0 || reply->error() != QNetworkReply::NoError)  uploadSuccessful = false;
    else {

        //XXXride->ride()->setTag("Strava uploadId", QString("%1").arg(stravaUploadId));
        //XXXride->setDirty(true);

        //qDebug() << "uploadId: " << stravaUploadId;
        uploadSuccessful = true;
    }

    // return response
    if (uploadSuccessful && reply->error() == QNetworkReply::NoError) {
        notifyWriteComplete(replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Completed."));
    } else {
        notifyWriteComplete(replyName(static_cast<QNetworkReply*>(QObject::sender())), uploadError);
    }
}

void
Strava::readyRead()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

void
Strava::readFileCompleted()
{
    printd("Strava::readFileCompleted\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s\n", buffers.value(reply)->toStdString().c_str());

    QByteArray* data = prepareResponse(buffers.value(reply), replyName(reply));

    notifyReadComplete(data, replyName(reply), tr("Completed."));
}

QByteArray*
Strava::prepareResponse(QByteArray* data, QString name)
{
    printd("Strava::prepareResponse()\n");

    return data;
}

static bool addStrava() {
    CloudServiceFactory::instance().addService(new Strava(NULL));
    return true;
}

static bool add = addStrava();

