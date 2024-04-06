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

#if !defined(_PHYSICSUTILITY_H)
#define _PHYSICSUTILITY_H

// These Zone 0 Pressure/Density Constants and equations are from wikipedia

static const double s_Pb = 101325;         // static pressure(Pa)
static const double s_Tb = 288.15;         // standard temperature(K)
static const double s_Lb = -0.0065;        // standard temperature lapse rate(K / m) in ISA
static const double s_h0 = 0;              // height above sea level(m)
static const double s_hb = 0;              // height at bottom of layer b(meters; e.g., h1 = 11 000 m)
static const double s_Rstar = 8.3144598;   // universal gas constant : 8.3144598 J / mol / K
static const double s_g0 = 9.80665;        // gravitational acceleration at sea level: 9.80665 m / s2
static const double s_M = 0.0289644;       // molar mass of Earth's air: 0.0289644 kg/mol
static const double s_Rspecific = 287.058; // Specific gas constant for dry air (J/(Kg*K))

double AirPressure(double altitudeInMeters); // returns pressure in pascals
double AirDensity(double altitudeInMeters, double temperatureInKelvin); // returns density in kg/(m^3)
double KmhToMs(double);
double MsToKmh(double);

double computeInstantSpeed(double weightKG,
                           double slope,
                           double altitude,         // altitude in meters
                           double pw,               // watts
                           double crr_in = 0.004,   // rolling resistance
                           double Cm_in = 1.0,      // powertrain loss
                           double Cd_in = 1.0,      // coefficient of drag
                           double A_in = 0.5,       // frontal area (m2)
                           double temp_K = 293.15); // temp in kelvin

#endif // _PHYSICSUTILITY_H