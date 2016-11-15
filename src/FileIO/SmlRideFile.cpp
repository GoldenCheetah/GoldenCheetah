/*
 * Copyright (c) 2015 Alejandro Martinez (amtriathlon@gmail.com)
 *               Based on TcxRideFile.cpp
 *
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net),
 *                    J.T Conklin (jtc@acorntoolworks.com)
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

#include "SmlRideFile.h"
#include "SmlParser.h"

static int smlFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "sml", "Suunto Markup Language format", new SmlFileReader());

RideFile *SmlFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    (void) errors;
    RideFile *rideFile = new RideFile();
    rideFile->setRecIntSecs(1.0);
    rideFile->setDeviceType("Suunto");
    rideFile->setFileFormat("Suunto Markup Language Format (sml)");

    SmlParser handler(rideFile);

    QXmlInputSource source (&file);
    QXmlSimpleReader reader;
    reader.setContentHandler (&handler);
    reader.parse (source);

    return rideFile;
}
