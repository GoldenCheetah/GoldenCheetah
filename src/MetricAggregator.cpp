/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#include "MainWindow.h"
#include "Athlete.h"
#include "Context.h"
#include "MetricAggregator.h"
#include "DBAccess.h"
#include "RideFile.h"
#include "RideFileCache.h"
#ifdef GC_HAVE_LUCENE
#include "Lucene.h"
#endif
#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"
#include "Settings.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "BestIntervalDialog.h"
#include "RideMetric.h"
#include "Route.h"
#include "TimeUtils.h"
#include <math.h>
#include <QtXml/QtXml>
#include <QProgressDialog>

#include "GProgressDialog.h"

MetricAggregator::MetricAggregator(Context *context) : QObject(context), context(context), first(true)
{
    colorEngine = new ColorEngine(context);
    dbaccess = new DBAccess(context);
    connect(context, SIGNAL(configChanged()), this, SLOT(update()));
    connect(context, SIGNAL(rideClean(RideItem*)), this, SLOT(update(void)));
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(addRide(RideItem*)));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(update(void)));
}

MetricAggregator::~MetricAggregator()
{
    delete colorEngine;
    delete dbaccess;
}

/*----------------------------------------------------------------------
 * Refresh the database -- only updates metrics when they are out
 *                         of date or missing altogether or where
 *                         the ride file no longer exists
 *----------------------------------------------------------------------*/

// used to store timestamp, file crc and schema fingerprint used in database
struct status { unsigned long timestamp, crc, fingerprint; };

// Refresh not up to date metrics
void MetricAggregator::refreshMetrics()
{
    refreshMetrics(QDateTime());
}
QStringList
MetricAggregator::allActivityFilenames()
{
    QStringList returning;

    // get a Hash map of statistic records and timestamps
    QSqlQuery query(dbaccess->connection());
    bool rc = query.exec("SELECT filename FROM metrics ORDER BY ride_date;");
    while (rc && query.next()) {
        QString filename = query.value(0).toString();
        returning << filename;
    }

    return returning;
}


// Refresh not up to date metrics and metrics after date
void MetricAggregator::refreshMetrics(QDateTime forceAfterThisDate)
{
    // only if we have established a connection to the database
    if (dbaccess == NULL || context->athlete->isclean==true) return;

    // first check db structure is still up to date
    // this is because metadata.xml may add new fields
    dbaccess->checkDBVersion();

    // Get a list of the ride files
    QRegExp rx = RideFileFactory::instance().rideFileRegExp();
    QStringList filenames = RideFileFactory::instance().listRideFiles(context->athlete->home->activities());
    QStringListIterator i(filenames);

    // get a Hash maps of statistic records and timestamps
    QSqlQuery query(dbaccess->connection());
    QHash <QString, status> dbStatus;
    bool rc = query.exec("SELECT filename, crc, timestamp, fingerprint FROM metrics ORDER BY ride_date;");
    while (rc && query.next()) {
        status add;
        QString filename = query.value(0).toString();
        add.crc = query.value(1).toInt();
        add.timestamp = query.value(2).toInt();
        add.fingerprint = query.value(3).toInt();
        dbStatus.insert(filename, add);
    }
    QHash <QString, status> dbIntervalStatus;
    rc = query.exec("SELECT type, groupName, filename, start, crc, timestamp, fingerprint FROM interval_metrics ORDER BY ride_date;");
    while (rc && query.next()) {
        status add;
        QString key = query.value(0).toString()+"|"+query.value(1).toString()+"|"+query.value(2).toString()+"|"+query.value(3).toString();
        add.crc = query.value(4).toInt();
        add.timestamp = query.value(5).toInt();
        add.fingerprint = query.value(6).toInt();
        dbIntervalStatus.insert(key, add);
    }

    // begin LUW -- byproduct of turning off sync (nosync)
    dbaccess->connection().transaction();

    // Delete statistics for non-existant ride files
    QHash<QString, status>::iterator d;
    for (d = dbStatus.begin(); d != dbStatus.end(); ++d) {
        if (QFile(context->athlete->home->activities().canonicalPath() + "/" + d.key()).exists() == false) {
            dbaccess->deleteRide(d.key());
            dbaccess->deleteIntervalsForRide(d.key());
#ifdef GC_HAVE_LUCENE
            context->athlete->lucene->deleteRide(d.key());
#endif
        }
    }


    unsigned long zoneFingerPrint = static_cast<unsigned long>(context->athlete->zones()->getFingerprint(context))
                                  + static_cast<unsigned long>(context->athlete->paceZones()->getFingerprint())
                                  + static_cast<unsigned long>(context->athlete->hrZones()->getFingerprint()); // checksum of *all* zone data (HR and Power)

    // update statistics for ride files which are out of date
    // showing a progress bar as we go
    QTime elapsed;
    elapsed.start();
    QString title = context->athlete->cyclist;
    GProgressDialog *bar = NULL;

    int processed=0;
    int updates=0;
    QApplication::processEvents(); // get that dialog up!

    // log of progress
    QFile log(context->athlete->home->logs().canonicalPath() + "/" + "metric.log");
    log.open(QIODevice::WriteOnly);
    log.resize(0);
    QTextStream out(&log);
    out << "METRIC REFRESH STARTS: " << QDateTime::currentDateTime().toString() + "\r\n";

    while (i.hasNext()) {
        QString name = i.next();
        QFile file(context->athlete->home->activities().canonicalPath() + "/" + name);

        // if it s missing or out of date then update it!
        status current = dbStatus.value(name);
        unsigned long dbTimeStamp = current.timestamp;
        unsigned long crc = current.crc;
        unsigned long fingerprint = current.fingerprint;

        RideFile *ride = NULL;
        bool updateForRide = false;

        processed++;

        // create the dialog if we need to show progress for long running uodate
        long elapsedtime = elapsed.elapsed();
        if ((!forceAfterThisDate.isNull() || first || elapsedtime > 6000) && bar == NULL) {
            bar = new GProgressDialog(title, 0, filenames.count(), context->mainWindow->init, context->mainWindow);
            bar->show(); // lets hide until elapsed time is > 6 seconds

            // lets make sure it goes to the center!
            QApplication::processEvents();
        }

        // update the dialog always after 6 seconds
        if (!forceAfterThisDate.isNull() || first || elapsedtime > 6000) {

            // update progress bar
            QString elapsedString = QString("%1:%2:%3").arg(elapsedtime/3600000,2)
                                                .arg((elapsedtime%3600000)/60000,2,10,QLatin1Char('0'))
                                                .arg((elapsedtime%60000)/1000,2,10,QLatin1Char('0'));
            QString title = tr("Update Statistics\nElapsed: %1\n\n%2").arg(elapsedString).arg(name);
            bar->setLabelText(title);
            bar->setValue(processed);
        }
        QApplication::processEvents();

        if (dbTimeStamp < QFileInfo(file).lastModified().toTime_t() ||
            zoneFingerPrint != fingerprint ||
            (!forceAfterThisDate.isNull() && name >= forceAfterThisDate.toString("yyyy_MM_dd_hh_mm_ss"))) {
            QStringList errors;

            // ooh we have one to update -- lets check the CRC in case
            // its actually unchanged since last time and the timestamps
            // have been mucked up by dropbox / file copying / backups etc

            // but still update if we're doing this because settings changed not the ride!
            QString fullPath =  QString(context->athlete->home->activities().absolutePath()) + "/" + name;
            if ((crc == 0 || crc != DBAccess::computeFileCRC(fullPath)) ||
                zoneFingerPrint != fingerprint ||
                (!forceAfterThisDate.isNull() && name >= forceAfterThisDate.toString("yyyy_MM_dd_hh_mm_ss"))) {

                updateForRide = true;
                // log
                out << "Opening ride: " << name << "\r\n";

                // read file and process it if we didn't already...
                if (ride == NULL) ride = RideFileFactory::instance().openRideFile(context, file, errors);

                out << "File open completed: " << name << "\r\n";

                if (ride != NULL) {

                    out << "Getting weight: " << name << "\r\n";
                    ride->getWeight();
                    out << "Updating statistics: " << name << "\r\n";
                    importRide(context->athlete->home->activities(), ride, name, zoneFingerPrint, (dbTimeStamp > 0));

                }
                updates++;
            }
        }

        QDateTime _rideStartTime;
        if (ride != NULL) {
            _rideStartTime = ride->startTime();
        } else {
            RideFile::parseRideFileName(name, &_rideStartTime);
        }

        //Search all Routes for this ride startdate or name
        Routes* routes = context->athlete->routes;
        if (routes->routes.count()>0) {

            for (int n=0;n<routes->routes.count();n++) {
                RouteSegment* route = &routes->routes[n];

                for (int j=0;j<route->getRides().count();j++) {
                    RouteRide _ride = route->getRides()[j];

                    QDateTime rideStartDate = route->getRides()[j].startTime;
                    QString rideSegmentName = route->getRides()[j].filename;

                    if (rideSegmentName == name  || rideStartDate == _rideStartTime) {
                        out << "Route " << route->getName() << " in " << name << "\r\n";
                        //qDebug() << "Route " << route->getName() << " in " << name;

                        bool updateForInterval = false;

                        // Verify interval in db
                        // type, groupName, filename, start,
                        QString key = QString("%1|%2|%3|%4").arg("Route").arg(route->getName()).arg(name).arg(_ride.start);

                        // if it s missing or out of date then update it!
                        status intCurrent = dbIntervalStatus.value(key);
                        unsigned long intDbTimeStamp = intCurrent.timestamp;
                        //unsigned long intCrc = intCurrent.crc;
                        unsigned long intFingerprint = intCurrent.fingerprint;

                        if (intDbTimeStamp < QFileInfo(file).lastModified().toTime_t() || zoneFingerPrint != intFingerprint) {
                            updateForInterval= true;
                        }


                        if (updateForRide || updateForInterval) {
                            //qDebug() << "updateForRide " << updateForRide << " updateForInterval " << updateForInterval;

                            // read file and process it if we didn't already...
                            QStringList errors;
                            if (ride == NULL) ride = RideFileFactory::instance().openRideFile(context, file, errors);

                            out << "Insert new Route interval\r\n";
                            //qDebug() << "Insert new Route interval";

                            IntervalItem* interval = new IntervalItem(ride, route->getName(), _ride.start, _ride.stop, 0, 0, j);
                            importInterval(interval, "Route", route->getName(), zoneFingerPrint, (intDbTimeStamp > 0));
                        }
                    }
                }
             }
        }

        // update cache (will check timestamps itself)
        // if ride wasn't opened it will do it itself
        // we only want to check so passing check=true
        // because we don't actually want the results now
        // it will also check the file CRC as well as timestamps
        RideFileCache updater(context, context->athlete->home->activities().canonicalPath() + "/" + name, ride, true);

        // free memory - if needed
        if (ride) delete ride;

        // for model run...
        // RideFileCache::meanMaxPowerFor(context, context->athlete->home.canonicalPath() + "/" + name);

        if (bar && bar->wasCanceled()) {
            out << "METRIC REFRESH CANCELLED\r\n";
            break;
        }
    }


    // end LUW -- now syncs DB
    out << "COMMIT: " << QDateTime::currentDateTime().toString() + "\r\n";
    dbaccess->connection().commit();

#ifdef GC_HAVE_LUCENE
#ifndef WIN32 // windows crashes here....
    out << "OPTIMISE: " << QDateTime::currentDateTime().toString() + "\r\n";
    context->athlete->lucene->optimise();
#endif
#endif
    context->athlete->isclean = true;

    refreshBestIntervals();

    // clear out the estimates if something changed!
    if (updates) context->athlete->PDEstimates.clear();

    // now zap the progress bar
    if (bar) delete bar;

    // stop logging
    out << "SIGNAL DATA CHANGED: " << QDateTime::currentDateTime().toString() + "\r\n";
    dataChanged(); // notify models/views

    out << "METRIC REFRESH ENDS: " << QDateTime::currentDateTime().toString() + "\r\n";
    log.close();

    first = false;
}

bool greaterThan(const SummaryBest &s1, const SummaryBest &s2)
{
     return s1.nvalue > s2.nvalue;
}

void
MetricAggregator::refreshBestIntervals()
{
    // TEST method not used and not finish...

    QString symbol = "20m_critical_power";
    int n = 10;

    // Remove old interval
    dbaccess->deleteIntervalsForTypeAndGroupName("Best", "Best 20min");

    // get all fields...
    QList<SummaryMetrics> allRides = context->athlete->metricDB->getAllMetricsFor(QDateTime(), QDateTime());

    QList<SummaryBest> bests;

    // get the metric details, so we can convert etc
    const RideMetric *metric = RideMetricFactory::instance().rideMetric(symbol);
    if (!metric) return;

    // loop through and aggregate
    foreach (SummaryMetrics rideMetrics, allRides) {

        // get this value
        SummaryBest add;
        add.nvalue = rideMetrics.getForSymbol(symbol);
        add.date = rideMetrics.getRideDate().date();
        add.fileName = rideMetrics.getFileName();

        // XXX this needs improving for all cases ... hack for now
        add.value = QString("%1").arg(add.nvalue, 0, 'f', metric->precision());

        // nil values are not needed
        if (add.nvalue < 0 || add.nvalue > 0) bests << add;
    }

    // now sort
    qStableSort(bests.begin(), bests.end(), greaterThan);

    // truncate
    if (bests.count() > n) bests.erase(bests.begin()+n,bests.end());


    QStringList filenames = RideFileFactory::instance().listRideFiles(context->athlete->home->activities());
    QStringListIterator i(filenames);


    int pos=1;
    foreach(SummaryBest best, bests) {
        /*qDebug() << QString("%1. %2W le %3")
                   .arg(pos)
                   .arg(best.value)
                   .arg(best.date.toString(tr("d MMM yyyy")));*/
        i.toFront();
        while (i.hasNext()) {
            QString name = i.next();


            if (name == best.fileName) {
                QFile file(context->athlete->home->activities().canonicalPath() + "/" + name);
                QStringList errors;
                RideFile *ride = RideFileFactory::instance().openRideFile(context, file, errors);

                QList<BestIntervalDialog::BestInterval> results;
                BestIntervalDialog::findBests(ride, 1200, 1, results);
                if (results.isEmpty()) return;
                const BestIntervalDialog::BestInterval &b = results.first();

                /*qDebug() << QString("Peak 20min %1 w (%2-%3)")
                            .arg((int) round(b.avg))
                            .arg(b.start)
                            .arg(b.stop);*/

                IntervalItem* interval = new IntervalItem(ride, "Best 20min", b.start, b.stop-ride->recIntSecs(), 0, 0, pos++);
                importInterval(interval, "Best", "Best 20min", 0l, false);

                i.toBack();
            }
        }
    }

    // END Best
}



/*----------------------------------------------------------------------
 * Calculate the metrics for a ride file using the metrics factory
 *----------------------------------------------------------------------*/

// add a ride (after import / download)
void MetricAggregator::addRide(RideItem*ride)
{
    if (ride && ride->ride()) {
        importRide(context->athlete->home->activities(), ride->ride(), ride->fileName, context->athlete->zones()->getFingerprint(context), true);
        RideFileCache updater(context, context->athlete->home->activities().canonicalPath() + "/" + ride->fileName, ride->ride(), true); // update cpx etc
        dataChanged(); // notify models/views
    }
}

void MetricAggregator::update() {
    context->athlete->isclean = false;
    refreshMetrics();
}

bool MetricAggregator::importRide(QDir path, RideFile *ride, QString fileName, unsigned long fingerprint, bool modify)
{
    SummaryMetrics summaryMetric;

    QRegExp rx = RideFileFactory::instance().rideFileRegExp();
    if (!rx.exactMatch(fileName)) {
        return false; // not a ridefile!
    }
    summaryMetric.setFileName(fileName);
    //assert(rx.numCaptures() == 7); -- it was an exact match of course there are 7
    QDate date(rx.cap(1).toInt(), rx.cap(2).toInt(),rx.cap(3).toInt());
    QTime time(rx.cap(4).toInt(), rx.cap(5).toInt(),rx.cap(6).toInt());
    QDateTime dateTime(date, time);

    summaryMetric.setRideDate(dateTime);
    summaryMetric.setId(ride->id());
    summaryMetric.setIsRun(ride->isRun());

    const RideMetricFactory &factory = RideMetricFactory::instance();
    QStringList metrics;

    for (int i = 0; i < factory.metricCount(); ++i)
        metrics << factory.metricName(i);

    // compute all the metrics
    QHash<QString, RideMetricPtr> computed = RideMetric::computeMetrics(context, ride, context->athlete->zones(), context->athlete->hrZones(), metrics);

    // get metrics into summaryMetric QMap
    for(int i = 0; i < factory.metricCount(); ++i) {
        // check for override
        summaryMetric.setForSymbol(factory.metricName(i), computed.value(factory.metricName(i))->value(true));
    }

    // what color will this ride be?
    QColor color = colorEngine->colorFor(ride->getTag(context->athlete->rideMetadata()->getColorField(), ""));

    dbaccess->importRide(&summaryMetric, ride, color, fingerprint, modify);
#ifdef GC_HAVE_LUCENE
    context->athlete->lucene->importRide(&summaryMetric, ride, color, fingerprint, modify);
#endif

    return true;
}

bool MetricAggregator::importInterval(IntervalItem *interval, QString type, QString group, unsigned long fingerprint, bool modify)
{
    SummaryMetrics summaryMetric;

    const RideFile* ride = interval->ride;

    RideFile subride(const_cast<RideFile*>(ride));
    int start = ride->timeIndex(interval->start);
    int end = ride->timeIndex(interval->stop);
    for (int i = start; i <= end; ++i) {
        const RideFilePoint *p = ride->dataPoints()[i];
        subride.appendPoint(p->secs, p->cad, p->hr, p->km, p->kph, p->nm,
                      p->watts, p->alt, p->lon, p->lat, p->headwind, p->slope, p->temp, p->lrbalance,
                      p->lte, p->rte, p->lps, p->rps, p->smo2, p->thb,
                      p->rvert, p->rcad, p->rcontact, 0);

        // derived data
        RideFilePoint *l = subride.dataPoints().last();
        l->np = p->np;
        l->xp = p->xp;
        l->apower = p->apower;
    }

    summaryMetric.setFileName(ride->getTag("Filename",""));
    summaryMetric.setRideDate(ride->startTime());
    summaryMetric.setId(ride->id());
    summaryMetric.setIsRun(ride->isRun());

    const RideMetricFactory &factory = RideMetricFactory::instance();
    QStringList metrics;

    for (int i = 0; i < factory.metricCount(); ++i)
        metrics << factory.metricName(i);

    // compute all the metrics
    QHash<QString, RideMetricPtr> computed = RideMetric::computeMetrics(context, &subride, context->athlete->zones(), context->athlete->hrZones(), metrics);

    // get metrics into summaryMetric QMap
    for(int i = 0; i < factory.metricCount(); ++i) {
        // check for override
        summaryMetric.setForSymbol(factory.metricName(i), computed.value(factory.metricName(i))->value(true));
    }

    // what color will this ride be?
    QColor color = colorEngine->colorFor(ride->getTag(context->athlete->rideMetadata()->getColorField(), ""));

    dbaccess->importInterval(&summaryMetric, interval, type, group, color, fingerprint, modify);

    return true;
}

void
MetricAggregator::importMeasure(SummaryMetrics *sm)
{
    dbaccess->importMeasure(sm);
}

/*----------------------------------------------------------------------
 * Query functions are wrappers around DBAccess functions
 *----------------------------------------------------------------------*/
void
MetricAggregator::writeAsCSV(QString filename)
{
    // write all metrics as a CSV file
    QList<SummaryMetrics> all = getAllMetricsFor(QDateTime(), QDateTime());

    // write headings
    if (!all.count()) return; // no dice

    // open file.. truncate if exists already
    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.resize(0);
    QTextStream out(&file);

    // write headings
    out<<"date, time, filename,";
    QMapIterator<QString, double>i(all[0].values());
    while (i.hasNext()) {
        i.next();
        out<<i.key()<<",";
    }
    out<<"\n";

    // write values
    foreach(SummaryMetrics x, all) {
        out<<x.getRideDate().date().toString("MM/dd/yy")<<","
           <<x.getRideDate().time().toString()<<","
           <<x.getFileName()<<",";

        QMapIterator<QString, double>i(x.values());
        while (i.hasNext()) {
            i.next();
            out<<i.value()<<",";
        }
        out<<"\n";
    }
    file.close();
}

QList<SummaryMetrics>
MetricAggregator::getAllMetricsFor(DateRange dr)
{
    return getAllMetricsFor(QDateTime(dr.from, QTime(0,0,0)), QDateTime(dr.to, QTime(23,59,59)));
}

QList<SummaryMetrics>
MetricAggregator::getAllMetricsFor(QDateTime start, QDateTime end)
{
    if (context->athlete->isclean == false) refreshMetrics(); // get them up-to-date

    QList<SummaryMetrics> empty;

    // only if we have established a connection to the database
    if (dbaccess == NULL) {
        qDebug()<<"lost db connection?";
        return empty;
    }

    // apparently using transactions for queries
    // can improve performance!
    dbaccess->connection().transaction();
    QList<SummaryMetrics> results = dbaccess->getAllMetricsFor(start, end);
    dbaccess->connection().commit();
    return results;
}

SummaryMetrics
MetricAggregator::getAllMetricsFor(QString filename)
{
    if (context->athlete->isclean == false) refreshMetrics(); // get them up-to-date

    SummaryMetrics results;
    QColor color; // ignored for now...

    // only if we have established a connection to the database
    if (dbaccess == NULL) {
        qDebug()<<"lost db connection?";
        return results;
    }

    // apparently using transactions for queries
    // can improve performance!
    dbaccess->connection().transaction();
    dbaccess->getRide(filename, results, color);
    dbaccess->connection().commit();
    return results;
}

QList<SummaryMetrics>
MetricAggregator::getAllMeasuresFor(DateRange dr)
{
    return getAllMeasuresFor(QDateTime(dr.from, QTime(0,0,0)), QDateTime(dr.to, QTime(23,59,59)));
}

QList<SummaryMetrics>
MetricAggregator::getAllMeasuresFor(QDateTime start, QDateTime end)
{
    QList<SummaryMetrics> empty;

    // only if we have established a connection to the database
    if (dbaccess == NULL) {
        qDebug()<<"lost db connection?";
        return empty;
    }
    return dbaccess->getAllMeasuresFor(start, end);
}

SummaryMetrics
MetricAggregator::getRideMetrics(QString filename)
{
    if (context->athlete->isclean == false) refreshMetrics(); // get them up-to-date

    SummaryMetrics empty;

    // only if we have established a connection to the database
    if (dbaccess == NULL) {
        qDebug()<<"lost db connection?";
        return empty;
    }
    return dbaccess->getRideMetrics(filename);
}

void
MetricAggregator::refreshCPModelMetrics(bool bg)
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
    QSqlQuery query(dbaccess->connection());
    bool rc = query.exec("SELECT ride_date FROM metrics WHERE present LIKE '%P%' ORDER BY ride_date;");
    bool first = true;
    while (rc && query.next()) {
        if (first) {
            from = query.value(0).toDate();
            if (from.year() >= 1990) first = false; // ignore daft dates
        } else {
            to = query.value(0).toDate();
        }
    }

    // if we don't have 2 rides or more then skip this but add a blank estimate
    if (to == QDate()) {
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

    // if we have a progress dialog lets update the bar to show
    // progress for the model parameters
    GProgressDialog *bar = NULL;
    if (!bg) {
        bar = new GProgressDialog(tr("Update Model Estimates"), 1, (lastYear*12 + lastMonth) - (year*12 + month), 
                                  context->mainWindow->init, context->mainWindow);
        bar->setValue(1);
        bar->show(); // lets hide until elapsed time is > 6 seconds
        QApplication::processEvents();
    }

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

        // show some progress
        if (!bg) bar->setValue(count);
    }

    if (!bg) delete bar;

    // add a dummy entry if we have no estimates to stop constantly trying to refresh
    if (context->athlete->PDEstimates.count() == 0) {
        context->athlete->PDEstimates << PDEstimate();
    }

    emit modelProgress(0, 0); // all done
}
