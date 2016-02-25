/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2014 Andy Froncioni
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

#ifndef _GC_MUPool_h
#define _GC_MUPool_h 1

#include "MUPlot.h"
#include "CriticalPowerWindow.h"

// CONSTANT / DEFAULT VALUES

// twitchiness (x)
#define MU_TW0      0
#define MU_TW1      1

// maximum power per kg
#define MU_MAXP0    10
#define MU_MAXP1    200

// minimum power per kg
#define MU_MINP0    20
#define MU_MINP1    0

// time till decay
#define MU_TAU0     100
#define MU_TAU1     0

// constant alpha
#define MU_ALPHA    0.1f

// athlete muscle mass (KG)
#define MU_MAX_MUSCLE 20.00f; // large male
#define MU_MIN_MUSCLE 8.00f;  // small female

// initial mean for MU distrbution
#define MU_FASTMEAN 0.8f
#define MU_SLOWMEAN 0.2f
#define MU_FASTXMEAN 0.5f

//
// An MU pool represents a pool of muscle fibers
// we typically have a QVector containing 1000 of these
//
// for now its just a plain old struct with no methods
// but that is likely to change soon
class MUPool 
{
    public:

        // set all to zero
        MUPool() : twitch(0), pmax(0), pmin(0), mass(0), tau(0), alpha(0) {}

        // set with values
        MUPool(double twitch, double pmax, double pmin, double mass, double tau, double alpha) 
             : twitch(twitch), pmax(pmax), pmin(pmin), mass(mass), tau(tau), alpha(alpha) {}


        // members
        double twitch;      // from slow to fast                  0 TO 1
        double pmax, pmin;  // max and minimum power capability   PER KG
        double mass;        // muscle mass of this pool           IN KG
        double tau;         // time till decays                   IN SECS
        double alpha;       // fatigue rate of tau                0.1 CONSTANT
};

//
// An MUSet that we can use to simulate activity against
// we create with an unfatigued set of motor pools and then
// can use the member functions to calculate whatever we 
// need.
//
class MUSet
{
    public:
    MUSet(QVector<MUPool>* set);

    // reset the pool
    void reset(QVector<MUPool>* set);
    void setMinMax();

    // apply load and return maximum duration
    // this load can be applied
    int applyLoad(int watts);

    // the actual pools of motor units (1,000)
    QVector<MUPool> muSet;
    double pMax, pMin;
};

#endif // _GC_MUPool_h
