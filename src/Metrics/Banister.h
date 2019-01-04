/*
 * Copyright (c) 2019 Mark Liversedge (liversedge@gmail.com)
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

#ifndef GC_Banister_h
#define GC_Banister_h 1

// typical athlete as defined by the opendata analysis
// using floats because they are typically used in calculations
// that require float precision (the actual values are integers)
extern const double typical_CP, typical_WPrime, typical_Pmax;

// calculate power index, used to model in cp plot too
extern double powerIndex(double averagepower, double duration);
#endif
