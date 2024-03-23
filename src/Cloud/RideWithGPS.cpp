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
#include "Secrets.h"
#include "Settings.h"
#include "Units.h"
#include "mvjson.h"
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

    // config
    settings.insert(Username, GC_RWGPSUSER);
    settings.insert(Password, GC_RWGPSPASS);
    settings.insert(OAuthToken, GC_RWGPS_AUTH_TOKEN);
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
    /*QString username = getSetting(GC_RWGPSUSER).toString();
    if (username == "") {
        errors << tr("RideWithGPS account not configured.");
        return false;
    }*/
    // do we have a token
    QString token = getSetting(GC_RWGPS_AUTH_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with RideWithGPS first");
        return false;
    }
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
RideWithGPS::writeFile(QByteArray &, QString remotename, RideFile *ride)
{
    Q_UNUSED(ride);

    printd("RideWithGPS::writeFile(%s)\n", remotename.toStdString().c_str());

    // RIDE WITH GPS USES ITS OWN FORMAT, WE CONSTRUCT FROM THE RIDE
    int prevSecs = 0;
    long diffSecs = 0;


    int size = 0;
    int totalSize = ride->dataPoints().size();
    QDateTime rideDateTime = ride->startTime();
    QString out, data;

    QString username = getSetting(GC_RWGPSUSER).toString();
    QString password = getSetting(GC_RWGPSPASS).toString();
    QString auth_token = getSetting(GC_RWGPS_AUTH_TOKEN).toString();

    // application/json
    out += "{\"apikey\": \""+QString(GC_RWGPS_API_KEY)+"\", ";
    //out += "\"email\": \""+username+"\", ";
    //out += "\"password\": \""+password+"\", ";
    out += "\"auth_token\": \""+auth_token+"\", ";
    out += "\"version\": \"2\", ";

    out += "\"trip\": {\"track_points\": \"";

    data += "\[";
    foreach (const RideFilePoint *point, ride->dataPoints()) {
        size++;

        if (point->secs > 0.0) {
            diffSecs = point->secs - prevSecs;
            prevSecs = point->secs;
            rideDateTime = rideDateTime.addSecs(diffSecs);
        }
        /*
         x: -122.0, //longitude
          y: 45.0, //latitude
          e: 100.0, //elevation in meters
          t: 1368018754, //time in seconds since epoch. **Note** NOT in milliseconds!
          c: 90, //cadence
          h: 150, //heartrate
          p: 250, //power in watts
          s: 4.5, //speed, meters/second.  Don't provide unless from wheel sensor
          d: 45, //distance from start in meters. Only provide if from wheel sensor
          T: 20 //temperature in celcius
        */

        data += "{\"x\":";
        data += QString("%1").arg(point->lon,0,'f',GPS_COORD_TO_STRING);
        data += ",\"y\":";
        data += QString("%1").arg(point->lat,0,'f',GPS_COORD_TO_STRING);
        data += ",\"t\":";
        data += QString("%1").arg(rideDateTime.toSecsSinceEpoch());
        data += ",\"e\":";
        data += QString("%1").arg(point->alt);
        data += ",\"p\":";
        data += QString("%1").arg(point->watts);
        data += ",\"c\":";
        data += QString("%1").arg(point->cad);
        data += ",\"h\":";
        data += QString("%1").arg(point->hr);

        // We should verify if there is a wheel sensor
        //data += ",\"s\":";
        //data += QString("%1").arg(point->kph);
        //data += ",\"d\":";
        //data += QString("%1").arg(point->km);

        if ( ride->areDataPresent()->temp && point->temp != RideFile::NA) {
            data += ",\"T\":";
            data += QString("%1").arg(point->temp);
        }

        data += "}";

        if(size < totalSize)
           data += ",";
    }
    data += "]";
    out += data.replace("\"","\\\"");
    out += "\"}";

    printd("out:%s\n", out.toLatin1().toStdString().c_str());

    QUrl url = QUrl("https://ridewithgps.com/trips.json");
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done
    reply = nam->post(request, out.toLatin1());

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

    bool uploadSuccessful = false;
    QString uploadError;
    //XXXint tripid = 0;

    try {

        // parse the response
        QString response = reply->readAll();
        MVJSONReader jsonResponse(string(response.toLatin1()));

        // get values
        if (jsonResponse.root != NULL)
            uploadError = jsonResponse.root->getFieldString("error").c_str();
        //XXXif (jsonResponse.root->hasField("trip")) {
        //XXX    tripid = jsonResponse.root->getField("trip")->getFieldInt("id");
        //XXX}

    } catch(...) {

        // problem!
        uploadError = "bad response or parser exception.";
    }

    // no error so clear
    if (uploadError.toLower() == "none" || uploadError.toLower() == "null") uploadError = "";

    // set tags if uploaded (XXX not supported after refactor)
    if (uploadError.length()>0 || reply->error() != QNetworkReply::NoError) uploadSuccessful = false;
    else {
        //XXX ride->ride()->setTag("RideWithGPS tripid", QString("%1").arg(tripid));
        //XXX ride->setDirty(true);

        uploadSuccessful = true;
    }

    if (uploadSuccessful && reply->error() == QNetworkReply::NoError) {
        notifyWriteComplete( replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Completed."));
    } else {
        notifyWriteComplete( replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Network Error - Upload failed."));
    }
}

static bool addRideWithGPS() {
    CloudServiceFactory::instance().addService(new RideWithGPS(NULL));
    return true;
}

static bool add = addRideWithGPS();
