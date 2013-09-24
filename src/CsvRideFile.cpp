/*
 * Copyright (c) 2007-2009 Sean C. Rhea (srhea@srhea.net),
 *                         Justin F. Knotzke (jknotzke@shampoo.ca)
 * Copyright (c) 2012      Magnus Gille <mgille@gmail.com>
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
#include "Units.h"
#include <QRegExp>
#include <QTextStream>
#include <QVector>
#include <algorithm> // for std::sort
#include "math.h"

static int csvFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "csv","Comma-Separated Values", new CsvFileReader());

RideFile *CsvFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    QRegExp metricUnits("(km|kph|km/h)", Qt::CaseInsensitive);
    QRegExp englishUnits("(miles|mph|mp/h)", Qt::CaseInsensitive);
    QRegExp degCUnits("Temperature .*C", Qt::CaseInsensitive);
    QRegExp degFUnits("Temperature .*F", Qt::CaseInsensitive);
    bool metric = true;
    enum temperature { degF, degC, degNone };
    typedef enum temperature Temperature;
    static Temperature tempType = degNone;
    QDateTime startTime;

    // TODO: a more robust regex for ergomo files
    // i don't have an example with english headers
    // the ergomo CSV has two rows of headers. units are on the second row.
    /*
    ZEIT,STRECKE,POWER,RPM,SPEED,PULS,HÖHE,TEMP,INTERVAL,PAUSE
    L_SEC,KM,WATT,RPM,KM/H,BPM,METER,°C,NUM,SEC
    */
    QRegExp ergomoCSV("(ZEIT|STRECKE)", Qt::CaseInsensitive);
    bool ergomo = false;

    QRegExp motoActvCSV("activity_id", Qt::CaseInsensitive);
    bool motoActv = false;
    bool epoch_set = false;
    quint64 epoch_offset=0;
    QChar ergomo_separator;
    int unitsHeader = 1;
    int total_pause = 0;
    int currentInterval = 0;
    int prevInterval = 0;

    /* Joule 1.0
    Version,Date/Time,Km,Minutes,RPE,Tags,"Weight, kg","Work, kJ",FTP,"Sample Rate, s",Device Type,Firmware Version,Last Updated,Category 1,Category 2
    6,2012-11-27 13:40:41,0,0,0,,55.8,788,227,1,Joule,18.018,,0,
    User Name,Power Zone 1,Power Zone 2,Power Zone 3,Power Zone 4,Power Zone 5,Power Zone 6,HR Zone 1,HR Zone 2,HR Zone 3,HR Zone 4,HR Zone 5,Calc Power A,Calc Power B,Calc Power 
    ,0,0,0,0,0,0,150,160,170,180,250,0,-0,
    Minutes, Torq (N-m),Km/h,Watts,Km,Cadence,Hrate,ID,Altitude (m),Temperature (�C),"Grade, %",Latitude,Longitude,Power Calc'd,Calc Power,Right Pedal,Pedal Power %,Cad. Smoot
    0,0,0,45,0,0,69,0,62,16.7,0,0,0,0,0,0,0,
    */
    QRegExp jouleCSV("Device Type,Firmware Version", Qt::CaseInsensitive);
    QRegExp jouleMetriCSV(",Km,", Qt::CaseInsensitive);
    bool joule = false;

    // TODO: with all these formats, should the logic change to a switch/case structure?
    // The iBike format CSV file has five lines of headers (data begins on line 6)
    // starting with:
    /*
    iBike,8,english
    2008,8,8,6,32,52

    {Various configuration data, recording interval at line[4][4]}
    Speed (mph),Wind Speed (mph),Power (W),Distance (miles),Cadence (RPM),Heartrate (BPM),Elevation (feet),Hill slope (%),Internal,Internal,Internal,DFPM Power,Latitude,Longitude
    */
        //  Modified the regExp string to allow for 2-digit version numbers - 23 Mar 2009, thm
    QRegExp iBikeCSV("iBike,\\d\\d?,[a-z]+", Qt::CaseInsensitive);
    bool iBike = false;
    int recInterval = 1;

    if (!file.open(QFile::ReadOnly)) {
        errors << ("Could not open ride file: \""
                   + file.fileName() + "\"");
        return NULL;
    }
    int lineno = 1;
    QTextStream is(&file);
    RideFile *rideFile = new RideFile();
    int iBikeInterval = 0;
    bool dfpmExists   = false;
    int iBikeVersion  = 0;
    while (!is.atEnd()) {
        // the readLine() method doesn't handle old Macintosh CR line endings
        // this workaround will load the the entire file if it has CR endings
        // then split and loop through each line
        // otherwise, there will be nothing to split and it will read each line as expected.
        QString linesIn = is.readLine();
        QStringList lines = linesIn.split('\r');
        // workaround for empty lines
        if(lines.isEmpty()) {
            lineno++;
            continue;
        }
        for (int li = 0; li < lines.size(); ++li) {
            QString line = lines[li];

            if (lineno == 1) {
                if (ergomoCSV.indexIn(line) != -1) {
                    ergomo = true;
                    rideFile->setDeviceType("Ergomo");
                    rideFile->setFileFormat("Ergomo CSV (csv)");
                    unitsHeader = 2;

                    QStringList headers = line.split(';');

                    if (headers.size()>1)
                        ergomo_separator = ';';
                    else
                        ergomo_separator = ',';

                    ++lineno;
                    continue;
                }
                else if(iBikeCSV.indexIn(line) != -1) {
                    iBike = true;
                    rideFile->setDeviceType("iBike");
                    rideFile->setFileFormat("iBike CSV (csv)");
                    unitsHeader = 5;
                    iBikeVersion = line.section( ',', 1, 1 ).toInt();
                    ++lineno;
                    continue;
                 }
                 else if(motoActvCSV.indexIn(line) != -1) {
                     motoActv = true;
                     rideFile->setDeviceType("MotoACTV");
                     rideFile->setFileFormat("MotoACTV CSV (csv)");
                     unitsHeader = -1;
                     /* MotoACTV files are always metric */
                     metric = true;
                     ++lineno;
                     continue;
                 }
                 else if(jouleCSV.indexIn(line) != -1) {
                     joule = true;
                     rideFile->setDeviceType("Joule");
                     rideFile->setFileFormat("Joule CSV (csv)");
                     if(jouleMetriCSV.indexIn(line) != -1) {
                         unitsHeader = 5;
                         metric = true;
                     }
                     else { /* ? */ }
                     ++lineno;
                     continue;
                 }
                 // default
                 rideFile->setDeviceType("PowerTap");
                 rideFile->setFileFormat("PowerTap CSV (csv)");
            }
            if (iBike && lineno == 2) {
                QStringList f = line.split(",");
                if (f.size() == 6) {
                    startTime = QDateTime(
                        QDate(f[0].toInt(), f[1].toInt(), f[2].toInt()),
                        QTime(f[3].toInt(), f[4].toInt(), f[5].toInt()));
                }
            }
            if (iBike && lineno == 4) {
                // this is the line with the iBike configuration data
                // recording interval is in the [4] location (zero-based array)
                // the trailing zeroes in the configuration area seem to be causing an error
                // the number is in the format 5.000000
                recInterval = (int)line.section(',',4,4).toDouble();
            }
            if (joule && lineno == 2) {
                // 6,2012-11-27 13:40:41,0,0,0,,55.8,788,227,1,Joule,18.018,,0,
                QStringList f = line.split(",");
                if (f.size() >= 2) {
                    int f0l;
                    QStringList f0 = f[1].split("|");
                    // new format? due to new PowerAgent version (7.5.7.34)?
                    // 6,2011-01-02 21:22:20|2011-01-02 21:22|01/02/2011 21:22|2011-01-02 21-22-20,0,0, ...

                    f0l = f0.size();
                    if (f0l >= 2) {
                       startTime = QDateTime::fromString(f0[0], "yyyy-MM-dd H:mm:ss");
                    } else {
                       startTime = QDateTime::fromString(f[1], "yyyy-MM-dd H:mm:ss");
                    }
                }
            }
            if (lineno == unitsHeader) {
                if (metricUnits.indexIn(line) != -1)
                    metric = true;
                else if (englishUnits.indexIn(line) != -1)
                    metric = false;
                else {
                    errors << "Can't find units in first line: \"" + line + "\" of file \"" + file.fileName() + "\".";
                    delete rideFile;
                    file.close();
                    return NULL;
                }
                if (degCUnits.indexIn(line) != -1)
                    tempType = degC;
                else if (degFUnits.indexIn(line) != -1)
                    tempType = degF;
            }
            else if (lineno > unitsHeader) {
                double minutes=0,nm,kph,watts,km,cad,alt,hr,dfpm, seconds=0.0;
                double temp=RideFile::noTemp;
                double slope=0.0;
                bool ok;
                double lat = 0.0, lon = 0.0;
                double headwind = 0.0;
                int interval=0;
                int pause=0;
                quint64 ms;

                if (!ergomo && !iBike && !motoActv) {
                     minutes = line.section(',', 0, 0).toDouble();
                     nm = line.section(',', 1, 1).toDouble();
                     kph = line.section(',', 2, 2).toDouble();
                     watts = line.section(',', 3, 3).toDouble();
                     km = line.section(',', 4, 4).toDouble();
                     cad = line.section(',', 5, 5).toDouble();
                     hr = line.section(',', 6, 6).toDouble();
                     interval = line.section(',', 7, 7).toInt();
                     alt = line.section(',', 8, 8).toDouble();
                    if (joule && tempType != degNone) {
                        // is the position always the same?
                        // should we read the header and assign positions
                        // to each item instead?
                        temp = line.section(',', 9, 9).toDouble();
                        if (tempType == degF) {
                           // convert to deg C
                           temp *= FAHRENHEIT_PER_CENTIGRADE + FAHRENHEIT_ADD_CENTIGRADE;
                        }
                    }
                    if (!metric) {
                        km *= KM_PER_MILE;
                        kph *= KM_PER_MILE;
                        alt *= METERS_PER_FOOT;
                    }
                }
                else if (iBike) {
                    // this must be iBike
                    // can't find time as a column.
                    // will we have to extrapolate based on the recording interval?
                    // reading recording interval from config data in ibike csv file
                    //
                    // For iBike software version 11 or higher:
                    // use "power" field until a the "dfpm" field becomes non-zero.
                     minutes = (recInterval * lineno - unitsHeader)/60.0;
                     nm = 0; //no torque
                     kph = line.section(',', 0, 0).toDouble();
                     dfpm = line.section( ',', 11, 11).toDouble();
                     if( iBikeVersion >= 11 && ( dfpm > 0.0 || dfpmExists ) ) {
                         dfpmExists = true;
                         watts = dfpm;
                         headwind = line.section(',', 1, 1).toDouble();
                     }
                     else {
                         watts = line.section(',', 2, 2).toDouble();
                     }
                     km = line.section(',', 3, 3).toDouble();
                     cad = line.section(',', 4, 4).toDouble();
                     hr = line.section(',', 5, 5).toDouble();
                     alt = line.section(',', 6, 6).toDouble();
                     lat = line.section(',', 12, 12).toDouble();
                     lon = line.section(',', 13, 13).toDouble();
                     temp = line.section(',', 8, 8).toDouble();
                     slope = line.section(',', 7, 7).toDouble();
                     int lap = line.section(',', 9, 9).toInt();
                     if (lap > 0) {
                         iBikeInterval += 1;
                         interval = iBikeInterval;
                     }
                    if (!metric) {
                        km *= KM_PER_MILE;
                        kph *= KM_PER_MILE;
                        alt *= METERS_PER_FOOT;
                        headwind *= KM_PER_MILE;
                    }
                }
               else if(motoActv) {
                    /* MotoActv saves it all as kind of SI (m, ms, m/s, NM etc)
                     *  "double","double",.. so we need to filter out "
                     */

                    km = line.section(',', 0,0).remove("\"").toDouble()/1000;
                    hr = line.section(',', 2, 2).remove("\"").toDouble();
                    kph = line.section(',', 3, 3).remove("\"").toDouble()*3.6;

                    lat = line.section(',', 5, 5).remove("\"").toDouble();
                    /* Item 8 is crank torque, 13 is wheel torque */
                    nm = line.section(',', 8, 8).remove("\"").toDouble();

                    /* Ok there's no crank torque, try the wheel */
                    if(nm == 0.0) {
                         nm = line.section(',', 13, 13).remove("\"").toDouble();
                    }
                    if(epoch_set == false) {
                         epoch_set = true;
                         epoch_offset = line.section(',', 9,9).remove("\"").toULongLong(&ok, 10);

                         /* We use this first value as the start time */
                         startTime = QDateTime();
                         startTime.setMSecsSinceEpoch(epoch_offset);
                         rideFile->setStartTime(startTime);
                    }

                    ms = line.section(',', 9,9).remove("\"").toULongLong(&ok, 10);
                    ms -= epoch_offset;
                    seconds = ms/1000;

                    alt = line.section(',', 10, 10).remove("\"").toDouble();
                    watts = line.section(',', 11, 11).remove("\"").toDouble();
                    lon = line.section(',', 15, 15).remove("\"").toDouble();
                    cad = line.section(',', 16, 16).remove("\"").toDouble();
               }
                else {
                     // for ergomo formatted CSV files
                     minutes     = line.section(ergomo_separator, 0, 0).toDouble() + total_pause;
                     QString km_string = line.section(ergomo_separator, 1, 1);
                     km_string.replace(",",".");
                     km = km_string.toDouble();
                     watts = line.section(ergomo_separator, 2, 2).toDouble();
                     cad = line.section(ergomo_separator, 3, 3).toDouble();
                     QString kph_string = line.section(ergomo_separator, 4, 4);
                     kph_string.replace(",",".");
                     kph = kph_string.toDouble();
                     hr = line.section(ergomo_separator, 5, 5).toDouble();
                     alt = line.section(ergomo_separator, 6, 6).toDouble();
                     interval = line.section(',', 8, 8).toInt();
                     if (interval != prevInterval) {
                         prevInterval = interval;
                         if (interval != 0) currentInterval++;
                     }
                     if (interval != 0) interval = currentInterval;
                     pause = line.section(ergomo_separator, 9, 9).toInt();
                     total_pause += pause;
                     nm = 0; // torque is not provided in the Ergomo file

                     // the ergomo records the time in whole seconds
                     // RECORDING INT. 1, 2, 5, 10, 15 or 30 per sec
                     // Time is *always* perfectly sequential.  To find pauses,
                     // you need to read the PAUSE column.
                     minutes = minutes/60.0;

                     if (!metric) {
                         km *= KM_PER_MILE;
                         kph *= KM_PER_MILE;
                         alt *= METERS_PER_FOOT;
                     }
                }

                // PT reports no data as watts == -1.
                if (watts == -1)
                    watts = 0;

               if(motoActv)
                    rideFile->appendPoint(seconds, cad, hr, km,
                                          kph, nm, watts, alt, lon, lat, 0.0,
                                          0.0, temp, 0.0, interval);
               else
                    rideFile->appendPoint(minutes * 60.0, cad, hr, km,
                                          kph, nm, watts, alt, lon, lat,
                                          headwind, slope, temp, 0.0,
                                          interval);
            }
            ++lineno;
        }
    }
    file.close();

    // To estimate the recording interval, take the median of the
    // first 1000 samples and round to nearest millisecond.
    int n = rideFile->dataPoints().size();
    n = qMin(n, 1000);
    if (n >= 2) {
        QVector<double> secs(n-1);
        for (int i = 0; i < n-1; ++i) {
            double now = rideFile->dataPoints()[i]->secs;
            double then = rideFile->dataPoints()[i+1]->secs;
            secs[i] = then - now;
        }
        std::sort(secs.begin(), secs.end());
        int mid = n / 2 - 1;
        double recint = round(secs[mid] * 1000.0) / 1000.0;
        rideFile->setRecIntSecs(recint);
    }
    // less than 2 data points is not a valid ride file
    else {
        errors << "Insufficient valid data in file \"" + file.fileName() + "\".";
        delete rideFile;
        file.close();
        return NULL;
    }

    QRegExp rideTime("^.*/(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_"
                     "(\\d\\d)_(\\d\\d)_(\\d\\d)\\.csv$");
    rideTime.setCaseSensitivity(Qt::CaseInsensitive);

    if (startTime != QDateTime()) {

        // Start time was already set above?
        rideFile->setStartTime(startTime);

    } else if (rideTime.indexIn(file.fileName()) >= 0) {

        // It matches the GC naming convention?
        QDateTime datetime(QDate(rideTime.cap(1).toInt(),
                                 rideTime.cap(2).toInt(),
                                 rideTime.cap(3).toInt()),
                           QTime(rideTime.cap(4).toInt(),
                                 rideTime.cap(5).toInt(),
                                 rideTime.cap(6).toInt()));
        rideFile->setStartTime(datetime);

    } else {

        // Could be yyyyddmm_hhmmss_NAME.csv (case insensitive)
        rideTime.setPattern("(\\d\\d\\d\\d)(\\d\\d)(\\d\\d)_(\\d\\d)(\\d\\d)(\\d\\d)[^\\.]*\\.csv$");
        if (rideTime.indexIn(file.fileName()) >= 0) {
            QDateTime datetime(QDate(rideTime.cap(1).toInt(),
                                     rideTime.cap(2).toInt(),
                                     rideTime.cap(3).toInt()),
                               QTime(rideTime.cap(4).toInt(),
                                     rideTime.cap(5).toInt(),
                                     rideTime.cap(6).toInt()));
            rideFile->setStartTime(datetime);
        } else {

            // is it in poweragent format "name yyyy-mm-dd hh-mm-ss.csv"
            rideTime.setPattern("(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d) (\\d\\d)-(\\d\\d)-(\\d\\d)\\.csv$");
            if (rideTime.indexIn(file.fileName()) >=0) {

                QDateTime datetime(QDate(rideTime.cap(1).toInt(),
                                        rideTime.cap(2).toInt(),
                                        rideTime.cap(3).toInt()),
                                QTime(rideTime.cap(4).toInt(),
                                        rideTime.cap(5).toInt(),
                                        rideTime.cap(6).toInt()));
                rideFile->setStartTime(datetime);

            } else {

                // NO DICE

                // Note: qWarning("Failed to set start time");
                // console messages are no use, so commented out
                // this problem will ONLY occur during the import
                // process which traps these and corrects them
                // so no need to do anything here

            }
        }
    }

    // did we actually read any samples?
    if (rideFile->dataPoints().count() > 0) {
        return rideFile;
    } else {
        errors << "No samples present.";
        delete rideFile;
        return NULL;
    }
}

bool
CsvFileReader::writeRideFile(MainWindow *, const RideFile *ride, QFile &file) const
{
    if (!file.open(QIODevice::WriteOnly)) return(false);

    // always save CSV in metric format
    bool bIsMetric = true;

    // Use the column headers that make WKO+ happy.
    double convertUnit;
    QTextStream out(&file);
    if (!bIsMetric)
    {
        out << "Minutes,Torq (N-m),MPH,Watts,Miles,Cadence,Hrate,ID,Altitude (feet)\n";
        convertUnit = MILES_PER_KM;
    }
    else {
        out << "Minutes,Torq (N-m),Km/h,Watts,Km,Cadence,Hrate,ID,Altitude (m)\n";
        convertUnit = 1.0;
    }

    foreach (const RideFilePoint *point, ride->dataPoints()) {
        if (point->secs == 0.0)
            continue;
        out << point->secs/60.0;
        out << ",";
        out << ((point->nm >= 0) ? point->nm : 0.0);
        out << ",";
        out << ((point->kph >= 0) ? (point->kph * convertUnit) : 0.0);
        out << ",";
        out << ((point->watts >= 0) ? point->watts : 0.0);
        out << ",";
        out << point->km * convertUnit;
        out << ",";
        out << point->cad;
        out << ",";
        out << point->hr;
        out << ",";
        out << point->interval;
        out << ",";
        out << point->alt;
        out << "\n";
    }

    file.close();
    return true;
}
