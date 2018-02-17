/*
 * Copyright (c) 2018 Mark Liversedge <liversedge@gmail.com>
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

#include "ArchiveFile.h"
#include "../qzip/zipreader.h"

QStringList Archive::extract(QString name, QList<QString> want, QString folder)
{
    QStringList returning;

    foreach(QString filename, want)
        returning  << folder + "/" + filename;

    ZipReader czip(name);

    // slight bug, we extract all, not selective
    czip.extractAll(folder);

    return returning;
}

QList<QString>
Archive::dir(QString name)
{
    QList<QString> returning;

    // Establish what type it is and give up if not known
    enum { ZIP, GZIP, UNKNOWN } type;
    QString suffix = QFileInfo(name).suffix();
    if (!suffix.compare("zip", Qt::CaseInsensitive)) type = ZIP;
    else if (!suffix.compare("gz", Qt::CaseInsensitive)) type = GZIP;
    else if (!suffix.compare("gzip", Qt::CaseInsensitive)) type = GZIP;
    else  return returning;

    switch (type) {

        case ZIP:
        {
            ZipReader czip(name);

            foreach(ZipReader::FileInfo info, czip.fileInfoList()) {
                if (!info.isDir)
                    returning += info.filePath;
            }
        }
        break;
        case GZIP:
        {
        }
        break;
        default:
        break;
    }
    return returning;
}
