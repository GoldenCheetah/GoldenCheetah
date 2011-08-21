/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _JsonRideFile_h
#define _JsonRideFile_h

#include "RideFile.h"
#include <stdio.h>
#include <algorithm> // for std::sort
#include <QDomDocument>
#include <QVector>
#include <assert.h>
#include <QDebug>
#define DATETIME_FORMAT "yyyy/MM/dd hh:mm:ss' UTC'"


struct JsonFileReader : public RideFileReader {
    virtual RideFile *openRideFile(QFile &file, QStringList &errors) const; 
    virtual void writeRideFile(const RideFile *ride, QFile &file) const;
};

#endif // _JsonRideFile_h

