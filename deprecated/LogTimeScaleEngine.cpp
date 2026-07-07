/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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
 *
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 */

#include "LogTimeScaleEngine.h"

#include "qwt_math.h"
#include "qwt_scale_map.h"

/*!
  Return a transformation, for logarithmic (base 10) scales
*/
QwtTransform *LogTimeScaleEngine::transformation() const
{
    return new QwtLogTransform;
    //, log10XForm, QwtScaleTransformation::log10InvXForm);
}

/*!
    Align and divide an interval

   \param maxNumSteps Max. number of steps
   \param x1 First limit of the interval (In/Out)
   \param x2 Second limit of the interval (In/Out)
   \param stepSize Step size (Out)

   \sa QwtScaleEngine::setAttribute
*/
void LogTimeScaleEngine::autoScale(int maxNumSteps,
    double &x1, double &x2, double &stepSize) const
{
    if ( x1 > x2 )
        qSwap(x1, x2);

    QwtDoubleInterval interval(

     #if (QWT_VERSION >= 0x050200)
            x1 / pow(10.0, lowerMargin()),
            x2 * pow(10.0, upperMargin())
     #else
            x1 / pow(10.0, loMargin()),
            x2 * pow(10.0, hiMargin())
    #endif
    );

    double logRef = 1.0;
    if (reference() > LOG_MIN / 2)
        logRef = qwtMin(reference(), LOG_MAX / 2);

    if (testAttribute(QwtScaleEngine::Symmetric))
    {
        const double delta = qwtMax(interval.maxValue() / logRef,
            logRef / interval.minValue());
        interval.setInterval(logRef / delta, logRef * delta);
    }

    if (testAttribute(QwtScaleEngine::IncludeReference))
        interval = interval.extend(logRef);

    interval = interval.limited(LOG_MIN, LOG_MAX);

    if (interval.width() == 0.0)
        interval = buildInterval(interval.minValue());

    stepSize = divideInterval(log10(interval).width(), qwtMax(maxNumSteps, 1));
    if ( stepSize < 1.0 )
        stepSize = 1.0;

    if (!testAttribute(QwtScaleEngine::Floating))
        interval = align(interval, stepSize);

    x1 = interval.minValue();
    x2 = interval.maxValue();

    if (testAttribute(QwtScaleEngine::Inverted))
    {
        qSwap(x1, x2);
        stepSize = -stepSize;
    }
}

/*!
   \brief Calculate a scale division

   \param x1 First interval limit
   \param x2 Second interval limit
   \param maxMajSteps Maximum for the number of major steps
   \param maxMinSteps Maximum number of minor steps
   \param stepSize Step size. If stepSize == 0, the scaleEngine
                   calculates one.

   \sa QwtScaleEngine::stepSize, LogTimeScaleEngine::subDivide
*/
QwtScaleDiv LogTimeScaleEngine::divideScale(double x1, double x2,
    int maxMajSteps, int maxMinSteps, double stepSize) const
{
    QwtDoubleInterval interval = QwtDoubleInterval(x1, x2).normalized();
    interval = interval.limited(LOG_MIN, LOG_MAX);

    if (interval.width() <= 0 )
        return QwtScaleDiv();

    if (interval.maxValue() / interval.minValue() < 10.0)
    {
        // scale width is less than one decade -> build linear scale

        QwtLinearScaleEngine linearScaler;
        linearScaler.setAttributes(attributes());
        linearScaler.setReference(reference());
        linearScaler.setMargins(
                                #if (QWT_VERSION >= 0x050200)
				lowerMargin(), upperMargin()
				#else
				loMargin(), hiMargin()
				#endif
				);

        return linearScaler.divideScale(x1, x2,
            maxMajSteps, maxMinSteps, stepSize);
    }

    stepSize = qwtAbs(stepSize);
    if ( stepSize == 0.0 )
    {
        if ( maxMajSteps < 1 )
            maxMajSteps = 1;

        stepSize = divideInterval(log10(interval).width(), maxMajSteps);
        if ( stepSize < 1.0 )
            stepSize = 1.0; // major step must be >= 1 decade
    }

    QwtScaleDiv scaleDiv;
    if ( stepSize != 0.0 )
    {
        QList<double> ticks[QwtScaleDiv::NTickTypes];
        buildTicks(interval, stepSize, maxMinSteps, ticks);

        scaleDiv = QwtScaleDiv(interval, ticks);
    }

    if ( x1 > x2 )
        scaleDiv.invert();

    return scaleDiv;
}

void LogTimeScaleEngine::buildTicks(
    const QwtDoubleInterval& interval, double stepSize, int maxMinSteps,
    QList<double> ticks[QwtScaleDiv::NTickTypes]) const
{
    const QwtDoubleInterval boundingInterval =
        align(interval, stepSize);

    ticks[QwtScaleDiv::MajorTick] =
        buildMajorTicks(boundingInterval, stepSize);

    if ( maxMinSteps > 0 )
    {
        ticks[QwtScaleDiv::MinorTick] = buildMinorTicks(
            ticks[QwtScaleDiv::MajorTick], maxMinSteps, stepSize);
    }

    for ( int i = 0; i < QwtScaleDiv::NTickTypes; i++ )
        ticks[i] = strip(ticks[i], interval);
}

struct tick_info_t {
    double x;
    const char *label;
};

tick_info_t tick_info[] = {
    {  1.0/60.0,    "1s" },
    {  5.0/60.0,    "5s" },
    { 15.0/60.0,   "15s" },
    {       0.5,   "30s" },
    {       1.0,    "1m" },
    {       2.0,    "2m" },
    {       3.0,    "3m" },
    {       5.0,    "5m" },
    {      10.0,   "10m" },
    {      20.0,   "20m" },
    {      30.0,   "30m" },
    {      60.0,    "1h" },
    {     120.0,    "2h" },
    {     180.0,    "3h" },
    {     300.0,    "5h" },
    {      -1.0,    NULL }
};

QList<double> LogTimeScaleEngine::buildMajorTicks(
    const QwtDoubleInterval &interval, double stepSize) const
{
    (void) interval;
    (void) stepSize;
    QList<double> ticks;
    tick_info_t *walker = tick_info;
    while (walker->label) {
        ticks += walker->x;
        ++walker;
    }
    return ticks;
}

QList<double> LogTimeScaleEngine::buildMinorTicks(
    const QList<double> &majorTicks,
    int maxMinSteps, double stepSize) const
{
    (void) majorTicks;
    (void) maxMinSteps;
    (void) stepSize;
    QList<double> minorTicks;
    return minorTicks;
}

/*!
  \brief Align an interval to a step size

  The limits of an interval are aligned that both are integer
  multiples of the step size.

  \param interval Interval
  \param stepSize Step size

  \return Aligned interval
*/
QwtDoubleInterval LogTimeScaleEngine::align(
    const QwtDoubleInterval &interval, double stepSize) const
{
    const QwtDoubleInterval intv = log10(interval);

    const double x1 = QwtScaleArithmetic::floorEps(intv.minValue(), stepSize);
    const double x2 = QwtScaleArithmetic::ceilEps(intv.maxValue(), stepSize);

    return pow10(QwtDoubleInterval(x1, x2));
}

/*!
  Return the interval [log10(interval.minValue(), log10(interval.maxValue]
*/

QwtDoubleInterval LogTimeScaleEngine::log10(
    const QwtDoubleInterval &interval) const
{
    return QwtDoubleInterval(::log10(interval.minValue()),
            ::log10(interval.maxValue()));
}

/*!
  Return the interval [pow10(interval.minValue(), pow10(interval.maxValue]
*/
QwtDoubleInterval LogTimeScaleEngine::pow10(
    const QwtDoubleInterval &interval) const
{
    return QwtDoubleInterval(pow(10.0, interval.minValue()),
            pow(10.0, interval.maxValue()));
}
