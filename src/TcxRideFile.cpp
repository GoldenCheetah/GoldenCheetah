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

#include "TcxRideFile.h"
#include "TcxParser.h"
#include <QDomDocument>

#include "MainWindow.h"
#include "RideMetric.h"

#ifndef GC_VERSION
#define GC_VERSION "(developer build)"
#endif

static int tcxFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "tcx", "Garmin Training Centre TCX", new TcxFileReader());

RideFile *TcxFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*list) const
{
    (void) errors;
    RideFile *rideFile = new RideFile();
    rideFile->setRecIntSecs(1.0);
    rideFile->setDeviceType("Garmin");
    rideFile->setFileFormat("Garmin Training Centre (tcx)");

    TcxParser handler(rideFile, list);

    QXmlInputSource source (&file);
    QXmlSimpleReader reader;
    reader.setContentHandler (&handler);
    reader.parse (source);

    return rideFile;
}

bool
TcxFileReader::writeRideFile(MainWindow *mainWindow, const RideFile *ride, QFile &file) const
{
    QDomText text;
    QDomDocument doc;
    QDomProcessingInstruction hdr = doc.createProcessingInstruction("xml","version=\"1.0\"");
    doc.appendChild(hdr);

    // pwx
    QDomElement tcx = doc.createElementNS("http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2", "TrainingCenterDatabase");
    tcx.setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    tcx.setAttribute("xsi:schemaLocation", "http://www.garmin.com/xmlschemas/ActivityExtension/v2 http://www.garmin.com/xmlschemas/ActivityExtensionv2.xsd http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2 http://www.garmin.com/xmlschemas/TrainingCenterDatabasev2.xsd");
    //tcx.setAttribute("version", "2.0");
    doc.appendChild(tcx);

    // activities, we just serialise one ride
    QDomElement activities = doc.createElement("Activities");
    tcx.appendChild(activities);
    QDomElement activity = doc.createElement("Activity");
    activity.setAttribute("Sport", "Biking"); // was ride->getTag("Sport", "Biking") but must be Biking, Running or Other
    activities.appendChild(activity);

    // time
    QDomElement id = doc.createElement("Id");
    text = doc.createTextNode(ride->startTime().toUTC().toString(Qt::ISODate));
    id.appendChild(text);
    activity.appendChild(id);

    QDomElement lap = doc.createElement("Lap");
    lap.setAttribute("StartTime", ride->startTime().toUTC().toString(Qt::ISODate));
    activity.appendChild(lap);

    const char *metrics[] = {
        "total_distance",
        "workout_time",
        "total_work",
        NULL
    };

    QStringList worklist = QStringList();
    for (int i=0; metrics[i];i++) worklist << metrics[i];

    QHash<QString, RideMetricPtr> computed = RideMetric::computeMetrics(mainWindow, ride, mainWindow->zones(), mainWindow->hrZones(), worklist);

    QDomElement lap_time = doc.createElement("TotalTimeSeconds");
    text = doc.createTextNode(QString("%1").arg(computed.value("workout_time")->value(true)));
    //text = doc.createTextNode(ride->dataPoints().last()->secs);
    lap_time.appendChild(text);
    lap.appendChild(lap_time);

    QDomElement lap_distance = doc.createElement("DistanceMeters");
    text = doc.createTextNode(QString("%1").arg(1000*computed.value("total_distance")->value(true)));
    //text = doc.createTextNode(ride->dataPoints().last()->km);
    lap_distance.appendChild(text);
    lap.appendChild(lap_distance);

    QDomElement lap_calories = doc.createElement("Calories");
    text = doc.createTextNode(QString("%1").arg((int)computed.value("total_work")->value(true)));
    lap_calories.appendChild(text);
    lap.appendChild(lap_calories);

    QDomElement lap_intensity = doc.createElement("Intensity");
    text = doc.createTextNode("Active");
    lap_intensity.appendChild(text);
    lap.appendChild(lap_intensity);

    QDomElement lap_triggerMethod = doc.createElement("TriggerMethod");
    text = doc.createTextNode("Manual");
    lap_triggerMethod.appendChild(text);
    lap.appendChild(lap_triggerMethod);

    // samples
    // data points: timeoffset, dist, hr, spd, pwr, torq, cad, lat, lon, alt
    if (!ride->dataPoints().empty()) {
        QDomElement track = doc.createElement("Track");
        lap.appendChild(track);

        foreach (const RideFilePoint *point, ride->dataPoints()) {
            QDomElement trackpoint = doc.createElement("Trackpoint");
            track.appendChild(trackpoint);

            // time
            QDomElement time = doc.createElement("Time");
            text = doc.createTextNode(ride->startTime().toUTC().addSecs(point->secs).toString(Qt::ISODate));
            time.appendChild(text);
            trackpoint.appendChild(time);

            // position
            if (ride->areDataPresent()->lat && point->lat > -90.0 && point->lat < 90.0 && point->lat != 0.0 &&
                ride->areDataPresent()->lon && point->lon > -180.00 && point->lon < 180.00 && point->lon != 0.0 ) {
                QDomElement position = doc.createElement("Position");
                trackpoint.appendChild(position);

                // lat
                QDomElement lat = doc.createElement("LatitudeDegrees");
                text = doc.createTextNode(QString("%1").arg(point->lat, 0, 'g', 11));
                lat.appendChild(text);
                position.appendChild(lat);

                // lon
                QDomElement lon = doc.createElement("LongitudeDegrees");
                text = doc.createTextNode(QString("%1").arg(point->lon, 0, 'g', 11));
                lon.appendChild(text);
                position.appendChild(lon);
            }

            // alt
            if (ride->areDataPresent()->alt && point->alt != 0.0) {
                QDomElement alt = doc.createElement("AltitudeMeters");
                text = doc.createTextNode(QString("%1").arg(point->alt));
                alt.appendChild(text);
                trackpoint.appendChild(alt);
            }

            // distance - meters
            if (ride->areDataPresent()->km) {
                QDomElement dist = doc.createElement("DistanceMeters");
                text = doc.createTextNode(QString("%1").arg((int)(point->km*1000)));
                dist.appendChild(text);
                trackpoint.appendChild(dist);
            }

            // hr
            if (ride->areDataPresent()->hr && point->hr >0.00) {
                QDomElement hr = doc.createElement("HeartRateBpm");
                hr.setAttribute("xsi:type", "HeartRateInBeatsPerMinute_t");
                QDomElement value = doc.createElement("Value");
                text = doc.createTextNode(QString("%1").arg((int)point->hr));
                value.appendChild(text);
                hr.appendChild(value);
                trackpoint.appendChild(hr);
            }

            // cad
            if (ride->areDataPresent()->cad && point->cad < 255) { //xsd maxInclusive value="254"
                QDomElement cad = doc.createElement("Cadence");
                text = doc.createTextNode(QString("%1").arg((int)(point->cad)));
                cad.appendChild(text);
                trackpoint.appendChild(cad);
            }

            if (ride->areDataPresent()->kph || ride->areDataPresent()->watts) {
                QDomElement extension = doc.createElement("Extensions");
                trackpoint.appendChild(extension);
                QDomElement tpx = doc.createElement("TPX");
                tpx.setAttribute("xmlns", "http://www.garmin.com/xmlschemas/ActivityExtension/v2");
                extension.appendChild(tpx);

                // spd - meters per second
                if (ride->areDataPresent()->kph) {
                    QDomElement spd = doc.createElement("Speed");
                    text = doc.createTextNode(QString("%1").arg(point->kph / 3.6));
                    spd.appendChild(text);
                    tpx.appendChild(spd);
                }
                // pwr
                if (ride->areDataPresent()->watts) {
                    QDomElement pwr = doc.createElement("Watts");
                    text = doc.createTextNode(QString("%1").arg(point->watts));
                    pwr.appendChild(text);
                    tpx.appendChild(pwr);
                }
            }
        }
    }

    // Creator - Device
    QDomElement creator = doc.createElement("Creator");
    creator.setAttribute("xsi:type", "Device_t");
    activity.appendChild(creator);

    QDomElement creator_name = doc.createElement("Name");
    if (ride->deviceType() != "")
        text = doc.createTextNode(ride->deviceType());
    else
        text = doc.createTextNode("Unknow");
    creator_name.appendChild(text);
    creator.appendChild(creator_name);

    QDomElement creator_unitId = doc.createElement("UnitId");
    text = doc.createTextNode("0");
    creator_unitId.appendChild(text);
    creator.appendChild(creator_unitId);

    QDomElement creator_productId = doc.createElement("ProductId");
    text = doc.createTextNode("0");
    creator_productId.appendChild(text);
    creator.appendChild(creator_productId);


    QDomElement creator_version = doc.createElement("Version");
    creator.appendChild(creator_version);

    QDomElement creator_version_version_major = doc.createElement("VersionMajor");
    text = doc.createTextNode("3");
    creator_version_version_major.appendChild(text);
    creator_version.appendChild(creator_version_version_major);

    QDomElement creator_version_version_minor = doc.createElement("VersionMinor");
    text = doc.createTextNode("0");
    creator_version_version_minor.appendChild(text);
    creator_version.appendChild(creator_version_version_minor);

    QDomElement creator_version_build_major = doc.createElement("BuildMajor");
    text = doc.createTextNode("0");
    creator_version_build_major.appendChild(text);
    creator_version.appendChild(creator_version_build_major);

    QDomElement creator_version_build_minor = doc.createElement("BuildMinor");
    text = doc.createTextNode("0");
    creator_version_build_minor.appendChild(text);
    creator_version.appendChild(creator_version_build_minor);


    // Author - Application
    QDomElement author = doc.createElement("Author");
    author.setAttribute("xsi:type", "Application_t");
    tcx.appendChild(author);

    QDomElement author_name = doc.createElement("Name");
    text = doc.createTextNode("GoldenCheetah");
    author_name.appendChild(text);
    author.appendChild(author_name);

    QDomElement author_build = doc.createElement("Build");
    author.appendChild(author_build);

    QDomElement author_version = doc.createElement("Version");
    author_build.appendChild(author_version);

    QDomElement author_version_version_major = doc.createElement("VersionMajor");
    text = doc.createTextNode("3");
    author_version_version_major.appendChild(text);
    author_version.appendChild(author_version_version_major);

    QDomElement author_version_version_minor = doc.createElement("VersionMinor");
    text = doc.createTextNode("0");
    author_version_version_minor.appendChild(text);
    author_version.appendChild(author_version_version_minor);

    QDomElement author_version_build_major = doc.createElement("BuildMajor");
    text = doc.createTextNode("0");
    author_version_build_major.appendChild(text);
    author_version.appendChild(author_version_build_major);

    QDomElement author_version_build_minor = doc.createElement("BuildMinor");
    text = doc.createTextNode("0");
    author_version_build_minor.appendChild(text);
    author_version.appendChild(author_version_build_minor);

    QDomElement author_type = doc.createElement("Type");
    text = doc.createTextNode("Beta"); // was GC_VERSION but should be Interna | Alpha | Beta | Release
    author_type.appendChild(text);
    author_build.appendChild(author_type);

    QDomElement author_part_number = doc.createElement("PartNumber");
    text = doc.createTextNode("0");
    author_part_number.appendChild(text);
    author.appendChild(author_part_number);


    QByteArray xml = doc.toByteArray(4);
    if (!file.open(QIODevice::WriteOnly)) return(false);
    if (file.write(xml) != xml.size()) return(false);
    file.close();
    return(true);
}
