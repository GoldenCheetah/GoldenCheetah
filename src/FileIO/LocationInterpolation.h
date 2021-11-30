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

#if !defined(_LOCATION_INTERPOLATION_H)
#define _LOCATION_INTERPOLATION_H

#include <tuple>
#include <ratio>

#include "qwt_math.h"

#if defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE
#endif

struct geolocation;

class v3
{
    std::tuple<double, double, double> m_t;

public:

    v3(double a, double b, double c) : m_t(a, b, c) {};

    v3(const v3& o) : m_t(o.m_t) {}

    double  x() const { return std::get<0>(m_t); }
    double  y() const { return std::get<1>(m_t); }
    double  z() const { return std::get<2>(m_t); }

    double& x() { return std::get<0>(m_t); }
    double& y() { return std::get<1>(m_t); }
    double& z() { return std::get<2>(m_t); }

    v3 add(const v3 &a)      const { return v3(x() + a.x(), y() + a.y(), z() + a.z()); }
    v3 subtract(const v3 &a) const { return v3(x() - a.x(), y() - a.y(), z() - a.z()); }
    v3 scale(double d)       const { return v3(x() * d, y() * d, z() * d); }

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

    double DistanceFrom(const xyz& from) const {
        return this->subtract(from).magnitude();
    }

    geolocation togeolocation() const;
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

    xyz toxyz() const;

    double DistanceFrom(const geolocation& from) const {
        xyz x0 = from.toxyz();
        xyz x1 = this->toxyz();

        double dist = x1.subtract(x0).magnitude();

        return dist;
    }

    bool IsReasonableAltitude() const {
        return (this->Alt() >= -1000 && this->Alt() < 10000);
    }

    bool IsReasonableGeoLocation() const {
        return  (this->Lat() && this->Lat() >= double(-90) && this->Lat() <= double(90) &&
            this->Long() && this->Long() >= double(-180) && this->Long() <= double(180) &&
            IsReasonableAltitude());
    }

    // Compute initial bearing from this geoloc to another, in RADIANS.
    // Note that unless bearing is 0 or pi it will vary on the path from one to the other.
    double BearingTo(const geolocation& to) const;
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
public:
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

// Coefficient caching. A large benefit is that by performing compute separate
// from use the use will be inlined. Cache efficiency depends on route point
// distances but typical rides cache succeeds 5-10 times before invalidated.
class LazyCoefficients
{
    mutable bool m_CoefsValid;
public:
    bool Valid()       const { return m_CoefsValid; }
    void Validate()    const { m_CoefsValid = true; }
    void Invalidate()  const { m_CoefsValid = false; }

    LazyCoefficients() : m_CoefsValid(false) {}
};

template <typename T_SourceCoefType, typename T_Ratio>
class LazyLocationCoefficients : public LazyCoefficients
{
    // Cache of computed coefficients for location computation.
    mutable std::tuple<double, double, double, double> m_coefs;

    // Computes and stores coefficients for cubic splne location calculation.
    NOINLINE void Compute(const T_SourceCoefType& base) const
    {
        // Curvature-parameterized CatmullRom equation courtesy of wolfram alpha:
        // [1, u, u ^ 2, u ^ 3] * [[0, 1, 0, 0], [-t, 0, t, 0], [2 * t, t - 3, 3 - 2t, -t], [-t, 2 - t, t - 2, t]] * [p0, p1, p2, p3]

        double t = (double)T_Ratio::num / (double)T_Ratio::den;

        double p0 = std::get<0>(base);
        double p1 = std::get<1>(base);
        double p2 = std::get<2>(base);
        double p3 = std::get<3>(base);

        double A = ((p3 + p2 - p1 - p0) * t - 2 * p2 + 2 * p1);
        double B = ((-p3 - 2 * p2 + p1 + 2 * p0) * t + 3 * p2 - 3 * p1);
        double C = (p2 - p0) * t;
        double D = p1;

        std::get<0>(m_coefs) = A;
        std::get<1>(m_coefs) = B;
        std::get<2>(m_coefs) = C;
        std::get<3>(m_coefs) = D;

        Validate();
    }

public:

    void Get(const T_SourceCoefType &base, double &A, double &B, double &C, double &D) const
    {
        if (!Valid()) {
            Compute(base);
        }

        A = std::get<0>(m_coefs);
        B = std::get<1>(m_coefs);
        C = std::get<2>(m_coefs);
        D = std::get<3>(m_coefs);
    }
};

template <typename T_SourceCoefType, typename T_Ratio>
class LazyTangentCoefficients : public LazyCoefficients
{
    // Cache of computed coefficients for tangent computation.
    mutable std::tuple<double, double, double> m_coefs;

    // Computes and stores coefficients for cubic splne tangent calculation.
    NOINLINE void Compute(const T_SourceCoefType &base) const
    {
        // d f(u)/ du of Curvature-parameterized CatmullRom equation courtesy of wolfram alpha:
        // [1, u, u ^ 2, u ^ 3] * [[0, 1, 0, 0], [-t, 0, t, 0], [2 * t, t - 3, 3 - 2t, -t], [-t, 2 - t, t - 2, t]] * [p0, p1, p2, p3]
        double t = (double)T_Ratio::num / (double)T_Ratio::den;

        double p0 = std::get<0>(base);
        double p1 = std::get<1>(base);
        double p2 = std::get<2>(base);
        double p3 = std::get<3>(base);

        double A = 3 * ((p3 + p2 - p1 - p0) * t - 2 * p2 + 2 * p1);
        double B = 2 * ((-p3 - 2 * p2 + p1 + 2 * p0)*t + 3 * p2 - 3 * p1);
        double C = (p2 - p0) * t;

        std::get<0>(m_coefs) = A;
        std::get<1>(m_coefs) = B;
        std::get<2>(m_coefs) = C;

        Validate();
    }

public:

    LazyTangentCoefficients() {};

    void Get(const T_SourceCoefType &base, double &A, double &B, double &C) const
    {
        if (!Valid()) {
            Compute(base);
        }

        A = std::get<0>(m_coefs);
        B = std::get<1>(m_coefs);
        C = std::get<2>(m_coefs);
    }
};


class UnitCatmullRomInterpolator
{
    // Control curvature:
    //   0   is standard (straigtest)
    //   0.5 is called centripetal
    //   1   is chordal (loopiest)
    typedef std::ratio<1, 2> T;

    typedef std::tuple<double, double, double, double> CoefType;

    CoefType m_baseCoefs;

    LazyLocationCoefficients<CoefType, T> m_locCoefs;
    LazyTangentCoefficients<CoefType, T>  m_tanCoefs;

public:

    void Init(double pm1, double p0, double p1, double p2);
    UnitCatmullRomInterpolator();
    UnitCatmullRomInterpolator(double pm1, double p0, double p1, double p2);
    double Location(double u) const;
    double Tangent(double u) const;
    bool   Inverse(double r, double &u) const;
};

class UnitCatmullRomInterpolator3D
{
    UnitCatmullRomInterpolator x, y, z;

public:

    void Init(xyz pm1, xyz p0, xyz p1, xyz p2);
    UnitCatmullRomInterpolator3D() : x(), y(), z() {}
    UnitCatmullRomInterpolator3D(xyz pm1, xyz p0, xyz p1, xyz p2);
    xyz Location(double frac) const;
    xyz Tangent(double frac) const;
};

// Visual studio has an error in how it compiles bitset that prevents
// us from mixing it with qt headers >= 5.12.0. The toolchain error will
// allegedly be fixed in vs2019 so roll our own for this very limited case.
template <size_t T_bitsize> class MyBitset
{
    static_assert(T_bitsize <= 32, "T_bitsize must be <= 32.");
    static_assert(T_bitsize >= 1, "T_bitsize must be >= 1.");

    unsigned m_mask;

    unsigned popcnt(unsigned x) const {
        x = x - ((x >> 1) & 0x55555555);
        x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
        return (((x + (x >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
    }

    void truncate() { m_mask &= ((~0U << (32 - T_bitsize)) >> (32 - T_bitsize)); }

public:

    MyBitset(unsigned m) : m_mask(m) {}
    void reset() { m_mask = 0; }
    bool test(unsigned u) const { return ((m_mask >> u) & 1) != 0; }
    void set(unsigned u) { m_mask |= (1 << u); truncate(); }
    unsigned count() const { return popcnt(m_mask); }
    MyBitset<T_bitsize>& operator <<=(unsigned u) { m_mask <<= u; truncate(); return (*this); }
};

// 4 element sliding window to hold interpolation points
template <typename T> class SlidingWindow
{
    std::tuple<T, T, T, T>     m_Window;
    MyBitset<4>                m_ElementExists;
    //std::bitset<4>             m_ElementExists; // Visual studio error prevents bitset use alongside qtbase header.

    UnitCatmullRomInterpolator u;
    bool                       m_InterpolatorNeedsInit;

    void Maintain()
    {
        if (m_InterpolatorNeedsInit) {
            u.Init(pm1(), p0(), p1(), p2());
            m_InterpolatorNeedsInit = false;
        }
    }

public:

    SlidingWindow() : m_ElementExists(0), m_InterpolatorNeedsInit(true) {}

    void Reset() {
        m_ElementExists.reset();
    }

    unsigned Count() const { return (unsigned)m_ElementExists.count(); }

    T& pm1(){ m_InterpolatorNeedsInit = true; return std::get<0>(m_Window); }
    T& p0() { m_InterpolatorNeedsInit = true; return std::get<1>(m_Window); }
    T& p1() { m_InterpolatorNeedsInit = true; return std::get<2>(m_Window); }
    T& p2() { m_InterpolatorNeedsInit = true; return std::get<3>(m_Window); }

    T  pm1()         const { return std::get<0>(m_Window); }
    T  p0()          const { return std::get<1>(m_Window); }
    T  p1()          const { return std::get<2>(m_Window); }
    T  p2()          const { return std::get<3>(m_Window); }

    bool  haspm1()   const { return m_ElementExists.test(3); }
    bool  hasp0()    const { return m_ElementExists.test(2); }
    bool  hasp1()    const { return m_ElementExists.test(1); }
    bool  hasp2()    const { return m_ElementExists.test(0); }

    bool BracketFromDistance(double distance, double &bracket)
    {
        Maintain();

        return u.Inverse(distance, bracket);
    }

    // Returns rate of change of distance spline at bracket [0..1].
    double FrameSpeed(double frac) const
    {
        // Do not call Maintain() here. frac input implies that current
        // interpolator state should be queried.
        // If anything assert that interpolator state needs no update.

        double frameSpeed = u.Tangent(frac);

        // Slope can be zero at start (when prior distance window entries are zero.)
        // Just use average until there are enough datapoints to interpolate.

        // Note frameSpeed should never be negative since that would imply
        // route distance can decrease.

        frameSpeed = (frameSpeed > 0.) ? frameSpeed : (p1() - p0());

        return frameSpeed;
    }

    void Push(const T& t)
    {
        m_InterpolatorNeedsInit = true;

        m_ElementExists <<= 1;
        m_ElementExists.set(0); // set p2 existing

        pm1() = p0();
        p0() = p1();
        p1() = p2();
        p2() = t;
    }

    void Advance()
    {
        m_InterpolatorNeedsInit = true;

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
template <typename T_TwoPointInterpolator> class UnitCatmullRomInterpolator3DWindow : public T_TwoPointInterpolator
{
    UnitCatmullRomInterpolator3D m_Interpolator;
    SlidingWindow<xyz>           m_PointWindow;
    bool                         m_DidChange;

public:

    UnitCatmullRomInterpolator3DWindow() : m_DidChange(true) {}

    void Reset() {
        m_PointWindow.Reset();
        m_DidChange = true;
    }

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

    bool NeedsUpdate() { return m_DidChange; }

    // Update interpolator if input state has changed
    // This method synthesizes fake points to allow interpolation on partially filled point queue
    // which occurs when there are insufficient points or at the start and end of the interpolation.
    NOINLINE void Update()
    {
        // Queue changed, update interpolator with new points, interpolate new points if necessary
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
            }
            else {
                s = &p0;
                e = &p1;
                pr = &p2;
            }
            *pr = this->InterpolateNext(*s, *e);
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
            }
            else if (!m_PointWindow.haspm1()) {
                // interpolate to pm1 and p2
                pr0 = &pm1;
                s0 = &p1;
                e0 = &p0;

                pr1 = &p2;
                s1 = &p0;
                e1 = &p1;
            }
            else {
                // interpolate to p1 and p2
                pr0 = &p1;
                s0 = &pm1;
                e0 = &p0;

                pr1 = &p2;
                s1 = &p0;
                e1 = pr0;
            }

            *pr0 = this->InterpolateNext(*s0, *e0);
            *pr1 = this->InterpolateNext(*s1, *e1);
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

    // Interpolate between points[1] and points[2].
    // If points[0] and points[3] dont exist then
    // create them.
    xyz Location(double frac)
    {
        // Ensure interpolator has current state - synthesize fake points if needed.
        if (NeedsUpdate())
            Update();

        return m_Interpolator.Location(frac);
    }

    // Interpolate between points[1] and points[2].
    // If points[0] and points[3] dont exist then
    // create them.
    xyz Tangent(double frac)
    {
        // Ensure interpolator has current state - synthesize fake points if needed.
        if (NeedsUpdate())
            Update();

        return m_Interpolator.Tangent(frac);
    }
};

template <typename T_TwoPointInterpolator> class DistancePointInterpolator
{
    UnitCatmullRomInterpolator3DWindow<T_TwoPointInterpolator> m_Interpolator;
    SlidingWindow<double>                                      m_DistanceWindow;
    bool                                                       m_fInputComplete;

    static double OffsetInRangeToRatio(double distance, double start, double end)
    {
        if (end == start)
            return 0;

        return (distance - start) / (end - start);
    }

    // Prepare interpolation window for query at distance, then return
    // bracket ratio for distance within current bracket.
    double DistanceToBracketRatio(double distance)
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
        double ratio = 0.0;
        switch (m_DistanceWindow.Count())
        {
        case 0:
        case 1:
            break;
        case 2:
            if (!m_DistanceWindow.haspm1() && !m_DistanceWindow.hasp0()) {
                // missing: pm1 && p0
                // Query distance comes before any known range.
                ratio = 1.0;
            }
            else if (!m_DistanceWindow.haspm1()) {
                // missing: pm1 && p2
                ratio = OffsetInRangeToRatio(distance, m_DistanceWindow.p0(), m_DistanceWindow.p1());
            }
            else {
                // missing: p1  && p2
                ratio = 0.0;
            }
            break;
        case 3:
            ratio = OffsetInRangeToRatio(distance, m_DistanceWindow.p0(), m_DistanceWindow.p1());
            break;
        case 4:
            {
                if (distance == m_DistanceWindow.p0()) return 0.;
                if (distance == m_DistanceWindow.p1()) return 1.;
                bool fInv = m_DistanceWindow.BracketFromDistance(distance, ratio);
                if (!fInv) {
                    ratio = OffsetInRangeToRatio(distance, m_DistanceWindow.p0(), m_DistanceWindow.p1());
                }
            }
            break;
        }

        return ratio;
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

    void Reset()
    {
        m_fInputComplete = false;
        m_Interpolator.Reset();
        m_DistanceWindow.Reset();
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

    // Return center two points of the four point interpolation window.
    bool GetBracket(double &d0, double &d1)
    {
        if (!m_DistanceWindow.hasp0() || !m_DistanceWindow.hasp1())
            return false;

        d0 = m_DistanceWindow.p0();
        d1 = m_DistanceWindow.p1();

        return true;
    }

    // Location interpolation when tangent isn't needed.
    xyz Location(double distance)
    {
        double ratio = DistanceToBracketRatio(distance);
        xyz newXYZLocation = m_Interpolator.Location(ratio);
        return newXYZLocation;
    }

    // Location interpolation including tangent vector computation
    xyz Location(double distance, xyz &tangentVector)
    {
        double bracketRatio = DistanceToBracketRatio(distance);

        tangentVector = m_Interpolator.Tangent(bracketRatio);

        // Frame speed is speed of parametric distance.
        // Remove frame speed from tangent vector.
        double frameSpeed = m_DistanceWindow.FrameSpeed(bracketRatio);
        double tangentSpeed = tangentVector.magnitude();
        double correctionScale = tangentSpeed ? (frameSpeed / (tangentSpeed * tangentSpeed)) : 1.;

        // This rescale converts tangent vector into a unit vector
        // WRT parametric route velocity.
        // Tangent vector's remaining non-unit-ness is due to
        // distortion from mapping route speed to geometric
        // speed, but that cannot be corrected here since we
        // are in ECEF and vector's altitude component is not
        // distorted while orthagonal plane is.
        tangentVector = tangentVector.scale(correctionScale);

        xyz l0xyz = m_Interpolator.Location(bracketRatio);

        return l0xyz;
    }

private:

    struct CalcSplineLengthBracketPair
    {
        double d0, d1;

        CalcSplineLengthBracketPair() {}
        CalcSplineLengthBracketPair(double a0, double a1) : d0(a0), d1(a1) {}
    };

    // A static sized (static sized/frame allocatable) worklist for pushing and popping stuffs.
    template <typename T, int T_count> class CalcSplineLengthWorklist
    {
        static_assert(T_count > 0 && T_count <= 64, "Worklist element count must be within 1 and 64");

        T m_worklist[T_count];
        int m_idx; // points to first unused element in worklist

    public:

        CalcSplineLengthWorklist() : m_idx(0) {}

        unsigned EmptySlots() const {
            return T_count - m_idx;
        }

        bool Push(T rT) {
            if (m_idx >= T_count)
                return false;

            m_worklist[m_idx] = rT;
            m_idx++;
            return true;
        }

        bool Pop(T& rT) {
            if (m_idx == 0) return false;

            m_idx--;
            rT = m_worklist[m_idx];
            return true;
        }
    };

public:

    //
    // SplineLength: Estimate path distance across bracketed spline range.
    //
    // Estimation is necessary here because there is no closed form for length of cubic spline.
    //
    // Cubic spline has 4 control points. The interpolation is only valid within the middle 2 points.
    // I call these middle two points the 'bracket'.
    //
    // Method takes two distances within the bracket and estimates the length of the spline that is
    // interpolated between those two interpolated points.
    //
    // Estimation technique I use :
    //
    // 1 Break provided distance range into 3 equidistant distance ranges (4 distances)
    // 2 Interpolate location for each of those 4 locations
    // 3 Compute 'linear distance': the geometric distance from first to last location.
    // 4 Compute 'quad distance': the sum of geometric distance between those 4 locations.
    // 5 Examine ratio of linear distance / quad distance.
    //   a If below threshold accumulate the summed quad distance
    //   b If above threshold then push the 3 ranges onto worklist and goto 1
    //
    double SplineLength(double d0, double d1, double thresholdLimit = 0.000001)
    {
        double bracketStart, bracketEnd;

        // Ensure:
        // - bracket exists
        // - requested range is within bracket
        // - range is ordered
        if (!this->GetBracket(bracketStart, bracketEnd)
            || d0 < bracketStart
            || d0 > bracketEnd
            || d1 < bracketStart
            || d1 > bracketEnd
            || d0 > d1)
        {
            return 0.0;
        }

        double finalLength = 0.0;

        static const unsigned s_worklistSize = 32;
        CalcSplineLengthWorklist<CalcSplineLengthBracketPair, s_worklistSize> worklist;

        // Push initial range for processing.
        if (!worklist.Push(CalcSplineLengthBracketPair(d0, d1)))
            return 0.0;

        CalcSplineLengthBracketPair workitem;
        while (worklist.Pop(workitem)) {
            d0 = workitem.d0;
            d1 = workitem.d1;

            // Step 1
            const double thirdspan = (d1 - d0) / 3;
            const double inter0 = d0 + thirdspan;
            const double inter1 = d0 + thirdspan + thirdspan;

            // Step 2
            const xyz pm1 = this->Location(d0);
            const xyz  p0 = this->Location(inter0);
            const xyz  p1 = this->Location(inter1);
            const xyz  p2 = this->Location(d1);

            // Step 3
            double linearDistance = p2.DistanceFrom(pm1);
            if (linearDistance < 0.0001)
                break;

            // Step 4
            double span0 = p0.DistanceFrom(pm1);
            double span1 = p1.DistanceFrom(p0);
            double span2 = p2.DistanceFrom(p1);

            double arcDistance = span0 + span1 + span2;

            // Step 5
            double difference = fabs(arcDistance / linearDistance) - 1.0;

            // 5A: Settle for quaddistance if threshold met or no more room on worklist.
            if (difference < thresholdLimit || worklist.EmptySlots() < 3) {
                finalLength += arcDistance;
            } else {
                // 5B: otherwise push the 3 new subsegments onto worklist.
                worklist.Push(CalcSplineLengthBracketPair(d0, inter0));
                worklist.Push(CalcSplineLengthBracketPair(inter0, inter1));
                worklist.Push(CalcSplineLengthBracketPair(inter1, d1));
            }
        }

        return finalLength;
    }
};

class GeoPointInterpolator : public DistancePointInterpolator<SphericalTwoPointInterpolator>
{
    enum LocationState { Unset, YesLocation, NoLocation};
    LocationState m_locationState;

public:

    GeoPointInterpolator() : DistancePointInterpolator<SphericalTwoPointInterpolator>(), m_locationState(Unset) {}

    void Reset() {
        m_locationState = Unset;
        DistancePointInterpolator<SphericalTwoPointInterpolator>::Reset();
    }

    geolocation Location(double distance);
    geolocation Location(double distance, double &slope);

    bool HasLocation() { return m_locationState == YesLocation; }

    void Push(double distance, geolocation point);
    void Push(double distance, double altitude);
};

#endif
