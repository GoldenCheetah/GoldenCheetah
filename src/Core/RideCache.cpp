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

RideCache::RideCache(Context *context) : context(context)
{
    directory = context->athlete->home->activities();
    plannedDirectory = context->athlete->home->planned();

    progress_ = 100;
    refreshingEstimates = false;
    exiting = false;

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

            // reset schema version
            UserMetricSchemaVersion = RideMetric::userMetricFingerprint(_userMetrics);

            // now add initial metrics
            foreach(UserMetricSettings m, _userMetrics) {
                RideMetricFactory::instance().addMetric(UserMetric(context, m));
            }
        }
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

    // load the store - will unstale once cache restored
    load();

    // now sort it
    qSort(rides_.begin(), rides_.end(), rideCacheLessThan);

    // set model once we have the basics
    model_ = new RideCacheModel(context, this);

    // now refresh just in case.
    refresh();

    // do we have any stale items ?
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // future watching
    connect(&watcher, SIGNAL(finished()), this, SLOT(garbageCollect()));
    connect(&watcher, SIGNAL(finished()), this, SLOT(save()));
    connect(&watcher, SIGNAL(finished()), context, SLOT(notifyRefreshEnd()));
    connect(&watcher, SIGNAL(started()), context, SLOT(notifyRefreshStart()));
    connect(&watcher, SIGNAL(progressValueChanged(int)), this, SLOT(progressing(int)));
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
            item->metadata_.insert("Calendar Text", context->athlete->rideMetadata()->calendarText(item));
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
RideCache::addRide(QString name, bool dosignal, bool useTempActivities, bool planned)
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
        qSort(rides_.begin(), rides_.end(), rideCacheLessThan);
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
    context->ride = last;
    context->notifyRideSelected(last);
}

void
RideCache::removeCurrentRide()
{
    if (context->ride == NULL) return;

    RideItem *select = NULL; // ride to select once its gone
    RideItem *todelete = context->ride;

    bool found = false;
    int index=0; // index to wipe out

    // find ours in the list and select the one
    // immediately after it, but if it is the last
    // one on the list select the one before
    for(index=0; index < rides_.count(); index++) {

       if (rides_[index]->fileName == context->ride->fileName) {

          // bingo!
          found = true;
          if (rides_.count()-index > 1) select = rides_[index+1];
          else if (index > 0) select = rides_[index-1];
          break;

       }
    }

    // WTAF!?
    if (!found) {
        qDebug()<<"ERROR: delete not found.";
        return;
    }

    // remove from the cache, before deleting it this is so
    // any aggregating functions no longer see it, when recalculating
    // during aride deleted operation
    // but model needs to know about this!
    model_->startRemove(index);
    rides_.remove(index, 1);
    delete_<<todelete;
    model_->endRemove(index);

    // delete the file by renaming it
    QString strOldFileName = context->ride->fileName;

    QFile file((context->ride->planned ? plannedDirectory : directory).canonicalPath() + "/" + strOldFileName);
    // purposefully don't remove the old ext so the user wouldn't have to figure out what the old file type was
    QString strNewName = strOldFileName + ".bak";

    // in case there was an existing bak file, delete it
    // ignore errors since it probably isn't there.
    QFile::remove(context->athlete->home->fileBackup().canonicalPath() + "/" + strNewName);

    if (!file.rename(context->athlete->home->fileBackup().canonicalPath() + "/" + strNewName)) {
        QMessageBox::critical(NULL, "Rename Error", tr("Can't rename %1 to %2 in %3")
            .arg(strOldFileName).arg(strNewName).arg(context->athlete->home->fileBackup().canonicalPath()));
    }

    // remove any other derived/additional files; notes, cpi etc (they can only exist in /cache )
    QStringList extras;
    extras << "notes" << "cpi" << "cpx";
    foreach (QString extension, extras) {

        QString deleteMe = QFileInfo(strOldFileName).baseName() + "." + extension;
        QFile::remove(context->athlete->home->cache().canonicalPath() + "/" + deleteMe);

    }

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
        out << item->dateTime.date().toString("MM/dd/yy");
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

void
itemRefresh(RideItem *&item)
{
    // need parser to be reentrant !item->refresh();
    if (item->isstale) {
        item->refresh();

        // and trap changes during refresh to current ride
        if (item == item->context->currentRideItem())
            item->context->notifyRideChanged(item);

#ifdef SLOW_REFRESH
        sleep(1);
#endif
    }
}

void
RideCache::progressing(int value)
{
    // we're working away, notfy everyone where we got
    progress_ = 100.0f * (double(value) / double(watcher.progressMaximum()));
    if (value) {
        QDate here = reverse_.at(value-1)->dateTime.date();
        context->notifyRefreshUpdate(here);
    }
}

// cancel the refresh map, we're about to exit !
void
RideCache::cancel()
{
    if (future.isRunning()) {
        future.cancel();
        future.waitForFinished();
    }
}

// check if we need to refresh the metrics then start the thread if needed
void
RideCache::refresh()
{
    // already on it !
    if (future.isRunning()) return;

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
        qSort(reverse_.begin(), reverse_.end(), rideCacheGreaterThan);
        future = QtConcurrent::map(reverse_, itemRefresh);
        watcher.setFuture(future);
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
        double count = item->getForSymbol("workout_time"); // for averaging

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
    qStableSort(results.begin(), results.end(), metric->isLowerBetter() ?
                                                rideCachesummaryBestLowerThan :
                                                rideCachesummaryBestGreaterThan);

    // truncate
    if (results.count() > n) results.erase(results.begin()+n,results.end());

    // return the array with the right number of entries in #1 - n order
    return results;
}

class RollingBests {
    private:

        // buffer of best values; Watts or Watts/KG
        // is a double to handle both use cases
        QVector<QVector<float> > buffer;

        // current location in circular buffer
        int index;

    public:

        // iniitalise with circular buffer size
        RollingBests(int size) {
            index=1;
            buffer.resize(size);
        }

        // add a new weeks worth of data, losing
        // whatever is at the back of the buffer
        void addBests(QVector<float> array) {
            buffer[index++] = array;
            if (index >= buffer.count()) index=0;
        }

        // get an aggregate of all the bests
        // currently in the circular buffer
        QVector<float> aggregate() {

            QVector<float> returning;

            // set return buffer size
            int size=0;
            for(int i=0; i<buffer.count(); i++)
                if (buffer[i].size() > size)
                    size = buffer[i].size();

            // initialise return values
            returning.fill(0.0f, size);

            // get largest values
            for(int i=0; i<buffer.count(); i++) 
                for (int j=0; j<buffer[i].count(); j++)
                    if(buffer[i].at(j) > returning[j])
                        returning[j] = buffer[i].at(j);

            // return the aggregate
            return returning;
        }
};

void
RideCache::refreshCPModelMetrics()
{
    // we're refreshing, so away
    if (refreshingEstimates == true) return;

    // need to lock
    refreshingEstimates = true;
    context->athlete->lock.lock();

    // this needs to be done once all the other metrics
    // Calculate a *monthly* estimate of CP, W' etc using
    // bests data from the previous 12 weeks
    RollingBests bests(12);
    RollingBests bestsWPK(12);

    // clear any previous calculations
    context->athlete->PDEstimates_.clear();

    // we do this by aggregating power data into bests
    // for each month, and having a rolling set of 3 aggregates
    // then aggregating those up into a rolling 3 month 'bests'
    // which we feed to the models to get the estimates for that
    // point in time based upon the available data
    QDate from, to;

    // what dates have any power data ?
    foreach(RideItem *item, rides()) {

        // has power, but not running
        if (item->present.contains("P") && !item->isRun) {

            // no date set
            if (from == QDate()) from = item->dateTime.date();
            if (to == QDate()) to = item->dateTime.date();

            // later...
            if (item->dateTime.date() < from) from = item->dateTime.date();

            // earlier...
            if (item->dateTime.date() > to) to = item->dateTime.date();
        }
    }

    // if we don't have 2 rides or more then skip this but add a blank estimate
    if (from == to || to == QDate()) {
        context->athlete->PDEstimates_ << PDEstimate();
        context->athlete->lock.unlock();
        refreshingEstimates = false;
        return;
    }

    // set up the models we support
    CP2Model p2model(context);
    CP3Model p3model(context);
    WSModel wsmodel(context);
    MultiModel multimodel(context);
    ExtendedModel extmodel(context);

    QList <PDModel *> models;
    models << &p2model;
    models << &p3model;
    models << &multimodel;
    models << &extmodel;
    models << &wsmodel;


    // from has first ride with Power data / looking at the next 7 days of data with Power
    // calculate Estimates for all data per week including the week of the last Power recording
    QDate date = from;
    while (date < to) {

        QDate begin = date;
        QDate end = date.addDays(6);

        // let others know where we got to...
        emit modelProgress(date.year(), date.month());

        // months is a rolling 3 months sets of bests
        QVector<float> wpk; // for getting the wpk values

        // don't include RUNS ..................................................vvvvv
        bests.addBests(RideFileCache::meanMaxPowerFor(context, wpk, begin, end, false));
        bestsWPK.addBests(wpk);

        // we now have the data
        foreach(PDModel *model, models) {

            PDEstimate add;

            // set the data
            model->setData(bests.aggregate());
            model->saveParameters(add.parameters); // save the computed parms

            add.wpk = false;
            add.from = begin;
            add.to = end;
            add.model = model->code();
            add.WPrime = model->hasWPrime() ? model->WPrime() : 0;
            add.CP = model->hasCP() ? model->CP() : 0;
            add.PMax = model->hasPMax() ? model->PMax() : 0;
            add.FTP = model->hasFTP() ? model->FTP() : 0;

            if (add.CP && add.WPrime) add.EI = add.WPrime / add.CP ;

            // so long as the important model derived values are sensible ...
            if (add.WPrime > 1000 && add.CP > 100) 
                context->athlete->PDEstimates_ << add;

            //qDebug()<<add.to<<add.from<<model->code()<< "W'="<< model->WPrime() <<"CP="<< model->CP() <<"pMax="<<model->PMax();

            // set the wpk data
            model->setData(bestsWPK.aggregate());
            model->saveParameters(add.parameters); // save the computed parms

            add.wpk = true;
            add.from = begin;
            add.to = end;
            add.model = model->code();
            add.WPrime = model->hasWPrime() ? model->WPrime() : 0;
            add.CP = model->hasCP() ? model->CP() : 0;
            add.PMax = model->hasPMax() ? model->PMax() : 0;
            add.FTP = model->hasFTP() ? model->FTP() : 0;
            if (add.CP && add.WPrime) add.EI = add.WPrime / add.CP ;

            // so long as the model derived values are sensible ...
            if ((!model->hasWPrime() || add.WPrime > 10.0f) && 
                (!model->hasCP() || add.CP > 1.0f) &&
                (!model->hasPMax() || add.PMax > 1.0f) &&
                (!model->hasFTP() || add.FTP > 1.0f))
                context->athlete->PDEstimates_ << add;

            //qDebug()<<add.from<<model->code()<< "KG W'="<< model->WPrime() <<"CP="<< model->CP() <<"pMax="<<model->PMax();
        }

        // go forward a week
        date = date.addDays(7);
    }

    // add a dummy entry if we have no estimates to stop constantly trying to refresh
    if (context->athlete->PDEstimates_.count() == 0) {
        context->athlete->PDEstimates_ << PDEstimate();
    }

    // unlock
    context->athlete->lock.unlock();
    refreshingEstimates = false;

    emit modelProgress(0, 0); // all done
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
    qSort(ranked.begin(), ranked.end(), rideCacheOrderListGreaterThan);

    // extract ordered values
    foreach(OrderedList x, ranked)
        returning << x.string;

    return returning;
}

void
RideCache::getRideTypeCounts(Specification specification, int& nActivities,
                             int& nRides, int& nRuns, int& nSwims)
{
    nActivities = nRides = nRuns = nSwims = 0;

    // loop through and aggregate
    foreach (RideItem *ride, rides_) {

        // skip filtered rides
        if (!specification.pass(ride)) continue;

        nActivities++;
        if (ride->isSwim) nSwims++;
        else if (ride->isRun) nRuns++;
        else nRides++;
    }
}
