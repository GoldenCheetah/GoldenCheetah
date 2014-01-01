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
#include "Settings.h"
#include "RideItem.h"
#include "RideMetric.h"
#include "TimeUtils.h"
#include <math.h>
#include <QtXml/QtXml>
#include <QProgressDialog>

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

// used to store timestamp and fingerprint used in database
struct status { unsigned long timestamp, fingerprint; };

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
    QStringList filenames = RideFileFactory::instance().listRideFiles(context->athlete->home);
    QStringListIterator i(filenames);

    // get a Hash map of statistic records and timestamps
    QSqlQuery query(dbaccess->connection());
    QHash <QString, status> dbStatus;
    bool rc = query.exec("SELECT filename, timestamp, fingerprint FROM metrics ORDER BY ride_date;");
    while (rc && query.next()) {
        status add;
        QString filename = query.value(0).toString();
        add.timestamp = query.value(1).toInt();
        add.fingerprint = query.value(2).toInt();
        dbStatus.insert(filename, add);
    }

    // begin LUW -- byproduct of turning off sync (nosync)
    dbaccess->connection().transaction();

    // Delete statistics for non-existant ride files
    QHash<QString, status>::iterator d;
    for (d = dbStatus.begin(); d != dbStatus.end(); ++d) {
        if (QFile(context->athlete->home.absolutePath() + "/" + d.key()).exists() == false) {
            dbaccess->deleteRide(d.key());
#ifdef GC_HAVE_LUCENE
            context->athlete->lucene->deleteRide(d.key());
#endif
        }
    }

    unsigned long zoneFingerPrint = static_cast<unsigned long>(context->athlete->zones()->getFingerprint())
                                  + static_cast<unsigned long>(context->athlete->hrZones()->getFingerprint()); // checksum of *all* zone data (HR and Power)

    // update statistics for ride files which are out of date
    // showing a progress bar as we go
    QTime elapsed;
    elapsed.start();
    QString title = tr("Updating Statistics\nStarted");
    QProgressDialog *bar = NULL;

    int processed=0;
    QApplication::processEvents(); // get that dialog up!

    // log of progress
    QFile log(context->athlete->home.absolutePath() + "/" + "metric.log");
    log.open(QIODevice::WriteOnly);
    log.resize(0);
    QTextStream out(&log);
    out << "METRIC REFRESH STARTS: " << QDateTime::currentDateTime().toString() + "\r\n";

    while (i.hasNext()) {
        QString name = i.next();
        QFile file(context->athlete->home.absolutePath() + "/" + name);

        // if it s missing or out of date then update it!
        status current = dbStatus.value(name);
        unsigned long dbTimeStamp = current.timestamp;
        unsigned long fingerprint = current.fingerprint;

        RideFile *ride = NULL;

        processed++;

        // create the dialog if we need to show progress for long running uodate
        long elapsedtime = elapsed.elapsed();
        if ((first || elapsedtime > 6000) && bar == NULL) {
            bar = new QProgressDialog(title, tr("Abort"), 0, filenames.count()); // not owned by mainwindow
            bar->setWindowFlags(bar->windowFlags() | Qt::FramelessWindowHint);
            bar->setWindowModality(Qt::WindowModal);
            bar->setMinimumDuration(0);
            bar->show(); // lets hide until elapsed time is > 6 seconds
        }

        // update the dialog always after 6 seconds
        if (first || elapsedtime > 6000) {

            // update progress bar
            QString elapsedString = QString("%1:%2:%3").arg(elapsedtime/3600000,2)
                                                .arg((elapsedtime%3600000)/60000,2,10,QLatin1Char('0'))
                                                .arg((elapsedtime%60000)/1000,2,10,QLatin1Char('0'));
            QString title = tr("%1\n\nUpdate Statistics\nElapsed: %2\n\n%3").arg(context->athlete->cyclist).arg(elapsedString).arg(name);
            bar->setLabelText(title);
            bar->setValue(processed);
        }
        QApplication::processEvents();

        if (dbTimeStamp < QFileInfo(file).lastModified().toTime_t() ||
            zoneFingerPrint != fingerprint ||
            (!forceAfterThisDate.isNull() && name >= forceAfterThisDate.toString("yyyy_MM_dd_hh_mm_ss"))) {
            QStringList errors;

            // log
            out << "Opening ride: " << name << "\r\n";

            // read file and process it if we didn't already...
            if (ride == NULL) ride = RideFileFactory::instance().openRideFile(context, file, errors);

            out << "File open completed: " << name << "\r\n";

            if (ride != NULL) {

                out << "Getting weight: " << name << "\r\n";
                ride->getWeight();
                out << "Updating statistics: " << name << "\r\n";
                importRide(context->athlete->home, ride, name, zoneFingerPrint, (dbTimeStamp > 0));

            }
        }

        // update cache (will check timestamps itself)
        // if ride wasn't opened it will do it itself
        // we only want to check so passing check=true
        // because we don't actually want the results now
        RideFileCache updater(context, context->athlete->home.absolutePath() + "/" + name, ride, true);

        // free memory - if needed
        if (ride) delete ride;

        if (bar && bar->wasCanceled()) {
            out << "METRIC REFRESH CANCELLED\r\n";
            break;
        }
    }

    // now zap the progress bar
    if (bar) delete bar;

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

    // stop logging
    out << "SIGNAL DATA CHANGED: " << QDateTime::currentDateTime().toString() + "\r\n";
    dataChanged(); // notify models/views

    out << "METRIC REFRESH ENDS: " << QDateTime::currentDateTime().toString() + "\r\n";
    log.close();

    first = false;
}

/*----------------------------------------------------------------------
 * Calculate the metrics for a ride file using the metrics factory
 *----------------------------------------------------------------------*/

// add a ride (after import / download)
void MetricAggregator::addRide(RideItem*ride)
{
    if (ride && ride->ride()) {
        importRide(context->athlete->home, ride->ride(), ride->fileName, context->athlete->zones()->getFingerprint(), true);
        RideFileCache updater(context, context->athlete->home.absolutePath() + "/" + ride->fileName, ride->ride(), true); // update cpx etc
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
    QFile file(path.absolutePath() + "/" + fileName);

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
