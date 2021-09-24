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
#define NOLIO_DEBUG false
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
    settings.insert(OAuthToken, GC_NOLIO_TOKEN);
    settings.insert(URL, GC_NOLIO_URL);
    settings.insert(DefaultURL, "https://nolio.io");
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

    // Check if we have a token
    QString token = getSetting(GC_NOLIO_TOKEN, "").toString();
    if (token == "") {
        errors << "No authorization token found for Nolio";
        return false;
    }

    // OAuth stuff

    return 0;
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
