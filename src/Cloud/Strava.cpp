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
#define STRAVA_DEBUG false
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
    settings.insert(Metadata1, QString("%1::Activity Name").arg(GC_STRAVA_ACTIVITY_NAME));
}

Strava::~Strava() {
    if (context) delete nam;
}

QImage Strava::logo() const
{
    return QImage(":images/services/strava.png");
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

    // Do Paginated Access to the Activities List
    const int pageSize = 30;
    int offset = 0;
    int resultCount = INT_MAX;

    while (offset < resultCount) {
        QString urlstr = "https://www.strava.com/api/v3/athlete/activities?";

#if QT_VERSION > 0x050000
        QUrlQuery params;
#else
        QUrl params;
#endif

        // use toMSecsSinceEpoch for compatibility with QT4
        params.addQueryItem("before", QString::number(to.addDays(1).toMSecsSinceEpoch()/1000.0f, 'f', 0));
        params.addQueryItem("after", QString::number(from.addDays(-1).toMSecsSinceEpoch()/1000.0f, 'f', 0));
        params.addQueryItem("per_page", QString("%1").arg(pageSize));
        params.addQueryItem("page",  QString("%1").arg(offset/pageSize+1));

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
        printd("response: %s\n", r.toStdString().c_str());

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        // if path was returned all is good, lets set root
        if (parseError.error == QJsonParseError::NoError) {

            // results ?
            QJsonArray results = document.array();

            // lets look at that then
            if (results.size()>0) {
                for(int i=0; i<results.size(); i++) {
                    QJsonObject each = results.at(i).toObject();
                    CloudServiceEntry *add = newCloudServiceEntry();


                    //Strava has full path, we just want the file name
                    add->label = QFileInfo(each["name"].toString()).fileName();
                    add->id = QString("%1").arg(each["id"].toVariant().toULongLong());

                    add->isDir = false;
                    add->distance = each["distance"].toDouble()/1000.0;
                    add->duration = each["elapsed_time"].toInt();
                    add->name = QDateTime::fromString(each["start_date_local"].toString(), Qt::ISODate).toString("yyyy_MM_dd_HH_mm_ss")+".json";

                    printd("direntry: %s %s\n", add->id.toStdString().c_str(), add->name.toStdString().c_str());

                    returning << add;
                }
                // next page
                offset += pageSize;
            } else
                offset = INT_MAX;

        } else {
            // we had a parsing error - so something is wrong - stop requesting more data by ending the loop
            offset = INT_MAX;
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

    QString filename = QFileInfo(remotename).baseName();

    QHttpPart activityNamePart;
    activityNamePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"name\""));

    // use metadata config if the user selected it
    QString fieldname = getSetting(GC_STRAVA_ACTIVITY_NAME, QVariant("")).toString();
    QString activityName = "";
    if (fieldname != "") activityName = ride->getTag(fieldname, "");
    activityNamePart.setBody(activityName.toLatin1());

    QHttpPart dataTypePart;
    dataTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"data_type\""));
    dataTypePart.setBody("tcx.gz");

    QHttpPart externalIdPart;
    externalIdPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"external_id\""));
    externalIdPart.setBody(filename.toStdString().c_str());

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
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\""+remotename+"\"; type=\"text/xml\""));
    filePart.setBody(data);

    multiPart->append(accessTokenPart);
    multiPart->append(activityTypePart);
    if (activityName != "") {
        multiPart->append(activityNamePart);
    }
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

    bool uploadSuccessful = false;
    QString response = reply->readLine();
    QString uploadError="invalid response or parser error";

    printd("reply:%s\n", response.toStdString().c_str());

    try {

        // parse !
        MVJSONReader jsonResponse(response.toStdString());

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
Strava::addSamples(RideFile* ret, QString remoteid)
{
    printd("Strava::addSamples(%s)\n", remoteid.toStdString().c_str());

    // do we have a token ?
    QString token = getSetting(GC_STRAVA_TOKEN, "").toString();
    if (token == "") return;

    // lets connect and get basic info on the root directory
    QString streamsList = "time,latlng,distance,altitude,velocity_smooth,heartrate,cadence,watts,temp";
    QString urlstr = QString("https://www.strava.com/api/v3/activities/%1/streams/%2")
          .arg(remoteid).arg(streamsList);

    QUrl url = QUrl( urlstr );
    printd("url:%s\n", url.url().toStdString().c_str());

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
        return;
    }
    // did we get a good response ?
    QByteArray r = reply->readAll();
    printd("response: %s\n", r.toStdString().c_str());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {
        // Streams is the Strava term for the raw data associated with an activity.
        // All streams for a given activity or segment effort will be the same length
        // and the values at a given index correspond to the same time.

        QJsonArray streams = document.array();

        // Stream types
        // ==============
        // Streams are available in 11 different types. If the stream is not available for a particular activity it will be left out of the request results.

        // time:	integer seconds
        // latlng:	floats [latitude, longitude]
        // distance:	float meters
        // altitude:	float meters
        // velocity_smooth:	float meters per second
        // heartrate:	integer BPM
        // cadence:	integer RPM
        // watts:	integer watts
        // temp:	integer degrees Celsius
        // moving:	boolean
        // grade_smooth:	float percent

        // for mapping from the names used in the Strava json response
        // to the series names we use in GC
        static struct {
            RideFile::seriestype type;
            const char *stravaname;
            double factor;                  // to convert from Strava units to GC units
        } seriesnames[] = {

            // seriestype          strava name           conversion factor
            { RideFile::secs,      "time",                         1.0f   },
            { RideFile::lat,       "latlng",                       1.0f   },
            { RideFile::alt,       "altitude",                     1.0f   },
            { RideFile::km,        "distance" ,                    0.001f },
            { RideFile::kph,       "velocity_smooth",              3.6f   },
            { RideFile::hr,        "heartrate",                    1.0f   },
            { RideFile::cad,       "cadence",                      1.0f   },
            { RideFile::watts,     "watts",                        1.0f   },
            { RideFile::temp,      "temp",                         1.0f   },
            { RideFile::none,      "",                             0.0f   }

        };

        // data to combine into a new ride
        class strava_stream {
        public:
            double factor; // for converting
            RideFile::seriestype type;
            QJsonArray samples;
        };

        // create a list of all the data we will work with
        QList<strava_stream> data;

        // examine the returned object and extract sample data
        for(int i=0; i<streams.size(); i++) {
            QJsonObject stream = streams.at(i).toObject();
            QString type = stream["type"].toString();

            for(int j=0; seriesnames[j].type != RideFile::none; j++) {
                QString name = seriesnames[j].stravaname;
                strava_stream add;

                // contained?
                if (type == name) {
                    add.factor = seriesnames[j].factor;
                    add.type = seriesnames[j].type;
                    add.samples = stream["data"].toArray();

                    data << add;
                    break;
                }
            }
        }

        bool end = false;
        int index=0;
        do {
            RideFilePoint add;

            // move through streams if they're waiting for this point
            foreach(strava_stream stream, data) {

                // if this stream still has data to consume
                if (index < stream.samples.count()) {

                    if (stream.type == RideFile::lat) {

                        double lat = stream.factor * stream.samples.at(index).toArray().at(0).toDouble();
                        double lon = stream.factor * stream.samples.at(index).toArray().at(1).toDouble();

                        // this is one for us, update and move on
                        add.setValue(RideFile::lat, lat);
                        add.setValue(RideFile::lon, lon);

                    } else {

                        // hr, distance et al
                        double value = stream.factor * stream.samples.at(index).toDouble();

                        // this is one for us, update and move on
                        add.setValue(stream.type, value);
                    }

                } else
                    end = true;
            }

            ret->appendPoint(add);
            index++;

        } while (!end);
    }
}

void
Strava:: fixLapSwim(RideFile* ret, QJsonArray laps)
{
    QVariant isGarminSmartRecording = appsettings->value(NULL, GC_GARMIN_SMARTRECORD,Qt::Checked);

    // Lap Swim & SmartRecording enabled: use distance from laps to fix samples
    if (isGarminSmartRecording.toInt() != 0 && ret->isSwim() &&
        ret->isDataPresent(RideFile::km) &&
        !ret->isDataPresent(RideFile::lat)) {

        int lastSecs = 0;
        double lastDist = 0.0;
        for (int i=0; i<laps.count() && i<ret->intervals().count(); i++) {
            QJsonObject lap = laps[i].toObject();
            double start = ret->intervals()[i]->start;
            double end = ret->intervals()[i]->stop;
            int startIndex = ret->timeIndex(start) ? ret->timeIndex(start)-1 : 0;
            double km = ret->getPointValue(startIndex, RideFile::km);

            // fill gaps and fix distance before the lap
            double deltaDist = (start>lastSecs && km>lastDist+0.001) ? (km-lastDist)/(start-lastSecs) : 0;
            double kph = 3600.0*deltaDist;
            for (int secs=lastSecs; secs<start; secs++) {
                ret->appendOrUpdatePoint(secs, 0.0, 0.0, lastDist,
                        kph, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        RideFile::NA, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0, false);
                // force update, even when secs or kph are 0
                if (secs == 0 || kph == 0.0)
                    ret->setPointValue(secs, RideFile::kph, kph);
                lastDist += deltaDist;
            }

            // fill gaps and fix distance inside the lap
            deltaDist = 0.001*lap["distance"].toDouble()/(end-start);
            kph = 3600.0*deltaDist;
            for (int secs=start; secs<end; secs++) {
                ret->appendOrUpdatePoint(secs, 0.0, 0.0, lastDist,
                        kph, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        RideFile::NA, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0, false);
                // force update, even when secs or kph are 0
                if (secs == 0 || kph == 0.0)
                    ret->setPointValue(secs, RideFile::kph, kph);
                lastDist += deltaDist;
            }
            lastSecs = end;
        }
    }
}

void
Strava:: fixSmartRecording(RideFile* ret)
{
    QVariant isGarminSmartRecording = appsettings->value(NULL, GC_GARMIN_SMARTRECORD,Qt::Checked);
    // do nothing if disabled
    if (isGarminSmartRecording.toInt() == 0) return;

    QVariant GarminHWM = appsettings->value(NULL, GC_GARMIN_HWMARK);
    // default to 30 seconds
    if (GarminHWM.isNull() || GarminHWM.toInt() == 0) GarminHWM.setValue(30);
    // minimum 90 seconds for swims
    if (ret->isSwim() && GarminHWM.toInt()<90) GarminHWM.setValue(90);

    // The following fragment was adapted from FixGaps

    // If there are less than 2 dataPoints then there
    // is no way of post processing anyway (e.g. manual workouts)
    if (ret->dataPoints().count() < 2) return;

    // OK, so there are probably some gaps, lets post process them
    RideFilePoint *last = NULL;
    int dropouts = 0;
    double dropouttime = 0.0;

    for (int position = 0; position < ret->dataPoints().count(); position++) {
        RideFilePoint *point = ret->dataPoints()[position];

        if (NULL != last) {
            double gap = point->secs - last->secs - ret->recIntSecs();

            // if we have gps and we moved, then this isn't a stop
            bool stationary = ((last->lat || last->lon) && (point->lat || point->lon)) // gps is present
                         && last->lat == point->lat && last->lon == point->lon;

            // moved for less than GarminHWM seconds ... interpolate
            if (!stationary && gap >= 1 && gap <= GarminHWM.toInt()) {

                // what's needed?
                dropouts++;
                dropouttime += gap;

                int count = gap/ret->recIntSecs();
                double hrdelta = (point->hr - last->hr) / (double) count;
                double pwrdelta = (point->watts - last->watts) / (double) count;
                double kphdelta = (point->kph - last->kph) / (double) count;
                double kmdelta = (point->km - last->km) / (double) count;
                double caddelta = (point->cad - last->cad) / (double) count;
                double altdelta = (point->alt - last->alt) / (double) count;
                double nmdelta = (point->nm - last->nm) / (double) count;
                double londelta = (point->lon - last->lon) / (double) count;
                double latdelta = (point->lat - last->lat) / (double) count;
                double hwdelta = (point->headwind - last->headwind) / (double) count;
                double slopedelta = (point->slope - last->slope) / (double) count;
                double temperaturedelta = (point->temp - last->temp) / (double) count;
                double lrbalancedelta = (point->lrbalance - last->lrbalance) / (double) count;
                double ltedelta = (point->lte - last->lte) / (double) count;
                double rtedelta = (point->rte - last->rte) / (double) count;
                double lpsdelta = (point->lps - last->lps) / (double) count;
                double rpsdelta = (point->rps - last->rps) / (double) count;
                double lpcodelta = (point->lpco - last->lpco) / (double) count;
                double rpcodelta = (point->rpco - last->rpco) / (double) count;
                double lppbdelta = (point->lppb - last->lppb) / (double) count;
                double rppbdelta = (point->rppb - last->rppb) / (double) count;
                double lppedelta = (point->lppe - last->lppe) / (double) count;
                double rppedelta = (point->rppe - last->rppe) / (double) count;
                double lpppbdelta = (point->lpppb - last->lpppb) / (double) count;
                double rpppbdelta = (point->rpppb - last->rpppb) / (double) count;
                double lpppedelta = (point->lpppe - last->lpppe) / (double) count;
                double rpppedelta = (point->rpppe - last->rpppe) / (double) count;
                double smo2delta = (point->smo2 - last->smo2) / (double) count;
                double thbdelta = (point->thb - last->thb) / (double) count;
                double rcontactdelta = (point->rcontact - last->rcontact) / (double) count;
                double rcaddelta = (point->rcad - last->rcad) / (double) count;
                double rvertdelta = (point->rvert - last->rvert) / (double) count;
                double tcoredelta = (point->tcore - last->tcore) / (double) count;

                // add the points
                for(int i=0; i<count; i++) {
                    RideFilePoint *add = new RideFilePoint(last->secs+((i+1)*ret->recIntSecs()),
                                                           last->cad+((i+1)*caddelta),
                                                           last->hr + ((i+1)*hrdelta),
                                                           last->km + ((i+1)*kmdelta),
                                                           last->kph + ((i+1)*kphdelta),
                                                           last->nm + ((i+1)*nmdelta),
                                                           last->watts + ((i+1)*pwrdelta),
                                                           last->alt + ((i+1)*altdelta),
                                                           last->lon + ((i+1)*londelta),
                                                           last->lat + ((i+1)*latdelta),
                                                           last->headwind + ((i+1)*hwdelta),
                                                           last->slope + ((i+1)*slopedelta),
                                                           last->temp + ((i+1)*temperaturedelta),
                                                           last->lrbalance + ((i+1)*lrbalancedelta),
                                                           last->lte + ((i+1)*ltedelta),
                                                           last->rte + ((i+1)*rtedelta),
                                                           last->lps + ((i+1)*lpsdelta),
                                                           last->rps + ((i+1)*rpsdelta),
                                                           last->lpco + ((i+1)*lpcodelta),
                                                           last->rpco + ((i+1)*rpcodelta),
                                                           last->lppb + ((i+1)*lppbdelta),
                                                           last->rppb + ((i+1)*rppbdelta),
                                                           last->lppe + ((i+1)*lppedelta),
                                                           last->rppe + ((i+1)*rppedelta),
                                                           last->lpppb + ((i+1)*lpppbdelta),
                                                           last->rpppb + ((i+1)*rpppbdelta),
                                                           last->lpppe + ((i+1)*lpppedelta),
                                                           last->rpppe + ((i+1)*rpppedelta),
                                                           last->smo2 + ((i+1)*smo2delta),
                                                           last->thb + ((i+1)*thbdelta),
                                                           last->rvert + ((i+1)*rvertdelta),
                                                           last->rcad + ((i+1)*rcaddelta),
                                                           last->rcontact + ((i+1)*rcontactdelta),
                                                           last->tcore + ((i+1)*tcoredelta),
                                                           last->interval);

                    ret->insertPoint(position++, add);
                }

            }
        }
        last = point;
    }
}

QByteArray*
Strava::prepareResponse(QByteArray* data)
{
    printd("Strava::prepareResponse()\n");

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(data->constData(), &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {
        QJsonObject each = document.object();

        QDateTime starttime = QDateTime::fromString(each["start_date_local"].toString(), Qt::ISODate);

        // 1s samples with start time
        RideFile *ride = new RideFile(starttime.toUTC(), 1.0f);

        // what sport?
        if (!each["type"].isNull()) {
            QString stype = each["type"].toString();
            if (stype == "Ride") ride->setTag("Sport", "Bike");
            else if (stype == "Run") ride->setTag("Sport", "Run");
            else if (stype == "Swim") ride->setTag("Sport", "Swim");
            else ride->setTag("Sport", stype);
        }

        if (each["device_name"].toString().length()>0)
            ride->setDeviceType(each["device_name"].toString());
        else
            ride->setDeviceType("Strava"); // The device type is unknown

        // activity name save to configured meta field
        if (!each["name"].isNull()) {
            QString meta = getSetting(GC_STRAVA_ACTIVITY_NAME, QVariant("")).toString();
            if (meta != "") ride->setTag(meta, each["name"].toString());
        }

        if (each["manual"].toBool()) {
            if (each["distance"].toDouble()>0) {
                QMap<QString,QString> map;
                map.insert("value", QString("%1").arg(each["distance"].toDouble()/1000.0));
                ride->metricOverrides.insert("total_distance", map);
            }
            if (each["moving_time"].toDouble()>0) {
                QMap<QString,QString> map;
                map.insert("value", QString("%1").arg(each["moving_time"].toDouble()));
                ride->metricOverrides.insert("time_riding", map);
            }
            if (each["elapsed_time"].toDouble()>0) {
                QMap<QString,QString> map;
                map.insert("value", QString("%1").arg(each["elapsed_time"].toDouble()));
                ride->metricOverrides.insert("workout_time", map);
            }
            if (each["total_elevation_gain"].toDouble()>0) {
                QMap<QString,QString> map;
                map.insert("value", QString("%1").arg(each["total_elevation_gain"].toDouble()));
                ride->metricOverrides.insert("elevation_gain", map);
            }

        } else {
            addSamples(ride, QString("%1").arg(each["id"].toVariant().toULongLong()));
            // laps?
            if (!each["laps"].isNull()) {
                QJsonArray laps = each["laps"].toArray();

                foreach (QJsonValue value, laps) {
                    QJsonObject lap = value.toObject();
                    double start = ride->getPoint(lap["start_index"].toInt(), RideFile::secs).toDouble();
                    double end = start + lap["elapsed_time"].toDouble();
                    ride->addInterval(RideFileInterval::USER, start, end, lap["name"].toString());
                }

                // Fix distance from laps and fill gaps for pool swims
                fixLapSwim(ride, laps);
            }
            fixSmartRecording(ride);
        }



        JsonFileReader reader;
        data->clear();
        data->append(reader.toByteArray(context, ride, true, true, true, true));

        // temp ride not needed anymore
        delete ride;

        printd("reply:%s\n", data->toStdString().c_str());
    }
    return data;
}

static bool addStrava() {
    CloudServiceFactory::instance().addService(new Strava(NULL));
    return true;
}

static bool add = addStrava();

