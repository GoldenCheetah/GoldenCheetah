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
        urlstr.append("redirect_uri=http://www.goldencheetah.org/&");
        urlstr.append("response_type=code&");
        urlstr.append("approval_prompt=force");

    } else if (site == TWITTER) {

#ifdef GC_HAVE_LIBOAUTH

        int rc;
        char **rv = NULL;
        QString token;
        QString url = QString();
        t_key = NULL;
        t_secret = NULL;
        const char *request_token_uri = "https://api.twitter.com/oauth/request_token";
        char *req_url = NULL;
        char *postarg = NULL;
        char *reply   = NULL;

        // get the url
        req_url = oauth_sign_url2(request_token_uri, NULL, OA_HMAC, NULL, GC_TWITTER_CONSUMER_KEY, GC_TWITTER_CONSUMER_SECRET, NULL, NULL);
        if (req_url != NULL) {


            // post it
            reply = oauth_http_get(req_url,postarg);
            if (reply != NULL) {

                // will split reply into parameters using strdup
                rc = oauth_split_url_parameters(reply, &rv);

                if (rc >= 3) {

                    // really ?
                    qsort(rv, rc, sizeof(char *), oauth_cmpstringp);

                    token = QString(rv[1]);
                    t_key  =strdup(&(rv[1][12]));
                    t_secret =strdup(&(rv[2][19]));
                    urlstr = QString("https://api.twitter.com/oauth/authorize?");
                    urlstr.append(token);

                    // free memory using count rc
                    for(int i=0; i<rc; i++) free(rv[i]);
                }

                //urlstr.append("&oauth_callback=http%3A%2F%2Fwww.goldencheetah.org%2F");
                requestToken = true;
            }
        }
#endif

    } else if (site == CYCLING_ANALYTICS) {

        urlstr = QString("https://www.cyclinganalytics.com/api/auth?");
        urlstr.append("client_id=").append(GC_CYCLINGANALYTICS_CLIENT_ID).append("&");
        urlstr.append("scope=modify_rides&");
        urlstr.append("redirect_uri=http://www.goldencheetah.org/&");
        urlstr.append("response_type=code&");
        urlstr.append("approval_prompt=force");
    }


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
        //qDebug()<< view->page()->mainFrame()->toHtml();

        int at = view->page()->mainFrame()->toHtml().indexOf("\"access_token\":");
        if (at==-1)
            at = view->page()->mainFrame()->toHtml().indexOf("<code>");
        if (at>-1) {
            int i = -1;
            int j = -1;

            if (site == TWITTER) {
                i = at+5;
                j = view->page()->mainFrame()->toHtml().indexOf("</code>", i+1);
            }
            else  {
                i = view->page()->mainFrame()->toHtml().indexOf("\"", at+15);
                j = view->page()->mainFrame()->toHtml().indexOf("\"", i+1);
            }
            if (i>-1 && j>-1) {
                QString access_token = view->page()->mainFrame()->toHtml().mid(i+1,j-i-1);
                //qDebug() << "access_token" << access_token;

                if (site == STRAVA) {
                    appsettings->setCValue(context->athlete->cyclist, GC_STRAVA_TOKEN, access_token);
                }
                else if (site == TWITTER) {
#ifdef GC_HAVE_LIBOAUTH
                    char *reply;
                    char *req_url;
                    char **rv = NULL;
                    char *postarg = NULL;
                    QString url = QString("https://api.twitter.com/oauth/access_token?a=b&oauth_verifier=");

                    url.append(access_token);

                    req_url = oauth_sign_url2(url.toLatin1(), NULL, OA_HMAC, NULL, GC_TWITTER_CONSUMER_KEY, GC_TWITTER_CONSUMER_SECRET, t_key, t_secret);
                    reply = oauth_http_get(req_url,postarg);

                    int rc = oauth_split_url_parameters(reply, &rv);

                    if(rc ==4)
                    {
                        qsort(rv, rc, sizeof(char *), oauth_cmpstringp);

                        const char *oauth_token = strdup(&(rv[0][12]));
                        const char *oauth_secret = strdup(&(rv[1][19]));

                        //Save Twitter oauth_token and oauth_secret;
                        appsettings->setCValue(context->athlete->cyclist, GC_TWITTER_TOKEN, oauth_token);
                        appsettings->setCValue(context->athlete->cyclist, GC_TWITTER_SECRET, oauth_secret);
                    }
#endif
                }
                else if (site == CYCLING_ANALYTICS) {
                    appsettings->setCValue(context->athlete->cyclist, GC_CYCLINGANALYTICS_TOKEN, access_token);
                }
                accept();
            }
        }
    }
}
