/*
 * Copyright (c) 2015 Joern Rischmueller (joern.rm@gmail.com)
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

#ifndef GC_LocalFileStore_h
#define GC_LocalFileStore_h

#include "CloudService.h"
#include <QImage>

class LocalFileStore : public CloudService {

    Q_OBJECT

    public:

        LocalFileStore(Context *context);
        CloudService *clone(Context *context) { return new LocalFileStore(context); }
        ~LocalFileStore();

        QString id() const { return "Local Store"; }
        QString uiName() const { return tr("Local Store"); }
        QString description() const { return (tr("Sync with a local folder or thumbdrive.")); }
        QImage logo() const { return QImage(":images/services/localstore.png"); }


        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        // home directory
        QString home();

        // write a file 
        bool writeFile(QByteArray &data, QString remotename, RideFile *ride);

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString);

        // create a folder
        bool createFolder(QString path);

        // dirent style api
        CloudServiceEntry *root() { return root_; }
        QList<CloudServiceEntry*> readdir(QString path, QStringList &errors);

    private:
        Context *context;
        CloudServiceEntry *root_;

};
#endif
