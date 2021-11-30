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

#if !defined(_BLINNSOLVER_H)
#define _BLINNSOLVER_H

// Is value zero wrt scale of arg? Returns true if value is indistinguishable
// from zero with respect to magnitude precision of arg. Put another way,
// is arg + value ~= arg?
bool IsZero2(double value, double arg);

// Holds up to 3 roots.
template <typename T> struct T_resultstruct
{
private:
    T r[3];
    unsigned count;

public:

    T_resultstruct() : count(0) {}
    T_resultstruct(T r0)             { count = 1; r[0] = r0; }
    T_resultstruct(T r0, T r1)       { count = 2; r[0] = r0; r[1] = r1; }
    T_resultstruct(T r0, T r1, T r2) { count = 3; r[0] = r0; r[1] = r1; r[2] = r2; }

    unsigned resultcount() const { return count; }

    T result(unsigned u) const {
        if (u < count)
            return r[u];

        return T();
    }
};

struct Root2D {
    double x, w;
};

typedef T_resultstruct<Root2D> Roots;

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
Roots BlinnCubicSolver(double A, double B, double C, double D);

#endif // _BLINNSOLVER_H