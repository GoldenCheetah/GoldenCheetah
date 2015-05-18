/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#include "RideItem.h"
#include "RideMetric.h"
#include "RideFile.h"
#include "RideFileCache.h"
#include "RideMetadata.h"
#include "IntervalItem.h"
#include "Route.h"
#include "Context.h"
#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"
#include "Settings.h"
#include "Colors.h" // for ColorEngine
#include "BestIntervalDialog.h" // till we fixup ridefilecache to have offsets
#include "TimeUtils.h" // time_to_string()

#include <cmath>
#include <QtAlgorithms>
#include <QMap>
#include <QMapIterator>
#include <QByteArray>

// used to create a temporary ride item that is not in the cache and just
// used to enable using the same calling semantics in things like the
// merge wizard and interval navigator
RideItem::RideItem() 
    : 
    ride_(NULL), fileCache_(NULL), context(NULL), isdirty(false), isstale(true), isedit(false), skipsave(false), path(""), fileName(""),
    color(QColor(1,1,1)), isRun(false), isSwim(false), samples(false), fingerprint(0), metacrc(0), crc(0), timestamp(0), dbversion(0), weight(0) {
    metrics_.fill(0, RideMetricFactory::instance().metricCount());
}

RideItem::RideItem(RideFile *ride, Context *context) 
    : 
    ride_(ride), fileCache_(NULL), context(context), isdirty(false), isstale(true), isedit(false), skipsave(false), path(""), fileName(""),
    color(QColor(1,1,1)), isRun(false), isSwim(false), samples(false), fingerprint(0), metacrc(0), crc(0), timestamp(0), dbversion(0), weight(0) 
{
    metrics_.fill(0, RideMetricFactory::instance().metricCount());
}

RideItem::RideItem(QString path, QString fileName, QDateTime &dateTime, Context *context) 
    :
    ride_(NULL), fileCache_(NULL), context(context), isdirty(false), isstale(true), isedit(false), skipsave(false), path(path), 
    fileName(fileName), dateTime(dateTime), color(QColor(1,1,1)), isRun(false), isSwim(false), samples(false), fingerprint(0), 
    metacrc(0), crc(0), timestamp(0), dbversion(0), weight(0) 
{
    metrics_.fill(0, RideMetricFactory::instance().metricCount());
}

// Create a new RideItem destined for the ride cache and used for caching
// pre-computed metrics and storing ride metadata
RideItem::RideItem(RideFile *ride, QDateTime &dateTime, Context *context)
    :
    ride_(ride), fileCache_(NULL), context(context), isdirty(true), isstale(true), isedit(false), skipsave(false), dateTime(dateTime),
    fingerprint(0), metacrc(0), crc(0), timestamp(0), dbversion(0), weight(0)
{
    metrics_.fill(0, RideMetricFactory::instance().metricCount());
}

// clone a ride item
void
RideItem::setFrom(RideItem&here) // used when loading cache/rideDB.json
{
    ride_ = NULL;
    fileCache_ = NULL;
    metrics_ = here.metrics_;
	metadata_ = here.metadata_;
	errors_ = here.errors_;
    intervals_ = here.intervals_;
    foreach(IntervalItem *p, intervals_) p->rideItem_ = this;
	context = here.context;
	isdirty = here.isdirty;
    isstale = here.isstale;
	isedit = here.isedit;
	skipsave = here.skipsave;
	path = here.path;
	fileName = here.fileName;
	dateTime = here.dateTime;
	fingerprint = here.fingerprint;
	metacrc = here.metacrc;
    crc = here.crc;
	timestamp = here.timestamp;
	dbversion = here.dbversion;
	color = here.color;
	present = here.present;
    isRun = here.isRun;
    isSwim = here.isSwim;
	weight = here.weight;
    samples = here.samples;
}

// set the metric array
void
RideItem::setFrom(QHash<QString, RideMetricPtr> computed)
{
    QHashIterator<QString, RideMetricPtr> i(computed);
    while (i.hasNext()) {
        i.next();
        metrics_[i.value()->index()] = i.value()->value();
    }
}

// calculate metadata crc
unsigned long 
RideItem::metaCRC()
{
    QMapIterator<QString,QString> i(metadata_);
    QByteArray ba;
    i.toFront();
    while(i.hasNext()) {
        i.next();
        ba.append(i.key());
        ba.append(i.value());
    }
    return qChecksum(ba, ba.length());
}

RideFile *RideItem::ride(bool open)
{
    if (!open || ride_) return ride_;

    // open the ride file
    QFile file(path + "/" + fileName);
    ride_ = RideFileFactory::instance().openRideFile(context, file, errors_);
    if (ride_ == NULL) return NULL; // failed to read ride

    // link any USER intervals to the ride, bit fiddly but only used
    // when updating the physical model via the logical
    if (intervals_.count()) {
        //qDebug()<<fileName<<"LINKING INTERVALS";
        int findex=0;
        for(int index=0; index<intervals_.count(); index++) {

            // only linking user intervals
            if (intervals_.at(index)->type != RideFileInterval::USER) continue;

            if (ride_->intervals().count()<=findex) {
                // none left to link to, so wipe!
                //qDebug()<<"user interval not found"<<intervals_.at(index)->name;

            } else {

                // look for us ...
                while (findex < ride_->intervals().count()) {
                    if (ride_->intervals().at(findex)->name == intervals_.at(index)->name) {
                        //qDebug()<<"linking"<<intervals_.at(index)->name;
                        intervals_.at(index)->rideInterval = ride_->intervals().at(findex);
                        findex++;
                        goto next;
                    } else {
                        //qDebug()<<"seeking"<<intervals_.at(index)->name;
                        ;
                    }
                    findex++;
                }
            }
            next:;
        }
    }

    // refresh if stale..
    refresh();

    setDirty(false); // we're gonna use on-disk so by
                     // definition it is clean - but do it *after*
                     // we read the file since it will almost
                     // certainly be referenced by consuming widgets

    // stay aware of state changes to our ride
    // Context saves and RideFileCommand modifies
    connect(ride_, SIGNAL(modified()), this, SLOT(modified()));
    connect(ride_, SIGNAL(saved()), this, SLOT(saved()));
    connect(ride_, SIGNAL(reverted()), this, SLOT(reverted()));

    return ride_;
}

RideItem::~RideItem()
{
    //qDebug()<<"deleting:"<<fileName;
    if (isOpen()) close();
    if (fileCache_) delete fileCache_;
    //XXX need to consider what to do here for the intervalitem
    //XXX used by the RideDB parser - we don't want to wipe away
    //XXX the intervals we just passed into setFrom()
    //XXX foreach(IntervalItem*x, intervals_) delete x;
}

RideFileCache *
RideItem::fileCache()
{
    if (!fileCache_) {
        fileCache_ = new RideFileCache(context, fileName, getWeight(), ride());
        if (isDirty()) fileCache_->refresh(ride_); // refresh from what we have now !
    }
    return fileCache_;
}

void
RideItem::setRide(RideFile *overwrite)
{
    RideFile *old = ride_;
    ride_ = overwrite; // overwrite

    // connect up to new one
    connect(ride_, SIGNAL(modified()), this, SLOT(modified()));
    connect(ride_, SIGNAL(saved()), this, SLOT(saved()));
    connect(ride_, SIGNAL(reverted()), this, SLOT(reverted()));

    // don't bother with the old one any more
    disconnect(old);

    // update status
    setDirty(true);
    notifyRideDataChanged();

    //XXX SORRY ! memory leak XXX
    //XXX delete old; // now wipe it once referrers had chance to change
    //XXX this is only used by MergeActivityWizard and causes issues
    //XXX because the data is accessed in separate threads (Wizard is a dialog)
    //XXX because it is such an edge case (Merge) we will leave it for now
}

bool
RideItem::removeInterval(IntervalItem *x)
{
    int index = intervals_.indexOf(x);

    if (ride_ == NULL) return false; // file not open
    if (index < 0 || index > intervals_.count()) return false; // out of bounds
    if (x->type != RideFileInterval::USER) return false; // wrong type
    if (x->rideInterval == NULL) return false; // no link to ridefileinterval
    if (ride_->removeInterval(x->rideInterval) == false) return false; // failed to remove from ridefile
    intervals_.removeAt(index);

    setDirty(true);
    return true;
}

void
RideItem::addInterval(IntervalItem item)
{
    IntervalItem *add = new IntervalItem();
    add->setFrom(item);
    add->rideItem_ = this;
    intervals_ << add;
}

void
RideItem::notifyRideDataChanged()
{
    // refresh the metrics
    isstale=true;

    // force a recompute of derived data series
    if (ride_) {
        ride_->wstale = true;
        ride_->recalculateDerivedSeries(true);
    }

    // refresh the cache
    if (fileCache_) fileCache_->refresh(ride_);

    // refresh the data
    refresh();

    emit rideDataChanged();
}

void
RideItem::notifyRideMetadataChanged()
{
    // refresh the metrics
    isstale=true;
    refresh();

    emit rideMetadataChanged();
}

void
RideItem::modified()
{
    setDirty(true);
}

void
RideItem::saved()
{
    setDirty(false);
    isstale=true;
    refresh(); // update !
    context->notifyRideSaved(this);
}

void
RideItem::reverted()
{
    setDirty(false);
    isstale=true;
    refresh();
}

void
RideItem::setDirty(bool val)
{
    if (isdirty == val) return; // np change

    isdirty = val;

    if (isdirty == true) {

        context->notifyRideDirty();

    } else {

        context->notifyRideClean();
    }
}

// name gets changed when file is converted in save
void
RideItem::setFileName(QString path, QString fileName)
{
    this->path = path;
    this->fileName = fileName;
}

bool
RideItem::isOpen()
{
    return ride_ != NULL;
}

void
RideItem::close()
{
    if (ride_) {
        // break link to ride file
        foreach(IntervalItem *x, intervals()) x->rideInterval = NULL;
        delete ride_;
        ride_ = NULL;
    }
}

void
RideItem::setStartTime(QDateTime newDateTime)
{
    dateTime = newDateTime;
    ride()->setStartTime(newDateTime);
}

// check if we need to be refreshed
bool
RideItem::checkStale()
{
    // if we're marked stale already then just return that !
    if (isstale) return true;

    // just change it .. its as quick to change as it is to check !
    color = context->athlete->colorEngine->colorFor(getText(context->athlete->rideMetadata()->getColorField(), ""));

    // upgraded metrics
    if (dbversion != DBSchemaVersion) {

        isstale = true;

    } else {

        // has weight changed?
        unsigned long prior  = 1000.0f * weight;
        unsigned long now = 1000.0f * getWeight();

        if (prior != now) {

            getWeight();
            isstale = true;

        } else {

            // or have cp / zones have changed ?
            // note we now get the fingerprint from the zone range
            // and not the entire config so that if you add a new
            // range (e.g. set CP from today) but none of the other
            // ranges change then there is no need to recompute the
            // metrics for older rides !

            // get the new zone configuration fingerprint that applies for the ride date
            unsigned long rfingerprint = static_cast<unsigned long>(context->athlete->zones()->getFingerprint(dateTime.date()))
                        + static_cast<unsigned long>(context->athlete->paceZones()->getFingerprint(dateTime.date()))
                        + static_cast<unsigned long>(context->athlete->hrZones()->getFingerprint(dateTime.date()));

            if (fingerprint != rfingerprint) {

                isstale = true;

            } else {

                // or has file content changed ?
                QString fullPath =  QString(context->athlete->home->activities().absolutePath()) + "/" + fileName;
                QFile file(fullPath);

                // has timestamp changed ?
                if (timestamp < QFileInfo(file).lastModified().toTime_t()) {

                    // if timestamp has changed then check crc
                    unsigned long fcrc = RideFile::computeFileCRC(fullPath);

                    if (crc == 0 || crc != fcrc) {
                        crc = fcrc; // update as expensive to calculate
                        isstale = true;
                    }
                }


                // no intervals ?
                if (samples && intervals_.count() == 0)
                    isstale = true;

            }
        }
    }

    // still reckon its clean? what about the cache ?
    if (isstale == false) isstale = RideFileCache::checkStale(context, this);

    // we need to mark stale in case "special" fields may have changed (e.g. CP)
    if (metacrc != metaCRC()) isstale = true;

    return isstale;
}

void
RideItem::refresh()
{
    if (!isstale) return;

    // update current state coz we'll fix it below
    isstale = false;

    // open ride file will extract details too, but only if not
    // already open since its a user entry point and will call
    // refresh when opened. We don't want a recursion here.
    // And if already open no need to close
    RideFile *f;
    bool doclose = false;
    if (!isOpen()) { 
        doclose = true;
        f = ride(); // will call us but isstale is false above
    } else f=ride_;

    if (f) {

        // get the metadata & metric overrides
        metadata_ = f->tags();

        // get weight that applies to the date
        getWeight();

        // first class stuff
        isRun = f->isRun();
        isSwim = f->isSwim();
        color = context->athlete->colorEngine->colorFor(f->getTag(context->athlete->rideMetadata()->getColorField(), ""));
        present = f->getTag("Data", "");
        samples = f->dataPoints().count() > 0;

        // refresh metrics etc
        const RideMetricFactory &factory = RideMetricFactory::instance();
        QHash<QString,RideMetricPtr> computed= RideMetric::computeMetrics(context, f, context->athlete->zones(), 
                                               context->athlete->hrZones(), factory.allMetrics());


        // ressize and initialize so we can store metric values at
        // RideMetric::index offsets into the metrics_ qvector
        metrics_.fill(0, factory.metricCount());

        // snaffle away all the computed values into the array
        QHashIterator<QString, RideMetricPtr> i(computed);
        while (i.hasNext()) {
            i.next();
            metrics_[i.value()->index()] = i.value()->value();
        }

        // clean any bad values
        for(int j=0; j<factory.metricCount(); j++)
            if (std::isinf(metrics_[j]) || std::isnan(metrics_[j]))
                metrics_[j] = 0.00f;

        // Update auto intervals AFTER ridefilecache as used for bests
        updateIntervals();

        // update fingerprints etc, crc done above
        fingerprint = static_cast<unsigned long>(context->athlete->zones()->getFingerprint(dateTime.date()))
                    + static_cast<unsigned long>(context->athlete->paceZones()->getFingerprint(dateTime.date()))
                    + static_cast<unsigned long>(context->athlete->hrZones()->getFingerprint(dateTime.date()));

        dbversion = DBSchemaVersion;
        timestamp = QDateTime::currentDateTime().toTime_t();

        // RideFile cache needs refreshing possibly
        RideFileCache updater(context, context->athlete->home->activities().canonicalPath() + "/" + fileName, getWeight(), ride_, true);

        // we now match
        metacrc = metaCRC();

        // close if we opened it
        if (doclose) {
            close();
        } else {

            // if it is open then recompute
            ride_->wstale = true;
            ride_->recalculateDerivedSeries(true);
        }

    } else {
        qDebug()<<"** FILE READ ERROR: "<<fileName;
        isstale = false;
        samples = false;
    }
}

double
RideItem::getWeight()
{
    // withings first
    weight = context->athlete->getWithingsWeight(dateTime.date());

    // from metadata
    if (!weight) weight = metadata_.value("Weight", "0.0").toDouble();

    // global options
    if (!weight) weight = appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT, "75.0").toString().toDouble(); // default to 75kg
    
    // No weight default is weird, we'll set to 80kg
    if (weight <= 0.00) weight = 80.00;

    return weight;
}

double
RideItem::getForSymbol(QString name, bool useMetricUnits)
{
    if (metrics_.size()) {
        // return the precomputed metric value
        const RideMetricFactory &factory = RideMetricFactory::instance();
        const RideMetric *m = factory.rideMetric(name);
        if (m) {
            if (useMetricUnits) return metrics_[m->index()];
            else {
                // little hack to set/get for conversion
                const_cast<RideMetric*>(m)->setValue(metrics_[m->index()]);
                return m->value(useMetricUnits);
            }
        }
    }
    return 0.0f;
}

QString
RideItem::getStringForSymbol(QString name, bool useMetricUnits)
{
    QString returning("-");

    if (metrics_.size()) {
        // return the precomputed metric value
        const RideMetricFactory &factory = RideMetricFactory::instance();
        const RideMetric *m = factory.rideMetric(name);
        if (m) {

            double value = metrics_[m->index()];
            if (std::isinf(value) || std::isnan(value)) value=0;
            const_cast<RideMetric*>(m)->setValue(value);
            returning = m->toString(useMetricUnits);
        }
    }
    return returning;
}

struct effort {
    int start, duration, joules;
    double quality;
};

void
RideItem::updateIntervals()
{
    // DO NOT USE ride() since it will call a refresh !
    RideFile *f = ride_;

    // clear what is there
    foreach(IntervalItem *x, intervals_) delete x;
    intervals_.clear();

    // no ride data available ?
    if (!samples) return;

    // Get CP and W' estimates for date of ride
    double CP = 250;
    double WPRIME = 22000;
    double PMAX = 1000;

    if (context->athlete->zones()) {

        // if range is -1 we need to fall back to a default value
        int zoneRange = context->athlete->zones()->whichRange(dateTime.date());
        CP = zoneRange >= 0 ? context->athlete->zones()->getCP(zoneRange) : 250;
        WPRIME = zoneRange >= 0 ? context->athlete->zones()->getWprime(zoneRange) : 22000;
        PMAX = zoneRange >= 0 ? context->athlete->zones()->getPmax(zoneRange) : 1000;

        // did we override CP in metadata ?
        int oCP = getText("CP","0").toInt();
        if (oCP) CP=oCP;
    }

    // USER / DEVICE INTERVALS
    // first we create interval items for all intervals
    // that are in the ridefile, but ignore Peaks since we
    // add those automatically for HR and Power where those
    // data series are present

    // ride start and end
    RideFilePoint *begin = f->dataPoints().first();
    RideFilePoint *end = f->dataPoints().last();

    // add entire ride using ride metrics
    IntervalItem *entire = new IntervalItem(f, tr("Entire Activity"), 
                                            begin->secs, end->secs, 
                                            f->timeToDistance(begin->secs),
                                            f->timeToDistance(end->secs),
                                            0,
                                            QColor(Qt::darkBlue),
                                            RideFileInterval::ALL);

    // same as the whole ride, not need to compute
    entire->metrics() = metrics();
    entire->rideItem_ = this;
    entire->rideInterval = NULL;
    intervals_ << entire;

    int count = 1;
    foreach(RideFileInterval *interval, f->intervals()) {

        // skip peaks, they're autodiscovered now
        if (interval->isPeak()) continue;

        // skip climbs, they're autodiscovered now
        if (interval->isClimb()) continue;

        // skip entire ride, they're autodiscovered too
        if (interval->start <= begin->secs && interval->stop >= end->secs) continue;

        // same as ride but offset by recintsecs
        if (((interval->start - f->recIntSecs()) <= begin->secs && (interval->stop-f->recIntSecs()) >= end->secs) ||
           (interval->start <= begin->secs && (interval->stop+f->recIntSecs()) >= end->secs))
             continue;

        // skip empty backward intervals
        if (interval->start >= interval->stop) continue;

        // create a new interval item
        IntervalItem *intervalItem = new IntervalItem(f, interval->name, 
                                                      interval->start, interval->stop, 
                                                      f->timeToDistance(interval->start),
                                                      f->timeToDistance(interval->stop),
                                                      count,
                                                      standardColor(count++),
                                                      RideFileInterval::USER);
        intervalItem->rideItem_ = this; // XXX will go when we refactor and be passed instead of ridefile
        intervalItem->rideInterval = interval;
        intervalItem->refresh();        // XXX will get called in constructore when refactor
        intervals_ << intervalItem;

        //qDebug()<<"interval:"<<interval.name<<interval.start<<interval.stop<<"f:"<<begin->secs<<end->secs;
    }

    // DISCOVERY

    //qDebug() << "SEARCH PEAK POWERS"
    if (!f->isRun() && !f->isSwim() && f->isDataPresent(RideFile::watts)) {

        // what we looking for ?
        static int durations[] = { 1, 5, 10, 15, 20, 30, 60, 300, 600, 1200, 1800, 2700, 3600, 0 };
        static const char *names[] = { "1 second", "5 seconds", "10 seconds", "15 seconds", "20 seconds", "30 seconds", 
                                "1 minute", "5 minutes", "10 minutes", "20 minutes", "30 minutes", "45 minutes",
                                "1 hour" };
    
        for(int i=0; durations[i] != 0; i++) {

            // go hunting for best peak
            QList<BestIntervalDialog::BestInterval> results;
            BestIntervalDialog::findBests(f, durations[i], 1, results);

            // did we get one ?
            if (results.count() > 0 && results[0].avg > 0 && results[0].stop > 0) {
                // qDebug()<<"found"<<names[i]<<"peak power"<<results[0].start<<"-"<<results[0].stop<<"of"<<results[0].avg<<"watts";
                IntervalItem *intervalItem = new IntervalItem(f, QString(tr("%1 (%2 watts)")).arg(names[i]).arg(int(results[0].avg)),
                                                            results[0].start, results[0].stop, 
                                                            f->timeToDistance(results[0].start),
                                                            f->timeToDistance(results[0].stop),
                                                            count++,
                                                            QColor(Qt::gray),
                                                            RideFileInterval::PEAKPOWER);
                intervalItem->rideItem_ = this; // XXX will go when we refactor and be passed instead of ridefile
                intervalItem->rideInterval = NULL;
                intervalItem->refresh();        // XXX will get called in constructore when refactor
                intervals_ << intervalItem;
            }
        }
    }

    //qDebug() << "SEARCH EFFORTS";
    QList<effort> candidates;
    QList<effort> candidates_sprint;
    
    if (!f->isRun() && !f->isSwim() && f->isDataPresent(RideFile::watts)) {

        const int SAMPLERATE = 1000; // 1000ms samplerate = 1 second samples

        RideFilePoint sample;        // we reuse this to aggregate all values
        long time = 0L;              // current time accumulates as we run through data
        double lastT = 0.0f;         // last sample time seen in seconds

        // set the array size
        int arraySize = f->dataPoints().last()->secs + f->recIntSecs();

        QTime timer;
        timer.start();

        // setup an integrated series
        long *integrated_series = (long*)malloc(sizeof(long) * arraySize);
        long *pi = integrated_series;
        *pi = 0L;

        int secs = 0;
        foreach(RideFilePoint *p, f->dataPoints()) {

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
                    sample.watts += float(dt) * p->watts;
                    dt = 0;

                } else {

                    // dt is more than we need to fill and entire sample
                    // so lets just take the fraction we need
                    dt -= need;

                    // accumulating time and distance
                    sample.secs = time; time += double(SAMPLERATE) / 1000.0f;

                    // averaging sample data
                    sample.watts += float(need) * p->watts;
                    sample.watts /= 1000;

                    // integrate
                    *pi += sample.watts;
                    *(pi+1) = *pi;

                    // move on
                    pi++;
                    secs++;

                    // reset back to zero so we can aggregate
                    // the next sample
                    sample.secs = 0;
                    sample.watts = 0;
                }
            }
        }

        // now the data is integrated we can look at the 
        // accumulated energy for each ride

        for (int i=0; i<secs; i++) {

            // start out at 30 minutes and drop back to
            // 2 minutes, anything shorter and we are done
            int t = (secs-i) > 3600 ? 3600 : secs-i;

            // if we find one lets record it
            bool found = false;
            bool foundSprint = false;
            effort tte;
            effort sprint;

            while (t > 120) {

                // calulate the TTE for the joules in the interval
                // starting at i seconds with duration t
                // This takes the monod equation p(t) = W'/t + CP and
                // solves for t, but the added complication of also
                // accounting for the fact it is expressed in joules
                // So take Joules = (W'/t + CP) * t and solving that
                // for t gives t = (Joules - W') / CP
                double tc = ((integrated_series[i+t]-integrated_series[i]) - WPRIME) / CP;

                // the TTE for this interval is greater or equal to
                // the duration of the interval !
                if (tc >= (t*0.85f)) {

                    if (found == false) {

                        // first one we found
                        found = true;

                        // register a candidate
                        tte.start = i;
                        tte.duration = t;
                        tte.joules = integrated_series[i+t]-integrated_series[i];
                        tte.quality = tc / double(t);

                    } else {

                        double thisquality = tc / double(t);

                        // found one with a higher quality
                        if (tte.quality < thisquality) {
                            tte.duration = t;
                            tte.joules = integrated_series[i+t]-integrated_series[i];
                            tte.quality = thisquality;
                        }

                    }

                    // look for smaller
                    t--;

                } else {
                    t = tc;
                    if (t<120)
                        t=120;
                }
            }

            // Search sprint


            while (t >= 5) {
                double tc = (integrated_series[i+t]-integrated_series[i]) / (PMAX*0.85f);

                if (tc >= t) {

                    if (found == false) {

                        // first one we found
                        foundSprint = true;

                        // register a candidate
                        sprint.start = i;
                        sprint.duration = t;
                        sprint.joules = integrated_series[i+t]-integrated_series[i];
                        sprint.quality = tc / double(t);

                    } else {

                        double thisquality = tc / double(t);

                        // found one with a higher quality
                        if (sprint.quality < thisquality) {
                            sprint.duration = t;
                            sprint.joules = integrated_series[i+t]-integrated_series[i];
                            sprint.quality = thisquality;
                        }

                    }
                    //qDebug() << "sprint" << i << sprint.duration << sprint.joules/sprint.duration << "J";
                }
                // look for smaller
                t--;
            }


            // add the best one we found here
            if (found) {

                // if we overlap with the last one and
                // we are better then replace otherwise skip
                if (candidates.count()) {

                    effort &last = candidates.last();
                    if ((tte.start >= last.start && tte.start <= (last.start+last.duration)) ||
                       (tte.start+tte.duration >= last.start && tte.start+tte.duration <= (last.start+last.duration))) {

                        // we overlap but we are higher quality
                        if (tte.quality > last.quality) last = tte;

                    } else{

                        // we don't overlap
                        candidates << tte;
                    }
                } else {

                    // we are the first
                    candidates << tte;
                }
            }

            // add the best one we found here
            if (foundSprint) {

                // if we overlap with the last one and
                // we are better then replace otherwise skip
                if (candidates_sprint.count()) {

                    effort &last = candidates_sprint.last();
                    if ((sprint.start >= last.start && sprint.start <= (last.start+last.duration)) ||
                       (sprint.start+sprint.duration >= last.start && sprint.start+sprint.duration <= (last.start+last.duration))) {

                        // we overlap but we are higher quality
                        if (sprint.quality > last.quality) last = sprint;

                    } else{

                        // we don't overlap
                        candidates_sprint << sprint;
                    }
                } else {

                    // we are the first
                    candidates_sprint << sprint;
                }
            }
        }

        // add any we found
        foreach(effort x, candidates) {

            IntervalItem *intervalItem=NULL;
            if (x.quality >= 1.0f) {
                intervalItem = new IntervalItem(f, 
                                                QString(tr("TTE of %1  (%2 watts)")).arg(time_to_string(x.duration)).arg(x.joules/x.duration),
                                                x.start, x.start+x.duration, 
                                                f->timeToDistance(x.start), f->timeToDistance(x.start+x.duration),
                                                count++, QColor(Qt::red), RideFileInterval::TTE);
            } else {
                intervalItem = new IntervalItem(f, 
                                                QString(tr("%3% EFFORT of %1  (%2 watts)")).arg(time_to_string(x.duration)).arg(x.joules/x.duration).arg(int(x.quality*100)),
                                                x.start, x.start+x.duration, 
                                                f->timeToDistance(x.start), f->timeToDistance(x.start+x.duration),
                                                count++, QColor(Qt::red), RideFileInterval::EFFORT);
            }

            intervalItem->rideItem_ = this; // XXX will go when we refactor 
            intervalItem->rideInterval = NULL;
            intervalItem->refresh();        // XXX will get called in constructore when refactor
            intervals_ << intervalItem;

            //qDebug()<<fileName<<"IS EFFORT"<<x.quality<<"at"<<x.start<<"duration"<<x.duration;

        }

        foreach(effort x, candidates_sprint) {

            IntervalItem *intervalItem=NULL;

            intervalItem = new IntervalItem(f,
                                            QString(tr("SPRINT of %1 secs (%2 watts)")).arg(x.duration).arg(x.joules/x.duration),
                                            x.start, x.start+x.duration,
                                            f->timeToDistance(x.start), f->timeToDistance(x.start+x.duration),
                                            count++, QColor(Qt::red), RideFileInterval::SPRINT);


            intervalItem->rideItem_ = this; // XXX will go when we refactor
            intervalItem->rideInterval = NULL;
            intervalItem->refresh();        // XXX will get called in constructore when refactor
            intervals_ << intervalItem;

            //qDebug()<<fileName<<"IS EFFORT"<<x.quality<<"at"<<x.start<<"duration"<<x.duration;

        }

        free(integrated_series);
        //qDebug()<<fileName<<"of"<<secs<<"seconds took "<<timer.elapsed()<<"ms to find"<<candidates.count();
    }


    //qDebug() << "SEARCH HILLS";
    if (!f->isSwim() && f->isDataPresent(RideFile::alt)) {
        // log of progress
        QFile log(context->athlete->home->logs().canonicalPath() + "/" + "climb.log");
        log.open(QIODevice::ReadWrite);
        log.atEnd();
        QTextStream out(&log);

        //qDebug() << "SEARCH CLIMB STARTS: " << fileName;
        out << "SEARCH CLIMB STARTS: " << fileName << "\r\n";
        out << "START" << QDateTime::currentDateTime().toString() + "\r\n";

        // Initialisation
        int hills = 0;

        RideFilePoint *pstart = f->dataPoints().at(0);
        RideFilePoint *pstop = f->dataPoints().at(0);

        foreach(RideFilePoint *p, f->dataPoints()) {
            // new min altitude
            if (pstart->alt > p->alt) {
                //update start
                pstart = p;
                // update stop
                pstop = p;
            }
            // Update max altitude
            if (pstop->alt < p->alt) {
                // update stop
                pstop = p;
            }

            bool downhill = (pstop->alt > p->alt+0.2*(pstop->alt-pstart->alt));
            bool flat = (!downhill && (p->km - pstop->km)>1/3.0*(p->km - pstart->km));
            bool end = (p == f->dataPoints().last() );



            if (flat || downhill || end ) {
                double distance =  pstop->km - pstart->km;


                if (distance >= 0.5) {
                    // Candidat

                    // Check groundrise at end
                    int start = f->dataPoints().indexOf(pstart);
                    int stop = f->dataPoints().indexOf(pstop);

                    for (int i=stop;i>start;i--) {
                        RideFilePoint *p2 = f->dataPoints().at(i);
                        double distance2 =  pstop->km - p2->km;
                        if (distance2>0.1) {
                            if ((pstop->alt-p2->alt)/distance2<20.0) {
                                //qDebug() << "        correct stop " << (pstop->alt-p2->alt)/distance2;
                                pstop = p2;
                            } else
                                i = start;
                        }
                    }

                    for (int i=start;i<stop;i++) {
                        RideFilePoint *p2 = f->dataPoints().at(i);
                        double distance2 = p2->km-pstart->km;
                        if (distance2>0.1) {
                            if ((p2->alt-pstart->alt)/distance2<20.0) {
                                //qDebug() << "        correct start " << (p2->alt-pstart->alt)/distance2;
                                pstart = p2;
                            } else
                                i = stop;
                        }
                    }

                    distance =  pstop->km - pstart->km;
                    double height = pstop->alt - pstart->alt;

                    if (distance >= 0.5) {

                        if ((distance < 4.0 && height/distance >= 60-10*distance) ||
                            (distance >= 4.0 && height/distance >= 20)) {

                            //qDebug() << "    NEW HILL " << (hills+1) << " at " << pstart->km  << "km " << pstart->secs/60.0 <<"-"<< pstop->secs/60.0 << "min " << distance << "km " << height/distance/10.0 << "%";
                            out << "    NEW HILL " << (hills+1) << " at " << pstart->km << "km " << pstart->secs/60.0 <<"-"<< pstop->secs/60.0 << "min " << distance << "km " << height/distance/10.0 << "%\r\n";


                            // create a new interval item
                            IntervalItem *intervalItem = new IntervalItem(f, QString("Climb %1").arg(++hills),
                                                                          pstart->secs, pstop->secs,
                                                                          pstart->km,
                                                                          pstop->km,
                                                                          count++,
                                                                          QColor(Qt::green),
                                                                          RideFileInterval::CLIMB);
                            intervalItem->rideItem_ = this; // XXX will go when we refactor and be passed instead of ridefile
                            intervalItem->rideInterval = NULL;
                            intervalItem->refresh();        // XXX will get called in constructore when refactor
                            intervals_ << intervalItem;
                        } else {
                            out << "        NOT HILL " << "at " << pstart->km << "km " << pstart->secs/60.0 <<"-"<< pstop->secs/60.0 << "min " << distance << "km " << height/distance/10.0 << "%\r\n";

                            //qDebug() << "        NOT HILL " << "at " << pstart->km << "km " <<  pstart->secs/60.0 <<"-"<< pstop->secs/60.0 << "min " <<  distance  << "km" << height/distance/10.0 << "%";
                        }
                    }
                }

                pstart = pstop;
            }
        }
        out << "STOP" << QDateTime::currentDateTime().toString() + "\r\n";
    }


    //Search routes
    if (f->isDataPresent(RideFile::lon)) {
        Routes* routes = context->athlete->routes;
        if (routes->routes.count()>0) {
            for (int n=0;n<routes->routes.count();n++) {
                RouteSegment* route = &routes->routes[n];
                //qDebug() << "find route "<< route->getName() << n;


                for (int j=0;j<route->getRides().count();j++) {
                    RouteRide _ride = route->getRides()[j];
                    QDateTime rideStartDate = route->getRides()[j].startTime;
                    QString rideSegmentName = route->getRides()[j].filename;

                    if (f->startTime() == rideStartDate) {
                        //qDebug() << "find ride "<< fileName <<" for " <<rideSegmentName;

                        // create a new interval item
                        IntervalItem *intervalItem = new IntervalItem(f, route->getName(),
                                                                      _ride.start, _ride.stop,
                                                                      f->timeToDistance(_ride.start),
                                                                      f->timeToDistance(_ride.stop),
                                                                      count++,  // sequence defaults to count
                                                                      QColor(Qt::gray),
                                                                      RideFileInterval::ROUTE);
                        intervalItem->rideItem_ = this; // XXX will go when we refactor and be passed instead of ridefile
                        intervalItem->rideInterval = NULL;
                        intervalItem->refresh();        // XXX will get called in constructore when refactor
                        intervals_ << intervalItem;
                    }
                }
            }
        }
    }
}

QList<IntervalItem*> RideItem::intervalsSelected()
{
    QList<IntervalItem*> returning;
    foreach(IntervalItem *p, intervals_) {
        if (p->selected) returning << p;
    }
    return returning;
}

QList<IntervalItem*> RideItem::intervalsSelected(RideFileInterval::intervaltype type)
{
    QList<IntervalItem*> returning;
    foreach(IntervalItem *p, intervals_) {
        if (p->selected && p->type==type) returning << p;
    }
    return returning;
}

QList<IntervalItem*> RideItem::intervals(RideFileInterval::intervaltype type)
{
    QList<IntervalItem*> returning;
    foreach(IntervalItem *p, intervals_) {
        if (p->type == type) returning << p;
    }
    return returning;
}
