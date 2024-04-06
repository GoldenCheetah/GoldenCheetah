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
#include "Measures.h"
#include "WithingsReading.h"

class WithingsDownload : public QObject
{
    Q_OBJECT
    G_OBJECT


public:
    WithingsDownload(Context *context);
    bool getBodyMeasures(QString &error, QDateTime from, QDateTime to, QList<Measure> &data);

signals:
    void downloadStarted(int);
    void downloadProgress(int);
    void downloadEnded(int);

private:
    Context *context;
    QNetworkAccessManager *nam;
    QString response;

    QEventLoop loop;

    QJsonParseError parse(QString text, QList<Measure> &bodyMeasures);
    QList<WithingsReading> jsonDocumentToWithingsReading(QJsonDocument withingsJson);

private slots:

    void downloadFinished(QNetworkReply *reply);
};
#endif
