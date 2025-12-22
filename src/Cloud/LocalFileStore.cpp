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

#include "LocalFileStore.h"
#include "Athlete.h"
#include "Settings.h"

LocalFileStore::LocalFileStore(Context *context) : CloudService(context), context(context) {

    if (context) {
        // we have a root
        root_ = newCloudServiceEntry();

        // root is always root on a local file store
        root_->name = "/";
        root_->isDir = true;
        root_->size = 1;
    }

    // settings
    settings.insert(Folder, GC_NETWORKFILESTORE_FOLDER);
}

LocalFileStore::~LocalFileStore() {

}

// open by connecting and getting a basic list of folders available
bool
LocalFileStore::open(QStringList &errors)
{

    QString folder = getSetting(GC_NETWORKFILESTORE_FOLDER, "").toString();
    if (folder == "") {
        errors << tr("You must define a network folder first");
        return false;
    }

    QDir folder_dir = QDir(folder);
    if (!folder_dir.exists()) {
        errors << tr("Folder %1 does not exist/is not accessible").arg(folder);
        return false;
    }

    // ok so far ?
    if (errors.count()) return false;
    return true;
}

bool 
LocalFileStore::close()
{
    // nothing to do for now
    return true;
}

// home dire
QString
LocalFileStore::home()
{
    if (context) return getSetting(GC_NETWORKFILESTORE_FOLDER, "").toString();
    else return "";
}

bool
LocalFileStore::createFolder(QString path)
{
    // create a folder on the store e.g. preferences / for a backup etc
    QDir directory = QFileInfo(path).dir();
    if (directory.exists())
        return directory.mkdir(QFileInfo(path).fileName());

    return false;
}

QList<CloudServiceEntry*> 
LocalFileStore::readdir(QString path, QStringList &errors)
{
    QList<CloudServiceEntry*> returning;

    QDir current_path = QDir(path);
    if (!current_path.exists()) {
        errors << tr("Folder %1 does not exist").arg(path);
        return returning;
    }

    QFileInfoList files = current_path.entryInfoList();
    foreach (QFileInfo info, files) {

        // skip . and ..
        if (info.fileName() == "." || info.fileName() == "..") continue;

        CloudServiceEntry *add = newCloudServiceEntry();

        //QFileInfo has full path, we just want the file name
        add->name = info.fileName();
        add->id = add->name;
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
LocalFileStore::readFile(QByteArray *data, QString remotename, QString)
{

    // is the path set ?
    QString path = getSetting(GC_NETWORKFILESTORE_FOLDER, "").toString();
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
        return false;
    };

    emit readComplete(data, remotename, tr("Completed."));

    return true;
}

bool 
LocalFileStore::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    Q_UNUSED(ride);

    // is the path set ?
    QString path = getSetting(GC_NETWORKFILESTORE_FOLDER, "").toString();
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
        (void)file.write(data);
        file.close();
    } else {
        emit writeComplete("", tr("Write to folder %1 failed").arg(path));  // required for single upload to get to an end
        return false;
    };

    emit writeComplete("", tr("Completed."));

    return true;
}

static bool addLocalFileStore() {
    CloudServiceFactory::instance().addService(new LocalFileStore(NULL));
    return true;
}

static bool add = addLocalFileStore();
