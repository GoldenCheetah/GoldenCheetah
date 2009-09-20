/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_INTERVAL_DATA_H
#define QWT_INTERVAL_DATA_H 1

#include "qwt_global.h"
#include "qwt_math.h"
#include "qwt_array.h"
#include "qwt_double_interval.h"
#include "qwt_double_rect.h"

#if defined(_MSC_VER) && (_MSC_VER > 1310)
#include <string.h>
#endif

#if defined(QWT_TEMPLATEDLL)
// MOC_SKIP_BEGIN
template class QWT_EXPORT QwtArray<QwtDoubleInterval>;
template class QWT_EXPORT QwtArray<double>;
// MOC_SKIP_END
#endif

/*!
  \brief Series of samples of a value and an interval

  QwtIntervalData is a series of samples of a value and an interval.
  F.e. error bars are built from samples [x, y1-y2], while a 
  histogram might consist of [x1-x2, y] samples.
*/
class QWT_EXPORT QwtIntervalData
{
public:
    QwtIntervalData();
    QwtIntervalData(const QwtArray<QwtDoubleInterval> &, 
        const QwtArray<double> &);

    ~QwtIntervalData();
    
    void setData(const QwtArray<QwtDoubleInterval> &, 
        const QwtArray<double> &);

    size_t size() const;
    const QwtDoubleInterval &interval(size_t i) const;
    double value(size_t i) const;

    QwtDoubleRect boundingRect() const;

private:
    QwtArray<QwtDoubleInterval> d_intervals;
    QwtArray<double> d_values;
};

//! \return Number of samples
inline size_t QwtIntervalData::size() const
{
    return qwtMin(d_intervals.size(), d_values.size());
}

/*! 
  Interval of a sample

  \param i Sample index
  \return Interval
  \sa value(), size()
*/
inline const QwtDoubleInterval &QwtIntervalData::interval(size_t i) const
{
    return d_intervals[int(i)];
}

/*! 
  Value of a sample

  \param i Sample index
  \return Value
  \sa interval(), size()
*/
inline double QwtIntervalData::value(size_t i) const
{
    return d_values[int(i)];
}

#endif 
