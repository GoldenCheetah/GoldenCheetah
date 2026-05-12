/*
 * Copyright (c) 2026 Felix Gertz (felix@tredict.com)
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

#ifndef GC_TredictMeasuresDownload_h
#define GC_TredictMeasuresDownload_h

#include "GoldenCheetah.h"

#include <QObject>
#include <QNetworkAccessManager>

#include "Context.h"
#include "Measures.h"

class TredictMeasuresDownload : public QObject
{
    Q_OBJECT
    G_OBJECT

public:
    TredictMeasuresDownload(Context *context);

    bool getBodyMeasures(QString &error, QDateTime from, QDateTime to, QList<Measure> &data);
    bool getHrvMeasures(QString &error, QDateTime from, QDateTime to, QList<Measure> &data);

signals:
    void downloadStarted(int);
    void downloadProgress(int);
    void downloadEnded(int);

private:
    bool refreshToken(QString &error);
    QByteArray fetchEndpoint(const QString &url, QString &error);

    Context *context;
    QNetworkAccessManager *nam;
};

#endif
