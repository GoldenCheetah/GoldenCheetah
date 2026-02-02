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

#include <QDebug>

#define DATETIME_FORMAT "yyyy/MM/dd hh:mm:ss' UTC'"

static int gcFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "gc", "GoldenCheetah XML", new GcFileReader());

RideFile *
GcFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    QDomDocument doc("GoldenCheetah");
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

    RideFile *rideFile = new RideFile();
    QDomElement root = doc.documentElement();
    QDomNode attributes = root.firstChildElement("attributes");

    for (QDomElement attr = attributes.firstChildElement("attribute");
         !attr.isNull(); attr = attr.nextSiblingElement("attribute")) {
        QString key = attr.attribute("key");
        QString value = attr.attribute("value");
        if (key == "Device type")
            rideFile->setDeviceType(value);
        else if (key == "File Format")
            rideFile->setFileFormat(value);
        if (key == "Start time") {
            // by default QDateTime is localtime - the source however is UTC
            QDateTime aslocal = QDateTime::fromString(value, DATETIME_FORMAT);
            // construct in UTC so we can honour the conversion to localtime
            QDateTime asUTC = QDateTime(aslocal.date(), aslocal.time(), QTimeZone::UTC);
            // now set in localtime
            rideFile->setStartTime(asUTC.toLocalTime());
        }
        if (key == "Identifier") {
            rideFile->setId(value);
        }
    }

    // read in metric overrides:
    //  <override>
    //    <metric name="skiba_bike_score" value="100"/>
    //    <metric name="average_speed" secs="3600" km="30"/>
    //  </override>

    QDomNode overrides = root.firstChildElement("override");
    if (!overrides.isNull()) {

        for (QDomElement override = overrides.firstChildElement("metric");
            !override.isNull();
            override = override.nextSiblingElement("metric")) {

            // setup the metric overrides QMap
            QMap<QString, QString> bsm;

            // for now only value is known to be maintained
            bsm.insert("value", override.attribute("value"));

            // insert into the rideFile overrides
            rideFile->metricOverrides.insert(override.attribute("name"), bsm);
        }
    }

    // read in the name/value metadata pairs
    QDomNode tags = root.firstChildElement("tags");
    if (!tags.isNull()) {

        for (QDomElement tag = tags.firstChildElement("tag");
             !tag.isNull();
             tag = tag.nextSiblingElement("tag")) {

            rideFile->setTag(tag.attribute("name"), tag.attribute("value"));
        }
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
            rideFile->addInterval(RideFileInterval::DEVICE, add.start, add.stop, add.name);
        }
    }
    std::sort(intervalStops.begin(), intervalStops.end()); // just in case
    int interval = 0;

    QDomElement samples = root.firstChildElement("samples");
    if (samples.isNull()) return rideFile; // manual file will have no samples

    bool recIntSet = false;
    for (QDomElement sample = samples.firstChildElement("sample");
         !sample.isNull(); sample = sample.nextSiblingElement("sample")) {
        double secs, cad, hr, km, kph, nm, watts, alt, lon, lat;
        double headwind = 0.0;
        secs = sample.attribute("secs", "0.0").toDouble();
        cad = sample.attribute("cad", "0.0").toDouble();
        hr = sample.attribute("hr", "0.0").toDouble();
        km = sample.attribute("km", "0.0").toDouble();
        kph = sample.attribute("kph", "0.0").toDouble();
        nm = sample.attribute("nm", "0.0").toDouble();
        watts = sample.attribute("watts", "0.0").toDouble();
        alt = sample.attribute("alt", "0.0").toDouble();
        lon = sample.attribute("lon", "0.0").toDouble();
        lat = sample.attribute("lat", "0.0").toDouble();
        while ((interval < intervalStops.size()) && (secs >= intervalStops[interval]))
            ++interval;
        rideFile->appendPoint(secs, cad, hr, km, kph, nm, watts, alt, lon, lat, headwind, 0.0,
                               RideFile::NA, RideFile::NA,
                              0.0, 0.0, 0.0, 0.0,
                              0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, interval);
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

// normal precision (Qt defaults)
#define add_sample(name) \
    if (present->name) \
        sample.setAttribute(#name, QString("%1").arg(point->name));

// high precision (6 decimals)
#define add_sample_hp(name) \
    if (present->name) \
        sample.setAttribute(#name, QString("%1").arg(point->name, 0, 'g', 11));
bool
GcFileReader::writeRideFile(Context *,const RideFile *ride, QFile &file) const
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
        "value", ride->startTime().toUTC().toString(DATETIME_FORMAT));
    attribute = doc.createElement("attribute");
    attributes.appendChild(attribute);
    attribute.setAttribute("key", "Device type");
    attribute.setAttribute("value", ride->deviceType());
    attribute = doc.createElement("attribute");
    attributes.appendChild(attribute);
    attribute.setAttribute("key", "Identifier");
    attribute.setAttribute("value", ride->id());

    // write out in metric overrides:
    //  <override>
    //    <metric name="skiba_bike_score" value="100"/>
    //    <metric name="average_speed" secs="3600" km="30"/>
    //  </override>
    // write out the QMap tag/value pairs
    QDomElement overrides = doc.createElement("override");
    root.appendChild(overrides);
    QMap<QString,QMap<QString, QString> >::const_iterator k;
    for (k=ride->metricOverrides.constBegin(); k != ride->metricOverrides.constEnd(); k++) {

        // may not contain anything
        if (k.value().isEmpty()) continue;

        QDomElement override = doc.createElement("metric");
        overrides.appendChild(override);

        // metric name
        override.setAttribute("name", k.key());

        // key/value pairs
        QMap<QString, QString>::const_iterator j;
        for (j=k.value().constBegin(); j != k.value().constEnd(); j++) {
            override.setAttribute(j.key(), j.value());
        }
    }

    // write out the QMap tag/value pairs
    QDomElement tags = doc.createElement("tags");
    root.appendChild(tags);
    QMap<QString,QString>::const_iterator i;
    for (i=ride->tags().constBegin(); i != ride->tags().constEnd(); i++) {
            QDomElement tag = doc.createElement("tag");
            tags.appendChild(tag);

            tag.setAttribute("name", i.key());
            tag.setAttribute("value", i.value());
    }

    if (!ride->intervals().empty()) {
        QDomElement intervals = doc.createElement("intervals");
        root.appendChild(intervals);
        foreach (RideFileInterval *i, ride->intervals()) {
            QDomElement interval = doc.createElement("interval");
            intervals.appendChild(interval);
            interval.setAttribute("name", i->name);
            interval.setAttribute("start", QString("%1").arg(i->start));
            interval.setAttribute("stop", QString("%1").arg(i->stop));
        }
    }

    if (!ride->dataPoints().empty()) {
        QDomElement samples = doc.createElement("samples");
        root.appendChild(samples);
        const RideFileDataPresent *present = ride->areDataPresent();
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            QDomElement sample = doc.createElement("sample");
            samples.appendChild(sample);
            add_sample_hp(secs);
            add_sample(cad);
            add_sample(hr);
            add_sample(km);
            add_sample(kph);
            add_sample(nm);
            add_sample(watts);
            add_sample(alt);
            add_sample_hp(lon);
            add_sample_hp(lat);
            sample.setAttribute("len", QString("%1").arg(ride->recIntSecs()));
        }
    }

    QByteArray xml = doc.toByteArray(4);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.resize(0);
    QTextStream out(&file);
    out.setGenerateByteOrderMark(true);
    out << xml;
    out.flush();
    file.close();
    return true;
}
