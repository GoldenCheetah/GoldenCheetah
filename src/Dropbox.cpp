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

Dropbox::Dropbox(Context *context) : FileStore(context), context(context), root_(NULL) {
    nam = new QNetworkAccessManager(this);
}

Dropbox::~Dropbox() {
    delete nam;
}

// open by connecting and getting a basic list of folders available
bool
Dropbox::open(QStringList &errors)
{
    // do we have a token
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_DROPBOX_TOKEN, "").toString();
    if (token == "") {
        errors << "You must authorise with Dropbox first";
        return false;
    }

    // lets connect and get basic info on the root directory
    QString url("https://api.dropboxapi.com/1/metadata/auto/?list=false");

    // request using the bearer token
    QNetworkRequest request(url);
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

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {

        // we have a root
        root_ = newFileStoreEntry();

        // path name
        root_->name = document.object()["path"].toString();
        root_->isDir = document.object()["is_dir"].toBool();
        root_->size = document.object()["bytes"].toInt();

    }

    // happy with what we got ?
    if (root_->name != "/") errors << tr("invalid root path.");
    if (root_->isDir != true) errors << tr("root is not a directory.");

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
    return appsettings->cvalue(context->athlete->cyclist, GC_DROPBOX_FOLDER, "").toString(); 
}

bool Dropbox::createFolder(QString path)
{
    // do we have a token
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_DROPBOX_TOKEN, "").toString();
    if (token == "") return false;

    // lets connect and get basic info on the root directory
    QString url("https://api.dropboxapi.com/1/fileops/create_folder?root=auto&path=" + path);

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    QNetworkReply *reply = nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // 403 EXISTS, otherwise OK
    return (reply->error() == QNetworkReply::NoError);
}

QList<FileStoreEntry*> 
Dropbox::readdir(QString path, QStringList &errors)
{
    QList<FileStoreEntry*> returning;

    // do we have a token
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_DROPBOX_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with Dropbox first");
        return returning;
    }

    // lets connect and get basic info on the root directory
    QString url("https://api.dropboxapi.com/1/metadata/auto" + path + "?include_deleted=false&list=true");

    // request using the bearer token
    QNetworkRequest request(url);
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

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {

        // contents ?
        QJsonArray contents = document.object()["contents"].toArray();

        // lets look at that then
        for(int i=0; i<contents.size(); i++) {

            QJsonValue each = contents.at(i);
            FileStoreEntry *add = newFileStoreEntry();

            //dropbox has full path, we just want the file name
            add->name = QFileInfo(each.toObject()["path"].toString()).fileName();
            add->isDir = each.toObject()["is_dir"].toBool();
            add->size = each.toObject()["bytes"].toInt();

            // dates in format "Tue, 19 Jul 2011 21:55:38 +0000"
            add->modified = QDateTime::fromString(each.toObject()["modified"].toString().mid(0,25),
                               "ddd, dd MMM yyyy hh:mm:ss");

            returning << add;
        }
    }

    // all good ?
    return returning;
}

// read a file at location (relative to home) into passed array
bool
Dropbox::readFile(QByteArray *data, QString remotename)
{
    // this must be performed asyncronously and call made
    // to notifyReadComplete(QByteArray &data, QString remotename, QString message) when done

    // do we have a token ?
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_DROPBOX_TOKEN, "").toString();
    if (token == "") return false;

    // is the path set ?
    QString path = appsettings->cvalue(context->athlete->cyclist, GC_DROPBOX_FOLDER, "").toString();
    if (path == "") return false;

    // lets connect and get basic info on the root directory
    QString url("https://content.dropboxapi.com/1/files/auto/" + path + "/" + remotename);

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
Dropbox::writeFile(QByteArray &data, QString remotename)
{
    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done

    // do we have a token ?
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_DROPBOX_TOKEN, "").toString();
    if (token == "") return false;

    // is the path set ?
    QString path = appsettings->cvalue(context->athlete->cyclist, GC_DROPBOX_FOLDER, "").toString();
    if (path == "") return false;

    // lets connect and get basic info on the root directory
    QString url("https://content.dropboxapi.com/1/files_put/auto/" + path + "/" + remotename + "?overwrite=true&autorename=false");

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    // put the file
    QNetworkReply *reply = nam->put(request, data);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply,remotename);
    return true;
}

void
Dropbox::writeFileCompleted()
{
    notifyWriteComplete(replyName(static_cast<QNetworkReply*>(QObject::sender())), tr("Completed."));
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
