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
#include <QApplication>

class WorkoutTime : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(WorkoutTime)

    double seconds;

    public:

    WorkoutTime() : seconds(0.0)
    {
        setSymbol("workout_time");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Duration");
    }
    void initialize() {
#endif
        setName(tr("Duration"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
    }
    void compute(const RideFile *ride, const Zones *, int, const HrZones *, int,
                 const QHash<QString,RideMetric*> &) {
        seconds = ride->dataPoints().back()->secs -
                  ride->dataPoints().front()->secs + ride->recIntSecs();
        setValue(seconds);
    }
    RideMetric *clone() const { return new WorkoutTime(*this); }
};

static bool workoutTimeAdded =
    RideMetricFactory::instance().addMetric(WorkoutTime());

//////////////////////////////////////////////////////////////////////////////

class TimeRiding : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TimeRiding)

    double secsMovingOrPedaling;

    public:

    TimeRiding() : secsMovingOrPedaling(0.0)
    {
        setSymbol("time_riding");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Time Riding");
    }
    void initialize() {
#endif
        setName(tr("Time Riding"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
    }
    void compute(const RideFile *ride, const Zones *, int, const HrZones *, int,
                 const QHash<QString,RideMetric*> &) {
        secsMovingOrPedaling = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if ((point->kph > 0.0) || (point->cad > 0.0))
                secsMovingOrPedaling += ride->recIntSecs();
        }
        setValue(secsMovingOrPedaling);
    }
    void override(const QMap<QString,QString> &map) {
        if (map.contains("value"))
            secsMovingOrPedaling = map.value("value").toDouble();
    }
    RideMetric *clone() const { return new TimeRiding(*this); }
};

static bool timeRidingAdded =
    RideMetricFactory::instance().addMetric(TimeRiding());

//////////////////////////////////////////////////////////////////////////////

class TotalDistance : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TotalDistance)

    double km;

    public:

    TotalDistance() : km(0.0)
    {
        setSymbol("total_distance");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Distance");
    }
    void initialize() {
#endif
        setName(tr("Distance"));
        setType(RideMetric::Total);
        setMetricUnits(tr("km"));
        setImperialUnits(tr("miles"));
        setPrecision(1);
        setConversion(MILES_PER_KM);
    }
    void compute(const RideFile *ride, const Zones *, int, const HrZones *, int,
                 const QHash<QString,RideMetric*> &) {
        // Note: The 'km' in each sample is the distance travelled by the
        // *end* of the sampling period.  The last term in this equation
        // accounts for the distance traveled *during* the first sample.
        km = ride->dataPoints().back()->km - ride->dataPoints().front()->km
            + ride->dataPoints().front()->kph / 3600.0 * ride->recIntSecs();
        setValue(km);
    }

    RideMetric *clone() const { return new TotalDistance(*this); }
};

static bool totalDistanceAdded =
    RideMetricFactory::instance().addMetric(TotalDistance());

//////////////////////////////////////////////////////////////////////////////


class ElevationGain : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ElevationGain)

    double elegain;
    double prevalt;

    public:

    ElevationGain() : elegain(0.0), prevalt(0.0)
    {
        setSymbol("elevation_gain");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Elevation Gain");
    }
    void initialize() {
#endif
        setName(tr("Elevation Gain"));
        setType(RideMetric::Total);
        setMetricUnits(tr("meters"));
        setImperialUnits(tr("feet"));
        setConversion(FEET_PER_METER);
    }
    void compute(const RideFile *ride, const Zones *, int, const HrZones *, int,
                 const QHash<QString,RideMetric*> &) {
        const double hysteresis = 3.0;
        bool first = true;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (first) {
                first = false;
                prevalt = point->alt;
            }
            else if (point->alt > prevalt + hysteresis) {
                elegain += point->alt - prevalt;
                prevalt = point->alt;
            }
            else if (point->alt < prevalt - hysteresis) {
                prevalt = point->alt;
            }
        }
        setValue(elegain);
    }
    RideMetric *clone() const { return new ElevationGain(*this); }
};

static bool elevationGainAdded =
    RideMetricFactory::instance().addMetric(ElevationGain());

//////////////////////////////////////////////////////////////////////////////

class TotalWork : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TotalWork)

    double joules;

    public:

    TotalWork() : joules(0.0)
    {
        setSymbol("total_work");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Work");
    }
    void initialize() {
#endif
        setName(tr("Work"));
        setMetricUnits(tr("kJ"));
        setImperialUnits(tr("kJ"));
    }
    void compute(const RideFile *ride, const Zones *, int, const HrZones *, int,
                 const QHash<QString,RideMetric*> &) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->watts >= 0.0)
                joules += point->watts * ride->recIntSecs();
        }
        setValue(joules/1000);
    }
    RideMetric *clone() const { return new TotalWork(*this); }
};

static bool totalWorkAdded =
    RideMetricFactory::instance().addMetric(TotalWork());

//////////////////////////////////////////////////////////////////////////////

class AvgSpeed : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgSpeed)

    double secsMoving;
    double km;

    public:

    AvgSpeed() : secsMoving(0.0), km(0.0)
    {
        setSymbol("average_speed");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Average Speed");
    }
    void initialize() {
#endif
        setName(tr("Average Speed"));
        setMetricUnits(tr("km/h"));
        setImperialUnits(tr("mph"));
        setType(RideMetric::Average);
        setPrecision(1);
        setConversion(MILES_PER_KM);
    }
    void compute(const RideFile *ride, const Zones *, int, const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps) {
        assert(deps.contains("total_distance"));
        km = deps.value("total_distance")->value(true);
        foreach (const RideFilePoint *point, ride->dataPoints())
            if (point->kph > 0.0) secsMoving += ride->recIntSecs();

        setValue(secsMoving ? km / secsMoving * 3600.0 : 0.0);
    }

    void aggregateWith(const RideMetric &other) { 
        assert(symbol() == other.symbol());
        const AvgSpeed &as = dynamic_cast<const AvgSpeed&>(other);
        secsMoving += as.secsMoving;
        km += as.km;

        setValue(secsMoving ? km / secsMoving * 3600.0 : 0.0);
    }
    RideMetric *clone() const { return new AvgSpeed(*this); }
};

static bool avgSpeedAdded =
    RideMetricFactory::instance().addMetric(
        AvgSpeed(), &(QVector<QString>() << "total_distance"));

//////////////////////////////////////////////////////////////////////////////

class AvgPower : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgPower)

    double count, total;

    public:

    AvgPower()
    {
        setSymbol("average_power");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Average Power");
    }
    void initialize() {
#endif
        setName(tr("Average Power"));
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int, const HrZones *, int,
                 const QHash<QString,RideMetric*> &) {
        total = count = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->watts >= 0.0) {
                total += point->watts;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }
    RideMetric *clone() const { return new AvgPower(*this); }
};

static bool avgPowerAdded =
    RideMetricFactory::instance().addMetric(AvgPower());

//////////////////////////////////////////////////////////////////////////////

class AvgHeartRate : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgHeartRate)

    double total, count;

    public:

    AvgHeartRate()
    {
        setSymbol("average_hr");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Average Heart Rate");
    }
    void initialize() {
#endif
        setName(tr("Average Heart Rate"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int, const HrZones *, int,
                 const QHash<QString,RideMetric*> &) {
        total = count = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->hr > 0) {
                total += point->hr;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }
    RideMetric *clone() const { return new AvgHeartRate(*this); }
};

static bool avgHeartRateAdded =
    RideMetricFactory::instance().addMetric(AvgHeartRate());

//////////////////////////////////////////////////////////////////////////////

class AvgCadence : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgCadence)

    double total, count;

    public:

    AvgCadence()
    {
        setSymbol("average_cad");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Average Cadence");
    }
    void initialize() {
#endif
        setName(tr("Average Cadence"));
        setMetricUnits(tr("rpm"));
        setImperialUnits(tr("rpm"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int, const HrZones *, int,
                 const QHash<QString,RideMetric*> &) {
        total = count = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->cad > 0) {
                total += point->cad;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : count);
        setCount(count);
    }
    RideMetric *clone() const { return new AvgCadence(*this); }
};

static bool avgCadenceAdded =
    RideMetricFactory::instance().addMetric(AvgCadence());

//////////////////////////////////////////////////////////////////////////////

class MaxPower : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MaxPower)

    double max;
    public:
    MaxPower() : max(0.0)
    {
        setSymbol("max_power");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Max Power");
    }
    void initialize() {
#endif
        setName(tr("Max Power"));
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setType(RideMetric::Peak);
    }
    void compute(const RideFile *ride, const Zones *, int, const HrZones *, int,
                 const QHash<QString,RideMetric*> &) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->watts >= max)
                max = point->watts;
        }
        setValue(max);
    }
    RideMetric *clone() const { return new MaxPower(*this); }
};

static bool maxPowerAdded =
    RideMetricFactory::instance().addMetric(MaxPower());

//////////////////////////////////////////////////////////////////////////////

class MaxHr : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MaxHr)

    double max;
    public:
    MaxHr() : max(0.0)
    {
        setSymbol("max_heartrate");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Max Heartrate");
    }
    void initialize() {
#endif
        setName(tr("Max Heartrate"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
        setType(RideMetric::Peak);
    }
    void compute(const RideFile *ride, const Zones *, int, const HrZones *, int,
                 const QHash<QString,RideMetric*> &) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->hr >= max)
                max = point->hr;
        }
        setValue(max);
    }
    RideMetric *clone() const { return new MaxHr(*this); }
};

static bool maxHrAdded =
    RideMetricFactory::instance().addMetric(MaxHr());

//////////////////////////////////////////////////////////////////////////////

class NinetyFivePercentHeartRate : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(NinetyFivePercentHeartRate)

    double hr;
    public:
    NinetyFivePercentHeartRate() : hr(0.0)
    {
        setSymbol("ninety_five_percent_hr");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("95% Heartrate");
    }
    void initialize() {
#endif
        setName(tr("95% Heartrate"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int, const HrZones *, int,
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
        setValue(hr);
    }
    RideMetric *clone() const { return new NinetyFivePercentHeartRate(*this); }
};

static bool ninetyFivePercentHeartRateAdded =
    RideMetricFactory::instance().addMetric(NinetyFivePercentHeartRate());

