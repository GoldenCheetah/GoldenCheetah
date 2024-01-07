/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#include "Dropbox.h"
#include "Athlete.h"
#include "Settings.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>

Dropbox::Dropbox(Context *context) : CloudService(context), context(context), root_(NULL) {
    if (context) {
        nam = new QNetworkAccessManager(this);
    }

    // config
    settings.insert(OAuthToken, GC_DROPBOX_TOKEN);
    settings.insert(Folder, GC_DROPBOX_FOLDER);
}

Dropbox::~Dropbox() {
    if (context) {delete nam;}
}

// open by connecting and getting a basic list of folders available
bool
Dropbox::open(QStringList &errors)
{
    // do we have a token
    QString token = getSetting(GC_DROPBOX_TOKEN, "").toString();
    if (token == "") {
        errors << "You must authorise with Dropbox first";
        return false;
    }

    // lets connect and read "" which is root - with API V2 root
    // does not provide any metadata any more, so we just
    // check that we have access to "something"
    readdir("", errors);

    // if path was returned all is good, lets set root
    if (errors.count() == 0) {

        // we have a root
        root_ = newCloudServiceEntry();

        // path name
        root_->name = "/";
        root_->isDir = true;
        root_->size = 0;

    } else {
        errors << tr("Problem accessing Dropbox data");
    }

    // ok so far ?
    if (errors.count()) return false;
    return true;
}

bool 
Dropbox::close()
{
    // nothing to do for now
    return true;
}

// home dire
QString
Dropbox::home()
{
    return getSetting(GC_DROPBOX_FOLDER, "").toString(); 
}

bool Dropbox::createFolder(QString path)
{

    // do we have a token
    QString token = getSetting(GC_DROPBOX_TOKEN, "").toString();
    if (token == "") return false;

    // lets connect and get basic info on the root directory
    QString url("https://api.dropboxapi.com/2/files/create_folder_v2");

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setRawHeader("Content-Type", "application/json");

    QByteArray data;
    data.append(QString("{ \"path\": \"%1\", \"autorename\": false }").arg(path));
    QNetworkReply *reply = nam->post(request, data);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // 403 EXISTS, otherwise OK
    return (reply->error() == QNetworkReply::NoError);
}

QList<CloudServiceEntry*> 
Dropbox::readdir(QString path, QStringList &errors)
{
    // Dropbox API V2 uses "" for root, not "/" to not change any other places in the code,
    // affecting other cloud services, special handling is introduced here
    if (path == "/") path = "";

    QList<CloudServiceEntry*> returning;

    // do we have a token
    QString token = getSetting(GC_DROPBOX_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with Dropbox first");
        return returning;
    }

    // lets connect and get basic info on the root directory
    QString url("https://api.dropboxapi.com/2/files/list_folder");

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setRawHeader("Content-Type", "application/json");

    bool listHasMoreEntries = true;
    bool firstRequest = true;
    QString cursor;
    while (listHasMoreEntries) {
        QByteArray data;

        if (firstRequest) {
            data.append(QString("{ \"path\": \"%1\", \"recursive\": false ,\"include_deleted\": false }").arg(path));
            firstRequest = false;
        } else {
            request.setUrl(QUrl("https://api.dropboxapi.com/2/files/list_folder/continue"));
            data.append(QString("{ \"cursor\": \"%1\" }").arg(cursor));
        }
        QNetworkReply *reply = nam->post(request, data);

        // blocking request
        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        // oops, no dice
        if (reply->error() != 0) {
            errors << reply->errorString();
            return returning;
        }

        // did we get a good response ?
        QByteArray r = reply->readAll();

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        // if path was returned all is good, lets set root
        if (parseError.error == QJsonParseError::NoError) {

            // contents ?
            QJsonArray contents = document.object()["entries"].toArray();
            cursor = document.object()["cursor"].toString();
            if (document.object()["has_more"].toBool()) {
                listHasMoreEntries = true;
            } else {
                listHasMoreEntries = false;
            }

            // lets look at that then
            for(int i=0; i<contents.size(); i++) {

                QJsonValue each = contents.at(i);
                CloudServiceEntry *add = newCloudServiceEntry();

                //dropbox has full path, we just want the file name
                add->name = QFileInfo(each.toObject()["path_display"].toString()).fileName();
                add->isDir = each.toObject()[".tag"].toString() == "folder";
                add->size = each.toObject()["bytes"].toInt();
                add->id = add->name;

                // dates in format "Tue, 19 Jul 2011 21:55:38 +0000"
                add->modified = QDateTime::fromString(each.toObject()["client_modified"].toString(),
                        "ddd, dd MMM yyyy hh:mm:ss");

                returning << add;
            }
        } else {
            errors << tr("Parsing Error: %1").arg(QString::fromUtf8(r));
            return returning;
        }
    }

    // all good ?
    return returning;
}

// read a file at location (relative to home) into passed array
bool
Dropbox::readFile(QByteArray *data, QString remotename, QString)
{
    // this must be performed asyncronously and call made
    // to notifyReadComplete(QByteArray &data, QString remotename, QString message) when done

    // do we have a token ?
    QString token = getSetting(GC_DROPBOX_TOKEN, "").toString();
    if (token == "") return false;

    // is the path set ?
    QString path = getSetting(GC_DROPBOX_FOLDER, "").toString();
    if (path == "") return false;

    // lets connect and get basic info on the root directory
    QString url("https://content.dropboxapi.com/2/files/download");

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setRawHeader("Dropbox-API-Arg", (QString("{ \"path\": \"%1/%2\" }").arg(path).arg(remotename)).toLatin1());
    // put the file
    QByteArray emptyPostData = "";
    QNetworkReply *reply = nam->post(request, emptyPostData);

    // remember
    mapReply(reply,remotename);
    buffers.insert(reply,data);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(readFileCompleted()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    return true;
}

bool 
Dropbox::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    Q_UNUSED(ride);

    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done

    // do we have a token ?
    QString token = getSetting(GC_DROPBOX_TOKEN, "").toString();
    if (token == "") return false;

    // is the path set ?
    QString path = getSetting(GC_DROPBOX_FOLDER, "").toString();
    if (path == "") return false;

    // lets connect and get basic info on the root directory
    QString url("https://content.dropboxapi.com/2/files/upload");

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setRawHeader("Dropbox-API-Arg", (QString("{ \"path\": \"%1/%2\", \"mode\": \"overwrite\" }").arg(path).arg(remotename)).toLatin1());
    request.setRawHeader("Content-Type", "application/octet-stream");

    // put the file
    QNetworkReply *reply = nam->post(request, data);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply,remotename);
    return true;
}

void
Dropbox::writeFileCompleted()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
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
Dropbox::readyRead()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

void
Dropbox::readFileCompleted()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    notifyReadComplete(buffers.value(reply), replyName(reply), tr("Completed."));
}

static bool addDropbox() {
    CloudServiceFactory::instance().addService(new Dropbox(NULL));
    return true;
}

static bool add = addDropbox();
