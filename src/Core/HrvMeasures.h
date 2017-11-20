/*
 * Copyright (c) 2015 Joern Rischmueller (joern.rm@gmail.com)
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

#ifndef _Gc_HrvMeasures_h
#define _Gc_HrvMeasures_h

#include "Context.h"
#include "Measures.h"

#include <QString>
#include <QStringList>
#include <QDateTime>

class HrvMeasure : public Measure {
    Q_DECLARE_TR_FUNCTIONS(HrvMeasure)
public:

    enum hrvmeasuretype { HR = 0, AVNN = 1, SDNN = 2, RMSSD = 3, PNN50 = 4, LF = 5, HF = 6, RECOVERY_POINTS = 7 };
    typedef enum hrvmeasuretype HrvMeasureType;

    HrvMeasure() : hr(0), avnn(0), sdnn(0), rmssd(0), pnn50(0), lf(0), hf(0), recovery_points(0) {}

    // depending on datasource not all fields may be filled with actual values

    double  hr,             // Average heart rate
            avnn,           // Average of all NN intervals
            sdnn,           // Standard Deviation of all NN intervals
            rmssd,          // Square root of the mean of the squares of differences between adjacent NN intervals
            pnn50,          // Percentage of differences between adjacent NN intervals that are greater than 50 ms
            lf,             // Power in Low Frequency range
            hf,             // Power in High Frequency range
            recovery_points;// Log transform of rMSSD

    // calculate a CRC for the HrvMeasure data - used to see if
    // data is changed in Configuration pages
    quint16 getFingerprint() const;
};

class HrvMeasureParser  {

public:
    static bool serialize(QString, QList<HrvMeasure> &);
    static bool unserialize(QFile &, QList<HrvMeasure> &);
};

class HrvMeasures : public MeasuresGroup {
    Q_DECLARE_TR_FUNCTIONS(HrvMeasures)
public:
    // Default constructor intended to access metadata,
    // directory and withData must be provided to access data.
    HrvMeasures(QDir dir=QDir(), bool withData=false);
    ~HrvMeasures() {}
    void write();
    QList<HrvMeasure>& hrvMeasures() { return hrvMeasures_; }
    void setHrvMeasures(QList<HrvMeasure>&x);
    void getHrvMeasure(QDate date, HrvMeasure&) const;

    QString getSymbol() const { return "Hrv"; }
    QString getName() const { return tr("Hrv"); }
    QStringList getFieldSymbols() const ;
    QStringList getFieldNames() const;
    QDate getStartDate() const;
    QDate getEndDate() const;
    QString getFieldUnits(int field, bool useMetricUnits=true) const;
    double getFieldValue(QDate date, int field, bool useMetricUnits=true) const;

private:
    QDir dir;
    bool withData;
    QList<HrvMeasure> hrvMeasures_;
};

#endif
