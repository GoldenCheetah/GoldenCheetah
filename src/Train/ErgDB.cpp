/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#include "ErgDB.h"

static const QString ErgDBUrl = "http://www.73summits.com/ergdb/";

ErgDB::ErgDB(QObject *parent) : QObject(parent)
{
    getList(); // get all the files...
}

// fetch a list from ErgDB
void
ErgDB::getList()
{

    QEventLoop eventLoop; // holding pattern whilst waiting for a reply...
    QNetworkAccessManager networkMgr; // for getting request

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(getListFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
    QByteArray out;

    // construct and make the call
    QNetworkRequest request = QNetworkRequest(QUrl(ErgDBUrl + "api/list"));
    networkMgr.get(request);

    // holding pattern
    eventLoop.exec();
}

void
ErgDB::getListFinished(QNetworkReply *reply)
{
    _items.clear();

    // we got something
    QString line;
    bool first=true;
    while((line=reply->readLine().data()) != "") {

        if (first == true) { // skip csv header line
            first = false;
            continue;
        }

        QStringList tokens = line.split(",");
        if (tokens.count() == 6) {
            ErgDBItem add;
            add.id = tokens[0].toInt();
            add.workoutType = tokens[1];
            add.author = tokens[2];
            add.name = tokens[3];
            add.duration = tokens[4].toInt();
            add.added = QDateTime::fromString(tokens[5].replace("\n",""), "yyyy-MM-dd hh:mm:ss");
            _items << add;
        }
    }
}

// fetch a file from ErgDB
QString
ErgDB::getFile(int id, int ftp)
{

    QEventLoop eventLoop; // holding pattern whilst waiting for a reply...
    QNetworkAccessManager networkMgr; // for getting request

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(getFileFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
    QByteArray out;

    // construct and make the call
    QNetworkRequest request = QNetworkRequest(QUrl(ErgDBUrl + QString("api/workout/%1/%2").arg(id).arg(ftp)));
    networkMgr.get(request);

    // holding pattern
    eventLoop.exec();

    // return the file
    return fileContents;
}

void
ErgDB::getFileFinished(QNetworkReply *reply)
{
    fileContents = reply->readAll().data();
}
