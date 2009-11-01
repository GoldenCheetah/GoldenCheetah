/*
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net)
 *               2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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
#include "Settings.h"
#include "Units.h"
#include <QtXml/QtXml>
#include <algorithm> // for std::lower_bound
#include <assert.h>

#define mark() \
{ \
    addInterval(start, previous->secs + recIntSecs_, \
                QString("%1").arg(interval)); \
    ++interval; \
    start = point->secs; \
}

void
RideFile::fillInIntervals()
{
    if (dataPoints_.empty())
        return;
    intervals_.clear();
    double start = 0.0;
    int interval = dataPoints().first()->interval;
    const RideFilePoint *point, *previous;
    foreach (point, dataPoints()) {
        if (point->interval != interval)
            mark();
        previous = point;
    }
    mark();
}

struct ComparePoints {
    bool operator()(const RideFilePoint *p1, const RideFilePoint *p2) {
        return p1->secs < p2->secs;
    }
};

int
RideFile::intervalBegin(const RideFileInterval &interval) const
{
    RideFilePoint p;
    p.secs = interval.start;
    QVector<RideFilePoint*>::const_iterator i = std::lower_bound(
        dataPoints_.begin(), dataPoints_.end(), &p, ComparePoints());
    assert(i != dataPoints_.end());
    return i - dataPoints_.begin();
}

void RideFile::writeAsCsv(QFile &file, bool bIsMetric) const
{

    // Use the column headers that make WKO+ happy.
    double convertUnit;
    QTextStream out(&file);
    if (!bIsMetric)
    {
        out << "Minutes,Torq (N-m),MPH,Watts,Miles,Cadence,Hrate,ID,Altitude (feet)\n";
        convertUnit = MILES_PER_KM;
    }
    else {
        out << "Minutes,Torq (N-m),Km/h,Watts,Km,Cadence,Hrate,ID,Altitude (m)\n";
        convertUnit = 1.0;
    }

    foreach (const RideFilePoint *point, dataPoints()) {
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
        out << ",";
        out << point->alt;
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
                                    const QString &description,
                                       RideFileReader *reader)
{
    assert(!readFuncs_.contains(suffix));
    readFuncs_.insert(suffix, reader);
    descriptions_.insert(suffix, description);
    return 1;
}

QStringList RideFileFactory::suffixes() const
{
    return readFuncs_.keys();
}

QRegExp
RideFileFactory::rideFileRegExp() const
{
    QStringList suffixList = RideFileFactory::instance().suffixes();
    QString s("^(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)\\.(%1)$");
    return QRegExp(s.arg(suffixList.join("|")));
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
    RideFile *result = reader->openRideFile(file, errors);
    if (result && result->intervals().empty())
        result->fillInIntervals();
    return result;
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
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();;

    QVariant isAscending = settings->value(GC_ALLRIDES_ASCENDING,Qt::Checked);
    if(isAscending.toInt()>0){
        return dir.entryList(filters, QDir::Files, QDir::Name);
    }
    return dir.entryList(filters, QDir::Files, QDir::Name|QDir::Reversed);
}

void RideFile::appendPoint(double secs, double cad, double hr, double km,
                           double kph, double nm, double watts, double alt,
                           int interval, double bs)
{
    dataPoints_.append(new RideFilePoint(secs, cad, hr, km, kph,
                                         nm, watts, alt, interval,bs));
    dataPresent.secs  |= (secs != 0);
    dataPresent.cad   |= (cad != 0);
    dataPresent.hr    |= (hr != 0);
    dataPresent.km    |= (km != 0);
    dataPresent.kph   |= (kph != 0);
    dataPresent.nm    |= (nm != 0);
    dataPresent.watts |= (watts != 0);
    dataPresent.alt   |= (alt != 0);
    dataPresent.interval |= (interval != 0);
}
