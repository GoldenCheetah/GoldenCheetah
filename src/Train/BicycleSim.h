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

#if !defined(BICYCLESIM_H)
#define BICYCLESIM_H

#include <chrono>

#include "RealtimeData.h"
#include "PhysicsUtility.h"
#include "ErgFile.h"

class BicycleWheel
{
    double m_massKG, m_I, m_equivalentMassKG;

public:

    BicycleWheel(double outerR,          // outer radius in meters (for circumference)
                 double innerR,          // inner rim radius
                 double weightKG,        // total wheel weight
                 double centerWeightKG,
                 double spokeCount,
                 double spokeMass);

    BicycleWheel() {}

    double KEMass()           const { return MassKG() + EquivalentMassKG(); }
    double MassKG()           const { return m_massKG; }
    double EquivalentMassKG() const { return m_equivalentMassKG; }
    double I()                const { return m_I; }
};

struct BicycleConstants
{
    double m_crr;                      // coefficient rolling resistance
    double m_Cm;                       // coefficient of powertrain loss
    double m_Cd;                       // coefficient of drag
    double m_A;                        // frontal area (m2)
    double m_T;                        // temp in kelvin
    double m_AltitudeCorrectionFactor; // inverse of altitude cp penalty

    BicycleConstants() :
        m_crr(0.004),
        m_Cm(1.0),
        m_Cd(1.0 - 0.0045),
        m_A(0.5),
        m_T(293.15),
        m_AltitudeCorrectionFactor(1.)
    {}

    BicycleConstants(double crr, double Cm, double Cd, double A, double T, double acf) :
        m_crr(crr), m_Cm(Cm), m_Cd(Cd), m_A(A), m_T(T), m_AltitudeCorrectionFactor(acf)
    {}
};

struct SpeedDistance
{
    double v, d;
};

class BicycleSimState
{
    enum Names { eWatts = 0, eAltitude, eSlope, eLastName };

    double v[eLastName];

public:

    double Watts()   const { return (v[eWatts]); }
    double Altitude()const { return (v[eAltitude]); }
    double Slope()   const { return (v[eSlope]); }

    double& Watts()        { return (v[eWatts]); }
    double& Altitude()     { return (v[eAltitude]); }
    double& Slope()        { return (v[eSlope]); }

    void Clear()
    {
        for (int i = 0; i < eLastName; i++)
        {
            v[i] = 0.0;
        }
    }

    BicycleSimState() {}
    BicycleSimState(const RealtimeData &rtData)
    {
        Clear();
        this->Altitude() = rtData.getAltitude();
        this->Slope()    = rtData.getSlope();
        this->Watts()    = rtData.getWatts();
    }

    BicycleSimState(double watts, double slope, double altitude)
    {
        Clear();
        this->Altitude() = altitude;
        this->Slope()    = slope;
        this->Watts()    = watts;
    }

    // Linear interpolate all fields to new sim state.
    static BicycleSimState Interpolate(const BicycleSimState &v0, const BicycleSimState &v1, double t0, double t1, double t)
    {
        if (t == t0) return v0;
        if (t == t1) return v1;

        double p = (t - t0) / (t1 - t0);

        BicycleSimState r;
        for (int i = 0; i < eLastName; i++)
        {
            r.v[i] = (v1.v[i] - v0.v[i]) * p + v0.v[i];
        }

        return r;
    }
};

class Bicycle
{
    // Invariant bicycle and rider properties.
    BicycleConstants m_constants;
    BicycleWheel m_frontWheel, m_rearWheel;
    double m_bicycleMassWithoutWheelsKG, m_riderMassKG;
    double m_KEMass; // effective mass used for computing kinetic energy
    bool m_useSimulatedHypoxia;

    // Dynamic data persisted for modelling momentum.
    BicycleSimState m_state;
    double          m_Velocity; // velocity in meters per second.

    bool            m_isFirstSample;
    std::chrono::high_resolution_clock::time_point m_prevSampleTime;

    void Init(bool fUseSimulateHypoxia, BicycleConstants constants, double riderWeightKG, double bicycleMassWithoutWheelsKG, BicycleWheel frontWheel, BicycleWheel rearWheel);

    double SampleDT(); // Gather sample time stamp and return dt (in seconds)

     // Reduce power spikes by limiting power growth per sample period, also apply altitude penalty.
    double FilterWattIncrease(double watts, double altitude);

public:

    Bicycle(Context* context, BicycleConstants constants, double riderWeightKG, double bicycleMassWithoutWheelsKG, BicycleWheel frontWheel, BicycleWheel rearWheel);
    Bicycle(Context* context);
    SpeedDistance SampleSpeed(BicycleSimState &newState);

    void Reset(Context* context);

    double MassKG() const;                   // Actual mass of bike and rider.
    double EquivalentMassKG() const;         // Additional mass due to rotational inertia
    double KEMass() const;                   // Total effective kinetic mass
    double RollingResistance() const { return m_constants.m_crr; }
    double WindResistance(double altitude) const { return m_constants.m_Cd * m_constants.m_A * AirDensity(altitude, m_constants.m_T); }

    const BicycleWheel &FrontWheel() const { return m_frontWheel; }
    const BicycleWheel &RearWheel() const { return m_rearWheel; }

    // Reset timer. This forces next sample to use 0.1s as its dt.
    void resettimer()
    {
        m_isFirstSample = true;
    }

    // Restart sim. Reset time and all history.
    void clear()
    {
        m_Velocity = 0.0;
        resettimer();
        this->m_state.Clear();
    }

    double KEFromV(double v) const;  // Compute kinetic energy of bike and rider (J) at speed v (in m/s)
    double VFromKE(double ke) const; // Compute velocity (m/s) from kinetic energy of bike and rider (J)

    double WattsForV(const BicycleSimState &simState, double v) const;

    // Compute new velocity in m/s after simState applied for dt seconds.
    double V(const BicycleSimState &simState,   // current sim state
             double v,                          // current velocity (m/s)
             double dt) const;                  // impulse duration

    double DV(const BicycleSimState &simState,  // current sim state
             double v) const;                        // current velocity

};


#endif // BICYCLESIM_H
