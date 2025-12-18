/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#include "FitlogRideFile.h"
#include "FitlogParser.h"
#include <QDomDocument>

#include "Context.h"
#include "Athlete.h"
#include "RideMetric.h"
#include "RideItem.h"

#ifndef GC_VERSION
#define GC_VERSION "(developer build)"
#endif

static int fitlogFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "fitlog", "SportTracks Fitlog", new FitlogFileReader());

RideFile *FitlogFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*list) const
{
    (void) errors;
    RideFile *rideFile = new RideFile();
    rideFile->setRecIntSecs(1.0);
    //rideFile->setDeviceType("SportTracks Fitlog");
    rideFile->setFileFormat("SportTracks (*.fitlog)");

    FitlogParser handler(rideFile, list);

    QXmlInputSource source (&file);
    QXmlSimpleReader reader;
    reader.setContentHandler (&handler);
    reader.parse (source);

    return rideFile;
}

bool
FitlogFileReader::writeRideFile(Context *context, const RideFile *ride, QFile &file) const
{
    QDomText text;
    QDomDocument doc;
    QDomProcessingInstruction hdr = doc.createProcessingInstruction("xml","version=\"1.0\"");
    doc.appendChild(hdr);

    // fitlog
    // FitnessWorkbook

    QDomElement fitnessWorkbook = doc.createElementNS("http://www.zonefivesoftware.com/xmlschemas/FitnessLogbook/v2", "FitnessWorkbook");
    fitnessWorkbook.setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    fitnessWorkbook.setAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema");
    doc.appendChild(fitnessWorkbook);

    QDomElement athleteLog = doc.createElement("AthleteLog");
    fitnessWorkbook.appendChild(athleteLog);

    QDomElement athlete = doc.createElement("Athlete");
    athlete.setAttribute("athlete",context->athlete->cyclist);
    athleteLog.appendChild(athlete);

    QDomElement activity = doc.createElement("Activity");
    activity.setAttribute("StartTime", ride->startTime().toString(Qt::ISODate)+"Z");
    activity.setAttribute("Id", ride->id());
    athleteLog.appendChild(activity);

    QDomElement metadata = doc.createElement("Metadata");
    metadata.setAttribute("Source", "GoldenCheetah");
    //metadata.setAttribute("Created", "");
    //metadata.setAttribute("Modified", "");
    activity.appendChild(metadata);

    const char *metrics[] = {
        "total_distance",
        "workout_time",
        "total_work",
        "elevation_gain",
        "average_hr",
        "max_heartrate",
        "average_cad",
        "max_cadence",
        "average_power",
        "max_power",
        NULL
    };
    
    QStringList worklist = QStringList();
    for (int i=0; metrics[i];i++) worklist << metrics[i];

    RideItem *tempItem = new RideItem(const_cast<RideFile*>(ride), context);
    QHash<QString,RideMetricPtr> computed = RideMetric::computeMetrics(tempItem, Specification(), worklist);

    QDomElement duration = doc.createElement("Duration");
    duration.setAttribute("TotalSeconds", QString("%1").arg(computed.value("workout_time")->value(true)));
    activity.appendChild(duration);

    QDomElement distance = doc.createElement("Distance");
    distance.setAttribute("TotalMeters", QString("%1").arg(1000*computed.value("total_distance")->value(true)));
    activity.appendChild(distance);

    QDomElement elevation = doc.createElement("Elevation");
    //elevation.setAttribute("DescendMeters", "");
    elevation.setAttribute("AscendMeters", QString("%1").arg(computed.value("elevation_gain")->value(true)));
    activity.appendChild(elevation);

    QDomElement heartRate = doc.createElement("HeartRate");
    heartRate.setAttribute("AverageBPM", QString("%1").arg(computed.value("average_hr")->value(true)));
    heartRate.setAttribute("MaximumBPM", QString("%1").arg(computed.value("max_heartrate")->value(true)));
    activity.appendChild(heartRate);

    QDomElement cadence = doc.createElement("Cadence");
    cadence.setAttribute("AverageRPM", QString("%1").arg(computed.value("average_cad")->value(true)));
    cadence.setAttribute("MaximumRPM", QString("%1").arg(computed.value("max_cadence")->value(true)));
    activity.appendChild(cadence);

    QDomElement power = doc.createElement("Power");
    power.setAttribute("AverageWatts", QString("%1").arg(computed.value("average_power")->value(true)));
    power.setAttribute("MaximumWatts", QString("%1").arg(computed.value("max_power")->value(true)));
    activity.appendChild(power);

    QDomElement calories = doc.createElement("Calories");
    calories.setAttribute("TotalCal", QString("%1").arg(computed.value("total_work")->value(true)));
    activity.appendChild(calories);

    //QDomElement laps = doc.createElement("Laps");
    //activity.appendChild(laps);

    if (!ride->intervals().empty()) {
        QDomElement laps = doc.createElement("Laps");
        activity.appendChild(laps);

        foreach (RideFileInterval *interval, ride->intervals()) {
            RideFile f(ride->startTime(), ride->recIntSecs());
            for (int i = ride->intervalBegin(*interval); i < ride->dataPoints().size(); ++i) {
                const RideFilePoint *p = ride->dataPoints()[i];
                if (p->secs >= interval->stop)
                    break;
                f.appendPoint(p->secs, p->cad, p->hr, p->km, p->kph, p->nm,
                              p->watts, p->alt, p->lon, p->lat, p->headwind,
                              0.0, RideFile::NA, RideFile::NA,
                              0.0, 0.0, 0.0, 0.0,
                              0.0, 0.0,
                              0.0, 0.0, 0.0, 0.0,
                              0.0, 0.0, 0.0, 0.0,
                              0.0, 0.0,
                              0.0, 0.0, 0.0, // running dynamics
                              0.0, //tcore
                              0);
            }
            if (f.dataPoints().size() == 0) {
                // Interval empty, do not compute any metrics
                continue;
            }

            RideItem *tempItem = new RideItem(const_cast<RideFile*>(&f), context);
            computed = RideMetric::computeMetrics(tempItem, Specification(), worklist);

            QDomElement lap = doc.createElement("Lap");
            lap.setAttribute("StartTime", ride->startTime().addSecs(interval->start).toString(Qt::ISODate)+"Z");
            lap.setAttribute("DurationSeconds", interval->stop-interval->start);
            laps.appendChild(lap);

            QDomElement lap_distance = doc.createElement("Distance");
            lap_distance.setAttribute("TotalMeters", QString("%1").arg(1000*computed.value("total_distance")->value(true)));
            lap.appendChild(lap_distance);

            QDomElement lap_elevation = doc.createElement("Elevation");
            //elevation.setAttribute("DescendMeters", "");
            lap_elevation.setAttribute("AscendMeters", QString("%1").arg(computed.value("elevation_gain")->value(true)));
            lap.appendChild(lap_elevation);

            QDomElement lap_heartRate = doc.createElement("HeartRate");
            lap_heartRate.setAttribute("AverageBPM", QString("%1").arg(computed.value("average_hr")->value(true)));
            lap_heartRate.setAttribute("MaximumBPM", QString("%1").arg(computed.value("max_heartrate")->value(true)));
            lap.appendChild(lap_heartRate);

            QDomElement lap_cadence = doc.createElement("Cadence");
            lap_cadence.setAttribute("AverageRPM", QString("%1").arg(computed.value("average_cad")->value(true)));
            lap_cadence.setAttribute("MaximumRPM", QString("%1").arg(computed.value("max_cadence")->value(true)));
            lap.appendChild(lap_cadence);

            QDomElement lap_power = doc.createElement("Power");
            lap_power.setAttribute("AverageWatts", QString("%1").arg(computed.value("average_power")->value(true)));
            lap_power.setAttribute("MaximumWatts", QString("%1").arg(computed.value("max_power")->value(true)));
            lap.appendChild(lap_power);

            QDomElement lap_calories = doc.createElement("Calories");
            lap_calories.setAttribute("TotalCal", QString("%1").arg(computed.value("total_work")->value(true)));
            lap.appendChild(lap_calories);

        }
    }



    // data points
    if (!ride->dataPoints().empty()) {
        QDomElement track = doc.createElement("Track");
        track.setAttribute("StartTime", ride->startTime().toString(Qt::ISODate)+"Z");
        activity.appendChild(track);

        foreach (const RideFilePoint *point, ride->dataPoints()) {
            QDomElement pt = doc.createElement("pt");
            pt.setAttribute("tm", point->secs);
            // position
            if (ride->areDataPresent()->lat && point->lat > -90.0 && point->lat < 90.0 && point->lat != 0.0 &&
                ride->areDataPresent()->lon && point->lon > -180.00 && point->lon < 180.00 && point->lon != 0.0 ) {

                pt.setAttribute("lat", point->lat);
                pt.setAttribute("lon", point->lon);
            } else
                pt.setAttribute("dist", point->km*1000);

            pt.setAttribute("ele", point->alt);
            pt.setAttribute("hr", point->hr);
            pt.setAttribute("cadence", point->cad);
            pt.setAttribute("power", point->watts);

            track.appendChild(pt);
        }
    }

    QByteArray xml = doc.toByteArray(4);
    if (!file.open(QIODevice::WriteOnly)) return(false);
    file.resize(0);
    QTextStream out(&file);
    out.setGenerateByteOrderMark(true);
    out << xml;
    out.flush();
    file.close();
    return(true);
}
