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
#include <algorithm>

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
        seconds = ride->dataPoints().back()->secs -
            ride->dataPoints().front()->secs + ride->recIntSecs();
    }
    bool canAggregate() const { return true; }
    void aggregateWith(const RideMetric &other) { seconds += other.value(true); }
    RideMetric *clone() const { return new WorkoutTime(*this); }
};

static bool workoutTimeAdded =
    RideMetricFactory::instance().addMetric(WorkoutTime());

//////////////////////////////////////////////////////////////////////////////

class TimeRiding : public RideMetric {
    double secsMovingOrPedaling;

    public:

    TimeRiding() : secsMovingOrPedaling(0.0) {}
    QString symbol() const { return "time_riding"; }
    QString name() const { return tr("Time Riding"); }
    QString units(bool) const { return "seconds"; }
    int precision() const { return 0; }
    double value(bool) const { return secsMovingOrPedaling; }
    void compute(const RideFile *ride, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if ((point->kph > 0.0) || (point->cad > 0.0))
                secsMovingOrPedaling += ride->recIntSecs();
        }
    }
    bool canAggregate() const { return true; }
    void aggregateWith(const RideMetric &other) {
        secsMovingOrPedaling += other.value(true);
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
        // Note: The 'km' in each sample is the distance travelled by the
        // *end* of the sampling period.  The last term in this equation
        // accounts for the distance traveled *during* the first sample.
        km = ride->dataPoints().back()->km - ride->dataPoints().front()->km
            + ride->dataPoints().front()->kph / 3600.0 * ride->recIntSecs();
    }
    bool canAggregate() const { return true; }
    void aggregateWith(const RideMetric &other) { km += other.value(true); }
    RideMetric *clone() const { return new TotalDistance(*this); }
};

static bool totalDistanceAdded =
    RideMetricFactory::instance().addMetric(TotalDistance());

//////////////////////////////////////////////////////////////////////////////


class ElevationGain : public RideMetric {
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
    void compute(const RideFile *ride, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {
        bool first = true;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (first)
                first = false;
            else if (point->alt > prevalt)
                elegain += point->alt - prevalt;
            prevalt = point->alt;
        }
    }
    bool canAggregate() const { return true; }
    void aggregateWith(const RideMetric &other) {
        elegain += other.value(true);
    }
    RideMetric *clone() const { return new ElevationGain(*this); }
};

static bool elevationGainAdded =
    RideMetricFactory::instance().addMetric(ElevationGain());

//////////////////////////////////////////////////////////////////////////////

class TotalWork : public RideMetric {
    double joules;

    public:

    TotalWork() : joules(0.0) {}
    QString symbol() const { return "total_work"; }
    QString name() const { return tr("Work"); }
    QString units(bool) const { return "kJ"; }
    int precision() const { return 0; }
    double value(bool) const { return joules / 1000.0; }
    void compute(const RideFile *ride, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->watts >= 0.0)
                joules += point->watts * ride->recIntSecs();
        }
    }
    bool canAggregate() const { return true; }
    void aggregateWith(const RideMetric &other) {
        assert(symbol() == other.symbol());
        const TotalWork &tw = dynamic_cast<const TotalWork&>(other);
        joules += tw.joules;
    }
    RideMetric *clone() const { return new TotalWork(*this); }
};

static bool totalWorkAdded =
    RideMetricFactory::instance().addMetric(TotalWork());

//////////////////////////////////////////////////////////////////////////////

class AvgSpeed : public RideMetric {
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
    void compute(const RideFile *ride, const Zones *, int,
                 const QHash<QString,RideMetric*> &deps) {
        assert(deps.contains("total_distance"));
        km = deps.value("total_distance")->value(true);
        foreach (const RideFilePoint *point, ride->dataPoints())
            if (point->kph > 0.0) secsMoving += ride->recIntSecs();
    }
    bool canAggregate() const { return true; }
    void aggregateWith(const RideMetric &other) { 
        assert(symbol() == other.symbol());
        const AvgSpeed &as = dynamic_cast<const AvgSpeed&>(other);
        secsMoving += as.secsMoving;
        km += as.km;
    }
    RideMetric *clone() const { return new AvgSpeed(*this); }
};

static bool avgSpeedAdded =
    RideMetricFactory::instance().addMetric(
        AvgSpeed(), &(QVector<QString>() << "total_distance"));

//////////////////////////////////////////////////////////////////////////////

struct AvgPower : public AvgRideMetric {

    QString symbol() const { return "average_power"; }
    QString name() const { return tr("Average Power"); }
    QString units(bool) const { return "watts"; }
    int precision() const { return 0; }
    void compute(const RideFile *ride, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->watts >= 0.0) {
                total += point->watts;
                ++count;
            }
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
    void compute(const RideFile *ride, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->hr > 0) {
                total += point->hr;
                ++count;
            }
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
    void compute(const RideFile *ride, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->cad > 0) {
                total += point->cad;
                ++count;
            }
        }
    }
    RideMetric *clone() const { return new AvgCadence(*this); }
};

static bool avgCadenceAdded =
    RideMetricFactory::instance().addMetric(AvgCadence());

//////////////////////////////////////////////////////////////////////////////

class MaxPower : public RideMetric {
    double max;
    public:
    MaxPower() : max(0.0) {}
    QString symbol() const { return "max_power"; }
    QString name() const { return tr("Max Power"); }
    QString units(bool) const { return "watts"; }
    int precision() const { return 0; }
    double value(bool) const { return max; }
    void compute(const RideFile *ride, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->watts >= max)
                max = point->watts;
        }
    }
    RideMetric *clone() const { return new MaxPower(*this); }
};

static bool maxPowerAdded =
    RideMetricFactory::instance().addMetric(MaxPower());

//////////////////////////////////////////////////////////////////////////////

class NinetyFivePercentHeartRate : public RideMetric {
    double hr;
    public:
    NinetyFivePercentHeartRate() : hr(0.0) {}
    QString symbol() const { return "ninety_five_percent_hr"; }
    QString name() const { return tr("95% Heart Rate"); }
    QString units(bool) const { return "bpm"; }
    int precision() const { return 0; }
    double value(bool) const { return hr; }
    void compute(const RideFile *ride, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {
        QVector<double> hrs;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->hr >= 0.0)
                hrs.append(point->hr);
        }
        if (hrs.size() > 0) {
            std::sort(hrs.begin(), hrs.end());
            hr = hrs[hrs.size() * 0.95];
        }
    }
    RideMetric *clone() const { return new NinetyFivePercentHeartRate(*this); }
};

static bool ninetyFivePercentHeartRateAdded =
    RideMetricFactory::instance().addMetric(NinetyFivePercentHeartRate());

