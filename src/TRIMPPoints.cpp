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
#include "Settings.h"
#include "Zones.h"
#include "HrZones.h"
#include <QObject>
#include <math.h>
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
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("TRIMP Points");
    }
    void initialize() {
#endif
        setName(tr("TRIMP Points"));
        setMetricUnits("");
        setImperialUnits("");
        setType(RideMetric::Total);
    }
    void compute(const RideFile *rideFile,
                 const Zones *, int ,
                 const HrZones *hrZones, int hrZoneRange,
                 const QHash<QString,RideMetric*> &deps)
    {
        if (!hrZones || hrZoneRange < 0) {
            setValue(0);
            return;
        }

        // use resting HR from zones, but allow it to be
        // overriden in ride metadata
        double maxHr = hrZones->getMaxHr(hrZoneRange);
        double restHr = hrZones->getRestHr(hrZoneRange);
        restHr = rideFile->getTag("Rest HR", QString("%1").arg(restHr)).toDouble();

        assert(deps.contains("time_riding"));
        assert(deps.contains("average_hr"));
        //const RideMetric *workoutTimeMetric = deps.value("workout_time");
        const RideMetric *timeRidingMetric = deps.value("time_riding");
        const RideMetric *averageHrMetric = deps.value("average_hr");
        assert(timeRidingMetric);
        assert(averageHrMetric);

        double secs = timeRidingMetric->value(true);
        double hr = averageHrMetric->value(true);

        //TRIMP: = t x %HRR x 0.64e1,92(%HRR)

        // Can we lookup the athletes gender?
        // Default to male if we fail
        QString athlete;
        double ksex = 1.92;
        if ((athlete = rideFile->getTag("Athlete", "unknown")) != "unknown") {
            boost::shared_ptr<QSettings> settings = GetApplicationSettings();
            QString key = QString("%1/%2").arg(athlete).arg(GC_SEX);
            if (settings->value(key).toInt() == 1) ksex = 1.67; // Female
            else ksex = 1.92; // Male
        }

        // ok lets work the score out
        score = (secs == 0.0 || hr<restHr) ? 0.0 :  secs/60 *
                (hr-restHr)/(maxHr-restHr)*0.64*exp(ksex*(hr-restHr)/(maxHr-restHr));
        setValue(score);
    }

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
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("TRIMP(100) Points");
    }
    void initialize() {
#endif
        setName(tr("TRIMP(100) Points"));
        setMetricUnits("");
        setImperialUnits("");
        setType(RideMetric::Total);
    }
    void compute(const RideFile *rideFile,
                 const Zones *, int,
                 const HrZones *hrZones, int hrZoneRange,
                 const QHash<QString,RideMetric*> &deps)
    {
        if (!hrZones || hrZoneRange < 0) {
            setValue(0);
            return;
        }

        // use resting HR from zones, but allow it to be
        // overriden in ride metadata
        double maxHr = hrZones->getMaxHr(hrZoneRange);
        double restHr = hrZones->getRestHr(hrZoneRange);
        double ltHr = hrZones->getLT(hrZoneRange);
        restHr = rideFile->getTag("Rest HR", QString("%1").arg(restHr)).toDouble();


        assert(deps.contains("trimp_points"));
        const RideMetric *trimpPointsMetric = deps.value("trimp_points");
        assert(trimpPointsMetric);
        double trimp = trimpPointsMetric->value(true);

        //TRIMP: = t x %HRR x 0.64e1,92(%HRR)

        // Can we lookup the athletes gender?
        // Default to male if we fail
        QString athlete;
        double ksex = 1.92;
        if ((athlete = rideFile->getTag("Athlete", "unknown")) != "unknown") {
            boost::shared_ptr<QSettings> settings = GetApplicationSettings();
            QString key = QString("%1/%2").arg(athlete).arg(GC_SEX);
            if (settings->value(key).toInt() == 1) ksex = 1.67; // Female
            else ksex = 1.92; // Male
        }

        score = trimp == 0.0 ? 0.0 :  100 * trimp /
                (60 * (ltHr-restHr)/(maxHr-restHr)*0.64*exp(ksex*(ltHr-restHr)/(maxHr-restHr)));

        setValue(score);
    }
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
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("TRIMP Zonal Points");
    }
    void initialize() {
#endif
        setName(tr("TRIMP Zonal Points"));
        setMetricUnits("");
        setImperialUnits("");
        setType(RideMetric::Total);
    }
    void compute(const RideFile *,
                 const Zones *, int,
                 const HrZones *hrZones, int hrZoneRange,
                 const QHash<QString,RideMetric*> &deps)
    {
        assert(deps.contains("average_hr"));
        const RideMetric *averageHrMetric = deps.value("average_hr");
        assert(averageHrMetric);
        double hr = averageHrMetric->value(true);

        if (hrZoneRange == -1 || hr == 0) {
            setValue(0);
            return;
        }

        QList <double> trimpk = hrZones->getZoneTrimps(hrZoneRange);
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

        setValue(value/60);
        return;
    }
    RideMetric *clone() const { return new TRIMPZonalPoints(*this); }
};


// RPE is the rate of percieved exercion (borg scale).
// Is a numerical value the riders give in "average" fatigue of the training session he percieved.
//
// Calculate the session RPE that is the product of RPE * time (minutes) of training/race ride. I
// We have 3 different "training load" parameters:
//    - internal load (TRIMPS)
//    - external load (bikescore/TSS)
//    - perceived load (session RPE)
//
class SessionRPE : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TRIMPPoints)

    double score;

    public:

    SessionRPE() : score(0.0)
    {
        setSymbol("session_rpe");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Session RPE");
    }
    void initialize() {
#endif
        setName(tr("Session RPE"));
        setMetricUnits("");
        setImperialUnits("");
        setType(RideMetric::Total);
    }
    void compute(const RideFile *rideFile,
                 const Zones *, int ,
                 const HrZones *hrZones, int hrZoneRange,
                 const QHash<QString,RideMetric*> &deps)
    {
        // use RPE value in ride metadata
        double rpe = rideFile->getTag("RPE", "0.0").toDouble();

        assert(deps.contains("time_riding"));
        const RideMetric *timeRidingMetric = deps.value("time_riding");
        assert(timeRidingMetric);

        double secs = timeRidingMetric->value(true);

        // ok lets work the score out
        score = ((secs == 0.0 || rpe == 0) ? 0.0 :  secs/60 *rpe);
        setValue(score);
    }

    RideMetric *clone() const { return new SessionRPE(*this); }
};

static bool added() {
    QVector<QString> deps;
    deps.append("time_riding");
    deps.append("average_hr");
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
    RideMetricFactory::instance().addMetric(TRIMPZonalPoints(), &deps);

    deps.clear();
    deps.append("time_riding");
    RideMetricFactory::instance().addMetric(SessionRPE(), &deps);
    return true;
}

static bool added_ = added();
