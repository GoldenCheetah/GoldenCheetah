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

#include <cmath>
#include "PhysicsUtility.h"
#include "BlinnSolver.h"

#define MATHCONST_PI 		    3.141592653589793238462643383279502884L /* pi */

// These Zone 0 Pressure/Density Constants and equations are from wikipedia
double AirPressure(double altitudeInMeters)
{
    double p = s_Pb * pow((s_Tb / (s_Tb + s_Lb * (altitudeInMeters - s_hb))), 1 + (s_g0 * s_M) / (s_Rstar * s_Lb));

    return p; // pascals
}

double AirDensity(double altitudeInMeters, double temperatureInKelvin)
{
    double Rho = AirPressure(altitudeInMeters) / (s_Rspecific*temperatureInKelvin);

    return Rho; // kg/(m^3)
}

double KmhToMs(double kmh)
{
    return kmh * (1000.0 / (60.0 * 60.0));
}

double MsToKmh(double ms)
{
    return ms * ((60.0 * 60.0) / 1000.0);
}

// Function to compute the sustainable speed in KM/H from input params and a
// bunch of real world constants.
// Relies upon cubic root finder.
// The bicycle related equations came from http://kreuzotter.de/english/espeed.htm,
// The pressure and density equations came from wikipedia.
double computeInstantSpeed(double weightKG,
                           double slope,
                           double altitude, // altitude in meters
                           double pw,       // watts
                           double crr,      // rolling resistance
                           double Cm,       // powertrain loss
                           double Cd,       // coefficient of drag
                           double A,        // frontal area (m2)
                           double T)        // temp in kelvin
{
    double vs = 0; // Return value

    double sl = slope / 100;          // 10% = 0.1

    // Compute velocity using equations from http://kreuzotter.de/english/espeed.htm
    const double CrV = 0.1;           // Coefficient for velocity - dependent dynamic rolling resistance, here approximated with 0.1
    double CrVn = CrV * cos(sl);      // Coefficient for the dynamic rolling resistance, normalized to road inclination; CrVn = CrV*cos(slope)
    const double W = 0;               // Windspeed
    double Hnn = altitude;            // Altitude above sea level (m).
    double m = weightKG;

    double Rho = AirDensity(Hnn, T);

    double cosBeta = 1 / std::sqrt(sl*sl + 1); // cos(atan(sl))
    double sinBeta = sl * cosBeta;             // sin(atan(sl))

    double Frg = s_g0 * m * (crr * cosBeta + sinBeta);

    // Home Rolled cubic solver:
    // v^3 + v^2 * 2 * (W + CrVn/(Cd *A*Rho)) + V*(W^2 + (2*Frg/(Cd*A*Rho))) - (2*P)/(Cm*Cd*A*Rho) = 0;
    //
    //  A = 1 (monic)
    //  B = 2 * (W + CrVn / (Cd*A*Rho))
    //  C = (W ^ 2 + (2 * Frg / (Cd*A*Rho)))
    //  D = -(2 * P) / (Cm*Cd*A*Rho)

    double CdARho = Cd * A * Rho;

    double  a = 1;
    double  b = 2 * (W + CrVn / (CdARho));
    double  c = (W * W + (2 * Frg / (CdARho)));
    double  d = -(2 * pw) / (Cm * CdARho);

    vs = 0;
    {
        Roots r = BlinnCubicSolver(a, b, c, d);

        // Iterate across roots for one that provides a positive velocity,
        // otherwise try and choose a positive one that is closest to zero,
        // finally tolerate a negative one thats closest to zero.
        for (unsigned u = 0; u < r.resultcount(); u++) {
            double t = r.result(u).x / r.result(u).w;
            if (vs == 0) vs = t;
            else if (vs > 0) {
                if (t > 0 && t < vs) vs = t;
            } else {
                if (t > vs) vs = t;
            }
        }
    }

#if 0
    // Now to test we run the equation the other way and compute power needed to sustain computed speed.
    // A properly computed v should very closely predict the power needed to sustain that speed.
    double testPw = Cm * vs * (Cd * A * Rho / 2 * (vs + W)*(vs + W) + Frg + vs * CrVn);
    double delta = fabs(testPw - pw);
#endif

    vs *= 3.6; // m/s to km/h.

    return vs;
}
