/* 
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net)
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

#include "RideMetric.h"

#define MILES_PER_KM 0.62137119

class WorkoutTime : public RideMetric {
    double seconds;

    public:

    WorkoutTime() : seconds(0.0) {}
    QString name() const { return "workout_time"; }
    QString units(bool) const { return "seconds"; }
    double value(bool) const { return seconds; }
    void compute(const RideFile *ride, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {
        seconds = ride->dataPoints().back()->secs;
    }
    bool canAggregate() const { return true; }
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
    void perPoint(const RideFilePoint *point, double secsDelta, 
                  const RideFile *, const Zones *, int) {
        if ((point->kph > 0.0) || (point->cad > 0.0))
            secsMovingOrPedaling += secsDelta;
    }
    bool canAggregate() const { return true; }
    void aggregateWith(RideMetric *other) {
        secsMovingOrPedaling += other->value(true);
    }
    RideMetric *clone() const { return new TimeRiding(*this); }
};

static bool timeRidingAdded =
    RideMetricFactory::instance().addMetric(TimeRiding());

//////////////////////////////////////////////////////////////////////////////

class TotalDistance : public RideMetric {
    double km;

    public:

    TotalDistance() : km(0.0) {}
    QString name() const { return "total_distance"; }
    QString units(bool metric) const { return metric ? "km" : "miles"; }
    double value(bool metric) const {
        return metric ? km : (km * MILES_PER_KM);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {
        km = ride->dataPoints().back()->km;
    }
    bool canAggregate() const { return true; }
    void aggregateWith(RideMetric *other) { km += other->value(true); }
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
    void perPoint(const RideFilePoint *point, double secsDelta, 
                  const RideFile *, const Zones *, int) {
        if (point->watts >= 0.0)
            joules += point->watts * secsDelta;
    }
    bool canAggregate() const { return true; }
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
    double km;

    public:

    AvgSpeed() : secsMoving(0.0), km(0.0) {}
    QString name() const { return "average_speed"; }
    QString units(bool metric) const { return metric ? "kph" : "mph"; }
    double value(bool metric) const {
        if (secsMoving == 0.0) return 0.0;
        double kph = km / secsMoving * 3600.0;
        return metric ? kph : (kph * MILES_PER_KM);
    }
    void compute(const RideFile *ride, const Zones *zones, int zoneRange,
                 const QHash<QString,RideMetric*> &deps) {
        PointwiseRideMetric::compute(ride, zones, zoneRange, deps);
        km = ride->dataPoints().back()->km;
    }
    void perPoint(const RideFilePoint *point, double secsDelta, 
                  const RideFile *, const Zones *, int) {
        if (point->kph > 0.0) secsMoving += secsDelta;
    }
    bool canAggregate() const { return true; }
    void aggregateWith(RideMetric *other) { 
        assert(name() == other->name());
        AvgSpeed *as = dynamic_cast<AvgSpeed*>(other);
        secsMoving += as->secsMoving;
        km += as->km;
    }
    RideMetric *clone() const { return new AvgSpeed(*this); }
};

static bool avgSpeedAdded =
    RideMetricFactory::instance().addMetric(AvgSpeed());

//////////////////////////////////////////////////////////////////////////////

struct AvgPower : public AvgRideMetric {

    QString name() const { return "average_power"; }
    QString units(bool) const { return "watts"; }
    void perPoint(const RideFilePoint *point, double, 
                  const RideFile *, const Zones *, int) {
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
    void perPoint(const RideFilePoint *point, double, 
                  const RideFile *, const Zones *, int) {
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
    void perPoint(const RideFilePoint *point, double, 
                  const RideFile *, const Zones *, int) {
        if (point->cad > 0) {
            total += point->cad;
            ++count;
        }
    }
    RideMetric *clone() const { return new AvgCadence(*this); }
};

static bool avgCadenceAdded =
    RideMetricFactory::instance().addMetric(AvgCadence());

