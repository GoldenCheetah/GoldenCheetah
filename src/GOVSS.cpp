/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 * Based on Coggan.cpp modified by Alejandro Martinez (amtriathlon@gmail.com)
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
#include "PaceZones.h"
#include "Units.h"
#include <math.h>
#include <algorithm>
#include <QApplication>

// NOTE: This code follows the description of LNP, IWF and GOVSS in 
// "Calculation of Power Output and Quantification of Training Stress in
// Distance Runners: The Development of the GOVSS Algorithm", by Phil Skiba:
// http://www.physfarm.com/govss.pdf
// Additional details are taken from a spreadsheet published by Dr. Skiba:
// GOVSSCalculatorVer10 (creation date 4-mar-2006)

// Running Power based on speed and slope
static inline double running_power( double weight, double height,
                                    double speed, double slope=0.0,
                                    double distance=0.0) {
    // Aero contribution - probably needs refinement
    double cAero = 0.25*0.01*pow(speed, 2)*pow(height,-3);
    // Energy Cost of Running according to slope
    double cSlope = 155.4*pow(slope,5) - 30.4*pow(slope,4) - 43.3*pow(slope,3) + 46.3*pow(slope,2) + 19.5*slope + 3.6;
    // Efficiency
    double eff = 0.25 + 0.054 * speed;
    // Kinetic Energy contribution doesn't seem to be right, it is not used.
    double cKin = distance ? pow(speed,2)/distance : 0.0;
    return (cAero + cSlope*eff*(1 - 0.5*speed/8.33) + cKin)*weight*speed;
}

// Lactate Normalized Power, used for GOVSS and xPace calculation
class LNP : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(LNP)
    double lnp;
    double secs;

    public:

    LNP() : lnp(0.0), secs(0.0)
    {
        setSymbol("govss_lnp");
        setInternalName("LNP");
    }
    void initialize() {
        setName("LNP");
        setType(RideMetric::Average);
        setMetricUnits("watts");
        setImperialUnits("watts");
        setPrecision(0);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if(ride->recIntSecs() == 0) return;

        // LNP only makes sense for running
        if (!ride->isRun()) return;

        // unconst naughty boy, get athlete's data
        RideFile *uride = const_cast<RideFile*>(ride);
        double weight = uride->getWeight();
        double height = uride->getHeight();

        int rollingwindowsize120 = 120 / ride->recIntSecs();
        int rollingwindowsize30 = 30 / ride->recIntSecs();

        double total = 0.0;
        int count = 0;

        // no point doing a rolling average if the
        // sample rate is greater than the rolling average
        // window!!
        if (rollingwindowsize30 > 1) {

            QVector<double> rollingSpeed(rollingwindowsize120);
            QVector<double> rollingSlope(rollingwindowsize120);
            QVector<double> rollingPower(rollingwindowsize30);
            int index120 = 0;
            int index30 = 0;
            double sumSpeed = 0.0;
            double sumSlope = 0.0;
            double sumPower = 0.0;

            // loop over the data and convert to a rolling
            // average for the given windowsize
            for (int i=0; i<ride->dataPoints().size(); i++) {

                double speed = ride->dataPoints()[i]->kph/3.6;
                sumSpeed += speed;
                sumSpeed -= rollingSpeed[index120];
                rollingSpeed[index120] = speed;
                double speed120 = sumSpeed/std::min(count+1, rollingwindowsize120); // speed rolling average

                double slope = ride->dataPoints()[i]->slope/100.0;
                sumSlope += slope;
                sumSlope -= rollingSlope[index120];
                rollingSlope[index120] = slope;
                double slope120 = sumSlope/std::min(count+1, rollingwindowsize120); // slope rolling average

                // running power based on 120sec averages
                double watts = running_power(weight, height, speed120, slope120); // sumSpeed*ride->recIntSecs()); KE contribution disabled

                sumPower += watts;
                sumPower -= rollingPower[index30];
                rollingPower[index30] = watts;

                total += pow(sumPower/std::min(count+1, rollingwindowsize30), 4); // raise rolling average to 4th power
                count ++;

                // move index on/round
                index120 = (index120 >= rollingwindowsize120-1) ? 0 : index120+1;
                index30 = (index30 >= rollingwindowsize30-1) ? 0 : index30+1;
            }
        }
        if (count) {
            lnp = pow(total/count, 0.25);
            secs = count * ride->recIntSecs();
        } else {
            lnp = secs = 0;
        }

        setValue(lnp);
        setCount(secs);
    }
    RideMetric *clone() const { return new LNP(*this); }
};

// xPace: constant Pace which, on flat surface, gives same Lactate Normalized Power
class XPace : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(XPace)
    double xPace;

    public:

    XPace() : xPace(0.0)
    {
        setSymbol("xPace");
        setInternalName("xPace");
    }
    void initialize() {
        setName(tr("xPace"));
        setType(RideMetric::Average);
        setMetricUnits(tr("min/km"));
        setImperialUnits(tr("min/mile"));
        setPrecision(1);
        setConversion(KM_PER_MILE);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {
        // xPace only makes sense for running
        if (!ride->isRun()) return;

        // unconst naughty boy, get athlete's data
        RideFile *uride = const_cast<RideFile*>(ride);
        double weight = uride->getWeight();
        double height = uride->getHeight();

        assert(deps.contains("govss_lnp"));
        LNP *lnp = dynamic_cast<LNP*>(deps.value("govss_lnp"));
        assert(lnp);
        double lnp_watts = lnp->value(true);

        // search for speed which gives flat power within 0.001watt of LNP
        // up to around 10 iterations for speed within 0.01m/s or ~1sec/km
        double low = 0.0, high = 10.0, speed;
        if (lnp_watts <= 0.0) speed = low;
        else if (lnp_watts >= running_power(weight, height, high)) speed = high;
        else do {
            speed = (low + high)/2.0;
            double watts = running_power(weight, height, speed);
            if (abs(watts - lnp_watts) < 0.001) break;
            else if (watts < lnp_watts) low = speed;
            else if (watts > lnp_watts) high = speed;
        } while (high - low > 0.01);
        // divide by zero or stupidly low pace
        if (speed > 0.01) xPace = (1000.0/60.0) / speed;
        else xPace = 0.0;

        setValue(xPace);
    }

    RideMetric *clone() const { return new XPace(*this); }
};

// Running Threshold Power based on CV, used for GOVSS calculation
class RTP : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(RTP)

    public:

    RTP()
    {
        setSymbol("govss_rtp");
        setInternalName("RTP");
    }
    void initialize() {
        setName(tr("RTP"));
        setType(RideMetric::Average);
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setPrecision(0);
    }
    void compute(const RideFile *ride, const Zones *, int ,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *context) {
        // LNP only makes sense for running
        if (!ride->isRun()) return;

        // unconst naughty boy, get athlete's data
        RideFile *uride = const_cast<RideFile*>(ride);
        double weight = uride->getWeight();
        double height = uride->getHeight();
        
        const PaceZones *zones = context->athlete->paceZones();
        int zoneRange = context->athlete->paceZones()->whichRange(ride->startTime().date());

        // did user override for this ride?
        double cv = ride->getTag("CV","0").toInt();

        // not overriden so use the set value
        // if it has been set at all
        if (!cv && zones && zoneRange >= 0) 
            cv = zones->getCV(zoneRange);
        
        // Running power at cv on flat surface
        double watts = running_power(weight, height, cv/3.6); //120*cv/3.6); KE contribution disabled

        setValue(watts);
    }

    RideMetric *clone() const { return new RTP(*this); }
};

// Intensity Weighting Factor, used for GOVSS calculation
class IWF : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(IWF)
    double reli;
    double secs;

    public:

    IWF() : reli(0.0), secs(0.0)
    {
        setSymbol("govss_iwf");
        setInternalName("IWF");
    }
    void initialize() {
        setName(tr("IWF"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(2);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {
        // IWF only makes sense for running
        if (!ride->isRun()) return;

        assert(deps.contains("govss_lnp"));
        LNP *lnp = dynamic_cast<LNP*>(deps.value("govss_lnp"));
        assert(lnp);
        assert(deps.contains("govss_rtp"));
        RTP *rtp = dynamic_cast<RTP*>(deps.value("govss_rtp"));
        assert(rtp);
        reli = rtp->value(true) ? lnp->value(true) / rtp->value(true) : 0;
        secs = lnp->count();

        setValue(reli);
        setCount(secs);
    }

    RideMetric *clone() const { return new IWF(*this); }
};

// GOVSS Metric for running
class GOVSS : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(GOVSS)
    double score;

    public:

    GOVSS() : score(0.0)
    {
        setSymbol("govss");
        setInternalName("GOVSS");
    }
    void initialize() {
        setName("GOVSS");
        setType(RideMetric::Total);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
	    const QHash<QString,RideMetric*> &deps,
                 const Context *) {
        // GOVSS only makes sense for running
        if (!ride->isRun()) return;

        assert(deps.contains("govss_lnp"));
        assert(deps.contains("govss_rtp"));
        assert(deps.contains("govss_iwf"));
        LNP *lnp = dynamic_cast<LNP*>(deps.value("govss_lnp"));
        assert(lnp);
        RideMetric *iwf = deps.value("govss_iwf");
        assert(iwf);
        RideMetric *rtp = deps.value("govss_rtp");
        assert(rtp);
        double normWork = lnp->value(true) * lnp->count();
        double rawGOVSS = normWork * iwf->value(true);
        double workInAnHourAtRTP = rtp->value(true) * 3600;
        score = workInAnHourAtRTP ? rawGOVSS / workInAnHourAtRTP * 100.0 : 0;

        setValue(score);
    }

    RideMetric *clone() const { return new GOVSS(*this); }
};

static bool addAllGOVSS() {
    RideMetricFactory::instance().addMetric(LNP());
    RideMetricFactory::instance().addMetric(RTP());
    QVector<QString> deps;
    deps.append("govss_lnp");
    RideMetricFactory::instance().addMetric(XPace(), &deps);
    deps.append("govss_rtp");
    RideMetricFactory::instance().addMetric(IWF(), &deps);
    deps.append("govss_iwf");
    RideMetricFactory::instance().addMetric(GOVSS(), &deps);
    return true;
}

static bool GOVSSAdded = addAllGOVSS();