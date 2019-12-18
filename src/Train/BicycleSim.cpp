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

#include "BicycleSim.h"
#include "Settings.h"
#include "Pages.h"

BicycleWheel::BicycleWheel(double outerR,         // outer radius in meters (for circumference)
                           double innerR,         // inner rim radius
                           double massKG,         // total wheel mass
                           double centerMassKG,   // Everything in center of wheel: hub and rotors and skewers...
                           double spokeCount,
                           double spokeMassKG)    // spoke and nipples and washers...
{
    m_massKG = massKG;

    double spokeLengthM = innerR;
    double spokeI = (1.0 / 3.0) * spokeMassKG * spokeLengthM * spokeLengthM;
    double totalSpokeI = spokeI * spokeCount;

    double totalSpokeMassKG = spokeMassKG * spokeCount;
    double rimTireSealantMassKG = massKG - (centerMassKG + totalSpokeMassKG);

    m_I = 0.5 * rimTireSealantMassKG * (innerR * innerR + outerR * outerR) + totalSpokeI;

    m_equivalentMassKG = m_I / (outerR * outerR);
}

void Bicycle::Init(BicycleConstants constants, double riderMassKG, double bicycleMassWithoutWheelsKG, BicycleWheel frontWheel, BicycleWheel rearWheel)
{
    m_constants = constants;
    m_riderMassKG = riderMassKG;
    m_bicycleMassWithoutWheelsKG = bicycleMassWithoutWheelsKG;
    m_frontWheel = frontWheel;
    m_rearWheel = rearWheel;

    // Precompute effective mass for KE calc here since it never changes.
    m_KEMass = (MassKG() + EquivalentMassKG());

    if (m_KEMass < 0) // Should be impossible but bad if it happens.
    {
        m_KEMass = 100;
    }

    // Init simulation info.
    this->clear();
}

Bicycle::Bicycle(Context *context, BicycleConstants constants, double riderMassKG, double bicycleMassWithoutWheelsKG, BicycleWheel frontWheel, BicycleWheel rearWheel)
{
    context;
    Init(constants, riderMassKG, bicycleMassWithoutWheelsKG, frontWheel, rearWheel);
}

Bicycle::Bicycle(Context* context)
{
    // Tolerate NULL context since used by NullTrainer for testing.

    double riderMassKG = (context != NULL)
        ? context->athlete->getWeight(QDate::currentDate())
        : 80;

    double simBikeValues[SimBicyclePage::BicycleParts::LastPart];
    for (int i = 0; i < SimBicyclePage::BicycleParts::LastPart; i++)
    {
        simBikeValues[i] = SimBicyclePage::GetBicyclePartValue(context, i);
    }

    const double bicycleMassWithoutWheelsG = simBikeValues[SimBicyclePage::BicycleWithoutWheelsG];
    const double bareFrontWheelG           = simBikeValues[SimBicyclePage::FrontWheelG          ];
    const double frontSpokeCount           = simBikeValues[SimBicyclePage::FrontSpokeCount      ];
    const double frontSpokeNippleG         = simBikeValues[SimBicyclePage::FrontSpokeNippleG    ];
    const double frontWheelOuterRadiusM    = simBikeValues[SimBicyclePage::FrontOuterRadiusM    ];
    const double frontRimInnerRadiusM      = simBikeValues[SimBicyclePage::FrontRimInnerRadiusM ];
    const double frontRimG                 = simBikeValues[SimBicyclePage::FrontRimG            ];
    const double frontRotorG               = simBikeValues[SimBicyclePage::FrontRotorG          ];
    const double frontSkewerG              = simBikeValues[SimBicyclePage::FrontSkewerG         ];
    const double frontTireG                = simBikeValues[SimBicyclePage::FrontTireG           ];
    const double frontTubeOrSealantG       = simBikeValues[SimBicyclePage::FrontTubeSealantG    ];
    const double bareRearWheelG            = simBikeValues[SimBicyclePage::RearWheelG           ];
    const double rearSpokeCount            = simBikeValues[SimBicyclePage::RearSpokeCount       ];
    const double rearSpokeNippleG          = simBikeValues[SimBicyclePage::RearSpokeNippleG     ];
    const double rearWheelOuterRadiusM     = simBikeValues[SimBicyclePage::RearOuterRadiusM     ];
    const double rearRimInnerRadiusM       = simBikeValues[SimBicyclePage::RearRimInnerRadiusM  ];
    const double rearRimG                  = simBikeValues[SimBicyclePage::RearRimG             ];
    const double rearRotorG                = simBikeValues[SimBicyclePage::RearRotorG           ];
    const double rearSkewerG               = simBikeValues[SimBicyclePage::RearSkewerG          ];
    const double rearTireG                 = simBikeValues[SimBicyclePage::RearTireG            ];
    const double rearTubeOrSealantG        = simBikeValues[SimBicyclePage::RearTubeSealantG     ];
    const double cassetteG                 = simBikeValues[SimBicyclePage::CassetteG            ];

    const double frontWheelG = bareFrontWheelG + frontRotorG + frontSkewerG + frontTireG + frontTubeOrSealantG;
    const double frontWheelRotatingG = frontRimG + frontTireG + frontTubeOrSealantG + (frontSpokeCount * frontSpokeNippleG);
    const double frontWheelCenterG = frontWheelG - frontWheelRotatingG;

    BicycleWheel frontWheel(frontWheelOuterRadiusM, frontRimInnerRadiusM, frontWheelG / 1000, frontWheelCenterG /1000, frontSpokeCount, frontSpokeNippleG/1000);

    const double rearWheelG = bareRearWheelG + cassetteG + rearRotorG + rearSkewerG + rearTireG + rearTubeOrSealantG;
    const double rearWheelRotatingG = rearRimG + rearTireG + rearTubeOrSealantG + (rearSpokeCount * rearSpokeNippleG);
    const double rearWheelCenterG = rearWheelG - rearWheelRotatingG;

    BicycleWheel rearWheel (rearWheelOuterRadiusM,  rearRimInnerRadiusM,  rearWheelG / 1000,  rearWheelCenterG / 1000,  rearSpokeCount,  rearSpokeNippleG/1000);

    BicycleConstants constants(
        simBikeValues[SimBicyclePage::CRR],
        simBikeValues[SimBicyclePage::Cm],
        simBikeValues[SimBicyclePage::Cd],
        simBikeValues[SimBicyclePage::Am2],
        simBikeValues[SimBicyclePage::Tk]);

    Init(constants, riderMassKG, bicycleMassWithoutWheelsG / 1000., frontWheel, rearWheel);
}

double Bicycle::MassKG() const
{
    return m_riderMassKG + m_bicycleMassWithoutWheelsKG + m_frontWheel.MassKG() + m_rearWheel.MassKG();
}

double Bicycle::EquivalentMassKG() const
{
    return m_frontWheel.EquivalentMassKG() + m_rearWheel.EquivalentMassKG();
}

double Bicycle::KEMass() const
{
    return m_KEMass;
}

double Bicycle::KEFromV(double ms) const
{
    double ke = KEMass() * ms * ms / 2;

    return ke;
}

double Bicycle::VFromKE(double ke) const
{
    // Virtual rider has virtual brakes so we don't roll backwards
    // (and so we don't need to support complex speed...)
    bool isNegative = ke < 0;
    ke = fabs(ke);
    double v = sqrt(2 * ke / KEMass());
    if (isNegative) v = -v;

    return v;
}

struct MotionStatePair
{
    const Bicycle * m_pb;
    BicycleSimState m_s0;
    BicycleSimState m_s1;
    double m_t0; // time of s0
    double m_t1; // time of s1

    MotionStatePair(const Bicycle * b, const BicycleSimState &s0, double t0, const BicycleSimState &s1, double t1) :
        m_pb(b), m_s0(s0), m_s1(s1), m_t0(t0), m_t1(t1)
    {}

    double T0() const { return m_t0; }
    double T1() const { return m_t1; }
    double DT() const { return m_t1 - m_t0; }
    double CalcV(const BicycleSimState& s, double v, double t) const { return m_pb->V(s, v, t); }

    BicycleSimState Interpolate(double t) const
    {
        return BicycleSimState::Interpolate(m_s0, m_s1, m_t0, m_t1, t);
    }

    double dVdT(double v, double t) const
    {
        static const double s_step = 0.01;

        // Linear interpolate the state info before sampling velocities at t and t+step.
        BicycleSimState tn0 = this->Interpolate(t);
        double v0 = CalcV(tn0, v, t);

        BicycleSimState tn1 = this->Interpolate(t + s_step);
        double v1 = CalcV(tn1, v, t + s_step);

        double m = (v1 - v0) / s_step;

        return m;
    }
};

// Compute new velocity from current state and impulse duration.
double Bicycle::V(const BicycleSimState &simState,     // current sim state
                  double v,                            // current velocity
                  double dt) const                     // duration that state was applied
{
    if (dt == 0.0) return v;

    double sl = simState.Slope() / 100; // 10% = 0.1
    const double CrV = 0.1;             // Coefficient for velocity - dependent dynamic rolling resistance, here approximated with 0.1
    double CrVn = CrV * cos(sl);        // Coefficient for the dynamic rolling resistance, normalized to road inclination; CrVn = CrV*cos(slope)
    double Beta = atan(sl);
    double cosBeta = cos(Beta);
    double sinBeta = sin(Beta);
    double m = MassKG();
    double Frg = s_g0 * m * (m_constants.m_crr * cosBeta + sinBeta);

    double Hnn = simState.Altitude();   // Altitude above sea level (m).
    double Rho = AirDensity(Hnn, m_constants.m_T);
    double CdARho = m_constants.m_Cd * m_constants.m_A * Rho;

    const double W = 0;                 // Windspeed
    double wattsForV = m_constants.m_Cm * v * (CdARho / 2 * (v + W)*(v + W) + Frg + v * CrVn);
    double extraWatts = simState.Watts() - wattsForV;

    double j = KEFromV(v);
    double newJ = j + (extraWatts * dt);
    double newV = VFromKE(newJ);

    return newV;
}

template <typename T>
SpeedDistance
Integrate_EulerDirect(const T &state, double v)
{
    double dt = state.DT();
    double vt = state.CalcV(state.m_s1, v, dt);
    SpeedDistance ret = { vt, vt * dt};
    return ret;
}

// Classic Runge-Kutta 4 as per Wikipedia
template <typename T>
SpeedDistance
Integrate_RK4(const T &state, double v)
{
    double t0 = state.T0();
    double dt = state.DT();

    double m1 = state.dVdT(v, t0);
    double m2 = state.dVdT(v + (m1 * dt / 2), t0 + (dt / 2));
    double m3 = state.dVdT(v + (m2 * dt / 2), t0 + (dt / 2));
    double m4 = state.dVdT(v + (m3 * dt), dt);

    double m = (m1 + 2 * m2 + 2 * m3 + m4) / 6;

    double vt = v + m * dt;

    SpeedDistance ret = { vt, vt * dt };

    return ret;
}

// 3/8 Rule Runge-Kutta 4 as per Wikipedia
template <typename T>
SpeedDistance
Integrate_RK4_38(const T &state, double v)
{
    double t0 = state.T0();
    double dt = state.DT();

    double m1 = state.dVdT(v, t0);
    double m2 = state.dVdT(v + (m1 * dt / 3), t0 + dt / 3);
    double m3 = state.dVdT(v + (m1 * dt / -3) + (m2 * dt), t0 + (2 * dt / 3));
    double m4 = state.dVdT(v + (m1 * dt) + (m2 * -dt) + (m3 * dt), dt);

    double m = (m1 + (3 * m2) + (3 * m3) + m4) / 8;

    double vt = v + m * dt;

    SpeedDistance ret = { vt,  dt * vt };

    return ret;
}

//#define TEST_COEFFICIENTS
#if defined(TEST_COEFFICIENTS)
// Function for testing integrator coefficients.
//
// Each ordered sum of 'c' terms is multiplied by dt
// to choose sample time. If this time falls out of range
// t0 to dt then derivative cannot be trusted.
//
// Function computes all the scan locations. Returns
// true if coefficients will cause out of range sample,
// otherwise false.
//
// Don't use coefficients that fail this test.
//
bool TestCoeffs(const double *pc, int terms)
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
SpeedDistance SymplecticSum(int taps, const T& state, double v, const double * d, const double * c)
{
#if defined(TEST_COEFFICIENTS)
    bool tc = TestCoeffs(c, taps);
    ASSERT(tc);
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

    SpeedDistance ret = { v, distanceTick };

    return ret;
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
SpeedDistance Integrate_Yoshida4(const T &state, double v)
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
SpeedDistance Integrate_Yoshida6(const T &state, double v)
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
// Dont use this one. Its pretty bad for this use because all the 8th order
// Yoshida coefficients sample way outside the range t0 to dt.
template <typename T>
SpeedDistance Integrate_Yoshida8(const T &state, double v)
{
    // Yoshida Solution A, B, C
    static const double s_WA[] = {-1.61582374150097,-2.44699182370524,-0.00716989419708120, 2.44002732616735, 0.157739928123617, 1.82020630970714, 1.04242620869991 };
    static const double s_WB[] = {-0.00169248587770116, 2.89195744315849, 0.00378039588360192, -2.89688250328827, 2.89105148970595, -2.33864815101035, 1.48819229202922 };
    static const double s_WC[] = { 0.311790812418427,-1.55946803821447,-1.6789692825964, 1.66335809963315,-1.06458714789183, 1.36934946416871, 0.629030650210433 };

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
SpeedDistance Integrate_BlanesMoan(const T &state, double v)
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
SpeedDistance Integrate_Hairer8(const T &state, double v)
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
template <typename T>
SpeedDistance Integrate_KahanLi8(const T &state, double v)
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
    static SpeedDistance I(const T &state, double v, eIntegrator e = Hairer8)
    {
        typedef SpeedDistance(*PIntegrator)(const T&, double);
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

double Bicycle::SampleDT()
{
    std::chrono::high_resolution_clock::time_point newSampleTime = std::chrono::high_resolution_clock::now();

    // Compute seconds since last sample.
    static const std::chrono::duration<double> dt01(0.01);
    std::chrono::duration<double> dt = dt01;
    if (m_isFirstSample) m_isFirstSample = false;
    else dt = std::chrono::duration_cast<std::chrono::duration<double>>(newSampleTime - m_prevSampleTime);

    // If time is more than a second then something is amiss or we're running in debugger.
    // In any case the illusion of continuous time has been shattered. Reset dt to a nice small value.
    // Nothing good happens if you interpolate with huge time values.
    if (dt.count() > 1.0) dt = dt01;

    m_prevSampleTime = newSampleTime;

    return dt.count();
}

// Detect and filter obvious power spikes.
double Bicycle::FilterWattIncrease(double watts)
{
    // Not possible to gain an imperial ton of watts in a single sample.
    static const double s_imperialWattTon = 800.;
    static const double s_wattGrowthLimit = 200.;
    if (watts - m_state.Watts() > s_imperialWattTon)
    {
        // Is important to filter these spikes because they
        // lead to crazy velocity spikes which eat up travelled
        // distance - no free watts.
        watts = m_state.Watts() + s_wattGrowthLimit;
    }
    return watts;
}

// Compute new speed from state and time duration since last sample.
SpeedDistance
Bicycle::SampleSpeed(BicycleSimState &nowState)
{
    // Detect and filter obvious power spikes.
    nowState.Watts() = FilterWattIncrease(nowState.Watts());

    // Record current time and return dt since last sample.
    double dt = SampleDT();

    // Compute new speed.
    MotionStatePair state(this,          // BicycleSim object (for accessing methods and constants)
                          this->m_state, // previous tick state
                          0,             // time of previous tick (0)
                          nowState,      // current state
                          dt);           // time of current state (dt)

    // Integrate speed from previous state to current state.
    SpeedDistance out = Integrator<MotionStatePair>::I(state, m_Velocity);

    // Done with old state, save new state for next query.
    m_Velocity = out.v;
    this->m_state = nowState;

    // Return new velocity (in kmh) and distance tick (in km)
    return { MsToKmh(out.v), out.d / 1000 };
}
