/*
 * Copyright (c) 2016 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _zwoparser_h
#define _zwoparser_h

#include <QXmlStreamReader>
#include <QFile>

#include "ErgFileBase.h"

class ZwoParser
{
public:
    bool parseFile(QFile *file);

    QString buffer;
    int secs;  // rolling secs as points read
    int sSecs; // rolling starting secs as points read
    int watts; // watts set at last point

    // the data in it
    QString name,               // <name>
            description,        // <description>
            author;             // <author>
    QStringList tags;           // <tags><tag>

    // category
    int categoryIndex;          // <categoryIndex>
    QString category;           // <category>
    QString subcategory;        // <subcategory>

    // all the points - as a percentage of FTP, not scaled
    //                  so range from 0.00 - 1.00
    QList<ErgFilePoint> points;

    // texts
    QList<ErgFileText> texts;

private:
    bool parseWorkoutFile(QXmlStreamReader &reader);
    bool parseTags(QXmlStreamReader &reader);
    bool parseWorkout(QXmlStreamReader &reader);

    void resetData();
};

#endif //ZwoParser
