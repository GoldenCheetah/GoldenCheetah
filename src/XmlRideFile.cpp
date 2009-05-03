/* 
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net), 
 *                    Justin F. Knotzke (jknotzke@shampoo.ca)
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

#include "XmlRideFile.h"
#include <QRegExp>
#include <QtXml/QtXml>
#include <QVector>
#include <assert.h>

static int xmlFileReaderRegistered = 
    RideFileFactory::instance().registerReader("xml", new XmlFileReader());

struct Interval {
    double from_secs, thru_secs;
    int number;
    Interval() : from_secs(0.0), thru_secs(0.0), number(0) {}
    Interval(double f, double t, int n)
        : from_secs(f), thru_secs(t), number(n) {}
};

RideFile *XmlFileReader::openRideFile(QFile &file, QStringList &errors) const 
{
    QVector<Interval> intervals;
    if (!file.open(QFile::ReadOnly)) {
        errors << ("Could not open ride file: \"" + file.fileName() + "\"");
        return NULL;
    }
    QDomDocument doc("GoldenCheetah-1.0");
    QString errMsg;
    int errLine, errCol;
    if (!doc.setContent(&file, false, &errMsg, &errLine, &errCol)) {
        errors << (QString("xml parsing error line %1, col %2: ").arg(errLine).arg(errCol) + errMsg);
        return NULL;
    }
    file.close();
    QDomElement xride = doc.documentElement();
    if (xride.tagName() != "ride") {
        errors << "root should be <ride>";
        return NULL;
    }
    RideFile *rideFile = new RideFile();
    QRegExp re("^.*/(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_"
               "(\\d\\d)_(\\d\\d)_(\\d\\d)\\.xml$");
    if (re.indexIn(file.fileName()) >= 0) {
        QDateTime dt(QDate(re.cap(1).toInt(), re.cap(2).toInt(), re.cap(3).toInt()),
                     QTime(re.cap(4).toInt(), re.cap(5).toInt(), re.cap(6).toInt()));
        rideFile->setStartTime(dt);
    }
    QDomNode n = xride.firstChild();
    while (!n.isNull()) {
        QDomElement e = n.toElement();
        if (!e.isNull()) {
            if (e.tagName() == "header") {
                QDomNode m = e.firstChild();
                while (!m.isNull()) {
                    QDomElement f = m.toElement();
                    if (f.tagName() == "start_time") {
                        QRegExp re("^ *(\\d\\d\\d\\d)/(\\d\\d)/(\\d\\d) "
                                   "(\\d\\d):(\\d\\d):(\\d\\d) *$");
                        if (re.indexIn(f.text()) < 0) {
                            errors << ("can't parse start time \"" + f.text() + "\"");
                        }
                        else {
                            QDateTime dt(QDate(re.cap(1).toInt(), re.cap(2).toInt(), re.cap(3).toInt()),
                                         QTime(re.cap(4).toInt(), re.cap(5).toInt(), re.cap(6).toInt()));
                            if (dt != rideFile->startTime()) {
                                errors << "encoded start time differs from file name";
                            }
                        }
                    }
                    else if (f.tagName() == "rec_int_secs") {
                        rideFile->setRecIntSecs(f.text().toDouble());
                    }
                    else if (f.tagName() == "device_type") {
                        rideFile->setDeviceType(f.text());
                    }
                    else {
                        errors << ("unexpected element <" + e.tagName() + ">");
                    }
                    m = m.nextSibling();
                }
            }
            else if (e.tagName() == "intervals") {
                QDomNode m = e.firstChild();
                while (!m.isNull()) {
                    QDomElement f = m.toElement();
                    if (f.tagName() == "interval") {
                        double from_secs = f.attribute("from_secs", "0.0").toDouble();
                        double thru_secs = f.attribute("thru_secs", "0.0").toDouble();
                        int number = intervals.size();
                        intervals.append(Interval(from_secs, thru_secs, number));
                    }
                    else {
                        errors << ("unexpected element <" + e.tagName() + ">");
                    }
                    m = m.nextSibling();
                }
            }
            else if (e.tagName() == "samples") {
                QDomNode m = e.firstChild();
                while (!m.isNull()) {
                    QDomElement f = m.toElement();
                    if (f.tagName() == "sample") {
                        double cad = f.attribute("cad", "0.0").toDouble();
                        double hr = f.attribute("hr", "0.0").toDouble();
                        double km = f.attribute("km", "0.0").toDouble();
                        double kph = f.attribute("kph", "0.0").toDouble();
                        double nm = f.attribute("nm", "0.0").toDouble();
                        double secs = f.attribute("secs", "0.0").toDouble();
                        double watts = f.attribute("watts", "0.0").toDouble();
                        int interval = 0;
                        for (int i = 0; i < intervals.size(); ++i) {
                            if ((secs >= intervals[i].from_secs) 
                                && (secs <= intervals[i].thru_secs)) {
                                interval = intervals[i].number;
                                break;
                            }
                        }
                        rideFile->appendPoint(secs, cad, hr, km, kph, nm, watts, interval);
                    }
                    else {
                        errors << ("unexpected element <" + e.tagName() + ">");
                    }
                    m = m.nextSibling();
                }
            }
            else {
                errors << ("unexpected element <" + e.tagName() + ">");
            }
        }
        n = n.nextSibling();
    }
    return rideFile;
}

