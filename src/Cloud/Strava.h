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

#ifndef GC_Strava_h
#define GC_Strava_h

#include "CloudService.h"

class QNetworkReply;
class QNetworkAccessManager;

class Strava : public CloudService {

    Q_OBJECT

    public:

        QString id() const { return "Strava"; }
        QString uiName() const { return tr("Strava"); }
        QString description() const { return (tr("Sync with the social network for cyclists and runners.")); }
        QImage logo() const;

        Strava(Context *context);
        CloudService *clone(Context *context) { return new Strava(context); }
        ~Strava();

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        //virtual int capabilities() const { return OAuth | Upload | Download | Query ; } // Default

        QString authiconpath() const { return QString(":images/services/strava_connect.png"); }

        // write a file
        bool writeFile(QByteArray &data, QString remotename, RideFile *ride);

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString remoteid);

        // dirent style api
        CloudServiceEntry *root() { return root_; }
        QList<CloudServiceEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to);

    public slots:

        // getting data
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

        QByteArray* prepareResponse(QByteArray* data);

        void addSamples(RideFile* ret, QString remoteid);
        void fixLapSwim(RideFile* ret, QJsonArray laps);
        void fixSmartRecording(RideFile* ret);

    private slots:
        void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);
};
#endif
