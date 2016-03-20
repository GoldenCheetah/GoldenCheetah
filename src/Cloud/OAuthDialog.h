/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#ifndef OAUTHDIALOG_H
#define OAUTHDIALOG_H
#include "GoldenCheetah.h"
#include "Pages.h"
#ifdef GC_HAVE_KQOAUTH
#include <kqoauthmanager.h>
#endif
#include <QObject>
#include <QtGui>
#include <QWidget>
#include <QStackedLayout>
#include <QUrl>
#include <QSslSocket>
#ifndef NOWEBKIT
#include <QtWebKit>
#include <QWebView>
#include <QWebFrame>
#endif

// QUrl split into QUrlQuerty in QT5
#if QT_VERSION > 0x050000
#include <QUrlQuery>
#endif
// QWebEngine if on Mac, -or- we don't have webkit
#if defined(NOWEBKIT) || ((QT_VERSION > 0x050000) && defined(Q_OS_MAC))
#include <QWebEngineHistory>
#include <QWebEngineHistoryItem>
#include <QWebEnginePage>
#include <QWebEngineView>
#endif


class OAuthDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

public:
    typedef enum {
        STRAVA,
        TWITTER,
        DROPBOX,
        CYCLING_ANALYTICS,
        GOOGLE_CALENDAR,
        GOOGLE_DRIVE,
    } OAuthSite;

    OAuthDialog(Context *context, OAuthSite site);

    bool sslLibMissing() { return noSSLlib; }

private slots:
    // Strava/Cyclinganalytics/Google
    void urlChanged(const QUrl& url);
    void loadFinished(bool ok);
    void networkRequestFinished(QNetworkReply *reply);

#ifdef GC_HAVE_KQOAUTH
    // Twitter OAUTH
    void onTemporaryTokenReceived(QString, QString);
    void onAuthorizationReceived(QString, QString);
    void onAccessTokenReceived(QString token, QString tokenSecret);
    void onAuthorizedRequestDone();
    void onRequestReady(QByteArray response);
    void onAuthorizationPageRequested (QUrl pageUrl);
#endif


private:
    Context *context;
    bool noSSLlib;
    OAuthSite site;

    QVBoxLayout *layout;

    // QUrl split into QUrlQuerty in QT5
#if defined(NOWEBKIT) || ((QT_VERSION > 0x050000) && defined(Q_OS_MAC))
    QWebEngineView *view;
#else
    QWebView *view;
#endif

    QNetworkAccessManager* manager;

    QUrl url;

#ifdef GC_HAVE_KQOAUTH
    KQOAuthManager *oauthManager;
    KQOAuthRequest *oauthRequest;
#endif

};

#endif // OAUTHDIALOG_H
