/*
 * Copyright (c)  2007 Sean C. Rhea (srhea@srhea.net),
 *                     Justin F. Knotzke (jknotzke@shampoo.ca)
 * Copyright (c)  2009 Greg Lonnon (greg.lonnon@gmail.com)
 *
 * Additional contributions from:
 *     Steve Gribble  (gribble [at] cs.washington.edu)  [December 3, 2009]
 *     Daniel Stark [December 3, 2009]
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

#include <QDateTime>
#include <QDebug>
#include <QRegExp>
#include <QString>
#include <QTextStream>
#include <QVector>

#include <algorithm>            // for std::sort
#include <stdint.h>    // for int8_t, int16_t, etc.
#include <iostream>

#include "cmath"
#include "Units.h"


static int Computrainer3dpFileReaderRegistered =
    RideFileFactory::instance().registerReader("3dp",
                                               "Computrainer 3dp",
                                               new
                                               Computrainer3dpFileReader
                                               ());

RideFile *Computrainer3dpFileReader::openRideFile(QFile & file,
                                                  QStringList & errors,
                                                  QList<RideFile*>*)
    const
{
    // open up the .3dp file, prepare a little-endian-ordered
    // QDataStream
    if (!file.open(QFile::ReadOnly)) {
        errors << ("Could not open ride file: \"" + file.fileName() +
                   "\"");
        return NULL;
    }
    RideFile *rideFile = new RideFile();
    QDataStream is(&file);

    // Note that QT4.6 and above default to 64 bit floats.  For
    // backwards and forwards compatibility, we'll freeze the stream
    // version we use for parsing at 4.0, and then add in LittleEndian
    // number format, which is what Computrainer3dp has.
    is.setVersion(QDataStream::Qt_4_0);
    is.setByteOrder(QDataStream::LittleEndian);

    // start parsing the header

    // looks like the first part is a header... ignore it.
    is.skipRawData(4);

    // the next 4 bytes are the ASCII characters 'perf'
    char perfStr[5];
    is.readRawData(perfStr, 4);
    perfStr[4] = '\0';
    if(strcmp(perfStr,"perf"))
    {
        errors << "File is encrypted.";
        return NULL;
    }


    // not sure what the next 8 bytes are; skip them
    is.skipRawData(0x8);

    // the next 65 bytes are a null-terminated and padded
    // ASCII user name string
    char userName[65];
    is.readRawData(userName, 65);

    // next is a single byte of user age, in years.  I guess
    // Computrainer doesn't allow people to get older than 255
    // years. ;)
    uint8_t age;
    is >> age;

    // not sure what the next 6 bytes are; skip them.
    is.skipRawData(6);

    // next is a (4 byte) C-style floating point with weight in kg
    float weight;
    is >> weight;

    // next is the upper heart rate limit (4 byte int)
    uint32_t upperHR;
    is >> upperHR;

    // and then the resting heart rate (4 byte int)
    uint32_t lowerHR;
    is >> lowerHR;

    // then year, month, day, hour, minute the exercise started
    // (4, 1, 1, 1, 1 bytes)
    uint32_t year;
    is >> year;
    uint8_t month;
    is >> month;
    uint8_t day;
    is >> day;
    uint8_t hour;
    is >> hour;
    uint8_t minute;
    is >> minute;

    // the number of exercise data points in the file (4 byte int)
    uint32_t numberSamples;
    is >> numberSamples;

    // go back to the start, and skip header to go to
    // the start of the data samples.
    file.seek(0);
    is.skipRawData(0xf8);

    // we'll keep track of the altitude over time.  since computrainer
    // gives us slope, we can calculate change in altitude if we know
    // change in distance traveled, so we also need to keep track of
    // the previous sample's distance.
    float altitude = 100.0;  // arbitrary starting altitude of 100m
    float lastKM = 0;

    // computrainer 3d software lets you start your ride partway into
    // a course.  if you do this, then the first distance reported in
    // the corresponding log file will be that offset, rather than
    // zero.  so, we'll stash away the first reported distance, and
    // use that to offset distances that we report to GC so that they
    // are zero-based (i.e., so that the first data point is at
    // distance zero).
    float firstKM = 0;
    bool gotFirstKM = false;

    // for computrainer / VELOtron we need to convert from the variable rate
    // the file uses to a fixed rate since thats a base assumption across the
    // GC codebase. This parameter can be adjusted to the sample (recIntSecs) rate
    // but in milliseconds
    const int SAMPLERATE = 1000; // we want 1 second samples, re-used below, change to taste

    RideFilePoint sample;        // we reuse this to aggregate all values
    long time = 0L;              // current time accumulates as we run through data
    double lastT = 0.0f;         // last sample time seen in seconds
    double lastK = 0.0f;         // last sample distance seen in kilometers

    // loop through samples
    for (; numberSamples; numberSamples--) {

        //
        // READ A SAMPLE FROM FILE - VARIABLE BUT HI-RESOLUTION SAMPLE RATE
        //

        // 1 byte heart rate, in BPM
        uint8_t hr;
        is >> hr;

        // 1 byte cadence, in RPM
        uint8_t cad;
        is >> cad;

        // 2 unsigned bytes of watts
        uint16_t watts;
        is >> watts;

        // 4 bytes of floating point speed (in mph/160 !!)
        float speed;
        is >> speed;
        speed = speed * 160 * KM_PER_MILE;  // convert to kph

        // 4 bytes of total elapsed time, in milliseconds
        uint32_t ms;
        is >> ms;

        // 2 signed bytes of 100 * [percent grade]
        // (i.e., grade == 100 * 100 * rise/run !!)
        int16_t grade;
        is >> grade;

        // not sure what the next 2 bytes are
        is.skipRawData(2);

        // 4 bytes of floating point total distance traveled, in KM
        float km;
        is >> km;
        if (!gotFirstKM) {
          firstKM = km;
          gotFirstKM = true;
        }
        // subtract off the first KM so that distances are zero-based.
        km -= firstKM;

        // calculate change in altitude over the past interval.
        // first, calculate grade measured as rise/run.
        float floatGrade;
        floatGrade = 0.01 * 0.01 * grade;  // floatgrade = rise/run
        // then, convert grade to angle (in radians).
        float angle = atan(floatGrade);
        // calculate distance traveled over past interval
        float delta_distance_meters = (1000.0) * (km - lastKM);
        // change in altitude is:
        //     sin(angle) * (distance traveled in past interval).
        altitude = altitude + delta_distance_meters*sin(angle);

        // not sure what the next 28 bytes are.
        is.skipRawData(0x1c);

        //
        // LETS SAVE OUR SAMPLE
        //
        RideFilePoint value;
        value.secs = ms/1000;
        value.watts = watts;
        value.cad = cad;
        value.hr = hr;
        value.km = km;
        value.kph = speed;
        value.alt = altitude;

        // whats the dt in microseconds
        int dt = (value.secs * 1000) - (lastT * 1000);
        int odt = dt;
        lastT = value.secs;

        // whats the dk in meters
        int dk = (value.km * 1000) - (lastK * 1000);
        lastK = value.km;

        //
        // RESAMPLE INTO SAMPLERATE
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
                sample.watts /= double(SAMPLERATE);
                sample.cad /= double(SAMPLERATE);
                sample.hr /= double(SAMPLERATE);
                sample.kph /= double(SAMPLERATE);

                // so now we can add to the ride
                rideFile->appendPoint(sample.secs, sample.cad, sample.hr, sample.km, 
                                      sample.kph, 0.0, sample.watts, sample.alt, 0.0, 0.0, 
                                      0.0, 0.0, RideFile::NA, RideFile::NA,
                                      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 
                                      0.0, 0.0,0.0,0.0,0.0, 0.0,0.0,0.0,0.0,0.0,0.0, 0);

                // reset back to zero so we can aggregate
                // the next sample
                sample.secs = 0;
                sample.watts = 0;
                sample.cad = 0;
                sample.hr = 0;
                sample.km = 0;
                sample.kph = 0;
                sample.alt = 0;
                sample.headwind = 0;
            }
        }
    }
    file.close();

    // convert the start time we parsed from the header into
    // what GC wants.
    QDateTime dateTime;
    QDate date;
    QTime ridetime;
    date.setDate(year, month, day);
    ridetime.setHMS(hour, minute, 0, 0);
    dateTime.setDate(date);
    dateTime.setTime(ridetime);
    rideFile->setStartTime(dateTime);

    rideFile->setRecIntSecs(((double) SAMPLERATE) / 1000.0);

    // tell GC what kind of device a computrainer is
    rideFile->setDeviceType("Computrainer");
    rideFile->setFileFormat("Computrainer 3DP (3dp)");

    // all done!  close up.
    file.close();
    return rideFile;
}
