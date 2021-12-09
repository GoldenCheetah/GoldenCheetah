/*
 * Copyright (c) 2017 Mark Liversedge <liversedge@gmail.com>
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
#include "Nolio.h"
#include "MainWindow.h"
#include "JsonRideFile.h"
#include "Athlete.h"
#include "Settings.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef NOLIO_DEBUG
#define NOLIO_DEBUG true
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (NOLIO_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (NOLIO_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

Nolio::Nolio(Context *context) : CloudService(context), context(context), root_(NULL) {
    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    //uploadCompression = gzip; // gzip
    downloadCompression = none;
    filetype = uploadType::JSON;
    //useMetric = true; // distance and duration metadata

    // config
    settings.insert(OAuthToken, GC_NOLIO_ACCESS_TOKEN);
    settings.insert(URL, GC_NOLIO_URL);
    settings.insert(DefaultURL, "https://www.nolio.io");
    settings.insert(Local1, GC_NOLIO_REFRESH_TOKEN);
    settings.insert(Local2, GC_NOLIO_LAST_REFRESH);
    //settings.insert(Key, GC_NOLIO_USERKEY);
    settings.insert(AthleteID, GC_NOLIO_ATHLETE_ID);
    settings.insert(Local3, GC_NOLIO_ATHLETE_NAME);
}

Nolio::~Nolio() {
    if (context) delete nam;
}

void Nolio::onSslErrors(QNetworkReply *reply, const QList<QSslError>&){
    reply->ignoreSslErrors();
}

bool Nolio::open(QStringList &errors){
    printd("Nolio::open\n");

    QString refresh_token = getSetting(GC_NOLIO_REFRESH_TOKEN, "").toString();
    if (refresh_token == "") {
        return false;
    }

    QString last_refresh_str = getSetting(GC_NOLIO_LAST_REFRESH, "0").toString();
    QDateTime last_refresh = QDateTime::fromString(last_refresh_str);
    last_refresh = last_refresh.addSecs(86400); // nolio tokens are valid for one day
    QDateTime now = QDateTime::currentDateTime();
    // check if we need to refresh the access token
    if (now <= last_refresh) { // credentials are still valid
        printd("tokens still valid\n");
        return true;
    }

    // get new credentials using refresh_token
    QNetworkRequest request(QUrl("https://nolio2.eu.ngrok.io/api/token/"));
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");

    QString authheader = QString("%1:%2").arg(GC_NOLIO_CLIENT_ID).arg(GC_NOLIO_CLIENT_SECRET);
    request.setRawHeader("Authorization", "Basic " + authheader.toLatin1().toBase64());

    QString data = QString("grant_type=refresh_token&refresh_token=").append(refresh_token);

    QNetworkReply* reply = nam->post(request, data.toLatin1());

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    printd("HTTP response code: %d\n", statusCode);

    if (reply->error() != 0) {
        printd("Got error %d\n", reply->error());
        return false;
    }
    QByteArray r = reply->readAll();
    printd("Got response: %s\n", r.data());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        printd("Parse error!\n");
        return false;
    }

    QString new_access_token = document.object()["access_token"].toString();
    QString new_refresh_token = document.object()["refresh_token"].toString();
    if (new_access_token != "") setSetting(GC_NOLIO_ACCESS_TOKEN, new_access_token);
    if (new_refresh_token != "") setSetting(GC_NOLIO_REFRESH_TOKEN, new_refresh_token);
    setSetting(GC_NOLIO_LAST_REFRESH, now.toString());
    CloudServiceFactory::instance().saveSettings(this, context);
    return true;
}

QList<CloudServiceEntry*> Nolio::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to){
    printd("Nolio::readdir(%s)\n", path.toStdString().c_str());
    QList<CloudServiceEntry*> returning;

    // do we have a token
    QString access_token = getSetting(GC_NOLIO_ACCESS_TOKEN, "").toString();
    if (access_token == "") {
        errors << "You must authorise with Nolio first";
        return returning;
    }


    QString userId = getSetting(GC_NOLIO_ATHLETE_ID, "").toString();

    QString urlstr = "https://nolio2.eu.ngrok.io/api/get/training/?";
    QUrlQuery params;
    params.addQueryItem("from", from.toString("yyyy-MM-dd"));
    params.addQueryItem("to", to.toString("yyyy-MM-dd"));
    if(userId.length() > 0) params.addQueryItem("athlete_id", userId);
    QUrl url = QUrl(urlstr + params.toString());
    QNetworkRequest request(url);
    // request using the bearer token
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(access_token)).toLatin1());
    QNetworkReply *reply = nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "error" << reply->errorString();
        errors << "Network Problem reading Nolio data";
        //return returning;
    }
    // did we get a good response ?
    QByteArray r = reply->readAll();

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {
        QJsonArray results = document.array();

        if (results.size() > 0) {
            for (int i = 0; i < results.size(); i++) {
                QJsonObject each = results.at(i).toObject();
                CloudServiceEntry *add = newCloudServiceEntry();

                add->label = QFileInfo(each["name"].toString()).fileName();
                add->id = QString("%1").arg(each["nolio_id"].toVariant().toULongLong());
                add->isDir = false;
                add->distance = each["distance"].toDouble();
                add->duration = each["duration"].toInt();
                add->name = QDateTime::fromString(each["date_start"].toString(), Qt::ISODate).toString("yyyy_MM_dd_HH_mm_ss")+".json";

                //qDebug() << add->label << add->name;
                returning << add;
            }
        }
    }
    return returning;
}

bool Nolio::readFile(QByteArray *data, QString remotename, QString remoteid){
    printd("Nolio::readFile\n");
    qDebug() << remoteid << remotename;

    // do we have a token
    QString access_token = getSetting(GC_NOLIO_ACCESS_TOKEN, "").toString();
    if (access_token == "") {
        qDebug() << "You must authorise with Nolio first";
        return false;
    }

    QString urlstr = "https://nolio2.eu.ngrok.io/api/get/training/info/?";
    QUrlQuery params;
    params.addQueryItem("id", remoteid);
    QUrl url = QUrl(urlstr + params.toString());
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(access_token)).toLatin1());

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

void Nolio::readyRead(){
    printd("Nolio::readyRead");
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

void Nolio::readFileCompleted(){
    printd("Nolio::readCompleted");
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    //printd("reply:%s\n", buffers.value(reply)->toStdString().c_str());
    QByteArray* data = prepareResponse(buffers.value(reply));
    notifyReadComplete(data, replyName(reply), tr("Completed."));
}

QByteArray* Nolio::prepareResponse(QByteArray* data){
    printd("Nolio::prepareResponse()\n");
    //qDebug() << *data;
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(data->constData(), &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {
        QJsonObject activity = document.object();
        QDateTime starttime = QDateTime::fromString(activity["date_start"].toString(), Qt::ISODate);
        // 1s samples with start time
        RideFile *ride = new RideFile(starttime.toUTC(), 1.0f);
        // set nolio id in metadata
        if (!activity["nolio_id"].isNull()) ride->setTag("NolioID",  QString("%1").arg(activity["id"].toVariant().toULongLong()));
        // what sport?
        if (!activity["sport_id"].isNull()) {
            double sport_id = activity["sport_id"].toDouble();
            if (sport_id == 14 or sport_id == 15 or sport_id == 18 or sport_id == 28 or sport_id == 35 or sport_id == 36) ride->setTag("Sport", "Bike");
            else if (sport_id == 23) ride->setTag("Sport", "Run");
            else if (sport_id == 19) ride->setTag("Sport", "Swim");
            else if (sport_id == 33 or sport_id == 69) ride->setTag("Sport", "Row");
            else if (sport_id == 3 or sport_id == 4) ride->setTag("Sport", "Ski");
            else ride->setTag("Sport", activity["sport"].toString());
        }
        if (!activity["rpe"].isNull()) {
            QString rpe = QString("%1").arg(activity["rpe"].toDouble());
            ride->setTag("RPE", rpe);
        }

        // description saved to Notes
        if (!activity["description"].isNull()) {
            ride->setTag("Notes", activity["description"].toString());
        }
        /*
        if (activity["distance"].toDouble()>0) {
            QMap<QString,QString> map;
            map.insert("value", QString("%1").arg(activity["distance"].toDouble()));
            ride->metricOverrides.insert("total_distance", map);
        }
        if (activity["duration"].toDouble()>0) {
            QMap<QString,QString> map;
            map.insert("value", QString("%1").arg(activity["duration"].toDouble()));
            ride->metricOverrides.insert("time_riding", map);
        }
        if (activity["elevation_gain"].toDouble()>0) {
            QMap<QString,QString> map;
            map.insert("value", QString("%1").arg(activity["elevation_gain"].toDouble()));
            ride->metricOverrides.insert("elevation_gain", map);
        }
        */
        QString athlete_name = getSetting(GC_NOLIO_ATHLETE_NAME, "").toString();
        if (athlete_name.length() > 0) ride->setTag("Athlete", athlete_name);

        // Streams

        QJsonArray streams = activity["streams"].toArray();
        if (streams.size() > 0) {
            for (int i = 0; i < streams.size(); i++) {
                QJsonObject sample = streams.at(i).toObject();

                RideFilePoint add;
                add.setValue(RideFile::secs, sample["time"].toDouble());
                add.setValue(RideFile::km, sample["distance"].toDouble() * 0.001f);
                add.setValue(RideFile::kph, sample["pace"].toDouble() * 3.6f);
                add.setValue(RideFile::alt, sample["altitude"].toDouble());
                add.setValue(RideFile::watts, sample["watts"].toDouble());
                add.setValue(RideFile::cad, sample["cadence"].toDouble());
                add.setValue(RideFile::hr, sample["heartrate"].toDouble());
                if (sample.contains("torque")) add.setValue(RideFile::nm, sample["torque"].toDouble());
                if (sample.contains("gps")){
                    QJsonArray gps_points = sample["gps"].toArray();
                    add.setValue(RideFile::lat, gps_points[0].toDouble());
                    add.setValue(RideFile::lon, gps_points[1].toDouble());
                }
                ride->appendPoint(add);
            }
        }

        JsonFileReader reader;
        data->clear();
        data->append(reader.toByteArray(context, ride, true, true, true, true));

        delete ride; // delete useless temp ride
    }
    printd("Nolio::prepareResponse() finished\n");
    //qDebug() << *data;
    return data;
}

QList<CloudServiceAthlete> Nolio::listAthletes(){
    printd("Nolio::listAthletes\n");
    QList<CloudServiceAthlete> returning;

    QString access_token = getSetting(GC_NOLIO_ACCESS_TOKEN, "").toString();
    if (access_token == "") {
        return returning;
    }
    QString urlstr = "https://nolio2.eu.ngrok.io/api/get/athletes/";
    QUrl url = QUrl(urlstr);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(access_token)).toLatin1());
    QNetworkReply *reply = nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // did we get a good response ?
    QByteArray r = reply->readAll();

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    if (parseError.error == QJsonParseError::NoError) {
        QJsonArray results = document.array();
        qDebug() << results;
        if (results.size() > 0) {
            for (int i = 0; i < results.size(); i++) {
                QJsonObject each = results.at(i).toObject();
                CloudServiceAthlete add;

                add.id = QString("%1").arg(each["nolio_id"].toInt());
                add.name = each["name"].toString();
                add.desc = QString("relationship: athlete");
                returning << add;
            }
        }
    }
    return returning;
}

bool Nolio::selectAthlete(CloudServiceAthlete athlete){
    printd("Nolio::selectAthlete\n");
    qDebug() << athlete.name << athlete.id;
    // extract athlete name and identifier from the selected athlete
    // TODO
    setSetting(GC_NOLIO_ATHLETE_ID, athlete.id.toInt());
    setSetting(GC_NOLIO_ATHLETE_NAME, athlete.name);
    return true;
}

bool Nolio::createFolder(QString){
    printd("Nolio::createFolder\n");
    return false;
}

bool Nolio::close(){
    return true;
}

QString Nolio::home() {
    return "";
}

static bool addNolio() {
    CloudServiceFactory::instance().addService(new Nolio(NULL));
    return true;
}

static bool add = addNolio();
