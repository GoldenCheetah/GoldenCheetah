/*
* Copyright (c) 2019 Eric Christoffersen (impolexg@outlook.com)
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

#include "LocationInterpolation.h"
#include "BlinnSolver.h"

static double radianstodegrees(double r) { return r * (360.0 / (2 * M_PI)); }
static double degreestoradians(double d) { return d * (2 * M_PI / 360.0); }

xyz geolocation::toxyz() const
{
    // Approach developed by:
    //
    // (Olson, D.K. (1996).
    // "Converting earth-Centered, Earth-Fixed Coordinates to Geodetic Coordinates,"
    // IEEE Transactions on Aerospace and Electronic Systems, Vol. 32, No. 1, January 1996, pp. 473 - 476).
    //
    //
    // Java implementation:
    //
    // D. Rose (2014).
    // "Converting between Earth-Centered, Earth Fixed and Geodetic Coordinates"
    //

    //Convert Lat, Lon, Altitude to Earth-Centered-Earth-Fixed (ECEF)
    //Input is a three element array containing lat, lon (rads) and alt (m)
    //Returned array contains x, y, z in meters
    static const double  a = 6378137.0;                  //WGS-84 semi-major axis
    static const double e2 = 6.6943799901377997e-3;      //WGS-84 first eccentricity squared

    double lat = degreestoradians(Lat());
    double lon = degreestoradians(Long());
    double alt = Alt();
    double n = a / sqrt(1 - e2 * sin(lat)*sin(lat));
    double dx = ((n + alt)*cos(lat)*cos(lon));           //ECEF x
    double dy = ((n + alt)*cos(lat)*sin(lon));           //ECEF y
    double dz = ((n*(1 - e2) + alt)*sin(lat));           //ECEF z
    return(xyz(dx, dy, dz));                             //Return x, y, z in ECEF
}

geolocation xyz::togeolocation() const
{
    // Approach developed by:
    // (Olson, D.K. (1996).
    // "Converting earth-Centered, Earth-Fixed Coordinates to Geodetic Coordinates,"
    // IEEE Transactions on Aerospace and Electronic Systems, Vol. 32, No. 1, January 1996, pp. 473 - 476).
    //
    // Java implementation:
    //
    // D. Rose (2014).
    // "Converting between Earth-Centered, Earth Fixed and Geodetic Coordinates"
    //

    //Convert Earth-Centered-Earth-Fixed (ECEF) to lat, Lon, Altitude
    //Input is a three element array containing x, y, z in meters
    //Returned array contains lat and lon in radians, and altitude in meters

    static const double  a = 6378137.0;              //WGS-84 semi-major axis
    static const double e2 = 6.6943799901377997e-3;  //WGS-84 first eccentricity squared
    static const double a1 = 4.2697672707157535e+4;  //a1 = a*e2
    static const double a2 = 1.8230912546075455e+9;  //a2 = a1*a1
    static const double a3 = 1.4291722289812413e+2;  //a3 = a1*e2/2
    static const double a4 = 4.5577281365188637e+9;  //a4 = 2.5*a2
    static const double a5 = 4.2840589930055659e+4;  //a5 = a1+a3
    static const double a6 = 9.9330562000986220e-1;  //a6 = 1-e2

    double x = xyz::x();
    double y = xyz::y();
    double z = xyz::z();
    double zp = std::abs(z);
    double w2 = x * x + y * y;
    double w = sqrt(w2);
    double r2 = w2 + z * z;
    double r = sqrt(r2);

    double retLong;
    double retLat;
    double retAlt;

    retLong = atan2(y, x);    // Lon (final)
    double s2 = z * z / r2;
    double c2 = w2 / r2;
    double u = a2 / r;
    double v = a3 - a4 / r;
    double s, ss, c;
    if (c2 > 0.3) {
        s = (zp / r)*(1.0 + c2 * (a1 + u + s2 * v) / r);
        retLat = asin(s);      //Lat
        ss = s * s;
        c = sqrt(1.0 - ss);
    }
    else {
        c = (w / r)*(1.0 - s2 * (a5 - u - c2 * v) / r);
        retLat = acos(c);      //Lat
        ss = 1.0 - c * c;
        s = sqrt(ss);
    }
    double g = 1.0 - e2 * ss;
    double rg = a / sqrt(g);
    double rf = a6 * rg;
    u = w - rg * c;
    v = zp - rf * s;
    double f = c * u + s * v;
    double m = c * v - s * u;
    double p = m / (rf / g + f);
    retLat = retLat + p;            //Lat
    retAlt = f + m * p / 2.0;         //Altitude
    if (z < 0.0) {
        retLat = retLat * -1.0;     //Lat
    }

    return geolocation(radianstodegrees(retLat), radianstodegrees(retLong), retAlt);
}

xyz Slerper::Slerp(double frac)
{
    if (m_sin_angle != 0.0)
    {
        double scale = sin(frac * m_angle) / m_sin_angle;
        return m_x0_norm.scale(sin((1 - frac) * m_angle) / m_sin_angle).add(m_x1_norm.scale(scale));
    }
    return m_x0_norm;
}

// Precompute invariant values needed to geoslerp
Slerper::Slerper(geolocation g0, geolocation g1) :
    m_g0(g0),
    m_g1(g1),
    m_x0(m_g0.toxyz()),
    m_x1(m_g1.toxyz()),
    m_x0_norm(m_x0.normalize()),
    m_x1_norm(m_x1.normalize()),
    m_x0_magnitude(m_x0.magnitude())
{
    m_angle = atan2(m_x0_norm.cross(m_x1_norm).magnitude(), m_x0_norm.dot(m_x1_norm));
    m_sin_angle = sin(m_angle);
    m_altitude_delta = (m_g1.Alt() - m_g0.Alt());
}

// Interpolate fractional arc between two wgs84 vectors.
// This models the earths ellipsoid.
geolocation Slerper::GeoSlerp(double frac)
{
    // Generate normalized ECEF vector
    xyz slerp = Slerp(frac);

    // Scale ecef unit vector so its roughly at earth surface
    double interpaltitudestep = m_altitude_delta * frac;
    slerp = slerp.scale(m_x0_magnitude + interpaltitudestep);

    // Convert ecef back to wgs84
    geolocation slerpgeo = slerp.togeolocation();

    // Set linear interpolated altitude on slerped wgs84 vector
    double interpaltitude = m_altitude_delta * frac;
    slerpgeo.Alt() = m_g0.Alt() + interpaltitude;

    return slerpgeo;
}

xyz LinearTwoPointInterpolator::InterpolateNext(xyz p0, xyz p1)
{
    xyz delta = p1.subtract(p0);
    xyz retval = p1.add(delta);
    return retval;
}

xyz SphericalTwoPointInterpolator::InterpolateNext(xyz p0, xyz p1)
{
    Slerper slerper(p0.togeolocation(), p1.togeolocation());

    geolocation geo = slerper.GeoSlerp(2.0);

    return geo.toxyz();
}

void UnitCatmullRomInterpolator::Init(double pm1, double p0, double p1, double p2)
{
    std::get<0>(m_p) = pm1;
    std::get<1>(m_p) = p0;
    std::get<2>(m_p) = p1;
    std::get<3>(m_p) = p2;
}

UnitCatmullRomInterpolator::UnitCatmullRomInterpolator()
{
    Init(0.0, 0.0, 0.0, 0.0);
}

UnitCatmullRomInterpolator::UnitCatmullRomInterpolator(double pm1, double p0, double p1, double p2)
{
    Init(pm1, p0, p1, p2);
}

double UnitCatmullRomInterpolator::T()
{
    static const double s_T = 0.5; // Control curvature: 0 is linear, 1 is pretty loopy, most people like 0.5.

    return s_T;
}

double UnitCatmullRomInterpolator::Location(double u)
{
    static const double s_T = T();

    double p0 = std::get<0>(m_p);
    double p1 = std::get<1>(m_p);
    double p2 = std::get<2>(m_p);
    double p3 = std::get<3>(m_p);

    // Curvature-parameterized CatmullRom equation courtesy of wolfram alpha:
    // [1, u, u ^ 2, u ^ 3] * [[0, 1, 0, 0], [-t, 0, t, 0], [2 * t, t - 3, 3 - 2t, -t], [-t, 2 - t, t - 2, t]] * [p0, p1, p2, p3]

    double retval = p0 * u * (u * (2 * s_T - s_T * u) - s_T) +
        u * (u * (u * (p1 * (-s_T) + 2 * p1 + p2 * (s_T - 2) + p3 * s_T) + p1 * s_T - 3 * p1 + p2 * (3 - 2 * s_T) - p3 * s_T) + p2 * s_T) + p1;

    return retval;
}

double UnitCatmullRomInterpolator::Tangent(double u)
{
    static const double s_T = T();

    double p0 = std::get<0>(m_p);
    double p1 = std::get<1>(m_p);
    double p2 = std::get<2>(m_p);
    double p3 = std::get<3>(m_p);

    // Curvature-parameterized CatmullRom equation courtesy of wolfram alpha:
    // [1, u, u ^ 2, u ^ 3] * [[0, 1, 0, 0], [-t, 0, t, 0], [2 * t, t - 3, 3 - 2t, -t], [-t, 2 - t, t - 2, t]] * [p0, p1, p2, p3]

    // dx/du

    // Closed form slope for cubic at point u in 0..1.
    double retval = p0 * u * (2 * s_T - 2 * s_T * u)
        + p0 * (u * (2 * s_T - s_T * u) - s_T)
        + u * (u * (p1 * (-s_T) + 2 * p1 + p2 * (s_T - 2) + p3 * s_T) + p1 * s_T - 3 * p1 + p2 * (3 - 2 * s_T) - p3 * s_T)
        + u * (2 * u * (p1 * (-s_T) + 2 * p1 + p2 * (s_T - 2) + p3 * s_T) + p1 * s_T - 3 * p1 + p2 * (3 - 2 * s_T) - p3 * s_T)
        + p2 * s_T;

    return retval;
}

// Given interpolated value v, provides u that would yield v.
// Only successful if spline is invertable (which is true for
// distance spline.)
bool UnitCatmullRomInterpolator::Inverse(double r, double &u)
{
    const double t = T();

    double p0 = std::get<0>(m_p);
    double p1 = std::get<1>(m_p);
    double p2 = std::get<2>(m_p);
    double p3 = std::get<3>(m_p);

    // Normalized form of equation from Location.
    double a = (p3*t - p0 * t + p2 * (t - 2) + p1 * (2 - t));
    double b = (-p3 * t + 2 * p0*t + p1 * (t - 3) + p2 * (3 - 2 * t));
    double c = (p2*t - p0 * t);
    double d = p1 - r; // "- r" because root finder expects form that equals zero.

    Roots roots = BlinnCubicSolver(a, b, c, d);

    // There are 0, 1, 2 or 3 roots.
    if (roots.resultcount() < 1) return false;

    double r0 = roots.result(0).x / roots.result(0).w;
    double r1 = r0;
    double r2 = r0;

    if (roots.resultcount() > 1)
        r1 = roots.result(1).x / roots.result(1).w;

    if (roots.resultcount() > 2)
        r2 = roots.result(2).x / roots.result(2).w;

    // We have success if there is a root in the range [0..1].

    // TODO: For now we return a single root.
    // It is certainly possible that there are multiple roots in range... give caller a choice?
    bool ret = false;
    if      (r0 >= 0. && r0 <= 1.) { u = r0; ret = true; }
    else if (r1 >= 0. && r1 <= 1.) { u = r1; ret = true; }
    else if (r2 >= 0. && r2 <= 1.) { u = r2; ret = true; }

    return ret;
}

void UnitCatmullRomInterpolator3D::Init(xyz pm1, xyz p0, xyz p1, xyz p2)
{
    x.Init(pm1.x(), p0.x(), p1.x(), p2.x());
    y.Init(pm1.y(), p0.y(), p1.y(), p2.y());
    z.Init(pm1.z(), p0.z(), p1.z(), p2.z());
}

UnitCatmullRomInterpolator3D::UnitCatmullRomInterpolator3D(xyz pm1, xyz p0, xyz p1, xyz p2)
{
    Init(pm1, p0, p1, p2);
}

xyz UnitCatmullRomInterpolator3D::Location(double frac)
{
    return xyz(x.Location(frac), y.Location(frac), z.Location(frac));
}

xyz UnitCatmullRomInterpolator3D::Tangent(double frac)
{
    return xyz(x.Tangent(frac), y.Tangent(frac), z.Tangent(frac));
}

geolocation GeoPointInterpolator::Location(double distance)
{
    xyz l0xyz = DistancePointInterpolator<SphericalTwoPointInterpolator>::Location(distance);
    geolocation l0 = l0xyz.togeolocation();
    return l0;
}

geolocation GeoPointInterpolator::Location(double distance, double &slope)
{
    xyz tangentVector;
    xyz l0xyz = DistancePointInterpolator<SphericalTwoPointInterpolator>::Location(distance, tangentVector);

    // Compute earth gradient from location and tangent.
    geolocation l1 = xyz(l0xyz.add(tangentVector.normalize())).togeolocation();
    geolocation l0 = l0xyz.togeolocation();

    double a0 = l0.Alt();
    double a1 = l1.Alt();
    double rise = a1 - a0;
    double run = sqrt(1 - fabs(rise)); // length of adjacent

    slope = run ? rise / run : 0;

    return l0;
}

void GeoPointInterpolator::Push(double distance, geolocation point)
{
    DistancePointInterpolator<SphericalTwoPointInterpolator>::Push(distance, point.toxyz());
}
