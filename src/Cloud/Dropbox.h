/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#ifndef GC_Dropbox_h
#define GC_Dropbox_h

#include "FileStore.h"
#include <QNetworkAccessManager>

class Dropbox : public FileStore {

    Q_OBJECT

    public:

        Dropbox(Context *context);
        ~Dropbox();

        QString name() { return (tr("Dropbox Cloud Storage")); }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        // home directory
        QString home();

        // write a file 
        bool writeFile(QByteArray &data, QString remotename);

        // read a file
        bool readFile(QByteArray *data, QString remotename); 

        // create a folder
        bool createFolder(QString path);

        // dirent style api
        FileStoreEntry *root() { return root_; }
        QList<FileStoreEntry*> readdir(QString path, QStringList &errors);

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
};
#endif
