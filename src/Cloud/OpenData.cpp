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
#include "Settings.h"
#include "Secrets.h"
#include "Colors.h"

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
void OpenData::onSslErrors(QNetworkReply *reply, const QList<QSslError>&) { reply->ignoreSslErrors(); }

// check if its time to ask or send data
void
OpenData::check(Context *context)
{
    // if already said no then bail
    QString granted = appsettings->cvalue(context->athlete->cyclist, GC_OPENDATA_GRANTED, "X").toString();
    if (granted == "N") return;

    // get and increment start count
    int startcount = appsettings->cvalue(context->athlete->cyclist, GC_OPENDATA_RUNCOUNT, 0).toInt()+1;
    appsettings->setCValue(context->athlete->cyclist, GC_OPENDATA_RUNCOUNT, startcount);

    // time to ask permission...
    if (granted == "X" && startcount > 10)  {
        printd("need to ask\n");
        OpenDataDialog *ask = new OpenDataDialog(context);
        ask->exec(); // will ask and post
    }

    // may have changed !
    granted = appsettings->cvalue(context->athlete->cyclist, GC_OPENDATA_GRANTED, "X").toString();
    QDate lastpost = appsettings->cvalue(context->athlete->cyclist, GC_OPENDATA_LASTPOSTED, QDate(1970,01,01)).toDate();
    if (granted == "Y" && lastpost.daysTo(QDate::currentDate()) > 365) {
        // might be time, but lets just check we have new workouts
        int newworkouts = context->athlete->rideCache->count() -
                          appsettings->cvalue(context->athlete->cyclist, GC_OPENDATA_LASTPOSTCOUNT, 0).toInt();

        // we need AT LEAST 100 new workouts to make this worthwhile
        if (newworkouts > 100) {
            OpenData *od = new OpenData(context); // no mem leak as a QThread that terminates itself
            od->postData();
            printd("sending new workouts\n");
        }
    }
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

    // record the fact we sent some stuff
    appsettings->setCValue(context->athlete->cyclist, GC_OPENDATA_LASTPOSTED, QDate::currentDate());
    appsettings->cvalue(context->athlete->cyclist, GC_OPENDATA_LASTPOSTCOUNT,  context->athlete->rideCache->count());

    // and terminate
    emit progress(0, last, tr("Done"));

    printd("terminate thread\n");
    delete nam;
    quit();
}

OpenDataDialog::OpenDataDialog(Context *context) : context(context)
{

    setWindowTitle(QString(tr("OpenData")));
    setMinimumWidth(550);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QPushButton *important = new QPushButton(style()->standardIcon(QStyle::SP_MessageBoxInformation), "", this);
    important->setFixedSize(80*dpiXFactor,80*dpiYFactor);
    important->setFlat(true);
    important->setIconSize(QSize(80*dpiXFactor,80*dpiYFactor));
    important->setAutoFillBackground(false);
    important->setFocusPolicy(Qt::NoFocus);

    QLabel *header = new QLabel(this);
    header->setWordWrap(true);
    header->setTextFormat(Qt::RichText);
    header->setText(QString(tr("<b><big>Share aggregated data</big></b>")));

    QHBoxLayout *toprow = new QHBoxLayout;
    toprow->addWidget(important);
    toprow->addWidget(header);
    layout->addLayout(toprow);

    QLabel *text = new QLabel(this);
    text->setWordWrap(true);
    text->setTextFormat(Qt::RichText);
    text->setText(tr("<center><b>Your chance to give back</b><br><p></center>"
                     "We have started a new project to collect user workout data, in aggregate format, to enable "
                     "researchers, coaches and others to develop new models and solutions.<p>"
                     "All data shared is anonymous and cannot be traced back to the original user, no personal data is "
                     "shared and the workout data is limited to metrics and distributions. No sample by sample data is being "
                     "collected at all.<p>"
                     "The data will be published to the general public via github every quarter, in exactly the same format "
                     "you have provided it in.<p>"
                     "<b>WE WILL NOT</b>:<p>"
                     "- Collect personal information <p>"
                     "- Collect GPS information <p>"
                     "- Collect notes or other metadata  <p>"
                     "- Collect sample data <p>"
                     "<p>"
                     "<b>WE WILL</b>:<p>"
                     "- Collect basic athlete info: Gender, Year of Birth and UUID<p>"
                     "- Collect metrics for every activity stored for this athlete<p>"
                     "- Collect distribution and mean-max aggregates of activity data<p>"
                     ));

    scrollText = new QScrollArea();
    scrollText->setWidget(text);
    scrollText->setWidgetResizable(true);
    layout->addWidget(scrollText);

    QHBoxLayout *lastRow = new QHBoxLayout;

    proceedButton = new QPushButton(tr("Yes, I want to share"), this);
    proceedButton->setEnabled(true);
    connect(proceedButton, SIGNAL(clicked()), this, SLOT(acceptConditions()));
    abortButton = new QPushButton(tr("No thanks"), this);
    abortButton->setDefault(true);
    connect(abortButton, SIGNAL(clicked()), this, SLOT(rejectConditions()));

    lastRow->addWidget(abortButton);
    lastRow->addStretch();
    lastRow->addWidget(proceedButton);
    layout->addLayout(lastRow);

}

void OpenDataDialog::acceptConditions() {

    // document the decision
    appsettings->setCValue(context->athlete->cyclist, GC_OPENDATA_GRANTED, "Y");
    accept();
}

void OpenDataDialog::rejectConditions() {

    // document the decision
    appsettings->setCValue(context->athlete->cyclist, GC_OPENDATA_GRANTED, "N");
    reject();
}
