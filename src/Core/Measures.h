/*
 * Copyright (c) 2017 Ale Martinez (amtriathlon@gmail.com)
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

#ifndef _Gc_Measures_h
#define _Gc_Measures_h

#include "GoldenCheetah.h"

#include <QDate>
#include <QDir>
#include <QString>
#include <QStringList>

#define MAX_MEASURES 16
class Measure {
    Q_DECLARE_TR_FUNCTIONS(Measure)
public:

    enum measuresource { Manual, Withings, TodaysPlan, CSV };
    typedef enum measuresource MeasureSource;

    enum bodymeasuretype { WeightKg = 0, FatKg = 1, MuscleKg = 2, BonesKg = 3, LeanKg = 4, FatPercent = 5 };

    Measure() : when(QDateTime()), comment(""),
                source(Manual), originalSource("") {
        for (int i = 0; i<MAX_MEASURES; i++) values[i] = 0.0;
    }
    Measure(const Measure &other) {
        this->when = other.when;
        this->comment = other.comment;
        this->source = other.source;
        this->originalSource = other.originalSource;
        for (int i = 0; i<MAX_MEASURES; i++) this->values[i] = other.values[i];
    }
    virtual ~Measure() {}

    QDateTime when;              // when was this reading taken
    QString comment;             // user commentary regarding this measurement

    MeasureSource source;
    QString originalSource;      // if delivered from the cloud service

    double values[MAX_MEASURES]; // field values for standard measures

    // used by qSort()
    bool operator< (Measure right) const {
        return (when < right.when);
    }
    // calculate a CRC for the Measure data - used to see if
    // data is changed in Configuration pages
    virtual quint16 getFingerprint() const;

    // getdescription text for source
    virtual QString getSourceDescription() const;
};

class MeasuresGroup {

public:
    // Default constructor intended to access metadata,
    // directory and withData must be provided to access data.
    MeasuresGroup(QString symbol, QString name, QStringList symbols, QStringList names, QStringList metricUnits, QStringList imperialUnits, QList<double> unitsFactors, QList<QStringList> headers,  QDir dir=QDir(), bool withData=false);
    MeasuresGroup(QDir dir=QDir(), bool withData=false) : dir(dir), withData(withData) {}
    virtual ~MeasuresGroup() {}
    virtual void write();
    virtual QList<Measure>& measures() { return measures_; }
    virtual void setMeasures(QList<Measure>&x);
    virtual void getMeasure(QDate date, Measure&) const;

    // Common access to Measures
    virtual QString getSymbol() const { return symbol; }
    virtual QString getName() const { return name; }
    virtual QStringList getFieldSymbols() const { return symbols; }
    virtual QStringList getFieldNames() const { return names; }
    virtual QStringList getFieldHeaders(int field) const { return headers.value(field); }
    virtual QString getFieldUnits(int field, bool useMetricUnits=true) const { return useMetricUnits ? metricUnits.value(field) : imperialUnits.value(field); }
    virtual QList<double> getFieldUnitsFactors() const { return unitsFactors; }
    virtual double getFieldValue(QDate date, int field=0, bool useMetricUnits=true) const;
    virtual QDate getStartDate() const;
    virtual QDate getEndDate() const;

protected:
    const QDir dir;
    const bool withData;

private:
    const QString symbol, name;
    const QStringList symbols, names, metricUnits, imperialUnits;
    const QList<double> unitsFactors;
    const QList<QStringList> headers;
    QList<Measure> measures_;

    bool serialize(QString, QList<Measure> &);
    bool unserialize(QFile &, QList<Measure> &);
};

class Measures {
    Q_DECLARE_TR_FUNCTIONS(Measures)
public:
    // Default constructor intended to access metadata,
    // directory and withData must be provided to access data.
    Measures(QDir dir=QDir(), bool withData=false);
    ~Measures();

    QStringList getGroupSymbols() const;
    QStringList getGroupNames() const;
    MeasuresGroup* getGroup(int group);

    enum measuresgrouptype { Body = 0, Hrv = 1 };
    typedef enum measuresgrouptype MeasuresGroupType;

    // Convenience methods to simplify client code
    QStringList getFieldSymbols(int group) const;
    QStringList getFieldNames(int group) const;
    QDate getStartDate(int group) const;
    QDate getEndDate(int group) const;
    QString getFieldUnits(int group, int field, bool useMetricUnits=true) const;
    double getFieldValue(int group, QDate date, int field, bool useMetricUnits=true) const;

private:
    QDir dir;
    bool withData;
    QList<MeasuresGroup*> groups;
};

#endif
