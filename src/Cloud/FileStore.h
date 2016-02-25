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

#ifndef GC_FileStore_h
#define GC_FileStore_h

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QObject>
#include <QNetworkReply>

#include <QDialog>
#include <QCheckBox>
#include <QTreeWidget>
#include <QSplitter>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>

#include "Context.h"

// A filestore is a base class for working with data stores
// we want to sync or backup to. The initial version is to support
// working with Dropbox but could be extended to support other
// stores including Google, Microsoft "cloud" storage or even
// to sync / backup to a pen drive or similar

class FileStoreEntry;
class RideItem;
class FileStore : public QObject {

    Q_OBJECT

    public:

        // REIMPLEMENT EACH FROM BELOW FOR A NEW
        // TYPE OF FILESTORE. SEE Dropbox.{h,cpp}
        // FOR A REFERENCE IMPLEMENTATION THE

        FileStore(Context *context);
        virtual ~FileStore();

        // The following must be reimplemented
        virtual QString name() { return (tr("Base class")); }

        // open/connect and close/disconnect
        virtual bool open(QStringList &errors) { Q_UNUSED(errors); return false; }
        virtual bool close() { return false; }

        // what is the path to the home directory on this store
        virtual QString home() { return "/"; }

        // create a folder
        virtual bool createFolder(QString path) { Q_UNUSED(path); return false; }

        // write a file - call notify when done
        virtual bool writeFile(QByteArray &data, QString remotename) {
            Q_UNUSED(data); Q_UNUSED(remotename); return false;
        }
        void notifyWriteComplete(QString name,QString message) { emit writeComplete(name,message); }

        // read a file  and notify when done
        virtual bool readFile(QByteArray *data, QString remotename) {
            Q_UNUSED(data); Q_UNUSED(remotename); return false;
        }
        void notifyReadComplete(QByteArray *data, QString name, QString message) { emit readComplete(data,name,message); }

        // we use a dirent style API for traversing
        // root - get me the root of the store
        // readdir - get me the contents of a path
        virtual FileStoreEntry *root() { return NULL; }
        virtual QList<FileStoreEntry*> readdir(QString path, QStringList &errors) { 
            Q_UNUSED(path); errors << "not implemented."; return QList<FileStoreEntry*>(); 
        }

        // UTILITY
        void mapReply(QNetworkReply *reply, QString name) { replymap_.insert(reply,name); }
        QString replyName(QNetworkReply *reply) { return replymap_.value(reply,""); }
        void compressRide(RideFile*ride, QByteArray &data, QString name);
        RideFile *uncompressRide(QByteArray *data, QString name, QStringList &errors);

        // PUBLIC INTERFACES. DO NOT REIMPLEMENT
        static bool upload(QWidget *parent, FileStore *store, RideItem*);

    signals:
        void writeComplete(QString name, QString message);
        void readComplete(QByteArray *data, QString name, QString message);

    protected:

        // if you want a new filestoreentry struct
        // we manage them in the file store so you
        // don't have to. When the filestore is deleted
        // these entries are deleted too
        FileStoreEntry *newFileStoreEntry();
        QMap<QNetworkReply*,QString> replymap_;
        QList<FileStoreEntry*> list_;

    private:
        Context *context;
        
};

// UPLOADER dialog to upload a single rideitem to the file
//          store. Typically as a quick ^U type operation or
//          via a MainWindow menu option
class FileStoreUploadDialog : public QDialog
{

    Q_OBJECT

    public:
        FileStoreUploadDialog(QWidget *parent, FileStore *store, RideItem *item);

        QLabel *info;               // how much being uploaded / status
        QProgressBar *progress;     // whilst we wait
        QPushButton *okcancel;      // cancel whilst occurring, ok once done

    public slots:
        void completed(QString name, QString message);

    private:
        FileStore *store;
        RideItem *item;
        QByteArray data;            // compressed data to upload
};

// XXX a better approach might be to reimplement QFileSystemModel on 
// a FileStore and use the standard file dialogs instead. XXX
class FileStoreDialog : public QDialog
{
    Q_OBJECT

    public:
        FileStoreDialog(QWidget *parent, FileStore *store, QString title, QString pathname, bool dironly=false);
        QString pathnameSelected() { return pathname; }

    public slots:

        // trap enter pressed as we don't want to close on it
        bool eventFilter(QObject *obj, QEvent *evt);

        // user hit return on the path edit
        void returnPressed();
        void folderSelectionChanged();
        void folderClicked(QTreeWidgetItem*, int);

        // user double clicked on a file
        void fileDoubleClicked(QTreeWidgetItem*, int);

        // user typed or selected a path
        void setPath(QString path, bool refresh=false);

        // set the folder or files list
        void setFolders(FileStoreEntry *fse);
        void setFiles(FileStoreEntry *fse);

        // create a folder
        void createFolderClicked();

    protected:
        QLineEdit   *pathEdit; // full path shown
        QSplitter   *splitter; // left and right had side
        QTreeWidget *folders;  // left side folder list
        QTreeWidget *files;    // right side "files" list

        QPushButton *cancel;   // ugh. did the wrong thing
        QPushButton *open;     // open the selected "file"
        QPushButton *create;   // create a folder

        FileStore *store;
        QString title;
        QString pathname;
        bool dironly;
};

class FolderNameDialog : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(FolderNameDialog)
    public:
        FolderNameDialog(QWidget *parent);
        QString name() { return nameEdit->text(); }
        
        QLineEdit   *nameEdit; // full path shown
        QPushButton *cancel;   // ugh. did the wrong thing
        QPushButton *create;     // use name we just provided

};

//
// The Sync Dialog
//
class FileStoreSyncDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        FileStoreSyncDialog(Context *context, FileStore *store);
	
    public slots:

        void cancelClicked();
        void refreshClicked();
        void tabChanged(int);
        void downloadClicked();
        void refreshCount();
        void refreshUpCount();
        void refreshSyncCount();
        void selectAllChanged(int);
        void selectAllUpChanged(int);
        void selectAllSyncChanged(int);

        void completedRead(QByteArray *data, QString name, QString message);
        void completedWrite(QString name,QString message);
    private:
        Context *context;
        FileStore *store;

        bool downloading;
        bool sync;
        bool aborted;

        // Quick lists for checking if file exists
        // locally (rideFiles) or remotely (uploadFiles)
        QStringList rideFiles;
        QStringList uploadFiles;

        // keeping track of progress...
        int downloadcounter,    // *x* of n downloading
            downloadtotal,      // x of *n* downloading
            successful,         // how many downloaded ok?
            listindex;          // where in rideList we've got to

        bool saveRide(RideFile *, QStringList &);
        bool syncNext();        // kick off another download/upload
                                // returns false if none left
        bool downloadNext();    // kick off another download
                                // returns false if none left
        bool uploadNext();     // kick off another upload
                                // returns false if none left

        // tabs - Upload/Download
        QTabWidget *tabs;

        // athlete selection
        //QMap<QString, QString> athlete;
        QComboBox *athleteCombo;

        QPushButton *refreshButton;
        QPushButton *cancelButton;
        QPushButton *downloadButton;

        QDateEdit *from, *to;

        // Download
        QCheckBox *selectAll;
        QTreeWidget *rideListDown;

        // Upload
        QCheckBox *selectAllUp;
        QTreeWidget *rideListUp;

        // Sync
        QCheckBox *selectAllSync;
        QTreeWidget *rideListSync;
        QComboBox *syncMode;

        // show progress
        QProgressBar *progressBar;
        QLabel *progressLabel;

        QCheckBox *overwrite;
};

// Representing a File or Folder
class FileStoreEntry
{
    public:

        // THESE MEMBERS NEED TO BE MAINTAINED BY
        // THE FILESTORE IMPLEMENTATION (Dropbox, Google etc)
        QString name;                       // file name
        bool isDir;                         // is a directory
        unsigned long size;                 // my size
        QDateTime modified;                 // last modification date

        // This is just file metadata written by the implementation.
        //QMap<QString, QString> metadata;
        // THESE MEMBERS ARE MAINTAINED BY THE 
        // FILESTORE BASE IMPLEMENTATION
        FileStoreEntry *parent;             // parent directory, NULL for root.
        QList<FileStoreEntry *> children;   // parent directory, NULL for root.
        bool initial;                       // haven't scanned for children yet.

        // find the index of a child, return -1 if not found
        int child(QString directory) {
            bool found = false;
            int i = 0;
            for(; i<children.count(); i++) {
                if (children[i]->name == directory) {
                    found = true;
                    break;
                }
            }
            if (found) return i;
            else return -1;
        }
};
#endif
