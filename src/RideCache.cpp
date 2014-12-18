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

#include "DBAccess.h"

#include "Context.h"
#include "Athlete.h"
#include "RideFileCache.h"
#include "Specification.h"

#include "Route.h"
#include "RouteWindow.h"

#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"

#include "JsonRideFile.h" // for DATETIME_FORMAT

RideCache::RideCache(Context *context) : context(context)
{
    progress_ = 100;
    exiting = false;

    // get the new zone configuration fingerprint
    fingerprint = static_cast<unsigned long>(context->athlete->zones()->getFingerprint(context))
                  + static_cast<unsigned long>(context->athlete->paceZones()->getFingerprint())
                  + static_cast<unsigned long>(context->athlete->hrZones()->getFingerprint());

    // set the list
    // populate ride list
    RideItem *last = NULL;
    QStringListIterator i(RideFileFactory::instance().listRideFiles(context->athlete->home->activities()));
    while (i.hasNext()) {
        QString name = i.next();
        QDateTime dt;
        if (RideFile::parseRideFileName(name, &dt)) {
            last = new RideItem(context->athlete->home->activities().canonicalPath(), name, dt, context);
            rides_ << last;
        }
    }

    // load the store - will unstale once cache restored
    load();

    // now refresh just in case.
    refresh();

    // do we have any stale items ?
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));

    // future watching
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
RideCache::configChanged()
{
    // this is the overall config fingerprint, not for a specific
    // ride. We now refresh only when the zones change, and refresh
    // a single ride only when the zones that apply to that ride change
    unsigned long prior = fingerprint;

    // get the new zone configuration fingerprint
    fingerprint = static_cast<unsigned long>(context->athlete->zones()->getFingerprint(context))
                  + static_cast<unsigned long>(context->athlete->paceZones()->getFingerprint())
                  + static_cast<unsigned long>(context->athlete->hrZones()->getFingerprint());

    // if zones etc changed then recalculate metrics
    if (prior != fingerprint) refresh();
}

// add a new ride
void
RideCache::addRide(QString name, bool dosignal)
{
    // ignore malformed names
    QDateTime dt;
    if (!RideFile::parseRideFileName(name, &dt)) return;

    // new ride item
    RideItem *last = new RideItem(context->athlete->home->activities().canonicalPath(), name, dt, context);

    // now add to the list, or replace if already there
    bool added = false;
    for (int index=0; index < rides_.count(); index++) {
        if (rides_[index]->fileName == last->fileName) {
            rides_[index] = last;
            break;
        }
    }

    if (!added) rides_ << last;
    qSort(rides_); // sort by date

    // refresh metrics for *this ride only* 
    last->refresh();

    if (dosignal) context->notifyRideAdded(last); // here so emitted BEFORE rideSelected is emitted!

#ifdef GC_HAVE_INTERVALS
    //Search routes
    context->athlete->routes->searchRoutesInRide(last->ride());
#endif

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
    rides_.remove(index, 1);

    // delete the file by renaming it
    QString strOldFileName = context->ride->fileName;

    QFile file(context->athlete->home->activities().canonicalPath() + "/" + strOldFileName);
    // purposefully don't remove the old ext so the user wouldn't have to figure out what the old file type was
    QString strNewName = strOldFileName + ".bak";

    // in case there was an existing bak file, delete it
    // ignore errors since it probably isn't there.
    QFile::remove(context->athlete->home->fileBackup().canonicalPath() + "/" + strNewName);

    if (!file.rename(context->athlete->home->fileBackup().canonicalPath() + "/" + strNewName)) {
        QMessageBox::critical(NULL, "Rename Error", tr("Can't rename %1 to %2")
            .arg(strOldFileName).arg(strNewName));
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

    // notify AFTER deleted from DISK..
    context->notifyRideDeleted(todelete);

    // ..but before MEMORY cleared
    todelete->close();
    delete todelete;

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
    file.open(QFile::WriteOnly);
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
    if (item->isstale) item->refresh();
}

void
RideCache::progressing(int value)
{
    // we're working away, notfy everyone where we got
    progress_ = 100.0f * (double(value) / double(watcher.progressMaximum()));
    context->notifyRefreshUpdate();
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
        qSort(reverse_.begin(), reverse_.end(), qGreater<RideItem*>());
        future = QtConcurrent::map(reverse_, itemRefresh);
        watcher.setFuture(future);
    }
}

QString
RideCache::getAggregate(QString name, Specification spec, bool useMetricUnits, bool nofmt)
{
    // get the metric details, so we can convert etc
    const RideMetric *metric = RideMetricFactory::instance().rideMetric(name);
    if (!metric) return QString("%1 unknown").arg(name);

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
        if (isnan(value) || isinf(value)) value = 0;

        // imperial / metric conversion
        if (useMetricUnits == false) {
            value *= metric->conversion();
            value += metric->conversionSum();
        }

        // do we aggregate zero values ?
        bool aggZero = metric->aggregateZero();

        // set aggZero to false and value to zero if is temperature and -255
        if (metric->symbol() == "average_temp" && value == RideFile::NoTemp) {
            value = 0;
            aggZero = false;
        }

        switch (metric->type()) {
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

    // Format appropriately
    QString result;
    if (metric->units(useMetricUnits) == "seconds" ||
        metric->units(useMetricUnits) == tr("seconds")) {
        if (nofmt) result = QString("%1").arg(rvalue);
        else result = time_to_string(rvalue);

    } else result = QString("%1").arg(rvalue, 0, 'f', metric->precision());

    // 0 temp from aggregate means no values 
    if ((metric->symbol() == "average_temp" || metric->symbol() == "max_temp") && result == "0.0") result = "-";
    return result;
}

bool rideCachesummaryBestGreaterThan(const SummaryBest &s1, const SummaryBest &s2)
{
     return s1.nvalue > s2.nvalue;
}

QList<SummaryBest> 
RideCache::getBests(QString symbol, int n, Specification specification, bool useMetricUnits)
{
    QList<SummaryBest> results;

    // get the metric details, so we can convert etc
    const RideMetric *metric = RideMetricFactory::instance().rideMetric(symbol);
    if (!metric) return results;

    // loop through and aggregate
    foreach (RideItem *ride, rides_) {

        // skip filtered rides
        if (!specification.pass(ride)) continue;

        // get this value
        SummaryBest add;
        add.nvalue = ride->getForSymbol(symbol, true);
        add.date = ride->dateTime.date();

        const_cast<RideMetric*>(metric)->setValue(add.nvalue);
        add.value = metric->toString(useMetricUnits);

        // nil values are not needed
        if (add.nvalue < 0 || add.nvalue > 0) results << add;
    }

    // now sort
    qStableSort(results.begin(), results.end(), rideCachesummaryBestGreaterThan);

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
    // this needs to be done once all the other metrics
    // Calculate a *monthly* estimate of CP, W' etc using
    // bests data from the previous 12 weeks
    RollingBests bests(12);
    RollingBests bestsWPK(12);

    // clear any previous calculations
    context->athlete->PDEstimates.clear(); 

    // we do this by aggregating power data into bests
    // for each month, and having a rolling set of 3 aggregates
    // then aggregating those up into a rolling 3 month 'bests'
    // which we feed to the models to get the estimates for that
    // point in time based upon the available data
    QDate from, to;

    // what dates have any power data ?
    foreach(RideItem *item, rides()) {

        if (item->present.contains("P")) {

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
        context->athlete->PDEstimates << PDEstimate();
        return;
    }

    // set up the models we support
    CP2Model p2model(context);
    CP3Model p3model(context);
    MultiModel multimodel(context);
    ExtendedModel extmodel(context);

    QList <PDModel *> models;
    models << &p2model;
    models << &p3model;
    models << &multimodel;
    models << &extmodel;


    // run backwards stopping when date is at 1990 or first ride date with data
    QDate date = from.addDays(-84);
    while (date < to.addDays(7)) {

        QDate begin = date;
        QDate end = date.addDays(7);

        // let others know where we got to...
        emit modelProgress(date.year(), date.month());

        // months is a rolling 3 months sets of bests
        QVector<float> wpk; // for getting the wpk values
        bests.addBests(RideFileCache::meanMaxPowerFor(context, wpk, begin, end));
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

            // so long as the model derived values are sensible ...
            if (add.WPrime > 1000 && add.CP > 100 && add.PMax > 100 && add.FTP > 100)
                context->athlete->PDEstimates << add;

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
            if (add.WPrime > 100.0f && add.CP > 1.0f && add.PMax > 1.0f && add.FTP > 1.0f)
                context->athlete->PDEstimates << add;

            //qDebug()<<add.from<<model->code()<< "KG W'="<< model->WPrime() <<"CP="<< model->CP() <<"pMax="<<model->PMax();
        }

        // go back a week
        date = date.addDays(7);
    }

    // add a dummy entry if we have no estimates to stop constantly trying to refresh
    if (context->athlete->PDEstimates.count() == 0) {
        context->athlete->PDEstimates << PDEstimate();
    }

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
            returning.insert(value,count);
        }
    }    
    return returning;
}

QStringList
RideCache::getDistinctValues(QString field)
{
    QStringList returning;
    QHashIterator<QString,int> i(getRankedValues(field));
    while(i.hasNext()) {
        i.next();
        returning << i.key();
    }
    return returning;
}
