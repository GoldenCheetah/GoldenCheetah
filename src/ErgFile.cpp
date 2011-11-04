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


ErgFile::ErgFile(QString filename, int &mode, double Cp, MainWindow *main) : main(main)
{
    QFile ergFile(filename);
    int section = NOMANSLAND;            // section 0=init, 1=header data, 2=course data
    leftPoint=rightPoint=0;
    MaxWatts = Ftp = 0;
    int lapcounter = 0;
    format = ERG;                         // either ERG or MRC

    // running totals for CRS file format
    long rdist = 0; // running total for distance
    long ralt = 200; // always start at 200 meters just to prettify the graph

    // open the file
    if (ergFile.open(QIODevice::ReadOnly | QIODevice::Text) == false) {
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
    QRegExp absoluteSlope("^[ \\t]*([0-9\\.]+)[ \\t]*([-0-9\\.]+)[ \\t\\n]([-0-9\\.]+)[ \\t\\n]*$",
                           Qt::CaseInsensitive);

    // Lap marker in an ERG/MRC file
    QRegExp lapmarker("^[ \\t]*([0-9\\.]+)[ \\t]*LAP[ \\t\\n]*$", Qt::CaseInsensitive);
    QRegExp crslapmarker("^[ \\t]*LAP[ \\t\\n]*$", Qt::CaseInsensitive);

    // ok. opened ok lets parse.
    QTextStream inputStream(&ergFile);
    while (!inputStream.atEnd()) {

        // Code plagiarised from CsvRideFile.
        // the readLine() method doesn't handle old Macintosh CR line endings
        // this workaround will load the the entire file if it has CR endings
        // then split and loop through each line
        // otherwise, there will be nothing to split and it will read each line as expected.
        QString linesIn = ergFile.readLine();
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
                Laps.append(add);

            } else if (crslapmarker.exactMatch(line)) {
                // new distance lapmarker
                ErgFileLap add;

                add.x = rdist;
                add.LapNum = ++lapcounter;
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
			// XXX FIXME XXX
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
                qDebug()<<"huh?" << line;
	    }

        }

    }

    // done.
    ergFile.close();

    if (Points.count() > 0) {
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

    // is it in bounds?
    if (x < 0 || x > Duration) return 0;   // out of bounds!!!

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

    if (format == CRS) {

        ErgFilePoint last;
        bool first = true;
        foreach (ErgFilePoint p, Points) {

            if (first == true) {
                first = false;
            } else if (p.y > last.y) {

                ELEDIST += p.x - last.x;
                ELE += p.y - last.y;
            }
            last = p;
        }
        GRADE = ELE/ELEDIST * 100;

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
