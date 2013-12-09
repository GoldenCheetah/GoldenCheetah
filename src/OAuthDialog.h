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
#include <QObject>
#include <QtGui>
#include <QWidget>
#include <QStackedLayout>
#include <QtWebKit>
#include <QWebView>
#include <QWebFrame>
#include <QUrl>
#include <QUrlQuery>

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





signals:

private slots:
    void urlChanged(const QUrl& url);
    void loadFinished();


private:
    Context *context;
    OAuthSite site;

    QVBoxLayout *layout;
    QWebView *view;

    QUrl url;

    bool requestToken;
    bool requestAuth;

};

#endif // OAUTHDIALOG_H
