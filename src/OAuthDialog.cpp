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
    context(context), site(site), requestToken(false), requestAuth(false)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("OAuth"));

    layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2,0,2,2);
    setLayout(layout);

    QString urlstr = "";
    if (site == STRAVA) {
        urlstr = QString("https://www.strava.com/oauth/authorize?");
        urlstr.append("client_id=").append(GC_STRAVA_CLIENT_ID).append("&");
        urlstr.append("scope=view_private,write&");
    }
    else if (site == TWITTER) {
        urlstr = QString("http://api.twitter.com/oauth/request_token?");
        // TODO
    }
    else if (site == CYCLING_ANALYTICS) {
        urlstr = QString("https://www.cyclinganalytics.com/api/auth?");
        urlstr.append("client_id=").append(GC_CYCLINGANALYTICS_CLIENT_ID).append("&");
        urlstr.append("scope=modify_rides&");
    }
    urlstr.append("redirect_uri=http://www.goldencheetah.org/&");  
    urlstr.append("response_type=code&");
    urlstr.append("approval_prompt=force");


    url = QUrl(urlstr);

    view = new QWebView();
    view->setContentsMargins(0,0,0,0);
    view->page()->view()->setContentsMargins(0,0,0,0);
    view->setUrl(url);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAcceptDrops(false);
    layout->addWidget(view);

    //
    // connects
    //
    connect(view, SIGNAL(urlChanged(const QUrl&)), this, SLOT(urlChanged(const QUrl&)));
    connect(view, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished()));

}

void
OAuthDialog::urlChanged(const QUrl &url)
{
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

        if (site == STRAVA) {
            urlstr = QString("https://www.strava.com/oauth/token?");
            params.addQueryItem("client_id", GC_STRAVA_CLIENT_ID);
#ifdef GC_STRAVA_CLIENT_SECRET
            params.addQueryItem("client_secret", GC_STRAVA_CLIENT_SECRET);
#endif
            params.addQueryItem("redirect_uri", "http://www.goldencheetah.org/");
        }
        else if (site == TWITTER) {
            urlstr = QString("http://api.twitter.com/oauth/token?");
            // TODO
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

        QUrl url = QUrl(urlstr);
        QNetworkRequest request = QNetworkRequest(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

        requestToken = true;
        view->load(request,QNetworkAccessManager::PostOperation,data);
    }
}

void
OAuthDialog::loadFinished()
{
    if (requestToken) {
        int at = view->page()->mainFrame()->toHtml().indexOf("\"access_token\":");
        if (at>-1) {
            int i = view->page()->mainFrame()->toHtml().indexOf("\"", at+15);
            int j = view->page()->mainFrame()->toHtml().indexOf("\"", i+1);
            if (i>-1 && j>-1) {
                QString access_token = view->page()->mainFrame()->toHtml().mid(i+1,j-i-1);
                if (site == STRAVA) {
                    appsettings->setCValue(context->athlete->cyclist, GC_STRAVA_TOKEN, access_token);
                }
                else if (site == TWITTER) {
                    // TODO
                }
                else if (site == CYCLING_ANALYTICS) {
                    appsettings->setCValue(context->athlete->cyclist, GC_CYCLINGANALYTICS_TOKEN, access_token);
                }
                accept();
            }
        }
    }
}
