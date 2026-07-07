/*
 * Copyright (c) 2020 Eric Christoffersen (impolexg@outlook.com)
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

#ifndef _GC_Integrator_h
#define  _GC_Integrator_h

#include <tuple>

struct IntegrateResult : public std::tuple<double, double> {
    IntegrateResult(double _endPoint, double _sum) : std::tuple<double, double>(_endPoint, _sum) {}
    double endPoint() const { return std::get<0>(*this); }
    double sum()      const { return std::get<1>(*this); }
};

template <typename T>
IntegrateResult
Integrate_EulerDirect(const T &state, double v)
{
    double dt = state.DT();
    double vt = state.CalcV(v, dt);

    return { vt, vt * dt};
}

// Classic Runge-Kutta 4 as per Wikipedia
template <typename T>
IntegrateResult
Integrate_RK4(const T &state, double v0)
{
    double t0 = state.T0();
    double dt = state.DT();

    double sum = 0.;
    double m = 0.;

    double v1 = v0;
    double m1 = state.dVdT(v1, t0);
    sum += v1;
    m += m1;

    double v2 = v0 + (m1 * dt / 2);
    double m2 = state.dVdT(v2, t0 + dt / 2);
    sum += 2 * v2;
    m += 2 * m2;

    double v3 = v0 + (m2 * dt / 2);
    double m3 = state.dVdT(v3, t0 + dt / 2);
    sum += 2 * v3;
    m += 2 * m3;

    double v4 = v0 + (m3 * dt);
    double m4 = state.dVdT(v4, t0 + dt);
    sum += v4;
    m += m4;

    m /= 6;
    sum /= 6;

    return { v0 + m * dt , sum * dt };
}

// 3/8 Rule Runge-Kutta 4 as per Wikipedia
template <typename T>
IntegrateResult
Integrate_RK4_38(const T &state, double v0)
{
    double t0 = state.T0();
    double dt = state.DT();

    double sum = 0.;
    double m = 0.;

    double v1 = v0;
    sum += v1;
    double m1 = state.dVdT(v1, t0);
    m += m1;

    double v2 = v0 + (m1 * dt / 3);
    double m2 = state.dVdT(v2, t0 + dt / 3);
    sum += 3 * v2;
    m += 3 * m2;

    double v3 = v0 + (m1 * dt / -3) + (m2 * dt);
    double m3 = state.dVdT(v3, t0 + (2 * dt / 3));
    sum += 3 * v3;
    m += 3 * m3;

    double v4 = v0 + (m1 * dt) + (m2 * -dt) + (m3 * dt);
    double m4 = state.dVdT(v4, t0 + dt);
    sum += v4;
    m += m4;

    m /= 8;
    sum /= 8;

    return { v0 + m * dt , sum * dt };
}

//#define TEST_COEFFICIENTS
#if defined(TEST_COEFFICIENTS)
#include <assert.h>
// Function for testing integrator coefficients.
//
// Each ordered sum of 'c' terms is multiplied by dt
// to choose sample time. If this time falls out of range
// t0 to dt then derivative may be unreliable.
//
// Function computes all the scan locations. Returns
// false if coefficients will cause out of range sample,
// otherwise true.
//
// Be certain your function is stable in extended range
// if you will use coefficients that fail this test.
static bool TestCoeffs(const double *pc, int terms)
{
    bool becomesNegative = false;
    bool becomesOver1 = false;
    double sum = 0;
    for (int i = terms - 1; i >= 0; i--)
    {
        sum += pc[i];
        if (sum < 0) becomesNegative = true;
        if (sum > 1) becomesOver1 = true;
    }

    return (!becomesNegative && !becomesOver1);
}
#endif

template <typename T>
IntegrateResult
SymplecticSum(int taps, const T& state, double v, const double * d, const double * c)
{
#if defined(TEST_COEFFICIENTS)
    bool tc = TestCoeffs(c, taps);
    Q_ASSERT(tc);
#endif

    double t = state.T0();
    double dt = state.DT();

    double distanceTick = 0;

    for (int i = 0; i < taps; i++)
    {
        const double step = dt * c[i];

        t += step;
        distanceTick +=  v * step;

        if (i == (taps - 1))
            break;

        double m = state.dVdT(v, t);
        v += (dt * d[i]) * m;
    }

    return { v, distanceTick };
}

//
// Fourth order Symplectic Integrator using Yoshida coefficients.
//
// Based upon:
// "Construction of higher order symplectic integrators"
// Haruo Yoshida National Astronomical Observatory, Mitaka, Tokyo 181, Japan
// Received 11 July 1990; accepted for publication 11 September 1990 Communicated by D.D.Holm
// http://cacs.usc.edu/education/phys516/Yoshida-symplectic-PLA00.pdf
//
// Constants courtesy of wolfram alpha.
template <typename T>
IntegrateResult
Integrate_Yoshida4(const T &state, double v)
{
    static const double s_x0  = -1.702414383919315268095375617942921653843998752434289656657; // -2^(1/3)/(2-(2^(1/3))
    static const double s_x1  =  1.351207191959657634047687808971460826921999376217144828328; // 1/(2-(2^(1/3))
    static const double s_c14 =  0.675603595979828817023843904485730413460999688108572414164; // (1/2)/(s_x1)
    static const double s_c23 = -0.17560359597982881702384390448573041346099968810857241416;  // (s_x0 + s_x1) / 2
    static const double s_d[3] = { s_x1, s_x0, s_x1 };
    static const double s_c[4] = { s_c14, s_c23, s_c23, s_c14 };

    static const int s_taps = sizeof(s_c) / sizeof(s_c[0]);
    return SymplecticSum(s_taps, state, v, s_d, s_c);
}

//
// Sixth order Symplectic Integrator using Yoshida Solution A coefficients.
//
// Based upon:
// "Construction of higher order symplectic integrators"
// Haruo Yoshida National Astronomical Observatory, Mitaka, Tokyo 181, Japan
// Received 11 July 1990; accepted for publication 11 September 1990 Communicated by D.D.Holm
// http://cacs.usc.edu/education/phys516/Yoshida-symplectic-PLA00.pdf
template <typename T>
IntegrateResult
Integrate_Yoshida6(const T &state, double v)
{
    static const double s_w1 = -0.117767998417887E1;
    static const double s_w2 = 0.235573213359357E0;
    static const double s_w3 = 0.784513610477560E0;
    static const double s_w0 = (1 - 2 * (s_w1 + s_w2 + s_w3));
    static const double s_d[7] = { s_w3, s_w2, s_w1, s_w0, s_w1, s_w2, s_w3 };
    static const double s_c[8] = {          s_w3 / 2, (s_w3 + s_w2) / 2, (s_w2 + s_w1) / 2, (s_w1 + s_w0) / 2,
                                   (s_w1 + s_w0) / 2, (s_w2 + s_w1) / 2, (s_w3 + s_w2) / 2, s_w3 / 2 };

    static const int s_taps = sizeof(s_c) / sizeof(s_c[0]);
    return SymplecticSum(s_taps, state, v, s_d, s_c);
}

//
// Eighth order Symplectic Integrator using Yoshida Solution A coefficients.
//
// Based upon:
// "Construction of higher order symplectic integrators"
// Haruo Yoshida National Astronomical Observatory, Mitaka, Tokyo 181, Japan
// Received 11 July 1990; accepted for publication 11 September 1990 Communicated by D.D.Holm
// http://cacs.usc.edu/education/phys516/Yoshida-symplectic-PLA00.pdf
//
// Be careful with this one. Its pretty bad for some uses because all the 8th order
// Yoshida coefficients sample way outside the range t0 to dt. KahanLi converges as
// fast and only samples within range.
template <typename T>
IntegrateResult
Integrate_Yoshida8(const T &state, double v)
{
    // Yoshida Solution A, B, C
    static const double s_WA[] = {-1.61582374150097,-2.44699182370524,-0.00716989419708120, 2.44002732616735, 0.157739928123617, 1.82020630970714, 1.04242620869991 };
    // static const double s_WB[] = {-0.00169248587770116, 2.89195744315849, 0.00378039588360192, -2.89688250328827, 2.89105148970595, -2.33864815101035, 1.48819229202922 };
    // static const double s_WC[] = { 0.311790812418427,-1.55946803821447,-1.6789692825964, 1.66335809963315,-1.06458714789183, 1.36934946416871, 0.629030650210433 };

    static const double *s_W = s_WA;

    static const double s_W1 = s_W[0];
    static const double s_W2 = s_W[1];
    static const double s_W3 = s_W[2];
    static const double s_W4 = s_W[3];
    static const double s_W5 = s_W[4];
    static const double s_W6 = s_W[5];
    static const double s_W7 = s_W[6];
    static const double s_W0 = (1 - 2 * (s_W1 + s_W2 + s_W3 + s_W4 + s_W5 + s_W6 + s_W7));

    static const double s_c[16] = { s_W7 / 2,         (s_W7 + s_W6) / 2, (s_W6 + s_W5) / 2, (s_W5 + s_W4) / 2,
                                    (s_W4 + s_W3) / 2, (s_W3 + s_W2) / 2, (s_W2 + s_W1) / 2, (s_W1 + s_W0) / 2,
                                    (s_W1 + s_W0) / 2, (s_W2 + s_W1) / 2, (s_W3 + s_W2) / 2, (s_W4 + s_W3) / 2,
                                    (s_W5 + s_W4) / 2, (s_W6 + s_W5) / 2, (s_W7 + s_W6) / 2,  s_W7 / 2 };

    static const double s_d[15] = { s_W7, s_W6, s_W5, s_W4, s_W3, s_W2, s_W1, s_W0,
                                    s_W1, s_W2, s_W3, s_W4, s_W5, s_W6, s_W7 };

    static const int s_taps = sizeof(s_c) / sizeof(s_c[0]);
    return SymplecticSum(s_taps, state, v, s_d, s_c);
}


// Reasonable coefficients from:
// Practical symplectic partitioned Runge–Kutta and Runge–Kutta–Nystrom methods S.Blanes, P.C.Moan
template <typename T>
IntegrateResult
Integrate_BlanesMoan(const T &state, double v)
{
    static const double a[] = { 0.0378593198406116, 0.102635633102435,-0.0258678882665587, 0.314241403071447,-0.130144459517415, 0.106417700369543,-0.00879424312851058 };

    static const double s_W7 = a[0];
    static const double s_W6 = a[1];
    static const double s_W5 = a[2];
    static const double s_W4 = a[3];
    static const double s_W3 = a[4];
    static const double s_W2 = a[5];
    static const double s_W1 = a[6];

    static const double s_W0 = (1 - 2 * (s_W1 + s_W2 + s_W3 + s_W4 + s_W5 + s_W6 + s_W7));

    static const double s_d[15] = { s_W7, s_W6, s_W5, s_W4, s_W3, s_W2, s_W1, s_W0,
                                    s_W1, s_W2, s_W3, s_W4, s_W5, s_W6, s_W7 };

    static const double s_c[16] = { s_W7 / 2,         (s_W7 + s_W6) / 2, (s_W6 + s_W5) / 2, (s_W5 + s_W4) / 2,
                                   (s_W4 + s_W3) / 2, (s_W3 + s_W2) / 2, (s_W2 + s_W1) / 2, (s_W1 + s_W0) / 2,
                                   (s_W1 + s_W0) / 2, (s_W2 + s_W1) / 2, (s_W3 + s_W2) / 2, (s_W4 + s_W3) / 2,
                                   (s_W5 + s_W4) / 2, (s_W6 + s_W5) / 2, (s_W7 + s_W6) / 2,  s_W7 / 2 };

    static const int s_taps = sizeof(s_c) / sizeof(s_c[0]);
    return SymplecticSum(s_taps, state, v, s_d, s_c);
}

//
// Optimal 8th order Sympleptic Integrator
// as described by Ernst Hairer in the paper:
//   Lecture 2: Symplectic integrators
//   https://www.unige.ch/~hairer/poly_geoint/week2.pdf
template <typename T>
IntegrateResult
Integrate_Hairer8(const T &state, double v)
{
    //static const double s_W0 = (1 - 2 * (s_W1 + s_W2 + s_W3 + s_W4 + s_W5 + s_W6 + s_W7));
    static const double s_W0  = -0.79688793935291635401978884;
    static const double s_W1  =  0.74167036435061295344822780;
    static const double s_W2  = -0.40910082580003159399730010;
    static const double s_W3  =  0.19075471029623837995387626;
    static const double s_W4  = -0.57386247111608226665638773;
    static const double s_W5  =  0.29906418130365592384446354;
    static const double s_W6  =  0.33462491824529818378495798;
    static const double s_W7  =  0.31529309239676659663205666;

    static const double s_d[15] = { s_W7, s_W6, s_W5, s_W4, s_W3, s_W2, s_W1, s_W0,
                                    s_W1, s_W2, s_W3, s_W4, s_W5, s_W6, s_W7 };

    static const double s_c[16] = { s_W7 / 2,         (s_W7 + s_W6) / 2, (s_W6 + s_W5) / 2, (s_W5 + s_W4) / 2,
                                   (s_W4 + s_W3) / 2, (s_W3 + s_W2) / 2, (s_W2 + s_W1) / 2, (s_W1 + s_W0) / 2,
                                   (s_W1 + s_W0) / 2, (s_W2 + s_W1) / 2, (s_W3 + s_W2) / 2, (s_W4 + s_W3) / 2,
                                   (s_W5 + s_W4) / 2, (s_W6 + s_W5) / 2, (s_W7 + s_W6) / 2,  s_W7 / 2 };

    static const int s_taps = sizeof(s_c) / sizeof(s_c[0]);
    return SymplecticSum(s_taps, state, v, s_d, s_c);
}

// KahanLi integrator from Julia
// Coefficients from Darrel Huffman (The Happy Koala)
// This converges very quickly.
template <typename T>
IntegrateResult
Integrate_KahanLi8(const T &state, double v)
{
    static const double s_c[16] = {
         0.3708351821753065,   0.16628476927529068, -0.1091730577518966,  -0.19155388040992194,
        -0.13739914490621316,  0.31684454977447707,  0.3249590053210324,  -0.24079742347807487,
        -0.24079742347807487,  0.3249590053210324,   0.31684454977447707, -0.13739914490621316,
        -0.19155388040992194, -0.1091730577518966,   0.16628476927529068,  0.3708351821753065
    };

    static const double s_d[15] = {
         0.741670364350613,   -0.4091008258000316,   0.1907547102962384,  -0.5738624711160822,
         0.2990641813036559,   0.33462491824529816,  0.3152930923967666,  -0.7968879393529164,
         0.3152930923967666,   0.33462491824529816,  0.2990641813036559,  -0.5738624711160822,
         0.1907547102962384,  -0.4091008258000316,   0.741670364350613
    };

    static const int s_taps = sizeof(s_c) / sizeof(s_c[0]);
    return SymplecticSum(s_taps, state, v, s_d, s_c);
}

template <typename T>
struct Integrator
{
    enum eIntegrator {EulerDirect = 0, RK4, RK4_38, Yoshida4, Yoshida6, Yoshida8, BlanesMoan, Hairer8, KahanLi8};
    static IntegrateResult I(const T &state, double v, eIntegrator e = KahanLi8)
    {
        typedef IntegrateResult(*PIntegrator)(const T&, double);
        static const PIntegrator fpa[] = {
            Integrate_EulerDirect<T>,
            Integrate_RK4<T>,
            Integrate_RK4_38<T>,
            Integrate_Yoshida4<T>,
            Integrate_Yoshida6<T>,
            Integrate_Yoshida8<T>,
            Integrate_BlanesMoan<T>,
            Integrate_Hairer8<T>,
            Integrate_KahanLi8<T>
        };

        return fpa[e](state, v);
    }
};

#endif // _GC_Integrator_h