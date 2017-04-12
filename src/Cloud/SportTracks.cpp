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

#include "SportTracks.h"
#include "Athlete.h"
#include "Settings.h"
#include "Secrets.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef SPORTTRACKS_DEBUG
// TODO(gille): This should be a command line flag.
#define SPORTTRACKS_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (SPORTTRACKS_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (SPORTTRACKS_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

SportTracks::SportTracks(Context *context) : CloudService(context), context(context), root_(NULL) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = none; // gzip
    downloadCompression = none;
    filetype = FIT;
    useMetric = true; // distance and duration metadata

    // config
    settings.insert(OAuthToken, GC_SPORTTRACKS_TOKEN);
    settings.insert(Local1, GC_SPORTTRACKS_REFRESH_TOKEN);
    settings.insert(Local2, GC_SPORTTRACKS_LAST_REFRESH);
}

SportTracks::~SportTracks() {
    if (context) delete nam;
}

void
SportTracks::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

// open by connecting and getting a basic list of folders available
bool
SportTracks::open(QStringList &errors)
{
    printd("SportTracks::open\n");

    // do we have a token
    QString token = getSetting(GC_SPORTTRACKS_REFRESH_TOKEN, "").toString();
    if (token == "") {
        errors << "You must authorise with SportTracks first";
        return false;
    }

    printd("Get access token for this session.\n");

    // refresh endpoint
    QNetworkRequest request(QUrl("https://api.sporttracks.mobi/oauth2/token?"));
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");

    // set params
    QString data;
    data += "client_id=" GC_SPORTTRACKS_CLIENT_ID;
    data += "&client_secret=" GC_SPORTTRACKS_CLIENT_SECRET;
    data += "&refresh_token=" + getSetting(GC_SPORTTRACKS_REFRESH_TOKEN).toString();
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
    if (access_token != "") setSetting(GC_SPORTTRACKS_TOKEN, access_token);
    if (refresh_token != "") setSetting(GC_SPORTTRACKS_REFRESH_TOKEN, refresh_token);
    setSetting(GC_SPORTTRACKS_LAST_REFRESH, QDateTime::currentDateTime());

    // get the factory to save our settings permanently
    CloudServiceFactory::instance().saveSettings(this, context);
    return true;
}

bool
SportTracks::close()
{
    printd("SportTracks::close\n");
    // nothing to do for now
    return true;
}

QList<CloudServiceEntry*>
SportTracks::readdir(QString path, QStringList &errors, QDateTime, QDateTime)
{
    printd("SportTracks::readdir(%s)\n", path.toStdString().c_str());

    QList<CloudServiceEntry*> returning;

    // do we have a token
    QString token = getSetting(GC_SPORTTRACKS_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with SportTracks first");
        return returning;
    }

    // we get pages of 25 entries
    bool wantmore=true;
    int page=0;

    while (wantmore) {

        // we assume there are no more unless we get a full page
        wantmore=false;

        // set params - lets explicitly get 25 per page
        QString urlstr("https://api.sporttracks.mobi/api/v2/fitnessActivities?");
        urlstr += "pageSize=25";
        urlstr += "&page=" + QString("%1").arg(page);

        // fetch page by page
        QUrl url(urlstr);
        QNetworkRequest request(url);
        request.setRawHeader("Authorization", QString("Bearer %1").arg(getSetting(GC_SPORTTRACKS_TOKEN,"").toString()).toLatin1());
        request.setRawHeader("Accept", "application/json");

        // make request
        printd("fetch page: %s\n", urlstr.toStdString().c_str());
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

            int received = 0;
            printd("page %d: %s\n", page, r.toStdString().c_str());

            // parse JSON payload
            QJsonParseError parseError;
            QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

            printd("parse (%d): %s\n", parseError.error, parseError.errorString().toStdString().c_str());
            if (parseError.error == QJsonParseError::NoError) {

                QJsonArray items = document.object()["items"].toArray();
                received = items.count();
                for(int i=0; i<items.count(); i++) {

                    QJsonObject item = items.at(i).toObject();
                    CloudServiceEntry *add = newCloudServiceEntry();

                    // each item looks like this:
                    // {
                    //     "duration": "6492.00",
                    //     "name": "Cycling",
                    //     "start_time": "2016-08-29T11:14:46+01:00",
                    //     "total_distance": "40188",
                    //     "type": "Cycling",
                    //     "uri": "https://api.sporttracks.mobi/api/v2/fitnessActivities/13861860",
                    //     "user_id": "57556"
                    // }

                    add->name = QDateTime::fromString(item["start_time"].toString(), Qt::ISODate).toString("yyyy_MM_dd_HH_mm_ss")+".json";
                    add->distance = item["total_distance"].toString().toDouble() / 1000.0f;
                    add->duration = item["duration"].toString().toDouble();
                    add->id = item["uri"].toString().split("/").last();
                    add->label = add->id;
                    add->isDir = false;

                    printd("item: %s\n", add->name.toStdString().c_str());

                    returning << add;
                }

                // we should see if there are more
                if (received == 25) wantmore = true;
            }
        }
        page++;
    }

    // all good ?
    printd("returning count(%d), errors(%s)\n", returning.count(), errors.join(",").toStdString().c_str());
    return returning;
}

// read a file at location (relative to home) into passed array
bool
SportTracks::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("SportTracks::readFile(%s, %s)\n", remotename.toStdString().c_str(), remoteid.toStdString().c_str());

    // do we have a token ?
    QString token = getSetting(GC_SPORTTRACKS_TOKEN, "").toString();
    if (token == "") return false;

    // fetch
    QString url = QString("https://api.sporttracks.mobi/api/v2/fitnessActivities/%1").arg(remoteid);

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
SportTracks::readyRead()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

// SportTracks workouts are delivered back as JSON, the original is lost
// so we need to parse the response and turn it into a JSON file to
// import. The description of the format is here:
// https://sporttracks.mobi/api/doc/data-structures
void
SportTracks::readFileCompleted()
{
    printd("SportTracks::readFileCompleted\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s ...\n", buffers.value(reply)->toStdString().substr(0,900).c_str());

    // we need to parse the JSON and create a ridefile from it
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(*buffers.value(reply), &parseError);

    printd("parse (%d): %s\n", parseError.error, parseError.errorString().toStdString().c_str());
    if (parseError.error == QJsonParseError::NoError) {

        // extract the main elements
        //
        // SUMMARY DATA
        // start_time	string (ISO 8601 DateTime)	Start time of the workout
        // type	string ( Workout Type)	Type of the workout
        // name	string	Display name of the workout
        // notes	string	Workout notes
        // location_name	string	Named (text) workout location
        // laps	array	An array of Laps
        // timer_stops	array	An array of Timer Stops
        //
        // SAMPLES DATA
        // location	array (Track Data)	List of lat/lng pairs associated with this workout
        // elevation	array (Track Data)	List of altitude data (meters) associated with this workout
        // distance	array (Track Data)	List of meters moved associated with this workout
        // heartrate	array (Track Data)	List of heartrate data (bpm) associated with this workout
        // cadence	array (Track Data)	List of cadence data (rpm) associated with this workout
        // power	array (Track Data)	List of power data (watts) associated with this workout
        // temperature	array (Track Data)	List of temperature data (Celsius) associated with this workout
        // vertical_oscillation	array (Track Data)	List of vertical oscillation data (millimeters) associated with this workout
        // ground_contact_time	array (Track Data)	List of ground contact time data (milliseconds) associated with this workout
        // left_power_percent	array (Track Data)	List of power balance data (0=left, 100=right) associated with this workout
        // total_hemoglobin_conc	array (Track Data)	List of total blood hemoglobin saturation (g/dL) associated with this workout
        // saturated_hemoglobin_percent	array (Track Data)	List of percent hemoglobin oxygen saturation (0 - 100) associated with this workout

#if 0
        QJsonObject ride = document.object();
        QJsonArray location = ride["location"].toArray();
        QJsonArray distance = ride["distance"].toArray();
        QJsonArray cadence = ride["cadence"].toArray();
        QJsonArray power = ride["power"].toArray();

        printd("samples: loc %d, dist %d, cad %d, power %d\n", location.count(), distance.count(), cadence.count(), power.count());
#endif
    }

    //XX not yet notifyReadComplete(buffers.value(reply), replyName(reply), tr("Completed."));
}

bool
SportTracks::writeFile(QByteArray &data, QString remotename, RideFile *)
{
    printd("SportTracks::writeFile(%s)\n", remotename.toStdString().c_str());
#if 0
    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done

    // do we have a token ?
    QString token = getSetting(GC_SPORTTRACKS_TOKEN, "").toString();
    if (token == "") return false;

    // lets connect and get basic info on the root directory
    QString url = QString("%1/rest/files/upload")
          .arg(getSetting(GC_SPORTTRACKS_URL, "https://whats.todaysplan.com.au").toString());

    printd("URL used: %s\n", url.toStdString().c_str());

    QNetworkRequest request = QNetworkRequest(url);

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QString boundary = QVariant(qrand()).toString()+QVariant(qrand()).toString()+QVariant(qrand()).toString();
    multiPart->setBoundary(boundary.toLatin1());

    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    QHttpPart jsonPart;
    jsonPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"json\""));

    QString userId = getSetting(GC_SPORTTRACKS_ATHLETE_ID, "").toString();

    QString json;
    if (userId.length()>0) {
        json  = QString("{ filename: \"%1\", userId: %2 }").arg(remotename).arg(userId);
    } else {
        json  = QString("{ filename: \"%1\" }").arg(remotename);
    }
    printd("request: %s\n", json.toStdString().c_str());
    jsonPart.setBody(json.toLatin1());

    QHttpPart attachmentPart;
    attachmentPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"attachment\"; type=\"text/xml\""));
    attachmentPart.setBody(data);

    multiPart->append(jsonPart);
    multiPart->append(attachmentPart);

    // post the file
    QNetworkReply *reply;

    reply = nam->post(request, multiPart);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply,remotename);
#endif
    return true;
}

void
SportTracks::writeFileCompleted()
{
    printd("SportTracks::writeFileCompleted()\n");
#if 0
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    QByteArray r = reply->readAll();
    printd("reply:%s\n", r.toStdString().c_str());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    if (reply->error() == QNetworkReply::NoError) {
        QString name = replyName(static_cast<QNetworkReply*>(QObject::sender()));

        QJsonObject result = document.object();//["result"].toObject();
        replyActivity.insert(name, result);
        //rideSend(name);

        notifyWriteComplete( name, tr("Completed."));

    } else {

        notifyWriteComplete( replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Network Error - Upload failed."));
    }
#endif

}
// development put on hold - AccessLink API compatibility issues with Desktop applications
static bool addSportTracks() {
    CloudServiceFactory::instance().addService(new SportTracks(NULL));
    return true;
}

static bool add = addSportTracks();
