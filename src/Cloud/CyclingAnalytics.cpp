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

#include "CyclingAnalytics.h"
#include "Athlete.h"
#include "Settings.h"
#include "mvjson.h"
#include "JsonRideFile.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef CYCLINGANALYTICS_DEBUG
#define CYCLINGANALYTICS_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (CYCLINGANALYTICS_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (CYCLINGANALYTICS_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

CyclingAnalytics::CyclingAnalytics(Context *context) : CloudService(context), context(context), root_(NULL) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = none; // gzip
    downloadCompression = none; // gzip
    filetype = uploadType::FIT;
    useMetric = true; // distance and duration metadata

    // config
    settings.insert(OAuthToken, GC_CYCLINGANALYTICS_TOKEN);
}

CyclingAnalytics::~CyclingAnalytics() {
    if (context) delete nam;
}

void
CyclingAnalytics::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

bool
CyclingAnalytics::open(QStringList &errors)
{
    printd("CyclingAnalytics::open\n");
    QString token = getSetting(GC_CYCLINGANALYTICS_TOKEN, "").toString();
    if (token == "") {
        errors << tr("Account is not configured.");
        return false;
    }
    return true;
}

bool
CyclingAnalytics::close()
{
    printd("CyclingAnalytics::close\n");
    // nothing to do for now
    return true;
}

QList<CloudServiceEntry*>
CyclingAnalytics::readdir(QString path, QStringList &errors, QDateTime, QDateTime)
{
    printd("CyclingAnalytics::readdir(%s)\n", path.toStdString().c_str());

    QList<CloudServiceEntry*> returning;

    // do we have a token
    QString token = getSetting(GC_CYCLINGANALYTICS_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with Cycling Analytics first");
        return returning;
    }

    // set params - lets explicitly get 25 per page
    QString urlstr("https://www.cyclinganalytics.com/api/me/rides?");
    urlstr += "power_curve=false";
    urlstr += "&epower_curve=false";

    // fetch in one hit
    QUrl url(urlstr);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(getSetting(GC_CYCLINGANALYTICS_TOKEN,"").toString()).toLatin1());
    request.setRawHeader("Accept", "application/json");

    // make request
    printd("fetch list: %s\n", urlstr.toStdString().c_str());
    QNetworkReply *reply = nam->get(request);

    // blocking request, with a 30s timeout
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    QTimer::singleShot(30000,&loop, SLOT(quit())); // timeout after 30 seconds

    // if successful, lets unpack
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    printd("fetch response: %d: %s\n", reply->error(), reply->errorString().toStdString().c_str());

    if (reply->error() == 0) {

        // get the data
        QByteArray r = reply->readAll();

        // parse JSON payload
        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        printd("parse (%d): %s\n", parseError.error, parseError.errorString().toStdString().c_str());
        if (parseError.error == QJsonParseError::NoError) {

            QJsonArray rides = document.object()["rides"].toArray();
            for(int i=0; i<rides.count(); i++) {

                QJsonObject item = rides.at(i).toObject();
                CloudServiceEntry *add = newCloudServiceEntry();

                // We extract:
                // * summary - distance
                // * summary - duration
                // * starttime UTC
                // * format
                // * id
                //
                // Each record looks like this:
                // {
                //     "user_id": 8862271,
                //     "summary": {
                //         "pwc_r2": 0,
                //         "avg_power": 0,
                //         "avg_heartrate": 1,
                //         "riding_time": 5320,
                //         "max_speed": 53.0,
                //         "load": 0,
                //         "work": 0,
                //         "pwc170": 0,
                //         "max_heartrate": 1,
                //         "intensity": 0.0,
                //         "max_cadence": 0,
                //         "distance": 39.852,
                //         "pwc150": 0,
                //         "avg_speed": 26.96751879699248,
                //         "trimp": 0,
                //         "max_power": 0,
                //         "climbing": 578,
                //         "variability": 0,
                //         "max_temperature": 0.0,
                //         "lrbalance": null,
                //         "epower": 0,
                //         "avg_temperature": 0.0,
                //         "moving_time": 5320,
                //         "duration": 5471,
                //         "min_temperature": 0.0,
                //         "zones": {
                //             "heartrate": [
                //                 0,
                //                 0,
                //                 0,
                //                 0,
                //                 0
                //             ],
                //             "power": [
                //                 5471,
                //                 0,
                //                 0,
                //                 0,
                //                 0,
                //                 0,
                //                 0
                //             ]
                //         },
                //         "total_time": 5476,
                //         "avg_cadence": 0
                //     },
                //     "utc_datetime": "2012-08-18T11:11:27",
                //     "trainer": false,
                //     "format": "tcx",
                //     "has": {
                //         "temperature": false,
                //         "cadence": false,
                //         "gps": true,
                //         "speed": true,
                //         "heartrate": true,
                //         "power": false,
                //         "elevation": true
                //     },
                //     "id": 104592924158,
                //     "title": "40km around Cranleigh",
                //     "notes": "",
                //     "local_datetime": "2012-08-18T12:11:27"
                // }

                add->name = QDateTime::fromString(item["local_datetime"].toString(), Qt::ISODate).toString("yyyy_MM_dd_HH_mm_ss")+".json";

                QJsonObject summary = item["summary"].toObject();
                add->distance = summary["distance"].toDouble();
                add->duration = summary["duration"].toDouble();
                add->id = QString("%1").arg(item["id"].toDouble(), 0, 'f', 0);
                add->label = add->id;
                add->isDir = false;

                printd("item: %s %s\n", add->id.toStdString().c_str(), add->name.toStdString().c_str());

                returning << add;
            }
        }
    }


    // all good ?
    printd("returning count(%d), errors(%s)\n", returning.count(), errors.join(",").toStdString().c_str());
    return returning;
}

// read a file at location (relative to home) into passed array
bool
CyclingAnalytics::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("CyclingAnalytics::readFile(%s, %s)\n", remotename.toStdString().c_str(), remoteid.toStdString().c_str());

    // do we have a token ?
    QString token = getSetting(GC_CYCLINGANALYTICS_TOKEN, "").toString();
    if (token == "") return false;

    // fetch - all data streams
    QString url = QString("https://www.cyclinganalytics.com/api/ride/%1?streams=all").arg(remoteid);

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
CyclingAnalytics::readyRead()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

// CyclingAnalytics workouts are delivered back as JSON, the original is lost
// so we need to parse the response and turn it into a JSON file to
// import. The description of the format is here:
// https://sporttracks.mobi/api/doc/data-structures
void
CyclingAnalytics::readFileCompleted()
{
    printd("CyclingAnalytics::readFileCompleted\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s ...\n", buffers.value(reply)->toStdString().substr(0,900).c_str());

    QByteArray *returning = buffers.value(reply);

    // we need to parse the JSON and create a ridefile from it
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(*buffers.value(reply), &parseError);

    printd("parse (%d): %s\n", parseError.error, parseError.errorString().toStdString().c_str());
    if (parseError.error == QJsonParseError::NoError) {

        // extract the main elements
        //
        // SUMMARY DATA
        // local_datetime	string (ISO 8601 DateTime)	Local Starttime of the activity
        // title	string	Display name of the workout
        // notes	string	Workout notes

        QJsonObject ride = document.object();
        QDateTime starttime = QDateTime::fromString(ride["local_datetime"].toString(), Qt::ISODate);

        // 1s samples with start time as UTC
        RideFile *ret = new RideFile(starttime.toUTC(), 1.0f);

        // it is called Cycling Analytics after all !
        ret->setTag("Sport", "Bike");

        // location => route
        if (!ride["title"].isNull()) ret->setTag(tr("Title"), ride["title"].toString());
        if (!ride["notes"].isNull()) ret->setTag(tr("Notes"), ride["notes"].toString());

        // SAMPLES DATA
        //
        // for mapping from the names used in the CyclingAnalytics json response
        // to the series names we use in GC, note that lat covers lat and lon
        // so needs to be treated as a special case
        static struct {
            RideFile::seriestype type;
            const char *caname;
            double factor;                  // to convert from ST units to GC units
        } seriesnames[] = {

            // seriestype          sporttracks name           conversion factor
            { RideFile::lat,       "latitude",                     1.0f   },
            { RideFile::lon,       "longitude",                    1.0f   },
            { RideFile::lrbalance, "lrbalance",                    1.0f   },
            { RideFile::alt,       "elevation",                    1.0f   },
            { RideFile::km,        "distance" ,                    1.0f   },
            { RideFile::hr,        "heartrate",                    1.0f   },
            { RideFile::cad,       "cadence",                      1.0f   },
            { RideFile::watts,     "power",                        1.0f   },
            { RideFile::temp,      "temperature",                  1.0f   },
            { RideFile::kph,       "speed",                        1.0f   },
            { RideFile::slope,     "gradient",                     1.0f   },
            { RideFile::none,      "",                             0.0f   }

        };

        // data to combine into a new ride
        class st_track {
        public:
            double factor; // for converting
            RideFile::seriestype type;
            QJsonArray samples;
        };

        // create a list of all the data we will work with
        QList<st_track> data;

        QJsonObject streams = ride["streams"].toObject();
        int count=0; // how many samples in the streams?

        // examine the returned object and extract sample data
        for(int i=0; seriesnames[i].type != RideFile::none; i++) {
            QString name = seriesnames[i].caname;
            st_track add;

            // contained?
            if (streams[name].isArray()) {

                add.factor = seriesnames[i].factor;
                add.type = seriesnames[i].type;
                add.samples = streams[name].toArray();

                // they're all the same length
                count = add.samples.count();

                data << add;
            }
        }

        // we now work through the tracks combining them into samples
        // with a common offset (ie, by row, not column).
        int index=0;
        while (index < count) {

            // add new point for the point in time we are at
            RideFilePoint add;
            add.secs = index;

            // move through tracks if they're waiting for this point
            bool updated=false;
            for(int t=0; t<data.count(); t++) {

               // hr, distance et al
               double value = data[t].factor * data[t].samples.at(index).toDouble();

               // this is one for us, update and move on
               add.setValue(data[t].type, value);
            }

            // don't add blanks
            ret->appendPoint(add);
            index++;
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
CyclingAnalytics::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    Q_UNUSED(ride);

    printd("CyclingAnalytics::writeFile(%s)\n", remotename.toStdString().c_str());

    QString token = getSetting(GC_CYCLINGANALYTICS_TOKEN, "").toString();
    QUrl url = QUrl( "https://www.cyclinganalytics.com/api/me/upload" );
    QNetworkRequest request = QNetworkRequest(url);

    QString boundary = QVariant(QRandomGenerator::global()->generate()).toString()+QVariant(QRandomGenerator::global()->generate()).toString()+QVariant(QRandomGenerator::global()->generate()).toString();

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    multiPart->setBoundary(boundary.toLatin1());

    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    QHttpPart notesPart;
    notesPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"notes\""));
    notesPart.setBody(ride->getTag(tr("Notes"), "").toLatin1());

    QHttpPart dataTypePart;
    dataTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"format\""));
    dataTypePart.setBody("fit");

    QHttpPart filenamePart;
    filenamePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"filename\""));
    filenamePart.setBody(remotename.toLatin1());

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"data\"; filename=\"file.fit\"; type=\"text/xml\""));
    filePart.setBody(data);

    multiPart->append(notesPart);
    multiPart->append(filenamePart);
    multiPart->append(dataTypePart);
    multiPart->append(filePart);

    reply = nam->post(request, multiPart);
    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply,remotename);
    return true;
}

void
CyclingAnalytics::writeFileCompleted()
{
    printd("CyclingAnalytics::writeFileCompleted()\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());


    // remember why we errored
    bool uploadSuccessful = false;
    QString uploadError;

    try {

        // parse the response
        QString response = reply->readAll();
        MVJSONReader jsonResponse(response.toStdString());

        printd("reply:%s\n", response.toStdString().c_str());

        // get values
        if (jsonResponse.root) uploadError = jsonResponse.root->getFieldString("error").c_str();
        else uploadError = "unrecognised response.";
        //XXXcyclingAnalyticsUploadId = jsonResponse.root->getFieldInt("upload_id");

    } catch(...) {

        // problem!
        uploadError = "bad response or parser exception.";
        //XXXcyclingAnalyticsUploadId = 0;
    }

    // if not there clean out
    if (uploadError.toLower() == "none" || uploadError.toLower() == "null") uploadError = "";

    // update id if uploaded successfully
    if (uploadError.length()>0 || reply->error() != QNetworkReply::NoError)  uploadSuccessful = false;
    else {

        // Success
        //XXX ride->ride()->setTag("CyclingAnalytics uploadId", QString("%1").arg(cyclingAnalyticsUploadId));
        //XXX ride->setDirty(true);
        uploadSuccessful = true;
    }

    if (uploadSuccessful && reply->error() == QNetworkReply::NoError) {
        notifyWriteComplete( replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Completed."));
    } else {
        notifyWriteComplete( replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Network Error - Upload failed."));
    }
}

static bool addCyclingAnalytics() {
    CloudServiceFactory::instance().addService(new CyclingAnalytics(NULL));
    return true;
}

static bool add = addCyclingAnalytics();
