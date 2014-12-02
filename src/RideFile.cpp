/*
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net)
 *               2009 Justin F. Knotzke (jknotzke@shampoo.ca)
 *               2013 Mark Liversedge (liversedge@gmail.com)
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
#include "WPrime.h"
#include "Athlete.h"
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
#include <math.h>
#include <qwt_spline.h>

#define mark() \
{ \
    addInterval(start, previous->secs + recIntSecs_, \
                QString("%1").arg(interval)); \
    interval = point->interval; \
    start = point->secs; \
}

const QChar deltaChar(0x0394);

RideFile::RideFile(const QDateTime &startTime, double recIntSecs) :
            startTime_(startTime), recIntSecs_(recIntSecs),
            deviceType_("unknown"), data(NULL), wprime_(NULL), 
            wstale(true), weight_(0), totalCount(0), totalTemp(0), dstale(true)
{
    command = new RideFileCommand(this);

    minPoint = new RideFilePoint();
    maxPoint = new RideFilePoint();
    avgPoint = new RideFilePoint();
    totalPoint = new RideFilePoint();
}

// construct from another is mostly just to get the tags
// when constructing a temporary ridefile when computing intervals
// and we want to get special fields and ESPECIALLY "CP" and "Weight"
RideFile::RideFile(RideFile *p) :
    recIntSecs_(p->recIntSecs_), deviceType_(p->deviceType_), data(NULL), wprime_(NULL), 
    wstale(true), weight_(p->weight_), totalCount(0), dstale(true)
{
    startTime_ = p->startTime_;
    tags_ = p->tags_;
    referencePoints_ = p->referencePoints_;
    deviceType_ = p->deviceType_;
    fileFormat_ = p->fileFormat_;
    intervals_ = p->intervals_;
    calibrations_ = p->calibrations_;
    context = p->context;

    command = new RideFileCommand(this);
    minPoint = new RideFilePoint();
    maxPoint = new RideFilePoint();
    avgPoint = new RideFilePoint();
    totalPoint = new RideFilePoint();

}

RideFile::RideFile() : 
    recIntSecs_(0.0), deviceType_("unknown"), data(NULL), wprime_(NULL), 
    wstale(true), weight_(0), totalCount(0), dstale(true)
{
    command = new RideFileCommand(this);

    minPoint = new RideFilePoint();
    maxPoint = new RideFilePoint();
    avgPoint = new RideFilePoint();
    totalPoint = new RideFilePoint();
}

RideFile::~RideFile()
{
    emit deleted();
    foreach(RideFilePoint *point, dataPoints_)
        delete point;
    delete command;
    if (wprime_) delete wprime_;
    //!!! if (data) delete data; // need a mechanism to notify the editor
}

WPrime *
RideFile::wprimeData()
{
    if (wprime_ == NULL || wstale) {
        if (!wprime_) wprime_ = new WPrime();
        wprime_->setRide(const_cast<RideFile*>(this)); // recompute
        wstale = false;
    }
    return wprime_;
}

bool
RideFile::isRun() const
{
    // for now we just look at Sport and if there are any
    // running specific data series in the data
    return (getTag("Sport", "") == "Run" || getTag("Sport", "") == tr("Run")) ||
           (areDataPresent()->rvert || areDataPresent()->rcad || areDataPresent()->rcontact);
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
    case RideFile::kphd: return QString(tr("Acceleration"));
    case RideFile::wattsd: return QString(tr("Power %1").arg(deltaChar));
    case RideFile::cadd: return QString(tr("Cadence %1").arg(deltaChar));
    case RideFile::nmd: return QString(tr("Torque %1").arg(deltaChar));
    case RideFile::hrd: return QString(tr("Heartrate %1").arg(deltaChar));
    case RideFile::nm: return QString(tr("Torque"));
    case RideFile::watts: return QString(tr("Power"));
    case RideFile::xPower: return QString(tr("xPower"));
    case RideFile::aPower: return QString(tr("aPower"));
    case RideFile::aTISS: return QString(tr("aTISS"));
    case RideFile::anTISS: return QString(tr("anTISS"));
    case RideFile::NP: return QString(tr("Normalized Power"));
    case RideFile::alt: return QString(tr("Altitude"));
    case RideFile::lon: return QString(tr("Longitude"));
    case RideFile::lat: return QString(tr("Latitude"));
    case RideFile::headwind: return QString(tr("Headwind"));
    case RideFile::slope: return QString(tr("Slope"));
    case RideFile::temp: return QString(tr("Temperature"));
    case RideFile::lrbalance: return QString(tr("Left/Right Balance"));
    case RideFile::lte: return QString(tr("Left Torque Efficiency"));
    case RideFile::rte: return QString(tr("Right Torque Efficiency"));
    case RideFile::lps: return QString(tr("Left Pedal Smoothness"));
    case RideFile::rps: return QString(tr("Righ Pedal Smoothness"));
    case RideFile::interval: return QString(tr("Interval"));
    case RideFile::vam: return QString(tr("VAM"));
    case RideFile::wattsKg: return QString(tr("Watts per Kilogram"));
    case RideFile::wprime: return QString(tr("W' balance"));
    case RideFile::smo2: return QString(tr("SmO2"));
    case RideFile::thb: return QString(tr("THb"));
    case RideFile::o2hb: return QString(tr("O2Hb"));
    case RideFile::hhb: return QString(tr("HHb"));
    case RideFile::rvert: return QString(tr("Vertical Oscillation"));
    case RideFile::rcad: return QString(tr("Run Cadence"));
    case RideFile::rcontact: return QString(tr("GCT"));
    case RideFile::gear: return QString(tr("Gear Ratio"));
    default: return QString(tr("Unknown"));
    }
}

QColor
RideFile::colorFor(SeriesType series)
{
    switch (series) {
    case RideFile::cad: return GColor(CCADENCE);
    case RideFile::cadd: return GColor(CCADENCE);
    case RideFile::hr: return GColor(CHEARTRATE);
    case RideFile::hrd: return GColor(CHEARTRATE);
    case RideFile::kph: return GColor(CSPEED);
    case RideFile::kphd: return GColor(CSPEED);
    case RideFile::nm: return GColor(CTORQUE);
    case RideFile::nmd: return GColor(CTORQUE);
    case RideFile::watts: return GColor(CPOWER);
    case RideFile::wattsd: return GColor(CPOWER);
    case RideFile::xPower: return GColor(CXPOWER);
    case RideFile::aPower: return GColor(CAPOWER);
    case RideFile::aTISS: return GColor(CNPOWER);
    case RideFile::anTISS: return GColor(CNPOWER);
    case RideFile::NP: return GColor(CNPOWER);
    case RideFile::alt: return GColor(CALTITUDE);
    case RideFile::headwind: return GColor(CWINDSPEED);
    case RideFile::temp: return GColor(CTEMP);
    case RideFile::lrbalance: return GColor(CBALANCELEFT);
    case RideFile::lte: return GColor(CLTE);
    case RideFile::rte: return GColor(CRTE);
    case RideFile::lps: return GColor(CLPS);
    case RideFile::rps: return GColor(CRPS);
    case RideFile::interval: return QColor(Qt::white);
    case RideFile::wattsKg: return GColor(CPOWER);
    case RideFile::wprime: return GColor(CWBAL);
    case RideFile::smo2: return GColor(CSMO2);
    case RideFile::thb: return GColor(CTHB);
    case RideFile::o2hb: return GColor(CO2HB);
    case RideFile::hhb: return GColor(CHHB);
    case RideFile::slope: return GColor(CSLOPE);
    case RideFile::rvert: return GColor(CRV);
    case RideFile::rcontact: return GColor(CRGCT);
    case RideFile::rcad: return GColor(CRCAD);
    case RideFile::gear: return GColor(CGEAR);
    case RideFile::secs:
    case RideFile::km:
    case RideFile::vam:
    case RideFile::lon:
    case RideFile::lat:
    default: return GColor(CPLOTMARKER);
    }
}

QString
RideFile::unitName(SeriesType series, Context *context)
{
    bool useMetricUnits = context->athlete->useMetricUnits;

    switch (series) {
    case RideFile::secs: return QString(tr("seconds"));
    case RideFile::cad: return QString(tr("rpm"));
    case RideFile::cadd: return QString(tr("rpm/s"));
    case RideFile::hr: return QString(tr("bpm"));
    case RideFile::hrd: return QString(tr("bpm/s"));
    case RideFile::km: return QString(useMetricUnits ? tr("km") : tr("miles"));
    case RideFile::kph: return QString(useMetricUnits ? tr("kph") : tr("mph"));
    case RideFile::kphd: return QString(tr("m/s/s"));
    case RideFile::nm: return QString(tr("N"));
    case RideFile::nmd: return QString(tr("N/s"));
    case RideFile::watts: return QString(tr("watts"));
    case RideFile::wattsd: return QString(tr("watts/s"));
    case RideFile::xPower: return QString(tr("watts"));
    case RideFile::aPower: return QString(tr("watts"));
    case RideFile::aTISS: return QString(tr("TISS"));
    case RideFile::anTISS: return QString(tr("TISS"));
    case RideFile::NP: return QString(tr("watts"));
    case RideFile::alt: return QString(useMetricUnits ? tr("metres") : tr("feet"));
    case RideFile::lon: return QString(tr("lon"));
    case RideFile::lat: return QString(tr("lat"));
    case RideFile::headwind: return QString(tr("kph"));
    case RideFile::slope: return QString(tr("%"));
    case RideFile::temp: return QString(tr("°C"));
    case RideFile::lrbalance: return QString(tr("%"));
    case RideFile::lte: return QString(tr("%"));
    case RideFile::rte: return QString(tr("%"));
    case RideFile::lps: return QString(tr("%"));
    case RideFile::rps: return QString(tr("%"));
    case RideFile::interval: return QString(tr("Interval"));
    case RideFile::vam: return QString(tr("meters per hour"));
    case RideFile::wattsKg: return QString(useMetricUnits ? tr("watts/kg") : tr("watts/kg")); // always kg !
    case RideFile::wprime: return QString(useMetricUnits ? tr("joules") : tr("joules"));
    case RideFile::smo2: return QString(tr("%"));
    case RideFile::thb: return QString(tr("g/dL"));
    case RideFile::o2hb: return QString(tr(""));
    case RideFile::hhb: return QString(tr(""));
    case RideFile::rcad: return QString(tr("spm"));
    case RideFile::rvert: return QString(tr("cm"));
    case RideFile::rcontact: return QString(tr("ms"));
    case RideFile::gear: return QString(tr("ratio"));
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
RideFile::distanceToTime(double km) const
{
    RideFilePoint p;
    p.km = km;

    // Check we have some data and the secs is in bounds
    if (dataPoints_.isEmpty()) return 0;
    if (km < dataPoints_.first()->km) return dataPoints_.first()->secs;
    if (km > dataPoints_.last()->km) return dataPoints_.last()->secs;

    QVector<RideFilePoint*>::const_iterator i = std::lower_bound(dataPoints_.begin(), dataPoints_.end(), &p, ComparePointKm());
    return (*i)->secs;
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
RideFileFactory::writeRideFile(Context *context, const RideFile *ride, QFile &file, QString format) const
{
    // get the ride file writer for this format
    RideFileReader *reader = readFuncs_.value(format.toLower());

    // write away
    if (!reader) return false;
    else return reader->writeRideFile(context, ride, file);
}

RideFileReader *RideFileFactory::readerForSuffix(QString suffix) const
{
    return readFuncs_.value(suffix.toLower());
}

RideFile *RideFileFactory::openRideFile(Context *context, QFile &file,
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
        result->context = context;

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
        QString notesFileName = fileInfo.canonicalPath() + '/' + fileInfo.baseName() + ".notes";
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
        foreach (FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
            if (field.diary == true && result->getTag(field.name, "") != "") {
                QString value = result->getTag(field.name, "");
                if (field.name == "Weight") {
                    bool useMetric = context->athlete->useMetricUnits;
                    if (useMetric == false) {
                        // This code appears in RideFile.cpp and RideMetaData.cpp and needs to be kept in sync for now.
                        double lbs = (double) qRound(100 * (value.toDouble() * LB_PER_KG + 0.001)) / 100;
                        value = QString("%1").arg(lbs);
                    }
                    // Add units (the format should be localized)
                    const RideMetric *aw = RideMetricFactory::instance().rideMetric("athlete_weight");
                    QString units = aw->units(useMetric);
                    value = QString("%1 %2").arg(value, units);
                }
                calendarText += QString("%1\n")
                        .arg(value);
            }
        }
        result->setTag("Calendar Text", calendarText);

        // set other "special" fields
        result->setTag("Filename", QFileInfo(file.fileName()).fileName());
        result->setTag("Device", result->deviceType());
        result->setTag("File Format", result->fileFormat());
        result->setTag("Athlete", context->athlete->cyclist);
        result->setTag("Year", result->startTime().toString("yyyy"));
        result->setTag("Month", result->startTime().toString("MMMM"));
        result->setTag("Weekday", result->startTime().toString("ddd"));

        // reset timestamps and distances to always start from zero
        double timeOffset=0.00f, kmOffset=0.00f;
        if (result->dataPoints().count()) {
            timeOffset=result->dataPoints()[0]->secs;
            kmOffset=result->dataPoints()[0]->km;
        }

        if (timeOffset || kmOffset) {
            foreach (RideFilePoint *p, result->dataPoints()) {
                p->km = p->km - kmOffset;
                p->secs = p->secs - timeOffset;
            }
        }

        DataProcessorFactory::instance().autoProcess(result);

        // calculate derived data series -- after data fixers applied above
        result->recalculateDerivedSeries();

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
        if (result->areDataPresent()->slope) flags += 'L'; // Slope
        else flags += '-';
        if (result->areDataPresent()->headwind) flags += 'W'; // Windspeed
        else flags += '-';
        if (result->areDataPresent()->temp) flags += 'E'; // Temperature
        else flags += '-';
        if (result->areDataPresent()->lrbalance) flags += 'V'; // V for "Vector" aka lr pedal data
        else flags += '-';
        if (result->areDataPresent()->smo2 ||
            result->areDataPresent()->thb) flags += 'O'; // Moxy O2/Haemoglobin
        else flags += '-';
        if (result->areDataPresent()->rcontact ||
            result->areDataPresent()->rvert ||
            result->areDataPresent()->rcontact) flags += 'R'; // R is for running dynamics
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

void RideFile::updateMin(RideFilePoint* point)
{
    // MIN
    if (point->secs<minPoint->secs)
       minPoint->secs = point->secs;
    if (minPoint->cad == 0 || point->cad<minPoint->cad)
       minPoint->cad = point->cad;
    if (minPoint->hr == 0 || point->hr<minPoint->hr)
       minPoint->hr = point->hr;
    if (minPoint->km == 0 || point->km<minPoint->km)
       minPoint->km = point->km;
    if (minPoint->kph == 0 || point->kph<minPoint->kph)
       minPoint->kph = point->kph;
    if (minPoint->nm == 0 || point->nm<minPoint->nm)
       minPoint->nm = point->nm;
    if (minPoint->watts == 0 || point->watts<minPoint->watts)
       minPoint->watts = point->watts;
    if (point->alt<minPoint->alt)
       minPoint->alt = point->alt;
    if (point->lon<minPoint->lon)
       minPoint->lon = point->lon;
    if (point->lat<minPoint->lat)
       minPoint->lat = point->lat;
    if (point->headwind<minPoint->headwind)
       minPoint->headwind = point->headwind;
    if (point->slope<minPoint->slope)
       minPoint->slope = point->slope;
    if (point->temp != NoTemp && point->temp<minPoint->temp)
       minPoint->temp = point->temp;
    if (minPoint->lte == 0 || point->lte<minPoint->lte)
       minPoint->lte = point->lte;
    if (minPoint->rte == 0 || point->rte<minPoint->rte)
       minPoint->rte = point->rte;
    if (minPoint->lps == 0 || point->lps<minPoint->lps)
       minPoint->lps = point->lps;
    if (minPoint->rps == 0 || point->rps<minPoint->rps)
       minPoint->rps = point->rps;
    if (minPoint->lrbalance == 0 || point->lrbalance<minPoint->lrbalance)
       minPoint->lrbalance = point->lrbalance;
    if (minPoint->smo2 == 0 || point->smo2<minPoint->smo2)
       minPoint->smo2 = point->smo2;
    if (minPoint->o2hb == 0 || point->o2hb<minPoint->o2hb)
       minPoint->o2hb = point->o2hb;
    if (minPoint->hhb == 0 || point->hhb<minPoint->hhb)
       minPoint->hhb = point->hhb;
    if (minPoint->thb == 0 || point->thb<minPoint->thb)
       minPoint->thb = point->thb;
    if (minPoint->rvert == 0 || point->rvert<minPoint->rvert)
       minPoint->rvert = point->rvert;
    if (minPoint->rcad == 0 || point->rcad<minPoint->rcad)
       minPoint->rcad = point->rcad;
    if (minPoint->rcontact == 0 || point->rcontact<minPoint->rcontact)
       minPoint->rcontact = point->rcontact;
    if (minPoint->gear == 0 || point->gear<minPoint->gear)
       minPoint->gear = point->gear;
}

void RideFile::updateMax(RideFilePoint* point)
{
    // MAX
    if (point->secs>maxPoint->secs)
       maxPoint->secs = point->secs;
    if (point->cad>maxPoint->cad)
       maxPoint->cad = point->cad;
    if (point->hr>maxPoint->hr)
       maxPoint->hr = point->hr;
    if (point->km>maxPoint->km)
       maxPoint->km = point->km;
    if (point->kph>maxPoint->kph)
       maxPoint->kph = point->kph;
    if (point->nm>maxPoint->nm)
       maxPoint->nm = point->nm;
    if (point->watts>maxPoint->watts)
       maxPoint->watts = point->watts;
    if (point->alt>maxPoint->alt)
       maxPoint->alt = point->alt;
    if (point->lon>maxPoint->lon)
       maxPoint->lon = point->lon;
    if (point->lat>maxPoint->lat)
       maxPoint->lat = point->lat;
    if (point->headwind>maxPoint->headwind)
       maxPoint->headwind = point->headwind;
    if (point->slope>maxPoint->slope)
       maxPoint->slope = point->slope;
    if (point->temp != NoTemp && point->temp>maxPoint->temp)
       maxPoint->temp = point->temp;
    if (point->lte>maxPoint->lte)
       maxPoint->lte = point->lte;
    if (point->rte>maxPoint->rte)
       maxPoint->rte = point->rte;
    if (point->lps>maxPoint->lps)
       maxPoint->lps = point->lps;
    if (point->rps>maxPoint->rps)
       maxPoint->rps = point->rps;
    if (point->lrbalance>maxPoint->lrbalance)
       maxPoint->lrbalance = point->lrbalance;
    if (point->smo2>maxPoint->smo2)
       maxPoint->smo2 = point->smo2;
    if (point->thb>maxPoint->thb)
       maxPoint->thb = point->thb;
    if (point->o2hb>maxPoint->o2hb)
       maxPoint->o2hb = point->o2hb;
    if (point->hhb>maxPoint->hhb)
       maxPoint->hhb = point->hhb;
    if (point->rvert>maxPoint->rvert)
       maxPoint->rvert = point->rvert;
    if (point->rcad>maxPoint->rcad)
       maxPoint->rcad = point->rcad;
    if (point->rcontact>maxPoint->rcontact)
       maxPoint->rcontact = point->rcontact;
    if (point->gear>maxPoint->gear)
       maxPoint->gear = point->gear;
}

void RideFile::updateAvg(RideFilePoint* point)
{
    // AVG
    totalPoint->secs += point->secs;
    totalPoint->cad += point->cad;
    totalPoint->hr += point->hr;
    totalPoint->km += point->km;
    totalPoint->kph += point->kph;
    totalPoint->nm += point->nm;
    totalPoint->watts += point->watts;
    totalPoint->alt += point->alt;
    totalPoint->lon += point->lon;
    totalPoint->lat += point->lat;
    totalPoint->headwind += point->headwind;
    totalPoint->slope += point->slope;
    totalPoint->temp += point->temp == NoTemp ? 0 : point->temp;
    totalPoint->lte += point->lte;
    totalPoint->rte += point->rte;
    totalPoint->lps += point->lps;
    totalPoint->rps += point->rps;
    totalPoint->lrbalance += point->lrbalance;
    totalPoint->smo2 += point->smo2;
    totalPoint->thb += point->thb;
    totalPoint->o2hb += point->o2hb;
    totalPoint->hhb += point->hhb;
    totalPoint->rvert += point->rvert;
    totalPoint->rcad += point->rcad;
    totalPoint->rcontact += point->rcontact;
    totalPoint->gear += point->gear;

    ++totalCount;
    if (point->temp != NoTemp) ++totalTemp;

    // todo : division only for last after last point
    avgPoint->secs = totalPoint->secs/totalCount;
    avgPoint->cad = totalPoint->cad/totalCount;
    avgPoint->hr = totalPoint->hr/totalCount;
    avgPoint->km = totalPoint->km/totalCount;
    avgPoint->kph = totalPoint->kph/totalCount;
    avgPoint->nm = totalPoint->nm/totalCount;
    avgPoint->watts = totalPoint->watts/totalCount;
    avgPoint->alt = totalPoint->alt/totalCount;
    avgPoint->lon = totalPoint->lon/totalCount;
    avgPoint->lat = totalPoint->lat/totalCount;
    avgPoint->headwind = totalPoint->headwind/totalCount;
    avgPoint->slope = totalPoint->slope/totalCount;
    if (totalTemp) avgPoint->temp = totalPoint->temp/totalTemp;
    avgPoint->lrbalance = totalPoint->lrbalance/totalCount;
    avgPoint->lte = totalPoint->lte/totalCount;
    avgPoint->rte = totalPoint->rte/totalCount;
    avgPoint->lps = totalPoint->lps/totalCount;
    avgPoint->rps = totalPoint->rps/totalCount;
    avgPoint->smo2 = totalPoint->smo2/totalCount;
    avgPoint->thb = totalPoint->thb/totalCount;
    avgPoint->o2hb = totalPoint->o2hb/totalCount;
    avgPoint->hhb = totalPoint->hhb/totalCount;
    avgPoint->rvert = totalPoint->rvert/totalCount;
    avgPoint->rcad = totalPoint->rcad/totalCount;
    avgPoint->rcontact = totalPoint->rcontact/totalCount;
    avgPoint->gear = totalPoint->gear/totalCount;
}

void RideFile::appendPoint(double secs, double cad, double hr, double km,
                           double kph, double nm, double watts, double alt,
                           double lon, double lat, double headwind,
                           double slope, double temp, double lrbalance, 
                           double lte, double rte, double lps, double rps,
                           double smo2, double thb,
                           double rvert, double rcad, double rcontact,
                           int interval)
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
    if (!isfinite(lps) || lps<0) lps=0;
    if (!isfinite(rps) || rps<0) rps=0;
    if (!isfinite(lte) || lte<0) lte=0;
    if (!isfinite(rte) || rte<0) rte=0;
    if (!isfinite(smo2) || smo2<0) smo2=0;
    if (!isfinite(thb) || thb<0) thb=0;
    if (!isfinite(rvert) || rvert<0) rvert=0;
    if (!isfinite(rcad) || rcad<0) rcad=0;
    if (!isfinite(rcontact) || rcontact<0) rcontact=0;

    // truncate alt out of bounds -- ? should do for all, but uncomfortable about
    //                                 setting an absolute max. At least We know the highest
    //                                 point on Earth (Mt Everest).
    if (alt > RideFile::maximumFor(RideFile::alt)) alt = RideFile::maximumFor(RideFile::alt);

    RideFilePoint* point = new RideFilePoint(secs, cad, hr, km, kph, nm, watts, alt, lon, lat, 
                                             headwind, slope, temp, lrbalance, lte, rte, lps, rps,
                                             smo2, thb,
                                             rvert, rcad, rcontact,
                                             interval);
    dataPoints_.append(point);

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
    dataPresent.temp     |= (temp != NoTemp);
    dataPresent.lrbalance|= (lrbalance != 0);
    dataPresent.lte      |= (lte != 0);
    dataPresent.rte      |= (rte != 0);
    dataPresent.lps      |= (lps != 0);
    dataPresent.rps      |= (rps != 0);
    dataPresent.smo2     |= (smo2 != 0);
    dataPresent.thb      |= (thb != 0);
    dataPresent.rvert    |= (rvert != 0);
    dataPresent.rcad     |= (rcad != 0);
    dataPresent.rcontact |= (rcontact != 0);
    dataPresent.interval |= (interval != 0);

    updateMin(point);
    updateMax(point);
    updateAvg(point);
}

void RideFile::appendPoint(const RideFilePoint &point)
{
    appendPoint(point.secs,point.cad,point.hr,point.km,point.kph,
                point.nm,point.watts,point.alt,point.lon,point.lat,
                point.headwind, point.slope, point.temp, point.lrbalance,
                point.lte, point.rte, point.lps, point.rps,
                point.smo2, point.thb,
                point.rvert, point.rcad, point.rcontact,
                point.interval);
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
        case lte : dataPresent.lte = value; break;
        case rte : dataPresent.rte = value; break;
        case lps : dataPresent.lps = value; break;
        case rps : dataPresent.rps = value; break;
        case smo2 : dataPresent.smo2 = value; break;
        case thb : dataPresent.thb = value; break;
        case o2hb : dataPresent.o2hb = value; break;
        case hhb : dataPresent.hhb = value; break;
        case rcad : dataPresent.rcad = value; break;
        case rvert : dataPresent.rvert = value; break;
        case rcontact : dataPresent.rcontact = value; break;
        case gear : dataPresent.gear = value; break;
        case interval : dataPresent.interval = value; break;
        case wprime : dataPresent.wprime = value; break;
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
        case aPower : return dataPresent.apower; break;
        case aTISS : return dataPresent.atiss; break;
        case anTISS : return dataPresent.antiss; break;
        case alt : return dataPresent.alt; break;
        case lon : return dataPresent.lon; break;
        case lat : return dataPresent.lat; break;
        case headwind : return dataPresent.headwind; break;
        case slope : return dataPresent.slope; break;
        case temp : return dataPresent.temp; break;
        case lrbalance : return dataPresent.lrbalance; break;
        case lps : return dataPresent.lps; break;
        case rps : return dataPresent.rps; break;
        case lte : return dataPresent.lte; break;
        case rte : return dataPresent.rte; break;
        case smo2 : return dataPresent.smo2; break;
        case thb : return dataPresent.thb; break;
        case o2hb : return dataPresent.o2hb; break;
        case hhb : return dataPresent.hhb; break;
        case rvert : return dataPresent.rvert; break;
        case rcad : return dataPresent.rcad; break;
        case rcontact : return dataPresent.rcontact; break;
        case gear : return dataPresent.gear; break;
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
        case lte : dataPoints_[index]->lte = value; break;
        case rte : dataPoints_[index]->rte = value; break;
        case lps : dataPoints_[index]->lps = value; break;
        case rps : dataPoints_[index]->rps = value; break;
        case smo2 : dataPoints_[index]->smo2 = value; break;
        case thb : dataPoints_[index]->thb = value; break;
        case o2hb : dataPoints_[index]->o2hb = value; break;
        case hhb : dataPoints_[index]->hhb = value; break;
        case rcad : dataPoints_[index]->rcad = value; break;
        case rvert : dataPoints_[index]->rvert = value; break;
        case rcontact : dataPoints_[index]->rcontact = value; break;
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
        case RideFile::kphd : return kphd; break;
        case RideFile::cadd : return cadd; break;
        case RideFile::nmd : return nmd; break;
        case RideFile::hrd : return hrd; break;
        case RideFile::nm : return nm; break;
        case RideFile::watts : return watts; break;
        case RideFile::wattsd : return wattsd; break;
        case RideFile::alt : return alt; break;
        case RideFile::lon : return lon; break;
        case RideFile::lat : return lat; break;
        case RideFile::headwind : return headwind; break;
        case RideFile::slope : return slope; break;
        case RideFile::temp : return temp; break;
        case RideFile::lrbalance : return lrbalance; break;
        case RideFile::lte : return lte; break;
        case RideFile::rte : return rte; break;
        case RideFile::lps : return lps; break;
        case RideFile::rps : return rps; break;
        case RideFile::thb : return thb; break;
        case RideFile::smo2 : return smo2; break;
        case RideFile::o2hb : return o2hb; break;
        case RideFile::hhb : return hhb; break;
        case RideFile::rcad : return rcad; break;
        case RideFile::rvert : return rvert; break;
        case RideFile::rcontact : return rcontact; break;
        case RideFile::gear : return gear; break;
        case RideFile::interval : return interval; break;
        case RideFile::NP : return np; break;
        case RideFile::xPower : return xp; break;
        case RideFile::aPower : return apower; break;
        case RideFile::aTISS : return atiss; break;
        case RideFile::anTISS : return antiss; break;

        default:
        case RideFile::none : break;
    }
    return 0.0;
}

void
RideFilePoint::setValue(RideFile::SeriesType series, double value)
{
    switch (series) {
        case RideFile::secs : secs = value; break;
        case RideFile::cad : cad = value; break;
        case RideFile::hr : hr = value; break;
        case RideFile::km : km = value; break;
        case RideFile::kph : kph = value; break;
        case RideFile::kphd : kphd = value; break;
        case RideFile::cadd : cadd = value; break;
        case RideFile::nmd : nmd = value; break;
        case RideFile::hrd : hrd = value; break;
        case RideFile::nm : nm = value; break;
        case RideFile::watts : watts = value; break;
        case RideFile::wattsd : wattsd = value; break;
        case RideFile::alt : alt = value; break;
        case RideFile::lon : lon = value; break;
        case RideFile::lat : lat = value; break;
        case RideFile::headwind : headwind = value; break;
        case RideFile::slope : slope = value; break;
        case RideFile::temp : temp = value; break;
        case RideFile::lrbalance : lrbalance = value; break;
        case RideFile::lte : lte = value; break;
        case RideFile::rte : rte = value; break;
        case RideFile::lps : lps = value; break;
        case RideFile::rps : rps = value; break;
        case RideFile::thb : thb = value; break;
        case RideFile::smo2 : smo2 = value; break;
        case RideFile::o2hb : o2hb = value; break;
        case RideFile::hhb : hhb = value; break;
        case RideFile::rcad : rcad = value; break;
        case RideFile::rvert : rvert = value; break;
        case RideFile::rcontact : rcontact = value; break;
        case RideFile::gear : gear = value; break;
        case RideFile::interval : interval = value; break;
        case RideFile::NP : np = value; break;
        case RideFile::xPower : xp = value; break;
        case RideFile::aPower : apower = value; break;
        case RideFile::aTISS : atiss = value; break;
        case RideFile::anTISS : antiss = value; break;

        default:
        case RideFile::none : break;
    }
}

double
RideFile::getPointValue(int index, SeriesType series) const
{
    return dataPoints_[index]->value(series);
}

QVariant
RideFile::getPointFromValue(double value, SeriesType series) const
{
    if (series==RideFile::temp && value == RideFile::NoTemp)
        return "";
    else if (series==RideFile::wattsKg)
        return "";
    return value;
}

QVariant
RideFile::getPoint(int index, SeriesType series) const
{
    return getPointFromValue(getPointValue(index, series), series);
}

QVariant
RideFile::getMinPoint(SeriesType series) const
{
    return getPointFromValue(minPoint->value(series), series);
}

QVariant
RideFile::getAvgPoint(SeriesType series) const
{
    return getPointFromValue(avgPoint->value(series), series);
}

QVariant
RideFile::getMaxPoint(SeriesType series) const
{
    return getPointFromValue(maxPoint->value(series), series);
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
        case aPower : return 0; break;
        case aTISS : return 1; break;
        case anTISS : return 1; break;
        case NP : return 0; break;
        case alt : return 3; break;
        case lon : return 8; break;
        case lat : return 8; break;
        case headwind : return 4; break;
        case slope : return 1; break;
        case temp : return 1; break;
        case interval : return 0; break;
        case vam : return 0; break;
        case wattsKg : return 2; break;
        case lrbalance : return 1; break;
        case lps :
        case rps :
        case lte :
        case rte : return 0; break;
        case smo2 : return 0; break;
        case thb : return 2; break;
        case o2hb : return 2; break;
        case hhb : return 2; break;
        case rcad : return 0; break;
        case rvert : return 1; break;
        case rcontact : return 1; break;
        case gear : return 2; break;
        case wprime : return 0; break;
        default:
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
        case aPower : return 2500; break;
        case aTISS : return 1000; break;
        case anTISS : return 100; break;
        case alt : return 8850; break; // mt everest is highest point above sea level
        case lon : return 180; break;
        case lat : return 90; break;
        case headwind : return 999; break;
        case slope : return 100; break;
        case temp : return 100; break;
        case interval : return 999; break;
        case vam : return 9999; break;
        case wattsKg : return 50; break;
        case lps :
        case rps :
        case lte :
        case rte :
        case lrbalance : return 100; break;
        case smo2 : return 100; break;
        case thb : return 20; break;
        case o2hb : return 20; break;
        case hhb : return 20; break;
        case rcad : return 500; break;
        case rvert : return 50; break;
        case rcontact : return 1000; break;
        case gear : return 7; break; // 53x8
        case wprime : return 99999; break;
        default :
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
        case aPower : return 0; break;
        case aTISS : return 0; break;
        case anTISS : return 0; break;
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
        case lte :
        case rte :
        case lps :
        case rps :
        case lrbalance : return 0; break;
        case smo2 : return 0; break;
        case thb : return 0; break;
        case o2hb : return 0; break;
        case hhb : return 0; break;
        case rcad : return 0; break;
        case rvert : return 0; break;
        case rcontact : return 0; break;
        case gear : return 0; break;
        case wprime : return 0; break;
        default :
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
    wstale = dstale = true;
    emit saved();
}

void
RideFile::emitReverted()
{
    weight_ = 0;
    wstale = dstale = true;
    emit reverted();
}

void
RideFile::emitModified()
{
    weight_ = 0;
    wstale = dstale = true;
    emit modified();
}

void
RideFile::setWeight(double x)
{
    weight_ = x;
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
    QList<SummaryMetrics> measures = context->athlete->metricDB->getAllMeasuresFor(QDateTime::fromString("Jan 1 00:00:00 1900"), startTime());
    int i = measures.count()-1;
    if (i) {
        while (i>=0) {
            if ((weight_ = measures[i].getText("Weight_m", "0.0").toDouble()) > 0) {
               return weight_;
            }
            i--;
        }
    }


    // global options
    weight_ = appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT, "75.0").toString().toDouble(); // default to 75kg

    // if set to zero in global options then override it.
    // it must not be zero!!!
    if (weight_ <= 0.00) weight_ = 75.00;

    return weight_;
}

double
RideFile::getHeight()
{
    double const height_default = (this->getWeight()+100.0)/98.43; // default to Stillman Average
    double height = height_default;

    // ride
    if ((height = getTag("Height", "0.0").toDouble()) > 0) {
        return height;
    }

    // is withings upported for height?

    // global options
    height = appsettings->cvalue(context->athlete->cyclist, GC_HEIGHT, height_default).toString().toDouble();

    // if set to zero in global options then override it.
    // it must not be zero!!!
    if (height <= 0.00) height = height_default;

    return height;
}

void RideFile::appendReference(const RideFilePoint &point)
{
    referencePoints_.append(new RideFilePoint(point.secs,point.cad,point.hr,point.km,point.kph,point.nm,
                                              point.watts,point.alt,point.lon,point.lat,
                                              point.headwind, point.slope, point.temp, point.lrbalance, 
                                              point.lte, point.rte, point.lps, point.rps, point.smo2, point.thb, 
                                              point.rvert, point.rcad, point.rcontact,
                                              point.interval));
}

void RideFile::removeReference(int index)
{
    referencePoints_.remove(index);
}

bool
RideFile::parseRideFileName(const QString &name, QDateTime *dt)
{
    static char rideFileRegExp[] = "^((\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)"
                                   "_(\\d\\d)_(\\d\\d)_(\\d\\d))\\.(.+)$";
    QRegExp rx(rideFileRegExp);
    if (!rx.exactMatch(name))
            return false;
    assert(rx.captureCount() == 8);
    QDate date(rx.cap(2).toInt(), rx.cap(3).toInt(),rx.cap(4).toInt());
    QTime time(rx.cap(5).toInt(), rx.cap(6).toInt(),rx.cap(7).toInt());
    if ((! date.isValid()) || (! time.isValid())) {
	QMessageBox::warning(NULL,
			     tr("Invalid Ride File Name"),
			     tr("Invalid date/time in filename:\n%1\nSkipping file...").arg(name)
			     );
	return false;
    }
    *dt = QDateTime(date, time);
    return true;
}

//
// Calculate derived data series, including a new metric aPower
// aPower is based upon the models and research presented in
// "Altitude training and Athletic Performance" by Randall L. Wilber
// and Peronnet et al. (1991): Peronnet, F., G. Thibault, and D.L. Cousineau 1991.
// "A theoretical analisys of the effect of altitude on running
// performance." Journal of Applied Physiology 70:399-404
//

void
RideFile::recalculateDerivedSeries()
{
    // derived data is calculated from the data that is present
    // we should set to 0 where we cannot derive since we may
    // be called after data is deleted or added
    if (dstale == false) return; // we're already up to date

    //
    // NP Initialisation -- working variables
    //
    QVector<double> NProlling;
    int NProllingwindowsize = 30 / (recIntSecs_ ? recIntSecs_ : 1);
    if (NProllingwindowsize > 1) NProlling.resize(NProllingwindowsize);
    double NPtotal = 0;
    int NPcount = 0;
    int NPindex = 0;
    double NPsum = 0;

    //
    // XPower Initialisation -- working variables
    //
    static const double EPSILON = 0.1;
    static const double NEGLIGIBLE = 0.1;
    double XPsecsDelta = recIntSecs_ ? recIntSecs_ : 1;
    double XPsampsPerWindow = 25.0 / XPsecsDelta;
    double XPattenuation = XPsampsPerWindow / (XPsampsPerWindow + XPsecsDelta);
    double XPsampleWeight = XPsecsDelta / (XPsampsPerWindow + XPsecsDelta);
    double XPlastSecs = 0.0;
    double XPweighted = 0.0;
    double XPtotal = 0.0;
    int XPcount = 0;

    //
    // APower Initialisation -- working variables
    double APtotal=0;
    double APcount=0;

    // aTISS - Aerobic Training Impact Scoring System
    static const double a = 0.663788683661645f;
    static const double b = -7.5095428451195f;
    static const double c = -0.86118031563782f;
    //static const double t = 2;
    // anTISS
    static const double an = 0.238923886004611f;
    //static const double bn = -12.2066385296127f;
    static const double bn = -61.849f;
    static const double cn = -1.73549567522521f;

    int CP = 0;
    //int WPRIME = 0;
    double aTISS = 0.0f;
    double anTISS = 0.0f;

    // set WPrime and CP
    if (context->athlete->zones()) {
        int zoneRange = context->athlete->zones()->whichRange(startTime().date());
        CP = zoneRange >= 0 ? context->athlete->zones()->getCP(zoneRange) : 0;
        //WPRIME = zoneRange >= 0 ? context->athlete->zones()->getWprime(zoneRange) : 0;

        // did we override CP in metadata / metrics ?
        int oCP = getTag("CP","0").toInt();
        if (oCP) CP=oCP;
    }

    // wheelsize - use meta, then config then drop to 2100
    double wheelsize = getTag(tr("Wheelsize"), "0.0").toDouble();
    if (wheelsize == 0) wheelsize = appsettings->value(this, GC_WHEELSIZE, 2100).toInt();
    wheelsize /= 1000.00f; // need it in meters

    // last point looked at
    RideFilePoint *lastP = NULL;

    foreach(RideFilePoint *p, dataPoints_) {

        // Delta
        if (lastP) {

            double deltaSpeed = (p->kph - lastP->kph) / 3.60f;
            double deltaTime = p->secs - lastP->secs;

            if (deltaTime > 0) {

                p->kphd = deltaSpeed / deltaTime;

                // Other delta values -- only interested in growth for power, cadence 
                double pd = (p->watts - lastP->watts) / deltaTime;
                p->wattsd = pd > 0 && pd < 2500 ? pd : 0;

                double cd = (p->cad - lastP->cad) / deltaTime;
                p->cadd = cd > 0 && cd < 200 ? cd : 0;

                double nd = (p->nm - lastP->nm) / deltaTime;
                p->nmd = nd; // we want drops when looking for jump out saddle vs sit down

                // we want recovery and increase times for hr
                p->hrd = (p->hr - lastP->hr) / deltaTime;
                // ignore hr dropouts -- 0 means dropout not dead!
                if (!p->hr || (lastP && lastP->hr == 0)) p->hrd = 0;

            }
        }

        //
        // NP
        //
        if (dataPresent.watts && NProllingwindowsize > 1) {

            dataPresent.np = true;

            // sum last 30secs
            NPsum += p->watts;
            NPsum -= NProlling[NPindex];
            NProlling[NPindex] = p->watts;

            // running total and count
            NPtotal += pow(NPsum/NProllingwindowsize,4); // raise rolling average to 4th power
            NPcount ++;

            // root for ride so far
            if (NPcount && NPcount*recIntSecs_ > 30) {
                p->np = pow(NPtotal / (NPcount), 0.25);
            } else {
                p->np = 0.00f;
            }

            // move index on/round
            NPindex = (NPindex >= NProllingwindowsize-1) ? 0 : NPindex+1;

        } else {

            p->np = 0.00f;
        }

        // now the min and max values for NP
        if (p->np > maxPoint->np) maxPoint->np = p->np;
        if (p->np < minPoint->np) minPoint->np = p->np;

        //
        // xPower
        //
        if (dataPresent.watts) {

            dataPresent.xp = true;

            while ((XPweighted > NEGLIGIBLE) && (p->secs > XPlastSecs + XPsecsDelta + EPSILON)) {
                XPweighted *= XPattenuation;
                XPlastSecs += XPsecsDelta;
                XPtotal += pow(XPweighted, 4.0);
                XPcount++;
            }

            XPweighted *= XPattenuation;
            XPweighted += XPsampleWeight * p->watts;
            XPlastSecs = p->secs;
            XPtotal += pow(XPweighted, 4.0);
            XPcount++;
        
            p->xp = pow(XPtotal / XPcount, 0.25);
        }

        // now the min and max values for NP
        if (p->xp > maxPoint->xp) maxPoint->xp = p->xp;
        if (p->xp < minPoint->xp) minPoint->xp = p->xp;

        // aPower
        if (dataPresent.watts == true && dataPresent.alt == true) {

            dataPresent.apower = true;

            static const double a0  = -174.1448622f;
            static const double a1  = 1.0899959f;
            static const double a2  = -0.0015119f;
            static const double a3  = 7.2674E-07f;
            //static const double E = 2.71828183f;

            if (p->alt > 0) {
                // pbar [mbar]= 0.76*EXP( -alt[m] / 7000 )*1000 
                double pbar = 0.76f * exp(p->alt / -7000.00f) * 1000.00f;

                // %Vo2max= a0 + a1 * pbar + a2 * pbar ^2 + a3 * pbar ^3 (with pbar in mbar)
                double vo2maxPCT = a0 + (a1 * pbar) + (a2 * pow(pbar,2)) + (a3 * pow(pbar,3)); 

                p->apower = double(p->watts / vo2maxPCT) * 100;

            } else {

                p->apower = p->watts;
            }

        } else {

            p->apower = p->watts;
        }

        // now the min and max values for NP
        if (p->apower > maxPoint->apower) maxPoint->apower = p->apower;
        if (p->apower < minPoint->apower) minPoint->apower = p->apower;

        APtotal += p->apower;
        APcount++;

        // Anaerobic and Aerobic TISS
        if (CP && dataPresent.watts) {

            // a * exp (b * exp (c * fraction of cp) ) 
            aTISS += recIntSecs_ * (a * exp(b * exp(c * (double(p->watts) / double(CP)))));
            anTISS += recIntSecs_ * (an * exp(bn * exp(cn * (double(p->watts) / double(CP)))));
            p->atiss = aTISS;
            p->antiss = anTISS;
        }

        if (!dataPresent.slope && dataPresent.alt && dataPresent.km) {
            if (lastP) {
                double deltaDistance = (p->km - lastP->km) * 1000;
                double deltaAltitude = p->alt - lastP->alt;
                if (deltaDistance>0) {
                    p->slope = (deltaAltitude / deltaDistance) * 100;
                } else {
                    p->slope = 0;
                }
                if (p->slope > 20 || p->slope < -20) {
                    p->slope = lastP->slope;
                }
            }
        }

        // can we derive gear ratio ?
        // needs speed and cadence
        if (p->kph && p->cad && !isRun()) {

            // need to say we got it
            setDataPresent(RideFile::gear, true);

            // calculate gear ratio, with simple 3 level rounding (considering that the ratio steps are not linear):
            // -> below ratio 1, round to next 0,05 border (ratio step of 2 tooth change is around 0,03 for 20/36 (MTB)
            // -> above ratio 1 and 3,  round to next 0,1 border (MTB + Racebike - bigger differences per shifting step)
            // -> above ration 3, round to next 0,5 border (mainly Racebike - even wider differences)
            // speed and wheelsize in meters
            // but only if ride point has power, cadence and speed > 0 otherwise calculation will give a random result
            if (p->watts > 0.0f && p->cad > 0.0f && p->kph > 0.0f) {
                p->gear = (1000.00f * p->kph) / (p->cad * 60.00f * wheelsize);
                // do the rounding
                double rounding = 0.0f;
                if (p->gear < 1.0f) {
                  rounding = 0.05f;
                }
                else if (p->gear >= 1.0f && p->gear < 3.0f) {
                    rounding = 0.1f;
                } else {
                    rounding = 0.5f;
                }
                double mod = fmod(p->gear, rounding);
                double factor = trunc(p->gear / rounding);
                if (mod < rounding) p->gear = factor * rounding;
                else p->gear = (factor+1) * rounding;

                // final rounding to 2 decimals
                p->gear = round(p->gear * 100.00f) / 100.00f;
            }
            else {
                p->gear = 0.0f; // to be filled up with previous gear later
            }

            // truncate big values
            if (p->gear > maximumFor(RideFile::gear)) p->gear = 0;

        } else {
            p->gear = 0.0f;
        }

        // split out O2Hb and HHb when we have SmO2 and tHb
        // O2Hb is oxygenated haemoglobin and HHb is deoxygenated haemoglobin
        if (dataPresent.smo2 && dataPresent.thb) {

            if (p->smo2 > 0 && p->thb > 0) {
                setDataPresent(RideFile::o2hb, true);
                setDataPresent(RideFile::hhb, true);

                p->o2hb = (p->thb * p->smo2) / 100.00f;
                p->hhb = p->thb - p->o2hb;
            } else {

                p->o2hb = p->hhb = 0;
            }
        }

        // last point
        lastP = p;
    }

    // remove gear outlier (for single outlier values = 1 second) and
    // fill 0 gaps in Gear series with previous or next gear ration value (whichever of those is above 0)
    if (dataPresent.gear) {
        double last = 0.0f;
        double current = 0.0f;
        double next = 0.0f;
        double lastGear = 0.0;
        for (int i = 0; i<dataPoints_.count(); i++) {
            // first handle the zeros
            if (dataPoints_[i]->gear > 0)
                lastGear = dataPoints_[i]->gear;
            else
                dataPoints_[i]->gear = lastGear;
            // set the single outliers (there might be better ways, but this is easy
            if (i>0) last = dataPoints_[i-1]->gear; else last = 0.0f;
            current = dataPoints_[i]->gear;
            if (i<dataPoints_.count()-1) next = dataPoints_[i+1]->gear; else next = 0.0f;
            // if there is a big jump to current in relation to last-next consider this a outlier
            double diff1 = abs(last-next);
            double diff2 = abs(last-current);
            if ((diff1 < 0.01f) || (diff2 >= (diff1+0.5f))){
                // single outlier (no shift up/down in 2 seconds
                dataPoints_[i]->gear = (last>next) ? last : next;
            }

        }
    }

    // Smooth the slope if it has been derived
    if (!dataPresent.slope && dataPresent.alt && dataPresent.km) {
        int smoothPoints = 10;
        // initialise rolling average
        double rtot = 0;
        for (int i=smoothPoints; i>0 && dataPoints_.count()-i >=0; i--) {
            rtot += dataPoints_[dataPoints_.count()-i]->slope;
        }

        // now run backwards setting the rolling average
        for (int i=dataPoints_.count()-1; i>=smoothPoints; i--) {
            double here = dataPoints_[i]->slope;
            dataPoints_[i]->slope = rtot / smoothPoints;
            rtot -= here;
            rtot += dataPoints_[i-smoothPoints]->slope;
        }
        setDataPresent(RideFile::slope, true);
    }

    // Averages and Totals
    avgPoint->np = NPcount ? (NPtotal / NPcount) : 0;
    totalPoint->np = NPtotal;

    avgPoint->xp = XPcount ? (XPtotal / XPcount) : 0;
    totalPoint->xp = XPtotal;

    avgPoint->apower = APcount ? (APtotal / APcount) : 0;
    totalPoint->apower = APtotal;

    // and we're done
    dstale=false;
}

RideFile *
RideFile::resample(double newRecIntSecs, int interpolate)
{

    // resample if interval has changed
    if (newRecIntSecs != recIntSecs()) {

        QMap<SeriesType, QwtSpline *> splines;

        // we remember the last point in time with data 
        double last = 0;

        // create a spline for every series present in the ridefile
        for(int i=0; i < static_cast<int>(none); i++) {

            // save us casting all the time 
            SeriesType series = static_cast<SeriesType>(i);

            if (series == secs) continue; // don't resample that !

            // create a spline if its in the file
            if (isDataPresent(series)) {

                // collect the x,y points; x=time, y=series
                QVector<QPointF> points;

                double offset = 0; // always start from zero seconds (e.g. intervals start at and offset in ride)
                bool first = true;
                RideFilePoint *lp=NULL;

                foreach(RideFilePoint *p, dataPoints()) {

                    // yuck! nasty data -- ignore it
                    if (p->secs > (25*60*60)) continue;

                    // always start at 0 seconds
                    if (first) {
                        offset = p->secs;
                        first = false;
                    }

                    // fill gaps in recording with zeroes
                    if (lp) {

                        // fill with zeroes
                        for(double t=lp->secs+recIntSecs();
                                (t + recIntSecs()) < p->secs;
                                t += recIntSecs()) {
                            points << QPointF(t-offset, 0);
                        }
                    }

                    // lets not go backwards -- or two sampls at the same time
                    if ((lp && p->secs > lp->secs) || !lp) {
                        points << QPointF(p->secs - offset, p->value(series));
                        last = p->secs-offset;
                    }

                    // moving on to next sample
                    lp = p;
                }

                // Now create a spline with the values we've cleaned
                QwtSpline *spline = new QwtSpline();
                spline->setSplineType(QwtSpline::Periodic);
                spline->setPoints(QPolygonF(points));
                splines.insert(series,spline);
            }
        }

        // no data to resample
        if (splines.count() == 0 || last == 0) return NULL;

        // we have a bunch of splines so lets add resampled
        // data points to a clone of the current ride (ie. we
        // need to update a copy of this ride, not update it
        // directly)
        RideFile *returning = new RideFile(this);
        returning->setRecIntSecs(newRecIntSecs);
        returning->setDataPresent(secs, true);

        RideFilePoint lp;
        for (double seconds = 0.0f; seconds < (last-newRecIntSecs); seconds += newRecIntSecs) {

            RideFilePoint p;
            p.secs = seconds;

            // for each spline get the value for point secs
            QMapIterator<SeriesType, QwtSpline *> iterator(splines);
            while (iterator.hasNext()) {
                iterator.next();

                SeriesType series = iterator.key();
                QwtSpline *spline = iterator.value();

                double sum = 0;
                for (double i=0; i<1; i+= 0.25) {
                    double dt = seconds + (newRecIntSecs * i);
                    double dtn = seconds + (newRecIntSecs * (i+0.25f));
                    sum += (spline->value(dt) + spline->value(dtn)) /2.0f;
                }
                sum /= 4.0f;

                // round to the appropriate decimal places
                double rounded = 0.0f;
                if (decimalsFor(series) > 0)
                    rounded = QString("%1").arg(sum, 15, 'f', decimalsFor(series)).toDouble();
                else
                    rounded = qRound(sum);

                // don't go backwards for distance !
                if (series == km && rounded < lp.km)
                    p.setValue(series, lp.km);
                else
                    p.setValue(series, rounded);

                // make sure we get to see it !
                returning->setDataPresent(series, true);
            }
            returning->appendPoint(p);

            // remember last point
            lp = p;
        }

        // clean up and return
        // wipe away any splines created
        QMapIterator<SeriesType, QwtSpline *> iterator(splines);
        while (iterator.hasNext()) {
            iterator.next();
            delete iterator.value();
        }

        return returning;

    } else {

        // not resampling but cloning a working copy
        // and removing gaps in recording
        RideFile *returning = new RideFile(this);
        returning->setDataPresent(secs, true);

        // now clone the data points with gaps filled
        double offset = 0; // always start from zero seconds (e.g. intervals start at and offset in ride)
        bool first = true;
        RideFilePoint *lp=NULL;

        foreach(RideFilePoint *p, dataPoints()) {

            // yuck! nasty data -- ignore it
            if (p->secs > (25*60*60)) continue;

            // always start at 0 seconds
            if (first) {
                offset = p->secs;
                first = false;
            }

            // fill gaps in recording with zeroes
            if (lp) {

                // fill with zeroes
                for(double t=lp->secs+recIntSecs();
                        (t + recIntSecs()) < p->secs;
                        t += recIntSecs()) {

                    RideFilePoint add;
                    add.km = lp->km;
                    add.secs = t;

                    returning->appendPoint(add);
                }
            }

            // lets not go backwards -- or two sampls at the same time
            // but essentially just copying the data
            if ((lp && p->secs > lp->secs) || !lp) {

                RideFilePoint add;
                add = *p;
                add.secs -= offset;
                returning->appendPoint(add);
            }

            // moving on to next sample
            lp = p;
        }

        return returning;
    }
}
