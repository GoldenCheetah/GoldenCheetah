/*
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

#ifndef GC_CyclingAnalytics_h
#define GC_CyclingAnalytics_h

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QImage>

class CyclingAnalytics : public CloudService {

    Q_OBJECT

    public:

        QString id() const { return "Cycling Analytics"; }
        QString uiName() const { return tr("Cycling Analytics"); }
        QString description() const { return (tr("Sync with the power focused cycling site.")); }
        QImage logo() const { return QImage(":images/services/cyclinganalytics.png"); }

        CyclingAnalytics(Context *context);
        CloudService *clone(Context *context) { return new CyclingAnalytics(context); }
        ~CyclingAnalytics();

        // upload only and authenticates with OAuth tokens
        int capabilities() const { return OAuth | Upload | Download | Query; }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        // get list of rides
        QList<CloudServiceEntry *> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to);

        // write a file
        bool writeFile(QByteArray &data, QString remotename, RideFile *ride);

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString remoteid);


    public slots:

        // fetching data
        void readyRead(); // a readFile operation has work to do
        void readFileCompleted();

        // sending data
        void writeFileCompleted();

    private:
        Context *context;
        QNetworkAccessManager *nam;
        QNetworkReply *reply;
        CloudServiceEntry *root_;

        QMap<QNetworkReply*, QByteArray*> buffers;

    private slots:
        void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);
};
#endif
