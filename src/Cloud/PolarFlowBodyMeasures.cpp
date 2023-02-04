/*
 * Copyright (c) 2017 Joern Rischmueller (joern.rm@gmail.com)
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

#include "PolarFlowBodyMeasures.h"
#include "Settings.h"
#include "Athlete.h"
#include "PolarFlow.h"
#include "CloudService.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkReply>

#ifndef POLARFLOWBODYMEASURES_DEBUG
// TODO(gille): This should be a command line flag.
#define POLARFLOWBODYMEASURES_DEBUG true //true|false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (POLARFLOWBODYMEASURES_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (POLARFLOWBODYMEASURES_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

PolarFlowBodyMeasures::PolarFlowBodyMeasures(Context *context) : context(context) {

    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));

}

PolarFlowBodyMeasures::~PolarFlowBodyMeasures() {
    delete nam;
}


void
PolarFlowBodyMeasures::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

bool
open(QStringList &errors)
{
    printd("open - start\n");
    // Command-URL: POST https://www.polaraccesslink.com/v3/users/{user-id}/physical-information-transactions
      QString url = QString("%1/v3/users/%2/physical-information-transactions")
              .arg(GC_POLARFLOW_URL)//.toStdString())
              .arg(GC_POLARFLOW_USER_ID);//.toString());

}

bool
PolarFlowBodyMeasures::getBodyMeasures(QString &error, QDateTime from, QDateTime to, QList<Measure> &data) {

    // do we have a token
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_POLARFLOW_TOKEN, "").toString();
    if (token == "") {
        error = tr("You must authorise with Polar Flow first");
        return false;
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

        url = QString("%1/rest/users/day/%2/%3/%4")
                .arg(appsettings->cvalue(context->athlete->cyclist, GC_POLARFLOW_URL, "https://whats.todaysplan.com.au").toString())
                .arg(searchCommand)
                .arg(QString::number(offset))
                .arg(QString::number(pageSize));;

        // request using the bearer token
        QNetworkRequest request(url);
        QNetworkReply *reply;
        request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
        if (offset == 0) {

            // Prepare the Search Payload for First Call to Search
            QString userId = appsettings->cvalue(context->athlete->cyclist, GC_POLARFLOW_USER_ID, "").toString();
            // application/json
            QByteArray jsonString;
            jsonString += "{\"criteria\": { ";
            if (userId.length()>0)
                jsonString += " \"userIds\": [ "+ QString("%1").arg(userId) +" ], ";
            jsonString += "\"ranges\": [ {";
            jsonString += "\"floorTs\": \""+ QString("%1").arg(from.toMSecsSinceEpoch()) +"\", ";
            jsonString += "\"ceilTs\": \"" + QString("%1").arg(to.addDays(1).addSecs(-1).toMSecsSinceEpoch()) +"\"";
            jsonString += "} ] }, ";
            jsonString += "\"fields\": [\"att.ts\",\"att.weight\", \"att.fat\",\"att.muscleMass\",\"att.boneMass\",\"att.fatMass\", \"att.height\" , \"att.source\"] ";
            jsonString += "}";

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

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        // if path was returned all is good, lets set root
        if (parseError.error == QJsonParseError::NoError) {

            // number of Result Items
            if (offset == 0) {
                resultCount = document.object()["cnt"].toInt();
                emit downloadStarted(resultCount);
            }

            // results ?
            QJsonObject result = document.object()["result"].toObject();
            QJsonArray results = result["results"].toArray();

            // lets look at that then
            for(int i=0; i<results.size(); i++) {
                QJsonObject each = results.at(i).toObject();
                // check if we have attributes
                if (!each.contains("atts")) continue;
                // we have an array of attributes for a day
                QJsonArray atts = each["atts"].toArray();
                for (int j=0; j<atts.size(); j++) {
                    QJsonObject record = atts.at(j).toObject();
                    if (record.contains("ts") && record.contains("weight")) {
                        Measure add;
                        add.when = QDateTime::fromMSecsSinceEpoch(record["ts"].toDouble());
                        add.values[Measure::WeightKg] = record["weight"].toDouble();
                        add.values[Measure::BonesKg] = record["boneMass"].toDouble();
                        add.values[Measure::FatKg] = record["fatMass"].toDouble();
                        add.values[Measure::MuscleKg] = record["muscleMass"].toDouble();
                        add.values[Measure::FatPercent] = record["fat"].toDouble();
                        add.originalSource = record["source"].toString();
                        add.source = Measure::TodaysPlan;
                        data.append(add);
                    }

                }
            }
            // next page
            offset += pageSize;
            emit downloadProgress(offset);
        } else {
            // we had a parsing error - so something is wrong - stop requesting more data
            error = tr("Response parsing error: %1").arg(parseError.errorString());
            return false;
        }
    }
    // all good
    emit downloadEnded(resultCount);
    return true;
}




