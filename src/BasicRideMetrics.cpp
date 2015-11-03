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
#include "Athlete.h"
#include "Context.h"
#include "Settings.h"
#include "RideItem.h"
#include "LTMOutliers.h"
#include "Units.h"
#include "Zones.h"
#include "cmath"
#include <algorithm>
#include <QVector>
#include <QApplication>

class RideCount : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(RideCount)
    public:

    RideCount()
    {
        setSymbol("ride_count");
        setInternalName("Activities");
    }
    void initialize() {
        setName(tr("Activities"));
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
    }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
        setValue(1);
    }
    RideMetric *clone() const { return new RideCount(*this); }
};

static bool countAdded =
    RideMetricFactory::instance().addMetric(RideCount());

//////////////////////////////////////////////////////////////////////////////
class WorkoutTime : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(WorkoutTime)
    double seconds;

    public:

    WorkoutTime() : seconds(0.0)
    {
        setSymbol("workout_time");
        setInternalName("Duration");
    }
    bool isTime() const { return true; }
    void initialize() {
        setName(tr("Duration"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
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
    Q_DECLARE_TR_FUNCTIONS(TimeRiding)
    double secsMovingOrPedaling;

    public:

    TimeRiding() : secsMovingOrPedaling(0.0)
    {
        setSymbol("time_riding");
        setInternalName("Time Moving");
    }
    bool isTime() const { return true; }
    void initialize() {
        setName(tr("Time Moving"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        secsMovingOrPedaling = 0;
        if (ride->areDataPresent()->kph || ride->areDataPresent()->cad ) {
            foreach (const RideFilePoint *point, ride->dataPoints()) {
                if ((point->kph > 0.0) || (point->cad > 0.0))
                    secsMovingOrPedaling += ride->recIntSecs();
            }
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

class TimeCarrying : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TimeCarrying)
    double secsCarrying;
    double prevalt;

    public:

    TimeCarrying() : secsCarrying(0.0)
    {
        setSymbol("time_carrying");
        setInternalName("Time Carrying");
    }
    bool isTime() const { return true; }
    void initialize() {
        setName(tr("Time Carrying (Est)"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        secsCarrying = 0;
        if (ride->areDataPresent()->kph) {

            // hysteresis can be configured, we default to 3.0
            double hysteresis = appsettings->value(NULL, GC_ELEVATION_HYSTERESIS).toDouble();
            if (hysteresis <= 0.1) hysteresis = 3.00;

            bool first = true;
            foreach (const RideFilePoint *point, ride->dataPoints()) {
                // only consider pushing/carrying with elevation gain
                if (first) {
                    first = false;
                    prevalt = point->alt;
                }
                else if (point->alt > prevalt + hysteresis) {
                    prevalt = point->alt;
                }
                else if (point->alt < prevalt - hysteresis) {
                    prevalt = point->alt;
                }

                if ((point->kph > 0.0) &&          // we are moving
                    (point->kph < 8.0) &&          // but slow (even slower than 8 kph)
                    (point->alt > prevalt) &&  // gaining height
                    (point->cad == 0.0) &&     // but no cadence
                    (point->watts == 0.0))     // and no power

                    secsCarrying += ride->recIntSecs();
            }
        }
        setValue(secsCarrying);

    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new TimeCarrying(*this); }
};

static bool timeCarryingAdded =
    RideMetricFactory::instance().addMetric(TimeCarrying());

//////////////////////////////////////////////////////////////////////////////

class ElevationGainCarrying : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ElevationGain)
    double elegain;
    double prevalt;

    public:

    ElevationGainCarrying() : elegain(0.0), prevalt(0.0)
    {
        setSymbol("elevation_gain_carrying");
        setInternalName("Elevation Gain Carrying");
    }
    void initialize() {
        setName(tr("Elevation Gain Carrying (Est)"));
        setType(RideMetric::Total);
        setMetricUnits(tr("meters"));
        setImperialUnits(tr("feet"));
        setConversion(FEET_PER_METER);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        // hysteresis can be configured, we default to 3.0
        double hysteresis = appsettings->value(NULL, GC_ELEVATION_HYSTERESIS).toDouble();
        if (hysteresis <= 0.1) hysteresis = 3.00;

        bool first = true;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (first) {
                first = false;
                prevalt = point->alt;
            }
            else if (point->alt > prevalt + hysteresis) {
                if ((point->kph > 0.0) &&
                    (point->kph < 8.0) &&
                    (point->watts == 0.0) &&
                    (point->cad == 0.0)) {
                    elegain += point->alt - prevalt;
                };
                prevalt = point->alt;
            }
            else if (point->alt < prevalt - hysteresis) {
                prevalt = point->alt;
            }
        }
        setValue(elegain);
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new ElevationGainCarrying(*this); }
};

static bool elevationGainCarryingAdded =
    RideMetricFactory::instance().addMetric(ElevationGainCarrying());


//////////////////////////////////////////////////////////////////////////////

class TotalDistance : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TotalDistance)
    double km;

    public:

    TotalDistance() : km(0.0)
    {
        setSymbol("total_distance");
        setInternalName("Distance");
    }
    void initialize() {
        setName(tr("Distance"));
        setType(RideMetric::Total);
        setMetricUnits(tr("km"));
        setImperialUnits(tr("miles"));
        setPrecision(3);
        setConversion(MILES_PER_KM);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        // Note: The 'km' in each sample is the distance travelled by the
        // *end* of the sampling period.  The last term in this equation
        // accounts for the distance traveled *during* the first sample.
        if (ride->dataPoints().count() > 1 && ride->areDataPresent()->km) {

            km = ride->dataPoints().back()->km - ride->dataPoints().front()->km;

            if (ride->areDataPresent()->kph)
                km += ride->dataPoints().front()->kph / 3600.0 * ride->recIntSecs();

        } else {

            km = 0;

        }
        setValue(km);
    }

    RideMetric *clone() const { return new TotalDistance(*this); }
};

static bool totalDistanceAdded =
    RideMetricFactory::instance().addMetric(TotalDistance());

// DistanceSwim is TotalDistance in swim units, relevant for swims in yards //
class DistanceSwim : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(DistanceSwim)
    double mts;

    public:

    DistanceSwim() : mts(0.0)
    {
        setSymbol("distance_swim");
        setInternalName("Distance Swim");
    }
    // Overrides to use Swim Pace units setting
    QString units(bool) const {
        bool metricSwPace = appsettings->value(NULL, GC_SWIMPACE, true).toBool();
        return RideMetric::units(metricSwPace);
    }
    double value(bool) const {
        bool metricSwPace = appsettings->value(NULL, GC_SWIMPACE, true).toBool();
        return RideMetric::value(metricSwPace);
    }
    void initialize() {
        setName(tr("Distance Swim"));
        setType(RideMetric::Total);
        setMetricUnits(tr("m"));
        setImperialUnits(tr("yd"));
        setPrecision(0);
        setConversion(1.0/METERS_PER_YARD);
    }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        TotalDistance *distance = dynamic_cast<TotalDistance*>(deps.value("total_distance"));

        // convert to meters
        mts = distance->value(true) * 1000.0;
        setValue(mts);
        setCount(distance->count());
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->isSwim; }

    RideMetric *clone() const { return new DistanceSwim(*this); }
};

static bool addDistanceSwim()
{
    QVector<QString> deps;
    deps.append("total_distance");
    RideMetricFactory::instance().addMetric(DistanceSwim(), &deps);
    return true;
}
static bool distanceSwimAdded = addDistanceSwim();

// climb rating is essentially elev gain ^2 / distance
// a concept raised by Dan Conelly on his blog
class ClimbRating : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ClimbRating)
    double secsMoving;
    double km;

    public:

    ClimbRating() : secsMoving(0.0), km(0.0)
    {
        setSymbol("climb_rating");
        setInternalName("Climb Rating");
    }
    void initialize() {
        setName(tr("Climb Rating"));
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setType(RideMetric::Total);
        setPrecision(0);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        double rating = 0.0f;
        double distance = deps.value("total_distance")->value(true);

        if (ride->areDataPresent()->alt) {

            double ele = deps.value("elevation_gain")->value(true);

            // 100 is HARD !
            if (ele >0 && distance >0) {
                rating = (ele * ele) / distance;
                rating /= 1000.0f;
            }
        }

        setValue(rating);
        setCount(1);
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim; }
    RideMetric *clone() const { return new ClimbRating(*this); }
};

static bool climbRatingAdded =
    RideMetricFactory::instance().addMetric(
        ClimbRating(), &(QVector<QString>() << "total_distance" << "elevation_gain"));


class AthleteWeight : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AthleteWeight)
    double kg;

    public:

    AthleteWeight() : kg(0.0)
    {
        setSymbol("athlete_weight");
        setInternalName("Athlete Weight");
    }
    void initialize() {
        setName(tr("Athlete Weight"));
        setType(RideMetric::Average);
        setMetricUnits(tr("kg"));
        setImperialUnits(tr("lbs"));
        setPrecision(2);
        setConversion(LB_PER_KG);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *context) {

        // withings first
        double weight = context->athlete->getWithingsWeight(ride->startTime().date());

        // from metadata
        if (!weight) weight = ride->getTag("Weight", "0.0").toDouble();

        // global options
        if (!weight) weight = appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT, "75.0").toString().toDouble(); // default to 75kg

        // No weight default is weird, we'll set to 80kg
        if (weight <= 0.00) weight = 80.00;

        setValue(weight);
    }

    RideMetric *clone() const { return new AthleteWeight(*this); }
};

static bool athleteWeightAdded =
    RideMetricFactory::instance().addMetric(AthleteWeight());

//////////////////////////////////////////////////////////////////////////////

class AthleteFat : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AthleteFat)
    double kg;

    public:

    AthleteFat() : kg(0.0)
    {
        setSymbol("athlete_fat");
        setInternalName("Athlete Bodyfat");
    }
    void initialize() {
        setName(tr("Athlete Bodyfat"));
        setType(RideMetric::Average);
        setMetricUnits(tr("kg"));
        setImperialUnits(tr("lbs"));
        setPrecision(2);
        setConversion(LB_PER_KG);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *context) {

        WithingsReading here;

        // withings first
        context->athlete->getWithings(ride->startTime().date(), here);
        setValue(here.fatkg);
    }

    RideMetric *clone() const { return new AthleteFat(*this); }
};

static bool athleteFatAdded =
    RideMetricFactory::instance().addMetric(AthleteFat());

//////////////////////////////////////////////////////////////////////////////

class AthleteLean : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AthleteLean)
    double kg;

    public:

    AthleteLean() : kg(0.0)
    {
        setSymbol("athlete_lean");
        setInternalName("Athlete Lean Weight");
    }
    void initialize() {
        setName(tr("Athlete Lean Weight"));
        setType(RideMetric::Average);
        setMetricUnits(tr("kg"));
        setImperialUnits(tr("lbs"));
        setPrecision(2);
        setConversion(LB_PER_KG);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *context) {

        WithingsReading here;

        // withings first
        context->athlete->getWithings(ride->startTime().date(), here);
        setValue(here.leankg);
    }

    RideMetric *clone() const { return new AthleteLean(*this); }
};

static bool athleteLeanAdded =
    RideMetricFactory::instance().addMetric(AthleteLean());

//////////////////////////////////////////////////////////////////////////////

class AthleteFatP : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AthleteFatP)
    double kg;

    public:

    AthleteFatP() : kg(0.0)
    {
        setSymbol("athlete_fat_percent");
        setInternalName("Athlete Bodyfat Percent");
    }
    void initialize() {
        setName(tr("Athlete Bodyfat Percent"));
        setType(RideMetric::Average);
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setPrecision(1);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *context) {

        WithingsReading here;

        // withings first
        context->athlete->getWithings(ride->startTime().date(), here);
        setValue(here.fatpercent);
    }

    RideMetric *clone() const { return new AthleteFatP(*this); }
};

static bool athleteFatPAdded =
    RideMetricFactory::instance().addMetric(AthleteFatP());

//        case WITHINGS_LEANKG : return withings.leankg;
//////////////////////////////////////////////////////////////////////////////

class ElevationGain : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ElevationGain)
    double elegain;
    double prevalt;

    public:

    ElevationGain() : elegain(0.0), prevalt(0.0)
    {
        setSymbol("elevation_gain");
        setInternalName("Elevation Gain");
    }
    void initialize() {
        setName(tr("Elevation Gain"));
        setType(RideMetric::Total);
        setMetricUnits(tr("meters"));
        setImperialUnits(tr("feet"));
        setConversion(FEET_PER_METER);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        // hysteresis can be configured, we default to 3.0
        double hysteresis = appsettings->value(NULL, GC_ELEVATION_HYSTERESIS).toDouble();
        if (hysteresis <= 0.1) hysteresis = 3.00;

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
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim; }
    RideMetric *clone() const { return new ElevationGain(*this); }
};

static bool elevationGainAdded =
    RideMetricFactory::instance().addMetric(ElevationGain());

//////////////////////////////////////////////////////////////////////////////

class ElevationLoss : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ElevationLoss)
    double eleLoss;
    double prevalt;

    public:

    ElevationLoss() : eleLoss(0.0), prevalt(0.0)
    {
        setSymbol("elevation_loss");
        setInternalName("Elevation Loss");
    }
    void initialize() {
        setName(tr("Elevation Loss"));
        setType(RideMetric::Total);
        setMetricUnits(tr("meters"));
        setImperialUnits(tr("feet"));
        setConversion(FEET_PER_METER);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        // hysteresis can be configured, we default to 3.0
        double hysteresis = appsettings->value(NULL, GC_ELEVATION_HYSTERESIS).toDouble();
        if (hysteresis <= 0.1) hysteresis = 3.00;

        bool first = true;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (first) {
                first = false;
                prevalt = point->alt;
            }
            else if (point->alt < prevalt - hysteresis) {
                eleLoss += prevalt - point->alt;
                prevalt = point->alt;
            }
            else if (point->alt > prevalt + hysteresis) {
                prevalt = point->alt;
            }
        }
        setValue(eleLoss);
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim; }
    RideMetric *clone() const { return new ElevationLoss(*this); }
};

static bool elevationLossAdded =
    RideMetricFactory::instance().addMetric(ElevationLoss());

//////////////////////////////////////////////////////////////////////////////

class TotalWork : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TotalWork)
    double joules;

    public:

    TotalWork() : joules(0.0)
    {
        setSymbol("total_work");
        setInternalName("Work");
    }
    void initialize() {
        setName(tr("Work"));
        setMetricUnits(tr("kJ"));
        setImperialUnits(tr("kJ"));
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        joules = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->watts >= 0.0)
                joules += point->watts * ride->recIntSecs();
        }
        setValue(joules/1000);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
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
        setInternalName("Average Speed");
    }
    void initialize() {
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
                 const Context *) {
        assert(deps.contains("total_distance"));
        km = deps.value("total_distance")->value(true);

        if (ride->areDataPresent()->kph) {

            secsMoving = 0;
            bool withz = ride->isSwim(); // average with zeros for swims
            foreach (const RideFilePoint *point, ride->dataPoints())
                if (withz || point->kph > 0.0) secsMoving += ride->recIntSecs();

            setValue(secsMoving ? km / secsMoving * 3600.0 : 0.0);

        } else {

            // backup to duration if there is no speed channel
            assert(deps.contains("workout_time"));
            secsMoving = deps.value("workout_time")->value(true);
            setValue(secsMoving ? km / secsMoving * 3600.0 : 0.0);

        }
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
        AvgSpeed(), &(QVector<QString>() << "total_distance" << "workout_time"));

//////////////////////////////////////////////////////////////////////////////
class Pace : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(Pace)
    double pace;

    public:

    Pace() : pace(0.0)
    {
        setSymbol("pace");
        setInternalName("Pace");
    }
    // Pace ordering is reversed
    bool isLowerBetter() const { return true; }
    // Overrides to use Pace units setting
    QString units(bool) const {
        bool metricRunPace = appsettings->value(NULL, GC_PACE, true).toBool();
        return RideMetric::units(metricRunPace);
    }
    double value(bool) const {
        bool metricRunPace = appsettings->value(NULL, GC_PACE, true).toBool();
        return RideMetric::value(metricRunPace);
    }
    QString toString(bool metric) const {
        return time_to_string(value(metric)*60);
    }
    void initialize() {
        setName(tr("Pace"));
        setType(RideMetric::Average);
        setMetricUnits(tr("min/km"));
        setImperialUnits(tr("min/mile"));
        setPrecision(1);
        setConversion(KM_PER_MILE);
   }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        AvgSpeed *as = dynamic_cast<AvgSpeed*>(deps.value("average_speed"));

        // divide by zero or stupidly low pace
        if (as->value(true) > 0.00f) pace = 60.0f / as->value(true);
        else pace = 0;

        setValue(pace);
        setCount(as->count());
    }

    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim; }

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
class PaceSwim : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(PaceSwim)
    double pace;

    public:

    PaceSwim() : pace(0.0)
    {
        setSymbol("pace_swim");
        setInternalName("Pace Swim");
    }
    // Swim Pace ordering is reversed
    bool isLowerBetter() const { return true; }
    // Overrides to use Swim Pace units setting
    QString units(bool) const {
        bool metricRunPace = appsettings->value(NULL, GC_SWIMPACE, true).toBool();
        return RideMetric::units(metricRunPace);
    }
    double value(bool) const {
        bool metricRunPace = appsettings->value(NULL, GC_SWIMPACE, true).toBool();
        return RideMetric::value(metricRunPace);
    }
    QString toString(bool metric) const {
        return time_to_string(value(metric)*60);
    }
    void initialize() {
        setName(tr("Pace Swim"));
        setType(RideMetric::Average);
        setMetricUnits(tr("min/100m"));
        setImperialUnits(tr("min/100yd"));
        setPrecision(1);
        setConversion(METERS_PER_YARD);
   }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        AvgSpeed *as = dynamic_cast<AvgSpeed*>(deps.value("average_speed"));

        // divide by zero or stupidly low pace
        if (as->value(true) > 0.00f) pace = 6.0f / as->value(true);
        else pace = 0;

        setValue(pace);
        setCount(as->count());
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->isSwim; }

    RideMetric *clone() const { return new PaceSwim(*this); }
};

static bool addPaceSwim()
{
    QVector<QString> deps;
    deps.append("average_speed");
    RideMetricFactory::instance().addMetric(PaceSwim(), &deps);
    return true;
}
static bool paceSwimAdded = addPaceSwim();

//////////////////////////////////////////////////////////////////////////////

struct AvgPower : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgPower)

    double count, total;

    public:

    AvgPower()
    {
        setSymbol("average_power");
        setInternalName("Average Power");
    }
    void initialize() {
        setName(tr("Average Power"));
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
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
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new AvgPower(*this); }
};

static bool avgPowerAdded =
    RideMetricFactory::instance().addMetric(AvgPower());

//////////////////////////////////////////////////////////////////////////////

struct AvgSmO2 : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgSmO2)

    double count, total;

    public:

    AvgSmO2()
    {
        setSymbol("average_smo2");
        setInternalName("Average SmO2");
    }
    void initialize() {
        setName(tr("Average SmO2"));
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
        total = count = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->smo2 >= 0.0) {
                total += point->smo2;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("O"); }

    RideMetric *clone() const { return new AvgSmO2(*this); }
};

static bool avgSmO2Added =
    RideMetricFactory::instance().addMetric(AvgSmO2());

struct AvgtHb : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgtHb)

    double count, total;

    public:

    AvgtHb()
    {
        setSymbol("average_tHb");
        setInternalName("Average tHb");
    }
    void initialize() {
        setName(tr("Average tHb"));
        setMetricUnits(tr("g/dL"));
        setImperialUnits(tr("g/dL"));
        setType(RideMetric::Average);
        setPrecision(2);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
        total = count = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->thb >= 0.0) {
                total += point->thb;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("O"); }

    RideMetric *clone() const { return new AvgtHb(*this); }
};

static bool avgtHbAdded =
    RideMetricFactory::instance().addMetric(AvgtHb());

//////////////////////////////////////////////////////////////////////////////

struct AAvgPower : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AAvgPower)

    double count, total;

    public:

    AAvgPower()
    {
        setSymbol("average_apower");
        setInternalName("Average aPower");
    }
    void initialize() {
        setName(tr("Average aPower"));
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
        total = count = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->apower >= 0.0) {
                total += point->apower;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new AAvgPower(*this); }
};

static bool aavgPowerAdded =
    RideMetricFactory::instance().addMetric(AAvgPower());

//////////////////////////////////////////////////////////////////////////////

struct NonZeroPower : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(NonZeroPower)

    double count, total;

    public:

    NonZeroPower()
    {
        setSymbol("nonzero_power");
        setInternalName("Nonzero Average Power");
    }
    void initialize() {
        setName(tr("Nonzero Average Power"));
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
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
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new NonZeroPower(*this); }
};

static bool nonZeroPowerAdded =
    RideMetricFactory::instance().addMetric(NonZeroPower());

//////////////////////////////////////////////////////////////////////////////

struct AvgHeartRate : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgHeartRate)

    double total, count;

    public:

    AvgHeartRate()
    {
        setSymbol("average_hr");
        setInternalName("Average Heart Rate");
    }
    void initialize() {
        setName(tr("Average Heart Rate"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
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

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    RideMetric *clone() const { return new AvgHeartRate(*this); }
};

static bool avgHeartRateAdded =
    RideMetricFactory::instance().addMetric(AvgHeartRate());

struct AvgCoreTemp : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgCoreTemp)

    double total, count;

    public:

    AvgCoreTemp()
    {
        setSymbol("average_ct");
        setInternalName("Average Core Temperature");
        setPrecision(1);
    }
    void initialize() {
        setName(tr("Average Core Temperature"));
        setMetricUnits(tr("C"));
        setImperialUnits(tr("C"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
        total = count = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->tcore > 0) {
                total += point->tcore;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    RideMetric *clone() const { return new AvgCoreTemp(*this); }
};

static bool avgCTAdded =
    RideMetricFactory::instance().addMetric(AvgCoreTemp());

///////////////////////////////////////////////////////////////////////////////

struct HeartBeats : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(HeartBeats)

    double total;

    public:

    HeartBeats()
    {
        setSymbol("heartbeats");
        setInternalName("Heartbeats");
    }
    void initialize() {
        setName(tr("Heartbeats"));
        setMetricUnits(tr("beats"));
        setImperialUnits(tr("beats"));
        setType(RideMetric::Total);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
        total = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            total += (point->hr / 60) * ride->recIntSecs();
        }
        setValue(total);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    RideMetric *clone() const { return new HeartBeats(*this); }
};

static bool hbAdded =
    RideMetricFactory::instance().addMetric(HeartBeats());

////////////////////////////////////////////////////////////////////////////////

class HrPw : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(HrPw)

    public:
    HrPw()
    {
        setSymbol("hrpw");
        setInternalName("HrPw Ratio");
    }
    void initialize() {
        setName(tr("HrPw Ratio"));
        setImperialUnits("");
        setMetricUnits("");
        setPrecision(3);
        setType(RideMetric::Average);
    }
    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        AvgHeartRate *hr = dynamic_cast<AvgHeartRate*>(deps.value("average_hr"));
        AvgPower *pw = dynamic_cast<AvgPower*>(deps.value("average_power"));

        if (hr->value(true) > 100 && pw->value(true) > 100) { // ignore silly rides with low values
            setValue(pw->value(true) / hr->value(true));
        } else {
            setValue(0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new HrPw(*this); }
};

static bool addHrPw()
{
    QVector<QString> deps;
    deps.append("average_power");
    deps.append("average_hr");
    RideMetricFactory::instance().addMetric(HrPw(), &deps);
    return true;
}

static bool hrpwAdded = addHrPw();

////////////////////////////////////////////////////////////////////////////////

class Workbeat : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(Workbeats)

    public:
    Workbeat()
    {
        setSymbol("wb");
        setInternalName("Workbeat stress");
    }
    void initialize() {
        setName(tr("Workbeat stress"));
        setImperialUnits("");
        setMetricUnits("");
        setPrecision(0);
        setType(RideMetric::Total);
    }
    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        TotalWork *work = dynamic_cast<TotalWork*>(deps.value("total_work"));
        HeartBeats *hb = dynamic_cast<HeartBeats*>(deps.value("heartbeats"));

        setValue((work->value() * hb->value()) / 100000.00f);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new Workbeat(*this); }
};

static bool addWorkbeat()
{
    QVector<QString> deps;
    deps.append("total_work");
    deps.append("heartbeats");
    RideMetricFactory::instance().addMetric(Workbeat(), &deps);
    return true;
}

static bool workbeatAdded = addWorkbeat();

//////////////////////////////////////////////////////////////////////

class WattsRPE : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(WattsRPE)

    public:
    WattsRPE()
    {
        setSymbol("wattsRPE");
        setInternalName("Watts:RPE Ratio");
    }
    void initialize() {
        setName(tr("Watts:RPE Ratio"));
        setImperialUnits("");
        setMetricUnits("");
        setPrecision(3);
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        double ratio = 0.0f;
        AvgPower *pw = dynamic_cast<AvgPower*>(deps.value("average_power"));
        double rpe = ride->getTag("RPE", "0").toDouble();

        if (pw->value(true) > 100 && rpe > 0) { // ignore silly rides with low values
            ratio = pw->value(true) / rpe;
        }
        setValue(ratio);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new WattsRPE(*this); }
};

static bool addWattsRPE()
{
    QVector<QString> deps;
    deps.append("average_power");
    RideMetricFactory::instance().addMetric(WattsRPE(), &deps);
    return true;
}

static bool wattsRPEAdded = addWattsRPE();

///////////////////////////////////////////////////////////////////////////////

class APPercent : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(APPercent)

    public:
    APPercent()
    {
        setSymbol("ap_percent_max");
        setInternalName("Power Percent of Max");
    }
    void initialize() {
        setName(tr("Power Percent of Max"));
        setImperialUnits("");
        setMetricUnits("");
        setPrecision(0);
        setType(RideMetric::Average);
    }
    void compute(const RideFile *, const Zones *zones, int zoneRange,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        double percent = 0.0f;
        AvgPower *pw = dynamic_cast<AvgPower*>(deps.value("average_power"));

        if (pw->value(true) > 0.0f && zones && zoneRange >= 0) {
            // get Pmax
            double pmax = zones->getPmax(zoneRange);
            percent = pw->value(true)/pmax * 100;
        }
        setValue(percent);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new APPercent(*this); }
};

static bool addAPPercent()
{
    QVector<QString> deps;
    deps.append("average_power");
    RideMetricFactory::instance().addMetric(APPercent(), &deps);
    return true;
}

static bool APPercentAdded = addAPPercent();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class HrNp : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(HrNp)

    public:
    HrNp()
    {
        setSymbol("hrnp");
        setInternalName("HrNp Ratio");
    }
    void initialize() {
        setName(tr("HrNp Ratio"));
        setImperialUnits("");
        setMetricUnits("");
        setPrecision(3);
        setType(RideMetric::Average);
    }
    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        AvgHeartRate *hr = dynamic_cast<AvgHeartRate*>(deps.value("average_hr"));
        RideMetric *pw = dynamic_cast<RideMetric*>(deps.value("coggan_np"));

        if (hr->value(true) > 100 && pw->value(true) > 100) { // ignore silly rides with low values
            setValue(pw->value(true) / hr->value(true));
        } else {
            setValue(0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new HrNp(*this); }
};

static bool addHrNp()
{
    QVector<QString> deps;
    deps.append("coggan_np");
    deps.append("average_hr");
    RideMetricFactory::instance().addMetric(HrNp(), &deps);
    return true;
}

static bool hrnpAdded = addHrNp();

///////////////////////////////////////////////////////////////////////////////

struct AvgCadence : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgCadence)

    double total, count;

    public:

    AvgCadence()
    {
        setSymbol("average_cad");
        setInternalName("Average Cadence");
    }
    void initialize() {
        setName(tr("Average Cadence"));
        setMetricUnits(tr("rpm"));
        setImperialUnits(tr("rpm"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
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

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("C"); }

    RideMetric *clone() const { return new AvgCadence(*this); }
};

static bool avgCadenceAdded =
    RideMetricFactory::instance().addMetric(AvgCadence());

//////////////////////////////////////////////////////////////////////////////

struct AvgTemp : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgTemp)

    double total, count;

    public:

    AvgTemp()
    {
        setSymbol("average_temp");
        setInternalName("Average Temp");
    }

    // we DO aggregate zero, its -255 we ignore !
    bool aggregateZero() const { return true; }

    // override to special case NoTemp
    QString toString(bool useMetricUnits) const {
        if (value() == RideFile::NoTemp) return "-";
        return RideMetric::toString(useMetricUnits);
    }

    void initialize() {
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
                 const Context *) {

        if (ride->areDataPresent()->temp) {
            total = count = 0;
            foreach (const RideFilePoint *point, ride->dataPoints()) {
                if (point->temp != RideFile::NoTemp) {
                    total += point->temp;
                    ++count;
                }
            }
            setValue(count > 0 ? total / count : count);
            setCount(count);
        } else {
            setValue(RideFile::NoTemp);
            setCount(1);
        }
    }
    RideMetric *clone() const { return new AvgTemp(*this); }
};

static bool avgTempAdded =
    RideMetricFactory::instance().addMetric(AvgTemp());

//////////////////////////////////////////////////////////////////////////////

class MaxPower : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MaxPower)
    double max;
    public:
    MaxPower() : max(0.0)
    {
        setSymbol("max_power");
        setInternalName("Max Power");
    }
    void initialize() {
        setName(tr("Max Power"));
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setType(RideMetric::Peak);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->watts >= max)
                max = point->watts;
        }
        setValue(max);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new MaxPower(*this); }
};

static bool maxPowerAdded =
    RideMetricFactory::instance().addMetric(MaxPower());

//////////////////////////////////////////////////////////////////////////////

class MaxSmO2 : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MaxSmO2)
    double max;
    public:
    MaxSmO2() : max(0.0)
    {
        setSymbol("max_smo2");
        setInternalName("Max SmO2");
    }
    void initialize() {
        setName(tr("Max SmO2"));
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setType(RideMetric::Peak);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->smo2 >= max)
                max = point->smo2;
        }
        setValue(max);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("O"); }

    RideMetric *clone() const { return new MaxSmO2(*this); }
};

static bool maxSmO2Added =
    RideMetricFactory::instance().addMetric(MaxSmO2());

class MaxtHb : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MaxtHb)
    double max;
    public:
    MaxtHb() : max(0.0)
    {
        setSymbol("max_tHb");
        setInternalName("Max tHb");
    }
    void initialize() {
        setName(tr("Max tHb"));
        setMetricUnits(tr("g/dL"));
        setImperialUnits(tr("g/dL"));
        setType(RideMetric::Peak);
        setPrecision(2);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->thb >= max)
                max = point->thb;
        }
        setValue(max);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("O"); }

    RideMetric *clone() const { return new MaxtHb(*this); }
};

static bool maxtHbAdded =
    RideMetricFactory::instance().addMetric(MaxtHb());

//////////////////////////////////////////////////////////////////////////////

class MinSmO2 : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MinSmO2)
    double min;
    public:
    MinSmO2() : min(0.0)
    {
        setSymbol("min_smo2");
        setInternalName("Min SmO2");
    }
    void initialize() {
        setName(tr("Min SmO2"));
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setType(RideMetric::Peak);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->smo2 > 0 && point->smo2 >= min)
                min = point->smo2;
        }
        setValue(min);
    }
    RideMetric *clone() const { return new MinSmO2(*this); }
};

static bool minSmO2Added =
    RideMetricFactory::instance().addMetric(MinSmO2());

class MintHb : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MintHb)
    double min;
    public:
    MintHb() : min(0.0)
    {
        setSymbol("min_tHb");
        setInternalName("Min tHb");
    }
    void initialize() {
        setName(tr("Min tHb"));
        setMetricUnits(tr("g/dL"));
        setImperialUnits(tr("g/dL"));
        setType(RideMetric::Low);
        setPrecision(2);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->thb > 0 && point->thb >= min)
                min = point->thb;
        }
        setValue(min);
    }
    RideMetric *clone() const { return new MintHb(*this); }
};

static bool mintHb =
    RideMetricFactory::instance().addMetric(MintHb());

//////////////////////////////////////////////////////////////////////////////

class MaxHr : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MaxHr)
    double max;
    public:
    MaxHr() : max(0.0)
    {
        setSymbol("max_heartrate");
        setInternalName("Max Heartrate");
    }
    void initialize() {
        setName(tr("Max Heartrate"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
        setType(RideMetric::Peak);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->hr >= max)
                max = point->hr;
        }
        setValue(max);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    RideMetric *clone() const { return new MaxHr(*this); }
};

static bool maxHrAdded =
    RideMetricFactory::instance().addMetric(MaxHr());

//////////////////////////////////////////////////////////////////////////////

class MinHr : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MinHr)
    double min;
    public:
    MinHr() : min(0.0)
    {
        setSymbol("min_heartrate");
        setInternalName("Min Heartrate");
    }
    void initialize() {
        setName(tr("Min Heartrate"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
        setType(RideMetric::Peak);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
        bool notset = true;
        min = 0;
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->hr > 0 && (notset || point->hr < min)) {
                min = point->hr;
                notset = false;
            }
        }
        setValue(min);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    RideMetric *clone() const { return new MinHr(*this); }
};

static bool minHrAdded =
    RideMetricFactory::instance().addMetric(MinHr());

//////////////////////////////////////////////////////////////////////////////

class MaxCT : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MaxCT)
    double max;
    public:
    MaxCT() : max(0.0)
    {
        setSymbol("max_ct");
        setInternalName("Max Core Temperature");
        setPrecision(1);
    }
    void initialize() {
        setName(tr("Max Core Temperature"));
        setMetricUnits(tr("C"));
        setImperialUnits(tr("C"));
        setType(RideMetric::Peak);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
        foreach (const RideFilePoint *point, ride->dataPoints()) {
            if (point->tcore >= max)
                max = point->tcore;
        }
        setValue(max);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    RideMetric *clone() const { return new MaxCT(*this); }
};

static bool maxCTAdded =
    RideMetricFactory::instance().addMetric(MaxCT());

//////////////////////////////////////////////////////////////////////////////

class MaxSpeed : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MaxSpeed)
    public:

    MaxSpeed()
    {
        setSymbol("max_speed");
        setInternalName("Max Speed");
    }
    void initialize() {
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
                 const Context *) {
        double max = 0.0;

        if (ride->areDataPresent()->kph) {
            foreach (const RideFilePoint *point, ride->dataPoints())
                if (point->kph > max) max = point->kph;
        }

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
    Q_DECLARE_TR_FUNCTIONS(MaxCadence)
    public:

    MaxCadence()
    {
        setSymbol("max_cadence");
        setInternalName("Max Cadence");
    }
    void initialize() {
        setName(tr("Max Cadence"));
        setMetricUnits(tr("rpm"));
        setImperialUnits(tr("rpm"));
        setType(RideMetric::Peak);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
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

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("C"); }

    RideMetric *clone() const { return new MaxCadence(*this); }
};

static bool maxCadenceAdded =
    RideMetricFactory::instance().addMetric(MaxCadence());

//////////////////////////////////////////////////////////////////////////////

class MaxTemp : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MaxTemp)
    public:

    MaxTemp()
    {
        setSymbol("max_temp");
        setInternalName("Max Temp");
    }
    void initialize() {
        setName(tr("Max Temp"));
        setMetricUnits(tr("C"));
        setImperialUnits(tr("F"));
        setType(RideMetric::Peak);
        setPrecision(1);
        setConversion(FAHRENHEIT_PER_CENTIGRADE);
        setConversionSum(FAHRENHEIT_ADD_CENTIGRADE);
    }

    // override to special case NoTemp
    QString toString(bool useMetricUnits) const {
        if (value() == RideFile::NoTemp) return "-";
        return RideMetric::toString(useMetricUnits);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->temp) {
            double max = 0.0;
            foreach (const RideFilePoint *point, ride->dataPoints())
                if (point->temp != RideFile::NoTemp && point->temp > max) max = point->temp;

            setValue(max);
        } else {
            setValue(RideFile::NoTemp);
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
    Q_DECLARE_TR_FUNCTIONS(NinetyFivePercentHeartRate)
    double hr;
    public:
    NinetyFivePercentHeartRate() : hr(0.0)
    {
        setSymbol("ninety_five_percent_hr");
        setInternalName("95% Heartrate");
    }
    void initialize() {
        setName(tr("95% Heartrate"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {
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
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }
    RideMetric *clone() const { return new NinetyFivePercentHeartRate(*this); }
};

static bool ninetyFivePercentHeartRateAdded =
    RideMetricFactory::instance().addMetric(NinetyFivePercentHeartRate());

///////////////////////////////////////////////////////////////////////////////

class VAM : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(VAM)

    public:
    VAM()
    {
        setSymbol("vam");
        setInternalName("VAM");
    }
    void initialize() {
        setName(tr("VAM"));
        setImperialUnits("");
        setMetricUnits("");
        setType(RideMetric::Average);
    }
    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        ElevationGain *el = dynamic_cast<ElevationGain*>(deps.value("elevation_gain"));
        WorkoutTime *wt = dynamic_cast<WorkoutTime*>(deps.value("workout_time"));
        setValue((el->value(true)*3600)/wt->value(true));
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim; }
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

class EOA : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(EOA)

    public:
    EOA()
    {
        setSymbol("eoa");
        setInternalName("EOA");
    }
    void initialize() {
        setName(tr("Effect of Altitude"));
        setImperialUnits("%");
        setMetricUnits("%");
        setType(RideMetric::Average);
    }
    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        AAvgPower *aap = dynamic_cast<AAvgPower*>(deps.value("average_apower"));
        AvgPower *ap = dynamic_cast<AvgPower*>(deps.value("average_power"));
        setValue(((aap->value(true)-ap->value(true))/aap->value(true)) * 100.00f);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new EOA(*this); }
};

static bool addEOA()
{
    QVector<QString> deps;
    deps.append("average_apower");
    deps.append("average_power");
    RideMetricFactory::instance().addMetric(EOA(), &deps);
    return true;
}

static bool eoaAdded = addEOA();

///////////////////////////////////////////////////////////////////////////////

class Gradient : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(Gradient)

    public:
    Gradient()
    {
        setSymbol("gradient");
        setInternalName("Gradient");
    }
    void initialize() {
        setName(tr("Gradient"));
        setImperialUnits("%");
        setMetricUnits("%");
        setPrecision(1);
        setType(RideMetric::Average);
    }
    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        ElevationGain *el = dynamic_cast<ElevationGain*>(deps.value("elevation_gain"));
        TotalDistance *td = dynamic_cast<TotalDistance*>(deps.value("total_distance"));
        if (td->value(true) && el->value(true)) {
            setValue(100.00 *(el->value(true) / (1000 *td->value(true))));
        } else
            setValue(0.0);
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim; }
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
    Q_DECLARE_TR_FUNCTIONS(MeanPowerVariance)

    public:
    double topRank;

    MeanPowerVariance()
    {
        setSymbol("meanpowervariance");
        setInternalName("Average Power Variance");
    }
    void initialize() {
        setName(tr("Average Power Variance"));
        setImperialUnits("watts change");
        setMetricUnits("watts change");
        setPrecision(2);
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        // Less than 30s don't bother
        if (ride->dataPoints().count() < 30) {
            setValue(0);
            topRank=0.00;
        } else {

            QVector<double> power;
            QVector<double> secs;
            foreach (RideFilePoint *point, ride->dataPoints()) {
                power.append(point->watts);
                secs.append(point->secs);
            }

            LTMOutliers outliers(secs.data(), power.data(), power.count(), 30, false);
            setValue(outliers.getStdDeviation());
            topRank = outliers.getYForRank(0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
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
    Q_DECLARE_TR_FUNCTIONS(MaxPowerVariance)

    public:
    MaxPowerVariance()
    {
        setSymbol("maxpowervariance");
        setInternalName("Max Power Variance");
    }
    void initialize() {
        setName(tr("Max Power Variance"));
        setImperialUnits("watts change");
        setMetricUnits("watts change");
        setPrecision(2);
        setType(RideMetric::Average);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        MeanPowerVariance *mean = dynamic_cast<MeanPowerVariance*>(deps.value("meanpowervariance"));
        if (ride->dataPoints().count() < 30)
            setValue(0);
        else
            setValue(mean->topRank);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
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

class AvgLTE : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgLTE)

    public:

    AvgLTE()
    {
        setSymbol("average_lte");
        setInternalName("Average Left Torque Effectiveness");
    }
    void initialize() {
        setName(tr("Average Left Torque Effectiveness"));
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setType(RideMetric::Average);
        setPrecision(1);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->lte) {

            double total = 0.0f;
            double samples = 0.0f;

            foreach (const RideFilePoint *point, ride->dataPoints()) {

                if (point->lte) {
                    samples ++;
                    total += point->lte;
                }
            }

            if (total > 0.0f && samples > 0.0f) setValue(total / samples);
            else setValue(0.0);

        } else {

            setValue(0.0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgLTE(*this); }
};

//////////////////////////////////////////////////////////////////////////////

class AvgRTE : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgRTE)

    public:

    AvgRTE()
    {
        setSymbol("average_rte");
        setInternalName("Average Right Torque Effectiveness");
    }
    void initialize() {
        setName(tr("Average Right Torque Effectiveness"));
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setType(RideMetric::Average);
        setPrecision(1);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->rte) {

            double total = 0.0f;
            double samples = 0.0f;

            foreach (const RideFilePoint *point, ride->dataPoints()) {
                if (point->rte) {
                    samples ++;
                    total += point->rte;
                }
            }

            if (total > 0.0f && samples > 0.0f) setValue(total / samples);
            else setValue(0.0);

        } else {

            setValue(0.0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgRTE(*this); }
};

//////////////////////////////////////////////////////////////////////////////

class AvgLPS : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgLPS)

    public:

    AvgLPS()
    {
        setSymbol("average_lps");
        setInternalName("Average Left Pedal Smoothness");
    }
    void initialize() {
        setName(tr("Average Left Pedal Smoothness"));
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setType(RideMetric::Average);
        setPrecision(1);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->lps) {

            double total = 0.0f;
            double samples = 0.0f;

            foreach (const RideFilePoint *point, ride->dataPoints()) {

                if (point->lps) {
                    samples ++;
                    total += point->lps;
                }
            }

            if (total > 0.0f && samples > 0.0f) setValue(total / samples);
            else setValue(0.0);

        } else {

            setValue(0.0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgLPS(*this); }
};

//////////////////////////////////////////////////////////////////////////////

class AvgRPS : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgRPS)

    public:

    AvgRPS()
    {
        setSymbol("average_rps");
        setInternalName("Average Right Pedal Smoothness");
    }
    void initialize() {
        setName(tr("Average Right Pedal Smoothness"));
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setType(RideMetric::Average);
        setPrecision(1);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->rps) {

            double total = 0.0f;
            double samples = 0.0f;

            foreach (const RideFilePoint *point, ride->dataPoints()) {

                if (point->rps) {
                    samples ++;
                    total += point->rps;
                }
            }

            if (total > 0.0f && samples > 0.0f) setValue(total / samples);
            else setValue(0.0);

        } else {

            setValue(0.0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgRPS(*this); }
};

//////////////////////////////////////////////////////////////////////////////

class AvgLPCO : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgLPCO)

    public:

    AvgLPCO()
    {
        setSymbol("average_lpco");
        setInternalName("Average Left Pedal Center Offset");
    }
    void initialize() {
        setName(tr("Average Left Pedal Center Offset"));
        setMetricUnits(tr("mm"));
        setImperialUnits(tr("in")); // inches would need more precision than 1
        setType(RideMetric::Average);
        setPrecision(2);
        setConversion(INCH_PER_MM);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->lpco) {

            double total = 0.0f;
            double secs = 0.0f;

            foreach (const RideFilePoint *point, ride->dataPoints()) {

                if (point->cad) {
                    secs += ride->recIntSecs();
                    total += point->lpco;
                }
            }

            if (secs > 0.0f) setValue(total / secs);
            else setValue(0.0);

        } else {

            setValue(0.0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgLPCO(*this); }
};

//////////////////////////////////////////////////////////////////////////////

class AvgRPCO : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgRPCO)

    public:

    AvgRPCO()
    {
        setSymbol("average_rpco");
        setInternalName("Average Right Pedal Center Offset");
    }
    void initialize() {
        setName(tr("Average Right Pedal Center Offset"));
        setMetricUnits(tr("mm"));
        setImperialUnits(tr("in")); // inches would need more precision than 1
        setType(RideMetric::Average);
        setPrecision(2);
        setConversion(INCH_PER_MM);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->rpco) {

            double total = 0.0f;
            double secs = 0.0f;

            foreach (const RideFilePoint *point, ride->dataPoints()) {

                if (point->cad) {
                    secs += ride->recIntSecs();
                    total += point->rpco;
                }
            }

            if (secs > 0.0f) setValue(total / secs);
            else setValue(0.0);

        } else {

            setValue(0.0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgRPCO(*this); }
};

//////////////////////////////////////////////////////////////////////////////

class AvgLPPB : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgLPPB)

    public:

    AvgLPPB()
    {
        setSymbol("average_lppb");
        setInternalName("Average Left Power Phase Start");
    }
    void initialize() {
        setName(tr("Average Left Power Phase Start"));
        setMetricUnits(tr("°"));
        setType(RideMetric::Average);
        setPrecision(0);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->lppb) {

            double total = 0.0f;
            double secs = 0.0f;

            foreach (const RideFilePoint *point, ride->dataPoints()) {

                if (point->lppe>0) { // use for average if we have an end
                    secs += ride->recIntSecs();
                    total += point->lppb + (point->lppb>180?-360:0);
                }
            }

            if (secs > 0.0f) setValue(total / secs);
            else setValue(0.0);

        } else {

            setValue(0.0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgLPPB(*this); }
};

//////////////////////////////////////////////////////////////////////////////

class AvgRPPB : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgRTPP)

    public:

    AvgRPPB()
    {
        setSymbol("average_rppb");
        setInternalName("Average Right Power Phase Start");
    }
    void initialize() {
        setName(tr("Average Right Power Phase Start"));
        setMetricUnits(tr("°"));
        setType(RideMetric::Average);
        setPrecision(0);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->rppb) {

            double total = 0.0f;
            double secs = 0.0f;

            foreach (const RideFilePoint *point, ride->dataPoints()) {

                if (point->rppe>0) { // use for average if we have an end
                    secs += ride->recIntSecs();
                    total += point->rppb + (point->rppb>180?-360:0);
                }
            }

            if (secs > 0.0f) setValue(total / secs);
            else setValue(0.0);

        } else {

            setValue(0.0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgRPPB(*this); }
};


//////////////////////////////////////////////////////////////////////////////

class AvgLPPE : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgLPPE)

    public:

    AvgLPPE()
    {
        setSymbol("average_lppe");
        setInternalName("Average Left Power Phase End");
    }
    void initialize() {
        setName(tr("Average Left Power Phase End"));
        setMetricUnits(tr("°"));
        setType(RideMetric::Average);
        setPrecision(0);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->lppe) { // end has to be > 0

            double total = 0.0f;
            double secs = 0.0f;

            foreach (const RideFilePoint *point, ride->dataPoints()) {

                if (point->lppe > 0) {
                    secs += ride->recIntSecs();
                    total += point->lppe;
                }
            }

            if (secs > 0.0f) setValue(total / secs);
            else setValue(0.0);

        } else {

            setValue(0.0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgLPPE(*this); }
};

//////////////////////////////////////////////////////////////////////////////

class AvgRPPE : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgRPPE)

    public:

    AvgRPPE()
    {
        setSymbol("average_rppe");
        setInternalName("Average Right Power Phase End");
    }
    void initialize() {
        setName(tr("Average Right Power Phase End"));
        setMetricUnits(tr("°"));
        setType(RideMetric::Average);
        setPrecision(0);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->rppe) {

            double total = 0.0f;
            double secs = 0.0f;

            foreach (const RideFilePoint *point, ride->dataPoints()) {

                if (point->rppe > 0) { // end has to be > 0
                    secs += ride->recIntSecs();
                    total += point->rppe;
                }
            }

            if (secs > 0.0f) setValue(total / secs);
            else setValue(0.0);

        } else {

            setValue(0.0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgRPPE(*this); }
};


//////////////////////////////////////////////////////////////////////////////

class AvgLPPPB : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgLPPPB)

    public:

    AvgLPPPB()
    {
        setSymbol("average_lpppb");
        setInternalName("Average Left Peak Power Phase Start");
    }
    void initialize() {
        setName(tr("Average Left Peak Power Phase Start"));
        setMetricUnits(tr("°"));
        setType(RideMetric::Average);
        setPrecision(0);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->lpppb) {

            double total = 0.0f;
            double secs = 0.0f;

            foreach (const RideFilePoint *point, ride->dataPoints()) {

                if (point->lpppe>0) { // use for average if we have an end
                    secs += ride->recIntSecs();
                    total += point->lpppb + (point->lpppb>180?-360:0);
                }
            }

            if (secs > 0.0f) setValue(total / secs);
            else setValue(0.0);

        } else {

            setValue(0.0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgLPPPB(*this); }
};

//////////////////////////////////////////////////////////////////////////////

class AvgRPPPB : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgRPPPB)

    public:

    AvgRPPPB()
    {
        setSymbol("average_rpppb");
        setInternalName("Average Right Peak Power Phase Start");
    }
    void initialize() {
        setName(tr("Average Right Peak Power Phase Start"));
        setMetricUnits(tr("°"));
        setType(RideMetric::Average);
        setPrecision(0);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->rpppb) {

            double total = 0.0f;
            double secs = 0.0f;

            foreach (const RideFilePoint *point, ride->dataPoints()) {

                if (point->rpppe>0) { // use for average if we have an end
                    secs += ride->recIntSecs();
                    total += point->rpppb + (point->rpppb>180?-360:0);
                }
            }

            if (secs > 0.0f) setValue(total / secs);
            else setValue(0.0);

        } else {

            setValue(0.0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgRPPPB(*this); }
};


//////////////////////////////////////////////////////////////////////////////

class AvgLPPPE : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgLPPPE)

    public:

    AvgLPPPE()
    {
        setSymbol("average_lpppe");
        setInternalName("Average Left Peak Power Phase End");
    }
    void initialize() {
        setName(tr("Average Left Peak Power Phase End"));
        setMetricUnits(tr("°"));
        setType(RideMetric::Average);
        setPrecision(0);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->lpppe) {

            double total = 0.0f;
            double secs = 0.0f;

            foreach (const RideFilePoint *point, ride->dataPoints()) {

                if (point->lpppe > 0) { // end has to be > 0
                    secs += ride->recIntSecs();
                    total += point->lpppe;
                }
            }

            if (secs > 0.0f) setValue(total / secs);
            else setValue(0.0);

        } else {

            setValue(0.0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgLPPPE(*this); }
};

//////////////////////////////////////////////////////////////////////////////

class AvgRPPPE : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgRPPPE)

    public:

    AvgRPPPE()
    {
        setSymbol("average_rpppe");
        setInternalName("Average Right Peak Power Phase End");
    }
    void initialize() {
        setName(tr("Average Right Peak Power Phase End"));
        setMetricUnits(tr("°"));
        setType(RideMetric::Average);
        setPrecision(0);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if (ride->areDataPresent()->rpppe) {

            double total = 0.0f;
            double secs = 0.0f;

            foreach (const RideFilePoint *point, ride->dataPoints()) {

                if (point->rpppe > 0) { // end has to be > 0
                    secs += ride->recIntSecs();
                    total += point->rpppe;
                }
            }

            if (secs > 0.0f) setValue(total / secs);
            else setValue(0.0);

        } else {

            setValue(0.0);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgRPPPE(*this); }
};

//////////////////////////////////////////////////////////////////////////////

class AvgLPP : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgLPP)
    double average_lppb;
    double average_lppe;

    public:

    AvgLPP()
    {
        setSymbol("average_lpp");
        setInternalName("Average Left Power Phase Length");
    }
    void initialize() {
        setName(tr("Average Left Power Phase Length"));
        setMetricUnits(tr("°"));
        setType(RideMetric::Average);
        setPrecision(0);
    }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        average_lppb = deps.value("average_lppb")->value(true);
        average_lppe = deps.value("average_lppe")->value(true);

        if (average_lppe>0)  {

            setValue(average_lppe-average_lppb);
        } else {

            setValue(0.0);
        }

    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgLPP(*this); }
};


//////////////////////////////////////////////////////////////////////////////

class AvgRPP : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgRPP)
    double average_rppb;
    double average_rppe;

    public:

    AvgRPP()
    {
        setSymbol("average_rpp");
        setInternalName("Average Right Power Phase Length");
    }
    void initialize() {
        setName(tr("Average Right Power Phase Length"));
        setMetricUnits(tr("°"));
        setType(RideMetric::Average);
        setPrecision(0);
    }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        average_rppb = deps.value("average_rppb")->value(true);
        average_rppe = deps.value("average_rppe")->value(true);

        if (average_rppe>0)  {

            setValue(average_rppe-average_rppb);
        } else {

            setValue(0.0);
        }

    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgRPP(*this); }
};

//////////////////////////////////////////////////////////////////////////////

class AvgLPPP : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgLPPP)
    double average_lpppb;
    double average_lpppe;

    public:

    AvgLPPP()
    {
        setSymbol("average_lppp");
        setInternalName("Average Peak Left Power Phase Length");
    }
    void initialize() {
        setName(tr("Average Left Peak Power Phase Length"));
        setMetricUnits(tr("°"));
        setType(RideMetric::Average);
        setPrecision(0);
    }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        average_lpppb = deps.value("average_lpppb")->value(true);
        average_lpppe = deps.value("average_lpppe")->value(true);

        if (average_lpppe>0)  {

            setValue(average_lpppe-average_lpppb);
        } else {

            setValue(0.0);
        }

    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgLPPP(*this); }
};


//////////////////////////////////////////////////////////////////////////////

class AvgRPPP : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(AvgRPPP)
    double average_rpppb;
    double average_rpppe;

    public:

    AvgRPPP()
    {
        setSymbol("average_rppp");
        setInternalName("Average Right Peak Power Phase Length");
    }
    void initialize() {
        setName(tr("Average Right Peak Power Phase Length"));
        setMetricUnits(tr("°"));
        setType(RideMetric::Average);
        setPrecision(0);
    }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

        average_rpppb = deps.value("average_rpppb")->value(true);
        average_rpppe = deps.value("average_rpppe")->value(true);

        if (average_rpppe>0)  {

            setValue(average_rpppe-average_rpppb);
        } else {

            setValue(0.0);
        }

    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    RideMetric *clone() const { return new AvgRPPP(*this); }
};

static bool addLeftRight()
{
    QVector<QString> deps;
    RideMetricFactory::instance().addMetric(AvgLTE(), &deps);
    RideMetricFactory::instance().addMetric(AvgRTE(), &deps);
    RideMetricFactory::instance().addMetric(AvgLPS(), &deps);
    RideMetricFactory::instance().addMetric(AvgRPS(), &deps);
    RideMetricFactory::instance().addMetric(AvgLPCO(), &deps);
    RideMetricFactory::instance().addMetric(AvgRPCO(), &deps);
    RideMetricFactory::instance().addMetric(AvgLPPB(), &deps);
    RideMetricFactory::instance().addMetric(AvgRPPB(), &deps);
    RideMetricFactory::instance().addMetric(AvgLPPE(), &deps);
    RideMetricFactory::instance().addMetric(AvgRPPE(), &deps);
    RideMetricFactory::instance().addMetric(AvgLPPPB(), &deps);
    RideMetricFactory::instance().addMetric(AvgRPPPB(), &deps);
    RideMetricFactory::instance().addMetric(AvgLPPPE(), &deps);
    RideMetricFactory::instance().addMetric(AvgRPPPE(), &deps);

    deps.append("average_lppb");
    deps.append("average_lppe");
    deps.append("average_rppb");
    deps.append("average_rppe");
    deps.append("average_lpppb");
    deps.append("average_lpppe");
    deps.append("average_rpppb");
    deps.append("average_rpppe");

    RideMetricFactory::instance().addMetric(AvgLPP(), &deps);
    RideMetricFactory::instance().addMetric(AvgRPP(), &deps);
    RideMetricFactory::instance().addMetric(AvgLPPP(), &deps);
    RideMetricFactory::instance().addMetric(AvgRPPP(), &deps);
    return true;
}
static bool leftRightAdded = addLeftRight();
//////////////////////////////////////////////////////////////////////////////

struct TotalCalories : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TotalCalories)

    private:
    double average_hr, athlete_weight, duration, athlete_age;

    public:

    TotalCalories()
    {
        setSymbol("total_kcalories");
        setInternalName("Calories");
    }
    void initialize() {
        setName(tr("Calories (HR)"));
        setMetricUnits(tr("kcal"));
        setImperialUnits(tr("kcal"));
        setType(RideMetric::Total);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *context) {

        average_hr = deps.value("average_hr")->value(true);
        athlete_weight = deps.value("athlete_weight")->value(true);
        duration = deps.value("time_riding")->value(true); // time_riding or workout_time ?

        athlete_age = ride->startTime().date().year() - appsettings->cvalue(context->athlete->cyclist, GC_DOB).toDate().year();
        bool male = appsettings->cvalue(context->athlete->cyclist, GC_SEX).toInt() == 0;

        double kcalories = 0.0;

        if (duration>0 && average_hr>0 && athlete_weight>0 && athlete_age>0) {
            //Male: ((-55.0969 + (0.6309 x HR) + (0.1988 x W) + (0.2017 x A))/4.184) x 60 x T
            //Female: ((-20.4022 + (0.4472 x HR) - (0.1263 x W) + (0.074 x A))/4.184) x 60 x T
            if (male)
                kcalories = ((-55.0969 + (0.6309 * average_hr) + (0.1988 * athlete_weight) + (0.2017 * athlete_age))/4.184) * duration/60;
            else
                kcalories = ((-20.4022 + (0.4472 * average_hr) + (0.1263 * athlete_weight) + (0.074 * athlete_age))/4.184) * duration/60;

        }
        setValue(kcalories);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    RideMetric *clone() const { return new TotalCalories(*this); }
};

static bool addTotalCalories() {
    QVector<QString> deps;

    deps.append("average_hr");
    deps.append("time_riding");
    deps.append("athlete_weight");

    RideMetricFactory::instance().addMetric(TotalCalories(), &deps);
    return true;
}

static bool totalCaloriesAdded = addTotalCalories();


///////////////////////////////////////////////////////////////////////////////
