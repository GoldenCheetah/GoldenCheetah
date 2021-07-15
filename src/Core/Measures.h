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
        *this = other;
    }
    Measure& operator=(const Measure& other) {
        this->when = other.when;
        this->comment = other.comment;
        this->source = other.source;
        this->originalSource = other.originalSource;
        for (int i = 0; i<MAX_MEASURES; i++) this->values[i] = other.values[i];
        return *this;
    }
    ~Measure() {}

    QDateTime when;              // when was this reading taken
    QString comment;             // user commentary regarding this measurement

    MeasureSource source;
    QString originalSource;      // if delivered from the cloud service

    double values[MAX_MEASURES]; // field values for standard measures

    // used by std::sort
    bool operator< (Measure right) const { return (when < right.when); }

    // calculate a CRC for the Measure data - used to see if
    // data is changed in Configuration pages
    quint16 getFingerprint() const;

    // getdescription text for source
    QString getSourceDescription() const;
};

class MeasuresField {

public:
    MeasuresField() : symbol(""), name(""), metricUnits(""), imperialUnits(""), unitsFactor(1.0), headers(QStringList()) {}

    QString symbol, name, metricUnits, imperialUnits;
    double unitsFactor;
    QStringList headers;
};

class MeasuresGroup {

public:
    // Default constructor intended to access metadata,
    // directory and withData must be provided to access data.
    MeasuresGroup(QString symbol, QString name, QStringList symbols, QStringList names, QStringList metricUnits, QStringList imperialUnits, QList<double> unitsFactors, QList<QStringList> headers,  QDir dir=QDir(), bool withData=false);
    MeasuresGroup(QDir dir=QDir(), bool withData=false) : dir(dir), withData(withData) {}
    ~MeasuresGroup() {}
    void write();
    QList<Measure>& measures() { return measures_; }
    void setMeasures(QList<Measure>&x);
    void getMeasure(QDate date, Measure&) const;

    // Common access to Measures
    QString getSymbol() const { return symbol; }
    QString getName() const { return name; }
    MeasuresField getField(int field);
    QStringList getFieldSymbols() const { return symbols; }
    QStringList getFieldNames() const { return names; }
    QStringList getFieldMetricUnits() const { return metricUnits; }
    QStringList getFieldImperialUnits() const { return imperialUnits; }
    QStringList getFieldHeaders(int field) const { return headers.value(field); }
    QList<double> getFieldUnitsFactors() const { return unitsFactors; }

    QString getFieldUnits(int field, bool useMetricUnits=true) const { return useMetricUnits ? metricUnits.value(field) : imperialUnits.value(field); }
    double getFieldValue(QDate date, int field=0, bool useMetricUnits=true) const;
    QDate getStartDate() const;
    QDate getEndDate() const;

    // Setters for config
    void setSymbol(QString s) { symbol = s; }
    void setName(QString n) { name = n; }
    void addField(MeasuresField &fieldSettings);
    void setField(int field, MeasuresField &fieldSettings);
    void removeField(int field);

protected:
    const QDir dir;
    const bool withData;

private:
    QString symbol, name;
    QStringList symbols, names, metricUnits, imperialUnits;
    QList<double> unitsFactors;
    QList<QStringList> headers;
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
    void saveConfig();

    QStringList getGroupSymbols() const;
    QStringList getGroupNames() const;
    QList<MeasuresGroup*> getGroups() { return groups; };
    MeasuresGroup* getGroup(int group);
    void removeGroup(int group);
    void addGroup(MeasuresGroup* measuresGroup);

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
