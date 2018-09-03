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
#include "Context.h"
#include "Athlete.h"
#include "Zones.h"
#include "WPrime.h"
#include "Settings.h"

#include <QRegExp>
#include <QTextStream>
#include <QVector>
#include <algorithm> // for std::sort
#include "cmath"

static int csvFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "csv","Poweragent / PowerTap CSV", new CsvFileReader());

int static periMonth(QString month)
{
    if (month == "jan") return 1;
    if (month == "feb") return 2;
    if (month == "mar") return 3;
    if (month == "apr") return 4;
    if (month == "may") return 5;
    if (month == "jun") return 6;
    if (month == "jul") return 7;
    if (month == "aug") return 8;
    if (month == "sep") return 9;
    if (month == "oct") return 10;
    if (month == "nov") return 11;
    if (month == "dec") return 12;
    return QDate::currentDate().month(); // no idea
}

static QDate periDate(QString date)
{
    QStringList parts = date.split("-");
    if (parts.count() == 2) {
        return QDate(QDate::currentDate().year(), periMonth(parts[1]), parts[0].toInt());
    }
    return QDate::currentDate();
}
static QDate moxyDate(QString date)
{
    QStringList parts = date.split("-");
    if (parts.count() == 2) {
        return QDate(QDate::currentDate().year(), parts[0].toInt(), parts[1].toInt());
    }
    return QDate::currentDate();
}

static int moxySeconds(QString time)
{
    int seconds = 0;

    // moxy csv has time as n:n:n where it is NOT zero padded
    // and this causes all sorts of dumb problems
    // so we extract it manually
    QStringList parts = time.split(":");

    if (parts.count() == 3) {
        seconds += parts[0].toInt() * 3600;
        seconds += parts[1].toInt() * 60;
        seconds += parts[2].toInt();
    }

    return seconds;
}

RideFile *CsvFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    CsvType csvType = generic;

    QRegExp metricUnits("(km|kph|km/h)", Qt::CaseInsensitive);
    QRegExp englishUnits("(miles|mph|mp/h)", Qt::CaseInsensitive);
    QRegExp degCUnits("Temperature .*C", Qt::CaseInsensitive);
    QRegExp degFUnits("Temperature .*F", Qt::CaseInsensitive);
    bool metric = true;
    enum temperature { degF, degC, degNone };
    typedef enum temperature Temperature;
    static Temperature tempType = degNone;
    QDateTime startTime;

    // Minutes,Torq (N-m),Km/h,Watts,Km,Cadence,Hrate,ID
    // Minutes, Torq (N-m),Km/h,Watts,Km,Cadence,Hrate,ID
    // Minutes,Torq (N-m),Km/h,Watts,Km,Cadence,Hrate,ID,Altitude (m)
    QRegExp powertapCSV("Minutes,[ ]?Torq \\(N-m\\),(Km/h|MPH),Watts,(Km|Miles),Cadence,Hrate,ID", Qt::CaseInsensitive);

    // TODO: a more robust regex for ergomo files
    // i don't have an example with english headers
    // the ergomo CSV has two rows of headers. units are on the second row.
    /*
    ZEIT,STRECKE,POWER,RPM,SPEED,PULS,HÖHE,TEMP,INTERVAL,PAUSE
    L_SEC,KM,WATT,RPM,KM/H,BPM,METER,°C,NUM,SEC
    */
    QRegExp ergomoCSV("(ZEIT|STRECKE)", Qt::CaseInsensitive);

    QRegExp motoActvCSV("activity_id", Qt::CaseInsensitive);
    bool epoch_set = false;
    quint64 epoch_offset=0;
    QChar ergomo_separator;
    int unitsHeader = 1;
    int total_pause = 0;
    int currentInterval = 0;
    int prevInterval = 0;
    double lastKM=0; // when deriving distance from speed
    XDataSeries *rowSeries=NULL;
    XDataSeries *trainSeries=NULL;
    XDataSeries *rrSeries=NULL;
    XDataSeries *ibikeSeries=NULL;

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

    // TODO: with all these formats, should the logic change to a switch/case structure?
    // The iBike format CSV file has five lines of headers (data begins on line 6)
    // starting with:
    /*
    iBike,8,english
    2008,8,8,6,32,52

    {Various configuration data, recording interval at line[4][4]}
    Speed (mph),Wind Speed (mph),Power (W),Distance (miles),Cadence (RPM),Heartrate (BPM),Elevation (feet),Hill slope (%),Internal,Internal,Internal,DFPM Power,Latitude,Longitude
    */

    QRegExp iBikeCSV("iBike,\\d\\d?,[a-z]+", Qt::CaseInsensitive);
    QRegExp moxyCSV("FW Part Number:", Qt::CaseInsensitive);
    QRegExp smo2CSV("Type,Local Number,Message");
    QRegExp gcCSV("secs,cad,hr,km,kph,nm,watts,alt,lon,lat,headwind,slope,temp,interval,lrbalance,lte,rte,lps,rps,smo2,thb,o2hb,hhb");
    QRegExp gcCSVold("secs, cad, hr, km, kph, nm, watts, alt, lon, lat, headwind, slope, temp, interval, lrbalance, lte, rte, lps, rps, smo2, thb, o2hb, hhb");
    QRegExp periCSV("mm-dd,hh:mm:ss,SmO2 Live,SmO2 Averaged,THb,Target Power,Heart Rate,Speed,Power,Cadence");
    QRegExp freemotionCSV("Stages Data", Qt::CaseInsensitive);
    QRegExp cpexportCSV("seconds, value,[ model,]* date", Qt::CaseInsensitive);
    QRegExp rowproCSV("Date,Comment,Password,ID,Version,RowfileId,Rowfile_Id", Qt::CaseInsensitive);
    QRegExp wahooMACSV("GroundContactTime,MotionCount,MotionPowerZ,Cadence,MotionPowerX,WorkoutActive,Timestamp,Smoothness,MotionPowerY,_ID,VerticalOscillation,", Qt::CaseInsensitive);
    QRegExp rp3CSV ("\"id\",\"workout_interval_id\",\"ref\",\"stroke_number\",\"power\",\"avg_power\",\"stroke_rate\",\"time\",\"stroke_length\",\"distance\",\"distance_per_stroke\",\"estimated_500m_time\",\"energy_per_stroke\",\"energy_sum\",\"pulse\",\"work_per_pulse\",\"peak_force\",\"peak_force_pos\",\"rel_peak_force_pos\",\"drive_time\",\"recover_time\",\"k\",\"curve_data\",\"stroke_number_in_interval\",\"avg_calculated_power\"", Qt::CaseSensitive);


    // X-trainer format
    //starting with:
    //ver,4,113,848
    //col,time,pulse,rpm,watt,climb%,km/t
    //1,97,88,186,0,29
    //2,99,91,192,0,29
    //3,102,92,196,0,30
    //4,103,92,195,0,30
    // note the first column after the header is the time

    QRegExp xtrainCSV("ver,[0-9]+,[0-9]+,[0-9]+");

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

    int xTrainVersion  = 0;

    //UNUSED int timestampIndex=-1;
    int secsIndex=-1;
    //UNUSED int minutesIndex = -1;
    int wattsIndex=-1;
    int cadenceIndex=-1;
    int smo2Index=-1;
    int hrIndex=-1;
    int kphIndex = -1;
    int gctIndex = -1, voIndex =-1;

    double precAvg=0.0;
    //double precWatts=0.0;
    double precSecs=0.0;
    double maxWatts=0.0;
    double lastsecs=0.0;
    double lastkm=0.0;

    // RP3 accrual
    int lastinterval=0;
    double accruedseconds=0;
    double accruedkm=0;

    bool eof = false;
    while (!is.atEnd() && !eof) {
        // the readLine() method doesn't handle old Macintosh CR line endings
        // this workaround will load the entire file if it has CR endings
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

            if (line.length()==0) {
                continue;
            }

            if (lineno == 1) {
                if (ergomoCSV.indexIn(line) != -1) {
                    csvType = ergomo;
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
                    csvType = ibike;
                    rideFile->setDeviceType("iBike");
                    rideFile->setFileFormat("iBike CSV (csv)");
                    unitsHeader = 5;
                    iBikeVersion = line.section( ',', 1, 1 ).toInt();

                    ++lineno;
                    continue;
                }

                else if(xtrainCSV.indexIn(line) != -1) {
                    csvType = xtrain;
                    rideFile->setDeviceType("xtrainCSV");
                    rideFile->setFileFormat("xtrainCSV");
                    unitsHeader = 2;
                    xTrainVersion = line.section( ',', 1, 1 ).toInt();

                    ++lineno;
                    continue;
                }


                else if(motoActvCSV.indexIn(line) != -1) {
                    csvType = motoactv;
                    rideFile->setDeviceType("MotoACTV");
                    rideFile->setFileFormat("MotoACTV CSV (csv)");
                    unitsHeader = -1;
                    /* MotoACTV files are always metric */
                    metric = true;
                    ++lineno;
                    continue;
                 }
                 else if(jouleCSV.indexIn(line) != -1) {
                    csvType = joule;
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
                 else if(moxyCSV.indexIn(line) != -1) {
                    csvType = moxy;
                    rideFile->setDeviceType("Moxy");
                    rideFile->setFileFormat("Moxy CSV (csv)");
                    unitsHeader = 4;
                    recInterval = 1;
                    ++lineno;
                    continue;
                }
                else if(smo2CSV.indexIn(line) != -1) {
                   csvType = bsx;
                   rideFile->setDeviceType("BSX Insight");
                   rideFile->setFileFormat("BSX Insight CSV (csv)");
                   unitsHeader = 6;
                   recInterval = 1;
                   ++lineno;
                   continue;
               }
                else if(gcCSV.indexIn(line) != -1 || gcCSVold.indexIn(line) != -1) {
                    csvType = gc;
                    rideFile->setDeviceType("GoldenCheetah");
                    rideFile->setFileFormat("GoldenCheetah CSV (csv)");
                    unitsHeader = 1;
                    recInterval = 1;

                    ++lineno;
                    continue;
               }
                else if(periCSV.indexIn(line) != -1) {
                    csvType = peripedal;
                    rideFile->setDeviceType("Peripedal");
                    rideFile->setFileFormat("Peripedal CSV (csv)");
                    unitsHeader = 1;
                    recInterval = 1;
                    ++lineno;
                    continue;
               }
               else if(powertapCSV.indexIn(line) != -1) {
                    csvType = powertap;
                    rideFile->setDeviceType("PowerTap");
                    rideFile->setFileFormat("PowerTap CSV (csv)");
                    unitsHeader = 1;
                    ++lineno;
                    continue;
               }
               else if(freemotionCSV.indexIn(line) != -1) {
                    csvType = freemotion;
                    rideFile->setDeviceType("Freemotion Bike");
                    rideFile->setFileFormat("Stages Data (csv)");
                    unitsHeader = 3;
                    ++lineno;
                    continue;
               }
               else if(cpexportCSV.indexIn(line) != -1) {
                    csvType = cpexport;
                    rideFile->setDeviceType("CP Plot Export");
                    rideFile->setFileFormat("CP Plot Export (csv)");
                    unitsHeader = 1;
                    ++lineno;
                    continue;
               }
               else if(rowproCSV.indexIn(line) != -1) {
                     csvType = rowpro;
                     rideFile->setDeviceType("RowPro");
                     rideFile->setFileFormat("RowPro CSV (csv)");
                     unitsHeader = 10;
                     ++lineno;
                     continue;
               }
               else if(wahooMACSV.indexIn(line) != -1) {
                   csvType = wahooMA;
                   rideFile->setDeviceType("Wahoo Fitness");
                   rideFile->setFileFormat("Wahoo Motion Analysis CSV (csv)");
                   unitsHeader = 1;
                   recInterval = 1;
                   //++lineno;
                   //continue;
               } else if (rp3CSV.indexIn(line) != -1) {

                   csvType = rp3;
                   rideFile->setDeviceType("Row Perfect 3");
                   rideFile->setFileFormat("Row Perfect CSV (csv)");
                   unitsHeader = 1;
                   recInterval = 1; // oh.. its variable (!)

                   // add XDATA
                   rowSeries = new XDataSeries();
                   rowSeries->name = "ROW";
                   rowSeries->valuename << "ID"
                                        << "INTERVAL"
                                        << "REF"
                                        << "STROKE"
                                        << "POWER"
                                        << "AVGPOWER"
                                        << "STROKERATE"
                                        << "TIME"
                                        << "STROKELENGTH"
                                        << "DISTANCE"
                                        << "STROKEDISTANCE"
                                        << "ESTIMATE500MTIME"
                                        << "STROKEENERGY"
                                        << "ENERGYSUM"
                                        << "PULSE"
                                        << "WORKPERPULSE"
                                        << "PEAKFORCE"
                                        << "PEAKFORCEPOS"
                                        << "PEAKFORCERELPOS"
                                        << "DRIVETIME"
                                        << "RECOVERTIME"
                                        << "K"
                                        << "CURVEDATA"
                                        << "STROKENUMINTERVAL"
                                        << "AVGPOWER";

                    // and the associated units
                    rowSeries->unitname << "" << "" << "" << "" << "watts"
                                        << "watts" << "spm" << "" << "meters" << "meters"
                                        << "meters" << "" << "joules" << "joules" << "bpm"
                                        << "joules" << "newtons" << "" << "" << "secs"
                                        << "secs" << "" << "" << "" << "watts";

                    rideFile->addXData("ROW", rowSeries);


               } else if (line == "secs,km,power,hr,cad,alt") {
                    // OpenData CSV
                    csvType = opendata;
                    rideFile->setDeviceType("Unknown");
                    rideFile->setFileFormat("OpenData CSV (csv)");

               } else {  // default
                    csvType = generic;
                    rideFile->setDeviceType("Unknow");
                    rideFile->setFileFormat("Generic CSV (csv)");
               }
            }
            if (csvType == ibike) {
                if (lineno == 2) {
                    QStringList f = line.split(",");
                    if (f.size() == 6) {
                        startTime = QDateTime(
                            QDate(f[0].toInt(), f[1].toInt(), f[2].toInt()),
                            QTime(f[3].toInt(), f[4].toInt(), f[5].toInt()));
                    }
                }
                else if (lineno == 4) {
                    // this is the line with the iBike configuration data
                    // recording interval is in the [4] location (zero-based array)
                    // the trailing zeroes in the configuration area seem to be causing an error
                    // the number is in the format 5.000000
                    recInterval = (int)line.section(',',4,4).toDouble();
                    rideFile->setDeviceType(line.section(',',26,26));
                    rideFile->setTag("Device Info", line.section(',',20,21).remove(QChar('"')));

                    ibikeSeries = new XDataSeries();
                    ibikeSeries->name = "AERO";
                    ibikeSeries->valuename << "CALC-POWER";
                    ibikeSeries->unitname << "W";
                    //ibikeSeries->valuename << "CdA";
                    //ibikeSeries->unitname << "m^2";
                }
            }

            if (csvType == freemotion) {
                if (lineno == 2) {
                    if (line == "English")
                        metric = false;
                }
            }



            if (csvType == joule && lineno == 2) {
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
            if (csvType == rowpro && lineno == 8) {
                startTime = QDateTime::fromString(line.section(',', 0, 0), "dd/MM/yyyy H:mm:ss");
            }
            if (lineno == unitsHeader && (csvType == generic || csvType == bsx || csvType == wahooMA)) {
                QRegExp timeHeaderSecs("( )*(secs|sec|time|timestamp)( )*", Qt::CaseInsensitive);
                //QRegExp timeHeaderTimestamp("( )*(timestamp)( )*", Qt::CaseInsensitive);
                //QRegExp timeHeaderMins("( )*(min|minutes)( )*", Qt::CaseInsensitive);
                QRegExp wattsHeader("( )*(watts|power)( )*", Qt::CaseInsensitive);
                QRegExp cadenceHeader("( )*(cadence)( )*", Qt::CaseInsensitive);
                QRegExp smo2Header("( )*(smo2)( )*", Qt::CaseInsensitive);
                QRegExp hrHeader("( )*(hr|heart_rate)( )*", Qt::CaseInsensitive);
                QRegExp gctHeader("( )*(groundcontacttime)( )*", Qt::CaseInsensitive);
                QRegExp voHeader("( )*(verticaloscillation)( )*", Qt::CaseInsensitive);
                QRegExp kphHeader("( )*(speed)( )*", Qt::CaseInsensitive);
                QStringList headers = line.split(",");

                QStringListIterator i(headers);

                while (i.hasNext()) {
                    QString header = i.next();
                    if (timeHeaderSecs.indexIn(header) == 0)  {
                        secsIndex = headers.indexOf(header);
                        if (csvType == bsx)
                            secsIndex++;
                    }
                    /* UNUSEDelse if (timeHeaderTimestamp.indexIn(header) != -1)  {
                        timestampIndex = headers.indexOf(header);
                        if (csvType == bsx)
                            timestampIndex++;
                    } */
                    /* UNUSED else if (timeHeaderMins.indexIn(header) != -1)  {
                        minutesIndex = headers.indexOf(header);
                    } */

                    if (wattsHeader.indexIn(header) == 0)  {
                        wattsIndex = headers.indexOf(header);
                        if (csvType == bsx)
                            wattsIndex++;
                    }
                    if (cadenceHeader.indexIn(header) != -1)  {
                        cadenceIndex = headers.indexOf(header);
                        if (csvType == bsx)
                            cadenceIndex++;
                    }
                    if (hrHeader.indexIn(header) != -1)  {
                        hrIndex = headers.indexOf(header);
                        if (csvType == bsx)
                            hrIndex++;
                    }
                    if (smo2Header.indexIn(header) != -1)  {
                        smo2Index = headers.indexOf(header);
                        if (csvType == bsx)
                            smo2Index++;
                    }
                    if (gctHeader.indexIn(header) != -1)  {
                        gctIndex = headers.indexOf(header);
                        if (csvType == bsx)
                            gctIndex++;
                    }
                    if (voHeader.indexIn(header) != -1)  {
                        voIndex = headers.indexOf(header);
                        if (csvType == bsx)
                            voIndex++;
                    }
                    if (kphHeader.indexIn(header) != -1)  {
                        kphIndex = headers.indexOf(header);
                        if (csvType == bsx)
                            kphIndex++;
                    }
                }

            } else if (lineno == unitsHeader && csvType != moxy && csvType != peripedal && csvType != rowpro && csvType != rp3) {

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

            } else if (lineno > unitsHeader) {
                double minutes=0,nm=0,kph=0,watts=0,km=0,cad=0,alt=0,hr=0,dfpm=0, seconds=0.0;
                double temp=RideFile::NA;
                double slope=0.0;
                bool ok;
                double lat = 0.0, lon = 0.0;
                double headwind = 0.0;
                double lrbalance = RideFile::NA;
                double lte = 0.0, rte = 0.0;
                double lps = 0.0, rps = 0.0;
                double smo2 = 0.0, thb = 0.0;
                double gct = 0.0, vo = 0.0, rcad = 0.0;
                //UNUSED double o2hb = 0.0, hhb = 0.0;
                double target = 0.0;

                int interval=0;
                int pause=0;


                quint64 ms;

                if (csvType == powertap || csvType == joule) {
                     minutes = line.section(',', 0, 0).toDouble();
                     nm = line.section(',', 1, 1).toDouble();
                     kph = line.section(',', 2, 2).toDouble();
                     watts = line.section(',', 3, 3).toDouble();
                     km = line.section(',', 4, 4).toDouble();
                     cad = line.section(',', 5, 5).toDouble();
                     hr = line.section(',', 6, 6).toDouble();
                     interval = line.section(',', 7, 7).toInt();
                     alt = line.section(',', 8, 8).toDouble();
                    if (csvType == joule && tempType != degNone) {
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

                } else if (csvType == gc) {
                    // GoldenCheetah CVS Format "secs, cad, hr, km, kph, nm, watts, alt, lon, lat, headwind, slope, temp, interval, lrbalance, lte, rte, lps, rps, smo2, thb, o2hb, hhb\n";

                    seconds = line.section(',', 0, 0).toDouble();
                    minutes = seconds / 60.0f;
                    cad = line.section(',', 1, 1).toDouble();
                    hr = line.section(',', 2, 2).toDouble();
                    km = line.section(',', 3, 3).toDouble();
                    kph = line.section(',', 4, 4).toDouble();
                    nm = line.section(',', 5, 5).toDouble();
                    watts = line.section(',', 6, 6).toDouble();
                    alt = line.section(',', 7, 7).toDouble();
                    lon = line.section(',', 8, 8).toDouble();
                    lat = line.section(',', 9, 9).toDouble();
                    headwind = line.section(',', 10, 10).toDouble();
                    slope = line.section(',', 11, 11).toDouble();
                    temp = line.section(',', 12, 12)=="" ? double(RideFile::NA) : line.section(',', 12, 12).toDouble();
                    interval = line.section(',', 13, 13).toInt();
                    lrbalance = line.section(',', 14, 14).toDouble();
                    lte = line.section(',', 15, 15).toDouble();
                    rte = line.section(',', 16, 16).toDouble();
                    lps = line.section(',', 17, 17).toDouble();
                    rps = line.section(',', 18, 18).toDouble();
                    smo2 = line.section(',', 19, 19).toDouble();
                    thb = line.section(',', 20, 20).toDouble();
                    //UNUSED o2hb = line.section(',', 21, 21).toDouble();
                    //UNUSED hhb = line.section(',', 22, 22).toDouble();
                    target = line.section(',', 23, 23).toInt();

                } else if (csvType == peripedal) {

                    //mm-dd,hh:mm:ss,SmO2 Live,SmO2 Averaged,THb,Target Power,Heart Rate,Speed,Power,Cadence
                    // ignore lines with wrong number of entries
                    if (line.split(",").count() != 10) continue;

                    seconds = moxySeconds(line.section(',',1,1));
                    minutes = seconds / 60.0f;

                    if (startTime == QDateTime()) {
                        QDate date = periDate(line.section(',',0,0));
                        QTime time = QTime(0,0,0).addSecs(seconds);
                        startTime = QDateTime(date,time);
                    }

                    double aSmo2 = line.section(',', 3, 3).toDouble();
                    smo2 = line.section(',', 2, 2).toDouble();

                    // use average if live not available
                    if (aSmo2 && !smo2) smo2 = aSmo2;

                    thb = line.section(',', 4, 4).toDouble();
                    hr = line.section(',', 6, 6).toDouble();
                    kph = line.section(',', 7, 7).toDouble();
                    watts = line.section(',', 8, 8).toDouble();
                    cad = line.section(',', 10, 10).toDouble();

                    // dervice distance from speed
                    km = lastKM + (kph/3600.0f);
                    lastKM = km;

                    nm = 0;
                    alt = 0;
                    lon = 0;
                    lat = 0;
                    headwind = 0;
                    slope = 0;
                    temp = 0;
                    interval = 0;
                    lrbalance = 0;
                    lte = 0;
                    rte = 0;
                    lps = 0;
                    rps = 0;
                    //UNUSED o2hb = 0;
                    //UNUSED hhb = 0;

                } else if (csvType == freemotion) {
                    if (line == "Ride_Totals") {
                        eof = true;
                        continue;
                    }

                    QRegExp timestampRegEx("^([0-9]*):([0-9]*)$");
                    QString timestamp = line.section(',', 0, 0);


                    // Time,Miles,MPH,Watts,HR,RPM
                    if (!timestampRegEx.exactMatch(timestamp)) continue;

                    int sec = timestampRegEx.cap(2).toInt();
                    int min = timestampRegEx.cap(1).toInt();
                    minutes = (double(min) + double(sec)/60.0f);

                    cad = line.section(',', 5, 5).toDouble();
                    hr = line.section(',', 4, 4).toDouble();
                    km = line.section(',', 1, 1).toDouble();
                    kph = line.section(',', 2, 2).toDouble();
                    watts = line.section(',', 3, 3).toDouble();

                    if (!metric) {
                        km *= KM_PER_MILE;
                        kph *= KM_PER_MILE;
                    }

               } else if (csvType == ibike) {
                    // this must be iBike
                    // can't find time as a column.
                    // will we have to extrapolate based on the recording interval?
                    // reading recording interval from config data in ibike csv file
                    //
                    // For iBike software version 11 or higher:
                    // use "power" field until a the "dfpm" field becomes non-zero.
                     minutes = (recInterval * lineno - unitsHeader)/60.0;
                     QString timestamp = line.section( ',', 14, 14);
                     if (timestamp.length()>0){
                         minutes = startTime.secsTo(QDateTime::fromString(timestamp, Qt::ISODate))/60.0;
                     }
                     nm = 0; //no torque
                     kph = line.section(',', 0, 0).toDouble();
                     dfpm = line.section( ',', 11, 11).toDouble();
                     headwind = line.section(',', 1, 1).toDouble();
                     km = line.section(',', 3, 3).toDouble();

                     if( iBikeVersion >= 11 && ( dfpm > 0.0 || dfpmExists ) ) {
                         dfpmExists = true;
                         watts = dfpm;

                         XDataPoint *p = new XDataPoint();
                         p->secs = minutes*60.0;
                         p->km = km;
                         p->number[0] = line.section(',', 2, 2).toDouble();
                         p->number[1] = line.section(',', 16, 16).toDouble();

                         ibikeSeries->datapoints.append(p);
                     }
                     else {
                         watts = line.section(',', 2, 2).toDouble();
                     }

                     cad = line.section(',', 4, 4).toDouble();
                     hr = line.section(',', 5, 5).toDouble();
                     alt = line.section(',', 6, 6).toDouble();
                     slope = line.section(',', 7, 7).toDouble();
                     temp = line.section(',', 8, 8).toDouble();
                     lat = line.section(',', 12, 12).toDouble();
                     lon = line.section(',', 13, 13).toDouble();


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


                } else if (csvType == xtrain) {
                    // this must be xtrain
                    // ignore lines with wrong number of entries
                    if (line.split(",").count() != 6) continue;

                    minutes = (recInterval * lineno - unitsHeader)/60.0;
                    nm = 0; //no torque
                    hr = line.section(',', 1, 1).toDouble();
                    cad = line.section(',', 2, 2).toDouble();
                    watts = line.section(',', 3, 3).toDouble();
                    slope = line.section(',', 4, 4).toDouble()/10;
                    kph = line.section(',', 5, 5).toDouble();

                    // derive distance from speed
                    km = lastKM + (kph/3600.0f);
                    lastKM = km;



                } else if (csvType == moxy)  {

                    // we get crappy lines with no data so ignore them
                    // I think they're supposed to be delimiters for the file
                    // content, but are just noise to us !
                    if (line == (" ,,,,,") || line == ",,,,," ||
                        line == "" || line == " ") continue;

                    // need to get time from second column and note that
                    // there will be gaps when recording drops so shouldn't
                    // assume it is a continuous stream
                    double seconds = moxySeconds(line.section(',',1,1));

                    if (startTime == QDateTime()) {
                        QDate date = moxyDate(line.section(',',0,0));
                        QTime time = QTime(0,0,0).addSecs(seconds);
                        startTime = QDateTime(date,time);
                    }

                    if (seconds >0) {
                        minutes = seconds / 60.0f;
                        smo2 = line.section(',', 2, 2).remove("\"").toDouble();
                        thb = line.section(',', 4, 4).remove("\"").toDouble();
                    }
                }
                else if (csvType == bsx || csvType == wahooMA)  {
                    if (secsIndex > -1) {
                        seconds = line.section(',', secsIndex, secsIndex).toDouble();

                        QDateTime time;

                        if (seconds < 1000000000000.0L)
                            time = QDateTime::fromTime_t(seconds);
                        else
                            time = QDateTime::fromMSecsSinceEpoch(seconds);

                        if (startTime == QDateTime()) {
                            startTime = time;
                            seconds = 1;
                        }
                        else {
                            seconds = startTime.secsTo(time)+1;
                        }
                        minutes = seconds / 60.0f;
                    }

                    if (wattsIndex > -1) {
                        watts = line.section(',', wattsIndex, wattsIndex).toDouble();
                    }
                    if (cadenceIndex > -1) {
                        cad = line.section(',', cadenceIndex, cadenceIndex).toDouble();
                    }
                    if (hrIndex > -1) {
                        hr = line.section(',', hrIndex, hrIndex).toDouble();
                    }
                    if (smo2Index > -1) {
                        smo2 = line.section(',', smo2Index, smo2Index).toDouble();
                    }
                    if (gctIndex > -1) {
                        gct = line.section(',', gctIndex, gctIndex).toDouble();
                    }
                    if (voIndex > -1) {
                        vo = line.section(',', voIndex, voIndex).toDouble();
                    }
                    if (kphIndex > -1) {
                        kph = line.section(',', kphIndex, kphIndex).toDouble() * 3.6f; // running speed is given in m/s, convert to km/h
                        if (!metric) {
                           kph *= KM_PER_MILE;
                        }
                    }
                }
               else if(csvType == motoactv) {
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
                else if (csvType == ergomo) {
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
                } else if (csvType == cpexport) {
                    // seconds, value, (model), date
                    seconds = line.section(',',0,0).toDouble();
                    if (seconds == precSecs)
                        continue;
                    minutes = seconds / 60.0f;


                    //seconds = lineno -1 ;
                    double avgWatts = line.section(',', 1, 1).toDouble();
                    if ( avgWatts > maxWatts ) {
                        maxWatts = avgWatts;
                    }

                    //watts = (avgwatts * seconds - precSecs * precWatts) / (seconds - precSecs);
                    watts = (avgWatts * seconds - precSecs * precAvg) / (seconds - precSecs);
                    if ( watts > maxWatts ) {
                        watts = maxWatts;
                    }


                    for (int gap=1; seconds-gap>precSecs; gap++) {
                        rideFile->appendPoint(seconds-gap, cad, hr, km,
                                              kph, nm, watts, alt, lon, lat,
                                              headwind, slope, temp, lrbalance,
                                              lte, rte, lps, rps,
                                              0.0, 0.0,
                                              0.0, 0.0, 0.0, 0.0,
                                              0.0, 0.0, 0.0, 0.0,
                                              smo2, thb,
                                              0.0, 0.0, 0.0, 0.0, interval);
                    }

                    precAvg = (precAvg * precSecs + (watts>0?watts:0) * (seconds - precSecs)) / seconds;
                    //qDebug() << seconds << avgwatts << precSecs << precWatts << ":" <<watts << "->" << precAvg;
                    precSecs = seconds;
                    //precWatts = avgwatts;
                } else if (csvType == rowpro) {
                    // RowPro CSV type "Time,Distance,Pace,Watts,Cals,SPM,HR,DutyCycle,Rowfile_Id"
                    //                  0   , 1      , 2  , 3   , 4  , 5 , 6, 7       , 8
                    // Time is milliseconds
                    // Distance in meters
                    // Pace is seconds per meter

                    // Skip the summary at the end of the file
                    if (line == "Type,Time,Distance,AvgPace,AvgWatts,Cals,SPM,EndHR,Rowfile_Id,AvgHR") {
                        unitsHeader = lineno + 1000;
                        continue;
                    }
                    seconds = line.section(',', 0, 0).toDouble() / 1000;
                    minutes = seconds / 60.0f;
                    km = line.section(',', 1, 1).toDouble() / 1000;
                    double pace = line.section(',', 2, 2).toDouble();
                    if (pace > 0 ) {
                        kph = 3.6 / pace;
                    }
                    watts = line.section(',', 3, 3).toDouble();
                    cad = line.section(',', 5, 5).toDouble();
                    hr = line.section(',', 6, 6).toDouble();

               } else {
                    if (secsIndex > -1) {
                        seconds = line.section(',', secsIndex, secsIndex).toDouble();
                        minutes = seconds / 60.0f;
                     }
                }

                // PT reports no data as watts == -1.
                if (watts == -1)
                    watts = 0;

               if(csvType == motoactv)
                    rideFile->appendPoint(seconds, cad, hr, km,
                                          kph, nm, watts, alt, lon, lat, 0.0,
                                          0.0, temp, 0.0, 0.0, 0.0, 0.0, 0.0,
                                          0.0, 0.0, 0.0, 0.0,
                                          0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                          0.0, 0.0, 0.0, 0.0, 0.0, 0.0, interval);
               else if (csvType == moxy) {

                    // hack it in for now
                    // XXX IT COULD BE RECORDED WITH DIFFERENT INTERVALS XXX
                    rideFile->appendPoint(minutes * 60.0, cad, hr, km,
                                          kph, nm, watts, alt, lon, lat,
                                          headwind, slope, temp, 0.0,
                                          0.0, 0.0, 0.0, 0.0,
                                          0.0, 0.0,
                                          0.0, 0.0, 0.0, 0.0,
                                          0.0, 0.0, 0.0, 0.0,
                                          smo2, thb, 0.0, 0.0, 0.0, 0.0, interval);
                    rideFile->appendPoint((minutes * 60.0)+1, cad, hr, km, // dupe it so we have 1s recording easier to merge
                                          kph, nm, watts, alt, lon, lat,
                                          headwind, slope, temp, 0.0,
                                          0.0, 0.0, 0.0, 0.0,
                                          0.0, 0.0,
                                          0.0, 0.0, 0.0, 0.0,
                                          0.0, 0.0, 0.0, 0.0,
                                          smo2, thb, 0.0, 0.0, 0.0, 0.0, interval);

               } else if (csvType == rp3) {

                    // row perfect is variable rate (for every stroke)
                    // we add time, distance and power to standard fields
                    // and the rest becomes XDATA

                    // we need to handle data surrounded by quotes for RP3
                    QStringList els;
                    QString word;
                    bool inquote=false;
                    for(int i=0; i<line.length(); i++) {
                        switch(line[i].toLatin1()) {
                            case ',' : if (inquote) word.append(line.mid(i,1));
                                       else {
                                            els << word;
                                            word="";
                                       }
                                       break;
                            case '\"': inquote = !inquote;
                                       break;
                            default:   word.append(line.mid(i,1));
                                       break;
                        }
                    }
                    els << word; // last entry not comma delimeted

                    if (els.count() == 25) {

                        if (els[1].toInt() != lastinterval) {
                            // we have a new interval marker!
                            lastinterval = els[1].toInt();
                            accruedseconds = lastsecs >= 0 ? lastsecs : 0;
                            accruedkm = lastKM;
                            currentInterval++;
                        }

                        // ignore time goes backwards
                        lastsecs=accruedseconds + els[7].toDouble();
                        lastKM=accruedkm + (els[9].toDouble()/1000);

                        rideFile->appendPoint(lastsecs,      // time in seconds
                                              0,                      // cad
                                              els[14].toDouble(),     // hr
                                              lastKM, // distance (km, not meters)
                                              0, 0,                   // kph, nm
                                              els[4].toDouble(),      // power
                                              0, 0, 0, 0, 0,          // alt, lon, lat, headw, slope
                                              -255, 0, 0, 0, 0, 0,    // temp, lrb, lte, rte, lps, rps
                                              0.0, 0.0,
                                              0.0, 0.0, 0.0, 0.0,
                                              0.0, 0.0, 0.0, 0.0,
                                              0, 0,                   // smo2, thb
                                              0, 0, 0, 0.0,
                                              currentInterval);

                        // add ALL data series to XDATA
                        // with NO conversion, stored exactly as found
                        XDataPoint *p = new XDataPoint();
                        p->secs = lastsecs;
                        p->km = lastKM;
                        for(int i=0; i<25; i++)
                            p->number[i] = els[i].toDouble();

                        rowSeries->datapoints.append(p);
                    }

               } else if (csvType == opendata) {

                    // secs,km,power,hr,cad,alt
                    double secs = line.section(',', 0, 0).toDouble();
                    km = line.section(',', 1, 1).toDouble();
                    watts = line.section(',', 2, 2).toDouble();
                    hr = line.section(',', 3, 3).toDouble();
                    cad = line.section(',', 4, 4).toDouble();
                    alt = line.section(',', 5, 5).toDouble();
                    kph = (km - lastkm) / (secs-lastsecs) * 3600;

                    // for next time
                    lastsecs = secs;
                    lastkm = km;

                    rideFile->appendPoint(secs, cad, hr, km, kph, 0.0, watts, alt,
                                          0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                          0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                          0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                          0.0, 0.0, 0);

               } else {
                    if (vo>0 || gct>0) {
                       rcad = cad;
                       cad = 0.0;
                    }
                    rideFile->appendPoint(minutes * 60.0, cad, hr, km,
                                          kph, nm, watts, alt, lon, lat,
                                          headwind, slope, temp, lrbalance,
                                          lte, rte, lps, rps,
                                          0.0, 0.0,
                                          0.0, 0.0, 0.0, 0.0,
                                          0.0, 0.0, 0.0, 0.0,
                                          smo2, thb,
                                          vo, rcad, gct, 0.0, interval);

                    if (target > 0.0) {
                        if (trainSeries == NULL)  {
                            // add XDATA
                            trainSeries = new XDataSeries();
                            trainSeries->name = "TRAIN";
                            trainSeries->valuename << "TARGET";
                            trainSeries->unitname << "Watts";
                        }

                        XDataPoint *p = new XDataPoint();
                        p->secs = minutes * 60.0;
                        p->km = km;
                        p->number[0] = target;

                        trainSeries->datapoints.append(p);
                    }
               }
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
        errors << "Insufficient valid data in file \"" + file.fileName() + "\". ";
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

            // Could be Tryyyyddmmhhmm.csv (case insensitive)
            // X-trainer file
            rideTime.setPattern("Tr(\\d\\d\\d\\d)(\\d\\d)(\\d\\d)(\\d\\d)(\\d\\d)[^\\.]*\\.csv$");
            if (rideTime.indexIn(file.fileName()) >= 0) {
                QDateTime datetime(QDate(rideTime.cap(1).toInt(),
                                         rideTime.cap(2).toInt(),
                                         rideTime.cap(3).toInt()),
                                   QTime(rideTime.cap(4).toInt(),
                                         rideTime.cap(5).toInt()));
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
 }
    if (csvType == rowpro) rideFile->setTag("Sport","Row");

    if (trainSeries != NULL) {
        if (trainSeries->datapoints.count()>0)
            rideFile->addXData("TRAIN", trainSeries);
        else
            delete trainSeries;
    }

    if (ibikeSeries != NULL) {
        if (ibikeSeries->datapoints.count()>0)
            rideFile->addXData("AERO", ibikeSeries);
        else
            delete ibikeSeries;
    }

    // did we actually read any samples?
    if (rideFile->dataPoints().count() == 0) {
        errors << "No samples present.";
        delete rideFile;
        return NULL;
    }

    // last, is there an associated rr file?
    //
    // typically only for GC csv, but lets not constrain that
    // so long as the filename matches we'll import it into XDATA
    QFile rrfile(file.fileName().replace(".csv",".rr"));
    if (!rrfile.open(QFile::ReadOnly)) return rideFile;

    // create the XDATA series
    rrSeries = new XDataSeries();
    rrSeries->name = "HRV"; // using same format as Polar HRV imports
    rrSeries->valuename << "R-R";
    rrSeries->unitname << "msecs";

    // attempt to read and add the data
    lineno=1;
    QTextStream rs(&rrfile);

    // loop through lines and truncate etc
    while (!rs.atEnd()) {
        // the readLine() method doesn't handle old Macintosh CR line endings
        // this workaround will load the the entire file if it has CR endings
        // then split and loop through each line
        // otherwise, there will be nothing to split and it will read each line as expected.
        QString linesIn = rs.readLine();
        QStringList lines = linesIn.split('\r');
        // workaround for empty lines
        if(lines.isEmpty()) {
            lineno++;
            continue;
        }
        for (int li = 0; li < lines.size(); ++li) {
            QString line = lines[li];

            if (line.length()==0) {
                continue;
            }

            // first line is a header line
            if (lineno > 1) {

                // split comma separated secs, hr, msecs
                QStringList values = line.split(",", QString::KeepEmptyParts);

                // and add
                XDataPoint *p = new XDataPoint();
                p->secs = values.at(0).toDouble();
                p->km = 0;
                p->number[0] = values.at(2).toDouble();

                rrSeries->datapoints.append(p);
            }

            // onto next line
            ++lineno;
        }
    }
    // free handle
    rrfile.close();

    // add if we got any ....
    if (rrSeries->datapoints.count() > 0) rideFile->addXData("HRV", rrSeries);

    // all done
    return rideFile;
}

bool
CsvFileReader::writeRideFile(Context *, const RideFile *ride, QFile &file, CsvType format) const
{
    if (!file.open(QIODevice::WriteOnly)) return(false);

    // always save CSV in metric format
    bool bIsMetric = true;

    // Use the column headers that make WKO+ happy.
    double convertUnit;
    QTextStream out(&file);

    if (format == gc) {
        // CSV File header
        out << "secs,cad,hr,km,kph,nm,watts,alt,lon,lat,headwind,slope,temp,interval,lrbalance,lte,rte,lps,rps,smo2,thb,o2hb,hhb\n";

        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->secs == 0.0)
                continue;
            out << point->secs;
            out << ",";
            out << point->cad;
            out << ",";
            out << point->hr;
            out << ",";
            out << point->km;
            out << ",";
            out << ((point->kph >= 0) ? point->kph : 0.0);
            out << ",";
            out << ((point->nm >= 0) ? point->nm : 0.0);
            out << ",";
            out << ((point->watts >= 0) ? point->watts : 0.0);
            out << ",";
            out << point->alt;
            out << ",";
            out << QString::number(point->lon, 'f', 8);
            out << ",";
            out << QString::number(point->lat, 'f', 8);
            out << ",";
            out << point->headwind;
            out << ",";
            out << point->slope;
            out << ",";
            out << point->temp;
            out << ",";
            out << point->interval;
            out << ",";
            out << point->lrbalance;
            out << ",";
            out << point->lte;
            out << ",";
            out << point->rte;
            out << ",";
            out << point->lps;
            out << ",";
            out << point->rps;
            out << ",";
            out << point->smo2;
            out << ",";
            out << point->thb;
            out << ",";
            out << point->o2hb;
            out << ",";
            out << point->hhb;

            out << "\n";
        }
    }
    else if (format == powertap) {
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
    }
    else if (format == wprime) {
        WPrime *wp = ((RideFile*)ride)->wprimeData();
        bool integral = (appsettings->value(NULL, GC_WBALFORM, "int").toString() == "int");

        // Infos
        out << "CP=" << wp->CP << ",WPRIME=" << wp->WPRIME << ",TAU=" << wp->TAU << ",model=" << (integral?"integral":"differential") <<"\n";

        // CSV File header
        out << "secs,watts,w'bal\n";

        for (int i=0;i<wp->xdata(false).count();i++) {
            out << wp->xdata(false).at(i)*60;
            out << ",";
            out << wp->smoothArray.at(i);
            out << ",";
            out << wp->ydata().at(i);
            out << "\n";
        }
    }

    file.close();
    return true;
}
