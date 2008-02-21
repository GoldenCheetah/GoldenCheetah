
#include "RideMetric.h"
#include "Zones.h"
#include <math.h>

// NOTE: This code follows the description of xPower, Relative Intensity, and
// BikeScore in "Analysis of Power Output and Training Stress in Cyclists: The
// Development of the BikeScore(TM) Algorithm", page 5, by Phil Skiba:
//
// http://www.physfarm.com/Analysis%20of%20Power%20Output%20and%20Training%20Stress%20in%20Cyclists-%20BikeScore.pdf
//
// The weighting factors for the exponentially weighted average are taken from
// a spreadsheet provided by Dr. Skiba.

class XPower : public RideMetric {
    double xpower;
    double secs;

    friend class RelativeIntensity;
    friend class BikeScore;

    public:

    XPower() : xpower(0.0), secs(0.0) {}
    QString name() const { return "skiba_xpower"; }
    QString units(bool) const { return "watts"; }
    double value(bool) const { return xpower; }
    void compute(const RawFile *raw, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {

        static const double EPSILON = 0.1;
        static const double NEGLIGIBLE = 0.1;

        double secsDelta = raw->rec_int_ms / 1000.0;
        double sampsPerWindow = 25.0 / secsDelta;
        double attenuation = sampsPerWindow / (sampsPerWindow + secsDelta);
        double sampleWeight = secsDelta / (sampsPerWindow + secsDelta);
        
        double lastSecs = 0.0;
        double weighted = 0.0;

        double total = 0.0;
        int count = 0;

        QListIterator<RawFilePoint*> i(raw->points);
        while (i.hasNext()) {
            const RawFilePoint *point = i.next();
            while ((weighted > NEGLIGIBLE) 
                   && (point->secs > lastSecs + secsDelta + EPSILON)) {
                weighted *= attenuation;
                lastSecs += secsDelta;
            }
            weighted *= attenuation;
            weighted += sampleWeight * point->watts;
            lastSecs = point->secs;
            total += pow(weighted, 4.0);
            count++;
        }
        xpower = pow(total / count, 0.25);
        secs = count * secsDelta;
    }
    RideMetric *clone() const { return new XPower(*this); }
};

class RelativeIntensity : public RideMetric {
    double reli;
    double secs;

    public:

    RelativeIntensity() : reli(0.0), secs(0.0) {}
    QString name() const { return "skiba_relative_intensity"; }
    QString units(bool) const { return ""; }
    double value(bool) const { return reli; }
    void compute(const RawFile *, const Zones *zones, int zoneRange,
                 const QHash<QString,RideMetric*> &deps) {
        if (zones) {
            assert(deps.contains("skiba_xpower"));
            XPower *xp = dynamic_cast<XPower*>(deps.value("skiba_xpower"));
            assert(xp);
            reli = xp->xpower / zones->getFTP(zoneRange);
            secs = xp->secs;
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
    void compute(const RawFile *, const Zones *zones, int zoneRange,
                 const QHash<QString,RideMetric*> &deps) {
        assert(deps.contains("skiba_xpower"));
        assert(deps.contains("skiba_relative_intensity"));
        XPower *xp = dynamic_cast<XPower*>(deps.value("skiba_xpower"));
        RideMetric *ri = deps.value("skiba_relative_intensity");
        assert(ri);
        double normWork = xp->xpower * xp->secs;
        double rawBikeScore = normWork * ri->value(true);
        // TODO: use CP, not FTP here
        double workInAnHourAtCP = zones->getFTP(zoneRange) * 3600; 
        score = rawBikeScore / workInAnHourAtCP * 100.0;
    }
    RideMetric *clone() const { return new BikeScore(*this); }
    bool canAggregate() const { return true; }
    void aggregateWith(RideMetric *other) { score += other->value(true); }
};

static bool addAllThree() {
    RideMetricFactory::instance().addMetric(XPower());
    QVector<QString> deps;
    deps.append("skiba_xpower");
    RideMetricFactory::instance().addMetric(RelativeIntensity(), &deps);
    deps.append("skiba_relative_intensity");
    RideMetricFactory::instance().addMetric(BikeScore(), &deps);
    return true;
}

static bool allThreeAdded = addAllThree();

