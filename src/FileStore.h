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
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QObject>
#include "Context.h"

// A filestore is a base class for working with data stores
// we want to sync or backup to. The initial version is to support
// working with Dropbox but could be extended to support other
// stores including Google, Microsoft "cloud" storage or even
// to sync / backup to a pen drive or similar

class FileStoreEntry;
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

        // we use a dirent style API 
        // root - get me the root of the store
        // readdir - get me the contents of a path
        virtual FileStoreEntry *root() { return NULL; }
        virtual QList<FileStoreEntry*> readdir(QString path, QStringList &errors) { 
            Q_UNUSED(path); errors << "not implemented."; return QList<FileStoreEntry*>(); 
        }

    protected:

        // if you want a new filestoreentry struct
        // we manage them in the file store so you
        // don't have to. When the filestore is deleted
        // these entries are deleted too
        FileStoreEntry *newFileStoreEntry();
        QList<FileStoreEntry*> list_;

    private:
        Context *context;
        
};

class FileStoreEntry
{
    public:

        // about me
        QString name;                       // file name
        bool isDir;                         // is a directory
        unsigned long size;                 // my size
        QDateTime modified;                 // last modification date

        // parent and children
        FileStoreEntry *parent;             // parent directory, NULL for root.
        QList<FileStoreEntry *> children;   // parent directory, NULL for root.
};
#endif
