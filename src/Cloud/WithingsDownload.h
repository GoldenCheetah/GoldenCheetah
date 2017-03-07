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

#ifndef _Gc_WithingsDownload_h
#define _Gc_WithingsDownload_h
#include "GoldenCheetah.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "Context.h"
#include "Settings.h"
#include "WithingsParser.h"
#include "BodyMeasures.h"

#ifdef GC_HAVE_KQOAUTH
#include <kqoauthmanager.h>
#endif

class WithingsDownload : public QObject
{
    Q_OBJECT
    G_OBJECT


public:
    WithingsDownload(Context *context);
    bool getBodyMeasures(QString &error, QDateTime from, QDateTime to, QList<BodyMeasure> &data);

signals:
    void downloadStarted(int);
    void downloadProgress(int);
    void downloadEnded(int);

private:
    Context *context;
    QNetworkAccessManager *nam;
    WithingsParser *parser;
    QString response;

    #ifdef GC_HAVE_KQOAUTH
    KQOAuthManager *oauthManager;
    KQOAuthRequest *oauthRequest;
    QEventLoop loop;
    #endif

    void parse(QString text, QStringList &errors, QList<BodyMeasure> &bodyMeasures);

private slots:

    void downloadFinished(QNetworkReply *reply);
    #ifdef GC_HAVE_KQOAUTH
    void onRequestReady(QByteArray);
    void onAuthorizedRequestDone();
    #endif
};
#endif
