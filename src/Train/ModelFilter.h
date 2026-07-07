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

#ifndef _GC_ModelFilter_h
#define _GC_ModelFilter_h

#include <cfloat>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QMetaType>


class ModelFilter {
public:
    ModelFilter(int modelColumn, QMetaType::Type type);
    virtual ~ModelFilter();

    int modelColumn() const;
    QMetaType::Type supportedType() const;
    QString toString() const;
    virtual bool accept(const QVariant &data) const = 0;

protected:
    virtual QString toStringSuffix() const = 0;

private:
    int _modelColumn;
    QMetaType::Type _type;

    QString toStringPrefix() const;
};


class ModelNumberRangeFilter: public ModelFilter {
public:
    ModelNumberRangeFilter(int modelColumn, long minValue, long maxValue = LONG_MAX);
    virtual ~ModelNumberRangeFilter();

    virtual bool accept(const QVariant &data) const;

protected:
    virtual QString toStringSuffix() const;

private:
    long _minValue;
    long _maxValue;
};


class ModelNumberEqualFilter: public ModelNumberRangeFilter {
public:
    ModelNumberEqualFilter(int modelColumn, long value, long delta = 0);
    virtual ~ModelNumberEqualFilter();

protected:
    virtual QString toStringSuffix() const;

private:
    long _value;
    long _delta;
};


class ModelFloatRangeFilter: public ModelFilter {
public:
    ModelFloatRangeFilter(int modelColumn, float minValue, float maxValue = FLT_MAX);
    virtual ~ModelFloatRangeFilter();

    virtual bool accept(const QVariant &data) const;

protected:
    virtual QString toStringSuffix() const;

private:
    float _minValue;
    float _maxValue;
};


class ModelFloatEqualFilter: public ModelFloatRangeFilter {
public:
    ModelFloatEqualFilter(int modelColumn, float value, float delta = 0);
    virtual ~ModelFloatEqualFilter();

protected:
    virtual QString toStringSuffix() const;

private:
    float _value;
    float _delta;
};


class ModelStringContainsFilter: public ModelFilter {
public:
    ModelStringContainsFilter(int modelColumn, QStringList values, bool all = false);
    virtual ~ModelStringContainsFilter();

    virtual bool accept(const QVariant &data) const;

protected:
    virtual QString toStringSuffix() const;

private:
    QStringList _values;
    bool _all;
};


#endif
