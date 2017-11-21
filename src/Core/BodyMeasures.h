/*
 * Copyright (c) 2015 Joern Rischmueller (joern.rm@gmail.com)
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

#ifndef _Gc_BodyMeasures_h
#define _Gc_BodyMeasures_h

#include "Context.h"
#include "Measures.h"

#include <QString>
#include <QStringList>
#include <QDateTime>

class BodyMeasure : public Measure {
    Q_DECLARE_TR_FUNCTIONS(BodyMeasure)
public:

    enum bodymeasuretype { WeightKg = 0, FatKg = 1, MuscleKg = 2, BonesKg = 3, LeanKg = 4, FatPercent = 5 };
    typedef enum bodymeasuretype BodyMeasureType;

    BodyMeasure() : weightkg(0), fatkg(0), musclekg(0), boneskg(0), leankg(0), fatpercent(0) {}

    // depending on datasource not all fields may be filled with actual values

    double  weightkg,       // weight in Kilograms
            fatkg,          // fat in Kilograms
			musclekg,       // muscles in Kilograms
			boneskg,        // bones in Kilograms
            leankg,         // lean mass in Kilograms
            fatpercent;     // body fat as a percentage of weight

    // calculate a CRC for the BodyMeasure data - used to see if
    // data is changed in Configuration pages
    quint16 getFingerprint() const;
};

class BodyMeasureParser  {

public:
    static bool serialize(QString, QList<BodyMeasure> &);
    static bool unserialize(QFile &, QList<BodyMeasure> &);
};


class BodyMeasures : public MeasuresGroup {
    Q_DECLARE_TR_FUNCTIONS(BodyMeasures)
public:
    // Default constructor intended to access metadata,
    // directory and withData must be provided to access data.
    BodyMeasures(QDir dir=QDir(), bool withData=false);
    ~BodyMeasures() {}
    void write();
    QList<BodyMeasure>& bodyMeasures() { return bodyMeasures_; }
    void setBodyMeasures(QList<BodyMeasure>&x);
    void getBodyMeasure(QDate date, BodyMeasure&) const;

    QString getSymbol() const { return "Body"; }
    QString getName() const { return tr("Body"); }
    QStringList getFieldSymbols() const ;
    QStringList getFieldNames() const;
    QDate getStartDate() const;
    QDate getEndDate() const;
    QString getFieldUnits(int field, bool useMetricUnits=true) const;
    double getFieldValue(QDate date, int field=BodyMeasure::WeightKg, bool useMetricUnits=true) const;

private:
    QDir dir;
    bool withData;
    QList<BodyMeasure> bodyMeasures_;
};


#endif
