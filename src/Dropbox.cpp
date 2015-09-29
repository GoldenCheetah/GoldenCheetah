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

QList<FileStoreEntry*> 
Dropbox::readdir(QString path, QStringList &errors)
{
    QList<FileStoreEntry*> returning;

    // do we have a token
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_DROPBOX_TOKEN, "").toString();
    if (token == "") {
        errors << "You must authorise with Dropbox first";
        return returning;
    }

    // lets connect and get basic info on the root directory
    QString url("https://api.dropboxapi.com/1/metadata/auto" + path + "?list=true");

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

            add->name = each.toObject()["path"].toString();
            add->isDir = each.toObject()["is_dir"].toBool();
            add->size = each.toObject()["bytes"].toInt();

            returning << add;
        }
    }

    // all good ?
    return returning;
}
