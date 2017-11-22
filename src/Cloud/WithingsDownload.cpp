/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "WithingsDownload.h"
#include "WithingsReading.h"
#include "MainWindow.h"
#include "Athlete.h"
#include "RideCache.h"
#include "Secrets.h"
#include "BodyMeasures.h"
#include <QMessageBox>

#ifdef GC_HAVE_KQOAUTH
#include <kqoauthmanager.h>
#include <kqoauthrequest.h>
#endif

#ifndef WITHINGS_DEBUG
#define WITHINGS_DEBUG true
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (WITHINGS_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (WITHINGS_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

WithingsDownload::WithingsDownload(Context *context) : context(context)
{
    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));

    #ifdef GC_HAVE_KQOAUTH
    oauthRequest = new KQOAuthRequest();
    oauthManager = new KQOAuthManager();

    connect(oauthManager, SIGNAL(authorizedRequestDone()),
            this, SLOT(onAuthorizedRequestDone()));
    connect(oauthManager, SIGNAL(requestReady(QByteArray)),
            this, SLOT(onRequestReady(QByteArray)));

    #endif
}

bool
WithingsDownload::getBodyMeasures(QString &error, QDateTime from, QDateTime to, QList<BodyMeasure> &data)
{
    response = "";
    // New API (OAuth)
    QString strToken = "";
    QString strSecret = "";

    QString strNokiaToken = "";
    QString strNokiaRefreshToken = "";

    QString access_token = "";

    #ifdef GC_HAVE_KQOAUTH
    strToken = appsettings->cvalue(context->athlete->cyclist, GC_WITHINGS_TOKEN).toString();
    strSecret= appsettings->cvalue(context->athlete->cyclist, GC_WITHINGS_SECRET).toString();
    #endif

    strNokiaToken = appsettings->cvalue(context->athlete->cyclist, GC_NOKIA_TOKEN).toString();
    strNokiaRefreshToken = appsettings->cvalue(context->athlete->cyclist, GC_NOKIA_REFRESH_TOKEN).toString();


    QString strOldKey = appsettings->cvalue(context->athlete->cyclist, GC_WIKEY).toString();


    if((strToken.isEmpty() || strSecret.isEmpty() ||
       strToken == "" || strToken == "0" ||
       strSecret == "" || strSecret == "0" ) &&
       (strOldKey.isEmpty() || strOldKey == "" || strOldKey == "0" ) &&
       (strNokiaRefreshToken.isEmpty() || strNokiaRefreshToken == "" || strNokiaRefreshToken == "0" ))
    {
        #ifdef Q_OS_MACX
        #define GC_PREF tr("Golden Cheetah->Preferences")
        #else
        #define GC_PREF tr("Tools->Options")
        #endif
        QString advise = QString(tr("Error fetching OAuth credentials.  Please make sure to complete the Withings authorization procedure found under %1.")).arg(GC_PREF);
        QMessageBox oautherr(QMessageBox::Critical, tr("OAuth Error"), advise);
        oautherr.exec();
        return false;
    }

    if(!strNokiaRefreshToken.isEmpty() ||
           (!strToken.isEmpty() &&! strSecret.isEmpty() &&
            strToken != "" && strToken != "0" &&
            strSecret != "" && strSecret != "0" )) {
        printd("OAuth 2.0 API\n");

#if QT_VERSION > 0x050000
        QUrlQuery postData;
#else
        QUrl postData;
#endif

        QString refresh_token = appsettings->cvalue(context->athlete->cyclist, GC_NOKIA_REFRESH_TOKEN).toString();
        if (refresh_token.isEmpty())
            refresh_token = QString("%1:%2").arg(strToken).arg(strSecret);

        postData.addQueryItem("grant_type", "refresh_token");
        postData.addQueryItem("client_id", GC_NOKIA_CLIENT_ID );
        postData.addQueryItem("client_secret", GC_NOKIA_CLIENT_SECRET );
        postData.addQueryItem("refresh_token", refresh_token );

        QUrl url = QUrl( "https://account.withings.com/oauth2/token" );

        emit downloadStarted(100);

        //oauthManager->executeRequest(oauthRequest);
        QNetworkRequest request(url);
        request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
        nam->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
        printd("url %s %s\n", url.toString().toStdString().c_str(), postData.toString().toStdString().c_str());

        // blocking request
        loop.exec(); // we go on after receiving the data in SLOT(onRequestReady(QByteArray))

        printd("response: %s\n", response.toStdString().c_str());


        if (response.contains("\"access_token\"", Qt::CaseInsensitive))
        {
                QJsonParseError parseResult;
                QJsonDocument migrateJson = QJsonDocument::fromJson(response.toUtf8(), &parseResult);

                access_token = migrateJson.object()["access_token"].toString();
                QString refresh_token = migrateJson.object()["refresh_token"].toString();
                QString userid = QString("%1").arg(migrateJson.object()["userid"].toInt());


                if (access_token != "") appsettings->setCValue(context->athlete->cyclist, GC_NOKIA_TOKEN, access_token);
                if (refresh_token != "") appsettings->setCValue(context->athlete->cyclist, GC_NOKIA_REFRESH_TOKEN, refresh_token);
                if (userid != "") appsettings->setCValue(context->athlete->cyclist, GC_WIUSER, userid);


            #if QT_VERSION > 0x050000
                QUrlQuery params;
            #else
                QUrl params;
            #endif

                emit downloadStarted(100);

                params.addQueryItem("action", "getmeas");
                //params.addQueryItem("userid", userid);
                params.addQueryItem("access_token", access_token);
                params.addQueryItem("startdate", QString::number(from.toMSecsSinceEpoch()/1000));
                params.addQueryItem("enddate", QString::number(to.toMSecsSinceEpoch()/1000));


                QUrl url = QUrl( "https://api.health.nokia.com/measure?" + params.toString() );

                printd("URL: %s\n", url.url().toStdString().c_str());

                QNetworkRequest request(url);
                //request.setRawHeader("Authorization", QString("Bearer %1").arg(access_token).toLatin1());

                nam->get(request);

                emit downloadProgress(50);

                // blocking request
                loop.exec(); // we go on after receiving the data in SLOT(onRequestReady(QByteArray))

                emit downloadEnded(100);

        }

    }

    if(access_token.isEmpty() && !strToken.isEmpty() &&! strSecret.isEmpty() &&
            strToken != "" && strToken != "0" &&
            strSecret != "" && strSecret != "0" ) {
        printd("OAuth 1.0 API\n");

        #ifdef GC_HAVE_KQOAUTH
        oauthRequest->initRequest(KQOAuthRequest::AuthorizedRequest, QUrl("http://wbsapi.withings.net/measure"));
        oauthRequest->setHttpMethod(KQOAuthRequest::GET);
        //oauthRequest->setEnableDebugOutput(true);

        oauthRequest->setConsumerKey(GC_WITHINGS_CONSUMER_KEY);
        oauthRequest->setConsumerSecretKey(GC_WITHINGS_CONSUMER_SECRET);

        // set the user token and secret
        oauthRequest->setToken(strToken);
        oauthRequest->setTokenSecret(strSecret);

        KQOAuthParameters params;
        params.insert("action", "getmeas");
        params.insert("userid", appsettings->cvalue(context->athlete->cyclist, GC_WIUSER, "").toString());
        params.insert("startdate", QString::number(from.toMSecsSinceEpoch()/1000));
        params.insert("enddate", QString::number(to.toMSecsSinceEpoch()/1000));

        oauthRequest->setAdditionalParameters(params);

        // Hack...
        // Why should we add params manually (GET) ????
        // We can use KQOAuth because Nokia/Withings expect token in url.

        QList<QByteArray> requestParameters = oauthRequest->requestParameters();

#if QT_VERSION > 0x050000
        QUrlQuery params2;
#else
        QUrl params2;
#endif
        for (int i=0; i<requestParameters.count(); i++) {
            QString _rp = requestParameters.at(i);
            _rp = _rp;

            QString key = _rp.left(_rp.indexOf("="));
            QString value = _rp.right(_rp.length()-key.length()-1).replace("\"", "");

            params2.addQueryItem(key, value);
        }
        params2.addQueryItem("action", "getmeas");
        params2.addQueryItem("userid", appsettings->cvalue(context->athlete->cyclist, GC_WIUSER, "").toString());
        params2.addQueryItem("startdate", QString::number(from.toMSecsSinceEpoch()/1000));
        params2.addQueryItem("enddate", QString::number(to.toMSecsSinceEpoch()/1000));

        QUrl url = QUrl( "https://wbsapi.withings.net/measure?" + params2.toString() );
        printd("URL : %s\n", url.url().toStdString().c_str());

        emit downloadStarted(100);

        //oauthManager->executeRequest(oauthRequest);
        QNetworkRequest request(url);
        nam->get(request);

        emit downloadProgress(50);

        // blocking request
        loop.exec(); // we go on after receiving the data in SLOT(onRequestReady(QByteArray))

        emit downloadEnded(100);
        #endif
    } else if (access_token.isEmpty()) {
        printd("Withings password API\n");

        // account for trailing slash, remove it if it is there (it was the default in preferences)
        QString server = appsettings->cvalue(context->athlete->cyclist, GC_WIURL, "http://wbsapi.withings.net").toString();
        if (server.endsWith("/")) server=server.mid(0, server.length()-1);

        QString request = QString("%1/measure?action=getmeas&userid=%2&publickey=%3&startdate=%4&enddate=%5")
                                 .arg(server)
                                 .arg(appsettings->cvalue(context->athlete->cyclist, GC_WIUSER, "").toString())
                                 .arg(appsettings->cvalue(context->athlete->cyclist, GC_WIKEY, "").toString())
                                 .arg(QString::number(from.toMSecsSinceEpoch()/1000))
                                 .arg(QString::number(to.toMSecsSinceEpoch()/1000));

        emit downloadStarted(100);
        QNetworkReply *reply = nam->get(QNetworkRequest(QUrl(request)));

        emit downloadProgress(50);
        // blocking request
        loop.exec(); // we go on after receiving the data in SLOT(downloadFinished(QNetworkReply))

        emit downloadEnded(100);
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(context->mainWindow, tr("Nokia Health (Withings) Data Download"), reply->errorString());
            return false;
        }
    }

    printd("response: %s\n", response.toStdString().c_str());

    QJsonParseError parseResult;
    if (response.contains("\"status\":0", Qt::CaseInsensitive))
    {
    		parseResult = parse(response, data);
    } else {
        QMessageBox oautherr(QMessageBox::Critical, tr("Error"),
                             tr("There was an error during fetching. Please check the error description."));
        oautherr.setDetailedText(response); // probably blank
        oautherr.exec();
        return false;

    }

    if (QJsonParseError::NoError != parseResult.error) {
        QMessageBox oautherr(QMessageBox::Critical, tr("Error"),
                             tr("Error parsing Withings API response. Please check the error description."));
        QString errorStr = parseResult.errorString() + " at offset " + QString::number(parseResult.offset);
        error = errorStr.simplified();
        oautherr.setDetailedText(error);
        oautherr.exec();
        return false;
    };
    return true;
}


QJsonParseError
WithingsDownload::parse(QString text, QList<BodyMeasure> &bodyMeasures)
{

    QJsonParseError parseResult;
	QJsonDocument withingsJson = QJsonDocument::fromJson(text.toUtf8(), &parseResult);
	QList<WithingsReading> readings = jsonDocumentToWithingsReading(withingsJson);

    if (parseResult.error != QJsonParseError::NoError) {
    		return parseResult;
    }

    // convert from Withings to general format
    foreach(WithingsReading r, readings) {
        BodyMeasure w;
        // we just take
        if (r.weightkg > 0 && r.when.isValid()) {
            w.when =  r.when;
            w.comment = r.comment;
            w.weightkg = r.weightkg;
            w.fatkg = r.fatkg;
            w.leankg = r.leankg;
            w.fatpercent = r.fatpercent;
            w.source = BodyMeasure::Withings;
            bodyMeasures.append(w);
        }
    }

    return parseResult;

}

QList<WithingsReading>
WithingsDownload::jsonDocumentToWithingsReading(QJsonDocument doc) {
    QList<WithingsReading> readings = QList<WithingsReading>();

    //Get the array of measurement groups
    QJsonArray jMeasureGrpsArr = doc.object()["body"].toObject()["measuregrps"].toArray();

    //Iterate the measurement groups
    for (int i = 0; i < jMeasureGrpsArr.size(); i++) {

        QJsonObject jThisMeasureGroup = jMeasureGrpsArr[i].toObject();
        QJsonArray jMeasuresArr = jThisMeasureGroup["measures"].toArray();

        //Get the context for this readings
        int grpid = jThisMeasureGroup["grpid"].toInt();
        int attrib = jThisMeasureGroup["attrib"].toInt();
        int category = jThisMeasureGroup["category"].toInt();
        int date = jThisMeasureGroup["date"].toInt();
        QString comment = jThisMeasureGroup["comment"].toString();

        WithingsReading thisReading = WithingsReading();
        thisReading.groupId = grpid;
        thisReading.attribution = attrib;
        thisReading.category = category;
        thisReading.when.setTime_t(date);
        thisReading.comment = comment;

        //Iterate the individual measurements in each group to create a WithingsReading object
        for (int j = 0; j < jMeasuresArr.size(); j++) {

            QJsonObject jMeasure = jMeasuresArr[j].toObject();
            int unscaledValue = jMeasure["value"].toInt();
            int type = jMeasure["type"].toInt();
            int unit = jMeasure["unit"].toInt();

            double value = (double)unscaledValue * pow(10.00, unit);
            switch (type) {
                case 1 : thisReading.weightkg = value; break;
                case 4 : thisReading.sizemeter = value; break;
                case 5 : thisReading.leankg = value; break;
                case 6 : thisReading.fatpercent = value; break;
                case 8 : thisReading.fatkg = value; break;
                default: break;
            }
        }
        readings << thisReading;
    }
    return readings;
}

// SLOTs for asynchronous calls

void
WithingsDownload::downloadFinished(QNetworkReply *reply)
{
    response = reply->readAll();
    loop.exit(0);
}


#ifdef GC_HAVE_KQOAUTH
void
WithingsDownload::onAuthorizedRequestDone() {
    // printd("Request sent to Withings!\n");
}

void
WithingsDownload::onRequestReady(QByteArray r) {
    //printd("Response from the Withings's service: %s\n", response..toStdString().c_str());

    response = r;
    loop.exit(0);

}
#endif
