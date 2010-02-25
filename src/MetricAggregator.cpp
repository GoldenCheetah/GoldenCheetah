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

bool MetricAggregator::isclean = false;

MetricAggregator::MetricAggregator(MainWindow *parent, QDir home, const Zones *zones) : QWidget(parent), parent(parent), home(home), zones(zones)
{
    dbaccess = new DBAccess(home);
    connect(parent, SIGNAL(configChanged()), this, SLOT(update()));
    connect(parent, SIGNAL(rideAdded(RideItem*)), this, SLOT(update(void)));
    connect(parent, SIGNAL(rideDeleted(RideItem*)), this, SLOT(update(void)));
}

MetricAggregator::~MetricAggregator()
{
    // close the database connection
    if (dbaccess != NULL) {
        dbaccess->closeConnection();
        delete dbaccess;
    }
}

/*----------------------------------------------------------------------
 * Refresh the database -- only updates metrics when they are out
 *                         of date or missing altogether or where
 *                         the ride file no longer exists
 *----------------------------------------------------------------------*/
void MetricAggregator::refreshMetrics()
{
    // only if we have established a connection to the database
    if (dbaccess == NULL || isclean==true) return;

    // Get a list of the ride files
    QRegExp rx = RideFileFactory::instance().rideFileRegExp();
    QStringList errors;
    QStringList filenames = RideFileFactory::instance().listRideFiles(home);
    QStringListIterator i(filenames);

    // get a Hash map of statistic records and timestamps
    QSqlQuery query(dbaccess->connection());
    QHash <QString, unsigned long> dbStatus;
    bool rc = query.exec("SELECT filename, timestamp FROM metrics ORDER BY ride_date;");
    while (rc && query.next()) {
        QString filename = query.value(0).toString();
        unsigned long timestamp = query.value(1).toInt();
        dbStatus.insert(filename, timestamp);
    }

    // Delete statistics for non-existant ride files
    QHash<QString, unsigned long>::iterator d;
    for (d = dbStatus.begin(); d != dbStatus.end(); ++d) {
        if (QFile(home.absolutePath() + "/" + d.key()).exists() == false) {
            dbaccess->deleteRide(d.key());
        }
    }

    // get power.zones timestamp to refresh on CP changes
    unsigned long zonesTimeStamp = 0;
    QString zonesfile = home.absolutePath() + "/power.zones";
    if (QFileInfo(zonesfile).exists())
        zonesTimeStamp = QFileInfo(zonesfile).lastModified().toTime_t();

    // update statistics for ride files which are out of date
    // showing a progress bar as we go
    QProgressDialog bar("Refreshing Metrics Database...", "Abort", 0, filenames.count(), parent);
    bar.setWindowModality(Qt::WindowModal);
    int processed=0;
    while (i.hasNext()) {
        QString name = i.next();
        QFile file(home.absolutePath() + "/" + name);

        // if it s missing or out of date then update it!
        unsigned long dbTimeStamp = dbStatus.value(name, 0);
        if (dbTimeStamp < QFileInfo(file).lastModified().toTime_t() ||
            dbTimeStamp < zonesTimeStamp) {

            // read file and process it
            RideFile *ride = RideFileFactory::instance().openRideFile(file, errors);
            if (ride != NULL) {
                importRide(home, ride, name, (dbTimeStamp > 0));
                delete ride;
            }
        }
        // update progress bar
        bar.setValue(++processed);
        QApplication::processEvents();

        if (bar.wasCanceled())
            break;
    }
    isclean = true;
}

/*----------------------------------------------------------------------
 * Calculate the metrics for a ride file using the metrics factory
 *----------------------------------------------------------------------*/
bool MetricAggregator::importRide(QDir path, RideFile *ride, QString fileName, bool modify)
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
        summaryMetric->setForSymbol(factory.metricName(i), computed.value(factory.metricName(i))->value(true));
    }

    dbaccess->importRide(summaryMetric, modify);
    delete summaryMetric;

    return true;
}

/*----------------------------------------------------------------------
 * Query functions are wrappers around DBAccess functions
 *----------------------------------------------------------------------*/
QList<SummaryMetrics>
MetricAggregator::getAllMetricsFor(QDateTime start, QDateTime end)
{
    if (isclean == false) refreshMetrics(); // get them up-to-date

    QList<SummaryMetrics> empty;

    // only if we have established a connection to the database
    if (dbaccess == NULL) return empty;
    return dbaccess->getAllMetricsFor(start, end);
}
