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

#include <tuple>
#include <bitset>
#include "qwt_math.h"

struct geolocation;

class v3
{
    std::tuple<double, double, double> m_t;

public:

    v3(double a, double b, double c) : m_t(a, b, c) {};

    double  x() const { return std::get<0>(m_t); }
    double  y() const { return std::get<1>(m_t); }
    double  z() const { return std::get<2>(m_t); }

    double& x()       { return std::get<0>(m_t); }
    double& y()       { return std::get<1>(m_t); }
    double& z()       { return std::get<2>(m_t); }

    v3 add(const v3 &a)      const { return v3(x() + a.x(), y() + a.y(), z() + a.z()); }
    v3 subtract(const v3 &a) const { return v3(x() - a.x(), y() - a.y(), z() - a.z()); }
    v3 scale(double d)       const { return v3(x() * d,     y() * d,     z() * d); }

    double magnitude()       const { return sqrt(dot(*this)); }

    v3 normalize() const
    {
        double mag = magnitude();

        if (mag == 0) {
            // assert?
            mag = 1.0;
        }

        return scale(1.0 / mag);
    }

    double dot(const v3 &a) const { return x() * a.x() + y() * a.y() + z() * a.z(); }
    v3 cross(const v3 &a) const
    {
        return v3(
            y()*a.z() - z()*a.y(),
            z()*a.x() - x()*a.z(),
            x()*a.y() - y()*a.x());
    }
};

struct xyz : public v3
{
    xyz(double x, double y, double z) : v3(x, y, z) {}
    xyz(const v3 &a) : v3(a) {}
    xyz() : v3(0.0, 0.0, 0.0) {}

    //void Print(){
    //    qDebug() << "< X:" << x() << " y:" << y() << " z:" << z() << " >";
    //}

    geolocation togeolocation();
};

struct geolocation : v3 
{
    double Lat()  const { return x(); }
    double Long() const { return y(); }
    double Alt()  const { return z(); }

    double &Lat() { return x(); }
    double &Long() { return y(); }
    double &Alt() { return z(); }

    geolocation(double la, double lo, double al) : v3(la, lo, al) {}
    geolocation(const v3 &a) : v3(a) { }
    geolocation() :v3(0, 0, 0) {}

    //void Print() {
    //    qDebug() << "< Lat:" << Lat() << " Long:" << Long() << " Alt:" << Alt() << " >";
    //}

    xyz toxyz();
};

// Class to wrap classic game spherical interpolation
class Slerper
{
    geolocation m_g0;             // start location (wgs84)
    geolocation m_g1;             // end location   (wgs84)
    xyz         m_x0;             // start location (ecef)
    xyz         m_x1;             // end location   (ecef)
    xyz         m_x0_norm;        // start location unit vector
    xyz         m_x1_norm;        // end location unit vector
    double      m_x0_magnitude;   // magnitude of ecef start location vector
    double      m_angle;          // angle between start and end locations
    double      m_sin_angle;      // sin of angle betweeen start and end locations
    double      m_altitude_delta; // delta between start and end altitudes

                                  // Spherical interpolate using normalized start and end locations, returns normalized interpolated
                                  // unit vector that lies 'frac' from the two vectors.
    xyz Slerp(double frac);

public:

    // Precompute invariant values needed to geoslerp
    Slerper(geolocation g0, geolocation g1);

    // Interpolate fractional arc between two wgs84 vectors.
    // This models the earths ellipsoid.
    geolocation GeoSlerp(double frac);
};

class TwoPointInterpolator
{
    virtual xyz InterpolateNext(xyz p0, xyz p1) = 0;
};

struct LinearTwoPointInterpolator : public TwoPointInterpolator
{
    xyz InterpolateNext(xyz p0, xyz p1);
};

struct SphericalTwoPointInterpolator : public TwoPointInterpolator 
{
    xyz InterpolateNext(xyz p0, xyz p1);
};

class UnitCatmullRomInterpolator 
{
    std::tuple<double, double, double, double> m_p;

public:

    void Init(double pm1, double p0, double p1, double p2);
    UnitCatmullRomInterpolator();
    UnitCatmullRomInterpolator(double pm1, double p0, double p1, double p2);
    double Interpolate(double u);
};

class UnitCatmullRomInterpolator3D 
{
    UnitCatmullRomInterpolator x, y, z;

public:

    void Init(xyz pm1, xyz p0, xyz p1, xyz p2);
    UnitCatmullRomInterpolator3D() : x(), y(), z() {}
    UnitCatmullRomInterpolator3D(xyz pm1, xyz p0, xyz p1, xyz p2);
    xyz Interpolate(double frac);
};

// 4 element sliding window to hold interpolation points
template <typename T> class SlidingWindow
{
    std::tuple<T, T, T, T> m_Window;
    std::bitset<4>         m_ElementExists;

public:

    SlidingWindow() : m_ElementExists(0) {}

    unsigned Count() const { return (unsigned)m_ElementExists.count(); }

    T& pm1()               { return std::get<0>(m_Window); }
    T& p0()                { return std::get<1>(m_Window); }
    T& p1()                { return std::get<2>(m_Window); }
    T& p2()                { return std::get<3>(m_Window); }

    T  pm1()         const { return std::get<0>(m_Window); }
    T  p0()          const { return std::get<1>(m_Window); }
    T  p1()          const { return std::get<2>(m_Window); }
    T  p2()          const { return std::get<3>(m_Window); }

    bool  haspm1()   const { return m_ElementExists.test(3); }
    bool  hasp0()    const { return m_ElementExists.test(2); }
    bool  hasp1()    const { return m_ElementExists.test(1); }
    bool  hasp2()    const { return m_ElementExists.test(0); }

    void Push(const T& t)
    {
        m_ElementExists <<= 1;
        m_ElementExists.set(0); // set p2 existing

        pm1() = p0();
        p0() = p1();
        p1() = p2();
        p2() = t;
    }

    void Advance()
    {
        m_ElementExists <<= 1;

        pm1() = p0();
        p0() = p1();
        p1() = p2();

        // p2 left alone
    }
};

// Class maintains queue of user points and interpolator. Interpolator has
// separate copy of points since it may operate on synthesized points when
// window is not full. Window only holds user points.
template <typename T_TwoPointInterpolator> class UnitCatmullRomInterpolator3DWindow : T_TwoPointInterpolator
{
    UnitCatmullRomInterpolator3D m_Interpolator;
    SlidingWindow<xyz>           m_PointWindow;
    bool                         m_DidChange;

public:

    UnitCatmullRomInterpolator3DWindow() : m_DidChange(true) {}

    // Add points to interpolation queue.
    // Do something reasonable with partially filled queue.
    void Push(const xyz& point)
    {
        m_PointWindow.Push(point);
        m_DidChange = true;
    }

    void Advance()
    {
        m_PointWindow.Advance();
        m_DidChange = true;
    }

    // Update interpolator if input state has changed
    // This method synthesizes fake points to allow interpolation on partially filled point queue
    // which occurs when there are insufficient points or at the start and end of the interpolation.
    void update()
    {
        // Queue changed, update interpolator with new points, interpolate new points if necessary
        if (m_DidChange) {
            xyz pm1(m_PointWindow.pm1());
            xyz p0(m_PointWindow.p0());
            xyz p1(m_PointWindow.p1());
            xyz p2(m_PointWindow.p2());

            switch (m_PointWindow.Count()) {
            case 4:
                // All points are set.
                break;

            case 3:
                // Either pm1 or p2 is missing, interpolate the missing one.
            {
                xyz *s, *e, *pr;
                if (!m_PointWindow.haspm1()) {
                    s = &p1;
                    e = &p0;
                    pr = &pm1;
                } else {
                    s = &p0;
                    e = &p1;
                    pr = &p2;
                }
                *pr = InterpolateNext(*s, *e);
            }
            break;

            case 2:

            {
                xyz *pr0, *pr1, *s0, *s1, *e0, *e1;

                // pm1 and p2
                // p1 and p2
                if (!m_PointWindow.haspm1() && !m_PointWindow.hasp0()) {
                    pr0 = &p0;
                    s0 = &p2;
                    e0 = &p1;

                    pr1 = &pm1;
                    s1 = &p1;
                    e1 = pr0;
                } else if (!m_PointWindow.haspm1()) {
                    // interpolate to pm1 and p2
                    pr0 = &pm1;
                    s0 = &p1;
                    e0 = &p0;

                    pr1 = &p2;
                    s1 = &p0;
                    e1 = &p1;
                } else {
                    // interpolate to p1 and p2
                    pr0 = &p1;
                    s0 = &pm1;
                    e0 = &p0;

                    pr1 = &p2;
                    s1 = &p0;
                    e1 = pr0;
                }

                *pr0 = InterpolateNext(*s0, *e0);
                *pr1 = InterpolateNext(*s1, *e1);
            }
            break;

            case 1:
                // one point is present, propagate it to remaining entries.
            {
                xyz t = p2;
                if (m_PointWindow.hasp1()) t = p1;
                else if (m_PointWindow.hasp0()) t = p0;
                else if (m_PointWindow.haspm1()) t = pm1;

                pm1 = t;
                p0 = t;
                p1 = t;
                p2 = t;
            }
            break;

            case 0:
            {
                // no points, default 0.0 is as good as anything.
                // or assert?
                xyz zero(0, 0, 0);
                pm1 = zero;
                p0 = zero;
                p1 = zero;
                p2 = zero;
            }
            break;
            }

            m_Interpolator.Init(pm1, p0, p1, p2);
            m_DidChange = false;
        }
    }

    // Interpolate between points[1] and points[2].
    // If points[0] and points[3] dont exist then
    // create them.
    xyz Interpolate(double frac)
    {
        // Ensure interpolator has current state - synthesize fake points if needed.
        update();

        // assert frac in range 0-1.

        return m_Interpolator.Interpolate(frac);
    }
};

template <typename T_TwoPointInterpolator> class DistancePointInterpolator
{
    UnitCatmullRomInterpolator3DWindow<T_TwoPointInterpolator> m_Interpolator;
    SlidingWindow<double>                                      m_DistanceWindow;
    bool                                                       m_fInputComplete;

    static double OffsetInRangeToFraction(double distance, double start, double end)
    {
        // assert distance >= start && distance <= end
        return (distance - start) / (end - start);
    }

public:

    DistancePointInterpolator() : m_fInputComplete(false) {}

    void Push(double pointdistance, xyz point)
    {
        m_Interpolator.Push(point);
        m_DistanceWindow.Push(pointdistance);
    }

    void NotifyInputComplete()
    {
        m_fInputComplete = true;
    }

    bool WantsInput(double distance) const
    {
        if (m_fInputComplete) return false;

        bool r = false;
        switch (m_DistanceWindow.Count()) {
        case 0:
        case 1:
            r = true;
            break;
        case 2:
        case 3:
        case 4:
            r = distance >= m_DistanceWindow.p1();
            break;
        }
        return r;
    }

    xyz Interpolate(double distance)
    {
        // Continue to advance queue after input is complete.
        if (m_fInputComplete) {
            // When input is finished we must continue to advance the point state based on distance
            while (m_DistanceWindow.hasp1() && distance >= m_DistanceWindow.p1()) {
                m_DistanceWindow.Advance();
                m_Interpolator.Advance();
            }
        }

        // Determine fraction of range that this distance specifies (frac must be 0-1.)
        double frac = 0.0;
        switch (m_DistanceWindow.Count())
        {
        case 0:
        case 1:
            break;
        case 2:
            if (!m_DistanceWindow.haspm1() && !m_DistanceWindow.hasp0()) {
                // missing: pm1 && p0
                // Query distance comes before any known range.
                frac = 1.0;
            } else if (!m_DistanceWindow.haspm1()) {
                // missing: pm1 && p2
                frac = OffsetInRangeToFraction(distance, m_DistanceWindow.p0(), m_DistanceWindow.p1());
            } else {
                // missing: p1  && p2
                frac = 0.0;
            }
            break;
        case 3:
        case 4:
            frac = OffsetInRangeToFraction(distance, m_DistanceWindow.p0(), m_DistanceWindow.p1());
            break;
        }

        return m_Interpolator.Interpolate(frac);
    }
};

class GeoPointInterpolator : public DistancePointInterpolator<SphericalTwoPointInterpolator>
{
public:

    GeoPointInterpolator() : DistancePointInterpolator() {}

    geolocation Interpolate(double distance);

    void Push(double distance, geolocation point);
};
