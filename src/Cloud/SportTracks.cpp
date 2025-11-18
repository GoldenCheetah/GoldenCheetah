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
#include "RideFile.h"
#include "JsonRideFile.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

Q_DECLARE_LOGGING_CATEGORY(gcSportTracks)
Q_LOGGING_CATEGORY(gcSportTracks, "gc.sporttracks")

#ifdef Q_CC_MSVC
#define printd(fmt, ...) qCDebug(gcSportTracks, fmt, __VA_ARGS__);
#else
#define printd(fmt, args...) qCDebug(gcSportTracks, fmt, ##args);
#endif

SportTracks::SportTracks(Context *context) : CloudService(context), context(context), root_(NULL) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = none; // gzip
    downloadCompression = none;
    filetype = TCX; // fit loses texts
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
SportTracks::onSslErrors(QNetworkReply *reply, const QList<QSslError>&errors)
{
    CloudDBCommon::sslErrors(reply, errors);
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
        printd("HTTP response code: %d\n", statusCode);
        if (reply->error() != 0) printd("fetch response: %d: %s\n", reply->error(), reply->errorString().toStdString().c_str());

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
    printd("returning count(%lld), errors(%s)\n", returning.count(), errors.join(",").toStdString().c_str());
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
    QByteArray *returning = buffers.value(reply);

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
        // laps	array	An array of Laps ** ignored for now
        // timer_stops	array	An array of Timer Stops ** ignored for now

        QJsonObject ride = document.object();
        QDateTime starttime = QDateTime::fromString(ride["start_time"].toString(), Qt::ISODate);

        // 1s samples with start time as UTC
        RideFile *ret = new RideFile(starttime.toUTC(), 1.0f);

        // what sport?
        if (!ride["type"].isNull()) {
            QString stype = ride["type"].toString();
            if (stype == "Cycling") ret->setTag("Sport", "Bike");
            else if (stype == "Running") ret->setTag("Sport", "Run");
            else if (stype == "Swimming") ret->setTag("Sport", "Swim");
            else ret->setTag("Sport", stype);
        }

        // location => route
        if (!ride["name"].isNull()) ret->setTag("Objective", ride["name"].toString());
        if (!ride["notes"].isNull()) ret->setTag("Notes", ride["notes"].toString());
        if (!ride["location_name"].isNull()) ret->setTag("Route", ride["location_name"].toString());

        // SAMPLES DATA
        //
        // NOTE: there is no time data or speed data, the offsets are given in seconds
        //       so the data is downsampled if <1s and upsampled if >1s samples.
        //       its not clear how they handle older PT 1.26s data (if at all)
        //
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


        // for mapping from the names used in the SportTracks json response
        // to the series names we use in GC, note that lat covers lat and lon
        // so needs to be treated as a special case
        static struct {
            RideFile::seriestype type;
            const char *sporttracksname;
            double factor;                  // to convert from ST units to GC units
        } seriesnames[] = {

            // seriestype          sporttracks name           conversion factor
            { RideFile::lat,       "location",                     1.0f   },
            { RideFile::alt,       "elevation",                    1.0f   },
            { RideFile::km,        "distance" ,                    0.001f },
            { RideFile::hr,        "heartrate",                    1.0f   },
            { RideFile::cad,       "cadence",                      1.0f   },
            { RideFile::watts,     "power",                        1.0f   },
            { RideFile::temp,      "temperature",                  1.0f   },
            { RideFile::rvert,     "vertical_oscillation",         1.0f   },
            { RideFile::rcontact,  "ground_contact_time",          1.0f   },
            { RideFile::lrbalance, "left_power_percent",           1.0f   },
            { RideFile::thb,       "total_hemoglobin_conc",        1.0f   },
            { RideFile::smo2,      "saturated_hemoglobin_percent", 1.0f   },
            { RideFile::none,      "",                             0.0f   }

        };

        // data to combine into a new ride
        class st_track {
        public:
            int index; // for tracking progress as we move through it
            double factor; // for converting
            RideFile::seriestype type;
            QJsonArray samples;
        };

        // create a list of all the data we will work with
        QList<st_track> data;

        // examine the returned object and extract sample data
        for(int i=0; seriesnames[i].type != RideFile::none; i++) {
            QString name = seriesnames[i].sporttracksname;
            st_track add;

            // contained?
            if (ride[name].isArray()) {

                add.index = 0;
                add.factor = seriesnames[i].factor;
                add.type = seriesnames[i].type;
                add.samples = ride[name].toArray();

                data << add;
            }
        }

        // Add Laps
        if (ride["laps"].isArray()) {
            for(int i=0; i<ride["laps"].toArray().count(); i++) {
                QJsonObject lap = ride["laps"].toArray().at(i).toObject();
                QDateTime lap_starttime = QDateTime::fromString(lap["start_time"].toString(), Qt::ISODate);
                double duration = lap["duration"].toDouble();
                int number = lap["number"].toInt();

                double start = starttime.secsTo(lap_starttime);
                double stop = start + duration;

                ret->addInterval(RideFileInterval::DEVICE, start, stop, QString("#%1").arg(number));
            }
        }

        // we now work through the tracks combining them into samples
        // with a common offset (ie, by row, not column).
        int index=0;

        // for estimated speed
        int rollingwindowsize = 3;

        QVector<double> rolling(rollingwindowsize);

        int rolling_index = 0;
        double rolling_sum = 0;
        double rolling_secs = 0.0;
        double rolling_km = 0.0;

        do {

            // We stop when all tracks have been accomodated
            bool stop=true;
            foreach(st_track track, data) if (track.index < track.samples.count() && track.samples.at(track.samples.count()-2).toInt() >= index) stop=false;
            if (stop) break;

            // add new point for the point in time we are at
            RideFilePoint add;
            add.secs = index;

            // move through tracks if they're waiting for this point
            bool updated=false;
            for(int t=0; t<data.count(); t++) {

                // if this track still has data to consume
                if (data[t].index < data[t].samples.count()) {

                    // get the offset for the current sample in the current track
                    int ct = data[t].samples.at(data[t].index).toInt();

                    // if data missing we have to repeat last sample
                    if (ct > index) {
                        data[t].index = data[t].index - 2;
                    }
                    if (ct >= index) {

                        if (data[t].type == RideFile::lat) {

                            // hr, distance et al
                            double lat = data[t].factor * data[t].samples.at(data[t].index+1).toArray().at(0).toDouble();
                            double lon = data[t].factor * data[t].samples.at(data[t].index+1).toArray().at(1).toDouble();

                            // this is one for us, update and move on
                            add.setValue(RideFile::lat, lat);
                            add.setValue(RideFile::lon, lon);
                            data[t].index = data[t].index + 2; // pairs

                        } else {

                            // hr, distance et al
                            double value = data[t].factor * data[t].samples.at(data[t].index+1).toDouble();

                            // this is one for us, update and move on
                            add.setValue(data[t].type, value);
                            data[t].index = data[t].index + 2; // pairs
                        }
                        updated=true;
                    }
                }
            }

            // Estimate Speed from travelled distance
            double kph = 0.0;
            if (add.secs - rolling_secs > 0) kph = 3600 * (add.km - rolling_km) / (add.secs - rolling_secs);
            //if (kph>100)
            //    kph = 100;

            // compute rolling average for rollingwindowsize seconds
            rolling_sum += kph;
            rolling_sum -= rolling[rolling_index];
            rolling[rolling_index] = kph;
            kph = rolling_sum/std::min(index+1, rollingwindowsize);
            // move index on/round
            rolling_index = (rolling_index >= rollingwindowsize-1) ? 0 : rolling_index+1;
            add.kph = kph;
            // update accumulated time and distance
            rolling_secs = add.secs;
            rolling_km = add.km;

            // don't add blanks
            if (updated) ret->appendPoint(add);
            index++;

        } while (true);

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
SportTracks::writeFile(QByteArray &data, QString remotename, RideFile *)
{
    printd("SportTracks::writeFile(%s)\n", remotename.toStdString().c_str());

    // do we have a token ?
    QString token = getSetting(GC_SPORTTRACKS_TOKEN, "").toString();
    if (token == "") return false;

    // set the target url
    QString url = QString("https://api.sporttracks.mobi/api/v2/fileUpload");
    //QString url = QString("http://requestb.in/1hfhjkx1");
    printd("endpoint: '%s'\n", url.toStdString().c_str());

    // create a request passing the session token and user
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery params;

    params.addQueryItem("format", "TCX");
    params.addQueryItem("data", data); // I know, this is really how it gets posted !

    QNetworkReply *reply = nam->post(request, params.query(QUrl::FullyEncoded).toUtf8());

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply,remotename);

    return true;
}

void
SportTracks::writeFileCompleted()
{
    printd("SportTracks::writeFileCompleted()\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    QString name = replyName(static_cast<QNetworkReply*>(QObject::sender()));

    QByteArray r = reply->readAll();
    printd("reply:%s\n", r.toStdString().c_str());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    if (reply->error() == QNetworkReply::NoError) {

        //QJsonObject result = document.object();
        notifyWriteComplete( name, tr("Completed."));

    } else {

        notifyWriteComplete( replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Network Error - Upload failed."));
    }
}

// development put on hold - AccessLink API compatibility issues with Desktop applications
static bool addSportTracks() {
    CloudServiceFactory::instance().addService(new SportTracks(NULL));
    return true;
}

static bool add = addSportTracks();
