/*
 * Copyright (c) 2017 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#ifndef GC_Xert_h
#define GC_Xert_h

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QImage>

class Xert : public CloudService {

    Q_OBJECT

    public:

        Xert(Context *context);
        CloudService *clone(Context *context) { return new Xert(context); }
        ~Xert();

        QString id() const { return "Xert"; }
        QString uiName() const { return tr("Xert"); }
        QString description() const { return(tr("Sync with the innovative site for fitness monitoring, tracking, and planning.")); }
        QImage logo() const { return QImage(":images/services/xert.png"); }

        // now upload only and authenticates with a user and password
        int capabilities() const { return UserPass | OAuth | Upload | Query; }

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

        QString getRideName(RideFile *ride);
        QJsonObject readActivityDetail(QString path, bool withSessionData);
};
#endif
