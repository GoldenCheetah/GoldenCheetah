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
#include <math.h> // for pow()
#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QtAlgorithms> // for qStableSort

// cache from ride
RideFileCache::RideFileCache(MainWindow *main, QString fileName, RideFile *passedride, bool check) :
               main(main), rideFileName(fileName), ride(passedride)
{
    // resize all the arrays to zero
    wattsMeanMax.resize(0);
    hrMeanMax.resize(0);
    cadMeanMax.resize(0);
    nmMeanMax.resize(0);
    kphMeanMax.resize(0);
    xPowerMeanMax.resize(0);
    npMeanMax.resize(0);
    wattsDistribution.resize(0);
    hrDistribution.resize(0);
    cadDistribution.resize(0);
    nmDistribution.resize(0);
    kphDistribution.resize(0);
    xPowerDistribution.resize(0);
    npDistribution.resize(0);

    // Get info for ride file and cache file
    QFileInfo rideFileInfo(rideFileName);
    cacheFileName = rideFileInfo.path() + "/" + rideFileInfo.baseName() + ".cpx";
    QFileInfo cacheFileInfo(cacheFileName);

    // is it up-to-date?
    if (cacheFileInfo.exists() && rideFileInfo.lastModified() < cacheFileInfo.lastModified() &&
        cacheFileInfo.size() != 0) {
        if (check == false) readCache(); // if check is false we aren't just checking
        return;
    }

    // not up-to-date we need to refresh from the ridefile
    if (ride) {

        // we got passed the ride - so update from that
        refreshCache();

    } else {

        // we need to open it to update since we were not passed one
        QStringList errors;
        QFile file(rideFileName);

        ride = RideFileFactory::instance().openRideFile(main, file, errors);

        if (ride) {
            refreshCache();
            delete ride;
        }
        ride = 0;
    }
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

        case RideFile::xPower:
            return xPowerMeanMaxDate;
            break;

        case RideFile::NP:
            return npMeanMaxDate;
            break;

        default:
            //? dunno give em power anyway
            return wattsMeanMaxDate;
            break;
    }
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

        case RideFile::xPower:
            return xPowerMeanMaxDouble;
            break;

        case RideFile::NP:
            return npMeanMaxDouble;
            break;

        default:
            //? dunno give em power anyway
            return wattsMeanMaxDouble;
            break;
    }
}


//
// COMPUTATION
//
void
RideFileCache::refreshCache()
{
    static bool writeerror=false;

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
    MeanMaxComputer thread1(ride, npMeanMax, RideFile::NP); thread1.start();
    MeanMaxComputer thread2(ride, xPowerMeanMax, RideFile::xPower); thread2.start();
    MeanMaxComputer thread3(ride, wattsMeanMax, RideFile::watts); thread3.start();
    MeanMaxComputer thread4(ride, hrMeanMax, RideFile::hr); thread4.start();
    MeanMaxComputer thread5(ride, cadMeanMax, RideFile::cad); thread5.start();
    MeanMaxComputer thread6(ride, nmMeanMax, RideFile::nm); thread6.start();
    MeanMaxComputer thread7(ride, kphMeanMax, RideFile::kph); thread7.start();
    MeanMaxComputer thread8(ride, xPowerMeanMax, RideFile::xPower); thread8.start();
    MeanMaxComputer thread9(ride, npMeanMax, RideFile::NP); thread9.start();

    // all the different distributions
#if 0
    MeanMaxComputer thread10(ride, npDistribution, RideFile::NP); thread10.start();
    MeanMaxComputer thread11(ride, xPowerDistribution, RideFile::xPower); thread11.start();
#endif
    computeDistribution(wattsDistribution, RideFile::watts);
    computeDistribution(hrDistribution, RideFile::hr);
    computeDistribution(cadDistribution, RideFile::cad);
    computeDistribution(nmDistribution, RideFile::nm);
    computeDistribution(kphDistribution, RideFile::kph);
    computeDistribution(xPowerDistribution, RideFile::xPower);
    computeDistribution(npDistribution, RideFile::NP);

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
#if 0
    thread10.wait();
    thread11.wait();
#endif
}

void
MeanMaxComputer::run()
{
    // only bother if the data series is actually present
    if (ride->isDataPresent(series) == false) return;

    // if we want decimal places only keep to 1 dp max
    // this is a factor that is applied at the end to
    // convert from high-precision double to long
    // e.g. 145.456 becomes 1455 if we want decimals
    // and becomes 145 if we don't
    double decimals = RideFile::decimalsFor(series) ? 10 : 1;

    // decritize the data series - seems wrong, since it just
    // rounds to the nearest second - what if the recIntSecs
    // is less than a second? Has been used for a long while
    // so going to leave in tact for now - apart from the
    // addition of code to fill in gaps in recording since
    // they affect the NP/xPower algorithm badly and will skew
    // the calculations of >6m since windowsize is used to
    // determine segment duration rather than examining the
    // timestamps on each sample
    cpintdata data;
    data.rec_int_ms = (int) round(ride->recIntSecs() * 1000.0);
    double lastsecs = -1;
    foreach (const RideFilePoint *p, ride->dataPoints()) {

        // fill in any gaps in recording - use same dodgy rounding as before
        if (lastsecs != -1) {
            int count = (p->secs - lastsecs - ride->recIntSecs()) / ride->recIntSecs();
            for(int i=0; i<count; i++) 
                data.points.append(cpintpoint(round(lastsecs+((i+1)*ride->recIntSecs() *1000.0)/1000), 0));
        }
        lastsecs = p->secs;

        double secs = round(p->secs * 1000.0) / 1000;
        if (secs > 0) data.points.append(cpintpoint(secs, (int) round(p->value(series))));
    }
    int total_secs = (int) ceil(data.points.back().secs);

    // don't allow data more than two days
    // was one week, but no single ride is longer
    // than 2 days, even if you are doing RAAM
    if (total_secs > 2*24*60*60) return;

    // ride_bests is used to temporarily hold
    // the computed best intervals since I do
    // not want to disturb the code at this stage
    QVector <double> ride_bests(total_secs + 1);

    // loop through the decritized data from top
    // FIRST 5 MINUTES DO BESTS FOR EVERY SECOND
    for (int i = 0; i < data.points.size() - 1; ++i) {

        cpintpoint *p = &data.points[i];

        double sum = 0.0;
        double prev_secs = p->secs;

        // from current point to end loop over remaining points
        // look at every duration int seconds up to 300 seconds (5 minutes)
        for (int j = i + 1; j < data.points.size() && data.points[j].secs - data.points[i].secs <= 360 ; ++j) {

            cpintpoint *q = &data.points[j];

            sum += data.rec_int_ms / 1000.0 * q->value;
            double dur_secs = q->secs - p->secs;
            double avg = sum / dur_secs;
            int dur_secs_top = (int) floor(dur_secs);
            int dur_secs_bot = qMax((int) floor(dur_secs - data.rec_int_ms / 1000.0), 0);

            // loop over our bests (1 for every second of the ride)
            // to see if we have a new best
            for (int k = dur_secs_top; k > dur_secs_bot; --k) {
                if (ride_bests[k] < avg) ride_bests[k] = avg;
            }
            prev_secs = q->secs;
        }
    }

    // NOW DO BESTS FOR EVERY 60s 
    // BETWEEN 6mins and the rest of the ride
    //
    // because we are dealing with longer durations we
    // can afford to be less sensitive to missed data
    // and sample rates - so we can downsample the sample
    // data now to 5s samples - but if the recording interval
    // is > 5s we won't bother, just set the interval used to
    // whatever the sample rate is for the device.
    QVector<double> downsampled(0);
    double samplerate;

    // moving to 5s samples would INCREASE the work...
    if (ride->recIntSecs() >= 5) {

        samplerate = ride->recIntSecs();
        for (int i=0; i<data.points.size(); i++)
            downsampled.append(data.points[i].value);

    } else {

        // moving to 10s samples is DECREASING the work...
        samplerate = 5;

        // we are downsampling to 10s
        long five=5; // start at 1st 10s sample
        double fivesum=0;
        int fivecount=0;
        for (int i=0; i<data.points.size(); i++) {

            if (data.points[i].secs <= five) {

                fivesum += data.points[i].value;
                fivecount++;

            } else {

                downsampled.append(fivesum / fivecount);
                fivecount = 1;
                fivesum = data.points[i].value;
                five += 5;
            }
        }
    }
//qDebug()<<"downsampled to "<<samplerate <<"second samples, ride duration="<<data.points.last().secs <<"have"<<downsampled.size()<<"samples";

    // now we have downsampled lets find bests for every 20s
    // starting at 6mins
    for (int slice = 360; slice < ride_bests.size(); slice += 20) {

        QVector<double> sums(downsampled.size());
        int windowsize = slice / samplerate;
        int index=0;
        double sum=0;

        for (int i=0; i<downsampled.size(); i++) {
            sum += downsampled[i];

            if (i>windowsize) {
                sum -= downsampled[i-windowsize];
                sums[index++] = sum;
            }
        }
        qSort(sums.begin(), sums.end());
        ride_bests[slice] = sums.last() / windowsize;
    }

    // XXX Commented out since it just 'smooths' the drop
    //     off in CPs which is actually quite enlightening
    //     LEFT HERE IN CASE IT IS IMPORTANT!
#if 0
    // avoid decreasing work with increasing duration
    {
        double maxwork = 0.0;
        for (int i = 1; i <= total_secs; ++i) {
            // note index is being used here in lieu of time, as the index
            // is assumed to be proportional to time
            double work = ride_bests[i] * i;
            if (maxwork > work)
                ride_bests[i] = round(maxwork / i);
            else
                maxwork = work;
        }
    }
#endif

    //
    // FILL IN THE GAPS
    //
    // We want to present a full set of bests for
    // every duration so the data interface for this
    // cache can remain the same, but the level of
    // accuracy/granularity can change in here in the
    // future if some fancy new algorithm arrives
    //
    double last = 0;
    for (int i=ride_bests.size()-1; i; i--) {
        if (ride_bests[i] == 0) ride_bests[i]=last;
        else last = ride_bests[i];
    }

    //
    // Now copy across into the array passed
    // encoding decimal places as we go
    array.resize(ride_bests.count());
    for (int i=0; i<ride_bests.count(); i++) {

        // convert from double to long, preserving the
        // precision by applying a multiplier
        array[i] = ride_bests[i] * decimals;

    }
}

void
RideFileCache::computeDistribution(QVector<unsigned long> &array, RideFile::SeriesType series)
{
    // Derived data series are a special case
    if (series == RideFile::xPower) {
        computeDistributionXPower();
        return;
    }

    if (series == RideFile::NP) {
        computeDistributionNP();
        return;
    }

    // only bother if the data series is actually present
    if (ride->isDataPresent(series) == false) return;

    // setup the array based upon the ride
    int decimals = RideFile::decimalsFor(series) ? 1 : 0;
    double min = RideFile::minimumFor(series) * pow(10, decimals);
    double max = RideFile::maximumFor(series) * pow(10, decimals);

    // lets resize the array to the right size
    // it will also initialise with a default value
    // which for longs is handily zero
    array.resize(max-min);

    foreach(RideFilePoint *dp, ride->dataPoints()) {
        double value = dp->value(series);
        unsigned long lvalue = value * pow(10, decimals);

        int offset = lvalue - min;
        if (offset >= 0 && offset < array.size()) array[offset]++; // XXX recintsecs != 1
    }
}

void
RideFileCache::computeDistributionNP() {}

void
RideFileCache::computeDistributionXPower() {}

void
RideFileCache::computeMeanMaxNP() {}

void
RideFileCache::computeMeanMaxXPower() {}

//
// AGGREGATE FOR A GIVEN DATE RANGE
//
static QDate dateFromFileName(const QString filename) {
    QRegExp rx("^(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_\\d\\d_\\d\\d_\\d\\d\\..*$");
    if (rx.exactMatch(filename)) {
        QDate date(rx.cap(1).toInt(), rx.cap(2).toInt(), rx.cap(3).toInt());
        if (date.isValid()) return date;
    }
    return QDate(); // nil date
}

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
    for (int i=0; i<into.size(); i++) into[i] += other[i];
}

RideFileCache::RideFileCache(MainWindow *main, QDate start, QDate end)
               : main(main), rideFileName(""), ride(0)
{
    // resize all the arrays to zero - expand as neccessary
    xPowerMeanMax.resize(0);
    npMeanMax.resize(0);
    wattsMeanMax.resize(0);
    hrMeanMax.resize(0);
    cadMeanMax.resize(0);
    nmMeanMax.resize(0);
    kphMeanMax.resize(0);
    xPowerMeanMax.resize(0);
    npMeanMax.resize(0);
    wattsDistribution.resize(0);
    hrDistribution.resize(0);
    cadDistribution.resize(0);
    nmDistribution.resize(0);
    kphDistribution.resize(0);
    xPowerDistribution.resize(0);
    npDistribution.resize(0);

    // Iterate over the ride files (not the cpx files since they /might/ not
    // exist, or /might/ be out of date.
    foreach (QString rideFileName, RideFileFactory::instance().listRideFiles(main->home)) {
        QDate rideDate = dateFromFileName(rideFileName);
        if (rideDate >= start && rideDate <= end) {
            // get its cached values (will refresh if needed...)
            RideFileCache rideCache(main, main->home.absolutePath() + "/" + rideFileName);

            // lets aggregate
            meanMaxAggregate(wattsMeanMaxDouble, rideCache.wattsMeanMaxDouble, wattsMeanMaxDate, rideDate);
            meanMaxAggregate(hrMeanMaxDouble, rideCache.hrMeanMaxDouble, hrMeanMaxDate, rideDate);
            meanMaxAggregate(cadMeanMaxDouble, rideCache.cadMeanMaxDouble, cadMeanMaxDate, rideDate);
            meanMaxAggregate(nmMeanMaxDouble, rideCache.nmMeanMaxDouble, nmMeanMaxDate, rideDate);
            meanMaxAggregate(kphMeanMaxDouble, rideCache.kphMeanMaxDouble, kphMeanMaxDate, rideDate);
            meanMaxAggregate(xPowerMeanMaxDouble, rideCache.xPowerMeanMaxDouble, xPowerMeanMaxDate, rideDate);
            meanMaxAggregate(npMeanMaxDouble, rideCache.npMeanMaxDouble, npMeanMaxDate, rideDate);

            distAggregate(wattsDistributionDouble, rideCache.wattsDistributionDouble);
            distAggregate(hrDistributionDouble, rideCache.hrDistributionDouble);
            distAggregate(cadDistributionDouble, rideCache.cadDistributionDouble);
            distAggregate(nmDistributionDouble, rideCache.nmDistributionDouble);
            distAggregate(kphDistributionDouble, rideCache.kphDistributionDouble);
            distAggregate(xPowerDistributionDouble, rideCache.xPowerDistributionDouble);
            distAggregate(npDistributionDouble, rideCache.npDistributionDouble);
        }
    }
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
    head.wattsMeanMaxCount = wattsMeanMax.size();
    head.hrMeanMaxCount = hrMeanMax.size();
    head.cadMeanMaxCount = cadMeanMax.size();
    head.nmMeanMaxCount = nmMeanMax.size();
    head.kphMeanMaxCount = kphMeanMax.size();
    head.xPowerMeanMaxCount = xPowerMeanMax.size();
    head.npMeanMaxCount = npMeanMax.size();

    head.wattsDistCount = wattsDistribution.size();
    head.xPowerDistCount = xPowerDistribution.size();
    head.npDistCount = xPowerDistribution.size();
    head.hrDistCount = hrDistribution.size();
    head.cadDistCount = cadDistribution.size();
    head.nmDistrCount = nmDistribution.size();
    head.kphDistCount = kphDistribution.size();
    out->writeRawData((const char *) &head, sizeof(head));

    // write meanmax
    out->writeRawData((const char *) wattsMeanMax.data(), sizeof(unsigned long) * wattsMeanMax.size());
    out->writeRawData((const char *) hrMeanMax.data(), sizeof(unsigned long) * hrMeanMax.size());
    out->writeRawData((const char *) cadMeanMax.data(), sizeof(unsigned long) * cadMeanMax.size());
    out->writeRawData((const char *) nmMeanMax.data(), sizeof(unsigned long) * nmMeanMax.size());
    out->writeRawData((const char *) kphMeanMax.data(), sizeof(unsigned long) * kphMeanMax.size());
    out->writeRawData((const char *) xPowerMeanMax.data(), sizeof(unsigned long) * xPowerMeanMax.size());
    out->writeRawData((const char *) npMeanMax.data(), sizeof(unsigned long) * npMeanMax.size());

    // write dist
    out->writeRawData((const char *) wattsDistribution.data(), sizeof(unsigned long) * wattsDistribution.size());
    out->writeRawData((const char *) hrDistribution.data(), sizeof(unsigned long) * hrDistribution.size());
    out->writeRawData((const char *) cadDistribution.data(), sizeof(unsigned long) * cadDistribution.size());
    out->writeRawData((const char *) nmDistribution.data(), sizeof(unsigned long) * nmDistribution.size());
    out->writeRawData((const char *) kphDistribution.data(), sizeof(unsigned long) * kphDistribution.size());
    out->writeRawData((const char *) xPowerDistribution.data(), sizeof(unsigned long) * xPowerDistribution.size());
    out->writeRawData((const char *) npDistribution.data(), sizeof(unsigned long) * npDistribution.size());
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
        wattsDistribution.resize(head.wattsDistCount);
        hrDistribution.resize(head.hrDistCount);
        cadDistribution.resize(head.cadDistCount);
        nmDistribution.resize(head.nmDistrCount);
        kphDistribution.resize(head.kphDistCount);
        xPowerDistribution.resize(head.xPowerDistCount);
        npDistribution.resize(head.npDistCount);

        // read in the arrays
        inFile.readRawData((char *) wattsMeanMax.data(), sizeof(unsigned long) * wattsMeanMax.size());
        inFile.readRawData((char *) hrMeanMax.data(), sizeof(unsigned long) * hrMeanMax.size());
        inFile.readRawData((char *) cadMeanMax.data(), sizeof(unsigned long) * cadMeanMax.size());
        inFile.readRawData((char *) nmMeanMax.data(), sizeof(unsigned long) * nmMeanMax.size());
        inFile.readRawData((char *) kphMeanMax.data(), sizeof(unsigned long) * kphMeanMax.size());
        inFile.readRawData((char *) xPowerMeanMax.data(), sizeof(unsigned long) * xPowerMeanMax.size());
        inFile.readRawData((char *) npMeanMax.data(), sizeof(unsigned long) * npMeanMax.size());

        // write dist
        inFile.readRawData((char *) wattsDistribution.data(), sizeof(unsigned long) * wattsDistribution.size());
        inFile.readRawData((char *) hrDistribution.data(), sizeof(unsigned long) * hrDistribution.size());
        inFile.readRawData((char *) cadDistribution.data(), sizeof(unsigned long) * cadDistribution.size());
        inFile.readRawData((char *) nmDistribution.data(), sizeof(unsigned long) * nmDistribution.size());
        inFile.readRawData((char *) kphDistribution.data(), sizeof(unsigned long) * kphDistribution.size());
        inFile.readRawData((char *) xPowerDistribution.data(), sizeof(unsigned long) * xPowerDistribution.size());
        inFile.readRawData((char *) npDistribution.data(), sizeof(unsigned long) * npDistribution.size());

        // setup the doubles the users use
        doubleArray(wattsMeanMaxDouble, wattsMeanMax, RideFile::watts);
        doubleArray(hrMeanMaxDouble, hrMeanMax, RideFile::hr);
        doubleArray(cadMeanMaxDouble, cadMeanMax, RideFile::cad);
        doubleArray(nmMeanMaxDouble, nmMeanMax, RideFile::nm);
        doubleArray(kphMeanMaxDouble, kphMeanMax, RideFile::kph);
        doubleArray(wattsDistributionDouble, wattsDistribution, RideFile::watts);
        doubleArray(hrDistributionDouble, hrDistribution, RideFile::hr);
        doubleArray(cadDistributionDouble, cadDistribution, RideFile::cad);
        doubleArray(nmDistributionDouble, nmDistribution, RideFile::nm);
        doubleArray(kphDistributionDouble, kphDistribution, RideFile::kph);
        doubleArray(xPowerDistributionDouble, xPowerDistribution, RideFile::xPower);
        doubleArray(npDistributionDouble, npDistribution, RideFile::NP);

        cacheFile.close();
    }
}

// unpack the longs into a double array
void RideFileCache::doubleArray(QVector<double> &into, QVector<unsigned long> &from, RideFile::SeriesType series)
{
    double divisor = RideFile::decimalsFor(series) ? 10 : 1;
    into.resize(from.size());
    for(int i=0; i<from.size(); i++) into[i] = from[i] / divisor;

    return;
}

