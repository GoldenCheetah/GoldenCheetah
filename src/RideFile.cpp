/* 
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net)
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

#include "RideFile.h"
#include <QtXml/QtXml>
#include <assert.h>
#include "Settings.h"

static void
markInterval(QDomDocument &doc, QDomNode &xride, QDomNode &xintervals,
             double &startSecs, double prevSecs,
             int &thisInterval, RideFilePoint *sample)
{
    if (xintervals.isNull()) {
        xintervals = doc.createElement("intervals");
        xride.appendChild(xintervals);
    }
    QDomElement xint = doc.createElement("interval").toElement();
    xintervals.appendChild(xint);
    xint.setAttribute("name", thisInterval);
    xint.setAttribute("from_secs", QString("%1").arg(startSecs, 0, 'f', 2));
    xint.setAttribute("thru_secs", QString("%1").arg(prevSecs, 0, 'f', 2));
    startSecs = sample->secs;
    thisInterval = sample->interval;
}

static void
append_text(QDomDocument &doc, QDomNode &parent,
            const QString &child_name, const QString &child_value)
{
    QDomNode child = parent.appendChild(doc.createElement(child_name));
    child.appendChild(doc.createTextNode(child_value));
}

bool
RideFile::writeAsXml(QFile &file, QString &err) const
{
    (void) err;
    QDomDocument doc("GoldenCheetah-1.0");
    QDomNode xride = doc.appendChild(doc.createElement("ride"));
    QDomNode xheader = xride.appendChild(doc.createElement("header"));
    append_text(doc, xheader, "start_time", startTime_.toString("yyyy/MM/dd hh:mm:ss"));
    append_text(doc, xheader, "device_type", deviceType_);
    append_text(doc, xheader, "rec_int_secs", QString("%1").arg(recIntSecs_, 0, 'f', 3));
    QDomNode xintervals;
    bool hasNm = false;
    double startSecs = 0.0, prevSecs = 0.0;
    int thisInterval = 0;
    QListIterator<RideFilePoint*> i(dataPoints_);
    RideFilePoint *sample = NULL;
    while (i.hasNext()) {
        sample = i.next();
        if (sample->nm > 0.0)
            hasNm = true;
        assert(sample->secs >= 0.0);
        if (sample->interval != thisInterval) {
            markInterval(doc, xride, xintervals, startSecs,
                         prevSecs, thisInterval, sample);
        }
        prevSecs = sample->secs;
    }
    if (sample) {
        markInterval(doc, xride, xintervals, startSecs,
                     prevSecs, thisInterval, sample);
    }
    QDomNode xsamples = doc.createElement("samples");
    xride.appendChild(xsamples);
    i.toFront();
    while (i.hasNext()) {
        RideFilePoint *sample = i.next();
        QDomElement xsamp = doc.createElement("sample").toElement();
        xsamples.appendChild(xsamp);
        xsamp.setAttribute("secs", QString("%1").arg(sample->secs, 0, 'f', 2));
        xsamp.setAttribute("cad", QString("%1").arg(sample->cad, 0, 'f', 0));
        xsamp.setAttribute("hr", QString("%1").arg(sample->hr, 0, 'f', 0));
        xsamp.setAttribute("km", QString("%1").arg(sample->km, 0, 'f', 3));
        xsamp.setAttribute("kph", QString("%1").arg(sample->kph, 0, 'f', 1));
        xsamp.setAttribute("watts", sample->watts);
        if (hasNm) {
            double nm = (sample->watts > 0.0) ? sample->nm : 0.0;
            xsamp.setAttribute("nm", QString("%1").arg(nm, 0,'f', 1));
        }
    }
    file.open(QFile::WriteOnly);
    QTextStream ts(&file);
    doc.save(ts, 4);
    file.close();
    return true;
}

void RideFile::writeAsCsv(QFile &file, bool bIsMetric) const
{

    // Use the column headers that make WKO+ happy.
    double convertUnit;
    QTextStream out(&file);
    if (!bIsMetric)
    {
        out << "Minutes,Torq (N-m),MPH,Watts,Miles,Cadence,Hrate,ID\n";
        const double MILES_PER_KM = 0.62137119;
        convertUnit = MILES_PER_KM;
    }
    else {
        out << "Minutes,Torq (N-m),Km/h,Watts,Km,Cadence,Hrate,ID\n";
        // TODO: use KM_TO_MI from lib/pt.c instead?
        convertUnit = 1.0;
    }

    QListIterator<RideFilePoint*> i(dataPoints());
    while (i.hasNext()) {
        RideFilePoint *point = i.next();
        if (point->secs == 0.0)
            continue;
        out << point->secs/60.0;
        out << ",";
        out << ((point->nm >= 0) ? point->nm : 0.0);
        out << ",";
        out << ((point->kph >= 0) ? (point->kph * convertUnit) : 0.0);
        out << ",";
        out << ((point->watts >= 0) ? point->watts : 0.0);
        out << ",";
        out << point->km * convertUnit;
        out << ",";
        out << point->cad;
        out << ",";
        out << point->hr;
        out << ",";
        out << point->interval;
        if (point->bs > 0.0) {
        	out << ",";
		out << point->bs;
	}
	out << "\n";
    }

    file.close();
}

RideFileFactory *RideFileFactory::instance_;

RideFileFactory &RideFileFactory::instance() 
{ 
    if (!instance_) 
        instance_ = new RideFileFactory();
    return *instance_;
}

int RideFileFactory::registerReader(const QString &suffix, 
                                       RideFileReader *reader) 
{
    assert(!readFuncs_.contains(suffix));
    readFuncs_.insert(suffix, reader);
    return 1;
}

RideFile *RideFileFactory::openRideFile(QFile &file, 
                                           QStringList &errors) const 
{
    QString suffix = file.fileName();
    int dot = suffix.lastIndexOf(".");
    assert(dot >= 0);
    suffix.remove(0, dot + 1);
    RideFileReader *reader = readFuncs_.value(suffix.toLower());
    assert(reader);
    return reader->openRideFile(file, errors);
}

QStringList RideFileFactory::listRideFiles(const QDir &dir) const 
{
    QStringList filters;
    QMapIterator<QString,RideFileReader*> i(readFuncs_);
    while (i.hasNext()) {
        i.next();
        filters << ("*." + i.key());
    }
    // This will read the user preferences and change the file list order as necessary:
    QSettings settings(GC_SETTINGS_CO, GC_SETTINGS_APP);
    QVariant isAscending = settings.value(GC_ALLRIDES_ASCENDING,Qt::Checked);
    if(isAscending.toInt()>0){
        return dir.entryList(filters, QDir::Files, QDir::Name);
    }
    return dir.entryList(filters, QDir::Files, QDir::Name|QDir::Reversed);
}

void RideFile::appendPoint(double secs, double cad, double hr, double km, 
		      double kph, double nm, double watts, int interval,
			double bs)
{
    dataPoints_.append(new RideFilePoint(secs, cad, hr, km, kph, 
		nm, watts, interval,bs));
    dataPresent.secs  |= (secs != 0);
    dataPresent.cad   |= (cad != 0);
    dataPresent.hr    |= (hr != 0);
    dataPresent.km    |= (km != 0);
    dataPresent.kph   |= (kph != 0);
    dataPresent.nm    |= (nm != 0);
    dataPresent.watts |= (watts != 0);
    dataPresent.interval |= (interval != 0);
}

void
RideFile::appendPoint(double secs, double cad, double hr, double km, 
                      double kph, double nm, double watts, int interval)
{
    dataPoints_.append(new RideFilePoint(secs, cad, hr, km, kph, 
                                         nm, watts, interval,0.0));
    dataPresent.secs  |= (secs != 0);
    dataPresent.cad   |= (cad != 0);
    dataPresent.hr    |= (hr != 0);
    dataPresent.km    |= (km != 0);
    dataPresent.kph   |= (kph != 0);
    dataPresent.nm    |= (nm != 0);
    dataPresent.watts |= (watts != 0);
    dataPresent.interval |= (interval != 0);
}
