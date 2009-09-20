/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_SCALE_DIV_H
#define QWT_SCALE_DIV_H

#include "qwt_global.h"
#include "qwt_valuelist.h"
#include "qwt_double_interval.h"

class QwtDoubleInterval;

/*!
  \brief A class representing a scale division

  A scale division consists of its limits and 3 list
  of tick values qualified as major, medium and minor ticks.

  In most cases scale divisions are calculated by a QwtScaleEngine.

  \sa subDivideInto(), subDivide()
*/

class QWT_EXPORT QwtScaleDiv
{
public:
    //! Scale tick types
    enum TickType
    {
        NoTick = -1,

        MinorTick,
        MediumTick,
        MajorTick,

        NTickTypes
    };

    explicit QwtScaleDiv();
    explicit QwtScaleDiv(const QwtDoubleInterval &,
        QwtValueList[NTickTypes]);
    explicit QwtScaleDiv(double lowerBound, double upperBound,
        QwtValueList[NTickTypes]);

    int operator==(const QwtScaleDiv &s) const;
    int operator!=(const QwtScaleDiv &s) const;
    
    void setInterval(double lowerBound, double upperBound);
    void setInterval(const QwtDoubleInterval &);
    QwtDoubleInterval interval() const;

    double lowerBound() const;
    double upperBound() const;
    double range() const;

    bool contains(double v) const;

    void setTicks(int type, const QwtValueList &);
    const QwtValueList &ticks(int type) const;

    void invalidate();
    bool isValid() const;
 
    void invert();

private:
    double d_lowerBound;
    double d_upperBound;
    QwtValueList d_ticks[NTickTypes];

    bool d_isValid;
};

/*!
   Change the interval
   \param lowerBound lower bound
   \param upperBound upper bound
*/
inline void QwtScaleDiv::setInterval(double lowerBound, double upperBound)
{
    d_lowerBound = lowerBound;
    d_upperBound = upperBound;
}

/*! 
  \return lowerBound -> upperBound
*/
inline QwtDoubleInterval QwtScaleDiv::interval() const
{
    return QwtDoubleInterval(d_lowerBound, d_upperBound);
}

/*! 
  \return lower bound
  \sa upperBound()
*/
inline double QwtScaleDiv::lowerBound() const 
{ 
    return d_lowerBound;
}

/*! 
  \return upper bound
  \sa lowerBound()
*/
inline double QwtScaleDiv::upperBound() const 
{ 
    return d_upperBound;
}

/*! 
  \return upperBound() - lowerBound()
*/
inline double QwtScaleDiv::range() const 
{ 
    return d_upperBound - d_lowerBound;
}
#endif
