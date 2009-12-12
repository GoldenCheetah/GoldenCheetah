/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net),
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

#include "GcRideFile.h"
#include <algorithm> // for std::sort
#include <QDomDocument>
#include <QVector>
#include <assert.h>

static int gcFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "gc", "GoldenCheetah Native Format", new GcFileReader());

RideFile *
GcFileReader::openRideFile(QFile &file, QStringList &errors) const
{
    QDomDocument doc("GoldenCheetah");
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

    RideFile *rideFile = new RideFile();
    QDomElement root = doc.documentElement();
    QDomNode attributes = root.firstChildElement("attributes");

    for (QDomElement attr = attributes.firstChildElement("attribute");
         !attr.isNull(); attr = attr.nextSiblingElement("attribute")) {
        QString key = attr.attribute("key");
        QString value = attr.attribute("value");
        if (key == "Device type")
            rideFile->setDeviceType(value);
        if (key == "Start time")
            rideFile->setStartTime(QDateTime::fromString(value)); // TODO format
    }

    QVector<double> intervalStops; // used to set the interval number for each point
    RideFileInterval add;          // used to add each named interval to RideFile
    QDomNode intervals = root.firstChildElement("intervals");
    if (!intervals.isNull()) {
        for (QDomElement interval = intervals.firstChildElement("interval");
             !interval.isNull(); interval = interval.nextSiblingElement("interval")) {

            // record the stops for old-style datapoint interval numbering
            double stop = interval.attribute("stop").toDouble();
            intervalStops.append(stop);

            // add a new interval to the new-style interval ranges
            add.stop = stop;
            add.start = interval.attribute("start").toDouble();
            add.name = interval.attribute("name");
            rideFile->addInterval(add.start, add.stop, add.name);
        }
    }
    std::sort(intervalStops.begin(), intervalStops.end()); // just in case
    int interval = 0;

    QDomElement samples = root.firstChildElement("samples");
    if (samples.isNull()) {
        errors << "no sample section in ride file";
        return NULL;
    }

    bool recIntSet = false;
    for (QDomElement sample = samples.firstChildElement("sample");
         !sample.isNull(); sample = sample.nextSiblingElement("sample")) {
        double secs, cad, hr, km, kph, nm, watts, alt;
        secs = sample.attribute("secs", "0.0").toDouble();
        cad = sample.attribute("cad", "0.0").toDouble();
        hr = sample.attribute("hr", "0.0").toDouble();
        km = sample.attribute("km", "0.0").toDouble();
        kph = sample.attribute("kph", "0.0").toDouble();
        nm = sample.attribute("nm", "0.0").toDouble();
        watts = sample.attribute("watts", "0.0").toDouble();
        alt = sample.attribute("alt", "0.0").toDouble();
        while ((interval < intervalStops.size()) && (secs >= intervalStops[interval]))
            ++interval;
        rideFile->appendPoint(secs, cad, hr, km, kph, nm, watts, alt, interval);
        if (!recIntSet) {
            rideFile->setRecIntSecs(sample.attribute("len").toDouble());
            recIntSet = true;
        }
    }

    if (!recIntSet) {
        errors << "no samples in ride file";
        return NULL;
    }

    return rideFile;
}

#define add_sample(name) \
    if (present->name) \
        sample.setAttribute(#name, QString("%1").arg(point->name));

void
GcFileReader::writeRideFile(const RideFile *ride, QFile &file) const
{
    QDomDocument doc("GoldenCheetah");
    QDomElement root = doc.createElement("ride");
    doc.appendChild(root);

    QDomElement attributes = doc.createElement("attributes");
    root.appendChild(attributes);

    QDomElement attribute = doc.createElement("attribute");
    attributes.appendChild(attribute);
    attribute.setAttribute("key", "Start time");
    attribute.setAttribute(
        "value", ride->startTime().toUTC().toString("yyyy/MM/dd hh:mm:ss' UTC'"));
    attribute = doc.createElement("attribute");
    attributes.appendChild(attribute);
    attribute.setAttribute("key", "Device type");
    attribute.setAttribute("value", ride->deviceType());

    if (!ride->intervals().empty()) {
        QDomElement intervals = doc.createElement("intervals");
        root.appendChild(intervals);
        foreach (RideFileInterval i, ride->intervals()) {
            QDomElement interval = doc.createElement("interval");
            intervals.appendChild(interval);
            interval.setAttribute("name", i.name);
            interval.setAttribute("start", QString("%1").arg(i.start));
            interval.setAttribute("stop", QString("%1").arg(i.stop));
        }
    }

    if (!ride->dataPoints().empty()) {
        QDomElement samples = doc.createElement("samples");
        root.appendChild(samples);
        const RideFileDataPresent *present = ride->areDataPresent();
        assert(present->secs);
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            QDomElement sample = doc.createElement("sample");
            samples.appendChild(sample);
            assert(present->secs);
            add_sample(secs);
            add_sample(cad);
            add_sample(hr);
            add_sample(km);
            add_sample(kph);
            add_sample(nm);
            add_sample(watts);
            add_sample(alt);
            sample.setAttribute("len", QString("%1").arg(ride->recIntSecs()));
        }
    }

    QByteArray xml = doc.toByteArray(4);
    if (!file.open(QIODevice::WriteOnly))
        assert(false);
    if (file.write(xml) != xml.size())
        assert(false);
    file.close();
}

