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

#include "RideWithGPS.h"
#include "Athlete.h"
#include "Settings.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef RIDEWITHGPS_DEBUG
#define RIDEWITHGPS_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (RIDEWITHGPS_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (RIDEWITHGPS_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

RideWithGPS::RideWithGPS(Context *context) : CloudService(context), context(context), root_(NULL) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = none; // gzip
    useMetric = true; // distance and duration metadata
}

RideWithGPS::~RideWithGPS() {
    if (context) delete nam;
}

void
RideWithGPS::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

bool
RideWithGPS::open(QStringList &errors)
{
    printd("RideWithGPS::open\n");
    return true;
}

bool
RideWithGPS::close()
{
    printd("RideWithGPS::close\n");
    // nothing to do for now
    return true;
}

bool
RideWithGPS::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    Q_UNUSED(ride);

    printd("RideWithGPS::writeFile(%s)\n", remotename.toStdString().c_str());

    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply,remotename);
    return true;
}

void
RideWithGPS::writeFileCompleted()
{
    printd("RideWithGPS::writeFileCompleted()\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s\n", reply->readAll().toStdString().c_str());

    if (reply->error() == QNetworkReply::NoError) {
        notifyWriteComplete(
            replyName(static_cast<QNetworkReply*>(QObject::sender())),
            tr("Completed."));
    } else {
        notifyWriteComplete(
            replyName(static_cast<QNetworkReply*>(QObject::sender())),
            tr("Network Error - Upload failed."));
    }
}

static bool addRideWithGPS() {
    CloudServiceFactory::instance().addService(new RideWithGPS(NULL));
    return true;
}

static bool add = addRideWithGPS();
