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
#include <cmath>
#include <QDebug>
#include <QMutex>
#include <QList>

#include "RideFile.h"
#include "UserMetricSettings.h"

class Zones;
class HrZones;
class Context;
class RideMetric;
class RideFile;
class RideItem;
class DataFilter;
class DataFilterRuntime;
class Leaf;

// keep track of schema changes
extern int DBSchemaVersion;
extern QList<UserMetricSettings> _userMetrics;
extern quint16 UserMetricSchemaVersion;

typedef QSharedPointer<RideMetric> RideMetricPtr;

class RideMetric {
    Q_DECLARE_TR_FUNCTIONS(RideMetric)

public:

    // Which Sport(s) does this metric apply to (not OR'ed together, Tri/Any shorthands) ?
    enum metricsport { Bike=0x01, Run=0x02, Swim=0x04, Triathlon=0x07, Other=0x08, Any=0xff };
    typedef enum metricsport MetricSport;

    // Type of metric for aggregation
    enum metrictype { Total=0, Average, Peak, Low, RunningTotal, MeanSquareRoot, StdDev};
    typedef enum metrictype MetricType;

    // Class of metric to help browse
    enum metricclass { Undefined, Stress, Volume, Intensity, Distribution, ModelEstimate };
    typedef enum metricclass MetricClass;

    // Validity of metric from a scientific standpoint - to guide users
    //
    // 0 - UNRELIABLE - its known to be problematic (e.g. Estimated Vo2max from 5 min power)
    // 1 - UNKNOWN - we just don't know, there is no data (e.g. TISS)
    // 2 - UNCLEAR - lack of sufficient evidence or concerns exist (e.g. BikeStress and BikeScore)
    // 3 - USEFUL - published and peer reviewed, or results shown independently to be useful (e.g. IsoPower, TRIMP, W'bal)
    // 4 - RELIABLE - Adopted and/or verified by the scientific community (e.g. HRV rmSSD)
    // 5 - HIGH - Known to be highly reliable, assuming sufficient device accuracy (e.g. Average HR, Average Power, Work kJ)
    enum metricvalidity { Unreliable, Unknown, Unclear, Useful, Reliable, High };
    typedef enum metricvalidity MetricValidity;

    int index_;

    RideMetric() {
        // some sensible defaults
        conversion_ = 1.0;
        conversionSum_ = 0.0;
        precision_ = 0;
        type_ = Total;
        count_ = 1;
        value_ = 0.0;
        index_ = -1;
    }
    virtual ~RideMetric() {}

    // is this a user defined one?
    virtual bool isUser() const { return false; }

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

    // classification, to help browse and find metrics
    virtual MetricClass classification() const { return Undefined; }
    virtual MetricValidity validity() const { return Unknown; }
    virtual int sport() const { return Bike; }

    // English name used in metadata.xml for compatibility
    virtual QString internalName() const { return internalName_; }

    // A description explaining metrics details to be shown in tooltips,
    // it should be translated using tr()
    virtual QString description() const { return description_.isEmpty() ? name() : description_; }

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
    virtual double value(bool metric) const { return metric ? value_ : (value_ * conversion_ + conversionSum_); }
    virtual double value(double v, bool metric) const { return metric ? v : (v * conversion_ + conversionSum_); }

    // The internal value of this ride metric, useful to cache and then setValue.
    double value() const { return value_; }

    // for averages the count of items included in the average
    virtual double count() const { return count_; }

    // for std deviation we need to provide stdmean and stdvariances values
    virtual double stdmean() { return 0.0f; }
    virtual double stdvariance() { return 0.0f; }

    // when aggregating averages, should we include zeroes ? no by default
    virtual bool aggregateZero() const { return false; }

    // is this metric relevant
    virtual bool isRelevantForRide(const RideItem *) const { return true; }

    // Factor to multiple value to convert from metric to imperial
    virtual double conversion() const { return conversion_; }
    // And sum for example Fahrenheit from CentigradE
    virtual double conversionSum() const { return conversionSum_; }

    // Compute the ride metric from a file.
    virtual void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &deps) = 0;

    // is a time value, ie. render as hh:mm:ss
    virtual bool isTime() const { return false; }

    // is a date since the epoch ie. render as dd mmm yy
    virtual bool isDate() const { return false; }

    // Convert value to string, taking into account metric pref
    virtual QString toString(bool useMetricUnits) const;
    virtual QString toString(double value) const;

    // Criterium to compare values, overridden by Pace metrics
    // Default just see if it is a RideMetric::Low
    virtual bool isLowerBetter() const { return type_ == Low ? true : false; }

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
        case Low:
            setValue((value(true) > 0.0 && value(true) < other.value(true)) ? value(true) : other.value(true));
            break;
        case Average:
            setValue(((value(true) * count()) + (other.value(true) * other.count()))
                                / (count()+other.count()));
            break;
        case MeanSquareRoot:
            setValue(sqrt((pow(value(true), 2)* count() + pow(other.value(true), 2)* other.count())
                          /(count()+other.count())));
        }
    }

    // because we compute metrics in a map/reduce operation using
    // multiple threads the metric object is cloned to ensure
    // no conflict, but the clone operation can dereference
    // members from source and reference count them to be space efficient
    virtual RideMetric *clone() const { return NULL; }

    static QHash<QString,RideMetricPtr>
    computeMetrics(RideItem *item, Specification spec, const QStringList &metrics);

    // get the value for metric m from precomputed values stored at p
    static double getForSymbol(QString m, const QHash<QString,RideMetric*> *p);

    // generate a CRC based upon the user metric settings
    // using the currently loaded _userMetrics
    static quint16 userMetricFingerprint(QList<UserMetricSettings> these);

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
    void setDescription(QString x) { description_ = x; }
    void setSymbol(QString x) { symbol_ = x; }
    void setType(MetricType x) { type_ = x; }

    protected:
        double  value_,
                count_, // used when averaging
                conversion_,
                conversionSum_,
                precision_;

        QString metricUnits_, imperialUnits_;
        QString name_, symbol_, internalName_, description_;
        MetricType type_;
};


//
// The interface between a UserMetric and the codebase
// for working with ride metrics.
//
class UserMetric: public RideMetric {

public:

    UserMetric(Context *context, UserMetricSettings settings);
    UserMetric(const UserMetric *from);
    ~UserMetric();

    // is this a user defined one?
    bool isUser() const { return true; }

    // did we clone (i.e. datafilter doesn't belong to us)
    bool isClone() const { return clone_; }

    void initialize();
    static void addCompatibility(QList<UserMetricSettings> &metrics);

    QString symbol() const;

    QString name() const; 
    QString internalName() const; 
    QString description() const;

    // Long term metrics charts
    RideMetric::MetricType type() const; 

    // units
    QString units(bool metric) const; 

    // Factor to multiple value to convert from metric to imperial
    double conversion() const;
    // And sum for example Fahrenheit from CentigradE
    double conversionSum() const;

    // How many digits after the decimal we should show when displaying the
    // value of a UserRideMetric.
    int precision() const; 

    // Get the value and apply conversion if needed
    double value(bool metric) const;

    // Apply conversion if needed to the supplied value
    double value(double v, bool metric) const;

    // for averages the count of items included in the average
    double count() const; 

    // when aggregating averages, should we include zeroes ? no by default
    bool aggregateZero() const; 

    // is this metric relevant
    bool isRelevantForRide(const RideItem *) const; 

    // Compute the ride metric from a file.
    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &deps);

    // is a time value, ie. render as hh:mm:ss
    bool isTime() const;

    RideMetric *clone() const; 

    // WE DO NOT REIMPLEMENT THE STANDARD toString() METHOD
    // virtual QString toString(bool useMetricUnits) const;

    // isLowerBetter() reimplemented to use the redefined type()
    virtual bool isLowerBetter() const { return type() == Low ? true : false; }

    private:
    
        using RideMetric::value;

        // all attributes and methods are implemented in the
        // usermetric class (which uses a datafilter and has
        // utility classes for editing, save/load config etc).
        UserMetricSettings settings;

        // and we compile into this for runtime
        DataFilter *program;
        Leaf *root;

        // functions, to save lots of lookups
        Leaf *finit, *frelevant, *fsample, *fbefore, *fafter, *fvalue, *fcount;

        // our runtime
        DataFilterRuntime *rt;

        // true if we are a clone
        bool clone_;

};

class RideMetricFactory {

public:
    static QList<QString> compatibilitymetrics;

private:
    static RideMetricFactory *_instance;
    static QVector<QString> noDeps;

    QStringList metricNames;
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
                if (!metrics.contains(dependency))
                    qDebug()<<"metric dep error:"<<dependency;
        }
        const_cast<RideMetricFactory*>(this)->dependenciesChecked = true;
    }

    public:

    QMutex mutex;

    static RideMetricFactory &instance() {
        if (!_instance)
            _instance = new RideMetricFactory();
        return *_instance;
    }

    int metricCount() const { return metricNames.size(); }
    QHash<QString,RideMetric*> metricHash() const { return metrics; }

    void initialize() {
        foreach(const QString &metricName, metrics.keys())
            metrics[metricName]->initialize();
    }

    const QStringList &allMetrics() const { return metricNames; }
    const QString &metricName(int i) const { return metricNames[i]; }
    const RideMetric::MetricType &metricType(int i) const { return metricTypes[i]; }
    const RideMetric *rideMetric(QString name) const { return metrics.value(name, NULL); }

    bool haveMetric(const QString &symbol) const {
        return metrics.contains(symbol);
    }

    RideMetric *newMetric(const QString &symbol) const {
        checkDependencies();
        return metrics.value(symbol)->clone();
    }

    // clear out user metrics, we're readding them
    void removeUserMetrics() {
        int firstUser=-1;
        for(int i=0; i<metricNames.count(); i++) {
            RideMetric *m = metrics.value(metricNames[i], NULL);
            if (m && m->isUser()) {
                firstUser=i;
                break;
            }
        }

        // now delete
        if (firstUser >0) {
            while (firstUser < metricNames.count()) {
                QString current = metricNames.at(firstUser);

                metrics.remove(current);
                dependencyMap.remove(current);
                metricNames.takeAt(firstUser);
                metricTypes.remove(firstUser);
            }
        }
    }

    bool addMetric(const RideMetric &metric,
                   const QVector<QString> *deps = NULL) {
        if(metrics.contains(metric.symbol())) return false;
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
        if(!metrics.contains(symbol)) return noDeps;
        QVector<QString> *result = dependencyMap.value(symbol);
        return result ? *result : noDeps;
    }
};

#endif // _GC_RideMetric_h

