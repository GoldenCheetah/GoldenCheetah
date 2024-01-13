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

#ifndef _GC_LogTimeScaleEngine_h
#define _GC_LogTimeScaleEngine_h 1
#include "GoldenCheetah.h"

#include "qwt_scale_engine.h"
#include "qwt_point_3d.h"
#include "qwt_compat.h"

class LogTimeScaleEngine : public QwtScaleEngine
{
    public:
        virtual void autoScale(int maxSteps, double &x1, double &x2,
                               double &stepSize) const;

        virtual QwtScaleDiv divideScale(double x1, double x2,
                                        int numMajorSteps, int numMinorSteps,
                                        double stepSize = 0.0) const;

        virtual QwtTransform *transformation() const;

    protected:
        QwtDoubleInterval log10(const QwtDoubleInterval&) const;
        QwtDoubleInterval pow10(const QwtDoubleInterval&) const;

    private:
        QwtDoubleInterval align(const QwtDoubleInterval&,
                                double stepSize) const;

        void buildTicks(
            const QwtDoubleInterval &, double stepSize, int maxMinSteps,
            QList<double> ticks[QwtScaleDiv::NTickTypes]) const;

        QList<double> buildMinorTicks(
            const QList<double>& majorTicks,
            int maxMinMark, double step) const;

        QList<double> buildMajorTicks(
            const QwtDoubleInterval &interval, double stepSize) const;
};

#endif // _GC_LogTimeScaleEngine_h

