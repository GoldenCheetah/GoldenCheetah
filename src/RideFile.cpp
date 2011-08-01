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
#include "DataProcessor.h"
#include "Settings.h"
#include "Units.h"
#include <QtXml/QtXml>
#include <algorithm> // for std::lower_bound
#include <assert.h>

#define mark() \
{ \
    addInterval(start, previous->secs + recIntSecs_, \
                QString("%1").arg(interval)); \
    interval = point->interval; \
    start = point->secs; \
}

#define tr(s) QObject::tr(s)

RideFile::RideFile(const QDateTime &startTime, double recIntSecs) :
            startTime_(startTime), recIntSecs_(recIntSecs),
            deviceType_("unknown"), data(NULL)
{
    command = new RideFileCommand(this);
}

RideFile::RideFile() : recIntSecs_(0.0), deviceType_("unknown"), data(NULL)
{
    command = new RideFileCommand(this);
}

RideFile::~RideFile()
{
    foreach(RideFilePoint *point, dataPoints_)
        delete point;
    delete command;
}

QString
RideFile::seriesName(SeriesType series)
{
    switch (series) {
    case RideFile::secs: return QString(tr("Time"));
    case RideFile::cad: return QString(tr("Cadence"));
    case RideFile::hr: return QString(tr("Heartrate"));
    case RideFile::km: return QString(tr("Distance"));
    case RideFile::kph: return QString(tr("Speed"));
    case RideFile::nm: return QString(tr("Torque"));
    case RideFile::watts: return QString(tr("Power"));
    case RideFile::alt: return QString(tr("Altitude"));
    case RideFile::lon: return QString(tr("Longitude"));
    case RideFile::lat: return QString(tr("Latitude"));
    case RideFile::headwind: return QString(tr("Headwind"));
    case RideFile::interval: return QString(tr("Interval"));
    default: return QString(tr("Unknown"));
    }
}


void
RideFile::clearIntervals()
{
    intervals_.clear();
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
    if (interval > 0)
        mark();
}

struct ComparePointKm {
    bool operator()(const RideFilePoint *p1, const RideFilePoint *p2) {
        return p1->km < p2->km;
    }
};

struct ComparePointSecs {
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
        dataPoints_.begin(), dataPoints_.end(), &p, ComparePointSecs());
    if (i == dataPoints_.end())
        return dataPoints_.size();
    return i - dataPoints_.begin();
}

double
RideFile::timeToDistance(double secs) const
{
    RideFilePoint p;
    p.secs = secs;

    // Check we have some data and the secs is in bounds
    if (dataPoints_.isEmpty()) return 0;
    if (secs < dataPoints_.first()->secs) return dataPoints_.first()->km;
    if (secs > dataPoints_.last()->secs) return dataPoints_.last()->km;

    QVector<RideFilePoint*>::const_iterator i = std::lower_bound(dataPoints_.begin(), dataPoints_.end(), &p, ComparePointSecs());
    return (*i)->km;
}

int
RideFile::timeIndex(double secs) const
{
    // return index offset for specified time
    RideFilePoint p;
    p.secs = secs;

    QVector<RideFilePoint*>::const_iterator i = std::lower_bound(
        dataPoints_.begin(), dataPoints_.end(), &p, ComparePointSecs());
    if (i == dataPoints_.end())
        return dataPoints_.size();
    return i - dataPoints_.begin();
}

int
RideFile::distanceIndex(double km) const
{
    // return index offset for specified distance in km
    RideFilePoint p;
    p.km = km;

    QVector<RideFilePoint*>::const_iterator i = std::lower_bound(
        dataPoints_.begin(), dataPoints_.end(), &p, ComparePointKm());
    if (i == dataPoints_.end())
        return dataPoints_.size();
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
    return QRegExp(s.arg(suffixList.join("|")), Qt::CaseInsensitive);
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

    // NULL returned to indicate openRide failed
    if (result) {
        if (result->intervals().empty()) result->fillInIntervals();

        // override the file ride time with that set from the filename
        // but only if it matches the GC format
        QFileInfo fileInfo(file.fileName());
        QRegExp rx ("^((\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d))\\.(.+)$");

        if (rx.exactMatch(fileInfo.fileName())) {

            QDate date(rx.cap(2).toInt(), rx.cap(3).toInt(),rx.cap(4).toInt());
            QTime time(rx.cap(5).toInt(), rx.cap(6).toInt(),rx.cap(7).toInt());
            QDateTime datetime(date, time);
            result->setStartTime(datetime);
        }

        result->setTag("Filename", file.fileName());
        result->setTag("Athlete", QFileInfo(file).dir().dirName());
        DataProcessorFactory::instance().autoProcess(result);
    }

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
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    QVariant isAscending = settings->value(GC_ALLRIDES_ASCENDING,Qt::Checked);
    QFlags<QDir::Filter> spec = QDir::Files;
#ifdef Q_OS_WIN32
    spec |= QDir::Hidden;
#endif
    if(isAscending.toInt()>0){
        return dir.entryList(filters, spec, QDir::Name);
    }
    return dir.entryList(filters, spec, QDir::Name|QDir::Reversed);
}

void RideFile::appendPoint(double secs, double cad, double hr, double km,
                           double kph, double nm, double watts, double alt,
                           double lon, double lat, double headwind, int interval)
{
    // negative values are not good, make them zero
    // although alt, lat, lon, headwind can be negative of course!
    if (!isfinite(secs) || secs<0) secs=0;
    if (!isfinite(cad) || cad<0) cad=0;
    if (!isfinite(hr) || hr<0) hr=0;
    if (!isfinite(km) || km<0) km=0;
    if (!isfinite(kph) || kph<0) kph=0;
    if (!isfinite(nm) || nm<0) nm=0;
    if (!isfinite(watts) || watts<0) watts=0;
    if (!isfinite(interval) || interval<0) interval=0;

    dataPoints_.append(new RideFilePoint(secs, cad, hr, km, kph,
                                         nm, watts, alt, lon, lat, headwind, interval));
    dataPresent.secs  |= (secs != 0);
    dataPresent.cad   |= (cad != 0);
    dataPresent.hr    |= (hr != 0);
    dataPresent.km    |= (km != 0);
    dataPresent.kph   |= (kph != 0);
    dataPresent.nm    |= (nm != 0);
    dataPresent.watts |= (watts != 0);
    dataPresent.alt   |= (alt != 0);
    dataPresent.lon   |= (lon != 0);
    dataPresent.lat   |= (lat != 0);
    dataPresent.headwind |= (headwind != 0);
    dataPresent.interval |= (interval != 0);
}

void
RideFile::setDataPresent(SeriesType series, bool value)
{
    switch (series) {
        case secs : dataPresent.secs = value; break;
        case cad : dataPresent.cad = value; break;
        case hr : dataPresent.hr = value; break;
        case km : dataPresent.km = value; break;
        case kph : dataPresent.kph = value; break;
        case nm : dataPresent.nm = value; break;
        case watts : dataPresent.watts = value; break;
        case alt : dataPresent.alt = value; break;
        case lon : dataPresent.lon = value; break;
        case lat : dataPresent.lat = value; break;
        case headwind : dataPresent.headwind = value; break;
        case interval : dataPresent.interval = value; break;
        case none : break;
    }
}

bool
RideFile::isDataPresent(SeriesType series)
{
    switch (series) {
        case secs : return dataPresent.secs; break;
        case cad : return dataPresent.cad; break;
        case hr : return dataPresent.hr; break;
        case km : return dataPresent.km; break;
        case kph : return dataPresent.kph; break;
        case nm : return dataPresent.nm; break;
        case watts : return dataPresent.watts; break;
        case alt : return dataPresent.alt; break;
        case lon : return dataPresent.lon; break;
        case lat : return dataPresent.lat; break;
        case headwind : return dataPresent.headwind; break;
        case interval : return dataPresent.interval; break;
        case none : break;
    }
    return false;
}
void
RideFile::setPointValue(int index, SeriesType series, double value)
{
    switch (series) {
        case secs : dataPoints_[index]->secs = value; break;
        case cad : dataPoints_[index]->cad = value; break;
        case hr : dataPoints_[index]->hr = value; break;
        case km : dataPoints_[index]->km = value; break;
        case kph : dataPoints_[index]->kph = value; break;
        case nm : dataPoints_[index]->nm = value; break;
        case watts : dataPoints_[index]->watts = value; break;
        case alt : dataPoints_[index]->alt = value; break;
        case lon : dataPoints_[index]->lon = value; break;
        case lat : dataPoints_[index]->lat = value; break;
        case headwind : dataPoints_[index]->headwind = value; break;
        case interval : dataPoints_[index]->interval = value; break;
        case none : break;
    }
}

double
RideFile::getPointValue(int index, SeriesType series)
{
    switch (series) {
        case secs : return dataPoints_[index]->secs; break;
        case cad : return dataPoints_[index]->cad; break;
        case hr : return dataPoints_[index]->hr; break;
        case km : return dataPoints_[index]->km; break;
        case kph : return dataPoints_[index]->kph; break;
        case nm : return dataPoints_[index]->nm; break;
        case watts : return dataPoints_[index]->watts; break;
        case alt : return dataPoints_[index]->alt; break;
        case lon : return dataPoints_[index]->lon; break;
        case lat : return dataPoints_[index]->lat; break;
        case headwind : return dataPoints_[index]->headwind; break;
        case interval : return dataPoints_[index]->interval; break;
        case none : break;
    }
    return 0.0; // shutup the compiler
}

int
RideFile::decimalsFor(SeriesType series)
{
    switch (series) {
        case secs : return 3; break;
        case cad : return 0; break;
        case hr : return 0; break;
        case km : return 6; break;
        case kph : return 4; break;
        case nm : return 2; break;
        case watts : return 0; break;
        case alt : return 3; break;
        case lon : return 6; break;
        case lat : return 6; break;
        case headwind : return 4; break;
        case interval : return 0; break;
        case none : break;
    }
    return 2; // default
}

double
RideFile::maximumFor(SeriesType series)
{
    switch (series) {
        case secs : return 999999; break;
        case cad : return 300; break;
        case hr : return 300; break;
        case km : return 999999; break;
        case kph : return 999; break;
        case nm : return 999; break;
        case watts : return 4000; break;
        case alt : return 8850; break; // mt everest is highest point above sea level
        case lon : return 180; break;
        case lat : return 90; break;
        case headwind : return 999; break;
        case interval : return 999; break;
        case none : break;
    }
    return 9999; // default
}

double
RideFile::minimumFor(SeriesType series)
{
    switch (series) {
        case secs : return 0; break;
        case cad : return 0; break;
        case hr : return 0; break;
        case km : return 0; break;
        case kph : return 0; break;
        case nm : return 0; break;
        case watts : return 0; break;
        case alt : return -413; break; // the Red Sea is lowest land point on earth
        case lon : return -180; break;
        case lat : return -90; break;
        case headwind : return -999; break;
        case interval : return 0; break;
        case none : break;
    }
    return 0; // default
}

void
RideFile::deletePoint(int index)
{
    delete dataPoints_[index];
    dataPoints_.remove(index);
}

void
RideFile::deletePoints(int index, int count)
{
    for(int i=index; i<(index+count); i++) delete dataPoints_[i];
    dataPoints_.remove(index, count);
}

void
RideFile::insertPoint(int index, RideFilePoint *point)
{
    dataPoints_.insert(index, point);
}

void
RideFile::appendPoints(QVector <struct RideFilePoint *> newRows)
{
    dataPoints_ += newRows;
}

void
RideFile::emitSaved()
{
    emit saved();
}

void
RideFile::emitReverted()
{
    emit reverted();
}

void
RideFile::emitModified()
{
    emit modified();
}
