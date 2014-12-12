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
