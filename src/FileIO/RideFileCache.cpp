/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#include "RideFileCache.h"
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "RideCache.h"
#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"
#include "WPrime.h" // for wbal zones
#include "LTMSettings.h" // getAllBestsFor needs this

#include <cmath> // for pow()
#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QtAlgorithms> // for qStableSort

static const int maxcache = 25; // lets max out at 25 caches

// cache from ride
RideFileCache::RideFileCache(Context *context, QString fileName, double weight, RideFile *passedride, bool check, bool refresh) :
               incomplete(false), context(context), rideFileName(fileName), ride(passedride)
{
    // resize all the arrays to zero
    wattsMeanMax.resize(0);
    heatMeanMax.resize(0);
    hrMeanMax.resize(0);
    cadMeanMax.resize(0);
    nmMeanMax.resize(0);
    kphMeanMax.resize(0);
    wattsdMeanMax.resize(0);
    caddMeanMax.resize(0);
    nmdMeanMax.resize(0);
    hrdMeanMax.resize(0);
    kphdMeanMax.resize(0);
    xPowerMeanMax.resize(0);
    npMeanMax.resize(0);
    vamMeanMax.resize(0);
    wattsKgMeanMax.resize(0);
    aPowerMeanMax.resize(0);
    aPowerKgMeanMax.resize(0);
    wattsDistribution.resize(0);
    hrDistribution.resize(0);
    cadDistribution.resize(0);
    gearDistribution.resize(0);
    nmDistribution.resize(0);
    kphDistribution.resize(0);
    xPowerDistribution.resize(0);
    npDistribution.resize(0);
    wattsKgDistribution.resize(0);
    aPowerDistribution.resize(0);
    smo2Distribution.resize(0);
    wbalDistribution.resize(0);

    // time in zone are fixed to 10 zone max
    wattsTimeInZone.resize(10);
    wattsCPTimeInZone.resize(4); // zero, I, II, III
    hrTimeInZone.resize(10);
    hrCPTimeInZone.resize(4); // zero, I, II, III
    paceTimeInZone.resize(10);
    paceCPTimeInZone.resize(4); // zero, I, II, III
    wbalTimeInZone.resize(4); // 0-25, 25-50, 50-75, 75% +

    // Get info for ride file and cache file
    QFileInfo rideFileInfo(rideFileName);
    cacheFileName = context->athlete->home->cache().canonicalPath() + "/" + rideFileInfo.baseName() + ".cpx";
    QFileInfo cacheFileInfo(cacheFileName);

    // is it up-to-date?
    if (cacheFileInfo.exists() && cacheFileInfo.size() >= (int)sizeof(struct RideFileCacheHeader)) {

        // we have a file, it is more recent than the ride file
        // but is it the latest version?
        RideFileCacheHeader head;
        QFile cacheFile(cacheFileName);
        if (cacheFile.open(QIODevice::ReadOnly) == true) {

            // read the header
            QDataStream inFile(&cacheFile);
            inFile.readRawData((char *) &head, sizeof(head));
            cacheFile.close();

            // its more recent -or- the crc is the same
            if (rideFileInfo.lastModified() <= cacheFileInfo.lastModified() ||
                head.crc == RideFile::computeFileCRC(rideFileName)) {
 
                // it is the same ?
                if (head.version == RideFileCacheVersion && head.WEIGHT == weight) {

                    // WE'RE GOOD
                    if (check == false) readCache(); // if check is false we aren't just checking
                    return;
                } else {
                    // for debug only
                    //qDebug()<<"refresh because version ("<<RideFileCacheVersion<<","<<head.version<<")"
                    //        << " weight ("<< weight <<"," <<head.WEIGHT<<")";
                }
            }
        }
    }

    // NEED TO UPDATE!!
    if (refresh) {

        // not up-to-date we need to refresh from the ridefile
        if (ride) {

            // we got passed the ride - so update from that
            WEIGHT = ride->getWeight(); // before threads are created
            refreshCache();

        } else {

            // we need to open it to update since we were not passed one
            QStringList errors;
            QFile file(rideFileName);

            ride = RideFileFactory::instance().openRideFile(context, file, errors);

            if (ride) {
                WEIGHT = ride->getWeight(); // before threads are created
                refreshCache();
                delete ride;
            }
            ride = 0;
        }
    } else {
        incomplete = true; // data not available !
    }
}

bool 
RideFileCache::checkStale(Context *context, RideItem*item)
{
    // check if we're stale ?
    // Get info for ride file and cache file
    QString rideFileName;
    if (item->planned)
        rideFileName = context->athlete->home->planned().canonicalPath() + "/" + item->fileName;
    else
        rideFileName = context->athlete->home->activities().canonicalPath() + "/" + item->fileName;

    QFileInfo rideFileInfo(rideFileName);
    QString cacheFileName;
    if (item->planned)
        cacheFileName = context->athlete->home->cache().canonicalPath() + "/planned/" + rideFileInfo.baseName() + ".cpx";
    else
        cacheFileName = context->athlete->home->cache().canonicalPath() + "/" + rideFileInfo.baseName() + ".cpx";

    QFileInfo cacheFileInfo(cacheFileName);

    // is it up-to-date?
    if (cacheFileInfo.exists() && cacheFileInfo.size() >= (int)sizeof(struct RideFileCacheHeader)) {

        // we have a file, it is more recent than the ride file
        // but is it the latest version?
        RideFileCacheHeader head;
        QFile cacheFile(cacheFileName);
        if (cacheFile.open(QIODevice::ReadOnly) == true) {

            // read the header
            QDataStream inFile(&cacheFile);
            inFile.readRawData((char *) &head, sizeof(head));
            cacheFile.close();

            // its more recent -or- the crc is the same
            if (rideFileInfo.lastModified() <= cacheFileInfo.lastModified() ||
                head.crc == RideFile::computeFileCRC(rideFileName)) {

                // it is the same ?
                if (head.version == RideFileCacheVersion && head.WEIGHT == item->getWeight()) {

                    // WE'RE GOOD
                    return false;
                }
            }
        }
    }

    // its stale !
    return true;
}

// returns offset from end of head
static long offsetForMeanMax(RideFileCacheHeader head, RideFile::SeriesType series)
{
    long offset = 0;

    switch (series) {
    case RideFile::aPowerKg : offset += head.aPowerMeanMaxCount * sizeof(float); // intentional fallthrough
    case RideFile::aPower : offset += head.vamMeanMaxCount * sizeof(float);  // intentional fallthrough
    case RideFile::vam : offset += head.npMeanMaxCount * sizeof(float);  // intentional fallthrough
    case RideFile::IsoPower : offset += head.xPowerMeanMaxCount * sizeof(float);  // intentional fallthrough
    case RideFile::xPower : offset += head.kphMeanMaxCount * sizeof(float);  // intentional fallthrough
    case RideFile::hrd : offset += head.hrdMeanMaxCount * sizeof(float);  // intentional fallthrough
    case RideFile::nmd : offset += head.nmdMeanMaxCount * sizeof(float);  // intentional fallthrough
    case RideFile::cadd : offset += head.caddMeanMaxCount * sizeof(float);  // intentional fallthrough
    case RideFile::wattsd : offset += head.wattsdMeanMaxCount * sizeof(float);  // intentional fallthrough
    case RideFile::kphd : offset += head.kphdMeanMaxCount * sizeof(float);  // intentional fallthrough
    case RideFile::kph : offset += head.nmMeanMaxCount * sizeof(float);  // intentional fallthrough
    case RideFile::nm : offset += head.cadMeanMaxCount * sizeof(float);  // intentional fallthrough
    case RideFile::cad : offset += head.hrMeanMaxCount * sizeof(float);  // intentional fallthrough
    case RideFile::hr : offset += head.wattsKgMeanMaxCount * sizeof(float);  // intentional fallthrough
    case RideFile::wattsKg : offset += head.wattsMeanMaxCount * sizeof(float);  // intentional fallthrough
    case RideFile::watts : offset += 0;  // intentional fallthrough
    default:
        break;
    }

    return offset;
}

//offset to tiz table
static long offsetForTiz(RideFileCacheHeader head, RideFile::SeriesType series)
{
    long offset = 0;

    // skip past the mean max arrays
    offset += head.aPowerKgMeanMaxCount * sizeof(float);
    offset += head.aPowerMeanMaxCount * sizeof(float);
    offset += head.vamMeanMaxCount * sizeof(float);
    offset += head.npMeanMaxCount * sizeof(float);
    offset += head.xPowerMeanMaxCount * sizeof(float);
    offset += head.hrdMeanMaxCount * sizeof(float);
    offset += head.nmdMeanMaxCount * sizeof(float);
    offset += head.caddMeanMaxCount * sizeof(float);
    offset += head.wattsdMeanMaxCount * sizeof(float);
    offset += head.kphdMeanMaxCount * sizeof(float);
    offset += head.kphMeanMaxCount * sizeof(float);
    offset += head.nmMeanMaxCount * sizeof(float);
    offset += head.cadMeanMaxCount * sizeof(float);
    offset += head.hrMeanMaxCount * sizeof(float);
    offset += head.wattsKgMeanMaxCount * sizeof(float);
    offset += head.wattsMeanMaxCount * sizeof(float);

    // skip past the distribution arrays
    offset += head.wattsDistCount * sizeof(float);
    offset += head.hrDistCount * sizeof(float);
    offset += head.cadDistCount * sizeof(float);
    offset += head.gearDistCount * sizeof(float);
    offset += head.nmDistrCount * sizeof(float);
    offset += head.kphDistCount * sizeof(float);
    offset += head.xPowerDistCount * sizeof(float);
    offset += head.npDistCount * sizeof(float);
    offset += head.wattsKgDistCount * sizeof(float);
    offset += head.aPowerDistCount * sizeof(float);
    offset += head.smo2DistCount * sizeof(float);
    offset += head.wbalDistCount * sizeof(float);

    // tiz ist currently just for RideFile:watts, RideFile:hr, RideFile:kph and RideFile:wbal
    // watts is first - so move on with offset only for 'hr' and 'kph' and 'wbal'
    // structure for "tiz" data - watts(10)/CPwatts(4)/HR(10)/CPhr(4)/PACE(10)/CPpace
    if (series == RideFile::hr) offset += (10+4) * sizeof(float);
    if (series == RideFile::kph) offset += 2*(10+4) * sizeof(float);
    if (series == RideFile::wbal) offset += 3*(10+4) * sizeof(float);

    return offset;
}


// returns offset from end of head
static long countForMeanMax(RideFileCacheHeader head, RideFile::SeriesType series)
{
    switch (series) {
    case RideFile::aPowerKg : return head.aPowerKgMeanMaxCount;
    case RideFile::aPower : return head.aPowerMeanMaxCount;
    case RideFile::wattsKg : return head.wattsKgMeanMaxCount;
    case RideFile::vam : return head.vamMeanMaxCount;
    case RideFile::IsoPower : return head.npMeanMaxCount;
    case RideFile::xPower :  return head.xPowerMeanMaxCount;
    case RideFile::kph :  return head.kphMeanMaxCount;
    case RideFile::kphd :  return head.kphdMeanMaxCount;
    case RideFile::wattsd :  return head.wattsdMeanMaxCount;
    case RideFile::cadd :  return head.caddMeanMaxCount;
    case RideFile::nmd :  return head.nmdMeanMaxCount;
    case RideFile::hrd :  return head.hrdMeanMaxCount;
    case RideFile::nm :  return head.nmMeanMaxCount;
    case RideFile::cad :  return head.cadMeanMaxCount;
    case RideFile::hr :  return head.hrMeanMaxCount;
    case RideFile::watts :  return head.wattsMeanMaxCount;
    default:
        break;
    }

    return 0;
}

QVector<float> RideFileCache::meanMaxPowerFor(Context *context, QVector<float> &wpk, QDate from, QDate to, QVector<QDate>*dates, bool wantruns)
{
    QVector<float> returning;
    QVector<float> returningwpk;
    bool first = true;

    // look at all the rides
    foreach (RideItem *item, context->athlete->rideCache->rides()) {

        if (item->dateTime.date() < from || item->dateTime.date() > to) continue; // not one we want

        if (item->isRun && !wantruns) continue; // they don't want runs

        // get the power data
        if (first == true) {

            // first time through the whole thing is going to be best
            returning =  meanMaxPowerFor(context, returningwpk, context->athlete->home->activities().canonicalPath() + "/" + item->fileName);

            // set a;; dates to this
            if (dates) {
                dates->resize(returning.size());
                for(int i=0; i<dates->size(); i++) (*dates)[i]=item->dateTime.date();
            }
            first = false;

        } else {

            QVector<float> thiswpk;

            // next time through we should only pick out better times
            QVector<float> ridebest = meanMaxPowerFor(context, thiswpk, context->athlete->home->activities().canonicalPath() + "/" + item->fileName);

            // do we need to increase the returning array?
            if (returning.size() < ridebest.size()) returning.resize(ridebest.size());
            if (dates && dates->size() < ridebest.size()) dates->resize(ridebest.size());

            // now update where its a better number
            for (int i=0; i<ridebest.size(); i++) {
                if (ridebest[i] > returning[i]) {
                    returning[i] = ridebest[i];
                    (*dates)[i]=item->dateTime.date();
                }
           }

            // do we need to increase the returning array?
            if (returningwpk.size() < thiswpk.size()) returningwpk.resize(thiswpk.size());

            // now update where its a better number
            for (int i=0; i<thiswpk.size(); i++)
                if (thiswpk[i] > returningwpk[i]) returningwpk[i] =thiswpk[i];
        }
    }

    // set aggregated wpk
    wpk = returningwpk;
    return returning;
}

QVector<float> RideFileCache::meanMaxPowerFor(Context *context, QVector<float>&wpk, QString fileName)
{
    QTime start;
    start.start();

    QVector<float> returning;

    // Get info for ride file and cache file
    QFileInfo rideFileInfo(fileName);
    QString cacheFilename = context->athlete->home->cache().canonicalPath() + "/" + rideFileInfo.baseName() + ".cpx";
    QFileInfo cacheFileInfo(cacheFilename);

    // is it up-to-date?
    if (cacheFileInfo.exists() && cacheFileInfo.size() >= (int)sizeof(struct RideFileCacheHeader)) {

        // we have a file, it is more recent than the ride file
        // but is it the latest version?
        RideFileCacheHeader head;
        QFile cacheFile(cacheFilename);
        if (cacheFile.open(QIODevice::ReadOnly) == true) {

            // read the header
            QDataStream inFile(&cacheFile);
            inFile.readRawData((char *) &head, sizeof(head));

            // check its an up to date format and contains power
            if (head.version == RideFileCacheVersion && head.wattsMeanMaxCount > 0) {

                // seek to start of meanmax array in the cache
                long offset = offsetForMeanMax(head, RideFile::watts) + sizeof(head);
                cacheFile.seek(qint64(offset));

                // read from cache and put straight into QVector memory
                // a little naughty but seems to work ok
                returning.resize(head.wattsMeanMaxCount);
                inFile.readRawData((char*)returning.constData(), head.wattsMeanMaxCount * sizeof(float));

                offset = offsetForMeanMax(head, RideFile::wattsKg) + sizeof(head);
                cacheFile.seek(qint64(offset));

                wpk.resize(head.wattsKgMeanMaxCount);
                inFile.readRawData((char*)wpk.constData(), head.wattsKgMeanMaxCount * sizeof(float));
                for(int i=0; i<wpk.size(); i++) wpk[i] = wpk[i] / 100.00f;

                //qDebug()<<"retrieved:"<<head.wattsMeanMaxCount<<"in:"<<start.elapsed()<<"ms";
            }

            // we're done reading
            cacheFile.close();
        }
    }

    // will be empty if no up to date cache
    return returning;

}

// the next 2 are used by the API web services to extract meanmax data from the cache

// API bests for a ride
QVector<float> RideFileCache::meanMaxFor(QString cacheFilename, RideFile::SeriesType series)
{
    QTime start;
    start.start();

    QVector<float> returning;

    // Get info for ride file and cache file
    QFileInfo cacheFileInfo(cacheFilename);

    // is it up-to-date?
    if (cacheFileInfo.exists() && cacheFileInfo.size() >= (int)sizeof(struct RideFileCacheHeader)) {

        // we have a file, it is more recent than the ride file
        // but is it the latest version?
        RideFileCacheHeader head;
        QFile cacheFile(cacheFilename);
        if (cacheFile.open(QIODevice::ReadOnly) == true) {

            // read the header
            QDataStream inFile(&cacheFile);
            inFile.readRawData((char *) &head, sizeof(head));

            int count = countForMeanMax(head, series);

            // check its an up to date format and contains power
            if (head.version == RideFileCacheVersion && count>0) {

                // seek to start of meanmax array in the cache
                long offset = offsetForMeanMax(head, series) + sizeof(head);
                cacheFile.seek(qint64(offset));

                // read from cache and put straight into QVector memory
                // a little naughty but seems to work ok
                returning.resize(count);
                inFile.readRawData((char*)returning.constData(), count * sizeof(float));
            }

            // we're done reading
            cacheFile.close();
        }
    }

    // will be empty if no up to date cache
    return returning;

}

// API bests for a date range
QVector<float> RideFileCache::meanMaxFor(QString cacheDir, RideFile::SeriesType series, QDate from, QDate to)
{
    QTime start;
    start.start();

    bool first = true;
    QVector<float> returning;

    // loop through all CPX files
    foreach(QString cacheFilename, QDir(cacheDir).entryList(QDir::Files)) {

        // is it a cpx file ?
        if (!cacheFilename.endsWith(".cpx")) continue;
   
        // is it big enough ? 
        if (QFileInfo(cacheDir + "/" + cacheFilename).size() < (int)sizeof(struct RideFileCacheHeader)) continue;

        // lets check it parses ok ?
        QDateTime dt;
        if (!RideFile::parseRideFileName(cacheFilename, &dt)) continue; 

        // in range?
        if (dt.date() < from || dt.date() > to) continue;

        // get data
        QVector<float> current = RideFileCache::meanMaxFor(cacheDir + "/" + cacheFilename, series);

        // first ?
        if (first) {
            first = false;
            returning = current;
        } else {
            if (current.size() > returning.size()) returning.resize(current.size());
            for(int i=0; i< current.size(); i++) if (current[i] > returning[i]) returning[i]=current[i];
        }
    }

    // will be empty if no up to date cache
    return returning;

}

RideFileCache::RideFileCache(RideFile *ride) :
               incomplete(false), context(ride->context), rideFileName(""), ride(ride)
{
    // resize all the arrays to zero
    wattsMeanMax.resize(0);
    heatMeanMax.resize(0);
    hrMeanMax.resize(0);
    cadMeanMax.resize(0);
    nmMeanMax.resize(0);
    kphMeanMax.resize(0);
    kphdMeanMax.resize(0);
    wattsdMeanMax.resize(0);
    caddMeanMax.resize(0);
    nmdMeanMax.resize(0);
    hrdMeanMax.resize(0);
    xPowerMeanMax.resize(0);
    npMeanMax.resize(0);
    vamMeanMax.resize(0);
    wattsKgMeanMax.resize(0);
    aPowerMeanMax.resize(0);
    aPowerKgMeanMax.resize(0);
    wattsDistribution.resize(0);
    hrDistribution.resize(0);
    cadDistribution.resize(0);
    gearDistribution.resize(0);
    nmDistribution.resize(0);
    kphDistribution.resize(0);
    xPowerDistribution.resize(0);
    npDistribution.resize(0);
    wattsKgDistribution.resize(0);
    aPowerDistribution.resize(0);
    smo2Distribution.resize(0);
    wbalDistribution.resize(0);

    // time in zone are fixed to 10 zone max
    wattsTimeInZone.resize(10);
    wattsCPTimeInZone.resize(4);
    hrTimeInZone.resize(10);
    hrCPTimeInZone.resize(4);
    paceTimeInZone.resize(10);
    paceCPTimeInZone.resize(4);
    wbalTimeInZone.resize(4);

    WEIGHT = ride->getWeight();
    ride->recalculateDerivedSeries(); // accel and others

    // calculate all the arrays
    compute();
}

int
RideFileCache::decimalsFor(RideFile::SeriesType series)
{
    switch (series) {
        case RideFile::secs : return 0; break;
        case RideFile::cad : return 0; break;
        case RideFile::gear : return 2; break;
        case RideFile::hr : return 0; break;
        case RideFile::km : return 3; break;
        case RideFile::kph : return 1; break;
        case RideFile::kphd : return 2; break;
        case RideFile::wattsd : return 0; break;
        case RideFile::cadd : return 0; break;
        case RideFile::nmd : return 2; break;
        case RideFile::hrd : return 0; break;
        case RideFile::nm : return 2; break;
        case RideFile::watts : return 0; break;
        case RideFile::xPower : return 0; break;
        case RideFile::IsoPower : return 0; break;
        case RideFile::alt : return 1; break;
        case RideFile::lon : return 6; break;
        case RideFile::lat : return 6; break;
        case RideFile::headwind : return 1; break;
        case RideFile::slope : return 1; break;
        case RideFile::temp : return 1; break;
        case RideFile::interval : return 0; break;
        case RideFile::vam : return 0; break;
        case RideFile::wattsKg : return 2; break;
        case RideFile::aPower : return 0; break;
        case RideFile::aPowerKg : return 2; break;
        case RideFile::smo2 : return 0; break;
        case RideFile::wbal : return 0; break;
        case RideFile::lrbalance : return 1; break;
        case RideFile::wprime :  return 0; break;
        case RideFile::none : break;
        default : return 2;
    }
    return 2; // default
}

//
// DATA ACCESS
//
QVector<QDate> &
RideFileCache::meanMaxDates(RideFile::SeriesType series)
{
    switch (series) {

        case RideFile::watts:
            return wattsMeanMaxDate;
            break;

        case RideFile::cad:
            return cadMeanMaxDate;
            break;

        case RideFile::hr:
            return hrMeanMaxDate;
            break;

        case RideFile::nm:
            return nmMeanMaxDate;
            break;

        case RideFile::kph:
            return kphMeanMaxDate;
            break;

        case RideFile::kphd:
            return kphdMeanMaxDate;
            break;

        case RideFile::wattsd:
            return wattsdMeanMaxDate;
            break;

        case RideFile::cadd:
            return caddMeanMaxDate;
            break;

        case RideFile::nmd:
            return nmdMeanMaxDate;
            break;

        case RideFile::hrd:
            return hrdMeanMaxDate;
            break;

        case RideFile::xPower:
            return xPowerMeanMaxDate;
            break;

        case RideFile::IsoPower:
            return npMeanMaxDate;
            break;

        case RideFile::vam:
            return vamMeanMaxDate;
            break;

        case RideFile::aPower:
            return aPowerMeanMaxDate;
            break;

        case RideFile::aPowerKg:
            return aPowerKgMeanMaxDate;
            break;

        case RideFile::wattsKg:
            return wattsKgMeanMaxDate;
            break;

        default:
            //? dunno give em power anyway
            return wattsMeanMaxDate;
            break;
    }
}

QList<RideFile::SeriesType> RideFileCache::meanMaxList()
{
        QList<RideFile::SeriesType> list;
        list << RideFile::watts
             << RideFile::wattsKg
             << RideFile::nm
             << RideFile::hr
             << RideFile::cad
             << RideFile::kph
             << RideFile::vam
             << RideFile::IsoPower
             << RideFile::aPower
             << RideFile::aPowerKg
             << RideFile::xPower
             ;

    return list;
}

QVector<double> &
RideFileCache::meanMaxArray(RideFile::SeriesType series)
{
    switch (series) {

        case RideFile::watts:
            return wattsMeanMaxDouble;
            break;

        case RideFile::cad:
            return cadMeanMaxDouble;
            break;

        case RideFile::hr:
            return hrMeanMaxDouble;
            break;

        case RideFile::nm:
            return nmMeanMaxDouble;
            break;

        case RideFile::kph:
            return kphMeanMaxDouble;
            break;

        case RideFile::kphd:
            return kphdMeanMaxDouble;
            break;

        case RideFile::wattsd:
            return wattsdMeanMaxDouble;
            break;

        case RideFile::cadd:
            return caddMeanMaxDouble;
            break;

        case RideFile::nmd:
            return nmdMeanMaxDouble;
            break;

        case RideFile::hrd:
            return hrdMeanMaxDouble;
            break;

        case RideFile::xPower:
            return xPowerMeanMaxDouble;
            break;

        case RideFile::IsoPower:
            return npMeanMaxDouble;
            break;

        case RideFile::vam:
            return vamMeanMaxDouble;
            break;

        case RideFile::aPower:
            return aPowerMeanMaxDouble;
            break;

        case RideFile::aPowerKg:
            return aPowerKgMeanMaxDouble;
            break;

        case RideFile::wattsKg:
            return wattsKgMeanMaxDouble;
            break;

        default:
            //? dunno give em power anyway
            return wattsMeanMaxDouble;
            break;
    }
}

QVector<double> &
RideFileCache::distributionArray(RideFile::SeriesType series)
{
    switch (series) {

        case RideFile::watts:
            return wattsDistributionDouble;
            break;

        case RideFile::cad:
            return cadDistributionDouble;
            break;

       case RideFile::gear:
           return gearDistributionDouble;
           break;

        case RideFile::hr:
            return hrDistributionDouble;
            break;

        case RideFile::nm:
            return nmDistributionDouble;
            break;

        case RideFile::kph:
            return kphDistributionDouble;
            break;

        case RideFile::aPower:
            return aPowerDistributionDouble;
            break;

        case RideFile::smo2:
            return smo2DistributionDouble;
            break;

        case RideFile::wbal:
            return wbalDistributionDouble;
            break;

        case RideFile::wattsKg:
            return wattsKgDistributionDouble;
            break;

        default:
            //? dunno give em power anyway
            return wattsMeanMaxDouble;
            break;
    }
}

RideFileCache *
RideFileCache::createCacheFor(RideFile*rideFile)
{
    return new RideFileCache(rideFile);
}

//
// COMPUTATION
//
void
RideFileCache::refreshCache()
{
    static bool writeerror=false;

    // set head crc
    crc = RideFile::computeFileCRC(rideFileName);

    // update cache!
    QFile cacheFile(cacheFileName);

    if (cacheFile.open(QIODevice::WriteOnly) == true) {

        // ok so we are going to be able to write this stuff
        // so lets go recalculate it all
        compute();

        QDataStream outFile(&cacheFile);

        // go write it out
        serialize(&outFile);

        // all done now, phew
        cacheFile.close();

        // invalidate any incore cache of aggregate
        // that contains this ride in its date range
        QDate date = ride->startTime().date();
        for (int i=0; i<context->athlete->cpxCache.count();) {
            if (date >= context->athlete->cpxCache.at(i)->start &&
                date <= context->athlete->cpxCache.at(i)->end) {
                delete context->athlete->cpxCache.at(i);
                context->athlete->cpxCache.removeAt(i);
            } else i++;
        }


    } else if (writeerror == false) {

        // popup the first time...
        writeerror = true;
        QMessageBox err;
        QString errMessage = QString("Cannot create cache file %1.").arg(cacheFileName);
        err.setText(errMessage);
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;

    } else {

        // send a console message instead...
        qDebug()<<"cannot create cache file"<<cacheFileName;
    }
}

// if you already have a cache open and want
// to refresh it from in-memory data then refresh()
// does that
void RideFileCache::refresh(RideFile *file)
{
    // set and refresh
    if (file == NULL && ride == NULL) return;
    if (file) ride = file;

    // just call compute!
    compute();
}

// this function is a candidate for supporting
// threaded calculations, each of the computes
// in here could go in its own thread. Users
// with many cores would benefit enormously
void RideFileCache::RideFileCache::compute()
{
    if (ride == NULL) {
        return;
    }

    // all the mean maxes
    MeanMaxComputer thread1(ride, wattsMeanMax, RideFile::watts); thread1.start();
    MeanMaxComputer thread2(ride, hrMeanMax, RideFile::hr); thread2.start();
    MeanMaxComputer thread3(ride, cadMeanMax, RideFile::cad); thread3.start();
    MeanMaxComputer thread4(ride, nmMeanMax, RideFile::nm); thread4.start();
    MeanMaxComputer thread5(ride, kphMeanMax, RideFile::kph); thread5.start();
    MeanMaxComputer thread6(ride, xPowerMeanMax, RideFile::xPower); thread6.start();
    MeanMaxComputer thread7(ride, npMeanMax, RideFile::IsoPower); thread7.start();
    MeanMaxComputer thread8(ride, vamMeanMax, RideFile::vam); thread8.start();
    MeanMaxComputer thread9(ride, wattsKgMeanMax, RideFile::wattsKg); thread9.start();
    MeanMaxComputer thread10(ride, aPowerMeanMax, RideFile::aPower); thread10.start();
    MeanMaxComputer thread11(ride, kphdMeanMax, RideFile::kphd); thread11.start();
    MeanMaxComputer thread12(ride, wattsdMeanMax, RideFile::wattsd); thread12.start();
    MeanMaxComputer thread13(ride, caddMeanMax, RideFile::cadd); thread13.start();
    MeanMaxComputer thread14(ride, nmdMeanMax, RideFile::nmd); thread14.start();
    MeanMaxComputer thread15(ride, hrdMeanMax, RideFile::hrd); thread15.start();
    MeanMaxComputer thread16(ride, aPowerKgMeanMax, RideFile::aPowerKg); thread16.start();

    // all the different distributions
    computeDistribution(wattsDistribution, RideFile::watts);
    computeDistribution(hrDistribution, RideFile::hr);
    computeDistribution(cadDistribution, RideFile::cad);
    computeDistribution(gearDistribution, RideFile::gear);
    computeDistribution(nmDistribution, RideFile::nm);
    computeDistribution(kphDistribution, RideFile::kph);
    computeDistribution(wattsKgDistribution, RideFile::wattsKg);
    computeDistribution(aPowerDistribution, RideFile::aPower);
    computeDistribution(smo2Distribution, RideFile::smo2);
    computeDistribution(wbalDistribution, RideFile::wbal);

    // wait for them threads
    thread1.wait();
    thread2.wait();
    thread3.wait();
    thread4.wait();
    thread5.wait();
    thread6.wait();
    thread7.wait();
    thread8.wait();
    thread9.wait();
    thread10.wait();
    thread11.wait();
    thread12.wait();
    thread13.wait();
    thread14.wait();
    thread15.wait();
    thread16.wait();

    // setup the doubles the users use
    doubleArray(wattsMeanMaxDouble, wattsMeanMax, RideFile::watts);
    doubleArray(hrMeanMaxDouble, hrMeanMax, RideFile::hr);
    doubleArray(cadMeanMaxDouble, cadMeanMax, RideFile::cad);
    doubleArray(nmMeanMaxDouble, nmMeanMax, RideFile::nm);
    doubleArray(kphMeanMaxDouble, kphMeanMax, RideFile::kph);
    doubleArray(kphdMeanMaxDouble, kphdMeanMax, RideFile::kphd);
    doubleArray(wattsdMeanMaxDouble, wattsdMeanMax, RideFile::wattsd);
    doubleArray(caddMeanMaxDouble, caddMeanMax, RideFile::cadd);
    doubleArray(nmdMeanMaxDouble, nmdMeanMax, RideFile::nmd);
    doubleArray(hrdMeanMaxDouble, hrdMeanMax, RideFile::hrd);
    doubleArray(npMeanMaxDouble, npMeanMax, RideFile::IsoPower);
    doubleArray(vamMeanMaxDouble, vamMeanMax, RideFile::vam);
    doubleArray(xPowerMeanMaxDouble, xPowerMeanMax, RideFile::xPower);
    doubleArray(wattsKgMeanMaxDouble, wattsKgMeanMax, RideFile::wattsKg);
    doubleArray(aPowerMeanMaxDouble, aPowerMeanMax, RideFile::aPower);
    doubleArray(aPowerKgMeanMaxDouble, aPowerKgMeanMax, RideFile::aPowerKg);

    doubleArrayForDistribution(wattsDistributionDouble, wattsDistribution);
    doubleArrayForDistribution(hrDistributionDouble, hrDistribution);
    doubleArrayForDistribution(cadDistributionDouble, cadDistribution);
    doubleArrayForDistribution(gearDistributionDouble, gearDistribution);
    doubleArrayForDistribution(nmDistributionDouble, nmDistribution);
    doubleArrayForDistribution(kphDistributionDouble, kphDistribution);
    doubleArrayForDistribution(xPowerDistributionDouble, xPowerDistribution);
    doubleArrayForDistribution(npDistributionDouble, npDistribution);
    doubleArrayForDistribution(wattsKgDistributionDouble, wattsKgDistribution);
    doubleArrayForDistribution(aPowerDistributionDouble, aPowerDistribution);
    doubleArrayForDistribution(smo2DistributionDouble, smo2Distribution);
    doubleArrayForDistribution(wbalDistributionDouble, wbalDistribution);
}

//----------------------------------------------------------------------
// Mark Rages' Algorithm for Fast Find of Mean-Max
//----------------------------------------------------------------------

/*

   A Faster Mean-Max Algorithm

   Premises:

   1 - maximum average power for a given interval occurs at maximum
       energy for the interval, because the interval time is fixed;

   2 - the energy in an interval enclosing a smaller interval will
       always be equal or greater than an interval;

   3 - finding maximum of means is a search algorithm, so biggest
       gains are found in reducing the search space as quickly as
       possible.

   Algorithm

   note: I find it easier to reason with concrete numbers, so I will
   describe the algorithm in terms of power and 60 second max-mean:

   To find the maximum average power for one minute:

   1 - integrate the watts over the entire ride to get accumulated
       energy in joules.  This is a monotonic function (assuming watts
       are positive).  The final value is the energy for the whole
       ride.  Once this is done, the energy for any section can be
       found with a single subtraction.

   2 - divide the energy into overlapping two-minute sections.
       Section one = 0:00 -> 2:00, section two = 1:00 -> 3:00, etc.

       Example:  Find 60s MM in 5-minute file

       +----------+----------+----------+----------+----------+
       | minute 1 | minute 2 | minute 3 | minute 4 | minute 5 |
       +----------+----------+----------+----------+----------+
       |             |_MEAN_MAX_|                             |
       +---------------------+---------------------+----------+
       |      segment 1      |      segment 3      |
       +----------+----------+----------+----------+----------+
                  |      segment 2      |      segment 4      |
                  +---------------------+---------------------+

       So no matter where the MEAN_MAX segment is located in time, it
       will be wholly contained in one segment.

       In practice, it is a little faster to make the windows smaller
       and overlap more:
       +----------+----------+----------+----------+----------+
       | minute 1 | minute 2 | minute 3 | minute 4 | minute 5 |
       +----------+----------+----------+----------+----------+
       |             |_MEAN_MAX_|                             |
       +-------------+----------------------------------------+
          |  segment 1  |
          +--+----------+--+
          |  segment 2  |
          +--+----------+--+
             |  segment 3  |
             +--+----------+--+
                |  segment 4  |
                +--+----------+--+
                   |  segment 5  |
                   +--+----------+--+
                      |  segment 6  |
                      +--+----------+--+
                         |  segment 7  |
                         +--+----------+--+
                            |  segment 8  |
                            +--+----------+--+
                               |  segment 9  |
                               +-------------+
                                            ... etc.

       ( This is because whenever the actual mean max energy is
         greater than a segment energy, we can skip the detail
         comparison within that segment altogether.  The exact
         tradeoff for optimum performance depends on the distribution
         of the data.  It's a pretty shallow curve.  Values in the 1
         minute to 1.5 minute range seem to work pretty well. )

   3 - for each two minute section, subtract the accumulated energy at
       the end of the section from the accumulated energy at the
       beginning of the section.  That gives the energy for that section.

   4 - in the first section, go second-by-second to find the maximum
       60-second energy.  This is our candidate for 60-second energy

   5 - go down the sorted list of sections.  If the energy in the next
       section is less than the 60-second energy in the best candidate so
       far, skip to the next section without examining it carefully,
       because the section cannot possibly have a one-minute section with
       greater energy.

       while (section->energy > candidate) {
         candidate=max(candidate, search(section, 60));
         section++;
       }

   6. candidate is the mean max for 60 seconds.

   Enhancements that are not implemented:

     - The two-minute overlapping sections can be reused for 59
       seconds, etc.  The algorithm will degrade to exhaustive search
       if the looked-for interval is much smaller than the enclosing
       interval.

     - The sections can be sorted by energy in reverse order before
       step #4.  Then the search in #5 can be terminated early, the
       first time it fails.  In practice, the comparisons in the
       search outnumber the saved comparisons.  But this might be a
       useful optimization if the windows are reused per the previous
       idea.

*/

static data_t *
integrate_series(cpintdata &data)
{
    // would be better to do pure QT and use QVector -- but no memory leak
    data_t *integrated= (data_t *)malloc(sizeof(data_t)*(data.points.size()+1)); 
    int i;
    data_t acc=0;

    for (i=0; i<data.points.size(); i++) {
        integrated[i]=acc;
        acc+=data.points[i].value;
    }
    integrated[i]=acc;

    return integrated;
}

static data_t
partial_max_mean(data_t *dataseries_i, int start, int end, int length, int *offset)
{
    int i=0;
    data_t candidate=0;

    int best_i=0;

    for (i=start; i<(1+end-length); i++) {
        data_t test_energy=dataseries_i[length+i]-dataseries_i[i];
        if (test_energy>candidate) {
            candidate=test_energy;
            best_i=i;
        }
    }
    if (offset) *offset=best_i;

    return candidate;
}


static data_t
divided_max_mean(data_t *dataseries_i, int datalength, int length, int *offset)
{
    int shift=length;

    //if sorting data the following is an important speedup hack
    if (shift>180) shift=180;

    int window_length=length+shift;

    if (window_length>datalength) window_length=datalength;

    // put down as many windows as will fit without overrunning data
    int start=0;
    int end=0;
    data_t energy=0;

    data_t candidate=0;
    int this_offset=0;

    for (start=0; start+window_length<=datalength; start+=shift) {
        end=start+window_length;
        energy=dataseries_i[end]-dataseries_i[start];

        if (energy < candidate) {
          continue;
        }
        data_t window_mm=partial_max_mean(dataseries_i, start, end, length, &this_offset);

        if (window_mm>candidate) {
            candidate=window_mm;
            if (offset) *offset=this_offset;
        }
    }

    // if the overlapping windows don't extend to the end of the data,
    // let's tack another one on at the end

    if (end<datalength) {
        start=datalength-window_length;
        end=datalength;
        energy=dataseries_i[end]-dataseries_i[start];

        if (energy >= candidate) {

            data_t window_mm=partial_max_mean(dataseries_i, start, end, length, &this_offset);

            if (window_mm>candidate) {
                candidate=window_mm;
                if (offset) *offset=this_offset;
            }
        }
    }

    return candidate;
}


void
MeanMaxComputer::run()
{
    // xPower and IsoPower need watts to be present
    RideFile::SeriesType baseSeries = (series == RideFile::xPower || series == RideFile::IsoPower || series == RideFile::wattsKg) ?
                                      RideFile::watts : series;

    if (series == RideFile::aPowerKg) baseSeries = RideFile::aPower;
    else if (series == RideFile::vam) baseSeries = RideFile::alt;

    // there is a distinction between needing it present and using it in calcs
    RideFile::SeriesType needSeries = baseSeries;
    if (series == RideFile::kphd) needSeries = RideFile::kph;
    if (series == RideFile::wattsd) needSeries = RideFile::watts;
    if (series == RideFile::cadd) needSeries = RideFile::cad;
    if (series == RideFile::nmd) needSeries = RideFile::nm;
    if (series == RideFile::hrd) needSeries = RideFile::hr;

    // only bother if the data series is actually present
    if (ride->isDataPresent(needSeries) == false) return;

    // if we want decimal places only keep to 1 dp max
    // this is a factor that is applied at the end to
    // convert from high-precision double to long
    // e.g. 145.456 becomes 1455 if we want decimals
    // and becomes 145 if we don't
    double decimals =  pow(10, RideFileCache::decimalsFor(series));
    //double decimals = RideFile::decimalsFor(baseSeries) ? 10 : 1;

    // decritize the data series - seems wrong, since it just
    // rounds to the nearest second - what if the recIntSecs
    // is less than a second? Has been used for a long while
    // so going to leave in tact for now - apart from the
    // addition of code to fill in gaps in recording since
    // they affect the IsoPower/xPower algorithm badly and will skew
    // the calculations of >6m since windowsize is used to
    // determine segment duration rather than examining the
    // timestamps on each sample
    // the decrit will also pull timestamps back to start at
    // zero, since some files have a very large start time
    // that creates work for nil effect (but increases compute
    // time drastically).
    cpintdata data;
    data.rec_int_ms = (int) round(ride->recIntSecs() * 1000.0);
    double lastsecs = 0;
    bool first = true;
    double offset = 0;
    foreach (const RideFilePoint *p, ride->dataPoints()) {

        // get offset to apply on all samples if first sample
        if (first == true) {
            offset = p->secs;
            first = false;
        }

        // drag back to start at 1s or whatever recIntSecs() is !
        double psecs = p->secs - offset + ride->recIntSecs();

        // fill in any gaps in recording - use same dodgy rounding as before
        int count = (psecs - lastsecs - ride->recIntSecs()) / ride->recIntSecs();

        // gap more than an hour, damn that ride file is a mess
        if (count > 3600) count = 1;

        for(int i=0; i<count; i++)
            data.points.append(cpintpoint(round(lastsecs+((i+1)*ride->recIntSecs() *1000.0)/1000), 0));
        lastsecs = psecs;

        double secs = round(psecs * 1000.0) / 1000;
        if (secs > 0) data.points.append(cpintpoint(secs, (int) round(p->value(baseSeries)*double(decimals))));
    }


    // don't bother with insufficient data
    if (!data.points.count()) return;

    int total_secs = (int) ceil(data.points.back().secs);

    // don't allow data more than two days
    // was one week, but no single ride is longer
    // than 2 days, even if you are doing RAAM
    if (total_secs > 2*24*60*60) return;

    // don't allow if badly parsed or time goes backwards
    if (total_secs < 0) return;

    //
    // Pre-process the data for IsoPower, xPower and VAM
    //

    // VAM - adjust to Vertical Ascent per Hour
    if (series == RideFile::vam) {

        double lastAlt=0;

        for (int i=0; i<data.points.size(); i++) {

            // handle drops gracefully (and first sample too)
            // if you manage to rise >5m in a second thats a data error too!
            if (!lastAlt || (data.points[i].value - lastAlt) > 5) lastAlt=data.points[i].value;

            // NOTE: It is 360 not 3600 because Altitude is factored for decimal places
            //       since it is the base data series, but we are calculating VAM
            //       And we multiply by 10 at the end!
            double vam = (((data.points[i].value - lastAlt) * 360)/ride->recIntSecs()) * 10;
            if (vam < 0) vam = 0;
            lastAlt = data.points[i].value;
            data.points[i].value = vam;
        }
    }

    // IsoPower - rolling 30s avg ^ 4
    if (series == RideFile::IsoPower) {

        int rollingwindowsize = 30 / ride->recIntSecs();

        // no point doing a rolling average if the
        // sample rate is greater than the rolling average
        // window!!
        if (rollingwindowsize > 1) {

            QVector<double> rolling(rollingwindowsize);
            int index = 0;
            double sum = 0;

            // loop over the data and convert to a rolling
            // average for the given windowsize
            for (int i=0; i<data.points.size(); i++) {

                sum += data.points[i].value;
                sum -= rolling[index];

                rolling[index] = data.points[i].value;
                data.points[i].value = pow(sum/(double)rollingwindowsize,4.0f); // raise rolling average to 4th power

                // move index on/round
                index = (index >= rollingwindowsize-1) ? 0 : index+1;
            }
        }
    }

    // xPower - 25s EWA - uses same algorithm as BikeScore.cpp
    if (series == RideFile::xPower) {

        const double exp = ride->recIntSecs() / ((25.0f / ride->recIntSecs()) + ride->recIntSecs());
        const double rem = 1.0f - exp;

        int rollingwindowsize = 25 / ride->recIntSecs();
        double ewma = 0.0;
        double sum = 0.0; // as we ramp up

        // no point doing a rolling average if the
        // sample rate is greater than the rolling average
        // window!!
        if (rollingwindowsize > 1) {

            // loop over the data and convert to a EWMA
            for (int i=0; i<data.points.size(); i++) {

                // dgr : BikeScore has weighting value from first point
                if (false && i < rollingwindowsize) {

                    // get up to speed
                    sum += data.points[i].value;
                    ewma = sum / (i+1);

                } else {

                    // we're up to speed
                    ewma = (data.points[i].value * exp) + (ewma * rem);
                }
                data.points[i].value = pow(ewma, 4.0f);
            }
        }
    }

    if (series == RideFile::wattsKg || series == RideFile::aPowerKg) {
        for (int i=0; i<data.points.size(); i++) {
            double wattsKg = data.points[i].value / ride->getWeight();
            data.points[i].value = wattsKg;
        }
    }


    // the bests go in here...
    QVector <double> ride_bests(total_secs + 1);

    data_t *dataseries_i = integrate_series(data);

    for (int i=1; i<data.points.size();) {

        int offset;
        data_t c=divided_max_mean(dataseries_i,data.points.size(),i,&offset);

        // snaffle it away
        int sec = i*ride->recIntSecs();
        data_t val = c / (data_t)i;

        if (sec < ride_bests.size()) {
            if (series == RideFile::IsoPower || series == RideFile::xPower)
                ride_bests[sec] = pow(val, 0.25f);
            else
                ride_bests[sec] = val;
        }


        // increments to limit search scope
        if (i<120) i++;
        else if (i<600) i+= 2;
        else if (i<1200) i += 5;
        else if (i<3600) i += 20;
        else if (i<7200) i += 120;
        else i += 300;
    }
    free(dataseries_i);

    //
    // FILL IN THE GAPS AND FILL TARGET ARRAY
    //
    // We want to present a full set of bests for
    // every duration so the data interface for this
    // cache can remain the same, but the level of
    // accuracy/granularity can change in here in the
    // future if some fancy new algorithm arrives
    //

    // XXX seems we can end up with 0 at the end ?
    // XXX don't know why, so, for now, just clean that
    while (ride_bests.size() && ride_bests[ride_bests.size()-1] == 0)
        ride_bests.resize(ride_bests.size()-1);

    double last = 0;

    // only care about first 3 minutes MAX for delta series
    if ((series == RideFile::kphd  || series == RideFile::wattsd || series == RideFile::cadd ||
        series == RideFile::nmd  || series == RideFile::hrd) && ride_bests.count() > 180) {
        ride_bests.resize(180);
        array.resize(180);
    } else {
        array.resize(ride_bests.count());
    }

    // bounds check, it might be empty!
    if (ride_bests.size()) {
        for (int i=ride_bests.size()-1; i; i--) {
            if (ride_bests[i] == 0) ride_bests[i]=last;
            else last = ride_bests[i];

            // convert from double to long, preserving the
            // precision by applying a multiplier
            array[i] = ride_bests[i]; // * decimals; -- we did that earlier
        }
    }
}

// self-contained static routine to perform the fast search algorithm
// on a single series of data, using ints only assuming data is in 1s 
// intervals with no data issues.
void RideFileCache::fastSearch(QVector<int>&input, QVector<int>&ride_bests, QVector<int>&ride_offsets)
{
    // use the raw C structure to reduce overhead and on stack
    // Although with MSVC we have no choice, sadly.
#ifdef Q_CC_MSVC
    data_t *dataseries_i = new data_t[input.count()+1];
#else
    data_t dataseries_i[input.count()+1];
#endif
    data_t acc=0;

    // resize output
    ride_bests.resize(input.count()+1);
    ride_offsets.resize(input.count()+1);

    // aggregate here, instead of using utility function
    int j=0;
    for (; j<input.count(); j++) {
        dataseries_i[j]=acc;
        acc+=input[j];
    }
    dataseries_i[j]=acc;

    // run the algorithm
    for (int i=1; i<input.count();) {

        int offset;
        data_t c=divided_max_mean(dataseries_i,input.count(),i,&offset);

        // snaffle it away
        data_t val = c / (data_t)i;

        // save away
        ride_bests[i] = val;
        ride_offsets[i] = offset;

        // increments to limit search scope
        if (i<120) i++;
        else if (i<600) i+= 2;
        else if (i<1200) i += 5;
        else if (i<3600) i += 20;
        else if (i<7200) i += 120;
        else i += 300;
    }
#ifdef Q_CC_MSVC
    delete[] dataseries_i;
#endif

    // since we minimise the search space over
    // longer durations we need to fill in the gaps
    int last=0;
    for (int i=ride_bests.size()-1; i; i--) {
        if (ride_bests[i] == 0) ride_bests[i]=last;
        else last = ride_bests[i];
    }
}

void
RideFileCache::computeDistribution(QVector<float> &array, RideFile::SeriesType series)
{
    RideFile::SeriesType baseSeries = series;

    if (series == RideFile::wattsKg)
        baseSeries = RideFile::watts;
    else if (series == RideFile::aPowerKg)
        baseSeries = RideFile::aPower;

    // there is a distinction between needing it present and using it in calcs
    RideFile::SeriesType needSeries = baseSeries;
    if (series == RideFile::kphd) needSeries = RideFile::kph;
    if (series == RideFile::wattsd) needSeries = RideFile::watts;
    if (series == RideFile::cadd) needSeries = RideFile::cad;
    if (series == RideFile::nmd) needSeries = RideFile::nm;
    if (series == RideFile::hrd) needSeries = RideFile::hr;
    if (series == RideFile::wbal) needSeries = RideFile::watts;

    // only bother if the data series is actually present
    if (ride->isDataPresent(needSeries) == false) return;

    // get zones that apply, if any
    int zoneRange = context->athlete->zones(ride->isRun()) ? context->athlete->zones(ride->isRun())->whichRange(ride->startTime().date()) : -1;
    int hrZoneRange = context->athlete->hrZones(ride->isRun()) ? context->athlete->hrZones(ride->isRun())->whichRange(ride->startTime().date()) : -1;
    int paceZoneRange = context->athlete->paceZones(ride->isSwim()) ? context->athlete->paceZones(ride->isSwim())->whichRange(ride->startTime().date()) : -1;

    if (zoneRange != -1) CP=context->athlete->zones(ride->isRun())->getCP(zoneRange);
    else CP=0;

    if (zoneRange != -1) WPRIME=context->athlete->zones(ride->isRun())->getWprime(zoneRange);
    else WPRIME=0;

    if (hrZoneRange != -1) LTHR=context->athlete->hrZones(ride->isRun())->getLT(hrZoneRange);
    else LTHR=0;

    if (paceZoneRange != -1) CV=context->athlete->paceZones(ride->isSwim())->getCV(paceZoneRange);
    else CV=0;

    // setup the array based upon the ride
    int decimals = decimalsFor(series); //RideFile::decimalsFor(series) ? 1 : 0;
    double min = RideFile::minimumFor(series) * pow(10, decimals);
    double max = RideFile::maximumFor(series) * pow(10, decimals);

    // lets resize the array to the right size
    // it will also initialise with a default value
    // which for longs is handily zero
    array.resize(max-min+1);

    // wbal uses the wprimeData, not the ridefile
    if (series == RideFile::wbal) {

        // set timeinzone to zero
        wbalTimeInZone.fill(0.0f, 4);
        array.fill(0.0f);
        int count = 0;

        // lets count them first then turn into percentages
        // after we have traversed all the data
        foreach(int value, ride->wprimeData()->ydata()) {

            // percent is PERCENT OF W' USED
            int percent = 100.0f - ((double (value) / WPRIME) * 100.0f);
            if (percent < 0.0f) percent = 0.0f;
            if (percent > 100.0f) percent = 100.0f;

            // increment counts
            array[percent]++;
            count++;

            // and zones in 1s increments
            if (percent <= 25.0f) wbalTimeInZone[0]++;
            else if (percent <= 50.0f) wbalTimeInZone[1]++;
            else if (percent <= 75.0f) wbalTimeInZone[2]++;
            else wbalTimeInZone[3]++;
        }

    } else {

        foreach(RideFilePoint *dp, ride->dataPoints()) {
            double value = dp->value(baseSeries);
            if (series == RideFile::wattsKg || series == RideFile::aPowerKg) {
                value /= ride->getWeight();
            }

            float lvalue = value * pow(10, decimals);

            // watts time in zone
            if (series == RideFile::watts && zoneRange != -1) {
                int index = context->athlete->zones(ride->isRun())->whichZone(zoneRange, dp->value(series));
                if (index >=0) wattsTimeInZone[index] += ride->recIntSecs();
            }

            // Polarized zones :- I(<0.85*CP), II (<CP and >0.85*CP), III (>CP)
            if (series == RideFile::watts && zoneRange != -1 && CP) {
                if (dp->value(series) < 1) // I zero watts
                    wattsCPTimeInZone[0] += ride->recIntSecs();
                else if (dp->value(series) < (CP*0.85f)) // I
                    wattsCPTimeInZone[1] += ride->recIntSecs();
                else if (dp->value(series) < CP) // II
                    wattsCPTimeInZone[2] += ride->recIntSecs();
                else // III
                    wattsCPTimeInZone[3] += ride->recIntSecs();
            }

            // hr time in zone
            if (series == RideFile::hr && hrZoneRange != -1) {
                int index = context->athlete->hrZones(ride->isRun())->whichZone(hrZoneRange, dp->value(series));
                if (index >= 0) hrTimeInZone[index] += ride->recIntSecs();
            }

            // Polarized zones :- I(<0.9*LTHR), II (<LTHR and >0.9*LTHR), III (>LTHR)
            if (series == RideFile::hr && hrZoneRange != -1 && LTHR) {
                if (dp->value(series) < 1) // I zero
                    hrCPTimeInZone[0] += ride->recIntSecs();
                else if (dp->value(series) < (LTHR*0.9f)) // I
                    hrCPTimeInZone[1] += ride->recIntSecs();
                else if (dp->value(series) < LTHR) // II
                    hrCPTimeInZone[2] += ride->recIntSecs();
                else // III
                    hrCPTimeInZone[3] += ride->recIntSecs();
            }

            // pace time in zone, only for running and swimming activities
            if (series == RideFile::kph && paceZoneRange != -1 && (ride->isRun() || ride->isSwim())) {
                int index = context->athlete->paceZones(ride->isSwim())->whichZone(paceZoneRange, dp->value(series));
                if (index >= 0) paceTimeInZone[index] += ride->recIntSecs();
            }

            // Polarized zones Run:- I(<0.9*CV), II (<CV and >0.9*CV), III (>CV)
            // Polarized zones Swim:- I(<0.975*CV), II (<CV and >0.975*CV), III (>CV)
            if (series == RideFile::kph && paceZoneRange != -1 && CV && (ride->isRun() || ride->isSwim())) {
                if (dp->value(series) < 0.1) // I zero
                    paceCPTimeInZone[0] += ride->recIntSecs();
                else if (ride->isRun() && dp->value(series) < (CV*0.9f)) // I for run
                    paceCPTimeInZone[1] += ride->recIntSecs();
                else if (ride->isSwim() && dp->value(series) < (CV*0.975f)) // I for swim
                    paceCPTimeInZone[1] += ride->recIntSecs();
                else if (dp->value(series) < CV) // II
                    paceCPTimeInZone[2] += ride->recIntSecs();
                else // III
                    paceCPTimeInZone[3] += ride->recIntSecs();
            }

            int offset = lvalue - min;
            if (offset >= 0 && offset < array.size()) array[offset] += ride->recIntSecs();
        }
    }
}

//
// AGGREGATE FOR A GIVEN DATE RANGE
//

// select and update bests
static void meanMaxAggregate(QVector<double> &into, QVector<double> &other, QVector<QDate>&dates, QDate rideDate)
{
    if (into.size() < other.size()) {
        into.resize(other.size());
        dates.resize(other.size());
    }

    for (int i=0; i<other.size(); i++)
        if (other[i] > into[i]) {
            into[i] = other[i];
            dates[i] = rideDate;
        }
}

// resize into and then sum the arrays
static void distAggregate(QVector<double> &into, QVector<double> &other)
{
    if (into.size() < other.size()) into.resize(other.size());
    for (int i=0; i<other.size(); i++) into[i] += other[i];

}

RideFileCache::RideFileCache(Context *context, QDate start, QDate end, bool filter, QStringList files, bool onhome, RideItem *rideItem)
               : start(start), end(end), incomplete(false), context(context), rideFileName(""), ride(0)
{

    // remember parameters for getting heat
    this->filter = filter;
    this->files = files;
    this->onhome = onhome;

    // Oh lets get from the cache if we can -- but not if filtered
    if (!filter && !context->isfiltered && !rideItem) {

        // oh and not if we're onhome and homefiltered
        if ((onhome && !context->ishomefiltered) || !onhome) {
            foreach(RideFileCache *p, context->athlete->cpxCache) {
                if (p->start == start && p->end == end) {
                    *this = *p;
                    return;
                }
            }
        }
    }

    // resize all the arrays to zero - expand as neccessary
    xPowerMeanMax.resize(0);
    npMeanMax.resize(0);
    wattsMeanMax.resize(0);
    heatMeanMax.resize(0);
    hrMeanMax.resize(0);
    cadMeanMax.resize(0);
    nmMeanMax.resize(0);
    kphMeanMax.resize(0);
    kphdMeanMax.resize(0);
    wattsdMeanMax.resize(0);
    caddMeanMax.resize(0);
    nmdMeanMax.resize(0);
    hrdMeanMax.resize(0);
    xPowerMeanMax.resize(0);
    npMeanMax.resize(0);
    vamMeanMax.resize(0);
    wattsKgMeanMax.resize(0);
    aPowerMeanMax.resize(0);
    aPowerKgMeanMax.resize(0);
    wattsDistribution.resize(0);
    hrDistribution.resize(0);
    cadDistribution.resize(0);
    nmDistribution.resize(0);
    kphDistribution.resize(0);
    xPowerDistribution.resize(0);
    npDistribution.resize(0);
    wattsKgDistribution.resize(0);
    aPowerDistribution.resize(0);
    smo2Distribution.resize(0);
    wbalDistribution.resize(0);

    // time in zone are fixed to 10 zone max
    wattsTimeInZone.resize(10);
    wattsCPTimeInZone.resize(4);
    hrTimeInZone.resize(10);
    hrCPTimeInZone.resize(4);
    paceTimeInZone.resize(10);
    paceCPTimeInZone.resize(4);
    wbalTimeInZone.resize(4);

    // set cursor busy whilst we aggregate -- bit of feedback
    // and less intrusive than a popup box
    context->mainWindow->setCursor(Qt::WaitCursor);

    // Iterate over the ride files (not the cpx files since they /might/ not
    // exist, or /might/ be out of date.
    foreach (RideItem *item, context->athlete->rideCache->rides()) {

        QDate rideDate = item->dateTime.date();

        if (((filter == true && files.contains(item->fileName)) || filter == false) &&
            rideDate >= start && rideDate <= end) {

            // skip globally filtered values
            if (context->isfiltered && !context->filters.contains(item->fileName)) continue;
            if (onhome && context->ishomefiltered && !context->homeFilters.contains(item->fileName)) continue;
            // skip other sports if rideItem is given
            if (rideItem && ((rideItem->isRun != item->isRun) || (rideItem->isSwim != item->isSwim))) continue;

            // get its cached values (will NOT! refresh if needed...)
            // the true means it will check only
            RideFileCache rideCache(context, context->athlete->home->activities().canonicalPath() + "/" + item->fileName, item->getWeight(), NULL, false, false);
            if (rideCache.incomplete == true) {
                // ack, data not available !
                incomplete = true;
            } else {

                // lets aggregate
                meanMaxAggregate(wattsMeanMaxDouble, rideCache.wattsMeanMaxDouble, wattsMeanMaxDate, rideDate);
                meanMaxAggregate(hrMeanMaxDouble, rideCache.hrMeanMaxDouble, hrMeanMaxDate, rideDate);
                meanMaxAggregate(cadMeanMaxDouble, rideCache.cadMeanMaxDouble, cadMeanMaxDate, rideDate);
                meanMaxAggregate(nmMeanMaxDouble, rideCache.nmMeanMaxDouble, nmMeanMaxDate, rideDate);
                meanMaxAggregate(kphMeanMaxDouble, rideCache.kphMeanMaxDouble, kphMeanMaxDate, rideDate);
                meanMaxAggregate(kphdMeanMaxDouble, rideCache.kphdMeanMaxDouble, kphdMeanMaxDate, rideDate);
                meanMaxAggregate(wattsdMeanMaxDouble, rideCache.wattsdMeanMaxDouble, wattsdMeanMaxDate, rideDate);
                meanMaxAggregate(caddMeanMaxDouble, rideCache.caddMeanMaxDouble, caddMeanMaxDate, rideDate);
                meanMaxAggregate(nmdMeanMaxDouble, rideCache.nmdMeanMaxDouble, nmdMeanMaxDate, rideDate);
                meanMaxAggregate(hrdMeanMaxDouble, rideCache.hrdMeanMaxDouble, hrdMeanMaxDate, rideDate);
                meanMaxAggregate(xPowerMeanMaxDouble, rideCache.xPowerMeanMaxDouble, xPowerMeanMaxDate, rideDate);
                meanMaxAggregate(npMeanMaxDouble, rideCache.npMeanMaxDouble, npMeanMaxDate, rideDate);
                meanMaxAggregate(vamMeanMaxDouble, rideCache.vamMeanMaxDouble, vamMeanMaxDate, rideDate);
                meanMaxAggregate(wattsKgMeanMaxDouble, rideCache.wattsKgMeanMaxDouble, wattsKgMeanMaxDate, rideDate);
                meanMaxAggregate(aPowerMeanMaxDouble, rideCache.aPowerMeanMaxDouble, aPowerMeanMaxDate, rideDate);
                meanMaxAggregate(aPowerKgMeanMaxDouble, rideCache.aPowerKgMeanMaxDouble, aPowerKgMeanMaxDate, rideDate);

                distAggregate(wattsDistributionDouble, rideCache.wattsDistributionDouble);
                distAggregate(hrDistributionDouble, rideCache.hrDistributionDouble);
                distAggregate(cadDistributionDouble, rideCache.cadDistributionDouble);
                distAggregate(gearDistributionDouble, rideCache.gearDistributionDouble);
                distAggregate(nmDistributionDouble, rideCache.nmDistributionDouble);
                distAggregate(kphDistributionDouble, rideCache.kphDistributionDouble);
                distAggregate(xPowerDistributionDouble, rideCache.xPowerDistributionDouble);
                distAggregate(npDistributionDouble, rideCache.npDistributionDouble);
                distAggregate(wattsKgDistributionDouble, rideCache.wattsKgDistributionDouble);
                distAggregate(aPowerDistributionDouble, rideCache.aPowerDistributionDouble);
                distAggregate(smo2DistributionDouble, rideCache.smo2DistributionDouble);
                distAggregate(wbalDistributionDouble, rideCache.wbalDistributionDouble);

                // cumulate timeinzones
                for (int i=0; i<10; i++) {
                    paceTimeInZone[i] += rideCache.paceTimeInZone[i];
                    hrTimeInZone[i] += rideCache.hrTimeInZone[i];
                    wattsTimeInZone[i] += rideCache.wattsTimeInZone[i];
                    if (i<4) {
                        paceCPTimeInZone[i] += rideCache.paceCPTimeInZone[i];
                        hrCPTimeInZone[i] += rideCache.hrCPTimeInZone[i];
                        wattsCPTimeInZone[i] += rideCache.wattsCPTimeInZone[i];
                        wbalTimeInZone[i] += rideCache.wbalTimeInZone[i];
                    }
                }
            }
        }
    }

    // set the cursor back to normal
    context->mainWindow->setCursor(Qt::ArrowCursor);

    // lets add to the cache for others to re-use -- but not if filtered or incomplete
    if (incomplete == false && !context->isfiltered && (!context->ishomefiltered || !onhome) && !filter) {

        if (context->athlete->cpxCache.count() > maxcache) {
            delete(context->athlete->cpxCache.at(0));
            context->athlete->cpxCache.removeAt(0);
        }
        context->athlete->cpxCache.append(new RideFileCache(this));
    }
}

//
// Get heat mean max -- if an aggregated curve
//
QVector<float> &RideFileCache::heatMeanMaxArray()
{
    // not aggregated or already done it return the result
    if (ride || heatMeanMax.count()) return heatMeanMax;

    // make it big enough
    heatMeanMax.resize(wattsMeanMaxDouble.size());

    // ok, we need to iterate again and compute heat based upon
    // how close to the absolute best we've got
    foreach(RideItem *item, context->athlete->rideCache->rides()) {

        QDate rideDate = item->dateTime.date();

        if (((filter == true && files.contains(item->fileName)) || filter == false) &&
            rideDate >= start && rideDate <= end) {

            // skip globally filtered values
            if (context->isfiltered && !context->filters.contains(item->fileName)) continue;
            if (onhome && context->ishomefiltered && !context->homeFilters.contains(item->fileName)) continue;

            // get its cached values (will refresh if needed...)
            RideFileCache rideCache(context, context->athlete->home->activities().canonicalPath() + "/" + item->fileName, item->getWeight());

            for(int i=0; i<rideCache.wattsMeanMaxDouble.count() && i<wattsMeanMaxDouble.count(); i++) {

                // is it within 10% of the best we have ?
                if (rideCache.wattsMeanMaxDouble[i] >= (0.9f * wattsMeanMaxDouble[i]))
                    heatMeanMax[i] = heatMeanMax[i] + 1;
            }
        }
    }

    return heatMeanMax;
}

//
// PERSISTANCE
//
void
RideFileCache::serialize(QDataStream *out)
{
    RideFileCacheHeader head;

    // write header
    head.version = RideFileCacheVersion;
    head.crc = crc;
    head.CP = CP;
    head.WPRIME = WPRIME;
    head.LTHR = LTHR;
    head.CV = CV;
    head.WEIGHT = WEIGHT;

    head.wattsMeanMaxCount = wattsMeanMax.size();
    head.hrMeanMaxCount = hrMeanMax.size();
    head.cadMeanMaxCount = cadMeanMax.size();
    head.nmMeanMaxCount = nmMeanMax.size();
    head.kphMeanMaxCount = kphMeanMax.size();
    head.kphdMeanMaxCount = kphdMeanMax.size();
    head.wattsdMeanMaxCount = wattsdMeanMax.size();
    head.caddMeanMaxCount = caddMeanMax.size();
    head.nmdMeanMaxCount = nmdMeanMax.size();
    head.hrdMeanMaxCount = hrdMeanMax.size();
    head.xPowerMeanMaxCount = xPowerMeanMax.size();
    head.npMeanMaxCount = npMeanMax.size();
    head.vamMeanMaxCount = vamMeanMax.size();
    head.wattsKgMeanMaxCount = wattsKgMeanMax.size();
    head.aPowerMeanMaxCount = aPowerMeanMax.size();
    head.aPowerKgMeanMaxCount = aPowerKgMeanMax.size();
    head.wattsDistCount = wattsDistribution.size();
    head.xPowerDistCount = xPowerDistribution.size();
    head.npDistCount = xPowerDistribution.size();
    head.hrDistCount = hrDistribution.size();
    head.cadDistCount = cadDistribution.size();
    head.gearDistCount = gearDistribution.size();
    head.nmDistrCount = nmDistribution.size();
    head.kphDistCount = kphDistribution.size();
    head.wattsKgDistCount = wattsKgDistribution.size();
    head.aPowerDistCount = aPowerDistribution.size();
    head.smo2DistCount = smo2Distribution.size();
    head.wbalDistCount = wbalDistribution.size();

    out->writeRawData((const char *) &head, sizeof(head));

    // write meanmax
    out->writeRawData((const char *) wattsMeanMax.data(), sizeof(float) * wattsMeanMax.size());
    out->writeRawData((const char *) wattsKgMeanMax.data(), sizeof(float) * wattsKgMeanMax.size());
    out->writeRawData((const char *) hrMeanMax.data(), sizeof(float) * hrMeanMax.size());
    out->writeRawData((const char *) cadMeanMax.data(), sizeof(float) * cadMeanMax.size());
    out->writeRawData((const char *) nmMeanMax.data(), sizeof(float) * nmMeanMax.size());
    out->writeRawData((const char *) kphMeanMax.data(), sizeof(float) * kphMeanMax.size());
    out->writeRawData((const char *) kphdMeanMax.data(), sizeof(float) * kphdMeanMax.size());
    out->writeRawData((const char *) wattsdMeanMax.data(), sizeof(float) * wattsdMeanMax.size());
    out->writeRawData((const char *) caddMeanMax.data(), sizeof(float) * caddMeanMax.size());
    out->writeRawData((const char *) nmdMeanMax.data(), sizeof(float) * nmdMeanMax.size());
    out->writeRawData((const char *) hrdMeanMax.data(), sizeof(float) * hrdMeanMax.size());
    out->writeRawData((const char *) xPowerMeanMax.data(), sizeof(float) * xPowerMeanMax.size());
    out->writeRawData((const char *) npMeanMax.data(), sizeof(float) * npMeanMax.size());
    out->writeRawData((const char *) vamMeanMax.data(), sizeof(float) * vamMeanMax.size());
    out->writeRawData((const char *) aPowerMeanMax.data(), sizeof(float) * aPowerMeanMax.size());
    out->writeRawData((const char *) aPowerKgMeanMax.data(), sizeof(float) * aPowerKgMeanMax.size());


    // write dist
    out->writeRawData((const char *) wattsDistribution.data(), sizeof(float) * wattsDistribution.size());
    out->writeRawData((const char *) hrDistribution.data(), sizeof(float) * hrDistribution.size());
    out->writeRawData((const char *) cadDistribution.data(), sizeof(float) * cadDistribution.size());
    out->writeRawData((const char *) gearDistribution.data(), sizeof(float) * gearDistribution.size());
    out->writeRawData((const char *) nmDistribution.data(), sizeof(float) * nmDistribution.size());
    out->writeRawData((const char *) kphDistribution.data(), sizeof(float) * kphDistribution.size());
    out->writeRawData((const char *) xPowerDistribution.data(), sizeof(float) * xPowerDistribution.size());
    out->writeRawData((const char *) npDistribution.data(), sizeof(float) * npDistribution.size());
    out->writeRawData((const char *) wattsKgDistribution.data(), sizeof(float) * wattsKgDistribution.size());
    out->writeRawData((const char *) aPowerDistribution.data(), sizeof(float) * aPowerDistribution.size());
    out->writeRawData((const char *) smo2Distribution.data(), sizeof(float) * smo2Distribution.size());
    out->writeRawData((const char *) wbalDistribution.data(), sizeof(float) * wbalDistribution.size());

    // time in zone
    out->writeRawData((const char *) wattsTimeInZone.data(), sizeof(float) * wattsTimeInZone.size());
    out->writeRawData((const char *) wattsCPTimeInZone.data(), sizeof(float) * wattsCPTimeInZone.size());
    out->writeRawData((const char *) hrTimeInZone.data(), sizeof(float) * hrTimeInZone.size());
    out->writeRawData((const char *) hrCPTimeInZone.data(), sizeof(float) * hrCPTimeInZone.size());
    out->writeRawData((const char *) paceTimeInZone.data(), sizeof(float) * paceTimeInZone.size());
    out->writeRawData((const char *) paceCPTimeInZone.data(), sizeof(float) * paceCPTimeInZone.size());
    out->writeRawData((const char *) wbalTimeInZone.data(), sizeof(float) * wbalTimeInZone.size());
}

void
RideFileCache::readCache()
{
    RideFileCacheHeader head;
    QFile cacheFile(cacheFileName);

    if (cacheFile.open(QIODevice::ReadOnly) == true) {
        QDataStream inFile(&cacheFile);

        inFile.readRawData((char *) &head, sizeof(head));

        // resize all the arrays to fit
        wattsMeanMax.resize(head.wattsMeanMaxCount);
        hrMeanMax.resize(head.hrMeanMaxCount);
        cadMeanMax.resize(head.cadMeanMaxCount);
        nmMeanMax.resize(head.nmMeanMaxCount);
        kphMeanMax.resize(head.kphMeanMaxCount);
        kphdMeanMax.resize(head.kphdMeanMaxCount);
        wattsdMeanMax.resize(head.wattsdMeanMaxCount);
        caddMeanMax.resize(head.caddMeanMaxCount);
        nmdMeanMax.resize(head.nmdMeanMaxCount);
        hrdMeanMax.resize(head.hrdMeanMaxCount);
        npMeanMax.resize(head.npMeanMaxCount);
        vamMeanMax.resize(head.vamMeanMaxCount);
        xPowerMeanMax.resize(head.xPowerMeanMaxCount);
        wattsKgMeanMax.resize(head.wattsKgMeanMaxCount);
        aPowerMeanMax.resize(head.aPowerMeanMaxCount);
        aPowerKgMeanMax.resize(head.aPowerKgMeanMaxCount);

        wattsDistribution.resize(head.wattsDistCount);
        hrDistribution.resize(head.hrDistCount);
        cadDistribution.resize(head.cadDistCount);
        gearDistribution.resize(head.gearDistCount);
        nmDistribution.resize(head.nmDistrCount);
        kphDistribution.resize(head.kphDistCount);
        xPowerDistribution.resize(head.xPowerDistCount);
        npDistribution.resize(head.npDistCount);
        wattsKgDistribution.resize(head.wattsKgDistCount);
        aPowerDistribution.resize(head.aPowerDistCount);
        smo2Distribution.resize(head.smo2DistCount);
        wbalDistribution.resize(head.wbalDistCount);

        // read in the arrays
        inFile.readRawData((char *) wattsMeanMax.data(), sizeof(float) * wattsMeanMax.size());
        inFile.readRawData((char *) wattsKgMeanMax.data(), sizeof(float) * wattsKgMeanMax.size());
        inFile.readRawData((char *) hrMeanMax.data(), sizeof(float) * hrMeanMax.size());
        inFile.readRawData((char *) cadMeanMax.data(), sizeof(float) * cadMeanMax.size());
        inFile.readRawData((char *) nmMeanMax.data(), sizeof(float) * nmMeanMax.size());
        inFile.readRawData((char *) kphMeanMax.data(), sizeof(float) * kphMeanMax.size());
        inFile.readRawData((char *) kphdMeanMax.data(), sizeof(float) * kphdMeanMax.size());
        inFile.readRawData((char *) wattsdMeanMax.data(), sizeof(float) * wattsdMeanMax.size());
        inFile.readRawData((char *) caddMeanMax.data(), sizeof(float) * caddMeanMax.size());
        inFile.readRawData((char *) nmdMeanMax.data(), sizeof(float) * nmdMeanMax.size());
        inFile.readRawData((char *) hrdMeanMax.data(), sizeof(float) * hrdMeanMax.size());
        inFile.readRawData((char *) xPowerMeanMax.data(), sizeof(float) * xPowerMeanMax.size());
        inFile.readRawData((char *) npMeanMax.data(), sizeof(float) * npMeanMax.size());
        inFile.readRawData((char *) vamMeanMax.data(), sizeof(float) * vamMeanMax.size());
        inFile.readRawData((char *) aPowerMeanMax.data(), sizeof(float) * aPowerMeanMax.size());
        inFile.readRawData((char *) aPowerKgMeanMax.data(), sizeof(float) * aPowerKgMeanMax.size());


        // write dist
        inFile.readRawData((char *) wattsDistribution.data(), sizeof(float) * wattsDistribution.size());
        inFile.readRawData((char *) hrDistribution.data(), sizeof(float) * hrDistribution.size());
        inFile.readRawData((char *) cadDistribution.data(), sizeof(float) * cadDistribution.size());
        inFile.readRawData((char *) gearDistribution.data(), sizeof(float) * gearDistribution.size());
        inFile.readRawData((char *) nmDistribution.data(), sizeof(float) * nmDistribution.size());
        inFile.readRawData((char *) kphDistribution.data(), sizeof(float) * kphDistribution.size());
        inFile.readRawData((char *) xPowerDistribution.data(), sizeof(float) * xPowerDistribution.size());
        inFile.readRawData((char *) npDistribution.data(), sizeof(float) * npDistribution.size());
        inFile.readRawData((char *) wattsKgDistribution.data(), sizeof(float) * wattsKgDistribution.size());
        inFile.readRawData((char *) aPowerDistribution.data(), sizeof(float) * aPowerDistribution.size());
        inFile.readRawData((char *) smo2Distribution.data(), sizeof(float) * smo2Distribution.size());
        inFile.readRawData((char *) wbalDistribution.data(), sizeof(float) * wbalDistribution.size());

        // time in zone
        inFile.readRawData((char *) wattsTimeInZone.data(), sizeof(float) * 10);
        inFile.readRawData((char *) wattsCPTimeInZone.data(), sizeof(float) * 4);
        inFile.readRawData((char *) hrTimeInZone.data(), sizeof(float) * 10);
        inFile.readRawData((char *) hrCPTimeInZone.data(), sizeof(float) * 4);
        inFile.readRawData((char *) paceTimeInZone.data(), sizeof(float) * 10);
        inFile.readRawData((char *) paceCPTimeInZone.data(), sizeof(float) * 4);
        inFile.readRawData((char *) wbalTimeInZone.data(), sizeof(float) * 4);

        // setup the doubles the users use
        doubleArray(wattsMeanMaxDouble, wattsMeanMax, RideFile::watts);
        doubleArray(hrMeanMaxDouble, hrMeanMax, RideFile::hr);
        doubleArray(cadMeanMaxDouble, cadMeanMax, RideFile::cad);
        doubleArray(nmMeanMaxDouble, nmMeanMax, RideFile::nm);
        doubleArray(kphMeanMaxDouble, kphMeanMax, RideFile::kph);
        doubleArray(kphdMeanMaxDouble, kphdMeanMax, RideFile::kphd);
        doubleArray(wattsdMeanMaxDouble, wattsdMeanMax, RideFile::wattsd);
        doubleArray(caddMeanMaxDouble, caddMeanMax, RideFile::cadd);
        doubleArray(nmdMeanMaxDouble, nmdMeanMax, RideFile::nmd);
        doubleArray(hrdMeanMaxDouble, hrdMeanMax, RideFile::hrd);
        doubleArray(npMeanMaxDouble, npMeanMax, RideFile::IsoPower);
        doubleArray(vamMeanMaxDouble, vamMeanMax, RideFile::vam);
        doubleArray(xPowerMeanMaxDouble, xPowerMeanMax, RideFile::xPower);
        doubleArray(wattsKgMeanMaxDouble, wattsKgMeanMax, RideFile::wattsKg);
        doubleArray(aPowerMeanMaxDouble, aPowerMeanMax, RideFile::aPower);
        doubleArray(aPowerKgMeanMaxDouble, aPowerKgMeanMax, RideFile::aPowerKg);

        doubleArrayForDistribution(wattsDistributionDouble, wattsDistribution);
        doubleArrayForDistribution(hrDistributionDouble, hrDistribution);
        doubleArrayForDistribution(cadDistributionDouble, cadDistribution);
        doubleArrayForDistribution(gearDistributionDouble, gearDistribution);
        doubleArrayForDistribution(nmDistributionDouble, nmDistribution);
        doubleArrayForDistribution(kphDistributionDouble, kphDistribution);
        doubleArrayForDistribution(xPowerDistributionDouble, xPowerDistribution);
        doubleArrayForDistribution(npDistributionDouble, npDistribution);
        doubleArrayForDistribution(wattsKgDistributionDouble, wattsKgDistribution);
        doubleArrayForDistribution(aPowerDistributionDouble, aPowerDistribution);
        doubleArrayForDistribution(smo2DistributionDouble, smo2Distribution);
        doubleArrayForDistribution(wbalDistributionDouble, wbalDistribution);

        cacheFile.close();
    }
}

// unpack the longs into a double array
void RideFileCache::doubleArray(QVector<double> &into, QVector<float> &from, RideFile::SeriesType series)
{
    double divisor = pow(10, decimalsFor(series)); // ? 10 : 1;
    into.resize(from.size());
    for(int i=0; i<from.size(); i++) into[i] = double(from[i]) / divisor;

    return;
}

// for Distribution Series the values in Long/Float are ALWAYS Seconds (therefore no decimals adjustment calculation required)
void RideFileCache::doubleArrayForDistribution(QVector<double> &into, QVector<float> &from)
{
    into.resize(from.size());
    for(int i=0; i<from.size(); i++) into[i] = double(from[i]);

    return;
}

// get a list of all values for the spec, series and duration and see where the value ranks
int RideFileCache::rank(Context *context, RideFile::SeriesType series, int duration, 
         double value, Specification spec, int &of)
{

    QList<double> values;

    foreach(RideItem*item, context->athlete->rideCache->rides()) {
        if (!spec.pass(item)) continue;

        // get the best for this one
        values << RideFileCache::best(context, item->fileName, series, duration);
    }

    // sort the list
    qSort(values.begin(), values.end(), qGreater<double>());

    // get the ranking and count
    of = values.count();

    // where do we fit?
    for (int i=0; i<values.count(); i++)
        if (values.at(i) <= value)
            return (i+1);

    return values.count();
}

double 
RideFileCache::best(Context *context, QString filename, RideFile::SeriesType series, int duration)
{
    // read the header
    QFileInfo rideFileInfo(context->athlete->home->activities().canonicalPath() + "/" + filename);
    QString cacheFileName(context->athlete->home->cache().canonicalPath() + "/" + rideFileInfo.baseName() + ".cpx");
    QFileInfo cacheFileInfo(cacheFileName);

    // head
    RideFileCacheHeader head;
    QFile cacheFile(cacheFileName);

    if (cacheFile.open(QIODevice::ReadOnly | QIODevice::Unbuffered) == true) {
        QDataStream inFile(&cacheFile);
        inFile.readRawData((char *) &head, sizeof(head));

        // out of date or not enough samples
        if (head.version != RideFileCacheVersion || duration > countForMeanMax(head, series)) {
            cacheFile.close();
            return 0;
        }

        // jump to correct offset
        long offset = offsetForMeanMax(head, series) + (sizeof(float) * (duration));
        inFile.skipRawData(offset);

        float readhere = 0;
        inFile.readRawData((char*)&readhere, sizeof(float));
        cacheFile.close();

        double divisor = pow(10, decimalsFor(series)); // ? 10 : 1;
        return readhere / divisor; // will convert to double
    }

    return 0;
}

int 
RideFileCache::tiz(Context *context, QString filename, RideFile::SeriesType series, int zone)
{
    if (zone < 1 || zone > 10) return 0;

    // read the header
    QFileInfo rideFileInfo(context->athlete->home->activities().canonicalPath() + "/" + filename);
    QString cacheFileName(context->athlete->home->cache().canonicalPath() + "/" + rideFileInfo.baseName() + ".cpx");
    QFileInfo cacheFileInfo(cacheFileName);

    // head
    RideFileCacheHeader head;
    QFile cacheFile(cacheFileName);

    if (cacheFile.open(QIODevice::ReadOnly | QIODevice::Unbuffered) == true) {
        QDataStream inFile(&cacheFile);
        inFile.readRawData((char *) &head, sizeof(head));

        // out of date 
        if (head.version != RideFileCacheVersion) {
            cacheFile.close();
            return 0;
        }

        // jump to correct offset
        long offset = offsetForTiz(head, series) + (sizeof(float) * (zone-1));
        inFile.skipRawData(offset);

        float readhere = 0;
        inFile.readRawData((char*)&readhere, sizeof(float));
        cacheFile.close();

        return readhere; // will convert to double
    }

    return 0;
}

// get best values (as passed in the list of MetricDetails between the dates specified
// and return as an array of RideBests)
//
// this is to 're-use' the metric api (especially in the LTM code) for passing back multiple
// bests across multiple rides in one object. We do this so we can optimise the read/seek across
// the CPX files within a single call.
//
// We order the bests requested in the order they will appear in the CPX file so we can open
// and seek forward to each value before putting into the summary metric. Since it is placed
// on the stack as a return parameter we also don't need to worry about memory allocation just
// like the metric code works.
// 
//
QList<RideBest>
RideFileCache::getAllBestsFor(Context *context, QList<MetricDetail> metrics, Specification specification)
{
    QList<RideBest> results;
    QList<MetricDetail> worklist;

    // lets get a worklist
    foreach(MetricDetail x, metrics) {
        if (x.type == METRIC_BEST) {
            worklist << x;
        }
    }
    if (worklist.count() == 0) return results; // no work to do

    // get a list of rides & iterate over them
    foreach(RideItem *ride, context->athlete->rideCache->rides()) {

        if (!specification.pass(ride)) continue;

        // get the ride cache name

        // CPX ?
        QFileInfo rideFileInfo(context->athlete->home->activities().canonicalPath() + "/" + ride->fileName);
        QString cacheFileName(context->athlete->home->cache().canonicalPath() + "/" + rideFileInfo.baseName() + ".cpx");
        RideFileCacheHeader head;
        QFile cacheFile(cacheFileName);

        // open ok ?
        if (cacheFile.open(QIODevice::ReadOnly | QIODevice::Unbuffered) == false) continue;

        // get header
        QDataStream inFile(&cacheFile);
        inFile.readRawData((char *) &head, sizeof(head));

        // out of date - just skip
        if (head.version != RideFileCacheVersion) {
            cacheFile.close();
            continue;
        }

        RideBest add;
        add.setFileName(ride->fileName);
        add.setRideDate(ride->dateTime);

        // work through the worklist adding each best
        foreach (MetricDetail workitem, worklist) {

            int seconds = workitem.duration * workitem.duration_units;
            float value;

            if (seconds > countForMeanMax(head, workitem.series)) value=0.0;
            else {

                // get the values and place into the summarymetric map
                long offset = offsetForMeanMax(head, workitem.series) + 
                              (sizeof(head)) +
                              (sizeof(float) * ((workitem.duration*workitem.duration_units)));

                cacheFile.seek(qint64(offset));
                inFile.readRawData((char*)&value, sizeof(float));
                double divisor = pow(10, decimalsFor(workitem.series));
                value = value / divisor;

            }
            add.setForSymbol(workitem.bestSymbol, value);

        }

        // add to the results
        results << add;

        // close CPX file
        cacheFile.close();
    }

    // all done, return results
    return results;
}

static
const RideMetric *metricForSymbol(QString symbol)
{
    const RideMetricFactory &factory = RideMetricFactory::instance();
    return factory.rideMetric(symbol);
}

double
RideBest::getForSymbol(QString symbol, bool metric) const
{
    if (metric) return value.value(symbol, 0.0);
    else {
        const RideMetric *m = metricForSymbol(symbol);
        double metricValue = value.value(symbol, 0.0);
        metricValue *= m->conversion();
        metricValue += m->conversionSum();
        return metricValue;
    }
}

int
RideFileCache::bestTime(double km)
{
    // divisor for series and conversion from secs to hours
    double divisor = pow(10, decimalsFor(RideFile::kph)) * 3600.0;
    // linear search over kph mean max array
    int secs = 0;
    while (secs < kphMeanMax.count() &&
           double(kphMeanMax[secs] * secs) / divisor < km) secs++;
    if (secs < kphMeanMax.count()) return secs;
    return RideFile::NIL;
}
