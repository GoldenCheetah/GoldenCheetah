/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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
#include <QApplication>

// This metric computes aerobic decoupling percentage as described
// by Joe Friel:
//
//   http://www.trainingbible.com/pdf/AeT_Training.pdf
//
// Aerobic decoupling is a measure of how much heart rate rises or
// how much power falls off during the course of a long ride.  Joe suggests
// that a rise of more than 5% in the heart rate to power ratio between the
// two halfs of a long ride indicates that the ride was performed above the
// aerobic threshold.
//
// How you actually compute these things over messy data isn't always
// straightforward.  For this implementation, I cut the set of data points in
// half and compute the average power and heart rate over all the points in
// either half that have a non-zero heart rate.  I then calculate the change
// in heart rate to power ratio as described by Friel.

class AerobicDecoupling : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AerobicDecoupling)

    double percent;

    public:

    AerobicDecoupling() : percent(0.0)
    {
        setSymbol("aerobic_decoupling");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Aerobic Decoupling");
    }
    void initialize() {
#endif
        setName(tr("Aerobic Decoupling"));
        setType(RideMetric::Average);
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setPrecision(2);
    }
    void compute(const RideFile *ride, const Zones *, int, const HrZones *, int,
                 const QHash<QString,RideMetric*> &) {
        double firstHalfPower = 0.0, secondHalfPower = 0.0;
        double firstHalfHR = 0.0, secondHalfHR = 0.0;
        int halfway = ride->dataPoints().size() / 2;
        int count = 0;
        int firstHalfCount = 0;
        int secondHalfCount = 0;
        percent = 0;
        foreach(const RideFilePoint *point, ride->dataPoints()) {
            if (count++ < halfway) {
                if (point->hr > 0) {
                    firstHalfPower += point->watts;
                    firstHalfHR += point->hr;
                    ++firstHalfCount;
                }
            }
            else {
                if (point->hr > 0) {
                    secondHalfPower += point->watts;
                    secondHalfHR += point->hr;
                    ++secondHalfCount;
                }
            }
        }
        if ((firstHalfPower > 0) && (secondHalfPower > 0)) {
            firstHalfPower /= firstHalfCount;
            secondHalfPower /= secondHalfCount;
            firstHalfHR /= firstHalfCount;
            secondHalfHR /= secondHalfCount;
            double firstHalfRatio = firstHalfHR / firstHalfPower;
            double secondHalfRatio = secondHalfHR / secondHalfPower;
            percent = 100.0 * (secondHalfRatio - firstHalfRatio) / firstHalfRatio;
        }
        setValue(percent);
    }

    RideMetric *clone() const { return new AerobicDecoupling(*this); }
};

static bool add() {
    RideMetricFactory::instance().addMetric(AerobicDecoupling());
    return true;
}

static bool added = add();

