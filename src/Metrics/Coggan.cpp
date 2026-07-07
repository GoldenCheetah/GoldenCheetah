/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "Context.h"
#include "RideMetric.h"
#include "RideItem.h"
#include "Zones.h"
#include "Settings.h"
#include "Athlete.h"
#include "Specification.h"
#include "Units.h"
#include <cmath>
#include <assert.h>
#include <QApplication>


class IsoPower : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(IsoPower)
    double np;
    double secs;

    public:

    IsoPower() : np(0.0), secs(0.0)
    {
        setSymbol("coggan_np");
        setInternalName("IsoPower");
    }
    void initialize() {
        setName(tr("IsoPower"));
        setType(RideMetric::Average);
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setPrecision(0);
        setDescription(tr("Iso Power is an estimate of the power that you could have maintained for the same physiological 'cost' if your power output had been perfectly constant."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || item->ride()->recIntSecs() == 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        int rollingwindowsize = 30 / item->ride()->recIntSecs();

        double total = 0;
        int count = 0;

        // no point doing a rolling average if the
        // sample rate is greater than the rolling average
        // window!!
        if (rollingwindowsize > 1) {

            QVector<double> rolling(rollingwindowsize);
            int index = 0;
            double sum = 0;

            // loop over the data and convert to a rolling
            // average for the given windowsize
            RideFileIterator it(item->ride(), spec);
            while (it.hasNext()) {
                struct RideFilePoint *point = it.next();

                sum += point->watts;
                sum -= rolling[index];

                rolling[index] = point->watts;

                total += pow(sum/rollingwindowsize,4); // raise rolling average to 4th power
                count ++;

                // move index on/round
                index = (index >= rollingwindowsize-1) ? 0 : index+1;
            }
        }
        if (count) {
            np = pow(total / (count), 0.25);
            secs = count * item->ride()->recIntSecs();
        } else {
            np = secs = 0;
        }

        setValue(np);
        setCount(secs);
    }

    bool isRelevantForRide(const RideItem*ride) const { return ride->present.contains("P") || (!ride->isRun && !ride->isSwim); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new IsoPower(*this); }
};

class VI : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(VI)
    double vi;
    double secs;

    public:

    VI() : vi(0.0), secs(0.0)
    {
        setSymbol("coggam_variability_index");
        setInternalName("VI");
    }

    void initialize() {
        setName("VI");
        setType(RideMetric::Average);
        setPrecision(3);
        setDescription(tr("Variability Index is the ratio between IsoPower and Average Power."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        assert(deps.contains("coggan_np"));
        assert(deps.contains("average_power"));
        IsoPower *np = dynamic_cast<IsoPower*>(deps.value("coggan_np"));
        assert(np);
        RideMetric *ap = dynamic_cast<RideMetric*>(deps.value("average_power"));
        assert(ap);
        vi = np->value(true) / ap->value(true);
        secs = np->count();

        setValue(vi);
        setCount(secs);
    }

    bool isRelevantForRide(const RideItem*ride) const { return ride->present.contains("P") || (!ride->isRun && !ride->isSwim); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new VI(*this); }
};

class IntensityFactor : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(IntensityFactor)
    double rif;
    double secs;

    public:

    IntensityFactor() : rif(0.0), secs(0.0)
    {
        setSymbol("coggan_if");
        setInternalName("BikeIntensity");
    }

    void initialize() {
        setName("BikeIntensity");
        setType(RideMetric::Average);
        setPrecision(3);
        setDescription(tr("Intensity Factor is the ratio between IsoPower and the Functional Threshold Power (FTP) configured in Power Zones."));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // no zones
        if (!item->context->athlete->zones(item->sport) || item->zoneRange < 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        assert(deps.contains("coggan_np"));
        IsoPower *np = dynamic_cast<IsoPower*>(deps.value("coggan_np"));
        assert(np);

        int ftp = item->getText("FTP","0").toInt();

        bool useCPForFTP = (appsettings->cvalue(item->context->athlete->cyclist, item->context->athlete->zones(item->sport)->useCPforFTPSetting(), 0).toInt() == 0);

        if (useCPForFTP) {
            int cp = item->getText("CP","0").toInt();
            if (cp == 0)
                cp = item->context->athlete->zones(item->sport)->getCP(item->zoneRange);

            ftp = cp;
        }

        rif = np->value(true) / (ftp ? ftp : item->context->athlete->zones(item->sport)->getFTP(item->zoneRange));
        secs = np->count();

        setValue(rif);
        setCount(secs);
    }

    bool isRelevantForRide(const RideItem*ride) const { return ride->present.contains("P") || (!ride->isRun && !ride->isSwim); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new IntensityFactor(*this); }
};

class BikeStress : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(BikeStress)
    double score;

    public:

    BikeStress() : score(0.0)
    {
        setSymbol("coggan_tss");
        setInternalName("BikeStress");
    }

    void initialize() {
        setName("BikeStress");
        setType(RideMetric::Total);
        setDescription(tr("Training Stress Score takes into account both the intensity and the duration of the training session, it can be computed as 100 * hours * IF^2"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // run, swim or no zones
        if (item->isSwim || item->isRun ||
           !item->context->athlete->zones(item->sport) || item->zoneRange < 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        assert(deps.contains("coggan_np"));
        assert(deps.contains("coggan_if"));
        IsoPower *np = dynamic_cast<IsoPower*>(deps.value("coggan_np"));
        RideMetric *rif = deps.value("coggan_if");
        assert(rif);
        double normWork = np->value(true) * np->count();
        double rawTSS = normWork * rif->value(true);

        int ftp = item->getText("FTP","0").toInt();

        bool useCPForFTP = (appsettings->cvalue(item->context->athlete->cyclist, item->context->athlete->zones(item->sport)->useCPforFTPSetting(), 0).toInt() == 0);

        if (useCPForFTP) {
            int cp = item->getText("CP","0").toInt();
            if (cp == 0)
                cp = item->context->athlete->zones(item->sport)->getCP(item->zoneRange);

            ftp = cp;
        }

        double workInAnHourAtCP = (ftp ? ftp : item->context->athlete->zones(item->sport)->getFTP(item->zoneRange)) * 3600;
        score = rawTSS / workInAnHourAtCP * 100.0;

        setValue(score);
    }

    bool isRelevantForRide(const RideItem*ride) const { return (!ride->isRun && !ride->isSwim); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new BikeStress(*this); }
};

class TSSPerHour : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TSSPerHour)
    double points;
    double hours;

    public:

    TSSPerHour() : points(0.0), hours(0.0)
    {
        setSymbol("coggan_tssperhour");
        setInternalName("BikeStress per hour");
    }

    void initialize() {
        setName(tr("BikeStress per hour"));
        setType(RideMetric::Average);
        setPrecision(0);
        setDescription(tr("Training Stress Score divided by Duration in hours"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // doesn't apply to swims or runs
        if (item->isSwim || item->isRun) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // tss
        assert(deps.contains("coggan_tss"));
        BikeStress *tss = dynamic_cast<BikeStress*>(deps.value("coggan_tss"));
        assert(tss);

        // duration
        assert(deps.contains("workout_time"));
        RideMetric *duration = deps.value("workout_time");
        assert(duration);

        points = tss->value(true);
        hours = duration->value(true) / 3600.0;

        // set
        if (hours) setValue(points/hours);
        else setValue(RideFile::NIL);
        setCount(duration->value(true));
    }

    bool isRelevantForRide(const RideItem*ride) const { return (!ride->isRun && !ride->isSwim); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new TSSPerHour(*this); }
};

/* Running update based on: http://www.joefrielsblog.com/2014/11/the-efficiency-factor-in-running.html */
class EfficiencyFactor : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(EfficiencyFactor)
    double ef;

    public:

    EfficiencyFactor() : ef(0.0)
    {
        setSymbol("friel_efficiency_factor");
        setInternalName("Efficiency Factor");
    }

    void initialize() {
        setName(tr("Efficiency Factor"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(3);
        setDescription(tr("The ratio between IsoPower and Average HR for Cycling and xPace (in yd/min) and Average HR for Running"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        assert(deps.contains("coggan_np"));
        assert(deps.contains("xPace"));
        assert(deps.contains("average_hr"));
        if (item->isRun && deps.value("coggan_np")->value() == 0) {
            RideMetric *xPace = dynamic_cast<RideMetric*>(deps.value("xPace"));
            assert(xPace);
            ef = xPace->value(true) > 0 ? ((1000.0/METERS_PER_YARD) / xPace->value(true)) : 0.0;
        } else {
            IsoPower *np = dynamic_cast<IsoPower*>(deps.value("coggan_np"));
            assert(np);
            ef = np->value(true);
        }
        RideMetric *ah = dynamic_cast<RideMetric*>(deps.value("average_hr"));
        assert(ah);
        ef = ah->value(true) > 0 ? ef / ah->value(true) : 0.0;

        setValue(ef);
    }
    bool isRelevantForRide(const RideItem*ride) const { return ride->present.contains("H") && (ride->present.contains("P") || (ride->isRun && ride->present.contains("S"))); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new EfficiencyFactor(*this); }
};

static bool addAllCoggan() {
    RideMetricFactory::instance().addMetric(IsoPower());
    QVector<QString> deps;
    deps.append("coggan_np");
    RideMetricFactory::instance().addMetric(IntensityFactor(), &deps);
    deps.append("coggan_if");
    RideMetricFactory::instance().addMetric(BikeStress(), &deps);
    deps.clear();
    deps.append("coggan_np");
    deps.append("average_power");
    RideMetricFactory::instance().addMetric(VI(), &deps);
    deps.clear();
    deps.append("coggan_np");
    deps.append("xPace");
    deps.append("average_hr");
    RideMetricFactory::instance().addMetric(EfficiencyFactor(), &deps);
    deps.clear();
    deps.append("coggan_tss");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(TSSPerHour(), &deps);
    return true;
}

static bool CogganAdded = addAllCoggan();

