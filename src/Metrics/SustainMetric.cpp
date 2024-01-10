/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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
#include "Specification.h"
#include "RideItem.h"
#include "LTMOutliers.h"
#include "Units.h"
#include "Zones.h"
#include "cmath"
#include <algorithm>
#include <QVector>
#include <QApplication>


// all the sustain metrics are filled in when the intervals
// are updated and will be zero for intervals
class L1Sustain : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(L1Sustain)
    public:

    L1Sustain()
    {
        setSymbol("l1_sustain");
        setInternalName("L1 Sustained Time");
    }

    bool isTime() const { return true; }

    void initialize() {
        setName(tr("L1 Sustained Time"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Sustained Time in Power Zone 1, based on (sustained) EFFORT intervals."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &) {
        setValue(0);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new L1Sustain(*this); }
};

static bool l1Added = RideMetricFactory::instance().addMetric(L1Sustain());

class L2Sustain : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(L2Sustain)
    public:

    L2Sustain()
    {
        setSymbol("l2_sustain");
        setInternalName("L2 Sustained Time");
    }

    bool isTime() const { return true; }

    void initialize() {
        setName(tr("L2 Sustained Time"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Sustained Time in Power Zone 2, based on (sustained) EFFORT intervals."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &) {
        setValue(0);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new L2Sustain(*this); }
};

static bool l2Added = RideMetricFactory::instance().addMetric(L2Sustain());

class L3Sustain : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(L3Sustain)
    public:

    L3Sustain()
    {
        setSymbol("l3_sustain");
        setInternalName("L3 Sustained Time");
    }

    bool isTime() const { return true; }
    void initialize() {
        setName(tr("L3 Sustained Time"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Sustained Time in Power Zone 3, based on (sustained) EFFORT intervals."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &) {
        setValue(0);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new L3Sustain(*this); }
};

static bool l3Added = RideMetricFactory::instance().addMetric(L3Sustain());

class L4Sustain : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(L4Sustain)
    public:

    L4Sustain()
    {
        setSymbol("l4_sustain");
        setInternalName("L4 Sustained Time");
    }

    bool isTime() const { return true; }
    void initialize() {
        setName(tr("L4 Sustained Time"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Sustained Time in Power Zone 4, based on (sustained) EFFORT intervals."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &) {
        setValue(0);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new L4Sustain(*this); }
};

static bool l4Added = RideMetricFactory::instance().addMetric(L4Sustain());

class L5Sustain : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(L5Sustain)
    public:

    L5Sustain()
    {
        setSymbol("l5_sustain");
        setInternalName("L5 Sustained Time");
    }

    bool isTime() const { return true; }
    void initialize() {
        setName(tr("L5 Sustained Time"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Sustained Time in Power Zone 5, based on (sustained) EFFORT intervals."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &) {
        setValue(0);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new L5Sustain(*this); }
};

static bool l5Added = RideMetricFactory::instance().addMetric(L5Sustain());

class L6Sustain : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(L6Sustain)
    public:

    L6Sustain()
    {
        setSymbol("l6_sustain");
        setInternalName("L6 Sustained Time");
    }

    bool isTime() const { return true; }
    void initialize() {
        setName(tr("L6 Sustained Time"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Sustained Time in Power Zone 6, based on (sustained) EFFORT intervals."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &) {
        setValue(0);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new L6Sustain(*this); }
};

static bool l6Added = RideMetricFactory::instance().addMetric(L6Sustain());

class L7Sustain : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(L7Sustain)
    public:

    L7Sustain()
    {
        setSymbol("l7_sustain");
        setInternalName("L7 Sustained Time");
    }

    bool isTime() const { return true; }
    void initialize() {
        setName(tr("L7 Sustained Time"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Sustained Time in Power Zone 7, based on (sustained) EFFORT intervals."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &) {
        setValue(0);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new L7Sustain(*this); }
};

static bool l7Added = RideMetricFactory::instance().addMetric(L7Sustain());

class L8Sustain : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(L8Sustain)
    public:

    L8Sustain()
    {
        setSymbol("l8_sustain");
        setInternalName("L8 Sustained Time");
    }

    bool isTime() const { return true; }
    void initialize() {
        setName(tr("L8 Sustained Time"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Sustained Time in Power Zone 8, based on (sustained) EFFORT intervals."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &) {
        setValue(0);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new L8Sustain(*this); }
};

static bool l8Added = RideMetricFactory::instance().addMetric(L8Sustain());

class L9Sustain : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(L9Sustain)
    public:

    L9Sustain()
    {
        setSymbol("l9_sustain");
        setInternalName("L9 Sustained Time");
    }

    bool isTime() const { return true; }
    void initialize() {
        setName(tr("L9 Sustained Time"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Sustained Time in Power Zone 9, based on (sustained) EFFORT intervals."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &) {
        setValue(0);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new L9Sustain(*this); }
};

static bool l9Added = RideMetricFactory::instance().addMetric(L9Sustain());

class L10Sustain : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(L10Sustain)
    public:

    L10Sustain()
    {
        setSymbol("l10_sustain");
        setInternalName("L10 Sustained Time");
    }

    bool isTime() const { return true; }
    void initialize() {
        setName(tr("L10 Sustained Time"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Sustained Time in Power Zone 10, based on (sustained) EFFORT intervals."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &) {
        setValue(0);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new L10Sustain(*this); }
};

static bool l10Added = RideMetricFactory::instance().addMetric(L10Sustain());

