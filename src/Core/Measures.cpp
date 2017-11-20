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

#include "Measures.h"
#include "BodyMeasures.h"
#include "HrvMeasures.h"

quint16
Measure::getFingerprint() const
{
    quint64 x = 0;

    x += when.date().toJulianDay();

    QByteArray ba = QByteArray::number(x);
    ba.append(comment);

    return qChecksum(ba, ba.length());
}

QString
Measure::getSourceDescription() const {

    switch(source) {
    case Measure::Manual:
        return tr("Manual entry");
    case Measure::Withings:
        return tr("Withings");
    case Measure::TodaysPlan:
        return tr("Today's Plan");
    case Measure::CSV:
        return tr("CSV Upload");
    default:
        return tr("Unknown");
    }
}

Measures::Measures(QDir dir, bool withData) : dir(dir), withData(withData) {
    // load in MeasuresGroupType order!
    groups.append(new BodyMeasures(dir, withData));
    groups.append(new HrvMeasures(dir, withData));
}

Measures::~Measures() {
    foreach (MeasuresGroup* measuresGroup, groups)
        delete measuresGroup;
}

QStringList
Measures::getGroupSymbols() const {
    QStringList symbols;
    foreach (MeasuresGroup* measuresGroup, groups)
        symbols.append(measuresGroup->getSymbol());
    return symbols;
}

QStringList
Measures::getGroupNames() const {
    QStringList names;
    foreach (MeasuresGroup* measuresGroup, groups)
        names.append(measuresGroup->getName());
    return names;
}

MeasuresGroup*
Measures::getGroup(int group) {
    if (group >= 0 && group < groups.count())
        return groups[group];
    else
        return NULL;
}

QStringList
Measures::getFieldSymbols(int group) const {
    return groups.at(group) != NULL ? groups.at(group)->getFieldSymbols() : QStringList();
}

QStringList
Measures::getFieldNames(int group) const {
    return groups.at(group) != NULL ? groups.at(group)->getFieldNames() : QStringList();
}

QDate
Measures::getStartDate(int group) const {
    return groups.at(group) != NULL ? groups.at(group)->getStartDate() : QDate();
}

QDate
Measures::getEndDate(int group) const {
    return groups.at(group) != NULL ? groups.at(group)->getEndDate() : QDate();
}

QString
Measures::getFieldUnits(int group, int field, bool useMetricUnits) const {
    return groups.at(group) != NULL ? groups.at(group)->getFieldUnits(field, useMetricUnits) : QString();
}

double
Measures::getFieldValue(int group, QDate date, int field, bool useMetricUnits) const {
    return groups.at(group) != NULL ? groups.at(group)->getFieldValue(date, field, useMetricUnits) : 0.0;
}

