/* 
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net), 
 *                    Justin F. Knotzke (jknotzke@shampoo.ca)
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
    QRegExp metricUnits("(km|kph|km/h)", Qt::CaseInsensitive);
    QRegExp englishUnits("(miles|mph|mp/h)", Qt::CaseInsensitive);
    bool metric;
    
    // TODO: a more robust regex for ergomo files
    // i don't have an example with english headers
    // the ergomo CSV has two rows of headers. units are on the second row.
    /*
    ZEIT,STRECKE,POWER,RPM,SPEED,PULS,HÖHE,TEMP,INTERVAL,PAUSE
    L_SEC,KM,WATT,RPM,KM/H,BPM,METER,°C,NUM,SEC
    */
    QRegExp ergomoCSV("(ZEIT|STRECKE)", Qt::CaseInsensitive);
    bool ergomo;
    int unitsHeader = 1;
    
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
            if (ergomoCSV.indexIn(line) != -1) {
                ergomo = true;
                unitsHeader = 2;
                ++lineno;
                continue;
            }
            else {
                ergomo = false;
            }
        }
        if (lineno == unitsHeader) {
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
        else {
            double minutes,nm,kph,watts,km,cad,hr;
            int interval;
            if (!ergomo) {
                 minutes = line.section(',', 0, 0).toDouble();
                 nm      = line.section(',', 1, 1).toDouble();
                 kph     = line.section(',', 2, 2).toDouble();
                 watts   = line.section(',', 3, 3).toDouble();
                 km      = line.section(',', 4, 4).toDouble();
                 cad     = line.section(',', 5, 5).toDouble();
                 hr      = line.section(',', 6, 6).toDouble();
                 interval   = line.section(',', 7, 7).toInt();
                if (!metric) {
                    km *= MILES_TO_KM;
                    kph *= MILES_TO_KM;
                }
            } 
            else {
                // for ergomo formatted CSV files
                 minutes = line.section(',', 0, 0).toDouble();
                 km      = line.section(',', 1, 1).toDouble();
                 watts   = line.section(',', 2, 2).toDouble();
                 cad     = line.section(',', 3, 3).toDouble();
                 kph     = line.section(',', 4, 4).toDouble();
                 hr      = line.section(',', 5, 5).toDouble();
                 interval   = line.section(',', 8, 8).toInt();
                 nm      = NULL; // torque is not provided in the Ergomo file
                
                // the ergomo records the time in whole seconds
                // RECORDING INT. 1, 2, 5, 10, 15 or 30 per sec
                minutes = minutes/60.0;
                
                if (!metric) {
                    km *= MILES_TO_KM;
                    kph *= MILES_TO_KM;
                }
            }
            rideFile->appendPoint(minutes * 60.0, cad, hr, km, 
                                  kph, nm, watts, interval);
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
    QRegExp rideTime("^.*/(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_"
                     "(\\d\\d)_(\\d\\d)_(\\d\\d)\\.csv$");
    if (rideTime.indexIn(file.fileName()) >= 0) {
        QDateTime datetime(QDate(rideTime.cap(1).toInt(),
                                 rideTime.cap(2).toInt(),
                                 rideTime.cap(3).toInt()),
                           QTime(rideTime.cap(4).toInt(),
                                 rideTime.cap(5).toInt(),
                                 rideTime.cap(6).toInt()));
        rideFile->setStartTime(datetime);
    }
    file.close();
    return rideFile;
}

