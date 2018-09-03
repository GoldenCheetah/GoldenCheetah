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
#include "cmath"

static int txtFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "txt","Racermate/Ergvideo/Wattbike", new TxtFileReader());

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

RideFile *TxtFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
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
    int milesIndex = -1;
    int wattsIndex = -1;
    int headwindIndex = -1;

    bool metric = true;   // are the values in metric or imperial?
    QDateTime startTime;  // parsed from filename

    // just to quieten the compiler, since we don't seem to
    // use the metric bool anywhere in the code at present
    if (metric) { }

    // Lets make sure we can open the file
    if (!file.open(QFile::ReadOnly)) {
        errors << ("Could not open ride file: \"" + file.fileName() + "\"");
        return NULL;
    }
    QTextStream in(&file);

    // Now we need to determine if this is a Wattbike export
    // or a Racermate export. We can do this by looking at
    // the first line of the file.
    //
    // For it to be a wattbike, the first line is made up
    // of multiple tokens, separated by a tab, which match
    // the pattern "name [units]"
    bool isWattBike = false;
    QString line = in.readLine();
    QStringList tokens = line.split(QRegExp("[\t\n\r]"), QString::SkipEmptyParts);

    if (tokens.count() > 1) {
        // ok, so we have a bunch of tokens, thats a good sign this
        // may be a wattbike export, now are all the tokens of the
        // form "name [units]", or just "Index" or "name []"
        isWattBike=true;
        QRegExp wattToken("^[^[]+ \\[[^]]+\\]$");
        foreach(QString token, tokens) {
            if (wattToken.exactMatch(token) != true) {
                QRegExp noUnits("^[^[]+ \\[\\]$");
                if (token != "Index" && !noUnits.exactMatch(token)) {
                    isWattBike = false;
                }
            }
        }
    }

    // RR text file can include RR interval values in one or two column format.
    // That is, the RR interval values can be given as:
    //    Type 1     Type 2
    //    0.759      0.759 0.759
    //    0.690      1.449 0.690
    //    0.702      2.151 0.702
    // So in the second type of input, the first column includes the time
    // indexes of R wave detections (zero time for the first detection) and
    // second column the RR interval values. The RR interval values above are
    // given in seconds, but millisecond values can also be given.
    enum { RR_None, RR_Type1, RR_Type2 } rrType = RR_None;
    if (!isWattBike) {
        bool ok;
        tokens = line.split(QRegExp("[ \t]"), QString::SkipEmptyParts);
        if (tokens.count() == 1 && tokens[0].toDouble() > 0) rrType = RR_Type1;
        if (tokens.count() == 2 && tokens[0].toDouble(&ok) >= 0 && ok && tokens[1].toDouble() > 0) rrType = RR_Type2;
    }

    if (rrType == RR_None && !isWattBike) {

        // RACERMATE STYLE

        file.close(); // start again (seek did weird things on Linux, bug (?)
        if (!file.open(QFile::ReadOnly)) {
            errors << ("Could not open ride file: \"" + file.fileName() + "\"");
            return NULL;
        }
        QTextStream is(&file);

        // Lets construct our rideFile
        RideFile *rideFile = new RideFile();
        rideFile->setDeviceType("Computrainer/Velotron");
        rideFile->setFileFormat("Computrainer/Velotron text file (txt)");

        // for computrainer / VELOtron we need to convert from the variable rate
        // the file uses to a fixed rate since thats a base assumption across the
        // GC codebase. This parameter can be adjusted to the sample (recIntSecs) rate
        // but in milliseconds
        const int SAMPLERATE = 1000; // we want 1 second samples, re-used below, change to taste

        RideFilePoint sample;        // we reuse this to aggregate all values
        long time = 0L;              // current time accumulates as we run through data
        double lastT = 0.0f;         // last sample time seen in seconds
        double lastK = 0.0f;         // last sample distance seen in kilometers

        while (!is.atEnd()) {

            // the readLine() method doesn't handle old Macintosh CR line endings
            // this workaround will load the entire file if it has CR endings
            // then split and loop through each line
            // otherwise, there will be nothing to split and it will read each line as expected.
            QString linesIn = is.readLine();
            QStringList lines = linesIn.split('\r');
            if(lines.isEmpty()) continue;

            // loop through the lines we got
            foreach (QString line, lines) {

                //
                // SKIP ALL THE GUNK
                //
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
                    milesIndex = headings.indexOf("miles");
                    kphIndex = headings.indexOf("speed");
                    headwindIndex = headings.indexOf("wind");
                    continue;
                }

                // right! we now have a record
                // first replace spaces in quoted values as they break
                // the way we find separators 
                line.replace(QRegExp("\" +"), "\"");

                // now split
                QStringList values = line.split(sepPattern, QString::SkipEmptyParts);

                // mmm... didn't get much data
                if (values.count() < 2) continue;

                //
                // EXTRACT A ROW OF DATA
                //
                RideFilePoint value;
                value.secs = timeIndex > -1 ? values[timeIndex].remove("\"").toDouble() / (double) 1000 : 0.0;
                value.watts = wattsIndex > -1 ? values[wattsIndex].remove("\"").toDouble() : 0.0;
                value.cad = cadIndex > -1 ? values[cadIndex].remove("\"").toDouble() : 0.0;
                value.hr = hrIndex > -1 ? values[hrIndex].remove("\"").toDouble() : 0.0;
                value.km = kmIndex > -1 ? values[kmIndex].remove("\"").toDouble() : 0.0;
                value.kph = kphIndex > -1 ? values[kphIndex].remove("\"").toDouble() : 0.0;
                value.headwind = headwindIndex > -1 ? values[headwindIndex].remove("\"").toDouble() : 0.0;

                double miles = milesIndex > -1 ? values[milesIndex].remove("\"").toDouble() : 0.0;
                if (miles != 0) {
                    // imperial!
                    value.kph *= KM_PER_MILE;
                    value.km = miles * KM_PER_MILE;
                }

                // whats the dt in microseconds
                int dt = (value.secs * 1000) - (lastT * 1000);
                int odt = dt;
                lastT = value.secs;

                // whats the dk in meters
                int dk = (value.km * 1000) - (lastK * 1000);
                lastK = value.km;

                //
                // AGGREGATE INTO SAMPLES
                //
                while (dt > 0) {

                    // we keep track of how much time has been aggregated
                    // into sample, so 'need' is whats left to aggregate 
                    // for the full sample
                    int need = SAMPLERATE - sample.secs;

                    // aggregate
                    if (dt < need) {

                        // the entire sample read is less than we need
                        // so aggregate the whole lot and wait fore more
                        // data to be read. If there is no more data then
                        // this will be lost, we don't keep incomplete samples
                        sample.secs += dt;
                        sample.watts += float(dt) * value.watts;
                        sample.cad += float(dt) * value.cad;
                        sample.hr += float(dt) * value.hr;
                        sample.kph += float(dt) * value.kph;
                        sample.headwind += float(dt) * value.headwind;
                        dt = 0;

                    } else {

                        // dt is more than we need to fill and entire sample
                        // so lets just take the fraction we need
                        dt -= need;

                        // accumulating time and distance
                        sample.secs = time; time += double(SAMPLERATE) / 1000.0f;

                        // subtract remains of this sample from the distance for
                        // the entire sample, remembering that dk is meters and
                        // dt is milliseconds
                        sample.km = lastK - ((float(dt)/(float(odt)) * dk) / 1000.0f);

                        // averaging sample data
                        sample.watts += float(need) * value.watts;
                        sample.cad += float(need) * value.cad;
                        sample.hr += float(need) * value.hr;
                        sample.kph += float(need) * value.kph;
                        sample.headwind += float(need) * value.headwind;
                        sample.watts /= double(SAMPLERATE);
                        sample.cad /= double(SAMPLERATE);
                        sample.hr /= double(SAMPLERATE);
                        sample.kph /= double(SAMPLERATE);
                        sample.headwind /= double(SAMPLERATE);

                        // so now we can add to the ride
                        rideFile->appendPoint(sample.secs, sample.cad, sample.hr, sample.km, 
                                              sample.kph, 0.0, sample.watts, 0.0, 0.0, 0.0, 
                                              sample.headwind, 0.0,
                                              RideFile::NA, RideFile::NA,
                                              0.0,0.0,0.0,0.0,
                                              0.0,0.0,
                                              0.0,0.0,0.0,0.0,
                                              0.0,0.0,0.0,0.0,
                                              0.0,0.0,
                                              0.0,0.0,0.0,0.0,
                                              0);

                        // reset back to zero so we can aggregate
                        // the next sample
                        sample.secs = 0;
                        sample.watts = 0;
                        sample.cad = 0;
                        sample.hr = 0;
                        sample.km = 0;
                        sample.kph = 0;
                        sample.headwind = 0;
                    }
                }
            }
        }
        file.close();

        rideFile->setTag("Device Info", deviceInfo);
        rideFile->setRecIntSecs(double(SAMPLERATE)/1000.0f);


        //
        // Get date time from standard GC name
        //
        QRegExp gcPattern("^.*/(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)\\.txt$");
        gcPattern.setCaseSensitivity(Qt::CaseInsensitive);

        // Racermate uses name-mode-yyyy-mm-dd-hh-mm-ss.CDF.txt
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

    } else if (rrType == RR_None && isWattBike)  {
        // WATTBIKE STYLE 

        // Lets construct our rideFile
        RideFile *rideFile = new RideFile();
        rideFile->setDeviceType("Wattbike");
        rideFile->setFileFormat("Wattbike text file (txt)");
        rideFile->setRecIntSecs(1);

        // We need to work out which column represents 
        // which data series. The heading tokens specify
        // both the series and units, but for now we will
        // assume the units are standardised until we have
        // seen files with different values for units.

        // Here are the known heading tokens:
        //      "Elapsed time total [s]"
        //      "Cadence [1/min]"
        //      "Velocity [km/h]"
        //      "Distance total [m]"
        //      "Heart rate [1/min]"
        //      "Torque per revolution [Nm]"
        //      "Power per revolution [W]"
        //
        // Note: no "spinscan" type data is provided.

        int timeIndex = -1;
        int kmIndex = -1;
        int rpmIndex = -1;
        int kphIndex = -1;
        int bpmIndex = -1;
        int torqIndex = -1;
        int wattsIndex = -1;

        // lets initialise the indexes for all the values
        QRegExp wattToken("^([^[]+) \\[([^]]+)\\]$");
        int i=0;
        int columns = tokens.count();

        foreach(QString token, tokens) {

            if (wattToken.exactMatch(token)) {
                QString name = wattToken.cap(1);
                QString unit = wattToken.cap(2);

                if (name == "Elapsed time total") timeIndex = i;
                if (name == "Distance total") kmIndex = i;
                if (name == "Cadence") rpmIndex = i;
                if (name == "Velocity") kphIndex = i;
                if (name == "Heart rate") bpmIndex = i;
                if (name == "Torque per revolution") torqIndex = i;
                if (name == "Power per revolution") wattsIndex = i;

            }
            i++;
        }

        // lets loop through each row of data adding a sample
        // using the indexes we set above
        double rsecs = 0;
        while (!in.atEnd()) {

            QString line = in.readLine();
            QStringList tokens = line.split(QRegExp("[\r\n\t]"), QString::SkipEmptyParts);

            // do we have as many columns as we expected?
            if (tokens.count() == columns) {

                double secs = 0.00f;
                double km = 0.00f;
                double rpm = 0.00f;
                double kph = 0.00f;
                double bpm = 0.00f;
                double torq = 0.00f;
                double watts = 0.00f;

                if (timeIndex >= 0) {
                    QTime time = QTime::fromString(tokens.at(timeIndex), "mm:ss:00");
                    secs = QTime::fromString("00:00:00", "mm:ss:00").secsTo(time);

                    // its a bit shit, but the format appears to wrap round
                    // on the hour; 59:59:00 is followed by 00:00:00
                    // so we have a problem, since if there are gaps in
                    // recording then we don't know which hour this is for
                    // and if we assume largely contiguous data we may as
                    // well just use a counter instead.
                    //
                    // for expediency, we use a counter for now:
                    secs = rsecs++;
                }
                if (kmIndex >= 0) km = tokens.at(kmIndex).toDouble() / 1000;
                if (rpmIndex >= 0) rpm = tokens.at(rpmIndex).toDouble();
                if (kphIndex >= 0) kph = tokens.at(kphIndex).toDouble();
                if (bpmIndex >= 0) bpm = tokens.at(bpmIndex).toDouble();
                if (torqIndex >= 0) torq = tokens.at(torqIndex).toDouble();
                if (wattsIndex >= 0) watts = tokens.at(wattsIndex).toDouble();

                rideFile->appendPoint(secs, rpm, bpm, km, kph, torq, watts, 0.0, 0.0, 0.0, 0.0, 0.0, RideFile::NA, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0, 0);
            }
        }

        QRegExp gcPattern("^.*/(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)\\.txt$");
        gcPattern.setCaseSensitivity(Qt::CaseInsensitive);

        // Only filename we have seen is name_yyymmdd or name_yyymmdd_all.txt
        QRegExp wb1Pattern("^.*/[^_]*_(\\d\\d\\d\\d)(\\d\\d)(\\d\\d).txt$");
        wb1Pattern.setCaseSensitivity(Qt::CaseInsensitive);
        QRegExp wb2Pattern("^.*/[^_]*_(\\d\\d\\d\\d)(\\d\\d)(\\d\\d)_all.txt$");
        wb2Pattern.setCaseSensitivity(Qt::CaseInsensitive);

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

        // It is an Wattbike Filename
        if (wb1Pattern.exactMatch(file.fileName())) {
            QDateTime datetime(QDate(wb1Pattern.cap(1).toInt(),
                                     wb1Pattern.cap(2).toInt(),
                                     wb1Pattern.cap(3).toInt()),
                               QTime(0, 0, 0));
            rideFile->setStartTime(datetime);
        }
        if (wb2Pattern.exactMatch(file.fileName())) {
            QDateTime datetime(QDate(wb2Pattern.cap(1).toInt(),
                                     wb2Pattern.cap(2).toInt(),
                                     wb2Pattern.cap(3).toInt()),
                               QTime(0, 0, 0));
            rideFile->setStartTime(datetime);
        }

        return rideFile;
    } else {
        // RR File

        // Lets construct our rideFile
        RideFile *rideFile = new RideFile();
        rideFile->setDeviceType("R-R");
        rideFile->setFileFormat("R-R text file (txt)");
        rideFile->setRecIntSecs(1);

        XDataSeries *hrvXdata = new XDataSeries();
        hrvXdata->name = "HRV";
        hrvXdata->valuename << "R-R";
        hrvXdata->unitname << "msecs";

        double secs = 0.0;
        do {
            double rr;

            if (rrType == RR_Type2 && tokens.count() > 1) rr = tokens[1].toDouble();
            else if (rrType == RR_Type1 && tokens.count() > 0) rr = tokens[0].toDouble();
            else continue; // skip blank or incomplete lines

            if (rr > 30) rr /= 1000.0; // convert to seconds if milliseconds

            double bpm = rr>0.0 ? 60.0/rr : 0.0; // HR without filtering

            rideFile->appendPoint(secs, 0.0, bpm, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, RideFile::NA, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0, 0);

            XDataPoint *p = new XDataPoint();
            p->secs = secs;
            p->km = 0;
            p->number[0] = rr * 1000.0;
            hrvXdata->datapoints.append(p);

            secs += rr;

            line = in.readLine();
            tokens = line.split(QRegExp("[ \t]"), QString::SkipEmptyParts);
        } while (!in.atEnd());

        if (hrvXdata->datapoints.count()>0)
            rideFile->addXData("HRV", hrvXdata);
        else
            delete hrvXdata;

        QRegExp gcPattern("^.*/(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)\\.txt$");
        gcPattern.setCaseSensitivity(Qt::CaseInsensitive);

        // Filenames we have seen are yyyy-mm-dd hh-mm-ss.txt
        QRegExp rr1Pattern("^.*/(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d)[ _](\\d\\d)-(\\d\\d)-(\\d\\d)\\.txt$");
        rr1Pattern.setCaseSensitivity(Qt::CaseInsensitive);
        // Filenames we have seen are name_yyyy-mm-dd_hh-mm-ss.txt
        QRegExp rr2Pattern("^.*/[^_]*_[^_]*_(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d)[ _](\\d\\d)-(\\d\\d)-(\\d\\d)\\.txt$");
        rr2Pattern.setCaseSensitivity(Qt::CaseInsensitive);

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

        // It is a R-R Filename
        if (rr1Pattern.exactMatch(file.fileName())) {
            QDateTime datetime(QDate(rr1Pattern.cap(1).toInt(),
                                    rr1Pattern.cap(2).toInt(),
                                    rr1Pattern.cap(3).toInt()),
                               QTime(rr1Pattern.cap(4).toInt(),
                                    rr1Pattern.cap(5).toInt(),
                                    rr1Pattern.cap(6).toInt()));
            rideFile->setStartTime(datetime);
        }
        if (rr2Pattern.exactMatch(file.fileName())) {
            QDateTime datetime(QDate(rr2Pattern.cap(1).toInt(),
                                    rr2Pattern.cap(2).toInt(),
                                    rr2Pattern.cap(3).toInt()),
                               QTime(rr2Pattern.cap(4).toInt(),
                                    rr2Pattern.cap(5).toInt(),
                                    rr2Pattern.cap(6).toInt()));
            rideFile->setStartTime(datetime);
        }

        return rideFile;
    }
}
