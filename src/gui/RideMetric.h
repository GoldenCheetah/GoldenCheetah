
#ifndef _GC_RideMetric_h
#define _GC_RideMetric_h 1

#include <QHash>
#include <QString>
#include <assert.h>

#include "RawFile.h"

class Zones;

struct RideMetric {
    virtual ~RideMetric() {}
    virtual QString name() const = 0;
    virtual QString units(bool metric) const = 0;
    virtual double value(bool metric) const = 0;
    virtual void compute(const RawFile *raw, 
                         const Zones *zones, int zoneRange) = 0;
    virtual void aggregateWith(RideMetric *other) = 0;
    virtual RideMetric *clone() const = 0;
};

struct PointwiseRideMetric : public RideMetric {
    void compute(const RawFile *raw, const Zones *zones, int zoneRange) {
        QListIterator<RawFilePoint*> i(raw->points);
        double secsDelta = raw->rec_int_ms / 1000.0;
        while (i.hasNext()) {
            const RawFilePoint *point = i.next();
            perPoint(point, secsDelta, raw, zones, zoneRange);
        }
    }
    virtual void perPoint(const RawFilePoint *point, double secsDelta,
                          const RawFile *raw, const Zones *zones,
                          int zoneRange) = 0;
};

class AvgRideMetric : public PointwiseRideMetric {

    protected:

    int count;
    double total;

    public:

    AvgRideMetric() : count(0), total(0.0) {}
    double value(bool) const {
        if (count == 0) return 0.0;
        return total / count;
    }
    void aggregateWith(RideMetric *other) { 
        assert(name() == other->name());
        AvgRideMetric *as = dynamic_cast<AvgRideMetric*>(other);
        count += as->count;
        total += as->total;
    }
};

class RideMetricFactory {

    public:

    typedef QHash<QString,RideMetric*> RideMetricMap;
    typedef QHashIterator<QString,RideMetric*> RideMetricIter;

    private:

    static RideMetricFactory *_instance;
    RideMetricMap _metrics;

    RideMetricFactory() {}
    RideMetricFactory(const RideMetricFactory &other);
    RideMetricFactory &operator=(const RideMetricFactory &other);

    public:

    static RideMetricFactory &instance() {
        if (!_instance)
            _instance = new RideMetricFactory();
        return *_instance;
    }

    const RideMetricMap &metrics() const { return _metrics; }

    RideMetric *newMetric(const QString &name) const {
        if (!_metrics.contains(name))
            return NULL;
        RideMetric *metric = _metrics.value(name);
        return metric->clone();
    }

    bool addMetric(const RideMetric &metric) {
        assert(!_metrics.contains(metric.name()));
        _metrics.insert(metric.name(), metric.clone());
        return true;
    }
};

#endif // _GC_RideMetric_h

