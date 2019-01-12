/*
 * Copyright (c) 2010 Justin Knotzke (jknotzke@shampoo.ca)
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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
#include "Secrets.h"

#include "OAuthDialog.h"
#include "Athlete.h"
#include "Context.h"
#include "Settings.h"
#include "Colors.h"
#include "TimeUtils.h"

#if QT_VERSION > 0x050000
#include "GoogleDrive.h"
#include "PolarFlow.h"

#include <QJsonParseError>
#endif

OAuthDialog::OAuthDialog(Context *context, OAuthSite site, CloudService *service, QString baseURL, QString clientsecret) :
    context(context), site(site), service(service), baseURL(baseURL), clientsecret(clientsecret)
{

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("OAuth"));

    // we need to scale up, since we zoom the webview on hi-dpi screens
    setMinimumSize(640 *dpiXFactor, 640 *dpiYFactor);

    if (service) { // ultimately this will be the only way this works
        if (service->id() == "Strava") site = this->site = STRAVA;
        if (service->id() == "Dropbox") site = this->site = DROPBOX;
        if (service->id() == "Cycling Analytics") site = this->site = CYCLING_ANALYTICS;
        if (service->id() == "Google Calendar") site = this->site = GOOGLE_CALENDAR;
        if (service->id() == "Google Drive") site = this->site = GOOGLE_DRIVE;
        if (service->id() == "University of Kent") site = this->site = KENTUNI;
        if (service->id() == "Today's Plan") site = this->site = TODAYSPLAN;
        if (service->id() == "Withings") site = this->site = WITHINGS;
        if (service->id() == "PolarFlow") site = this->site = POLAR;
        if (service->id() == "SportTracks.mobi") site = this->site = SPORTTRACKS;
        if (service->id() == "Xert") site = this->site = XERT;
    }

    // check if SSL is available - if not - message and end
    if (!QSslSocket::supportsSsl()) {
        QString text = QString(tr("SSL Security Libraries required for 'Authorise' are missing in this installation."));
        QMessageBox sslMissing(QMessageBox::Critical, tr("Authorization Error"), text);
        sslMissing.exec();
        noSSLlib = true;
        return;
    }

    // ignore responses to false, used by POLARFLOW when binding the user
    ignore = false;

    // SSL is available - so authorisation can take place
    noSSLlib = false;

    layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2,0,2,2);
    setLayout(layout);


    #if defined(NOWEBKIT) || (QT_VERSION > 0x050000 && defined(Q_OS_MAC))
    view = new QWebEngineView();
    view->setZoomFactor(dpiXFactor);
    #if (QT_VERSION >= 0x050600)
    view->page()->profile()->cookieStore()->deleteAllCookies();
    #endif
    #else
    view = new QWebView();
    #endif

    view->setContentsMargins(0,0,0,0);
    view->page()->view()->setContentsMargins(0,0,0,0);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAcceptDrops(false);
    layout->addWidget(view);

    //
    // All services have some kind of initial authorisation URL where the user needs
    // to login and confirm they are willing to authorise the particular app and
    // provide a temporary token to get the real token with
    //
    QString urlstr = "";

    if (site == STRAVA) {

        urlstr = QString("https://www.strava.com/oauth/authorize?");
        urlstr.append("client_id=").append(GC_STRAVA_CLIENT_ID).append("&");
        urlstr.append("scope=view_private,write&");
        urlstr.append("redirect_uri=http://www.goldencheetah.org/&");
        urlstr.append("response_type=code&");
        urlstr.append("approval_prompt=force");

    } else if (site == DROPBOX) {

        urlstr = QString("https://www.dropbox.com/oauth2/authorize?");
#ifdef GC_DROPBOX_CLIENT_ID
        urlstr.append("client_id=").append(GC_DROPBOX_CLIENT_ID).append("&");
#endif
        urlstr.append("redirect_uri=https://goldencheetah.github.io/blank.html&");
        urlstr.append("response_type=code&");
        urlstr.append("force_reapprove=true");

    } else if (site == CYCLING_ANALYTICS) {

        urlstr = QString("https://www.cyclinganalytics.com/api/auth?");
        urlstr.append("client_id=").append(GC_CYCLINGANALYTICS_CLIENT_ID).append("&");
        urlstr.append("scope=modify_rides&");
        urlstr.append("redirect_uri=http://www.goldencheetah.org/&");
        urlstr.append("response_type=code&");
        urlstr.append("approval_prompt=force");

#if QT_VERSION >= 0x050000

    } else if (site == GOOGLE_CALENDAR) {
        // OAUTH 2.0 - Google flow for installed applications
        urlstr = QString("https://accounts.google.com/o/oauth2/auth?");
        urlstr.append("scope=https://www.googleapis.com/auth/calendar&");
        urlstr.append("redirect_uri=urn:ietf:wg:oauth:2.0:oob&");
        urlstr.append("response_type=code&");
        urlstr.append("client_id=").append(GC_GOOGLE_CALENDAR_CLIENT_ID);

    } else if (site == GOOGLE_DRIVE) {

        const QString scope =  service->getSetting(GC_GOOGLE_DRIVE_AUTH_SCOPE, "drive.appdata").toString();
        // OAUTH 2.0 - Google flow for installed applications
        urlstr = QString("https://accounts.google.com/o/oauth2/auth?");
        // We only request access to the application data folder, not all files.
        urlstr.append("scope=https://www.googleapis.com/auth/" + scope + "&");
        urlstr.append("redirect_uri=urn:ietf:wg:oauth:2.0:oob&");
        urlstr.append("response_type=code&");
        urlstr.append("client_id=").append(GC_GOOGLE_DRIVE_CLIENT_ID);

    } else if (site == KENTUNI) {

        const QString scope =  service->getSetting(GC_UOK_GOOGLE_DRIVE_AUTH_SCOPE, "drive.appdata").toString();

        // OAUTH 2.0 - Google flow for installed applications
        urlstr = QString("https://accounts.google.com/o/oauth2/auth?");
        // We only request access to the application data folder, not all files.
        urlstr.append("scope=https://www.googleapis.com/auth/" + scope + "&");
        urlstr.append("redirect_uri=urn:ietf:wg:oauth:2.0:oob&");
        urlstr.append("response_type=code&");
        urlstr.append("client_id=").append(GC_GOOGLE_DRIVE_CLIENT_ID);

#endif

    } else if (site == TODAYSPLAN) {

        //urlstr = QString("https://whats.todaysplan.com.au/en/authorize/"); //XXX fixup below when pages.cpp goes
        if (baseURL=="") baseURL=service->getSetting(GC_TODAYSPLAN_URL, "https://whats.todaysplan.com.au").toString();
        urlstr = QString("%1/authorize/").arg(baseURL);
        urlstr.append(GC_TODAYSPLAN_CLIENT_ID);

    } else if (site == POLAR) {

        // OAUTH 2.0 - Google flow for installed applications
        urlstr = QString("%1?").arg(GC_POLARFLOW_OAUTH_URL);
        // We only request access to the application data folder, not all files.
        urlstr.append("response_type=code&");
        urlstr.append("client_id=").append(GC_POLARFLOW_CLIENT_ID);

    } else if (site == SPORTTRACKS) {

        // We only request access to the application data folder, not all files.
        urlstr = QString("https://api.sporttracks.mobi/oauth2/authorize?");
        urlstr.append("redirect_uri=http://www.goldencheetah.org&");
        urlstr.append("state=xyzzy&");
        urlstr.append("response_type=code&");
        urlstr.append("client_id=").append(GC_SPORTTRACKS_CLIENT_ID);

    } else if (site == WITHINGS) {

        urlstr = QString("https://account.withings.com/oauth2_user/authorize2?");
        urlstr.append("redirect_uri=http://www.goldencheetah.org&");
        urlstr.append("scope=user.info,user.metrics&");
        urlstr.append("response_type=code&");
        urlstr.append("state=xyzzy&");
        urlstr.append("client_id=").append(GC_NOKIA_CLIENT_ID);

    } else if (site == XERT) {
        urlChanged(QUrl("http://www.goldencheetah.org/?code=0"));
    }

    //
    // STEP 1: LOGIN AND AUTHORISE THE APPLICATION
    //
    if (site == DROPBOX || site == STRAVA || site == CYCLING_ANALYTICS || site == POLAR || site == SPORTTRACKS || site == GOOGLE_CALENDAR || site == GOOGLE_DRIVE || site == KENTUNI || site == TODAYSPLAN || site == WITHINGS) {

        url = QUrl(urlstr);
        view->setUrl(url);

        // connects
        connect(view, SIGNAL(urlChanged(const QUrl&)), this, SLOT(urlChanged(const QUrl&)));
        connect(view, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));
    }
}

// just ignore SSL handshake errors at all times
void
OAuthDialog::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}


//
// STEP 2: AUTHORISATION REDIRECT WITH TEMPORARY CODE
//
// When the URL changes, it will be the redirect with the temporary token after
// the initial authorisation. The URL will have some parameters to indicate this
// if they exist we should intercept the redirect to get the permanent token.
// If they don't get passed then we don't need to do anything.
//
void
OAuthDialog::urlChanged(const QUrl &url)
{
    QString authheader;

    // sites that use this scheme
    if (site == DROPBOX || site == STRAVA || site == CYCLING_ANALYTICS || site == TODAYSPLAN || site == POLAR || site == SPORTTRACKS || site == XERT || site == WITHINGS) {

        if (url.toString().startsWith("http://www.goldencheetah.org/?state=&code=") ||
                url.toString().contains("blank.html?code=") ||
                url.toString().startsWith("http://www.goldencheetah.org/?code=")) {

            QUrlQuery parse(url);
            QString code=parse.queryItemValue("code");

            QByteArray data;
#if QT_VERSION > 0x050000
            QUrlQuery params;
#else
            QUrl params;
#endif
            QString urlstr = "";

            // now get the final token to store
            if (site == DROPBOX) {

                urlstr = QString("https://api.dropboxapi.com/oauth2/token?");
                urlstr.append("redirect_uri=https://goldencheetah.github.io/blank.html&");
                params.addQueryItem("grant_type", "authorization_code");
#ifdef GC_DROPBOX_CLIENT_ID
                params.addQueryItem("client_id", GC_DROPBOX_CLIENT_ID);
#endif
#ifdef GC_DROPBOX_CLIENT_SECRET
                params.addQueryItem("client_secret", GC_DROPBOX_CLIENT_SECRET);
#endif

            } else if (site == POLAR) {

                urlstr = QString("%1?").arg(GC_POLARFLOW_TOKEN_URL);
                urlstr.append("redirect_uri=http://www.goldencheetah.org");
                params.addQueryItem("grant_type", "authorization_code");
#if (defined GC_POLARFLOW_CLIENT_ID) && (defined GC_POLARFLOW_CLIENT_SECRET)
                authheader = QString("%1:%2").arg(GC_POLARFLOW_CLIENT_ID).arg(GC_POLARFLOW_CLIENT_SECRET);
#endif

            } else if (site == SPORTTRACKS) {

                urlstr = QString("https://api.sporttracks.mobi/oauth2/token?");
                params.addQueryItem("client_id", GC_SPORTTRACKS_CLIENT_ID);
                params.addQueryItem("client_secret", GC_SPORTTRACKS_CLIENT_SECRET);
                params.addQueryItem("redirect_uri","http://www.goldencheetah.org");
                params.addQueryItem("grant_type", "authorization_code");

            } else if (site == STRAVA) {

                urlstr = QString("https://www.strava.com/oauth/token?");
                params.addQueryItem("client_id", GC_STRAVA_CLIENT_ID);
#ifdef GC_STRAVA_CLIENT_SECRET
                params.addQueryItem("client_secret", GC_STRAVA_CLIENT_SECRET);
#endif

            }  else if (site == CYCLING_ANALYTICS) {

                urlstr = QString("https://www.cyclinganalytics.com/api/token?");
                params.addQueryItem("client_id", GC_CYCLINGANALYTICS_CLIENT_ID);
#ifdef GC_CYCLINGANALYTICS_CLIENT_SECRET
                params.addQueryItem("client_secret", GC_CYCLINGANALYTICS_CLIENT_SECRET);
#endif
                params.addQueryItem("grant_type", "authorization_code");

            }  else if (site == TODAYSPLAN) {

                if (baseURL=="") baseURL=service->getSetting(GC_TODAYSPLAN_URL, "https://whats.todaysplan.com.au").toString();
                urlstr = QString("%1/rest/oauth/access_token?").arg(baseURL);
                params.addQueryItem("client_id", GC_TODAYSPLAN_CLIENT_ID);
#ifdef GC_TODAYSPLAN_CLIENT_SECRET
                if (clientsecret != "") //XXX get rid when pages.cpp goes
                    params.addQueryItem("client_secret", clientsecret);
                else if (service->getSetting(GC_TODAYSPLAN_USERKEY, "").toString() != "")
                    params.addQueryItem("client_secret", service->getSetting(GC_TODAYSPLAN_USERKEY, "").toString());
                else
                    params.addQueryItem("client_secret", GC_TODAYSPLAN_CLIENT_SECRET);

#endif
                params.addQueryItem("grant_type", "authorization_code");
                params.addQueryItem("redirect_uri", "https://goldencheetah.github.io/blank.html");

            }  else if (site == XERT) {

                urlstr = QString("https://www.xertonline.com/oauth/token");
                params.addQueryItem("username", service->getSetting(GC_XERTUSER, "").toString());
                params.addQueryItem("password", service->getSetting(GC_XERTPASS, "").toString());
                params.addQueryItem("grant_type", "password");

                authheader = QString("%1:%1").arg("xert_public");
            } else if (site == WITHINGS) {

                urlstr = QString("https://account.withings.com/oauth2/token?");
                params.addQueryItem("client_id", GC_NOKIA_CLIENT_ID);
                params.addQueryItem("client_secret", GC_NOKIA_CLIENT_SECRET);
                params.addQueryItem("redirect_uri","http://www.goldencheetah.org");
                params.addQueryItem("grant_type", "authorization_code");

            }

            // all services will need us to send the temporary code received
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

            // client id and secret are encoded and sent in the header for POLAR and XERT
            if (site == POLAR || site == XERT)  request.setRawHeader("Authorization", "Basic " +  authheader.toLatin1().toBase64());

            // now get the final token - but ignore errors
            manager = new QNetworkAccessManager(this);
            connect(manager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
            connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkRequestFinished(QNetworkReply*)));
            manager->post(request, data);

        }
    }
}

//
// GOOGLE DRIVE gets the code in the HTML title field (different to other services)
//
void
OAuthDialog::loadFinished(bool ok)
{

    if (site == GOOGLE_CALENDAR || site == GOOGLE_DRIVE || site == KENTUNI) {

        if (ok && url.toString().startsWith("https://accounts.google.com/o/oauth2/auth")) {

            // retrieve the code from the HTML page title
            QString title = view->title();

            if (title.contains("code")) {

                QString code = title.right(title.length()-title.indexOf("code=")-5);
                QByteArray data;
#if QT_VERSION > 0x050000
                QUrlQuery params;
#else
                QUrl params;
#endif
                QString urlstr = "https://www.googleapis.com/oauth2/v3/token?";
                if (site == GOOGLE_CALENDAR) {
                    params.addQueryItem("client_id", GC_GOOGLE_CALENDAR_CLIENT_ID);
                } else if (site == GOOGLE_DRIVE || site == KENTUNI) {
                    params.addQueryItem("client_id", GC_GOOGLE_DRIVE_CLIENT_ID);
                }

                if (site == GOOGLE_CALENDAR) {
                    params.addQueryItem("client_secret", GC_GOOGLE_CALENDAR_CLIENT_SECRET);
                } else if (site == GOOGLE_DRIVE || site == KENTUNI) {
                    params.addQueryItem("client_secret", GC_GOOGLE_DRIVE_CLIENT_SECRET);
                }

                params.addQueryItem("code", code);
                params.addQueryItem("redirect_uri", "urn:ietf:wg:oauth:2.0:oob");
                params.addQueryItem("grant_type", "authorization_code");

#if QT_VERSION > 0x050000
                data.append(params.query(QUrl::FullyEncoded));
#else
                data=params.encodedQuery();
#endif

                // trade-in the temporary access code retrieved by the
                // Call-Back URL for the finale token
                QUrl url = QUrl(urlstr);
                QNetworkRequest request = QNetworkRequest(url);
                request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

                // not get the final token - ignoring errors
                manager = new QNetworkAccessManager(this);
                connect(manager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
                connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkRequestFinished(QNetworkReply*)));
                manager->post(request, data);
            }
        }
    }
}

#if QT_VERSION < 0x050000
static QString RawJsonStringGrab(const QByteArray& payload,
                                 const QString& needle) {
    // A RegExp based JSON string parser. Not the best, but it does the job.
    QString regex =
        // This matches the key.
        "(" + needle + "|\"" + needle + "\"|'" + needle + "')"
        // Matches the separator.
        "[\\s]*:[\\s]*"
        // matches the value.
        "(\"\\S+\"|'\\S+')";
    QRegExp q(regex);
    if (!q.isValid()) {
        // Somehow failed to build the regex.
        return "";
    }
    int start = q.indexIn(payload);
    int pos = q.pos(2);
    if (start == -1 || pos == -1) {
        // Failed to find the key or the value.
        return "";
    }
    QString ret = payload.mid(pos, q.matchedLength() + start - pos);
    // Remove " or ' from the value.
    ret.remove(0, 1);
    ret.remove(ret.size() - 1, 1);
    return ret;
}
#endif

//
// STEP 3: REFRESH AND ACCESS TOKEN RECEIVED
//
// this is when we get the refresh or access tokens after a redirect has been loaded
// so pretty much at the end of the process. Each service can have slightly special
// needs and certainly needs to set the right setting anyway.
//
void
OAuthDialog::networkRequestFinished(QNetworkReply *reply)
{

    // we've been told to ignore responses (used by POLAR, maybe others in future)
    if (ignore) return;

    // we can handle SSL handshake errors, if we got here then some kind of protocol was agreed
    if (reply->error() == QNetworkReply::NoError || reply->error() == QNetworkReply::SslHandshakeFailedError) {

        QByteArray payload = reply->readAll(); // JSON
        QString refresh_token;
        QString access_token;
        double polar_userid=0;

        // parse the response and extract the tokens, pretty much the same for all services
        // although polar choose to also pass a user id, which is needed for future calls
#if QT_VERSION > 0x050000
        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            refresh_token = document.object()["refresh_token"].toString();
            access_token = document.object()["access_token"].toString();
            if (site == POLAR)  polar_userid = document.object()["x_user_id"].toDouble();
        }
#else
        refresh_token = RawJsonStringGrab(payload, "refresh_token");
        access_token = RawJsonStringGrab(payload, "access_token");
#endif

        // if we failed to extract then we have a big problem
        // google uses a refresh token so trap for them only
        if (((site == GOOGLE_CALENDAR || site == GOOGLE_DRIVE || site == KENTUNI) && refresh_token == "") || access_token == "") {

            // Something failed.
            // Only Google uses both refresh and access tokens.
            QString error = QString(tr("Error retrieving authoriation credentials"));
            QMessageBox oautherr(QMessageBox::Critical, tr("Authorization Error"), error);
            oautherr.setDetailedText(error);
            oautherr.exec();

            return;
        }

        // now set the tokens etc
        if (site == DROPBOX) {

            service->setSetting(GC_DROPBOX_TOKEN, access_token);
            QString info = QString(tr("Dropbox authorization was successful."));
            QMessageBox information(QMessageBox::Information, tr("Information"), info);
            information.exec();

        } else if (site == SPORTTRACKS) {

            service->setSetting(GC_SPORTTRACKS_TOKEN, access_token);
            service->setSetting(GC_SPORTTRACKS_REFRESH_TOKEN, refresh_token);
            service->setSetting(GC_SPORTTRACKS_LAST_REFRESH, QDateTime::currentDateTime());
            QString info = QString(tr("SportTracks authorization was successful."));
            QMessageBox information(QMessageBox::Information, tr("Information"), info);
            information.exec();

        } else if (site == POLAR) {

            service->setSetting(GC_POLARFLOW_TOKEN, access_token);
            service->setSetting(GC_POLARFLOW_USER_ID, polar_userid);

            // we now need to bind the user, this is a one time deal.
            QString url = QString("%1/v3/users").arg(GC_POLARFLOW_URL);

            // request using the bearer token
            QNetworkRequest request(url);
            request.setRawHeader("Authorization", (QString("Bearer %1").arg(access_token)).toLatin1());
            request.setRawHeader("Accept", "application/json");
            request.setRawHeader("Content-Type", "application/json");

            // data to post
            QByteArray data;
            data.append(QString("{\"member-id\":\"%1\"}").arg(context->athlete->cyclist));

            // the request will fallback to this method on networkRequestFinished
            // but we are done, so set ignore= true to get this function to just
            // return without doing anything
            ignore=true;
            QNetworkReply *bind = manager->post(request, data);

            // blocking request
            QEventLoop loop;
            connect(bind, SIGNAL(finished()), &loop, SLOT(quit()));
            loop.exec();

            // Bind response lists athlete details, we ignore them for now
            QByteArray r = bind->readAll();
            //qDebug()<<bind->errorString()<< "bind response="<<r;

            QString info = QString(tr("PolarFlow authorization was successful."));
            QMessageBox information(QMessageBox::Information, tr("Information"), info);
            information.exec();

        } else if (site == STRAVA) {

            service->setSetting(GC_STRAVA_TOKEN, access_token);
            QString info = QString(tr("Strava authorization was successful."));
            QMessageBox information(QMessageBox::Information, tr("Information"), info);
            information.exec();

        } else if (site == CYCLING_ANALYTICS) {

            service->setSetting(GC_CYCLINGANALYTICS_TOKEN, access_token);
            QString info = QString(tr("Cycling Analytics authorization was successful."));
            QMessageBox information(QMessageBox::Information, tr("Information"), info);
            information.exec();

        } else if (site == GOOGLE_CALENDAR) {
            // remove the Google Page first
            url = QUrl("http://www.goldencheetah.org");
            view->setUrl(url);
            appsettings->setCValue(context->athlete->cyclist, GC_GOOGLE_CALENDAR_REFRESH_TOKEN, refresh_token);
            QString info = QString(tr("Google Calendar authorization was successful."));
            QMessageBox information(QMessageBox::Information,
                                    tr("Information"), info);
            information.exec();

        } else if (site == KENTUNI) {

            service->setSetting(GC_UOK_GOOGLE_DRIVE_REFRESH_TOKEN, refresh_token);
            service->setSetting(GC_UOK_GOOGLE_DRIVE_ACCESS_TOKEN, access_token);
            service->setSetting(GC_UOK_GOOGLE_DRIVE_LAST_ACCESS_TOKEN_REFRESH, QDateTime::currentDateTime());
            QString info = QString(tr("Kent University Google Drive authorization was successful."));
            QMessageBox information(QMessageBox::Information, tr("Information"), info);
            information.exec();

        } else if (site == GOOGLE_DRIVE) {

            service->setSetting(GC_GOOGLE_DRIVE_REFRESH_TOKEN, refresh_token);
            service->setSetting(GC_GOOGLE_DRIVE_ACCESS_TOKEN, access_token);
            service->setSetting(GC_GOOGLE_DRIVE_LAST_ACCESS_TOKEN_REFRESH, QDateTime::currentDateTime());
            QString info = QString(tr("Google Drive authorization was successful."));
            QMessageBox information(QMessageBox::Information, tr("Information"), info);
            information.exec();

        } else if (site == TODAYSPLAN) {
            service->setSetting(GC_TODAYSPLAN_TOKEN, access_token);
            QString info = QString(tr("Today's Plan authorization was successful."));
            QMessageBox information(QMessageBox::Information, tr("Information"), info);
            information.exec();
        } else if (site == XERT) {

            service->setSetting(GC_XERT_TOKEN, access_token);
            service->setSetting(GC_XERT_REFRESH_TOKEN, refresh_token);

            // Try without Message Box

            //QString info = QString(tr("Xert authorization was successful."));
            //QMessageBox information(QMessageBox::Information, tr("Information"), info);
            //information.exec();

            service->message = "Xert authorization was successful.";

        } else if (site == WITHINGS) {

            service->setSetting(GC_NOKIA_TOKEN, access_token);
            service->setSetting(GC_NOKIA_REFRESH_TOKEN, refresh_token);

            // We have to ask for userid ?



            QString info = QString(tr("Withings authorization was successful."));
            QMessageBox information(QMessageBox::Information, tr("Information"), info);
            information.exec();

        }

    } else {

            // general error getting response
            QString error = QString(tr("Error retrieving access token, %1 (%2)")).arg(reply->errorString()).arg(reply->error());
            QMessageBox oautherr(QMessageBox::Critical, tr("SSL Token Refresh Error"), error);
            oautherr.setDetailedText(error);
            oautherr.exec();
    }

    // job done, dialog can be closed
    accept();
}
