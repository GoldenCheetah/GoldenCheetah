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

#ifndef GC_SportsPlusHealth_h
#define GC_SportsPlusHealth_h
#include <QImage>

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>

class SportsPlusHealth : public CloudService {

    Q_OBJECT

    public:

        QString id() const { return "SportPlusHealth"; }
        QString uiName() const { return tr("SportPlusHealth"); }
        QString description() const { return (tr("Upload to the cycling and running site.")); }
        QImage logo() const { return QImage(":images/services/sportplushealth.png"); }

        SportsPlusHealth(Context *context);
        CloudService *clone(Context *context) { return new SportsPlusHealth(context); }
        ~SportsPlusHealth();

        // upload only and authenticates with a user and password
        int capabilities() const { return UserPass | Upload; }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        // write a file
        bool writeFile(QByteArray &data, QString remotename, RideFile *ride);

    public slots:

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
