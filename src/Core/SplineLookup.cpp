/*
 * Copyright (C) 1997 Josef Wilgen
 * Copyright (C) 2002 Uwe Rathmann
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

#include "SplineLookup.h"


void
SplineLookup::update
(const QPolygonF &polygon)
{
    splinePolygon = polygon;
    splinePolynomials.clear();
}


void
SplineLookup::update
(const QwtSplineBasis &spline, const QPolygonF &polygon, double tolerance)
{
    splinePolygon = spline.polygon(polygon, tolerance);
    splinePolynomials.clear();
}


void
SplineLookup::update
(const QwtSplineCubic &spline, const QPolygonF &polygon, double tolerance)
{
    splinePolygon = spline.polygon(polygon, tolerance);
    splinePolynomials = spline.polynomials(splinePolygon);
}


double
SplineLookup::valueY
(double x) const
{
    if (splinePolygon.size() == 0) return x;
    const int i = lookupIndex(x);
    if (splinePolynomials.size() > 0) {
        const double deltaX = x - splinePolygon[i].x();
        return ((splinePolynomials[i].c3 * deltaX + splinePolynomials[i].c2) * deltaX + splinePolynomials[i].c1) * deltaX + splinePolygon[i].y();
    } else {
        return splinePolygon[i].y();
    }
}


int
SplineLookup::size
() const
{
    return splinePolygon.size();
}


double
SplineLookup::xMin
() const
{
    return splinePolygon[0].x();
}


double
SplineLookup::xMax
() const
{
    return splinePolygon[splinePolygon.size() - 1].x();
}


int
SplineLookup::lookupIndex
(double x) const
{
    int idx;
    const int size = splinePolygon.size();

    if (x <= splinePolygon[0].x()) {
        idx = 0;
    } else if (x >= splinePolygon[size - 2].x()) {
        idx = size - 2;
    } else {
        idx = 0;
        int l = 0;
        int r = size - 2;

        while (r - idx > 1) {
            l = idx + ((r - idx) >> 1);
            if (splinePolygon[l].x() > x) {
                r = l;
            } else {
                idx = l;
            }
        }
    }
    return idx;
}
