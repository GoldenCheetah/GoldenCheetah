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

#ifndef _GC_RideMetric_h
#define _GC_RideMetric_h 1
#include "GoldenCheetah.h"

#include <QHash>
#include <QString>
#include <QVector>
#include <QSharedPointer>
#include <assert.h>
#include <math.h>
#include <QDebug>

#include "RideFile.h"

class Zones;
class HrZones;
class Context;
class RideMetric;
class RideFile;

typedef QSharedPointer<RideMetric> RideMetricPtr;

class RideMetric {

public:

    enum metrictype { Total, Average, Peak, Low } types;
    typedef enum metrictype MetricType;
    int index_;

    RideMetric() {
        // some sensible defaults
        aggregate_ = true;
        conversion_ = 1.0;
        conversionSum_ = 0.0;
        precision_ = 0;
        type_ = Total;
        count_ = 1;
        value_ = 0.0;
        index_ = -1;
    }
    virtual ~RideMetric() {}

    // need an index for offset into array, each metric
    // now has a numeric identifier from 0 - metricCount
    int index() const { return index_; }
    void setIndex(int x) { index_ = x; }

    // Initialization moved from constructor to enable translation
    virtual void initialize() {}

    // The string by which we refer to this RideMetric in the code,
    // configuration files, and caches (like stress.cache).  It should
    // not be translated, and it should never be shown to the user.
    virtual QString symbol() const { return symbol_; }

    // A short string suitable for showing to the user in the ride
    // summaries, configuration dialogs, etc.  It should be translated
    // using tr().
    virtual QString name() const { return name_; }

    // English name used in metadata.xml for compatibility
    virtual QString internalName() const { return internalName_; }

    // What type of metric is this?
    // Drives the way metrics combined over a day or week in the
    // Long term metrics charts
    virtual MetricType type() const { return type_; }

    // The units in which this RideMetric is expressed.  It should be
    // translated using tr().
    virtual QString units(bool metric) const { return metric ? metricUnits_ : imperialUnits_; }

    // How many digits after the decimal we should show when displaying the
    // value of a RideMetric.
    virtual int precision() const { return precision_; }

    // The actual value of this ride metric, in the units above.
    virtual double value(bool metric) const { return metric ? value_ : (value_ * conversion_); }

    // for averages the count of items included in the average
    virtual double count() const { return count_; }

    // when aggregating averages, should we include zeroes ? no by default
    virtual bool aggregateZero() const { return false; }

    // Factor to multiple value to convert from metric to imperial
    virtual double conversion() const { return conversion_; }
    // And sum for example Fahrenheit from CentigradE
    virtual double conversionSum() const { return conversionSum_; }

    // Compute the ride metric from a file.
    virtual void compute(const RideFile *ride,
                         const Zones *zones, int zoneRange,
                         const HrZones *hrzones, int hrzoneRange,
                         const QHash<QString,RideMetric*> &deps,
                         const Context *context = 0) = 0;

    // is a time value, ie. render as hh:mm:ss
    virtual bool isTime() const { return false; }

    // Convert value to string, taking into account metric pref
    virtual QString toString(bool useMetricUnits);

    // Fill in the value of the ride metric using the mapping provided.  For
    // example, average speed might be specified by the mapping
    //
    //   { "km" => "18.0", "seconds" => "3600" }
    //
    // As the name suggests, we use this function instead of compute() to
    // override the value of a RideMetric using a value specified elsewhere.
    void override(const QMap<QString,QString> &map) {
        if (map.contains("value")) value_ = map.value("value").toDouble();
    }

    // Base implementation of aggregate just sums, derived classes
    // can override this for other purposes
    virtual void aggregateWith(const RideMetric &other) {
        switch (type_) {
        default:
        case Total:
            setValue(value(true) + other.value(true));
            break;
        case Peak:
            setValue(value(true) > other.value(true) ? value(true) : other.value(true));
            break;
        case Average:
            setValue(((value(true) * count()) + (other.value(true) * other.count()))
                                / (count()+other.count()));
            break;
        }
    }
    virtual bool canAggregate() { return aggregate_; }

    virtual RideMetric *clone() const = 0;

    static QHash<QString,RideMetricPtr>
    computeMetrics(const Context *context, const RideFile *ride, const Zones *zones, const HrZones *hrZones,
                   const QStringList &metrics);

    // Initialisers for derived classes to setup basic data
    void setValue(double x) { value_ = x; }
    void setCount(double x) { count_ = x; }
    void setConversion(double x) { conversion_ = x; }
    void setConversionSum(double x) { conversionSum_ = x; }
    void setPrecision(int x) { precision_ = x; }
    void setMetricUnits(QString x) { metricUnits_ = x; }
    void setImperialUnits(QString x) { imperialUnits_ = x; }
    void setName(QString x) { name_ = x; }
    void setInternalName(QString x) { internalName_ = x; }
    void setSymbol(QString x) { symbol_ = x; }
    void setType(MetricType x) { type_ = x; }
    void setAggregate(bool x) { aggregate_ = x; }

    private:
        bool    aggregate_;
        double  value_,
                count_, // used when averaging
                conversion_,
                conversionSum_,
                precision_;

        QString metricUnits_, imperialUnits_;
        QString name_, symbol_, internalName_;
        MetricType type_;
};

class RideMetricFactory {

    static RideMetricFactory *_instance;
    static QVector<QString> noDeps;

    QVector<QString> metricNames;
    QVector<RideMetric::MetricType> metricTypes;
    QHash<QString,RideMetric*> metrics;
    QHash<QString,QVector<QString>*> dependencyMap;
    bool dependenciesChecked;

    RideMetricFactory() : dependenciesChecked(false) {}
    RideMetricFactory(const RideMetricFactory &other);
    RideMetricFactory &operator=(const RideMetricFactory &other);

    void checkDependencies() const {
        if (dependenciesChecked) return;
        foreach(const QString &dependee, dependencyMap.keys()) {
            foreach(const QString &dependency, *dependencyMap[dependee])
                assert(metrics.contains(dependency));
        }
        const_cast<RideMetricFactory*>(this)->dependenciesChecked = true;
    }

    public:

    static RideMetricFactory &instance() {
        if (!_instance)
            _instance = new RideMetricFactory();
        return *_instance;
    }

    int metricCount() const { return metricNames.size(); }

    void initialize() {
        foreach(const QString &metricName, metrics.keys())
            metrics[metricName]->initialize();
    }

    const QString &metricName(int i) const { return metricNames[i]; }
    const RideMetric::MetricType &metricType(int i) const { return metricTypes[i]; }
    const RideMetric *rideMetric(QString name) const { return metrics.value(name, NULL); }

    bool haveMetric(const QString &symbol) const {
        return metrics.contains(symbol);
    }

    RideMetric *newMetric(const QString &symbol) const {
        assert(metrics.contains(symbol));
        checkDependencies();
        return metrics.value(symbol)->clone();
    }

    bool addMetric(const RideMetric &metric,
                   const QVector<QString> *deps = NULL) {
        assert(!metrics.contains(metric.symbol()));
        RideMetric *newMetric = metric.clone();
        newMetric->setIndex(metrics.count());
        metrics.insert(metric.symbol(), newMetric);
        metricNames.append(metric.symbol());
        metricTypes.append(metric.type());
        if (deps) {
            QVector<QString> *copy = new QVector<QString>;
            for (int i = 0; i < deps->size(); ++i)
                copy->append((*deps)[i]);
            dependencyMap.insert(metric.symbol(), copy);
            dependenciesChecked = false;
        }
        return true;
    }

    const QVector<QString> &dependencies(const QString &symbol) const {
        assert(metrics.contains(symbol));
        QVector<QString> *result = dependencyMap.value(symbol);
        return result ? *result : noDeps;
    }
};

#endif // _GC_RideMetric_h

