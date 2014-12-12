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

// Escape special characters (JSON compliance)
static QString protect(const QString string)
{
    QString s = string;
    s.replace("\\", "\\\\"); // backslash
    s.replace("\"", "\\\""); // quote
    s.replace("\t", "\\t");  // tab
    s.replace("\n", "\\n");  // newline
    s.replace("\r", "\\r");  // carriage-return
    s.replace("\b", "\\b");  // backspace
    s.replace("\f", "\\f");  // formfeed
    s.replace("/", "\\/");   // solidus

    // add a trailing space to avoid conflicting with GC special tokens
    s += " "; 

    return s;
}

// NOTE:
// We use a bison parser to reduce memory
// overhead and (believe it or not) simplicity
// RideCache::load() -- see RideDB.y

// save cache to disk, "cache/rideDB.json"
void RideCache::save()
{

    // now save data away
    QFile rideDB(QString("%1/rideDB.json").arg(context->athlete->home->cache().canonicalPath()));
    if (rideDB.open(QFile::WriteOnly)) {

        const RideMetricFactory &factory = RideMetricFactory::instance();

        // ok, lets write out the cache
        QTextStream stream(&rideDB);
        stream.setCodec("UTF-8");
        stream.setGenerateByteOrderMark(true);

        stream << "{" ;
        stream << "\n  \"VERSION\":\"1.0\",";
        stream << "\n  \"RIDES\":[\n";

        bool firstRide = true;
        foreach(RideItem *item, rides()) {

            // skip if not loaded/refreshed, a special case
            // if saving during an initial refresh
            if (item->metrics().count() == 0) continue;

            // comma separate each ride
            if (!firstRide) stream << ",\n";
            firstRide = false;

            // basic ride information
            stream << "\t{\n";
            stream << "\t\t\"filename\":\"" <<item->fileName <<"\",\n";
            stream << "\t\t\"date\":\"" <<item->dateTime.toUTC().toString(DATETIME_FORMAT) << "\",\n";
            stream << "\t\t\"fingerprint\":\"" <<item->fingerprint <<"\",\n";
            stream << "\t\t\"crc\":\"" <<item->crc <<"\",\n";
            stream << "\t\t\"timestamp\":\"" <<item->timestamp <<"\",\n";
            stream << "\t\t\"dbversion\":\"" <<item->dbversion <<"\",\n";
            stream << "\t\t\"color\":\"" <<item->color.name() <<"\",\n";
            stream << "\t\t\"present\":\"" <<item->present <<"\",\n";
            stream << "\t\t\"isRun\":\"" <<item->isRun <<"\",\n";
            stream << "\t\t\"weight\":\"" <<item->weight <<"\",\n";

            // pre-computed metrics
            stream << "\n\t\t\"METRICS\":{\n";

            bool firstMetric = true;
            for(int i=0; i<factory.metricCount(); i++) {
                QString name = factory.metricName(i);
                int index = factory.rideMetric(name)->index();

                if (!firstMetric) stream << ",\n";
                firstMetric = false;

                stream << "\t\t\t\"" << name << "\":\"" << QString("%1").arg(item->metrics()[index], 0, 'f', 5) <<"\"";
            }
            stream << "\n\t\t}";

            // pre-loaded metadata
            if (item->metadata().count()) {

                stream << ",\n\t\t\"TAGS\":{\n";

                QMap<QString,QString>::const_iterator i;
                for (i=item->metadata().constBegin(); i != item->metadata().constEnd(); i++) {

                    stream << "\t\t\t\"" << i.key() << "\":\"" << protect(i.value()) << "\"";
                    if (i+1 != item->metadata().constEnd()) stream << ",\n";
                    else stream << "\n";
                }

                // end of the tags
                stream << "\n\t\t}";

            }

            // end of the ride
            stream << "\n\t}";
        }

        stream << "\n  ]\n}";

        rideDB.close();
    }
}

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
        future = QtConcurrent::map(rides_, itemRefresh);
        watcher.setFuture(future);
    }
}

void
RideCache::refreshCPModelMetrics()
{
    // this needs to be done once all the other metrics
    // Calculate a *monthly* estimate of CP, W' etc using
    // bests data from the previous 3 months

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

    // run through each month with a rolling bests
    int year = from.year();
    int month = from.month();
    int lastYear = to.year();
    int lastMonth = to.month();
    int count = 0;

    // lets make sure we don't run wild when there is bad
    // ride dates etc -- ignore data before 1990 and after 
    // next year. This is belt and braces really
    if (year < 1990) year = 1990;
    if (lastYear > QDate::currentDate().year()+1) lastYear = QDate::currentDate().year()+1;

    QList< QVector<float> > months;
    QList< QVector<float> > monthsKG;

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


    // loop through
    while (year < lastYear || (year == lastYear && month <= lastMonth)) {

        QDate firstOfMonth = QDate(year, month, 01);
        QDate lastOfMonth = firstOfMonth.addMonths(1).addDays(-1);

        // let others know where we got to...
        emit modelProgress(year, month);

        // months is a rolling 3 months sets of bests
        QVector<float> wpk; // for getting the wpk values
        months << RideFileCache::meanMaxPowerFor(context, wpk, firstOfMonth, lastOfMonth);
        monthsKG << wpk;

        if (months.count() > 2) {
            months.removeFirst();
            monthsKG.removeFirst();
        }

        // create a rolling merge of all those months
        QVector<float> rollingBests = months[0];
        QVector<float> rollingBestsKG = monthsKG[0];

        switch(months.count()) {
            case 1 : // first time through we are done!
                break;

            case 2 : // second time through just apply month(1)
                {
                    // watts
                    if (months[1].size() > rollingBests.size()) rollingBests.resize(months[1].size());
                    for (int i=0; i<months[1].size(); i++)
                        if (months[1][i] > rollingBests[i]) rollingBests[i] = months[1][i];

                    // wattsKG
                    if (monthsKG[1].size() > rollingBestsKG.size()) rollingBestsKG.resize(monthsKG[1].size());
                    for (int i=0; i<monthsKG[1].size(); i++)
                        if (monthsKG[1][i] > rollingBestsKG[i]) rollingBestsKG[i] = monthsKG[1][i];
                }
                break;

            case 3 : // third time through resize to largest and compare to 1 and 2 XXX not used as limits to 2 month window
                {

                    // watts
                    if (months[1].size() > rollingBests.size()) rollingBests.resize(months[1].size());
                    if (months[2].size() > rollingBests.size()) rollingBests.resize(months[2].size());
                    for (int i=0; i<months[1].size(); i++) if (months[1][i] > rollingBests[i]) rollingBests[i] = months[1][i];
                    for (int i=0; i<months[2].size(); i++) if (months[2][i] > rollingBests[i]) rollingBests[i] = months[2][i];

                    // wattsKG
                    if (monthsKG[1].size() > rollingBestsKG.size()) rollingBestsKG.resize(monthsKG[1].size());
                    if (monthsKG[2].size() > rollingBestsKG.size()) rollingBestsKG.resize(monthsKG[2].size());
                    for (int i=0; i<monthsKG[1].size(); i++) if (monthsKG[1][i] > rollingBestsKG[i]) rollingBestsKG[i] = monthsKG[1][i];
                    for (int i=0; i<monthsKG[2].size(); i++) if (monthsKG[2][i] > rollingBestsKG[i]) rollingBestsKG[i] = monthsKG[2][i];
                }
        }

        // got some data lets rock
        if (rollingBests.size()) {

            // we now have the data
            foreach(PDModel *model, models) {

                PDEstimate add;

                // set the data
                model->setData(rollingBests);
                model->saveParameters(add.parameters); // save the computed parms

                add.wpk = false;
                add.from = firstOfMonth;
                add.to = lastOfMonth;
                add.model = model->code();
                add.WPrime = model->hasWPrime() ? model->WPrime() : 0;
                add.CP = model->hasCP() ? model->CP() : 0;
                add.PMax = model->hasPMax() ? model->PMax() : 0;
                add.FTP = model->hasFTP() ? model->FTP() : 0;

                if (add.CP && add.WPrime) add.EI = add.WPrime / add.CP ;

                // so long as the model derived values are sensible ...
                if (add.WPrime > 1000 && add.CP > 100 && add.PMax > 100 && add.FTP > 100)
                    context->athlete->PDEstimates << add;

                //qDebug()<<add.from<<model->code()<< "W'="<< model->WPrime() <<"CP="<< model->CP() <<"pMax="<<model->PMax();

                // set the wpk data
                model->setData(rollingBestsKG);
                model->saveParameters(add.parameters); // save the computed parms

                add.wpk = true;
                add.from = firstOfMonth;
                add.to = lastOfMonth;
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
        }

        // move onto the next month
        count++;
        if (month == 12) {
            year ++;
            month = 1;
        } else {
            month ++;
        }
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
