/*
 * Copyright (c) 2010 Greg Lonnon (greg.lonnon@gmail.com) copied from TcxRideFile.cpp
 *
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

#include "GpxRideFile.h"
#include "GpxParser.h"
#include "GcUpgrade.h"
#include <QDomDocument>

static int gpxFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "gpx", "GPS Exchange format", new GpxFileReader());

RideFile *GpxFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    (void) errors;
    RideFile *rideFile = new RideFile();
    rideFile->setRecIntSecs(1.0);
    //rideFile->setDeviceType("GPS Exchange Format");
    rideFile->setFileFormat("GPS Exchange Format (gpx)");

    GpxParser handler(rideFile);

    QXmlInputSource source (&file);
    QXmlSimpleReader reader;
    reader.setContentHandler (&handler);
    reader.parse (source);

    return rideFile;
}

QByteArray
GpxFileReader::toByteArray(Context *, const RideFile *ride, bool withAlt, bool withWatts, bool withHr, bool withCad) const
{
    //
    // GPX Standard defined here:  http://www.topografix.com/GPX/1/1/
    //
    QDomDocument doc;
    QDomProcessingInstruction hdr = doc.createProcessingInstruction("xml","version=\"1.0\"");
    doc.appendChild(hdr);

    QDomElement gpx = doc.createElementNS("http://www.topografix.com/GPX/1/1", "gpx");

    gpx.setAttribute("xmlns:xsi",    "http://www.w3.org/2001/XMLSchema-instance");
    gpx.setAttribute("xmlns:gpxtpx", "http://www.garmin.com/xmlschemas/TrackPointExtension/v1");
    gpx.setAttribute("xmlns:gpxpx",  "http://www.garmin.com/xmlschemas/PowerExtension/v1");

    gpx.setAttribute("xsi:schemaLocation",
                     "http://www.topografix.com/GPX/1/1"                           " " 
                     "http://www.topografix.com/GPX/1/1/gpx.xsd"                   " " 
                     "http://www.garmin.com/xmlschemas/TrackPointExtension/v1"     " " 
                     "http://www.garmin.com/xmlschemas/TrackPointExtensionv1.xsd"  " " 
                     "http://www.garmin.com/xmlschemas/PowerExtension/v1"          " " 
                     "http://www.garmin.com/xmlschemas/PowerExtensionv1.xsd"        );

    gpx.setAttribute("version", "1.1");
    gpx.setAttribute("creator", QString("GoldenCheetah (build %1) with Barometer").arg(VERSION_LATEST));
    doc.appendChild(gpx);


    // If we have data points, we'll have a <trk> and in that a <trkseg> and in that a bunch of <trkpt>
    if (!ride->dataPoints().empty()) {
        QDomElement trk = doc.createElement("trk");
        gpx.appendChild(trk);

        QDomElement trkseg = doc.createElement("trkseg");
        trk.appendChild(trkseg);        

        QLocale cLocale(QLocale::Language::C);

        foreach (const RideFilePoint *point, ride->dataPoints()) {
            QDomElement trkpt = doc.createElement("trkpt");
            trkseg.appendChild(trkpt);

            QString strLat = cLocale.toString(point->lat, 'g', 12);
            trkpt.setAttribute("lat", strLat);
            QString strLon = cLocale.toString(point->lon, 'g', 12);
            trkpt.setAttribute("lon", strLon);

            // GPX standard requires <ele>, if present, to be first
            if (withAlt && ride->areDataPresent()->alt) {
                QDomElement ele = doc.createElement("ele");
                //QDomText text = doc.createTextNode(QString("%1").arg(point->alt, 0, 'f', 1));
                //ele.appendChild(text);
                ele.appendChild(doc.createTextNode(QString("%1").arg(point->alt, 0, 'f', 1)));
                trkpt.appendChild(ele);
            }

            // GPX standard requires <time>, if present, to come next
            if (ride->areDataPresent()->secs && point->secs >= 0)
            {
                QDomElement tm = doc.createElement("time");
                QDomText text = doc.createTextNode(ride->startTime().toUTC().addSecs(point->secs).toString(Qt::ISODate));
                tm.appendChild(text);
                trkpt.appendChild(tm);
            }

            // Extra things, if any, need to go into an <extensions> tag
            QDomElement gpxtpx_atemp;     // temperature
            QDomElement gpxtpx_hr;        // HR
            QDomElement gpxtpx_cad;       // cadance
            QDomElement pwr_PowerInWatts; // power

            if (ride->areDataPresent()->temp && point->temp > -200) {
                gpxtpx_atemp = doc.createElement("gpxtpx:atemp");
                gpxtpx_atemp.appendChild(doc.createTextNode(QString("%1").arg(point->temp, 0, 'f', 1)));
            }

            if (withHr && ride->areDataPresent()->hr) {
                gpxtpx_hr = doc.createElement("gpxtpx:hr");
                gpxtpx_hr.appendChild(doc.createTextNode(QString("%1").arg(point->hr, 0, 'f', 0)));
            }

            if (withCad && ride->areDataPresent()->cad && point->cad < 255) {
                gpxtpx_cad = doc.createElement("gpxtpx:cad");
                gpxtpx_cad.appendChild(doc.createTextNode(QString("%1").arg(point->cad, 0, 'f', 0)));
            }

            if (withWatts && ride->areDataPresent()->watts) {
                pwr_PowerInWatts = doc.createElement("pwr:PowerInWatts");
                pwr_PowerInWatts.setAttribute("xmlns:pwr", "http://www.garmin.com/xmlschemas/PowerExtension/v1");
                pwr_PowerInWatts.appendChild(doc.createTextNode(QString("%1").arg(point->watts, 0, 'f', 0)));
            }


            // If we have at least one from among TEMP, CAD, and HR, we need a <gpxtpx:TrackPointExtension> tag
            QDomElement gpxtpx_TrackPointExtension;
            if (!gpxtpx_atemp.isNull() || !gpxtpx_cad.isNull() || !gpxtpx_hr.isNull()) {
                gpxtpx_TrackPointExtension = doc.createElement("gpxtpx:TrackPointExtension");
                
                // These must go in this order, as per http://www8.garmin.com/xmlschemas/TrackPointExtensionv1.xsd
                if (!gpxtpx_atemp.isNull())
                    gpxtpx_TrackPointExtension.appendChild(gpxtpx_atemp);
                if (!gpxtpx_hr.isNull())
                    gpxtpx_TrackPointExtension.appendChild(gpxtpx_hr);
                if (!gpxtpx_cad.isNull())
                    gpxtpx_TrackPointExtension.appendChild(gpxtpx_cad);
            }

            // If we have gpxtpx_TrackPointExtension and/or pwr_PowerInWatts, we need an <extension> tag to hold them.
            if (! gpxtpx_TrackPointExtension.isNull() || !pwr_PowerInWatts.isNull()) {
                QDomElement extensions = doc.createElement("extensions");
                trkpt.appendChild(extensions);

                if (! gpxtpx_TrackPointExtension.isNull())
                    extensions.appendChild(gpxtpx_TrackPointExtension);
                if (!pwr_PowerInWatts.isNull())
                    extensions.appendChild(pwr_PowerInWatts);
            }
        }
    }

    return doc.toByteArray(4);
}

bool
GpxFileReader::writeRideFile(Context *context, const RideFile *ride, QFile &file) const
{
    QByteArray xml = toByteArray(context, ride, true, true, true, true);

    if (!file.open(QIODevice::WriteOnly)) return(false);
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");
    //out.setGenerateByteOrderMark(true);
    out << xml;
    out.flush();
    file.close();
    return(true);
}
