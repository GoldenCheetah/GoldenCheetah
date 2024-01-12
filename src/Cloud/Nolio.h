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

#ifndef NOLIO_H
#define NOLIO_H

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QImage>

class Nolio : public CloudService {

    Q_OBJECT

    public:

        Nolio(Context *context);
        CloudService *clone(Context *context) { return new Nolio(context); }
        ~Nolio();

        QString id() const { return "Nolio"; }
        QString uiName() const { return tr("Nolio"); }
        QString description() const { return (tr("Sync with your favorite training partner.")); }
        QImage logo() const { return QImage(":images/services/nolio.png"); }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        virtual int capabilities() const { return OAuth | Download | Query; }
        // home directory
        QString home();

        QList<CloudServiceEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to);

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString remoteid);
        QByteArray* prepareResponse(QByteArray* data);

        // create a folder
        bool createFolder(QString);

        // athlete selection
        QList<CloudServiceAthlete> listAthletes();
        bool selectAthlete(CloudServiceAthlete);

        // dirent style api
        CloudServiceEntry *root() { return root_; }

    public slots:
        // getting data
        void readyRead(); // a readFile operation has work to do
        void readFileCompleted();

    private:
        Context *context;
        QNetworkAccessManager *nam;
        CloudServiceEntry *root_;

        QMap<QNetworkReply*, QByteArray*> buffers;


    private slots:
        void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);
};

#endif // NOLIO_H
