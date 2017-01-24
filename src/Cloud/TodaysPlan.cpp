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

TodaysPlan::TodaysPlan(Context *context) : FileStore(context), context(context), root_(NULL) {
    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));

    uploadCompression = gzip; // gzip
    downloadCompression = none;

    useMetric = true; // distance and duration metadata
}

TodaysPlan::~TodaysPlan() {
    delete nam;
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
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_TODAYSPLAN_TOKEN, "").toString();
    if (token == "") {
        errors << "You must authorise with TodaysPlan first";
        return false;
    }

    // use the configed URL
    QString url = QString("%1/rest/users/delegates/users")
          .arg(appsettings->cvalue(context->athlete->cyclist, GC_TODAYSPLAN_URL, "https://whats.todaysplan.com.au").toString());

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
        // we have a root
        root_ = newFileStoreEntry();

        // path name
        root_->name = "/";
        root_->isDir = true;
        root_->size = 0;

        // happy with what we got ?
        if (root_->name != "/") errors << tr("invalid root path.");
        if (root_->isDir != true) errors << tr("root is not a directory.");

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

QList<FileStoreEntry*>
TodaysPlan::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    printd("TodaysPlan::readdir(%s)\n", path.toStdString().c_str());

    QList<FileStoreEntry*> returning;

    // do we have a token
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_TODAYSPLAN_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with Today's Plan first");
        return returning;
    }

    // lets connect and get activities list
    // old API ?
    // QString url("https://whats.todaysplan.com.au/rest/files/search/0/100");
    QString url = QString("%1/rest/users/activities/search/0/100")
          .arg(appsettings->cvalue(context->athlete->cyclist, GC_TODAYSPLAN_URL, "https://whats.todaysplan.com.au").toString());

    printd("URL used: %s\n", url.toStdString().c_str());


    //url="https://staging.todaysplan.com.au/rest/files/search/0/100";

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    // application/json
    QByteArray jsonString;
    jsonString += "{\"criteria\": {";
    jsonString += "\"fromTs\": \""+ QString("%1").arg(from.toMSecsSinceEpoch()) +"\", ";
    jsonString += "\"toTs\": \"" + QString("%1").arg(to.addDays(1).addSecs(-1).toMSecsSinceEpoch()) + "\", ";
    jsonString += "\"isNotNull\": [\"fileId\"]}, ";
    jsonString += "\"fields\": [\"fileId\",\"name\",\"fileindex.id\",\"distance\",\"training\"], "; //\"avgWatts\"
    jsonString += "\"opts\": 0 ";
    jsonString += "}";

    QByteArray jsonStringDataSize = QByteArray::number(jsonString.size());
    request.setRawHeader("Content-Length", jsonStringDataSize);

    QNetworkReply *reply = nam->post(request, jsonString);

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
        // results ?
        QJsonObject result = document.object()["result"].toObject();
        QJsonArray results = result["results"].toArray();

        // lets look at that then
        for(int i=0; i<results.size(); i++) {
            QJsonObject each = results.at(i).toObject();
            FileStoreEntry *add = newFileStoreEntry();

            //TodaysPlan has full path, we just want the file name
            add->label = QFileInfo(each["name"].toString()).fileName();
            add->id = QString("%1").arg(each["fileId"].toInt());
            add->isDir = false;
            add->distance = each["distance"].toInt()/1000.0;
            add->duration = each["training"].toInt();
            //add->size
            //add->modified

            //QString name = QDateTime::fromMSecsSinceEpoch(each["ts"].toDouble()).toString("yyyy_MM_dd_HH_mm_ss")+=".json";
            //add->name = name;
            QJsonObject fileindex = each["fileindex"].toObject();
            add->name = QFileInfo(fileindex["filename"].toString()).fileName();


            returning << add;
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
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_TODAYSPLAN_TOKEN, "").toString();
    if (token == "") return false;

    // lets connect and get basic info on the root directory
    QString url = QString("%1/rest/files/download/%2")
          .arg(appsettings->cvalue(context->athlete->cyclist, GC_TODAYSPLAN_URL, "https://whats.todaysplan.com.au").toString()).arg(remoteid);

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
TodaysPlan::writeFile(QByteArray &data, QString remotename)
{
    printd("TodaysPlan::writeFile(%s)\n", remotename.toStdString().c_str());

    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done

    // do we have a token ?
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_TODAYSPLAN_TOKEN, "").toString();
    if (token == "") return false;

    // lets connect and get basic info on the root directory
    QString url = QString("%1/rest/files/upload")
          .arg(appsettings->cvalue(context->athlete->cyclist, GC_TODAYSPLAN_URL, "https://whats.todaysplan.com.au").toString());

    printd("URL used: %s\n", url.toStdString().c_str());

    QNetworkRequest request = QNetworkRequest(url);

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QString boundary = QVariant(qrand()).toString()+QVariant(qrand()).toString()+QVariant(qrand()).toString();
    multiPart->setBoundary(boundary.toLatin1());

    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    QHttpPart jsonPart;
    jsonPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"json\""));
    QString json = QString("{ filename: \"%1\" }").arg(remotename);
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

    printd("reply:%s\n", buffers.value(reply)->toStdString().c_str());

    notifyReadComplete(buffers.value(reply), replyName(reply), tr("Completed."));
}
