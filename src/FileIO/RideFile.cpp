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
#include "FilterHRV.h"
#include "WPrime.h"
#include "Athlete.h"
#include "DataProcessor.h"
#include "RideEditor.h"
#include "RideMetadata.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"
#include "SplineLookup.h"

#include <QtXml/QtXml>
#include <algorithm> // for std::lower_bound
#include <assert.h>
#ifdef Q_CC_MSVC
#include <float.h>
#endif
#include <cmath>

#ifdef GC_HAVE_SAMPLERATE
// we have libsamplerate
#include <samplerate.h>
#endif

#include "../qzip/zipwriter.h"
#include "../qzip/zipreader.h"

#ifdef Q_CC_MSVC
#include <QtZlib/zlib.h>
#else
#include <zlib.h>
#endif

#define mark() \
{ \
    addInterval(RideFileInterval::USER, start, previous->secs, \
                QString("%1").arg(interval)); \
    interval = point->interval; \
    start = point->secs; \
}

const QChar deltaChar(0x0394);

RideFile::RideFile(const QDateTime &startTime, double recIntSecs) :
            wstale(true), startTime_(startTime), recIntSecs_(recIntSecs),
            data(NULL), wprime_(NULL),
            weight_(0), totalCount(0), totalTemp(0), dstale(true)
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
    wstale(true), recIntSecs_(p->recIntSecs_), data(NULL), wprime_(NULL),
    weight_(p->weight_), totalCount(0), totalTemp(0), dstale(true)
{
    startTime_ = p->startTime_;
    tags_ = p->tags_;
    referencePoints_ = p->referencePoints_;
    setDeviceType(p->deviceType());
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
    wstale(true), recIntSecs_(0.0), data(NULL), wprime_(NULL),
    weight_(0), totalCount(0), totalTemp(0), dstale(true)
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
    //foreach(RideFileCalibration *calibration, calibrations_)
        //delete calibration;
    //foreach(RideFileInterval *interval, intervals_)
        //delete interval;
    delete command;
    if (wprime_) delete wprime_;

    // delete any Xdata
    QMapIterator<QString,XDataSeries*> it(xdata_);
    while(it.hasNext()) {
        it.next();
        XDataSeries *p = it.value();
        delete p;
    }
    xdata_.clear();
    //!!! if (data) delete data; // need a mechanism to notify the editor
}

void RideFile::setStartTime(const QDateTime &value) {
    startTime_ = value;
}

unsigned int
RideFile::computeFileCRC(QString filename)
{
    QFile file(filename);
    QFileInfo fileinfo(file);

    // open file
    if (!file.open(QFile::ReadOnly)) return 0;

    // allocate space
    QScopedArrayPointer<char> data(new char[file.size()]);

    // read entire file into memory
    QDataStream *rawstream(new QDataStream(&file));
    rawstream->readRawData(&data[0], file.size());
    file.close();

#if QT_VERSION < 0x060000
    return qChecksum(&data[0], file.size());
#else
    return qChecksum(QByteArrayView(&data[0], file.size()));
#endif
}

void
RideFile::updateDataTag()
{
    QString flags;

    if (areDataPresent()->secs) flags += 'T'; // time
    else flags += '-';
    if (areDataPresent()->km) flags += 'D'; // distance
    else flags += '-';
    if (areDataPresent()->kph) flags += 'S'; // speed
    else flags += '-';
    if (areDataPresent()->watts) flags += 'P'; // Power
    else flags += '-';
    if (areDataPresent()->hr) flags += 'H'; // Heartrate
    else flags += '-';
    if (areDataPresent()->cad ||
        areDataPresent()->rcad) flags += 'C'; // cadence
    else flags += '-';
    if (areDataPresent()->nm) flags += 'N'; // Torque
    else flags += '-';
    if (areDataPresent()->alt) flags += 'A'; // Altitude
    else flags += '-';
    if (areDataPresent()->lat ||
        areDataPresent()->lon ) flags += 'G'; // GPS
    else flags += '-';
    if (areDataPresent()->slope) flags += 'L'; // Slope
    else flags += '-';
    if (areDataPresent()->headwind) flags += 'W'; // Windspeed
    else flags += '-';
    if (areDataPresent()->temp) flags += 'E'; // Temperature
    else flags += '-';
    if (areDataPresent()->lrbalance) flags += 'V'; // V for "Vector" aka lr pedal data
    else flags += '-';
    if (areDataPresent()->smo2 ||
        areDataPresent()->thb) flags += 'O'; // Moxy O2/Haemoglobin
    else flags += '-';
    if (areDataPresent()->rcontact ||
        areDataPresent()->rvert ||
        areDataPresent()->rcad) flags += 'R'; // R is for running dynamics
    else flags += '-';
    setTag("Data", flags);
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

QString
RideFile::sportTag(QString sport)
{
    // Some sports are standarized, all others are up to the user
    static const QHash<QString, QString> sports = {
        { tr("Bike"), "Bike" },
        { "Biking", "Bike" }, { tr("Biking"), "Bike" },
        { "Cycle", "Bike" }, { tr("Cycle"), "Bike" },
        { "Cycling", "Bike" }, { tr("Cycling"), "Bike" },

        { tr("Run"), "Run" },
        { "Running", "Run" }, { tr("Running"), "Run" },

        { tr("Swim"), "Swim" },
        { "Swimming", "Swim" }, { tr("Swimming"), "Swim" },

        { tr("Row"), "Row" },
        { "Rowing", "Row" }, { tr("Rowing"), "Row" },

        { tr("Ski"), "Ski" },
        { "XC Ski", "Ski" }, { tr("XC Ski"), "Ski" },
        { "Cross Country Skiiing", "Ski" }, { tr("Cross Countr Skiing"), "Ski" },

        { tr("Gym"), "Gym" },
        { "Strength", "Gym" }, { tr("Strength"), "Gym" },

        { tr("Walk"), "Walk" },
        { "Walking", "Walk" }, { tr("Walking"), "Walk" },
    };

    return sports.value(sport, sport);
}

QString
RideFile::sport() const
{
    // Run, Bike and Swim are standarized, all others are up to the user
    if (isBike()) return "Bike";
    if (isRun()) return "Run";
    if (isSwim()) return "Swim";
    return sportTag(getTag("Sport",""));
}

bool
RideFile::isBike() const
{
    // for now we just look at Sport and default to Bike when Sport is not
    // set and isRun and isSwim are false- but if its an aero test it must be bike
    return isAero() || (sportTag(getTag("Sport", "")) == "Bike") ||
           (getTag("Sport","").isEmpty() && !isRun() && !isSwim());
}

bool
RideFile::isRun() const
{
    // for now we just look at Sport and if there are any
    // running specific data series in the data when Sport is not set
    return (sportTag(getTag("Sport", "")) == "Run") ||
           (getTag("Sport","").isEmpty() && (areDataPresent()->rvert || areDataPresent()->rcad || areDataPresent()->rcontact));
}

bool
RideFile::isSwim() const
{
    // for now we just look at Sport or presence of length data for lap swims
    return (sportTag(getTag("Sport", "")) == "Swim") ||
           (getTag("Sport","").isEmpty() && xdata_.value("SWIM", NULL) != NULL);
}

bool
RideFile::isXtrain() const
{
    return !isBike() && !isRun() && !isSwim() && !isAero();
}

bool
RideFile::isAero() const
{
    return (getTag("Sport","") == "Aero" || xdata("AERO") != NULL);
}

// compatibility means used in e.g. R so no spaces in names,
// NOT allowed to be translated,
// and use naming convention e.g. trackeR R package
QString
RideFile::seriesName(SeriesType series, bool compat)
{
    if (compat) {
        switch (series) {
        case RideFile::secs: return QString("seconds");
        case RideFile::cad: return QString("cadence");
        case RideFile::hr: return QString("heart.rate");
        case RideFile::km: return QString("distance");
        case RideFile::kph: return QString("speed");
        case RideFile::kphd: return QString("acceleration");
        case RideFile::wattsd: return QString("powerd");
        case RideFile::cadd: return QString("cadenced");
        case RideFile::nmd: return QString("torqued");
        case RideFile::hrd: return QString("heart.rated");
        case RideFile::nm: return QString("torque");
        case RideFile::watts: return QString("power");
        case RideFile::xPower: return QString("xpower");
        case RideFile::aPower: return QString("apower");
        case RideFile::aTISS: return QString("atiss");
        case RideFile::anTISS: return QString("antiss");
        case RideFile::IsoPower: return QString("IsoPower");
        case RideFile::alt: return QString("altitude");
        case RideFile::lon: return QString("longitude");
        case RideFile::lat: return QString("latitude");
        case RideFile::headwind: return QString("headwind");
        case RideFile::slope: return QString("slope");
        case RideFile::temp: return QString("temperature");
        case RideFile::lrbalance: return QString("lrbalance");
        case RideFile::lte: return QString("lte");
        case RideFile::rte: return QString("rte");
        case RideFile::lps: return QString("lps");
        case RideFile::rps: return QString("rps");
        case RideFile::lpco: return QString("lpco");
        case RideFile::rpco: return QString("rpco");
        case RideFile::lppb: return QString("lppb");
        case RideFile::rppb: return QString("rppb");
        case RideFile::lppe: return QString("lppe");
        case RideFile::rppe: return QString("rppe");
        case RideFile::lpppb: return QString("lpppb");
        case RideFile::rpppb: return QString("rpppb");
        case RideFile::lpppe: return QString("lpppe");
        case RideFile::rpppe: return QString("rpppe");
        case RideFile::interval: return QString("interval");
        case RideFile::vam: return QString("vam");
        case RideFile::wattsKg: return QString("wpk");
        case RideFile::wbal:
        case RideFile::wprime: return QString("wbal");
        case RideFile::smo2: return QString("smo2");
        case RideFile::thb: return QString("thb");
        case RideFile::o2hb: return QString("o2hb");
        case RideFile::hhb: return QString("hhb");
        case RideFile::rvert: return QString("rvert");
        case RideFile::rcad: return QString("rcad");
        case RideFile::rcontact: return QString("gct");
        case RideFile::gear: return QString("gearratio");
        case RideFile::index: return QString("index");
        case RideFile::tcore: return QString("tcore");
        default: return QString("unknown");
        }
    } else {
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
        case RideFile::IsoPower: return QString(tr("Iso Power"));
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
        case RideFile::rps: return QString(tr("Right Pedal Smoothness"));
        case RideFile::lpco: return QString(tr("Left Platform Center Offset"));
        case RideFile::rpco: return QString(tr("Right Platform Center Offset"));
        case RideFile::lppb: return QString(tr("Left Power Phase Start"));
        case RideFile::rppb: return QString(tr("Right Power Phase Start"));
        case RideFile::lppe: return QString(tr("Left Power Phase End"));
        case RideFile::rppe: return QString(tr("Right Power Phase End"));
        case RideFile::lpppb: return QString(tr("Left Peak Power Phase Start"));
        case RideFile::rpppb: return QString(tr("Right Peak Power Phase Start"));
        case RideFile::lpppe: return QString(tr("Left Peak Power Phase End"));
        case RideFile::rpppe: return QString(tr("Right Peak Power Phase End"));
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
        case RideFile::wbal: return QString(tr("W' Consumed"));
        case RideFile::index: return QString(tr("Sample Index"));
        case RideFile::tcore: return QString("Core Temperature");
        default: return QString(tr("Unknown"));
        }
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
    case RideFile::IsoPower: return GColor(CNPOWER);
    case RideFile::alt: return GColor(CALTITUDE);
    case RideFile::headwind: return GColor(CWINDSPEED);
    case RideFile::temp: return GColor(CTEMP);
    case RideFile::lrbalance: return GColor(CBALANCELEFT);
    case RideFile::lte: return GColor(CLTE);
    case RideFile::rte: return GColor(CRTE);
    case RideFile::lps: return GColor(CLPS);
    case RideFile::rps: return GColor(CRPS);
    case RideFile::lpco: return GColor(CLPS);
    case RideFile::rpco: return GColor(CRPS);
    case RideFile::lppb: return GColor(CLPS);
    case RideFile::rppb: return GColor(CRPS);
    case RideFile::lppe: return GColor(CLPS);
    case RideFile::rppe: return GColor(CRPS);
    case RideFile::lpppb: return GColor(CLPS);
    case RideFile::rpppb: return GColor(CRPS);
    case RideFile::lpppe: return GColor(CLPS);
    case RideFile::rpppe: return GColor(CRPS);
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
    Q_UNUSED(context)

    bool useMetricUnits = GlobalContext::context()->useMetricUnits;

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
    case RideFile::IsoPower: return QString(tr("watts"));
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
    case RideFile::lpco: return QString(tr("mm"));
    case RideFile::rpco: return QString(tr("mm"));
    case RideFile::lppb: return QString(tr("°"));
    case RideFile::rppb: return QString(tr("°"));
    case RideFile::lppe: return QString(tr("°"));
    case RideFile::rppe: return QString(tr("°"));
    case RideFile::lpppb: return QString(tr("°"));
    case RideFile::rpppb: return QString(tr("°"));
    case RideFile::lpppe: return QString(tr("°"));
    case RideFile::rpppe: return QString(tr("°"));
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

QString
RideFile::formatValueWithUnit(double value, SeriesType series, Conversion conversion, Context *context, bool isSwim)
{
    bool useMetricUnits = GlobalContext::context()->useMetricUnits;

    if (series == RideFile::kph && conversion == RideFile::pace)
        return kphToPace(value, useMetricUnits, isSwim);
    else
        return QString("%1%2").arg(round(value)).arg(unitName(series, context));
        //TODO: make the rounding series dependent
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
    double start = dataPoints().first()->secs;
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

bool
RideFile::removeInterval(RideFileInterval*x)
{
    int index = intervals_.indexOf(x);
    if (index == -1) return false;
    else {
        intervals_.removeAt(index);
        return true;
    }
}

void
RideFile::moveInterval(int from, int to)
{
    intervals_.move(from, to);
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

QString RideFileInterval::typeDescription(intervaltype x)
{
    switch (x) {
    default:
    case ALL : return tr("ALL"); break;
    case DEVICE : return tr("DEVICE"); break;
    case USER : return tr("USER"); break;
    case PEAKPACE : return tr("PEAK PACE"); break;
    case PEAKPOWER : return tr("PEAK POWER"); break;
    case ROUTE : return tr("SEGMENTS"); break;
    case CLIMB : return tr("CLIMBING"); break;
    case EFFORT : return tr("EFFORTS"); break;
    }
}
QString RideFileInterval::typeDescriptionLong(intervaltype x)
{
    switch (x) {
    default:
    case ALL : return tr("The entire activity"); break;
    case DEVICE : return tr("Device specific intervals"); break;
    case USER : return tr("User defined laps or marked intervals"); break;
    case PEAKPACE : return tr("Peak pace for running and swimming"); break;
    case PEAKPOWER : return tr("Peak powers for cycling 1s thru 1hr"); break;
    case ROUTE : return tr("Route segments using GPS data"); break;
    case CLIMB : return tr("Ascents for hills and mountains"); break;
    case EFFORT : return tr("Sustained efforts and matches using power"); break;
    }
}
qint32 
RideFileInterval::intervalTypeBits(intervaltype x) // used for config what is/isn't autodiscovered
{
    switch (x) {
    default:
    case ALL : return 1;
    case DEVICE : return 0;
    case USER : return 0;
    case PEAKPACE : return 2;
    case PEAKPOWER : return 4;
    case ROUTE : return 8;
    case CLIMB : return 16;
    case EFFORT : return 32;
    }
}

int
RideFile::intervalBegin(const RideFileInterval &interval) const
{
    return intervalBeginSecs(interval.start);
}

int
RideFile::intervalBeginSecs(const double secs) const
{
    RideFilePoint p;
    p.secs = secs;
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

int RideFileFactory::registerReader(const QString &suffix, const QString &description, RideFileReader *reader)
{
    readFuncs_.insert(suffix, reader);
    descriptions_.insert(suffix, description);
    return 1;
}

QStringList RideFileFactory::suffixes() const
{
    QStringList returning = readFuncs_.keys();
    returning += "zip";
    returning += "gz";
    return returning;
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

bool RideFileFactory::supportedFormat(QString filename) const
{
    return suffixes().contains(QFileInfo(filename).suffix());
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

static QByteArray gUncompress(const QByteArray &data)
{
    if (data.size() <= 4) {
        qWarning("gUncompress: Input data is truncated");
        return QByteArray();
    }

    QByteArray result;

    int ret;
    z_stream strm;
    static const int CHUNK_SIZE = 1024;
    char out[CHUNK_SIZE];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = data.size();
    strm.next_in = (Bytef*)(data.data());

    ret = inflateInit2(&strm, 15 +  16); // gzip decoding
    if (ret != Z_OK)
        return QByteArray();

    // run inflate()
    do {
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = (Bytef*)(out);

        ret = inflate(&strm, Z_NO_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR);  // state not clobbered

        switch (ret) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void)inflateEnd(&strm);
            return QByteArray();
        }

        result.append(out, CHUNK_SIZE - strm.avail_out);
    } while (strm.avail_out == 0);

    // clean up and return
    inflateEnd(&strm);
    return result;
}

RideFile *RideFileFactory::openRideFile(Context *context, QFile &file,
                                           QStringList &errors, QList<RideFile*> *rideList) const
{

    // since some file names contain "." as separator, not only for suffixes
    // find the file-type suffix and the compression type in a 2 step approach
    QStringList allNameParts = QFileInfo(file).fileName().split(".");
    QString compression;
    QString suffix;
    if (!allNameParts.isEmpty()) {
        if (allNameParts.last().toLower() == "zip" ||
            allNameParts.last().toLower() == "gz") {
            // gz/zip are handled by openRideFile
            compression = allNameParts.last();
            allNameParts.removeLast();
        }
        if (!allNameParts.isEmpty()) {
            suffix = allNameParts.last();
        }
    }

    // did we uncompress?
    QByteArray data;
    bool uncompressed=false;

    // the result we will return
    RideFile *result;

    // decompress if its compressed data
    if (compression == "zip") {

        // open zip and uncompress
        ZipReader reader(file.fileName());
        ZipReader::FileInfo info = reader.entryInfoAt(0);
        data = reader.fileData(info.filePath);

        uncompressed = true;

    }

    // decompress
    if (compression == "gz") {

        // read and uncompress
        file.open(QFile::ReadOnly);
        data = gUncompress(file.readAll());
        file.close();

        uncompressed = true;
    }

    // do we have a reader for this type of file?
    RideFileReader *reader = readFuncs_.value(suffix.toLower());
    if (!reader) return NULL;

    // if we uncompressed a ride, we need to save to a temporary ride for import
    if (uncompressed) {

        // create a temporary ride
        QString tmp = context->athlete->home->temp().absolutePath() + "/" + QFileInfo(file.fileName()).baseName() + "." + suffix;

        QFile ufile(tmp); // look at uncompressed version mot the source
        ufile.open(QFile::ReadWrite);
        ufile.write(data);
        ufile.close();

        // open and read the  uncompressed file
        result = reader->openRideFile(ufile, errors, rideList);

        // now zap the temporary file
        ufile.remove();

    } else {

        // open and read the file
        result = reader->openRideFile(file, errors, rideList);
    }

    // if it was successful, lets post process the file
    if (result) {

        result->context = context;

        // post process metadata- take tags from main metadata
        // and move to interval metadata where there is a match
        // this really only applies to JSON ride files
        QStringList removelist;
        QMap<QString,QString>::const_iterator i;
        for (i=result->tags().constBegin(); i != result->tags().constEnd(); i++) {

            QString name = i.key();
            QString value = i.value();

            // if contains '##' we should id it as interval metadata
            if (name.contains("##")) {
                bool found=false;
                foreach(FieldDefinition x, GlobalContext::context()->rideMetadata->getFields()) {
                    if (x.interval == true) {
                        if (name.endsWith("##" + x.name)) {
                            // we have some metadata, lets see if it matches
                            // any intervals we have defined
                            foreach(RideFileInterval *p, result->intervals()) {
                                if (name.startsWith(p->name + "##")) {
                                    // we have a winner, lets transfer
                                    p->setTag(x.name, value);
                                    found=true;
                                    removelist << name;
                                    break;
                                }
                            }
                        }
                    }
                    if (found) break;
                }
            }
        }
        // now remove metadata that was inserted into
        // interval metadata from the main tags
        foreach(QString key, removelist) result->tags_.remove(key);

        if (result->intervals().empty()) result->fillInIntervals();
        // override the file ride time with that set from the filename
        // but only if it matches the GC format
        QFileInfo fileInfo(file.fileName());

        // Regular expression to match either date format, including a mix of dashes and underscores
        // yyyy-MM-dd-hh-mm-ss.extension
        // or yyyy_MM_dd_hh_mm_ss.extension
        // year is the only one matching for 4 digits, the rest can either be 1 or 2 digits.
        QRegExp rx ("^((\\d{4})[-_](\\d{1,2})[-_](\\d{1,2})[-_](\\d{1,2})[-_](\\d{1,2})[-_](\\d{1,2}))\\.(.+)$");
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

        // set other "special" fields
        result->setTag("Filename", QFileInfo(file.fileName()).fileName());
        result->setTag("File Format", result->fileFormat());
        if (context) result->setTag("Athlete", context->athlete->cyclist);
        result->setTag("Year", result->startTime().toString("yyyy"));
        result->setTag("Month", result->startTime().toString("MMMM"));
        result->setTag("Weekday", result->startTime().toString("ddd"));

        // reset timestamps and distances to always start from zero
        double timeOffset=0.00f, kmOffset=0.00f;
        if (result->dataPoints().count()) {
            timeOffset=result->dataPoints()[0]->secs;
            kmOffset=result->dataPoints()[0]->km;
        }

        // drag back samples
        if (timeOffset || kmOffset) {
            foreach (RideFilePoint *p, result->dataPoints()) {
                p->km = p->km - kmOffset;
                p->secs = p->secs - timeOffset;
            }
        }

        // drag back intervals
        foreach(RideFileInterval *i, result->intervals()) {
            i->start -= timeOffset;
            i->stop -= timeOffset;
        }

        // calculate derived data series -- after data fixers applied above
        // Update presens and filter HRV
        XDataSeries *series = result->xdata("HRV");

        if (series && series->datapoints.count() > 0) {
            double rrMax = appsettings->value(NULL, GC_RR_MAX, "2000.0").toDouble();
            double rrMin = appsettings->value(NULL, GC_RR_MIN, "270.0").toDouble();
            double rrFilt = appsettings->value(NULL, GC_RR_FILT, "0.2").toDouble();
            int rrWindow = appsettings->value(NULL, GC_RR_WINDOW, "20").toInt();

            FilterHrv(series, rrMin, rrMax, rrFilt, rrWindow);
        }

        // calculate derived data series -- after data fixers applied above
        if (context) result->recalculateDerivedSeries();

        // what data is present - after processor in case 'derived' or adjusted
        result->updateDataTag();

        //foreach(RideFile::seriestype x, result->arePresent()) qDebug()<<"present="<<x;

        // sample code for using XDATA, left here temporarily till we have an
        // example of using it in a ride file reader
#if 0

        // ADD XDATA TO RIDEFILE

        // For testing xdata, this code just adds an xdata series
        // XDataSeries *xdata = new XDataSeries();
        // xdata->name = "SPEED";
        // xdata->valuename << "SPEED";
        // for(int i=0; i<100; i++) {
        // XDataPoint *p = new XDataPoint();
        // p->km = i;
        // p->secs = i;
        // p->number[0] = i;
        // xdata->datapoints.append(p);
        // }
        // result->addXData("SPEED", xdata);

        // DEBUG OUTPUT TO SHOW XDATA LOADED FROM RIDEFILE
        // for testing, print out what we loaded
        if (result->xdata_.count()) {

            // output the xdata series
            qDebug()<<"XDATA";

            QMapIterator<QString,XDataSeries*> xdata(result->xdata());
            xdata.toFront();
            while(xdata.hasNext()) {

                // iterate
                xdata.next();

                XDataSeries *series = xdata.value();

                // does it have values names?
                if (series->valuename.isEmpty()) {
                    qDebug()<<"empty xdata"<<series->name;
                    continue;
                } else {
                    qDebug()<<"xdata" <<series->name<<series->valuename<<series->datapoints.count();
                }

                // samples
                if (series->datapoints.count()) {
                    foreach(XDataPoint *p, series->datapoints)
                        qDebug()<<"sample:"<<p->secs<<p->km<<p->number[0]<<p->number[1];
                }
            }
        }
#endif
    }

    return result;
}

void
RideFile::addXData(QString name, XDataSeries *series)
{
    xdata_.insert(name, series);
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

double
RideFile::xdataValue(RideFilePoint *p, int &idx, QString sxdata, QString series, RideFile::XDataJoin xjoin)
{
    double returning = RideFile::NA;
    XDataSeries *s = xdata(sxdata);

    // if not there or no values return NA
    if (s == NULL || !s->valuename.contains(series) || s->datapoints.count()==0)
        return RideFile::NA;

    // get index of series we care about
    int vindex = s->valuename.indexOf(series);

    // where are we in the ride?
    double secs = p->secs;

    // do we need to move on?
    while (idx < s->datapoints.count() && s->datapoints[idx]->secs < secs)
        idx++;

    // so at this point we are looking at a point that is either
    // the same point as us or is ahead of us

    if (idx >= s->datapoints.count()) {
        //
        // PAST LAST XDATA
        //

        // return the last value we saw
        switch(xjoin) {
        case INTERPOLATE:
        case SPARSE:
        case RESAMPLE:
            returning = RideFile::NIL;
            break;

        case REPEAT:
            if (idx) returning = s->datapoints[idx-1]->number[vindex];
            else  returning = RideFile::NIL;
            break;
        }

    } else if (fabs(s->datapoints[idx]->secs - secs) < recIntSecs()) {
        //
        // ITS THE SAME AS US!
        //
        // if its a match we always take the value
        returning = s->datapoints[idx]->number[vindex];
    } else {
        //
        // ITS IN THE FUTURE
        //

        switch(xjoin) {
        case INTERPOLATE:
            if (idx) {
                // interpolate then
                double gap = s->datapoints[idx]->secs - s->datapoints[idx-1]->secs;
                double diff = secs - s->datapoints[idx-1]->secs;
                double ratio = diff/gap;
                double vgap = s->datapoints[idx]->number[vindex] - s->datapoints[idx-1]->number[vindex];
                returning = s->datapoints[idx-1]->number[vindex] + (vgap * ratio);
            }
            break;

        case SPARSE:
            returning = RideFile::NIL;
            break;

        case RESAMPLE:
            returning = RideFile::NIL;
            break;

        case REPEAT:
            // for now, just return the last value we saw
            if (idx) returning = s->datapoints[idx-1]->number[vindex];
            else  returning = RideFile::NA;
            break;
        }
    }
    return returning;
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
    if (minPoint->lon == 0.0 || (point->lon != 0.0 && point->lon<minPoint->lon))
       minPoint->lon = point->lon;
    if (minPoint->lat == 0.0 || (point->lat != 0.0 && point->lat<minPoint->lat))
       minPoint->lat = point->lat;
    if (point->headwind<minPoint->headwind)
       minPoint->headwind = point->headwind;
    if (point->slope<minPoint->slope)
       minPoint->slope = point->slope;
    if (point->temp != NA && point->temp<minPoint->temp)
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
    if (point->lpco<minPoint->lpco)
       minPoint->lpco = point->lpco;
    if (point->rpco<minPoint->rpco)
       minPoint->rpco = point->rpco;
    if (minPoint->lppb == 0 || point->lppb<minPoint->lppb)
       minPoint->lppb = point->lppb;
    if (minPoint->rppb == 0 || point->rppb<minPoint->rppb)
       minPoint->rppb = point->rppb;
    if (minPoint->lppe == 0 || point->lppe<minPoint->lppe)
       minPoint->lppe = point->lppb;
    if (minPoint->rppe == 0 || point->rppe<minPoint->rppe)
       minPoint->rppe = point->rppe;
    if (minPoint->lpppb == 0 || point->lpppb<minPoint->lpppb)
       minPoint->lpppb = point->lpppb;
    if (minPoint->rpppb == 0 || point->rpppb<minPoint->rpppb)
       minPoint->rpppb = point->rpppb;
    if (minPoint->lpppe == 0 || point->lpppe<minPoint->lpppe)
       minPoint->lpppe = point->lpppb;
    if (minPoint->rpppe == 0 || point->rpppe<minPoint->rpppe)
       minPoint->rpppe = point->rpppe;
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
    if (minPoint->tcore == 0 || point->tcore<minPoint->tcore)
       minPoint->tcore = point->tcore;
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
    if (maxPoint->lon == 0.0 || (point->lon != 0.0 && point->lon>maxPoint->lon))
       maxPoint->lon = point->lon;
    if (maxPoint->lat == 0.0 || (point->lat != 0.0 && point->lat>maxPoint->lat))
       maxPoint->lat = point->lat;
    if (point->headwind>maxPoint->headwind)
       maxPoint->headwind = point->headwind;
    if (point->slope>maxPoint->slope)
       maxPoint->slope = point->slope;
    if (point->temp != NA && point->temp>maxPoint->temp)
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
    if (point->lpco>maxPoint->lpco)
       maxPoint->lpco = point->lpco;
    if (point->rpco>maxPoint->rpco)
       maxPoint->rpco = point->rpco;
    if (point->lppb>maxPoint->lppb)
       maxPoint->lppb = point->lppb;
    if (point->rppb>maxPoint->rppb)
       maxPoint->rppb = point->rppb;
    if (point->lppe>maxPoint->lppe)
       maxPoint->lppe = point->lppb;
    if (point->rppe>maxPoint->rppe)
       maxPoint->rppe = point->rppe;
    if (point->lpppb>maxPoint->lpppb)
       maxPoint->lpppb = point->lpppb;
    if (point->rpppb>maxPoint->rpppb)
       maxPoint->rpppb = point->rpppb;
    if (point->lpppe>maxPoint->lpppe)
       maxPoint->lpppe = point->lpppb;
    if (point->rpppe>maxPoint->rpppe)
       maxPoint->rpppe = point->rpppe;
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
    if (point->tcore>maxPoint->tcore)
       maxPoint->tcore = point->tcore;
}

void RideFile::updateAvg(RideFilePoint* point)
{
    if (point!=NULL) {
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
        totalPoint->temp += point->temp == NA ? 0 : point->temp;
        totalPoint->lte += point->lte;
        totalPoint->rte += point->rte;
        totalPoint->lps += point->lps;
        totalPoint->rps += point->rps;
        totalPoint->lrbalance += point->lrbalance;
        totalPoint->lpco += point->lpco;
        totalPoint->rpco += point->rpco;
        totalPoint->lppb += point->lppb;
        totalPoint->rppb += point->rppb;
        totalPoint->rppe += point->rppe;
        totalPoint->lpppb += point->lpppb;
        totalPoint->rpppb += point->rpppb;
        totalPoint->lpppe += point->lpppe;
        totalPoint->rpppe += point->rpppe;
        totalPoint->smo2 += point->smo2;
        totalPoint->thb += point->thb;
        totalPoint->o2hb += point->o2hb;
        totalPoint->hhb += point->hhb;
        totalPoint->rvert += point->rvert;
        totalPoint->rcad += point->rcad;
        totalPoint->rcontact += point->rcontact;
        totalPoint->gear += point->gear;
        totalPoint->tcore += point->tcore;

        ++totalCount;
        if (point->temp != NA) ++totalTemp;
    }

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
    avgPoint->lpco = totalPoint->lpco/totalCount;
    avgPoint->rpco = totalPoint->rpco/totalCount;
    avgPoint->lppb = totalPoint->lppb/totalCount;
    avgPoint->rppb = totalPoint->rppb/totalCount;
    avgPoint->lppe = totalPoint->lppe/totalCount;
    avgPoint->rppe = totalPoint->rppe/totalCount;
    avgPoint->lpppb = totalPoint->lpppb/totalCount;
    avgPoint->rpppb = totalPoint->rpppb/totalCount;
    avgPoint->lpppe = totalPoint->lpppe/totalCount;
    avgPoint->rpppe = totalPoint->rpppe/totalCount;
    avgPoint->smo2 = totalPoint->smo2/totalCount;
    avgPoint->thb = totalPoint->thb/totalCount;
    avgPoint->o2hb = totalPoint->o2hb/totalCount;
    avgPoint->hhb = totalPoint->hhb/totalCount;
    avgPoint->rvert = totalPoint->rvert/totalCount;
    avgPoint->rcad = totalPoint->rcad/totalCount;
    avgPoint->rcontact = totalPoint->rcontact/totalCount;
    avgPoint->gear = totalPoint->gear/totalCount;
    avgPoint->tcore = totalPoint->tcore/totalCount;
}

void RideFile::updateAvg(SeriesType series, double value)
{
   switch (series) {
        case secs : totalPoint->secs += value; break;
        case cad : totalPoint->cad += value; break;
        case hr : totalPoint->hr += value; break;
        case km : totalPoint->km += value; break;
        case kph : totalPoint->kph += value; break;
        case nm : totalPoint->nm += value; break;
        case watts : totalPoint->watts += value; break;
        case alt : totalPoint->alt += value; break;
        case lon : totalPoint->lon += value; break;
        case lat : totalPoint->lat += value; break;
        case headwind : totalPoint->headwind += value; break;
        case slope : totalPoint->slope += value; break;
        case temp : totalPoint->temp += value; break;
        case lrbalance : totalPoint->lrbalance += value; break;
        case lte : totalPoint->lte += value; break;
        case rte : totalPoint->rte += value; break;
        case lps : totalPoint->lps += value; break;
        case rps : totalPoint->rps += value; break;
        case lpco : totalPoint->lpco += value; break;
        case rpco : totalPoint->rpco += value; break;
        case lppb : totalPoint->lppb += value; break;
        case rppb : totalPoint->rppb += value; break;
        case lppe : totalPoint->lppe += value; break;
        case rppe : totalPoint->rppe += value; break;
        case lpppb : totalPoint->lpppb += value; break;
        case rpppb : totalPoint->rpppb += value; break;
        case lpppe : totalPoint->lpppe += value; break;
        case rpppe : totalPoint->rpppe += value; break;
        case smo2 : totalPoint->smo2 += value; break;
        case thb : totalPoint->thb += value; break;
        case o2hb : totalPoint->o2hb += value; break;
        case hhb : totalPoint->hhb += value; break;
        case rvert : totalPoint->rvert += value; break;
        case rcad : totalPoint->rcad += value; break;
        case rcontact : totalPoint->rcontact += value; break;
        case gear : totalPoint->gear += value; break;
        case tcore : totalPoint->tcore += value; break;
        case wbal : break; // not present
        default:
        case none : break;
    }
    updateAvg(NULL);
}

void RideFile::appendPoint(double secs, double cad, double hr, double km,
                           double kph, double nm, double watts, double alt,
                           double lon, double lat, double headwind,
                           double slope, double temp, double lrbalance,
                           double lte, double rte, double lps, double rps,
                           double lpco, double rpco,
                           double lppb, double rppb, double lppe, double rppe,
                           double lpppb, double rpppb, double lpppe, double rpppe,
                           double smo2, double thb,
                           double rvert, double rcad, double rcontact, double tcore,
                           int interval)
{
    appendOrUpdatePoint(secs,cad,hr,km,kph,
                nm,watts,alt,lon,lat,
                headwind, slope,
                temp, lrbalance,
                lte, rte, lps, rps,
                lpco, rpco,
                lppb, rppb, lppe, rppe,
                lpppb, rpppb, lpppe, rpppe,
                smo2, thb,
                rvert, rcad, rcontact, tcore,
                interval, true);
}

void RideFile::appendOrUpdatePoint(double secs, double cad, double hr, double km,
                           double kph, double nm, double watts, double alt,
                           double lon, double lat, double headwind,
                           double slope, double temp, double lrbalance, 
                           double lte, double rte, double lps, double rps,
                           double lpco, double rpco,
                           double lppb, double rppb, double lppe, double rppe,
                           double lpppb, double rpppb, double lpppe, double rpppe,
                           double smo2, double thb,
                           double rvert, double rcad, double rcontact, double tcore,
                           int interval, bool forceAppend)
{
    // negative values are not good, make them zero
    // although alt, lat, lon, headwind, slope and temperature can be negative of course!
#ifdef Q_CC_MSVC
    if (!_finite(secs) || secs<0) secs=0;
    if (!_finite(cad) || cad<0) cad=0;
    if (!_finite(hr) || hr<0) hr=0;
    if (!_finite(km) || km<0) km=0;
    if (!_finite(kph) || kph<0) kph=0;
    if (!_finite(nm) || nm<0) nm=0;
    if (!_finite(watts) || watts<0) watts=0;
    if (!_finite(interval) || interval<0) interval=0;
    if (!_finite(lps) || lps<0) lps=0;
    if (!_finite(rps) || rps<0) rps=0;
    if (!_finite(lte) || lte<0) lte=0;
    if (!_finite(rte) || rte<0) rte=0;
    if (!_finite(lppb) || lppb<0) lppb=0;
    if (!_finite(rppb) || rppb<0) rppb=0;
    if (!_finite(lppe) || lppe<0) lppe=0;
    if (!_finite(rppe) || rppe<0) rppe=0;
    if (!_finite(lpppb) || lpppb<0) lpppb=0;
    if (!_finite(rpppb) || rpppb<0) rpppb=0;
    if (!_finite(lpppe) || lpppe<0) lpppe=0;
    if (!_finite(rpppe) || rpppe<0) rpppe=0;
    if (!_finite(smo2) || smo2<0) smo2=0;
    if (!_finite(thb) || thb<0) thb=0;
    if (!_finite(rvert) || rvert<0) rvert=0;
    if (!_finite(rcad) || rcad<0) rcad=0;
    if (!_finite(rcontact) || rcontact<0) rcontact=0;
    if (!_finite(tcore) || tcore<0) tcore=0;
#else
    if (!std::isfinite(secs) || secs<0) secs=0;
    if (!std::isfinite(cad) || cad<0) cad=0;
    if (!std::isfinite(hr) || hr<0) hr=0;
    if (!std::isfinite(km) || km<0) km=0;
    if (!std::isfinite(kph) || kph<0) kph=0;
    if (!std::isfinite(nm) || nm<0) nm=0;
    if (!std::isfinite(watts) || watts<0) watts=0;
    if (!std::isfinite(interval) || interval<0) interval=0;
    if (!std::isfinite(lps) || lps<0) lps=0;
    if (!std::isfinite(rps) || rps<0) rps=0;
    if (!std::isfinite(lte) || lte<0) lte=0;
    if (!std::isfinite(rte) || rte<0) rte=0;
    if (!std::isfinite(lppb) || lppb<0) lppb=0;
    if (!std::isfinite(rppb) || rppb<0) rppb=0;
    if (!std::isfinite(lppe) || lppe<0) lppe=0;
    if (!std::isfinite(rppe) || rppe<0) rppe=0;
    if (!std::isfinite(lpppb) || lpppb<0) lpppb=0;
    if (!std::isfinite(rpppb) || rpppb<0) rpppb=0;
    if (!std::isfinite(lpppe) || lpppe<0) lpppe=0;
    if (!std::isfinite(rpppe) || rpppe<0) rpppe=0;
    if (!std::isfinite(smo2) || smo2<0) smo2=0;
    if (!std::isfinite(thb) || thb<0) thb=0;
    if (!std::isfinite(rvert) || rvert<0) rvert=0;
    if (!std::isfinite(rcad) || rcad<0) rcad=0;
    if (!std::isfinite(rcontact) || rcontact<0) rcontact=0;
    if (!std::isfinite(tcore) || tcore<0) tcore=0;
#endif

    // if bad time or distance ignore it if NOT the first sample
    if (dataPoints_.count() != 0 && secs == 0.00f && km == 0.00f) return;

    // truncate alt out of bounds -- ? should do for all, but uncomfortable about
    //                                 setting an absolute max. At least We know the highest
    //                                 point on Earth (Mt Everest).
    if (alt > RideFile::maximumFor(RideFile::alt)) alt = RideFile::maximumFor(RideFile::alt);

    RideFilePoint* point = new RideFilePoint(secs, cad, hr, km, kph, nm, watts, alt, lon, lat,
                                             headwind, slope, temp,
                                             lrbalance,
                                             lte, rte, lps, rps,
                                             lpco, rpco,
                                             lppb, rppb, lppe, rppe,
                                             lpppb, rpppb, lpppe, rpppe,
                                             smo2, thb,
                                             rvert, rcad, rcontact, tcore,
                                             interval);


    if (!forceAppend) {

        int idx = timeIndex(secs);
        if (idx != -1) {
            if (dataPoints_.at(idx)->secs == secs) {
                updatePoint(point, dataPoints_.at(idx));
                *dataPoints_.at(idx) = *point;
                delete point;
            } else {
                if (dataPoints_.at(idx)->secs > secs)
                    dataPoints_.insert(idx, point);
                else
                    dataPoints_.insert(idx+1, point);
            }
        } else
           forceAppend = true; // note if clause below
    }

    if (forceAppend) { // note forceAppend = true above do not convert to else clause
        dataPoints_.append(point);
    }

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
    dataPresent.temp     |= (temp != NA);
    dataPresent.lrbalance|= (lrbalance != 0 && lrbalance != NA);
    dataPresent.lte      |= (lte != 0);
    dataPresent.rte      |= (rte != 0);
    dataPresent.lps      |= (lps != 0);
    dataPresent.rps      |= (rps != 0);
    dataPresent.lpco     |= (lpco != 0);
    dataPresent.rpco     |= (rpco != 0);
    dataPresent.lppb     |= (lppb != 0);
    dataPresent.rppb     |= (rppb != 0);
    dataPresent.lppe     |= (lppe != 0);
    dataPresent.rppe     |= (rppe != 0);
    dataPresent.lpppb    |= (lpppb != 0);
    dataPresent.rpppb    |= (rpppb != 0);
    dataPresent.lpppe    |= (lpppe != 0);
    dataPresent.rpppe    |= (rpppe != 0);
    dataPresent.smo2     |= (smo2 != 0);
    dataPresent.thb      |= (thb != 0);
    dataPresent.rvert    |= (rvert != 0);
    dataPresent.rcad     |= (rcad != 0);
    dataPresent.rcontact |= (rcontact != 0);
    dataPresent.tcore    |= (tcore != 0);
    dataPresent.interval |= (interval != 0);

    updateMin(point);
    updateMax(point);
    updateAvg(point);
}

void RideFile::appendPoint(const RideFilePoint &point)
{
    appendPoint(point.secs,point.cad,point.hr,point.km,point.kph,
                point.nm,point.watts,point.alt,point.lon,point.lat,
                point.headwind, point.slope,
                point.temp, point.lrbalance,
                point.lte, point.rte, point.lps, point.rps,
                point.lpco, point.rpco,
                point.lppb, point.rppb, point.lppe, point.rppe,
                point.lpppb, point.rpppb, point.lpppe, point.rpppe,
                point.smo2, point.thb,
                point.rvert, point.rcad, point.rcontact, point.tcore,
                point.interval);
}

void
RideFile::updatePoint(RideFilePoint *point, const RideFilePoint *oldPoint){
    if (point->cad == 0 && oldPoint->cad != 0)
        point->cad = oldPoint->cad;
    if (point->hr == 0 && oldPoint->hr != 0)
        point->hr = oldPoint->hr;
    if (point->km == 0 && oldPoint->km != 0)
        point->km = oldPoint->km;
    if (point->kph == 0 && oldPoint->kph != 0)
        point->kph = oldPoint->kph;

    if (point->nm == 0 && oldPoint->nm != 0)
        point->nm = oldPoint->nm;
    if (point->watts == 0 && oldPoint->watts != 0)
        point->watts = oldPoint->watts;
    if (point->alt == 0 && oldPoint->alt != 0)
        point->alt = oldPoint->alt;
    if (point->lat == 0 && oldPoint->lat != 0)
        point->lat = oldPoint->lat;
    if (point->lon == 0 && oldPoint->lon != 0)
        point->lon = oldPoint->lon;

    if (point->headwind == 0 && oldPoint->headwind != 0)
        point->headwind = oldPoint->headwind;
    if (point->slope == 0 && oldPoint->slope != 0)
        point->slope = oldPoint->slope;
    if (point->temp == RideFile::NA && oldPoint->temp != RideFile::NA)
        point->temp = oldPoint->temp;
    if (point->lrbalance == RideFile::NA && oldPoint->lrbalance != RideFile::NA)
        point->lrbalance = oldPoint->lrbalance;

    if (point->lte == 0 && oldPoint->lte != 0)
        point->lte = oldPoint->lte;
    if (point->rte == 0 && oldPoint->rte != 0)
        point->rte = oldPoint->rte;
    if (point->lps == 0 && oldPoint->lps != 0)
        point->lps = oldPoint->lps;
    if (point->rps == 0 && oldPoint->rps != 0)
        point->rps = oldPoint->rps;

    if (point->lpco == 0 && oldPoint->lpco != 0)
        point->lpco = oldPoint->lpco;
    if (point->rpco == 0 && oldPoint->rpco != 0)
        point->rpco = oldPoint->rpco;

    if (point->lppb == 0 && oldPoint->lppb != 0)
        point->lppb = oldPoint->lppb;
    if (point->rppb == 0 && oldPoint->rppb != 0)
        point->rppb = oldPoint->rppb;
    if (point->lppe == 0 && oldPoint->lppe != 0)
        point->lppe = oldPoint->lppe;
    if (point->rppe == 0 && oldPoint->rppe != 0)
        point->rppe = oldPoint->rppe;

    if (point->lpppb == 0 && oldPoint->lpppb != 0)
        point->lpppb = oldPoint->lpppb;
    if (point->rpppb == 0 && oldPoint->rpppb != 0)
        point->rpppb = oldPoint->rpppb;
    if (point->lpppe == 0 && oldPoint->lpppe != 0)
        point->lpppe = oldPoint->lpppe;
    if (point->rpppe == 0 && oldPoint->rpppe != 0)
        point->rpppe = oldPoint->rpppe;

    if (point->smo2 == 0 && oldPoint->smo2 != 0)
        point->smo2 = oldPoint->smo2;
    if (point->thb == 0 && oldPoint->thb != 0)
        point->thb = oldPoint->thb;

    if (point->rvert == 0 && oldPoint->rvert != 0)
        point->rvert = oldPoint->rvert;
    if (point->rcad == 0 && oldPoint->rcad != 0)
        point->rcad = oldPoint->rcad;
    if (point->rcontact == 0 && oldPoint->rcontact != 0)
        point->rcontact = oldPoint->rcontact;
    if (point->tcore == 0 && oldPoint->tcore != 0)
        point->tcore = oldPoint->tcore;

    if (point->interval == 0 && oldPoint->interval != 0)
        point->interval = oldPoint->interval;
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
        case lpco : dataPresent.lpco = value; break;
        case rpco : dataPresent.rpco = value; break;
        case lppb : dataPresent.lppb = value; break;
        case rppb : dataPresent.rppb = value; break;
        case lppe : dataPresent.lppe = value; break;
        case rppe : dataPresent.rppe = value; break;
        case lpppb : dataPresent.lpppb = value; break;
        case rpppb : dataPresent.rpppb = value; break;
        case lpppe : dataPresent.lpppe = value; break;
        case rpppe : dataPresent.rpppe = value; break;
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
        case tcore : dataPresent.tcore = value; break;
        case wbal : break; // not present
        default:
        case none : break;
    }
    updateDataTag();
}

bool
RideFile::isDataPresent(SeriesType series)
{
    switch (series) {
        case secs : return dataPresent.secs; break;
        case cadd :
        case cad : return dataPresent.cad; break;
        case hrd :
        case hr : return dataPresent.hr; break;
        case km : return dataPresent.km; break;
        case kphd :
        case kph : return dataPresent.kph; break;
        case nmd :
        case nm : return dataPresent.nm; break;
        case wbal :
        case wattsKg :
        case wattsd :
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
        case lpco : return dataPresent.lpco; break;
        case rpco : return dataPresent.rpco; break;
        case lppb : return dataPresent.lppb; break;
        case rppb : return dataPresent.rppb; break;
        case lppe : return dataPresent.lppe; break;
        case rppe : return dataPresent.rppe; break;
        case lpppb : return dataPresent.lpppb; break;
        case rpppb : return dataPresent.rpppb; break;
        case lpppe : return dataPresent.lpppe; break;
        case rpppe : return dataPresent.rpppe; break;
        case smo2 : return dataPresent.smo2; break;
        case thb : return dataPresent.thb; break;
        case o2hb : return dataPresent.o2hb; break;
        case hhb : return dataPresent.hhb; break;
        case rvert : return dataPresent.rvert; break;
        case rcad : return dataPresent.rcad; break;
        case rcontact : return dataPresent.rcontact; break;
        case gear : return dataPresent.gear; break;
        case interval : return dataPresent.interval; break;
        case tcore : return dataPresent.tcore; break;
        default:
        case none : return false; break;
    }
    return false;
}

void
RideFile::setPointValue(double secs, SeriesType series, double value) {
    int idx = timeIndex(secs);
    if ((idx != -1) && (dataPoints_.at(idx)->secs == secs)) {
        double previousVal = getPointValue(idx, series);
        setPointValue(idx, series, value);
        setDataPresent(series, true);

        updateAvg(series, value-previousVal);
        updateMin(dataPoints_.at(idx));
        updateMax(dataPoints_.at(idx));
    }
}

// void
// RideFile::setPointValue(double secs, SeriesType series, int value) {
//     int idx = timeIndex(secs);
//     if ((idx != -1) && (dataPoints_.at(idx)->secs == secs)) {
//         int previousVal = getPointValue(idx, series);
//         setPointValue(idx, series, value);
//         setDataPresent(series, true);

//         updateAvg(series, value-previousVal);
//         updateMin(dataPoints_.at(idx));
//         updateMax(dataPoints_.at(idx));
//     }
// }

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
        case lpco : dataPoints_[index]->lpco = value; break;
        case rpco : dataPoints_[index]->rpco = value; break;
        case lppb : dataPoints_[index]->lppb = value; break;
        case rppb : dataPoints_[index]->rppb = value; break;
        case lppe : dataPoints_[index]->lppe = value; break;
        case rppe : dataPoints_[index]->rppe = value; break;
        case lpppb : dataPoints_[index]->lpppb = value; break;
        case rpppb : dataPoints_[index]->rpppb = value; break;
        case lpppe : dataPoints_[index]->lpppe = value; break;
        case rpppe : dataPoints_[index]->rpppe = value; break;
        case smo2 : dataPoints_[index]->smo2 = value; break;
        case thb : dataPoints_[index]->thb = value; break;
        case o2hb : dataPoints_[index]->o2hb = value; break;
        case hhb : dataPoints_[index]->hhb = value; break;
        case rcad : dataPoints_[index]->rcad = value; break;
        case rvert : dataPoints_[index]->rvert = value; break;
        case rcontact : dataPoints_[index]->rcontact = value; break;
        case interval : dataPoints_[index]->interval = value; break;
        case tcore : dataPoints_[index]->tcore = value; break;
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
        case RideFile::lpco : return lpco; break;
        case RideFile::rpco : return rpco; break;
        case RideFile::lppb : return lppb; break;
        case RideFile::rppb : return rppb; break;
        case RideFile::lppe : return lppe; break;
        case RideFile::rppe : return rppe; break;
        case RideFile::lpppb : return lpppb; break;
        case RideFile::rpppb : return rpppb; break;
        case RideFile::lpppe : return lpppe; break;
        case RideFile::rpppe : return rpppe; break;
        case RideFile::smo2 : return smo2; break;
        case RideFile::o2hb : return o2hb; break;
        case RideFile::hhb : return hhb; break;
        case RideFile::rcad : return rcad; break;
        case RideFile::rvert : return rvert; break;
        case RideFile::rcontact : return rcontact; break;
        case RideFile::gear : return gear; break;
        case RideFile::interval : return interval; break;
        case RideFile::IsoPower : return np; break;
        case RideFile::xPower : return xp; break;
        case RideFile::aPower : return apower; break;
        case RideFile::aTISS : return atiss; break;
        case RideFile::anTISS : return antiss; break;
        case RideFile::tcore : return tcore; break;

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
        case RideFile::lpco : lpco = value; break;
        case RideFile::rpco : rpco = value; break;
        case RideFile::lppb : lppb = value; break;
        case RideFile::rppb : rppb = value; break;
        case RideFile::lppe : lppe = value; break;
        case RideFile::rppe : rppe = value; break;
        case RideFile::lpppb : lpppb = value; break;
        case RideFile::rpppb : rpppb = value; break;
        case RideFile::lpppe : lpppe = value; break;
        case RideFile::rpppe : rpppe = value; break;
        case RideFile::smo2 : smo2 = value; break;
        case RideFile::o2hb : o2hb = value; break;
        case RideFile::hhb : hhb = value; break;
        case RideFile::rcad : rcad = value; break;
        case RideFile::rvert : rvert = value; break;
        case RideFile::rcontact : rcontact = value; break;
        case RideFile::gear : gear = value; break;
        case RideFile::interval : interval = value; break;
        case RideFile::IsoPower : np = value; break;
        case RideFile::xPower : xp = value; break;
        case RideFile::aPower : apower = value; break;
        case RideFile::aTISS : atiss = value; break;
        case RideFile::anTISS : antiss = value; break;
        case RideFile::tcore : tcore = value; break;

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
    if ((series==RideFile::temp || series==RideFile::lrbalance) && value == RideFile::NA)
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
        case IsoPower : return 0; break;
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
        case lpco :
        case rpco :
        case lppb :
        case rppb :
        case lppe :
        case rppe :
        case lpppb :
        case rpppb :
        case lpppe :
        case rpppe : return 0; break;
        case smo2 : return 0; break;
        case thb : return 2; break;
        case o2hb : return 2; break;
        case hhb : return 2; break;
        case rcad : return 0; break;
        case rvert : return 1; break;
        case rcontact : return 1; break;
        case gear : return 2; break;
        case wprime : return 0; break;
        case wbal : return 0; break;
        case tcore : return 2; break;
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
        case IsoPower : return 2500; break;
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
        case lpco :
        case rpco : return 100; break;
        case lppb :
        case rppb :
        case lppe :
        case rppe :
        case lpppb :
        case rpppb :
        case lpppe :
        case rpppe : return 360; break;
        case smo2 : return 100; break;
        case thb : return 20; break;
        case o2hb : return 20; break;
        case hhb : return 20; break;
        case rcad : return 500; break;
        case rvert : return 50; break;
        case rcontact : return 1000; break;
        case gear : return 7; break; // 53x8
        case wprime : return 99999; break;
        case wbal : return 100; break; // wbal is from 0% used to 100% used 
        case tcore : return 40; break; // anything about 40C is mad
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
        case IsoPower : return 0; break;
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
        case lpco :
        case rpco : return -100; break;
        case lppb :
        case rppb :
        case lppe :
        case rppe :
        case lpppb :
        case rpppb :
        case lpppe :
        case rpppe : return 0; break;
        case smo2 : return 0; break;
        case thb : return 0; break;
        case o2hb : return 0; break;
        case hhb : return 0; break;
        case rcad : return 0; break;
        case rvert : return 0; break;
        case rcontact : return 0; break;
        case gear : return 0; break;
        case wprime : return 0; break;
        case wbal: return 0; break;
        case tcore: return 36; break; // min 36C in humans
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
RideFile::insertXDataPoint(QString _xdata, int index, XDataPoint *point)
{
    XDataSeries *series = xdata(_xdata);
    if (series)  series->datapoints.insert(index, point);
}

void
RideFile::deleteXDataPoints(QString _xdata, int index, int count)
{
    XDataSeries *series = xdata(_xdata);
    if (series) series->datapoints.remove(index, count);
}

void
RideFile::appendPoints(QVector <struct RideFilePoint *> newRows)
{
    dataPoints_ += newRows;
}

void
RideFile::appendXDataPoints(QString _xdata, QVector<XDataPoint *> points)
{
    XDataSeries *series = xdata(_xdata);
    if (series) series->datapoints << points;
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

void RideFile::appendReference(const RideFilePoint &point)
{
    referencePoints_.append(new RideFilePoint(point.secs,point.cad,point.hr,point.km,point.kph,point.nm,
                                              point.watts,point.alt,point.lon,point.lat,
                                              point.headwind, point.slope, point.temp, point.lrbalance, 
                                              point.lte, point.rte, point.lps, point.rps,
                                              point.lpco, point.rpco,
                                              point.lppb, point.rppb, point.lppe, point.rppe,
                                              point.lpppb, point.rpppb, point.lpppe, point.rpppe,
                                              point.smo2, point.thb,
                                              point.rvert, point.rcad, point.rcontact, point.tcore,
                                              point.interval));
}

void RideFile::removeReference(int index)
{
    referencePoints_.remove(index);
}

void RideFile::removeExhaustion(int index)
{
    if (index < 0) return;

    // wipe the index'th exhaustion point in the ride
    int i=-1;
    for(int k=0; k<referencePoints_.count(); k++) {
        if (referencePoints_[k]->secs > 0) {
            if (++i == index) {
                referencePoints_.remove(k);
                return;
            }
        }
    }
}

bool
RideFile::parseRideFileName(const QString &name, QDateTime *dt)
{
    static char rideFileRegExp[] = "^((\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)"
                                   "_(\\d\\d)_(\\d\\d)_(\\d\\d))\\.(.+)$";
    QRegExp rx(rideFileRegExp);

    // no dice
    if (!rx.exactMatch(name)) return false;

    QDate date(rx.cap(2).toInt(), rx.cap(3).toInt(),rx.cap(4).toInt());
    QTime time(rx.cap(5).toInt(), rx.cap(6).toInt(),rx.cap(7).toInt());

    // didn't parse a meaningful date
    if ((! date.isValid()) || (! time.isValid()))  return false;

    // return value
    *dt = QDateTime(date, time);
    return true;
}

QVector<RideFile::seriestype> 
RideFile::arePresent()
{
    QVector<RideFile::seriestype> returning;

    for (int i=0; i< static_cast<int>(lastSeriesType()) ; i++) {
        if (isDataPresent(static_cast<RideFile::seriestype>(i))) returning << static_cast<RideFile::seriestype>(i);
    }

    return returning;
}

//
// Calculate derived data series

// (1) aPower
//
// aPower is based upon the models and research presented in
// "Altitude training and Athletic Performance" by Randall L. Wilber
// and Peronnet et al. (1991): Peronnet, F., G. Thibault, and D.L. Cousineau 1991.
// "A theoretical analisys of the effect of altitude on running
// performance." Journal of Applied Physiology 70:399-404
//

//
// (2) Core Body Temperature
//
// Tcore, the core body temperature estimate is based upon the models
// and research presented in "Estimation of human core temperature from 
// sequential heart rate observations" Mark J Buller, William J Tharion,
// Samuel N Cheuvront, Scott J Montain, Robert W Kenefick, John 
// Castellani, William A Latzka, Warren S Roberts, Mark Richter,
// Odest Chadwicke Jenkins and Reed W Hoyt. (2013). Physiological 
// Measurement. IOP Publishing 34 (2013) 781–798.
//

// Other derived series are calculated from well-known algorithms;
//          * Gear ratio
//          * Hill Slope
//          * xPower (Skiba)
//          * Iso Power (Coggan)
//

void
RideFile::recalculateDerivedSeries(bool force)
{
    // derived data is calculated from the data that is present
    // we should set to 0 where we cannot derive since we may
    // be called after data is deleted or added
    if (!force && dstale == false) return; // we're already up to date

    //
    // IsoPower Initialisation -- working variables
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
    if (context->athlete->zones(sport())) {
        int zoneRange = context->athlete->zones(sport())->whichRange(startTime().date());
        CP = zoneRange >= 0 ? context->athlete->zones(sport())->getCP(zoneRange) : 0;
        //WPRIME = zoneRange >= 0 ? context->athlete->zones(sport())->getWprime(zoneRange) : 0;

        // did we override CP in metadata / metrics ?
        int oCP = getTag("CP","0").toInt();
        if (oCP) CP=oCP;
    }

    // wheelsize - use meta, then config then drop to 2100
    double wheelsize = getTag(tr("Wheelsize"), "0.0").toDouble();
    if (wheelsize == 0) wheelsize = appsettings->cvalue(context->athlete->cyclist, GC_WHEELSIZE, 2100).toInt();
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
        // IsoPower
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

        // now the min and max values for IsoPower
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

        // now the min and max values for IsoPower
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

        // now the min and max values for IsoPower
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
                double deltaDistance = p->km - lastP->km;
                double deltaAltitude = p->alt - lastP->alt;
                if (deltaDistance>0) {
                    p->slope = deltaAltitude / (deltaDistance * 10); // * 100 for gradient, / 1000 to convert to meters
                } else {
                    // Repeat previous slope if distance hasn't changed.
                    p->slope = lastP->slope;
                }
                if (p->slope > 40 || p->slope < -40) {
                    p->slope = lastP->slope;
                }
            }
        }

        // derive or calculate gear ratio either from XDATA (if "GEARS" XData data exists)
        // or from speed and cadence
        double front = RideFile::NA;
        double rear = RideFile::NA;

        XDataSeries *series = xdata("GEARS");
        if (series && series->datapoints.count() > 0)  {
            int idx=0;
            front = xdataValue(p, idx, "GEARS", "FRONT", RideFile::REPEAT);
            rear = xdataValue(p, idx, "GEARS", "REAR", RideFile::REPEAT);
        }

        if (front != RideFile::NA && rear != RideFile::NA) {
            // gear data were part of XDATA series, use it
            setDataPresent(RideFile::gear, true);
            p->gear = front / rear;
        } else {
            // gear data were not present in XDATA series, we have to derive from other data:
            // can we derive gear ratio ? needs speed and cadence
            if (p->kph && p->cad && !isRun() && !isSwim()) {
                // need to say we got it
                setDataPresent(RideFile::gear, true);
                // calculate gear ratio, with simple 3 level rounding (considering that the ratio steps are not linear):
                // -> below ratio 1, round to next 0,05 border (ratio step of 2 tooth change is around 0,03 for 20/36 (MTB)
                // -> above ratio 1 and 3,  round to next 0,1 border (MTB + Racebike - bigger differences per shifting step)
                // -> above ration 3, round to next 0,5 border (mainly Racebike - even wider differences)
                // speed and wheelsize in meters
                // but only if ride point has power, cadence and speed > 0 otherwise calculation will give a random result
                if ((p->watts > 0.0f || !dataPresent.watts) && p->cad > 0.0f && p->kph > 0.0f) {
                    p->gear = (1000.00f * p->kph) / (p->cad * 60.00f * wheelsize);
                    // Round Gear ratio to the hundreths.
                    // final rounding to 2 decimals
                    p->gear = floor(p->gear * 100.00f +.5) / 100.00f;
                }
                else {
                    p->gear = 0.0f; // to be filled up with previous gear later
                }

                // truncate big values
                if (p->gear > maximumFor(RideFile::gear)) p->gear = 0;

            } else {
                p->gear = 0.0f;
            }
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

        // can we derive cycle length ?
        // needs speed and cadence
        if (p->kph && (p->cad || p->rcad)) {
            // need to say we got it
            setDataPresent(RideFile::clength, true);

            //  only if ride point has cadence and speed > 0
            if ((p->cad > 0.0f  || p->rcad > 0.0f ) && p->kph > 0.0f) {
                double cad = p->rcad;
                if (cad == 0)
                    cad = p->cad;

                p->clength = (1000.00f * p->kph) / (cad * 60.00f);

                // rounding to 2 decimals
                p->clength = round(p->clength * 100.00f) / 100.00f;
            }
            else {
                p->clength = 0.0f; // to be filled up with previous gear later
            }

        } else {
            p->clength = 0.0f;
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
            double diff1 = std::abs(last-next);
            double diff2 = std::abs(last-current);
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
            // remove rounding effect 0.01% is flat ;)
            if (dataPoints_[i]->slope < 0.01f && dataPoints_[i]->slope > -0.01f) {
                dataPoints_[i]->slope = 0;
            }
            rtot -= here;
            rtot += dataPoints_[i-smoothPoints]->slope;
        }
        setDataPresent(RideFile::slope, true);
    }

    //
    // Core Temperature
    //

    // PLEASE NOTE:
    // The core body temperature models was developed by the U.S Army
    // Research Institute of Environmental Medicine and is patent pending.
    // We have sought and been granted permission to utilise this within GoldenCheetah

    // since we are dealing in minutes we need to resample down to minutes
    // then upsample back to recIntSecs in-situ i.e we will use the p->Tcore
    // value to hold the rolling 60second values (where they are non-zero)
    // run through and calculate each value and then backfill for the seconds
    // in between

    // we need HR data for this
    if (dataPresent.hr) {

        // resample the data into 60s samples
        static const int SAMPLERATE=60000; // milliseconds in a minute
        QVector<double> hrArray;
        int lastT=0;
        RideFilePoint sample;

        foreach(RideFilePoint *p, dataPoints_) {

            // whats the dt in microseconds
            int dt = (p->secs * 1000) - (lastT * 1000);
            lastT = p->secs;

            //
            // AGGREGATE INTO SAMPLES
            //
            while (dt) {

                // we keep track of how much time has been aggregated
                // into sample, so 'need' is whats left to aggregate 
                // for the full sample
                int need = SAMPLERATE - sample.secs;

                // aggregate
                if (dt < need) {

                    // the entire sample read is less than we need
                    // so aggregate the whole lot and wait fore more
                    // data to be read. If there is no more data then
                    // this will be lost, we don't keep incomplete samples
                    sample.secs += dt;
                    sample.hr += float(dt) * p->hr;
                    dt = 0;

                } else {

                    // dt is more than we need to fill and entire sample
                    // so lets just take the fraction we need
                    dt -= need;
                    sample.hr += float(need) * p->hr;

                    // add the accumulated value
                    hrArray.append(sample.hr / double(SAMPLERATE));

                    // reset back to zero so we can aggregate
                    // the next sample
                    sample.secs = 0;
                    sample.hr = 0;
                }
            }
        }

        // This code is based upon the matlab function provided as
        // part of the 2013 paper cited above, bear in mind that the
        // input is HR in minute by minute samples NOT seconds.
        //
        // Props to Andy Froncioni for helping to evaluate this code.
        //
        // function CT = KFModel(HR,CTstart)
        // %Inputs:
        //   %HR = A vector of minute to minute HR values.
        //   %CTstart = Core Body Temperature at time 0.
        //
        // %Outputs:
        //   %CT = A vector of minute to minute CT estimates
        // 
        // %Extended Kalman Filter Parameters
        //   a = 1; gamma = 0.022^2;
        //   b0 = -7887.1; b1 = 384.4286; b2 = -4.5714; sigma = 18.88^2;
        static const double CTStart = 37.0f;
        static const double a1 = 1.0f;
        static const double gamma = 0.022f * 0.022f;
        static const double b0 = -7887.1f; 
        static const double b1 = 384.4286f; 
        static const double b2 = -4.5714f; 
        static const double sigma = 18.88f * 18.88f;
        //
        // %Initialize Kalman filter
        //   x = CTstart; v = 0;            %v = 0 assumes confidence with start value.
        double x = CTStart;
        double v = 0;
        //
        // %Iterate through HR time sequence
        //   for time = 1:length(HR)
        //     %Time Update Phase
        //     x_pred = a ∗ x;                                         %Equation 3
        //     v_pred = (a^2) ∗ v+gamma;                               %Equation 4
        //
        //     %Observation Update Phase
        //     z = HR(time);
        //     c_vc = 2 ∗  b2 ∗ x_pred+b1;                             %Equation 5
        //     k = (v_pred ∗ c_vc)./((c_vc^2) ∗ v_pred+sigma);         %Equation 6
        //     x = x_pred+k ∗ (z-(b2 ∗ (x_pred^2)+b1 ∗ x_pred+b0));    %Equation 7
        //     v = (1-k ∗ c_vc) ∗ v_pred;                              %Equation 8
        //     CT(time) = x;
        // end

        // now compute CT using the algorithm provided
        QVector<double> ctArray(hrArray.size());

        for(int i=0; i<hrArray.count(); i++) {
            double x_pred = a1 * x;
            double v_pred = (a1  * a1 ) * v + gamma;

            double z = hrArray[i];
            double c_vc = 2.0f *  b2 * x_pred + b1;
            double k = (v_pred * c_vc)/((c_vc*c_vc) * v_pred+sigma);

            x = x_pred+k * (z-(b2 * (x_pred*x_pred)+b1 * x_pred+b0));
            v = (1-k * c_vc) * v_pred;

            ctArray[i] = x;
        }

        // now update the RideFile data points, but only if we got
        // any data i.e. ride is longer than a minute long!
        if (ctArray.count()) {
            int index=0;
            foreach(RideFilePoint *p, dataPoints_) {

                // move on to the next one
                if (double(index)*60.0f < p->secs && index < (ctArray.count()-1)) index++;

                // just use the current value first for index=0 and p->secs=0
                p->tcore = ctArray[index];

                // smooth the values
                //if (index && p->secs > 0 && p->secs <= (double(index)*60.0f)) {
                    //double pt = (p->secs - (double(index-1)*60.00f)) / 60.0f;
                    //p->tcore = (ctArray[index] - ctArray[index-1]) * pt;
                //}
            }
        } else {

            // just set to the starting body temperature for every point
            foreach(RideFilePoint *p, dataPoints_) p->tcore = CTStart;
        }
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

#ifdef GC_HAVE_SAMPLERATE
//
// if we have libsamplerate available we use their simple api
// but always fill the gaps in recording according to the 
// interpolate setting.
//
RideFile *
RideFile::resample(double newRecIntSecs, int interpolate)
{
    Q_UNUSED(interpolate);

    // structures used by libsamplerate
    SRC_DATA data;
    float *input, *output, *source, *target;

    // too few data points !
    if (dataPoints().count() < 3) return NULL;

    // allocate space for arrays
    QVector<RideFile::seriestype> series = arePresent();

    // how many samples /should/ there be ?
    float duration = dataPoints().last()->secs - dataPoints().first()->secs;

    // backwards !?
    if (duration <= 0) return NULL;

    float insamples = 1 + (duration / recIntSecs());
    float outsamples = 1 + (duration / newRecIntSecs);

    // allocate memory
    source = input = (float*)malloc(sizeof(float) * series.count() * insamples);
    target = output = (float*)malloc(sizeof(float) * series.count() * outsamples); 

    // create the input array
    bool first = true;
    RideFilePoint *lp=NULL;

    int inframes = 0;
    foreach(RideFilePoint *p, dataPoints()) {

        // hit limits ?
        if (inframes >= insamples) break;

        // yuck! nasty data -- ignore it
        if (p->secs > (25*60*60)) continue;

        // always start at 0 seconds
        if (first) {
            first = false;
        }

        // fill gaps in recording with zeroes
        if (lp) {

            // fill with zeroes
            for(double t=lp->secs+recIntSecs();
                    (t + recIntSecs()) < p->secs;
                    t += recIntSecs()) {

                // add zero point
                inframes++;
                for(int i=0; i<series.count(); i++) *source++ = 0.0f;
            }
        }

        // lets not go backwards -- or two samples at the same time
        if ((lp && p->secs > lp->secs) || !lp) {
            // add this point
            inframes++;
            foreach(RideFile::seriestype x, series) *source++ = float(p->value(x));
        }

        // moving on to next sample
        lp = p;

    }

    //
    // THE MAGIC HAPPENS HERE ... resample to new recording interval
    //
    data.src_ratio = float(recIntSecs()) / float(newRecIntSecs); 
    data.data_in = input;
    data.input_frames = inframes;
    data.data_out = output;
    data.output_frames = outsamples;
    data.input_frames_used = 0;
    data.output_frames_gen = 0;
    int count = src_simple(&data, SRC_LINEAR, series.count());

    if (count) {
        // didn't work !
        //qDebug()<<"resampling error:"<<src_strerror(count);
        free(input);
        free(output);
        return NULL;
    } else {

        // build new ridefile struct
        //qDebug()<<"went from"<<inframes<<"using"<<data.input_frames_used<<"to"<<data.output_frames_gen<<"during resampling.";

        RideFile *returning = new RideFile(this);
        returning->recIntSecs_ = newRecIntSecs;

        float time = 0;

        // unpack the data series
        for(int frame=0; frame < data.output_frames_gen; frame++) {

            RideFilePoint add;
            add.secs = time;

            // now set each of the series
            foreach(RideFile::seriestype x, series) {
                if (x != RideFile::secs)
                    add.setValue(x, *target++);
                else
                    target++;
            }

            // append
            returning->appendPoint(add);

            time += newRecIntSecs;
        }
        returning->setDataPresent(secs, true);

        // free memory
        free(input);
        free(output);

        return returning;
    }
}

#else

//
// If we do not have libsamplerate available we use a more primitive
// approach by creating a spline and downsampling from it
// This is sufficient for most users who are typically converting from
// 1.26s sampling or higher to 1s sampling when merging data.
//
RideFile *
RideFile::resample(double newRecIntSecs, int /*interpolate*/)
{
    // resample if interval has changed
    if (newRecIntSecs != recIntSecs()) {
        QwtSplineBasis spline;
        QMap<SeriesType, SplineLookup*> splineLookups;

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

                    // lets not go backwards -- or two samples at the same time
                    if ((lp && p->secs > lp->secs) || !lp) {
                        points << QPointF(p->secs - offset, p->value(series));
                        last = p->secs-offset;
                    }

                    // moving on to next sample
                    lp = p;
                }

                // Now create a spline with the values we've cleaned
                SplineLookup *splineLookup = new SplineLookup();
                splineLookup->update(spline, QPolygonF(points), 1);
                splineLookups.insert(series, splineLookup);
            }
        }

        // no data to resample
        if (splineLookups.count() == 0 || last == 0) return NULL;

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
            QMapIterator<SeriesType, SplineLookup*> iterator(splineLookups);
            while (iterator.hasNext()) {
                iterator.next();

                SeriesType series = iterator.key();
                SplineLookup *splineLookup = iterator.value();

                double sum = 0;
                for (double i=0; i<1; i+= 0.25) {
                    double dt = seconds + (newRecIntSecs * i);
                    double dtn = seconds + (newRecIntSecs * (i+0.25f));
                    sum += (splineLookup->valueY(dt) + splineLookup->valueY(dtn)) / 2.0f;
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
        QMapIterator<SeriesType, SplineLookup*> iterator(splineLookups);
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
#endif

double 
RideFile::getWeight()
{
    return context->athlete->getWeight(startTime_.date(), this);
}

double 
RideFile::getHeight()
{
    return context->athlete->getHeight(this);
}

//
// Intervals...
//
bool 
RideFileInterval::isPeak() const 
{ 
    QString peak = QString("^(%1 *[0-9]*(s|min)|Entire workout|%2 #[0-9]*) *\\([^)]*\\)$").arg(tr("Peak")).arg(tr("Find"));
    return QRegExp(peak).exactMatch(name);
}

bool 
RideFileInterval::isMatch() const 
{ 
    QString match = QString("^(%1 ).*").arg(tr("Match"));
    return QRegExp(match).exactMatch(name); 
}
bool 
RideFileInterval::isClimb() const 
{ 
    QString climb = QString("^(%1 ).*").arg(tr("Climb"));
    return QRegExp(climb).exactMatch(name); 
}
bool
RideFileInterval::isBest() const
{
    QString best = QString("^(%1 ).*").arg(tr("Best"));
    return QRegExp(best).exactMatch(name); 
}


// ride data is referenced with symbols in upper case to make 
// it clear that this is raw data
static struct {
    QString symbol;
    RideFile::SeriesType series;
} seriesSymbolTable[] = {
    { "INDEX", RideFile::index },
	{ "SECS", RideFile::secs },
	{ "CADENCE", RideFile::cad },
	{ "CADENCED", RideFile::cadd },
	{ "HEARTRATE", RideFile::hr },
	{ "HEARTRATED", RideFile::hrd },
	{ "DISTANCE", RideFile::km },
	{ "SPEED", RideFile::kph },
	{ "SPEEDD", RideFile::kphd },
	{ "TORQUE", RideFile::nm },
	{ "TORQUED", RideFile::nmd },
	{ "POWER", RideFile::watts },
	{ "POWERD", RideFile::wattsd },
	{ "ALTITUDE", RideFile::alt },
	{ "LON", RideFile::lon },
	{ "LAT", RideFile::lat },
    { "IsoPower", RideFile::IsoPower },
	{ "HEADWIND", RideFile::headwind },
	{ "SLOPE", RideFile::slope },
	{ "TEMPERATURE", RideFile::temp },
	{ "BALANCE", RideFile::lrbalance },
	{ "LEFTEFFECTIVENESS", RideFile::lte },
	{ "RIGHTEFFECTIVENESS", RideFile::rte },
	{ "LEFTSMOOTHNESS", RideFile::lps },
	{ "RIGHTSMOOTHNESS", RideFile::rps },
    //{ "COMBINEDSMOOTHNESS", RideFile::cps },
	{ "SMO2", RideFile::smo2 },
	{ "THB", RideFile::thb },
	{ "RUNVERT", RideFile::rvert },
	{ "RUNCADENCE", RideFile::rcad },
	{ "RUNCONTACT", RideFile::rcontact },
	{ "LEFTPCO", RideFile::lpco },
	{ "RIGHTPCO", RideFile::rpco },
	{ "LEFTPPB", RideFile::lppb },
	{ "RIGHTPPB", RideFile::rppb },
	{ "LEFTPPE", RideFile::lppe },
	{ "RIGHTPPE", RideFile::rppe },
	{ "LEFTPPPB", RideFile::lpppb },
	{ "RIGHTPPPB", RideFile::rpppb },
	{ "LEFTPPPE", RideFile::lpppe },
    { "RIGHTPPPE", RideFile::rpppe },
    { "WBAL", RideFile::wbal },
    { "TCORE", RideFile::tcore },
	{ "", RideFile::none  },
};

QStringList 
RideFile::symbols()
{
    // list of valid symbols
    QStringList returning;
    for(int i=0; seriesSymbolTable[i].series != none; i++)
        returning << seriesSymbolTable[i].symbol;

    return returning;
}

RideFile::SeriesType RideFile::seriesForSymbol(QString symbol)
{
    // get type for symbol
    for(int i=0; seriesSymbolTable[i].series != none; i++)
        if (seriesSymbolTable[i].symbol == symbol)
            return seriesSymbolTable[i].series;

    return none;
}

QString 
RideFile::symbolForSeries(SeriesType series)
{
    // get type for symbol
    for(int i=0; seriesSymbolTable[i].series != none; i++)
        if (seriesSymbolTable[i].series == series)
            return seriesSymbolTable[i].symbol;

    return "";
}

// Iterator
RideFileIterator::RideFileIterator(RideFile *f, Specification spec, IterationSpec mode)
    : f(f)
{
    // index, start and stop are set to -1
    // if they are out of bounds or f is NULL
    if (f != NULL) {

        // ok, so lets work out the begin and end index
        double startsecs = spec.secsStart();
        if (startsecs < 0) start = 0;
        else start = f->timeIndex(startsecs);

        // check!
        if (start >= f->dataPoints().count()) start = -1;

        // ok, so lets work out the begin and end index
        double stopsecs = spec.secsEnd();
        if (stopsecs < 0) stop = f->dataPoints().count()-1;
        else stop = f->timeIndex(stopsecs); // dgr was f->timeIndex(stopsecs)-1

        // check!
        if (stop >= f->dataPoints().count()) stop = -1;

        // ok, so now adjust for BEFORE, AFTER
        if (mode == Before) {
            if (start <= 0) {
                start = stop -1; // interval start of ride
            } else {
                stop = start;
                start = 0;
            }
        }

        if (mode == After) {
            if (stop == f->dataPoints().count()-1) {
                start = stop = -1;
            } else {
                start = stop;
                stop = f->dataPoints().count()-1;
            }
        }

    } else {

        // nothing doing
        start = stop = -1;
    }

    // move to front by default
    index = start;
}

void
RideFileIterator::toFront()
{
    index = start;
}

void
RideFileIterator::toBack()
{
    index = stop;
}

struct RideFilePoint *
RideFileIterator::first()
{
    return start >= 0 ? f->dataPoints()[start] : NULL; // efficient since dataPoints() returns a reference
}

struct RideFilePoint *
RideFileIterator::last()
{
    return stop >= 0 ? f->dataPoints()[stop] : NULL; // efficient since dataPoints() returns a reference
}

bool
RideFileIterator::hasNext() const
{
    return (index >= 0 && index <= stop);
}

bool
RideFileIterator::hasPrevious() const
{
    return (index >= 0 && index >= start);
}

struct RideFilePoint *
RideFileIterator::next()
{
    if (index >= 0 && index <= stop) return f->dataPoints()[index++];
    else return NULL;
}

struct RideFilePoint *
RideFileIterator::previous()
{
    if (index >= 0 && index >= start) return f->dataPoints()[index--];
    else return NULL;
}

struct CompareXDataPointSecs {
    bool operator()(const XDataPoint *p1, const XDataPoint *p2) {
        return p1->secs < p2->secs;
    }
};

int
XDataSeries::timeIndex(double secs) const
{
    // return index offset for specified time
    XDataPoint p;
    p.secs = secs;

    QVector<XDataPoint*>::const_iterator i = std::lower_bound(
        datapoints.begin(), datapoints.end(), &p, CompareXDataPointSecs());
    if (i == datapoints.end())
        return datapoints.size()-1;
    return i - datapoints.begin();
}
