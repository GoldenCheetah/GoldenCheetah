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
#include <QNetworkAccessManager>

class GoogleDrive : public FileStore {

    Q_OBJECT

    public:
        GoogleDrive(Context *context);
        ~GoogleDrive();

        QString name() { return (tr("Google Drive Cloud Storage")); }

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
        void MaybeRefreshCredentials();
        QJsonArray GetFiles(const QString& remote_name, const QString& path,
                            const QString& token);
        // Returns the file id or an empty string if the file does not exist.
        QString GetFileID(const QString& remote_name, const QString& path,
                          const QString& token);
        // Get the download url so we can fetch the file.
        QString GetDownloadURL(const QString& remote_name, const QString& path,
                               const QString& token);
        // Fetches a JSON document from the given URL.
        QJsonDocument FetchNextLink(const QString& url, const QString& token);

        static QNetworkRequest MakeRequestWithURL(
            const QString& url, const QString& token, const QString& args);
        static QNetworkRequest MakeRequest(
            const QString& token, const QString& args);
        // Make QString returns a "q" string usable for searches in Google
        // Drive.
        static QString MakeQString(const QString& remote_name,
                                   const QString& path);

        Context *context_;
        QNetworkAccessManager *nam_;
        QNetworkReply *reply_;
        FileStoreEntry *root_;

        QMap<QNetworkReply*, QByteArray*> buffers;
};
#endif  // GC_GOOGLE_DRIVE_H
