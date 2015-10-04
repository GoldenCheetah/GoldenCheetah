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

#include "NetworkFileStore.h"
#include "Athlete.h"
#include "Settings.h"

NetworkFileStore::NetworkFileStore(Context *context) : FileStore(context), context(context), root_(NULL) {

}

NetworkFileStore::~NetworkFileStore() {

}

// open by connecting and getting a basic list of folders available
bool
NetworkFileStore::open(QStringList &errors)
{

    QString folder = appsettings->cvalue(context->athlete->cyclist, GC_NETWORKFILESTORE_FOLDER, "").toString();
    if (folder == "") {
        errors << tr("You must define a network folder first");
        return false;
    }

    QDir folder_dir = QDir(folder);
    if (!folder_dir.exists()) {
        errors << tr("Folder %1 does not exist/is not accessible").arg(folder);
        return false;
    }

    // we have a root
    root_ = newFileStoreEntry();

    // path name
    root_->name = folder_dir.canonicalPath();
    root_->isDir = true;
    root_->size = 1;

    // ok so far ?
    if (errors.count()) return false;
    return true;
}

bool 
NetworkFileStore::close()
{
    // nothing to do for now
    return true;
}

// home dire
QString
NetworkFileStore::home()
{
    return appsettings->cvalue(context->athlete->cyclist, GC_NETWORKFILESTORE_FOLDER, "").toString();
}

bool
NetworkFileStore::createFolder(QString path)
{
    // not used for this FileStore, since the standard QFileDialog is used to define the Directory
    return false;
}

QList<FileStoreEntry*> 
NetworkFileStore::readdir(QString path, QStringList &errors)
{
    QList<FileStoreEntry*> returning;

    QDir current_path = QDir(path);
    if (!current_path.exists()) {
        errors << tr("Folder %1 does not exist").arg(path);
        return returning;
    }

    QFileInfoList files = current_path.entryInfoList();
    foreach (QFileInfo info, files) {

        FileStoreEntry *add = newFileStoreEntry();

        //QFileInfo has full path, we just want the file name
        add->name = info.fileName();
        add->isDir = info.isDir();
        add->size = info.size();

        // dates in format "Tue, 19 Jul 2011 21:55:38 +0000"
        add->modified = info.lastModified();

        returning << add;

    }

    // all good ?
    return returning;
}

// read a file at location (relative to home) into passed array
bool
NetworkFileStore::readFile(QByteArray *data, QString remotename)
{

    // is the path set ?
    QString path = appsettings->cvalue(context->athlete->cyclist, GC_NETWORKFILESTORE_FOLDER, "").toString();
    if (path == "") return false;

    // open the path
    QDir current_path = QDir(path);
    if (!current_path.exists()) return false;

    QFile file(path+"/"+remotename);
    if (!file.exists()) return false;

    if (file.open(QIODevice::ReadOnly)) {
        *data = file.readAll();
        file.close();
    } else {
        file.close();
        return false;
    };

    emit readComplete(data, "", tr("Completed"));

    return true;
}

bool 
NetworkFileStore::writeFile(QByteArray &data, QString remotename)
{

    // is the path set ?
    QString path = appsettings->cvalue(context->athlete->cyclist, GC_NETWORKFILESTORE_FOLDER, "").toString();
    if (path == "") {
        emit writeComplete("", tr("You must define a network folder first"));  // required for single upload to get to an end
        return false;
    };

    // open the path
    QDir current_path = QDir(path);
    if (!current_path.exists()) {
        emit writeComplete("", tr("Write to folder %1 failed").arg(path));  // required for single upload to get to an end
        return false;
    };

    QFile file(path+"/"+remotename);
    if (file.open(QIODevice::WriteOnly) ) {
        file.write(data);
        file.close();
    } else {
        file.close();
        emit writeComplete("", tr("Write to folder %1 failed").arg(path));  // required for single upload to get to an end
        return false;
    };

    emit writeComplete("", tr("Completed"));

    return true;
}


