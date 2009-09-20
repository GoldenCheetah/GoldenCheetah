/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qglobal.h>
#if QT_VERSION >= 0x040000
#include <qalgorithms.h>
#else
#include <qtl.h>
#endif

#include "qwt_math.h"
#include "qwt_double_interval.h"

/*!
   \brief Normalize the limits of the interval

   If maxValue() < minValue() the limits will be inverted.
   \return Normalized interval

   \sa isValid(), inverted()
*/
QwtDoubleInterval QwtDoubleInterval::normalized() const
{
    if ( d_minValue > d_maxValue )
    {
        return inverted();
    }
    if ( d_minValue == d_maxValue && d_borderFlags == ExcludeMinimum )
    {
        return inverted();
    }

    return *this;
}

/*!
   Invert the limits of the interval
   \return Inverted interval
   \sa normalized()
*/
QwtDoubleInterval QwtDoubleInterval::inverted() const
{
    int borderFlags = 0;
    if ( d_borderFlags & ExcludeMinimum )
        borderFlags |= ExcludeMaximum;
    if ( d_borderFlags & ExcludeMaximum )
        borderFlags |= ExcludeMinimum;

    return QwtDoubleInterval(d_maxValue, d_minValue, borderFlags);
}

/*!
  Test if a value is inside an interval

  \param value Value
  \return true, if value >= minValue() && value <= maxValue()
*/
bool QwtDoubleInterval::contains(double value) const
{
    if ( !isValid() )
        return false;

    if ( value < d_minValue || value > d_maxValue )
        return false;

    if ( value == d_minValue && d_borderFlags & ExcludeMinimum )
        return false;

    if ( value == d_maxValue && d_borderFlags & ExcludeMaximum )
        return false;

    return true;
}

//! Unite 2 intervals
QwtDoubleInterval QwtDoubleInterval::unite(
    const QwtDoubleInterval &other) const
{
    /*
     If one of the intervals is invalid return the other one.
     If both are invalid return an invalid default interval
     */
    if ( !isValid() )
    {
        if ( !other.isValid() )
            return QwtDoubleInterval();
        else
            return other;
    }
    if ( !other.isValid() )
        return *this;

    QwtDoubleInterval united;
    int flags = 0;

    // minimum
    if ( d_minValue < other.minValue() )
    {
        united.setMinValue(d_minValue);
        flags &= d_borderFlags & ExcludeMinimum;
    }
    else if ( other.minValue() < d_minValue )
    {
        united.setMinValue(other.minValue());
        flags &= other.borderFlags() & ExcludeMinimum;
    }
    else // d_minValue == other.minValue()
    {
        united.setMinValue(d_minValue);
        flags &= (d_borderFlags & other.borderFlags()) & ExcludeMinimum;
    }

    // maximum
    if ( d_maxValue > other.maxValue() )
    {
        united.setMaxValue(d_maxValue);
        flags &= d_borderFlags & ExcludeMaximum;
    }
    else if ( other.maxValue() > d_maxValue )
    {
        united.setMaxValue(other.maxValue());
        flags &= other.borderFlags() & ExcludeMaximum;
    }
    else // d_maxValue == other.maxValue() )
    {
        united.setMaxValue(d_maxValue);
        flags &= d_borderFlags & other.borderFlags() & ExcludeMaximum;
    }

    united.setBorderFlags(flags);
    return united;
}

//! Intersect 2 intervals
QwtDoubleInterval QwtDoubleInterval::intersect(
    const QwtDoubleInterval &other) const
{
    if ( !other.isValid() || !isValid() )
        return QwtDoubleInterval();

    QwtDoubleInterval i1 = *this;
    QwtDoubleInterval i2 = other;

    // swap i1/i2, so that the minimum of i1
    // is smaller then the minimum of i2

    if ( i1.minValue() > i2.minValue() ) 
    {
        qSwap(i1, i2);
    }
    else if ( i1.minValue() == i2.minValue() )
    {
        if ( i1.borderFlags() & ExcludeMinimum )
            qSwap(i1, i2);
    }

    if ( i1.maxValue() < i2.minValue() )
    {
        return QwtDoubleInterval();
    }

    if ( i1.maxValue() == i2.minValue() )
    {
        if ( i1.borderFlags() & ExcludeMaximum ||
            i2.borderFlags() & ExcludeMinimum )
        {
            return QwtDoubleInterval();
        }
    }

    QwtDoubleInterval intersected;
    int flags = 0;

    intersected.setMinValue(i2.minValue());
    flags |= i2.borderFlags() & ExcludeMinimum;

    if ( i1.maxValue() < i2.maxValue() )
    {
        intersected.setMaxValue(i1.maxValue());
        flags |= i1.borderFlags() & ExcludeMaximum;
    }
    else if ( i2.maxValue() < i1.maxValue() )
    {
        intersected.setMaxValue(i2.maxValue());
        flags |= i2.borderFlags() & ExcludeMaximum;
    }
    else // i1.maxValue() == i2.maxValue()
    {
        intersected.setMaxValue(i1.maxValue() );
        flags |= i1.borderFlags() & i2.borderFlags() & ExcludeMaximum;
    }

    intersected.setBorderFlags(flags);
    return intersected;
}

//! Unites this interval with the given interval.
QwtDoubleInterval& QwtDoubleInterval::operator|=(
    const QwtDoubleInterval &interval)
{
    *this = *this | interval;
    return *this;
}

//! Intersects this interval with the given interval.
QwtDoubleInterval& QwtDoubleInterval::operator&=(
    const QwtDoubleInterval &interval) 
{
    *this = *this & interval;
    return *this;
}

/*!
   Test if two intervals overlap
*/
bool QwtDoubleInterval::intersects(const QwtDoubleInterval &other) const
{
    if ( !isValid() || !other.isValid() )
        return false;

    QwtDoubleInterval i1 = *this;
    QwtDoubleInterval i2 = other;

    // swap i1/i2, so that the minimum of i1
    // is smaller then the minimum of i2

    if ( i1.minValue() > i2.minValue() ) 
    {
        qSwap(i1, i2);
    }
    else if ( i1.minValue() == i2.minValue() &&
            i1.borderFlags() & ExcludeMinimum )
    {
        qSwap(i1, i2);
    }

    if ( i1.maxValue() > i2.minValue() )
    {
        return true;
    }
    if ( i1.maxValue() == i2.minValue() )
    {
        return !( (i1.borderFlags() & ExcludeMaximum) || 
            (i2.borderFlags() & ExcludeMinimum) );
    }
    return false;
}

/*!
   Adjust the limit that is closer to value, so that value becomes
   the center of the interval.

   \param value Center
   \return Interval with value as center
*/
QwtDoubleInterval QwtDoubleInterval::symmetrize(double value) const
{
    if ( !isValid() )
        return *this;

    const double delta =
        qwtMax(qwtAbs(value - d_maxValue), qwtAbs(value - d_minValue));

    return QwtDoubleInterval(value - delta, value + delta);
}

/*!
   Limit the interval, keeping the border modes

   \param lowerBound Lower limit
   \param upperBound Upper limit

   \return Limited interval
*/
QwtDoubleInterval QwtDoubleInterval::limited(
    double lowerBound, double upperBound) const
{
    if ( !isValid() || lowerBound > upperBound )
        return QwtDoubleInterval();

    double minValue = qwtMax(d_minValue, lowerBound);
    minValue = qwtMin(minValue, upperBound);

    double maxValue = qwtMax(d_maxValue, lowerBound);
    maxValue = qwtMin(maxValue, upperBound);

    return QwtDoubleInterval(minValue, maxValue, d_borderFlags);
}

/*!
   Extend the interval

   If value is below minValue, value becomes the lower limit.
   If value is above maxValue, value becomes the upper limit.

   extend has no effect for invalid intervals

   \param value Value
   \sa isValid()
*/
QwtDoubleInterval QwtDoubleInterval::extend(double value) const
{
    if ( !isValid() )
        return *this;

    return QwtDoubleInterval( qwtMin(value, d_minValue), 
        qwtMax(value, d_maxValue), d_borderFlags );
}

QwtDoubleInterval& QwtDoubleInterval::operator|=(double value)
{
    *this = *this | value;
    return *this;
}
