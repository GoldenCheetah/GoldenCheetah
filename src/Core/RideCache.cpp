/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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
#include "RideCache.h"

#include "Context.h"
#include "Athlete.h"
#include "RideFileCache.h"
#include "RideCacheModel.h"
#include "Specification.h"
#include "DataProcessor.h"
#include "Estimator.h"

#include "Route.h"

#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"

#include "ErgFile.h"

#include "JsonRideFile.h" // for DATETIME_FORMAT

#ifdef SLOW_REFRESH
#include "unistd.h"
#endif

// we initialise the global user metrics
#include "RideMetric.h"
#include "UserMetricSettings.h"
#include "UserMetricParser.h"
#include "SpecialFields.h"
#include <QXmlInputSource>
#include <QXmlSimpleReader>

// for sorting
bool rideCacheGreaterThan(const RideItem *a, const RideItem *b) { return a->dateTime > b->dateTime; }
bool rideCacheLessThan(const RideItem *a, const RideItem *b) { return a->dateTime < b->dateTime; }

class RideCacheLoader : public QThread
{
public:

    RideCacheLoader(RideCache *cache) : cache(cache) {}
    void run() { cache->load(); }

private:

        RideCache *cache;
};

RideCache::RideCache(Context *context) : context(context)
{
    directory = context->athlete->home->activities();
    plannedDirectory = context->athlete->home->planned();

    progress_ = 100;
    exiting = false;
    estimator = new Estimator(context);

    // initial load of user defined metrics - do once we have an initial context
    // but before we refresh or check metrics for the first time
    if (UserMetricSchemaVersion == 0) {

        QString metrics = QString("%1/../usermetrics.xml").arg(context->athlete->home->root().absolutePath());
        if (QFile(metrics).exists()) {

            QFile metricfile(metrics);
            QXmlInputSource source(&metricfile);
            QXmlSimpleReader xmlReader;
            UserMetricParser handler;

            xmlReader.setContentHandler(&handler);
            xmlReader.setErrorHandler(&handler);

            // parse and get return values
            xmlReader.parse(source);
            _userMetrics = handler.getSettings();
            UserMetric::addCompatibility(_userMetrics);

            // reset schema version
            UserMetricSchemaVersion = RideMetric::userMetricFingerprint(_userMetrics);

            // now add initial metrics
            foreach(UserMetricSettings m, _userMetrics) {
                RideMetricFactory::instance().addMetric(UserMetric(context, m));
            }
        }

        // reset special fields to take into account user metrics
        SpecialFields::getInstance().reloadFields();
    }

    // set the list
    // populate ride list
    RideItem *last = NULL;
    QStringListIterator i(RideFileFactory::instance().listRideFiles(directory));
    while (i.hasNext()) {
        QString name = i.next();
        QDateTime dt;
        if (RideFile::parseRideFileName(name, &dt)) {
            last = new RideItem(directory.canonicalPath(), name, dt, context, false);

            connect(last, SIGNAL(rideDataChanged()), this, SLOT(itemChanged()));
            connect(last, SIGNAL(rideMetadataChanged()), this, SLOT(itemChanged()));

            rides_ << last;
        }
    }

    // set the list
    // populate the planned ride list
    QStringListIterator j(RideFileFactory::instance().listRideFiles(plannedDirectory));
    while (j.hasNext()) {
        QString name = j.next();
        QDateTime dt;
        if (RideFile::parseRideFileName(name, &dt)) {
            last = new RideItem(plannedDirectory.canonicalPath(), name, dt, context, true);

            connect(last, SIGNAL(rideDataChanged()), this, SLOT(itemChanged()));
            connect(last, SIGNAL(rideMetadataChanged()), this, SLOT(itemChanged()));

            rides_ << last;
        }
    }

    // now sort it - we need to use find on it
    std::sort(rides_.begin(), rides_.end(), rideCacheLessThan);

    // load the store - will unstale once cache restored
    RideCacheLoader *rideCacheLoader = new RideCacheLoader(this);
    connect(rideCacheLoader, SIGNAL(finished()), this, SLOT(postLoad()));
    connect(rideCacheLoader, SIGNAL(finished()), this, SIGNAL(loadComplete()));
    rideCacheLoader->start();
}

void
RideCache::postLoad()
{
    // set model once we have the basics
    model_ = new RideCacheModel(context, this);

    // after the first ridecache refresh we set initial pd estimates
    first= true;
    connect(context, SIGNAL(refreshEnd()), this, SLOT(initEstimates()));

    // now refresh just in case.
    refresh();

    // do we have any stale items ?
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

}

struct comparerideitem { bool operator()(const RideItem *p1, const RideItem *p2) { return p1->dateTime < p2->dateTime; } };

int
RideCache::find(RideItem *dt)
{
    // use lower_bound to binary search
    QVector<RideItem*>::const_iterator i = std::lower_bound(rides_.begin(), rides_.end(), dt, comparerideitem());
    int index = i - rides_.begin();

    // did it find the right value?
    if (index < 0 || index >= rides_.count() || rides_.at(index)->dateTime != dt->dateTime) return -1;
    return index;
}

RideCache::~RideCache()
{
    exiting = true;

    // cancel any refresh that may be running
    cancel();

    // save to store
    save();
}

void
RideCache::garbageCollect()
{
    foreach(RideItem *item, delete_) {
        if (item) item->deleteLater();
    }
    delete_.clear();
}

void
RideCache::initEstimates()
{
    // kickoff first calculation
    if (first) {
        first = false;
        estimator->calculate();
    }
}

void
RideCache::configChanged(qint32 what)
{

    // if the wbal formula changed invalidate all cached values
    if (what & CONFIG_WBAL) {
        foreach(RideItem *item, rides()) {
            if (item->isOpen()) item->ride()->wstale=true;
        }
    }

    // if metadata changed then recompute diary text
    if (what & CONFIG_FIELDS) {
        foreach(RideItem *item, rides()) {
            item->metadata_.insert("Calendar Text", GlobalContext::context()->rideMetadata->calendarText(item));
        }
    }

    // if zones or weight has changed refresh metrics
    // will add more as they come
    qint32 want = CONFIG_ATHLETE | CONFIG_ZONES | CONFIG_NOTECOLOR | CONFIG_DISCOVERY | CONFIG_GENERAL | CONFIG_USERMETRICS;
    if (what & want) {

        // restart !
        cancel();
        refresh();
    }
}

void
RideCache::itemChanged()
{
    // one of our kids changed, they grow up so fast.
    // NOTE ONLY CONNECT THIS TO RIDEITEMS !!!
    // BECAUSE IT IS ASSUMED BELOW THE SENDER IS A RIDEITEM
    RideItem *item = static_cast<RideItem*>(QObject::sender());

    // the model is particularly interested in ANY item that changes
    emit itemChanged(item);

    // current ride changed is more relevant for the charts lets notify
    // them the ride they're showing has changed
    if (item == context->currentRideItem()) {

        context->notifyRideChanged(item);
    }
}

// add a new ride
void
RideCache::addRide(QString name, bool dosignal, bool select, bool useTempActivities, bool planned)
{
    RideItem *prior = context->ride;

    // ignore malformed names
    QDateTime dt;
    if (!RideFile::parseRideFileName(name, &dt)) return;

    // new ride item
    RideItem *last;
    if (useTempActivities)
       last = new RideItem(context->athlete->home->tmpActivities().canonicalPath(), name, dt, context, false);
    else if (planned)
       last = new RideItem(plannedDirectory.canonicalPath(), name, dt, context, planned);
    else
       last = new RideItem(directory.canonicalPath(), name, dt, context, planned);

    connect(last, SIGNAL(rideDataChanged()), this, SLOT(itemChanged()));
    connect(last, SIGNAL(rideMetadataChanged()), this, SLOT(itemChanged()));

    // now add to the list, or replace if already there
    bool added = false;
    for (int index=0; index < rides_.count(); index++) {
        if (rides_[index]->fileName == last->fileName) {
            rides_[index] = last;
            added = true;
            break;
        }
    }

    // add and sort, model needs to know !
    if (!added) {
        model_->beginReset();
        rides_ << last;
        std::sort(rides_.begin(), rides_.end(), rideCacheLessThan);
        model_->endReset();
    }

    // refresh metrics for *this ride only*
    last->refresh();

    if (dosignal) context->notifyRideAdded(last); // here so emitted BEFORE rideSelected is emitted!

    // free up memory from last one, which is no biggie when importing
    // a single ride, but means we don't exhaust memory when we import
    // hundreds/thousands of rides in a batch import.
    if (prior) prior->close();

    // notify everyone to select it
    if (select) {
        context->ride = last;
        context->notifyRideSelected(last);
    } else{
        // notify everyone to select the one we were already on
        context->notifyRideSelected(prior);
    }

    // model estimates (lazy refresh)
    estimator->refresh();
}

bool
RideCache::removeCurrentRide() {

    // if there is no current activity to delete then return
    if (context->ride == NULL) return false;

    // pass the current ride filename for deletion
    return removeRide(context->ride->fileName);
}

bool
RideCache::removeRide(const QString& filenameToDelete) {
  
    // if there is no file activity to delete then return
    if (filenameToDelete.isEmpty()) return false;

    RideItem* select = NULL; // ride to select once its gone
    RideItem* todelete = NULL;
    int index = 0; // index to wipe out

    // find the filenameToDelete in the list and if it happens to be the
    // the current ride then select another one immediately after it, but
    // if it is the last one on the list select the one before
    for (index = 0; index < rides_.count(); index++) {

        RideItem* rideI = rides_[index];

        if (rideI->fileName == filenameToDelete) {

            // bingo!
            todelete = rideI;

            // if the ride to be deleted happens to be the current ride, then select another
            if (context->ride == todelete) {
                if (rides_.count() - index > 1) select = rides_[index + 1];
                else if (index > 0) select = rides_[index - 1];
            }
            break;
        }
    }

    // WTAF!?
    if (!todelete) {
        qDebug()<<"ERROR: delete not found.";
        return false;
    }

    // If this activity is linked, unlink it first
    if (todelete->hasLinkedActivity()) {
        QString linkedFileName = todelete->getLinkedFileName();
        RideItem *linkedItem = getLinkedActivity(todelete);
        if (linkedItem) {
            linkedItem->clearLinkedFileName();
            QString error;
            saveActivity(linkedItem, error);
        }
    }

    // dataprocessor runs on "save" which is a short
    // hand for add, update, delete
    DataProcessorFactory::instance().autoProcess(todelete->ride(), "Save", "DELETE");

    // remove from the cache, before deleting it this is so
    // any aggregating functions no longer see it, when recalculating
    // during aride deleted operation
    // but model needs to know about this!
    model_->startRemove(index);
    rides_.remove(index, 1);
    delete_<<todelete;
    model_->endRemove(index);

    // delete the file by renaming it
    QFile file((todelete->planned ? plannedDirectory : directory).canonicalPath() + "/" + filenameToDelete);

    // purposefully don't remove the old ext so the user wouldn't have to figure out what the old file type was
    QString strNewName = filenameToDelete + ".bak";

    // in case there was an existing bak file, delete it
    // ignore errors since it probably isn't there.
    QFile::remove(context->athlete->home->fileBackup().canonicalPath() + "/" + strNewName);

    if (!file.rename(context->athlete->home->fileBackup().canonicalPath() + "/" + strNewName)) {
        QMessageBox::critical(NULL, "Rename Error", tr("Can't rename %1 to %2 in %3")
            .arg(filenameToDelete).arg(strNewName).arg(context->athlete->home->fileBackup().canonicalPath()));
    }

    // remove any other derived/additional files; notes, cpi etc (they can only exist in /cache )
    QStringList extras;
    extras << "notes" << "cpi" << "cpx";
    foreach (QString extension, extras) {

        QString deleteMe = QFileInfo(filenameToDelete).baseName() + "." + extension;
        QFile::remove(context->athlete->home->cache().canonicalPath() + "/" + deleteMe);
    }

    if (select) {

        // we don't want the whole delete, select next flicker
        context->mainWindow->setUpdatesEnabled(false);

        // select a different ride
        context->ride = select;

        // notify after removed from list
        context->notifyRideDeleted(todelete);

        // now we can update
        context->mainWindow->setUpdatesEnabled(true);
        QApplication::processEvents();

        // now select another ride
        context->notifyRideSelected(select);

    } else {
        // re-select the context ride (if it exists) when deleting a non current ride
        context->notifyRideSelected(context->ride);
    }

    // model estimates (lazy refresh)
    estimator->refresh();

    return true;
}

// NOTE:
// We use a bison parser to reduce memory
// overhead and (believe it or not) simplicity
// RideCache::load() and save() -- see RideDB.y

// export metrics to csv, for users to play with R, Matlab, Excel etc
void
RideCache::writeAsCSV(QString filename)
{
    const RideMetricFactory &factory = RideMetricFactory::instance();
    QVector<const RideMetric *> indexed(factory.metricCount());

    // get metrics indexed in same order as the array
    foreach(QString name, factory.allMetrics()) {

        const RideMetric *m = factory.rideMetric(name);
        indexed[m->index()] = m;
    }

    // open file.. truncate if exists already
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("Problem Saving Ride Cache"));
        msgBox.setInformativeText(tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return;
    };
    file.resize(0);
    QTextStream out(&file);

    // write headings
    out<<"date, time, filename";
    foreach(const RideMetric *m, indexed) {
        if (m->name().startsWith("BikeScore"))
            out <<", BikeScore";
        else
            out <<", " <<m->name();
    }
    out<<"\n";

    // write values
    foreach(RideItem *item, rides()) {

        // date, time, filename
        out << item->dateTime.date().toString(Qt::ISODate);
        out << "," << item->dateTime.time().toString("hh:mm:ss");
        out << "," << item->fileName;

        // values
        foreach(double value, item->metrics()) {
            out << "," << QString("%1").arg(value, 'f').simplified();
        }

        out<<"\n";
    }
    file.close();
}

int
RideCache::nextRefresh()
{
    int returning=-1;
    updateMutex.lock();

    if (updates < 0) {
        returning = -1; // force termination by returning -1
    } else if (updates < reverse_.count()) {
        returning = updates;
        updates++;
        progressing(returning);
    }
    updateMutex.unlock();
    return(returning);
}

void
RideCache::threadCompleted(RideCacheRefreshThread*thread)
{
    updateMutex.lock();
    refreshThreads.removeOne(thread);
    updateMutex.unlock();

    if (refreshThreads.count() == 0) {
        //fprintf(stderr,"refresh ended\n"); fflush(stderr);
        context->notifyRefreshEnd();
        garbageCollect();
        save();
    }
}

void
RideCache::progressing(int value)
{
    // we're working away, notfy everyone where we got
    progress_ = 100.0f * (double(value) / double(reverse_.count()));

    // Avoid GUI event queue overflow- update every for every decile
    if (reverse_.count() && (reverse_.count()/10) && (value == reverse_.count() || value % (reverse_.count()/10) == 1)) {
        QDate here = reverse_.at(value-1)->dateTime.date();
        context->notifyRefreshUpdate(here);
    }
}

// cancel the refresh map, we're about to exit !
void
RideCache::cancel()
{
    updateMutex.lock();
    QVector<RideCacheRefreshThread*>current = refreshThreads;
    updates=-1;
    updateMutex.unlock();

    // wait till threads are empty, but use our copy as the master
    // is going to be changing as threads terminate and we need to be
    // sure all our threads have stopped before returning.
    foreach(RideCacheRefreshThread *thread, current) {
        thread->wait();
    }
}

// check if we need to refresh the metrics then start the thread if needed
void
RideCache::refresh()
{
    // already on it !
    if (refreshThreads.count()) return;

    // how many need refreshing ?
    int staleCount = 0;

    foreach(RideItem *item, rides_) {
        // ok set stale so we refresh
        if (item->checkStale())
            staleCount++;
    }

    // start if there is work to do
    // and future watcher can notify of updates
    if (staleCount)  {

        reverse_ = rides_;
        std::sort(reverse_.begin(), reverse_.end(), rideCacheGreaterThan);
        //future = QtConcurrent::map(reverse_, itemRefresh);
        //watcher.setFuture(future);

        // calculate number of threads and work per thread
        int maxthreads = QThreadPool::globalInstance()->maxThreadCount();
        int threads = maxthreads / 2;
        if (threads==0) threads=1; // need at least one!
        int n=0;

        // refresh happenning
        updates = 0;
        context->notifyRefreshStart();

        while(n++ < threads) {

            // if goes past last make it the last
            RideCacheRefreshThread *thread = new RideCacheRefreshThread(this);
            refreshThreads << thread;
            thread->start();
        }


    } else {


        // wait five seconds, so mainwindow can get up and running...
        QTimer::singleShot(5000, context, SLOT(notifyRefreshEnd()));
    }
}

QString
RideCache::getAggregate(QString name, Specification spec, bool useMetricUnits, bool nofmt)
{
    // get the metric details, so we can convert etc
    const RideMetric *metric = RideMetricFactory::instance().rideMetric(name);
    if (!metric) {
        qDebug()<<"unknown metric:"<<name;
        return QString("%1 unknown").arg(name);
    }

    // what we will return
    double rvalue = 0;
    double rcount = 0; // using double to avoid rounding issues with int when dividing

    // loop through and aggregate
    foreach (RideItem *item, rides()) {

        // skip filtered rides
        if (!spec.pass(item)) continue;

        // get this value
        double value = item->getForSymbol(name);
        double count = item->getCountForSymbol(name); // for averaging

        // check values are bounded, just in case
        if (std::isnan(value) || std::isinf(value)) value = 0;

        // do we aggregate zero values ?
        bool aggZero = metric->aggregateZero();

        // set aggZero to false and value to zero if is temperature and -255
        if (metric->symbol() == "average_temp" && value == RideFile::NA) {
            value = 0;
            aggZero = false;
        }

        switch (metric->type()) {
        case RideMetric::RunningTotal:
        case RideMetric::Total:
            rvalue += value;
            break;
        default:
        case RideMetric::Average:
            {
            // average should be calculated taking into account
            // the duration of the ride, otherwise high value but
            // short rides will skew the overall average
            if (value || aggZero) {
                rvalue += value*count;
                rcount += count;
            }
            break;
            }
        case RideMetric::Low:
            {
            if (value < rvalue) rvalue = value;
            break;
            }
        case RideMetric::Peak:
            {
            if (value > rvalue) rvalue = value;
            break;
            }
        case RideMetric::MeanSquareRoot:
            {
                rvalue = sqrt((pow(rvalue, 2)*rcount + pow(value,2)*count)/(rcount + count));
                rcount += count;
                break;
            }
        }
    }

    // now compute the average
    if (metric->type() == RideMetric::Average) {
        if (rcount) rvalue = rvalue / rcount;
    }

    const_cast<RideMetric*>(metric)->setValue(rvalue);
    // Format appropriately
    QString result;
    if (metric->units(useMetricUnits) == "seconds" ||
        metric->units(useMetricUnits) == tr("seconds")) {
        if (nofmt) result = QString("%1").arg(rvalue);
        else result = metric->toString(useMetricUnits);

    } else result = metric->toString(useMetricUnits);

    // 0 temp from aggregate means no values
    if ((metric->symbol() == "average_temp" || metric->symbol() == "max_temp") && result == "0.0") result = "-";
    return result;
}

bool rideCachesummaryBestGreaterThan(const AthleteBest &s1, const AthleteBest &s2)
{
     return s1.nvalue > s2.nvalue;
}

bool rideCachesummaryBestLowerThan(const AthleteBest &s1, const AthleteBest &s2)
{
     return s1.nvalue < s2.nvalue;
}

QList<AthleteBest>
RideCache::getBests(QString symbol, int n, Specification specification, bool useMetricUnits)
{
    QList<AthleteBest> results;

    // get the metric details, so we can convert etc
    const RideMetric *metric = RideMetricFactory::instance().rideMetric(symbol);
    if (!metric) return results;

    // loop through and aggregate
    foreach (RideItem *ride, rides_) {

        // skip filtered rides
        if (!specification.pass(ride)) continue;

        // get this value
        AthleteBest add;
        add.nvalue = ride->getForSymbol(symbol, true);
        add.date = ride->dateTime.date();

        const_cast<RideMetric*>(metric)->setValue(add.nvalue);
        add.value = metric->toString(useMetricUnits);

        // nil values are not needed
        if (add.nvalue < 0 || add.nvalue > 0) results << add;
    }

    // now sort
    std::stable_sort(results.begin(), results.end(), metric->isLowerBetter() ?
                                                rideCachesummaryBestLowerThan :
                                                rideCachesummaryBestGreaterThan);

    // truncate
    if (results.count() > n) results.erase(results.begin()+n,results.end());

    // return the array with the right number of entries in #1 - n order
    return results;
}

QList<QDateTime>
RideCache::getAllDates()
{
    QList<QDateTime> returning;
    foreach(RideItem *item, rides()) {
        returning << item->dateTime;
    }
    return returning;
}

QStringList
RideCache::getAllFilenames()
{
    QStringList returning;
    foreach(RideItem *item, rides()) {
        returning << item->fileName;
    }
    return returning;
}

RideItem *
RideCache::getRide(QString filename)
{
    foreach(RideItem *item, rides())
        if (item->fileName == filename)
            return item;
    return NULL;
}


RideItem*
RideCache::getRide
(const QString &filename, bool planned)
{
    for (RideItem *rideItem : rides()) {
        if (rideItem != nullptr && rideItem->planned == planned && rideItem->fileName == filename) {
            return rideItem;
        }
    }
    return nullptr;
}


RideItem *
RideCache::getRide(QDateTime dateTime)
{
    foreach(RideItem *item, rides())
        if (item->dateTime == dateTime)
            return item;
    return NULL;
}



QHash<QString,int>
RideCache::getRankedValues(QString field)
{
    QHash<QString, int> returning;
    foreach(RideItem *item, rides()) {
        QString value = item->metadata().value(field, "");
        if (value != "") {
            int count = returning.value(value,0);
            returning.insert(value,++count);
        }
    }
    return returning;
}

class OrderedList {
    public:
        OrderedList(QString string, int rank) : string(string), rank(rank) {}
        QString string;
        int rank;
};

bool rideCacheOrderListGreaterThan(const OrderedList a, const OrderedList b) { return a.rank > b.rank; }

QStringList
RideCache::getDistinctValues(QString field)
{
    QStringList returning;

    // ranked
    QHashIterator<QString,int> i(getRankedValues(field));
    QList<OrderedList> ranked;
    while(i.hasNext()) {
        i.next();
        ranked << OrderedList(i.key(), i.value());
    }

    // sort from big to small
    std::sort(ranked.begin(), ranked.end(), rideCacheOrderListGreaterThan);

    // extract ordered values
    foreach(OrderedList x, ranked)
        returning << x.string;

    return returning;
}

void
RideCache::getRideTypeCounts(Specification specification, int& nActivities,
                             int& nRides, int& nRuns, int& nSwims, QString& sport)
{
    nActivities = nRides = nRuns = nSwims = 0;
    sport = "";

    // loop through and aggregate
    foreach (RideItem *ride, rides_) {

        // skip filtered rides
        if (!specification.pass(ride)) continue;

        // sport is not empty only when all activities are from the same sport
        if (nActivities == 0) sport = ride->sport;
        else if (sport != ride-> sport) sport = "";

        nActivities++;
        if (ride->isSwim) nSwims++;
        else if (ride->isRun) nRuns++;
        else if (ride->isBike) nRides++;
    }
}

bool
RideCache::isMetricRelevantForRides(Specification specification,
                                    const RideMetric* metric,
                                    SportRestriction sport)
{
    // loop through and aggregate
    foreach (RideItem *ride, rides_) {

        // skip filtered rides
        if (!specification.pass(ride)) continue;

        // skip non selected sports when restriction supplied
        if ((sport == OnlyRides) && !ride->isBike) continue;
        if ((sport == OnlyRuns) && !ride->isRun) continue;
        if ((sport == OnlySwims) && !ride->isSwim) continue;
        if ((sport == OnlyXtrains) && !ride->isXtrain) continue;

        if (metric->isRelevantForRide(ride)) return true;
    }

    return false;
}


RideCache::OperationPreCheck
RideCache::checkLinkActivities
(RideItem *item1, RideItem *item2)
{
    OperationPreCheck check;

    if (! isValidLink(item1, item2, check.blockingReason)) {
        check.canProceed = false;
        return check;
    }
    if (item1->hasLinkedActivity()) {
        check.canProceed = false;
        check.blockingReason = tr("%1 is already linked to %2").arg(item1->fileName).arg(item1->getLinkedFileName());
        return check;
    }
    if (item2->hasLinkedActivity()) {
        check.canProceed = false;
        check.blockingReason = tr("%1 is already linked to %2").arg(item2->fileName).arg(item2->getLinkedFileName());
        return check;
    }

    check.affectedItems << item1 << item2;
    if (item1->isDirty()) {
        check.dirtyItems << item1;
    }
    if (item2->isDirty()) {
        check.dirtyItems << item2;
    }
    if (! check.dirtyItems.isEmpty()) {
        check.requiresUserDecision = true;
        QStringList dirtyNames;
        for (RideItem *item : check.dirtyItems) {
            dirtyNames << item->fileName;
        }
        check.warningMessage = tr(
            "The following activities have unsaved changes:\n%1\n\n"
            "Linking will modify both activities. You must save or discard changes first.")
            .arg(dirtyNames.join("\n"));
    }

    return check;
}


RideCache::OperationResult
RideCache::linkActivities
(RideItem *item1, RideItem *item2)
{
    OperationResult result;

    item1->setLinkedFileName(item2->fileName);
    item2->setLinkedFileName(item1->fileName);

    result.success = true;
    result.affectedCount = 2;

    emit itemChanged(item1);
    emit itemChanged(item2);

    return result;
}


RideCache::OperationPreCheck
RideCache::checkUnlinkActivity
(RideItem *item)
{
    OperationPreCheck check;

    if (! item) {
        check.canProceed = false;
        check.blockingReason = tr("No activity given");
        return check;
    }
    QString linkedFileName = item->getLinkedFileName();
    if (linkedFileName.isEmpty()) {
        check.canProceed = false;
        check.blockingReason = tr("Activity is not linked");
        return check;
    }
    RideItem *linkedItem = getLinkedActivity(item);
    if (! linkedItem) {
        check.canProceed = false;
        check.blockingReason = tr("Linked activity not found: %1").arg(linkedFileName);
        return check;
    }

    check.affectedItems << item << linkedItem;
    if (item->isDirty()) {
        check.dirtyItems << item;
    }
    if (linkedItem->isDirty()) {
        check.dirtyItems << linkedItem;
    }
    if (! check.dirtyItems.isEmpty()) {
        check.requiresUserDecision = true;
        QStringList dirtyNames;
        for (RideItem *item : check.dirtyItems) {
            dirtyNames << item->fileName;
        }
        check.warningMessage = tr(
            "The following activities have unsaved changes:\n%1\n\n"
            "Unlinking will modify both activities. You must save or discard changes first.")
            .arg(dirtyNames.join("\n"));
    }

    return check;
}


RideCache::OperationResult
RideCache::unlinkActivity
(RideItem *item)
{
    OperationResult result;

    RideItem *linkedItem = getLinkedActivity(item);

    linkedItem->clearLinkedFileName();
    item->clearLinkedFileName();

    result.success = true;
    result.affectedCount = 2;

    emit itemChanged(item);
    emit itemChanged(linkedItem);

    return result;
}


RideCache::OperationPreCheck
RideCache::checkUnlinkActivities
(const QList<RideItem*> &items)
{
    OperationPreCheck batchCheck;

    if (items.isEmpty()) {
        batchCheck.canProceed = false;
        batchCheck.blockingReason = tr("No activities given");
        return batchCheck;
    }

    QSet<RideItem*> processedItems;
    for (RideItem *item : items) {
        if (! item || processedItems.contains(item)) {
            continue;
        }
        OperationPreCheck itemCheck = checkUnlinkActivity(item);
        if (! itemCheck.canProceed) {
            continue;
        }
        batchCheck.affectedItems.append(itemCheck.affectedItems);
        batchCheck.dirtyItems.append(itemCheck.dirtyItems);
        for (RideItem *affectedItem : itemCheck.affectedItems) {
            processedItems.insert(affectedItem);
        }
    }
    if (batchCheck.affectedItems.isEmpty()) {
        batchCheck.canProceed = false;
        batchCheck.blockingReason = tr("No valid linked activities to unlink");
        return batchCheck;
    }
    if (! batchCheck.dirtyItems.isEmpty()) {
        batchCheck.requiresUserDecision = true;
        QStringList dirtyNames;
        for (RideItem *item : batchCheck.dirtyItems) {
            dirtyNames << item->fileName;
        }
        batchCheck.warningMessage = tr(
            "The following activities have unsaved changes:\n%1\n\n"
            "Unlinking will modify these activities. You must save or discard changes first.")
            .arg(dirtyNames.join("\n"));
    }

    return batchCheck;
}


RideCache::OperationResult
RideCache::unlinkActivities
(const QList<RideItem*> &items)
{
    OperationResult batchResult;
    QSet<RideItem*> processedItems;

    for (RideItem *item : items) {
        if (! item || processedItems.contains(item)) {
            continue;
        }
        RideItem *linkedItem = getLinkedActivity(item);
        if (! linkedItem) {
            continue;
        }
        if (processedItems.contains(linkedItem)) {
            continue;
        }
        OperationResult itemResult = unlinkActivity(item);
        if (itemResult.success) {
            batchResult.affectedCount += itemResult.affectedCount;
            processedItems.insert(item);
            processedItems.insert(linkedItem);
        }
    }
    batchResult.success = (batchResult.affectedCount > 0);
    return batchResult;
}


RideCache::OperationPreCheck
RideCache::checkMoveActivity
(RideItem *item, const QDateTime &newDateTime)
{
    OperationPreCheck check;

    if (! item) {
        check.canProceed = false;
        check.blockingReason = tr("No activity given");
        return check;
    }
    if (! newDateTime.isValid()) {
        check.canProceed = false;
        check.blockingReason = tr("Invalid date/time specified");
        return check;
    }

    QFileInfo oldInfo(item->fileName);
    QString newFileName = newDateTime.toString("yyyy_MM_dd_HH_mm_ss") + "." + oldInfo.suffix();
    QString newPath = (item->planned ? plannedDirectory : directory).canonicalPath() + "/" + newFileName;
    if (QFile::exists(newPath)) {
        check.canProceed = false;
        check.blockingReason = tr("Target file already exists: %1").arg(newFileName);
        return check;
    }
    check.affectedItems << item;
    if (item->isDirty()) {
        check.dirtyItems << item;
    }

    RideItem *linkedItem = getLinkedActivity(item);
    if (linkedItem) {
        check.affectedItems << linkedItem;
        if (linkedItem->isDirty()) {
            check.dirtyItems << linkedItem;
        }
    }
    if (! check.dirtyItems.isEmpty()) {
        check.requiresUserDecision = true;
        QStringList dirtyNames;
        for (RideItem *dirtyItem : check.dirtyItems) {
            dirtyNames << dirtyItem->fileName;
        }
        check.warningMessage = tr(
            "The following activities have unsaved changes:\n%1\n\n"
            "Moving will update the link reference. You must save or discard changes first.")
            .arg(dirtyNames.join("\n"));
    }
    return check;
}


RideCache::OperationResult
RideCache::moveActivity
(RideItem *item, const QDateTime &newDateTime)
{
    OperationResult result;

    QString oldFileName = item->fileName;
    QDateTime oldDateTime = item->dateTime;

    QFileInfo oldInfo(oldFileName);
    QString newFileName = newDateTime.toString("yyyy_MM_dd_HH_mm_ss") + "." + oldInfo.suffix();

    RideFile *ride = item->ride(true);
    if (! ride) {
        result.error = tr("Failed to open activity file");
        return result;
    }

    item->setStartTime(newDateTime);
    ride->setTag("Year", newDateTime.toString("yyyy"));
    ride->setTag("Month", newDateTime.toString("MMMM"));
    ride->setTag("Weekday", newDateTime.toString("ddd"));
    item->metadata_.insert("Calendar Text", GlobalContext::context()->rideMetadata->calendarText(item));
    item->close();

    QString renameError;
    if (! renameRideFiles(oldFileName, newFileName, item->planned, renameError)) {
        item->dateTime = oldDateTime;
        item->fileName = oldFileName;
        result.error = tr("Failed to rename files: %1").arg(renameError);
        return result;
    }

    int index = rides_.indexOf(item);
    if (index >= 0) {
        model_->startRemove(index);
        rides_.remove(index, 1);
        model_->endRemove(index);
    }

    item->setFileName((item->planned ? plannedDirectory : directory).canonicalPath(), newFileName);

    model_->beginReset();
    rides_ << item;
    std::sort(rides_.begin(), rides_.end(), rideCacheLessThan);
    model_->endReset();

    item->isstale = true;

    RideItem *linkedItem = getLinkedActivity(item);
    if (linkedItem) {
        linkedItem->setLinkedFileName(newFileName);
        emit itemChanged(linkedItem);
        result.affectedCount = 2;
    } else {
        result.affectedCount = 1;
    }

    if (item->planned) {
        updateFromWorkout(item, false);
    }

    item->refresh();
    context->notifyRideChanged(item);
    if (context->ride == item) {
        context->notifyRideSelected(item);
    }
    estimator->refresh();

    result.success = true;

    return result;
}


RideCache::OperationPreCheck
RideCache::checkCopyPlannedActivity
(RideItem *sourceItem, const QDate &newDate)
{
    OperationPreCheck check;

    if (! sourceItem) {
        check.canProceed = false;
        check.blockingReason = tr("No activity given");
        return check;
    }
    if (! newDate.isValid()) {
        check.canProceed = false;
        check.blockingReason = tr("Invalid date specified");
        return check;
    }

    QDateTime newDateTime(newDate, sourceItem->dateTime.time());
    QFileInfo oldInfo(sourceItem->fileName);
    QString newFileName = newDateTime.toString("yyyy_MM_dd_HH_mm_ss") + "." + oldInfo.suffix();
    QString newPath = plannedDirectory.canonicalPath() + "/" + newFileName;
    if (QFile::exists(newPath)) {
        check.canProceed = false;
        check.blockingReason = tr("Target file already exists: %1").arg(newFileName);
        return check;
    }

    return check;
}


RideCache::OperationResult
RideCache::copyPlannedActivity
(RideItem *sourceItem, const QDate &newDate)
{
    OperationResult result;

    QString error;
    RideItem *newItem = copyPlannedRideFile(sourceItem, newDate, error);

    if (! newItem) {
        result.error = error;
        return result;
    }

    model_->beginReset();
    rides_ << newItem;
    std::sort(rides_.begin(), rides_.end(), rideCacheLessThan);
    model_->endReset();

    newItem->refresh();

    result.success = true;
    result.affectedCount = 1;

    return result;
}


RideCache::OperationPreCheck
RideCache::checkCopyPlannedActivities
(const QList<std::pair<RideItem*, QDate>> &sourceItemsAndTargets)
{
    OperationPreCheck check;

    if (sourceItemsAndTargets.isEmpty()) {
        check.canProceed = false;
        check.blockingReason = tr("No items specified");
        return check;
    }

    for (const std::pair<RideItem*, QDate> &pair : sourceItemsAndTargets) {
        RideItem *sourceItem = pair.first;
        QDate targetDate = pair.second;

        if (! sourceItem) {
            check.canProceed = false;
            check.blockingReason = tr("Invalid source item");
            return check;
        }
        if (! sourceItem->planned) {
            check.canProceed = false;
            check.blockingReason = tr("Source item is not a planned activity: %1").arg(sourceItem->fileName);
            return check;
        }
        if (! targetDate.isValid()) {
            check.canProceed = false;
            check.blockingReason = tr("Invalid target date for: %1").arg(sourceItem->fileName);
            return check;
        }

        QDateTime newDateTime(targetDate, sourceItem->dateTime.time());
        QFileInfo oldInfo(sourceItem->fileName);
        QString newFileName = newDateTime.toString("yyyy_MM_dd_HH_mm_ss") + "." + oldInfo.suffix();
        QString newPath = plannedDirectory.canonicalPath() + "/" + newFileName;

        if (QFile::exists(newPath)) {
            check.canProceed = false;
            check.blockingReason = tr("Target file already exists: %1").arg(newFileName);
            return check;
        }
    }

    return check;
}


RideCache::OperationResult
RideCache::copyPlannedActivities
(const QList<std::pair<RideItem*, QDate>> &sourceItemsAndTargets)
{
    OperationResult result;

    if (sourceItemsAndTargets.isEmpty()) {
        result.error = tr("No files specified");
        return result;
    }

    QList<RideItem*> newItems;
    QStringList failedFiles;
    for (const std::pair<RideItem*, QDate> &pair : sourceItemsAndTargets) {
        QString error;
        RideItem *newItem = copyPlannedRideFile(pair.first, pair.second, error);
        if (newItem) {
            newItems << newItem;
        } else {
            failedFiles << pair.first->fileName;
        }
    }

    if (! newItems.isEmpty()) {
        model_->beginReset();
        rides_ << newItems;
        std::sort(rides_.begin(), rides_.end(), rideCacheLessThan);
        model_->endReset();
        foreach(RideItem *item, newItems) {
            item->refresh();
        }
        refresh();
        estimator->refresh();
    }
    if (! failedFiles.isEmpty()) {
        result.error = tr("Failed to copy %1 of %2 activities: %3")
                         .arg(failedFiles.count())
                         .arg(sourceItemsAndTargets.count())
                         .arg(failedFiles.join(", "));
    }

    result.success = !newItems.isEmpty();
    result.affectedCount = newItems.count();

    return result;
}


RideCache::OperationPreCheck
RideCache::checkShiftPlannedActivities
(const QDate &fromDate, int dayOffset)
{
    OperationPreCheck check;

    if (! fromDate.isValid()) {
        check.canProceed = false;
        check.blockingReason = tr("Invalid from date specified");
        return check;
    }
    if (dayOffset == 0) {
        check.canProceed = true;
        return check;
    }

    QList<RideItem*> itemsToShift;
    for (RideItem *item : rides_) {
        if (item->planned && item->dateTime.date() >= fromDate) {
            itemsToShift.append(item);
            check.affectedItems << item;
        }
    }
    if (itemsToShift.isEmpty()) {
        check.canProceed = true;
        return check;
    }

    for (RideItem *item : itemsToShift) {
        RideItem *linkedItem = getLinkedActivity(item);
        if (linkedItem && ! linkedItem->planned) {
            check.affectedItems << linkedItem;
        }
    }
    for (RideItem *item : check.affectedItems) {
        if (item->isDirty()) {
            check.dirtyItems << item;
        }
    }

    if (! check.dirtyItems.isEmpty()) {
        check.requiresUserDecision = true;

        QStringList plannedDirty;
        QStringList actualDirty;
        for (RideItem *item : check.dirtyItems) {
            if (item->planned) {
                plannedDirty << item->fileName;
            } else {
                actualDirty << item->fileName;
            }
        }
        QString msg = tr("This operation will shift %1 planned activities.\n\n").arg(itemsToShift.count());
        if (! plannedDirty.isEmpty()) {
            msg += tr("Planned activities with unsaved changes:\n%1\n\n").arg(plannedDirty.join("\n"));
        }
        if (! actualDirty.isEmpty()) {
            msg += tr("Linked actual activities with unsaved changes:\n%1\n\n").arg(actualDirty.join("\n"));
        }

        msg += tr("All affected activities must be saved or changes discarded before shifting.");
        check.warningMessage = msg;
    }

    return check;
}


RideCache::OperationResult
RideCache::shiftPlannedActivities
(const QDate &fromDate, int dayOffset)
{
    OperationResult result;

    if (dayOffset == 0) {
        result.success = true;
        result.affectedCount = 0;
        return result;
    }
    QList<RideItem*> itemsToShift;
    for (RideItem *item : rides_) {
        if (item->planned && item->dateTime.date() >= fromDate) {
            itemsToShift.append(item);
        }
    }
    if (itemsToShift.isEmpty()) {
        result.success = true;
        result.affectedCount = 0;
        return result;
    }

    // prevent shifting any activity to before fromDate
    int effectiveOffset = dayOffset;
    if (dayOffset < 0) {
        QDate earliestDate = itemsToShift[0]->dateTime.date();
        for (RideItem *item : itemsToShift) {
            if (item->dateTime.date() < earliestDate) {
                earliestDate = item->dateTime.date();
            }
        }
        int maxBackwardShift = fromDate.daysTo(earliestDate);
        if (-dayOffset > maxBackwardShift) {
            effectiveOffset = -maxBackwardShift;
        }
        if (effectiveOffset == 0) {
            result.success = true;
            result.affectedCount = 0;
            return result;
        }
    }

    // avoid filename collisions: copy forward / backward, depending on offset
    if (effectiveOffset > 0) {
        std::sort(itemsToShift.begin(), itemsToShift.end(), [](RideItem *a, RideItem *b) { return a->dateTime > b->dateTime; });
    } else {
        std::sort(itemsToShift.begin(), itemsToShift.end(), [](RideItem *a, RideItem *b) { return a->dateTime < b->dateTime; });
    }

    QStringList failedFiles;
    int successCount = 0;
    for (RideItem *item : itemsToShift) {
        QString oldFileName = item->fileName;
        QDate newDate = item->dateTime.date().addDays(effectiveOffset);
        QDateTime newDateTime(newDate, item->dateTime.time());

        QFileInfo oldInfo(oldFileName);
        QString newFileName = newDateTime.toString("yyyy_MM_dd_HH_mm_ss") + "." + oldInfo.suffix();

        RideFile *ride = item->ride(true);
        if (! ride) {
            failedFiles << oldFileName;
            continue;
        }

        item->setStartTime(newDateTime);
        ride->setTag("Year", newDateTime.toString("yyyy"));
        ride->setTag("Month", newDateTime.toString("MMMM"));
        ride->setTag("Weekday", newDateTime.toString("ddd"));
        item->metadata_.insert("Calendar Text", GlobalContext::context()->rideMetadata->calendarText(item));
        item->close();

        QString renameError;
        if (! renameRideFiles(oldFileName, newFileName, true, renameError)) {
            failedFiles << oldFileName;
            continue;
        }
        item->setFileName(plannedDirectory.canonicalPath(), newFileName);
        updateFromWorkout(item, true);
        item->isstale = true;

        RideItem *linkedItem = getLinkedActivity(item);
        if (linkedItem) {
            linkedItem->setLinkedFileName(item->fileName);
            emit itemChanged(linkedItem);
        }

        successCount++;
    }

    if (successCount > 0) {
        model_->beginReset();
        std::sort(rides_.begin(), rides_.end(), rideCacheLessThan);
        model_->endReset();

        refresh();
        estimator->refresh();
    }

    if (! failedFiles.isEmpty()) {
        result.error = tr("Failed to shift %1 of %2 activities: %3")
                         .arg(failedFiles.count())
                         .arg(itemsToShift.count())
                         .arg(failedFiles.join(", "));
    }

    result.success = true;
    result.affectedCount = successCount;

    return result;
}


bool
RideCache::saveActivity
(RideItem *item, QString &error)
{
    error = "";
    if (! item) {
        error = tr("No activity given");
        return false;
    }
    if (item->isDirty()) {
        context->mainWindow->saveSilent(context, item);
        item->setDirty(false);
        emit itemSaved(item);
    }
    return true;
}


bool
RideCache::saveActivities
(QList<RideItem*> items, QString &error)
{
    QStringList failed;

    for (RideItem *item : items) {
        QString itemError;
        if (! saveActivity(item, itemError)) {
            failed << item->fileName;
        }
    }
    if (! failed.isEmpty()) {
        error = tr("Failed to save: %1").arg(failed.join(", "));
        return false;
    }

    return true;
}


bool
RideCache::renameRideFiles
(const QString &oldFileName, const QString &newFileName, bool isPlanned, QString &error)
{
    QFileInfo oldInfo(oldFileName);
    QFileInfo newInfo(newFileName);

    QDir activeDir = isPlanned ? plannedDirectory : directory;

    QString oldPath = activeDir.canonicalPath() + "/" + oldFileName;
    QString newPath = activeDir.canonicalPath() + "/" + newFileName;

    if (! QFile::rename(oldPath, newPath)) {
        error = tr("Failed to rename activity file from %1 to %2").arg(oldFileName).arg(newFileName);
        return false;
    }

    QStringList extensions;
    extensions << "notes" << "cpi" << "cpx";
    for (const QString &ext : extensions) {
        QString oldExtPath = context->athlete->home->cache().canonicalPath() + "/" + oldInfo.baseName() + "." + ext;
        QString newExtPath = context->athlete->home->cache().canonicalPath() + "/" + newInfo.baseName() + "." + ext;
        if (QFile::exists(oldExtPath)) {
            QFile::rename(oldExtPath, newExtPath);
        }
    }

    return true;
}


RideItem*
RideCache::getLinkedActivity
(RideItem *item)
{
    if (! item) {
        return nullptr;
    }
    QString linkedFileName = item->getLinkedFileName();
    if (linkedFileName.isEmpty()) {
        return nullptr;
    }
    return getRide(linkedFileName, ! item->planned);
}


RideItem*
RideCache::findSuggestion
(RideItem *rideItem)
{
    RideItem *closest = nullptr;
    for (RideItem *o: this->context->athlete->rideCache->rides()) {
        if (   o != nullptr
            && o->planned == ! rideItem->planned
            && o->dateTime.date() == rideItem->dateTime.date()
            && o->sport == rideItem->sport) {
            if (closest == nullptr) {
                closest = o;
            } else if (std::abs(rideItem->dateTime.time().secsTo(o->dateTime.time())) < std::abs(rideItem->dateTime.time().secsTo(closest->dateTime.time()))) {
                closest = o;
            }
        }
        if (o->dateTime.date() > rideItem->dateTime.date()) {
            break;
        }
    }
    return closest;
}


bool
RideCache::updateFromWorkout
(RideItem *item, bool autoSave)
{
    if (item == nullptr || ! item->planned) {
        return false;
    }
    QString workoutFilename = item->getText("WorkoutFilename", item->ride()->getTag("WorkoutFilename", "")).trimmed();
    if (workoutFilename.isEmpty()) {
        return false;
    }
    ErgFile ergFile(workoutFilename, ErgFileFormat::unknown, context, item->dateTime.date());
    if (! ergFile.hasRelativeWatts()) {
        return false;
    }
    bool changed = false;
    for (const QString &name : item->overrides_) {
        int value = static_cast<int>(item->getForSymbol(name));
        // Operate only on the values overridden by ManualActivityWizard
        if (name == "average_power") {
            if (value != std::round(ergFile.AP())) {
                QMap<QString, QString> values;
                values.insert("value", QString::number(std::round(ergFile.AP())));
                item->ride()->metricOverrides.insert(name, values);
                changed = true;
            }
        } else if (name == "coggan_np") {
            if (value != std::round(ergFile.IsoPower())) {
                QMap<QString, QString> values;
                values.insert("value", QString::number(std::round(ergFile.IsoPower())));
                item->ride()->metricOverrides.insert(name, values);
                changed = true;
            }
        } else if (name == "coggan_tss") {
            if (value != std::round(ergFile.bikeStress())) {
                QMap<QString, QString> values;
                values.insert("value", QString::number(std::round(ergFile.bikeStress())));
                item->ride()->metricOverrides.insert(name, values);
                changed = true;
            }
        } else if (name == "skiba_bike_score") {
            if (value != std::round(ergFile.BS())) {
                QMap<QString, QString> values;
                values.insert("value", QString::number(std::round(ergFile.BS())));
                item->ride()->metricOverrides.insert(name, values);
                changed = true;
            }
        } else if (name == "skiba_xpower") {
            if (value != std::round(ergFile.XP())) {
                QMap<QString, QString> values;
                values.insert("value", QString::number(std::round(ergFile.XP())));
                item->ride()->metricOverrides.insert(name, values);
                changed = true;
            }
        }
    }
    if (changed) {
        item->setDirty(true);
        item->isstale = true;
        if (autoSave) {
            QString error;
            saveActivity(item, error);
        }
    }
    return changed;
}


bool
RideCache::updateFromWorkoutAfter
(const QDate &when, bool autoSave)
{
    QList<RideItem*> changedItems;
    for (RideItem *item : context->athlete->rideCache->rides()) {
        if (item->planned && item->dateTime.date() >= when) {
            if (context->athlete->rideCache->updateFromWorkout(item, false)) {
                changedItems << item;
            }
        }
    }
    if (changedItems.count() > 0) {
        if (autoSave) {
            QString error;
            saveActivities(changedItems, error);
        }
        cancel();
        refresh();
        estimator->refresh();
    }
    return changedItems.count() > 0;
}


bool
RideCache::isValidLink
(RideItem *item1, RideItem *item2, QString &error)
{
    error = "";
    if (! item1 || ! item2) {
        error = tr("Invalid activities for linking");
        return false;
    }
    if (item1 == item2) {
        error = tr("Can't link to self");
        return false;
    }
    if (item1->planned == item2->planned) {
        error = tr("Cannot link two activities of the same type. One must be planned, one actual.");
        return false;
    }
    return true;
}


RideItem*
RideCache::copyPlannedRideFile
(RideItem *sourceItem, const QDate &newDate, QString &error)
{
    QDateTime newDateTime(newDate, sourceItem->dateTime.time());
    QFileInfo oldInfo(sourceItem->fileName);
    QString newFileName = newDateTime.toString("yyyy_MM_dd_HH_mm_ss") + "." + oldInfo.suffix();
    QString newPath = plannedDirectory.canonicalPath() + "/" + newFileName;
    QString sourcePath = plannedDirectory.canonicalPath() + "/" + sourceItem->fileName;

    if (! QFile::copy(sourcePath, newPath)) {
        error = tr("Failed to copy file");
        return nullptr;
    }

    QFile file(newPath);
    QStringList errors;
    RideFile *newRide = RideFileFactory::instance().openRideFile(context, file, errors);
    if (! newRide) {
        QFile::remove(newPath);
        error = tr("Failed to open copied file");
        return nullptr;
    }

    newRide->setStartTime(QDateTime(newDate, sourceItem->dateTime.time()));
    newRide->setTag("Year", newDateTime.toString("yyyy"));
    newRide->setTag("Month", newDateTime.toString("MMMM"));
    newRide->setTag("Weekday", newDateTime.toString("ddd"));

    if (! newRide->getTag("Linked Filename", "").isEmpty()) {
        newRide->removeTag("Linked Filename");
    }

    QFile outFile(newPath);
    if (! RideFileFactory::instance().writeRideFile(context, newRide, outFile, oldInfo.suffix())) {
        error = tr("Failed to write modified file");
        delete newRide;
        QFile::remove(newPath);
        return nullptr;
    }
    delete newRide;

    RideItem *newItem = new RideItem(plannedDirectory.canonicalPath(), newFileName, newDateTime, context, true);
    updateFromWorkout(newItem, true);
    newItem->isstale = true;

    return newItem;
}


// refresh metrics
void RideCacheRefreshThread::run()
{
    //fprintf(stderr, "worker thread starts!\n"); fflush(stderr);
    while (1) {

        int n = cache->nextRefresh();
        //fprintf(stderr, "refreshing %d of %d\n", n+1, cache->reverse_.count()); fflush(stderr);
        if (n<0) {
            //fprintf(stderr, "worker thread exits!\n"); fflush(stderr);
            goto exitthread;
        }

        // we have one to do
        RideItem *item = cache->reverse_[n];
        if(item->isstale) {
            item->refresh();
            if (item == item->context->currentRideItem())
                item->context->notifyRideChanged(item);
        }
    }

exitthread:
    cache->threadCompleted(this);
    return;
}
