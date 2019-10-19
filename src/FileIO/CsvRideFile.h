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

#ifndef _CsvRideFile_h
#define _CsvRideFile_h
#include "GoldenCheetah.h"

#include "RideFile.h"

struct CsvFileReader : public RideFileReader {
    enum csvtypes { generic, gc, powertap, joule, ergomo, motoactv, ibike, xtrain, moxy, freemotion, peripedal, cpexport, bsx, rowpro, wprime, wahooMA, rp3, opendata, xdata };
    typedef enum csvtypes CsvType;

    virtual RideFile *openRideFile(QFile &file, QStringList &errors, QList<RideFile*>* = 0) const; 

    // standard calling semantics - will write as powertap csv
    bool writeRideFile(Context *context, const RideFile *ride, QFile &file) const
    { return writeRideFile(context, ride, file, powertap); }

    // write but able to select format
    bool writeRideFile(Context *context, const RideFile *ride, QFile &file, CsvType format) const;
    bool hasWrite() const { return true; }
};

#endif // _CsvRideFile_h

