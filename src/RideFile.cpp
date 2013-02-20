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
#include "RideEditor.h"
#include "RideMetadata.h"
#include "MetricAggregator.h"
#include "SummaryMetrics.h"
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

RideFile::RideFile(const QDateTime &startTime, double recIntSecs) :
            startTime_(startTime), recIntSecs_(recIntSecs),
            deviceType_("unknown"), data(NULL), weight_(0)
{
    command = new RideFileCommand(this);
}

RideFile::RideFile() : recIntSecs_(0.0), deviceType_("unknown"), data(NULL), weight_(0)
{
    command = new RideFileCommand(this);
}

RideFile::~RideFile()
{
    emit deleted();
    foreach(RideFilePoint *point, dataPoints_)
        delete point;
    delete command;
    if (data) delete data;
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
    case RideFile::xPower: return QString(tr("xPower"));
    case RideFile::NP: return QString(tr("Normalized Power"));
    case RideFile::alt: return QString(tr("Altitude"));
    case RideFile::lon: return QString(tr("Longitude"));
    case RideFile::lat: return QString(tr("Latitude"));
    case RideFile::headwind: return QString(tr("Headwind"));
    case RideFile::slope: return QString(tr("Slope"));
    case RideFile::temp: return QString(tr("Temperature"));
    case RideFile::lrbalance: return QString(tr("Left/Right Balance"));
    case RideFile::interval: return QString(tr("Interval"));
    case RideFile::vam: return QString(tr("VAM"));
    case RideFile::wattsKg: return QString(tr("Watts per Kilogram"));
    default: return QString(tr("Unknown"));
    }
}

QString
RideFile::unitName(SeriesType series, MainWindow *main)
{
    bool useMetricUnits = main->useMetricUnits;

    switch (series) {
    case RideFile::secs: return QString(tr("seconds"));
    case RideFile::cad: return QString(tr("rpm"));
    case RideFile::hr: return QString(tr("bpm"));
    case RideFile::km: return QString(useMetricUnits ? tr("km") : tr("miles"));
    case RideFile::kph: return QString(useMetricUnits ? tr("kph") : tr("mph"));
    case RideFile::nm: return QString(tr("N"));
    case RideFile::watts: return QString(tr("watts"));
    case RideFile::xPower: return QString(tr("watts"));
    case RideFile::NP: return QString(tr("watts"));
    case RideFile::alt: return QString(useMetricUnits ? tr("metres") : tr("feet"));
    case RideFile::lon: return QString(tr("lon"));
    case RideFile::lat: return QString(tr("lat"));
    case RideFile::headwind: return QString(tr("kph"));
    case RideFile::slope: return QString(tr("%"));
    case RideFile::temp: return QString(tr("°C"));
    case RideFile::lrbalance: return QString(tr("%"));
    case RideFile::interval: return QString(tr("Interval"));
    case RideFile::vam: return QString(tr("meters per hour"));
    case RideFile::wattsKg: return QString(useMetricUnits ? tr("watts/kg") : tr("watts/lb"));
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
    const RideFilePoint *point=NULL, *previous=NULL;
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
        return dataPoints_.size()-1;
    int offset = i - dataPoints_.begin();
    if (offset > dataPoints_.size()) return dataPoints_.size()-1;
    else if (offset <0) return 0;
    else return offset;
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
        return dataPoints_.size()-1;
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
        return dataPoints_.size()-1;
    return i - dataPoints_.begin();
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

QStringList RideFileFactory::writeSuffixes() const
{
    QStringList returning;
    QMapIterator<QString,RideFileReader*> i(readFuncs_);
    while (i.hasNext()) {
        i.next();
        if (i.value()->hasWrite()) returning << i.key();
    }
    return returning;
}

QRegExp
RideFileFactory::rideFileRegExp() const
{
    QStringList suffixList = RideFileFactory::instance().suffixes();
    QString s("^(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)\\.(%1)$");
    return QRegExp(s.arg(suffixList.join("|")), Qt::CaseInsensitive);
}

bool
RideFileFactory::writeRideFile(MainWindow *main, const RideFile *ride, QFile &file, QString format) const
{
    // get the ride file writer for this format
    RideFileReader *reader = readFuncs_.value(format.toLower());

    // write away
    if (!reader) return false;
    else return reader->writeRideFile(main, ride, file);
}

RideFile *RideFileFactory::openRideFile(MainWindow *main, QFile &file,
                                           QStringList &errors, QList<RideFile*> *rideList) const
{
    QString suffix = file.fileName();
    int dot = suffix.lastIndexOf(".");
    assert(dot >= 0);
    suffix.remove(0, dot + 1);
    RideFileReader *reader = readFuncs_.value(suffix.toLower());
    assert(reader);
//qDebug()<<"open"<<file.fileName()<<"start:"<<QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    RideFile *result = reader->openRideFile(file, errors, rideList);
//qDebug()<<"open"<<file.fileName()<<"end:"<<QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    // NULL returned to indicate openRide failed
    if (result) {
        result->mainwindow = main;
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

        // legacy support for .notes file
        QString notesFileName = fileInfo.absolutePath() + '/' + fileInfo.baseName() + ".notes";
        QFile notesFile(notesFileName);

        // read it in if it exists and "Notes" is not already set
        if (result->getTag("Notes", "") == "" && notesFile.exists() &&
            notesFile.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream in(&notesFile);
            result->setTag("Notes", in.readAll());
            notesFile.close();
        }

        // Construct the summary text used on the calendar
        QString calendarText;
        foreach (FieldDefinition field, main->rideMetadata()->getFields()) {
            if (field.diary == true && result->getTag(field.name, "") != "") {
                calendarText += QString("%1\n")
                        .arg(result->getTag(field.name, ""));
            }
        }
        result->setTag("Calendar Text", calendarText);

        // set other "special" fields
        result->setTag("Filename", QFileInfo(file.fileName()).fileName());
        result->setTag("Device", result->deviceType());
        result->setTag("File Format", result->fileFormat());
        result->setTag("Athlete", QFileInfo(file).dir().dirName());
        result->setTag("Year", result->startTime().toString("yyyy"));
        result->setTag("Month", result->startTime().toString("MMMM"));
        result->setTag("Weekday", result->startTime().toString("ddd"));

        DataProcessorFactory::instance().autoProcess(result);

        // what data is present - after processor in case 'derived' or adjusted
        QString flags;

        if (result->areDataPresent()->secs) flags += 'T'; // time
        else flags += '-';
        if (result->areDataPresent()->km) flags += 'D'; // distance
        else flags += '-';
        if (result->areDataPresent()->kph) flags += 'S'; // speed
        else flags += '-';
        if (result->areDataPresent()->watts) flags += 'P'; // Power
        else flags += '-';
        if (result->areDataPresent()->hr) flags += 'H'; // Heartrate
        else flags += '-';
        if (result->areDataPresent()->cad) flags += 'C'; // cadence
        else flags += '-';
        if (result->areDataPresent()->nm) flags += 'N'; // Torque
        else flags += '-';
        if (result->areDataPresent()->alt) flags += 'A'; // Altitude
        else flags += '-';
        if (result->areDataPresent()->lat ||
            result->areDataPresent()->lon ) flags += 'G'; // GPS
        else flags += '-';
        if (result->areDataPresent()->headwind) flags += 'W'; // Windspeed
        else flags += '-';
        if (result->areDataPresent()->temp) flags += 'E'; // Temperature
        else flags += '-';
        if (result->areDataPresent()->lrbalance) flags += 'B'; // Left/Right Balance, TODO Walibu, unsure about this flag? 'B' ok?
        else flags += '-';
        result->setTag("Data", flags);

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
    QFlags<QDir::Filter> spec = QDir::Files;
#ifdef Q_OS_WIN32
    spec |= QDir::Hidden;
#endif
    return dir.entryList(filters, spec, QDir::Name);
}

void RideFile::appendPoint(double secs, double cad, double hr, double km,
                           double kph, double nm, double watts, double alt,
                           double lon, double lat, double headwind,
                           double slope, double temp, double lrbalance, int interval)
{
    // negative values are not good, make them zero
    // although alt, lat, lon, headwind, slope and temperature can be negative of course!
    if (!isfinite(secs) || secs<0) secs=0;
    if (!isfinite(cad) || cad<0) cad=0;
    if (!isfinite(hr) || hr<0) hr=0;
    if (!isfinite(km) || km<0) km=0;
    if (!isfinite(kph) || kph<0) kph=0;
    if (!isfinite(nm) || nm<0) nm=0;
    if (!isfinite(watts) || watts<0) watts=0;
    if (!isfinite(interval) || interval<0) interval=0;

    dataPoints_.append(new RideFilePoint(secs, cad, hr, km, kph,
                                         nm, watts, alt, lon, lat, headwind, slope, temp, lrbalance, interval));
    dataPresent.secs     |= (secs != 0);
    dataPresent.cad      |= (cad != 0);
    dataPresent.hr       |= (hr != 0);
    dataPresent.km       |= (km != 0);
    dataPresent.kph      |= (kph != 0);
    dataPresent.nm       |= (nm != 0);
    dataPresent.watts    |= (watts != 0);
    dataPresent.alt      |= (alt != 0);
    dataPresent.lon      |= (lon != 0);
    dataPresent.lat      |= (lat != 0);
    dataPresent.headwind |= (headwind != 0);
    dataPresent.slope    |= (slope != 0);
    dataPresent.temp     |= (temp != noTemp);
    dataPresent.lrbalance|= (lrbalance != 0);
    dataPresent.interval |= (interval != 0);
}

void RideFile::appendPoint(const RideFilePoint &point)
{
    dataPoints_.append(new RideFilePoint(point.secs,point.cad,point.hr,point.km,point.kph,point.nm,point.watts,point.alt,point.lon,point.lat,
                                         point.headwind, point.slope, point.temp, point.lrbalance, point.interval));
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
        case slope : dataPresent.slope = value; break;
        case temp : dataPresent.temp = value; break;
        case lrbalance : dataPresent.lrbalance = value; break;
        case interval : dataPresent.interval = value; break;
        default:
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
        case slope : return dataPresent.slope; break;
        case temp : return dataPresent.temp; break;
        case lrbalance : return dataPresent.lrbalance; break;
        case interval : return dataPresent.interval; break;
        default:
        case none : return false; break;
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
        case slope : dataPoints_[index]->slope = value; break;
        case temp : dataPoints_[index]->temp = value; break;
        case lrbalance : dataPoints_[index]->lrbalance = value; break;
        case interval : dataPoints_[index]->interval = value; break;
        default:
        case none : break;
    }
}

double
RideFilePoint::value(RideFile::SeriesType series) const
{
    switch (series) {
        case RideFile::secs : return secs; break;
        case RideFile::cad : return cad; break;
        case RideFile::hr : return hr; break;
        case RideFile::km : return km; break;
        case RideFile::kph : return kph; break;
        case RideFile::nm : return nm; break;
        case RideFile::watts : return watts; break;
        case RideFile::alt : return alt; break;
        case RideFile::lon : return lon; break;
        case RideFile::lat : return lat; break;
        case RideFile::headwind : return headwind; break;
        case RideFile::slope : return slope; break;
        case RideFile::temp : return temp; break;
        case RideFile::lrbalance : return lrbalance; break;
        case RideFile::interval : return interval; break;

        default:
        case RideFile::none : break;
    }
    return 0.0;
}

double
RideFile::getPointValue(int index, SeriesType series) const
{
    return dataPoints_[index]->value(series);
}

QVariant
RideFile::getPoint(int index, SeriesType series) const
{
    double value = getPointValue(index, series);
    if (series==RideFile::temp && value == RideFile::noTemp)
        return "";
    else if (series==RideFile::wattsKg)
        return "";
    return value;
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
        case xPower : return 0; break;
        case NP : return 0; break;
        case alt : return 3; break;
        case lon : return 6; break;
        case lat : return 6; break;
        case headwind : return 4; break;
        case slope : return 1; break;
        case temp : return 1; break;
        case interval : return 0; break;
        case vam : return 0; break;
        case wattsKg : return 2; break;
        case lrbalance : return 1; break;
        case none : break;
    }
    return 2; // default
}

double
RideFile::maximumFor(SeriesType series)
{
    switch (series) {
        case secs : return 999999; break;
        case cad : return 255; break;
        case hr : return 255; break;
        case km : return 999999; break;
        case kph : return 150; break;
        case nm : return 100; break;
        case watts : return 2500; break;
        case NP : return 2500; break;
        case xPower : return 2500; break;
        case alt : return 8850; break; // mt everest is highest point above sea level
        case lon : return 180; break;
        case lat : return 90; break;
        case headwind : return 999; break;
        case slope : return 100; break;
        case temp : return 100; break;
        case interval : return 999; break;
        case vam : return 9999; break;
        case wattsKg : return 50; break;
        case lrbalance : return 100; break;
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
        case xPower : return 0; break;
        case NP : return 0; break;
        case alt : return -413; break; // the Red Sea is lowest land point on earth
        case lon : return -180; break;
        case lat : return -90; break;
        case headwind : return -999; break;
        case slope : return -100; break;
        case temp : return -100; break;
        case interval : return 0; break;
        case vam : return 0; break;
        case wattsKg : return 0; break;
        case lrbalance : return 0; break;
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
    weight_ = 0;
    emit saved();
}

void
RideFile::emitReverted()
{
    weight_ = 0;
    emit reverted();
}

void
RideFile::emitModified()
{
    weight_ = 0;
    emit modified();
}

double
RideFile::getWeight()
{
    if (weight_) return weight_; // cached value

    // ride
    if ((weight_ = getTag("Weight", "0.0").toDouble()) > 0) {
        return weight_;
    }

    // withings?
    QList<SummaryMetrics> measures = mainwindow->metricDB->getAllMeasuresFor(QDateTime::fromString("Jan 1 00:00:00 1900"), startTime());
    int i = measures.count()-1;
    if (i) {
        while (i>=0) {
            if ((weight_ = measures[i].getText("Weight", "0.0").toDouble()) > 0) {
               return weight_;
            }
            i--;
        }
    }


    // global options
    return weight_ = appsettings->cvalue(mainwindow->cyclist, GC_WEIGHT, "75.0").toString().toDouble(); // default to 75kg
}
