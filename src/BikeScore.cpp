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
#include "Zones.h"
#include <math.h>

const double  bikeScoreN   = 4.0;
const double  bikeScoreTau = 25.0;

// NOTE: This code follows the description of xPower, Relative Intensity, and
// BikeScore in "Analysis of Power Output and Training Stress in Cyclists: The
// Development of the BikeScore(TM) Algorithm", page 5, by Phil Skiba:
//
// http://www.physfarm.com/Analysis%20of%20Power%20Output%20and%20Training%20Stress%20in%20Cyclists-%20BikeScore.pdf
//
// The weighting factors for the exponentially weighted average are taken from
// a spreadsheet provided by Dr. Skiba.

class XPower : public RideMetric {
    double xpower;
    double secs;

    friend class RelativeIntensity;
    friend class BikeScore;

    public:

    XPower() : xpower(0.0), secs(0.0) {}
    QString name() const { return "skiba_xpower"; }
    QString units(bool) const { return "watts"; }
    double value(bool) const { return xpower; }
    void compute(const RideFile *ride, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {

        double secsDelta = ride->recIntSecs();

	// djconnel:
	double attenuation  = exp(-secsDelta / bikeScoreTau);
	double sampleWeight = 1 - attenuation;
        
        double lastSecs = 0.0;                // previous point
	double initialSecs = 0.0;             // time associated with start of data
        double weighted = 0.0;                // exponentially smoothed power
	double epsilon_time = secsDelta / 100; // for comparison of times

        double total = 0.0;

	/* djconnel: calculate bikescore:
	   For all values of t, smoothed power
	   p* = integral { -infinity to t } (1/tau) exp[(t' - t) / tau] p(t') dt'

	   From this we calculate an integral, xw:
	   xw = integral {t0 to t} { p*^N dt }
	   (in the code, p* -> "weighted"; xw -> "total")

	   During any interval t0 <= t < t1, with p* = p*(t0) at the start of
	   the interval, with power p constant during the interval:

	   p*(t) = p*(t0) exp[(t0 - t) / tau] + p ( 1 - exp[(t0 - t) / tau] )

	   So the contribution to xw is then:

	   delta_xw = integral { t0 to t1 } [ p*(t0) exp[(t0 - t) / tau] + p ( 1 - exp[(t0 - t) / tau] ) ]^N

	   Consider the simplified case p = 0, and t1 = t0 + deltat, then this is evaluated:

	   delta_xw = integral { t0 to t1 } ( p*(t0) exp[(t0 - t) / tau] )^N
	            = integral { t0 to t1 } ( p*(t0)^N exp[N (t0 - t) / tau] )
		    = (tau / N) p*(t0)^N (1 - exp[-N deltat / tau])

	   This is the component which should be added to xw during idle periods.

	   More generally:
	   delta_xw = integral { t0 to t1 } [ p*(t0) exp[(t0 - t) / tau] + p ( 1 - exp[(t0 - t) / tau] ) ]^N
	            = integral { 0 to deltat }
		      [
		         p*(t0)^N exp[-N t' / tau] +
			 N p*(t0)^(N - 1) p exp[-(N - 1) t' / tau] (1 - exp[-t' / tau]) +
			 [N (N - 1) / 2] p*(t0)^(N - 2) p^2 exp[-(N - 2) t' / tau] (1 - exp[-2 t' / tau]) +
			 [N (N - 1) (N - 2) / 6] p*(t0)^(N - 3) p^3 exp[-(N - 3) t' / tau] (1 - exp[-3 t' / tau]) +
			 [N (N - 1) (N - 2) (N - 3) / 24] p*(t0)^(N - 4) p^4 exp[-(N - 4) t' / tau] (1 - exp[-4 t' / tau]) +
			 ...
		      ] dt'

           but a linearized solution is fine as long as the sampling interval is << the smoothing time.
	*/
	   
	
        QListIterator<RideFilePoint*> i(ride->dataPoints());

	int count = 0;
        while (i.hasNext()) {
            const RideFilePoint *point = i.next();

	    // if there are missing data then add in the contribution
	    // from the exponentially decaying smoothed power
	    if (count == 0)
		initialSecs = point->secs - secsDelta;
	    else {
		double dt = point->secs - lastSecs - secsDelta;
		if (dt > epsilon_time) {
		    double alpha = exp(-bikeScoreN * dt / bikeScoreTau);
		    total +=
			(bikeScoreTau / bikeScoreN) * pow(weighted, bikeScoreN) * (1 - alpha);
		    weighted *= exp(-dt / bikeScoreTau);
		}
	    }

	    // the existing weighted average is exponentially decayed by one sampling time,
	    // then the contribution from the present point is added
	    weighted = attenuation * weighted + sampleWeight * point->watts;
            total += pow(weighted, bikeScoreN);

            lastSecs = point->secs;
            count++;
        }

	// after the ride is over, assume idleness (exponentially decaying smoothed power) to infinity
	total += 
	    (bikeScoreTau / bikeScoreN) * pow(weighted, bikeScoreN);

        secs = lastSecs - initialSecs;
	xpower = (secs > 0) ?
	    pow(total * secsDelta / secs, 1 / bikeScoreN) :
	    0.0;
    }

    // added djconnel: allow RI to be combined across rides
    bool canAggregate() const { return true; }
    void aggregateWith(RideMetric *other) { 
        assert(name() == other->name());
	XPower *ap = dynamic_cast<XPower*>(other);
	xpower = pow(xpower, bikeScoreN) * secs + pow(ap->xpower, bikeScoreN) * ap->secs;
	secs += ap->secs;
	xpower = pow(xpower / secs, 1 / bikeScoreN);
    }
    // end added djconnel

    RideMetric *clone() const { return new XPower(*this); }
};

class RelativeIntensity : public RideMetric {
    double reli;
    double secs;

    public:

    RelativeIntensity() : reli(0.0), secs(0.0) {}
    QString name() const { return "skiba_relative_intensity"; }
    QString units(bool) const { return ""; }
    double value(bool) const { return reli; }
    void compute(const RideFile *, const Zones *zones, int zoneRange,
                 const QHash<QString,RideMetric*> &deps) {
        if (zones) {
            assert(deps.contains("skiba_xpower"));
            XPower *xp = dynamic_cast<XPower*>(deps.value("skiba_xpower"));
            assert(xp);
            reli = xp->xpower / zones->getCP(zoneRange);
            secs = xp->secs;
        }
    }

    // added djconnel: allow RI to be combined across rides
    bool canAggregate() const { return true; }
    void aggregateWith(RideMetric *other) { 
        assert(name() == other->name());
	RelativeIntensity *ap = dynamic_cast<RelativeIntensity*>(other);
	reli = secs * pow(reli, bikeScoreN) + ap->secs * pow(ap->reli, bikeScoreN);
	secs += ap->secs;
	reli = pow(reli / secs, 1.0 / bikeScoreN);
    }
    // end added djconnel

    RideMetric *clone() const { return new RelativeIntensity(*this); }
};

class BikeScore : public RideMetric {
    double score;

    public:

    BikeScore() : score(0.0) {}
    QString name() const { return "skiba_bike_score"; }
    QString units(bool) const { return ""; }
    double value(bool) const { return score; }
    void compute(const RideFile *, const Zones *zones, int zoneRange,
                 const QHash<QString,RideMetric*> &deps) {
        if (!zones)
            return;
        assert(deps.contains("skiba_xpower"));
        assert(deps.contains("skiba_relative_intensity"));
        XPower *xp = dynamic_cast<XPower*>(deps.value("skiba_xpower"));
        RideMetric *ri = deps.value("skiba_relative_intensity");
        assert(ri);
        double normWork = xp->xpower * xp->secs;
        double rawBikeScore = normWork * ri->value(true);
        double workInAnHourAtCP = zones->getCP(zoneRange) * 3600; 
        score = rawBikeScore / workInAnHourAtCP * 100.0;
    }
    RideMetric *clone() const { return new BikeScore(*this); }
    bool canAggregate() const { return true; }
    void aggregateWith(RideMetric *other) { score += other->value(true); }
};

static bool addAllThree() {
    RideMetricFactory::instance().addMetric(XPower());
    QVector<QString> deps;
    deps.append("skiba_xpower");
    RideMetricFactory::instance().addMetric(RelativeIntensity(), &deps);
    deps.append("skiba_relative_intensity");
    RideMetricFactory::instance().addMetric(BikeScore(), &deps);
    return true;
}

static bool allThreeAdded = addAllThree();

