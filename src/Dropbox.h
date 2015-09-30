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

    public:

        Dropbox(Context *context);
        ~Dropbox();

        QString name() { return (tr("Dropbox Cloud Storage")); }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        bool createFolder(QString path);

        FileStoreEntry *root() { return root_; }
        QList<FileStoreEntry*> readdir(QString path, QStringList &errors);

    private:
        Context *context;
        QNetworkAccessManager *nam;
        FileStoreEntry *root_;
};
#endif
