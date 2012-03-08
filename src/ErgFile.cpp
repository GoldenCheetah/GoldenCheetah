/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "ErgFile.h"

#include <stdint.h>
#include "Units.h"

ErgFile::ErgFile(QString filename, int &mode, double Cp, MainWindow *main) : 
    Cp(Cp), filename(filename), main(main), mode(mode)
{
    reload();
}

ErgFile::ErgFile(MainWindow *main) : main(main), mode(nomode)
{
    Cp = 300;
    filename ="";
}

ErgFile *
ErgFile::fromContent(QString contents, MainWindow *main)
{
    ErgFile *p = new ErgFile(main);
    p->parseComputrainer(contents);
    return p;
}

void ErgFile::reload()
{
    // which parser to call? XXX should look at moving to an ergfile factory
    // like we do with ride files if we end up with lots of different formats
    if (filename.endsWith(".pgmf", Qt::CaseInsensitive)) parseTacx();
    else parseComputrainer();
    
}

void ErgFile::parseTacx()
{
    // Initialise
    Version = "";
    Units = "";
    Filename = "";
    Name = "";
    Duration = -1;
    Ftp = 0;            // FTP this file was targetted at
    MaxWatts = 0;       // maxWatts in this ergfile (scaling)
    valid = false;             // did it parse ok?
    rightPoint = leftPoint = 0;
    format = CRS; // default to couse until we know
    Points.clear();
    Laps.clear();

    // running totals
    double rdist = 0; // running total for distance
    double ralt = 200; // always start at 200 meters just to prettify the graph

    // open the file for binary reading and open a datastream
    QFile pgmfFile(filename);
    if (pgmfFile.open(QIODevice::ReadOnly) == false) return;
    QDataStream input(&pgmfFile);
    input.setByteOrder(QDataStream::LittleEndian);
    input.setVersion(QDataStream::Qt_4_0); // 32 bit floats not 64 bit.

    bool happy = true; // are we ok to continue reading?

    //
    // BASIC DATA STRUCTURES
    //
    struct {
        uint16_t fingerprint;
        uint16_t version;
        uint32_t blocks;
    } header; // file header

    struct {
        uint16_t type;
        uint16_t version;
        uint32_t records;
        uint32_t recordSize;
    } info; // tells us what to read

    struct {
        quint32 checksum; // 4
        // we don't use an array for the filename since C++ arrays are prepended by a 16bit size
        quint8 name[34];
        qint32 wattSlopePulse; // 42
        qint32 timeDist; // 46
        double totalTimeDist; // 54
        double energyCons;  // 62
        float  altStart; // 66
        qint32 brakeCategory; // 70
    } general; // type 1010

    struct {
        float distance;
        float slope;
        float friction;
    } program; // type 1020

    //
    // FILE HEADER
    //
    int rc = input.readRawData((char*)&header, sizeof(header));
    if (rc == sizeof(header)) {
        if (header.fingerprint == 1000 && header.version == 100) happy = true;
        else happy = false;
    } else happy = false;

    unsigned int block = 0; // keep track of how many blocks we have read

    //
    // READ THE BLOCKS INSIDE THE FILE
    //
    while (happy && block < header.blocks) {

        // read the info for this block
        rc = input.readRawData((char*)&info, sizeof(info));
        if (rc == sizeof(info)) {

            // okay now read tha block
            switch (info.type) {

            case 1010 : // general
                {
                    // read it but mostly ignore -- for now
                    // we read member by member to avoid struct word alignment problem caused
                    // by the filename being 34 bytes long (why didn't they use 32 or 36?)
                    input>>general.checksum;
                    input.readRawData((char*)&general.name[0], 34);
                    input>>general.wattSlopePulse;
                    input>>general.timeDist;
                    input>>general.totalTimeDist;
                    input>>general.energyCons;
                    input>>general.altStart;
                    input>>general.brakeCategory;
                    switch (general.wattSlopePulse) {
                    case 0 :
                        mode = format = ERG;
                        break;
                    case 1 :
                        mode = format = CRS;
                        break;
                    default:
                        happy = false;
                        break;
                    }
                    ralt = general.altStart;
                }
                break;

            case 1020 : // program
                {
                    // read in the program records
                    for (unsigned int record=0; record < info.records; record++) {

                        // get the next record
                        if (sizeof(program) != input.readRawData((char*)&program, sizeof(program))) {
                            happy = false;
                            break;
                        } 

                        ErgFilePoint add;

                        if (format == CRS) {
		                    // distance guff
                            add.x = rdist;
		                    double distance = program.distance; // in meters
		                    rdist += distance;

		                    // gradient and altitude
                            add.val = program.slope;
                            add.y = ralt;
		                    ralt += (distance * add.val) / 100;

                        } else {

                            add.x = rdist;
                            rdist += program.distance * 1000; // 1000ths of a second
                            add.val = add.y = program.slope; // its watts now
                        }

                        Points.append(add);
                        if (add.y > MaxWatts) MaxWatts=add.y;

                    }
                }
                break;

            default: // unexpected block type
                happy = false;
                break;
            }

            block++;

        } else happy = false;
    }

    // done
    pgmfFile.close();

    // if we got here and are still happy then it
    // must have been a valid file.
    if (happy) {
        valid = true;

        // set ErgFile duration
        Duration = Points.last().x;      // last is the end point in msecs
        leftPoint = 0;
        rightPoint = 1;

        // calculate climbing etc
        calculateMetrics();
    }
}

void ErgFile::parseComputrainer(QString p)
{
    QFile ergFile(filename);
    int section = NOMANSLAND;            // section 0=init, 1=header data, 2=course data
    leftPoint=rightPoint=0;
    MaxWatts = Ftp = 0;
    int lapcounter = 0;
    format = ERG;                         // either ERG or MRC
    Points.clear();

    // start by assuming the input file is Metric
    bool bIsMetric = true;

    // running totals for CRS file format
    long rdist = 0; // running total for distance
    long ralt = 200; // always start at 200 meters just to prettify the graph

    // open the file
    if (p == "" && ergFile.open(QIODevice::ReadOnly | QIODevice::Text) == false) {
        valid = false;
        return;
    }
    // Section markers
    QRegExp startHeader("^.*\\[COURSE HEADER\\].*$", Qt::CaseInsensitive);
    QRegExp endHeader("^.*\\[END COURSE HEADER\\].*$", Qt::CaseInsensitive);
    QRegExp startData("^.*\\[COURSE DATA\\].*$", Qt::CaseInsensitive);
    QRegExp endData("^.*\\[END COURSE DATA\\].*$", Qt::CaseInsensitive);
    // ignore whitespace and support for ';' comments (a GC extension)
    QRegExp ignore("^(;.*|[ \\t\\n]*)$", Qt::CaseInsensitive);
    // workout settings
    QRegExp settings("^([^=]*)=[ \\t]*([^=\\n\\r\\t]*).*$", Qt::CaseInsensitive);

    // format setting for ergformat
    QRegExp ergformat("^[;]*(MINUTES[ \\t]+WATTS).*$", Qt::CaseInsensitive);
    QRegExp mrcformat("^[;]*(MINUTES[ \\t]+PERCENT).*$", Qt::CaseInsensitive);
    QRegExp crsformat("^[;]*(DISTANCE[ \\t]+GRADE[ \\t]+WIND).*$", Qt::CaseInsensitive);

    // time watts records
    QRegExp absoluteWatts("^[ \\t]*([0-9\\.]+)[ \\t]*([0-9\\.]+)[ \\t\\n]*$", Qt::CaseInsensitive);
    QRegExp relativeWatts("^[ \\t]*([0-9\\.]+)[ \\t]*([0-9\\.]+)%[ \\t\\n]*$", Qt::CaseInsensitive);

    // distance slope wind records
    QRegExp absoluteSlope("^[ \\t]*([0-9\\.]+)[ \\t]*([-0-9\\.]+)[ \\t\\n]*([-0-9\\.]+)[ \\t\\n]*$",
                           Qt::CaseInsensitive);

    // Lap marker in an ERG/MRC file
    QRegExp lapmarker("^[ \\t]*([0-9\\.]+)[ \\t]*LAP[ \\t\\n]*(.*)$", Qt::CaseInsensitive);
    QRegExp crslapmarker("^[ \\t]*LAP[ \\t\\n]*(.*)$", Qt::CaseInsensitive);

    // ok. opened ok lets parse.
    QTextStream inputStream(&ergFile);
    QTextStream stringStream(&p);
    if (p != "") inputStream.setString(&p); // use a string not a file!
    while ((p=="" && !inputStream.atEnd()) || (p!="" && !stringStream.atEnd())) {

        // Code plagiarised from CsvRideFile.
        // the readLine() method doesn't handle old Macintosh CR line endings
        // this workaround will load the the entire file if it has CR endings
        // then split and loop through each line
        // otherwise, there will be nothing to split and it will read each line as expected.
        QString linesIn = (p != "" ? stringStream.readLine() : ergFile.readLine());
        QStringList lines = linesIn.split('\r');

        // workaround for empty lines
        if(lines.isEmpty()) {
            continue;
        }

        for (int li = 0; li < lines.size(); ++li) {
            QString line = lines[li];

            // so what we go then?
            if (startHeader.exactMatch(line)) {
                section = SETTINGS;
            } else if (endHeader.exactMatch(line)) {
                section = NOMANSLAND;
            } else if (startData.exactMatch(line)) {
                section = DATA;
            } else if (endData.exactMatch(line)) {
                section = END;
            } else if (ergformat.exactMatch(line)) {
                // save away the format
                mode = format = ERG;
            } else if (mrcformat.exactMatch(line)) {
                // save away the format
                mode = format = MRC;
            } else if (crsformat.exactMatch(line)) {
                // save away the format
                mode = format = CRS;
            } else if (lapmarker.exactMatch(line)) {
                // lap marker found
                ErgFileLap add;

                add.x = lapmarker.cap(1).toDouble() * 60000; // from mins to 1000ths of a second
                add.LapNum = ++lapcounter;
                add.name = lapmarker.cap(2).simplified();
                Laps.append(add);

            } else if (crslapmarker.exactMatch(line)) {
                // new distance lapmarker
                ErgFileLap add;

                add.x = rdist;
                add.LapNum = ++lapcounter;
                add.name = lapmarker.cap(2).simplified();
                Laps.append(add);

            } else if (settings.exactMatch(line)) {
                // we have name = value setting
                QRegExp pversion("^VERSION *", Qt::CaseInsensitive);
                if (pversion.exactMatch(settings.cap(1))) Version = settings.cap(2);

                QRegExp pfilename("^FILE NAME *", Qt::CaseInsensitive);
                if (pfilename.exactMatch(settings.cap(1))) Filename = settings.cap(2);

                QRegExp pname("^DESCRIPTION *", Qt::CaseInsensitive);
                if (pname.exactMatch(settings.cap(1))) Name = settings.cap(2);

                QRegExp punit("^UNITS *", Qt::CaseInsensitive);
                if (punit.exactMatch(settings.cap(1))) {
                    Units = settings.cap(2);
                    // UNITS can be ENGLISH or METRIC (miles/km)
                    QRegExp penglish(" ENGLISH$", Qt::CaseInsensitive);
                    if (penglish.exactMatch(Units)) { // Units <> METRIC 
                      //qDebug("Setting conversion to ENGLISH");
                      bIsMetric = false;
                    }
                  }

                QRegExp pftp("^FTP *", Qt::CaseInsensitive);
                if (pftp.exactMatch(settings.cap(1))) Ftp = settings.cap(2).toInt();

            } else if (absoluteWatts.exactMatch(line)) {
                // we have mins watts line
                ErgFilePoint add;

                add.x = absoluteWatts.cap(1).toDouble() * 60000; // from mins to 1000ths of a second
                add.val = add.y = round(absoluteWatts.cap(2).toDouble());             // plain watts

                switch (format) {

                case ERG:       // its an absolute wattage
                    if (Ftp) { // adjust if target FTP is set.
                        // if ftp is set then convert to the users CP

                        double watts = add.y;
                        double ftp = Ftp;
                        watts *= Cp/ftp;
                        add.y = add.val = watts;
                    }
                    break;
                case MRC:       // its a percent relative to CP (mrc file)
                    add.y *= Cp;
                    add.y /= 100.00;
                    add.val = add.y;
                    break;
                }
                Points.append(add);
                if (add.y > MaxWatts) MaxWatts=add.y;

            } else if (relativeWatts.exactMatch(line)) {

                // we have a relative watts match
                ErgFilePoint add;
                add.x = relativeWatts.cap(1).toDouble() * 60000; // from mins to 1000ths of a second
                add.val = add.y = (relativeWatts.cap(2).toDouble() /100.00) * Cp;
                Points.append(add);
                if (add.y > MaxWatts) MaxWatts=add.y;

            } else if (absoluteSlope.exactMatch(line)) {
                // dist, grade, wind strength
                ErgFilePoint add;

                // distance guff
                add.x = rdist;
                int distance = absoluteSlope.cap(1).toDouble() * 1000; // convert to meters

                if (!bIsMetric) distance *= KM_PER_MILE;
                rdist += distance;

                // gradient and altitude
                add.val = absoluteSlope.cap(2).toDouble();
                add.y = ralt;
                ralt += distance * add.val / 100; /* paused */

                Points.append(add);
                if (add.y > MaxWatts) MaxWatts=add.y;


            } else if (ignore.exactMatch(line)) {
                // do nothing for this line
            } else {
              // ignore bad lines for now. just bark.
              //qDebug()<<"huh?" << line;
            }

        }

    }

    // done.
    if (p=="") ergFile.close();

    if (section == END && Points.count() > 0) {
        valid = true;

        // add the last point for a crs file
        if (mode == CRS) {
            ErgFilePoint add;
            add.x = rdist;
            add.val = 0.0;
            add.y = ralt;
            Points.append(add);
            if (add.y > MaxWatts) MaxWatts=add.y;
        }

        // add a start point if it doesn't exist
        if (Points.at(0).x > 0) {
            ErgFilePoint add;
            add.x = 0;
            add.y = Points.at(0).y;
            add.val = Points.at(0).val;
            Points.insert(0, add);
        }

        // set ErgFile duration
        Duration = Points.last().x;      // last is the end point in msecs

        leftPoint = 0;
        rightPoint = 1;

        calculateMetrics();

    } else {
        valid = false;
    }
}

ErgFile::~ErgFile()
{
    Points.clear();
}

bool
ErgFile::isValid()
{
    return valid;
}

int
ErgFile::wattsAt(long x, int &lapnum)
{
    // workout what wattage load should be set for any given
    // point in time in msecs.
    if (!isValid()) return -100; // not a valuid ergfile

    // is it in bounds?
    if (x < 0 || x > Duration) return -100;   // out of bounds!!!

    // do we need to return the Lap marker?
    if (Laps.count() > 0) {
        int lap=0;
        for (int i=0; i<Laps.count(); i++) {
            if (x>=Laps.at(i).x) lap += 1;
        }
        lapnum = lap;

    } else lapnum = 0;

    // find right section of the file
    while (x < Points.at(leftPoint).x || x > Points.at(rightPoint).x) {
        if (x < Points.at(leftPoint).x) {
            leftPoint--;
            rightPoint--;
        } else if (x > Points.at(rightPoint).x) {
            leftPoint++;
            rightPoint++;
        }
    }

    // two different points in time but the same watts
    // at both, it doesn't really matter which value
    // we use
    if (Points.at(leftPoint).val == Points.at(rightPoint).val)
        return Points.at(rightPoint).val;

    // the erg file will list the point in time twice
    // to show a jump from one wattage to another
    // at this point in ime (i.e x=100 watts=100 followed
    // by x=100 watts=200)
    if (Points.at(leftPoint).x == Points.at(rightPoint).x)
        return Points.at(rightPoint).val;

    // so this point in time between two points and
    // we are ramping from one point and another
    // the steps in the calculation have been explicitly
    // listed for code clarity
    double deltaW = Points.at(rightPoint).val - Points.at(leftPoint).val;
    double deltaT = Points.at(rightPoint).x - Points.at(leftPoint).x;
    double offT = x - Points.at(leftPoint).x;
    double factor = offT / deltaT;

    double nowW = Points.at(leftPoint).val + (deltaW * factor);

    return nowW;
}

double
ErgFile::gradientAt(long x, int &lapnum)
{
    // workout what wattage load should be set for any given
    // point in time in msecs.
    if (!isValid()) return -100; // not a valid ergfile

    // is it in bounds?
    if (x < 0 || x > Duration) return -100;   // out of bounds!!! (-10 through +15 are valid return vals)

    // do we need to return the Lap marker?
    if (Laps.count() > 0) {
        int lap=0;
        for (int i=0; i<Laps.count(); i++) {
            if (x>=Laps.at(i).x) lap += 1;
        }
        lapnum = lap;
    }

    // find right section of the file
    while (x < Points.at(leftPoint).x || x > Points.at(rightPoint).x) {
        if (x < Points.at(leftPoint).x) {
            leftPoint--;
            rightPoint--;
        } else if (x > Points.at(rightPoint).x) {
            leftPoint++;
            rightPoint++;
        }
    }
    return Points.at(leftPoint).val;
}

int ErgFile::nextLap(long x)
{
    if (!isValid()) return -1; // not a valid ergfile

    // do we need to return the Lap marker?
    if (Laps.count() > 0) {
        for (int i=0; i<Laps.count(); i++) {
            if (x<Laps.at(i).x) return Laps.at(i).x;
        }
    }
    return -1; // nope, no marker ahead of there
}

void
ErgFile::calculateMetrics()
{

    // reset metrics
    XP = CP = AP = NP = IF = RI = TSS = BS = SVI = VI = 0;
    ELE = ELEDIST = GRADE = 0;

    maxY = 0; // we need to reset it

    // is it valid?
    if (!isValid()) return;

    if (format == CRS) {

        ErgFilePoint last;
        bool first = true;
        foreach (ErgFilePoint p, Points) {

            // set the maximum Y value
            if (p.y > maxY) maxY= p.y;

            if (first == true) {
                first = false;
            } else if (p.y > last.y) {

                ELEDIST += p.x - last.x;
                ELE += p.y - last.y;
            }
            last = p;
        }
        if (ELE == 0 || ELEDIST == 0) GRADE = 0;
        else GRADE = ELE/ELEDIST * 100;

    } else {

        QVector<double> rolling(30);
        rolling.fill(0.0f);
        int index = 0;
        double sum = 0; // 30s rolling average
        double apsum = 0; // average power used in VI calculation

        double total = 0;
        double count = 0;
        long nextSecs = 0;

        static const double EPSILON = 0.1;
        static const double NEGLIGIBLE = 0.1;

        double secsDelta = 1;
        double sampsPerWindow = 25.0;
        double attenuation = sampsPerWindow / (sampsPerWindow + secsDelta);
        double sampleWeight = secsDelta / (sampsPerWindow + secsDelta);

        double lastSecs = 0.0;
        double weighted = 0.0;

        double sktotal = 0.0;
        int skcount = 0;

        ErgFilePoint last;
        foreach (ErgFilePoint p, Points) {

            // set the maximum Y value
            if (p.y > maxY) maxY= p.y;

            while (nextSecs < p.x) {

                // CALCULATE NP
                apsum += last.y;
                sum +=  last.y;
                sum -= rolling[index];

                // update 30s circular buffer
                rolling[index] = last.y;
                if (index == 29) index=0;
                else index++;

                total += pow(sum/30, 4);
                count ++;

                // CALCULATE XPOWER
                while ((weighted > NEGLIGIBLE) && ((nextSecs / 1000) > (lastSecs/1000) + 1000 + EPSILON)) {
                    weighted *= attenuation;
                    lastSecs += 1000;
                    sktotal += pow(weighted, 4.0);
                    skcount++;
                }
                weighted *= attenuation;
                weighted += sampleWeight * last.y;
                lastSecs = p.x;
                sktotal += pow(weighted, 4.0);
                skcount++;

                nextSecs += 1000;
            }
            last = p;
        }

        // XP, NP and AP
        XP = pow(sktotal / skcount, 0.25);
        NP = pow(total / count, 0.25);
        AP = apsum / count;

        // CP
        if (main->zones()) {
            int zonerange = main->zones()->whichRange(QDateTime::currentDateTime().date());
            if (zonerange >= 0) CP = main->zones()->getCP(zonerange);
        }

        // IF
        if (CP) {
            IF = NP / CP;
            RI = XP / CP;
        }

        // TSS
        double normWork = NP * (Duration / 1000); // msecs
        double rawTSS = normWork * IF;
        double workInAnHourAtCP = CP * 3600;
        TSS = rawTSS / workInAnHourAtCP * 100.0;

        // BS
        double xWork = XP * (Duration / 1000); // msecs
        double rawBS = xWork * RI;
        BS = rawBS / workInAnHourAtCP * 100.0;

        // VI and RI
        if (AP) {
            VI = NP / AP;
            SVI = XP / AP;
        }
    }
}
