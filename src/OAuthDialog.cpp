/*
 * Copyright (c) 2010 Justin Knotzke (jknotzke@shampoo.ca)
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

#include "OAuthDialog.h"
#include "Athlete.h"
#include "Context.h"
#include "Settings.h"
#include "TimeUtils.h"


OAuthDialog::OAuthDialog(Context *context, OAuthSite site) :
    context(context), site(site)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("OAuth"));

    layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2,0,2,2);
    setLayout(layout);

    view = new QWebView();
    view->setContentsMargins(0,0,0,0);
    view->page()->view()->setContentsMargins(0,0,0,0);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAcceptDrops(false);
    layout->addWidget(view);

    QString urlstr = "";
    if (site == STRAVA) {
        urlstr = QString("https://www.strava.com/oauth/authorize?");
        urlstr.append("client_id=").append(GC_STRAVA_CLIENT_ID).append("&");
        urlstr.append("scope=view_private,write&");
        urlstr.append("redirect_uri=http://www.goldencheetah.org/&");
        urlstr.append("response_type=code&");
        urlstr.append("approval_prompt=force");

    } else if (site == TWITTER) {

#ifdef GC_HAVE_KQOAUTH
        oauthRequest = new KQOAuthRequest;
        oauthManager = new KQOAuthManager(this);

        connect(oauthManager, SIGNAL(temporaryTokenReceived(QString,QString)),
                this, SLOT(onTemporaryTokenReceived(QString, QString)));

        connect(oauthManager, SIGNAL(authorizationReceived(QString,QString)),
                this, SLOT( onAuthorizationReceived(QString, QString)));

        connect(oauthManager, SIGNAL(accessTokenReceived(QString,QString)),
                this, SLOT(onAccessTokenReceived(QString,QString)));

        connect(oauthManager, SIGNAL(requestReady(QByteArray)),
                this, SLOT(onRequestReady(QByteArray)));

        connect(oauthManager, SIGNAL(authorizationPageRequested(QUrl)),
                this, SLOT(onAuthorizationPageRequested(QUrl)));


        oauthRequest->initRequest(KQOAuthRequest::TemporaryCredentials, QUrl("https://api.twitter.com/oauth/request_token"));

        oauthRequest->setConsumerKey(GC_TWITTER_CONSUMER_KEY);
        oauthRequest->setConsumerSecretKey(GC_TWITTER_CONSUMER_SECRET);

        oauthManager->setHandleUserAuthorization(true);
        oauthManager->setHandleAuthorizationPageOpening(false);

        oauthManager->executeRequest(oauthRequest);

#endif
    } else if (site == CYCLING_ANALYTICS) {

        urlstr = QString("https://www.cyclinganalytics.com/api/auth?");
        urlstr.append("client_id=").append(GC_CYCLINGANALYTICS_CLIENT_ID).append("&");
        urlstr.append("scope=modify_rides&");
        urlstr.append("redirect_uri=http://www.goldencheetah.org/&");
        urlstr.append("response_type=code&");
        urlstr.append("approval_prompt=force");
    }

    // different process to get the token for STRAVA, CYCLINGANALYTICS vs. TWITTER
    if (site == STRAVA || site == CYCLING_ANALYTICS) {


        url = QUrl(urlstr);
        view->setUrl(url);

        // connects
        connect(view, SIGNAL(urlChanged(const QUrl&)), this, SLOT(urlChanged(const QUrl&)));
    }
}

#ifdef GC_HAVE_KQOAUTH
// ****************** Twitter OAUTH ******************************************************

void OAuthDialog::onTemporaryTokenReceived(QString, QString)
{
    QUrl userAuthURL("https://api.twitter.com/oauth/authorize");

    if( oauthManager->lastError() == KQOAuthManager::NoError) {
        oauthManager->getUserAuthorization(userAuthURL);
    }

}

void OAuthDialog::onAuthorizationReceived(QString, QString) {
    // qDebug() << "Authorization token received: " << token << verifier;

    oauthManager->getUserAccessTokens(QUrl("https://api.twitter.com/oauth/access_token"));
    if( oauthManager->lastError() != KQOAuthManager::NoError) {
        QString error = QString(tr("Error fetching OAuth credentials - Endpoint: /oauth/access_token"));
        QMessageBox oautherr(QMessageBox::Critical, tr("Authorization Error"), error);
        oautherr.exec();
        accept();
    }
}

void OAuthDialog::onAccessTokenReceived(QString token, QString tokenSecret) {
    // qDebug() << "Access token received: " << token << tokenSecret;

    appsettings->setCValue(context->athlete->cyclist, GC_TWITTER_TOKEN, token);
    appsettings->setCValue(context->athlete->cyclist, GC_TWITTER_SECRET,  tokenSecret);

    QString info = QString(tr("Twitter authorization was successful."));
    QMessageBox information(QMessageBox::Information, tr("Information"), info);
    information.exec();
    accept();
}

void OAuthDialog::onAuthorizedRequestDone() {
    // request sent to Twitter - do nothing
}

void OAuthDialog::onRequestReady(QByteArray response) {
    // qDebug() << "Response received: " << response;
    QString r = response;
    if (r.contains("\"errors\"", Qt::CaseInsensitive))
    {
        QMessageBox oautherr(QMessageBox::Critical, tr("Error in authorization"),
             tr("There was an error during authorization. Please check the error description."));
             oautherr.setDetailedText(r); // probably blank
         oautherr.exec();
    }
}


void OAuthDialog::onAuthorizationPageRequested(QUrl url) {
    // open Authorization page in view
    view->setUrl(url);

}
#endif

// ****************** Strava / Cyclinganalytics authorization*********************************************

void
OAuthDialog::urlChanged(const QUrl &url)
{
    // STRAVA & CYCLINGANALYTICS work with Call-back URLs / change of URL indicates next step is required

    if (url.toString().startsWith("http://www.goldencheetah.org/?state=&code=") ||
        url.toString().startsWith("http://www.goldencheetah.org/?code=")) {
        QString code = url.toString().right(url.toString().length()-url.toString().indexOf("code=")-5);

        QByteArray data;
#if QT_VERSION > 0x050000
        QUrlQuery params;
#else
        QUrl params;
#endif
        QString urlstr = "";

        // now get the final token to store
        if (site == STRAVA) {
            urlstr = QString("https://www.strava.com/oauth/token?");
            params.addQueryItem("client_id", GC_STRAVA_CLIENT_ID);
#ifdef GC_STRAVA_CLIENT_SECRET
            params.addQueryItem("client_secret", GC_STRAVA_CLIENT_SECRET);
#endif
        }

        else if (site == CYCLING_ANALYTICS) {
            urlstr = QString("https://www.cyclinganalytics.com/api/token?");
            params.addQueryItem("client_id", GC_CYCLINGANALYTICS_CLIENT_ID);
#ifdef GC_CYCLINGANALYTICS_CLIENT_SECRET
            params.addQueryItem("client_secret", GC_CYCLINGANALYTICS_CLIENT_SECRET);
#endif
            params.addQueryItem("grant_type", "authorization_code");
        }
        params.addQueryItem("code", code);

#if QT_VERSION > 0x050000
        data.append(params.query(QUrl::FullyEncoded));
#else
        data=params.encodedQuery();
#endif

        // trade-in the temporary access code retrieved by the Call-Back URL for the finale token
        QUrl url = QUrl(urlstr);
        QNetworkRequest request = QNetworkRequest(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

        // not get the final token
        manager = new QNetworkAccessManager(this);
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkRequestFinished(QNetworkReply*)));
        manager->post(request, data);

    }
}

void OAuthDialog::networkRequestFinished(QNetworkReply *reply) {

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray payload = reply->readAll(); // JSON
        int at = payload.indexOf("\"access_token\":");
        if (at >=0 ) {
            int from = at + 15; // first char after ":"
            int next = payload.indexOf("\"", from);
            from = next + 1;
            int to = payload.indexOf("\"", from);
            QString access_token = payload.mid(from, to-from);
            if (site == STRAVA) {
                appsettings->setCValue(context->athlete->cyclist, GC_STRAVA_TOKEN, access_token);
                QString info = QString(tr("Strava authorization was successful."));
                QMessageBox information(QMessageBox::Information, tr("Information"), info);
                information.exec();
            } else if (site == CYCLING_ANALYTICS) {
                appsettings->setCValue(context->athlete->cyclist, GC_CYCLINGANALYTICS_TOKEN, access_token);
                QString info = QString(tr("Cycling Analytics authorization was successful."));
                QMessageBox information(QMessageBox::Information, tr("Information"), info);
                information.exec();
            };
        }
    } else { // something failed

        QString error = QString(tr("Error authoriation credentials"));
        QMessageBox oautherr(QMessageBox::Critical, tr("Authorization Error"), error);
        oautherr.setDetailedText(error);
        oautherr.exec();
    }

    // job done, dialog can be closed
    accept();

}
