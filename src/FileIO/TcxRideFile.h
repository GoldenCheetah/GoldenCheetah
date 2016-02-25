/*
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

#ifndef _TcxRideFile_h
#define _TcxRideFile_h
#include "GoldenCheetah.h"

#include "RideFile.h"

class TcxFileReader : public RideFileReader {
    Q_DECLARE_TR_FUNCTIONS(TcxFileReader)
    public:

    virtual RideFile *openRideFile(QFile &file, QStringList &errors, QList<RideFile*>* = 0) const; 
    QByteArray toByteArray(Context *context, const RideFile *ride, bool withAlt, bool withWatts, bool withHr, bool withCad) const;
    bool writeRideFile(Context *context, const RideFile *ride, QFile &file) const;
    bool hasWrite() const { return true; }
};

#endif // _TcxRideFile_h

