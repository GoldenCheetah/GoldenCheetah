/*
 * Copyright (c) 2016 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#include "TodaysPlan.h"
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

#ifndef TODAYSPLAN_DEBUG
// TODO(gille): This should be a command line flag.
#define TODAYSPLAN_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (TODAYSPLAN_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (TODAYSPLAN_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

TodaysPlan::TodaysPlan(Context *context) : CloudService(context), context(context), root_(NULL) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = gzip; // gzip
    downloadCompression = none;
    useMetric = true; // distance and duration metadata

    // config
    settings.insert(OAuthToken, GC_TODAYSPLAN_TOKEN);
    settings.insert(URL, GC_TODAYSPLAN_URL);
    settings.insert(DefaultURL, "https://whats.todaysplan.com.au");
    settings.insert(Key, GC_TODAYSPLAN_USERKEY);
    settings.insert(AthleteID, GC_TODAYSPLAN_ATHLETE_ID);
    settings.insert(Local1, GC_TODAYSPLAN_ATHLETE_NAME);
}

TodaysPlan::~TodaysPlan() {
    if (context) delete nam;
}

void
TodaysPlan::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

// open by connecting and getting a basic list of folders available
bool
TodaysPlan::open(QStringList &errors)
{
    printd("TodaysPlan::open\n");

    // do we have a token
    QString token = getSetting(GC_TODAYSPLAN_TOKEN, "").toString();
    if (token == "") {
        errors << "You must authorise with TodaysPlan first";
        return false;
    }

    // use the configed URL
    QString url = QString("%1/rest/users/delegates/users")
          .arg(getSetting(GC_TODAYSPLAN_URL, "https://whats.todaysplan.com.au").toString());

    printd("URL used: %s\n", url.toStdString().c_str());

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
        errors << tr("Network Problem reading TodaysPlan data");
        return false;
    }
    // did we get a good response ?
    QByteArray r = reply->readAll();
    printd("response: %s\n", r.toStdString().c_str());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {
        printd("NoError");

        userId = getSetting(GC_TODAYSPLAN_ATHLETE_ID, "").toString();

        if (document.array().count()>1) {
            if (userId.length()==0) {
                errors << tr("Please re-authorise and select an athlete");
            }
            else {
                bool found = false;
                for (int i=0;i<document.array().count();i++) {
                    if (document.array()[i].toObject()["id"].toInt() == userId.toInt()) {
                        if (document.array()[i].toObject()["relationship"].toString() == "" ||
                            document.array()[i].toObject()["relationship"].toString() == "coach" ||
                            document.array()[i].toObject()["relationship"].toString() == "manager")
                            found = true;
                    }
                }
                if (!found)
                    errors << tr("It seems you can't access anymore to this athlete. Please re-authorise.");
            }
        }
        // we have a root
        root_ = newCloudServiceEntry();

        // path name
        root_->name = "/";
        root_->isDir = true;
        root_->size = 0;
    } else {
        errors << tr("problem parsing TodaysPlan data");
    }

    // ok so far ?
    if (errors.count()) return false;
    return true;
}

bool
TodaysPlan::close()
{
    printd("TodaysPlan::close\n");
    // nothing to do for now
    return true;
}

// home dire
QString
TodaysPlan::home()
{
    return "";
}

bool
TodaysPlan::createFolder(QString)
{
    printd("TodaysPlan::createFolder\n");

    return false;
}

QList<CloudServiceEntry*>
TodaysPlan::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    printd("TodaysPlan::readdir(%s)\n", path.toStdString().c_str());

    QList<CloudServiceEntry*> returning;

    // do we have a token
    QString token = getSetting(GC_TODAYSPLAN_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with Today's Plan first");
        return returning;
    }

    // Do Paginated Access to the Activities List
    const int pageSize = 100;
    int offset = 0;
    int resultCount = INT_MAX;

    while (offset < resultCount) {

        QString url;
        QString searchCommand;
        if (offset == 0) {
            // fist call
            searchCommand = "search";
        } else {
            // subsequent pages
            searchCommand = "page";
        }

        url = QString("%1/rest/users/activities/%2/%3/%4")
                .arg(getSetting(GC_TODAYSPLAN_URL, "https://whats.todaysplan.com.au").toString())
                .arg(searchCommand)
                .arg(QString::number(offset))
                .arg(QString::number(pageSize));

        printd("URL used: %s\n", url.toStdString().c_str());

        // request using the bearer token
        QNetworkRequest request(url);
        QNetworkReply *reply;
        request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
        request.setRawHeader("tp-nodecorate", "true"); // without fields description
        if (offset == 0) {

            // Prepare the Search Payload for First Call to Search
            QString userId = getSetting(GC_TODAYSPLAN_ATHLETE_ID, "").toString();
            // application/json
            QByteArray jsonString;
            jsonString += "{\"criteria\": {";
            if (userId.length()>0)
                jsonString += "\"user\": "+ QString("%1").arg(userId) +", ";
            jsonString += "\"fromTs\": \""+ QString("%1").arg(from.toMSecsSinceEpoch()) +"\", ";
            jsonString += "\"toTs\": \"" + QString("%1").arg(to.addDays(1).addSecs(-1).toMSecsSinceEpoch()) + "\", ";
            jsonString += "\"isNotNull\": [\"fileId\"], ";
            jsonString += "\"sports\": [\"ride\",\"swim\",\"run\"] ";
            jsonString += "}, "; // end of "criteria"
            jsonString += "\"fields\": [\"fileId\",\"name\",\"fileindex.id\",\"distance\",\"startTs\",\"training\", \"type\", \"rpe\",\"tqr\",\"pain\"], ";
            jsonString += "\"opts\": 1 "; // without fields description
            jsonString += "}";

            printd("request: %s\n", jsonString.toStdString().c_str());

            QByteArray jsonStringDataSize = QByteArray::number(jsonString.size());

            request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
            request.setRawHeader("Content-Length", jsonStringDataSize);
            reply = nam->post(request, jsonString);
        } else {
            // get further pages of the Search
            reply = nam->get(request);
        }

        // blocking request
        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        // did we get a good response ?
        QByteArray r = reply->readAll();
        printd("response: %s\n", r.toStdString().c_str());

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        // if path was returned all is good, lets set root
        if (parseError.error == QJsonParseError::NoError) {

            // number of Result Items
            if (offset == 0) {
                resultCount = document.object()["cnt"].toInt();
            }

            // results ?
            QJsonObject result = document.object()["result"].toObject();
            QJsonArray results = result["results"].toArray();

            // lets look at that then
            for(int i=0; i<results.size(); i++) {
                QJsonObject each = results.at(i).toObject();
                CloudServiceEntry *add = newCloudServiceEntry();

                // file details
                QJsonObject fileindex = each["fileindex"].toObject();
                QString suffix = QFileInfo(fileindex["filename"].toString()).suffix();
                if (suffix == "") {
                    // Zwift uploads files without an extension - work ongoing to get Zwift fixed
                    if (fileindex["filename"].toString().startsWith("zwift-activity-")) {
                        qDebug() << "Correcting Zwift Activity extension: " << fileindex["filename"].toString();
                        suffix = "fit";
                    } else {
                        suffix = "json";
                    }
                }

                //TodaysPlan's Label may contain the FileName, or Descriptive Text (whatever is shown/edited on the TP's UI)
                add->label = QFileInfo(each["name"].toString()).fileName();
                add->id = QString("%1").arg(each["fileId"].toInt());
                add->isDir = false;
                add->distance = each["distance"].toDouble()/1000.0;
                add->duration = each["training"].toInt();
                add->name = QDateTime::fromMSecsSinceEpoch(each["startTs"].toDouble()).toString("yyyy_MM_dd_HH_mm_ss")+"."+suffix;

                // only our own name is a reliable key
                replyActivity.insert(add->name, each);

                //add->size
                //add->modified

                //QJsonObject fileindex = each["fileindex"].toObject();
                //add->name = QFileInfo(fileindex["filename"].toString()).fileName();

                printd("direntry: %s %s\n", add->id.toStdString().c_str(), add->name.toStdString().c_str());

                returning << add;
            }
            // next page
            offset += pageSize;
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
TodaysPlan::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("TodaysPlan::readFile(%s)\n", remotename.toStdString().c_str());

    // this must be performed asyncronously and call made
    // to notifyReadComplete(QByteArray &data, QString remotename, QString message) when done

    // do we have a token ?
    QString token = getSetting(GC_TODAYSPLAN_TOKEN, "").toString();
    if (token == "") return false;

    // lets connect and get basic info on the root directory
    QString url = QString("%1/rest/files/download/%2")
          .arg(getSetting(GC_TODAYSPLAN_URL, "https://whats.todaysplan.com.au").toString())
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
TodaysPlan::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    printd("TodaysPlan::writeFile(%s)\n", remotename.toStdString().c_str());

    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done

    // do we have a token ?
    QString token = getSetting(GC_TODAYSPLAN_TOKEN, "").toString();
    if (token == "") return false;

    // lets connect and get basic info on the root directory
    QString url = QString("%1/rest/files/upload")
          .arg(getSetting(GC_TODAYSPLAN_URL, "https://whats.todaysplan.com.au").toString());

    printd("URL used: %s\n", url.toStdString().c_str());

    QNetworkRequest request = QNetworkRequest(url);

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QString boundary = QVariant(qrand()).toString()+QVariant(qrand()).toString()+QVariant(qrand()).toString();
    multiPart->setBoundary(boundary.toLatin1());

    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    QHttpPart jsonPart;
    jsonPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"json\""));

    QString userId = getSetting(GC_TODAYSPLAN_ATHLETE_ID, "").toString();

    QString json = QString("{ filename: \"%1\"").arg(remotename);

    if (userId.length()>0) {
        json  += QString(", userId: %1").arg(userId);
    }
    // RPE, LQS and TQR
    double rpe = ride->getTag("RPE", "0.0").toDouble();
    if (rpe > 0.0) {
        json  += QString(", rpe: %1").arg(rpe);
    }
    double tqr = ride->getTag("TQR", "0.0").toDouble();
    if (tqr > 0.0) {
        json  += QString(", tqr: %1").arg(tqr);
    }
    double lqs = ride->getTag("LQS", "0.0").toDouble();
    if (lqs > 0.0) {
        json  += QString(", pain: %1").arg(lqs);
    }

    json  += " }";

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
    return true;
}

void
TodaysPlan::writeFileCompleted()
{
    printd("TodaysPlan::writeFileCompleted()\n");

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

        notifyWriteComplete(
            name,
            tr("Completed."));
    } else {
        notifyWriteComplete(
            replyName(static_cast<QNetworkReply*>(QObject::sender())),
            tr("Network Error - Upload failed."));
    }
}

void
TodaysPlan::readyRead()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

void
TodaysPlan::readFileCompleted()
{
    printd("TodaysPlan::readFileCompleted\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    // even in debug mode we don't want the whole thing...
    printd("reply:%s\n", buffers.value(reply)->mid(0,500).toStdString().c_str());

    // prepateResponse will rename the file if it converts to JSON
    // to add RPE data, so we need to spot name changes to notify
    // upstream that it did (e.g. FIT => JSON)
    QString rename = replyName(reply);
    QByteArray* data = prepareResponse(buffers.value(reply), rename);

    // notify complete with a rename
    notifyReadComplete(data, rename, tr("Completed."));
}

QList<CloudServiceAthlete>
TodaysPlan::listAthletes()
{
    QList<CloudServiceAthlete> returning;

    // use the configed URL
    QString url = QString("%1/rest/users/delegates/users").arg(getSetting(GC_TODAYSPLAN_URL, "https://whats.todaysplan.com.au").toString());

    // request using the bearer token
    QNetworkRequest request(url);
    QString token = getSetting(GC_TODAYSPLAN_TOKEN, "").toString();
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    QNetworkReply *reply = nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // did we get a good response ?
    QByteArray r = reply->readAll();

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);


    if (parseError.error == QJsonParseError::NoError) {
        if (document.array().count()>0) {

            for (int i=0;i<document.array().count();i++) {
                //qDebug() << document.array()[i].toObject()["_name"].toString() << document.array()[i].toObject()["relationship"].toString();
                if (document.array()[i].toObject()["relationship"].toString() == "" ||
                    document.array()[i].toObject()["relationship"].toString() == "coach" ||
                    document.array()[i].toObject()["relationship"].toString() == "manager") {

                    CloudServiceAthlete add;
                    add.id = QString("%1").arg(document.array()[i].toObject()["id"].toInt());
                    add.name = document.array()[i].toObject()["_name"].toString();
                    QString rel = document.array()[i].toObject()["relationship"].toString();
                    if (rel=="") rel="athlete";
                    add.desc = QString("relationship: %1").arg(rel);
                    returning << add;
                }
            }
        }
    }
    return returning;
}

bool
TodaysPlan::selectAthlete(CloudServiceAthlete athlete)
{
    // extract athlete name and identifier from the selected athlete
    // TODO
    setSetting(GC_TODAYSPLAN_ATHLETE_ID, athlete.id.toInt());
    setSetting(GC_TODAYSPLAN_ATHLETE_NAME, athlete.name);
    return true;
}

QByteArray*
TodaysPlan::prepareResponse(QByteArray* data, QString &name)
{
    printd("TodaysPlan::prepareResponse()\n");

    // uncompress and parse
    QStringList errors;
    RideFile *ride = uncompressRide(data, name, errors);

    if (ride) {
        QJsonObject activity = replyActivity.value(name, QJsonObject());
        if (activity["rpe"].isDouble()) {
            QString rpe = QString("%1").arg(activity["rpe"].toDouble());
            ride->setTag("RPE", rpe);
        }
        if (activity["tqr"].isDouble()) {
            QString tqr = QString("%1").arg(activity["tqr"].toDouble());
            ride->setTag("TQR", tqr);
        }
        if (activity["pain"].isDouble()) {
            QString lqs = QString("%1").arg(activity["pain"].toDouble());
            ride->setTag("LQS", lqs);
        }

        // convert
        JsonFileReader reader;
        data->clear();
        data->append(reader.toByteArray(context, ride, true, true, true, true));

        // rename
        if (QFileInfo(name).suffix() != "json") name = QFileInfo(name).baseName() + ".json";
    }

    return data;
}

static bool addTodaysPlan() {
    CloudServiceFactory::instance().addService(new TodaysPlan(NULL));
    return true;
}

static bool add = addTodaysPlan();

