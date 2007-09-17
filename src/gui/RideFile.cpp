/* 
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net)
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

#include "RideFile.h"
#include <assert.h>

CombinedFileReader *CombinedFileReader::instance_;

CombinedFileReader &CombinedFileReader::instance() 
{ 
    if (!instance_) 
        instance_ = new CombinedFileReader();
    return *instance_;
}

int CombinedFileReader::registerReader(const QString &suffix, 
                                       RideFileReader *reader) 
{
    assert(!readFuncs_.contains(suffix));
    readFuncs_.insert(suffix, reader);
    return 1;
}

RideFile *CombinedFileReader::openRideFile(QFile &file, 
                                           QStringList &errors) const 
{
    QString suffix = file.fileName();
    int dot = suffix.lastIndexOf(".");
    assert(dot >= 0);
    suffix.remove(0, dot + 1);
    RideFileReader *reader = readFuncs_.value(suffix);
    assert(reader);
    return reader->openRideFile(file, errors);
}

QStringList CombinedFileReader::listRideFiles(const QDir &dir) const 
{
    QStringList filters;
    QMapIterator<QString,RideFileReader*> i(readFuncs_);
    while (i.hasNext()) {
        i.next();
        filters << ("*." + i.key());
    }
    return dir.entryList(filters, QDir::Files, QDir::Name);
}

