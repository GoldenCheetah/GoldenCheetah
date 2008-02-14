
#ifndef _GC_BasicRideMetrics_h
#define _GC_BasicRideMetrics_h 1

#include "RideMetric.h"
#include "RawFile.h"

class TotalDistanceRideMetric : public RideMetric {
    double miles;

    public:

    TotalDistanceRideMetric() : miles(0.0) {}
    QString name() const { return "total_distance"; }
    QString units(bool metric) const { return metric ? "km" : "miles"; }
    double value(bool metric) const {
        return metric ? (miles / 0.62137119) : miles;
    }
    void compute(const RawFile *raw, const Zones *zones, int zoneRange) {
        (void) zones;
        (void) zoneRange;
        miles = raw->points.back()->miles;
    }
    void aggregateWith(RideMetric *other) {
        assert(other->name() == name());
        TotalDistanceRideMetric *m = (TotalDistanceRideMetric*) other;
        miles += m->miles;
    }
    RideMetric *clone() const { return new TotalDistanceRideMetric(*this); }
};

#endif // _GC_BasicRideMetrics_h

