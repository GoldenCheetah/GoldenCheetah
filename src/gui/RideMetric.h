
#ifndef _GC_RideMetric_h
#define _GC_RideMetric_h 1

#include <QHash>
#include <QString>

class RawFile;
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

