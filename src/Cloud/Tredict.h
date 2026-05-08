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

#ifndef GC_Tredict_h
#define GC_Tredict_h

#include "CloudService.h"

class QNetworkReply;
class QNetworkAccessManager;

class Tredict : public CloudService {

    Q_OBJECT

    public:

        QString id() const { return "Tredict"; }
        QString uiName() const { return tr("Tredict"); }
        QString description() const { return (tr("Sync activities, body measures, HRV and planned workouts with Tredict.")); }
        QImage logo() const;

        Tredict(Context *context);
        CloudService *clone(Context *context) { return new Tredict(context); }
        ~Tredict();

        int capabilities() const { return OAuth | Upload | Download | Query; }
        int type() const { return Activities | Measures; }

        bool open(QStringList &errors);
        bool close();

        bool writeFile(QByteArray &data, QString remotename, RideFile *ride);
        bool readFile(QByteArray *data, QString remotename, QString remoteid);

        CloudServiceEntry *root() { return root_; }
        QList<CloudServiceEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to);

    public slots:

        void readyRead();
        void readFileCompleted();
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

#endif // GC_Tredict_h
