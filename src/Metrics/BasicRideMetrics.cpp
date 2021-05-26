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
#include "IntervalItem.h"
#include "LTMOutliers.h"
#include "Units.h"
#include "Zones.h"
#include "cmath"
#include <assert.h>
#include <algorithm>
#include <QVector>
#include <QApplication>

class RideDate : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(RideDate)
    public:

    RideDate()
    {
        setSymbol("activity_date"); // ride_date already special (!!)
        setInternalName("Activity Date");
    }
    bool isDate() const { return true; }
    void initialize() {
        setName(tr("Activity Date"));
        setType(MetricType::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setDescription(tr("Activity Date"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {
        setValue (QDate(1900,01,01).daysTo(item->dateTime.date()));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new RideDate(*this); }
};

static bool dateAdded =
    RideMetricFactory::instance().addMetric(RideDate());

//////////////////////////////////////////////////////////////////////////////
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
        setType(MetricType::Total);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setDescription(tr("Activity Count"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &) {
        setValue(1);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new RideCount(*this); }
};

static bool countAdded =
    RideMetricFactory::instance().addMetric(RideCount());

//////////////////////////////////////////////////////////////////////////////


class ToExhaustion : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ToExhaustion)
    public:

    ToExhaustion()
    {
        setSymbol("ride_te");
        setInternalName("To Exhaustion");
    }
    void initialize() {
        setName(tr("To Exhaustion"));
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setDescription(tr("Count of exhaustion points marked by the user in an activity"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {
        int c=0;
        if (item && item->ride()) {
            foreach(RideFilePoint *rp, item->ride()->referencePoints()) {
                if (rp->secs > 0) c++;
            }
        }
        setValue(c);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new ToExhaustion(*this); }
};

static bool teAdded =
    RideMetricFactory::instance().addMetric(ToExhaustion());

class ElapsedTime : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ElapsedTime)
    public:

    ElapsedTime()
    {
        setSymbol("elapsed_time");
        setInternalName("Elapsed Time");
    }

    bool isTime() const { return true; }

    void initialize() {
        setName(tr("Elapsed Time"));
        setMetricUnits(tr("secs"));
        setImperialUnits(tr(""));
        setDescription(tr("Only useful for intervals, time the interval started"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {
        Q_UNUSED(item)

        setValue(0);
        if (spec.interval()) setValue(spec.interval()->start);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new ElapsedTime(*this); }
};

static bool etAdded =
    RideMetricFactory::instance().addMetric(ElapsedTime());

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
        setDescription(tr("Total Duration including pauses a.k.a. Elapsed Time"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        RideFileIterator it(item->ride(), spec);

        // just subtract first timepoint from last timepoint
        if (it.last() && it.first()) 
            seconds = it.last()->secs - it.first()->secs + item->ride()->recIntSecs();
        else seconds = RideFile::NA;

        setValue(seconds);
        
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new WorkoutTime(*this); }
};

static bool workoutTimeAdded =
    RideMetricFactory::instance().addMetric(WorkoutTime());

//////////////////////////////////////////////////////////////////////////////

class TimeRecording : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TimeRecording)
    double secsRecording;

    public:

    TimeRecording() : secsRecording(0.0)
    {
        setSymbol("time_recording");
        setInternalName("Time Recording");
    }

    bool isTime() const { return true; }

    void initialize() {
        setName(tr("Time Recording"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Time when device was recording, excludes gaps in recording due to pauses or missing samples"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        secsRecording = 0;

        // loop through and count
        for (RideFileIterator it(item->ride(), spec); it.hasNext(); it.next())
            secsRecording += item->ride()->recIntSecs();
        setValue(secsRecording);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new TimeRecording(*this); }
};

static bool timeRecordingAdded =
    RideMetricFactory::instance().addMetric(TimeRecording());

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
        setDescription(tr("Time with speed or cadence different from zero"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        secsMovingOrPedaling = 0;

        // must have speed and cadence
        if (item->ride()->areDataPresent()->kph || item->ride()->areDataPresent()->cad ) {

            // loop through and count
            RideFileIterator it(item->ride(), spec);
            while (it.hasNext()) {
                struct RideFilePoint *point = it.next();
                if ((point->kph > 0.0) || (point->cad > 0.0))
                    secsMovingOrPedaling += item->ride()->recIntSecs();
            }
        }
        setValue(secsMovingOrPedaling);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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

    TimeCarrying() : secsCarrying(0.0), prevalt(0.0)
    {
        setSymbol("time_carrying");
        setInternalName("Time Carrying");
    }

    bool isTime() const { return true; }

    void initialize() {
        setName(tr("Time Carrying (Est)"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Time with low speed and elevation gain but no power nor cadence"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        secsCarrying = 0;

        if (item->ride()->areDataPresent()->kph) {

            // hysteresis can be configured, we default to 3.0
            double hysteresis = appsettings->value(NULL, GC_ELEVATION_HYSTERESIS).toDouble();
            if (hysteresis <= 0.1) hysteresis = 3.00;

            RideFileIterator it(item->ride(), spec);
            bool first = true;

            while (it.hasNext()) {
                struct RideFilePoint *point = it.next();

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

                    secsCarrying += item->ride()->recIntSecs();
            }
        }
        setValue(secsCarrying);

    }

    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Elevation gained at low speed with no power nor cadence"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // hysteresis can be configured, we default to 3.0
        double hysteresis = appsettings->value(NULL, GC_ELEVATION_HYSTERESIS).toDouble();
        if (hysteresis <= 0.1) hysteresis = 3.00;

        bool first = true;

        RideFileIterator it(item->ride(), spec);

        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

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

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Total Distance in km or miles"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // Note: The 'km' in each sample is the distance travelled by the
        // *end* of the sampling period.  The last term in this equation
        // accounts for the distance traveled *during* the first sample.
        if (item->ride()->areDataPresent()->km) {

            RideFileIterator it(item->ride(), spec);

            // just subtract first from last
            if (it.last() && it.first()) 
                km = it.last()->km - it.first()->km;
            else km = RideFile::NA;

            if (km != RideFile::NA && item->ride()->areDataPresent()->kph)
                km += it.first()->kph / 3600.0 * item->ride()->recIntSecs();

        } else {

            km = 0;

        }
        setValue(km);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new TotalDistance(*this); }
};

static bool totalDistanceAdded =
    RideMetricFactory::instance().addMetric(TotalDistance());

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
        setDescription(tr("According to Dan Conelly: Elevation Gain ^2 / Distance / 1000, 100 is HARD"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        double rating = 0.0f;
        double distance = deps.value("total_distance")->value(true);

        if (item->ride()->areDataPresent()->alt) {

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
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Weight in kg or lbs: first from Athlete body measurements, then from Activity metadata and last from Athlete configuration with 75kg default"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {

        // body measures first
        double weight = item->getWeight();

        // from metadata
        if (!weight) weight = item->getText("Weight", "0.0").toDouble();

        // global options
        if (!weight) weight = appsettings->cvalue(item->context->athlete->cyclist, GC_WEIGHT, "75.0").toString().toDouble(); // default to 75kg

        // No weight default is weird, we'll set to 80kg
        if (weight <= 0.00) weight = 80.00;

        setValue(weight);
        setCount(1);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Athlete bodyfat in kg or lbs from body measurements"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {

        setValue(item->getWeight(Measure::FatKg));
        if (item->getWeight(Measure::FatKg) > 0)
            setValue(item->getWeight(Measure::FatKg));
        else
            setValue(item->getWeight() * item->getWeight(Measure::FatPercent) / 100.0);
        setCount(1);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new AthleteFat(*this); }
};

static bool athleteFatAdded =
    RideMetricFactory::instance().addMetric(AthleteFat());

//////////////////////////////////////////////////////////////////////////////

class AthleteBones : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AthleteBones)
    double kg;

    public:

    AthleteBones() : kg(0.0)
    {
        setSymbol("athlete_bones");
        setInternalName("Athlete Bones");
    }

    void initialize() {
        setName(tr("Athlete Bones"));
        setType(RideMetric::Average);
        setMetricUnits(tr("kg"));
        setImperialUnits(tr("lbs"));
        setPrecision(2);
        setConversion(LB_PER_KG);
        setDescription(tr("Athlete bones in kg or lbs from body measurements"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {
        setValue(item->getWeight(Measure::BonesKg));
        setCount(1);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new AthleteBones(*this); }
};

static bool athleteBonesAdded =
    RideMetricFactory::instance().addMetric(AthleteBones());

//////////////////////////////////////////////////////////////////////////////

class AthleteMuscles : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AthleteMuscles)
    double kg;

    public:

    AthleteMuscles() : kg(0.0)
    {
        setSymbol("athlete_muscles");
        setInternalName("Athlete Muscles");
    }

    void initialize() {
        setName(tr("Athlete Muscles"));
        setType(RideMetric::Average);
        setMetricUnits(tr("kg"));
        setImperialUnits(tr("lbs"));
        setPrecision(2);
        setConversion(LB_PER_KG);
        setDescription(tr("Athlete muscles in kg or lbs from body measurements"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {
        setValue(item->getWeight(Measure::MuscleKg));
        setCount(1);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new AthleteMuscles(*this); }
};

static bool athleteMusclesAdded =
    RideMetricFactory::instance().addMetric(AthleteMuscles());

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
        setDescription(tr("Lean Weight in kg or lbs from body measurements"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {
        setValue(item->getWeight(Measure::LeanKg));
        setCount(1);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Bodyfat in Percent from body measurements"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {
        if (item->getWeight(Measure::FatPercent) > 0)
            setValue(item->getWeight(Measure::FatPercent));
        else if (item->getWeight() > 0)
            setValue(100 * item->getWeight(Measure::FatKg) / item->getWeight());
        else
            setValue(0.0);
        setCount(1);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new AthleteFatP(*this); }
};

static bool athleteFatPAdded =
    RideMetricFactory::instance().addMetric(AthleteFatP());

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
        setDescription(tr("Elevation Gain in meters of feets"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // hysteresis can be configured, we default to 3.0
        double hysteresis = appsettings->value(NULL, GC_ELEVATION_HYSTERESIS).toDouble();
        if (hysteresis <= 0.1) hysteresis = 3.00;

        bool first = true;
        RideFileIterator it(item->ride(), spec);

        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

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
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Elevation Loss in meters of feets"));
    }


    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // hysteresis can be configured, we default to 3.0
        double hysteresis = appsettings->value(NULL, GC_ELEVATION_HYSTERESIS).toDouble();
        if (hysteresis <= 0.1) hysteresis = 3.00;

        bool first = true;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

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
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setType(RideMetric::Total);
        setMetricUnits(tr("kJ"));
        setImperialUnits(tr("kJ"));
        setDescription(tr("Total Work in kJ computed from power data"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        joules = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->watts >= 0.0)
                joules += point->watts * item->ride()->recIntSecs();
        }
        setValue(joules/1000);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Average Speed in kph or mph, computed from distance over time when speed not zero"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &deps) {

        assert(deps.contains("total_distance"));
        km = deps.value("total_distance")->value(true);

        if (item->ride()->areDataPresent()->kph) {

            secsMoving = 0;

            RideFileIterator it(item->ride(), spec);
            while (it.hasNext()) {
                struct RideFilePoint *point = it.next();
                if (point->kph > 0.0) secsMoving += item->ride()->recIntSecs();
            }

            setValue(secsMoving ? km / secsMoving * 3600.0 : 0.0);

        } else {

            // backup to duration if there is no speed channel
            assert(deps.contains("workout_time"));
            secsMoving = deps.value("workout_time")->value(true);
            setValue(secsMoving ? km / secsMoving * 3600.0 : 0.0);

        }
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new AvgSpeed(*this); }
};

static bool avgSpeedAdded =
    RideMetricFactory::instance().addMetric(
        AvgSpeed(), &(QVector<QString>() << "total_distance" << "workout_time"));

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
        setDescription(tr("Average Power from all samples with power greater than or equal to zero"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (item->ride() == NULL || !item->ride()->areDataPresent()->watts || item->ride()->dataPoints().count() == 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;
    
        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->watts >= 0.0) {
                total += point->watts;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Average Muscle Oxygen Saturation, the percentage of hemoglobin that is carrying oxygen."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (item->ride() == NULL || !item->ride()->areDataPresent()->smo2 || item->ride()->dataPoints().count() == 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->smo2 > 0.0f) {  // SmO2 should always be > 0.0f
                total += point->smo2;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("O"); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Average total hemoglobin concentration. The total grams of hemoglobin per deciliter."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (item->ride() == NULL || !item->ride()->areDataPresent()->thb || item->ride()->dataPoints().count() == 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->thb > 0.0f) {
                total += point->thb;
                ++count;
            }
        }
        setValue(count > 0.0f ? total / count : 0.0f);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("O"); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Average altitude power. Recorded power adjusted to take into account the effect of altitude on vo2max and thus power output."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->apower >= 0.0) {
                total += point->apower;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Average Power without zero values, it gives inflated values when frecuent coasting is present"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (item->ride() == NULL || !item->ride()->areDataPresent()->watts || item->ride()->dataPoints().count() == 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->watts > 0.0) {
                total += point->watts;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Average Heart Rate computed for samples when hr is greater than zero"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (item->ride() == NULL || !item->ride()->areDataPresent()->hr || item->ride()->dataPoints().count() == 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;
        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->hr > 0) {
                total += point->hr;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Average Core Temperature. The core body temperature estimate is based on HR data"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->tcore > 0) {
                total += point->tcore;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Total Heartbeats"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            total += (point->hr / 60) * item->ride()->recIntSecs();
        }
        setValue(total);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Power to Heart Rate Ratio in watts/bpm"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        AvgHeartRate *hr = dynamic_cast<AvgHeartRate*>(deps.value("average_hr"));
        AvgPower *pw = dynamic_cast<AvgPower*>(deps.value("average_power"));

        if (hr->value(true) > 100 && pw->value(true) > 100) { // ignore silly rides with low values
            setValue(pw->value(true) / hr->value(true));
        } else {
            setValue(RideFile::NIL);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Work * Heartbeats / 100000"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        TotalWork *work = dynamic_cast<TotalWork*>(deps.value("total_work"));
        HeartBeats *hb = dynamic_cast<HeartBeats*>(deps.value("heartbeats"));

        // if either zero we get zero
        setValue((work->value() * hb->value()) / 100000.00f);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Watts to RPE ratio"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        double ratio = 0.0f;
        AvgPower *pw = dynamic_cast<AvgPower*>(deps.value("average_power"));
        double rpe = item->getText("RPE", "0").toDouble();

        if (pw->value(true) > 100 && rpe > 0) { // ignore silly rides with low values
            ratio = pw->value(true) / rpe;
        }
        setValue(ratio);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Power as percent of Pmax according to Power Zones"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        double percent = 0.0f;
        AvgPower *pw = dynamic_cast<AvgPower*>(deps.value("average_power"));

        if (pw->value(true) > 0.0f && item->context->athlete->zones(item->sport) && item->zoneRange >= 0) {

            // get Pmax
            double pmax = item->context->athlete->zones(item->sport)->getPmax(item->zoneRange);
            percent = pw->value(true)/pmax * 100;
        }
        setValue(percent);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Iso Power to Average Heart Rate ratio in watts/bpm"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        AvgHeartRate *hr = dynamic_cast<AvgHeartRate*>(deps.value("average_hr"));
        RideMetric *pw = dynamic_cast<RideMetric*>(deps.value("coggan_np"));

        if (hr->value(true) > 100 && pw->value(true) > 100) { // ignore silly rides with low values
            setValue(pw->value(true) / hr->value(true));
        } else {
            setValue(0);
        }
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Average Cadence, computed when Cadence > 0"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->cad > 0) {
                total += point->cad;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : count);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("C") && !ride->isRun; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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

    // override to special case NA
    QString toString(bool useMetricUnits) const {
        if (value() == RideFile::NA) return "-";
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
        setDescription(tr("Average Temp from activity data"));
    }


    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (item->ride() == NULL || !item->ride()->areDataPresent()->temp || item->ride()->dataPoints().count() == 0) {
            setValue(RideFile::NA);
            setCount(0);
            return;
        }

        total = count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->temp != RideFile::NA) {
                total += point->temp;
                ++count;
            }
        }

        setValue(count > 0 ? (total / count) : count);
        setCount(count);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Maximum Power"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->watts >= max)
                max = point->watts;
        }
        setValue(max);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Maximum Muscle Oxygen Saturation, the percentage of hemoglobin that is carrying oxygen."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->smo2 >= max)
                max = point->smo2;
        }
        setValue(max);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("O"); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Maximum total hemoglobin concentration. The total grams of hemoglobin per deciliter."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->thb >= max)
                max = point->thb;
        }
        setValue(max);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("O"); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setType(RideMetric::Low);
        setDescription(tr("Minimum Muscle Oxygen Saturation, the percentage of hemoglobin that is carrying oxygen."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        bool notset = true;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->smo2 >= 0.0f && (notset || point->smo2 < min)) {
                min = point->smo2;
                if (point->smo2 > 0.0f && notset)
                  notset = false;
            }
        }
        setValue(min);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Minimum total hemoglobin concentration. The total grams of hemoglobin per deciliter."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        bool notset = true;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->thb > 0.0f && (notset || point->thb < min)) {
                min = point->thb;
                notset = false;
            }
        }
        setValue(min);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Maximum Heart Rate."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->hr >= max)
                max = point->hr;
        }
        setValue(max);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setType(RideMetric::Low);
        setDescription(tr("Minimum Heart Rate."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        bool notset = true;
        min = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->hr > 0 && (notset || point->hr < min)) {
                min = point->hr;
                notset = false;
            }
        }
        setValue(min);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Maximum Core Temperature. The core body temperature estimate is based on HR data"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->tcore >= max)
                max = point->tcore;
        }
        setValue(max);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Maximum Speed"));
    }


    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double max = 0.0;

        if (item->ride()->areDataPresent()->kph) {

            RideFileIterator it(item->ride(), spec);
            while (it.hasNext()) {
                struct RideFilePoint *point = it.next();
                    if (point->kph > max) max = point->kph;
            }
        }
        setValue(max);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Maximum Cadence"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double max = 0.0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->cad > max) max = point->cad;
        }

        setValue(max);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("C") && !ride->isRun; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Maximum Temperature"));
    }

    // override to special case NA
    QString toString(bool useMetricUnits) const {
        if (value() == RideFile::NA) return "-";
        return RideMetric::toString(useMetricUnits);
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->temp) {
            setValue(RideFile::NA);
            setCount(0);
            return;
        }

        double max = RideFile::NA;
        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->temp != RideFile::NA && point->temp > max) max = point->temp;
        }

        setValue(max);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new MaxTemp(*this); }
};

static bool maxTempAdded =
    RideMetricFactory::instance().addMetric(MaxTemp());

//////////////////////////////////////////////////////////////////////////////

class MinTemp : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MinTemp)
    public:

    MinTemp()
    {
        setSymbol("min_temp");
        setInternalName("Min Temp");
    }

    void initialize() {
        setName(tr("Min Temp"));
        setMetricUnits(tr("C"));
        setImperialUnits(tr("F"));
        setType(RideMetric::Low);
        setPrecision(1);
        setConversion(FAHRENHEIT_PER_CENTIGRADE);
        setConversionSum(FAHRENHEIT_ADD_CENTIGRADE);
        setDescription(tr("Minimum Temperature"));
    }

    // override to special case NA
    QString toString(bool useMetricUnits) const {
        if (value() == RideFile::NA) return "-";
        return RideMetric::toString(useMetricUnits);
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->temp) {
            setValue(RideFile::NA);
            setCount(0);
            return;
        }

        double min = 10000;
        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->temp != RideFile::NA && point->temp < min) min = point->temp;
        }

        setValue(min < 10000 ? min : (double)(RideFile::NA));
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new MinTemp(*this); }
};

static bool minTempAdded =
    RideMetricFactory::instance().addMetric(MinTemp());

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
        setDescription(tr("Heart Rate for which 95% of activity samples has lower HR values"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        QVector<double> hrs;
        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
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
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Velocita Ascensionale Media, average ascent speed in vertical meters per hour"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        ElevationGain *el = dynamic_cast<ElevationGain*>(deps.value("elevation_gain"));
        WorkoutTime *wt = dynamic_cast<WorkoutTime*>(deps.value("workout_time"));
        setValue((el->value(true)*3600)/wt->value(true));
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Relationship between altitude adjusted power and recorded power"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        AAvgPower *aap = dynamic_cast<AAvgPower*>(deps.value("average_apower"));
        AvgPower *ap = dynamic_cast<AvgPower*>(deps.value("average_power"));
        setValue(((aap->value(true)-ap->value(true))/aap->value(true)) * 100.00f);
    }


    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Elevation Gain to Total Distance percent ratio"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        ElevationGain *el = dynamic_cast<ElevationGain*>(deps.value("elevation_gain"));
        TotalDistance *td = dynamic_cast<TotalDistance*>(deps.value("total_distance"));
        if (td->value(true) && el->value(true)) {
            setValue(100.00 *(el->value(true) / (1000 *td->value(true))));
        } else
            setValue(0.0);
    }

    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Mean Power Deviation with respect to 30sec Moving Average"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        RideFileIterator it(item->ride(), spec);

        // Less than 30s don't bother
        if ((it.last()->secs - it.first()->secs) < 30) {
            setValue(0);
            topRank=0.00;
        } else {

            QVector<double> power;
            QVector<double> secs;

            while (it.hasNext()) {
                struct RideFilePoint *point = it.next();
                power.append(point->watts);
                secs.append(point->secs);
            }

            LTMOutliers outliers(secs.data(), power.data(), power.count(), 30, false);
            setValue(outliers.getStdDeviation());
            topRank = outliers.getYForRank(0);
        }
    }

    bool isRelevantForRide(const RideItem *ride) const { 
        return ride->present.contains("P") || (!ride->isSwim && !ride->isRun);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Maximum Power Deviation with respect to 30sec Moving Average"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &deps) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        MeanPowerVariance *mean = dynamic_cast<MeanPowerVariance*>(deps.value("meanpowervariance"));

        RideFileIterator it(item->ride(), spec);

        // Less than 30s don't bother
        if ((it.last()->secs - it.first()->secs) < 30)
            setValue(0);
        else
            setValue(mean->topRank);
    }

    bool isRelevantForRide(const RideItem *ride) const {
        return ride->present.contains("P") || (!ride->isSwim && !ride->isRun);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It measures how much of the power delivered to the left pedal is pushing it forward, on average."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->watts || !item->ride()->areDataPresent()->lte) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double total = 0.0f;
        double samples = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->lte && point->watts > 0.0f && point->cad && point->lrbalance != RideFile::NA) {
                samples ++;
                total += point->lte;
            }
        }

        if (total > 0.0f && samples > 0.0f) setValue(total / samples);
        else setValue(0.0);

    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It measures how much of the power delivered to the right pedal is pushing it forward, on average."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->watts || !item->ride()->areDataPresent()->rte) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double total = 0.0f;
        double samples = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->rte && point->watts > 0.0f && point->cad && point->lrbalance != RideFile::NA) {
                samples ++;
                total += point->rte;
            }
        }

        if (total > 0.0f && samples > 0.0f) setValue(total / samples);
        else setValue(0.0);

    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It measures how smoothly power is delivered to the left pedal throughout the revolution, on average."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->watts || !item->ride()->areDataPresent()->lps) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double total = 0.0f;
        double samples = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->lps && point->watts > 0.0f && point->cad && point->lrbalance != RideFile::NA) {
                samples ++;
                total += point->lps;
            }
        }

        if (total > 0.0f && samples > 0.0f) setValue(total / samples);
        else setValue(0.0);
    }

    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It measures how smoothly power is delivered to the right pedal throughout the revolution, on average."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->watts || !item->ride()->areDataPresent()->rps) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double total = 0.0f;
        double samples = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->rps && point->watts > 0.0f && point->cad && point->lrbalance != RideFile::NA) {
                samples ++;
                total += point->rps;
            }
        }

        if (total > 0.0f && samples > 0.0f) setValue(total / samples);
        else setValue(0.0);
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Platform center offset is the location on the left pedal platform where you apply force, on average."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->lpco) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double total = 0.0f;
        double secs = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->cad) {
                secs += item->ride()->recIntSecs();
                total += point->lpco;
            }
        }

        if (secs > 0.0f) setValue(total / secs);
        else setValue(0.0);
    }

    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Platform center offset is the location on the right pedal platform where you apply force, on average."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->rpco) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double total = 0.0f;
        double secs = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->cad) {
                secs += item->ride()->recIntSecs();
                total += point->rpco;
            }
        }

        if (secs > 0.0f) setValue(total / secs);
        else setValue(0.0);
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It is the left pedal stroke angle where you start producing positive power, on average."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->lppb) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double total = 0.0f;
        double secs = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->lppe>0) { // use for average if we have an end
                secs += item->ride()->recIntSecs();
                total += point->lppb + (point->lppb>180?-360:0);
            }
        }

        if (secs > 0.0f) setValue(total / secs);
        else setValue(0.0);
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It is the right pedal stroke angle where you start producing positive power, on average."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->rppb) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double total = 0.0f;
        double secs = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->rppe>0) { // use for average if we have an end
                secs += item->ride()->recIntSecs();
                total += point->rppb + (point->rppb>180?-360:0);
            }
        }

        if (secs > 0.0f) setValue(total / secs);
        else setValue(0.0);
    }

    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It is the left pedal stroke angle where you end producing positive power, on average."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->lppe) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double total = 0.0f;
        double secs = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->lppe > 0) {
                secs += item->ride()->recIntSecs();
                total += point->lppe;
            }
        }

        if (secs > 0.0f) setValue(total / secs);
        else setValue(0.0);
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It is the right pedal stroke angle where you end producing positive power, on average."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->rppe) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double total = 0.0f;
        double secs = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->rppe > 0) { // end has to be > 0
                secs += item->ride()->recIntSecs();
                total += point->rppe;
            }
        }

        if (secs > 0.0f) setValue(total / secs);
        else setValue(0.0);
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It is the left pedal stroke angle where you start producing peak power, on average."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->lpppb) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double total = 0.0f;
        double secs = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->lpppe>0) { // use for average if we have an end
                secs += item->ride()->recIntSecs();
                total += point->lpppb + (point->lpppb>180?-360:0);
            }
        }

        if (secs > 0.0f) setValue(total / secs);
        else setValue(0.0);
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It is the right pedal stroke angle where you start producing peak power, on average."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->rpppb) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double total = 0.0f;
        double secs = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->rpppe>0) { // use for average if we have an end
                secs += item->ride()->recIntSecs();
                total += point->rpppb + (point->rpppb>180?-360:0);
            }
        }

        if (secs > 0.0f) setValue(total / secs);
        else setValue(0.0);
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It is the left pedal stroke angle where you end producing peak power, on average."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->lppe) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double total = 0.0f;
        double secs = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->lpppe > 0) { // end has to be > 0
                secs += item->ride()->recIntSecs();
                total += point->lpppe;
            }
        }

        if (secs > 0.0f) setValue(total / secs);
        else setValue(0.0);
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It is the right pedal stroke angle where you end producing peak power, on average."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->rpppe) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double total = 0.0f;
        double secs = 0.0f;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->rpppe > 0) { // end has to be > 0
                secs += item->ride()->recIntSecs();
                total += point->rpppe;
            }
        }

        if (secs > 0.0f) setValue(total / secs);
        else setValue(0.0);
    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It is the left pedal stroke region length where you produce positive power, on average."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        average_lppb = deps.value("average_lppb")->value(true);
        average_lppe = deps.value("average_lppe")->value(true);

        if (average_lppe>0)  {

            setValue(average_lppe-average_lppb);

        } else {

            setValue(0.0);
        }

    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It is the right pedal stroke region length where you produce positive power, on average."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        average_rppb = deps.value("average_rppb")->value(true);
        average_rppe = deps.value("average_rppe")->value(true);

        if (average_rppe>0)  {

            setValue(average_rppe-average_rppb);
        } else {

            setValue(0.0);
        }

    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It is the left pedal stroke region length where you produce peak power, on average."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        average_lpppb = deps.value("average_lpppb")->value(true);
        average_lpppe = deps.value("average_lpppe")->value(true);

        if (average_lpppe>0)  {

            setValue(average_lpppe-average_lpppb);

        } else {

            setValue(0.0);
        }

    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("It is the right pedal stroke region length where you produce peak power, on average."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        average_rpppb = deps.value("average_rpppb")->value(true);
        average_rpppe = deps.value("average_rpppe")->value(true);

        if (average_rpppe>0)  {

            setValue(average_rpppe-average_rpppb);
        } else {

            setValue(0.0);
        }

    }
    bool isRelevantForRide(const RideItem *ride) const { return !ride->isSwim && !ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Total Calories estimated from Time Moving, Heart Rate, Weight, Sex and Age"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        average_hr = deps.value("average_hr")->value(true);
        athlete_weight = deps.value("athlete_weight")->value(true);
        duration = deps.value("time_riding")->value(true); // time_riding or workout_time ?

        athlete_age = item->dateTime.date().year() - appsettings->cvalue(item->context->athlete->cyclist, GC_DOB).toDate().year();
        bool male = appsettings->cvalue(item->context->athlete->cyclist, GC_SEX).toInt() == 0;

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

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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


struct ActivityCRC : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ActivityCRC)

    private:
    long int CRC;

    public:

    ActivityCRC()
    {
        setSymbol("activity_crc");
        setInternalName("Checksum");
    }

    void initialize() {
        setName(tr("Checksum"));
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setType(RideMetric::Total);
        setDescription(tr("A checksum for the activity, can be used to trigger cache refresh in R scripts."));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {

        setValue(item->crc + item->metacrc + item->dateTime.toMSecsSinceEpoch());
    }

    bool isRelevantForRide(const RideItem *) const { return true; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new ActivityCRC(*this); }
};

static bool addActivityCRC() {
    QVector<QString> deps;
    RideMetricFactory::instance().addMetric(ActivityCRC(), &deps);
    return true;
}

static bool ActivityCRCAdded = addActivityCRC();


///////////////////////////////////////////////////////////////////////////////
