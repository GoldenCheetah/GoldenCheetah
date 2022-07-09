/*
 * Copyright (c) 2017 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#include "Xert.h"
#include "Athlete.h"
#include "Settings.h"
#include "Units.h"
#include "JsonRideFile.h"
#include "mvjson.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef XERT_DEBUG
#define XERT_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (XERT_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (XERT_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

Xert::Xert(Context *context) : CloudService(context), context(context), root_(NULL) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = none;
    downloadCompression = none;
    filetype = FIT;
    useMetric = true; // distance and duration metadata

    // config
    settings.insert(Username, GC_XERTUSER);
    settings.insert(Password, GC_XERTPASS);
    settings.insert(OAuthToken, GC_XERT_TOKEN);
    settings.insert(Local1, GC_XERT_REFRESH_TOKEN);
    settings.insert(Local2, GC_XERT_LAST_REFRESH);

}

Xert::~Xert() {
    if (context) delete nam;
}

void
Xert::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

bool
Xert::open(QStringList &errors)
{
    printd("Xert::open\n");

    // do we have a token
    QString token = getSetting(GC_XERT_REFRESH_TOKEN, "").toString();
    if (token == "") {
        errors << "You must authorise with Xert first";
        return false;
    }

    printd("Get access token for this session.\n");

    // refresh endpoint
    QNetworkRequest request(QUrl("https://www.xertonline.com/oauth/token"));
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");

    QString authheader = QString("%1:%2").arg(GC_XERT_CLIENT_ID).arg(GC_XERT_CLIENT_SECRET);

    request.setRawHeader("Authorization", "Basic " +  authheader.toLatin1().toBase64());

    // set params
    QString data;
    data += "refresh_token=" + getSetting(GC_XERT_REFRESH_TOKEN).toString();
    data += "&grant_type=refresh_token";

    // make request
    QNetworkReply* reply = nam->post(request, data.toLatin1());

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    printd("HTTP response code: %d\n", statusCode);

    // oops, no dice
    if (reply->error() != 0) {
        printd("Got error %s\n", reply->errorString().toStdString().c_str());
        errors << reply->errorString();
        return false;
    }

    // lets extract the access token, and possibly a new refresh token
    QByteArray r = reply->readAll();
    printd("Got response: %s\n", r.data());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // failed to parse result !?
    if (parseError.error != QJsonParseError::NoError) {
        printd("Parse error!\n");
        errors << tr("JSON parser error") << parseError.errorString();
        return false;
    }

    QString access_token = document.object()["access_token"].toString();
    QString refresh_token = document.object()["refresh_token"].toString();

    // update our settings
    if (access_token != "") setSetting(GC_XERT_TOKEN, access_token);
    if (refresh_token != "") setSetting(GC_XERT_REFRESH_TOKEN, refresh_token);
    setSetting(GC_XERT_LAST_REFRESH, QDateTime::currentDateTime());

    // get the factory to save our settings permanently
    CloudServiceFactory::instance().saveSettings(this, context);
    return true;
}

bool
Xert::close()
{
    printd("Xert::close\n");
    // nothing to do for now
    return true;
}

QList<CloudServiceEntry*>
Xert::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    printd("Xert::readdir(%s)\n", path.toStdString().c_str());

    QList<CloudServiceEntry*> returning;

    // do we have a token
    QString token = getSetting(GC_XERT_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with Xert first");
        return returning;
    }

    QString urlstr("https://www.xertonline.com/oauth/activity?");

    QUrlQuery params;

    // use toMSecsSinceEpoch for compatibility with QT4
    params.addQueryItem("to", QString::number(to.addDays(1).toMSecsSinceEpoch()/1000.0f, 'f', 0));
    params.addQueryItem("from", QString::number(from.addDays(-1).toMSecsSinceEpoch()/1000.0f, 'f', 0));

    QUrl url = QUrl( urlstr + params.toString() );
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(getSetting(GC_XERT_TOKEN,"").toString()).toLatin1());
    request.setRawHeader("Accept", "application/json");

    // make request
    printd("fetch : %s\n", urlstr.toStdString().c_str());
    QNetworkReply *reply = nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // if successful, lets unpack
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    printd("fetch response: %d: %s\n", reply->error(), reply->errorString().toStdString().c_str());

    if (reply->error() == 0) {

        // get the data
        QByteArray r = reply->readAll();

        printd("page : %s\n", r.toStdString().c_str());

        // parse JSON payload
        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        printd("parse (%d): %s\n", parseError.error, parseError.errorString().toStdString().c_str());
        if (parseError.error == QJsonParseError::NoError) {

            QJsonArray activities = document.object()["activities"].toArray();
            for(int i=0; i<activities.count(); i++) {

                QJsonObject activity = activities.at(i).toObject();
                CloudServiceEntry *add = newCloudServiceEntry();

                // each item looks like this:
                // {
                //      "name":"2267051020",
                //      "start_date":{
                //          "date":"2017-11-02 13:54:15.000000",
                //          "timezone_type":3,"timezone":"UTC"
                //      },
                //      "description":null,
                //      "path":"eef8hezs8eh24hv4",
                //      "activity_type":"Cycling"
                // }

                QDateTime startDate = QDateTime::fromString(activity["start_date"].toObject()["date"].toString(), Qt::ISODate);
                startDate.setTimeSpec(Qt::UTC);
                startDate = startDate.toLocalTime();

                add->name = startDate.toString("yyyy_MM_dd_HH_mm_ss")+".json";
                add->distance = activity["distance"].toDouble() / 1000.0f;
                add->duration = activity["duration"].toDouble();
                add->id = activity["path"].toString();
                add->label = activity["name"].toString();
                add->isDir = false;

                // Details
                QJsonObject detail = readActivityDetail(add->id, false);
                add->distance = detail["summary"].toObject()["distance"].toDouble();
                add->duration = detail["summary"].toObject()["duration"].toDouble();

                printd("item: %s\n", add->name.toStdString().c_str());

                returning << add;
            }
        }
    }

    // all good ?
    printd("returning count(%d), errors(%s)\n", returning.count(), errors.join(",").toStdString().c_str());
    return returning;
}

QString
Xert::getRideName(RideFile *ride)
{
    QString name = "";
    // is "Name" set?
    if (!ride->getTag("Name", "").isEmpty()) {
        name = ride->getTag("Name", "");
    } else {
        // is "Route" set?
        if (!ride->getTag("Route", "").isEmpty()) {
            name = ride->getTag("Route", "");
        } else {
            //  is Workout Code set?
            if (!ride->getTag("Workout Title", "").isEmpty()) {
                name = ride->getTag("Workout Title", "");
            } else if (!ride->getTag("Workout Code", "").isEmpty()) {
                name = ride->getTag("Workout Code", "");
            }
        }
    }
    return name;
}

QJsonObject
Xert::readActivityDetail(QString path, bool withSessionData)
{
    printd("Xert::readDetail(%s)\n", path.toStdString().c_str());

    QString urlstr("https://www.xertonline.com/oauth/activity/"+path);
    if (withSessionData)
        urlstr += "?include_session_data=1";
    QUrl url = QUrl( urlstr );
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(getSetting(GC_XERT_TOKEN,"").toString()).toLatin1());
    request.setRawHeader("Accept", "application/json");

    // make request
    printd("fetch : %s\n", urlstr.toStdString().c_str());
    QNetworkReply *reply = nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // if successful, lets unpack
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    printd("fetch response: %d: %s\n", reply->error(), reply->errorString().toStdString().c_str());

    if (reply->error() == 0) {

        // get the data
        QByteArray data = reply->readAll();

        printd("page : %s\n", data.toStdString().c_str());

        // parse JSON payload
        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(data, &parseError);

        printd("parse (%d): %s\n", parseError.error, parseError.errorString().toStdString().c_str());
        if (parseError.error == QJsonParseError::NoError) {

            return document.object();

        }
    }

    return QJsonObject();
}

// read a file at location (relative to home) into passed array
bool
Xert::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("Xert::readFile(%s, %s)\n", remotename.toStdString().c_str(), remoteid.toStdString().c_str());

    // do we have a token ?
    QString token = getSetting(GC_XERT_TOKEN, "").toString();
    if (token == "") return false;

    QString url("https://www.xertonline.com/oauth/activity/"+remoteid+"?include_session_data=1");

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

void
Xert::readyRead()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

// SportTracks workouts are delivered back as JSON, the original is lost
// so we need to parse the response and turn it into a JSON file to
// import. The description of the format is here:
// https://sporttracks.mobi/api/doc/data-structures
void
Xert::readFileCompleted()
{
    printd("Xert::readFileCompleted\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s ...\n", buffers.value(reply)->toStdString().substr(0,900).c_str());
    QByteArray *returning = buffers.value(reply);

    // we need to parse the JSON and create a ridefile from it
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(*buffers.value(reply), &parseError);

    printd("parse (%d): %s\n", parseError.error, parseError.errorString().toStdString().c_str());
    if (parseError.error == QJsonParseError::NoError) {

        QJsonObject ride = document.object();
        QDateTime starttime = QDateTime::fromString(ride["start_date"].toString(), Qt::ISODate);
        starttime.setTimeSpec(Qt::UTC);
        starttime = starttime.toLocalTime();

        // 1s samples with start time as UTC
        RideFile *ret = new RideFile(starttime, 1.0f);

        // what sport?
        if (!ride["summary"].toObject()["activity_type"].isNull()) {
            QString stype = ride["summary"].toObject()["activity_type"].toString();
            if (stype == "Cycling") ret->setTag("Sport", "Bike");
            else if (stype == "Running") ret->setTag("Sport", "Run");
            else if (stype == "Swimming") ret->setTag("Sport", "Swim");
            else ret->setTag("Sport", stype);
        }

        // location => route
        if (!ride["name"].isNull()) ret->setTag("Objectives", ride["name"].toString());
        if (!ride["notes"].isNull()) ret->setTag("Notes", ride["description"].toString());

        // SAMPLES DATA
        //

        QJsonArray sessionData = ride["session_data"].toArray();

        long unix_starttime=0;

        for (int i = 0; i< sessionData.count(); i++) {
            QJsonObject data = sessionData.at(i).toObject();
            RideFilePoint add;


            if (!data["unix_time"].isNull()) {
                if (unix_starttime == 0)
                    unix_starttime = data["unix_time"].toDouble();
                add.secs = (data["unix_time"].toDouble()-unix_starttime) / 1000;
            }

            if (!data["power"].isNull())
                add.watts = data["power"].toDouble();

            if (!data["hr"].isNull())
                add.hr = data["hr"].toInt();

            if (!data["cad"].isNull())
                add.cad = data["cad"].toDouble();

            if (!data["alt"].isNull())
                add.alt = data["alt"].toDouble();

            if (!data["spd"].isNull())
                add.kph = data["spd"].toDouble() / 1000.0;

            if (!data["dist"].isNull())
                add.km = data["dist"].toDouble() / 1000.0;

            if (!data["lat"].isNull())
                add.lat = data["lat"].toDouble();

            if (!data["lng"].isNull())
                add.lon = data["lng"].toDouble();

            ret->appendPoint(add);

        }


        // create a response
        JsonFileReader reader;
        returning->clear();
        returning->append(reader.toByteArray(context, ret, true, true, true, true));

        // temp ride not needed anymore
        delete ret;

    } else {

        returning->clear();
    }

    // return
    notifyReadComplete(returning, replyName(reply), tr("Completed."));
}

bool
Xert::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    printd("Xert::writeFile(%s)\n", remotename.toStdString().c_str());

    // do we have a token ?
    QString token = getSetting(GC_XERT_TOKEN, "").toString();
    if (token == "") return false;

    QString url = QString("https://www.xertonline.com/oauth/upload");

    QNetworkRequest request = QNetworkRequest(url);

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QString boundary = QVariant(qrand()).toString()+QVariant(qrand()).toString()+QVariant(qrand()).toString();
    multiPart->setBoundary(boundary.toLatin1());

    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    QString name = getRideName(ride);

    if (name != "") {
        QHttpPart namePart;
        namePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"name\""));
        printd("request: %s\n", name.toStdString().c_str());
        namePart.setBody(name.toLatin1());
        multiPart->append(namePart);
    }

    QHttpPart attachmentPart;
    attachmentPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\""+remotename+"\""));
    attachmentPart.setBody(data);
    multiPart->append(attachmentPart);

    // post the file
    QNetworkReply *reply;

    reply = nam->post(request, multiPart);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply,remotename);
    return true;
}

void
Xert::writeFileCompleted()
{
    printd("Xert::writeFileCompleted()\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    bool uploadSuccessful = false;

    bool success;
    QString uploadError;

    try {

        // parse the response
        QString response = reply->readAll();
        printd("reply:%s\n", response.toStdString().c_str());

        MVJSONReader jsonResponse(string(response.toLatin1()));

        // get values
        //uploadError = jsonResponse.root->getFieldString("error").c_str();
        success = jsonResponse.root->getFieldBool("success");
        if (!success)
            uploadError = "upload not successful";


    } catch(...) {

        // problem!
        uploadError = "bad response or parser exception.";
    }

    if (uploadError.length()>0 || reply->error() != QNetworkReply::NoError) uploadSuccessful = false;
    else {
        uploadSuccessful = true;
    }

    if (uploadSuccessful && reply->error() == QNetworkReply::NoError) {
        notifyWriteComplete( replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Completed."));
    } else {
        notifyWriteComplete( replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Network Error - Upload failed."));
    }
}

static bool addXert() {
    CloudServiceFactory::instance().addService(new Xert(NULL));
    return true;
}

static bool add = addXert();
