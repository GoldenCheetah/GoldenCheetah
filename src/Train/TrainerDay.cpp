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

#include "TrainerDay.h"
#include <QJsonDocument>
#include <QTextDocumentFragment>

#ifndef ERG_DEBUG
#define ERG_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (STRAVA_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (ERG_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

static const QString TrainerDayUrl = "https://shared-web.s3.amazonaws.com/ergdb/";

TrainerDay::TrainerDay(QObject *parent) : QObject(parent)
{
    getList(); // get all the files...
}

// fetch a list from TrainerDay
void
TrainerDay::getList()
{

    QEventLoop eventLoop; // holding pattern whilst waiting for a reply...
    QNetworkAccessManager networkMgr; // for getting request

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(getListFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));

    QByteArray out;

    // construct and make the call
    QNetworkRequest request = QNetworkRequest(QUrl(TrainerDayUrl + "json/public_workouts.json"));
    networkMgr.get(request);

    // holding pattern
    eventLoop.exec();
}

void
TrainerDay::getListFinished(QNetworkReply *reply)
{
    _items.clear();

    // we got something
    QByteArray r = reply->readAll();
    //printd("response: %s\n", r.toStdString().c_str());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    if (parseError.error == QJsonParseError::NoError) {

        QJsonArray results = document.array();

        // lets look at that then
        if (results.size()>0) {
            for(int i=0; i<results.size(); i++) {
                QJsonObject each = results.at(i).toObject();

                TrainerDayItem add;
                add.id = each["id"].toInt();
                // title is HTML encoded and may contain "/", lets try to make it a readable and valid filename
                add.name = QTextDocumentFragment::fromHtml(each["title"].toString()).toPlainText().replace("/", "-");
                QString type = each["user_wo_type"].toString();
                if (type != each["ergdb_wo_type"].toString())
                    type += "/"+each["ergdb_wo_type"].toString();
                add.workoutType = type;
                add.author = each["author"].toString();
                add.duration = each["minutes"].toString().toInt();
                add.added = QDateTime::fromString(each["create_date"].toString(), "yyyy-MM-ddT00:00:00:00");
                add.description = each["description"].toString();

                QJsonDocument doc(each);
                add.document = doc;

                _items << add;
            }
        }
    }



}

// fetch a file from TrainerDay
QString
TrainerDay::getWorkout(int id)
{
    fileContents = "";

    for(int i=0; i<_items.size(); i++) {
        TrainerDayItem each = _items.at(i);
        if (each.id == id) {
            fileContents = each.document.toJson(QJsonDocument::Indented);
        }
    }

    return fileContents;
}
