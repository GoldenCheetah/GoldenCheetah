/*
 * Copyright (c) 2010 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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
#include "RideItem.h"
#include "Settings.h"
#include "Zones.h"
#include "HrZones.h"
#include <cmath>
#include <assert.h>

#include "Context.h"
#include "Athlete.h"
#include "Specification.h"
#include <QApplication>

// This is Morton/Banister with Green et al coefficient.
//
// HR_TRIMP = time(min) * (AvgHR-RHR)/(MaxHR-RHR)*0.64*EXP(Ksex*(AvgHr-RHR)/(MaxHR-RHR))
//
// Ksex = 1.92 for man and 1.67 for woman
// RHR = resting heart rate
//
class TRIMPPoints : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TRIMPPoints)

    double score;

    public:

    static const double K;

    TRIMPPoints() : score(0.0)
    {
        setSymbol("trimp_points");
        setInternalName("TRIMP Points");
    }
    void initialize() {
        setName(tr("TRIMP Points"));
        setMetricUnits("");
        setImperialUnits("");
        setType(RideMetric::Total);
        setDescription(tr("Training Impulse according to Morton/Banister with Green et al coefficient."));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        if (!item->context->athlete->hrZones(item->isRun) || item->hrZoneRange < 0) {
            setValue(RideFile::NIL);
            return;
        }

        // use resting HR from zones, but allow it to be
        // overriden in ride metadata
        double maxHr = item->context->athlete->hrZones(item->isRun)->getMaxHr(item->hrZoneRange);
        double restHr = item->context->athlete->hrZones(item->isRun)->getRestHr(item->hrZoneRange);
        restHr = item->getText("Rest HR", QString("%1").arg(restHr)).toDouble();

        assert(deps.contains("time_riding"));
        assert(deps.contains("workout_time"));
        assert(deps.contains("average_hr"));
        //const RideMetric *workoutTimeMetric = deps.value("workout_time");
        const RideMetric *timeRidingMetric = deps.value("time_riding");
        const RideMetric *averageHrMetric = deps.value("average_hr");
        const RideMetric *durationMetric = deps.value("workout_time");
        assert(timeRidingMetric);
        assert(durationMetric);
        assert(averageHrMetric);

        double secs = timeRidingMetric->value(true) ? timeRidingMetric->value(true) : durationMetric->value(true);;
        double hr = averageHrMetric->value(true);

        //TRIMP: = t x %HRR x 0.64e1,92(%HRR)

        // gender
        double ksex = 1.92;
        if (appsettings->cvalue(item->context->athlete->cyclist, GC_SEX).toInt() == 1) ksex = 1.67; // Female
        else ksex = 1.92; // Male

        // ok lets work the score out
        score = (secs == 0.0 || hr<restHr) ? 0.0 :  secs/60 *
                (hr-restHr)/(maxHr-restHr)*0.64*exp(ksex*(hr-restHr)/(maxHr-restHr));
        setValue(score);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new TRIMPPoints(*this); }
};



class TRIMP100Points : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TRIMP100Points)

    double score;

public:

    static const double K;

    TRIMP100Points() : score(0.0)
    {
        setSymbol("trimp_100_points");
        setInternalName("TRIMP(100) Points");
    }
    void initialize() {
        setName(tr("TRIMP(100) Points"));
        setMetricUnits("");
        setImperialUnits("");
        setType(RideMetric::Total);
        setDescription(tr("TRIMP Points normalized to assign 100 points to 1 hour at threshold heart rate."));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        if (!item->context->athlete->hrZones(item->isRun) || item->hrZoneRange < 0) {
            setValue(RideFile::NIL);
            return;
        }

        // use resting HR from zones, but allow it to be
        // overriden in ride metadata
        double maxHr = item->context->athlete->hrZones(item->isRun)->getMaxHr(item->hrZoneRange);
        double restHr = item->context->athlete->hrZones(item->isRun)->getRestHr(item->hrZoneRange);
        double ltHr = item->context->athlete->hrZones(item->isRun)->getLT(item->hrZoneRange);
        restHr = item->getText("Rest HR", QString("%1").arg(restHr)).toDouble();


        assert(deps.contains("trimp_points"));
        const RideMetric *trimpPointsMetric = deps.value("trimp_points");
        assert(trimpPointsMetric);
        double trimp = trimpPointsMetric->value(true);

        //TRIMP: = t x %HRR x 0.64e1,92(%HRR)

        // gender
        double ksex = 1.92;
        if (appsettings->cvalue(item->context->athlete->cyclist, GC_SEX).toInt() == 1) ksex = 1.67; // Female
        else ksex = 1.92; // Male

        score = trimp == 0.0 ? 0.0 :  100 * trimp /
                (60 * (ltHr-restHr)/(maxHr-restHr)*0.64*exp(ksex*(ltHr-restHr)/(maxHr-restHr)));

        setValue(score);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new TRIMP100Points(*this); }
};

//0.84 (zone 1 64-76%), 1.65 (zone 2 77-83%), 2.57 (zone 3 84-89%), 4.01 (zone 4 90-94%), and 5.91 (zone 5 95-100%)
//1 (zone 1 50-60%), 1.1 (zone 2 60-70%), 1.2 (zone 3 70-80%), 2.2 (zone 4 80-90%), and 4.5 (zone 5 90-100%)

// 0, 68, 83, 94, 105 of LT for LT 80% Max-> 0, 55, 66, 75, 84
// 0.9 (zone 1 0-55%), 1.1 (zone 2 55-66%), 1.2 (zone 3 66-75%), 2 (zone 4 75-84%), and 5 (zone 5 84-100%)

class TRIMPZonalPoints : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TRIMPZonalPoints)

    double score;

public:

    static const double K;

    TRIMPZonalPoints() : score(0.0)
    {
        setSymbol("trimp_zonal_points");
        setInternalName("TRIMP Zonal Points");
    }
    void initialize() {
        setName(tr("TRIMP Zonal Points"));
        setMetricUnits("");
        setImperialUnits("");
        setType(RideMetric::Total);
        setDescription(tr("Training Impulse with time in zones weighted according to coefficients defined in Heart Rate Zones."));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        if (!item->context->athlete->hrZones(item->isRun) || item->hrZoneRange < 0) {
            setValue(RideFile::NIL);
            return;
        }

        assert(deps.contains("average_hr"));
        const RideMetric *averageHrMetric = deps.value("average_hr");
        assert(averageHrMetric);
        double hr = averageHrMetric->value(true);

        QList <double> trimpk = item->context->athlete->hrZones(item->isRun)->getZoneTrimps(item->hrZoneRange);
        double value = 0;

        if (trimpk.size()>0) {
            assert(deps.contains("time_in_zone_H1"));
            const RideMetric *time1Metric = deps.value("time_in_zone_H1");
            assert(time1Metric);
            double time1 = time1Metric->value(true);
            double trimpk1 = trimpk[0];
            value += trimpk1 * time1;
        }

        if (trimpk.size()>1) {
            assert(deps.contains("time_in_zone_H2"));
            const RideMetric *time2Metric = deps.value("time_in_zone_H2");
            assert(time2Metric);
            double time2 = time2Metric->value(true);
            double trimpk2 = trimpk[1];
            value += trimpk2 * time2;
        }

        if (trimpk.size()>2) {
            assert(deps.contains("time_in_zone_H3"));
            const RideMetric *time3Metric = deps.value("time_in_zone_H3");
            assert(time3Metric);
            double time3 = time3Metric->value(true);
            double trimpk3 = trimpk[2];
            value += trimpk3 * time3;
        }

        if (trimpk.size()>3) {
            assert(deps.contains("time_in_zone_H4"));
            const RideMetric *time4Metric = deps.value("time_in_zone_H4");
            assert(time4Metric);
            double time4 = time4Metric->value(true);
            double trimpk4 = trimpk[3];
            value += trimpk4 * time4;
        }

        if (trimpk.size()>4) {
            assert(deps.contains("time_in_zone_H5"));
            const RideMetric *time5Metric = deps.value("time_in_zone_H5");
            assert(time5Metric);
            double time5 = time5Metric->value(true);
            double trimpk5 = trimpk[4];
            value += trimpk5 * time5;
        }

        if (trimpk.size()>5) {
            assert(deps.contains("time_in_zone_H6"));
            const RideMetric *time6Metric = deps.value("time_in_zone_H6");
            assert(time6Metric);
            double time6 = time6Metric->value(true);
            double trimpk6 = trimpk[5];
            value += trimpk6 * time6;
        }

        if (trimpk.size()>6) {
            assert(deps.contains("time_in_zone_H7"));
            const RideMetric *time7Metric = deps.value("time_in_zone_H7");
            assert(time7Metric);
            double time7 = time7Metric->value(true);
            double trimpk7 = trimpk[6];
            value += trimpk7 * time7;
        }

        if (trimpk.size()>7) {
            assert(deps.contains("time_in_zone_H8"));
            const RideMetric *time8Metric = deps.value("time_in_zone_H8");
            assert(time8Metric);
            double time8 = time8Metric->value(true);
            double trimpk8 = trimpk[7];
            value += trimpk8 * time8;
        }

        // When time in zone is 0 fallback to Average HR for zone id,
        // since it could have been overridden, and assign time_riding
        // if available or workout_time to that zone.
        if (value == 0) {
            assert(deps.contains("time_riding"));
            assert(deps.contains("workout_time"));
            const RideMetric *timeRidingMetric = deps.value("time_riding");
            const RideMetric *durationMetric = deps.value("workout_time");
            assert(timeRidingMetric);
            assert(durationMetric);
            double secs = timeRidingMetric->value(true) ?
                            timeRidingMetric->value(true) :
                            durationMetric->value(true);
            int nZone = item->context->athlete->hrZones(item->isRun)->whichZone(item->hrZoneRange, hr);
            if (nZone >= 0 && nZone < trimpk.size())
                value += trimpk[nZone] * secs;
        }

        setValue(value/60);
        return;
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new TRIMPZonalPoints(*this); }
};


// RPE is the rate of perceived exercion (borg scale).
// Is a numerical value the riders give in "average" fatigue of the training session he perceived.
//
// Calculate the session RPE that is the product of RPE * time (minutes) of training/race ride. I
// We have 3 different "training load" parameters:
//    - internal load (TRIMPS)
//    - external load (bikescore/BikeStress)
//    - perceived load (session RPE)
//
class SessionRPE : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(SessionRPE)

    double score;

    public:

    SessionRPE() : score(0.0)
    {
        setSymbol("session_rpe");
        setInternalName("Session RPE");
    }
    void initialize() {
        setName(tr("Session RPE"));
        setMetricUnits("");
        setImperialUnits("");
        setType(RideMetric::Total);
        setDescription(tr("Session RPE is the product of RPE * minutes, where RPE is the rate of perceived exercion (10 point modified borg scale) and minutes is Time Moving if available or Duration otherwise."));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // use RPE value in ride metadata
        double rpe = item->getText("RPE", "0.0").toDouble();

        assert(deps.contains("time_riding"));
        const RideMetric *timeRidingMetric = deps.value("time_riding");
        assert(timeRidingMetric);
        assert(deps.contains("workout_time"));
        const RideMetric *durationMetric = deps.value("workout_time");
        assert(durationMetric);

        double secs = timeRidingMetric->value(true) ? timeRidingMetric->value(true) :
                                                      durationMetric->value(true);;

        // ok lets work the score out
        score = ((secs == 0.0 || rpe == 0) ? 0.0 :  secs/60 *rpe);
        setValue(score);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new SessionRPE(*this); }
};

static bool added() {
    QVector<QString> deps;
    deps.append("time_riding");
    deps.append("average_hr");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(TRIMPPoints(), &deps);

    deps.clear();
    deps.append("trimp_points");
    RideMetricFactory::instance().addMetric(TRIMP100Points(), &deps);

    deps.clear();
    deps.append("average_hr");
    deps.append("time_in_zone_H1");
    deps.append("time_in_zone_H2");
    deps.append("time_in_zone_H3");
    deps.append("time_in_zone_H4");
    deps.append("time_in_zone_H5");
    deps.append("time_in_zone_H6");
    deps.append("time_in_zone_H7");
    deps.append("time_in_zone_H8");
    deps.append("time_riding");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(TRIMPZonalPoints(), &deps);

    deps.clear();
    deps.append("time_riding");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(SessionRPE(), &deps);
    return true;
}

static bool added_ = added();
