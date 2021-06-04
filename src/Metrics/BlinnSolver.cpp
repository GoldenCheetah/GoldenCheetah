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
#include <algorithm>
#include <limits>
#include "BlinnSolver.h"

int GetExponent(double a) {
    int exp;
    std::frexp(a, &exp);
    return exp;
}

template <typename T>
int MaxExponent(T a) {
    return GetExponent(a);
}

template <typename T, typename ... Args>
int MaxExponent(T a, Args ... args) {
    return std::max(MaxExponent(args...), GetExponent(a));
}

// Returns true if value is so small compared to args that it is effectively zero.
// Very important that near-zero values are detected. Zero is defined relative to how large
// the use-expression's other operands are.
// The solver will be subtracting and losing precision. If one term is near zero the result
// will be noise since result has no effective precision. Much better results if you disregard
// a term with near zero coefficient and instead use the next lower order solver.
template <unsigned T_MantissaBitsNeeded, typename ... Args>
bool RangedZeroTest(double value, Args ... args) {
    const static int s_ExpLimit = std::numeric_limits<double>::digits - T_MantissaBitsNeeded;

    // Easy yes if value is actually zero.
    if (value == 0.) return true;

    int vExp = GetExponent(value);
    int maxExp = MaxExponent(args...);
    if (maxExp - vExp > s_ExpLimit)
        return true;

    return false;
}

// Define zero as 10 or fewer bits of mantissa precision shared between value and operand(s).
// Less than that we have no precision. Relative size must differs by 2^(53-bits)
const static int s_MantissaBitsNeeded = 10;

template <typename ... Args>
bool IsZero(double value, Args...args) {
    return RangedZeroTest<s_MantissaBitsNeeded>(value, args...);
}

bool IsZero2(double value, double arg) {
    return IsZero(value, arg);
}

// 0 == A*x + B
Roots LinearSolver(double A, double B) {
    if (IsZero(A, B)) {
        if (B == 0) return Roots({ 0,1 }); // choose 0
        return Roots(); // no root
    }

    return Roots({ -B / A, 1 });
}

// 0 == A*x^2 + B*x + C
Roots QuadraticSolver(double A, double B, double C) {

    // If A is zero then linear.
    if (IsZero(A, B, C)) {
        return LinearSolver(B, C);
    }

    // Make monic.
    B /= A;
    C /= A;
    A = 1;

    // No C term means a quick and accurate factor.
    if (IsZero(C, B)) {
        return Roots({ B, 1 }, { -B, 1 });
    }

    // Double root case.
    double det2 = B * B - 4 * C;
    if (IsZero(det2, B * B, 4 * C)) {
        return Roots({ -B / 2, 1 });
    }

    // Negative determinate means no roots.
    if (det2 < 0) {
        return Roots();
    }

    // Otherwise standard double roots.
    double r0 = (B + std::copysign(std::sqrt(det2), B)) / -2.;
    double r1 = C / r0;

    return Roots({ r0, 1 }, { r1, 1 });
}

// Cubic solver as described by James F. Blinn's EPIC paper:
// 'How to Solve A Cubic Equation', all 5 parts are awesome
// but part 5 provides the algorithmic conclusion that I used
// here. Highly recommended reading. Consider those papers an
// 85 page guide to what is going on here.
//
// https://courses.cs.washington.edu/courses/cse590b/13au/lecture_notes/solvecubic_p5.pdf 
//
// Note that Blinn's paper explores the development of an algorithm
// for solving a 2 dimensional cubic of x, w:
// 
//     A x^3 + 3 B x^2 w + 3 C x w^2 + D w^3 = 0
//
// This function is fed coefficients A,B,C,D, and outputs either 1
// or 3 vectors <x, w> that describe the roots.
// 
// To obtain solution for x with w == 1 simply divide
// x by w.
Roots
BlinnCubicSolver(double A, double B, double C, double D) {
    // Take care to detect near zero A coefficient since
    // it can destroy precision and cause solver to produce
    // the same result over and over. Much better to use 
    // quadratic solver.
    if (IsZero(A, B, C, D)) {
        return QuadraticSolver(B, C, D);
    }

    // Make Monic.
    B = B / A;
    C = C / A;
    D = D / A;
    A = 1;

    // Into 'Blinn Form' where center 2 coefficients have an implicit mul by 3.
    B /= 3;
    C /= 3;

    // Hessian.
    double d1 = A * C - B * B;
    double d2 = A * D - B * C;
    double d3 = B * D - C * C;

    // Determinant of Hessian
    double determinant = 4 * d1 * d3 - d2 * d2;

    if (determinant <= 0) {

        // There is a very cool picture of the A/D/zero precision on
        // page 15 of How To Solve A Cubic Equation - Part 2, which
        // explores and justifies the approach used below.
        // The two depression paths <A, D> are the two ways
        // that the original expression can be rewritten so the
        // B term is 0.
        double At, Cbar, Dbar;
        bool fUseApproachAD = B * B*B * D >= A * C*C*C;
        if (fUseApproachAD) {
            // Depress params for algorithm 'A/D'
            At = A;
            Cbar = d1;
            Dbar = -2 * B * d1 + A * d2;
        } else {
            // Depress params for algorithm 'D/A'
            At = D;
            Cbar = d3;
            Dbar = -D * d2 + 2 * C * d3;;
        }

        // Solving the Quadratic Really Properly
        double T0 = -copysign(fabs(At)*sqrt(-determinant), Dbar);
        double T1 = -Dbar + T0;
        double p = cbrt(T1 / 2);

        // That rare instance where floating point equality is actually
        // useful and correct. If the quadratic shows a double root
        // then p and q vary only by sign.
        double q;
        if (T0 == T1) {
            q = -p;
        } else {
            q = -Cbar / p;
        }

        // Add the Roots Nicely
        double xt1;
        if (Cbar <= 0) {
            xt1 = p + q;
        } else {
            xt1 = -Dbar / (p*p + q * q + Cbar);
        }

        // Un-depress.
        double x_1, w_1;
        if (fUseApproachAD) {
            x_1 = xt1 - B;
            w_1 = A;
        } else {
            x_1 = -D;
            w_1 = xt1 + C;
        }

        return Roots({ x_1, w_1 });
    } else {
        // Positive determinant means there are 3 roots: L, S, M.

        // L: Find root using 'A' algorithm (largest root)
        double CbarA = d1;
        double DbarA = -2 * B*d1 + A * d2;

        // LaPorte approach to finding theta
        double thetaA = fabs(atan2(A*sqrt(determinant), -DbarA)) / 3.;

        // Un-depress
        double xt1A = 2 * sqrt(-CbarA) * cos(thetaA);
        double xt3A = 2 * sqrt(-CbarA) * (cos(thetaA) / -2. - (sqrt(3.) / 2.)*sin(thetaA));

        double xtL;
        if (xt1A + xt3A > 2 * B) {
            xtL = xt1A;
        } else {
            xtL = xt3A;
        }

        Root2D L = { xtL - B, A };

        // S: Find root using D algorithm (smallest root)
        double CbarD = d3;
        double DbarD = -D * d2 + 2 * C * d3;

        // LaPorte approach to finding theta
        double thetaD = fabs(atan2(D*sqrt(determinant), -DbarD)) / 3.;

        // Un-depress
        double xt1D = 2 * sqrt(-CbarD)*cos(thetaD);
        double xt3D = 2 * sqrt(-CbarD)*(cos(thetaD) / -2. - (sqrt(3.) / 2.)*sin(thetaD));

        double xtS;
        if (xt1D + xt3D < 2 * C) {
            xtS = xt1D;
        } else {
            xtS = xt3D;
        }

        Root2D S = { -D, xtS + C };

        // M: Derive 3rd root vector from the two previously discovered root vectors.
        double E = L.w * S.w;
        double F = (-L.x * S.w) - (L.w * S.x);
        double G = L.x * S.x;

        Root2D M = { C * F - B * G, -B * F + C * E };

        return Roots(L, S, M);
    }
}
