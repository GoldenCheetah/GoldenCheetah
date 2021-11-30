/*
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net),
 *                    J.T Conklin (jtc@acorntoolworks.com)
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

#include <QString>
#include <QDebug>

#include "TcxParser.h"
#include "TimeUtils.h"

// use stc strtod to bypass Qt toDouble() issues
#include <stdlib.h>

// TCX XML Structure uses the following 2 Schema Definitions
// -- main schema http://www8.garmin.com/xmlschemas/TrainingCenterDatabasev2.xsd
// -- extension schema http://www8.garmin.com/xmlschemas/ActivityExtensionv2.xsd


TcxParser::TcxParser (RideFile* rideFile, QList<RideFile*> *rides) : rideFile(rideFile), rides(rides)
{
    isGarminSmartRecording = appsettings->value(NULL, GC_GARMIN_SMARTRECORD,Qt::Checked);
    GarminHWM = appsettings->value(NULL, GC_GARMIN_HWMARK);
    if (GarminHWM.isNull() || GarminHWM.toInt() == 0) GarminHWM.setValue(25); // default to 25 seconds.
    first = true;
    creator = false;

}

bool
TcxParser::startElement( const QString&, const QString&, const QString& qName, const QXmlAttributes& qAttributes)
{
    buffer.clear();

    if (qName == "Activity") {

        // First initialisation for altitude (not initialised for each point)
        alt= 0;
        // length-by-length pool swimming XData
        swimXdata = new XDataSeries();
        swimXdata->name = "SWIM";
        swimXdata->valuename << "TYPE";
        swimXdata->valuename << "DURATION";

        lap = 0;
        for (int i = 0; i < ltLast; ++i)
            lapCount[i] = 0;

        if (first == true) first = false;
        else {

            rideFile = new RideFile();
            rideFile->setRecIntSecs(1.0);
            rideFile->setDeviceType("Garmin");
            rideFile->setFileFormat("Garmin Training Centre (tcx)");
        }

        // if caller is looking for rides...
        if (rides) rides->append(rideFile);

        // Sport ("Biking", "Running", "Other")
        swim = NotSwim;
        QString sport = qAttributes.value("Sport");
        if (sport == "Biking") rideFile->setTag("Sport", "Bike");
        else if (sport == "Running") rideFile->setTag("Sport", "Run");
        else if (sport == "Other") swim = MayBeSwim;
        // start of last length for lap swimming
        lastLength = 0.0;

    } else if (qName == "Lap") {
        lap_start_time = convertToLocalTime(qAttributes.value("StartTime").trimmed());
        lapSecs = 0.0;
        lapTrigger = ltManual;

        // Use the time of the first lap as the time of the activity.
        if (lap == 0) {

            start_time = lap_start_time;
            rideFile->setStartTime(start_time);

            last_distance = 0.0;
            last_time = start_time;
        }
        lap++;

    } else if (qName == "Trackpoint") {

        power = 0.0;
        cadence = 0.0;
        rcad = 0.0;
        speed = 0.0;
        headwind = 0.0;
        torque = 0;
        hr = 0.0;
        lat = 0.0;
        lon = 0.0;
        lrbalance = RideFile::NA;
        lte = 0.0;
        rte = 0.0;
        lps = 0.0;
        rps = 0.0;
        badgps = false;
        //alt = 0.0; // TCX from FIT files have not alt point for each trackpoint
        distance = -1;  // nh - we set this to -1 so we can detect if there was a distance in the trackpoint.
        secs = 0;

    } else if (qName == "Creator") {
        creator = true;
    }

    return true;
}

bool
TcxParser::endElement( const QString&, const QString&, const QString& qName)
{
    if (qName == "Time") {
        time = convertToLocalTime(buffer);
        if (start_time == QDateTime()) {

            // redo initizlization on first trackpoint when Lap start time is missing
            start_time = time;
            rideFile->setStartTime(start_time);

            last_distance = 0.0;
            last_time = start_time;
        }
        secs = double(start_time.msecsTo(time)) / 1000.00f;

    } else if (qName == "DistanceMeters") { distance = buffer.toDouble() / 1000; }
    else if (qName == "TotalTimeSeconds") { lapSecs = buffer.toDouble(); }
    else if (qName == "Watts" || qName.endsWith( ":Watts")) { power = buffer.toDouble(); }          //TCX Extension Fields may use a namespace prefix
    else if (qName == "Speed" || qName.endsWith(":Speed")) { speed = buffer.toDouble() * 3.6; }     //TCX Extension Fields may use a namespace prefix
    else if (qName == "RunCadence" || qName.endsWith( ":RunCadence")) { rcad = buffer.toDouble(); } //TCX Extension Fields may use a namespace prefix
    else if (qName == "Value") { hr = buffer.toDouble(); }
    else if (qName == "Cadence") { cadence = buffer.toDouble(); }
    else if (qName == "PedalPower") { lrbalance = buffer.toDouble(); }
    else if (qName == "TorqueEffLeft") { lte = buffer.toDouble(); }
    else if (qName == "TorqueEffRight") { rte = buffer.toDouble(); }
    else if (qName == "PedalSmoothLeft") { lps = buffer.toDouble(); }
    else if (qName == "PedalSmoothRight") { rps = buffer.toDouble(); }
    else if (qName == "AltitudeMeters") {
        // on Suunto TCX files there are lots of 0 values between valid ones, skip these
        if (buffer.toDouble() != 0) {
            alt = buffer.toDouble();
        }
    } else if (qName == "LongitudeDegrees") {

        char *p; 
        setlocale(LC_NUMERIC,"C"); // strtod is locale dependent!
        lon = strtod(buffer.toLatin1(), &p);
        setlocale(LC_NUMERIC,"");

    } else if (qName == "LatitudeDegrees") {
        char *p;
        setlocale(LC_NUMERIC,"C"); // strtod is locale dependent!
        lat = strtod(buffer.toLatin1(), &p);
        setlocale(LC_NUMERIC,"");

    } else if (qName == "Trackpoint") {

        // Some TCX lap swimming files uses distance = 0 for no distance...
        if (swim == Swim && distance == 0) distance = -1;

        // Some TCX files have Speed, some have Distance
        // Lets derive Speed from Distance or vice-versa
        // If we have neither Speed nor Distance then we
        // add a point with 0 for speed and distance
        double sample_dist = distance;
        if (speed == 0 || distance < 0) {

            // compute the elapsed time and distance traveled since the
            // last recorded trackpoint
            double delta_t = double(last_time.msecsTo(time)) / 1000.0f;

            // Derive speed from distance
            if (speed == 0 && distance >0) {

                double delta_d = distance - last_distance;
                if (delta_d<0) delta_d=0;
                if (delta_t > 0.0) speed=delta_d / delta_t * 3600.0;

            } else if (distance < 0) { // otherwise derive distance from speed

                double delta_d = delta_t * speed / 3600.0;
                distance = last_distance + delta_d;

            }
        }

        // Record trackpoint

        if (lat == 0 && lon == 0) badgps = true;

        // If sport was Other and we have distance but no GPS data
        // we assume it is a pool swimming activity and first
        // distance is pool length
        if (swim == MayBeSwim && badgps && distance > 0) {
            swim = Swim;
            rideFile->setTag("Sport", "Swim");
            rideFile->setTag("Pool Length", // in meters
                             QString("%1").arg(distance*1000.0));
        }

        // for smart recording, the delta_t will not be constant
        // average all the calculations based on the previous
        // point.

        if(rideFile->dataPoints().empty()) {

            // first point
            rideFile->appendPoint(secs, cadence, hr, distance, speed, torque,
                                  power, alt, lon, lat, headwind, 0.0,
                                  RideFile::NA, lrbalance,
                                  lte,rte,lps,rps,
                                  0.0,0.0,
                                  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                  0.0,rcad,0.0, // no running dynamics in the schema ?
                                  0.0, //tcore
                                  lap);

        } else {

            // assumption that the change in ride is linear...  :)
            RideFilePoint *prevPoint = rideFile->dataPoints().back();
            double deltaSecs = secs - prevPoint->secs;
            double deltaCad = cadence - prevPoint->cad;
            double deltaHr = hr - prevPoint->hr;
            double deltaDist = distance - prevPoint->km;
            double deltaSpeed = speed - prevPoint->kph;
            double deltaTorque = torque - prevPoint->nm;
            double deltaPower = power - prevPoint->watts;
            double deltaAlt = alt - prevPoint->alt;
            double deltaLon = lon - prevPoint->lon;
            double deltaLat = lat - prevPoint->lat;
            double deltarcad = rcad - prevPoint->rcad;
            double deltaLrbalance = lrbalance - prevPoint->lrbalance;
            double deltaLte = lte - prevPoint->lte;
            double deltaRte = rte - prevPoint->rte;
            double deltaLps = lps - prevPoint->lps;
            double deltaRps = rps - prevPoint->rps;

            if (prevPoint->lat == 0 && prevPoint->lon == 0) badgps = true;
            // Smart Recording High Water Mark.
            if ((isGarminSmartRecording.toInt() == 0) || (deltaSecs == 1) || (deltaSecs >= GarminHWM.toInt() && swim != Swim)) {

                // no smart recording, or delta exceeds HW treshold, just insert the data
                rideFile->appendPoint(secs, cadence, hr, distance, speed, torque, power,
                                      alt, lon, lat, headwind, 0.0, RideFile::NA, lrbalance,
                                      lte,rte,lps,rps,
                                      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                      0.0,0.0,
                                      0.0, // vertical oscillation
                                      rcad, // run cadence
                                      0.0, // gct
                                      0.0, // tcore
                                      lap);

                // Update distance and speed for swimming lengths
                if (swim == Swim && sample_dist > 0.0 && secs > lastLength) {
                    double deltaSecs = secs - lastLength;
                    double deltaDist = (distance - last_distance) / deltaSecs;
                    double kph = 3600.0 * deltaDist;
                    // length-by-length Swim XData
                    XDataPoint *p = new XDataPoint();
                    p->secs = lastLength;
                    p->km = last_distance;
                    p->number[0] = deltaDist > 0 ? 1 : 0;
                    p->number[1] = deltaSecs;
                    if (swimXdata) swimXdata->datapoints.append(p);

                    for (int i = rideFile->timeIndex(lastLength);
                         i>= 0 && i < rideFile->dataPoints().size() &&
                         rideFile->dataPoints()[i]->secs <= secs;
                         ++i) {
                        rideFile->dataPoints()[i]->kph = kph;
                        rideFile->dataPoints()[i]->km = last_distance;
                        last_distance += deltaDist;
                    }
                    last_distance = distance;
                    if (kph > 0.0) rideFile->setDataPresent(rideFile->kph, true);
                    lastLength = secs;
                }

            } else {

                // smart recording is on and delta is less than GarminHWM seconds
                // length-by-length Swim XData
                if (swim == Swim && deltaSecs > 0) {
                    XDataPoint *p = new XDataPoint();
                    p->secs = prevPoint->secs;
                    p->km = last_distance;
                    p->number[0] = deltaDist > 0 ? 1 : 0;
                    p->number[1] = deltaSecs;
                    if (swimXdata) swimXdata->datapoints.append(p);
                    lastLength = p->secs + deltaSecs;
                }
                // or it is pool swimming and we limit expansion for safety
                for(int i = 1; i <= deltaSecs && i <= 300*GarminHWM.toInt(); i++) {
                    double weight = i/ deltaSecs;
                    double kph = (swim == Swim) ? speed : prevPoint->kph + (deltaSpeed *weight);
                    // need to make sure speed goes to zero
                    kph = kph > 0.35 ? kph : 0;
                    //double cad = prevPoint->cad + (deltaCad * weight);
                    //cad = cad > 0.35 ? cad : 0;
                    //double lat = prevPoint->lat + (deltaLat * weight);
                    //double lon = prevPoint->lon + (deltaLon * weight);

                    rideFile->appendPoint(prevPoint->secs + (deltaSecs * weight),
                                          prevPoint->cad  + (deltaCad * weight),
                                          prevPoint->hr +   (deltaHr * weight),
                                          prevPoint->km + (deltaDist * weight),
                                          kph,
                                          prevPoint->nm + (deltaTorque * weight),
                                          prevPoint->watts + (deltaPower * weight),
                                          prevPoint->alt + (deltaAlt * weight),
                                          badgps ? 0 : prevPoint->lon + (deltaLon * weight), // lon
                                          badgps ? 0 : prevPoint->lat + (deltaLat * weight), // lat
                                          headwind, // headwind
                                          0.0, // slope
                                          RideFile::NA,
                                          prevPoint->lrbalance + (deltaLrbalance * weight),
                                          prevPoint->lte + (deltaLte * weight),
                                          prevPoint->rte + (deltaRte * weight),
                                          prevPoint->lps + (deltaLps * weight),
                                          prevPoint->rps + (deltaRps * weight),
                                          0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                          0.0,0.0,
                                          0.0, // vertical oscillation
                                          prevPoint->rcad + (deltarcad * weight),// run cadence
                                          0.0, // gct
                                          0.0, //tcore
                                          lap);
                }
                prevPoint = rideFile->dataPoints().back();
            }
        }
        last_distance = distance;
        last_time = time;
    } else if (qName == "TriggerMethod") {
        // see "TriggerMethod_t" in Garmin's Training Center Database XML (TCX) Schema
        if (buffer == "Distance")
            lapTrigger = ltDistance;
        else if (buffer == "Location")
            lapTrigger = ltLocation;
        else if (buffer == "Time")
            lapTrigger = ltTime;
        else if (buffer == "HeartRate")
            lapTrigger = ltHeartRate;
    } else if (qName == "Lap") {
        // for pool swimming, laps with distance 0 are pauses, without trackpoints
        // length-by-length Swim XData
        if (swim == Swim && distance == 0.0) {
            XDataPoint *p = new XDataPoint();
            p->secs = secs;
            p->km = last_distance;
            p->number[0] = 0;
            p->number[1] = round(lapSecs);
            if (swimXdata) swimXdata->datapoints.append(p);
            lastLength = secs + round(lapSecs);
        }
        // expand even if Smart Recording is not enabled
        if (swim == Swim && distance == 0) {
            // fill in the pause, partially if too long
            for(int i = 1; i <= round(lapSecs) && i <= 300*GarminHWM.toInt(); i++)
                rideFile->appendPoint(secs + i,
                                      0.0,
                                      0.0,
                                      last_distance,
                                      0.0,
                                      0.0,
                                      0.0,
                                      0.0,
                                      0.0, // lon
                                      0.0, // lat
                                      0.0, // headwind
                                      0.0,
                                      RideFile::NA,
                                      0.0, // rlbalance
                                      0.0,0.0,0.0,0.0, // lte, rte, lps, rps
                                      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                      0.0,0.0,
                                      0.0, // vertical oscillation
                                      0.0, // run cadence
                                      0.0, // gct
                                      0.0, // tcore
                                      lap);
            last_time = last_time.addSecs(round(lapSecs));
        }

        QString name;
        switch (lapTrigger) {
        case ltDistance:
            name = QObject::tr("Distance %1").arg(++lapCount[lapTrigger]);
            break;
        case ltLocation:
            name = QObject::tr("Location %1").arg(++lapCount[lapTrigger]);
            break;
        case ltTime:
            name = QObject::tr("Time %1").arg(++lapCount[lapTrigger]);
            break;
        case ltHeartRate:
            name = QObject::tr("HeartRate %1").arg(++lapCount[lapTrigger]);
            break;
        default:
            name = QObject::tr("Lap %1").arg(++lapCount[ltManual]);
            break;
        }

        double start = double(start_time.msecsTo(lap_start_time)) / 1000.00f;
        rideFile->addInterval(RideFileInterval::DEVICE, start, start + lapSecs, name);
    } else if (qName == "Activity") {
        // Add length-by-length Swim XData, if present
        if (swimXdata && swimXdata->datapoints.count()>0) {
            rideFile->addXData("SWIM", swimXdata);
        } else if (swimXdata) {
            delete swimXdata;
            swimXdata = NULL;
        }
    } else if (qName == "Notes") {
        // Add Notes to metadata
        rideFile->setTag("Notes", buffer);
    } else if (qName == "Creator") {
        creator = false;
    } else if (creator && qName == "Name") {
        if (!buffer.isEmpty())
            rideFile->setDeviceType(buffer);
    }
    return true;
}

bool TcxParser::characters( const QString& str )
{
    buffer += str;
    return true;
}
