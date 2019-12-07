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

class Measure {
    Q_DECLARE_TR_FUNCTIONS(Measure)
public:

    enum measuresource { Manual, Withings, TodaysPlan, CSV };
    typedef enum measuresource MeasureSource;

    Measure() : when(QDateTime()), comment("") {}
    virtual ~Measure() {}

    QDateTime when;         // when was this reading taken
    QString comment;        // user commentary regarding this measurement

    MeasureSource source;
    QString originalSource; // if delivered from the cloud service

    // used by qSort()
    bool operator< (const Measure &right) const {
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
    virtual ~MeasuresGroup() {}

    // Common access to Measures
    virtual QString getSymbol() const = 0;
    virtual QString getName() const = 0;
    virtual QStringList getFieldSymbols() const= 0;
    virtual QStringList getFieldNames() const = 0;
    virtual QDate getStartDate() const = 0;
    virtual QDate getEndDate() const = 0;
    virtual QString getFieldUnits(int field, bool useMetricUnits=true) const = 0;
    virtual double getFieldValue(QDate date, int field=0, bool useMetricUnits=true) const = 0;
};

class Measures {

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
