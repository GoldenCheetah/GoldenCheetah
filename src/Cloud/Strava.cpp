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
#include "JsonRideFile.h"
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
    downloadCompression = none;
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
        //return returning;
    }
    // did we get a good response ?
    QByteArray r = reply->readAll();
    r.clear();
    r.append(mockActivities());
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

    QByteArray* data = prepareResponse(buffers.value(reply));

    notifyReadComplete(data, replyName(reply), tr("Completed."));
}

void
Strava::addSamples(RideFile* ride, QString remoteid)
{
    printd("Strava::addSamples(%s)\n", remoteid.toStdString().c_str());

    // do we have a token ?
    QString token = getSetting(GC_STRAVA_TOKEN, "").toString();
    if (token == "") return;

    // lets connect and get basic info on the root directory
    QString url = QString("https://www.strava.com/api/v3/activities/%1/streams")
          .arg(remoteid);

    printd("url:%s\n", url.toStdString().c_str());

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    // put the file
    QNetworkReply *reply = nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "error" << reply->errorString();
        //errors << tr("Network Problem reading Strava data");
        //return returning;
    }
    // did we get a good response ?
    QByteArray r = reply->readAll();
    r.clear();
    r.append(mockStream());


    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {
        QJsonArray streams = document.array();

        QJsonArray timeData;
        QJsonArray distanceData;
        QJsonArray altitudeData;
        QJsonArray latlngData;

        for(int i=0; i<streams.size(); i++) {
            QJsonObject stream = streams.at(i).toObject();

            QString type = stream["type"].toString();

            if (type == "time") {
                timeData = stream["data"].toArray();
            } else if (type == "distance") {
                distanceData = stream["data"].toArray();
            } else if (type == "altitude") {
                altitudeData = stream["data"].toArray();
            }else if (type == "latlng") {
                latlngData = stream["data"].toArray();
            }
        }

        for(int i=0; i<timeData.size(); i++) {
            double secs = timeData.at(i).toInt();
            double km = distanceData.at(i).toDouble();
            double alt = altitudeData.at(i).toDouble();
            double lat = latlngData.at(i).toArray().at(0).toDouble();
            double lon = latlngData.at(i).toArray().at(1).toDouble();

            RideFilePoint point;
            point.setValue(RideFile::secs, secs);
            point.setValue(RideFile::km, km);
            point.setValue(RideFile::alt, alt);
            point.setValue(RideFile::lat, lat);
            point.setValue(RideFile::lon, lon);

            ride->appendPoint(point);
         }
    }
}

QByteArray*
Strava::prepareResponse(QByteArray* data)
{
    printd("Strava::prepareResponse()\n");

    data->clear();
    data->append(mockActivity());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(data->constData(), &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {
        QJsonObject each = document.object();

        RideFile *ride = new RideFile();
        ride->setStartTime(QDateTime::fromString(each["start_date"].toString(), Qt::ISODate));
        if (each["device_name"].toString().length()>0)
            ride->setDeviceType(each["device_name"].toString());
        else
            ride->setDeviceType("Strava"); // The device type is unknown

        addSamples(ride, QString("%1").arg(each["id"].toInt()));

        JsonFileReader reader;
        data->clear();
        data->append(reader.toByteArray(context, ride, true, true, true, true));

        printd("reply:%s\n", data->toStdString().c_str());
    }
    return data;
}

QByteArray
Strava::mockActivities()
{
    printd("Strava::mockActivities()\n");

    return QByteArray("[{\"id\":937800748,\"resource_state\":2,\"external_id\":\"1671361237.fit\",\"upload_id\":1038059970,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Sortie à vélo l'après-midi\",\"distance\":18377.9,\"moving_time\":2640,\"elapsed_time\":2640,\"total_elevation_gain\":358.0,\"type\":\"Ride\",\"start_date\":\"2017-04-11T15:58:07Z\",\"start_date_local\":\"2017-04-11T17:58:07Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":7200.0,\"start_latlng\":[46.165599,6.112932],\"end_latlng\":[46.164842,6.111346],\"location_city\":\"Plan Les Ouates\",\"location_state\":\"Canton of Geneva\",\"location_country\":\"Switzerland\",\"start_latitude\":46.165599,\"start_longitude\":6.112932,\"achievement_count\":2,\"kudos_count\":0,\"comment_count\":0,\"athlete_count\":1,\"photo_count\":0,\"map\":{\"id\":\"a937800748\",\"summary_polyline\":\"}uwxGy|hd@bNrZd@}P`GmPw@_VvEhF`BeBfGjAxPkF|IlAvKaDvDyJhDwUlLu\\dI{\\jGwMzSqXtCqGfCuSfB_CzJy@pBwDpLuAnF}E~D]dGmKtBuSaBkEg@mKqCkFmFgBiK}Q}Lq@mJ}EgAjBlBhMeFRaAz[_B`GwCGmCrRgG~HkQhG}@w@F_E{HaBsGkI_FwPwBgW_I{VzBn@rC`JxBFbAwU~I@vRxHrQtPtBlEYnI~BxNFvI~DzItB`RbEjDJ~DkC~CuBdT_Un[uJxRqH|[sLd]oFnY}EdLuBlR_C|DqFjYsH|TmA|CaMcJmJt@yKwMiGaO\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b1433648\",\"average_speed\":6.961,\"max_speed\":15.6,\"average_cadence\":66.8,\"average_temp\":15.0,\"average_watts\":170.3,\"weighted_average_watts\":245,\"kilojoules\":449.6,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":129.9,\"max_heartrate\":169.0,\"max_watts\":1254,\"elev_high\":668.6,\"elev_low\":416.2,\"pr_count\":0,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null},{\"id\":934891389,\"resource_state\":2,\"external_id\":\"Ride.tcx\",\"upload_id\":1035056136,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Sortie à vélo l'après-midi\",\"distance\":53040.2,\"moving_time\":5447,\"elapsed_time\":5447,\"total_elevation_gain\":558.3,\"type\":\"Ride\",\"start_date\":\"2017-04-09T13:04:33Z\",\"start_date_local\":\"2017-04-09T15:04:33Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":7200.0,\"start_latlng\":[46.16099,6.088945],\"end_latlng\":[46.165945,6.111548],\"location_city\":null,\"location_state\":null,\"location_country\":\"Suisse\",\"start_latitude\":46.16099,\"start_longitude\":6.088945,\"achievement_count\":8,\"kudos_count\":20,\"comment_count\":0,\"athlete_count\":1,\"photo_count\":0,\"map\":{\"id\":\"a934891389\",\"summary_polyline\":\"eyvxG{fdd@tb@zr@zDyA~BvHfAdUhb@tXXnFlJrLnNtJdMnIjBgBzFyV_F{x@rAcAlE~EnEHpAnJe@nY|Czu@tMrZlLhCqAlUlMtf@zJvRjGtFfc@nU`N}AjB~CjChUbQfkB|b@rfH`CxLdJdVvUdjAZvPvCvV_Ajb@|BdZuA~\\oAnFkP|OkLnRePxNkX`TuQhBaPyBmIdAy@hLpC`MH~\\xEbX_CvKoGa@eHcGuHiOaKaJmCcJiBVdAnP~Ybf@Y~AgJaDiGd@aLiI{RaFkV|JsPwDumDuImb@oPqUqTcWo_@scDklGlFeLxF}\\BuF|DsF~DgNPeEyAaF|@eF|MgJbDeMpA_NQiKbI_[rEac@vDkEnCCdMcIfCqFu@yJt@eDlT{N~GcP|KoDFcCeDsFlh@zMrAh^lNvC}@`M~A|@dHqn@wHa[nFiGi@aJ`FoCpDwL_@gFsFgLjDcEtOyF`NaKxNiP|JkR`JyDbMgNhNyIsOgZm[unA|HcGxXy_@}Sex@kJan@lCmQ~[wl@g_@yq@u]qu@eOgb@h]}X\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b3197146\",\"average_speed\":9.738,\"max_speed\":17.5,\"average_cadence\":87.0,\"average_watts\":221.3,\"weighted_average_watts\":238,\"kilojoules\":1205.3,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":147.5,\"max_heartrate\":161.0,\"max_watts\":579,\"elev_high\":530.1,\"elev_low\":343.5,\"pr_count\":2,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null},{\"id\":932751462,\"resource_state\":2,\"external_id\":\"1664572897.fit\",\"upload_id\":1032839893,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Saxel\",\"distance\":76228.3,\"moving_time\":9482,\"elapsed_time\":9572,\"total_elevation_gain\":831.0,\"type\":\"Ride\",\"start_date\":\"2017-04-08T07:43:15Z\",\"start_date_local\":\"2017-04-08T09:43:15Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":7200.0,\"start_latlng\":[46.166112,6.113534],\"end_latlng\":[46.166178,6.11356],\"location_city\":\"Plan Les Ouates\",\"location_state\":\"Canton of Geneva\",\"location_country\":\"Switzerland\",\"start_latitude\":46.166112,\"start_longitude\":6.113534,\"achievement_count\":3,\"kudos_count\":14,\"comment_count\":0,\"athlete_count\":2,\"photo_count\":0,\"map\":{\"id\":\"a932751462\",\"summary_polyline\":\"eywxGq`id@}q@ifBjB}H_L{HbG}H}BaUnIwO{Wo^xMoc@qRyIs_@nBxHk^_T|Fsd@gd@lKu[o[i_@hA_a@e`@kP`AsYsSw^uIrKcTemAoT{OcVgkA`b@wKn[__@yYem@}R}MxPgr@el@ak@pCkPaP_Pos@lW{Km\\pBkNoOeKvCgYqCq_@|God@yp@}p@{H|OoH}`@d@ca@aQ{p@zEe_Aqe@cm@{LgdA{IaGHeTsd@{Y|@mO{V}uAzNuLpD}NeCkq@vEeNtFjXnKyo@lNwToDjZ~BxShL}^~Fu@jDgb@i^_jA`@}a@aKgz@hf@hl@~b@fDpVjOhpBkFnSmN~S`AxUa^lM^fOp]pe@lg@f_@jQzRfl@tWnYfO`@`Vbb@lN|GfQuJjr@|AlGvGxFde@gHzsAeYjfAcF|^rAjRwKdn@{Nbb@mo@h}@lRj_@qB|Ted@xi@wxAvSyPnc@y\\dVyCpYsZ~e@fTpkAxT~a@aNvLpJpTo\\`dBvj@nY`t@~nB~d@t]mArRtVbFbQzl@hSdOxC~ZdS|Shk@joC~x@z`@qAhLlGnHZlT|NzXdIhBqArTqT|m@dFjKyHd\\sNpB}@rQ}Tq@jDvH}GzQ\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b2074216\",\"average_speed\":8.039,\"max_speed\":16.2,\"average_cadence\":76.7,\"average_temp\":12.0,\"average_watts\":178.5,\"weighted_average_watts\":222,\"kilojoules\":1692.8,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":132.9,\"max_heartrate\":167.0,\"max_watts\":1172,\"elev_high\":993.2,\"elev_low\":432.0,\"pr_count\":2,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null},{\"id\":931818210,\"resource_state\":2,\"external_id\":\"Ride.tcx\",\"upload_id\":1031882551,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Lunch Ride\",\"distance\":34115.6,\"moving_time\":4889,\"elapsed_time\":4916,\"total_elevation_gain\":1094.7,\"type\":\"Ride\",\"start_date\":\"2017-04-07T10:15:23Z\",\"start_date_local\":\"2017-04-07T12:15:23Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":7200.0,\"start_latlng\":[46.181761,6.157311],\"end_latlng\":[46.166383,6.114523],\"location_city\":null,\"location_state\":null,\"location_country\":\"Suisse\",\"start_latitude\":46.181761,\"start_longitude\":6.157311,\"achievement_count\":2,\"kudos_count\":14,\"comment_count\":0,\"athlete_count\":1,\"photo_count\":0,\"map\":{\"id\":\"a931818210\",\"summary_polyline\":\"_{zxGerqd@lDmTsEiN|C_Rh^vXfAiAvGai@xFkHdQo^|@cYtBoFyCqIrKcLdKcVuBwTfA}La@wFqPsaAmJiZk@eScVkm@}AqYaL}QoC}TLgFdBwC`VqRpQ_It[`@jQ|K|KtU^lVvBzIkOaHbArGfYpZvDp@oCtApErQiGd^fAdUqIn]c@vLtZ}e@qOp{@sHfKnLaEpQaVzD}KxANx@fBmBpOyFtL|CkB|NrA_N`_@nGaCeBdG|BtJjBgIpKiAjNpDNbAyJrLjHnDdBtExAkBhL}@`IvVtIhFbKcAbBzLpBpDaCxBNxDdKpAtGmJfHsCtDrEGxSjCnKtc@zDtGxOlOxJrNiGnDx@yEdNaJ~H{Pi@mFvEXpIrBkF`BO~IzH{G\\fAxEv@eAzFxBoFdC_@bGeS}BjGpGuGIhN`NkGfCt@rEuVeKDfEqJoAqAlB`@xLvB|Gm@zMmH`Pon@~UoCnUy^rj@ac@|_BkK~D}E_BsT`GcGwAyBhBmIcIqQd@bD|HgI|K\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b2074216\",\"average_speed\":6.978,\"max_speed\":17.6,\"average_cadence\":74.7,\"average_watts\":208.1,\"weighted_average_watts\":259,\"kilojoules\":1017.4,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":140.6,\"max_heartrate\":169.0,\"max_watts\":476,\"elev_high\":1292.9,\"elev_low\":383.0,\"pr_count\":0,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":10},{\"id\":927589880,\"resource_state\":2,\"external_id\":\"2017-04-04-12-18-37.fit\",\"upload_id\":1027552242,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Sortie à vélo le midi\",\"distance\":36799.6,\"moving_time\":5551,\"elapsed_time\":5702,\"total_elevation_gain\":883.0,\"type\":\"Ride\",\"start_date\":\"2017-04-04T10:18:37Z\",\"start_date_local\":\"2017-04-04T12:18:37Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":7200.0,\"start_latlng\":[46.186833,6.139962],\"end_latlng\":[46.186656,6.13875],\"location_city\":null,\"location_state\":null,\"location_country\":\"Suisse\",\"start_latitude\":46.186833,\"start_longitude\":6.139962,\"achievement_count\":0,\"kudos_count\":20,\"comment_count\":0,\"athlete_count\":1,\"photo_count\":0,\"map\":{\"id\":\"a927589880\",\"summary_polyline\":\"uz{xGwend@`EU]_TtAeKnCaFr@eVnSur@rAmNmEcMvCuQp^|X`Ay@dHcj@xXah@|@wXjBkF}CkIjKwKvJyVqAiTv@}Q_QobAwJ_[y@yUmVul@AcSmNwW_CgQKuHtAoDbTgQpRgJb^N~PxKxKtUb@rVnB|HsNqGl@dGrYn[jDf@iCxApEnQmG|]fAtUqIh]]|LdTiXfE}JsJ`m@_DzKwG~JpLiExOgSxEiMzAOv@bBeBlNeGbM|S@}Md^lGgBaBxFdBvJt@S^qGlKeB|OpDcJbNpHlErAhElBwBvLSnFhT`JlGjL{@~AvLzBpDeCvBJjE`KtAvGyJ|G}CdExEGjTbClKne@~DhFxOxOpJtMoG|D\\oEtN}JfJiPaA{EtDIfJxBuFzAG|IzHeFQI~E`A_AxF`CmFhC@fGwRkBhEbFiFs@TrAnD~F|GdEoFfDf@hEkUuJKvDyH_BgEbCgHgD}KoRcMaAqJwEe@bCdBrK}F|@a@dXmAbGu@|AuCSuDrTeM|KaJ|BeAgA@sDcMcEuWf@}KgDyOdBoFcCwBfKuH|@eD|F}Pxk@{PkCw_@bMiOl@kO}DcFnCc]bDsCmF{BP\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b383868\",\"average_speed\":6.629,\"max_speed\":13.7,\"average_cadence\":70.1,\"average_temp\":15.0,\"average_watts\":186.6,\"weighted_average_watts\":241,\"kilojoules\":1035.8,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":126.0,\"max_heartrate\":165.0,\"max_watts\":948,\"elev_high\":1343.8,\"elev_low\":427.6,\"pr_count\":0,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null},{\"id\":925059418,\"resource_state\":2,\"external_id\":\"Ride.tcx\",\"upload_id\":1024950821,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Afternoon Ride\",\"distance\":57953.4,\"moving_time\":7507,\"elapsed_time\":7682,\"total_elevation_gain\":807.2,\"type\":\"Ride\",\"start_date\":\"2017-04-02T12:14:19Z\",\"start_date_local\":\"2017-04-02T14:14:19Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":7200.0,\"start_latlng\":[46.165482,6.11252],\"end_latlng\":[46.165889,6.113172],\"location_city\":\"Plan Les Ouates\",\"location_state\":\"Canton of Geneva\",\"location_country\":\"Switzerland\",\"start_latitude\":46.165482,\"start_longitude\":6.11252,\"achievement_count\":4,\"kudos_count\":24,\"comment_count\":0,\"athlete_count\":2,\"photo_count\":0,\"map\":{\"id\":\"a925059418\",\"summary_polyline\":\"guwxGgzhd@bWld@rLJnl@jg@eZdk@`FjTmEdJ|Orr@dAjTvb@nYNfFbJnL~]lUhYr_@}Mlg@cJ`LsNdd@jFjNgCpN~Evi@{Hfe@iAvt@eEjZbCxUwDb[eFtYsRdWuJeCaBfRyEaIsLmDe@zXcIfUr@~p@dIvR|n@|{@h^eV`YfWiEyMhGwTpr@{^zr@~y@ph@rLhg@aEdP{dArAimAzQwJhHis@|J{L~Kyf@dNqQfN_m@q@aR`C}OdOyYXi\\`SoLfEcx@bMe]xc@kDdFqa@xMmHvBi\\mEaNtLx@tCaHcD}[cRyf@kDoTw@gm@sf@ur@ypAg\\yHqOs^w^}_AyUs]ah@eMEiGe[cMoIgMmqA{H}XwWai@{JkGiD_T}TsW{XaGlApOsFj@s@pZuHfOeFkf@aRwPi^gJuDgh@}FoJeGqDc@fE{JacAkLpEoG}XaSk_@sGlIt@zE}DkAmJtWeMnLqO\\kUzOlNx|@d]btAfx@~_@aAxLhGpHRjS~NjYfIlBmAlUkTfm@`FjKsHj\\}NzB{@`Q{Ti@dDpH}IhM~BbF\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b641129\",\"average_speed\":7.72,\"max_speed\":16.6,\"average_cadence\":75.4,\"average_watts\":174.5,\"weighted_average_watts\":220,\"kilojoules\":1310.3,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":131.2,\"max_heartrate\":163.0,\"max_watts\":1192,\"elev_high\":797.9,\"elev_low\":347.0,\"pr_count\":1,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null},{\"id\":922987961,\"resource_state\":2,\"external_id\":\"Ride.tcx\",\"upload_id\":1022810846,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Morning Ride\",\"distance\":25618.6,\"moving_time\":4459,\"elapsed_time\":4628,\"total_elevation_gain\":908.1,\"type\":\"Ride\",\"start_date\":\"2017-04-01T08:30:33Z\",\"start_date_local\":\"2017-04-01T10:30:33Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":7200.0,\"start_latlng\":[46.165779,6.113012],\"end_latlng\":[46.164874,6.111476],\"location_city\":\"Plan Les Ouates\",\"location_state\":\"Canton of Geneva\",\"location_country\":\"Switzerland\",\"start_latitude\":46.165779,\"start_longitude\":6.113012,\"achievement_count\":0,\"kudos_count\":14,\"comment_count\":0,\"athlete_count\":1,\"photo_count\":0,\"map\":{\"id\":\"a922987961\",\"summary_polyline\":\"awwxGi}hd@`O~Zt@cQxFcPu@aVtD~EhC_BvFvA~QqFtIfA|JqCrBcDfFk[~Ly]|I__@xIcP`QkTjCaHfBmQbCeEW_EmE}DmB}QyDuJ`@sIbDLpB{Gp@wZvFm@yAuNjJhEtM~@pKnQfI`ErD}BhIxAa@cEnVjKm@mEdGwCcHcEyDiGpG?cG}F~RbBh@qG`FwAkGmCm@pAq@mFnGHyIgIcBKeCjFVqJ`EkDlPt@tJyIdFyNcFvN_K~IwO{@cFrDIpJ`CwFdBCbJnIuGMj@rFn@uAvGdCwBfCiCCe@lFaS}ApGhGqG?lEvGdHfEqGlCj@hEsV}JN|DsJkAyAtCf@`LtBdGs@lN{HjPyEr@qFvEaM~A{AlDyJt@qCzCsB~SeUv\\jNvQiNf^mA`OoFnMsD`ULwBmC_BuQsF_ChOqFrMcB`QeC`EsFjYkJnYwM{IyCd@aA`CzA|StL|@zIfEfFkKiZ_UmK`@aKgMqGwO\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b1433648\",\"average_speed\":5.745,\"max_speed\":14.4,\"average_cadence\":66.9,\"average_watts\":175.0,\"weighted_average_watts\":240,\"kilojoules\":780.2,\"device_watts\":true,\"has_heartrate\":false,\"max_watts\":1227,\"elev_high\":1175.2,\"elev_low\":413.0,\"pr_count\":0,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":10},{\"id\":920954459,\"resource_state\":2,\"external_id\":\"1648649432.fit\",\"upload_id\":1020712986,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Course à pied le soir\",\"distance\":5635.3,\"moving_time\":1492,\"elapsed_time\":1492,\"total_elevation_gain\":57.6,\"type\":\"Run\",\"start_date\":\"2017-03-30T16:43:58Z\",\"start_date_local\":\"2017-03-30T18:43:58Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":7200.0,\"start_latlng\":[46.165798,6.113073],\"end_latlng\":[46.165811,6.113232],\"location_city\":\"Plan Les Ouates\",\"location_state\":\"Canton of Geneva\",\"location_country\":\"Switzerland\",\"start_latitude\":46.165798,\"start_longitude\":6.113073,\"achievement_count\":6,\"kudos_count\":15,\"comment_count\":1,\"athlete_count\":1,\"photo_count\":0,\"map\":{\"id\":\"a920954459\",\"summary_polyline\":\"ewwxGu}hd@uCaFtJwMeEoHxP_AsJuUrKaXaDkETcBoAcCjBiLnHuB_DuNg@cMv@mB~JzKjCrXRxCcEhYvMrBcFpSuI@wBfBi@hPsBRiB`Fb@|A}CvEeB_@yBaF{HfLrB|E\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"g1891728\",\"average_speed\":3.777,\"max_speed\":5.3,\"average_cadence\":177.5,\"has_heartrate\":true,\"average_heartrate\":153.4,\"max_heartrate\":169.0,\"max_watts\":403,\"elev_high\":458.0,\"elev_low\":411.1,\"pr_count\":1,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null},{\"id\":919305840,\"resource_state\":2,\"external_id\":\"1646477850.fit\",\"upload_id\":1019030194,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Sortie à vélo le midi\",\"distance\":37053.0,\"moving_time\":5449,\"elapsed_time\":5499,\"total_elevation_gain\":955.0,\"type\":\"Ride\",\"start_date\":\"2017-03-29T10:52:12Z\",\"start_date_local\":\"2017-03-29T12:52:12Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":7200.0,\"start_latlng\":[46.188392,6.139721],\"end_latlng\":[46.187223,6.138553],\"location_city\":null,\"location_state\":null,\"location_country\":\"Suisse\",\"start_latitude\":46.188392,\"start_longitude\":6.139721,\"achievement_count\":3,\"kudos_count\":17,\"comment_count\":0,\"athlete_count\":1,\"photo_count\":0,\"map\":{\"id\":\"a919305840\",\"summary_polyline\":\"md|xGgdnd@lO_Bm@wLvE{Xn@wTjJ{_@tGcPrBeO{EkOdD_Rx^hYz@sA~Gui@vXyg@|@yXlBoFwC_JhKcKvJaWaBcTn@oRqPibAsJoZy@qUmVam@LkP}@sG}LwQgCuRAqGtAmD|SuPtQaJz^?jQ~KpKbUd@hWtBjIyNeHj@xFpYl[lDl@gC~AnEvQoGh^lAvUoIn\\]|L~Yyd@wMby@oIjLhLaEzOyS~EyL`BSx@bBuB`QuFvJbDsA|NhAeNz^nGqBcBtF~BlKr@c@OoFhAaApKmAlNpDgJlN`HtDdBxEfBsBzLa@lFfTlJ~G`LcAbBfMxBbDiCvBNnEhKpA|FgJrHmDbE~EGfT`CfKde@`EpFtO`PxJdMqGbEVgEvN_K~IiPg@_FzDKpI|BuFnBQvIlIiG@VfF~@mAnGrByFjDk@pFcRuBfGjGqGC~DlGhHdEkGdDp@hEcVyJK`EkKaAcAzAP|InCvJ{@dOkBdFuErHmEd@aFrE{LzA_BpDaK`AmChCaCtTaUx[eFk@sK{Gs]_F{UkAqP`Fcv@aMod@xMcMd@eOaEud@bIsC{F}Ep@\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b2074216\",\"average_speed\":6.8,\"max_speed\":15.9,\"average_cadence\":72.2,\"average_temp\":16.0,\"average_watts\":195.6,\"weighted_average_watts\":253,\"kilojoules\":1065.8,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":139.7,\"max_heartrate\":174.0,\"max_watts\":1008,\"elev_high\":1305.0,\"elev_low\":395.4,\"pr_count\":1,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null},{\"id\":915724970,\"resource_state\":2,\"external_id\":\"2017-03-26-14-25-08.fit\",\"upload_id\":1015360707,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Sortie à vélo l'après-midi\",\"distance\":51992.8,\"moving_time\":7115,\"elapsed_time\":8063,\"total_elevation_gain\":575.0,\"type\":\"Ride\",\"start_date\":\"2017-03-26T12:25:08Z\",\"start_date_local\":\"2017-03-26T14:25:08Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":7200.0,\"start_latlng\":[46.165548,6.112695],\"end_latlng\":[46.164669,6.111026],\"location_city\":\"Plan Les Ouates\",\"location_state\":\"Canton of Geneva\",\"location_country\":\"Switzerland\",\"start_latitude\":46.165548,\"start_longitude\":6.112695,\"achievement_count\":2,\"kudos_count\":11,\"comment_count\":0,\"athlete_count\":1,\"photo_count\":0,\"map\":{\"id\":\"a915724970\",\"summary_polyline\":\"suwxGi{hd@dLxWpOlPhAvSdNvAv^~T`Zqe@Ect@zDcg@xNuu@hNa]d\\j`@A{H|IqYrXdI`a@Vf\\rUiM{rAwH{W_X}h@eKeHkEwUeFwA_LkReWwG\\rP}F\\kAf]mFxDmCrRgFnH_RjHkA_GyL}DgPd@ke@okAmNm`BkJaJcSlCcVmYil@bP{OwT}ByXkRoMeQsl@kUmFbCgJuAuFgNyEoUaWin@geBa\\h`@ia@rKhMz`@hJvn@rc@fVfg@bv@H~Jha@vd@jG}Hpq@nW_D`TiOf]oWzX_Wk`@pAo`@o_@_Px^~O`@rPla@mUtHaOzUlI`HcObH}ApQ~QzTkOtQeA|KyLjJgV|C|Ai@kGlGgIdSz_@~GdYtKkFvJtcAx@kElNrOc@jLbFn\\f^~HrUvVtBnd@pH|\\dF~G{G`\\aUl\\rNpQaJhRuRx|@}Epk@Djt@kT~[oKyO{f@qW~Di^eCie@xAwRqBqA}HnBm@hQcUq@jD|HaJvMxQl`@cFuL\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b2074216\",\"average_speed\":7.307,\"max_speed\":13.7,\"average_cadence\":74.0,\"average_temp\":12.0,\"average_watts\":153.1,\"weighted_average_watts\":182,\"kilojoules\":1089.4,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":124.4,\"max_heartrate\":156.0,\"max_watts\":1233,\"elev_high\":679.2,\"elev_low\":410.2,\"pr_count\":0,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null},{\"id\":913784776,\"resource_state\":2,\"external_id\":\"1639003998.fit\",\"upload_id\":1013347540,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Tour du Salève bis\",\"distance\":48654.7,\"moving_time\":5904,\"elapsed_time\":5989,\"total_elevation_gain\":680.0,\"type\":\"Ride\",\"start_date\":\"2017-03-25T12:37:35Z\",\"start_date_local\":\"2017-03-25T13:37:35Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":3600.0,\"start_latlng\":[46.20524,6.173678],\"end_latlng\":[46.164932,6.111555],\"location_city\":null,\"location_state\":null,\"location_country\":\"Suisse\",\"start_latitude\":46.20524,\"start_longitude\":6.173678,\"achievement_count\":2,\"kudos_count\":20,\"comment_count\":0,\"athlete_count\":2,\"photo_count\":0,\"map\":{\"id\":\"a913784776\",\"summary_polyline\":\"wm_yGmxtd@gB~JtLfSlDsEmFiZxB}QxAwBnS|RrG_hAxB_LfS_i@bLocAfCwe@tElAfNc]jBoVrFkJtFa^zG_WxR`FlMs[l_@cU|JmDv[f@jP`KpGbLbDrIdAp[|JbNbWpCtIbItEpLvQfMfH]bRjIlc@nI`g@x^hJ`PvNlFfEjEbOxCtN{Dr]rUf[~DrStIfFvFrHzCnCbTxJoBlRjTvEjJlAjLvB?bDcEhH`A`FzIv@pH`ErGfE`Q~@bH[vI~MlF`@vNlGx@`EdHdJ~ClCjIlBtPhEjFjBnb@|IdUx[bG~IbIzCpLd@tMeAdNtVsApHjGhMdPzBnL|GpCdGjO@~KvN~TfSV`D|HpOhCx[xWjTkD`KjL|Ql@nCvK|LjEdG`IhO`x@o@`FsMdPgEdRiMpV}LnKq^jo@oNlAaTkD_k@vJwD}EgDjCgUIiCfGpA|PeEnFyMoJeFwJySu@G}QcBkIeYic@{KqLoY}Ku^yCsVaJsHsO{^}^mMiB}[}MaSsBoGwDwNmZiFoG_Jz@aDgCwAuEOmJyBwGi_@wXuIaFgS{B_OvAwLmFqCb@uKiHsGbBmNcLmDiVs\\fi@wVn{@}EfYeFzLiBhQeCdE{F~Y_IfVy@~A{MoJuJ\\aKmMkGqO\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b2074216\",\"average_speed\":8.241,\"max_speed\":19.2,\"average_cadence\":78.4,\"average_temp\":14.0,\"average_watts\":195.1,\"weighted_average_watts\":230,\"kilojoules\":1151.9,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":138.9,\"max_heartrate\":167.0,\"max_watts\":1192,\"elev_high\":821.8,\"elev_low\":342.0,\"pr_count\":0,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":10},{\"id\":912581349,\"resource_state\":2,\"external_id\":\"Ride.tcx\",\"upload_id\":1012101019,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Tour du Saleve\",\"distance\":50666.9,\"moving_time\":5575,\"elapsed_time\":5611,\"total_elevation_gain\":777.9,\"type\":\"Ride\",\"start_date\":\"2017-03-24T11:25:15Z\",\"start_date_local\":\"2017-03-24T12:25:15Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":3600.0,\"start_latlng\":[46.189865,6.139612],\"end_latlng\":[46.165116,6.111912],\"location_city\":null,\"location_state\":null,\"location_country\":\"Suisse\",\"start_latitude\":46.189865,\"start_longitude\":6.139612,\"achievement_count\":10,\"kudos_count\":20,\"comment_count\":0,\"athlete_count\":1,\"photo_count\":0,\"map\":{\"id\":\"a912581349\",\"summary_polyline\":\"sm|xGqcnd@dX}Ac@sLrFwYt@}VvIc^jGsOvBiOqE}NxCgRv^jYt@_AlHij@pXqh@`AsXhB_FqCcJvJsJhKcWaB_Uh@oRcPs`AeKm\\w@iUcVsl@w@yXcMmRiCiWxBiHz]gWzJkDpYf@nQvJpCxDhHxPjAh\\~IdM~WhD~IjIbE|K|QjMdHShRhIvb@bIzg@p_@jJxOxNjF`EfEhNvCvOsD`]jU~[jEzRhIjFzFpHxCvCbTtJcBfR~SfFfKdAlKrBFtC}DpHn@dFxIx@pHhEhHr@fHtC`H`@dRdN|F^fNfG|@jEhHlI`CvC~IjBxPlEnFpBzb@tInTn[hGhJ~H|CzLd@lMmAvMbW_AvGvF|MzP|BhL~GtCbGpOFhLvNlT|RVxClHrOjCl[xWbUyCfKnLhQ^zCzKdMvElFzGxOfx@u@vFqMvO_ErQwLbVkMjLaa@|t@yJ~Fec@lFsUdPyC~EgNnl@wEhD_DkAyOuSab@wXeFgQOy\\sAkJ_Zee@iL}LuXqKc_@_DeR}Gae@Pi[yFmWkOkp@cCkn@ca@yZqf@{O_IoZg]yU_KwHqPwIuGeQ{UoJtOwVp{@kFf[}FdN{AbPeC`EyFzYgIbWu@bAeMeJwJf@wKaNoHoQ\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b641129\",\"average_speed\":9.088,\"max_speed\":16.9,\"average_cadence\":80.7,\"average_watts\":227.3,\"weighted_average_watts\":238,\"kilojoules\":1267.4,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":144.4,\"max_heartrate\":159.0,\"max_watts\":1125,\"elev_high\":905.0,\"elev_low\":378.0,\"pr_count\":2,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null},{\"id\":906504554,\"resource_state\":2,\"external_id\":\"Ride.tcx\",\"upload_id\":1005755370,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Afternoon Ride\",\"distance\":45783.4,\"moving_time\":5410,\"elapsed_time\":5512,\"total_elevation_gain\":220.8,\"type\":\"Ride\",\"start_date\":\"2017-03-19T14:34:42Z\",\"start_date_local\":\"2017-03-19T15:34:42Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":3600.0,\"start_latlng\":[46.19874,6.181121],\"end_latlng\":[46.166491,6.114102],\"location_city\":null,\"location_state\":null,\"location_country\":\"Suisse\",\"start_latitude\":46.19874,\"start_longitude\":6.181121,\"achievement_count\":8,\"kudos_count\":18,\"comment_count\":0,\"athlete_count\":2,\"photo_count\":0,\"map\":{\"id\":\"a906504554\",\"summary_polyline\":\"ce~xG_gvd@|@wWqC{BwNa[yOgEwJg`@qBmXiKuCiFqGqIsl@sMk`@bG}FzYqDfScYtGgEaKeOa@sDyLsWqRwMvBiK_C`K|BvD}ErRbb@`RjHnGvPbi@jTdb@pJl^|ThWcHrq@oQ`c@aDhNmBjXRfJy@LyDfe@uZ{^aG~CoBgEwDmByHGyNqN}R_s@aAoJ}HoRwJeHmEmLoIiEoB}J_EsBoJqSwQmU}JcHsTuWkOkD}PkLaHk\\eO_IkI_LcR}o@|AqZ_Ky^oB}n@mBcQxKeQnK}r@bFoNzVkYnNqVnRpItB_[r@mBhEbFzY|]iHxc@jCf_@qC`ZtOtJqBpNdLf\\bl@yYdJ~CfKpLcC`QfRbMhPvXdHpDgQ~o@`C~D}EpRfa@vQdI|G~Pli@dTta@dKj_@rUjWnObG`A|EuClKxM]hG|ErQvm@~RnNr@~RfBlGjEvAxFvLnD|AtUsObQe@zHxLzJjI|DbAxPwBvVxW~El_AlK~VtEo@xDfQxG|MkDvFwEbCkBlKeMr@yGfHeHkCgBpKqH~@kDlGcQzj@sFeA{Tv[bRjM`KjLrSd`@qD`EiFxQ|EjIzCjLoQXlDvH_JrN\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b2074216\",\"average_speed\":8.463,\"max_speed\":16.6,\"average_cadence\":76.1,\"average_watts\":157.9,\"weighted_average_watts\":200,\"kilojoules\":854.4,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":127.6,\"max_heartrate\":169.0,\"max_watts\":1203,\"elev_high\":485.0,\"elev_low\":392.0,\"pr_count\":1,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":10},{\"id\":904995496,\"resource_state\":2,\"external_id\":\"Ride.tcx\",\"upload_id\":1004166457,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Afternoon Ride\",\"distance\":51434.4,\"moving_time\":6719,\"elapsed_time\":6719,\"total_elevation_gain\":556.0,\"type\":\"Ride\",\"start_date\":\"2017-03-18T12:58:29Z\",\"start_date_local\":\"2017-03-18T13:58:29Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":3600.0,\"start_latlng\":[46.162832,6.108215],\"end_latlng\":[46.165864,6.113162],\"location_city\":\"Plan Les Ouates\",\"location_state\":\"Canton of Geneva\",\"location_country\":\"Switzerland\",\"start_latitude\":46.162832,\"start_longitude\":6.108215,\"achievement_count\":2,\"kudos_count\":19,\"comment_count\":0,\"athlete_count\":2,\"photo_count\":0,\"map\":{\"id\":\"a904995496\",\"summary_polyline\":\"udwxGi_hd@pH|JzIy@|YhSlR`TkZ~j@`FlT{ElJbP`r@nAzTpb@dYrI`Sf_@rVfYd_@eNxg@eJvKyNhd@xFdNmCjL`Flk@cIze@_Abt@eEfZnCpV}Klt@{RjXuJcCmBdR{DuHgMaEiHa[aM~Xa]a_@ki@bMuO}XkE]oI~o@HmOsNkCiA{^}h@kMhDnL~[xGgAf}@qFs[qDmDcVuEuTrKkg@ba@op@dfB{DwPoW{AoHwQ{BdGvApXdIhNtIy[kIu@oGqPKoTmMeb@lV}EbGwRu@kPxa@g\\lLcc@O}n@th@nE~DoQ_FoF~E{FsEiGvBuViF}UpBu`@|Sy]eCuHf]md@jYqn@zLuo@jRwe@}Eeo@rZeM~CaK~`@b^nUxf@wAe`@j^is@_}@slBrZcY~AvDzEaIhJ~BnEm`@nP{EnLg[eb@MDgQxN{b@yFsLpT{l@jAmT}HeBgMgU}AiXiG}GgIfAkVrr@eFs@gU~[pSrMf]xl@_KpWhJrVaQv@dDfH}IbM`CfF\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b537083\",\"average_speed\":7.655,\"max_speed\":15.1,\"average_cadence\":77.5,\"average_watts\":189.9,\"weighted_average_watts\":218,\"kilojoules\":1276.1,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":132.1,\"max_heartrate\":161.0,\"max_watts\":1066,\"elev_high\":474.9,\"elev_low\":350.0,\"pr_count\":1,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":10},{\"id\":902721176,\"resource_state\":2,\"external_id\":\"1623811549.fit\",\"upload_id\":1001759048,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Course à pied l'après-midi\",\"distance\":3980.3,\"moving_time\":1045,\"elapsed_time\":1045,\"total_elevation_gain\":57.0,\"type\":\"Run\",\"start_date\":\"2017-03-16T15:45:55Z\",\"start_date_local\":\"2017-03-16T16:45:55Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":3600.0,\"start_latlng\":[46.165958,6.113192],\"end_latlng\":[46.165913,6.113281],\"location_city\":\"Plan Les Ouates\",\"location_state\":\"Canton of Geneva\",\"location_country\":\"Switzerland\",\"start_latitude\":46.165958,\"start_longitude\":6.113192,\"achievement_count\":4,\"kudos_count\":10,\"comment_count\":1,\"athlete_count\":1,\"photo_count\":0,\"map\":{\"id\":\"a902721176\",\"summary_polyline\":\"exwxGm~hd@{CyEvJeNyE_HnRcAoJmVdGeS~Ae@rGtFlAcDvPtBsFhUeIo@kCzCc@rOgCwAkQt@vD~G}ItMhBdE\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"g1891728\",\"average_speed\":3.809,\"max_speed\":5.7,\"average_cadence\":176.4,\"has_heartrate\":true,\"average_heartrate\":148.6,\"max_heartrate\":170.0,\"max_watts\":339,\"elev_high\":470.4,\"elev_low\":424.8,\"pr_count\":0,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null},{\"id\":901281196,\"resource_state\":2,\"external_id\":\"2017-03-15-16-12-05.fit\",\"upload_id\":1000237354,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Sortie à vélo l'après-midi\",\"distance\":43808.4,\"moving_time\":5071,\"elapsed_time\":5071,\"total_elevation_gain\":407.0,\"type\":\"Ride\",\"start_date\":\"2017-03-15T15:12:05Z\",\"start_date_local\":\"2017-03-15T16:12:05Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":3600.0,\"start_latlng\":[46.160582,6.103312],\"end_latlng\":[46.165378,6.112387],\"location_city\":\"Plan Les Ouates\",\"location_state\":\"Canton of Geneva\",\"location_country\":\"Switzerland\",\"start_latitude\":46.160582,\"start_longitude\":6.103312,\"achievement_count\":3,\"kudos_count\":18,\"comment_count\":3,\"athlete_count\":1,\"photo_count\":0,\"map\":{\"id\":\"a901281196\",\"summary_polyline\":\"svvxGu`gd@`@lCzMrAn^fU}Tdc@xEjUam@nfAcD|QzEx`@|XdfA_Xt^sInHb\\~rA`N`VwRfNiH|IoIhDyJdRaOlPuMvJuO~F_E~EnFfL\\xD}D~MmEbC^`K_FlG`Hf\\yG|l@oAuAl@uM{McC{A}^oh@sM]r@jElFg@|C~\\tG{ArLJ`p@sG_^uDyB{Gv@yKcGuTvK_KvI_AeJh@iDvUuPnGmO`JoBxVzC|EdD_BdLBvo@qFu\\}DcDcHr@aMaGmShK}J~IeAmInAsF~TmOdGeObJmBdTfCbHlD_BbLFjp@sFw\\{DgDkHx@}LgG}SxKkJrIeA{HjAaG`UsOhGeO~IgB`ZzDjBlC}AvI^jJcAxLp@hX{Gy]_E}BqGv@wKcG}`@bVu@sJz@kDxTeOlGkOhLuDCgCsCmFbh@bNlAx^jNfCw@|M|Ar@bHuo@_I}YlFkGg@oJxEmC`E}Me@uEyFsKhDkErPcGbO}KrMeOrJcRpIkDlMqNpN_JoM}Tw]ssAzIoH~We_@wY{gA_E_c@xCaNtZgk@k[_l@kCmJi\\wp@aOoa@`a@_^\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b2074216\",\"average_speed\":8.639,\"max_speed\":14.6,\"average_cadence\":77.1,\"average_temp\":15.0,\"average_watts\":190.8,\"weighted_average_watts\":246,\"kilojoules\":967.5,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":134.2,\"max_heartrate\":169.0,\"max_watts\":1085,\"elev_high\":459.4,\"elev_low\":358.6,\"pr_count\":1,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null},{\"id\":897197283,\"resource_state\":2,\"external_id\":\"Ride.tcx\",\"upload_id\":995892227,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Giron de la Côte #3\",\"distance\":85512.3,\"moving_time\":7982,\"elapsed_time\":8281,\"total_elevation_gain\":677.2,\"type\":\"Ride\",\"start_date\":\"2017-03-12T07:53:08Z\",\"start_date_local\":\"2017-03-12T08:53:08Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":3600.0,\"start_latlng\":[46.37018,6.227575],\"end_latlng\":[46.375835,6.208781],\"location_city\":null,\"location_state\":null,\"location_country\":\"Suisse\",\"start_latitude\":46.37018,\"start_longitude\":6.227575,\"achievement_count\":8,\"kudos_count\":23,\"comment_count\":0,\"athlete_count\":39,\"photo_count\":0,\"map\":{\"id\":\"a897197283\",\"summary_polyline\":\"st_zGii_e@`PxYjc@j[nPrTvAvR}ArUiXls@vNhL_@jbAw^xw@eIkGuMo`@ySm^qLmIrA_PaKiNcGq_@}DaGdQoa@p[dHvi@qHf\\rMeRrf@`O`Lg@fcA_^zv@wIaHyLo^{Sm^wMkK|AoO_KwMeMqh@pQca@bZnHvi@yHn]`NiRzf@`OxK[h`Aea@t{@cj@oiAiMsJdBmOwJoMmGm`@iEiG`Qma@vZdHdj@{Hl]dNmRtf@rNnKMd`Aga@h|@cj@siAgMeJpAwPiJwKwMsj@dQc`@vZhHbj@yHf]fNoRpf@tNnKUfaAia@b{@yUsk@iSm]gMoJbBcOaKyM_M{i@hQ}_@zZhH|h@uH~]hNkRff@pNnLQv_Au`@z{@wj@yiAoMgJ`BwOoJgMyG}a@sDiFfP{`@j[fHfi@wHb^~MmR`f@vN`LOx`Aga@t{@yi@eiAoMqJtAiPyJqMaMui@fQ}_@pZlHvi@{Hl]`NqRdf@|NdLSf`Aaa@l|@qi@}hAoMcKhA_P}JeN_Mci@fPg`@f\\bH~h@sHz]`N_Spf@bOzKWh`Aka@f|@yi@ajAeMkJpAkPgKeNyLog@hQga@zZfHri@wHr]`NwRlf@xNfLSb`Ay`@f|@{IgMwKw]aS_]cMuJ|A_OmK}NwKml@xPq\\lZlH|i@_If]jNuR~e@xNlL]daA}_@~z@_k@wiAkMsJjA{PiJmLeMyh@zQga@\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b641129\",\"average_speed\":10.713,\"max_speed\":22.0,\"average_cadence\":84.7,\"average_watts\":182.7,\"weighted_average_watts\":221,\"kilojoules\":1458.0,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":145.6,\"max_heartrate\":169.0,\"max_watts\":861,\"elev_high\":479.0,\"elev_low\":377.7,\"pr_count\":2,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null},{\"id\":895864889,\"resource_state\":2,\"external_id\":\"1614603716.fit\",\"upload_id\":994468632,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Sortie à vélo le midi\",\"distance\":59339.5,\"moving_time\":7222,\"elapsed_time\":7359,\"total_elevation_gain\":402.0,\"type\":\"Ride\",\"start_date\":\"2017-03-11T11:49:04Z\",\"start_date_local\":\"2017-03-11T12:49:04Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":3600.0,\"start_latlng\":[46.165798,6.112936],\"end_latlng\":[46.167099,6.115358],\"location_city\":\"Plan Les Ouates\",\"location_state\":\"Canton of Geneva\",\"location_country\":\"Switzerland\",\"start_latitude\":46.165798,\"start_longitude\":6.112936,\"achievement_count\":2,\"kudos_count\":20,\"comment_count\":0,\"athlete_count\":2,\"photo_count\":0,\"map\":{\"id\":\"a895864889\",\"summary_polyline\":\"ewwxGy|hd@jYlg@|JWt`@`XdLuP|TzCDwt@vDig@~N}u@hNo^sM}PmRiIqs@iImOqYWcSqGmIzA{Kw{@y`@wc@wb@hIyn@oNk~@{SaSiC}Y}R_OyQim@kUgFr@qQiNsF}TaVyq@mkBkWkh@}QeLdPgs@qk@cj@fCuPiP_P_Im@kh@xYuLk]lBmNcO{IpC}ZqC{^~Gud@up@{p@gIpOgHi`@f@aa@wG{`@oH}Ol@uj@lD}RuMeKkV}_@sI}_@uC}e@eIqD}PfQuL|@yq@gw@c\\d~AdIbTmR~JpBvO|O~W~Bre@WtQ{R`t@yAlZxPjjAe@fXzB`M{Lvy@bk@hj@``@zR~j@ni@rKuWfKzb@|LwRlGhDdTbWgJp\\|_@fYpLkMr[eCpc@|d@`FlXpHlG|ItW~EvYlVy[rMd`@vJjo@p`@hRfj@py@oGbw@v`@zP`@tPt`@gUzHqObVnIxGyN`HeBzQrRpMj{@}I|o@|VpU_Wl`@~EnHaNza@zWj^oJzNK`LhDxH_G`I`LpHsBzHvHvI|a@bpA\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b2074216\",\"average_speed\":8.216,\"max_speed\":17.7,\"average_cadence\":75.2,\"average_temp\":12.0,\"average_watts\":166.7,\"weighted_average_watts\":186,\"kilojoules\":1203.6,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":126.6,\"max_heartrate\":151.0,\"max_watts\":1202,\"elev_high\":588.6,\"elev_low\":405.6,\"pr_count\":0,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null},{\"id\":894504076,\"resource_state\":2,\"external_id\":\"2017-03-10-13-15-06.fit\",\"upload_id\":993008123,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Sortie à vélo l'après-midi\",\"distance\":22894.9,\"moving_time\":2495,\"elapsed_time\":2495,\"total_elevation_gain\":176.0,\"type\":\"Ride\",\"start_date\":\"2017-03-10T12:15:06Z\",\"start_date_local\":\"2017-03-10T13:15:06Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":3600.0,\"start_latlng\":[46.166103,6.113489],\"end_latlng\":[46.165115,6.111927],\"location_city\":\"Plan Les Ouates\",\"location_state\":\"Canton of Geneva\",\"location_country\":\"Switzerland\",\"start_latitude\":46.166103,\"start_longitude\":6.113489,\"achievement_count\":1,\"kudos_count\":14,\"comment_count\":1,\"athlete_count\":1,\"photo_count\":0,\"map\":{\"id\":\"a894504076\",\"summary_polyline\":\"cywxGg`id@aDgEu]z_@zVfq@n{@deBu^|t@i@nNzEbZ~Kve@jWvx@|\\vWdYpJjR|ZwNl]sF|H_i@l`@kYkd@y]ctAdIiGlXo_@mTix@cJ{k@`@cJrB}Gp\\cm@oYcg@ud@sbApZaZ~BzD|EuIpGpGhArS|MlA~^lU`X_d@mUeHzCy[sJuV_CjDoFtYsJ`ZwMcJ}E`Dj@rRxGzAqGsAeAkScPoQmHqQ\",\"resource_state\":2},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b2074216\",\"average_speed\":9.176,\"max_speed\":15.7,\"average_cadence\":80.3,\"average_temp\":14.0,\"average_watts\":222.1,\"weighted_average_watts\":261,\"kilojoules\":554.3,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":142.8,\"max_heartrate\":166.0,\"max_watts\":1244,\"elev_high\":506.4,\"elev_low\":430.6,\"pr_count\":0,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null}]");
}

QByteArray
Strava::mockActivity()
{
    printd("Strava::mockActivity()\n");

    return QByteArray("{\"id\":937800748,\"resource_state\":3,\"external_id\":\"1671361237.fit\",\"upload_id\":1038059970,\"athlete\":{\"id\":112765,\"resource_state\":1},\"name\":\"Sortie à vélo l'après-midi\",\"distance\":18377.9,\"moving_time\":2640,\"elapsed_time\":2640,\"total_elevation_gain\":358.0,\"type\":\"Ride\",\"start_date\":\"2017-04-11T15:58:07Z\",\"start_date_local\":\"2017-04-11T17:58:07Z\",\"timezone\":\"(GMT+01:00) Europe/Zurich\",\"utc_offset\":7200.0,\"start_latlng\":[46.165599,6.112932],\"end_latlng\":[46.164842,6.111346],\"location_city\":\"Plan Les Ouates\",\"location_state\":\"Canton of Geneva\",\"location_country\":\"Switzerland\",\"start_latitude\":46.165599,\"start_longitude\":6.112932,\"achievement_count\":2,\"kudos_count\":0,\"comment_count\":0,\"athlete_count\":1,\"photo_count\":0,\"map\":{\"id\":\"a937800748\",\"polyline\":\"}uwxGy|hd@jA|BvAtCZfAbA~BBRb@nAbA~Bh@|AJd@X`@TNLAFOFa@Be@Je@GmDBsA?{AHaBRaA^{@^i@~@gA^w@Pi@b@mBJi@JcAB{AEaAo@aDK{@ImACyAGkA?OJaADQHKJCJALDLHRZ|@jCHJX^RFXBZK\\YXs@LKZ?NF^Dd@Jn@XhAVZ?LAjIwBd@Q|Ay@v@Yb@KvBQ\\@RF~BjA\\FT@h@Qp@[pAw@~@]lAUV?^DP?PMjAkBZq@ZeAr@uCNw@n@qFJ]NQBK@]Eq@Dc@d@}C`@_BfBqFpAoDb@aAr@}Bb@eAt@aC`AkCRiAj@gCZeANu@x@yCVyAlB{HX}@`@iA`@i@FQh@{Ar@_Bp@cARk@Ze@pAcBz@oA~AeBfAwA`@q@Vq@p@kAvDwFDEPGX?NIHWHy@JY~AaCTc@Bg@IeBBs@PcAr@qCb@aE^{@~@aANSVi@V@|@XN?NCTMd@a@NGzBEf@ITIRQFKd@qBVWVOj@KlBUXGbAc@f@ApAFb@CZIXM`@WtC{C\\[VIb@En@?NEn@?RGNOJQfA_DZ]bA{@JQ|AoDNc@He@DWF{ACeA@aEJw@X_A@GCOEMYYa@u@[}A]yCAcAJiAG_AESI[Ue@_B{CW_@SMIAY?g@H]E]O]Ss@w@i@cAwAoD_@a@_Ai@USQ]wA{DYa@_@Ya@Ie@CeHO}@GUGe@SmEcDe@WSGUASBSJSNOTOx@@^D\\`BxGHx@?ZEZKVKNQFQASGq@o@QKQES@OFONKTIXIx@OhLAxDEzACv@QnAaA|D]bAMHM@OCo@e@QEQ@MFIPMn@U`EIt@Or@q@bCEXGjAOl@c@|@qAjA]^eAlB]Za@VwAh@aBrAc@Ry@XcE`Ac@Hg@FQASEQIMQIUCW?q@TcBCIEGSIe@IsBUm@Ko@Oo@[k@c@[YoB}Bk@{@o@sAkDuK_@yASgAWeCaAaO]_C_@qAaE{J}@qCa@iAGa@Ba@DMr@[n@l@V\\FN~@tC`AhDHPHDP@XIN?b@JNAJIHMF[?c@MoCBgBFu@\\gBHw@@u@G}A?IDSHELD|@z@PFXGr@a@x@Wr@Gz@FXHn@f@n@v@h@ZPF~B`@jCjAf@LnB\\\\Pp@l@jFtDb@^NR^n@lAjCV\\ZP~@VTLHJ`BtCN`@BT@p@UhBGx@Az@B|@T|DDtAFr@Lj@Tb@d@t@JRDVBr@CnCBdBBl@Nj@f@tAf@|@x@lAZn@H\\r@hDp@nEB|@AtBLr@Tp@Zh@RFb@?NBr@h@TXL`@Hz@Cz@Gd@Qd@W^k@j@k@ZIPGxBQjBOl@m@lBMn@Cv@FrAAZAZOn@sAzBa@t@[Z{@nAi@`AgBrCaBvB_AfB}DzDET@FLN?HMBMLyAfBsAhBaAnBk@fB]f@i@|AeBhHYpASnAo@zBUdA[hAg@vBSlAiAtCq@zBe@hA{@pC_@x@Sh@aDrJe@pB_@bCEd@Fx@AVEHMJY@GDCJYlCYpBQ|@i@xBe@vAO^cA~AUHIHEj@ENYn@]lA[vAYzAe@fDUtC?dBCTIPg@d@e@p@_@|@Ql@On@uDrSa@nB[dA_BrEwFzPGDMCwAiAqEcDmB}Aa@SQEc@?mCj@_@Lo@XQDQ?QGu@c@_@]k@y@Yi@[aAg@i@_@o@cAeAwA_Aa@e@o@oAe@mA_A{Bc@mAoAyC\",\"resource_state\":3,\"summary_polyline\":\"}uwxGy|hd@bNrZd@}P`GmPw@_VvEhF`BeBfGjAxPkF|IlAvKaDvDyJhDwUlLu\\dI{\\jGwMzSqXtCqGfCuSfB_CzJy@pBwDpLuAnF}E~D]dGmKtBuSaBkEg@mKqCkFmFgBiK}Q}Lq@mJ}EgAjBlBhMeFRaAz[_B`GwCGmCrRgG~HkQhG}@w@F_E{HaBsGkI_FwPwBgW_I{VzBn@rC`JxBFbAwU~I@vRxHrQtPtBlEYnI~BxNFvI~DzItB`RbEjDJ~DkC~CuBdT_Un[uJxRqH|[sLd]oFnY}EdLuBlR_C|DqFjYsH|TmA|CaMcJmJt@yKwMiGaO\"},\"trainer\":false,\"commute\":false,\"manual\":false,\"private\":false,\"flagged\":false,\"gear_id\":\"b1433648\",\"average_speed\":6.961,\"max_speed\":15.6,\"average_cadence\":66.8,\"average_temp\":15.0,\"average_watts\":170.3,\"weighted_average_watts\":245,\"kilojoules\":449.6,\"device_watts\":true,\"has_heartrate\":true,\"average_heartrate\":129.9,\"max_heartrate\":169.0,\"max_watts\":1254,\"elev_high\":668.6,\"elev_low\":416.2,\"pr_count\":0,\"total_photo_count\":0,\"has_kudoed\":false,\"workout_type\":null,\"description\":\"\",\"calories\":501.3,\"segment_efforts\":[{\"id\":22914381904,\"resource_state\":2,\"name\":\"Ch. des Vaulx/Ch. de la Vironde\",\"activity\":{\"id\":937800748,\"resource_state\":1},\"athlete\":{\"id\":112765,\"resource_state\":1},\"elapsed_time\":161,\"moving_time\":161,\"start_date\":\"2017-04-11T15:59:54Z\",\"start_date_local\":\"2017-04-11T17:59:54Z\",\"distance\":992.0,\"start_index\":107,\"end_index\":268,\"average_cadence\":62.2,\"device_watts\":true,\"average_watts\":168.5,\"average_heartrate\":123.5,\"max_heartrate\":138.0,\"segment\":{\"id\":7945706,\"resource_state\":2,\"name\":\"Ch. des Vaulx/Ch. de la Vironde\",\"activity_type\":\"Ride\",\"distance\":980.2,\"average_grade\":1.5,\"maximum_grade\":15.4,\"elevation_high\":422.6,\"elevation_low\":407.6,\"start_latlng\":[46.163073,6.109047],\"end_latlng\":[46.160266,6.117205],\"start_latitude\":46.163073,\"start_longitude\":6.109047,\"end_latitude\":46.160266,\"end_longitude\":6.117205,\"climb_category\":0,\"city\":\"Plan Les Ouates\",\"state\":\"GE\",\"country\":\"Switzerland\",\"private\":false,\"hazardous\":false,\"starred\":false},\"kom_rank\":null,\"pr_rank\":null,\"achievements\":[],\"hidden\":false},{\"id\":22914381923,\"resource_state\":2,\"name\":\"GP Lancy Petite Montee\",\"activity\":{\"id\":937800748,\"resource_state\":1},\"athlete\":{\"id\":112765,\"resource_state\":1},\"elapsed_time\":69,\"moving_time\":69,\"start_date\":\"2017-04-11T16:01:46Z\",\"start_date_local\":\"2017-04-11T18:01:46Z\",\"distance\":325.0,\"start_index\":219,\"end_index\":288,\"average_cadence\":59.4,\"device_watts\":true,\"average_watts\":259.8,\"average_heartrate\":130.1,\"max_heartrate\":142.0,\"segment\":{\"id\":10860647,\"resource_state\":2,\"name\":\"GP Lancy Petite Montee\",\"activity_type\":\"Ride\",\"distance\":321.2,\"average_grade\":5.2,\"maximum_grade\":9.4,\"elevation_high\":447.4,\"elevation_low\":428.9,\"start_latlng\":[46.162099,6.117888],\"end_latlng\":[46.160091,6.116432],\"start_latitude\":46.162099,\"start_longitude\":6.117888,\"end_latitude\":46.160091,\"end_longitude\":6.116432,\"climb_category\":0,\"city\":\"Plan-les-Ouates\",\"state\":\"Genève\",\"country\":\"Switzerland\",\"private\":false,\"hazardous\":false,\"starred\":false},\"kom_rank\":null,\"pr_rank\":null,\"achievements\":[],\"hidden\":false},{\"id\":22914381940,\"resource_state\":2,\"name\":\"Ch. de la Vironde\",\"activity\":{\"id\":937800748,\"resource_state\":1},\"athlete\":{\"id\":112765,\"resource_state\":1},\"elapsed_time\":122,\"moving_time\":122,\"start_date\":\"2017-04-11T16:02:45Z\",\"start_date_local\":\"2017-04-11T18:02:45Z\",\"distance\":645.1,\"start_index\":278,\"end_index\":400,\"average_cadence\":75.0,\"device_watts\":true,\"average_watts\":228.6,\"average_heartrate\":141.9,\"max_heartrate\":146.0,\"segment\":{\"id\":12817366,\"resource_state\":2,\"name\":\"Ch. de la Vironde\",\"activity_type\":\"Ride\",\"distance\":647.2,\"average_grade\":3.9,\"maximum_grade\":6.8,\"elevation_high\":474.0,\"elevation_low\":448.8,\"start_latlng\":[46.160003,6.117209],\"end_latlng\":[46.154429,6.117665],\"start_latitude\":46.160003,\"start_longitude\":6.117209,\"end_latitude\":46.154429,\"end_longitude\":6.117665,\"climb_category\":0,\"city\":\"Plan-les-Ouates\",\"state\":\"Genève\",\"country\":\"Switzerland\",\"private\":false,\"hazardous\":false,\"starred\":false},\"kom_rank\":null,\"pr_rank\":null,\"achievements\":[],\"hidden\":false},{\"id\":22914381954,\"resource_state\":2,\"name\":\"RT Route des Hospitaliers\",\"activity\":{\"id\":937800748,\"resource_state\":1},\"athlete\":{\"id\":112765,\"resource_state\":1},\"elapsed_time\":149,\"moving_time\":149,\"start_date\":\"2017-04-11T16:06:21Z\",\"start_date_local\":\"2017-04-11T18:06:21Z\",\"distance\":1235.5,\"start_index\":494,\"end_index\":643,\"average_cadence\":75.4,\"device_watts\":true,\"average_watts\":179.5,\"average_heartrate\":127.0,\"max_heartrate\":134.0,\"segment\":{\"id\":11281909,\"resource_state\":2,\"name\":\"RT Route des Hospitaliers\",\"activity_type\":\"Ride\",\"distance\":1234.2,\"average_grade\":0.5,\"maximum_grade\":3.9,\"elevation_high\":474.8,\"elevation_low\":466.6,\"start_latlng\":[46.151068,6.122564],\"end_latlng\":[46.145273,6.136094],\"start_latitude\":46.151068,\"start_longitude\":6.122564,\"end_latitude\":46.145273,\"end_longitude\":6.136094,\"climb_category\":0,\"city\":\"Bardonnex\",\"state\":\"Genève\",\"country\":\"Switzerland\",\"private\":false,\"hazardous\":false,\"starred\":false},\"kom_rank\":null,\"pr_rank\":null,\"achievements\":[],\"hidden\":false},{\"id\":22914381974,\"resource_state\":2,\"name\":\"Rue verdi sprint\",\"activity\":{\"id\":937800748,\"resource_state\":1},\"athlete\":{\"id\":112765,\"resource_state\":1},\"elapsed_time\":50,\"moving_time\":50,\"start_date\":\"2017-04-11T16:10:36Z\",\"start_date_local\":\"2017-04-11T18:10:36Z\",\"distance\":233.8,\"start_index\":749,\"end_index\":799,\"average_cadence\":75.5,\"device_watts\":true,\"average_watts\":218.1,\"average_heartrate\":137.7,\"max_heartrate\":140.0,\"segment\":{\"id\":13511481,\"resource_state\":2,\"name\":\"Rue verdi sprint\",\"activity_type\":\"Ride\",\"distance\":235.0,\"average_grade\":7.1,\"maximum_grade\":13.1,\"elevation_high\":502.0,\"elevation_low\":485.2,\"start_latlng\":[46.141434,6.141484],\"end_latlng\":[46.14092,6.144333],\"start_latitude\":46.141434,\"start_longitude\":6.141484,\"end_latitude\":46.14092,\"end_longitude\":6.144333,\"climb_category\":0,\"city\":null,\"state\":null,\"country\":null,\"private\":false,\"hazardous\":false,\"starred\":false},\"kom_rank\":null,\"pr_rank\":null,\"achievements\":[],\"hidden\":false},{\"id\":22914382050,\"resource_state\":2,\"name\":\"Collonges-Le Coin\",\"activity\":{\"id\":937800748,\"resource_state\":1},\"athlete\":{\"id\":112765,\"resource_state\":1},\"elapsed_time\":430,\"moving_time\":430,\"start_date\":\"2017-04-11T16:11:55Z\",\"start_date_local\":\"2017-04-11T18:11:55Z\",\"distance\":1599.1,\"start_index\":828,\"end_index\":1258,\"average_cadence\":67.7,\"device_watts\":true,\"average_watts\":273.7,\"average_heartrate\":152.3,\"max_heartrate\":166.0,\"segment\":{\"id\":4324159,\"resource_state\":2,\"name\":\"Collonges-Le Coin\",\"activity_type\":\"Ride\",\"distance\":1626.3,\"average_grade\":9.8,\"maximum_grade\":19.0,\"elevation_high\":629.6,\"elevation_low\":470.8,\"start_latlng\":[46.140225,6.145239],\"end_latlng\":[46.132309,6.156843],\"start_latitude\":46.140225,\"start_longitude\":6.145239,\"end_latitude\":46.132309,\"end_longitude\":6.156843,\"climb_category\":1,\"city\":\"Collonges Sous Salève\",\"state\":\"RA\",\"country\":\"France\",\"private\":false,\"hazardous\":false,\"starred\":false},\"kom_rank\":null,\"pr_rank\":null,\"achievements\":[],\"hidden\":false},{\"id\":22914382004,\"resource_state\":2,\"name\":\"Interval Bourg d en Haut\",\"activity\":{\"id\":937800748,\"resource_state\":1},\"athlete\":{\"id\":112765,\"resource_state\":1},\"elapsed_time\":104,\"moving_time\":104,\"start_date\":\"2017-04-11T16:12:00Z\",\"start_date_local\":\"2017-04-11T18:12:00Z\",\"distance\":531.7,\"start_index\":833,\"end_index\":937,\"average_cadence\":77.4,\"device_watts\":true,\"average_watts\":385.7,\"average_heartrate\":161.5,\"max_heartrate\":166.0,\"segment\":{\"id\":9065891,\"resource_state\":2,\"name\":\"Interval Bourg d en Haut\",\"activity_type\":\"Ride\",\"distance\":546.4,\"average_grade\":9.8,\"maximum_grade\":16.6,\"elevation_high\":555.4,\"elevation_low\":501.6,\"start_latlng\":[46.14005,6.145174],\"end_latlng\":[46.135666,6.146822],\"start_latitude\":46.14005,\"start_longitude\":6.145174,\"end_latitude\":46.135666,\"end_longitude\":6.146822,\"climb_category\":0,\"city\":\"Collonges-sous-Salève\",\"state\":\"Rhône-Alpes\",\"country\":\"France\",\"private\":false,\"hazardous\":false,\"starred\":false},\"kom_rank\":null,\"pr_rank\":2,\"achievements\":[{\"type_id\":3,\"type\":\"pr\",\"rank\":2}],\"hidden\":false},{\"id\":22914382115,\"resource_state\":2,\"name\":\"Les Terrasses de Genève\",\"activity\":{\"id\":937800748,\"resource_state\":1},\"athlete\":{\"id\":112765,\"resource_state\":1},\"elapsed_time\":273,\"moving_time\":273,\"start_date\":\"2017-04-11T16:26:32Z\",\"start_date_local\":\"2017-04-11T18:26:32Z\",\"distance\":1518.3,\"start_index\":1705,\"end_index\":1978,\"average_cadence\":76.2,\"device_watts\":true,\"average_watts\":352.9,\"average_heartrate\":160.1,\"max_heartrate\":169.0,\"segment\":{\"id\":1262132,\"resource_state\":2,\"name\":\"Les Terrasses de Genève\",\"activity_type\":\"Ride\",\"distance\":1552.41,\"average_grade\":7.0,\"maximum_grade\":14.0,\"elevation_high\":590.4,\"elevation_low\":480.6,\"start_latlng\":[46.15376872010529,6.161637771874666],\"end_latlng\":[46.144184516742826,6.15893729031086],\"start_latitude\":46.15376872010529,\"start_longitude\":6.161637771874666,\"end_latitude\":46.144184516742826,\"end_longitude\":6.15893729031086,\"climb_category\":1,\"city\":\"Bossey\",\"state\":\"Rhône-Alpes\",\"country\":\"France\",\"private\":false,\"hazardous\":false,\"starred\":false},\"kom_rank\":null,\"pr_rank\":3,\"achievements\":[{\"type_id\":3,\"type\":\"pr\",\"rank\":3}],\"hidden\":false},{\"id\":22914382138,\"resource_state\":2,\"name\":\"Route des Hospitaliers\",\"activity\":{\"id\":937800748,\"resource_state\":1},\"athlete\":{\"id\":112765,\"resource_state\":1},\"elapsed_time\":170,\"moving_time\":170,\"start_date\":\"2017-04-11T16:34:49Z\",\"start_date_local\":\"2017-04-11T18:34:49Z\",\"distance\":1381.3,\"start_index\":2202,\"end_index\":2372,\"average_cadence\":62.6,\"device_watts\":true,\"average_watts\":137.8,\"average_heartrate\":123.1,\"max_heartrate\":127.0,\"segment\":{\"id\":3731902,\"resource_state\":2,\"name\":\"Route des Hospitaliers\",\"activity_type\":\"Ride\",\"distance\":1349.99,\"average_grade\":-0.6,\"maximum_grade\":3.1,\"elevation_high\":478.8,\"elevation_low\":470.7,\"start_latlng\":[46.144911,6.13666],\"end_latlng\":[46.151253,6.121993],\"start_latitude\":46.144911,\"start_longitude\":6.13666,\"end_latitude\":46.151253,\"end_longitude\":6.121993,\"climb_category\":0,\"city\":\"Collonges Sous Salève\",\"state\":\"RA\",\"country\":\"France\",\"private\":false,\"hazardous\":false,\"starred\":false},\"kom_rank\":null,\"pr_rank\":null,\"achievements\":[],\"hidden\":false}],\"splits_metric\":[{\"distance\":1005.4,\"elapsed_time\":189,\"elevation_difference\":21.6,\"moving_time\":189,\"split\":1,\"average_heartrate\":117.41798941798942,\"average_speed\":5.32,\"pace_zone\":0},{\"distance\":994.7,\"elapsed_time\":184,\"elevation_difference\":39.8,\"moving_time\":184,\"split\":2,\"average_heartrate\":134.58152173913044,\"average_speed\":5.41,\"pace_zone\":0},{\"distance\":1000.9,\"elapsed_time\":151,\"elevation_difference\":2.4,\"moving_time\":151,\"split\":3,\"average_heartrate\":115.62913907284768,\"average_speed\":6.63,\"pace_zone\":0},{\"distance\":1000.6,\"elapsed_time\":121,\"elevation_difference\":6.6,\"moving_time\":121,\"split\":4,\"average_heartrate\":130.04132231404958,\"average_speed\":8.27,\"pace_zone\":0},{\"distance\":1001.4,\"elapsed_time\":197,\"elevation_difference\":32.0,\"moving_time\":197,\"split\":5,\"average_heartrate\":127.65482233502539,\"average_speed\":5.08,\"pace_zone\":0},{\"distance\":998.5,\"elapsed_time\":250,\"elevation_difference\":96.6,\"moving_time\":250,\"split\":6,\"average_heartrate\":158.976,\"average_speed\":3.99,\"pace_zone\":0},{\"distance\":1002.1,\"elapsed_time\":236,\"elevation_difference\":52.2,\"moving_time\":236,\"split\":7,\"average_heartrate\":138.42372881355934,\"average_speed\":4.25,\"pace_zone\":0},{\"distance\":999.0,\"elapsed_time\":89,\"elevation_difference\":-70.4,\"moving_time\":89,\"split\":8,\"average_heartrate\":96.67415730337079,\"average_speed\":11.22,\"pace_zone\":0},{\"distance\":1006.0,\"elapsed_time\":92,\"elevation_difference\":-90.8,\"moving_time\":92,\"split\":9,\"average_heartrate\":98.78260869565217,\"average_speed\":10.93,\"pace_zone\":0},{\"distance\":992.6,\"elapsed_time\":111,\"elevation_difference\":-42.4,\"moving_time\":111,\"split\":10,\"average_heartrate\":114.86486486486487,\"average_speed\":8.94,\"pace_zone\":0},{\"distance\":1003.0,\"elapsed_time\":115,\"elevation_difference\":-3.6,\"moving_time\":115,\"split\":11,\"average_heartrate\":125.94782608695652,\"average_speed\":8.72,\"pace_zone\":0},{\"distance\":996.3,\"elapsed_time\":187,\"elevation_difference\":82.2,\"moving_time\":187,\"split\":12,\"average_heartrate\":162.19786096256684,\"average_speed\":5.33,\"pace_zone\":0},{\"distance\":1005.6,\"elapsed_time\":127,\"elevation_difference\":-3.0,\"moving_time\":127,\"split\":13,\"average_heartrate\":147.51968503937007,\"average_speed\":7.92,\"pace_zone\":0},{\"distance\":999.8,\"elapsed_time\":98,\"elevation_difference\":-51.4,\"moving_time\":98,\"split\":14,\"average_heartrate\":107.83673469387755,\"average_speed\":10.2,\"pace_zone\":0},{\"distance\":997.9,\"elapsed_time\":128,\"elevation_difference\":-6.8,\"moving_time\":128,\"split\":15,\"average_heartrate\":119.0546875,\"average_speed\":7.8,\"pace_zone\":0},{\"distance\":996.9,\"elapsed_time\":121,\"elevation_difference\":0.4,\"moving_time\":121,\"split\":16,\"average_heartrate\":123.62809917355372,\"average_speed\":8.24,\"pace_zone\":0},{\"distance\":1004.6,\"elapsed_time\":99,\"elevation_difference\":-12.8,\"moving_time\":99,\"split\":17,\"average_heartrate\":131.82828282828282,\"average_speed\":10.15,\"pace_zone\":0},{\"distance\":994.9,\"elapsed_time\":99,\"elevation_difference\":-34.8,\"moving_time\":99,\"split\":18,\"average_heartrate\":126.85858585858585,\"average_speed\":10.05,\"pace_zone\":0},{\"distance\":381.2,\"elapsed_time\":46,\"elevation_difference\":-13.8,\"moving_time\":46,\"split\":19,\"average_heartrate\":103.1304347826087,\"average_speed\":8.29,\"pace_zone\":0}],\"splits_standard\":[{\"distance\":1609.4,\"elapsed_time\":303,\"elevation_difference\":45.6,\"moving_time\":303,\"split\":1,\"average_heartrate\":121.74917491749174,\"average_speed\":5.31,\"pace_zone\":0},{\"distance\":1612.9,\"elapsed_time\":248,\"elevation_difference\":19.2,\"moving_time\":248,\"split\":2,\"average_heartrate\":125.00403225806451,\"average_speed\":6.5,\"pace_zone\":0},{\"distance\":1607.7,\"elapsed_time\":247,\"elevation_difference\":20.8,\"moving_time\":247,\"split\":3,\"average_heartrate\":125.7246963562753,\"average_speed\":6.51,\"pace_zone\":0},{\"distance\":1611.1,\"elapsed_time\":434,\"elevation_difference\":157.0,\"moving_time\":434,\"split\":4,\"average_heartrate\":152.34101382488478,\"average_speed\":3.71,\"pace_zone\":0},{\"distance\":1611.7,\"elapsed_time\":189,\"elevation_difference\":-66.2,\"moving_time\":189,\"split\":5,\"average_heartrate\":114.52910052910053,\"average_speed\":8.53,\"pace_zone\":0},{\"distance\":1610.6,\"elapsed_time\":160,\"elevation_difference\":-125.6,\"moving_time\":160,\"split\":6,\"average_heartrate\":102.39375,\"average_speed\":10.07,\"pace_zone\":0},{\"distance\":1607.0,\"elapsed_time\":209,\"elevation_difference\":17.2,\"moving_time\":209,\"split\":7,\"average_heartrate\":134.41148325358853,\"average_speed\":7.69,\"pace_zone\":0},{\"distance\":1608.8,\"elapsed_time\":245,\"elevation_difference\":58.8,\"moving_time\":245,\"split\":8,\"average_heartrate\":158.67755102040817,\"average_speed\":6.57,\"pace_zone\":0},{\"distance\":1605.7,\"elapsed_time\":173,\"elevation_difference\":-57.2,\"moving_time\":173,\"split\":9,\"average_heartrate\":111.22543352601156,\"average_speed\":9.28,\"pace_zone\":0},{\"distance\":1613.2,\"elapsed_time\":200,\"elevation_difference\":-4.0,\"moving_time\":200,\"split\":10,\"average_heartrate\":123.48,\"average_speed\":8.07,\"pace_zone\":0},{\"distance\":1610.5,\"elapsed_time\":157,\"elevation_difference\":-35.6,\"moving_time\":157,\"split\":11,\"average_heartrate\":133.2611464968153,\"average_speed\":10.26,\"pace_zone\":0},{\"distance\":672.8,\"elapsed_time\":75,\"elevation_difference\":-26.0,\"moving_time\":75,\"split\":12,\"average_heartrate\":105.38666666666667,\"average_speed\":8.97,\"pace_zone\":0}],\"laps\":[{\"id\":2883952369,\"resource_state\":2,\"name\":\"Lap 1\",\"activity\":{\"id\":937800748,\"resource_state\":1},\"athlete\":{\"id\":112765,\"resource_state\":1},\"elapsed_time\":865,\"moving_time\":865,\"start_date\":\"2017-04-11T15:58:07Z\",\"start_date_local\":\"2017-04-11T17:58:07Z\",\"distance\":5108.04,\"start_index\":0,\"end_index\":865,\"total_elevation_gain\":116.0,\"average_speed\":5.91,\"max_speed\":9.9,\"average_cadence\":66.4,\"device_watts\":true,\"average_watts\":178.4,\"average_heartrate\":126.0,\"max_heartrate\":161.0,\"lap_index\":1,\"split\":1},{\"id\":2883952370,\"resource_state\":2,\"name\":\"Lap 2\",\"activity\":{\"id\":937800748,\"resource_state\":1},\"athlete\":{\"id\":112765,\"resource_state\":1},\"elapsed_time\":105,\"moving_time\":105,\"start_date\":\"2017-04-11T16:12:33Z\",\"start_date_local\":\"2017-04-11T18:12:33Z\",\"distance\":584.7,\"start_index\":866,\"end_index\":971,\"total_elevation_gain\":42.0,\"average_speed\":5.57,\"max_speed\":7.2,\"average_cadence\":77.1,\"device_watts\":true,\"average_watts\":346.7,\"average_heartrate\":164.3,\"max_heartrate\":166.0,\"lap_index\":2,\"split\":2},{\"id\":2883952371,\"resource_state\":2,\"name\":\"Lap 3\",\"activity\":{\"id\":937800748,\"resource_state\":1},\"athlete\":{\"id\":112765,\"resource_state\":1},\"elapsed_time\":733,\"moving_time\":733,\"start_date\":\"2017-04-11T16:14:19Z\",\"start_date_local\":\"2017-04-11T18:14:19Z\",\"distance\":5100.53,\"start_index\":972,\"end_index\":1704,\"total_elevation_gain\":92.0,\"average_speed\":6.96,\"max_speed\":12.9,\"average_cadence\":65.2,\"device_watts\":true,\"average_watts\":131.4,\"average_heartrate\":125.6,\"max_heartrate\":165.0,\"lap_index\":3,\"split\":3},{\"id\":2883952372,\"resource_state\":2,\"name\":\"Lap 4\",\"activity\":{\"id\":937800748,\"resource_state\":1},\"athlete\":{\"id\":112765,\"resource_state\":1},\"elapsed_time\":254,\"moving_time\":254,\"start_date\":\"2017-04-11T16:26:32Z\",\"start_date_local\":\"2017-04-11T18:26:32Z\",\"distance\":1416.57,\"start_index\":1705,\"end_index\":1959,\"total_elevation_gain\":101.0,\"average_speed\":5.58,\"max_speed\":9.0,\"average_cadence\":76.9,\"device_watts\":true,\"average_watts\":378.0,\"average_heartrate\":159.7,\"max_heartrate\":169.0,\"lap_index\":4,\"split\":4},{\"id\":2883952373,\"resource_state\":2,\"name\":\"Lap 5\",\"activity\":{\"id\":937800748,\"resource_state\":1},\"athlete\":{\"id\":112765,\"resource_state\":1},\"elapsed_time\":681,\"moving_time\":681,\"start_date\":\"2017-04-11T16:30:47Z\",\"start_date_local\":\"2017-04-11T18:30:47Z\",\"distance\":6171.6,\"start_index\":1960,\"end_index\":2640,\"total_elevation_gain\":7.0,\"average_speed\":9.06,\"max_speed\":15.6,\"average_cadence\":61.2,\"device_watts\":true,\"average_watts\":96.8,\"average_heartrate\":122.7,\"max_heartrate\":168.0,\"lap_index\":5,\"split\":5}],\"gear\":{\"id\":\"b1433648\",\"primary\":false,\"name\":\"Fascenario 0.7 (Bleu)\",\"resource_state\":2,\"distance\":1437272.0},\"partner_brand_tag\":null,\"photos\":{\"primary\":null,\"count\":0},\"device_name\":\"Garmin Edge 1000\",\"embed_token\":\"2a0c557e2f614dfc422f451eed7349da45876137\"}");
}

QByteArray
Strava::mockStream()
{
    printd("Strava::mockStream()\n");

    return QByteArray("[{\"type\": \"time\",\"data\": [0, 1, 99, 100]},{\"type\": \"latlng\",\"data\": [[43.96240499998943, 11.114369999870984],[43.96241602169922, 11.114389594048717],[43.96246041052659, 11.114477067306458],[43.96250000005456, 11.11457999984086]]},{\"type\": \"distance\",\"data\": [0.0, 9.222209323078733, 32353.339870102107, 32370.98213305391]},{\"type\": \"altitude\",\"data\": [97.73514151793162, 97.42, 115.21365665991632, 115.0]}]");
}

static bool addStrava() {
    CloudServiceFactory::instance().addService(new Strava(NULL));
    return true;
}

static bool add = addStrava();

