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

#ifndef GC_SixCycle_h
#define GC_SixCycle_h

#include "FileStore.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>

class SixCycle : public FileStore {

    Q_OBJECT

    public:

        SixCycle(Context *context);
        ~SixCycle();

        QString name() { return (tr("SixCycle")); }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        // home directory
        QString home();

        // write a file
        bool writeFile(QByteArray &data, QString remotename);

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString remoteid);

        // create a folder
        bool createFolder(QString);

        // dirent style api
        FileStoreEntry *root() { return root_; }
        QList<FileStoreEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to);

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
        FileStoreEntry *root_;

        QMap<QNetworkReply*, QByteArray*> buffers;

        // once authenticated we get a token to access and a session user id (as a URL)
        QString session_token;
        QString session_user;

    private slots:
        void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);
};
#endif
