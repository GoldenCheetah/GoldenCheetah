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

    connect(oauthManager, SIGNAL(requestReady(QByteArray)),
            this, SLOT(onRequestReady(QByteArray)));
    connect(oauthManager, SIGNAL(authorizedRequestDone()),
            this, SLOT(onAuthorizedRequestDone()));
    #endif
}

bool
WithingsDownload::download()
{
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

        oauthManager->executeRequest(oauthRequest);

        #endif
    } else  {

        // account for trailing slash, remove it if it is there (it was the default in preferences)
        QString server = appsettings->cvalue(context->athlete->cyclist, GC_WIURL, "http://wbsapi.withings.net").toString();
        if (server.endsWith("/")) server=server.mid(0, server.length()-1);

        QString request = QString("%1/measure?action=getmeas&userid=%2&publickey=%3")
                                 .arg(server)
                                 .arg(appsettings->cvalue(context->athlete->cyclist, GC_WIUSER, "").toString())
                                 .arg(appsettings->cvalue(context->athlete->cyclist, GC_WIKEY, "").toString());


        QNetworkReply *reply = nam->get(QNetworkRequest(QUrl(request)));

        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(context->mainWindow, tr("Withings Data Download"), reply->errorString());
            return false;
        }
        return true;
    }
    return false;
}

void
WithingsDownload::downloadFinished(QNetworkReply *reply)
{

    if (reply->error() != QNetworkReply::NoError) {
       QMessageBox::warning(context->mainWindow, tr("Network Problem"), tr("No Withings Data downloaded"));
       return;
    }
    parse(reply->readAll());
}

void
WithingsDownload::parse(QString text)
{
    QStringList errors;

    // parse it
    parser->parse(text, errors);

    int allMeasures = context->athlete->withings().count();
    int receivedMeasures = parser->readings().count();
    int newMeasures = receivedMeasures - allMeasures;

    if (receivedMeasures == 0) {
        newMeasures = 0;
    }

    QString status = tr("No new measurements");
    if (newMeasures > 0) status = QString(tr("%1 new measurements received.")).arg(newMeasures);

    QMessageBox::information(context->mainWindow, tr("Withings Data Download"), status);

    // hacky for now, just refresh for all dates where we have withings data
    // will go with SQL shortly.
    if (newMeasures) {

        // if refresh is running cancel it !
        context->athlete->rideCache->cancel();

        // store in athlete
        context->athlete->setWithings(parser->readings());

        // now save data away if we actually got something !
        // doing it here means we don't overwrite previous responses
        // when we fail to get any data (e.g. errors / network problems)
        QFile withingsJSON(QString("%1/withings.json").arg(context->athlete->home->cache().canonicalPath()));
        if (withingsJSON.open(QFile::WriteOnly)) {

            QTextStream stream(&withingsJSON);
            stream << text;
            withingsJSON.close();
        }

        // do a refresh, it will check if needed
        context->athlete->rideCache->refresh();
    }
    return;
}

#ifdef GC_HAVE_KQOAUTH
void
WithingsDownload::onAuthorizedRequestDone() {
    // qDebug() << "Request sent to Withings!";
}

void
WithingsDownload::onRequestReady(QByteArray response) {
    //qDebug() << "Response from the Withings's service: " << response;

    QString r = response;

    if (r.contains("\"status\":0", Qt::CaseInsensitive))
    {
        parse(response);
    } else {
        QMessageBox oautherr(QMessageBox::Critical, tr("Error"),
                     tr("There was an error during fetching. Please check the error description."));
                     oautherr.setDetailedText(r); // probably blank
                 oautherr.exec();
    }



}
#endif
