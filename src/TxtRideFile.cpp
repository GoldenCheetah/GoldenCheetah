/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "TxtRideFile.h"
#include "Units.h"
#include <QRegExp>
#include <QTextStream>
#include <QVector>
#include <QDebug>
#include <algorithm> // for std::sort
#include <assert.h>
#include "math.h"

static int txtFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "txt","Racermate/Ergvideo", new TxtFileReader());

// Racermate Workout Data Export File Format (.TXT)
// ------------------------------------------------
//
// The Racermate TXT format export of CDF format files is also
// used by ErgVideo 3. It is rather similar to the .ERG and .CRS
// file formats
//
// The sample values can be comma separated or space separated
// depending upon configuration settings in CompCS, indeed the
// fields that are present are also configurable, which is why
// there is a line prior to the sample data that lists the
// field names.
//
// There are two example files in the test/rides directory which
// can be used as a reference for the file format.

RideFile *TxtFileReader::openRideFile(QFile &file, QStringList &errors) const
{
    QString deviceInfo; // keep a record of all the data in the
                        // sections so we can store it in the device
                        // info metadata field

    QString section = ""; // the text for the section we are currently in
                     // if it is blank we are not in any section, and should
                     // expect to see a section, or 'number of records = ' or
                     // column headings or raw data

    QStringList headings; // the headings array
    int timeIndex = -1;
    int cadIndex = -1;
    int hrIndex = -1;
    int kmIndex = -1;
    int kphIndex = -1;
    int wattsIndex = -1;
    int headwindIndex = -1;

    bool metric = true;   // are the values in metric or imperial?
    QDateTime startTime;  // parsed from filename

    // lets get on with it then
    if (!file.open(QFile::ReadOnly)) {
        errors << ("Could not open ride file: \""
                   + file.fileName() + "\"");
        return NULL;
    }
    QTextStream is(&file);

    // Lets construct our rideFile
    RideFile *rideFile = new RideFile();
    rideFile->setDeviceType("Computrainer/Velotron");

    while (!is.atEnd()) {

        // the readLine() method doesn't handle old Macintosh CR line endings
        // this workaround will load the the entire file if it has CR endings
        // then split and loop through each line
        // otherwise, there will be nothing to split and it will read each line as expected.
        QString linesIn = is.readLine();
        QStringList lines = linesIn.split('\r');
        if(lines.isEmpty()) continue;

        // loop through the lines we got
        foreach (QString line, lines) {

            QRegExp sectionPattern("^\\[.*\\]$");
            QRegExp unitsPattern("^UNITS += +\\(.*\\)$");
            QRegExp sepPattern("( +|,)");

            // ignore blank lines
            if (line == "") continue;

            // begin or end of section
            if (sectionPattern.exactMatch(line)) {
                deviceInfo += line;
                deviceInfo += "\n";

                if (section == "") section = line;
                else section = "";
                continue;
            }

            // section Data
            if (section != "") {
                // save it away
                deviceInfo += line;
                deviceInfo += "\n";

                // look for UNITS line
                if (unitsPattern.exactMatch(line)) {
                    if (unitsPattern.cap(1) != "METRIC") metric = false;
                    else metric = true;
                }
                continue;
            }

            // number of records, jsut ignore it
            if (line.startsWith("number of")) continue;

            // either a data line, or a headings line
            if (headings.count() == 0) {
                headings = line.split(sepPattern, QString::SkipEmptyParts);

                // where are the values stored?
                timeIndex = headings.indexOf("ms");
                wattsIndex = headings.indexOf("watts");
                cadIndex = headings.indexOf("rpm");
                hrIndex = headings.indexOf("hr");
                kmIndex = headings.indexOf("KM");
                kphIndex = headings.indexOf("speed");
                headwindIndex = headings.indexOf("wind");
                continue;
            }
            // right! we now have a record
            QStringList values = line.split(sepPattern, QString::SkipEmptyParts);

            // mmm... didn't get much data
            if (values.count() < 2) continue;

            // extract out each value
            double secs = timeIndex > -1 ? values[timeIndex].toDouble() / (double) 1000 : 0.0;
            double watts = wattsIndex > -1 ? values[wattsIndex].toDouble() : 0.0;
            double cad = cadIndex > -1 ? values[cadIndex].toDouble() : 0.0;
            double hr = hrIndex > -1 ? values[hrIndex].toDouble() : 0.0;
            double km = kmIndex > -1 ? values[kmIndex].toDouble() : 0.0;
            double kph = kphIndex > -1 ? values[kphIndex].toDouble() : 0.0;
            double headwind = headwindIndex > -1 ? values[headwindIndex].toDouble() : 0.0;
            rideFile->appendPoint(secs, cad, hr, km, kph, 0.0, watts, 0.0, 0.0, 0.0, headwind, 0);

        }
    }
    file.close();

    rideFile->setTag("Device Info", deviceInfo);

    //
    // To estimate the recording interval, take the median of the
    // first 1000 samples and round to nearest millisecond.
    //
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

    } else {

        // less than 2 data points is not a valid ride file
        errors << "Insufficient valid data in file \"" + file.fileName() + "\".";
        delete rideFile;
        file.close();
        return NULL;
    }

    //
    // Get date time from standard GC name
    //
    QRegExp gcPattern("^.*/(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)\\.txt$");
    gcPattern.setCaseSensitivity(Qt::CaseInsensitive);

    // Racemermate uses name-mode-yyyy-mm-dd-hh-mm-ss.CDF.txt
    QRegExp rmPattern("^.*/[^-]*-[^-]*-(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d)-(\\d\\d)-(\\d\\d)-(\\d\\d)\\.cdf.txt$");
    rmPattern.setCaseSensitivity(Qt::CaseInsensitive);

    // Ergvideo uses name_ergvideo_ridename_yyyy-mm-dd@hh-mm-ss.txt
    QRegExp evPattern("^.*/[^_]*_[^_]*_[^_]*_(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d)@(\\d\\d)-(\\d\\d)-(\\d\\d)\\.txt$");
    evPattern.setCaseSensitivity(Qt::CaseInsensitive);

    // It is a GC Filename
    if (gcPattern.exactMatch(file.fileName())) {
        QDateTime datetime(QDate(gcPattern.cap(1).toInt(),
                                 gcPattern.cap(2).toInt(),
                                 gcPattern.cap(3).toInt()),
                           QTime(gcPattern.cap(4).toInt(),
                                 gcPattern.cap(5).toInt(),
                                 gcPattern.cap(6).toInt()));
        rideFile->setStartTime(datetime);
    }

    // It is an Ergvideo Filename
    if (evPattern.exactMatch(file.fileName())) {
        QDateTime datetime(QDate(evPattern.cap(1).toInt(),
                                 evPattern.cap(2).toInt(),
                                 evPattern.cap(3).toInt()),
                           QTime(evPattern.cap(4).toInt(),
                                 evPattern.cap(5).toInt(),
                                 evPattern.cap(6).toInt()));
        rideFile->setStartTime(datetime);
    }

    // It is an Racermate Filename
    if (rmPattern.exactMatch(file.fileName())) {
        QDateTime datetime(QDate(rmPattern.cap(1).toInt(),
                                 rmPattern.cap(2).toInt(),
                                 rmPattern.cap(3).toInt()),
                           QTime(rmPattern.cap(4).toInt(),
                                 rmPattern.cap(5).toInt(),
                                 rmPattern.cap(6).toInt()));
        rideFile->setStartTime(datetime);
    }

    return rideFile;
}
