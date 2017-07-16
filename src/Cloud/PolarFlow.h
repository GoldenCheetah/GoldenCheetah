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

#ifndef GC_PolarFlow_h
#define GC_PolarFlow_h

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QImage>

// OAuth domain (requires client id)
#define GC_POLARFLOW_OAUTH_URL "https://flow.polar.com/oauth2/authorization"

// Access token (requires client id and secret)
#define GC_POLARFLOW_TOKEN_URL "https://polarremote.com/v2/oauth2/token"

// Request URL (requires access token)
#define GC_POLARFLOW_URL "https://www.polaraccesslink.com"

class PolarFlow : public CloudService {

    Q_OBJECT

    public:

        PolarFlow(Context *context);
        CloudService *clone(Context *context) { return new PolarFlow(context); }
        ~PolarFlow();

        QString id() const { return "PolarFlow"; }
        QString uiName() const { return tr("PolarFlow"); }
        QString description() const { return (tr("Download from the popular Polar website.")); }
        QImage logo() const { return QImage(":images/services/polarflow.png"); }

        // polar
        virtual int capabilities() const { return OAuth | Download | Query; }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString remoteid);

        // dirent style api
        CloudServiceEntry *root() { return root_; }
        QList<CloudServiceEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to);

    public slots:

        // getting data
        void readyRead(); // a readFile operation has work to do
        void readFileCompleted();

    private:
        Context *context;
        QNetworkAccessManager *nam;
        QNetworkReply *reply;
        CloudServiceEntry *root_;

        QMap<QNetworkReply*, QByteArray*> buffers;

        QString userId;

    private slots:
        void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);
};
#endif
