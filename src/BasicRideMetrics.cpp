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
#include "LTMOutliers.h"
#include "Units.h"
#include "math.h"
#include <algorithm>
#include <QVector>

#define tr(s) QObject::tr(s)

class RideCount : public RideMetric {
    public:

    RideCount()
    {
        setSymbol("ride_count");
        setName(tr("Rides"));
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
    }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
        setValue(1);
    }
    RideMetric *clone() const { return new RideCount(*this); }
};

static bool countAdded =
    RideMetricFactory::instance().addMetric(RideCount());

//////////////////////////////////////////////////////////////////////////////
class WorkoutTime : public RideMetric {
    double seconds;

    public:

    WorkoutTime() : seconds(0.0)
    {
        setSymbol("workout_time");
        setName(tr("Duration"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
        if (!ride->dataPoints().isEmpty()) { 
            seconds = ride->dataPoints().back()->secs -
                      ride->dataPoints().front()->secs + ride->recIntSecs();
        } else {
            seconds = 0;
        }
        setValue(seconds);
        
    }
    RideMetric *clone() const { return new WorkoutTime(*this); }
};

static bool workoutTimeAdded =
    RideMetricFactory::instance().addMetric(WorkoutTime());

//////////////////////////////////////////////////////////////////////////////

class TimeRiding : public RideMetric {
    double secsMovingOrPedaling;

    public:

    TimeRiding() : secsMovingOrPedaling(0.0)
    {
        setSymbol("time_riding");
        setName(tr("Time Riding"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
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
    double km;

    public:

    TotalDistance() : km(0.0)
    {
        setSymbol("total_distance");
        setName(tr("Distance"));
        setType(RideMetric::Total);
        setMetricUnits(tr("km"));
        setImperialUnits(tr("miles"));
        setPrecision(2);
        setConversion(MILES_PER_KM);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
        // Note: The 'km' in each sample is the distance travelled by the
        // *end* of the sampling period.  The last term in this equation
        // accounts for the distance traveled *during* the first sample.
        if (!ride->dataPoints().isEmpty()) {
            km = ride->dataPoints().back()->km - ride->dataPoints().front()->km
                + ride->dataPoints().front()->kph / 3600.0 * ride->recIntSecs();
        } else {
            km = 0;
        }
        setValue(km);
    }

    RideMetric *clone() const { return new TotalDistance(*this); }
};

static bool totalDistanceAdded =
    RideMetricFactory::instance().addMetric(TotalDistance());

//////////////////////////////////////////////////////////////////////////////


class ElevationGain : public RideMetric {
    double elegain;
    double prevalt;

    public:

    ElevationGain() : elegain(0.0), prevalt(0.0)
    {
        setSymbol("elevation_gain");
        setName(tr("Elevation Gain"));
        setType(RideMetric::Total);
        setMetricUnits(tr("meters"));
        setImperialUnits(tr("feet"));
        setConversion(FEET_PER_METER);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
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
    double joules;

    public:

    TotalWork() : joules(0.0)
    {
        setSymbol("total_work");
        setName(tr("Work"));
        setMetricUnits(tr("kJ"));
        setImperialUnits(tr("kJ"));
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
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
    double secsMoving;
    double km;

    public:

    AvgSpeed() : secsMoving(0.0), km(0.0)
    {
        setSymbol("average_speed");
        setName(tr("Average Speed"));
        setMetricUnits(tr("kph"));
        setImperialUnits(tr("mph"));
        setType(RideMetric::Average);
        setPrecision(1);
        setConversion(MILES_PER_KM);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const MainWindow *) {
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
class Pace : public RideMetric {
    double pace;

    public:

    Pace() : pace(0.0)
    {
        setSymbol("pace");
        setName(tr("Minute Mile Pace"));
        setType(RideMetric::Average);
        setMetricUnits(tr("min/mile"));
        setImperialUnits(tr("min/mile"));
        setPrecision(1);
    }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const MainWindow *) {

        AvgSpeed *as = dynamic_cast<AvgSpeed*>(deps.value("average_speed"));

        // divide by zero or stupidly low pace
        if (as->value(true) > 0.00f) pace = 60.0f / (as->value(true) * MILES_PER_KM); // kph to minutes per mile
        else pace = 0;

        setValue(pace);
        setCount(as->count());
    }
    RideMetric *clone() const { return new Pace(*this); }
};

static bool addPace()
{
    QVector<QString> deps;
    deps.append("average_speed");
    RideMetricFactory::instance().addMetric(Pace(), &deps);
    return true;
}
static bool paceAdded = addPace();

//////////////////////////////////////////////////////////////////////////////

struct AvgPower : public RideMetric {

    double count, total;

    AvgPower()
    {
        setSymbol("average_power");
        setName(tr("Average Power"));
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
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

struct NonZeroPower : public RideMetric {

    double count, total;

    NonZeroPower()
    {
        setSymbol("nonzero_power");
        setName(tr("Nonzero Average Power"));
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
        total = count = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->watts > 0.0) {
                total += point->watts;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }
    RideMetric *clone() const { return new NonZeroPower(*this); }
};

static bool nonZeroPowerAdded =
    RideMetricFactory::instance().addMetric(NonZeroPower());

//////////////////////////////////////////////////////////////////////////////

struct AvgHeartRate : public RideMetric {

    double total, count;

    AvgHeartRate()
    {
        setSymbol("average_hr");
        setName(tr("Average Heart Rate"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
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

struct AvgCadence : public RideMetric {

    double total, count;

    AvgCadence()
    {
        setSymbol("average_cad");
        setName(tr("Average Cadence"));
        setMetricUnits(tr("rpm"));
        setImperialUnits(tr("rpm"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
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

struct AvgTemp : public RideMetric {

    double total, count;

    AvgTemp()
    {
        setSymbol("average_temp");
        setName(tr("Average Temp"));
        setMetricUnits(tr("C"));
        setImperialUnits(tr("F"));
        setPrecision(1);
        setConversion(FAHRENHEIT_PER_CENTIGRADE);
        setConversionSum(FAHRENHEIT_ADD_CENTIGRADE);
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {

        if (ride->areDataPresent()->temp) {
            total = count = 0;
            foreach (const RideFilePoint *point, ride->dataPoints()) {
                if (point->temp != RideFile::noTemp) {
                    total += point->temp;
                    ++count;
                }
            }
            setValue(count > 0 ? total / count : count);
            setCount(count);
        } else {
            setValue(RideFile::noTemp);
            setCount(1);
        }
    }
    RideMetric *clone() const { return new AvgTemp(*this); }
};

static bool avgTempAdded =
    RideMetricFactory::instance().addMetric(AvgTemp());

//////////////////////////////////////////////////////////////////////////////

class MaxPower : public RideMetric {
    double max;
    public:
    MaxPower() : max(0.0)
    {
        setSymbol("max_power");
        setName(tr("Max Power"));
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setType(RideMetric::Peak);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
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
    double max;
    public:
    MaxHr() : max(0.0)
    {
        setSymbol("max_heartrate");
        setName(tr("Max Heartrate"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
        setType(RideMetric::Peak);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
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

class MaxSpeed : public RideMetric {
    public:

    MaxSpeed()
    {
        setSymbol("max_speed");
        setName(tr("Max Speed"));
        setMetricUnits(tr("kph"));
        setImperialUnits(tr("mph"));
        setType(RideMetric::Peak);
        setPrecision(1);
        setConversion(MILES_PER_KM);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
        double max = 0.0;
        foreach (const RideFilePoint *point, ride->dataPoints())
            if (point->kph > max) max = point->kph;

        setValue(max);
    }

    void aggregateWith(const RideMetric &other) {
        assert(symbol() == other.symbol());
        const MaxSpeed &ms = dynamic_cast<const MaxSpeed&>(other);

        setValue(ms.value(true) > value(true) ? ms.value(true) : value(true));
    }
    RideMetric *clone() const { return new MaxSpeed(*this); }
};

static bool maxSpeedAdded =
    RideMetricFactory::instance().addMetric(MaxSpeed());

//////////////////////////////////////////////////////////////////////////////

class MaxCadence : public RideMetric {
    public:

    MaxCadence()
    {
        setSymbol("max_cadence");
        setName(tr("Max Cadence"));
        setMetricUnits(tr("rpm"));
        setImperialUnits(tr("rpm"));
        setType(RideMetric::Peak);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
        double max = 0.0;
        foreach (const RideFilePoint *point, ride->dataPoints())
            if (point->cad > max) max = point->cad;

        setValue(max);
    }

    void aggregateWith(const RideMetric &other) {
        assert(symbol() == other.symbol());
        const MaxCadence &mc = dynamic_cast<const MaxCadence&>(other);

        setValue(mc.value(true) > value(true) ? mc.value(true) : value(true));
    }
    RideMetric *clone() const { return new MaxCadence(*this); }
};

static bool maxCadenceAdded =
    RideMetricFactory::instance().addMetric(MaxCadence());

//////////////////////////////////////////////////////////////////////////////

class MaxTemp : public RideMetric {
    public:

    MaxTemp()
    {
        setSymbol("max_temp");
        setName(tr("Max Temp"));
        setMetricUnits(tr("C"));
        setImperialUnits(tr("F"));
        setType(RideMetric::Peak);
        setPrecision(1);
        setConversion(FAHRENHEIT_PER_CENTIGRADE);
        setConversionSum(FAHRENHEIT_ADD_CENTIGRADE);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {

        if (ride->areDataPresent()->temp) {
            double max = 0.0;
            foreach (const RideFilePoint *point, ride->dataPoints())
                if (point->temp != RideFile::noTemp && point->temp > max) max = point->temp;

            setValue(max);
        } else {
            setValue(RideFile::noTemp);
        }
    }

    void aggregateWith(const RideMetric &other) {
        assert(symbol() == other.symbol());
        const MaxTemp &mc = dynamic_cast<const MaxTemp&>(other);

        setValue(mc.value(true) > value(true) ? mc.value(true) : value(true));
    }
    RideMetric *clone() const { return new MaxTemp(*this); }
};

static bool maxTempAdded =
    RideMetricFactory::instance().addMetric(MaxTemp());

//////////////////////////////////////////////////////////////////////////////

class NinetyFivePercentHeartRate : public RideMetric {
    double hr;
    public:
    NinetyFivePercentHeartRate() : hr(0.0)
    {
        setSymbol("ninety_five_percent_hr");
        setName(tr("95% Heartrate"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
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

///////////////////////////////////////////////////////////////////////////////

class VAM : public RideMetric {

    public:
    VAM()
    {
        setSymbol("vam");
        setName(tr("VAM"));
        setImperialUnits("");
        setMetricUnits("");
        setType(RideMetric::Average);
    }
    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const MainWindow *) {

        ElevationGain *el = dynamic_cast<ElevationGain*>(deps.value("elevation_gain"));
        WorkoutTime *wt = dynamic_cast<WorkoutTime*>(deps.value("workout_time"));
        setValue((el->value(true)*3600)/wt->value(true));
    }
    RideMetric *clone() const { return new VAM(*this); }
};

static bool addVam()
{
    QVector<QString> deps;
    deps.append("elevation_gain");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(VAM(), &deps);
    return true;
}

static bool vamAdded = addVam();

///////////////////////////////////////////////////////////////////////////////

class Gradient : public RideMetric {

    public:
    Gradient()
    {
        setSymbol("gradient");
        setName(tr("Gradient"));
        setImperialUnits("%");
        setMetricUnits("%");
        setPrecision(1);
        setType(RideMetric::Average);
    }
    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const MainWindow *) {

        ElevationGain *el = dynamic_cast<ElevationGain*>(deps.value("elevation_gain"));
        TotalDistance *td = dynamic_cast<TotalDistance*>(deps.value("total_distance"));
        if (td->value(true) && el->value(true)) {
            setValue(100.00 *(el->value(true) / (1000 *td->value(true))));
        } else
            setValue(0.0);
    }
    RideMetric *clone() const { return new Gradient(*this); }
};

static bool addGradient()
{
    QVector<QString> deps;
    deps.append("elevation_gain");
    deps.append("total_distance");
    RideMetricFactory::instance().addMetric(Gradient(), &deps);
    return true;
}

static bool gradientAdded = addGradient();

///////////////////////////////////////////////////////////////////////////////

class MeanPowerVariance : public RideMetric {

    public:
    LTMOutliers *outliers;

    MeanPowerVariance()
    {
        outliers = NULL;
        setSymbol("meanpowervariance");
        setName(tr("Average Power Variance"));
        setImperialUnits("watts change");
        setMetricUnits("watts change");
        setPrecision(2);
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {

        // Less than 30s don't bother
        if (ride->dataPoints().count() < 30)
            setValue(0);
        else {

            QVector<double> power;
            QVector<double> secs;
            foreach (RideFilePoint *point, ride->dataPoints()) {
                power.append(point->watts);
                secs.append(point->secs);
            }

            if (outliers) delete outliers;
            outliers = new LTMOutliers(secs.data(), power.data(), power.count(), 30, false);
            setValue(outliers->getStdDeviation());
        }
    }
    RideMetric *clone() const { return new MeanPowerVariance(*this); }
};

static bool addMeanPowerVariance()
{
    RideMetricFactory::instance().addMetric(MeanPowerVariance());
    return true;
}

static bool meanPowerVarianceAdded = addMeanPowerVariance();

///////////////////////////////////////////////////////////////////////////////

class MaxPowerVariance : public RideMetric {

    public:
    MaxPowerVariance()
    {
        setSymbol("maxpowervariance");
        setName(tr("Max Power Variance"));
        setImperialUnits("watts change");
        setMetricUnits("watts change");
        setPrecision(2);
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const MainWindow *) {

        MeanPowerVariance *mean = dynamic_cast<MeanPowerVariance*>(deps.value("meanpowervariance"));
        if (ride->dataPoints().count() < 30)
            setValue(0);
        else
            setValue(mean->outliers->getYForRank(0));
    }
    RideMetric *clone() const { return new MaxPowerVariance(*this); }
};

static bool addMaxPowerVariance()
{
    QVector<QString> deps;
    deps.append("meanpowervariance");
    RideMetricFactory::instance().addMetric(MaxPowerVariance(), &deps);
    return true;
}

static bool maxPowerVarianceAdded = addMaxPowerVariance();

//////////////////////////////////////////////////////////////////////////////
