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
#include "Units.h"

#define tr(s) QObject::tr(s)

class WorkoutTime : public RideMetric {
    double seconds;

    public:

    WorkoutTime() : seconds(0.0) {}
    QString symbol() const { return "workout_time"; }
    QString name() const { return tr("Duration"); }
    QString units(bool) const { return "seconds"; }
    int precision() const { return 0; }
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
    QString symbol() const { return "time_riding"; }
    QString name() const { return tr("Time Riding"); }
    QString units(bool) const { return "seconds"; }
    int precision() const { return 0; }
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
    QString symbol() const { return "total_distance"; }
    QString name() const { return tr("Distance"); }
    QString units(bool metric) const { return metric ? "km" : "miles"; }
    int precision() const { return 1; }
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


class ElevationGain : public PointwiseRideMetric {
    double elegain;
    double prevalt;

    public:

    ElevationGain() : elegain(0.0), prevalt(0.0) {}
    QString symbol() const { return "elevation_gain"; }
    QString name() const { return tr("Elevation Gain"); }
    QString units(bool metric) const { return metric ? "meters" : "feet"; }
    int precision() const { return 1; }
    double value(bool metric) const {
        return metric ? elegain : (elegain * FEET_PER_METER);
    }
    void perPoint(const RideFilePoint *point, double,
                  const RideFile *, const Zones *, int) {

	if (prevalt <= 0){
		prevalt = point->alt;
	} else if (prevalt <= point->alt) {
		elegain += (point->alt-prevalt);
		prevalt = point->alt;
	} else {
		prevalt = point->alt;
	}

    }
    bool canAggregate() const { return true; }
    void aggregateWith(RideMetric *other) {
        elegain += other->value(true);
    }
    RideMetric *clone() const { return new ElevationGain(*this); }
};

static bool elevationGainAdded =
    RideMetricFactory::instance().addMetric(ElevationGain());

//////////////////////////////////////////////////////////////////////////////

class TotalWork : public PointwiseRideMetric {
    double joules;

    public:

    TotalWork() : joules(0.0) {}
    QString symbol() const { return "total_work"; }
    QString name() const { return tr("Work"); }
    QString units(bool) const { return "kJ"; }
    int precision() const { return 0; }
    double value(bool) const { return joules / 1000.0; }
    void perPoint(const RideFilePoint *point, double secsDelta, 
                  const RideFile *, const Zones *, int) {
        if (point->watts >= 0.0)
            joules += point->watts * secsDelta;
    }
    bool canAggregate() const { return true; }
    void aggregateWith(RideMetric *other) { 
        assert(symbol() == other->symbol());
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
    QString symbol() const { return "average_speed"; }
    QString name() const { return tr("Average Speed"); }
    QString units(bool metric) const { return metric ? "kph" : "mph"; }
    int precision() const { return 1; }
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
        assert(symbol() == other->symbol());
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

    QString symbol() const { return "average_power"; }
    QString name() const { return tr("Average Power"); }
    QString units(bool) const { return "watts"; }
    int precision() const { return 0; }
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

    QString symbol() const { return "average_hr"; }
    QString name() const { return tr("Average Heart Rate"); }
    QString units(bool) const { return "bpm"; }
    int precision() const { return 0; }
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

    QString symbol() const { return "average_cad"; }
    QString name() const { return tr("Average Cadence"); }
    QString units(bool) const { return "rpm"; }
    int precision() const { return 0; }
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

