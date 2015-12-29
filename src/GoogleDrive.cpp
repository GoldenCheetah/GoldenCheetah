/*
 * Copyright (c) 2015 Magnus Gille (mgille@gmail.com)
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

#include "GoogleDrive.h"

#include "Athlete.h"
#include "Secrets.h"
#include "Settings.h"

#include <stdio.h>

#include <set>
#include <string>

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef GOOGLE_DRIVE_DEBUG
#define GOOGLE_DRIVE_DEBUG false
#endif
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (GOOGLE_DRIVE_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)

namespace {
    QString kGoogleApiBaseAddr = "https://www.googleapis.com";
};

GoogleDrive::GoogleDrive(Context *context)
    : FileStore(context), context_(context), root_(NULL) {
    nam_ = new QNetworkAccessManager(this);
}

GoogleDrive::~GoogleDrive() {
    delete nam_;
}

// open by connecting and getting a basic list of folders available
bool GoogleDrive::open(QStringList &errors) {
    printd("open\n");
    // do we have a token
    QString token = appsettings->cvalue(
        context_->athlete->cyclist, GC_GOOGLE_DRIVE_ACCESS_TOKEN, "")
        .toString();
    if (token == "") {
        errors << "You must authorise with GoogleDrive first";
        return false;
    }

    // lets connect and get basic info on the root directory

    // request using the bearer token, we request the file list at "/".
    QNetworkRequest request = MakeRequest(token, "");
    QNetworkReply *reply = nam_->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // Ugh, here we get the list of files back, but we need to check up the
    // document id of the actual directory. we probably can cache this..

    // did we get a good response ?
    QByteArray r = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }
    // we have a root, we just make it up.
    root_ = newFileStoreEntry();
    // path name
    root_->name = "GoldenCheetah";
    root_->isDir = true;
    root_->size = 0;
    return true;
}

bool GoogleDrive::close() {
    // nothing to do for now
    return true;
}

// home dire
QString GoogleDrive::home() {
    return appsettings->cvalue(
        context_->athlete->cyclist, GC_GOOGLE_DRIVE_FOLDER, "").toString();
}

QNetworkRequest GoogleDrive::MakeRequestWithURL(
    const QString& url, const QString& token, const QString& args) {
    QString request_url(
        kGoogleApiBaseAddr + url + "/?key=" + GC_GOOGLE_DRIVE_API_KEY);
    if (args.length() != 0) {
        request_url += "&" + args;
    }
    QNetworkRequest request(request_url);
    request.setRawHeader(
        "Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    printd("Making request to %s using token: %s\n",
           request_url.toStdString().c_str(), token.toStdString().c_str());
    return request;
}

QNetworkRequest GoogleDrive::MakeRequest(
    const QString& token, const QString& args) {
    return MakeRequestWithURL("/drive/v2/files", token, args);
}

bool GoogleDrive::createFolder(QString path) {
    // folders are just metadata in Google Drive so this is a no-op.
    Q_UNUSED(path);
    return true;
}

QList<FileStoreEntry*> GoogleDrive::readdir(QString path, QStringList &errors) {
    printd("readdir\n");
    // First we need to find out the folder id.
    // Then we can list the actual folder.

    QList<FileStoreEntry*> returning;

    // do we have a token?
    MaybeRefreshCredentials();
    QString token = appsettings->cvalue(
        context_->athlete->cyclist, GC_GOOGLE_DRIVE_ACCESS_TOKEN, "")
        .toString();
    if (token == "") {
        errors << tr("You must authorise with GoogleDrive first");
        return returning;
    }

    // lets connect and get basic info on the root directory
    // request using the bearer token
    QNetworkRequest request = MakeRequest(token, MakeQString("", path) +
                                          QString("&maxResults=1000"));
    QNetworkReply *reply = nam_->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // did we get a good response ?
    QByteArray r = reply->readAll();
    printd("reply: %s\n", r.data());
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // If there's an error just give up.
    if (parseError.error != QJsonParseError::NoError) {
        return returning;
    }

    QJsonArray contents = document.object()["items"].toArray();

    // Fetch more files as long as there are more files.
    QString next_link = document.object()["nextLink"].toString();
    while (next_link != "") {
        printd("Fetching nextLink!\n");
        document = FetchNextLink(next_link, token);
        // Append items;
        QJsonArray tmp = document.object()["items"].toArray();
        for (int i = 0; i < tmp.size(); i++ ) {
            contents.push_back(tmp.at(i));
        }
        next_link = document.object()["nextLink"].toString();
    }

    for(int i=0; i < contents.size(); i++) {
        QJsonObject file = contents.at(i).toObject();
        FileStoreEntry *add = newFileStoreEntry();

        // Some paranoia.
        QJsonObject::iterator it = file.find("trashed");
        if (it != file.end() && it->toString() == "true") {
            continue;
        }
        it = file.find("explicitlyTrashed");
        if (it != file.end() && it->toString() == "true") {
            continue;
        }
        // Google Drive just stores the file name.
        add->name = file["title"].toString();
        add->isDir = false; // We don't have directories.
        add->size = file["fileSize"].toInt();
        // dates in format "Tue, 19 Jul 2011 21:55:38 +0000"
        add->modified = QDateTime::fromString(
            file["modifiedDate"].toString().mid(0,25),
            "ddd, dd MMM yyyy hh:mm:ss");
        returning << add;
    }

    // all good ?
    return returning;
}

// read a file at location (relative to home) into passed array
bool GoogleDrive::readFile(QByteArray *data, QString remote_name) {
    printd("readfile %s\n", remote_name.toStdString().c_str());
    // this must be performed asyncronously and call made
    // to notifyReadComplete(QByteArray &data, QString remotename,
    // QString message) when done

    MaybeRefreshCredentials();
    QString token = appsettings->cvalue(
        context_->athlete->cyclist, GC_GOOGLE_DRIVE_ACCESS_TOKEN, "")
        .toString();
    if (token == "") {
        return false;
    }

    // is the path set ?
    QString path = appsettings->cvalue(
        context_->athlete->cyclist, GC_GOOGLE_DRIVE_FOLDER, "").toString();
    if (path == "") return false;

    // lets connect and get basic info on the root directory

    // TODO(gille): Move this to metadata and pass it in.
    QString url = GetDownloadURL(remote_name, path, token);
    printd("url: %s\n", url.toStdString().c_str());
    QString file_id = GetFileID(remote_name, path, token);
    printd("Got file id: %s\n", file_id.toStdString().c_str());
    if (file_id == "") {
        return false;  // Apparently the file does not exist.
    }
    // request using the bearer token
    QNetworkRequest request = MakeRequest(token, "");

    // put the file
    QNetworkReply *reply = nam_->get(request);

    // remember
    mapReply(reply, remote_name);
    buffers.insert(reply, data);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(readFileCompleted()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    return true;
}

QString GoogleDrive::MakeQString(
    const QString& remote_name, const QString& path) {
    QString q = QString("q=") + QUrl::toPercentEncoding(
        "trashed+=+false+"
        "AND+properties+has+{+key+=+'GoldenCheetahData'+AND+"
        "+value+=+'true'+and+visibility='PRIVATE'}+"
        "AND+properties+has+{+key+=+'GoldenCheetahPath'+AND+"
        "+value+=+'" + path + "' AND visibility='PRIVATE'}"
        , "+");
    if (remote_name != "") {
        q += QUrl::toPercentEncoding(
            "+AND+title+=+'" + remote_name + "'", "+");
    }
    printd("q: %s\n", q.toStdString().c_str());
    return q;
}

QJsonArray GoogleDrive::GetFiles(
    const QString& remote_name, const QString& path, const QString& token) {
    QNetworkRequest request = MakeRequest(
        token, MakeQString(remote_name, path) + QString("&maxResults=1000"));
    QNetworkReply* reply = nam_->get(request);

    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    QJsonArray array;
    QByteArray r = reply->readAll();
    if (reply->error() != 0) {
        printd("Got error %d %s\n", reply->error(), r.data());
        // Return an error?
        return array;
    }
    printd("Got response: %s\n", r.data());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error != QJsonParseError::NoError) {
        printd("Parse error!\n");
        return array;
    }

    array = document.object()["items"].toArray();
    // Fetch more files if there are more files.
    for (;;) {
        QString next_link = document.object()["nextLink"].toString();
        if (next_link == "") {
            // No nextLink, this means we are done.
            break;
        }
        document = FetchNextLink(next_link, token);
        // Append items;
        QJsonArray tmp = document.object()["items"].toArray();
        for (int i = 0; i < tmp.size(); i++ ) {
            array.push_back(tmp.at(i));
        }
    }

    return array;
}

QJsonDocument GoogleDrive::FetchNextLink(
    const QString& url, const QString& token) {
    QNetworkRequest request(url);
    request.setRawHeader(
        "Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    QNetworkReply* reply = nam_->get(request);

    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    QJsonArray array;
    QByteArray r = reply->readAll();
    QJsonDocument empty_doc;
    if (reply->error() != 0) {
        printd("Got error %d %s\n", reply->error(), r.data());
        // Return an error?
        return empty_doc;
    }
    printd("Got response: %s\n", r.data());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return empty_doc; // Just return an empty document.
    }
    return document;
}

QString GoogleDrive::GetFileID(
    const QString& remote_name, const QString& path, const QString& token) {
    QJsonArray array = GetFiles(remote_name, path, token);
    for (int i = 0; i < array.size(); i++) {
        QJsonObject file = array.at(i).toObject();
        printd("item: %s\n", array.at(i).toString().toStdString().c_str());
        // Lets take a look.
        QJsonObject::iterator it = file.find("trashed");
        if (it != file.end() && it->toString() == "true") {
            continue;
        }
        it = file.find("explicitlyTrashed");
        if (it != file.end() && it->toString() == "true") {
            continue;
        }
        return file["id"].toString();
    }
    // No files or they are all trashed.
    return "";

}

QString GoogleDrive::GetDownloadURL(
    const QString& remote_name, const QString& path, const QString& token) {
    QJsonArray array = GetFiles(remote_name, path, token);
    for (int i = 0; i < array.size(); i++) {
        //QJsonValue each = array.at(i); // ValueRef?
        QJsonObject file = array.at(i).toObject();
        printd("item: %s\n", array.at(i).toString().toStdString().c_str());
        // Lets take a look.
        QJsonObject::iterator it = file.find("trashed");
        if (it != file.end() && it->toString() == "true") {
            continue;
        }
        it = file.find("explicitlyTrashed");
        if (it != file.end() && it->toString() == "true") {
            continue;
        }
        return file["downloadUrl"].toString();
    }
    // No files or they are all trashed.
    return "";
}


bool GoogleDrive::writeFile(QByteArray &data, QString remote_name) {
    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done

    MaybeRefreshCredentials();

    // do we have a token ?
    QString token = appsettings->cvalue(
        context_->athlete->cyclist, GC_GOOGLE_DRIVE_ACCESS_TOKEN, "")
        .toString();
    if (token == "") {
        return false;
    }

    // is the path set ?
    QString path = appsettings->cvalue(
        context_->athlete->cyclist, GC_GOOGLE_DRIVE_FOLDER, "").toString();
    if (path == "") {
        return false;
    }

    // GoogleDrive is a bit special, more than one file can have the same name
    // so we need to check their ID and make sure they are unique.
    QString file_id = GetFileID(remote_name, path, token);
    printd("Got file id: %s\n", file_id.toStdString().c_str());
    // lets connect and get basic info on the root directory
    // This needs to be a multi part request so we can name the files.

    QNetworkRequest request;
    if (file_id == "") {
        request = MakeRequestWithURL(
            "/upload/drive/v2/files", token, "uploadType=multipart");
    } else {
        // This file is known, just overwrite the old one.
        request = MakeRequestWithURL(
            "/upload/drive/v2/files/" + file_id, token, "uploadType=multipart");
    }
    QString boundary = "------------------------------------";
    QString delimiter = "\r\n--" + boundary + "\r\n";
    request.setRawHeader(
        "Content-Type",
        QString("multipart/mixed; boundary=\"" + boundary + "\"").toLatin1());
    QString base64data = data.toBase64();
    printd("Uploading file %s\n", remote_name.toStdString().c_str());
    QString multipartRequestBody =
        delimiter +
        "Content-Type: application/json\r\n\r\n" +
        "{ \"title\": \"" + remote_name + "\", " +
        "  parents: [{\"id\": \"appdata\"}], " +
        "  properties: [ "+
        "  { \"key\": \"GoldenCheetahData\", \"value\": \"true\", "
        "    \"visibility\": \"PRIVATE\"}, " +
        "  { \"key\": \"GoldenCheetahPath\", \"value\": \"" + path + "\", "
        "    \"visibility\": \"PRIVATE\" } " +
        "  ] " +
        "}" +
        delimiter +
        "Content-Type: application/zip\r\n" +
        "Content-Transfer-Encoding: base64\r\n\r\n" +
        base64data +
        "\r\n--" + boundary + "--";
    printd("Uploading %s\n", multipartRequestBody.toStdString().c_str());

    // post the file
    QNetworkReply *reply;
    if (file_id == "") {
        reply = nam_->post(request, multipartRequestBody.toLatin1());
    } else {
        reply = nam_->put(request, multipartRequestBody.toLatin1());
    }

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply, remote_name);
    return true;
}

void GoogleDrive::writeFileCompleted() {
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    QByteArray r = reply->readAll();
    printd("QNetworkReply: %d\n", reply->error());
    printd("write file: %s\n", r.data());
    if (reply->error() == QNetworkReply::NoError) {
        notifyWriteComplete(
            replyName(static_cast<QNetworkReply*>(QObject::sender())),
            tr("Completed."));
    } else {
        notifyWriteComplete(
            replyName(static_cast<QNetworkReply*>(QObject::sender())),
            tr("Upload failed") + QString(" ") + QString(r.data()));
    }
}

void GoogleDrive::readyRead() {
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

void GoogleDrive::readFileCompleted() {
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    notifyReadComplete(buffers.value(reply), replyName(reply),
                       tr("Completed."));
}

void GoogleDrive::MaybeRefreshCredentials() {
    QString last_refresh_str = appsettings->cvalue(
        context_->athlete->cyclist, GC_GOOGLE_DRIVE_LAST_ACCESS_TOKEN_REFRESH,
        "0").toString();
    QDateTime last_refresh = QDateTime::fromString(last_refresh_str);
    // TODO(gille): remember when it expires.
    last_refresh = last_refresh.addSecs(45 * 60);
    QString refresh_token = appsettings->cvalue(
        context_->athlete->cyclist, GC_GOOGLE_DRIVE_REFRESH_TOKEN, "")
        .toString();
    if (refresh_token == "") {
        return;
    }
    // If we need to refresh the access token do so.
    QDateTime now = QDateTime::currentDateTime();
    printd("times: %s %s\n", last_refresh_str.toStdString().c_str(),
           now.toString().toStdString().c_str());
    if (now > last_refresh) { // This is true if the refresh is older than 30
        // minutes.
        QNetworkRequest request(kGoogleApiBaseAddr + "/oauth2/v3/token");
        request.setRawHeader("Content-Type",
                             "application/x-www-form-urlencoded");
        QString data = QString("client_id=");
        data.append(GC_GOOGLE_DRIVE_CLIENT_ID)
            .append("&").append("client_secret=")
            .append(GC_GOOGLE_DRIVE_CLIENT_SECRET).append("&")
            .append("refresh_token=").append(refresh_token).append("&")
            .append("grant_type=refresh_token");
        printd("Making request to %s using token: %s data: %s\n",
               (kGoogleApiBaseAddr + "/oauth2/v3/token").toStdString().c_str(),
               refresh_token.toStdString().c_str(), data.toLatin1().data());
        QNetworkReply* reply = nam_->post(request, data.toLatin1());

        // blocking request
        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        int statusCode = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        printd("HTTP response code: %d\n", statusCode);

        if (reply->error() != 0) {
            printd("Got error %d\n", reply->error());
            // Return an error?
            return;
        }
        QByteArray r = reply->readAll();
        printd("Got response: %s\n", r.data());

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        // if path was returned all is good, lets set root
        if (parseError.error != QJsonParseError::NoError) {
            printd("Parse error!\n");
            return;
        }

        QString access_token = document.object()["access_token"].toString();
        appsettings->setCValue(
            context_->athlete->cyclist,
            GC_GOOGLE_DRIVE_ACCESS_TOKEN, access_token);
        appsettings->setCValue(
            context_->athlete->cyclist,
            GC_GOOGLE_DRIVE_LAST_ACCESS_TOKEN_REFRESH,
            now.toString());
    }
}
