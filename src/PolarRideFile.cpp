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

#include "PolarRideFile.h"
#include "Units.h"
#include <QRegExp>
#include <QTextStream>
#include <algorithm> // for std::sort
#include "math.h"


static int polarFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "hrm", "Polar Precision", new PolarFileReader());

RideFile *PolarFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
/*
* Polar HRM file format documented at www.polar.fi/files/Polar_HRM_file%20format.pdf
*/
    QRegExp metricUnits("(km|kph|km/h)", Qt::CaseInsensitive);
    QRegExp englishUnits("(miles|mph|mp/h)", Qt::CaseInsensitive);
    bool metric = true;

    QDate date;
    QString note("");

    double version=0;
    int monitor=0;

    double seconds=0;
    double distance=0;
    int interval = 0;

    bool speed = false;
    bool cadence = false;
    bool altitude = false;
    bool power = false;
    bool balance = false;


    int recInterval = 1;

    if (!file.open(QFile::ReadOnly)) {
        errors << ("Could not open ride file: \""
                   + file.fileName() + "\"");
        return NULL;
    }

    int lineno = 1;


    double next_interval=0;
    QList<double> intervals;

    QTextStream is(&file);
    RideFile *rideFile = new RideFile();
    QString section = NULL;

    while (!is.atEnd()) {
        // the readLine() method doesn't handle old Macintosh CR line endings
        // this workaround will load the the entire file if it has CR endings
        // then split and loop through each line
        // otherwise, there will be nothing to split and it will read each line as expected.
        QString linesIn = is.readLine();
        QStringList lines = linesIn.split('\r');
        // workaround for empty lines
        if(lines.size() == 0) {
            lineno++;
            continue;
        }
        for (int li = 0; li < lines.size(); ++li) {
            QString line = lines[li];

            if (line == "") {

            }
            else if (line.startsWith("[")) {
                //fprintf(stderr, "section : %s\n", line.toAscii().constData());
                section=line;
                if (section == "[HRData]") {
                    // Some systems, like the Tacx HRM exporter, do not add an [IntTimes] section, so we need to
                    // specify that the whole ride is one big interval.
                    if (intervals.isEmpty())
                        intervals.append(seconds);
                   next_interval = intervals.at(0);
               }
            }
            else if (section == "[Params]"){
                if (line.contains("Version=")) {
                    QString versionString = QString(line);
                    versionString.remove(0,8).insert(1, ".");
                    version = versionString.toFloat();
                    rideFile->setFileFormat("Polar HRM v"+versionString+" (hrm)");
                } else if (line.contains("Monitor=")) {
                    QString monitorString = QString(line);
                    monitorString.remove(0,8);
                    monitor = monitorString.toInt();
                    switch (monitor) {
                        case 1: rideFile->setDeviceType("Polar Sport Tester / Vantage XL"); break;
                        case 2: rideFile->setDeviceType("Polar Vantage NV (VNV)"); break;
                        case 3: rideFile->setDeviceType("Polar Accurex Plus"); break;
                        case 4: rideFile->setDeviceType("Polar XTrainer Plus"); break;
                        case 6: rideFile->setDeviceType("Polar S520"); break;
                        case 7: rideFile->setDeviceType("Polar Coach"); break;
                        case 8: rideFile->setDeviceType("Polar S210"); break;
                        case 9: rideFile->setDeviceType("Polar S410"); break;
                        case 10: rideFile->setDeviceType("Polar S510"); break;
                        case 11: rideFile->setDeviceType("Polar S610 / S610i"); break;
                        case 12: rideFile->setDeviceType("Polar S710 / S710i"); break;
                        case 13: rideFile->setDeviceType("Polar S810 / S810i"); break;
                        case 15: rideFile->setDeviceType("Polar E600"); break;
                        case 20: rideFile->setDeviceType("Polar AXN500"); break;
                        case 21: rideFile->setDeviceType("Polar AXN700"); break;
                        case 22: rideFile->setDeviceType("Polar S625X / S725X"); break;
                        case 23: rideFile->setDeviceType("Polar S725"); break;
                        case 33: rideFile->setDeviceType("Polar CS400"); break;
                        case 34: rideFile->setDeviceType("Polar CS600X"); break;
                        case 35: rideFile->setDeviceType("Polar CS600"); break;
                        case 36: rideFile->setDeviceType("Polar RS400"); break;
                        case 37: rideFile->setDeviceType("Polar RS800"); break;
                        case 38: rideFile->setDeviceType("Polar RS800X"); break;

                        default: rideFile->setDeviceType(QString("Unknown Polar Device %1").arg(monitor));
                   }
                } else if (line.contains("SMode=")) {
                    line.remove(0,6);
                    QString smode = QString(line);
                    if (smode.at(0)=='1')
                        speed = true;
                    if (smode.length()>0 && smode.at(1)=='1')
                        cadence = true;
                    if (smode.length()>1 && smode.at(2)=='1')
                        altitude = true;
                    if (smode.length()>2 && smode.at(3)=='1')
                        power = true;
                    if (smode.length()>3 && smode.at(4)=='1')
                        balance = true;
                    //if (smode.length()>4 && smode.at(5)=='1') pedaling_index = true;

/*
It appears that the Polar CS600 exports its data alays in metric when downloaded from the
polar software even when English units are displayed on the unit..  It also never sets
this bit low in the .hrm file.  This will have to get changed if other software downloads
this differently
*/

                    if (smode.length()>6 && smode.at(7)=='1')
                        metric = false;

                } else if (line.contains("Interval=")) {
                    recInterval = line.remove(0,9).toInt();
                    rideFile->setRecIntSecs(recInterval);
                } else if (line.contains("Date=")) {
                    line.remove(0,5);
                    date= QDate(line.left(4).toInt(),
                                line.mid(4,2).toInt(),
                                line.mid(6,2).toInt());
                } else if (line.contains("StartTime=")) {
                    line.remove(0,10);
                    QDateTime datetime(date,
                                      QTime(line.left(2).toInt(),
                                            line.mid(3,2).toInt(),
                                            line.mid(6,2).toInt()));
                    rideFile->setStartTime(datetime);
                 }


            }
            else if (section == "[Note]"){
                note.append(line);
            }
            else if (section == "[IntTimes]"){
                double int_seconds = line.left(2).toInt()*60*60+line.mid(3,2).toInt()*60+line.mid(6,3).toFloat();
                intervals.append(int_seconds);

                if (lines.size()==1) {
                   is.readLine();
                   is.readLine();
                   if (version>1.05) {
                       is.readLine();
                       is.readLine();
                   }
                } else {
                   li+=2;
                   if (version>1.05)
                      li+=2;
                }
            }
            else if (section == "[HRData]"){
                double nm=0,kph=0,watts=0,km=0,cad=0,hr=0,alt=0;
                double lrbalance=0;

                seconds += recInterval;

                int i=0;
                hr = line.section('\t', i, i).toDouble();
                i++;

                if (speed) {
                    kph = line.section('\t', i, i).toDouble()/10;
                    i++;
                }
                if (cadence) {
                    cad = line.section('\t', i, i).toDouble();
                    i++;
                }
                if (altitude) {
                    alt = line.section('\t', i, i).toDouble();
                    i++;
                }
                if (power) {
                    watts = line.section('\t', i, i).toDouble();
                    i++;
                }
                if (balance) {
                    // Power LRB + PI:  The value contains :
                    //  - Left Right Balance (LRB) and
                    //  - Pedaling Index (PI)
                    //
                    // in the following formula:
                    // value = PI * 256 + LRB   PI bits 15-8  LRB bits 7-0
                    // LRB is the value of left foot
                    // for example if LRB = 45, actual balance is L45 - 55R.
                    // PI values are percentages from 0 to 100.
                    // For example value 12857 (= 40 * 256 + 47)
                    // means: PI = 40 and LRB = 47 => L47 - 53R

                    lrbalance = line.section('\t', i, i).toInt() & 0xff;
                    i++;
                }

                distance = distance + kph/60/60*recInterval;
                km = distance;

                if (next_interval < seconds) {
                    interval = intervals.indexOf(next_interval);
                    if (intervals.count()>interval+1){
                        interval++;
                        next_interval = intervals.at(interval);
                    }
                }

                if (!metric) {
                    km *= KM_PER_MILE;
                    kph *= KM_PER_MILE;
                    alt *= METERS_PER_FOOT;
                }

                rideFile->appendPoint(seconds, cad, hr, km, kph, nm, watts, alt, 0.0, 0.0, 0.0, 0.0, RideFile::NoTemp, lrbalance, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, interval);
	            //fprintf(stderr, " %f, %f, %f, %f, %f, %f, %f, %d\n", seconds, cad, hr, km, kph, nm, watts, alt, interval);
            }

        ++lineno;
        }
    }

    QRegExp rideTime("^.*/(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_"
                     "(\\d\\d)_(\\d\\d)_(\\d\\d)\\.hrm$");
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

