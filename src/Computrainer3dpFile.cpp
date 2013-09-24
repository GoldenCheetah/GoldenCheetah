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

#include "math.h"
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

    // computrainer doesn't have a fixed inter-sample-interval; GC
    // expects one, and estimating one by averaging causes problems
    // for some calculations that GC does.  also, computrainer samples
    // so frequently (once every 30-50ms) that the O(n^2) critical
    // power plot calculation takes waaaaay too long.  to solve both
    // problems at once, we smooth the file, emitting an averaged data
    // point every 250 milliseconds.
    //
    // for HR, cadence, watts, and speed, we'll do time averaging to
    // figure out the correct average since the last emitted point.
    // for distance and altitude, we just need to interpolate from the
    // last data point in the computrainer file itself.
    float lastAltitude = 100.0;
    uint32_t lastEmittedMS = 0;
    uint32_t lastSampleMS = 0;
    double hr_sum = 0.0, cad_sum=0.0, speed_sum=0.0, watts_sum=0.0;

#define CT_EMIT_MS 250

    // loop over each sample in the file, do the averaging, interpolation,
    // and emit smoothed points every CT_EMIT_MS milliseconds
    for (; numberSamples; numberSamples--) {

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

        // OK -- we've pulled the next data point out of the ride
        // file.  let's figure out if it's time to emit the next
        // CT_EMIT_MS interval(s).  if so, emit it(them), and reset
        // the averaging sums.
        if (ms == 0) {
          // special case first data point
          rideFile->appendPoint((double) ms/1000, (double) cad,
                                (double) hr, km, speed, 0.0, watts,
                                altitude, 0, 0, 0.0, 0.0, RideFile::noTemp, 0.0, 0);
        }
        // while loop since an interval in the .3dp file might
        // span more than one CT_EMIT_MS interval
        while ((ms - lastEmittedMS) >= CT_EMIT_MS) {
          uint32_t sum_interval_ms;
          float interpol_km, interpol_alt, interpol_fraction;

          // figure out the averaging sum update interval (i.e., time
          // since we last added to the averaging sums).  it's either
          // the time since the last sample from the CT file, or
          // CT_EMIT_MS, depending on whether we've gone through this
          // while loop already, or this is the first loop through.
          if (lastSampleMS > lastEmittedMS)
            sum_interval_ms = (lastEmittedMS + CT_EMIT_MS) - lastSampleMS;
          else
            sum_interval_ms = CT_EMIT_MS;

          // update averaging sums with final bit of this sampling interval
          hr_sum += ((double) hr) * ((double) sum_interval_ms);
          cad_sum += ((double) cad) * ((double) sum_interval_ms);
          speed_sum += ((double) speed) * ((double) sum_interval_ms);
          watts_sum += ((double) watts) * ((double) sum_interval_ms);

          // figure out interpolation points based on time from previous
          // sample from the computrainer file
          interpol_fraction =
            ((float) ((lastEmittedMS + CT_EMIT_MS) - lastSampleMS)) /
            ((float) (ms - lastSampleMS));
          interpol_km = lastKM + (km - lastKM) * interpol_fraction;
          interpol_alt = lastAltitude +
            (altitude - lastAltitude) * interpol_fraction;

          // update last sample emit time
          lastEmittedMS = lastEmittedMS + CT_EMIT_MS;

          // emit averages and interpolated distance/altitude
          rideFile->appendPoint(
                                ((double) lastEmittedMS) / 1000,
                                ((double) cad_sum) / CT_EMIT_MS,
                                ((double) hr_sum) / CT_EMIT_MS,
                                interpol_km,
                                ((double) speed_sum) / CT_EMIT_MS,
                                0.0,
                                ((double) watts_sum) / CT_EMIT_MS,
                                interpol_alt,
                                0, // lon
                                0, // lat
                                0.0, // headwind
                                0.0, // slope
                                RideFile::noTemp, // temp
                                0.0,
                                0);

          // reset averaging sums
          hr_sum = cad_sum = speed_sum = watts_sum = 0.0;
        }

        // update averaging sums with interval to current
        // data point in .3dp file
        if (ms > lastEmittedMS) {
          uint32_t sum_interval_ms;

          if (lastSampleMS > lastEmittedMS)
            sum_interval_ms = ms - lastSampleMS;
          else
            sum_interval_ms = ms - lastEmittedMS;
          hr_sum += ((double) hr) * ((double) sum_interval_ms);
          cad_sum += ((double) cad) * ((double) sum_interval_ms);
          speed_sum += ((double) speed) * ((double) sum_interval_ms);
          watts_sum += ((double) watts) * ((double) sum_interval_ms);
        }

        // stash away distance, altitude, and time at end this
        // interval so can calculate distance traveled over next
        // interval in next loop iteration, and so we can interpolate.
        lastSampleMS = ms;
        lastKM = km;
        lastAltitude = altitude;
    }

    // convert the start time we parsed from the header into
    // what GC wants.
    QDateTime dateTime;
    QDate date;
    QTime time;
    date.setDate(year, month, day);
    time.setHMS(hour, minute, 0, 0);
    dateTime.setDate(date);
    dateTime.setTime(time);
    rideFile->setStartTime(dateTime);

    rideFile->setRecIntSecs(((double) CT_EMIT_MS) / 1000.0);

    // tell GC what kind of device a computrainer is
    rideFile->setDeviceType("Computrainer");
    rideFile->setFileFormat("Computrainer 3DP (3dp)");

    // all done!  close up.
    file.close();
    return rideFile;
}
