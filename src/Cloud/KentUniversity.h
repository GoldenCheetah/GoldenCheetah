/*
 * Copyright (c) 2015 Magnus Gille (mgille@gmail.com)
 *               2017 Mark Liversedge (liversedge@gmail.com)
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

#ifndef GC_KENT_UNI_H
#define GC_KENT_UNI_H

#include "GoogleDrive.h"

#include <QBuffer>
#include <QMap>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QTextEdit>
#include <QNetworkRequest>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QString>
#include <QImage>

// Kent Uni studies share data using Google Drive, but there is only an uploader
// and the file format is specific to their needs (csv file). Metadata is sent
// in a separate text file, with RPE and ROF mandated.
class KentUniversity : public GoogleDrive {

    Q_OBJECT

    public:
        KentUniversity(Context *context);
        CloudService *clone(Context *context) { return new KentUniversity(context); }
        ~KentUniversity();

        virtual QString id() const { return "University of Kent"; }
        virtual QString uiName() const { return tr("University of Kent"); }
        virtual QString description() const { return (tr("Participate in academic studies sharing data via google drive.")); }
        QImage logo() const { return QImage(":images/services/unikent.jpg"); }

        // open/connect and close/disconnect
        virtual bool open(QStringList &errors);
        virtual bool close();

        // home directory
        virtual QString home();

        // write a file
        virtual bool writeFile(QByteArray &data, QString remotename, RideFile *ride);

        // read a file
        virtual bool readFile(QByteArray *data, QString remotename, QString);

        // create a folder
        virtual bool createFolder(QString path);
        void folderSelected(QString path);

        // dirent style api
        virtual CloudServiceEntry *root() { return root_; }
        // Readdir reads the files from the remote side and updates root_dir_
        // with a local cache. readdir will read ALL files and refresh
        // everything.
        virtual QList<CloudServiceEntry*> readdir(
            QString path, QStringList &errors);

        // Returns the fild id or "" if no file was found, uses the local
        // cache to determine file id.
        QString GetFileId(const QString& path);

    public slots:

        // getting data
        void readyRead(); // a readFile operation has work to do
        void readFileCompleted();

        // sending data
        void writeFileCompleted();

        // dealing with SSL handshake problems
        void onSslErrors(QNetworkReply*reply, const QList<QSslError> & );

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
        CloudServiceEntry *root_;
        QString root_directory_id_;
        QScopedPointer<FileInfo> root_dir_;

        QMap<QNetworkReply*, QByteArray*> buffers_;
        QMap<QNetworkReply*, QSharedPointer<QBuffer> > patch_buffers_;
        QMutex mu_;
};

// SPECIAL UPLOADER dialog to upload a single rideitem but ensure
//          the relevant metadata and data quality is completed.
class KentUniversityUploadDialog : public QDialog
{

    Q_OBJECT

    public:
        KentUniversityUploadDialog(QWidget *parent, CloudService *store, RideItem *item);

        QLabel *feedback, *rpelabel, *roflabel;
        QComboBox *rpe, *rof;
        QLabel *noteslabel, *reasonlabel;
        QTextEdit *notes, *reasons;
        QCheckBox *isTest;

        QLabel *info;               // how much being uploaded / status
        QProgressBar *progress;     // whilst we wait
        QPushButton *okcancel;      // ok to go, cancel when working, then done at end

    public slots:
        //int exec();
        void check(); // check if all the fields are set, and set button as needed
        void uploadFile();
        void completed(QString name, QString message);

    private:
        CloudService *store;
        RideItem *item;
        QByteArray data;            // compressed data to upload
        bool status;                // did upload get kicked off ok?
};
#endif  // GC_KENT_UNI_H
