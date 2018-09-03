/*
 * Copyright (c) 2009 Eric Murray (ericm@lne.com)
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

#include "ManualRideFile.h"
#include "RideMetric.h"
#include "Units.h"
#include <QRegExp>
#include <QTextStream>
#include <algorithm> // for std::sort
#include "cmath"

static int manualFileReaderRegistered =
    RideFileFactory::instance().registerReader("man", "Manual File", new ManualFileReader());

RideFile *ManualFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    QRegExp metricUnits("(km|kph|km/h)", Qt::CaseInsensitive);
    QRegExp englishUnits("(miles|mph|mp/h)", Qt::CaseInsensitive);
    bool metric = false;

    int unitsHeader = 2;

    /*
     *  File format:
     * 	"manual"
     * 	"minutes,mph,watts,miles,hr,bikescore"  # header (metric or imperial)
     *	minutes,mph,watts,miles,hr,bikeScore    # data
     */
    QRegExp manualCSV("manual", Qt::CaseInsensitive);

    double rideSec = 0;

    if (!file.open(QFile::ReadOnly)) {
	errors << (tr("Could not open ride file: \"")
		+ file.fileName() + "\"");
	return NULL;
    }
    int lineno = 1;
    QStringList columnNames;
    QTextStream is(&file);
    RideFile *rideFile = new RideFile();
    while (!is.atEnd()) {
	// the readLine() method doesn't handle old Macintosh CR line endings
	// this workaround will load the entire file if it has CR endings
	// then split and loop through each line
	// otherwise, there will be nothing to split and it will read each line as expected.
	QString linesIn = is.readLine();
	QStringList lines = linesIn.split('\r');
	// workaround for empty lines
	if(lines.isEmpty()) {
	    lineno++;
	    continue;
	}
	for (int li = 0; li < lines.size(); ++li) {
	    QString line = lines[li];

	    if (lineno == 1) {
		if (manualCSV.indexIn(line) != -1) {
            rideFile->setDeviceType("Manual");
            rideFile->setFileFormat("Manual CSV (csv)");
		    ++lineno;
		    continue;
		}
	    }
	    else if (lineno == unitsHeader) {
		if (metricUnits.indexIn(line) != -1)
		    metric = true;
		else if (englishUnits.indexIn(line) != -1)
		    metric = false;
		else {
		    errors << (tr("Can't find units in first line: \"") + line + "\"");
		    delete rideFile;
		    file.close();
		    return NULL;
		}
                columnNames = line.split(",");
		++lineno;
		continue;
	    }
	    // minutes,kph,watts,km,hr,bikeScore
	    else if (lineno > unitsHeader) {
		double minutes=0,kph=0,watts=0,km=0,hr=0,alt=0,bs=0;
		double cad=0, nm=0;
		int interval=0;
                QStringList fields = line.split(",");
                minutes = fields[0].toDouble();
                kph = fields[1].toDouble();
                watts = fields[2].toDouble();
                km = fields[3].toDouble();
                hr = fields[4].toDouble();
                bs = fields[5].toDouble();
		if (!metric) {
		    km *= KM_PER_MILE;
		    kph *= KM_PER_MILE;
		}
                const RideMetricFactory &factory = RideMetricFactory::instance();
                for (int i = 6; i < fields.size(); ++i) {
                    if (factory.haveMetric(columnNames[i])) {
                        QMap<QString,QString> map;
                        map.insert("value", QString("%1").arg(fields[i]));
                        rideFile->metricOverrides.insert(columnNames[i], map);
                    }
                    else {
                        errors << tr("Unknown ride metric \"%1\".").arg(columnNames[i]);
                    }
                }
		cad = nm = 0.0;
		interval = 0;

		rideFile->appendPoint(minutes * 60.0, cad, hr, km,
                                      kph, nm, watts, alt,
                                      0.0, 0.0, 0.0, 0.0,
                                      RideFile::NA, RideFile::NA,
                                      0.0, 0.0, 0.0, 0.0,
                                      0.0, 0.0, // pedal platform offset
                                      0.0, 0.0, 0.0, 0.0, //pedal power phase
                                      0.0, 0.0, 0.0, 0.0, //pedal peak power phase
                                      0.0, 0.0,
                                      0.0, 0.0, 0.0, // running dynamics
                                      0.0, //tcore
                                      interval);
                QMap<QString,QString> bsm;
                bsm.insert("value", QString("%1").arg(bs));
                rideFile->metricOverrides.insert("skiba_bike_score", bsm);
                QMap<QString,QString> trm;
                trm.insert("value", QString("%1").arg(minutes * 60.0));
                rideFile->metricOverrides.insert("time_riding", trm);

		rideSec = minutes * 60.0;
	    }
	    ++lineno;
	}
    }
    // fix recording interval at ride length:
    rideFile->setRecIntSecs(rideSec);

    QRegExp rideTime("^.*/(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_"
	    "(\\d\\d)_(\\d\\d)_(\\d\\d)\\.man$");
    if (rideTime.indexIn(file.fileName()) >= 0) {
	QDateTime datetime(QDate(rideTime.cap(1).toInt(),
		    rideTime.cap(2).toInt(),
		    rideTime.cap(3).toInt()),
		QTime(rideTime.cap(4).toInt(),
		    rideTime.cap(5).toInt(),
		    rideTime.cap(6).toInt()));
	rideFile->setStartTime(datetime);
    }
    file.close();
    return rideFile;
}

