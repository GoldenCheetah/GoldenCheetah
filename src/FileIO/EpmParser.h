/*
 * Copyright (c) 2020 Alejandro Martinez (amtriathlon@gmail.com)
 *               Based on TcxParser.h
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
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301	 USA
 */

#ifndef _EpmParser_h
#define _EpmParser_h
#include "GoldenCheetah.h"

#include "RideFile.h"
#include <QString>
#include <QDateTime>
#include <QXmlDefaultHandler>

class EpmParser : public QXmlDefaultHandler
{
public:
    EpmParser(RideFile* rideFile);

    bool startElement( const QString&, const QString&, const QString&,
                       const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );

    bool characters( const QString& );

private:

    RideFile*   rideFile;

    QString     buffer;

    double      framerate;
    double      lastSecs;
    double      lastDist;
};

#endif // _EpmParser_h
