
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

class TotalWorkRideMetric : public RideMetric {
    double kJ;

    public:

    TotalWorkRideMetric() : kJ(0.0) {}
    QString name() const { return "total_work"; }
    QString units(bool metric) const { (void) metric; return "kJ"; }
    double value(bool metric) const { (void) metric; return kJ; }
    void compute(const RawFile *raw, const Zones *zones, int zoneRange) {
        (void) zones;
        (void) zoneRange;
        QListIterator<RawFilePoint*> i(raw->points);
        double secs_delta = raw->rec_int_ms / 1000.0;
        while (i.hasNext()) {
            RawFilePoint *point = i.next();
            if (point->watts >= 0.0)
                kJ += point->watts * secs_delta;
        }
        kJ /= 1000.0;
    }
    void aggregateWith(RideMetric *other) {
        assert(other->name() == name());
        TotalWorkRideMetric *m = (TotalWorkRideMetric*) other;
        kJ += m->kJ;
    }
    RideMetric *clone() const { return new TotalWorkRideMetric(*this); }
};

#endif // _GC_BasicRideMetrics_h

