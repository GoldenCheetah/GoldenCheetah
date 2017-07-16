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

#ifndef GC_SportTracks_h
#define GC_SportTracks_h

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QImage>

class SportTracks : public CloudService {

    Q_OBJECT

    public:

        SportTracks(Context *context);
        CloudService *clone(Context *context) { return new SportTracks(context); }
        ~SportTracks();

        QString id() const { return "SportTracks.mobi"; }
        QString uiName() const { return tr("SportTracks.mobi"); }
        QString description() const { return (tr("Sync with the popular multisport website.")); }
        QImage logo() const { return QImage(":images/services/sporttracks.png"); }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString remoteid);

        // write a file
        bool writeFile(QByteArray &data, QString remotename, RideFile *ride);

        // dirent style api
        CloudServiceEntry *root() { return root_; }
        QList<CloudServiceEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to);

    public slots:

        // getting data
        void readyRead(); // a readFile operation has work to do
        void readFileCompleted();
        void writeFileCompleted();

    private:
        Context *context;
        QNetworkAccessManager *nam;
        CloudServiceEntry *root_;

        QMap<QNetworkReply*, QByteArray*> buffers;

        QString userId;

    private slots:
        void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);
};
#endif
