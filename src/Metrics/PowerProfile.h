/*
 * Copyright (c) 2018 Mark Liversedge (liversedge@gmail.com)
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

#include <QString>
#include <QMap>
#include <QList>
#include <QVector>

#ifndef GC_PowerProfile
#define GC_PowerProfile 1
struct PowerPercentile {

        public:

        // when trying to reference the series
        enum type { wpk_3min, wpk_7min, wpk_12min, wpk_cp, wpk_w,wpk_pmax,
                    abs_3min, abs_7min, abs_12min, abs_cp, abs_w, abs_pmax };

        double percentile,
               _3_min_Power, // members cannot start with a number
               _3_min_WPK,
               _7_min_Power,
               _7_min_WPK,
               _12_min_Power,
               _12_min_WPK,
               _20min_Power,
               _20min_WPK,
               CP,
               CP_wpk,
               W,
               W_jpk,
               Pmax,
               Pmax_wpk;

        // which value?
        double value(type x) {
            switch(x) {
            case wpk_3min: return _3_min_WPK;
            case wpk_7min: return _7_min_WPK;
            case wpk_12min: return _12_min_WPK;
            case wpk_cp: return CP_wpk;
            case wpk_w: return W_jpk;
            case wpk_pmax: return Pmax_wpk;
            case abs_3min: return _3_min_Power;
            case abs_7min: return _7_min_Power;
            case abs_12min: return _12_min_Power;
            case abs_cp: return CP;
            case abs_w: return W;
            case abs_pmax:  return Pmax;
            default: return 0;
            }
        }
        // return a string that ranks the value for the type
       static QString rank(type x, double value);
};

typedef PowerPercentile PowerPercentile;
extern PowerPercentile powerPercentile[];

struct PowerProfile {
    QList<double> percentiles;                 // list of percentiles we have data for
    QVector <double> seconds;                     // t values from 1 .. 36000
    QMap <double, QVector<double> > values;    // key is percentile; 99.99, 1, 95 etc
};
typedef PowerProfile PowerProfile;
extern PowerProfile powerProfile, powerProfileWPK;
extern void initPowerProfile();

#endif
