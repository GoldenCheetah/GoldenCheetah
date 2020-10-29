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
#include "ErgFile.h"
#include "Integrator.h"

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
    Q_UNUSED(context)
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
    if (isNegative) v = 0;

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

    BicycleSimState Interpolate(double t) const
    {
        return BicycleSimState::Interpolate(m_s0, m_s1, m_t0, m_t1, t);
    }

    double CalcV(double v, double t) const 
    {
        // Linear interpolate the state info before sampling at t.
        BicycleSimState st = this->Interpolate(t);

        double newV = m_pb->V(st, v, t);

        return newV;
    }

    double dVdT(double v, double t) const
    {
        // Linear interpolate the state info before sampling at t.
        BicycleSimState st = this->Interpolate(t);

        double m = m_pb->DV(st, v);

        return m;
    }
};

// Returns power needed to maintain speed v against current simstate.
double Bicycle::WattsForV(const BicycleSimState &simState, double v) const
{
    double sl = simState.Slope() / 100; // 10% = 0.1
    const double CrV = 0.1;             // Coefficient for velocity - dependent dynamic rolling resistance, here approximated with 0.1
    double CrVn = CrV * cos(sl);        // Coefficient for the dynamic rolling resistance, normalized to road inclination; CrVn = CrV*cos(slope)

    double cosBeta = 1 / std::sqrt(sl*sl + 1); // cos(atan(sl))
    double sinBeta = sl * cosBeta;             // sin(atan(sl))

    double m = MassKG();
    double Frg = s_g0 * m * (m_constants.m_crr * cosBeta + sinBeta);

    double Hnn = simState.Altitude();   // Altitude above sea level (m).
    double Rho = AirDensity(Hnn, m_constants.m_T);
    double CdARho = m_constants.m_Cd * m_constants.m_A * Rho;

    const double W = 0;                 // Windspeed
    double wattsForV = m_constants.m_Cm * v * (CdARho / 2 * (v + W)*(v + W) + Frg + v * CrVn);

    return wattsForV;
}

// Return ms/s from current state, speed and impulse duration.
// This is linear application of watts over time to determine
// new speed.
double Bicycle::V(const BicycleSimState &simState,     // current sim state
                  double v,                            // current speed
                  double dt) const                     // duration that state was applied
{
    // Zero means no impulse, no change.
    //Negative means... we're not going there.
    if (dt <= 0.0) return v;

    double wattsForV = WattsForV(simState, v);
    double extraWatts = simState.Watts() - wattsForV;

    double j = KEFromV(v);               // state and speed to joules
    double newJ = j + (extraWatts * dt); // joules + (watts * time == more joules)
    double newV = VFromKE(newJ);         // new joules back to speed.

    return newV;
}

// returns m/s^2 for current state.
double Bicycle::DV(const BicycleSimState &simState,  // current sim state
                   double v) const                   // current speed
{
    double newV = V(simState, v, 1.);    // new speed with 1s impulse.
                                         
    return (newV - v);                   // implicit divide by 1s.
}

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
    IntegrateResult out = Integrator<MotionStatePair>::I(state, m_Velocity);

    // Done with old state, save new state for next query.
    m_Velocity = out.endPoint();
    this->m_state = nowState;

    // Return new velocity (in kmh) and distance tick (in km)
    return { MsToKmh(out.endPoint()), out.sum() / 1000 };
}
