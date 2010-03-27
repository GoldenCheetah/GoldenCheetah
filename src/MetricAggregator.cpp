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

#include "MetricAggregator.h"
#include "DBAccess.h"
#include "RideFile.h"
#include "Zones.h"
#include "Settings.h"
#include "RideItem.h"
#include "RideMetric.h"
#include "TimeUtils.h"
#include <assert.h>
#include <math.h>
#include <QtXml/QtXml>
#include <QProgressDialog>

MetricAggregator::MetricAggregator(MainWindow *main, QDir home, const Zones *zones) : QWidget(main), main(main), home(home), zones(zones)
{
    dbaccess = new DBAccess(main, home);
    connect(main, SIGNAL(configChanged()), this, SLOT(update()));
    connect(main, SIGNAL(rideAdded(RideItem*)), this, SLOT(update(void)));
    connect(main, SIGNAL(rideDeleted(RideItem*)), this, SLOT(update(void)));
}

MetricAggregator::~MetricAggregator()
{
    delete dbaccess;
}

/*----------------------------------------------------------------------
 * Refresh the database -- only updates metrics when they are out
 *                         of date or missing altogether or where
 *                         the ride file no longer exists
 *----------------------------------------------------------------------*/

// used to store timestamp and fingerprint used in database
struct status { unsigned long timestamp, fingerprint; };

void MetricAggregator::refreshMetrics()
{
    // only if we have established a connection to the database
    if (dbaccess == NULL || main->isclean==true) return;

    // first check db structure is still up to date
    // this is because metadata.xml may add new fields
    dbaccess->checkDBVersion();

    // Get a list of the ride files
    QRegExp rx = RideFileFactory::instance().rideFileRegExp();
    QStringList errors;
    QStringList filenames = RideFileFactory::instance().listRideFiles(home);
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

    // Delete statistics for non-existant ride files
    QHash<QString, status>::iterator d;
    for (d = dbStatus.begin(); d != dbStatus.end(); ++d) {
        if (QFile(home.absolutePath() + "/" + d.key()).exists() == false) {
            dbaccess->deleteRide(d.key());
        }
    }

    unsigned long zoneFingerPrint = zones->getFingerprint(); // crc of zone data

    // update statistics for ride files which are out of date
    // showing a progress bar as we go
    QProgressDialog bar("Refreshing Metrics Database...", "Abort", 0, filenames.count(), main);
    bar.setWindowModality(Qt::WindowModal);
    int processed=0;
    while (i.hasNext()) {
        QString name = i.next();
        QFile file(home.absolutePath() + "/" + name);

        // if it s missing or out of date then update it!
        status current = dbStatus.value(name);
        unsigned long dbTimeStamp = current.timestamp;
        unsigned long fingerprint = current.fingerprint;
        if (dbTimeStamp < QFileInfo(file).lastModified().toTime_t() ||
            zoneFingerPrint != fingerprint) {

            // read file and process it
            RideFile *ride = RideFileFactory::instance().openRideFile(file, errors);
            if (ride != NULL) {
                importRide(home, ride, name, zoneFingerPrint, (dbTimeStamp > 0));
                delete ride;
            }
        }
        // update progress bar
        bar.setValue(++processed);
        QApplication::processEvents();

        if (bar.wasCanceled())
            break;
    }
    main->isclean = true;
}

/*----------------------------------------------------------------------
 * Calculate the metrics for a ride file using the metrics factory
 *----------------------------------------------------------------------*/
bool MetricAggregator::importRide(QDir path, RideFile *ride, QString fileName, unsigned long fingerprint, bool modify)
{
    SummaryMetrics *summaryMetric = new SummaryMetrics();
    QFile file(path.absolutePath() + "/" + fileName);

    QRegExp rx = RideFileFactory::instance().rideFileRegExp();
    if (!rx.exactMatch(fileName)) {
        return false; // not a ridefile!
    }
    summaryMetric->setFileName(fileName);
    assert(rx.numCaptures() == 7);
    QDate date(rx.cap(1).toInt(), rx.cap(2).toInt(),rx.cap(3).toInt());
    QTime time(rx.cap(4).toInt(), rx.cap(5).toInt(),rx.cap(6).toInt());
    QDateTime dateTime(date, time);

    summaryMetric->setRideDate(dateTime);

    const RideMetricFactory &factory = RideMetricFactory::instance();
    QStringList metrics;

    for (int i = 0; i < factory.metricCount(); ++i)
        metrics << factory.metricName(i);

    // compute all the metrics
    QHash<QString, RideMetricPtr> computed = RideMetric::computeMetrics(ride, zones, metrics);

    // get metrics into summaryMetric QMap
    for(int i = 0; i < factory.metricCount(); ++i) {
        // check for override
        summaryMetric->setForSymbol(factory.metricName(i), computed.value(factory.metricName(i))->value(true));
    }

    dbaccess->importRide(summaryMetric, ride, fingerprint, modify);
    delete summaryMetric;

    return true;
}

/*----------------------------------------------------------------------
 * Query functions are wrappers around DBAccess functions
 *----------------------------------------------------------------------*/
QList<SummaryMetrics>
MetricAggregator::getAllMetricsFor(QDateTime start, QDateTime end)
{
    if (main->isclean == false) refreshMetrics(); // get them up-to-date

    QList<SummaryMetrics> empty;

    // only if we have established a connection to the database
    if (dbaccess == NULL) {
        qDebug()<<"lost db connection?";
        return empty;
    }
    return dbaccess->getAllMetricsFor(start, end);
}
