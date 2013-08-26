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
#include "Settings.h"
#include <QDomDocument>
#include <QVector>
#include <assert.h>

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

    bool parsed = doc.setContent(&file);
    file.close();
    if (!parsed) {
        errors << "Could not parse file.";
        return NULL;
    }

    return PwxFromDomDoc(doc, errors);
}

RideFile *
PwxFileReader::PwxFromDomDoc(QDomDocument doc, QStringList &errors) const
{
    RideFile *rideFile = new RideFile();
    QDomElement root = doc.documentElement();
    QDomNode workout = root.firstChildElement("workout");
    QDomNode node = workout.firstChild();

    // can arrive at any time, so lets cache them
    // and sort out at the end
    QDateTime rideDate;

    int intervals = 0;
    int samples = 0;

    // in case we need to calculate distance
    double rtime = 0;
    double rdist = 0;

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
                    rideFile->addInterval(add.start, add.stop, add.name);
                }
            }

        // data points: offset, hr, spd, pwr, torq, cad, dist, lat, lon, alt (ignored: temp, time)
        } else if (node.nodeName() == "sample") {
            RideFilePoint add;

            // offset (secs)
            QDomElement off = node.firstChildElement("timeoffset");
            if (!off.isNull()) add.secs = off.text().toDouble();
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

            // do we need to calculate distance?
            if (add.km == 0.0 && samples) {
                // delta secs * kph/3600
                add.km = rdist + ((add.secs - rtime) * (add.kph/3600));
            }

            // running totals
            samples++;
            rtime = add.secs;
            rdist = add.km;

            // add the data point
            rideFile->appendPoint(add.secs, add.cad, add.hr, add.km, add.kph,
                    add.nm, add.watts, add.alt, add.lon, add.lat, add.headwind,
                    add.slope, add.temp, add.lrbalance, add.interval);


        // ignored for now
        } else if (node.nodeName() == "summarydata") {
        } else if (node.nodeName() == "extension") {
        }

        node = node.nextSibling();
    }

    // post-process and check
    if (samples < 2) {
        errors << "Not enough samples present";
        delete rideFile;
        return NULL;
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
    return rideFile;
}

bool
PwxFileReader::writeRideFile(MainWindow *main, const RideFile *ride, QFile &file) const
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
    text = doc.createTextNode(main->cyclist); name.appendChild(text);
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
    text = doc.createTextNode(ride->startTime().toString(Qt::ISODate));
    time.appendChild(text);
    root.appendChild(time);

    if (!ride->intervals().empty()) {

        // summary data
        QDomElement summarydata = doc.createElement("summarydata");
        root.appendChild(summarydata);
        QDomElement beginning = doc.createElement("beginning");
        text = doc.createTextNode(QString("%1").arg(ride->dataPoints().first()->secs));
        beginning.appendChild(text);
        summarydata.appendChild(beginning);

        QDomElement duration = doc.createElement("duration");
        text = doc.createTextNode(QString("%1").arg(ride->dataPoints().last()->secs));
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
        text = doc.createTextNode(QString("%1").arg((int)(ride->dataPoints().last()->km * 1000)));
        dist.appendChild(text);
        summarydata.appendChild(dist);

        if (ride->areDataPresent()->alt) {
            QDomElement s = doc.createElement("alt");
            s.setAttribute("max", "0");
            s.setAttribute("min", "0");
            s.setAttribute("avg", "0");
            summarydata.appendChild(s);
        }

        // interval "segments"
        foreach (RideFileInterval i, ride->intervals()) {
            QDomElement segment = doc.createElement("segment");
            root.appendChild(segment);

            // name
            QDomElement name = doc.createElement("name");
            text = doc.createTextNode(i.name); name.appendChild(text);
            segment.appendChild(name);

            // summarydata
            QDomElement summarydata = doc.createElement("summarydata");
            segment.appendChild(summarydata);

            // beginning
            QDomElement beginning = doc.createElement("beginning");
            text = doc.createTextNode(QString("%1").arg(i.start));
            beginning.appendChild(text);
            summarydata.appendChild(beginning);

            // duration
            QDomElement duration = doc.createElement("duration");
            text = doc.createTextNode(QString("%1").arg(i.stop - i.start));
            duration.appendChild(text);
            summarydata.appendChild(duration);
        }
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
                    .arg((int)point->secs - ride->recIntSecs() ));
                timeoffset.appendChild(text);
                sample.appendChild(timeoffset);
            }

            QDomElement sample = doc.createElement("sample");
            root.appendChild(sample);

            // time
            QDomElement timeoffset = doc.createElement("timeoffset");
            text = doc.createTextNode(QString("%1").arg((int)point->secs));
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
            text = doc.createTextNode(QString("%1").arg((int)(point->km*1000)));
            dist.appendChild(text);
            sample.appendChild(dist);


            // lat
            if (ride->areDataPresent()->lat && point->lat > -90.0 && point->lat < 90.0) {
                QDomElement lat = doc.createElement("lat");
                text = doc.createTextNode(QString("%1").arg(point->lat));
                lat.appendChild(text);
                sample.appendChild(lat);
            }
            // lon
            if (ride->areDataPresent()->lon && point->lon > -180.00 && point->lon < 180.00) {
                QDomElement lon = doc.createElement("lon");
                text = doc.createTextNode(QString("%1").arg(point->lon));
                lon.appendChild(text);
                sample.appendChild(lon);
            }

            // alt
            if (ride->areDataPresent()->alt) {
                QDomElement alt = doc.createElement("alt");
                text = doc.createTextNode(QString("%1").arg(point->alt));
                alt.appendChild(text);
                sample.appendChild(alt);
            }
        }
    }

    QByteArray xml = doc.toByteArray(4);
    if (!file.open(QIODevice::WriteOnly)) return(false);
    if (file.write(xml) != xml.size()) return(false);
    file.close();
    return(true);
}
