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

#ifndef _GC_RideFileCache_h
#define _GC_RideFileCache_h 1
#include "RideFile.h"
#include <QString>
#include <QDataStream>
#include <QVector>
#include <QThread>

class Context;
class RideFile;
class SummaryMetrics;
class MetricDetail;

#include "GoldenCheetah.h"

// used by Mark Rages' Mean Max Algorithm
#include <stdlib.h>
#include <stdint.h>
typedef double data_t;

// RideFileCache is used to get meanmax and sample distribution
// arrays when plotting CP curves and histograms. It is precoputed
// to save time and cached in a file .cpx
//
static const unsigned int RideFileCacheVersion = 21;
// revision history:
// version  date         description
// 1        29-Apr-11    Initial - header, mean-max & distribution data blocks
// 2        02-May-11    Added LTHR/CP used to header and Time In Zone block
// 3        02-May-11    Moved to float precision not integer.
// 4        02-May-11    Moved to Mark Rages mean-max function with higher precision
// 5        18-Aug-11    Added VAM mean maximals
// 6        27-Jun-12    Added W/kg mean maximals and distribution
// 7        03-Dec-12    Fixed W/kg calculations!
// 8        13-Feb-13    Fixed VAM calculations
// 9        06-Nov-13    Added aPower
// 10       13-Feb-14    Added Moderate, Heavy and Severe domains
// 11       17-Feb-14    Changed 3zone model to have 85% CP < middle < CP
// 12       21-Feb-14    Added Acceleration (speed)
// 12       22-Feb-14    Acceleration precision way too high!
// 13-15    24-Feb-14    Add hr, cad, watts, nm Δ data series
// 13-15    24-Feb-14    Add crc to the header
// 17       09-Jun-14    Move wpk meanmax array next to watts for fast read
// 18       19-Oct-14    Added gearRatio distribution
// 19       11-Nov-14    Added Pace Zones distribution
// 20       17-Nov-14    Added Polarized Zones for HR and Pace
// 21       27-Nov-14    Added SmO2 distribution 

// The cache file (.cpx) has a binary format:
// 1 x Header data - describing the version and contents of the cache
// n x Blocks - meanmax or distribution arrays
// 1 x Watts TIZ - 10 floats
// 1 x Heartrate TIZ - 10 floats

// The header is written directly to disk, the only
// field which is endian sensitive is the count field
// which will always be written in local format since these
// files are local caches we do not worry about endianness
struct RideFileCacheHeader {

    unsigned int version;
    unsigned int crc;

    unsigned int wattsMeanMaxCount,
                 hrMeanMaxCount,
                 cadMeanMaxCount,
                 nmMeanMaxCount,
                 kphMeanMaxCount,
                 kphdMeanMaxCount,
                 wattsdMeanMaxCount,
                 caddMeanMaxCount,
                 nmdMeanMaxCount,
                 hrdMeanMaxCount,
                 xPowerMeanMaxCount,
                 npMeanMaxCount,
                 vamMeanMaxCount,
                 wattsKgMeanMaxCount,
                 aPowerMeanMaxCount,
                 wattsDistCount,
                 hrDistCount,
                 cadDistCount,
                 gearDistCount,
                 nmDistrCount,
                 kphDistCount,
                 xPowerDistCount,
                 npDistCount,
                 wattsKgDistCount,
                 aPowerDistCount,
                 smo2DistCount;

    int LTHR, // used to calculate Time in Zone (TIZ)
        CP;   // used to calculate Time in Zone (TIZ)
    double CV;   // used to calculate Time in Zone (TIZ)
                
};


// Each block of data is an array of uint32_t (32-bit "local-endian")
// integers so the "count" setting within the block definition tells
// us how long it is so we can read in one instruction and reference
// it directly. Of course, this means that for data series that require
// decimal places (e.g. speed) they are stored multiplied by 10^dp.
// so 27.1 is stored as 271, 27.454 is stored as 27454, 100.0001 is
// stored as 1000001.

// So that none of the plots need to understand the format of this
// cache file this class is repsonsible for supplying the pre-computed
// values they desire. If the values have not been computed or are
// out of date then they are computed as needed.
//
// This cache is also updated by the metricaggregator to ensure it
// is updated alongside the metrics. So, in theory, at runtime, once
// the arrays have been computed they can be retrieved quickly.
//
// This is the main user entry to the ridefile cached data.
class RideFileCache
{
    public:
        enum cachetype { meanmax, distribution, none };
        typedef enum cachetype CacheType;
        QDate start, end;
        unsigned int crc;

        // Construct from a ridefile or its filename
        // will reference cache if it exists, and create it
        // if it doesn't. We allow to create from ridefile to
        // save on ridefile reading if it is already opened by
        // the calling class.
        // to save time you can pass the ride file if you already have it open
        // and if you don't want the data and just want to check pass check=true
        RideFileCache(Context *context, QString filename, RideFile *ride =0, bool check = false);

        // Construct a ridefile cache that represents the data
        // across a date range. This is used to provide aggregated data.
        RideFileCache(Context *context, QDate start, QDate end, bool filter = false, QStringList files = QStringList(), bool onhome = true);

        // Just get mean max values for power & wpk for a ride
        static QVector<float> meanMaxPowerFor(Context *context, QVector<float>&wpk, QDate from, QDate to);
        static QVector<float> meanMaxPowerFor(Context *context, QVector<float>&wpk, QString filename);

        // not actually a copy constructor -- but we call it IN the constructor.
        RideFileCache(RideFileCache *other) { *this = *other; }

        // just from a raw ride file class (usually for intervals)
        RideFileCache(RideFile*);

        // get a single best or time in zone value from the cache file
        // intended to be very fast (using lseek to jump direct to the value requested
        static double best(Context *context, QString fileName, RideFile::SeriesType series, int duration);
        static int tiz(Context *context, QString fileName, RideFile::SeriesType series, int zone);

        // get all the bests passed and return a list of summary metrics, like the DBAccess
        // function but using CPX files as the source
        static QList<SummaryMetrics> getAllBestsFor(Context *context, QList<MetricDetail>, QDateTime from, QDateTime to);

        static int decimalsFor(RideFile::SeriesType series);

        // compute the cache and return it for the ride
        static RideFileCache *createCacheFor(RideFile*);

        // get data
        QVector<double> &meanMaxArray(RideFile::SeriesType); // return meanmax array for the given series
        QVector<QDate> &meanMaxDates(RideFile::SeriesType series); // the dates of the bests
        QVector<double> &distributionArray(RideFile::SeriesType); // return distribution array for the given series
        QVector<float> &wattsZoneArray() { return wattsTimeInZone; }
        QVector<float> &wattsCPZoneArray() { return wattsCPTimeInZone; } // Polarized Zones
        QVector<float> &hrZoneArray() { return hrTimeInZone; }
        QVector<float> &hrCPZoneArray() { return hrCPTimeInZone; } // Polarized Zones
        QVector<float> &paceZoneArray() { return paceTimeInZone; }
        QVector<float> &paceCPZoneArray() { return paceCPTimeInZone; } // Polarized Zones

        QVector<float> &heatMeanMaxArray();  // will compute if neccessary

        // explain the array binning / sampling
        double &distBinSize(RideFile::SeriesType); // return distribution bin size
        double &meanMaxBinSize(RideFile::SeriesType); // return distribution bin size

        // we need to return doubles not longs, we just use longs
        // to reduce disk storage
        static void doubleArray(QVector<double> &into, QVector<float> &from, RideFile::SeriesType series);
        static void doubleArrayForDistribution(QVector<double> &into, QVector<float> &from);

    protected:

        void refreshCache();              // compute arrays and update cache
        void readCache();                 // just read from saved file and setup arrays
        void serialize(QDataStream *out); // write to file

        void compute();             // compute all arrays

        // NOW replaced computeMeanMax with MeanMaxComputer class see bottom of file
        //void computeMeanMax(QVector<float>&, RideFile::SeriesType);      // compute mean max arrays
        void computeDistribution(QVector<float>&, RideFile::SeriesType); // compute the distributions


    private:

        Context *context;
        QString rideFileName; // filename of ride
        QString cacheFileName; // filename of cache file
        RideFile *ride;

        // used for zoning
        int CP;
        int LTHR;
        double CV;

        //
        // MEAN MAXIMAL VALUES
        //
        // each array has a best for duration 0 - RideDuration seconds
        QVector<float> wattsMeanMax; // RideFile::watts
        QVector<float> hrMeanMax; // RideFile::hr
        QVector<float> cadMeanMax; // RideFile::cad
        QVector<float> nmMeanMax; // RideFile::nm
        QVector<float> kphMeanMax; // RideFile::kph
        QVector<float> kphdMeanMax; // RideFile::kphd
        QVector<float> wattsdMeanMax; // RideFile::wattsd
        QVector<float> caddMeanMax; // RideFile::cadd
        QVector<float> nmdMeanMax; // RideFile::nmd
        QVector<float> hrdMeanMax; // RideFile::hrd
        QVector<float> xPowerMeanMax; // RideFile::kph
        QVector<float> npMeanMax; // RideFile::kph
        QVector<float> vamMeanMax; // RideFile::vam
        QVector<float> wattsKgMeanMax; // watts/kg
        QVector<float> aPowerMeanMax; // RideFile::aPower

        QVector<float> heatMeanMax; // The heat of training for aggregated power data

        bool filter, onhome; // saving parameters re-used when aggregating heat
        QStringList files;


        QVector<double> wattsMeanMaxDouble; // RideFile::watts
        QVector<double> hrMeanMaxDouble; // RideFile::hr
        QVector<double> cadMeanMaxDouble; // RideFile::cad
        QVector<double> nmMeanMaxDouble; // RideFile::nm
        QVector<double> kphMeanMaxDouble; // RideFile::kph
        QVector<double> kphdMeanMaxDouble; // RideFile::kphd
        QVector<double> wattsdMeanMaxDouble; // RideFile::wattsd
        QVector<double> caddMeanMaxDouble; // RideFile::cadd
        QVector<double> nmdMeanMaxDouble; // RideFile::nmd
        QVector<double> hrdMeanMaxDouble; // RideFile::hrd
        QVector<double> xPowerMeanMaxDouble; // RideFile::kph
        QVector<double> npMeanMaxDouble; // RideFile::kph
        QVector<double> vamMeanMaxDouble; // RideFile::kph
        QVector<double> wattsKgMeanMaxDouble; // watts/kg
        QVector<double> aPowerMeanMaxDouble; // RideFile::aPower

        QVector<QDate> wattsMeanMaxDate; // RideFile::watts
        QVector<QDate> hrMeanMaxDate; // RideFile::hr
        QVector<QDate> cadMeanMaxDate; // RideFile::cad
        QVector<QDate> nmMeanMaxDate; // RideFile::nm
        QVector<QDate> kphMeanMaxDate; // RideFile::kph
        QVector<QDate> kphdMeanMaxDate; // RideFile::kph
        QVector<QDate> wattsdMeanMaxDate; // RideFile::wattsd
        QVector<QDate> caddMeanMaxDate; // RideFile::cadd
        QVector<QDate> nmdMeanMaxDate; // RideFile::nmd
        QVector<QDate> hrdMeanMaxDate; // RideFile::hrd
        QVector<QDate> xPowerMeanMaxDate; // RideFile::kph
        QVector<QDate> npMeanMaxDate; // RideFile::kph
        QVector<QDate> vamMeanMaxDate; // RideFile::vam
        QVector<QDate> wattsKgMeanMaxDate; // watts/kg
        QVector<QDate> aPowerMeanMaxDate; // RideFile::aPower

        //
        // SAMPLE DISTRIBUTION
        //
        // the distribution matches RideFile::decimalsFor(SeriesType series);
        // each array contains a count (duration in recIntSecs) for each distrbution
        // from RideFile::minimumFor() to RideFile::maximumFor(). The steps (binsize)
        // is 1.0 or if the dataseries in question does have a nonZero value for
        // RideFile::decimalsFor() then it will be distributed in 0.1 of a unit
        QVector<float> wattsDistribution; // RideFile::watts
        QVector<float> hrDistribution; // RideFile::hr
        QVector<float> gearDistribution; // RideFile::gear
        QVector<float> cadDistribution; // RideFile::cad
        QVector<float> nmDistribution; // RideFile::nm
        QVector<float> kphDistribution; // RideFile::kph
        QVector<float> kphdDistribution; // RideFile::kphd
        QVector<float> xPowerDistribution; // RideFile::kph
        QVector<float> npDistribution; // RideFile::kph
        QVector<float> wattsKgDistribution; // RideFile::wattsKg
        QVector<float> aPowerDistribution; // RideFile::aPower
        QVector<float> smo2Distribution; // RideFile::smo2

        QVector<double> wattsDistributionDouble; // RideFile::watts
        QVector<double> hrDistributionDouble; // RideFile::hr
        QVector<double> gearDistributionDouble; // RideFile::gear
        QVector<double> cadDistributionDouble; // RideFile::cad
        QVector<double> nmDistributionDouble; // RideFile::nm
        QVector<double> kphDistributionDouble; // RideFile::kph
        QVector<double> xPowerDistributionDouble; // RideFile::xpower
        QVector<double> npDistributionDouble; // RideFile::np
        QVector<double> wattsKgDistributionDouble; // RideFile::wattsKg
        QVector<double> aPowerDistributionDouble; // RideFile::aPower
        QVector<double> smo2DistributionDouble; // RideFile::aPower


        QVector<float> wattsTimeInZone;   // time in zone in seconds
        QVector<float> wattsCPTimeInZone;   // time in zone in seconds for polarized zones
        QVector<float> hrTimeInZone;      // time in zone in seconds
        QVector<float> hrCPTimeInZone;   // time in zone in seconds for polarized zones
        QVector<float> paceTimeInZone;      // time in zone in seconds
        QVector<float> paceCPTimeInZone;   // time in zone in seconds for polarized zones
};

// Working structured inherited from CPPlot.cpp
// could probably be factored out and just use the
// ridefile structures, but this keeps well tested
// and stable legacy code intact
struct cpintpoint {
    double secs;
    double value;
    cpintpoint() : secs(0.0), value(0) {}
    cpintpoint(double s, int w) : secs(s), value(w) {}
};

struct cpintdata {
    QStringList errors;
    QVector<cpintpoint> points;
    int rec_int_ms;
    cpintdata() : rec_int_ms(0) {}
};

// the mean-max computer ... runs in a thread
class MeanMaxComputer : public QThread
{
    public:
        MeanMaxComputer(RideFile *ride, QVector<float>&array, RideFile::SeriesType series)
        : ride(ride), array(array), series(series) {}
        void run();

    private:

        // Mark Rages' algorithm for fast find of mean max
        data_t *integrate_series(cpintdata &data);
        data_t partial_max_mean(data_t *dataseries_i, int start, int end, int length, int *offset);
        data_t divided_max_mean(data_t *dataseries_i, int datalength, int length, int *offset);

        RideFile *ride;
        QVector<float> &array;
        QVector<data_t> integratedArray;

        RideFile::SeriesType series;
};
#endif // _GC_RideFileCache_h
