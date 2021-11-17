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
    //downloadCompression = none;
    filetype = uploadType::JSON;
    //useMetric = true; // distance and duration metadata

    // config
    settings.insert(OAuthToken, GC_NOLIO_ACCESS_TOKEN);
    settings.insert(URL, GC_NOLIO_URL);
    settings.insert(DefaultURL, "https://www.nolio.io");
    settings.insert(Local1, GC_NOLIO_REFRESH_TOKEN);
    settings.insert(Local2, GC_NOLIO_LAST_REFRESH);
    //settings.insert(Key, GC_NOLIO_USERKEY);
    //settings.insert(AthleteID, GC_NOLIO_ATHLETE_ID);
    //settings.insert(Local1, GC_NOLIO_ATHLETE_NAME);
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
    QNetworkRequest request(QUrl("https://www.nolio.io/api/token/"));
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
    qDebug() << path << from << to;
    QList<CloudServiceEntry*> returning;

    // do we have a token
    QString access_token = getSetting(GC_NOLIO_ACCESS_TOKEN, "").toString();
    if (access_token == "") {
        errors << "You must authorise with Nolio first";
        return returning;
    }

    QString urlstr = "https://www.nolio.io/api/get/training/";
    QUrl url = QUrl(urlstr);
    QNetworkRequest request(url);
    //printd("URL used: %s\n", url.url().toStdString().c_str());
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

                // printd("direntry: %s %s\n", add->id.toStdString().c_str(), add->name.toStdString().c_str());
                qDebug() << add->label << add->name;
                returning << add;
            }
        }
    }
    return returning;
}

bool Nolio::readFile(QByteArray *data, QString remotename, QString remoteid){
    printd("Nolio::readFile\n");
    qDebug() << "AAAAAAAAAAAAAAAAAAAAAa" << remoteid << remotename;

    // do we have a token
    QString access_token = getSetting(GC_NOLIO_ACCESS_TOKEN, "").toString();
    if (access_token == "") {
        qDebug() << "You must authorise with Nolio first";
        return false;
    }

    qDebug() << access_token;

    QString urlstr = "https://www.nolio.io/api/get/training/";
    QUrl url = QUrl(urlstr);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(access_token)).toLatin1());
    QNetworkReply *reply = nam->get(request);
    return false;
}

bool Nolio::createFolder(QString){
    printd("Nolio::createFolder\n");
    return false;
}

void Nolio::readyRead(){
    printd("Nolio::readyRead");
}
void Nolio::readFileCompleted(){
    printd("Nolio::readCompleted");
    /*
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    // even in debug mode we don't want the whole thing...
    printd("reply:%s\n", buffers.value(reply)->mid(0,500).toStdString().c_str());

    // prepateResponse will rename the file if it converts to JSON
    // to add RPE data, so we need to spot name changes to notify
    // upstream that it did (e.g. FIT => JSON)
    QString rename = replyName(reply);
    QByteArray* data = prepareResponse(buffers.value(reply), rename);

    // notify complete with a rename
    notifyReadComplete(data, rename, tr("Completed."));*/
}

bool Nolio::close(){
    return true;
}

QString Nolio::home(){
    return "";
}

static bool addNolio() {
    CloudServiceFactory::instance().addService(new Nolio(NULL));
    return true;
}

static bool add = addNolio();
