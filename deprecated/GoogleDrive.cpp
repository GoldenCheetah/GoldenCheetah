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

#include <stdio.h>

#include <set>
#include <string>

#include "Athlete.h"
#include "Secrets.h"
#include "Settings.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef GOOGLE_DRIVE_DEBUG
// TODO(gille): This should be a command line flag.
#define GOOGLE_DRIVE_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (GOOGLE_DRIVE_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (GOOGLE_DRIVE_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

namespace {
    static const QString kGoogleApiBaseAddr = "https://www.googleapis.com";
    static const QString kFileMimeType = "application/zip";
    static const QString kDirectoryMimeType =
        "application/vnd.google-apps.folder";
    static const QString kMetadataMimeType = "application/json";
    // This is an integer but written as a string.
    static const QString kMaxResults = "1000";
};

struct GoogleDrive::FileInfo {
    QString name;
    QString id;  // This is the unique identifier.
    QString parent; // id of the parent.
    QString download_url;

    QDateTime modified;
    int size;
    bool is_dir;

    // This is a map of names => FileInfos for quick searching of children.
    std::map<QString, QSharedPointer<FileInfo> > children;
};

GoogleDrive::GoogleDrive(Context *context)
    : CloudService(context), context_(context), root_(NULL) {
    if (context) {
        printd("GoogleDrive::GoogleDrive\n");
        nam_ = new QNetworkAccessManager(this);
        connect(nam_, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }
    root_ = NULL;

    // config
    // Scope was "%1::Scope::drive::drive.appdata::drive.file", but drive access
    // was revoked by Google and drive.appdata doesn't seem to work.
    // drive.file is enough for our purposes.
    settings.insert(Combo1, QString("%1::Scope::drive.file").arg(GC_GOOGLE_DRIVE_AUTH_SCOPE));
    settings.insert(OAuthToken, GC_GOOGLE_DRIVE_ACCESS_TOKEN);
    settings.insert(Folder, GC_GOOGLE_DRIVE_FOLDER);
    settings.insert(Local1, GC_GOOGLE_DRIVE_FOLDER_ID); // derived during config, no user action
    settings.insert(Local2, GC_GOOGLE_DRIVE_REFRESH_TOKEN); // derived during config, no user action
    settings.insert(Local3, GC_GOOGLE_DRIVE_LAST_ACCESS_TOKEN_REFRESH); // derived during config, no user action
}

GoogleDrive::~GoogleDrive() {
    if (context) delete nam_;
}

void GoogleDrive::onSslErrors(QNetworkReply*reply, const QList<QSslError> & )
{
    reply->ignoreSslErrors();

}

// open by connecting and getting a basic list of folders available
bool GoogleDrive::open(QStringList &errors) {
    printd("open\n");
    // do we have a token
    QString token = getSetting(GC_GOOGLE_DRIVE_ACCESS_TOKEN, "").toString();
    if (token == "") {
        errors << "You must authorise with GoogleDrive first";
        return false;
    }

    root_ = newCloudServiceEntry();
    root_->name = "GoldenCheetah";
    root_->isDir = true;
    root_->size = 0;

    FileInfo* new_root = new FileInfo;
    new_root->name = "/";
    new_root->id = GetRootDirId();
    root_dir_.reset(new_root);

    MaybeRefreshCredentials();

    // if this fails we're toast
    readdir(home(), errors);
    return errors.count() == 0;
}

bool GoogleDrive::close() {
    // nothing to do for now
    return true;
}

// home dire
QString GoogleDrive::home() {
    return getSetting(GC_GOOGLE_DRIVE_FOLDER, "").toString();
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
    return MakeRequestWithURL("/drive/v3/files", token, args);
}

bool GoogleDrive::createFolder(QString path) {
    printd("createFolder: %s\n", path.toStdString().c_str());
    MaybeRefreshCredentials();
    QString token = getSetting(GC_GOOGLE_DRIVE_ACCESS_TOKEN, "").toString();
    if (token == "") {
        return false;
    }
    while (*path.begin() == '/') {
        path = path.remove(0, 1);
    }

    if (path.size() == 0) {
        // Eh?
        return true;
    }
    // TODO(gille): This only supports directories in the root. Fix that.
    QStringList parts = path.split("/", Qt::SkipEmptyParts);
    QString dir_name = parts.back();
    FileInfo* parent_fi = WalkFileInfo(path, true);
    if (parent_fi == NULL) {
        // This shouldn't happen...
        return false;
    }

    QNetworkRequest request = MakeRequestWithURL(
        "/drive/v3/files", token, "");

    request.setRawHeader("Content-Type", kMetadataMimeType.toLatin1());
    //QString("multipart/mixed; boundary=\"" + boundary + "\"").toLatin1());
    printd("Creating directory %s\n", path.toStdString().c_str());

    QJsonObject json_request;
    json_request.insert("name", QJsonValue(dir_name));
    json_request.insert("mimeType", QJsonValue(kDirectoryMimeType));

    if (parent_fi->id != "") {//FIXME
        QJsonArray array;
        QJsonObject parent;
        parent.insert("id", parent_fi->id);
        array.append(parent);
        json_request.insert("parents", array);
    }

    QString requestBody = QJsonDocument(json_request).toJson() + "\r\n";;
    printd("Creating: %s\n", requestBody.toStdString().c_str());

    // post the file
    QNetworkReply *reply = nam_->post(request, requestBody.toLatin1());
    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // did we get a good response ?
    QByteArray r = reply->readAll();
    if (reply->error() != 0) {
        printd("Got error %d %s\n", reply->error(), r.data());
        // Return an error?
        return false;
    }
    printd("reply: %s\n", r.data());
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // If there's an error just give up.
    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }
    return true;
}

GoogleDrive::FileInfo* GoogleDrive::WalkFileInfo(const QString& path,
                                                 bool foo) {
    QStringList parts = path.split("/", Qt::SkipEmptyParts);
    FileInfo* target = root_dir_.data();
    // Lets walk!
    for (QStringList::iterator it = parts.begin(); it != parts.end(); ++it) {
        std::map<QString, QSharedPointer<FileInfo> >::iterator child =
            target->children.find(*it);
        if (child == target->children.end()) {
            // Directory doesn't exist!
            printd("Bailing, because I couldn't find: %s in %s\n",
                   (*it).toStdString().c_str(),
                   target->name.toStdString().c_str());
            if (foo) {
                return target;
            }
            return NULL;
        }
        target = child->second.data();
    }
    return target;
}

QList<CloudServiceEntry*> GoogleDrive::readdir(QString path, QStringList &errors) {
    // Ugh. Listing files in Google Drive is "different".
    // There can be many files with the same name, directories are just metadata
    // so we just list everything and then we turn it into a normal structure
    // locally.

    // Note, if we call readdir on "/" we nuke all and any caches we have.
    // This is to try and keep life "easier".

    printd("readdir %s\n", path.toStdString().c_str());
    // First we need to find out the folder id.
    // Then we can list the actual folder.

    QList<CloudServiceEntry*> returning;
    // Trim some / if necssary.
    while (*path.end() == '/' && path.size() > 1) {
        path = path.remove(path.size() - 1, 1);
    }
    // do we have a token?
    MaybeRefreshCredentials();
    QString token = getSetting(GC_GOOGLE_DRIVE_ACCESS_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with GoogleDrive first");
        return returning;
    }

    FileInfo* parent_fi = WalkFileInfo(path, false);
    if (parent_fi == NULL) {
        // This can happen.. If it does we kind of have to walk our way up
        // here even though it'll be slow and painful.
        // We could store the directory id and find it that way...

        // Ok. Lets try to fake it out. Who knows maybe it'll work.
        // TODO(gille): Handle an empty response/404 below?
        if (path == home()) {
            printd("Build path for home directory.\n");
            parent_fi = BuildDirectoriesForAthleteDirectory(path);
        }
        if (parent_fi == NULL) {
            errors << tr("No such directory, try setting a new location in "
                         "options.");
            return returning;
        }
    }
    QNetworkRequest request = MakeRequest(
        token, MakeQString(parent_fi->id) +
        QString("&pageSize=" + kMaxResults + "&fields=nextPageToken,files(explicitlyTrashed,fileExtension,id,kind,mimeType,modifiedTime,name,properties,size,trashed)"));
    QString url = request.url().toString();
    QNetworkReply *reply = nam_->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // did we get a good response ?
    QByteArray r = reply->readAll();
    if (reply->error() != 0) {
        printd("Got error %d %s\n", reply->error(), r.data());
        // Return an error?
        return returning;
    }
    printd("reply: %s\n", r.data());
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // If there's an error just give up.
    if (parseError.error != QJsonParseError::NoError) {
        printd("json parse error: ....\n"); // FIXME
        return returning;
    }

    QJsonArray contents = document.object()["files"].toArray();
    // Fetch more files as long as there are more files.
    QString next_page_token = document.object()["nextPageToken"].toString();
    while (next_page_token != "") {
        printd("Fetching next page!\n");
        document = FetchNextLink(url + "&pageToken=" + next_page_token, token);
        // Append items;
        QJsonArray tmp = document.object()["files"].toArray();
        for (int i = 0; i < tmp.size(); i++ ) {
            contents.push_back(tmp.at(i));
        }
        next_page_token = document.object()["nextPageToken"].toString();
    }
    // Ok. We have reeeived all the files.
    // Technically we could cache this, but if we do we need to figure out
    // when to invalidate it.
    std::map<QString, QSharedPointer<FileInfo> > file_by_id;
    for(int i = 0; i < contents.size(); i++) {
        QJsonObject file = contents.at(i).toObject();
        // Some paranoia.
        QJsonObject::iterator it = file.find("trashed");
        if (it != file.end() && (*it).toString() == "true") {
            continue;
        }
        it = file.find("explicitlyTrashed");
        if (it != file.end() && (*it).toString() == "true") {
            continue;
        }
        QSharedPointer<FileInfo> fi(new FileInfo);
        fi->name = file["name"].toString();
        fi->id = file["id"].toString();
        QJsonArray parents = file["parents"].toArray();
        // First parent is best parent!
        fi->parent = parents.at(0).toObject()["id"].toString();
        if (fi->parent == "") {
            // NOTE(gille): This doesn't really happen since folders in the
            // root belong to a specific id. But I'd rather be safe than sorry.
            fi->parent = GetRootDirId();
        }
        it = file.find("mimeType");
        if (it != file.end() && (*it).toString() == kDirectoryMimeType) {
            fi->is_dir = true;
        } else {
            fi->is_dir = false;
        }
        fi->size = file["fileSize"].toInt();
        // dates in format "Tue, 19 Jul 2011 21:55:38 +0000"
        fi->modified = QDateTime::fromString(
            file["modifiedDate"].toString().mid(0,25),
            "ddd, dd MMM yyyy hh:mm:ss");
        fi->download_url = QString(
            "https://www.googleapis.com/drive/v3/files/" + fi->id +
            "?alt=media");
        file_by_id[fi->id] = fi;
    }
    // Ok. We now have all valid files. Build the tree. We only rebuild the part
    // that we updated.
    FileInfo* new_root;
    if (path == "/") {
        new_root = new FileInfo;
        new_root->name = "/";
        new_root->id = GetRootDirId();
        root_dir_.reset(new_root);
        printd("Creating new root.\n");
    } else {
        new_root = parent_fi;
        new_root->children.clear();
        printd("Appending to old root.\n");
    }

    // We know they're all where they should be right?
    for (std::map<QString, QSharedPointer<FileInfo> >::iterator it =
             file_by_id.begin();
         it != file_by_id.end(); ++it) {
        new_root->children[it->second->name] = it->second;
    }

    // Ok. It's now in a nice format. Lets walk so we can return what the
    // other layers expect.
    FileInfo* target = WalkFileInfo(path, false);
    if (target == NULL) {
        printd("Unable to walk the paths.\n");
        return returning;
    }
    for (std::map<QString, QSharedPointer<FileInfo> >::iterator it =
             target->children.begin();
         it != target->children.end(); ++it) {
        CloudServiceEntry *add = newCloudServiceEntry();
        // Google Drive just stores the file name.
        add->name = it->second->name;
        add->id = add->name;
        printd("Returning entry: %s\n", add->name.toStdString().c_str());
        add->isDir = it->second->is_dir;
        add->size = it->second->size;
        add->modified = it->second->modified;
        returning << add;
    }
    // all good!
    printd("returning %d entries.\n", returning.size());
    return returning;
}

// read a file at location (relative to home) into passed array
bool GoogleDrive::readFile(QByteArray *data, QString remote_name, QString) {
    printd("readfile %s\n", remote_name.toStdString().c_str());
    // this must be performed asyncronously and call made
    // to notifyReadComplete(QByteArray &data, QString remotename,
    // QString message) when done

    MaybeRefreshCredentials();
    QString token = getSetting(GC_GOOGLE_DRIVE_ACCESS_TOKEN, "").toString();
    if (token == "") {
        return false;
    }

    // Before this is done we know we have called readdir so we have the id.
    FileInfo* fi = WalkFileInfo(home() + "/" + remote_name, false);
    // TODO(gille): Is it worth doing readdir if this fails?
    if (fi == NULL) {
        printd("Trying to download files that don't exist?\n");
        return false;
    }
    printd("Trying to download: %s from %s\n",
           remote_name.toStdString().c_str(),
           fi->download_url.toStdString().c_str());
    QNetworkRequest request(fi->download_url);
    request.setRawHeader(
        "Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    // Get the file.
    QNetworkReply *reply = nam_->get(request);
    // remember the file.
    mapReply(reply, remote_name);
    buffers_.insert(reply, data);
    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(readFileCompleted()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    return true;
}

QString GoogleDrive::MakeQString(const QString& parent) {
    QString q;
    if (parent == "") {
        q = QString("q=") + QUrl::toPercentEncoding(
            "trashed+=+false+AND+'root'+IN+parents", "+");
    } else {
        q = QString("q=") + QUrl::toPercentEncoding(
            "trashed+=+false+AND+'" + parent + "'+IN+parents", "+");
    }
    return q;
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

bool GoogleDrive::writeFile(QByteArray &data, QString remote_name, RideFile *ride) {

    Q_UNUSED(ride);

    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done

    MaybeRefreshCredentials();

    // do we have a token ?
    QString token = getSetting(GC_GOOGLE_DRIVE_ACCESS_TOKEN, "").toString();
    if (token == "") {
        return false;
    }
    QString path = home();
    FileInfo* parent_fi = WalkFileInfo(path, false);
    if (parent_fi == NULL) {
        return false;
    }

    // GoogleDrive is a bit special, more than one file can have the same name
    // so we need to check their ID and make sure they are unique.
    FileInfo* fi = WalkFileInfo(path + "/" + remote_name, false);

    QNetworkRequest request;
    if (fi == NULL) {
        // This means we will post the file further down.
        request = MakeRequestWithURL(
            "/upload/drive/v3/files", token, "uploadType=multipart");
    } else {
        // This file is known, just overwrite the old one.
        request = MakeRequestWithURL(
            "/upload/drive/v3/files/" + fi->id, token, "uploadType=multipart");
    }
    QString boundary = "------------------------------------";
    QString delimiter = "\r\n--" + boundary + "\r\n";
    request.setRawHeader(
        "Content-Type",
        QString("multipart/mixed; boundary=\"" + boundary + "\"").toLatin1());
    QString base64data = data.toBase64();
    printd("Uploading file %s\n", remote_name.toStdString().c_str());
    printd("Using parent id: %s\n", parent_fi->id.toStdString().c_str());

    QString multipartRequestBody =
        delimiter +
        "Content-Type: " + kMetadataMimeType + "\r\n\r\n" +
        "{ name: \"" + remote_name + "\" ";
    if (fi == NULL) {
        // NOTE(gille): Parents is only valid on upload.
        multipartRequestBody += 
            "  ,parents: [ \"" + parent_fi->id + "\"  ] ";
    }
    multipartRequestBody += 
        "}" +
        delimiter +
        "Content-Type: " + kFileMimeType + "\r\n" +
        "Content-Transfer-Encoding: base64\r\n\r\n" +
        base64data +
        "\r\n--" + boundary + "--";

    // post the file
    QNetworkReply *reply;
    if (fi == NULL) {
        printd("Posting the file\n");
        reply = nam_->post(request, multipartRequestBody.toLatin1());
    } else {
        printd("Patching the file\n");
        // Ugh, QT doesn't support Patch and we need it to upload files.
        // So we get to handle this all on our own.
        QSharedPointer<QBuffer> buffer(new QBuffer());
        buffer->open(QBuffer::ReadWrite);
        buffer->write(multipartRequestBody.toLatin1());
        buffer->seek(0);
        reply = nam_->sendCustomRequest(request, "PATCH", buffer.data());

        mu_.lock();
        patch_buffers_[reply] = buffer;
        mu_.unlock();
    }

    // remember
    mapReply(reply, remote_name);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));
    return true;
}

void GoogleDrive::writeFileCompleted() {
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    QByteArray r = reply->readAll();

    printd("QNetworkReply: %d\n", reply->error());
    printd("write file: %s\n", r.data());

    // Erase the data buffer if there is one.
    
    mu_.lock();
    QMap<QNetworkReply*, QSharedPointer<QBuffer> >::iterator it =
        patch_buffers_.find(reply);
    if (it != patch_buffers_.end()) {
        patch_buffers_.erase(it);
    }
    mu_.unlock();
    
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
    buffers_.value(reply)->append(reply->readAll());
}

void GoogleDrive::readFileCompleted() {
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    notifyReadComplete(buffers_.value(reply), replyName(reply),
                       tr("Completed."));
    // erase from buffer?
}

void GoogleDrive::MaybeRefreshCredentials() {
    QString last_refresh_str = getSetting(GC_GOOGLE_DRIVE_LAST_ACCESS_TOKEN_REFRESH, "0").toString();
    QDateTime last_refresh = QDateTime::fromString(last_refresh_str);
    // TODO(gille): remember when it expires.
    last_refresh = last_refresh.addSecs(45 * 60);
    QString refresh_token = getSetting(GC_GOOGLE_DRIVE_REFRESH_TOKEN, "").toString();
    if (refresh_token == "") {
        return;
    }
    // If we need to refresh the access token do so.
    QDateTime now = QDateTime::currentDateTime();
    printd("times: %s %s\n", last_refresh_str.toStdString().c_str(),
           now.toString().toStdString().c_str());
    if (now > last_refresh) { // This is true if the refresh is older than 45
        // minutes.
        printd("Refreshing credentials.\n");
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

        // LOCALLY MAINTAINED -- WILL BE AN ISSUE IF ALLOW >1 ACCOUNT XXX
        setSetting(GC_GOOGLE_DRIVE_ACCESS_TOKEN, access_token);
        setSetting(GC_GOOGLE_DRIVE_LAST_ACCESS_TOKEN_REFRESH, now.toString());
        appsettings->setCValue(context_->athlete->cyclist, GC_GOOGLE_DRIVE_ACCESS_TOKEN, access_token);
        appsettings->setCValue(context_->athlete->cyclist, GC_GOOGLE_DRIVE_LAST_ACCESS_TOKEN_REFRESH, now.toString());
    } else {
        printd("Credentials are still good, not refreshing.\n");
    }
}

QString GoogleDrive::GetRootDirId() {
    QString scope = getSetting(GC_GOOGLE_DRIVE_AUTH_SCOPE, "drive.appdata").toString();
    if (scope == "drive.appdata") {
        return "appdata"; // dgr : not appDataFolder ?
    } else {
        return "root";
    }
}

void GoogleDrive::folderSelected(QString path)
{
    // we selected a folder so we need to set the FOLDER_ID
    QString id = GetFileId(path);
    setSetting(GC_GOOGLE_DRIVE_FOLDER_ID, id);
}

QString GoogleDrive::GetFileId(const QString& path) {
    FileInfo* fi = WalkFileInfo(path, false);
    if (fi == NULL) {
        return "";
    }
    return fi->id;
}

GoogleDrive::FileInfo* GoogleDrive::BuildDirectoriesForAthleteDirectory(
    const QString& path) {
    // Because Google Drive is a little bit "different" we can't just read
    // "/foo/bar/baz, we need to know the id's of both foo and bar to find
    // baz.
    const QString id = getSetting(GC_GOOGLE_DRIVE_FOLDER_ID, "").toString();
    if (id == "") {
        // TODO(gille): Maybe we should actually find this dir if this happens
        // however this is a weird configuration error, that (tm)
        // should not happen.
        return NULL;
    }
    printd("GC_GOOGLE_DRIVE_FOLDER_ID: %s\n", id.toStdString().c_str());
    QStringList parts = path.split("/", Qt::SkipEmptyParts);
    FileInfo *fi = root_dir_.data();

    for (QStringList::iterator it = parts.begin(); it != parts.end(); ++it) {
        std::map<QString, QSharedPointer<FileInfo> >::iterator next =
            fi->children.find(*it);
        if (next == fi->children.end()) {
            QSharedPointer<FileInfo> next_fi(new FileInfo);
            next_fi->name = *it;
            next_fi->id = " INVALID ";
            next_fi->parent = " INVALID ";
            fi->children[*it] = next_fi;
            fi = next_fi.data();
        } else {
            fi = next->second.data();
        }
    }
    // Overwrite the final directory id with the right one.
    fi->id = id;
    return fi;
}


static bool addGoogleDrive() {
    CloudServiceFactory::instance().addService(new GoogleDrive(NULL));
    return true;
}

static bool add = addGoogleDrive();
