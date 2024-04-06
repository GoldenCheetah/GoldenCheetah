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

#include "JsonRideFile.h" // for DATETIME_FORMAT

#ifdef SLOW_REFRESH
#include "unistd.h"
#endif

// we initialise the global user metrics
#include "RideMetric.h"
#include "UserMetricSettings.h"
#include "UserMetricParser.h"
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
        GlobalContext::context()->specialFields = SpecialFields();
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
#if QT_VERSION < 0x060000
    out.setCodec("UTF-8"); // Metric names can be translated
#endif

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
