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
#include "MultiRegressionizer.h"
#include "Integrator.h"

const PolyFit<double>* GetAltitudeFit(unsigned maxOrder) {
    // Aerobic Power Adjustment for Altitude
    T_MultiRegressionizer<XYVector<double>> fit(0, maxOrder);

#if 1
    // Data from : Prediction of Critical Powerand W? in Hypoxia : Application to Work - Balance Modelling
    //             by Nathan E.Townsend1, David S.Nichols, Philip F.Skiba, Sebastien Racinais and Julien D.P�riard
    //
    // Data in paper doesn't exceed 4250m. Cubic equation in paper decreases slope after 5000m, which doesn't
    // make sense, so using best fit 3rd order rational instead. Our equation continues at constant slope from
    // 5000 - 7000m.
    //
    // Someday in the future we'll have more data and can simply add it to this table to automatically
    // generate a new fit.
    static const std::vector<std::tuple<double, double>> NathenTownsendAltitudeData =
    {
        std::make_tuple<double, double>(0      ,1        ),
        std::make_tuple<double, double>(0.01   ,1        ),
        std::make_tuple<double, double>(0.02   ,1        ),
        std::make_tuple<double, double>(0.03   ,1        ),
        std::make_tuple<double, double>(0.04   ,1        ),
        std::make_tuple<double, double>(0.05   ,1        ),
        std::make_tuple<double, double>(0.06   ,1        ),
        std::make_tuple<double, double>(0.07   ,1        ),
        std::make_tuple<double, double>(0.08   ,1        ),
        std::make_tuple<double, double>(0.09   ,1        ),
        std::make_tuple<double, double>(0.1    ,0.9996446),
        std::make_tuple<double, double>(0.2    ,0.9964848),
        std::make_tuple<double, double>(0.3    ,0.9930302),
        std::make_tuple<double, double>(0.4    ,0.9892904),
        std::make_tuple<double, double>(0.5    ,0.985275 ),
        std::make_tuple<double, double>(0.6    ,0.9809936),
        std::make_tuple<double, double>(0.7    ,0.9764558),
        std::make_tuple<double, double>(0.8    ,0.9716712),
        std::make_tuple<double, double>(0.9    ,0.9666494),
        std::make_tuple<double, double>(1      ,0.9614   ),
        std::make_tuple<double, double>(1.1    ,0.9559326),
        std::make_tuple<double, double>(1.2    ,0.9502568),
        std::make_tuple<double, double>(1.3    ,0.9443822),
        std::make_tuple<double, double>(1.4    ,0.9383184),
        std::make_tuple<double, double>(1.5    ,0.932075 ),
        std::make_tuple<double, double>(1.6    ,0.9256616),
        std::make_tuple<double, double>(1.7    ,0.9190878),
        std::make_tuple<double, double>(1.8    ,0.9123632),
        std::make_tuple<double, double>(1.9    ,0.9054974),
        std::make_tuple<double, double>(2      ,0.8985   ),
        std::make_tuple<double, double>(2.1    ,0.8913806),
        std::make_tuple<double, double>(2.2    ,0.8841488),
        std::make_tuple<double, double>(2.3    ,0.8768142),
        std::make_tuple<double, double>(2.4    ,0.8693864),
        std::make_tuple<double, double>(2.5    ,0.861875 ),
        std::make_tuple<double, double>(2.6    ,0.8542896),
        std::make_tuple<double, double>(2.7    ,0.8466398),
        std::make_tuple<double, double>(2.8    ,0.8389352),
        std::make_tuple<double, double>(2.9    ,0.8311854),
        std::make_tuple<double, double>(3      ,0.8234   ),
        std::make_tuple<double, double>(3.1    ,0.8155886),
        std::make_tuple<double, double>(3.2    ,0.8077608),
        std::make_tuple<double, double>(3.3    ,0.7999262),
        std::make_tuple<double, double>(3.4    ,0.7920944),
        std::make_tuple<double, double>(3.5    ,0.784275 ),
        std::make_tuple<double, double>(3.6    ,0.7764776),
        std::make_tuple<double, double>(3.7    ,0.7687118),
        std::make_tuple<double, double>(3.8    ,0.7609872),
        std::make_tuple<double, double>(3.9    ,0.7533134),
        std::make_tuple<double, double>(4      ,0.7457   ),
        std::make_tuple<double, double>(4.1    ,0.7381566),
        std::make_tuple<double, double>(4.2    ,0.7306928),

        // Below this line data is projected as linear.
        std::make_tuple<double, double>(4.3    ,0.722483 ),
        std::make_tuple<double, double>(4.4    ,0.714783 ),
        std::make_tuple<double, double>(4.5    ,0.707083 ),
        std::make_tuple<double, double>(4.6    ,0.699383 ),
        std::make_tuple<double, double>(4.7    ,0.691683 ),
        std::make_tuple<double, double>(4.8    ,0.683983 ),
        std::make_tuple<double, double>(4.9    ,0.676283 ),
        std::make_tuple<double, double>(5      ,0.668583 ),
        std::make_tuple<double, double>(5.1    ,0.660883 ),
        std::make_tuple<double, double>(5.2    ,0.653183 ),
        std::make_tuple<double, double>(5.3    ,0.645483 ),
        std::make_tuple<double, double>(5.4    ,0.637783 ),
        std::make_tuple<double, double>(5.5    ,0.630083 ),
        std::make_tuple<double, double>(5.6    ,0.622383 ),
        std::make_tuple<double, double>(5.7    ,0.614683 ),
        std::make_tuple<double, double>(5.8    ,0.606983 ),
        std::make_tuple<double, double>(5.9    ,0.599283 ),
        std::make_tuple<double, double>(6      ,0.591583 ),
        std::make_tuple<double, double>(6.1    ,0.583883 ),
        std::make_tuple<double, double>(6.2    ,0.576183 ),
        std::make_tuple<double, double>(6.3    ,0.568483 ),
        std::make_tuple<double, double>(6.4    ,0.560783 ),
        std::make_tuple<double, double>(6.5    ,0.553083 ),
        std::make_tuple<double, double>(6.6    ,0.545383 ),
        std::make_tuple<double, double>(6.7    ,0.537683 ),
        std::make_tuple<double, double>(6.8    ,0.529983 ),
        std::make_tuple<double, double>(6.9    ,0.522283 ),
        std::make_tuple<double, double>(7      ,0.514583 )
    };

    for (int i = 0; i < NathenTownsendAltitudeData.size(); i++) {
        const std::tuple<double, double>& v = NathenTownsendAltitudeData.at(i);

        fit.Push({ std::get<0>(v), std::get<1>(v) });
    }

    const PolyFit<double>* pf = fit.AsPolyFit();

#else
    // Closed form for current rational polynomial fit - avoids solving for least squares
    // For if this construction is ever on a hot path.
    const PolyFit<double>* pf = PolyFitGenerator::GetRationalPolyFit(
        { 1.0009048349382108, 0.37214585400953015, -0.039387977741974042, 0.00069424347367219726 },
        { 1., 0.38762014584456522 }, 1.);
#endif

    // Print table for debug or graphing
    //for (double a = 0.; a < 7.; a += 0.1) {
    //    qDebug() << a << "," << pf->Fit(a)<< ", " << pf->Slope(a);
    //}

    return pf;
}

double GetAltitudeAdjustmentFactor(double altitudeKM)
{
    // Third order rational fit to Townsend data.
    static const PolyFit<double>* s_pf3 = GetAltitudeFit(3);

    if (!s_pf3) return 1.;

    return s_pf3->Fit(altitudeKM);
}

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

void Bicycle::Init(bool fUseSimulatedHypoxia, BicycleConstants constants, double riderMassKG, double bicycleMassWithoutWheelsKG, BicycleWheel frontWheel, BicycleWheel rearWheel)
{
    m_constants = constants;
    m_riderMassKG = riderMassKG;
    m_bicycleMassWithoutWheelsKG = bicycleMassWithoutWheelsKG;
    m_frontWheel = frontWheel;
    m_rearWheel = rearWheel;

    // Precompute effective mass for KE calc here since it never changes.
    m_KEMass = (MassKG() + EquivalentMassKG());

    m_useSimulatedHypoxia = fUseSimulatedHypoxia;

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

    Init(true, constants, riderMassKG, bicycleMassWithoutWheelsKG, frontWheel, rearWheel);
}

Bicycle::Bicycle(Context* context)
{
    this->Reset(context);
}

// Reset - gather new data from athlete settings.
void Bicycle::Reset(Context* context)
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

    const double actualTrainerAltitudeM    = simBikeValues[SimBicyclePage::ActualTrainerAltitudeM];

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
        simBikeValues[SimBicyclePage::Tk],
        1./GetAltitudeAdjustmentFactor(actualTrainerAltitudeM / 1000.));

    bool useSimulatedHypoxia = appsettings->value(NULL, TRAIN_USESIMULATEDHYPOXIA, false).toBool();

    Init(useSimulatedHypoxia, constants, riderMassKG, bicycleMassWithoutWheelsG / 1000., frontWheel, rearWheel);
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
double Bicycle::FilterWattIncrease(double watts, double altitudeMeters)
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

    // adjust power due to altitude
    if (m_useSimulatedHypoxia) {
        double adjustmentFactor = GetAltitudeAdjustmentFactor(altitudeMeters / 1000.);
        
        double newWatts = watts * adjustmentFactor * m_constants.m_AltitudeCorrectionFactor;

        watts = newWatts;
    }

    return watts;
}

// Compute new speed from state and time duration since last sample.
SpeedDistance
Bicycle::SampleSpeed(BicycleSimState &nowState)
{
    // Detect and filter obvious power spikes.
    nowState.Watts() = FilterWattIncrease(nowState.Watts(), nowState.Altitude());

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
