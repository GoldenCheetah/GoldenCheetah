/*
 * Copyright (c) 2012 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#ifndef _Gc_ZeoDownload_h
#define _Gc_ZeoDownload_h
#include "GoldenCheetah.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "Context.h"
#include "Settings.h"
#include "WithingsParser.h"
#include "MetricAggregator.h"

class ZeoDownload : public QObject
{
    Q_OBJECT
    G_OBJECT


public:
    ZeoDownload(Context *context);
    bool download();

public slots:
    void downloadFinished(QNetworkReply *reply);

private:
    Context *context;
    QNetworkAccessManager *nam;

    QString host;
    QString port;
    QString base_api_url;
    QString api_key;
    QString usernameAndPasswordEncoded;
    QList<QDate> dates;

    int allMeasures;
    int newMeasures;

    bool nextDate();
};
#endif
