
#include "RideMetric.h"
#include "Zones.h"
#include <math.h>

class XPower : public RideMetric {
    double xpower;
    double secs;

    friend class RelativeIntensity;

    public:

    XPower() : xpower(0.0), secs(0) {}
    QString name() const { return "skiba_xpower"; }
    QString units(bool) const { return "watts"; }
    double value(bool) const { return xpower; }
    void compute(const RawFile *raw, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {
        double total = 0.0;
        int count = 0;
        // we're interested in a 25 second moving average
        int movingAvgLen = (int) ceil(25000 / raw->rec_int_ms);
        double *movingAvgWatts = new double[movingAvgLen];
        int j = 0; // an index for the moving average function
        QListIterator<RawFilePoint*> i(raw->points);
        double secsDelta = raw->rec_int_ms / 1000.0;
        while (i.hasNext()) {
            const RawFilePoint *point = i.next();
            if (point->watts >= 0.0) {
                secs += secsDelta;
                // 25 second moving average
                // populate the array in the first 25 seconds
                if (secs <= 25)
                    movingAvgWatts[j] = point->watts;
                else {
                    // add the most recent value, and calculate average
                    movingAvgWatts[j] = point->watts;
                    double avg = 0.0;
                    for (int jj = 0; jj < movingAvgLen; jj++)
                        avg += movingAvgWatts[jj];
                    total += pow(avg / movingAvgLen, 4.0);
                    count++;
                }
                j = (j + 1) % movingAvgLen;
            }
        }
        xpower = pow(total / count, 0.25);
        delete [] movingAvgWatts;
    }
    RideMetric *clone() const { return new XPower(*this); }
};

class RelativeIntensity : public RideMetric {
    double reli;

    public:

    RelativeIntensity() : reli(0.0) {}
    QString name() const { return "skiba_relative_intensity"; }
    QString units(bool) const { return ""; }
    double value(bool) const { return reli; }
    void compute(const RawFile *, const Zones *zones, int zoneRange,
                 const QHash<QString,RideMetric*> &deps) {
        if (zones) {
            assert(deps.contains("skiba_xpower"));
            RideMetric *xp = deps.value("skiba_xpower");
            assert(xp);
            reli = xp->value(true) / zones->getFTP(zoneRange);
        }
    }
    RideMetric *clone() const { return new RelativeIntensity(*this); }
};

class BikeScore : public RideMetric {
    double score;

    public:

    BikeScore() : score(0.0) {}
    QString name() const { return "skiba_bike_score"; }
    QString units(bool) const { return ""; }
    double value(bool) const { return score; }
    void compute(const RawFile *raw, const Zones *, int,
                 const QHash<QString,RideMetric*> &deps) {
        assert(deps.contains("skiba_relative_intensity"));
        RideMetric *ri = deps.value("skiba_relative_intensity");
        assert(ri);
        score = ((raw->points.back()->secs / 60) / 60) 
                * (pow(ri->value(true), 2)) * 100.0;
     }
    RideMetric *clone() const { return new BikeScore(*this); }
};

static bool addAllThree() {
    RideMetricFactory::instance().addMetric(XPower());
    QVector<QString> deps;
    deps.append("skiba_xpower");
    RideMetricFactory::instance().addMetric(RelativeIntensity(), &deps);
    deps.clear();
    deps.append("skiba_relative_intensity");
    RideMetricFactory::instance().addMetric(BikeScore(), &deps);
    return true;
}

static bool allThreeAdded = addAllThree();

