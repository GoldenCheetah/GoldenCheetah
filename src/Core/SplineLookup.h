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

#ifndef _GC_SplineLookup_h
#define _GC_SplineLookup_h 1

#include "qwt_spline_basis.h"
#include <qwt_spline_cubic.h>
#include <qwt_spline_polynomial.h>
#include <QPolygonF>
#include <QVector>


class SplineLookup {
public:
    void update(const QPolygonF &polygon);
    void update(const QwtSplineBasis &spline, const QPolygonF &polygon, double tolerance);
    void update(const QwtSplineCubic &spline, const QPolygonF &polygon, double tolerance);

    double valueY(double x) const;
    int size() const;
    double xMin() const;
    double xMax() const;

private:
    int lookupIndex(double x) const;

    QPolygonF splinePolygon;
    QVector<QwtSplinePolynomial> splinePolynomials;
};

#endif
