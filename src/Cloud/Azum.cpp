#include "Azum.h"
#include "Settings.h"
#include "Secrets.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QWebEngineView>

#ifndef AZUM_DEBUG
#define AZUM_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (AZUM_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (AZUM_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif


Azum::Azum(Context *context) : CloudService(context), context(context), root_(NULL)
{
    printd("Azum::Azum\n");
    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    // how is data uploaded and downloaded
    downloadCompression = none;
    useMetric = true; // distance and duration metadata

    // config
    settings.insert(OAuthToken, GC_AZUM_ACCESS_TOKEN);
    settings.insert(Local1, GC_AZUM_REFRESH_TOKEN);
    settings.insert(URL, GC_AZUM_URL);
    settings.insert(DefaultURL, "https://training.azum.com");
    settings.insert(AthleteID, GC_AZUM_ATHLETE_ID);
}

Azum::~Azum() {
    printd("Azum::~Azum\n");
    if (context) delete nam;
}

void
Azum::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    printd("Azum::onSslErrors\n");
    reply->ignoreSslErrors();
}

bool
Azum::open(QStringList &errors)
{
    printd("Azum::open\n");

    QString token = getSetting(GC_AZUM_ACCESS_TOKEN, "").toString();
    if (token == "") {
        errors << tr("There is no token");
        return false;
    }

    QString athleteId = getSetting(GC_AZUM_ATHLETE_ID, "").toString();
    if (athleteId == "") {
        errors << tr("There is no selected athlete");
        return false;
    }

    printd("Get access token for this session.\n");

    // refresh endpoint
    QString baseUrl = QString("%1/oauth/token/")
          .arg(getSetting(GC_AZUM_URL, "https://training.azum.com").toString());
    QNetworkRequest request(baseUrl);
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");

    // set params
    QString data;
    data += "client_id=" GC_AZUM_CLIENT_ID;
    data += "&client_secret=" GC_AZUM_CLIENT_SECRET;
    data += "&refresh_token=" + getSetting(GC_AZUM_REFRESH_TOKEN).toString();
    data += "&grant_type=refresh_token";

    // make request
    QNetworkReply* reply = nam->post(request, data.toLatin1());

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    printd("HTTP response code: %d\n", statusCode);

    if (reply->error() != 0) {
        printd("Got error %s\n", reply->errorString().toStdString().c_str());
        errors << reply->errorString();
        return false;
    }

    QByteArray r = reply->readAll();
    printd("Got response: %s\n", r.data());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        printd("Parse error!\n");
        errors << tr("JSON parser error") << parseError.errorString();
        return false;
    }

    QString access_token = document.object()["access_token"].toString();
    QString refresh_token = document.object()["refresh_token"].toString();

    // update our settings
    if (access_token != "") setSetting(GC_AZUM_ACCESS_TOKEN, access_token);
    if (refresh_token != "") setSetting(GC_AZUM_REFRESH_TOKEN, refresh_token);

    // get the factory to save our settings permanently
    CloudServiceFactory::instance().saveSettings(this, context);
    return true;
}

bool
Azum::close()
{
    printd("Azum::close\n");
    return true;
}

// home dire
QString
Azum::home()
{
    printd("Azum::home\n");
    return "";
}

bool
Azum::createFolder(QString)
{
    printd("Azum::createFolder\n");
    return false;
}


QList<CloudServiceEntry*>
Azum::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    printd("Azum::readdir(%s)\n", path.toStdString().c_str());
    QList<CloudServiceEntry*> returning;
    QString athleteId = getSetting(GC_AZUM_ATHLETE_ID, "").toString();
    if (athleteId == "") {
        errors << tr("No selected athlete");
        return returning;
    }

    QString token = getSetting(GC_AZUM_ACCESS_TOKEN, "").toString();
    qDebug() << "token" << token;
    QString baseUrl = QString("%1/api/goldencheetah/athletes/%2/activities/?")
          .arg(getSetting(GC_AZUM_URL, "https://training.azum.com").toString())
          .arg(athleteId);
    QUrlQuery params;
    params.addQueryItem("date_from", from.toString("yyyy-MM-dd"));
    params.addQueryItem("date_to", to.toString("yyyy-MM-dd"));
    QUrl next = QUrl(baseUrl + params.toString());

    do {
        QNetworkRequest request(next);
        request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
        QNetworkReply *reply = nam->get(request);

        // blocking request
        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        if (reply->error() != QNetworkReply::NoError) {
            errors << "Network Problem reading Nolio data";
            return returning;
        }

        // did we get a good response ?
        QByteArray r = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        if (parseError.error == QJsonParseError::NoError) {
            next = document.object()["next"].toString();
            QJsonArray activities = document.object()["results"].toArray();
            if (activities.count() > 0) {
                for (int i=0;i<activities.count();i++) {
                    QJsonObject activity = activities.at(i).toObject();
                    QString export_name = activity["export_name"].toString();
                    if (export_name == NULL) {
                        continue;
                    }
                    QString export_name_extension = QFileInfo(export_name).suffix();
                    qDebug() << "export_name_extension " << export_name_extension ;
                    CloudServiceEntry *add = newCloudServiceEntry();

                    add->id = QString("%1").arg(activity["id"].toString());
                    add->name = QDateTime::fromString(activity["start"].toString(), Qt::ISODate).toString("yyyy_MM_dd_HH_mm_ss")+"."+export_name_extension;
                    add->label = add->id;
                    add->distance = activity["distance"].toDouble() / 1000.0f; // m -> km
                    add->isDir = false;
                    // Duration return in the following format - 'P0DT00H25M57S' (https://en.wikipedia.org/wiki/ISO_8601#Durations)
                    const QRegExp rx(QLatin1String("[^0-9]+"));
                    const auto&& parts = activity["timer_time"].toString().split(rx, QString::SkipEmptyParts);
                    int sum = 0;
                    sum+= parts[0].toInt() * 24 * 60 * 60; // D - days
                    sum+= parts[1].toInt() * 60 * 60 ; // H - hours
                    sum+= parts[2].toInt() * 60; // M - minutes
                    sum+= parts[3].toInt(); // S - seconds
                    qDebug() << activity["timer_time"].toString() << " - " << sum;
                    add->duration = sum;

                    printd("item: %s %s %s %f %ld\n",
                           add->id.toStdString().c_str(),
                           add->name.toStdString().c_str(),
                           add->label.toStdString().c_str(),
                           add->distance,
                           add->duration
                    );
                    returning << add;
                }
            }
        }

    } while (!next.isEmpty());

    return returning;
}

bool
Azum::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("Azum::readFile(%s, %s)\n", remotename.toStdString().c_str(), remoteid.toStdString().c_str());
    QString url = QString("%1/api/goldencheetah/athletes/%2/activities/%3/export/")
          .arg(getSetting(GC_AZUM_URL, "https://training.azum.com").toString())
          .arg(getSetting(GC_AZUM_ATHLETE_ID, "").toString())
          .arg(remoteid);

    printd("url:%s\n", url.toStdString().c_str());

    QString token = getSetting(GC_AZUM_ACCESS_TOKEN, "").toString();

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


void
Azum::readyRead()
{
    printd("Azum::readRead\n");
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

void
Azum::readFileCompleted()
{
    printd("Azum::readFileCompleted\n");
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    notifyReadComplete(buffers.value(reply), replyName(reply), tr("Completed."));
}

QList<CloudServiceAthlete>
Azum::listAthletes()
{
    printd("Azum::listAthletes\n");
    QList<CloudServiceAthlete> returning;

    QString token = getSetting(GC_AZUM_ACCESS_TOKEN, "").toString();
    QString next = QString("%1/api/goldencheetah/athletes/")
          .arg(getSetting(GC_AZUM_URL, "https://training.azum.com").toString());

    qDebug() << "token: " << token;
    do {
        // request using csrf token + session id
        QNetworkRequest request(next);
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

        if (parseError.error == QJsonParseError::NoError) {
            next = document.object()["next"].toString();
            QJsonArray athletes = document.object()["results"].toArray();
            if (athletes.count()>0) {
                for (int i=0;i<athletes.count();i++) {
                    CloudServiceAthlete add;
                    QJsonObject athlete = athletes[i].toObject();
                    add.id = QString("%1").arg(athlete["user"].toInt());
                    add.name = athlete["full_name"].toString();
                    returning << add;
                }
            }
        } else {
            return returning;
        }
    } while (next != NULL);

    return returning;
}

bool
Azum::selectAthlete(CloudServiceAthlete athlete)
{
    printd("Azum::selectAthlete\n");
    setSetting(GC_AZUM_ATHLETE_ID, athlete.id.toInt());
    return true;
}

static bool addAzum() {
    CloudServiceFactory::instance().addService(new Azum(NULL));
    return true;
}

static bool add = addAzum();
