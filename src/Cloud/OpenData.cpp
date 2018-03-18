/*
 * Copyright (c) 2018 Mark Liversedge (liversedge@gmail.com)
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

#include "OpenData.h"
#include "Secrets.h"

#include <QUrl>
#include <QNetworkRequest>
#include <QHttpPart>
#include <QHttpMultiPart>

#include <QJsonDocument>
#include <QJsonArray>

#include "../qzip/zipwriter.h"

#ifndef OPENDATA_DEBUG
#define OPENDATA_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (OPENDATA_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (OPENDATA_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

OpenData::OpenData(Context *context) : context(context) {}

OpenData::~OpenData() {}

void
OpenData::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

// prepare and post data to opendata servers
void
OpenData::run()
{
    int step=0, last=5;
    printd("posting thread started\n");

    QNetworkAccessManager *nam = new QNetworkAccessManager(NULL);
    connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));

    // ----------------------------------------------------------------
    // STEP ONE: Get Servers List
    // ----------------------------------------------------------------
    step = 1;
    emit progress(step, last, tr("Fetching server list."));
    printd("fetching server list\n");

    QUrl serverlist(OPENDATA_SERVERSURL);
    QNetworkRequest request(serverlist);
    QNetworkReply *reply = nam->get(request);


    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        // how did it go?
        emit progress(step, 0, tr("Network Problem reading server list"));
        delete nam; quit();
    }

    // did we get a good response ?
    QByteArray r = reply->readAll();
    printd("response: %s\n", r.toStdString().c_str());

    // get server list
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // find a server thats responding, if cannot find then server
    // will still be an empty string when done
    if (parseError.error != QJsonParseError::NoError) {
        emit progress(step, 0, tr("Invalid server list, please try again later"));
        printd("invalid server list!");
        delete nam; quit();
    }

    // ----------------------------------------------------------------
    // STEP TWO: Find available server
    // ----------------------------------------------------------------
    step=2;
    emit progress(step, last, tr("Finding an available server."));
    printd("finding server\n");

    // results ?
    QJsonObject doc = document.object();
    QJsonArray results = doc["SERVERS"].toArray();
    QString server="";

    // lets look at that then
    if (results.size()>0) {

        for(int i=0; i<results.size(); i++) {
            QJsonObject each = results.at(i).toObject();

            // get count of files at server as a ping test
            QString trying = each["url"].toString();
            QUrl ping(trying + "metrics");
            QNetworkRequest tryserver(ping);

            // ping the server
            QNetworkReply *reply = nam->get(tryserver);

            // blocking request
            connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
            loop.exec();

            // responded?
            if (reply->error() == QNetworkReply::NoError) {
                server = trying;
                #if OPENDATA_DEBUG
                QByteArray r = reply->readAll();
                printd("server ping: %s\n", r.toStdString().c_str());
                #endif
                break;
            } else {
                printd("ping %s failed\n", trying.toStdString().c_str());
            }
        }
    }

    // servers not available
    if (server == "") {
        printd("Failed to find an available server\n");
        emit progress(step, 0, tr("No servers available, please try later."));
        delete nam; quit();
    }
    printd("found a server to post to: %s\n", server.toStdString().c_str());

    // ----------------------------------------------------------------
    // STEP THREE: Prepare and compress the data to send
    // ----------------------------------------------------------------
    step=3;
    emit progress(step, last, tr("Preparing data to send."));
    printd("prepping data\n");

    // lots of files and memory - sure this could be a lot cleaner
    // code, but for now, it works ok albeit with lots of overhead.
    QByteArray postingdata;

    // write OPENDATA to JSON
    QTemporaryFile tempfile;
    tempfile.open();
    tempfile.close();
    context->athlete->rideCache->save(true, tempfile.fileName());

    // Read JSON into MEMORY and write to ZIP
    QFile jsonFile(tempfile.fileName());
    jsonFile.open(QFile::ReadOnly);
    QTemporaryFile zipFile;
    zipFile.open();
    zipFile.close();
    ZipWriter writer(zipFile.fileName());
    QString zipname = context->athlete->id.toString() + ".json";
    writer.addFile(zipname, jsonFile.readAll());
    jsonFile.close();
    writer.close();

    // read the ZIP into MEMORY
    QFile zip(zipFile.fileName());
    zip.open(QFile::ReadOnly);
    postingdata = zip.readAll();
    zip.close();

    printd("Compressed to %s, size %d\n", zipFile.fileName().toStdString().c_str(), postingdata.size());

    // ----------------------------------------------------------------
    // STEP FOUR: Sending data to server
    // ----------------------------------------------------------------
    step=4;
    emit progress(step, last, tr("Sending data to server."));
    printd("sending data\n");

    QUrl serverpost(server + "metrics");
    QNetworkRequest post(serverpost);

    // send as multipart form data - "secret" field, "id" field and "data" file
    QHttpMultiPart form (QHttpMultiPart::FormDataType);
    QString boundary = QVariant(qrand()).toString() + QVariant(qrand()).toString() + QVariant(qrand()).toString();
    form.setBoundary(boundary.toLatin1());

    QHttpPart secretpart;
    secretpart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"secret\""));
    QString secret(GC_CLOUD_OPENDATA_SECRET);
    secretpart.setBody(secret.toLatin1());
    form.append(secretpart);

    QHttpPart useridpart;
    useridpart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"id\""));
    useridpart.setBody(context->athlete->id.toString().toLatin1());
    form.append(useridpart);

    QHttpPart filepart;
    filepart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/xml"));
    filepart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"data\"; filename=\""+context->athlete->id.toString()+".zip\"; type=\"application/zip\""));
    filepart.setBody(postingdata);
    form.append(filepart);

    // post it
    reply = nam->post(post, &form);
    printd("Posting to server...");

    // blocking request
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // success?
    if (reply->error() == QNetworkReply::NoError) {
        printd("post success %s\n", r.toStdString().c_str());
    } else {
        QByteArray r = reply->readAll();
        printd("post failed %s\n", r.data());
        emit progress(step, 0, QString("%s: %s").arg(tr("Server replied:")).arg(r.data()));
        delete nam; quit();
    }

    // ----------------------------------------------------------------
    // STEP FIVE: All done
    // ----------------------------------------------------------------
    step=5;
    emit progress(step, last, tr("Finishing."));
    printd("cleanup\n");

    // and terminate
    emit progress(0, last, tr("Done"));

    printd("terminate thread\n");
    delete nam;
    quit();
}
