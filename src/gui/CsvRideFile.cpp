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

#include "CsvRideFile.h"
#include <QRegExp>
#include <QTextStream>
#include <algorithm> // for std::sort
#include <assert.h>
#include "math.h"

#define MILES_TO_KM 1.609344
        
static int csvFileReaderRegistered = 
    CombinedFileReader::instance().registerReader("csv", new CsvFileReader());
 
RideFile *CsvFileReader::openRideFile(QFile &file, QStringList &errors) const 
{
    QRegExp rideTime("^.*/(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_"
                     "(\\d\\d)_(\\d\\d)_(\\d\\d)\\.csv$");
    if (rideTime.indexIn(file.fileName()) == -1) {
        errors << ("file name does not encode ride time: \"" 
                   + file.fileName() + "\"");
        return NULL;
    }
    QDateTime datetime(QDate(rideTime.cap(1).toInt(),
                             rideTime.cap(2).toInt(),
                             rideTime.cap(3).toInt()),
                       QTime(rideTime.cap(4).toInt(),
                             rideTime.cap(5).toInt(),
                             rideTime.cap(6).toInt()));
    QRegExp metricUnits("(km|kph)", Qt::CaseInsensitive);
    QRegExp englishUnits("(miles|mph)", Qt::CaseInsensitive);
    QRegExp sample("^\\s*"
                   "(\\d+|\\d+\\.\\d+)\\s*,\\s*"
                   "(\\d+|\\d+\\.\\d+)\\s*,\\s*"
                   "(\\d+|\\d+\\.\\d+)\\s*,\\s*"
                   "(\\d+|\\d+\\.\\d+)\\s*,\\s*"
                   "(\\d+|\\d+\\.\\d+)\\s*,\\s*"
                   "(\\d+|\\d+\\.\\d+)\\s*,\\s*"
                   "(\\d+|\\d+\\.\\d+)\\s*,\\s*"
                   "(\\d+)\\s*$");
    bool metric;
    if (!file.open(QFile::ReadOnly)) {
        errors << ("Could not open ride file: \"" 
                   + file.fileName() + "\"");
        return NULL;
    }
    int lineno = 1;
    QTextStream is(&file);
    RideFile *rideFile = new RideFile();
    while (!is.atEnd()) {
        QString line = is.readLine();
        if (lineno == 1) {
            if (metricUnits.indexIn(line) != -1)
                metric = true;
            else if (englishUnits.indexIn(line) != -1) 
                metric = false;
            else {
                errors << ("Can't find units in first line: \"" + line + "\"");
                delete rideFile;
                file.close();
                return NULL;
            }
        }
        else if (sample.indexIn(line) != -1) {
            assert(sample.numCaptures() == 8);
            double minutes = sample.cap(1).toDouble();
            double nm      = sample.cap(2).toDouble();
            double kph     = sample.cap(3).toDouble();
            double watts   = sample.cap(4).toDouble();
            double km      = sample.cap(5).toDouble();
            double cad     = sample.cap(6).toDouble();
            double hr      = sample.cap(7).toDouble();
            int interval   = sample.cap(8).toInt();
            if (!metric) {
                km *= MILES_TO_KM;
                kph *= MILES_TO_KM;
            }
            rideFile->appendPoint(minutes * 60.0, cad, hr, km, 
                                 kph, nm, watts, interval);
        }
        else {
            errors << ("Couldn't parse line: \"" + line + "\"");
        }
        ++lineno;
    }
    // To estimate the recording interval, take the median of the
    // first 1000 samples and round to nearest millisecond.
    int n = rideFile->dataPoints().size();
    n = qMin(n, 1000);
    if (n >= 2) {
        double *secs = new double[n-1];
        for (int i = 0; i < n-1; ++i) {
            double now = rideFile->dataPoints()[i]->secs;
            double then = rideFile->dataPoints()[i+1]->secs;
            secs[i] = then - now;
        }
        std::sort(secs, secs + n - 1);
        int mid = n / 2 - 1;
        double recint = round(secs[mid] * 1000.0) / 1000.0;
        rideFile->setRecIntSecs(recint);
    }
    rideFile->setStartTime(datetime);
    file.close();
    return rideFile;
}

