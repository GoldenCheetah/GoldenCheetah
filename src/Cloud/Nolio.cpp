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


bool Nolio::close(){
    printd("Nolio::close\n");
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
