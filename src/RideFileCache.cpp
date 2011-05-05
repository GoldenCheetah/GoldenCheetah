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
#include "Zones.h"
#include "HrZones.h"

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

    // time in zone are fixed to 10 zone max
    wattsTimeInZone.resize(10);
    hrTimeInZone.resize(10);

    // Get info for ride file and cache file
    QFileInfo rideFileInfo(rideFileName);
    cacheFileName = rideFileInfo.path() + "/" + rideFileInfo.baseName() + ".cpx";
    QFileInfo cacheFileInfo(cacheFileName);

    // is it up-to-date?
    if (cacheFileInfo.exists() && rideFileInfo.lastModified() < cacheFileInfo.lastModified() &&
        cacheFileInfo.size() >= (int)sizeof(struct RideFileCacheHeader)) {

        // we have a file, it is more recent than the ride file
        // but is it the latest version?
        RideFileCacheHeader head;
        QFile cacheFile(cacheFileName);
        if (cacheFile.open(QIODevice::ReadOnly) == true) {

            // read the header
            QDataStream inFile(&cacheFile);
            inFile.readRawData((char *) &head, sizeof(head));
            cacheFile.close();

            // is it as recent as we are?
            if (head.version == RideFileCacheVersion) {

                // Are the CP/LTHR values still correct
                // XXX todo

                // WE'RE GOOD
                if (check == false) readCache(); // if check is false we aren't just checking
                return;
            }
        }
    }

    // NEED TO UPDATE!!

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

        case RideFile::hr:
            return hrDistributionDouble;
            break;

        case RideFile::nm:
            return nmDistributionDouble;
            break;

        case RideFile::kph:
            return kphDistributionDouble;
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
    MeanMaxComputer thread1(ride, wattsMeanMax, RideFile::watts); thread1.start();
    MeanMaxComputer thread2(ride, hrMeanMax, RideFile::hr); thread2.start();
    MeanMaxComputer thread3(ride, cadMeanMax, RideFile::cad); thread3.start();
    MeanMaxComputer thread4(ride, nmMeanMax, RideFile::nm); thread4.start();
    MeanMaxComputer thread5(ride, kphMeanMax, RideFile::kph); thread5.start();
    MeanMaxComputer thread6(ride, xPowerMeanMax, RideFile::xPower); thread6.start();
    MeanMaxComputer thread7(ride, npMeanMax, RideFile::NP); thread7.start();

    // all the different distributions
    computeDistribution(wattsDistribution, RideFile::watts);
    computeDistribution(hrDistribution, RideFile::hr);
    computeDistribution(cadDistribution, RideFile::cad);
    computeDistribution(nmDistribution, RideFile::nm);
    computeDistribution(kphDistribution, RideFile::kph);

    // wait for them threads
    thread1.wait();
    thread2.wait();
    thread3.wait();
    thread4.wait();
    thread5.wait();
    thread6.wait();
    thread7.wait();
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

data_t *
MeanMaxComputer::integrate_series(cpintdata &data)
{

    data_t *integrated= (data_t *)malloc(sizeof(data_t)*(data.points.size()+1)); //XXX use QVector... todo
    int i;
    data_t acc=0;

    for (i=0; i<data.points.size(); i++) {
        integrated[i]=acc;
        acc+=data.points[i].value;
    }
    integrated[i]=acc;

    return integrated;
}

data_t
MeanMaxComputer::partial_max_mean(data_t *dataseries_i, int start, int end, int length, int *offset)
{
    int i;
    data_t candidate=0;

    int best_i;

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


data_t
MeanMaxComputer::divided_max_mean(data_t *dataseries_i, int datalength, int length, int *offset)
{
    int shift=length;

    //if sorting data the following is an important speedup hack
    if (shift>180) shift=180;

    int window_length=length+shift;

    if (window_length>datalength) window_length=datalength;

    // put down as many windows as will fit without overrunning data
    int start;
    int end;
    data_t energy;

    data_t candidate=0;
    int this_offset;

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
    // xPower and NP need watts to be present
    RideFile::SeriesType baseSeries = (series == RideFile::xPower || series == RideFile::NP) ?
                                      RideFile::watts : series;

    // only bother if the data series is actually present
    if (ride->isDataPresent(baseSeries) == false) return;

    // if we want decimal places only keep to 1 dp max
    // this is a factor that is applied at the end to
    // convert from high-precision double to long
    // e.g. 145.456 becomes 1455 if we want decimals
    // and becomes 145 if we don't
    double decimals = RideFile::decimalsFor(baseSeries) ? 10 : 1;

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
    double lastsecs = 0;
    foreach (const RideFilePoint *p, ride->dataPoints()) {

        // fill in any gaps in recording - use same dodgy rounding as before
        int count = (p->secs - lastsecs - ride->recIntSecs()) / ride->recIntSecs();
        for(int i=0; i<count; i++)
            data.points.append(cpintpoint(round(lastsecs+((i+1)*ride->recIntSecs() *1000.0)/1000), 0));
        lastsecs = p->secs;

        double secs = round(p->secs * 1000.0) / 1000;
        if (secs > 0) data.points.append(cpintpoint(secs, (int) round(p->value(baseSeries))));
    }
    int total_secs = (int) ceil(data.points.back().secs);

    // don't allow data more than two days
    // was one week, but no single ride is longer
    // than 2 days, even if you are doing RAAM
    if (total_secs > 2*24*60*60) return;

    // NP - rolling 30s avg ^ 4
    if (series == RideFile::NP) {

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

        const double exp = 2.0f / ((25.0f / ride->recIntSecs()) + 1.0f);
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

                if (i < rollingwindowsize) {

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

    // the bests go in here...
    QVector <double> ride_bests(total_secs + 1);

    data_t *dataseries_i = integrate_series(data);

    for (int i=1; i<data.points.size(); i++) {

        int offset;
        data_t c=divided_max_mean(dataseries_i,data.points.size(),i,&offset);

        // snaffle it away
        int sec = i*ride->recIntSecs();
        data_t val = c / (data_t)i;

        if (sec < ride_bests.size()) {
            if (series == RideFile::NP || series == RideFile::xPower)
                ride_bests[sec] = pow(val, 0.25f);
            else
                ride_bests[sec] = val;
        }
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
    double last = 0;
    array.resize(ride_bests.count());
    for (int i=ride_bests.size()-1; i; i--) {
        if (ride_bests[i] == 0) ride_bests[i]=last;
        else last = ride_bests[i];

        // convert from double to long, preserving the
        // precision by applying a multiplier
        array[i] = ride_bests[i] * decimals;
    }

}

void
RideFileCache::computeDistribution(QVector<float> &array, RideFile::SeriesType series)
{
    // only bother if the data series is actually present
    if (ride->isDataPresent(series) == false) return;

    // get zones that apply, if any
    int zoneRange = main->zones() ? main->zones()->whichRange(ride->startTime().date()) : -1;
    int hrZoneRange = main->hrZones() ? main->hrZones()->whichRange(ride->startTime().date()) : -1;

    if (zoneRange != -1) CP=main->zones()->getCP(zoneRange);
    else CP=0;

    if (hrZoneRange != -1) LTHR=main->hrZones()->getLT(hrZoneRange);
    else LTHR=0;

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
        float lvalue = value * pow(10, decimals);

        // watts time in zone
        if (series == RideFile::watts && zoneRange != -1)
            wattsTimeInZone[main->zones()->whichZone(zoneRange, dp->value(series))] += ride->recIntSecs();

        // hr time in zone
        if (series == RideFile::hr && hrZoneRange != -1)
            hrTimeInZone[main->hrZones()->whichZone(hrZoneRange, dp->value(series))] += ride->recIntSecs();

        int offset = lvalue - min;
        if (offset >= 0 && offset < array.size()) array[offset] += ride->recIntSecs();
    }
}

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
    if (into.size() < other.size()) into.resize(other.size());
    for (int i=0; i<other.size(); i++) into[i] += other[i];

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

    // time in zone are fixed to 10 zone max
    wattsTimeInZone.resize(10);
    hrTimeInZone.resize(10);

    // set cursor busy whilst we aggregate -- bit of feedback
    // and less intrusive than a popup box
    main->setCursor(Qt::WaitCursor);

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

            // cumulate timeinzones
            for (int i=0; i<10; i++) {
                hrTimeInZone[i] += rideCache.hrTimeInZone[i];
                wattsTimeInZone[i] += rideCache.wattsTimeInZone[i];
            }
        }
    }

    // set the cursor back to normal
    main->setCursor(Qt::ArrowCursor);
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
    head.CP = CP;
    head.LTHR = LTHR;

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
    out->writeRawData((const char *) wattsMeanMax.data(), sizeof(float) * wattsMeanMax.size());
    out->writeRawData((const char *) hrMeanMax.data(), sizeof(float) * hrMeanMax.size());
    out->writeRawData((const char *) cadMeanMax.data(), sizeof(float) * cadMeanMax.size());
    out->writeRawData((const char *) nmMeanMax.data(), sizeof(float) * nmMeanMax.size());
    out->writeRawData((const char *) kphMeanMax.data(), sizeof(float) * kphMeanMax.size());
    out->writeRawData((const char *) xPowerMeanMax.data(), sizeof(float) * xPowerMeanMax.size());
    out->writeRawData((const char *) npMeanMax.data(), sizeof(float) * npMeanMax.size());

    // write dist
    out->writeRawData((const char *) wattsDistribution.data(), sizeof(float) * wattsDistribution.size());
    out->writeRawData((const char *) hrDistribution.data(), sizeof(float) * hrDistribution.size());
    out->writeRawData((const char *) cadDistribution.data(), sizeof(float) * cadDistribution.size());
    out->writeRawData((const char *) nmDistribution.data(), sizeof(float) * nmDistribution.size());
    out->writeRawData((const char *) kphDistribution.data(), sizeof(float) * kphDistribution.size());
    out->writeRawData((const char *) xPowerDistribution.data(), sizeof(float) * xPowerDistribution.size());
    out->writeRawData((const char *) npDistribution.data(), sizeof(float) * npDistribution.size());

    // time in zone
    out->writeRawData((const char *) wattsTimeInZone.data(), sizeof(float) * wattsTimeInZone.size());
    out->writeRawData((const char *) hrTimeInZone.data(), sizeof(float) * hrTimeInZone.size());
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
        npMeanMax.resize(head.npMeanMaxCount);
        xPowerMeanMax.resize(head.xPowerMeanMaxCount);
        wattsDistribution.resize(head.wattsDistCount);
        hrDistribution.resize(head.hrDistCount);
        cadDistribution.resize(head.cadDistCount);
        nmDistribution.resize(head.nmDistrCount);
        kphDistribution.resize(head.kphDistCount);
        xPowerDistribution.resize(head.xPowerDistCount);
        npDistribution.resize(head.npDistCount);

        // read in the arrays
        inFile.readRawData((char *) wattsMeanMax.data(), sizeof(float) * wattsMeanMax.size());
        inFile.readRawData((char *) hrMeanMax.data(), sizeof(float) * hrMeanMax.size());
        inFile.readRawData((char *) cadMeanMax.data(), sizeof(float) * cadMeanMax.size());
        inFile.readRawData((char *) nmMeanMax.data(), sizeof(float) * nmMeanMax.size());
        inFile.readRawData((char *) kphMeanMax.data(), sizeof(float) * kphMeanMax.size());
        inFile.readRawData((char *) xPowerMeanMax.data(), sizeof(float) * xPowerMeanMax.size());
        inFile.readRawData((char *) npMeanMax.data(), sizeof(float) * npMeanMax.size());


        // write dist
        inFile.readRawData((char *) wattsDistribution.data(), sizeof(float) * wattsDistribution.size());
        inFile.readRawData((char *) hrDistribution.data(), sizeof(float) * hrDistribution.size());
        inFile.readRawData((char *) cadDistribution.data(), sizeof(float) * cadDistribution.size());
        inFile.readRawData((char *) nmDistribution.data(), sizeof(float) * nmDistribution.size());
        inFile.readRawData((char *) kphDistribution.data(), sizeof(float) * kphDistribution.size());
        inFile.readRawData((char *) xPowerDistribution.data(), sizeof(float) * xPowerDistribution.size());
        inFile.readRawData((char *) npDistribution.data(), sizeof(float) * npDistribution.size());

        // time in zone
        inFile.readRawData((char *) wattsTimeInZone.data(), sizeof(float) * 10);
        inFile.readRawData((char *) hrTimeInZone.data(), sizeof(float) * 10);

        // setup the doubles the users use
        doubleArray(wattsMeanMaxDouble, wattsMeanMax, RideFile::watts);
        doubleArray(hrMeanMaxDouble, hrMeanMax, RideFile::hr);
        doubleArray(cadMeanMaxDouble, cadMeanMax, RideFile::cad);
        doubleArray(nmMeanMaxDouble, nmMeanMax, RideFile::nm);
        doubleArray(kphMeanMaxDouble, kphMeanMax, RideFile::kph);
        doubleArray(npMeanMaxDouble, npMeanMax, RideFile::NP);
        doubleArray(xPowerMeanMaxDouble, xPowerMeanMax, RideFile::xPower);
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
void RideFileCache::doubleArray(QVector<double> &into, QVector<float> &from, RideFile::SeriesType series)
{
    double divisor = RideFile::decimalsFor(series) ? 10 : 1;
    into.resize(from.size());
    for(int i=0; i<from.size(); i++) into[i] = from[i] / divisor;

    return;
}

