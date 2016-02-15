/*
 * Copyright (c) 2015 Magnus Gille (mgille@gmail.com)
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

#ifndef GC_GOOGLE_DRIVE_H
#define GC_GOOGLE_DRIVE_H

#include "FileStore.h"

#include <QBuffer>
#include <QMap>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QString>

class GoogleDrive : public FileStore {

    Q_OBJECT

    public:
        GoogleDrive(Context *context);
        virtual ~GoogleDrive();

        virtual QString name() { return (tr("Google Drive Cloud Storage")); }

        // open/connect and close/disconnect
        virtual bool open(QStringList &errors);
        virtual bool close();

        // home directory
        virtual QString home();

        // write a file
        virtual bool writeFile(QByteArray &data, QString remotename);

        // read a file
        virtual bool readFile(QByteArray *data, QString remotename);

        // create a folder
        virtual bool createFolder(QString path);

        // dirent style api
        virtual FileStoreEntry *root() { return root_; }
        // Readdir reads the files from the remote side and updates root_dir_
        // with a local cache. readdir will read ALL files and refresh
        // everything.
        virtual QList<FileStoreEntry*> readdir(
            QString path, QStringList &errors);

        static QString GetScope(Context* context);

        // Returns the fild id or "" if no file was found, uses the local
        // cache to determine file id.
        QString GetFileId(const QString& path);

    public slots:

        // getting data
        void readyRead(); // a readFile operation has work to do
        void readFileCompleted();

        // sending data
        void writeFileCompleted();

    private:
        struct FileInfo;

        void MaybeRefreshCredentials();

        // Fetches a JSON document from the given URL.
        QJsonDocument FetchNextLink(const QString& url, const QString& token);

        FileInfo* WalkFileInfo(const QString& path, bool foo);

        FileInfo* BuildDirectoriesForAthleteDirectory(const QString& path);

        static QNetworkRequest MakeRequestWithURL(
            const QString& url, const QString& token, const QString& args);
        static QNetworkRequest MakeRequest(
            const QString& token, const QString& args);
        // Make QString returns a "q" string usable for searches in Google
        // Drive.
        static QString MakeQString(const QString& parent);
        QString GetRootDirId();

        Context *context_;
        QNetworkAccessManager *nam_;
        FileStoreEntry *root_;
        QString root_directory_id_;
        QScopedPointer<FileInfo> root_dir_;

        QMap<QNetworkReply*, QByteArray*> buffers_;
        QMap<QNetworkReply*, QSharedPointer<QBuffer> > patch_buffers_;
        QMutex mu_;
};

#endif  // GC_GOOGLE_DRIVE_H
