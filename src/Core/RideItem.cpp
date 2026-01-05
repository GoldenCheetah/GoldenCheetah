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
#include "RideCache.h"
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
#include "AddIntervalDialog.h" // till we fixup ridefilecache to have offsets
#include "TimeUtils.h" // time_to_string()
#include "WPrime.h" // for matches

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
    color(QColor(1,1,1)), sport(""), isBike(false), isRun(false), isSwim(false), isXtrain(false), isAero(false), samples(false), zoneRange(-1), hrZoneRange(-1), paceZoneRange(-1), fingerprint(0), metacrc(0), crc(0), timestamp(0), dbversion(0), udbversion(0), weight(0) {
    metrics_.fill(0, RideMetricFactory::instance().metricCount());
    count_.fill(0, RideMetricFactory::instance().metricCount());
}

RideItem::RideItem(RideFile *ride, Context *context) 
    : 
    ride_(ride), fileCache_(NULL), context(context), isdirty(false), isstale(true), isedit(false), skipsave(false), path(""), fileName(""),
    color(QColor(1,1,1)), sport(""), isBike(false), isRun(false), isSwim(false), isXtrain(false), isAero(false), samples(false), zoneRange(-1), hrZoneRange(-1), paceZoneRange(-1), fingerprint(0), metacrc(0), crc(0), timestamp(0), dbversion(0), udbversion(0), weight(0)
{
    metrics_.fill(0, RideMetricFactory::instance().metricCount());
    count_.fill(0, RideMetricFactory::instance().metricCount());
}

RideItem::RideItem(QString path, QString fileName, QDateTime &dateTime, Context *context, bool planned)
    :
    ride_(NULL), fileCache_(NULL), context(context), isdirty(false), isstale(true), isedit(false), skipsave(false), path(path), fileName(fileName),
    dateTime(dateTime), color(QColor(1,1,1)), planned(planned), sport(""), isBike(false), isRun(false), isSwim(false), isXtrain(false), isAero(false), samples(false), zoneRange(-1), hrZoneRange(-1), paceZoneRange(-1), fingerprint(0),
    metacrc(0), crc(0), timestamp(0), dbversion(0), udbversion(0), weight(0) 
{
    metrics_.fill(0, RideMetricFactory::instance().metricCount());
    count_.fill(0, RideMetricFactory::instance().metricCount());
}

// Create a new RideItem destined for the ride cache and used for caching
// pre-computed metrics and storing ride metadata
RideItem::RideItem(RideFile *ride, QDateTime &dateTime, Context *context)
    :
    ride_(ride), fileCache_(NULL), context(context), isdirty(true), isstale(true), isedit(false), skipsave(false), dateTime(dateTime),
    zoneRange(-1), hrZoneRange(-1), paceZoneRange(-1), fingerprint(0), metacrc(0), crc(0), timestamp(0), dbversion(0), udbversion(0), weight(0)
{
    metrics_.fill(0, RideMetricFactory::instance().metricCount());
    count_.fill(0, RideMetricFactory::instance().metricCount());
}

// clone a ride item
void
RideItem::setFrom(RideItem&here, bool temp) // used when loading cache/rideDB.json
{
    ride_ = NULL;
    fileCache_ = NULL;
    metrics_ = here.metrics_;
    count_ = here.count_;
    stdmean_ = here.stdmean_;
    stdvariance_ = here.stdvariance_;
    metadata_ = here.metadata_;
    xdata_ = here.xdata_;
    errors_ = here.errors_;
    intervals_ = here.intervals_;

    // don't update the interval pointers if this is a 
    // temporary "fake" rideitem.
    if (!temp)
        foreach(IntervalItem *p, intervals_)
            p->rideItem_ = this;

    context = here.context;
    isdirty = here.isdirty;
    isstale = here.isstale;
    isedit = here.isedit;
    skipsave = here.skipsave;
    if (planned == false)
        path = here.path;
    fileName = here.fileName;
    dateTime = here.dateTime;
    zoneRange = here.zoneRange;
    hrZoneRange = here.hrZoneRange;
    paceZoneRange = here.paceZoneRange;
    fingerprint = here.fingerprint;
    metacrc = here.metacrc;
    crc = here.crc;
    timestamp = here.timestamp;
    dbversion = here.dbversion;
    udbversion = here.udbversion;
    color = here.color;
    present = here.present;
    sport = here.sport;
    isBike = here.isBike;
    isRun = here.isRun;
    isSwim = here.isSwim;
    isXtrain = here.isXtrain;
    isAero = here.isAero;
    weight = here.weight;
    overrides_ = here.overrides_;
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
        count_[i.value()->index()] = i.value()->count();
        double stdmean = i.value()->stdmean();
        double stdvariance = i.value()->stdvariance();
        if (stdmean || stdvariance) {
            stdmean_.insert(i.value()->index(), stdmean);
            stdvariance_.insert(i.value()->index(), stdvariance);
        }
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

        // ignore calendar texts as they change 
        // with configuration, not user updates
        if (i.key() == "Calendar Text") continue;

        ba.append(i.key().toUtf8());
        ba.append(i.value().toUtf8());
    }
    return qChecksum(ba);
}

RideFile *RideItem::ride(bool open)
{
    if (!open || ride_) return ride_;

    // open the ride file
    QFile file(path + "/" + fileName);
    ride_ = RideFileFactory::instance().openRideFile(context, file, errors_);
    if (ride_ == NULL) return NULL; // failed to read ride

    // update the overrides
    overrides_.clear();
    QMap<QString,QMap<QString, QString> >::const_iterator k;
    for (k=ride_->metricOverrides.constBegin(); k != ride_->metricOverrides.constEnd(); k++) {
        overrides_ << k.key();
    }

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
    // add to the deleted list
    if (context && context->athlete && context->athlete->rideCache) context->athlete->rideCache->deletelist << this;

    //qDebug()<<"deleting:"<<fileName;
    if (isOpen()) close();
    if (fileCache_) delete fileCache_;
    //XXX need to consider what to do here for the intervalitem
    //XXX used by the RideDB parser - we don't want to wipe away
    //XXX the intervals we just passed into setFrom()
    //foreach(IntervalItem*x, intervals_) delete x;
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

    // connect up to new one - if its not null
    if (ride_) {
        connect(ride_, SIGNAL(modified()), this, SLOT(modified()));
        connect(ride_, SIGNAL(saved()), this, SLOT(saved()));
        connect(ride_, SIGNAL(reverted()), this, SLOT(reverted()));

        // update status
        setDirty(true);
        notifyRideDataChanged();
    }

    // don't bother with the old one any more
    if (old) disconnect(old);

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
RideItem::moveInterval(int from, int to)
{
    // Move in RideFile
    int from2 = ride()->intervals().indexOf(intervals_.at(from)->rideInterval);
    int to2 = ride()->intervals().indexOf(intervals_.at(to)->rideInterval);
    ride()->moveInterval(from2, to2);

    // Move in RideItem
    intervals_.move(from, to);
}

void
RideItem::addInterval(IntervalItem item)
{
    IntervalItem *add = new IntervalItem(item);
    add->rideItem_ = this;
    intervals_ << add;
}

IntervalItem *
RideItem::newInterval(QString name, double start, double stop, double startKM, double stopKM, QColor color, bool test)
{
    // add a new interval to the end of the list
    color = color == Qt::black ? standardColor(intervals(RideFileInterval::USER).count()) : color;

    IntervalItem *add = new IntervalItem(this, name, start, stop, startKM, stopKM, 1,
                                         color, test, RideFileInterval::USER);
    // add to RideFile
    add->rideInterval = ride()->newInterval(name, start, stop, color, test);

    // add to list
    intervals_ << add;

    // refresh metrics
    add->refresh();

    // still the item is dirty and needs to be saved
    setDirty(true);

    // and return
    return add;
}

void
RideItem::notifyRideDataChanged()
{
    // refresh the metrics
    isstale=true;

    // wipe user data
    userCache.clear();

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
    // ride data
    if (ride_) {
        // break link to ride file
        foreach(IntervalItem *x, intervals()) x->rideInterval = NULL;
        delete ride_;
        ride_ = NULL;
    }

    // and the cpx data
    if (fileCache_) {
    	delete fileCache_;
	fileCache_=NULL;
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
    color = GlobalContext::context()->colorEngine->colorFor(getText(GlobalContext::context()->rideMetadata->getColorField(), ""));

    // upgraded metrics
    if (udbversion != UserMetricSchemaVersion || dbversion != DBSchemaVersion) {

        isstale = true;

    } else {

        // has weight changed?
        unsigned long prior  = 1000.0f * weight;
        unsigned long now = 1000.0f * getWeight();

        if (prior != now) {

            getWeight();
            isstale = true;

        } else {

            // or have cp / zones or routes fingerprints changed ?
            // note we now get the fingerprint from the zone range
            // and not the entire config so that if you add a new
            // range (e.g. set CP from today) but none of the other
            // ranges change then there is no need to recompute the
            // metrics for older rides !
            // HRV fingerprint added to detect changes on HRV Measures

            // get the new zone configuration fingerprint that applies for the ride date
            unsigned long rfingerprint = static_cast<unsigned long>(context->athlete->zones(sport)->getFingerprint(dateTime.date()))
                        + (appsettings->cvalue(context->athlete->cyclist, context->athlete->zones(sport)->useCPforFTPSetting(), 0).toInt() ? 1 : 0)
                        + static_cast<unsigned long>(context->athlete->paceZones(isSwim)->getFingerprint(dateTime.date()))
                        + static_cast<unsigned long>(context->athlete->hrZones(sport)->getFingerprint(dateTime.date()))
                        + static_cast<unsigned long>(context->athlete->routes->getFingerprint())
                        + static_cast<unsigned long>(getHrvFingerprint())
                        + appsettings->cvalue(context->athlete->cyclist, GC_DISCOVERY, 57).toInt(); // 57 does not include search for PEAKS

            if (fingerprint != rfingerprint) {

                isstale = true;

            } else {

                // or has file content changed ?
                QString fullPath =  QString(context->athlete->home->activities().absolutePath()) + "/" + fileName;
                QFile file(fullPath);

                // has timestamp changed ?
                if (timestamp < QFileInfo(file).lastModified().toSecsSinceEpoch()) {

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


QString RideItem::getLinkedFileName() const
{
    return metadata_.value("Linked Filename", "");
}

void RideItem::setLinkedFileName(const QString &fileName)
{
    RideFile *r = ride(true);
    if (! r) {
        return;
    }
    r->setTag("Linked Filename", fileName);
    metadata_.insert("Linked Filename", fileName);
    setDirty(true);
    notifyRideMetadataChanged();
}

void RideItem::clearLinkedFileName()
{
    RideFile *r = ride(true);
    if (! r) {
        return;
    }
    r->removeTag("Linked Filename");
    metadata_.remove("Linked Filename");
    setDirty(true);
    notifyRideMetadataChanged();
}

bool RideItem::hasLinkedActivity() const
{
    return ! getLinkedFileName().isEmpty();
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

        // get the metadata
        metadata_ = f->tags();

        // get xdata definitions
        QMapIterator<QString, XDataSeries *>ie(f->xdata());
        ie.toFront();
        while(ie.hasNext()) {
            ie.next();

            // xdata and series names
            xdata_.insert(ie.value()->name, ie.value()->valuename);
        }

        // overrides
        overrides_.clear();
        QMap<QString,QMap<QString, QString> >::const_iterator k;
        for (k=ride_->metricOverrides.constBegin(); k != ride_->metricOverrides.constEnd(); k++) {
            overrides_ << k.key();
        }

        // get weight that applies to the date
        getWeight();

        // first class stuff
        sport = f->sport();
        isBike = f->isBike();
        isRun = f->isRun();
        isSwim = f->isSwim();
        isXtrain = f->isXtrain();
        isAero = f->isAero();
        color = GlobalContext::context()->colorEngine->colorFor(f->getTag(GlobalContext::context()->rideMetadata->getColorField(), ""));
        present = f->getTag("Data", "");
        samples = f->dataPoints().count() > 0;

        // zone ranges
        if (context->athlete->zones(sport)) zoneRange = context->athlete->zones(sport)->whichRange(dateTime.date());
        else zoneRange = -1;

        if (context->athlete->hrZones(sport)) hrZoneRange = context->athlete->hrZones(sport)->whichRange(dateTime.date());
        else hrZoneRange = -1;

        if (context->athlete->paceZones(isSwim)) paceZoneRange = context->athlete->paceZones(isSwim)->whichRange(dateTime.date());
        else paceZoneRange = -1;

        // RideFile cache refresh before metrics, as meanmax may be used in user formulas
        RideFileCache updater(context, context->athlete->home->activities().canonicalPath() + "/" + fileName, getWeight(), ride_, true);

        // refresh metrics etc
        const RideMetricFactory &factory = RideMetricFactory::instance();

        // ressize and initialize so we can store metric values at
        // RideMetric::index offsets into the metrics_ qvector
        metrics_.fill(0, factory.metricCount());
        count_.fill(0, factory.metricCount());

        // we compute all with not specification (not an interval)
        QHash<QString,RideMetricPtr> computed= RideMetric::computeMetrics(this, Specification(), factory.allMetrics());

        // snaffle away all the computed values into the array
        QHashIterator<QString, RideMetricPtr> i(computed);
        while (i.hasNext()) {
            i.next();
            //DEBUG if (i.value()->isUser()) qDebug()<<dateTime.date()<<i.value()->symbol()<<i.value()->value();
            metrics_[i.value()->index()] = i.value()->value();
            count_[i.value()->index()] = i.value()->count();
            double stdmean = i.value()->stdmean();
            double stdvariance = i.value()->stdvariance();
            if (stdmean || stdvariance) {
                stdmean_.insert(i.value()->index(), stdmean);
                stdvariance_.insert(i.value()->index(), stdvariance);
            }
        }

        // clean any bad values
        for(int j=0; j<factory.metricCount(); j++)
            if (std::isinf(metrics_[j]) || std::isnan(metrics_[j])) {
                metrics_[j] = 0.00f;
                count_[j] = 0.00f;
            }

        // Update auto intervals AFTER ridefilecache as used for bests
        updateIntervals();

        // update fingerprints etc, crc done above
        fingerprint = static_cast<unsigned long>(context->athlete->zones(sport)->getFingerprint(dateTime.date()))
                    + (appsettings->cvalue(context->athlete->cyclist, context->athlete->zones(sport)->useCPforFTPSetting(), 0).toInt() ? 1 : 0)
                    + static_cast<unsigned long>(context->athlete->paceZones(isSwim)->getFingerprint(dateTime.date()))
                    + static_cast<unsigned long>(context->athlete->hrZones(sport)->getFingerprint(dateTime.date()))
                    + static_cast<unsigned long>(context->athlete->routes->getFingerprint()) +
                    + static_cast<unsigned long>(getHrvFingerprint())
                    + appsettings->cvalue(context->athlete->cyclist, GC_DISCOVERY, 57).toInt(); // 57 does not include search for PEAKS

        dbversion = DBSchemaVersion;
        udbversion = UserMetricSchemaVersion;
        timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();

        // we now match
        metacrc = metaCRC();

        // Construct the summary text used on the calendar
        metadata_.insert("Calendar Text", GlobalContext::context()->rideMetadata->calendarText(this));

        // close if we opened it
        if (doclose) {
            close();
        } else {

            // if it is open then recompute
            userCache.clear();
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
RideItem::getWeight(int type)
{
    // get any body measurements first
    MeasuresGroup* pBodyMeasures = context->athlete->measures->getGroup(Measures::Body);
    double m = pBodyMeasures ? pBodyMeasures->getFieldValue(dateTime.date(), type) : 0.0;

    // return what was asked for!
    if (type == Measure::WeightKg) {
        // get weight from whatever we got
        weight = m;

        // from metadata
        if (weight <= 0.00) weight = metadata_.value("Weight", "0.0").toDouble();

        // global options and if not set default to 75 kg.
        if (weight <= 0.00) weight = appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT, "75.0").toString().toDouble();

        // No weight default is weird, we'll set to 80kg
        if (weight <= 0.00) weight = 80.00;

        return weight;
    } else {
        // all the other weight measures supported by BodyMetrics
        return m;
    }
}

double
RideItem::getHrvMeasure(QString fieldSymbol)
{
    // get HRV measure for the date of the ride
    MeasuresGroup *pHrvMeasures = context->athlete->measures->getGroup(Measures::Hrv);
    if (pHrvMeasures) {
        return pHrvMeasures->getFieldValue(dateTime.date(), pHrvMeasures->getFieldSymbols().indexOf(fieldSymbol));
    } else {
        return 0.0;
    }
}

unsigned short
RideItem::getHrvFingerprint()
{
    // get HRV measure for the date of the ride
    MeasuresGroup* pHrvMeasures = context->athlete->measures->getGroup(Measures::Hrv);
    if (pHrvMeasures) {
        Measure hrvMeasure;
        pHrvMeasures->getMeasure(dateTime.date(), hrvMeasure);
        return hrvMeasure.getFingerprint();
    } else {
        return 0;
    }
}

double
RideItem::getForSymbol(QString name, bool useMetricUnits)
{
    const RideMetricFactory &factory = RideMetricFactory::instance();
    if (metrics_.size() && metrics_.size() == factory.metricCount()) {
        // return the precomputed metric value
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

double
RideItem::getCountForSymbol(QString name)
{
    const RideMetricFactory &factory = RideMetricFactory::instance();
    if (metrics_.size() && metrics_.size() == factory.metricCount()) {
        // return the precomputed metric value
        const RideMetric *m = factory.rideMetric(name);
        if (m)  {
            // don't return zero (!)
            double returning = count_[m->index()];
            return returning ? returning : 1;
        }
    }
    // don't return zero, thats impossible
    return 1.0f;
}

double
RideItem::getStdMeanForSymbol(QString name)
{
    const RideMetricFactory &factory = RideMetricFactory::instance();
    if (metrics_.size() && metrics_.size() == factory.metricCount()) {
        // return the precomputed metric value
        const RideMetric *m = factory.rideMetric(name);
        if (m)  {
            // don't return zero (!)
            return stdmean_.value(m->index(), 0.0f);
        }
    }
    return 0.0f;
}

double
RideItem::getStdVarianceForSymbol(QString name)
{
    const RideMetricFactory &factory = RideMetricFactory::instance();
    if (metrics_.size() && metrics_.size() == factory.metricCount()) {
        // return the precomputed metric value
        const RideMetric *m = factory.rideMetric(name);
        if (m)  {
            // don't return zero (!)
            return stdvariance_.value(m->index(), 0.0f);
        }
    }
    return 0.0f;
}

// access the metadata
QString
RideItem::getText(QString name, QString fallback) const
{
    // Start Date and Time are special cases, defined as metadata fields but stored in a different way
    if (name == "Start Date") return QString::number(QDate(1900,01,01).daysTo(dateTime.date()));
    if (name == "Start Time") return QString::number(QTime(0,0,0).secsTo(dateTime.time()));
    return metadata_.value(name, fallback);
}

bool
RideItem::hasText(QString name) const
{
    if (name == "Start Date") return true;
    if (name == "Start Time") return true;
    return metadata_.contains(name);
}

QString
RideItem::getStringForSymbol(QString name, bool useMetricUnits)
{
    QString returning("-");

    const RideMetricFactory &factory = RideMetricFactory::instance();
    if (metrics_.size() && metrics_.size() == factory.metricCount()) {

        // return the precomputed metric value
        const RideMetric *m = factory.rideMetric(name);
        if (m) {

            double value = metrics_[m->index()];
            if (std::isinf(value) || std::isnan(value)) value=0;
            returning = m->toString(m->value(value, useMetricUnits));
        }
    }
    return returning;
}

struct effort {
    int start, duration, joules;
    int zone;
    double quality;
};

static bool intervalGreaterThanZone(const IntervalItem *a, const IntervalItem *b) { 
    return const_cast<IntervalItem*>(a)->getForSymbol("power_zone") > 
           const_cast<IntervalItem*>(b)->getForSymbol("power_zone"); 
}

void
RideItem::updateIntervals()
{
    // what do we need ?
    int discovery = appsettings->cvalue(context->athlete->cyclist, GC_DISCOVERY, 57).toInt(); // 57 does not include search for PEAKS

    // DO NOT USE ride() since it will call a refresh !
    RideFile *f = ride_;

    QList<IntervalItem*> deletelist = intervals_;
    intervals_.clear();

    // no ride data available ?
    if (!samples) {
        context->notifyIntervalsUpdate(this);
        return;
    }

    // Get CP and W' estimates for date of ride
    double CP = 0;
    double WPRIME = 0;
    double PMAX = 0;
    bool zoneok = false;

    if (context->athlete->zones(sport)) {

        // if range is -1 we need to fall back to a default value
        CP = zoneRange >= 0 ? context->athlete->zones(sport)->getCP(zoneRange) : 0;
        WPRIME = zoneRange >= 0 ? context->athlete->zones(sport)->getWprime(zoneRange) : 0;
        PMAX = zoneRange >= 0 ? context->athlete->zones(sport)->getPmax(zoneRange) : 0;

        // did we override CP in metadata ?
        int oCP = getText("CP","0").toInt();
        int oW = getText("W'","0").toInt();
        int oPMAX = getText("Pmax","0").toInt();
        if (oCP) CP=oCP;
        if (oW) WPRIME=oW;
        if (oPMAX) PMAX=oPMAX;

        if (zoneRange >= 0 && context->athlete->zones(sport)) zoneok=true;
    }

    // USER / DEVICE INTERVALS
    // first we create interval items for all intervals
    // that are in the ridefile, but ignore Peaks since we
    // add those automatically for HR and Power where those
    // data series are present

    // ride start and end
    RideFilePoint *begin = f->dataPoints().first();
    RideFilePoint *end = f->dataPoints().last();

    // ALL interval
    if (discovery & RideFileInterval::intervalTypeBits(RideFileInterval::ALL)) {

        // add entire ride using ride metrics
        IntervalItem *entire = new IntervalItem(this, tr("Entire Activity"), 
                                                begin->secs, end->secs, 
                                                f->timeToDistance(begin->secs),
                                                f->timeToDistance(end->secs),
                                                0,
                                                QColor(Qt::darkBlue),
                                                false,
                                                RideFileInterval::ALL);

        // same as the whole ride, not need to compute
        entire->refresh();
        entire->rideInterval = NULL;
        intervals_ << entire;
    }

    int count = 0;
    foreach(RideFileInterval *interval, f->intervals()) {

        // skip peaks when autodiscovered
        if (discovery & RideFileInterval::intervalTypeBits(RideFileInterval::PEAKPOWER) && interval->isPeak()) continue;

        // skip climbs when autodiscovered
        if (discovery & RideFileInterval::intervalTypeBits(RideFileInterval::CLIMB) && interval->isClimb()) continue;

        // skip matches when autodiscovered
        if (discovery & RideFileInterval::intervalTypeBits(RideFileInterval::EFFORT) && interval->isMatch()) continue;

        // skip entire ride when autodiscovered
        if (discovery & RideFileInterval::intervalTypeBits(RideFileInterval::ALL) && 
           ((interval->start <= begin->secs && interval->stop >= end->secs) ||
           (((interval->start - f->recIntSecs()) <= begin->secs && (interval->stop-f->recIntSecs()) >= end->secs) ||
           (interval->start <= begin->secs && (interval->stop+f->recIntSecs()) >= end->secs))))
             continue;

        // skip empty backward intervals
        if (interval->start > interval->stop) continue;

        // create a new interval item
        const int seq = count; // if passed directly, it could be incremented BEFORE being evaluated for the sequence arg as arg eval order is undefined
        IntervalItem *intervalItem = new IntervalItem(this, interval->name, 
                                                      interval->start, interval->stop, 
                                                      f->timeToDistance(interval->start),
                                                      f->timeToDistance(interval->stop),
                                                      seq,
                                                      (interval->color == Qt::black) ? standardColor(count) : interval->color,
                                                      interval->test,
                                                      RideFileInterval::USER);

        intervalItem->rideInterval = interval;
        intervalItem->refresh();        // XXX will get called in constructor when refactor
        intervals_ << intervalItem;

        count++;
        //qDebug()<<"interval:"<<interval.name<<interval.start<<interval.stop<<"f:"<<begin->secs<<end->secs;
    }

    // DISCOVERY

    //qDebug() << "SEARCH PEAK POWERS"
    if ((discovery & RideFileInterval::intervalTypeBits(RideFileInterval::PEAKPOWER)) &&
        !f->isRun() && !f->isSwim() && f->isDataPresent(RideFile::watts)) {

        // what we looking for ?
        static int durations[] = { 1, 5, 10, 15, 20, 30, 60, 300, 600, 1200, 1800, 2700, 3600, 0 };
        static QString names[] = { tr("1 second"), tr("5 seconds"), tr("10 seconds"), tr("15 seconds"), tr("20 seconds"), tr("30 seconds"),
                                tr("1 minute"), tr("5 minutes"), tr("10 minutes"), tr("20 minutes"), tr("30 minutes"), tr("45 minutes"),
                                tr("1 hour") };
    
        for(int i=0; durations[i] != 0; i++) {

            // go hunting for best peak
            QList<AddIntervalDialog::AddedInterval> results;
            AddIntervalDialog::findPeaks(context, true, f, Specification(), RideFile::watts, RideFile::original, durations[i], 1, results, "", "");

            // did we get one ?
            if (results.count() > 0 && results[0].avg > 0 && results[0].stop > 0) {
                // qDebug()<<"found"<<names[i]<<"peak power"<<results[0].start<<"-"<<results[0].stop<<"of"<<results[0].avg<<"watts";
                IntervalItem *intervalItem = new IntervalItem(this, QString(tr("%1 (%2 watts)")).arg(names[i]).arg(int(results[0].avg)),
                                                            results[0].start, results[0].stop, 
                                                            f->timeToDistance(results[0].start),
                                                            f->timeToDistance(results[0].stop),
                                                            count++,
                                                            QColor(Qt::gray),
                                                            false,
                                                            RideFileInterval::PEAKPOWER);
                intervalItem->rideInterval = NULL;
                intervalItem->refresh();        // XXX will get called in constructore when refactor
                intervals_ << intervalItem;
            }
        }
    }

    //qDebug() << "SEARCH PEAK PACE"
    if ((discovery & RideFileInterval::intervalTypeBits(RideFileInterval::PEAKPACE)) &&
        (f->isRun() || f->isSwim()) && f->isDataPresent(RideFile::kph)) {

        // what we looking for ?
        static int durations[] = { 10, 15, 20, 30, 60, 300, 600, 1200, 1800, 2700, 3600, 0 };
        static QString names[] = { tr("10 seconds"), tr("15 seconds"), tr("20 seconds"), tr("30 seconds"),
                                tr("1 minute"), tr("5 minutes"), tr("10 minutes"), tr("20 minutes"), tr("30 minutes"), tr("45 minutes"),
                                tr("1 hour") };

        bool metric = appsettings->value(this, context->athlete->paceZones(f->isSwim())->paceSetting(), GlobalContext::context()->useMetricUnits).toBool();
        for(int i=0; durations[i] != 0; i++) {

            // go hunting for best peak
            QList<AddIntervalDialog::AddedInterval> results;
            AddIntervalDialog::findPeaks(context, true, f, Specification(), RideFile::kph, RideFile::original, durations[i], 1, results, "", "");

            // did we get one ?
            if (results.count() > 0 && results[0].avg > 0 && results[0].stop > 0) {
                // qDebug()<<"found"<<names[i]<<"peak pace"<<results[0].start<<"-"<<results[0].stop<<"of"<<results[0].avg<<"kph";
                IntervalItem *intervalItem = new IntervalItem(this, QString(tr("%1 (%2 %3)")).arg(names[i])
                               .arg(context->athlete->paceZones(f->isSwim())->kphToPaceString(results[0].avg, metric))
                               .arg(context->athlete->paceZones(f->isSwim())->paceUnits(metric)),
                                                            results[0].start, results[0].stop, 
                                                            f->timeToDistance(results[0].start),
                                                            f->timeToDistance(results[0].stop),
                                                            count++,
                                                            QColor(Qt::gray),
                                                            false,
                                                            RideFileInterval::PEAKPACE);
                intervalItem->rideInterval = NULL;
                intervalItem->refresh();        // XXX will get called in constructore when refactor
                intervals_ << intervalItem;
            }
        }
    }


    //qDebug() << "SEARCH EFFORTS";
    QList<effort> candidates[10];
    QList<effort> candidates_sprint;
    
    if ((discovery & RideFileInterval::intervalTypeBits(RideFileInterval::EFFORT)) &&
        CP > 0 && WPRIME > 0 && PMAX > 0 && !f->isRun() && !f->isSwim() && f->isDataPresent(RideFile::watts)) {

        const int SAMPLERATE = 1000; // 1000ms samplerate = 1 second samples

        RideFilePoint sample;        // we reuse this to aggregate all values
        long time = 0L;              // current time accumulates as we run through data
        double lastT = 0.0f;         // last sample time seen in seconds

        // set the array size
        int arraySize = f->dataPoints().last()->secs + f->recIntSecs();

        // anything longer than a day or negative is skipped
        if (arraySize >= 0 && arraySize < (24*3600)) { // no indent, as added late

        QElapsedTimer timer;
        timer.start();

        // setup an integrated series
        long *integrated_series = (long*)malloc(sizeof(long) * arraySize);
        long *pi = integrated_series;
        long rtot = 0;

        long secs = 0;
        foreach(RideFilePoint *p, f->dataPoints()) {

            // increment secs by recIntSecs as the time series
            // always starts at zero, normalized by the file reader
            double psecs = p->secs + f->recIntSecs();

            // whats the dt in microseconds
            int dt = (psecs * 1000) - (lastT * 1000);
            lastT = psecs;


            // ignore time goes backwards
            if (dt < 0) continue;

            //
            // AGGREGATE INTO SAMPLES
            //
            while (secs < arraySize && dt) {

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
                    rtot += sample.watts;
                    *pi++ = rtot;
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
        for (long i=0; i<secs; i++) {

            // start out at 30 minutes and drop back to
            // 2 minutes, anything shorter and we are done
            int t = (secs-i-1) > 3600 ? 3600 : secs-i-1;

            // if we find one lets record it
            bool found = false;
            bool foundSprint = false;
            effort tte;
            effort sprint;

            while (t > 120) {

                // calculate the TTE for the joules in the interval
                // starting at i seconds with duration t
                // This takes the monod equation p(t) = W'/t + CP and
                // solves for t, but the added complication of also
                // accounting for the fact it is expressed in joules
                // So take Joules = (W'/t + CP) * t and solving that
                // for t gives t = (Joules - W') / CP
                double tc = ((integrated_series[i+t]-integrated_series[i]) - WPRIME) / CP;
                // NOTE FOR ABOVE: it is looking at accumulation AFTER this point
                //                 not FROM this point, so we are looking 1s ahead of i
                //                 which is why the interval is registered as starting
                //                 at i+1 in the code below

                // the TTE for this interval is greater or equal to
                // the duration of the interval !
                if (tc >= (t*0.85f)) {

                    if (found == false) {

                        // first one we found
                        found = true;

                        // register a candidate
                        tte.start = i + 1; // see NOTE above
                        tte.duration = t;
                        tte.joules = integrated_series[i+t]-integrated_series[i];
                        tte.quality = tc / double(t);
                        tte.zone = zoneok ? context->athlete->zones(sport)->whichZone(zoneRange, tte.joules/tte.duration) : 1;

                    } else {

                        double thisquality = tc / double(t);

                        // found one with a higher quality
                        if (tte.quality < thisquality) {
                            tte.duration = t;
                            tte.joules = integrated_series[i+t]-integrated_series[i];
                            tte.quality = thisquality;
                            tte.zone = zoneok ? context->athlete->zones(sport)->whichZone(zoneRange, tte.joules/tte.duration) : 1;
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

            //if (t>60)
            //    t=60;

            // Search sprint
            while (t >= 5) {
                // On Pmax only
                // double tc = (integrated_series[i+t]-integrated_series[i]) / (PMAX);

                // With the 3 components model
                // t = W'/(P − CP) + W'/(CP − Pmax)
                double p = (integrated_series[i+t]-integrated_series[i])/t;

                if (p>0.5*(PMAX-CP)+CP) {
                    double tc = WPRIME / (p-CP) + WPRIME / ( CP - PMAX);

                    if (tc >= (t*0.85f)) {

                        if (foundSprint == false) {

                            // first one we found
                            foundSprint = true;

                            // register a candidate
                            sprint.start = i + 1; // see NOTE above
                            sprint.duration = t;
                            sprint.joules = integrated_series[i+t]-integrated_series[i];
                            sprint.quality = double(t) + (sprint.joules/sprint.duration/1000.0);

                        } else {

                            double thisquality = double(t) + (integrated_series[i+t]-integrated_series[i])/t/1000.0;

                            // found one with a higher quality
                            if (sprint.quality < thisquality) {
                                sprint.duration = t;
                                sprint.joules = integrated_series[i+t]-integrated_series[i];
                                sprint.quality = thisquality;
                            }

                        }
                        //qDebug() << "sprint" << i << sprint.duration << sprint.joules/sprint.duration << "W" << sprint.quality;
                    }
                }
                // look for smaller
                t--;
            }


            // add the best one we found here
            if (found && tte.zone >= 0) {

                // if we overlap with the last one and
                // we are better then replace otherwise skip
                if (candidates[tte.zone].count()) {

                    effort &last = candidates[tte.zone].last();
                    if ((tte.start >= last.start && tte.start <= (last.start+last.duration)) ||
                       (tte.start+tte.duration >= last.start && tte.start+tte.duration <= (last.start+last.duration))) {

                        // we overlap but we are higher quality
                        if (tte.quality > last.quality) last = tte;

                    } else{

                        // we don't overlap
                        candidates[tte.zone] << tte;
                    }
                } else {

                    // we are the first
                    candidates[tte.zone] << tte;
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
        for (int i=0; i<10; i++) {
        foreach(effort x, candidates[i]) {

            IntervalItem *intervalItem=NULL;
            int zone = zoneok ? 1 + context->athlete->zones(sport)->whichZone(zoneRange, x.joules/x.duration) : 1;

            if (x.quality >= 1.0f) {
                intervalItem = new IntervalItem(this, 
                                                QString(tr("L%3 TTE of %1  (%2 watts)")).arg(time_to_string(x.duration)).arg(x.joules/x.duration).arg(zone),
                                                x.start, x.start+x.duration, 
                                                f->timeToDistance(x.start), f->timeToDistance(x.start+x.duration),
                                                count++, QColor(Qt::red), false, RideFileInterval::EFFORT);
            } else {
                intervalItem = new IntervalItem(this, 
                                                QString(tr("L%4 %3% EFFORT of %1  (%2 watts)")).arg(time_to_string(x.duration)).arg(x.joules/x.duration).arg(int(x.quality*100)).arg(zone),
                                                x.start, x.start+x.duration, 
                                                f->timeToDistance(x.start), f->timeToDistance(x.start+x.duration),
                                                count++, QColor(Qt::red), false, RideFileInterval::EFFORT);
            }

            intervalItem->rideInterval = NULL;
            intervalItem->refresh();        // XXX will get called in constructore when refactor
            intervals_ << intervalItem;

            //qDebug()<<fileName<<"IS EFFORT"<<x.quality<<"at"<<x.start<<"duration"<<x.duration;

        }
        }

        foreach(effort x, candidates_sprint) {

            IntervalItem *intervalItem=NULL;

            int zone = zoneok ? 1 + context->athlete->zones(sport)->whichZone(zoneRange, x.joules/x.duration) : 1;
            intervalItem = new IntervalItem(this,
                                            QString(tr("L%3 SPRINT of %1 secs (%2 watts)")).arg(x.duration).arg(x.joules/x.duration).arg(zone),
                                            x.start, x.start+x.duration,
                                            f->timeToDistance(x.start), f->timeToDistance(x.start+x.duration),
                                            count++, QColor(Qt::red), false, RideFileInterval::EFFORT);


            intervalItem->rideInterval = NULL;
            intervalItem->refresh();        // XXX will get called in constructore when refactor
            intervals_ << intervalItem;

            //qDebug()<<fileName<<"IS EFFORT"<<x.quality<<"at"<<x.start<<"duration"<<x.duration;

        }

        free(integrated_series);

    // we skipped for whatever reason
        //qDebug()<<fileName<<"of"<<secs<<"seconds took "<<timer.elapsed()<<"ms to find"<<candidates.count();
    }
    } // if arraySize is in bounds, no indent from above

    //qDebug() << "SEARCH HILLS";
    if ((discovery & RideFileInterval::intervalTypeBits(RideFileInterval::CLIMB)) &&
        !f->isSwim() && f->isDataPresent(RideFile::alt)) {

        //qDebug() << "SEARCH CLIMB STARTS: " << fileName;

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

                            // create a new interval item
                            IntervalItem *intervalItem = new IntervalItem(this, QString(tr("Climb %1")).arg(++hills),
                                                                          pstart->secs, pstop->secs,
                                                                          pstart->km,
                                                                          pstop->km,
                                                                          count++,
                                                                          QColor(Qt::green),
                                                                          false,
                                                                          RideFileInterval::CLIMB);
                            intervalItem->rideInterval = NULL;
                            intervalItem->refresh();        // XXX will get called in constructore when refactor
                            intervals_ << intervalItem;
                        } else {
                            //qDebug() << "        NOT HILL " << "at " << pstart->km << "km " <<  pstart->secs/60.0 <<"-"<< pstop->secs/60.0 << "min " <<  distance  << "km" << height/distance/10.0 << "%";
                        }
                    }
                }

                pstart = pstop;
            }
        }
        //qDebug() << "STOP" << QDateTime::currentDateTime().toString() + "\r\n";
    }


    //Search routes
    if ((discovery & RideFileInterval::intervalTypeBits(RideFileInterval::ROUTE)) && f->isDataPresent(RideFile::lon)) {

        // set intervals for routes
        QList<IntervalItem*> here;
        context->athlete->routes->search(this, f, here);

        // Sort routes so they are added by start time to the activity.
        std::sort(
            here.begin(),
            here.end(),
            [](IntervalItem* i1, IntervalItem* i2) {
                return *i1 < *i2;
            }
        );

        // add to ride !
        foreach(IntervalItem *add, here) {
            add->rideInterval = NULL;
            add->refresh();
            intervals_ << add;
        }
    }

    // Search W' MATCHES incl. those that take us to EXHAUSTION
    if ((discovery & RideFileInterval::intervalTypeBits(RideFileInterval::EFFORT)) &&
        f->isDataPresent(RideFile::watts) && f->wprimeData()) {

        // add one for each
        foreach(struct Match match, f->wprimeData()->matches) {

            // anything under 2000joules isn't worth worrying about
            if (match.cost > 2000) {

                // create a new interval item
                IntervalItem *intervalItem = new IntervalItem(this, "", // will update name once AP computed
                                                            match.start, match.stop,
                                                            f->timeToDistance(match.start), f->timeToDistance(match.stop),
                                                            count++,
                                                            match.exhaust ? QColor(255,69,0) : QColor(255,165,0),
                                                            false, // XXX FIXME should this be a test if to exhaustion ??? XXX
                                                            RideFileInterval::EFFORT);
                intervalItem->rideInterval = NULL;
                intervalItem->refresh();        // XXX will get called in constructore when refactor

                // now all the metrics are computed update the name to
                // reflect the AP which was calculated for it, and duration

                // which zone was this match ?
                double ap = intervalItem->getForSymbol("average_power");
                double duration = intervalItem->getForSymbol("workout_time");
                int zone = zoneok ? 1 + context->athlete->zones(sport)->whichZone(zoneRange, ap) : 1;

                intervalItem->name = QString(tr("L%1 %5 %2 (%3w %4 kJ)"))
                                                 .arg(zone)
                                                 .arg(time_to_string(duration))
                                                 .arg((int)ap)
                                                 .arg(match.cost/1000)
                                                 .arg(match.exhaust ? tr("TE MATCH") : tr("MATCH"));

                intervals_ << intervalItem;
            }
        }
    }

    // we now calculate sustained time in zone metrics
    // this uses the EFFORT intervals, if the point
    // is part of an effort interval we include it
    // and we start from the top zone and work down

    // aggregate in this array before updating the metric
    QList<IntervalItem *> efforts = intervals(RideFileInterval::EFFORT);

    // if not discovering then there won't be any!
    if (efforts.count()) {

        // we have some efforts so some time was in a sustained effort
        double stiz[10];
        for (int j=0; j<10; j++) stiz[j] = 0.00f;

        // get and sort the intervals by zone high to low
        std::sort(efforts.begin(), efforts.end(), intervalGreaterThanZone);

        foreach(RideFilePoint *p, f->dataPoints()) {

            foreach (IntervalItem *i, efforts) {
                if (i->start <= p->secs &&
                    i->stop >= p->secs) {

                    int zone = i->getForSymbol("power_zone")-1;
                    if (zone >= 0 && zone < 10) stiz[zone] += f->recIntSecs();
                    break;
                }
            }
        }

        // pack the values into the metric array
        RideMetricFactory &factory = RideMetricFactory::instance();
        for (int j=0; j<10; j++) {
            QString symbol = QString("l%1_sustain").arg(j+1);
            const RideMetric *m = factory.rideMetric(symbol);

            metrics()[m->index()] = stiz[j];
        }
    }

    // tell the world we changed
    context->notifyIntervalsUpdate(this);

    // wipe them away now
    foreach(IntervalItem *x, deletelist) delete x;
}

QList<IntervalItem*> RideItem::intervalsSelected() const
{
    QList<IntervalItem*> returning;
    foreach(IntervalItem *p, intervals_) {
        if (p && p->selected) returning << p;
    }
    return returning;
}

QList<IntervalItem*> RideItem::intervalsSelected(RideFileInterval::intervaltype type) const
{
    QList<IntervalItem*> returning;
    foreach(IntervalItem *p, intervals_) {
        if (p && p->selected && p->type==type) returning << p;
    }
    return returning;
}

QList<IntervalItem*> RideItem::intervals(RideFileInterval::intervaltype type) const
{
    QList<IntervalItem*> returning;
    foreach(IntervalItem *p, intervals_) {
        if (p && p->type == type) returning << p;
    }
    return returning;
}

// search through the xdata and match against wildcards passed
// if found return true and set mname and mseries to what matched
// otherwise return false
bool
RideItem::xdataMatch(QString name, QString series, QString &mname, QString &mseries)
{
    QMapIterator<QString, QStringList>xi(xdata_);
    xi.toFront();
    while (xi.hasNext()) {
        xi.next();

        if (name == xi.key() || QDir::match(name, xi.key())) {

            // name matches
            foreach(QString s, xi.value()) {

                if (s == series || QDir::match(series, s)) {

                    // series matches too
                    mname = xi.key();
                    mseries = s;
                    return true;
                }
            }
        }
    }
    return false;
}

bool
RideItem::addImage(QString filename)
{
    // get list of images
    QStringList list=images();

    // filename should be full path since we need to copy into the media folder
    // we will rename it with a number at the front that cycles
    QFileInfo fi(filename);
    if (!fi.exists() || !fi.isReadable() || !fi.isFile()) return false;

    // lets generate a target filename
    bool isduplicate=true;
    QString targetname;
    for(int prefix=1; isduplicate; prefix++) {
        targetname=QString("%1-%2").arg(prefix).arg(fi.fileName());
        isduplicate = list.contains(targetname);
        if (!isduplicate) {
            // lets check it doesn't already exist on disk
            QFileInfo ti(context->athlete->home->media().canonicalPath() + "/" + targetname);
            isduplicate = ti.exists();
        }
    }

    // lets copy from source full path, to media folder
    if (QFile::copy(filename, QString("%1/%2").arg(context->athlete->home->media().canonicalPath()).arg(targetname))) {
        // success !
        list << targetname;
        ride_->setTag("Images", list.join("\n"));

        // make sure it gets saved !
        setDirty(true);
        // lets others know metadata has changed
        notifyRideMetadataChanged();
        return true;
    }
    return false;
}

bool
RideItem::removeImage(QString filename)
{
    // if it really exists then zap it!
    QFileInfo fi(context->athlete->home->media().canonicalPath() + "/" + filename);
    if (fi.exists() && fi.isReadable() && fi.isFile()) {

        QFile::remove(fi.absoluteFilePath());

        // remove from metadata
        QStringList list=images();
        int index= list.indexOf(filename);
        if (index != -1) list.removeAt(index);
        ride_->setTag("Images", list.join("\n"));

        // set dirty
        setDirty(true);

        return true;
    }
    return false;
}

// read from the metadata but also check they actually exist
QStringList
RideItem::images() const
{
    QStringList exist;
    foreach(QString filename, ride_->getTag("Images", "").split("\n")) {
        QFileInfo fi(context->athlete->home->media().canonicalPath() + "/" + filename);
        if (fi.exists() && fi.isReadable() && fi.isFile()) exist << filename;
    }

    return exist;
}

// we hide the implementation of directory here since we may change our
// minds later and store in sub-folders etc
QStringList RideItem::imagePaths() const
{
    QStringList paths;
    foreach(QString filename, images())
        paths << context->athlete->home->media().canonicalPath() + "/" + filename;

    return paths;
}

int
RideItem::importImages(QStringList files)
{
    int count=0;

    foreach(QString file, files) {
        if (addImage(file) == true) count++;
    }

    return count;
}
