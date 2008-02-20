
#include "RideMetric.h"

#define MILES_PER_KM 0.62137119

class WorkoutTime : public RideMetric {
    double seconds;

    public:

    WorkoutTime() : seconds(0.0) {}
    QString name() const { return "workout_time"; }
    QString units(bool) const { return "seconds"; }
    double value(bool) const { return seconds; }
    void compute(const RawFile *raw, const Zones *, int) {
        seconds = raw->points.back()->secs;
    }
    void aggregateWith(RideMetric *other) { seconds += other->value(true); }
    RideMetric *clone() const { return new WorkoutTime(*this); }
};

static bool workoutTimeAdded =
    RideMetricFactory::instance().addMetric(WorkoutTime());

//////////////////////////////////////////////////////////////////////////////

class TimeRiding : public PointwiseRideMetric {
    double secsMovingOrPedaling;

    public:

    TimeRiding() : secsMovingOrPedaling(0.0) {}
    QString name() const { return "time_riding"; }
    QString units(bool) const { return "seconds"; }
    double value(bool) const { return secsMovingOrPedaling; }
    void perPoint(const RawFilePoint *point, double secsDelta, 
                  const RawFile *, const Zones *, int) {
        if ((point->mph > 0.0) || (point->cad > 0.0))
            secsMovingOrPedaling += secsDelta;
    }
    void aggregateWith(RideMetric *other) {
        secsMovingOrPedaling += other->value(true);
    }
    RideMetric *clone() const { return new TimeRiding(*this); }
};

static bool timeRidingAdded =
    RideMetricFactory::instance().addMetric(TimeRiding());

//////////////////////////////////////////////////////////////////////////////

class TotalDistance : public RideMetric {
    double miles;

    public:

    TotalDistance() : miles(0.0) {}
    QString name() const { return "total_distance"; }
    QString units(bool metric) const { return metric ? "km" : "miles"; }
    double value(bool metric) const {
        return metric ? (miles / MILES_PER_KM) : miles;
    }
    void compute(const RawFile *raw, const Zones *, int) {
        miles = raw->points.back()->miles;
    }
    void aggregateWith(RideMetric *other) { miles += other->value(false); }
    RideMetric *clone() const { return new TotalDistance(*this); }
};

static bool totalDistanceAdded =
    RideMetricFactory::instance().addMetric(TotalDistance());

//////////////////////////////////////////////////////////////////////////////

class TotalWork : public PointwiseRideMetric {
    double joules;

    public:

    TotalWork() : joules(0.0) {}
    QString name() const { return "total_work"; }
    QString units(bool) const { return "kJ"; }
    double value(bool) const { return joules / 1000.0; }
    void perPoint(const RawFilePoint *point, double secsDelta, 
                  const RawFile *, const Zones *, int) {
        if (point->watts >= 0.0)
            joules += point->watts * secsDelta;
    }
    void aggregateWith(RideMetric *other) { 
        assert(name() == other->name());
        TotalWork *tw = dynamic_cast<TotalWork*>(other);
        joules += tw->joules;
    }
    RideMetric *clone() const { return new TotalWork(*this); }
};

static bool totalWorkAdded =
    RideMetricFactory::instance().addMetric(TotalWork());

//////////////////////////////////////////////////////////////////////////////

class AvgSpeed : public PointwiseRideMetric {
    double secsMoving;
    double miles;

    public:

    AvgSpeed() : secsMoving(0.0), miles(0.0) {}
    QString name() const { return "average_speed"; }
    QString units(bool metric) const { return metric ? "kph" : "mph"; }
    double value(bool metric) const {
        if (secsMoving == 0.0) return 0.0;
        double mph = miles / secsMoving * 3600.0;
        return metric ? (mph / MILES_PER_KM) : mph;
    }
    void compute(const RawFile *raw, const Zones *zones, int zoneRange) {
        PointwiseRideMetric::compute(raw, zones, zoneRange);
        miles = raw->points.back()->miles;
    }
    void perPoint(const RawFilePoint *point, double secsDelta, 
                  const RawFile *, const Zones *, int) {
        if (point->mph > 0.0) secsMoving += secsDelta;
    }
    void aggregateWith(RideMetric *other) { 
        assert(name() == other->name());
        AvgSpeed *as = dynamic_cast<AvgSpeed*>(other);
        secsMoving += as->secsMoving;
        miles += as->miles;
    }
    RideMetric *clone() const { return new AvgSpeed(*this); }
};

static bool avgSpeedAdded =
    RideMetricFactory::instance().addMetric(AvgSpeed());

//////////////////////////////////////////////////////////////////////////////

struct AvgPower : public AvgRideMetric {

    QString name() const { return "average_power"; }
    QString units(bool) const { return "watts"; }
    void perPoint(const RawFilePoint *point, double, 
                  const RawFile *, const Zones *, int) {
        if (point->watts >= 0.0) {
            total += point->watts;
            ++count;
        }
    }
    RideMetric *clone() const { return new AvgPower(*this); }
};

static bool avgPowerAdded =
    RideMetricFactory::instance().addMetric(AvgPower());

//////////////////////////////////////////////////////////////////////////////

struct AvgHeartRate : public AvgRideMetric {

    QString name() const { return "average_hr"; }
    QString units(bool) const { return "bpm"; }
    void perPoint(const RawFilePoint *point, double, 
                  const RawFile *, const Zones *, int) {
        if (point->hr > 0) {
            total += point->hr;
            ++count;
        }
    }
    RideMetric *clone() const { return new AvgHeartRate(*this); }
};

static bool avgHeartRateAdded =
    RideMetricFactory::instance().addMetric(AvgHeartRate());

//////////////////////////////////////////////////////////////////////////////

struct AvgCadence : public AvgRideMetric {

    QString name() const { return "average_cad"; }
    QString units(bool) const { return "bpm"; }
    void perPoint(const RawFilePoint *point, double, 
                  const RawFile *, const Zones *, int) {
        if (point->cad > 0) {
            total += point->cad;
            ++count;
        }
    }
    RideMetric *clone() const { return new AvgCadence(*this); }
};

static bool avgCadenceAdded =
    RideMetricFactory::instance().addMetric(AvgCadence());

