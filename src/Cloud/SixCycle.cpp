/*
 * Copyright (c) 2017 Mark Liversedge <liversedge@gmail.com>
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

#include "SixCycle.h"
#include "Athlete.h"
#include "Settings.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>


#ifndef SIXCYCLE_DEBUG
#define SIXCYCLE_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (SIXCYCLE_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (SIXCYCLE_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

SixCycle::SixCycle(Context *context) : CloudService(context), context(context), root_(NULL)
{
    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    // how is data uploaded and downloaded?
    uploadCompression = gzip;
    downloadCompression = none;
    filetype = CloudService::uploadType::TCX;
    session_token = ""; // not authenticated yet

    useMetric = true; // distance and duration metadata
    useEndDate = false; // startDateTime added, so no longer need this

    // config
    settings.insert(Username, GC_SIXCYCLE_USER);
    settings.insert(Password, GC_SIXCYCLE_PASS);
    settings.insert(URL, GC_SIXCYCLE_URL);
    settings.insert(DefaultURL, "https://live.sixcycle.com");
}

SixCycle::~SixCycle() {
    if (context) delete nam;
}

void
SixCycle::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

// open by connecting and getting a basic list of folders available
bool
SixCycle::open(QStringList &errors)
{
    printd("Sixcycle::open\n");

    // do we have a token
    QString user = getSetting(GC_SIXCYCLE_USER, "").toString();
    QString pass = getSetting(GC_SIXCYCLE_PASS, "").toString();

    if (user == "") {
        errors << "You must setup Sixcycle login details first";
        return false;
    }

    // use the configed URL
    QString url = QString("%1/rest-auth/login/")
          .arg(getSetting(GC_SIXCYCLE_URL, "https://live.sixcycle.com").toString());

    // We need to POST a form with the user and password
    QUrlQuery postData(url);
    postData.addQueryItem("email", user);
    postData.addQueryItem("password", pass);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,  "application/x-www-form-urlencoded");
    QNetworkReply *reply = nam->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    // blocking request - wait for response, timeout after 5 seconds
    QEventLoop loop;

    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QTimer::singleShot(5000,&loop, SLOT(quit())); // timeout after 5 seconds

    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "error" << reply->errorString();
        errors << tr("Network Problem authenticating with the Sixcycle service");
        return false;
    }

    // did we get a good response ?
    QByteArray r = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {
        printd("NoError\n");

        QJsonObject content = document.object();

        // extract the user root and the session_token
        QJsonValue key = content["key"];
        session_token = key.toString();
        printd("session token: %s\n", session_token.toStdString().c_str());

        QJsonValue user = content["user"];
        session_user = user.toString();
        printd("session user: %s\n", session_user.toStdString().c_str());

        // set root (there is no directory concept for SixCycle)
        root_ = newCloudServiceEntry();

        // path name
        root_->name = "/";
        root_->isDir = true;
        root_->size = 0;

        // happy with what we got ?
        if (root_->name != "/") errors << tr("invalid root path.");
        if (root_->isDir != true) errors << tr("root is not a directory.");

    } else {
        errors << tr("Problem with Sixcycle authentication.");
        printd("timed out, empty or invalid response.\n");
    }

    // ok so far ?
    if (errors.count()) return false;
    return true;
}

bool
SixCycle::close()
{
    printd("Sixcycle::close\n");
    // nothing to do for now
    return true;
}

// home dire
QString
SixCycle::home()
{
    return "";
}

bool
SixCycle::createFolder(QString)
{
    printd("Sixcycle::createFolder\n");

    return false;
}

QList<CloudServiceEntry*>
SixCycle::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    Q_UNUSED(from)
    Q_UNUSED(to)
    
    printd("Sixcycle::readdir(%s)\n", path.toStdString().c_str());

    QList<CloudServiceEntry*> returning;
    if (session_token == "") {
        errors << tr("You must authenticate with Sixcycle first");
        return returning;
    }

    // From the SixCycle docs:
    //
    // curl -X POST -H "Authorization: Token b635be6030e563fc74840bdac7811f4e994c7b61"
    //                 https://live.sixcycle.com/api/v1/activitysummaryfile/
    //
    // example responses:
    // [{"url":"https://s3.amazonaws.com/sixcycle/rawFiles/biking_bd372d69-ae61-4e08-accc-a08805cc67c7.fit",
    //   "dataCheckSum":"28352b42980ce8d09e18feeee027621c","activityEndDateTime":"2015-07-11T17:36:26Z","id":28684},
    //  {"url":"https://s3.amazonaws.com/sixcycle/rawFiles/com_20160229_105346_2016-02-29-06-37-38_06f16b19-7f1f-418a-8af4-2eb0869a9d57.fit",
    //   "dataCheckSum":"9052927da57b8fc44783e76322d30c2c","activityEndDateTime":"2016-02-29T12:31:10Z","id":28682},
    //  {"url":"https://s3.amazonaws.com/sixcycle/rawFiles/840_conordunne_2015-09-13-08-55-15Z_d44fb7c7-ed3a-4e66-9b6b-c6a782974c5b.pwx",
    //   "dataCheckSum":"6caf38aa1c0bb2101225680e05f5ece6","activityEndDateTime":"2015-09-10T15:44:54Z","id":35929}]
    //
    QString url = QString("%1/api/v1/activitysummaryfile/")
          .arg(getSetting(GC_SIXCYCLE_URL, "https://live.sixcycle.com").toString());

    printd("endpoint: %s\n", url.toStdString().c_str());

    // request using the session token
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,  "application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", (QString("Token %1").arg(session_token)).toLatin1());

    printd("session token: %s\n", session_token.toStdString().c_str());

    // pass the user id
    QUrlQuery postData(url);
    postData.addQueryItem("user", session_user);

    printd("user: %s\n", session_user.toStdString().c_str());

    // post the request
    reply = nam->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    // blocking request, with a 10 seconds timeout (might have a lot of data)
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QTimer::singleShot(10000,&loop, SLOT(quit())); // timeout after 10000 seconds

    loop.exec();

    // did we get a good response ?
    QByteArray r = reply->readAll();
    printd("response begins: %s ...\n", r.toStdString().substr(0,900).c_str());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {

        // results ?
        QJsonArray results = document.array();

        printd("items found: %lld\n", results.size());

        // lets look at that then
        for(int i=0; i<results.size(); i++) {

            QJsonObject each = results.at(i).toObject();
            CloudServiceEntry *add = newCloudServiceEntry();

            //SixCycle has full path, we just want the file name
            add->id = each["url"].toString();
            add->label = QString("%1").arg(each["id"].toInt());
            add->isDir = false;
            add->distance = double(each["meters_in_distance"].toInt()) / 1000.0f;
            add->duration = double(each["seconds_in_activity"].toInt());
            //add->size
            //add->modified

            //QString name = QDateTime::fromMSecsSinceEpoch(each["ts"].toDouble()).toString("yyyy_MM_dd_HH_mm_ss")+=".json";
            //add->name = name;
            QString dateString = each["activityStartDateTime"].toString();
            QString suffix = QFileInfo(add->id).suffix();

            // data might be compressed remotely, need to adapt to their scheme of naming
            // the rawfiles with the format at the front and compression at the back of the
            // filename; e.g:
            // https://s3.amazonaws.com/sixcycle/rawFiles/tcx_ba4319c2-a6dd-4503-9a50-0cb1f0f832ab.gz
            //
            if (suffix == "gz") {
                QString prefix = add->id.split("/").last().split("_").first();
                suffix = prefix + "." + suffix;
            }
            QDateTime startTime= QDateTime::fromString(dateString, Qt::ISODate).toLocalTime();
            QChar zero = QLatin1Char ( '0' );
            add->name = QString ( "%1_%2_%3_%4_%5_%6.%7" )
                                       .arg ( startTime.date().year(), 4, 10, zero )
                                       .arg ( startTime.date().month(), 2, 10, zero )
                                       .arg ( startTime.date().day(), 2, 10, zero )
                                       .arg ( startTime.time().hour(), 2, 10, zero )
                                       .arg ( startTime.time().minute(), 2, 10, zero )
                                       .arg ( startTime.time().second(), 2, 10, zero )
                                       .arg ( suffix );

            printd("entry: %s (%s) [%s]\n", add->name.toStdString().c_str(), add->label.toStdString().c_str(), dateString.toStdString().c_str());

            returning << add;
        }
    }

    // all good ?
    return returning;
}

// read a file at location (relative to home) into passed array
bool
SixCycle::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("Sixcycle::readFile(%s)\n", remotename.toStdString().c_str());

    if (session_token == "") {
        return false;
    }

    // this must be performed asyncronously and call made
    // to notifyReadComplete(QByteArray &data, QString remotename, QString message) when done

    // lets connect and get basic info on the root directory
    QString url = remoteid;

    printd("url:%s\n", url.toStdString().c_str());

    // request using the bearer token
    QNetworkRequest request(url);
    //request.setRawHeader("Authorization", (QString("Token %1").arg(session_token)).toLatin1());
    //request.setRawHeader("Accept-Encoding", "gzip, deflate");

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
SixCycle::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    Q_UNUSED(ride);

    printd("Sixcycle::writeFile(%s)\n", remotename.toStdString().c_str());

    // if we are called to upload a single ride we will not have been
    // authenticated yet, so lets try and do that now. When called via
    // the SyncDialog this will have already been done.
    if (session_token == "") {

        // we need to authenticate !
        QStringList errors;
        open(errors);

        // we didn't get a token?
        if (session_token == "") return false;
    }

    // Writing to SixCycle:
    //
    // curl -X POST -H "Authorization: Token b635be6030e563fc74840bdac7811f4e994c7b61" -F "rawFile=@running.fit" -F user="http://stg.sixcycle.com/api/v1/user/1174/" -v https://stg.sixcycle.com/api/v1/activitysummarydata/
    //
    // The -v tag means verbose and will provide all the protocol info. It should look something like this:
    //
    //> POST /api/v1/activitysummarydata/ HTTP/1.1
    //> Host: stg.sixcycle.com
    //> User-Agent: curl/7.50.3
    //> Accept: */*
    //> Authorization: Token b635be6030e563fc74840bdac7811f4e994c7b61
    //> Content-Length: 36969
    //> Expect: 100-continue
    //> Content-Type: multipart/form-data; boundary=------------------------24675cd48c8a2baf

    //The header content-type should be multipart/form-data and we have 2 fields, user and rawFile.
    //The user field is a url given in the login response on success.

    // set the target url
    QString url = QString("%1/api/v1/activitysummarydata/")
          .arg(getSetting(GC_SIXCYCLE_URL, "https://live.sixcycle.com").toString());
    //url = "http://requestb.in/1bakg1q1";
    printd("endpoint: '%s'\n", url.toStdString().c_str());

    // create a request passing the session token and user
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Token %1").arg(session_token)).toLatin1());

    // MULTIPART *****************
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // boundary is a random number
    QString boundary = QVariant(QRandomGenerator::global()->generate()).toString()+QVariant(QRandomGenerator::global()->generate()).toString()+QVariant(QRandomGenerator::global()->generate()).toString();
    multiPart->setBoundary(boundary.toLatin1());
    printd("boundary: '%s'\n", boundary.toStdString().c_str());

    // set the user this is for
    QHttpPart userPart;
    userPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    userPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QString("form-data; name=\"user\";"));
    userPart.setBody(QByteArray(session_user.toLatin1()));

    // second part is the file (rawFile)
    printd("user: %s\n", session_user.toStdString().c_str());
    printd("remotename: %s\n", remotename.toStdString().c_str());
    printd("data begins: %s ...\n", data.toStdString().substr(0,20).c_str());

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QString("form-data; name=\"rawFile\"; filename=\"%1\";").arg(remotename));
    filePart.setBody(data);

    multiPart->append(filePart);
    multiPart->append(userPart);

    // post the file
    reply = nam->post(request, multiPart);

    multiPart->setParent(reply);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply,remotename);

    return true;
}

void
SixCycle::writeFileCompleted()
{
    printd("Sixcycle::writeFileCompleted()\n");

    QString writestatus =  reply->readAll();

    printd("reply begins: %s ...\n", writestatus.toStdString().substr(0,80).c_str());

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
SixCycle::readyRead()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

void
SixCycle::readFileCompleted()
{
    printd("Sixcycle::readFileCompleted\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply begins:%s", buffers.value(reply)->toStdString().substr(0,80).c_str());

    // lets not override the date use an unformatted file name, keeping any .tcx.gz style suffix
    QString name = QString("temp.%1").arg(QFileInfo(replyName(reply)).completeSuffix());

    // process it then ........
    notifyReadComplete(buffers.value(reply), name, tr("Completed."));
}

static bool addSixCycle() {
    CloudServiceFactory::instance().addService(new SixCycle(NULL));
    return true;
}

static bool add = addSixCycle();
