/*
 * Copyright (c)  2007 Sean C. Rhea (srhea@srhea.net),
 *                     Justin F. Knotzke (jknotzke@shampoo.ca)
 * Copyright (c)  2009 Greg Lonnon (greg.lonnon@gmail.com)
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

#include "Computrainer3dpFile.h"
#include <QRegExp>
#include <QTextStream>
#include <QDateTime>
#include <QString>
#include <algorithm>            // for std::sort
#include <assert.h>
#include "math.h"
#include <iostream>
#include <QDebug>
#include "Units.h"
#include <QVector>

typedef unsigned char ubyte;

static int Computrainer3dpFileReaderRegistered =
    RideFileFactory::instance().registerReader("3dp",
                                               "Computrainer 3dp",
                                               new
                                               Computrainer3dpFileReader
                                               ());

RideFile *Computrainer3dpFileReader::openRideFile(QFile & file,
                                                  QStringList & errors)
    const
{
    if (!file.open(QFile::ReadOnly)) {
        errors << ("Could not open ride file: \"" + file.fileName() +
                   "\"");
        return NULL;
    }

    RideFile *rideFile = new RideFile();
    QDataStream is(&file);

    is.setByteOrder(QDataStream::LittleEndian);

    // looks like the first part is a header... ignore it.
    is.skipRawData(4);
    char perfStr[5];

    is.readRawData(perfStr, 4);
    perfStr[4] = NULL;
    is.skipRawData(0x8);
    char userName[65];
    is.readRawData(userName, 65);
    ubyte age;
    is >> age;
    is.skipRawData(6);
    float weight;
    is >> weight;
    int upperHR;
    is >> upperHR;
    int lowerHR;
    is >> lowerHR;
    int year;
    is >> year;
    ubyte month;
    is >> month;
    ubyte day;
    is >> day;
    ubyte hour;
    is >> hour;
    ubyte minute;
    is >> minute;
    int numberSamples;
    is >> numberSamples;
    // go back to the start, and go to the start of the data samples
    file.seek(0);
    is.skipRawData(0xf8);
    for (; numberSamples; numberSamples--) {
        ubyte hr;
        is >> hr;
        ubyte cad;
        is >> cad;
        unsigned short watts;
        is >> watts;
        float speed;
        is >> speed;
        speed = speed * KM_PER_MILE * 100;
        int ms;
        is >> ms;
        is.skipRawData(4);
        float miles;
        is >> miles;
        float km = miles * KM_PER_MILE;
        is.skipRawData(0x1c);
        rideFile->appendPoint((double) ms / 1000, (double) cad,
                              (double) hr, km, speed, 0.0, watts, 0, 0, 0);

    }
    QDateTime dateTime;
    QDate date;
    QTime time;
    date.setDate(year, month, day);
    time.setHMS(hour, minute, 0, 0);
    dateTime.setDate(date);
    dateTime.setTime(time);

    rideFile->setStartTime(dateTime);

    int n = rideFile->dataPoints().size();
    n = qMin(n, 1000);
    if (n >= 2) {
        QVector < double >secs(n - 1);
        for (int i = 0; i < n - 1; ++i) {
            double now = rideFile->dataPoints()[i]->secs;
            double then = rideFile->dataPoints()[i + 1]->secs;
            secs[i] = then - now;
        }
        std::sort(secs.begin(), secs.end());
        int mid = n / 2 - 1;
        double recint = round(secs[mid] * 1000.0) / 1000.0;
        rideFile->setRecIntSecs(recint);
    }
    rideFile->setDeviceType("Computrainer 3DP");
    file.close();
    return rideFile;
}
