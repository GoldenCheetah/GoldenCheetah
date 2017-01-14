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

#include "SixCycle.h"
#include "Athlete.h"
#include "Settings.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>


#ifndef SIXCYCLE_DEBUG
#define SIXCYCLE_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (SIXCYCLE_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (SIXCYCLE_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

SixCycle::SixCycle(Context *context) : FileStore(context), context(context), root_(NULL) {
    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));

    uploadCompression = gzip; // gzip
    downloadCompression = none;
    session_token = ""; // not authenticated yet

    useMetric = true; // distance and duration metadata
}

SixCycle::~SixCycle() {
    delete nam;
}

void
SixCycle::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

// open by connecting and getting a basic list of folders available
bool
SixCycle::open(QStringList &errors)
{
    printd("SixCycle::open\n");

    // do we have a token
    QString user = appsettings->cvalue(context->athlete->cyclist, GC_SIXCYCLE_USER, "").toString();
    QString pass = appsettings->cvalue(context->athlete->cyclist, GC_SIXCYCLE_PASS, "").toString();

    if (user == "") {
        errors << "You must setup SixCylce login details first";
        return false;
    }

    // use the configed URL
    QString url = QString("%1/rest-auth/login/")
          .arg(appsettings->cvalue(context->athlete->cyclist, GC_SIXCYCLE_URL, "https://live.sixcycle.com").toString());

    // We need to POST a form with the user and password
    QUrlQuery postData(url);
    postData.addQueryItem("email", user);
    postData.addQueryItem("password", pass);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,  "application/x-www-form-urlencoded");
    QNetworkReply *reply = nam->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    // blocking request - wait for response
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "error" << reply->errorString();
        errors << tr("Network Problem authenticating with the SixCycle service");
        return false;
    }

    // did we get a good response ?
    QByteArray r = reply->readAll();
    printd("response: %s\n", r.toStdString().c_str());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {
        printd("NoError\n");

        QJsonObject content = document.object();

        // extract the user root and the session_token
        QJsonValue key = content["key"];
        session_token = key.toString();
        printd("session token: %s\n", session_token.toStdString().c_str());

        QJsonValue user = content["user"];
        session_user = user.toString();
        printd("session user: %s\n", session_user.toStdString().c_str());

        // set root (there is no directory concept for SixCycle)
        root_ = newFileStoreEntry();

        // path name
        root_->name = "/";
        root_->isDir = true;
        root_->size = 0;

        // happy with what we got ?
        if (root_->name != "/") errors << tr("invalid root path.");
        if (root_->isDir != true) errors << tr("root is not a directory.");

    } else {
        errors << tr("problem parsing SixCycle authentication response.");
    }

    // ok so far ?
    if (errors.count()) return false;
    return true;
}

bool
SixCycle::close()
{
    printd("SixCycle::close\n");
    // nothing to do for now
    return true;
}

// home dire
QString
SixCycle::home()
{
    return "";
}

bool
SixCycle::createFolder(QString)
{
    printd("SixCycle::createFolder\n");

    return false;
}

QList<FileStoreEntry*>
SixCycle::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    printd("SixCycle::readdir(%s)\n", path.toStdString().c_str());

    QList<FileStoreEntry*> returning;
    if (session_token == "") {
        errors << tr("You must authenticate with SixCycle first");
        return returning;
    }

    // lets connect and get activities list
    // old API ?
    // QString url("https://whats.SixCycle.com.au/rest/files/search/0/100");
    QString url = QString("%1/rest/users/activities/search/0/100")
          .arg(appsettings->cvalue(context->athlete->cyclist, GC_SIXCYCLE_URL, "https://whats.SixCycle.com.au").toString());


    //url="https://staging.SixCycle.com.au/rest/files/search/0/100";

    // request using the bearer token
    QNetworkRequest request(url);
    //request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
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

            //SixCycle has full path, we just want the file name
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
SixCycle::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("SixCycle::readFile(%s)\n", remotename.toStdString().c_str());

    if (session_token == "") {
        return false;
    }
    // this must be performed asyncronously and call made
    // to notifyReadComplete(QByteArray &data, QString remotename, QString message) when done

    // lets connect and get basic info on the root directory
    QString url = QString("%1/rest/files/download/%2")
          .arg(appsettings->cvalue(context->athlete->cyclist, GC_SIXCYCLE_URL, "https://whats.SixCycle.com.au").toString()).arg(remoteid);

    printd("url:%s\n", url.toStdString().c_str());

    // request using the bearer token
    QNetworkRequest request(url);
    //request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

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
SixCycle::writeFile(QByteArray &data, QString remotename)
{
    printd("SixCycle::writeFile(%s)\n", remotename.toStdString().c_str());

    // if we are called to upload a single ride we will not have been
    // authenticated yet, so lets try and do that now. When called via
    // the SyncDialog this will have already been done.
    if (session_token == "") {

        // we need to authenticate !
        QStringList errors;
        open(errors);

        // we didn't get a token?
        if (session_token == "") return false;
    }
    return false; //XXX not implemented yet!

    // lets connect and get basic info on the root directory
    QString url = QString("%1/rest/files/upload")
          .arg(appsettings->cvalue(context->athlete->cyclist, GC_SIXCYCLE_URL, "https://whats.SixCycle.com.au").toString());

    QNetworkRequest request = QNetworkRequest(url);

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QString boundary = QVariant(qrand()).toString()+QVariant(qrand()).toString()+QVariant(qrand()).toString();
    multiPart->setBoundary(boundary.toLatin1());

    //request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

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
SixCycle::writeFileCompleted()
{
    printd("SixCycle::writeFileCompleted()\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s", reply->readAll().toStdString().c_str());

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
SixCycle::readyRead()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

void
SixCycle::readFileCompleted()
{
    printd("SixCycle::readFileCompleted\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s", buffers.value(reply)->toStdString().c_str());

    notifyReadComplete(buffers.value(reply), replyName(reply), tr("Completed."));
}
