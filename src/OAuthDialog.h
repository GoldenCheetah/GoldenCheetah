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
#include <QtWebKit>
#include <QWebView>
#include <QWebFrame>

// QUrl split into QUrlQuerty in QT5
#if QT_VERSION > 0x050000
#include <QUrlQuery>
#endif


class OAuthDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

public:
    typedef enum {
        STRAVA,
        TWITTER,
        CYCLING_ANALYTICS
    } OAuthSite;

    OAuthDialog(Context *context, OAuthSite site);

private slots:

    // Strava/Cyclinganalytics
    void urlChanged(const QUrl& url);
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
    OAuthSite site;

    QVBoxLayout *layout;
    QWebView *view;
    QNetworkAccessManager* manager;

    QUrl url;

#ifdef GC_HAVE_KQOAUTH
    KQOAuthManager *oauthManager;
    KQOAuthRequest *oauthRequest;
#endif

};

#endif // OAUTHDIALOG_H
