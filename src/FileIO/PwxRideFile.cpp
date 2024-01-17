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

#include "PwxRideFile.h"
#include "Athlete.h"
#include "Settings.h"
#include <QDomDocument>
#include <QVector>

#include <QDebug>

static int pwxFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "pwx", "TrainingPeaks PWX", new PwxFileReader());

RideFile *
PwxFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    QDomDocument doc("TrainingPeaks PWX");
    if (!file.open(QIODevice::ReadOnly)) {
        errors << "Could not open file.";
        return NULL;
    }

    bool parsed = bool(doc.setContent(&file));
    file.close();
    if (!parsed) {
        errors << "Could not parse file.";
        return NULL;
    }

    return PwxFromDomDoc(doc, errors);
}

RideFile *
PwxFileReader::PwxFromDomDoc(QDomDocument doc, QStringList&) const
{
    RideFile *rideFile = new RideFile();
    QDomElement root = doc.documentElement();
    QDomNode workout = root.firstChildElement("workout");
    QDomNode node = workout.firstChild();

    // get the Smart Recording parameters
    QVariant isGarminSmartRecording = appsettings->value(NULL, GC_GARMIN_SMARTRECORD,Qt::Checked);
    QVariant GarminHWM = appsettings->value(NULL, GC_GARMIN_HWMARK);
    if (GarminHWM.isNull() || GarminHWM.toInt() == 0) GarminHWM.setValue(25); // default to 25 seconds.

    // can arrive at any time, so lets cache them
    // and sort out at the end
    QDateTime rideDate;

    // we collect summary data but discard it for all
    // bar manual ride files where this is all we are 
    // gonna get !
    double manualDuration = 0.00f;
    double manualWork = 0.00f;
    double manualTSS = 0.00f;
    double manualHR = 0.00f;
    double manualSpeed = 0.00f;
    double manualPower = 0.00f;
    double manualKM = 0.00f;
    double manualElevation = 0.00f;

    int intervals = 0;
    int samples = 0;

    // in case we need to calculate distance
    double rtime = 0;
    double rdist = 0;

    // length-by-length Swim XData
    XDataSeries *swimXdata = new XDataSeries();
    swimXdata->name = "SWIM";
    swimXdata->valuename << "TYPE";
    swimXdata->valuename << "DURATION";
    swimXdata->valuename << "STROKES";

    while (!node.isNull()) {

        // athlete
        if (node.nodeName() == "athlete") {

            QDomElement name = node.firstChildElement("name");
            if (!name.isNull()) {
                rideFile->setTag("Athlete Name", name.text());
            }

            QDomElement weight = node.firstChildElement("weight");
            if (!weight.isNull()) {
                rideFile->setTag("Weight", weight.text());
            }

        // workout code
        } else if (node.nodeName() == "code") {

            QDomElement code = node.toElement();
            rideFile->setTag("Workout Code", code.text());

        // workout title
        } else if (node.nodeName() == "title") {

            QDomElement title = node.toElement();
            rideFile->setTag("Workout Title", title.text());

        // goal / objective
        } else if (node.nodeName() == "goal") {

            QDomElement goal = node.toElement();
            rideFile->setTag("Objective", goal.text());

        // sport
        } else if (node.nodeName() == "sportType") {

            QDomElement sport = node.toElement();
            rideFile->setTag("Sport", sport.text());

        // notes
        } else if (node.nodeName() == "cmt") {

            // Add the PWX cmt tag as notes
            QDomElement notes = node.toElement();
            rideFile->setTag("Notes", notes.text());

        // device type and info
        } else if (node.nodeName() == "device") {

            QString devicetype;
            // make and model
            QDomElement make = node.firstChildElement("make");
            if (!make.isNull()) devicetype = make.text();
            QDomElement model = node.firstChildElement("model");
            if (!model.isNull()) {
                if (devicetype != "") devicetype += " ";
                devicetype += model.text();
            }
            rideFile->setDeviceType(devicetype);
            rideFile->setFileFormat("Peaksware Data File (pwx)");

            // device settings data
            QString deviceinfo;
            QDomElement extension = node.firstChildElement("extension");
            if (!extension.isNull()) {
                for (QDomElement info=extension.firstChildElement();
                    !info.isNull();
                    info = info.nextSiblingElement()) {
                    deviceinfo += info.tagName();
                    deviceinfo += ": ";
                    deviceinfo += info.text();
                    deviceinfo += '\n';
                }
            }
            rideFile->setTag("Device Info", deviceinfo);

        // start date/time
        } else if (node.nodeName() == "time") {
            QDomElement date = node.toElement();
            rideDate = QDateTime::fromString(date.text(), Qt::ISODate);
            rideFile->setStartTime(rideDate);

        // interval data
        } else if (node.nodeName() == "segment") {
            RideFileInterval add;

            // name
            QDomElement name = node.firstChildElement("name");
            if (!name.isNull()) add.name = name.text();
            else add.name = QString("Interval #%1").arg(++intervals);

            QDomElement summary = node.firstChildElement("summarydata");
            if (!summary.isNull()) {

                // start
                QDomElement beginning = summary.firstChildElement("beginning");
                if (!beginning.isNull()) add.start = beginning.text().toDouble();
                else add.start = -1;

                // duration - convert to end
                QDomElement duration = summary.firstChildElement("duration");
                if (!duration.isNull() && add.start != -1)
                    add.stop = duration.text().toDouble() + add.start;
                else
                    add.stop = -1;

                // add interval
                if (add.start != -1 && add.stop != -1) {
                    rideFile->addInterval(RideFileInterval::DEVICE, round(add.start+1), round(add.stop+1), add.name);
                }
            }

        // data points: offset, hr, spd, pwr, torq, cad, dist, lat, lon, alt, temp
        } else if (node.nodeName() == "sample") {
            RideFilePoint add;

            // offset (secs)
            QDomElement off = node.firstChildElement("timeoffset");
            if (!off.isNull()) add.secs = round(off.text().toDouble());
            else add.secs = 0.0;
            // hr
            QDomElement hr = node.firstChildElement("hr");
            if (!hr.isNull()) add.hr = hr.text().toDouble();
            else add.hr = 0.0;
            // spd in meters per second converted to kph
            QDomElement spd = node.firstChildElement("spd");
            if (!spd.isNull()) add.kph = spd.text().toDouble() * 3.6;
            else add.kph = 0.0;
            // pwr
            QDomElement pwr = node.firstChildElement("pwr");
            if (!pwr.isNull()) {
                add.watts = pwr.text().toDouble();
                // NOTE! undo the fudge to set zero values to
                //       1 in the writer (below). This is to keep
                //       the TP upload web-service happy with zero values
                if (add.watts == 1) add.watts = 0.0;
            } else add.watts = 0.0;
            // lrbalance (pwrright)
            QDomElement lrbalance = node.firstChildElement("pwrright");
            if (!lrbalance.isNull()) {
                if (add.watts == 0) {
                   add.lrbalance = 50.0;
                } else {
                    add.lrbalance =(add.watts-lrbalance.text().toDouble())/add.watts*100.0;
                }
            } else add.lrbalance = RideFile::NA;
            // torq
            QDomElement torq = node.firstChildElement("torq");
            if (!torq.isNull()) add.nm = torq.text().toDouble();
            else add.nm = 0.0;
            // cad
            QDomElement cad = node.firstChildElement("cad");
            if (!cad.isNull()) add.cad = cad.text().toDouble();
            else add.cad = 0.0;
            // dist
            QDomElement dist = node.firstChildElement("dist");
            if (!dist.isNull()) add.km = dist.text().toDouble() /1000;
            else add.km = 0.0;

            // lat
            QDomElement lat = node.firstChildElement("lat");
            if (!lat.isNull()) add.lat = lat.text().toDouble();
            else add.lat = 0.0;
            // lon
            QDomElement lon = node.firstChildElement("lon");
            if (!lon.isNull()) add.lon = lon.text().toDouble();
            else add.lon = 0.0;
            // alt
            QDomElement alt = node.firstChildElement("alt");
            if (!alt.isNull()) add.alt = alt.text().toDouble();
            else add.alt = 0.0;
            // temp
            QDomElement temp = node.firstChildElement("temp");
            if (!temp.isNull()) add.temp = temp.text().toDouble();
            else add.temp = RideFile::NA;

            // torque_effectiveness_left
            QDomElement lte = node.firstChildElement("torque_effectiveness_left");
            if (!lte.isNull()) add.lte = lte.text().toDouble();
            else add.lte = 0.0;
            // torque_effectiveness_right
            QDomElement rte = node.firstChildElement("torque_effectiveness_right");
            if (!rte.isNull()) add.rte = rte.text().toDouble();
            else add.rte = 0.0;
            // pedal_smoothness_left
            QDomElement lps = node.firstChildElement("pedal_smoothness_left");
            if (!lps.isNull()) add.lps = lps.text().toDouble();
            else add.lps = 0.0;
            // pedal_smoothness_right
            QDomElement rps = node.firstChildElement("pedal_smoothness_right");
            if (!rps.isNull()) add.rps = rps.text().toDouble();
            else add.rps = 0.0;

            // if there are data points && a time difference > 1sec && smartRecording processing is requested at all
            if ((!rideFile->dataPoints().empty()) && (add.secs > rtime + 1) && (isGarminSmartRecording.toInt() != 0)) {
                bool badgps = false;
                bool lapSwim = false;
                // Handle smart recording if configured in preferences.  Linearly interpolate missing points.
                RideFilePoint *prevPoint = rideFile->dataPoints().back();
                double deltaSecs = add.secs - prevPoint->secs;

                // If the last lat/lng was missing (0/0) then all points up to lat/lng are marked as 0/0.
                if (prevPoint->lat == 0 && prevPoint->lon == 0 ) badgps = true;

                double deltaCad = add.cad - prevPoint->cad;
                double deltaHr = add.hr - prevPoint->hr;
                double deltaDist = add.km - prevPoint->km;
                if (add.km < 0.00001) deltaDist = 0.000f; // effectively zero distance
                double deltaSpeed = add.kph - prevPoint->kph;
                double deltaTorque = add.nm - prevPoint->nm;
                double deltaPower = add.watts - prevPoint->watts;
                double deltaAlt = add.alt - prevPoint->alt;
                double deltaLon = add.lon - prevPoint->lon;
                double deltaLat = add.lat - prevPoint->lat;
                double deltaHeadwind = add.headwind - prevPoint->headwind;
                double deltaSlope = add.slope - prevPoint->slope;
                double deltaLeftRightBalance = add.lrbalance - prevPoint->lrbalance;
                double deltaLeftTE = add.lte - prevPoint->lte;
                double deltaRightTE = add.rte - prevPoint->rte;
                double deltaLeftPS = add.lps - prevPoint->lps;
                double deltaRightPS = add.rps - prevPoint->rps;
                double deltaLeftPedalCenterOffset = add.lpco - prevPoint->lpco;
                double deltaRightPedalCenterOffset = add.rpco - prevPoint->rpco;
                double deltaLeftTopDeathCenter = add.lppb - prevPoint->lppb;
                double deltaRightTopDeathCenter = add.rppb - prevPoint->rppb;
                double deltaLeftBottomDeathCenter = add.lppe - prevPoint->lppe;
                double deltaRightBottomDeathCenter = add.rppe - prevPoint->rppe;
                double deltaLeftTopPeakPowerPhase = add.lpppb - prevPoint->lpppb;
                double deltaRightTopPeakPowerPhase = add.rpppb - prevPoint->rpppb;
                double deltaLeftBottomPeakPowerPhase = add.lpppe - prevPoint->lpppe;
                double deltaRightBottomPeakPowerPhase = add.rpppe - prevPoint->rpppe;
                double deltaSmO2 = add.smo2 - prevPoint->smo2;
                double deltaTHb = add.thb - prevPoint->thb;
                double deltarvert = add.rvert - prevPoint->rvert;
                double deltarcad = add.rcad - prevPoint->rcad;
                double deltarcontact = add.rcontact - prevPoint->rcontact;

                // Swim with distance and no GPS => pool swim
                // limited to account for weird intervals or pauses
                if (rideFile->isSwim() && badgps && (add.km > 0 || rdist > 0)) {
                    lapSwim = true;
                    if (rdist == 0.0) // first length used to set Pool Length
                        rideFile->setTag("Pool Length", // in meters
                                         QString("%1").arg(add.km*1000.0));
                    add.kph = add.km > rdist ? (add.km - rdist)*3600/deltaSecs : 0.0;
                    if (add.kph == 0.0) add.cad = 0; // rest => no stroke rate
                }
                // length-by-length Swim XData
                if (lapSwim == true) {
                    XDataPoint *p = new XDataPoint();
                    p->secs = rtime;
                    p->km = rdist;
                    p->number[0] = (add.km > rdist) ? 1 : 0;
                    p->number[1] = deltaSecs;
                    p->number[2] = round(add.cad * deltaSecs / 60.0);
                    swimXdata->datapoints.append(p);
                }

                // only smooth the maximal smart recording gap defined in
                // preferences - we don't want to crash / stall on bad
                // or corrupt files, lap swimming lenghts/pauses limited
                // to 10x HWM for the same reason.
                if (deltaSecs > 0 && (deltaSecs < GarminHWM.toInt() || (lapSwim && deltaSecs < 10*GarminHWM.toInt()))) {

                    for (int i = 1; i < deltaSecs; i++) {
                        double weight = i /deltaSecs;
                        // running totals
                        samples++;
                        rtime++;
                        rdist = prevPoint->km + (deltaDist * weight);
                        // add the data point
                        rideFile->appendPoint(
                            rtime,
                            lapSwim ? add.cad : prevPoint->cad + (deltaCad * weight),
                            prevPoint->hr + (deltaHr * weight),
                            rdist,
                            lapSwim ? add.kph : prevPoint->kph + (deltaSpeed * weight),
                            prevPoint->nm + (deltaTorque * weight),
                            prevPoint->watts + (deltaPower * weight),
                            prevPoint->alt + (deltaAlt * weight),
                            (badgps == 1) ? 0 : prevPoint->lon + (deltaLon * weight),
                            (badgps == 1) ? 0 : prevPoint->lat + (deltaLat * weight),
                            prevPoint->headwind + (deltaHeadwind * weight),
                            prevPoint->slope + (deltaSlope * weight),
                            add.temp,
                            prevPoint->lrbalance + (deltaLeftRightBalance * weight),
                            prevPoint->lte + (deltaLeftTE * weight),
                            prevPoint->rte + (deltaRightTE * weight),
                            prevPoint->lps + (deltaLeftPS * weight),
                            prevPoint->rps + (deltaRightPS * weight),
                            prevPoint->lpco + (deltaLeftPedalCenterOffset * weight),
                            prevPoint->rpco + (deltaRightPedalCenterOffset * weight),
                            prevPoint->lppb + (deltaLeftTopDeathCenter * weight),
                            prevPoint->rppb + (deltaRightTopDeathCenter * weight),
                            prevPoint->lppe + (deltaLeftBottomDeathCenter * weight),
                            prevPoint->rppe + (deltaRightBottomDeathCenter * weight),
                            prevPoint->lpppb + (deltaLeftTopPeakPowerPhase * weight),
                            prevPoint->rpppb + (deltaRightTopPeakPowerPhase * weight),
                            prevPoint->lpppe + (deltaLeftBottomPeakPowerPhase * weight),
                            prevPoint->rpppe + (deltaRightBottomPeakPowerPhase * weight),
                            prevPoint->smo2 + (deltaSmO2 * weight),
                            prevPoint->thb + (deltaTHb * weight),
                            prevPoint->rvert + (deltarvert * weight),
                            prevPoint->rcad + (deltarcad * weight),
                            prevPoint->rcontact + (deltarcontact * weight),
                            0.0,
                            add.interval);
                    }
                }
            } else if (add.km == 0.0 && samples) {
                // do we need to calculate distance?
                // delta secs * kph/3600
                add.km = rdist + ((add.secs - rtime) * (add.kph/3600));
            }

            // add the data point avoiding duplicates
            if (add.secs > rtime || rideFile->dataPoints().empty()) {
                if (add.secs == 0.0) add.kph = 0.0; // avoids a glitch in km
                // running totals
                samples++;
                rtime = add.secs;
                rdist = add.km;
                rideFile->appendPoint(add.secs, add.cad, add.hr, add.km, add.kph,
                    add.nm, add.watts, add.alt, add.lon, add.lat, add.headwind,
                    add.slope, add.temp, add.lrbalance,
                    add.lte, add.rte, add.lps, add.rps,
                    add.lpco, add.rpco,
                    add.lppb, add.rppb, add.lppe, add.rppe,
                    add.lpppb, add.rpppb, add.lpppe, add.rpppe,
                    add.smo2, add.thb,
                    add.rvert, add.rcad, add.rcontact,
                    0.0, //tcore
                    add.interval);
            }
        
        } else if (node.nodeName() == "summarydata") {

            // get the summary data in case there are no samples
            // this is when there is a manual entry, so we can
            // set the overrides from this

            //<summarydata xmlns="http://www.peaksware.com/PWX/1/0">
            //<duration>600</duration>
            //<work>514.632000296428</work>
            //<tss>100</tss>
            //<hr></hr>
            //<spd></spd>
            //<pwr></pwr>
            //<dist>23000</dist>
            //<climbingelevation>14</climbingelevation>
            //</summarydata>

            // duration
            QDomElement off = node.firstChildElement("duration");
            if (!off.isNull()) manualDuration = off.text().toDouble();

            // work
            off = node.firstChildElement("work");
            if (!off.isNull()) manualWork = off.text().toDouble();

            // tss
            off = node.firstChildElement("tss");
            if (!off.isNull()) manualTSS = off.text().toDouble();

            // hr
            off = node.firstChildElement("hr");
            if (!off.isNull()) manualHR = off.text().toDouble();

            // speed
            off = node.firstChildElement("spd");
            if (!off.isNull()) manualSpeed = off.text().toDouble();

            // power
            off = node.firstChildElement("pwr");
            if (!off.isNull()) manualPower = off.text().toDouble();

            // distance
            off = node.firstChildElement("dist");
            if (!off.isNull()) manualKM = off.text().toDouble();

            // Elevation
            off = node.firstChildElement("climbingelevation");
            if (!off.isNull()) manualElevation = off.text().toDouble();


        } else if (node.nodeName() == "extension") {
        }

        node = node.nextSibling();
    }

    // post-process and check
    if (samples < 2) {

        // set to 1s, it really doesn't matter!
        rideFile->setRecIntSecs(1.0f);

        // we're creating a manual ride file so
        // set the overrides from the supplied summarydata

        // distance
        if (manualKM) {
            QMap<QString,QString> override;
            override.insert("value", QString("%1").arg(manualKM / 1000)); // its in meters
            rideFile->metricOverrides.insert("total_distance", override);
        }

        // duration
        if (manualDuration) {
            QMap<QString,QString> override;
            override.insert("value", QString("%1").arg(manualDuration));
            rideFile->metricOverrides.insert("workout_time", override);
            rideFile->metricOverrides.insert("time_riding", override);
        }

        // work
        if (manualWork) {
            QMap<QString,QString> override;
            override.insert("value", QString("%1").arg(manualWork));
            rideFile->metricOverrides.insert("total_work", override);
        }

        // BikeStress
        if (manualTSS) {
            QMap<QString,QString> override;
            override.insert("value", QString("%1").arg(manualTSS));
            rideFile->metricOverrides.insert("coggan_tss", override);
        }

        // HR
        if (manualHR) {
            QMap<QString,QString> override;
            override.insert("value", QString("%1").arg(manualHR));
            rideFile->metricOverrides.insert("average_hr", override);
        }

        // Speed
        if (manualSpeed) {
            QMap<QString,QString> override;
            override.insert("value", QString("%1").arg(manualSpeed));
            rideFile->metricOverrides.insert("average_speed", override);
        }

        // Power
        if (manualPower) {
            QMap<QString,QString> override;
            override.insert("value", QString("%1").arg(manualPower));
            rideFile->metricOverrides.insert("average_power", override);
        }

        // Elevation Gain
        if (manualElevation) {
            QMap<QString,QString> override;
            override.insert("value", QString("%1").arg(manualElevation));
            rideFile->metricOverrides.insert("elevation_gain", override);
        }

    } else {

        // need to determine the recIntSecs - first - second sample?
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
        } else {
            // zero or one sample just make it a second
            rideFile->setRecIntSecs(1);
        }


        // if its a daft number then make it 1s -- there is probably
        // a gap in recording in there.
        switch ((int)rideFile->recIntSecs()) {
            case 1 : // lots!
            case 2 : // Timex
            case 4 : // garmin smart recording
            case 5 : // polar sometimes
            case 10 : // polar and others
            case 15 :
                break;

            default:
                rideFile->setRecIntSecs(1);
                break;
        }
    }

    // Add length-by-length Swim XData, if present
    if (swimXdata->datapoints.count()>0)
        rideFile->addXData("SWIM", swimXdata);
    else
        delete swimXdata;

    return rideFile;
}

bool
PwxFileReader::writeRideFile(Context *context, const RideFile *ride, QFile &file) const
{
    QDomText text; // used all over
    QDomDocument doc;
    QDomProcessingInstruction hdr = doc.createProcessingInstruction("xml","version=\"1.0\"");
    doc.appendChild(hdr);

    // pwx
    QDomElement pwx = doc.createElementNS("http://www.peaksware.com/PWX/1/0", "pwx");
    pwx.setAttribute("creator", "Golden Cheetah");
    pwx.setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    pwx.setAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema");
    pwx.setAttribute("xsi:schemaLocation", "http://www.peaksware.com/PWX/1/0 http://www.peaksware.com/PWX/1/0/pwx.xsd");
    pwx.setAttribute("version", "1.0");
    doc.appendChild(pwx);

    // workouts... we just serialise 1 at a time
    QDomElement root = doc.createElement("workout");
    pwx.appendChild(root);

    // athlete details
    QDomElement athlete = doc.createElement("athlete");
    QDomElement name = doc.createElement("name");
    text = doc.createTextNode(context ? context->athlete->cyclist : "athlete"); name.appendChild(text);
    athlete.appendChild(name);
    double cyclistweight = ride->getTag("Weight", "0.0").toDouble();
    if (cyclistweight) {
        QDomElement weight = doc.createElement("weight");
        text = doc.createTextNode(QString("%1").arg(cyclistweight));
        weight.appendChild(text);
        athlete.appendChild(weight);
    }
    root.appendChild(athlete);

    // sport
    QString sport = ride->getTag("Sport", "Bike");
    if (sport == QObject::tr("Biking") || sport == QObject::tr("Cycling") || sport == QObject::tr("Cycle") || sport == QObject::tr("Bike")) {
        sport = "Bike";
    }
    QDomElement sportType = doc.createElement("sportType");
    text = doc.createTextNode(sport); 
    sportType.appendChild(text);
    root.appendChild(sportType);

    // notes
    if (ride->getTag("Notes","") != "") {
        QDomElement notes = doc.createElement("cmt");
        text = doc.createTextNode(ride->getTag("Notes","")); notes.appendChild(text);
        root.appendChild(notes);
    }
    
    
    // workout code
    if (ride->getTag("Workout Code", "") != "") {
        QString wcode = ride->getTag("Workout Code", "");
        QDomElement code = doc.createElement("code");
        text = doc.createTextNode(wcode); code.appendChild(text);
        root.appendChild(code);
    }

    // workout title
    QString wtitle;
    if (ride->getTag("Workout Title", "") != "") {
        wtitle = ride->getTag("Workout Title", "");
    } else {

        // We try metadata fields; Title, then Name then Route then Workout Code

        // is "Title" set?
        if (!ride->getTag("Title", "").isEmpty()) {
            wtitle = ride->getTag("Title", "");
        } else {

            // is "Name" set?
            if (!ride->getTag("Name", "").isEmpty()) {
                wtitle = ride->getTag("Name", "");
            } else {

                // is "Route" set?
                if (!ride->getTag("Route", "").isEmpty()) {
                    wtitle = ride->getTag("Route", "");
                } else {

                    //  is Workout Code set?
                    if (!ride->getTag("Workout Code", "").isEmpty()) {
                        wtitle = ride->getTag("Workout Code", "");
                    }
                }
            }
        }
    }
    // did we set it to /anything/ ?
    if (wtitle != "") {
        QDomElement title = doc.createElement("title");
        text = doc.createTextNode(wtitle); title.appendChild(text);
        root.appendChild(title);
    }

    // goal
    if (ride->getTag("Objective", "") != "") {
        QString obj = ride->getTag("Objective", "");
        QDomElement goal = doc.createElement("goal");
        text = doc.createTextNode(obj); goal.appendChild(text);
        root.appendChild(goal);
    }

    // device type 
    if (ride->deviceType() != "") { 

        QDomElement device = doc.createElement("device"); 
        device.setAttribute("id", ride->deviceType());

        QDomElement make = doc.createElement("make"); 
        text = doc.createTextNode("Golden Cheetah"); 
        make.appendChild(text); 
        device.appendChild(make);

        QDomElement model = doc.createElement("model"); 
        text = doc.createTextNode(ride->deviceType());
        model.appendChild(text); 
        device.appendChild(model);

        root.appendChild(device); 
    }
    
    // time
    QDomElement time = doc.createElement("time");
    text = doc.createTextNode(ride->startTime().toUTC().toString(Qt::ISODate));
    time.appendChild(text);
    root.appendChild(time);

    // summary data
    QDomElement summarydata = doc.createElement("summarydata");
    root.appendChild(summarydata);
    QDomElement beginning = doc.createElement("beginning");
    text = doc.createTextNode(QString("%1").arg(ride->dataPoints().empty()
        ? 0 : ride->dataPoints().first()->secs));
    beginning.appendChild(text);
    summarydata.appendChild(beginning);

    QDomElement duration = doc.createElement("duration");
    text = doc.createTextNode(QString("%1").arg(ride->dataPoints().empty()
        ? 0 : ride->dataPoints().last()->secs));
    duration.appendChild(text);
    summarydata.appendChild(duration);

    // the channels - min max avg get set by TP anyway
    // so we leave them blank to save time on calculating them
    if (ride->areDataPresent()->hr) {
        QDomElement s = doc.createElement("hr");
        s.setAttribute("max", "0");
        s.setAttribute("min", "0");
        s.setAttribute("avg", "0");
        summarydata.appendChild(s);
    }
    if (ride->areDataPresent()->kph) {
        QDomElement s = doc.createElement("spd");
        s.setAttribute("max", "0");
        s.setAttribute("min", "0");
        s.setAttribute("avg", "0");
        summarydata.appendChild(s);
    }
    if (ride->areDataPresent()->watts) {
        QDomElement s = doc.createElement("pwr");
        s.setAttribute("max", "0");
        s.setAttribute("min", "0");
        s.setAttribute("avg", "0");
        summarydata.appendChild(s);
    }
    if (ride->areDataPresent()->nm) {
        QDomElement s = doc.createElement("torq");
        s.setAttribute("max", "0");
        s.setAttribute("min", "0");
        s.setAttribute("avg", "0");
        summarydata.appendChild(s);
    }
    if (ride->areDataPresent()->cad) {
        QDomElement s = doc.createElement("cad");
        s.setAttribute("max", "0");
        s.setAttribute("min", "0");
        s.setAttribute("avg", "0");
        summarydata.appendChild(s);
    }
    QDomElement dist = doc.createElement("dist");
    text = doc.createTextNode(QString("%1")
        .arg((int)(ride->dataPoints().empty() ? 0
            : ride->dataPoints().last()->km * 1000)));
    dist.appendChild(text);
    summarydata.appendChild(dist);

    if (ride->areDataPresent()->alt) {
        QDomElement s = doc.createElement("alt");
        s.setAttribute("max", "0");
        s.setAttribute("min", "0");
        s.setAttribute("avg", "0");
        summarydata.appendChild(s);
    }

    if (ride->areDataPresent()->temp) {
        QDomElement s = doc.createElement("temp");
        s.setAttribute("max", "0");
        s.setAttribute("min", "0");
        s.setAttribute("avg", "0");
        summarydata.appendChild(s);
    }  

    // interval "segments"
    foreach (RideFileInterval *i, ride->intervals()) {
        QDomElement segment = doc.createElement("segment");
        root.appendChild(segment);

        // name
        QDomElement name = doc.createElement("name");
        text = doc.createTextNode(i->name); name.appendChild(text);
        segment.appendChild(name);

        // summarydata
        QDomElement summarydata = doc.createElement("summarydata");
        segment.appendChild(summarydata);

        // beginning
        QDomElement beginning = doc.createElement("beginning");
        text = doc.createTextNode(QString("%1").arg(i->start));
        beginning.appendChild(text);
        summarydata.appendChild(beginning);

        // duration
        QDomElement duration = doc.createElement("duration");
        text = doc.createTextNode(QString("%1").arg(i->stop - i->start));
        duration.appendChild(text);
        summarydata.appendChild(duration);
    }

    // samples
    // data points: timeoffset, dist, hr, spd, pwr, torq, cad, lat, lon, alt
    if (!ride->dataPoints().empty()) {
        int secs = 0;

        foreach (const RideFilePoint *point, ride->dataPoints()) {
            // if there was a gap, log time when this sample started:
            if( secs + ride->recIntSecs() < point->secs ){
                QDomElement sample = doc.createElement("sample");
                root.appendChild(sample);

                QDomElement timeoffset = doc.createElement("timeoffset");
                text = doc.createTextNode(QString("%1")
                    .arg(point->secs - ride->recIntSecs() ));
                timeoffset.appendChild(text);
                sample.appendChild(timeoffset);
            }

            QDomElement sample = doc.createElement("sample");
            root.appendChild(sample);

            // time
            QDomElement timeoffset = doc.createElement("timeoffset");
            text = doc.createTextNode(QString("%1").arg(point->secs));
            timeoffset.appendChild(text);
            sample.appendChild(timeoffset);

            // hr
            if (ride->areDataPresent()->hr) {
                QDomElement hr = doc.createElement("hr");
                text = doc.createTextNode(QString("%1").arg((int)point->hr));
                hr.appendChild(text);
                sample.appendChild(hr);
            }
            // spd - meters per second
            if (ride->areDataPresent()->kph) {
                QDomElement spd = doc.createElement("spd");
                text = doc.createTextNode(QString("%1").arg(point->kph / 3.6));
                spd.appendChild(text);
                sample.appendChild(spd);
            }
            // pwr
            if (ride->areDataPresent()->watts) {
                // TrainingPeaks.com file upload rejects rides
                // with excessive power of zero for some reason
                // looks like they expect some smoothing or something?
                // we set 0 to 1 to at least get an upload
                // and do the reverse in the reader above
                int watts = point->watts ? point->watts : 1;
                QDomElement pwr = doc.createElement("pwr");
                text = doc.createTextNode(QString("%1").arg(watts));
                pwr.appendChild(text);
                sample.appendChild(pwr);
            }
            // lrbalance
            if (ride->areDataPresent()->lrbalance) {
                int rwatts = point->watts ? (point->watts - (point->watts * (point->lrbalance/100))) : 0;
                QDomElement lrbalance = doc.createElement("pwrright");
                text = doc.createTextNode(QString("%1").arg(rwatts));
                lrbalance.appendChild(text);
                sample.appendChild(lrbalance);
            }
            // torq
            if (ride->areDataPresent()->nm) {
                QDomElement torq = doc.createElement("torq");
                text = doc.createTextNode(QString("%1").arg(point->nm));
                torq.appendChild(text);
                sample.appendChild(torq);
            }
            // cad
            if (ride->areDataPresent()->cad) {
                QDomElement cad = doc.createElement("cad");
                text = doc.createTextNode(QString("%1").arg((int)(point->cad)));
                cad.appendChild(text);
                sample.appendChild(cad);
            }

            // distance - meters
            QDomElement dist = doc.createElement("dist");
            text = doc.createTextNode(QString("%1").arg((point->km*1000)));
            dist.appendChild(text);
            sample.appendChild(dist);


            // lat/lon only if both non-zero and valid.
            if (point->lat && point->lon) {

                // lon
                if (ride->areDataPresent()->lat && point->lat > -90.0 && point->lat < 90.0) {
                    QDomElement lat = doc.createElement("lat");
                    text = doc.createTextNode(QString("%1").arg(point->lat, 0, 'g', 11));
                    lat.appendChild(text);
                    sample.appendChild(lat);
                }
                // lon
                if (ride->areDataPresent()->lon && point->lon > -180.00 && point->lon < 180.00) {
                    QDomElement lon = doc.createElement("lon");
                    text = doc.createTextNode(QString("%1").arg(point->lon, 0, 'g', 11));
                    lon.appendChild(text);
                    sample.appendChild(lon);
                }
            }

            // alt
            if (ride->areDataPresent()->alt) {
                QDomElement alt = doc.createElement("alt");
                text = doc.createTextNode(QString("%1").arg(point->alt));
                alt.appendChild(text);
                sample.appendChild(alt);
            }

            // temp
            if (ride->areDataPresent()->temp) {
                QDomElement temp = doc.createElement("temp");
                text = doc.createTextNode(QString("%1").arg(point->temp));
                temp.appendChild(text);
                sample.appendChild(temp);
            }

            // torque_effectiveness_left
            if (ride->areDataPresent()->lte) {
                QDomElement lte = doc.createElement("torque_effectiveness_left");
                text = doc.createTextNode(QString("%1").arg(point->lte));
                lte.appendChild(text);
                sample.appendChild(lte);
            }
            // torque_effectiveness_right
            if (ride->areDataPresent()->rte) {
                QDomElement rte = doc.createElement("torque_effectiveness_right");
                text = doc.createTextNode(QString("%1").arg(point->rte));
                rte.appendChild(text);
                sample.appendChild(rte);
            }
            // pedal_smoothness_left
            if (ride->areDataPresent()->lps) {
                QDomElement lps = doc.createElement("pedal_smoothness_left");
                text = doc.createTextNode(QString("%1").arg(point->lps));
                lps.appendChild(text);
                sample.appendChild(lps);
            }
            // pedal_smoothness_right
            if (ride->areDataPresent()->rps) {
                QDomElement rps = doc.createElement("pedal_smoothness_right");
                text = doc.createTextNode(QString("%1").arg(point->rps));
                rps.appendChild(text);
                sample.appendChild(rps);
            }
        }
    }

    QByteArray xml = doc.toByteArray(4);
    if (!file.open(QIODevice::WriteOnly)) return(false);
    file.resize(0);
    QTextStream out(&file);
#if QT_VERSION < 0x060000
    out.setCodec("UTF-8");
#endif
    out.setGenerateByteOrderMark(true);
    out << xml;
    out.flush();
    file.close();
    return(true);
}
