/*
 * Copyright (c) 2023 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "ModelFilter.h"

#include <QString>
#include <QStringList>
#include <QDebug>


ModelFilter::ModelFilter
(int modelColumn, QMetaType::Type type)
: _modelColumn(modelColumn), _type(type)
{
}


ModelFilter::~ModelFilter
()
{
}


int
ModelFilter::modelColumn
() const
{
    return _modelColumn;
}


QMetaType::Type
ModelFilter::supportedType
() const
{
    return _type;
}


QString
ModelFilter::toString
() const
{
    return QString("%1 %2").arg(toStringPrefix(), toStringSuffix());
}


QString
ModelFilter::toStringPrefix
() const
{
    return QString("%1 on column %2").arg(QString(QMetaType(_type).name())).arg(_modelColumn);
}


///////////////////

ModelNumberRangeFilter::ModelNumberRangeFilter
(int modelColumn, long minValue, long maxValue)
: ModelFilter(modelColumn, QMetaType::LongLong), _minValue(minValue), _maxValue(maxValue)
{
}

ModelNumberRangeFilter::~ModelNumberRangeFilter
()
{
}


bool
ModelNumberRangeFilter::accept
(const QVariant &data) const
{
    if (! data.canConvert(supportedType())) {
        return false;
    }
    bool ok = false;
    qlonglong dataValue = data.toLongLong(&ok);
    if (! ok) {
        return false;
    }
    return dataValue >= _minValue && dataValue <= _maxValue;
}


QString
ModelNumberRangeFilter::toStringSuffix
() const
{
    return QString("between %1 and %2").arg(_minValue).arg(_maxValue);
}



ModelNumberEqualFilter::ModelNumberEqualFilter
(int modelColumn, long value, long delta)
: ModelNumberRangeFilter(modelColumn, value - delta, value + delta), _value(value), _delta(delta)
{
}

ModelNumberEqualFilter::~ModelNumberEqualFilter
()
{
}


QString
ModelNumberEqualFilter::toStringSuffix() const
{
    if (_delta > 0) {
        return QString("equals %1 +- %2").arg(_value).arg(_delta);
    } else {
        return QString("equals %1").arg(_value);
    }
}


///////////////////

ModelFloatRangeFilter::ModelFloatRangeFilter
(int modelColumn, float minValue, float maxValue)
: ModelFilter(modelColumn, QMetaType::Float), _minValue(minValue), _maxValue(maxValue)
{
}

ModelFloatRangeFilter::~ModelFloatRangeFilter
()
{
}


bool
ModelFloatRangeFilter::accept
(const QVariant &data) const
{
    if (! data.canConvert(supportedType())) {
        return false;
    }
    bool ok = false;
    float dataValue = data.toFloat(&ok);
    if (! ok) {
        return false;
    }
    return dataValue >= _minValue && dataValue <= _maxValue;
}


QString
ModelFloatRangeFilter::toStringSuffix
() const
{
    return QString("between %1 and %2").arg(_minValue).arg(_maxValue);
}



ModelFloatEqualFilter::ModelFloatEqualFilter
(int modelColumn, float value, float delta)
: ModelFloatRangeFilter(modelColumn, value - delta, value + delta), _value(value), _delta(delta)
{
}

ModelFloatEqualFilter::~ModelFloatEqualFilter
()
{
}


QString
ModelFloatEqualFilter::toStringSuffix() const
{
    if (_delta > 0) {
        return QString("equals %1 +- %2").arg(_value).arg(_delta);
    } else {
        return QString("equals %1").arg(_value);
    }
}




ModelStringContainsFilter::ModelStringContainsFilter
(int modelColumn, QStringList values, bool all)
: ModelFilter(modelColumn, QMetaType::QString), _values(values), _all(all)
{
}

ModelStringContainsFilter::~ModelStringContainsFilter
()
{
}


bool
ModelStringContainsFilter::accept
(const QVariant &data) const
{
    if (! data.canConvert(supportedType())) {
        return false;
    }
    bool contains = _all;
    QString dataValue = data.toString();
    for (const auto &item : qAsConst(_values)) {
        if (_all) {
            contains &= dataValue.contains(item, Qt::CaseInsensitive);
        } else if (dataValue.contains(item, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return contains;
}


QString
ModelStringContainsFilter::toStringSuffix
() const
{
    if (_all) {
        return QString("contains all of %1").arg(_values.join(", "));
    } else {
        return QString("contains any of %1").arg(_values.join(", "));
    }
}
