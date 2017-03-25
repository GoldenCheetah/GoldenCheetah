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

WithingsDownload::WithingsDownload(Context *context) : context(context)
{
    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
    parser = new WithingsParser;

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
    #ifdef GC_HAVE_KQOAUTH
    strToken =appsettings->cvalue(context->athlete->cyclist, GC_WITHINGS_TOKEN).toString();
    strSecret= appsettings->cvalue(context->athlete->cyclist, GC_WITHINGS_SECRET).toString();
    #endif

    QString strOldKey = appsettings->cvalue(context->athlete->cyclist, GC_WIKEY).toString();


    if((strToken.isEmpty() || strSecret.isEmpty() ||
       strToken == "" || strToken == "0" ||
       strSecret == "" || strSecret == "0" ) &&
       (strOldKey.isEmpty() || strOldKey == "" || strOldKey == "0" ))
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

    if(!strToken.isEmpty() &&! strSecret.isEmpty() &&
            strToken != "" && strToken != "0" &&
            strSecret != "" && strSecret != "0" ) {
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

        // Hack...
        // Why should we add params manually (GET) ????
        QList<QByteArray> requestParameters = oauthRequest->requestParameters();

        for (int i=0; i<requestParameters.count(); i++) {
            //qDebug() << requestParameters.at(i);
#if QT_VERSION > 0x050000
            QUrlQuery _params;
            _params.setQuery(requestParameters.at(i));
#else
            QUrl _params;
            _params.setEncodedQuery(requestParameters.at(i));
#endif
            QString value = _params.queryItems().at(0).second;
            value = value.replace("\"", "");
            params.insert(_params.queryItems().at(0).first, value);

        }

        oauthRequest->setAdditionalParameters(params);

        emit downloadStarted(100);
        oauthManager->executeRequest(oauthRequest);
        emit downloadProgress(50);

        // blocking request
        loop.exec(); // we go on after receiving the data in SLOT(onRequestReady(QByteArray))

        emit downloadEnded(100);
        #endif
    } else  {

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
            QMessageBox::warning(context->mainWindow, tr("Withings Data Download"), reply->errorString());
            return false;
        }
    }

    QStringList errors;
    if (response.contains("\"status\":0", Qt::CaseInsensitive))
    {
        parse(response, errors, data);
    } else {
        QMessageBox oautherr(QMessageBox::Critical, tr("Error"),
                             tr("There was an error during fetching. Please check the error description."));
        oautherr.setDetailedText(response); // probably blank
        oautherr.exec();
        return false;

    }
    if (errors.count() > 0) {
        error = errors.join(", ");
        return false;
    };
    return true;
}


void
WithingsDownload::parse(QString text, QStringList &errors, QList<BodyMeasure> &bodyMeasures)
{

    // parse it
    parser->parse(text, errors);
    if (errors.count() > 0) return;

    // convert from Withings to general format
    foreach(WithingsReading r, parser->readings()) {
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
    // qDebug() << "Request sent to Withings!";
}

void
WithingsDownload::onRequestReady(QByteArray r) {
    //qDebug() << "Response from the Withings's service: " << response;

    response = r;
    loop.exit(0);

}
#endif
