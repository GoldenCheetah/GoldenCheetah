//
// BSplineCurve.h from Geometric Tools along with all the GTC headers
// it requires.

// disable warning messages
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

#define LogError(a)    (0); return 0;
#define LogAssert(a,b) (0)

// -----------------------------------------------------------------------
// Mathematics/Math.h
//
// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.09.03

#pragma once

// This file extends the <cmath> support to include convenient constants and
// functions.  The shared constants for CPU, Intel SSE and GPU lead to
// correctly rounded approximations of the constants when using 'float' or
// 'double'.  The file also includes a type trait, is_arbitrary_precision,
// to support selecting between floating-point arithmetic (float, double,
//long double) or arbitrary-precision arithmetic (BSNumber<T>, BSRational<T>)
// in an implementation using std::enable_if.  There is also a type trait,
// has_division_operator, to support selecting between numeric types that
// have a division operator (BSRational<T>) and those that do not have a
// division operator (BSNumber<T>).

#include <cfenv>
#include <cmath>
#include <limits>
#include <type_traits>

// Maximum number of iterations for bisection before a subinterval
// degenerates to a single point. TODO: Verify these.  I used the formula:
// 3 + std::numeric_limits<T>::digits - std::numeric_limits<T>::min_exponent.
//   IEEEBinary16:  digits = 11, min_exponent = -13
//   float:         digits = 27, min_exponent = -125
//   double:        digits = 53, min_exponent = -1021
// BSNumber and BSRational use std::numeric_limits<unsigned int>::max(),
// but maybe these should be set to a large number or be user configurable.
// The MAX_BISECTIONS_GENERIC is an arbitrary choice for now and is used
// in template code where Real is the template parameter.
#define GTE_C_MAX_BISECTIONS_FLOAT16    27u
#define GTE_C_MAX_BISECTIONS_FLOAT32    155u
#define GTE_C_MAX_BISECTIONS_FLOAT64    1077u
#define GTE_C_MAX_BISECTIONS_BSNUMBER   0xFFFFFFFFu
#define GTE_C_MAX_BISECTIONS_BSRATIONAL 0xFFFFFFFFu
#define GTE_C_MAX_BISECTIONS_GENERIC    2048u

// Constants involving pi.
#define GTE_C_PI 3.1415926535897931
#define GTE_C_HALF_PI 1.5707963267948966
#define GTE_C_QUARTER_PI 0.7853981633974483
#define GTE_C_TWO_PI 6.2831853071795862
#define GTE_C_INV_PI 0.3183098861837907
#define GTE_C_INV_TWO_PI 0.1591549430918953
#define GTE_C_INV_HALF_PI 0.6366197723675813

// Conversions between degrees and radians.
#define GTE_C_DEG_TO_RAD 0.0174532925199433
#define GTE_C_RAD_TO_DEG 57.295779513082321

// Common constants.
#define GTE_C_SQRT_2 1.4142135623730951
#define GTE_C_INV_SQRT_2 0.7071067811865475
#define GTE_C_LN_2 0.6931471805599453
#define GTE_C_INV_LN_2 1.4426950408889634
#define GTE_C_LN_10 2.3025850929940459
#define GTE_C_INV_LN_10 0.43429448190325176

// Constants for minimax polynomial approximations to sqrt(x).
// The algorithm minimizes the maximum absolute error on [1,2].
#define GTE_C_SQRT_DEG1_C0 +1.0
#define GTE_C_SQRT_DEG1_C1 +4.1421356237309505e-01
#define GTE_C_SQRT_DEG1_MAX_ERROR 1.7766952966368793e-2

#define GTE_C_SQRT_DEG2_C0 +1.0
#define GTE_C_SQRT_DEG2_C1 +4.8563183076125260e-01
#define GTE_C_SQRT_DEG2_C2 -7.1418268388157458e-02
#define GTE_C_SQRT_DEG2_MAX_ERROR 1.1795695163108744e-3

#define GTE_C_SQRT_DEG3_C0 +1.0
#define GTE_C_SQRT_DEG3_C1 +4.9750045320242231e-01
#define GTE_C_SQRT_DEG3_C2 -1.0787308044477850e-01
#define GTE_C_SQRT_DEG3_C3 +2.4586189615451115e-02
#define GTE_C_SQRT_DEG3_MAX_ERROR 1.1309620116468910e-4

#define GTE_C_SQRT_DEG4_C0 +1.0
#define GTE_C_SQRT_DEG4_C1 +4.9955939832918816e-01
#define GTE_C_SQRT_DEG4_C2 -1.2024066151943025e-01
#define GTE_C_SQRT_DEG4_C3 +4.5461507257698486e-02
#define GTE_C_SQRT_DEG4_C4 -1.0566681694362146e-02
#define GTE_C_SQRT_DEG4_MAX_ERROR 1.2741170151556180e-5

#define GTE_C_SQRT_DEG5_C0 +1.0
#define GTE_C_SQRT_DEG5_C1 +4.9992197660031912e-01
#define GTE_C_SQRT_DEG5_C2 -1.2378506719245053e-01
#define GTE_C_SQRT_DEG5_C3 +5.6122776972699739e-02
#define GTE_C_SQRT_DEG5_C4 -2.3128836281145482e-02
#define GTE_C_SQRT_DEG5_C5 +5.0827122737047148e-03
#define GTE_C_SQRT_DEG5_MAX_ERROR 1.5725568940708201e-6

#define GTE_C_SQRT_DEG6_C0 +1.0
#define GTE_C_SQRT_DEG6_C1 +4.9998616695784914e-01
#define GTE_C_SQRT_DEG6_C2 -1.2470733323278438e-01
#define GTE_C_SQRT_DEG6_C3 +6.0388587356982271e-02
#define GTE_C_SQRT_DEG6_C4 -3.1692053551807930e-02
#define GTE_C_SQRT_DEG6_C5 +1.2856590305148075e-02
#define GTE_C_SQRT_DEG6_C6 -2.6183954624343642e-03
#define GTE_C_SQRT_DEG6_MAX_ERROR 2.0584155535630089e-7

#define GTE_C_SQRT_DEG7_C0 +1.0
#define GTE_C_SQRT_DEG7_C1 +4.9999754817809228e-01
#define GTE_C_SQRT_DEG7_C2 -1.2493243476353655e-01
#define GTE_C_SQRT_DEG7_C3 +6.1859954146370910e-02
#define GTE_C_SQRT_DEG7_C4 -3.6091595023208356e-02
#define GTE_C_SQRT_DEG7_C5 +1.9483946523450868e-02
#define GTE_C_SQRT_DEG7_C6 -7.5166134568007692e-03
#define GTE_C_SQRT_DEG7_C7 +1.4127567687864939e-03
#define GTE_C_SQRT_DEG7_MAX_ERROR 2.8072302919734948e-8

#define GTE_C_SQRT_DEG8_C0 +1.0
#define GTE_C_SQRT_DEG8_C1 +4.9999956583056759e-01
#define GTE_C_SQRT_DEG8_C2 -1.2498490369914350e-01
#define GTE_C_SQRT_DEG8_C3 +6.2318494667579216e-02
#define GTE_C_SQRT_DEG8_C4 -3.7982961896432244e-02
#define GTE_C_SQRT_DEG8_C5 +2.3642612312869460e-02
#define GTE_C_SQRT_DEG8_C6 -1.2529377587270574e-02
#define GTE_C_SQRT_DEG8_C7 +4.5382426960713929e-03
#define GTE_C_SQRT_DEG8_C8 -7.8810995273670414e-04
#define GTE_C_SQRT_DEG8_MAX_ERROR 3.9460605685825989e-9

// Constants for minimax polynomial approximations to 1/sqrt(x).
// The algorithm minimizes the maximum absolute error on [1,2].
#define GTE_C_INVSQRT_DEG1_C0 +1.0
#define GTE_C_INVSQRT_DEG1_C1 -2.9289321881345254e-01
#define GTE_C_INVSQRT_DEG1_MAX_ERROR 3.7814314552701983e-2

#define GTE_C_INVSQRT_DEG2_C0 +1.0
#define GTE_C_INVSQRT_DEG2_C1 -4.4539812104566801e-01
#define GTE_C_INVSQRT_DEG2_C2 +1.5250490223221547e-01
#define GTE_C_INVSQRT_DEG2_MAX_ERROR 4.1953446330581234e-3

#define GTE_C_INVSQRT_DEG3_C0 +1.0
#define GTE_C_INVSQRT_DEG3_C1 -4.8703230993068791e-01
#define GTE_C_INVSQRT_DEG3_C2 +2.8163710486669835e-01
#define GTE_C_INVSQRT_DEG3_C3 -8.7498013749463421e-02
#define GTE_C_INVSQRT_DEG3_MAX_ERROR 5.6307702007266786e-4

#define GTE_C_INVSQRT_DEG4_C0 +1.0
#define GTE_C_INVSQRT_DEG4_C1 -4.9710061558048779e-01
#define GTE_C_INVSQRT_DEG4_C2 +3.4266247597676802e-01
#define GTE_C_INVSQRT_DEG4_C3 -1.9106356536293490e-01
#define GTE_C_INVSQRT_DEG4_C4 +5.2608486153198797e-02
#define GTE_C_INVSQRT_DEG4_MAX_ERROR 8.1513919987605266e-5

#define GTE_C_INVSQRT_DEG5_C0 +1.0
#define GTE_C_INVSQRT_DEG5_C1 -4.9937760586004143e-01
#define GTE_C_INVSQRT_DEG5_C2 +3.6508741295133973e-01
#define GTE_C_INVSQRT_DEG5_C3 -2.5884890281853501e-01
#define GTE_C_INVSQRT_DEG5_C4 +1.3275782221320753e-01
#define GTE_C_INVSQRT_DEG5_C5 -3.2511945299404488e-02
#define GTE_C_INVSQRT_DEG5_MAX_ERROR 1.2289367475583346e-5

#define GTE_C_INVSQRT_DEG6_C0 +1.0
#define GTE_C_INVSQRT_DEG6_C1 -4.9987029229547453e-01
#define GTE_C_INVSQRT_DEG6_C2 +3.7220923604495226e-01
#define GTE_C_INVSQRT_DEG6_C3 -2.9193067713256937e-01
#define GTE_C_INVSQRT_DEG6_C4 +1.9937605991094642e-01
#define GTE_C_INVSQRT_DEG6_C5 -9.3135712130901993e-02
#define GTE_C_INVSQRT_DEG6_C6 +2.0458166789566690e-02
#define GTE_C_INVSQRT_DEG6_MAX_ERROR 1.9001451223750465e-6

#define GTE_C_INVSQRT_DEG7_C0 +1.0
#define GTE_C_INVSQRT_DEG7_C1 -4.9997357250704977e-01
#define GTE_C_INVSQRT_DEG7_C2 +3.7426216884998809e-01
#define GTE_C_INVSQRT_DEG7_C3 -3.0539882498248971e-01
#define GTE_C_INVSQRT_DEG7_C4 +2.3976005607005391e-01
#define GTE_C_INVSQRT_DEG7_C5 -1.5410326351684489e-01
#define GTE_C_INVSQRT_DEG7_C6 +6.5598809723041995e-02
#define GTE_C_INVSQRT_DEG7_C7 -1.3038592450470787e-02
#define GTE_C_INVSQRT_DEG7_MAX_ERROR 2.9887724993168940e-7

#define GTE_C_INVSQRT_DEG8_C0 +1.0
#define GTE_C_INVSQRT_DEG8_C1 -4.9999471066120371e-01
#define GTE_C_INVSQRT_DEG8_C2 +3.7481415745794067e-01
#define GTE_C_INVSQRT_DEG8_C3 -3.1023804387422160e-01
#define GTE_C_INVSQRT_DEG8_C4 +2.5977002682930106e-01
#define GTE_C_INVSQRT_DEG8_C5 -1.9818790717727097e-01
#define GTE_C_INVSQRT_DEG8_C6 +1.1882414252613671e-01
#define GTE_C_INVSQRT_DEG8_C7 -4.6270038088550791e-02
#define GTE_C_INVSQRT_DEG8_C8 +8.3891541755747312e-03
#define GTE_C_INVSQRT_DEG8_MAX_ERROR 4.7596926146947771e-8

// Constants for minimax polynomial approximations to sin(x).
// The algorithm minimizes the maximum absolute error on [-pi/2,pi/2].
#define GTE_C_SIN_DEG3_C0 +1.0
#define GTE_C_SIN_DEG3_C1 -1.4727245910375519e-01
#define GTE_C_SIN_DEG3_MAX_ERROR 1.3481903639145865e-2

#define GTE_C_SIN_DEG5_C0 +1.0
#define GTE_C_SIN_DEG5_C1 -1.6600599923812209e-01
#define GTE_C_SIN_DEG5_C2 +7.5924178409012000e-03
#define GTE_C_SIN_DEG5_MAX_ERROR 1.4001209384639779e-4

#define GTE_C_SIN_DEG7_C0 +1.0
#define GTE_C_SIN_DEG7_C1 -1.6665578084732124e-01
#define GTE_C_SIN_DEG7_C2 +8.3109378830028557e-03
#define GTE_C_SIN_DEG7_C3 -1.8447486103462252e-04
#define GTE_C_SIN_DEG7_MAX_ERROR 1.0205878936686563e-6

#define GTE_C_SIN_DEG9_C0 +1.0
#define GTE_C_SIN_DEG9_C1 -1.6666656235308897e-01
#define GTE_C_SIN_DEG9_C2 +8.3329962509886002e-03
#define GTE_C_SIN_DEG9_C3 -1.9805100675274190e-04
#define GTE_C_SIN_DEG9_C4 +2.5967200279475300e-06
#define GTE_C_SIN_DEG9_MAX_ERROR 5.2010746265374053e-9

#define GTE_C_SIN_DEG11_C0 +1.0
#define GTE_C_SIN_DEG11_C1 -1.6666666601721269e-01
#define GTE_C_SIN_DEG11_C2 +8.3333303183525942e-03
#define GTE_C_SIN_DEG11_C3 -1.9840782426250314e-04
#define GTE_C_SIN_DEG11_C4 +2.7521557770526783e-06
#define GTE_C_SIN_DEG11_C5 -2.3828544692960918e-08
#define GTE_C_SIN_DEG11_MAX_ERROR 1.9295870457014530e-11

// Constants for minimax polynomial approximations to cos(x).
// The algorithm minimizes the maximum absolute error on [-pi/2,pi/2].
#define GTE_C_COS_DEG2_C0 +1.0
#define GTE_C_COS_DEG2_C1 -4.0528473456935105e-01
#define GTE_C_COS_DEG2_MAX_ERROR 5.4870946878404048e-2

#define GTE_C_COS_DEG4_C0 +1.0
#define GTE_C_COS_DEG4_C1 -4.9607181958647262e-01
#define GTE_C_COS_DEG4_C2 +3.6794619653489236e-02
#define GTE_C_COS_DEG4_MAX_ERROR 9.1879932449712154e-4

#define GTE_C_COS_DEG6_C0 +1.0
#define GTE_C_COS_DEG6_C1 -4.9992746217057404e-01
#define GTE_C_COS_DEG6_C2 +4.1493920348353308e-02
#define GTE_C_COS_DEG6_C3 -1.2712435011987822e-03
#define GTE_C_COS_DEG6_MAX_ERROR 9.2028470133065365e-6

#define GTE_C_COS_DEG8_C0 +1.0
#define GTE_C_COS_DEG8_C1 -4.9999925121358291e-01
#define GTE_C_COS_DEG8_C2 +4.1663780117805693e-02
#define GTE_C_COS_DEG8_C3 -1.3854239405310942e-03
#define GTE_C_COS_DEG8_C4 +2.3154171575501259e-05
#define GTE_C_COS_DEG8_MAX_ERROR 5.9804533020235695e-8

#define GTE_C_COS_DEG10_C0 +1.0
#define GTE_C_COS_DEG10_C1 -4.9999999508695869e-01
#define GTE_C_COS_DEG10_C2 +4.1666638865338612e-02
#define GTE_C_COS_DEG10_C3 -1.3888377661039897e-03
#define GTE_C_COS_DEG10_C4 +2.4760495088926859e-05
#define GTE_C_COS_DEG10_C5 -2.6051615464872668e-07
#define GTE_C_COS_DEG10_MAX_ERROR 2.7006769043325107e-10

// Constants for minimax polynomial approximations to tan(x).
// The algorithm minimizes the maximum absolute error on [-pi/4,pi/4].
#define GTE_C_TAN_DEG3_C0 1.0
#define GTE_C_TAN_DEG3_C1 4.4295926544736286e-01
#define GTE_C_TAN_DEG3_MAX_ERROR 1.1661892256204731e-2

#define GTE_C_TAN_DEG5_C0 1.0
#define GTE_C_TAN_DEG5_C1 3.1401320403542421e-01
#define GTE_C_TAN_DEG5_C2 2.0903948109240345e-01
#define GTE_C_TAN_DEG5_MAX_ERROR 5.8431854390143118e-4

#define GTE_C_TAN_DEG7_C0 1.0
#define GTE_C_TAN_DEG7_C1 3.3607213284422555e-01
#define GTE_C_TAN_DEG7_C2 1.1261037305184907e-01
#define GTE_C_TAN_DEG7_C3 9.8352099470524479e-02
#define GTE_C_TAN_DEG7_MAX_ERROR 3.5418688397723108e-5

#define GTE_C_TAN_DEG9_C0 1.0
#define GTE_C_TAN_DEG9_C1 3.3299232843941784e-01
#define GTE_C_TAN_DEG9_C2 1.3747843432474838e-01
#define GTE_C_TAN_DEG9_C3 3.7696344813028304e-02
#define GTE_C_TAN_DEG9_C4 4.6097377279281204e-02
#define GTE_C_TAN_DEG9_MAX_ERROR 2.2988173242199927e-6

#define GTE_C_TAN_DEG11_C0 1.0
#define GTE_C_TAN_DEG11_C1 3.3337224456224224e-01
#define GTE_C_TAN_DEG11_C2 1.3264516053824593e-01
#define GTE_C_TAN_DEG11_C3 5.8145237645931047e-02
#define GTE_C_TAN_DEG11_C4 1.0732193237572574e-02
#define GTE_C_TAN_DEG11_C5 2.1558456793513869e-02
#define GTE_C_TAN_DEG11_MAX_ERROR 1.5426257940140409e-7

#define GTE_C_TAN_DEG13_C0 1.0
#define GTE_C_TAN_DEG13_C1 3.3332916426394554e-01
#define GTE_C_TAN_DEG13_C2 1.3343404625112498e-01
#define GTE_C_TAN_DEG13_C3 5.3104565343119248e-02
#define GTE_C_TAN_DEG13_C4 2.5355038312682154e-02
#define GTE_C_TAN_DEG13_C5 1.8253255966556026e-03
#define GTE_C_TAN_DEG13_C6 1.0069407176615641e-02
#define GTE_C_TAN_DEG13_MAX_ERROR 1.0550264249037378e-8

// Constants for minimax polynomial approximations to acos(x), where the
// approximation is of the form acos(x) = sqrt(1 - x)*p(x) with p(x) a
// polynomial.  The algorithm minimizes the maximum error
// |acos(x)/sqrt(1-x) - p(x)| on [0,1].  At the same time we get an
// approximation for asin(x) = pi/2 - acos(x).
#define GTE_C_ACOS_DEG1_C0 +1.5707963267948966
#define GTE_C_ACOS_DEG1_C1 -1.5658276442180141e-01
#define GTE_C_ACOS_DEG1_MAX_ERROR 1.1659002803738105e-2

#define GTE_C_ACOS_DEG2_C0 +1.5707963267948966
#define GTE_C_ACOS_DEG2_C1 -2.0347053865798365e-01
#define GTE_C_ACOS_DEG2_C2 +4.6887774236182234e-02
#define GTE_C_ACOS_DEG2_MAX_ERROR 9.0311602490029258e-4

#define GTE_C_ACOS_DEG3_C0 +1.5707963267948966
#define GTE_C_ACOS_DEG3_C1 -2.1253291899190285e-01
#define GTE_C_ACOS_DEG3_C2 +7.4773789639484223e-02
#define GTE_C_ACOS_DEG3_C3 -1.8823635069382449e-02
#define GTE_C_ACOS_DEG3_MAX_ERROR 9.3066396954288172e-5

#define GTE_C_ACOS_DEG4_C0 +1.5707963267948966
#define GTE_C_ACOS_DEG4_C1 -2.1422258835275865e-01
#define GTE_C_ACOS_DEG4_C2 +8.4936675142844198e-02
#define GTE_C_ACOS_DEG4_C3 -3.5991475120957794e-02
#define GTE_C_ACOS_DEG4_C4 +8.6946239090712751e-03
#define GTE_C_ACOS_DEG4_MAX_ERROR 1.0930595804481413e-5

#define GTE_C_ACOS_DEG5_C0 +1.5707963267948966
#define GTE_C_ACOS_DEG5_C1 -2.1453292139805524e-01
#define GTE_C_ACOS_DEG5_C2 +8.7973089282889383e-02
#define GTE_C_ACOS_DEG5_C3 -4.5130266382166440e-02
#define GTE_C_ACOS_DEG5_C4 +1.9467466687281387e-02
#define GTE_C_ACOS_DEG5_C5 -4.3601326117634898e-03
#define GTE_C_ACOS_DEG5_MAX_ERROR 1.3861070257241426-6

#define GTE_C_ACOS_DEG6_C0 +1.5707963267948966
#define GTE_C_ACOS_DEG6_C1 -2.1458939285677325e-01
#define GTE_C_ACOS_DEG6_C2 +8.8784960563641491e-02
#define GTE_C_ACOS_DEG6_C3 -4.8887131453156485e-02
#define GTE_C_ACOS_DEG6_C4 +2.7011519960012720e-02
#define GTE_C_ACOS_DEG6_C5 -1.1210537323478320e-02
#define GTE_C_ACOS_DEG6_C6 +2.3078166879102469e-03
#define GTE_C_ACOS_DEG6_MAX_ERROR 1.8491291330427484e-7

#define GTE_C_ACOS_DEG7_C0 +1.5707963267948966
#define GTE_C_ACOS_DEG7_C1 -2.1459960076929829e-01
#define GTE_C_ACOS_DEG7_C2 +8.8986946573346160e-02
#define GTE_C_ACOS_DEG7_C3 -5.0207843052845647e-02
#define GTE_C_ACOS_DEG7_C4 +3.0961594977611639e-02
#define GTE_C_ACOS_DEG7_C5 -1.7162031184398074e-02
#define GTE_C_ACOS_DEG7_C6 +6.7072304676685235e-03
#define GTE_C_ACOS_DEG7_C7 -1.2690614339589956e-03
#define GTE_C_ACOS_DEG7_MAX_ERROR 2.5574620927948377e-8

#define GTE_C_ACOS_DEG8_C0 +1.5707963267948966
#define GTE_C_ACOS_DEG8_C1 -2.1460143648688035e-01
#define GTE_C_ACOS_DEG8_C2 +8.9034700107934128e-02
#define GTE_C_ACOS_DEG8_C3 -5.0625279962389413e-02
#define GTE_C_ACOS_DEG8_C4 +3.2683762943179318e-02
#define GTE_C_ACOS_DEG8_C5 -2.0949278766238422e-02
#define GTE_C_ACOS_DEG8_C6 +1.1272900916992512e-02
#define GTE_C_ACOS_DEG8_C7 -4.1160981058965262e-03
#define GTE_C_ACOS_DEG8_C8 +7.1796493341480527e-04
#define GTE_C_ACOS_DEG8_MAX_ERROR 3.6340015129032732e-9

// Constants for minimax polynomial approximations to atan(x).
// The algorithm minimizes the maximum absolute error on [-1,1].
#define GTE_C_ATAN_DEG3_C0 +1.0
#define GTE_C_ATAN_DEG3_C1 -2.1460183660255172e-01
#define GTE_C_ATAN_DEG3_MAX_ERROR 1.5970326392614240e-2

#define GTE_C_ATAN_DEG5_C0 +1.0
#define GTE_C_ATAN_DEG5_C1 -3.0189478312144946e-01
#define GTE_C_ATAN_DEG5_C2 +8.7292946518897740e-02
#define GTE_C_ATAN_DEG5_MAX_ERROR 1.3509832247372636e-3

#define GTE_C_ATAN_DEG7_C0 +1.0
#define GTE_C_ATAN_DEG7_C1 -3.2570157599356531e-01
#define GTE_C_ATAN_DEG7_C2 +1.5342994884206673e-01
#define GTE_C_ATAN_DEG7_C3 -4.2330209451053591e-02
#define GTE_C_ATAN_DEG7_MAX_ERROR 1.5051227215514412e-4

#define GTE_C_ATAN_DEG9_C0 +1.0
#define GTE_C_ATAN_DEG9_C1 -3.3157878236439586e-01
#define GTE_C_ATAN_DEG9_C2 +1.8383034738018011e-01
#define GTE_C_ATAN_DEG9_C3 -8.9253037587244677e-02
#define GTE_C_ATAN_DEG9_C4 +2.2399635968909593e-02
#define GTE_C_ATAN_DEG9_MAX_ERROR 1.8921598624582064e-5

#define GTE_C_ATAN_DEG11_C0 +1.0
#define GTE_C_ATAN_DEG11_C1 -3.3294527685374087e-01
#define GTE_C_ATAN_DEG11_C2 +1.9498657165383548e-01
#define GTE_C_ATAN_DEG11_C3 -1.1921576270475498e-01
#define GTE_C_ATAN_DEG11_C4 +5.5063351366968050e-02
#define GTE_C_ATAN_DEG11_C5 -1.2490720064867844e-02
#define GTE_C_ATAN_DEG11_MAX_ERROR 2.5477724974187765e-6

#define GTE_C_ATAN_DEG13_C0 +1.0
#define GTE_C_ATAN_DEG13_C1 -3.3324998579202170e-01
#define GTE_C_ATAN_DEG13_C2 +1.9856563505717162e-01
#define GTE_C_ATAN_DEG13_C3 -1.3374657325451267e-01
#define GTE_C_ATAN_DEG13_C4 +8.1675882859940430e-02
#define GTE_C_ATAN_DEG13_C5 -3.5059680836411644e-02
#define GTE_C_ATAN_DEG13_C6 +7.2128853633444123e-03
#define GTE_C_ATAN_DEG13_MAX_ERROR 3.5859104691865484e-7

// Constants for minimax polynomial approximations to exp2(x) = 2^x.
// The algorithm minimizes the maximum absolute error on [0,1].
#define GTE_C_EXP2_DEG1_C0 1.0
#define GTE_C_EXP2_DEG1_C1 1.0
#define GTE_C_EXP2_DEG1_MAX_ERROR 8.6071332055934313e-2

#define GTE_C_EXP2_DEG2_C0 1.0
#define GTE_C_EXP2_DEG2_C1 6.5571332605741528e-01
#define GTE_C_EXP2_DEG2_C2 3.4428667394258472e-01
#define GTE_C_EXP2_DEG2_MAX_ERROR 3.8132476831060358e-3

#define GTE_C_EXP2_DEG3_C0 1.0
#define GTE_C_EXP2_DEG3_C1 6.9589012084456225e-01
#define GTE_C_EXP2_DEG3_C2 2.2486494900110188e-01
#define GTE_C_EXP2_DEG3_C3 7.9244930154334980e-02
#define GTE_C_EXP2_DEG3_MAX_ERROR 1.4694877755186408e-4

#define GTE_C_EXP2_DEG4_C0 1.0
#define GTE_C_EXP2_DEG4_C1 6.9300392358459195e-01
#define GTE_C_EXP2_DEG4_C2 2.4154981722455560e-01
#define GTE_C_EXP2_DEG4_C3 5.1744260331489045e-02
#define GTE_C_EXP2_DEG4_C4 1.3701998859367848e-02
#define GTE_C_EXP2_DEG4_MAX_ERROR 4.7617792624521371e-6

#define GTE_C_EXP2_DEG5_C0 1.0
#define GTE_C_EXP2_DEG5_C1 6.9315298010274962e-01
#define GTE_C_EXP2_DEG5_C2 2.4014712313022102e-01
#define GTE_C_EXP2_DEG5_C3 5.5855296413199085e-02
#define GTE_C_EXP2_DEG5_C4 8.9477503096873079e-03
#define GTE_C_EXP2_DEG5_C5 1.8968500441332026e-03
#define GTE_C_EXP2_DEG5_MAX_ERROR 1.3162098333463490e-7

#define GTE_C_EXP2_DEG6_C0 1.0
#define GTE_C_EXP2_DEG6_C1 6.9314698914837525e-01
#define GTE_C_EXP2_DEG6_C2 2.4023013440952923e-01
#define GTE_C_EXP2_DEG6_C3 5.5481276898206033e-02
#define GTE_C_EXP2_DEG6_C4 9.6838443037086108e-03
#define GTE_C_EXP2_DEG6_C5 1.2388324048515642e-03
#define GTE_C_EXP2_DEG6_C6 2.1892283501756538e-04
#define GTE_C_EXP2_DEG6_MAX_ERROR 3.1589168225654163e-9

#define GTE_C_EXP2_DEG7_C0 1.0
#define GTE_C_EXP2_DEG7_C1 6.9314718588750690e-01
#define GTE_C_EXP2_DEG7_C2 2.4022637363165700e-01
#define GTE_C_EXP2_DEG7_C3 5.5505235570535660e-02
#define GTE_C_EXP2_DEG7_C4 9.6136265387940512e-03
#define GTE_C_EXP2_DEG7_C5 1.3429234504656051e-03
#define GTE_C_EXP2_DEG7_C6 1.4299202757683815e-04
#define GTE_C_EXP2_DEG7_C7 2.1662892777385423e-05
#define GTE_C_EXP2_DEG7_MAX_ERROR 6.6864513925679603e-11

// Constants for minimax polynomial approximations to log2(x).
// The algorithm minimizes the maximum absolute error on [1,2].
// The polynomials all have constant term zero.
#define GTE_C_LOG2_DEG1_C1 +1.0
#define GTE_C_LOG2_DEG1_MAX_ERROR 8.6071332055934202e-2

#define GTE_C_LOG2_DEG2_C1 +1.3465553856377803 
#define GTE_C_LOG2_DEG2_C2 -3.4655538563778032e-01
#define GTE_C_LOG2_DEG2_MAX_ERROR 7.6362868906658110e-3

#define GTE_C_LOG2_DEG3_C1 +1.4228653756681227
#define GTE_C_LOG2_DEG3_C2 -5.8208556916449616e-01
#define GTE_C_LOG2_DEG3_C3 +1.5922019349637218e-01
#define GTE_C_LOG2_DEG3_MAX_ERROR 8.7902902652883808e-4

#define GTE_C_LOG2_DEG4_C1 +1.4387257478171547
#define GTE_C_LOG2_DEG4_C2 -6.7778401359918661e-01
#define GTE_C_LOG2_DEG4_C3 +3.2118898377713379e-01
#define GTE_C_LOG2_DEG4_C4 -8.2130717995088531e-02
#define GTE_C_LOG2_DEG4_MAX_ERROR 1.1318551355360418e-4

#define GTE_C_LOG2_DEG5_C1 +1.4419170408633741
#define GTE_C_LOG2_DEG5_C2 -7.0909645927612530e-01
#define GTE_C_LOG2_DEG5_C3 +4.1560609399164150e-01
#define GTE_C_LOG2_DEG5_C4 -1.9357573729558908e-01
#define GTE_C_LOG2_DEG5_C5 +4.5149061716699634e-02
#define GTE_C_LOG2_DEG5_MAX_ERROR 1.5521274478735858e-5

#define GTE_C_LOG2_DEG6_C1 +1.4425449435950917
#define GTE_C_LOG2_DEG6_C2 -7.1814525675038965e-01
#define GTE_C_LOG2_DEG6_C3 +4.5754919692564044e-01
#define GTE_C_LOG2_DEG6_C4 -2.7790534462849337e-01
#define GTE_C_LOG2_DEG6_C5 +1.2179791068763279e-01
#define GTE_C_LOG2_DEG6_C6 -2.5841449829670182e-02
#define GTE_C_LOG2_DEG6_MAX_ERROR 2.2162051216689793e-6

#define GTE_C_LOG2_DEG7_C1 +1.4426664401536078
#define GTE_C_LOG2_DEG7_C2 -7.2055423726162360e-01
#define GTE_C_LOG2_DEG7_C3 +4.7332419162501083e-01
#define GTE_C_LOG2_DEG7_C4 -3.2514018752954144e-01
#define GTE_C_LOG2_DEG7_C5 +1.9302965529095673e-01
#define GTE_C_LOG2_DEG7_C6 -7.8534970641157997e-02
#define GTE_C_LOG2_DEG7_C7 +1.5209108363023915e-02
#define GTE_C_LOG2_DEG7_MAX_ERROR 3.2546531700261561e-7

#define GTE_C_LOG2_DEG8_C1 +1.4426896453621882
#define GTE_C_LOG2_DEG8_C2 -7.2115893912535967e-01
#define GTE_C_LOG2_DEG8_C3 +4.7861716616785088e-01
#define GTE_C_LOG2_DEG8_C4 -3.4699935395019565e-01
#define GTE_C_LOG2_DEG8_C5 +2.4114048765477492e-01
#define GTE_C_LOG2_DEG8_C6 -1.3657398692885181e-01
#define GTE_C_LOG2_DEG8_C7 +5.1421382871922106e-02
#define GTE_C_LOG2_DEG8_C8 -9.1364020499895560e-03
#define GTE_C_LOG2_DEG8_MAX_ERROR 4.8796219218050219e-8

// These functions are convenient for some applications.  The classes
// BSNumber, BSRational and IEEEBinary16 have implementations that
// (for now) use typecasting to call the 'float' or 'double' versions.
namespace gte
{
    inline float atandivpi(float x)
    {
        return std::atan(x) * (float)GTE_C_INV_PI;
    }

    inline float atan2divpi(float y, float x)
    {
        return std::atan2(y, x) * (float)GTE_C_INV_PI;
    }

    inline float clamp(float x, float xmin, float xmax)
    {
        return (x <= xmin ? xmin : (x >= xmax ? xmax : x));
    }

    inline float cospi(float x)
    {
        return std::cos(x * (float)GTE_C_PI);
    }

    inline float exp10(float x)
    {
        return std::exp(x * (float)GTE_C_LN_10);
    }

    inline float invsqrt(float x)
    {
        return 1.0f / std::sqrt(x);
    }

    inline int isign(float x)
    {
        return (x > 0.0f ? 1 : (x < 0.0f ? -1 : 0));
    }

    inline float saturate(float x)
    {
        return (x <= 0.0f ? 0.0f : (x >= 1.0f ? 1.0f : x));
    }

    inline float sign(float x)
    {
        return (x > 0.0f ? 1.0f : (x < 0.0f ? -1.0f : 0.0f));
    }

    inline float sinpi(float x)
    {
        return std::sin(x * (float)GTE_C_PI);
    }

    inline float sqr(float x)
    {
        return x * x;
    }


    inline double atandivpi(double x)
    {
        return std::atan(x) * GTE_C_INV_PI;
    }

    inline double atan2divpi(double y, double x)
    {
        return std::atan2(y, x) * GTE_C_INV_PI;
    }

    inline double clamp(double x, double xmin, double xmax)
    {
        return (x <= xmin ? xmin : (x >= xmax ? xmax : x));
    }

    inline double cospi(double x)
    {
        return std::cos(x * GTE_C_PI);
    }

    inline double exp10(double x)
    {
        return std::exp(x * GTE_C_LN_10);
    }

    inline double invsqrt(double x)
    {
        return 1.0 / std::sqrt(x);
    }

    inline double sign(double x)
    {
        return (x > 0.0 ? 1.0 : (x < 0.0 ? -1.0 : 0.0f));
    }

    inline int isign(double x)
    {
        return (x > 0.0 ? 1 : (x < 0.0 ? -1 : 0));
    }

    inline double saturate(double x)
    {
        return (x <= 0.0 ? 0.0 : (x >= 1.0 ? 1.0 : x));
    }

    inline double sinpi(double x)
    {
        return std::sin(x * GTE_C_PI);
    }

    inline double sqr(double x)
    {
        return x * x;
    }
}

// Type traits to support std::enable_if conditional compilation for
// numerical computations.
namespace gte
{
    // The trait is_arbitrary_precision<T> for type T of float, double or
    // long double generates is_arbitrary_precision<T>::value of false.  The
    // implementations for arbitrary-precision arithmetic are found in
    // GteArbitraryPrecision.h.
    template <typename T>
    struct is_arbitrary_precision_internal : std::false_type {};

    // EricChristoffersen 1/26/2020: Replace remove_cv_t with pre-c14 syntax so will compile with xcode.
    template <typename T>
    struct is_arbitrary_precision : is_arbitrary_precision_internal<typename std::remove_cv<T>::type>::type {};

    // The trait has_division_operator<T> for type T of float, double or
    // long double generates has_division_operator<T>::value of true.  The
    // implementations for arbitrary-precision arithmetic are found in
    // GteArbitraryPrecision.h.
    template <typename T>
    struct has_division_operator_internal : std::false_type {};

    // EricChristoffersen 1/26/2020: Replace remove_cv_t with pre-c14 syntax so will compile with xcode.
    template <typename T>
    struct has_division_operator : has_division_operator_internal<typename std::remove_cv<T>::type>::type {};

    template <>
    struct has_division_operator_internal<float> : std::true_type {};

    template <>
    struct has_division_operator_internal<double> : std::true_type {};

    template <>
    struct has_division_operator_internal<long double> : std::true_type {};
}

// -----------------------------------------------------------------------
// Mathematics/Array2.h
//
// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.08.13

#pragma once

#include <cstddef>
#include <vector>

// The Array2 class represents a 2-dimensional array that minimizes the number
// of new and delete calls.  The T objects are stored in a contiguous array.

namespace gte
{
    template <typename T>
    class Array2
    {
    public:
        // Construction.  The first constructor generates an array of objects
        // that are owned by Array2.  The second constructor is given an array
        // of objects that are owned by the caller.  The array has bound0
        // columns and bound1 rows.
        Array2(size_t bound0, size_t bound1)
            :
            mBound0(bound0),
            mBound1(bound1),
            mObjects(bound0 * bound1),
            mIndirect1(bound1)
        {
            SetPointers(mObjects.data());
        }

        Array2(size_t bound0, size_t bound1, T* objects)
            :
            mBound0(bound0),
            mBound1(bound1),
            mIndirect1(bound1)
        {
            SetPointers(objects);
        }

        // Support for dynamic resizing, copying, or moving.  If 'other' does
        // not own the original 'objects', they are not copied by the
        // assignment operator.
        Array2()
            :
            mBound0(0),
            mBound1(0)
        {
        }

        Array2(Array2 const& other)
            :
            mBound0(other.mBound0),
            mBound1(other.mBound1)
        {
            *this = other;
        }

        Array2& operator=(Array2 const& other)
        {
            // The copy is valid whether or not other.mObjects has elements.
            mObjects = other.mObjects;
            SetPointers(other);
            return *this;
        }

        Array2(Array2&& other) noexcept
            :
            mBound0(other.mBound0),
            mBound1(other.mBound1)
        {
            *this = std::move(other);
        }

        Array2& operator=(Array2&& other) noexcept
        {
            // The move is valid whether or not other.mObjects has elements.
            mObjects = std::move(other.mObjects);
            SetPointers(other);
            return *this;
        }

        // Access to the array.  Sample usage is
        //   Array2<T> myArray(3, 2);
        //   T* row1 = myArray[1];
        //   T row1Col2 = myArray[1][2];
        inline size_t GetBound0() const
        {
            return mBound0;
        }

        inline size_t GetBound1() const
        {
            return mBound1;
        }

        inline T const* operator[](int row) const
        {
            return mIndirect1[row];
        }

        inline T* operator[](int row)
        {
            return mIndirect1[row];
        }

    private:
        void SetPointers(T* objects)
        {
            for (size_t i1 = 0; i1 < mBound1; ++i1)
            {
                size_t j0 = mBound0 * i1;  // = bound0*(i1 + j1) where j1 = 0
                mIndirect1[i1] = &objects[j0];
            }
        }

        void SetPointers(Array2 const& other)
        {
            mBound0 = other.mBound0;
            mBound1 = other.mBound1;
            mIndirect1.resize(mBound1);

            if (mBound0 > 0)
            {
                // The objects are owned.
                SetPointers(mObjects.data());
            }
            else if (mIndirect1.size() > 0)
            {
                // The objects are not owned.
                SetPointers(other.mIndirect1[0]);
            }
            // else 'other' is an empty Array2.
        }

        size_t mBound0, mBound1;
        std::vector<T> mObjects;
        std::vector<T*> mIndirect1;
    };
}



// -----------------------------------------------------------------------
// Mathematics/BasisFunction.h
//
// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.08.13

#pragma once

//#include <Mathematics/Logger.h>
//#include <Mathematics/Math.h>
//#include <Mathematics/Array2.h>
#include <array>
#include <cstring>

namespace gte
{
    template <typename Real>
    struct UniqueKnot
    {
        Real t;
        int multiplicity;
    };

    template <typename Real>
    struct BasisFunctionInput
    {
        // The members are uninitialized.
        BasisFunctionInput()
        {
        }

        // Construct an open uniform curve with t in [0,1].
        BasisFunctionInput(int inNumControls, int inDegree)
            :
            numControls(inNumControls),
            degree(inDegree),
            uniform(true),
            periodic(false),
            numUniqueKnots(numControls - degree + 1),
            uniqueKnots(numUniqueKnots)
        {
            uniqueKnots.front().t = (Real)0;
            uniqueKnots.front().multiplicity = degree + 1;
            for (int i = 1; i <= numUniqueKnots - 2; ++i)
            {
                uniqueKnots[i].t = i / (numUniqueKnots - (Real)1);
                uniqueKnots[i].multiplicity = 1;
            }
            uniqueKnots.back().t = (Real)1;
            uniqueKnots.back().multiplicity = degree + 1;
        }

        int numControls;
        int degree;
        bool uniform;
        bool periodic;
        int numUniqueKnots;
        std::vector<UniqueKnot<Real>> uniqueKnots;
    };

    template <typename Real>
    class BasisFunction
    {
    public:
        // Let n be the number of control points. Let d be the degree, where
        // 1 <= d <= n-1.  The number of knots is k = n + d + 1.  The knots
        // are t[i] for 0 <= i < k and must be nondecreasing, t[i] <= t[i+1],
        // but a knot value can be repeated.  Let s be the number of distinct
        // knots.  Let the distinct knots be u[j] for 0 <= j < s, so u[j] <
        // u[j+1] for all j.  The set of u[j] is called a 'breakpoint
        // sequence'.  Let m[j] >= 1 be the multiplicity; that is, if t[i] is
        // the first occurrence of u[j], then t[i+r] = t[i] for 1 <= r < m[j].
        // The multiplicities have the constraints m[0] <= d+1, m[s-1] <= d+1,
        // and m[j] <= d for 1 <= j <= s-2.  Also, k = sum_{j=0}^{s-1} m[j],
        // which says the multiplicities account for all k knots.
        //
        // Given a knot vector (t[0],...,t[n+d]), the domain of the
        // corresponding B-spline curve is the interval [t[d],t[n]].
        //
        // The corresponding B-spline or NURBS curve is characterized as
        // follows.  See "Geometric Modeling with Splines: An Introduction" by
        // Elaine Cohen, Richard F. Riesenfeld and Gershon Elber, AK Peters,
        // 2001, Natick MA.  The curve is 'open' when m[0] = m[s-1] = d+1;
        // otherwise, it is 'floating'.  An open curve is uniform when the
        // knots t[d] through t[n] are equally spaced; that is, t[i+1] - t[i]
        // are a common value for d <= i <= n-1.  By implication, s = n-d+1
        // and m[j] = 1 for 1 <= j <= s-2.  An open curve that does not
        // satisfy these conditions is said to be nonuniform.  A floating
        // curve is uniform when m[j] = 1 for 0 <= j <= s-1 and t[i+1] - t[i]
        // are a common value for 0 <= i <= k-2; otherwise, the floating curve
        // is nonuniform.
        //
        // A special case of a floating curve is a periodic curve.  The intent
        // is that the curve is closed, so the first and last control points
        // should be the same, which ensures C^{0} continuity.  Higher-order
        // continuity is obtained by repeating more control points.  If the
        // control points are P[0] through P[n-1], append the points P[0]
        // through P[d-1] to ensure C^{d-1} continuity.  Additionally, the
        // knots must be chosen properly.  You may choose t[d] through t[n] as
        // you wish.  The other knots are defined by
        //   t[i] - t[i-1] = t[n-d+i] - t[n-d+i-1]
        //   t[n+i] - t[n+i-1] = t[d+i] - t[d+i-1]
        // for 1 <= i <= d.


        // Construction and destruction.  The determination that the curve is
        // open or floating is based on the multiplicities.  The 'uniform'
        // input is used to avoid misclassifications due to floating-point
        // rounding errors.  Specifically, the breakpoints might be equally
        // spaced (uniform) as real numbers, but the floating-point
        // representations can have rounding errors that cause the knot
        // differences not to be exactly the same constant.  A periodic curve
        // can have uniform or nonuniform knots.  This object makes copies of
        // the input arrays.
        BasisFunction()
            :
            mNumControls(0),
            mDegree(0),
            mTMin((Real)0),
            mTMax((Real)0),
            mTLength((Real)0),
            mOpen(false),
            mUniform(false),
            mPeriodic(false)
        {
        }

        BasisFunction(BasisFunctionInput<Real> const& input)
            :
            mNumControls(0),
            mDegree(0),
            mTMin((Real)0),
            mTMax((Real)0),
            mTLength((Real)0),
            mOpen(false),
            mUniform(false),
            mPeriodic(false)
        {
            Create(input);
        }


        ~BasisFunction()
        {
        }

        // No copying is allowed.
        BasisFunction(BasisFunction const&) = delete;
        BasisFunction& operator=(BasisFunction const&) = delete;

        // Support for explicit creation in classes that have std::array
        // members involving BasisFunction.  This is a call-once function.
        void Create(BasisFunctionInput<Real> const& input)
        {
            LogAssert(mNumControls == 0 && mDegree == 0, "Object already created.");
            LogAssert(input.numControls >= 2, "Invalid number of control points.");
            LogAssert(1 <= input.degree && input.degree < input.numControls, "Invalid degree.");
            LogAssert(input.numUniqueKnots >= 2, "Invalid number of unique knots.");

            mNumControls = (input.periodic ? input.numControls + input.degree : input.numControls);
            mDegree = input.degree;
            mTMin = (Real)0;
            mTMax = (Real)0;
            mTLength = (Real)0;
            mOpen = false;
            mUniform = input.uniform;
            mPeriodic = input.periodic;
            for (int i = 0; i < 4; ++i)
            {
                mJet[i] = Array2<Real>();
            }

            mUniqueKnots.resize(input.numUniqueKnots);
            std::copy(input.uniqueKnots.begin(),
                input.uniqueKnots.begin() + input.numUniqueKnots,
                mUniqueKnots.begin());

            Real u = mUniqueKnots.front().t;
            for (int i = 1; i < input.numUniqueKnots - 1; ++i)
            {
                Real uNext = mUniqueKnots[i].t;
                LogAssert(u < uNext, "Unique knots are not strictly increasing.");
                u = uNext;
            }

            int mult0 = mUniqueKnots.front().multiplicity;
            LogAssert(mult0 >= 1 && mult0 <= mDegree + 1, "Invalid first multiplicity.");

            int mult1 = mUniqueKnots.back().multiplicity;
            LogAssert(mult1 >= 1 && mult1 <= mDegree + 1, "Invalid last multiplicity.");

            for (int i = 1; i <= input.numUniqueKnots - 2; ++i)
            {
                int mult = mUniqueKnots[i].multiplicity;
                LogAssert(mult >= 1 && mult <= mDegree + 1, "Invalid interior multiplicity.");
            }

            mOpen = (mult0 == mult1 && mult0 == mDegree + 1);

            mKnots.resize(mNumControls + mDegree + 1);
            mKeys.resize(input.numUniqueKnots);
            int sum = 0;
            for (int i = 0, j = 0; i < input.numUniqueKnots; ++i)
            {
                Real tCommon = mUniqueKnots[i].t;
                int mult = mUniqueKnots[i].multiplicity;
                for (int k = 0; k < mult; ++k, ++j)
                {
                    mKnots[j] = tCommon;
                }

                mKeys[i].first = tCommon;
                mKeys[i].second = sum - 1;
                sum += mult;
            }

            mTMin = mKnots[mDegree];
            mTMax = mKnots[mNumControls];
            mTLength = mTMax - mTMin;

            size_t numRows = mDegree + 1;
            size_t numCols = mNumControls + mDegree;
            size_t numBytes = numRows * numCols * sizeof(Real);
            for (int i = 0; i < 4; ++i)
            {
                mJet[i] = Array2<Real>(numCols, numRows);
                std::memset(mJet[i][0], 0, numBytes);
            }
        }

        // Member access.
        inline int GetNumControls() const
        {
            return mNumControls;
        }

        inline int GetDegree() const
        {
            return mDegree;
        }

        inline int GetNumUniqueKnots() const
        {
            return static_cast<int>(mUniqueKnots.size());
        }

        inline int GetNumKnots() const
        {
            return static_cast<int>(mKnots.size());
        }

        inline Real GetMinDomain() const
        {
            return mTMin;
        }

        inline Real GetMaxDomain() const
        {
            return mTMax;
        }

        inline bool IsOpen() const
        {
            return mOpen;
        }

        inline bool IsUniform() const
        {
            return mUniform;
        }

        inline bool IsPeriodic() const
        {
            return mPeriodic;
        }

        inline UniqueKnot<Real> const* GetUniqueKnots() const
        {
            return &mUniqueKnots[0];
        }

        inline Real const* GetKnots() const
        {
            return &mKnots[0];
        }

        // Evaluation of the basis function and its derivatives through 
        // order 3.  For the function value only, pass order 0.  For the
        // function and first derivative, pass order 1, and so on.
        void Evaluate(Real t, unsigned int order, int& minIndex, int& maxIndex) const
        {
            LogAssert(order <= 3, "Invalid order.");

            int i = GetIndex(t);
            mJet[0][0][i] = (Real)1;

            if (order >= 1)
            {
                mJet[1][0][i] = (Real)0;
                if (order >= 2)
                {
                    mJet[2][0][i] = (Real)0;
                    if (order >= 3)
                    {
                        mJet[3][0][i] = (Real)0;
                    }
                }
            }

            Real n0 = t - mKnots[i], n1 = mKnots[i + 1] - t;
            Real e0, e1, d0, d1, invD0, invD1;
            int j;
            for (j = 1; j <= mDegree; j++)
            {
                d0 = mKnots[i + j] - mKnots[i];
                d1 = mKnots[i + 1] - mKnots[i - j + 1];
                invD0 = (d0 > (Real)0 ? (Real)1 / d0 : (Real)0);
                invD1 = (d1 > (Real)0 ? (Real)1 / d1 : (Real)0);

                e0 = n0 * mJet[0][j - 1][i];
                mJet[0][j][i] = e0 * invD0;
                e1 = n1 * mJet[0][j - 1][i - j + 1];
                mJet[0][j][i - j] = e1 * invD1;

                if (order >= 1)
                {
                    e0 = n0 * mJet[1][j - 1][i] + mJet[0][j - 1][i];
                    mJet[1][j][i] = e0 * invD0;
                    e1 = n1 * mJet[1][j - 1][i - j + 1] - mJet[0][j - 1][i - j + 1];
                    mJet[1][j][i - j] = e1 * invD1;

                    if (order >= 2)
                    {
                        e0 = n0 * mJet[2][j - 1][i] + ((Real)2) * mJet[1][j - 1][i];
                        mJet[2][j][i] = e0 * invD0;
                        e1 = n1 * mJet[2][j - 1][i - j + 1] - ((Real)2) * mJet[1][j - 1][i - j + 1];
                        mJet[2][j][i - j] = e1 * invD1;

                        if (order >= 3)
                        {
                            e0 = n0 * mJet[3][j - 1][i] + ((Real)3) * mJet[2][j - 1][i];
                            mJet[3][j][i] = e0 * invD0;
                            e1 = n1 * mJet[3][j - 1][i - j + 1] - ((Real)3) * mJet[2][j - 1][i - j + 1];
                            mJet[3][j][i - j] = e1 * invD1;
                        }
                    }
                }
            }

            for (j = 2; j <= mDegree; ++j)
            {
                for (int k = i - j + 1; k < i; ++k)
                {
                    n0 = t - mKnots[k];
                    n1 = mKnots[k + j + 1] - t;
                    d0 = mKnots[k + j] - mKnots[k];
                    d1 = mKnots[k + j + 1] - mKnots[k + 1];
                    invD0 = (d0 > (Real)0 ? (Real)1 / d0 : (Real)0);
                    invD1 = (d1 > (Real)0 ? (Real)1 / d1 : (Real)0);

                    e0 = n0 * mJet[0][j - 1][k];
                    e1 = n1 * mJet[0][j - 1][k + 1];
                    mJet[0][j][k] = e0 * invD0 + e1 * invD1;

                    if (order >= 1)
                    {
                        e0 = n0 * mJet[1][j - 1][k] + mJet[0][j - 1][k];
                        e1 = n1 * mJet[1][j - 1][k + 1] - mJet[0][j - 1][k + 1];
                        mJet[1][j][k] = e0 * invD0 + e1 * invD1;

                        if (order >= 2)
                        {
                            e0 = n0 * mJet[2][j - 1][k] + ((Real)2) * mJet[1][j - 1][k];
                            e1 = n1 * mJet[2][j - 1][k + 1] - ((Real)2) * mJet[1][j - 1][k + 1];
                            mJet[2][j][k] = e0 * invD0 + e1 * invD1;

                            if (order >= 3)
                            {
                                e0 = n0 * mJet[3][j - 1][k] + ((Real)3) * mJet[2][j - 1][k];
                                e1 = n1 * mJet[3][j - 1][k + 1] - ((Real)3) * mJet[2][j - 1][k + 1];
                                mJet[3][j][k] = e0 * invD0 + e1 * invD1;
                            }
                        }
                    }
                }
            }

            minIndex = i - mDegree;
            maxIndex = i;
        }

        // Access the results of the call to Evaluate(...).  The index i must
        // satisfy minIndex <= i <= maxIndex.  If it is not, the function
        // returns zero.  The separation of evaluation and access is based on
        // local control of the basis function; that is, only the accessible
        // values are (potentially) not zero.
        Real GetValue(unsigned int order, int i) const
        {
            if (order < 4)
            {
                if (0 <= i && i < mNumControls + mDegree)
                {
                    return mJet[order][mDegree][i];
                }
                LogError("Invalid index.");
            }
            LogError("Invalid order.");
        }

    private:
        // Determine the index i for which knot[i] <= t < knot[i+1].  The
        // t-value is modified (wrapped for periodic splines, clamped for
        // nonperiodic splines).
        int GetIndex(Real& t) const
        {
            // Find the index i for which knot[i] <= t < knot[i+1].
            if (mPeriodic)
            {
                // Wrap to [tmin,tmax].
                Real r = std::fmod(t - mTMin, mTLength);
                if (r < (Real)0)
                {
                    r += mTLength;
                }
                t = mTMin + r;
            }

            // Clamp to [tmin,tmax].  For the periodic case, this handles
            // small numerical rounding errors near the domain endpoints.
            if (t <= mTMin)
            {
                t = mTMin;
                return mDegree;
            }
            if (t >= mTMax)
            {
                t = mTMax;
                return mNumControls - 1;
            }

            // At this point, tmin < t < tmax.
            for (auto const& key : mKeys)
            {
                if (t < key.first)
                {
                    return key.second;
                }
            }

            // We should not reach this code.
            LogError("Unexpected condition.");
        }

        // Constructor inputs and values derived from them.
        int mNumControls;
        int mDegree;
        Real mTMin, mTMax, mTLength;
        bool mOpen;
        bool mUniform;
        bool mPeriodic;
        std::vector<UniqueKnot<Real>> mUniqueKnots;
        std::vector<Real> mKnots;

        // Lookup information for the GetIndex() function.  The first element
        // of the pair is a unique knot value.  The second element is the
        // index in mKnots[] for the last occurrence of that knot value.
        std::vector<std::pair<Real, int>> mKeys;

        // Storage for the basis functions and their first three derivatives;
        // mJet[i] is array[d+1][n+d].
        mutable std::array<Array2<Real>, 4> mJet;
    };
}

// -----------------------------------------------------------------------
// Mathematics/RootsPolynomia.h
//
// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.12.05

#pragma once

//#include <Mathematics/Math.h>
#include <algorithm>
#include <map>
#include <vector>

// The Find functions return the number of roots, if any, and this number
// of elements of the outputs are valid.  If the polynomial is identically
// zero, Find returns 1.
//
// Some root-bounding algorithms for real-valued roots are mentioned next for
// the polynomial p(t) = c[0] + c[1]*t + ... + c[d-1]*t^{d-1} + c[d]*t^d.
//
// 1. The roots must be contained by the interval [-M,M] where
//   M = 1 + max{|c[0]|, ..., |c[d-1]|}/|c[d]| >= 1
// is called the Cauchy bound.
//
// 2. You may search for roots in the interval [-1,1].  Define
//   q(t) = t^d*p(1/t) = c[0]*t^d + c[1]*t^{d-1} + ... + c[d-1]*t + c[d]
// The roots of p(t) not in [-1,1] are the roots of q(t) in [-1,1].
//
// 3. Between two consecutive roots of the derivative p'(t), say, r0 < r1,
// the function p(t) is strictly monotonic on the open interval (r0,r1).
// If additionally, p(r0) * p(r1) <= 0, then p(x) has a unique root on
// the closed interval [r0,r1].  Thus, one can compute the derivatives
// through order d for p(t), find roots for the derivative of order k+1,
// then use these to bound roots for the derivative of order k.
//
// 4. Sturm sequences of polynomials may be used to determine bounds on the
// roots.  This is a more sophisticated approach to root bounding than item 3.
// Moreover, a Sturm sequence allows you to compute the number of real-valued
// roots on a specified interval.
//
// 5. For the low-degree Solve* functions, see
// https://www.geometrictools.com/Documentation/LowDegreePolynomialRoots.pdf

// FOR INTERNAL USE ONLY (unit testing).  Do not define the symbol
// GTE_ROOTS_LOW_DEGREE_UNIT_TEST in your own code.
#if defined(GTE_ROOTS_LOW_DEGREE_UNIT_TEST)
extern void RootsLowDegreeBlock(int);
#define GTE_ROOTS_LOW_DEGREE_BLOCK(block) RootsLowDegreeBlock(block)
#else
#define GTE_ROOTS_LOW_DEGREE_BLOCK(block)
#endif

namespace gte
{
    template <typename Real>
    class RootsPolynomial
    {
    public:
        // Low-degree root finders.  These use exact rational arithmetic for
        // theoretically correct root classification.  The roots themselves
        // are computed with mixed types (rational and floating-point
        // arithmetic).  The Rational type must support rational arithmetic
        // (+, -, *, /); for example, BSRational<UIntegerAP32> suffices.  The
        // Rational class must have single-input constructors where the input
        // is type Real.  This ensures you can call the Solve* functions with
        // floating-point inputs; they will be converted to Rational
        // implicitly.  The highest-order coefficients must be nonzero
        // (p2 != 0 for quadratic, p3 != 0 for cubic, and p4 != 0 for
        // quartic).

        template <typename Rational>
        static void SolveQuadratic(Rational const& p0, Rational const& p1,
            Rational const& p2, std::map<Real, int>& rmMap)
        {
            Rational const rat2 = 2;
            Rational q0 = p0 / p2;
            Rational q1 = p1 / p2;
            Rational q1half = q1 / rat2;
            Rational c0 = q0 - q1half * q1half;

            std::map<Rational, int> rmLocalMap;
            SolveDepressedQuadratic(c0, rmLocalMap);

            rmMap.clear();
            for (auto& rm : rmLocalMap)
            {
                Rational root = rm.first - q1half;
                rmMap.insert(std::make_pair((Real)root, rm.second));
            }
        }

        template <typename Rational>
        static void SolveCubic(Rational const& p0, Rational const& p1,
            Rational const& p2, Rational const& p3, std::map<Real, int>& rmMap)
        {
            Rational const rat2 = 2, rat3 = 3;
            Rational q0 = p0 / p3;
            Rational q1 = p1 / p3;
            Rational q2 = p2 / p3;
            Rational q2third = q2 / rat3;
            Rational c0 = q0 - q2third * (q1 - rat2 * q2third * q2third);
            Rational c1 = q1 - q2 * q2third;

            std::map<Rational, int> rmLocalMap;
            SolveDepressedCubic(c0, c1, rmLocalMap);

            rmMap.clear();
            for (auto& rm : rmLocalMap)
            {
                Rational root = rm.first - q2third;
                rmMap.insert(std::make_pair((Real)root, rm.second));
            }
        }

        template <typename Rational>
        static void SolveQuartic(Rational const& p0, Rational const& p1,
            Rational const& p2, Rational const& p3, Rational const& p4,
            std::map<Real, int>& rmMap)
        {
            Rational const rat2 = 2, rat3 = 3, rat4 = 4, rat6 = 6;
            Rational q0 = p0 / p4;
            Rational q1 = p1 / p4;
            Rational q2 = p2 / p4;
            Rational q3 = p3 / p4;
            Rational q3fourth = q3 / rat4;
            Rational q3fourthSqr = q3fourth * q3fourth;
            Rational c0 = q0 - q3fourth * (q1 - q3fourth * (q2 - q3fourthSqr * rat3));
            Rational c1 = q1 - rat2 * q3fourth * (q2 - rat4 * q3fourthSqr);
            Rational c2 = q2 - rat6 * q3fourthSqr;

            std::map<Rational, int> rmLocalMap;
            SolveDepressedQuartic(c0, c1, c2, rmLocalMap);

            rmMap.clear();
            for (auto& rm : rmLocalMap)
            {
                Rational root = rm.first - q3fourth;
                rmMap.insert(std::make_pair((Real)root, rm.second));
            }
        }

        // Return only the number of real-valued roots and their
        // multiplicities.  info.size() is the number of real-valued roots
        // and info[i] is the multiplicity of root corresponding to index i.
        template <typename Rational>
        static void GetRootInfoQuadratic(Rational const& p0, Rational const& p1,
            Rational const& p2, std::vector<int>& info)
        {
            Rational const rat2 = 2;
            Rational q0 = p0 / p2;
            Rational q1 = p1 / p2;
            Rational q1half = q1 / rat2;
            Rational c0 = q0 - q1half * q1half;

            info.clear();
            info.reserve(2);
            GetRootInfoDepressedQuadratic(c0, info);
        }

        template <typename Rational>
        static void GetRootInfoCubic(Rational const& p0, Rational const& p1,
            Rational const& p2, Rational const& p3, std::vector<int>& info)
        {
            Rational const rat2 = 2, rat3 = 3;
            Rational q0 = p0 / p3;
            Rational q1 = p1 / p3;
            Rational q2 = p2 / p3;
            Rational q2third = q2 / rat3;
            Rational c0 = q0 - q2third * (q1 - rat2 * q2third * q2third);
            Rational c1 = q1 - q2 * q2third;

            info.clear();
            info.reserve(3);
            GetRootInfoDepressedCubic(c0, c1, info);
        }

        template <typename Rational>
        static void GetRootInfoQuartic(Rational const& p0, Rational const& p1,
            Rational const& p2, Rational const& p3, Rational const& p4,
            std::vector<int>& info)
        {
            Rational const rat2 = 2, rat3 = 3, rat4 = 4, rat6 = 6;
            Rational q0 = p0 / p4;
            Rational q1 = p1 / p4;
            Rational q2 = p2 / p4;
            Rational q3 = p3 / p4;
            Rational q3fourth = q3 / rat4;
            Rational q3fourthSqr = q3fourth * q3fourth;
            Rational c0 = q0 - q3fourth * (q1 - q3fourth * (q2 - q3fourthSqr * rat3));
            Rational c1 = q1 - rat2 * q3fourth * (q2 - rat4 * q3fourthSqr);
            Rational c2 = q2 - rat6 * q3fourthSqr;

            info.clear();
            info.reserve(4);
            GetRootInfoDepressedQuartic(c0, c1, c2, info);
        }

        // General equations: sum_{i=0}^{d} c(i)*t^i = 0.  The input array 'c'
        // must have at least d+1 elements and the output array 'root' must
        // have at least d elements.

        // Find the roots on (-infinity,+infinity).
        static int Find(int degree, Real const* c, unsigned int maxIterations, Real* roots)
        {
            if (degree >= 0 && c)
            {
                Real const zero = (Real)0;
                while (degree >= 0 && c[degree] == zero)
                {
                    --degree;
                }

                if (degree > 0)
                {
                    // Compute the Cauchy bound.
                    Real const one = (Real)1;
                    Real invLeading = one / c[degree];
                    Real maxValue = zero;
                    for (int i = 0; i < degree; ++i)
                    {
                        Real value = std::fabs(c[i] * invLeading);
                        if (value > maxValue)
                        {
                            maxValue = value;
                        }
                    }
                    Real bound = one + maxValue;

                    return FindRecursive(degree, c, -bound, bound, maxIterations,
                        roots);
                }
                else if (degree == 0)
                {
                    // The polynomial is a nonzero constant.
                    return 0;
                }
                else
                {
                    // The polynomial is identically zero.
                    roots[0] = zero;
                    return 1;
                }
            }
            else
            {
                // Invalid degree or c.
                return 0;
            }
        }

        // If you know that p(tmin) * p(tmax) <= 0, then there must be at
        // least one root in [tmin, tmax].  Compute it using bisection.
        static bool Find(int degree, Real const* c, Real tmin, Real tmax,
            unsigned int maxIterations, Real& root)
        {
            Real const zero = (Real)0;
            Real pmin = Evaluate(degree, c, tmin);
            if (pmin == zero)
            {
                root = tmin;
                return true;
            }
            Real pmax = Evaluate(degree, c, tmax);
            if (pmax == zero)
            {
                root = tmax;
                return true;
            }

            if (pmin * pmax > zero)
            {
                // It is not known whether the interval bounds a root.
                return false;
            }

            if (tmin >= tmax)
            {
                // Invalid ordering of interval endpoitns. 
                return false;
            }

            for (unsigned int i = 1; i <= maxIterations; ++i)
            {
                root = ((Real)0.5) * (tmin + tmax);

                // This test is designed for 'float' or 'double' when tmin
                // and tmax are consecutive floating-point numbers.
                if (root == tmin || root == tmax)
                {
                    break;
                }

                Real p = Evaluate(degree, c, root);
                Real product = p * pmin;
                if (product < zero)
                {
                    tmax = root;
                    pmax = p;
                }
                else if (product > zero)
                {
                    tmin = root;
                    pmin = p;
                }
                else
                {
                    break;
                }
            }

            return true;
        }

    private:
        // Support for the Solve* functions.
        template <typename Rational>
        static void SolveDepressedQuadratic(Rational const& c0,
            std::map<Rational, int>& rmMap)
        {
            Rational const zero = 0;
            if (c0 < zero)
            {
                // Two simple roots.
                Rational root1 = (Rational)std::sqrt((double)-c0);
                Rational root0 = -root1;
                rmMap.insert(std::make_pair(root0, 1));
                rmMap.insert(std::make_pair(root1, 1));
                GTE_ROOTS_LOW_DEGREE_BLOCK(0);
            }
            else if (c0 == zero)
            {
                // One double root.
                rmMap.insert(std::make_pair(zero, 2));
                GTE_ROOTS_LOW_DEGREE_BLOCK(1);
            }
            else  // c0 > 0
            {
                // A complex-conjugate pair of roots.
                // Complex z0 = -q1/2 - i*sqrt(c0);
                // Complex z0conj = -q1/2 + i*sqrt(c0);
                GTE_ROOTS_LOW_DEGREE_BLOCK(2);
            }
        }

        template <typename Rational>
        static void SolveDepressedCubic(Rational const& c0, Rational const& c1,
            std::map<Rational, int>& rmMap)
        {
            // Handle the special case of c0 = 0, in which case the polynomial
            // reduces to a depressed quadratic.
            Rational const zero = 0;
            if (c0 == zero)
            {
                SolveDepressedQuadratic(c1, rmMap);
                auto iter = rmMap.find(zero);
                if (iter != rmMap.end())
                {
                    // The quadratic has a root of zero, so the multiplicity
                    // must be increased.
                    ++iter->second;
                    GTE_ROOTS_LOW_DEGREE_BLOCK(3);
                }
                else
                {
                    // The quadratic does not have a root of zero.  Insert the
                    // one for the cubic.
                    rmMap.insert(std::make_pair(zero, 1));
                    GTE_ROOTS_LOW_DEGREE_BLOCK(4);
                }
                return;
            }

            // Handle the special case of c0 != 0 and c1 = 0.
            double const oneThird = 1.0 / 3.0;
            if (c1 == zero)
            {
                // One simple real root.
                Rational root0;
                if (c0 > zero)
                {
                    root0 = (Rational)-std::pow((double)c0, oneThird);
                    GTE_ROOTS_LOW_DEGREE_BLOCK(5);
                }
                else
                {
                    root0 = (Rational)std::pow(-(double)c0, oneThird);
                    GTE_ROOTS_LOW_DEGREE_BLOCK(6);
                }
                rmMap.insert(std::make_pair(root0, 1));

                // One complex conjugate pair.
                // Complex z0 = root0*(-1 - i*sqrt(3))/2;
                // Complex z0conj = root0*(-1 + i*sqrt(3))/2;
                return;
            }

            // At this time, c0 != 0 and c1 != 0.
            Rational const rat2 = 2, rat3 = 3, rat4 = 4, rat27 = 27, rat108 = 108;
            Rational delta = -(rat4 * c1 * c1 * c1 + rat27 * c0 * c0);
            if (delta > zero)
            {
                // Three simple roots.
                Rational deltaDiv108 = delta / rat108;
                Rational betaRe = -c0 / rat2;
                Rational betaIm = std::sqrt(deltaDiv108);
                Rational theta = std::atan2(betaIm, betaRe);
                Rational thetaDiv3 = theta / rat3;
                double angle = (double)thetaDiv3;
                Rational cs = (Rational)std::cos(angle);
                Rational sn = (Rational)std::sin(angle);
                Rational rhoSqr = betaRe * betaRe + betaIm * betaIm;
                Rational rhoPowThird = (Rational)std::pow((double)rhoSqr, 1.0 / 6.0);
                Rational temp0 = rhoPowThird * cs;
                Rational temp1 = rhoPowThird * sn * (Rational)std::sqrt(3.0);
                Rational root0 = rat2 * temp0;
                Rational root1 = -temp0 - temp1;
                Rational root2 = -temp0 + temp1;
                rmMap.insert(std::make_pair(root0, 1));
                rmMap.insert(std::make_pair(root1, 1));
                rmMap.insert(std::make_pair(root2, 1));
                GTE_ROOTS_LOW_DEGREE_BLOCK(7);
            }
            else if (delta < zero)
            {
                // One simple root.
                Rational deltaDiv108 = delta / rat108;
                Rational temp0 = -c0 / rat2;
                Rational temp1 = (Rational)std::sqrt(-(double)deltaDiv108);
                Rational temp2 = temp0 - temp1;
                Rational temp3 = temp0 + temp1;
                if (temp2 >= zero)
                {
                    temp2 = (Rational)std::pow((double)temp2, oneThird);
                    GTE_ROOTS_LOW_DEGREE_BLOCK(8);
                }
                else
                {
                    temp2 = (Rational)-std::pow(-(double)temp2, oneThird);
                    GTE_ROOTS_LOW_DEGREE_BLOCK(9);
                }
                if (temp3 >= zero)
                {
                    temp3 = (Rational)std::pow((double)temp3, oneThird);
                    GTE_ROOTS_LOW_DEGREE_BLOCK(10);
                }
                else
                {
                    temp3 = (Rational)-std::pow(-(double)temp3, oneThird);
                    GTE_ROOTS_LOW_DEGREE_BLOCK(11);
                }
                Rational root0 = temp2 + temp3;
                rmMap.insert(std::make_pair(root0, 1));

                // One complex conjugate pair.
                // Complex z0 = (-root0 - i*sqrt(3*root0*root0+4*c1))/2;
                // Complex z0conj = (-root0 + i*sqrt(3*root0*root0+4*c1))/2;
            }
            else  // delta = 0
            {
                // One simple root and one double root.
                Rational root0 = -rat3 * c0 / (rat2 * c1);
                Rational root1 = -rat2 * root0;
                rmMap.insert(std::make_pair(root0, 2));
                rmMap.insert(std::make_pair(root1, 1));
                GTE_ROOTS_LOW_DEGREE_BLOCK(12);
            }
        }

        template <typename Rational>
        static void SolveDepressedQuartic(Rational const& c0, Rational const& c1,
            Rational const& c2, std::map<Rational, int>& rmMap)
        {
            // Handle the special case of c0 = 0, in which case the polynomial
            // reduces to a depressed cubic.
            Rational const zero = 0;
            if (c0 == zero)
            {
                SolveDepressedCubic(c1, c2, rmMap);
                auto iter = rmMap.find(zero);
                if (iter != rmMap.end())
                {
                    // The cubic has a root of zero, so the multiplicity must
                    // be increased.
                    ++iter->second;
                    GTE_ROOTS_LOW_DEGREE_BLOCK(13);
                }
                else
                {
                    // The cubic does not have a root of zero.  Insert the one
                    // for the quartic.
                    rmMap.insert(std::make_pair(zero, 1));
                    GTE_ROOTS_LOW_DEGREE_BLOCK(14);
                }
                return;
            }

            // Handle the special case of c1 = 0, in which case the quartic is
            // a biquadratic
            //   x^4 + c1*x^2 + c0 = (x^2 + c2/2)^2 + (c0 - c2^2/4)
            if (c1 == zero)
            {
                SolveBiquadratic(c0, c2, rmMap);
                return;
            }

            // At this time, c0 != 0 and c1 != 0, which is a requirement for
            // the general solver that must use a root of a special cubic
            // polynomial.
            Rational const rat2 = 2, rat4 = 4, rat8 = 8, rat12 = 12, rat16 = 16;
            Rational const rat27 = 27, rat36 = 36;
            Rational c0sqr = c0 * c0, c1sqr = c1 * c1, c2sqr = c2 * c2;
            Rational delta = c1sqr * (-rat27 * c1sqr + rat4 * c2 *
                (rat36 * c0 - c2sqr)) + rat16 * c0 * (c2sqr * (c2sqr - rat8 * c0) +
                    rat16 * c0sqr);
            Rational a0 = rat12 * c0 + c2sqr;
            Rational a1 = rat4 * c0 - c2sqr;

            if (delta > zero)
            {
                if (c2 < zero && a1 < zero)
                {
                    // Four simple real roots.
                    std::map<Real, int> rmCubicMap;
                    SolveCubic(c1sqr - rat4 * c0 * c2, rat8 * c0, rat4 * c2, -rat8, rmCubicMap);
                    Rational t = (Rational)rmCubicMap.rbegin()->first;
                    Rational alphaSqr = rat2 * t - c2;
                    Rational alpha = (Rational)std::sqrt((double)alphaSqr);
                    double sgnC1;
                    if (c1 > zero)
                    {
                        sgnC1 = 1.0;
                        GTE_ROOTS_LOW_DEGREE_BLOCK(15);
                    }
                    else
                    {
                        sgnC1 = -1.0;
                        GTE_ROOTS_LOW_DEGREE_BLOCK(16);
                    }
                    Rational arg = t * t - c0;
                    Rational beta = (Rational)(sgnC1 * std::sqrt(std::max((double)arg, 0.0)));
                    Rational D0 = alphaSqr - rat4 * (t + beta);
                    Rational sqrtD0 = (Rational)std::sqrt(std::max((double)D0, 0.0));
                    Rational D1 = alphaSqr - rat4 * (t - beta);
                    Rational sqrtD1 = (Rational)std::sqrt(std::max((double)D1, 0.0));
                    Rational root0 = (alpha - sqrtD0) / rat2;
                    Rational root1 = (alpha + sqrtD0) / rat2;
                    Rational root2 = (-alpha - sqrtD1) / rat2;
                    Rational root3 = (-alpha + sqrtD1) / rat2;
                    rmMap.insert(std::make_pair(root0, 1));
                    rmMap.insert(std::make_pair(root1, 1));
                    rmMap.insert(std::make_pair(root2, 1));
                    rmMap.insert(std::make_pair(root3, 1));
                }
                else // c2 >= 0 or a1 >= 0
                {
                    // Two complex-conjugate pairs.  The values alpha, D0
                    // and D1 are those of the if-block.
                    // Complex z0 = (alpha - i*sqrt(-D0))/2;
                    // Complex z0conj = (alpha + i*sqrt(-D0))/2;
                    // Complex z1 = (-alpha - i*sqrt(-D1))/2;
                    // Complex z1conj = (-alpha + i*sqrt(-D1))/2;
                    GTE_ROOTS_LOW_DEGREE_BLOCK(17);
                }
            }
            else if (delta < zero)
            {
                // Two simple real roots, one complex-conjugate pair.
                std::map<Real, int> rmCubicMap;
                SolveCubic(c1sqr - rat4 * c0 * c2, rat8 * c0, rat4 * c2, -rat8,
                    rmCubicMap);
                Rational t = (Rational)rmCubicMap.rbegin()->first;
                Rational alphaSqr = rat2 * t - c2;
                Rational alpha = (Rational)std::sqrt(std::max((double)alphaSqr, 0.0));
                double sgnC1;
                if (c1 > zero)
                {
                    sgnC1 = 1.0;  // Leads to BLOCK(18)
                }
                else
                {
                    sgnC1 = -1.0;  // Leads to BLOCK(19)
                }
                Rational arg = t * t - c0;
                Rational beta = (Rational)(sgnC1 * std::sqrt(std::max((double)arg, 0.0)));
                Rational root0, root1;
                if (sgnC1 > 0.0)
                {
                    Rational D1 = alphaSqr - rat4 * (t - beta);
                    Rational sqrtD1 = (Rational)std::sqrt(std::max((double)D1, 0.0));
                    root0 = (-alpha - sqrtD1) / rat2;
                    root1 = (-alpha + sqrtD1) / rat2;

                    // One complex conjugate pair.
                    // Complex z0 = (alpha - i*sqrt(-D0))/2;
                    // Complex z0conj = (alpha + i*sqrt(-D0))/2;
                    GTE_ROOTS_LOW_DEGREE_BLOCK(18);
                }
                else
                {
                    Rational D0 = alphaSqr - rat4 * (t + beta);
                    Rational sqrtD0 = (Rational)std::sqrt(std::max((double)D0, 0.0));
                    root0 = (alpha - sqrtD0) / rat2;
                    root1 = (alpha + sqrtD0) / rat2;

                    // One complex conjugate pair.
                    // Complex z0 = (-alpha - i*sqrt(-D1))/2;
                    // Complex z0conj = (-alpha + i*sqrt(-D1))/2;
                    GTE_ROOTS_LOW_DEGREE_BLOCK(19);
                }
                rmMap.insert(std::make_pair(root0, 1));
                rmMap.insert(std::make_pair(root1, 1));
            }
            else  // delta = 0
            {
                if (a1 > zero || (c2 > zero && (a1 != zero || c1 != zero)))
                {
                    // One double real root, one complex-conjugate pair.
                    Rational const rat9 = 9;
                    Rational root0 = -c1 * a0 / (rat9 * c1sqr - rat2 * c2 * a1);
                    rmMap.insert(std::make_pair(root0, 2));

                    // One complex conjugate pair.
                    // Complex z0 = -root0 - i*sqrt(c2 + root0^2);
                    // Complex z0conj = -root0 + i*sqrt(c2 + root0^2);
                    GTE_ROOTS_LOW_DEGREE_BLOCK(20);
                }
                else
                {
                    Rational const rat3 = 3;
                    if (a0 != zero)
                    {
                        // One double real root, two simple real roots.
                        Rational const rat9 = 9;
                        Rational root0 = -c1 * a0 / (rat9 * c1sqr - rat2 * c2 * a1);
                        Rational alpha = rat2 * root0;
                        Rational beta = c2 + rat3 * root0 * root0;
                        Rational discr = alpha * alpha - rat4 * beta;
                        Rational temp1 = (Rational)std::sqrt((double)discr);
                        Rational root1 = (-alpha - temp1) / rat2;
                        Rational root2 = (-alpha + temp1) / rat2;
                        rmMap.insert(std::make_pair(root0, 2));
                        rmMap.insert(std::make_pair(root1, 1));
                        rmMap.insert(std::make_pair(root2, 1));
                        GTE_ROOTS_LOW_DEGREE_BLOCK(21);
                    }
                    else
                    {
                        // One triple real root, one simple real root.
                        Rational root0 = -rat3 * c1 / (rat4 * c2);
                        Rational root1 = -rat3 * root0;
                        rmMap.insert(std::make_pair(root0, 3));
                        rmMap.insert(std::make_pair(root1, 1));
                        GTE_ROOTS_LOW_DEGREE_BLOCK(22);
                    }
                }
            }
        }

        template <typename Rational>
        static void SolveBiquadratic(Rational const& c0, Rational const& c2,
            std::map<Rational, int>& rmMap)
        {
            // Solve 0 = x^4 + c2*x^2 + c0 = (x^2 + c2/2)^2 + a1, where
            // a1 = c0 - c2^2/2.  We know that c0 != 0 at the time of the
            // function call, so x = 0 is not a root.  The condition c1 = 0
            // implies the quartic Delta = 256*c0*a1^2.

            Rational const zero = 0, rat2 = 2, rat256 = 256;
            Rational c2Half = c2 / rat2;
            Rational a1 = c0 - c2Half * c2Half;
            Rational delta = rat256 * c0 * a1 * a1;
            if (delta > zero)
            {
                if (c2 < zero)
                {
                    if (a1 < zero)
                    {
                        // Four simple roots.
                        Rational temp0 = (Rational)std::sqrt(-(double)a1);
                        Rational temp1 = -c2Half - temp0;
                        Rational temp2 = -c2Half + temp0;
                        Rational root1 = (Rational)std::sqrt((double)temp1);
                        Rational root0 = -root1;
                        Rational root2 = (Rational)std::sqrt((double)temp2);
                        Rational root3 = -root2;
                        rmMap.insert(std::make_pair(root0, 1));
                        rmMap.insert(std::make_pair(root1, 1));
                        rmMap.insert(std::make_pair(root2, 1));
                        rmMap.insert(std::make_pair(root3, 1));
                        GTE_ROOTS_LOW_DEGREE_BLOCK(23);
                    }
                    else  // a1 > 0
                    {
                        // Two simple complex conjugate pairs.
                        // double thetaDiv2 = atan2(sqrt(a1), -c2/2) / 2.0;
                        // double cs = cos(thetaDiv2), sn = sin(thetaDiv2);
                        // double length = pow(c0, 0.25);
                        // Complex z0 = length*(cs + i*sn);
                        // Complex z0conj = length*(cs - i*sn);
                        // Complex z1 = length*(-cs + i*sn);
                        // Complex z1conj = length*(-cs - i*sn);
                        GTE_ROOTS_LOW_DEGREE_BLOCK(24);
                    }
                }
                else  // c2 >= 0
                {
                    // Two simple complex conjugate pairs.
                    // Complex z0 = -i*sqrt(c2/2 - sqrt(-a1));
                    // Complex z0conj = +i*sqrt(c2/2 - sqrt(-a1));
                    // Complex z1 = -i*sqrt(c2/2 + sqrt(-a1));
                    // Complex z1conj = +i*sqrt(c2/2 + sqrt(-a1));
                    GTE_ROOTS_LOW_DEGREE_BLOCK(25);
                }
            }
            else if (delta < zero)
            {
                // Two simple real roots.
                Rational temp0 = (Rational)std::sqrt(-(double)a1);
                Rational temp1 = -c2Half + temp0;
                Rational root1 = (Rational)std::sqrt((double)temp1);
                Rational root0 = -root1;
                rmMap.insert(std::make_pair(root0, 1));
                rmMap.insert(std::make_pair(root1, 1));

                // One complex conjugate pair.
                // Complex z0 = -i*sqrt(c2/2 + sqrt(-a1));
                // Complex z0conj = +i*sqrt(c2/2 + sqrt(-a1));
                GTE_ROOTS_LOW_DEGREE_BLOCK(26);
            }
            else  // delta = 0
            {
                if (c2 < zero)
                {
                    // Two double real roots.
                    Rational root1 = (Rational)std::sqrt(-(double)c2Half);
                    Rational root0 = -root1;
                    rmMap.insert(std::make_pair(root0, 2));
                    rmMap.insert(std::make_pair(root1, 2));
                    GTE_ROOTS_LOW_DEGREE_BLOCK(27);
                }
                else  // c2 > 0
                {
                    // Two double complex conjugate pairs.
                    // Complex z0 = -i*sqrt(c2/2);  // multiplicity 2
                    // Complex z0conj = +i*sqrt(c2/2);  // multiplicity 2
                    GTE_ROOTS_LOW_DEGREE_BLOCK(28);
                }
            }
        }

        // Support for the GetNumRoots* functions.
        template <typename Rational>
        static void GetRootInfoDepressedQuadratic(Rational const& c0,
            std::vector<int>& info)
        {
            Rational const zero = 0;
            if (c0 < zero)
            {
                // Two simple roots.
                info.push_back(1);
                info.push_back(1);
            }
            else if (c0 == zero)
            {
                // One double root.
                info.push_back(2);  // root is zero
            }
            else  // c0 > 0
            {
                // A complex-conjugate pair of roots.
            }
        }

        template <typename Rational>
        static void GetRootInfoDepressedCubic(Rational const& c0,
            Rational const& c1, std::vector<int>& info)
        {
            // Handle the special case of c0 = 0, in which case the polynomial
            // reduces to a depressed quadratic.
            Rational const zero = 0;
            if (c0 == zero)
            {
                if (c1 == zero)
                {
                    info.push_back(3);  // triple root of zero
                }
                else
                {
                    info.push_back(1);  // simple root of zero
                    GetRootInfoDepressedQuadratic(c1, info);
                }
                return;
            }

            Rational const rat4 = 4, rat27 = 27;
            Rational delta = -(rat4 * c1 * c1 * c1 + rat27 * c0 * c0);
            if (delta > zero)
            {
                // Three simple real roots.
                info.push_back(1);
                info.push_back(1);
                info.push_back(1);
            }
            else if (delta < zero)
            {
                // One simple real root.
                info.push_back(1);
            }
            else  // delta = 0
            {
                // One simple real root and one double real root.
                info.push_back(1);
                info.push_back(2);
            }
        }

        template <typename Rational>
        static void GetRootInfoDepressedQuartic(Rational const& c0,
            Rational const& c1, Rational const& c2, std::vector<int>& info)
        {
            // Handle the special case of c0 = 0, in which case the polynomial
            // reduces to a depressed cubic.
            Rational const zero = 0;
            if (c0 == zero)
            {
                if (c1 == zero)
                {
                    if (c2 == zero)
                    {
                        info.push_back(4);  // quadruple root of zero
                    }
                    else
                    {
                        info.push_back(2);  // double root of zero
                        GetRootInfoDepressedQuadratic(c2, info);
                    }
                }
                else
                {
                    info.push_back(1);  // simple root of zero
                    GetRootInfoDepressedCubic(c1, c2, info);
                }
                return;
            }

            // Handle the special case of c1 = 0, in which case the quartic is
            // a biquadratic
            //   x^4 + c1*x^2 + c0 = (x^2 + c2/2)^2 + (c0 - c2^2/4)
            if (c1 == zero)
            {
                GetRootInfoBiquadratic(c0, c2, info);
                return;
            }

            // At this time, c0 != 0 and c1 != 0, which is a requirement for
            // the general solver that must use a root of a special cubic
            // polynomial.
            Rational const rat4 = 4, rat8 = 8, rat12 = 12, rat16 = 16;
            Rational const rat27 = 27, rat36 = 36;
            Rational c0sqr = c0 * c0, c1sqr = c1 * c1, c2sqr = c2 * c2;
            Rational delta = c1sqr * (-rat27 * c1sqr + rat4 * c2 *
                (rat36 * c0 - c2sqr)) + rat16 * c0 * (c2sqr * (c2sqr - rat8 * c0) +
                    rat16 * c0sqr);
            Rational a0 = rat12 * c0 + c2sqr;
            Rational a1 = rat4 * c0 - c2sqr;

            if (delta > zero)
            {
                if (c2 < zero && a1 < zero)
                {
                    // Four simple real roots.
                    info.push_back(1);
                    info.push_back(1);
                    info.push_back(1);
                    info.push_back(1);
                }
                else // c2 >= 0 or a1 >= 0
                {
                    // Two complex-conjugate pairs.
                }
            }
            else if (delta < zero)
            {
                // Two simple real roots, one complex-conjugate pair.
                info.push_back(1);
                info.push_back(1);
            }
            else  // delta = 0
            {
                if (a1 > zero || (c2 > zero && (a1 != zero || c1 != zero)))
                {
                    // One double real root, one complex-conjugate pair.
                    info.push_back(2);
                }
                else
                {
                    if (a0 != zero)
                    {
                        // One double real root, two simple real roots.
                        info.push_back(2);
                        info.push_back(1);
                        info.push_back(1);
                    }
                    else
                    {
                        // One triple real root, one simple real root.
                        info.push_back(3);
                        info.push_back(1);
                    }
                }
            }
        }

        template <typename Rational>
        static void GetRootInfoBiquadratic(Rational const& c0,
            Rational const& c2, std::vector<int>& info)
        {
            // Solve 0 = x^4 + c2*x^2 + c0 = (x^2 + c2/2)^2 + a1, where
            // a1 = c0 - c2^2/2.  We know that c0 != 0 at the time of the
            // function call, so x = 0 is not a root.  The condition c1 = 0
            // implies the quartic Delta = 256*c0*a1^2.

            Rational const zero = 0, rat2 = 2, rat256 = 256;
            Rational c2Half = c2 / rat2;
            Rational a1 = c0 - c2Half * c2Half;
            Rational delta = rat256 * c0 * a1 * a1;
            if (delta > zero)
            {
                if (c2 < zero)
                {
                    if (a1 < zero)
                    {
                        // Four simple roots.
                        info.push_back(1);
                        info.push_back(1);
                        info.push_back(1);
                        info.push_back(1);
                    }
                    else  // a1 > 0
                    {
                        // Two simple complex conjugate pairs.
                    }
                }
                else  // c2 >= 0
                {
                    // Two simple complex conjugate pairs.
                }
            }
            else if (delta < zero)
            {
                // Two simple real roots, one complex conjugate pair.
                info.push_back(1);
                info.push_back(1);
            }
            else  // delta = 0
            {
                if (c2 < zero)
                {
                    // Two double real roots.
                    info.push_back(2);
                    info.push_back(2);
                }
                else  // c2 > 0
                {
                    // Two double complex conjugate pairs.
                }
            }
        }

        // Support for the Find functions.
        static int FindRecursive(int degree, Real const* c, Real tmin, Real tmax,
            unsigned int maxIterations, Real* roots)
        {
            // The base of the recursion.
            Real const zero = (Real)0;
            Real root = zero;
            if (degree == 1)
            {
                int numRoots;
                if (c[1] != zero)
                {
                    root = -c[0] / c[1];
                    numRoots = 1;
                }
                else if (c[0] == zero)
                {
                    root = zero;
                    numRoots = 1;
                }
                else
                {
                    numRoots = 0;
                }

                if (numRoots > 0 && tmin <= root && root <= tmax)
                {
                    roots[0] = root;
                    return 1;
                }
                return 0;
            }

            // Find the roots of the derivative polynomial scaled by 1/degree.
            // The scaling avoids the factorial growth in the coefficients;
            // for example, without the scaling, the high-order term x^d
            // becomes (d!)*x through multiple differentiations.  With the
            // scaling we instead get x.  This leads to better numerical
            // behavior of the root finder.
            int derivDegree = degree - 1;
            std::vector<Real> derivCoeff(derivDegree + 1);
            std::vector<Real> derivRoots(derivDegree);
            for (int i = 0; i <= derivDegree; ++i)
            {
                derivCoeff[i] = c[i + 1] * (Real)(i + 1) / (Real)degree;
            }
            int numDerivRoots = FindRecursive(degree - 1, &derivCoeff[0], tmin, tmax,
                maxIterations, &derivRoots[0]);

            int numRoots = 0;
            if (numDerivRoots > 0)
            {
                // Find root on [tmin,derivRoots[0]].
                if (Find(degree, c, tmin, derivRoots[0], maxIterations, root))
                {
                    roots[numRoots++] = root;
                }

                // Find root on [derivRoots[i],derivRoots[i+1]].
                for (int i = 0; i <= numDerivRoots - 2; ++i)
                {
                    if (Find(degree, c, derivRoots[i], derivRoots[i + 1],
                        maxIterations, root))
                    {
                        roots[numRoots++] = root;
                    }
                }

                // Find root on [derivRoots[numDerivRoots-1],tmax].
                if (Find(degree, c, derivRoots[numDerivRoots - 1], tmax,
                    maxIterations, root))
                {
                    roots[numRoots++] = root;
                }
            }
            else
            {
                // The polynomial is monotone on [tmin,tmax], so has at most one root.
                if (Find(degree, c, tmin, tmax, maxIterations, root))
                {
                    roots[numRoots++] = root;
                }
            }
            return numRoots;
        }

        static Real Evaluate(int degree, Real const* c, Real t)
        {
            int i = degree;
            Real result = c[i];
            while (--i >= 0)
            {
                result = t * result + c[i];
            }
            return result;
        }
    };
}

// -----------------------------------------------------------------------
// Mathematics/Integration.h
//
// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.08.13

#pragma once

//#include <Mathematics/RootsPolynomial.h>
#include <array>
#include <functional>

namespace gte
{
    template <typename Real>
    class Integration
    {
    public:
        // A simple algorithm, but slow to converge as the number of samples
        // is increased.  The 'numSamples' needs to be two or larger.
        static Real TrapezoidRule(int numSamples, Real a, Real b,
            std::function<Real(Real)> const& integrand)
        {
            Real h = (b - a) / (Real)(numSamples - 1);
            Real result = (Real)0.5 * (integrand(a) + integrand(b));
            for (int i = 1; i <= numSamples - 2; ++i)
            {
                result += integrand(a + i * h);
            }
            result *= h;
            return result;
        }

        // The trapezoid rule is used to generate initial estimates, but then
        // Richardson extrapolation is used to improve the estimates.  This is
        // preferred over TrapezoidRule.  The 'order' must be positive.
        static Real Romberg(int order, Real a, Real b,
            std::function<Real(Real)> const& integrand)
        {
            Real const half = (Real)0.5;
            std::vector<std::array<Real, 2>> rom(order);
            Real h = b - a;
            rom[0][0] = half * h * (integrand(a) + integrand(b));
            for (int i0 = 2, p0 = 1; i0 <= order; ++i0, p0 *= 2, h *= half)
            {
                // Approximations via the trapezoid rule.
                Real sum = (Real)0;
                int i1;
                for (i1 = 1; i1 <= p0; ++i1)
                {
                    sum += integrand(a + h * (i1 - half));
                }

                // Richardson extrapolation.
                rom[0][1] = half * (rom[0][0] + h * sum);
                for (int i2 = 1, p2 = 4; i2 < i0; ++i2, p2 *= 4)
                {
                    rom[i2][1] = (p2 * rom[i2 - 1][1] - rom[i2 - 1][0]) / (p2 - 1);
                }

                for (i1 = 0; i1 < i0; ++i1)
                {
                    rom[i1][0] = rom[i1][1];
                }
            }

            Real result = rom[order - 1][0];
            return result;
        }

        // Gaussian quadrature estimates the integral of a function f(x)
        // defined on [-1,1] using
        //   integral_{-1}^{1} f(t) dt = sum_{i=0}^{n-1} c[i]*f(r[i])
        // where r[i] are the roots to the Legendre polynomial p(t) of degree
        // n and
        //   c[i] = integral_{-1}^{1} prod_{j=0,j!=i} (t-r[j]/(r[i]-r[j]) dt
        // To integrate over [a,b], a transformation to [-1,1] is applied
        // internally:  x - ((b-a)*t + (b+a))/2.  The Legendre polynomials are
        // generated by
        //   P[0](x) = 1, P[1](x) = x,
        //   P[k](x) = ((2*k-1)*x*P[k-1](x) - (k-1)*P[k-2](x))/k, k >= 2
        // Implementing the polynomial generation is simple, and computing the
        // roots requires a numerical method for finding polynomial roots.
        // The challenging task is to develop an efficient algorithm for
        // computing the coefficients c[i] for a specified degree.  The
        // 'degree' must be two or larger.

        static void ComputeQuadratureInfo(int degree, std::vector<Real>& roots,
            std::vector<Real>& coefficients)
        {
            Real const zero = (Real)0;
            Real const one = (Real)1;
            Real const half = (Real)0.5;

            std::vector<std::vector<Real>> poly(degree + 1);

            poly[0].resize(1);
            poly[0][0] = one;

            poly[1].resize(2);
            poly[1][0] = zero;
            poly[1][1] = one;

            for (int n = 2; n <= degree; ++n)
            {
                Real mult0 = (Real)(n - 1) / (Real)n;
                Real mult1 = (Real)(2 * n - 1) / (Real)n;

                poly[n].resize(n + 1);
                poly[n][0] = -mult0 * poly[n - 2][0];
                for (int i = 1; i <= n - 2; ++i)
                {
                    poly[n][i] = mult1 * poly[n - 1][i - 1] - mult0 * poly[n - 2][i];
                }
                poly[n][n - 1] = mult1 * poly[n - 1][n - 2];
                poly[n][n] = mult1 * poly[n - 1][n - 1];
            }

            roots.resize(degree);
            RootsPolynomial<Real>::Find(degree, &poly[degree][0], 2048, &roots[0]);

            coefficients.resize(roots.size());
            size_t n = roots.size() - 1;
            std::vector<Real> subroots(n);
            for (size_t i = 0; i < roots.size(); ++i)
            {
                Real denominator = (Real)1;
                for (size_t j = 0, k = 0; j < roots.size(); ++j)
                {
                    if (j != i)
                    {
                        subroots[k++] = roots[j];
                        denominator *= roots[i] - roots[j];
                    }
                }

                std::array<Real, 2> delta =
                {
                    -one - subroots.back(),
                    +one - subroots.back()
                };

                std::vector<std::array<Real, 2>> weights(n);
                weights[0][0] = half * delta[0] * delta[0];
                weights[0][1] = half * delta[1] * delta[1];
                for (size_t k = 1; k < n; ++k)
                {
                    Real dk = (Real)k;
                    Real mult = -dk / (dk + (Real)2);
                    weights[k][0] = mult * delta[0] * weights[k - 1][0];
                    weights[k][1] = mult * delta[1] * weights[k - 1][1];
                }

                struct Info
                {
                    int numBits;
                    std::array<Real, 2> product;
                };

                int numElements = (1 << static_cast<unsigned int>(n - 1));
                std::vector<Info> info(numElements);
                info[0].numBits = 0;
                info[0].product[0] = one;
                info[0].product[1] = one;
                for (int ipow = 1, r = 0; ipow < numElements; ipow <<= 1, ++r)
                {
                    info[ipow].numBits = 1;
                    info[ipow].product[0] = -one - subroots[r];
                    info[ipow].product[1] = +one - subroots[r];
                    for (int m = 1, j = ipow + 1; m < ipow; ++m, ++j)
                    {
                        info[j].numBits = info[m].numBits + 1;
                        info[j].product[0] =
                            info[ipow].product[0] * info[m].product[0];
                        info[j].product[1] =
                            info[ipow].product[1] * info[m].product[1];
                    }
                }

                std::vector<std::array<Real, 2>> sum(n);
                std::array<Real, 2> zero2 = { zero, zero };
                std::fill(sum.begin(), sum.end(), zero2);
                for (size_t k = 0; k < info.size(); ++k)
                {
                    sum[info[k].numBits][0] += info[k].product[0];
                    sum[info[k].numBits][1] += info[k].product[1];
                }

                std::array<Real, 2> total = zero2;
                for (size_t k = 0; k < n; ++k)
                {
                    total[0] += weights[n - 1 - k][0] * sum[k][0];
                    total[1] += weights[n - 1 - k][1] * sum[k][1];
                }

                coefficients[i] = (total[1] - total[0]) / denominator;
            }
        }

        static Real GaussianQuadrature(std::vector<Real> const& roots,
            std::vector<Real>const& coefficients, Real a, Real b,
            std::function<Real(Real)> const& integrand)
        {
            Real const half = (Real)0.5;
            Real radius = half * (b - a);
            Real center = half * (b + a);
            Real result = (Real)0;
            for (size_t i = 0; i < roots.size(); ++i)
            {
                result += coefficients[i] * integrand(radius * roots[i] + center);
            }
            result *= radius;
            return result;
        }
    };
}

// -----------------------------------------------------------------------
// Mathematics/RootsBiSection.h
//
// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.08.13

#pragma once

#include <functional>

// Compute a root of a function F(t) on an interval [t0, t1].  The caller
// specifies the maximum number of iterations, in case you want limited
// accuracy for the root.  However, the function is designed for native types
// (Real = float/double).  If you specify a sufficiently large number of
// iterations, the root finder bisects until either F(t) is identically zero
// [a condition dependent on how you structure F(t) for evaluation] or the
// midpoint (t0 + t1)/2 rounds numerically to tmin or tmax.  Of course, it
// is required that t0 < t1.  The return value of Find is:
//   0: F(t0)*F(t1) > 0, we cannot determine a root
//   1: F(t0) = 0 or F(t1) = 0
//   2..maxIterations:  the number of bisections plus one
//   maxIterations+1:  the loop executed without a break (no convergence)

namespace gte
{
    template <typename Real>
    class RootsBisection
    {
    public:
        // Use this function when F(t0) and F(t1) are not already known.
        static unsigned int Find(std::function<Real(Real)> const& F, Real t0,
            Real t1, unsigned int maxIterations, Real& root)
        {
            // Set 'root' initially to avoid "potentially uninitialized
            // variable" warnings by a compiler.
            root = t0;

            if (t0 < t1)
            {
                // Test the endpoints to see whether F(t) is zero.
                Real f0 = F(t0);
                if (f0 == (Real)0)
                {
                    root = t0;
                    return 1;
                }

                Real f1 = F(t1);
                if (f1 == (Real)0)
                {
                    root = t1;
                    return 1;
                }

                if (f0 * f1 > (Real)0)
                {
                    // It is not known whether the interval bounds a root.
                    return 0;
                }

                unsigned int i;
                for (i = 2; i <= maxIterations; ++i)
                {
                    root = (Real)0.5 * (t0 + t1);
                    if (root == t0 || root == t1)
                    {
                        // The numbers t0 and t1 are consecutive
                        // floating-point numbers.
                        break;
                    }

                    Real fm = F(root);
                    Real product = fm * f0;
                    if (product < (Real)0)
                    {
                        t1 = root;
                        f1 = fm;
                    }
                    else if (product > (Real)0)
                    {
                        t0 = root;
                        f0 = fm;
                    }
                    else
                    {
                        break;
                    }
                }
                return i;
            }
            else
            {
                // The interval endpoints are invalid.
                return 0;
            }
        }

        // If f0 = F(t0) and f1 = F(t1) are already known, pass them to the
        // bisector.  This is useful when |f0| or |f1| is infinite, and you
        // can pass sign(f0) or sign(f1) rather than then infinity because
        // the bisector cares only about the signs of f.
        static unsigned int Find(std::function<Real(Real)> const& F, Real t0,
            Real t1, Real f0, Real f1, unsigned int maxIterations, Real& root)
        {
            // Set 'root' initially to avoid "potentially uninitialized
            // variable" warnings by a compiler.
            root = t0;

            if (t0 < t1)
            {
                // Test the endpoints to see whether F(t) is zero.
                if (f0 == (Real)0)
                {
                    root = t0;
                    return 1;
                }

                if (f1 == (Real)0)
                {
                    root = t1;
                    return 1;
                }

                if (f0 * f1 > (Real)0)
                {
                    // It is not known whether the interval bounds a root.
                    return 0;
                }

                unsigned int i;
                root = t0;
                for (i = 2; i <= maxIterations; ++i)
                {
                    root = (Real)0.5 * (t0 + t1);
                    if (root == t0 || root == t1)
                    {
                        // The numbers t0 and t1 are consecutive
                        // floating-point numbers.
                        break;
                    }

                    Real fm = F(root);
                    Real product = fm * f0;
                    if (product < (Real)0)
                    {
                        t1 = root;
                        f1 = fm;
                    }
                    else if (product > (Real)0)
                    {
                        t0 = root;
                        f0 = fm;
                    }
                    else
                    {
                        break;
                    }
                }
                return i;
            }
            else
            {
                // The interval endpoints are invalid.
                return 0;
            }
        }
    };
}

// -----------------------------------------------------------------------
// Mathematics/Vector.h
//
// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.08.13

#pragma once

//#include <Mathematics/Math.h>
#include <algorithm>
#include <array>
#include <initializer_list>

namespace gte
{
    template <int N, typename Real>
    class Vector
    {
    public:
        // The tuple is uninitialized.
        Vector() = default;

        // The tuple is fully initialized by the inputs.
        Vector(std::array<Real, N> const& values)
            :
            mTuple(values)
        {
        }

        // At most N elements are copied from the initializer list, setting
        // any remaining elements to zero.  Create the zero vector using the
        // syntax
        //   Vector<N,Real> zero{(Real)0};
        // WARNING:  The C++ 11 specification states that
        //   Vector<N,Real> zero{};
        // will lead to a call of the default constructor, not the initializer
        // constructor!
        Vector(std::initializer_list<Real> values)
        {
            int const numValues = static_cast<int>(values.size());
            if (N == numValues)
            {
                std::copy(values.begin(), values.end(), mTuple.begin());
            }
            else if (N > numValues)
            {
                std::copy(values.begin(), values.end(), mTuple.begin());
                std::fill(mTuple.begin() + numValues, mTuple.end(), (Real)0);
            }
            else // N < numValues
            {
                std::copy(values.begin(), values.begin() + N, mTuple.begin());
            }
        }

        // For 0 <= d < N, element d is 1 and all others are 0.  If d is
        // invalid, the zero vector is created.  This is a convenience for
        // creating the standard Euclidean basis vectors; see also
        // MakeUnit(int) and Unit(int).
        Vector(int d)
        {
            MakeUnit(d);
        }

        // The copy constructor, destructor, and assignment operator are
        // generated by the compiler.

        // Member access.  The first operator[] returns a const reference
        // rather than a Real value.  This supports writing via standard file
        // operations that require a const pointer to data.
        inline int GetSize() const
        {
            return N;
        }

        inline Real const& operator[](int i) const
        {
            return mTuple[i];
        }

        inline Real& operator[](int i)
        {
            return mTuple[i];
        }

        // Comparisons for sorted containers and geometric ordering.
        inline bool operator==(Vector const& vec) const
        {
            return mTuple == vec.mTuple;
        }

        inline bool operator!=(Vector const& vec) const
        {
            return mTuple != vec.mTuple;
        }

        inline bool operator< (Vector const& vec) const
        {
            return mTuple < vec.mTuple;
        }

        inline bool operator<=(Vector const& vec) const
        {
            return mTuple <= vec.mTuple;
        }

        inline bool operator> (Vector const& vec) const
        {
            return mTuple > vec.mTuple;
        }

        inline bool operator>=(Vector const& vec) const
        {
            return mTuple >= vec.mTuple;
        }

        // Special vectors.

        // All components are 0.
        void MakeZero()
        {
            std::fill(mTuple.begin(), mTuple.end(), (Real)0);
        }

        // All components are 1.
        void MakeOnes()
        {
            std::fill(mTuple.begin(), mTuple.end(), (Real)1);
        }

        // Component d is 1, all others are zero.
        void MakeUnit(int d)
        {
            std::fill(mTuple.begin(), mTuple.end(), (Real)0);
            if (0 <= d && d < N)
            {
                mTuple[d] = (Real)1;
            }
        }

        static Vector Zero()
        {
            Vector<N, Real> v;
            v.MakeZero();
            return v;
        }

        static Vector Ones()
        {
            Vector<N, Real> v;
            v.MakeOnes();
            return v;
        }

        static Vector Unit(int d)
        {
            Vector<N, Real> v;
            v.MakeUnit(d);
            return v;
        }

    protected:
        // This data structure takes advantage of the built-in operator[],
        // range checking, and visualizers in MSVS.
        std::array<Real, N> mTuple;
    };

    // Unary operations.
    template <int N, typename Real>
    Vector<N, Real> operator+(Vector<N, Real> const& v)
    {
        return v;
    }

    template <int N, typename Real>
    Vector<N, Real> operator-(Vector<N, Real> const& v)
    {
        Vector<N, Real> result;
        for (int i = 0; i < N; ++i)
        {
            result[i] = -v[i];
        }
        return result;
    }

    // Linear-algebraic operations.
    template <int N, typename Real>
    Vector<N, Real> operator+(Vector<N, Real> const& v0, Vector<N, Real> const& v1)
    {
        Vector<N, Real> result = v0;
        return result += v1;
    }

    template <int N, typename Real>
    Vector<N, Real> operator-(Vector<N, Real> const& v0, Vector<N, Real> const& v1)
    {
        Vector<N, Real> result = v0;
        return result -= v1;
    }

    template <int N, typename Real>
    Vector<N, Real> operator*(Vector<N, Real> const& v, Real scalar)
    {
        Vector<N, Real> result = v;
        return result *= scalar;
    }

    template <int N, typename Real>
    Vector<N, Real> operator*(Real scalar, Vector<N, Real> const& v)
    {
        Vector<N, Real> result = v;
        return result *= scalar;
    }

    template <int N, typename Real>
    Vector<N, Real> operator/(Vector<N, Real> const& v, Real scalar)
    {
        Vector<N, Real> result = v;
        return result /= scalar;
    }

    template <int N, typename Real>
    Vector<N, Real>& operator+=(Vector<N, Real>& v0, Vector<N, Real> const& v1)
    {
        for (int i = 0; i < N; ++i)
        {
            v0[i] += v1[i];
        }
        return v0;
    }

    template <int N, typename Real>
    Vector<N, Real>& operator-=(Vector<N, Real>& v0, Vector<N, Real> const& v1)
    {
        for (int i = 0; i < N; ++i)
        {
            v0[i] -= v1[i];
        }
        return v0;
    }

    template <int N, typename Real>
    Vector<N, Real>& operator*=(Vector<N, Real>& v, Real scalar)
    {
        for (int i = 0; i < N; ++i)
        {
            v[i] *= scalar;
        }
        return v;
    }

    template <int N, typename Real>
    Vector<N, Real>& operator/=(Vector<N, Real>& v, Real scalar)
    {
        if (scalar != (Real)0)
        {
            Real invScalar = (Real)1 / scalar;
            for (int i = 0; i < N; ++i)
            {
                v[i] *= invScalar;
            }
        }
        else
        {
            for (int i = 0; i < N; ++i)
            {
                v[i] = (Real)0;
            }
        }
        return v;
    }

    // Componentwise algebraic operations.
    template <int N, typename Real>
    Vector<N, Real> operator*(Vector<N, Real> const& v0, Vector<N, Real> const& v1)
    {
        Vector<N, Real> result = v0;
        return result *= v1;
    }

    template <int N, typename Real>
    Vector<N, Real> operator/(Vector<N, Real> const& v0, Vector<N, Real> const& v1)
    {
        Vector<N, Real> result = v0;
        return result /= v1;
    }

    template <int N, typename Real>
    Vector<N, Real>& operator*=(Vector<N, Real>& v0, Vector<N, Real> const& v1)
    {
        for (int i = 0; i < N; ++i)
        {
            v0[i] *= v1[i];
        }
        return v0;
    }

    template <int N, typename Real>
    Vector<N, Real>& operator/=(Vector<N, Real>& v0, Vector<N, Real> const& v1)
    {
        for (int i = 0; i < N; ++i)
        {
            v0[i] /= v1[i];
        }
        return v0;
    }

    // Geometric operations.  The functions with 'robust' set to 'false' use
    // the standard algorithm for normalizing a vector by computing the length
    // as a square root of the squared length and dividing by it.  The results
    // can be infinite (or NaN) if the length is zero.  When 'robust' is set
    // to 'true', the algorithm is designed to avoid floating-point overflow
    // and sets the normalized vector to zero when the length is zero.
    template <int N, typename Real>
    Real Dot(Vector<N, Real> const& v0, Vector<N, Real> const& v1)
    {
        Real dot = v0[0] * v1[0];
        for (int i = 1; i < N; ++i)
        {
            dot += v0[i] * v1[i];
        }
        return dot;
    }

    template <int N, typename Real>
    Real Length(Vector<N, Real> const& v, bool robust = false)
    {
        if (robust)
        {
            Real maxAbsComp = std::fabs(v[0]);
            for (int i = 1; i < N; ++i)
            {
                Real absComp = std::fabs(v[i]);
                if (absComp > maxAbsComp)
                {
                    maxAbsComp = absComp;
                }
            }

            Real length;
            if (maxAbsComp > (Real)0)
            {
                Vector<N, Real> scaled = v / maxAbsComp;
                length = maxAbsComp * std::sqrt(Dot(scaled, scaled));
            }
            else
            {
                length = (Real)0;
            }
            return length;
        }
        else
        {
            return std::sqrt(Dot(v, v));
        }
    }

    template <int N, typename Real>
    Real Normalize(Vector<N, Real>& v, bool robust = false)
    {
        if (robust)
        {
            Real maxAbsComp = std::fabs(v[0]);
            for (int i = 1; i < N; ++i)
            {
                Real absComp = std::fabs(v[i]);
                if (absComp > maxAbsComp)
                {
                    maxAbsComp = absComp;
                }
            }

            Real length;
            if (maxAbsComp > (Real)0)
            {
                v /= maxAbsComp;
                length = std::sqrt(Dot(v, v));
                v /= length;
                length *= maxAbsComp;
            }
            else
            {
                length = (Real)0;
                for (int i = 0; i < N; ++i)
                {
                    v[i] = (Real)0;
                }
            }
            return length;
        }
        else
        {
            Real length = std::sqrt(Dot(v, v));
            if (length > (Real)0)
            {
                v /= length;
            }
            else
            {
                for (int i = 0; i < N; ++i)
                {
                    v[i] = (Real)0;
                }
            }
            return length;
        }
    }

    // Gram-Schmidt orthonormalization to generate orthonormal vectors from
    // the linearly independent inputs.  The function returns the smallest
    // length of the unnormalized vectors computed during the process.  If
    // this value is nearly zero, it is possible that the inputs are linearly
    // dependent (within numerical round-off errors).  On input,
    // 1 <= numElements <= N and v[0] through v[numElements-1] must be
    // initialized.  On output, the vectors v[0] through v[numElements-1]
    // form an orthonormal set.
    template <int N, typename Real>
    Real Orthonormalize(int numInputs, Vector<N, Real>* v, bool robust = false)
    {
        if (v && 1 <= numInputs && numInputs <= N)
        {
            Real minLength = Normalize(v[0], robust);
            for (int i = 1; i < numInputs; ++i)
            {
                for (int j = 0; j < i; ++j)
                {
                    Real dot = Dot(v[i], v[j]);
                    v[i] -= v[j] * dot;
                }
                Real length = Normalize(v[i], robust);
                if (length < minLength)
                {
                    minLength = length;
                }
            }
            return minLength;
        }

        return (Real)0;
    }

    // Construct a single vector orthogonal to the nonzero input vector.  If
    // the maximum absolute component occurs at index i, then the orthogonal
    // vector U has u[i] = v[i+1], u[i+1] = -v[i], and all other components
    // zero.  The index addition i+1 is computed modulo N.
    template <int N, typename Real>
    Vector<N, Real> GetOrthogonal(Vector<N, Real> const& v, bool unitLength)
    {
        Real cmax = std::fabs(v[0]);
        int imax = 0;
        for (int i = 1; i < N; ++i)
        {
            Real c = std::fabs(v[i]);
            if (c > cmax)
            {
                cmax = c;
                imax = i;
            }
        }

        Vector<N, Real> result;
        result.MakeZero();
        int inext = imax + 1;
        if (inext == N)
        {
            inext = 0;
        }
        result[imax] = v[inext];
        result[inext] = -v[imax];
        if (unitLength)
        {
            Real sqrDistance = result[imax] * result[imax] + result[inext] * result[inext];
            Real invLength = ((Real)1) / std::sqrt(sqrDistance);
            result[imax] *= invLength;
            result[inext] *= invLength;
        }
        return result;
    }

    // Compute the axis-aligned bounding box of the vectors.  The return value
    // is 'true' iff the inputs are valid, in which case vmin and vmax have
    // valid values.
    template <int N, typename Real>
    bool ComputeExtremes(int numVectors, Vector<N, Real> const* v,
        Vector<N, Real>& vmin, Vector<N, Real>& vmax)
    {
        if (v && numVectors > 0)
        {
            vmin = v[0];
            vmax = vmin;
            for (int j = 1; j < numVectors; ++j)
            {
                Vector<N, Real> const& vec = v[j];
                for (int i = 0; i < N; ++i)
                {
                    if (vec[i] < vmin[i])
                    {
                        vmin[i] = vec[i];
                    }
                    else if (vec[i] > vmax[i])
                    {
                        vmax[i] = vec[i];
                    }
                }
            }
            return true;
        }

        return false;
    }

    // Lift n-tuple v to homogeneous (n+1)-tuple (v,last).
    template <int N, typename Real>
    Vector<N + 1, Real> HLift(Vector<N, Real> const& v, Real last)
    {
        Vector<N + 1, Real> result;
        for (int i = 0; i < N; ++i)
        {
            result[i] = v[i];
        }
        result[N] = last;
        return result;
    }

    // Project homogeneous n-tuple v = (u,v[n-1]) to (n-1)-tuple u.
    template <int N, typename Real>
    Vector<N - 1, Real> HProject(Vector<N, Real> const& v)
    {
        static_assert(N >= 2, "Invalid dimension.");
        Vector<N - 1, Real> result;
        for (int i = 0; i < N - 1; ++i)
        {
            result[i] = v[i];
        }
        return result;
    }

    // Lift n-tuple v = (w0,w1) to (n+1)-tuple u = (w0,u[inject],w1).  By
    // inference, w0 is a (inject)-tuple [nonexistent when inject=0] and w1 is
    // a (n-inject)-tuple [nonexistent when inject=n].
    template <int N, typename Real>
    Vector<N + 1, Real> Lift(Vector<N, Real> const& v, int inject, Real value)
    {
        Vector<N + 1, Real> result;
        int i;
        for (i = 0; i < inject; ++i)
        {
            result[i] = v[i];
        }
        result[i] = value;
        int j = i;
        for (++j; i < N; ++i, ++j)
        {
            result[j] = v[i];
        }
        return result;
    }

    // Project n-tuple v = (w0,v[reject],w1) to (n-1)-tuple u = (w0,w1).  By
    // inference, w0 is a (reject)-tuple [nonexistent when reject=0] and w1 is
    // a (n-1-reject)-tuple [nonexistent when reject=n-1].
    template <int N, typename Real>
    Vector<N - 1, Real> Project(Vector<N, Real> const& v, int reject)
    {
        static_assert(N >= 2, "Invalid dimension.");
        Vector<N - 1, Real> result;
        for (int i = 0, j = 0; i < N - 1; ++i, ++j)
        {
            if (j == reject)
            {
                ++j;
            }
            result[i] = v[j];
        }
        return result;
    }
}

// -----------------------------------------------------------------------
// Mathematics/Vector2.h
//
// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.08.13

#pragma once

//#include <Mathematics/Vector.h>

namespace gte
{
    // Template alias for convenience.
    template <typename Real>
    using Vector2 = Vector<2, Real>;

    // Compute the perpendicular using the formal determinant,
    //   perp = det{{e0,e1},{x0,x1}} = (x1,-x0)
    // where e0 = (1,0), e1 = (0,1), and v = (x0,x1).
    template <typename Real>
    Vector2<Real> Perp(Vector2<Real> const& v)
    {
        return Vector2<Real>{ v[1], -v[0] };
    }

    // Compute the normalized perpendicular.
    template <typename Real>
    Vector2<Real> UnitPerp(Vector2<Real> const& v, bool robust = false)
    {
        Vector2<Real> unitPerp{ v[1], -v[0] };
        Normalize(unitPerp, robust);
        return unitPerp;
    }

    // Compute Dot((x0,x1),Perp(y0,y1)) = x0*y1 - x1*y0, where v0 = (x0,x1)
    // and v1 = (y0,y1).
    template <typename Real>
    Real DotPerp(Vector2<Real> const& v0, Vector2<Real> const& v1)
    {
        return Dot(v0, Perp(v1));
    }

    // Compute a right-handed orthonormal basis for the orthogonal complement
    // of the input vectors.  The function returns the smallest length of the
    // unnormalized vectors computed during the process.  If this value is
    // nearly zero, it is possible that the inputs are linearly dependent
    // (within numerical round-off errors).  On input, numInputs must be 1 and
    // v[0] must be initialized.  On output, the vectors v[0] and v[1] form an
    // orthonormal set.
    template <typename Real>
    Real ComputeOrthogonalComplement(int numInputs, Vector2<Real>* v, bool robust = false)
    {
        if (numInputs == 1)
        {
            v[1] = -Perp(v[0]);
            return Orthonormalize<2, Real>(2, v, robust);
        }

        return (Real)0;
    }

    // Compute the barycentric coordinates of the point P with respect to the
    // triangle <V0,V1,V2>, P = b0*V0 + b1*V1 + b2*V2, where b0 + b1 + b2 = 1.
    // The return value is 'true' iff {V0,V1,V2} is a linearly independent
    // set.  Numerically, this is measured by |det[V0 V1 V2]| <= epsilon.  The
    // values bary[] are valid only when the return value is 'true' but set to
    // zero when the return value is 'false'.
    template <typename Real>
    bool ComputeBarycentrics(Vector2<Real> const& p, Vector2<Real> const& v0,
        Vector2<Real> const& v1, Vector2<Real> const& v2, Real bary[3],
        Real epsilon = (Real)0)
    {
        // Compute the vectors relative to V2 of the triangle.
        Vector2<Real> diff[3] = { v0 - v2, v1 - v2, p - v2 };

        Real det = DotPerp(diff[0], diff[1]);
        if (det < -epsilon || det > epsilon)
        {
            Real invDet = (Real)1 / det;
            bary[0] = DotPerp(diff[2], diff[1]) * invDet;
            bary[1] = DotPerp(diff[0], diff[2]) * invDet;
            bary[2] = (Real)1 - bary[0] - bary[1];
            return true;
        }

        for (int i = 0; i < 3; ++i)
        {
            bary[i] = (Real)0;
        }
        return false;
    }

    // Get intrinsic information about the input array of vectors.  The return
    // value is 'true' iff the inputs are valid (numVectors > 0, v is not
    // null, and epsilon >= 0), in which case the class members are valid.
    template <typename Real>
    class IntrinsicsVector2
    {
    public:
        // The constructor sets the class members based on the input set.
        IntrinsicsVector2(int numVectors, Vector2<Real> const* v, Real inEpsilon)
            :
            epsilon(inEpsilon),
            dimension(0),
            maxRange((Real)0),
            origin({ (Real)0, (Real)0 }),
            extremeCCW(false)
        {
            min[0] = (Real)0;
            min[1] = (Real)0;
            direction[0] = { (Real)0, (Real)0 };
            direction[1] = { (Real)0, (Real)0 };
            extreme[0] = 0;
            extreme[1] = 0;
            extreme[2] = 0;

            if (numVectors > 0 && v && epsilon >= (Real)0)
            {
                // Compute the axis-aligned bounding box for the input
                // vectors.  Keep track of the indices into 'vectors' for the
                // current min and max.
                int j, indexMin[2], indexMax[2];
                for (j = 0; j < 2; ++j)
                {
                    min[j] = v[0][j];
                    max[j] = min[j];
                    indexMin[j] = 0;
                    indexMax[j] = 0;
                }

                int i;
                for (i = 1; i < numVectors; ++i)
                {
                    for (j = 0; j < 2; ++j)
                    {
                        if (v[i][j] < min[j])
                        {
                            min[j] = v[i][j];
                            indexMin[j] = i;
                        }
                        else if (v[i][j] > max[j])
                        {
                            max[j] = v[i][j];
                            indexMax[j] = i;
                        }
                    }
                }

                // Determine the maximum range for the bounding box.
                maxRange = max[0] - min[0];
                extreme[0] = indexMin[0];
                extreme[1] = indexMax[0];
                Real range = max[1] - min[1];
                if (range > maxRange)
                {
                    maxRange = range;
                    extreme[0] = indexMin[1];
                    extreme[1] = indexMax[1];
                }

                // The origin is either the vector of minimum x0-value or
                // vector of minimum x1-value.
                origin = v[extreme[0]];

                // Test whether the vector set is (nearly) a vector.
                if (maxRange <= epsilon)
                {
                    dimension = 0;
                    for (j = 0; j < 2; ++j)
                    {
                        extreme[j + 1] = extreme[0];
                    }
                    return;
                }

                // Test whether the vector set is (nearly) a line segment.  We
                // need direction[1] to span the orthogonal complement of
                // direction[0].
                direction[0] = v[extreme[1]] - origin;
                Normalize(direction[0], false);
                direction[1] = -Perp(direction[0]);

                // Compute the maximum distance of the points from the line
                // origin+t*direction[0].
                Real maxDistance = (Real)0;
                Real maxSign = (Real)0;
                extreme[2] = extreme[0];
                for (i = 0; i < numVectors; ++i)
                {
                    Vector2<Real> diff = v[i] - origin;
                    Real distance = Dot(direction[1], diff);
                    Real sign = (distance > (Real)0 ? (Real)1 :
                        (distance < (Real)0 ? (Real)-1 : (Real)0));
                    distance = std::fabs(distance);
                    if (distance > maxDistance)
                    {
                        maxDistance = distance;
                        maxSign = sign;
                        extreme[2] = i;
                    }
                }

                if (maxDistance <= epsilon * maxRange)
                {
                    // The points are (nearly) on the line
                    // origin + t * direction[0].
                    dimension = 1;
                    extreme[2] = extreme[1];
                    return;
                }

                dimension = 2;
                extremeCCW = (maxSign > (Real)0);
                return;
            }
        }

        // A nonnegative tolerance that is used to determine the intrinsic
        // dimension of the set.
        Real epsilon;

        // The intrinsic dimension of the input set, computed based on the
        // nonnegative tolerance mEpsilon.
        int dimension;

        // Axis-aligned bounding box of the input set.  The maximum range is
        // the larger of max[0]-min[0] and max[1]-min[1].
        Real min[2], max[2];
        Real maxRange;

        // Coordinate system.  The origin is valid for any dimension d.  The
        // unit-length direction vector is valid only for 0 <= i < d.  The
        // extreme index is relative to the array of input points, and is also
        // valid only for 0 <= i < d.  If d = 0, all points are effectively
        // the same, but the use of an epsilon may lead to an extreme index
        // that is not zero.  If d = 1, all points effectively lie on a line
        // segment.  If d = 2, the points are not collinear.
        Vector2<Real> origin;
        Vector2<Real> direction[2];

        // The indices that define the maximum dimensional extents.  The
        // values extreme[0] and extreme[1] are the indices for the points
        // that define the largest extent in one of the coordinate axis
        // directions.  If the dimension is 2, then extreme[2] is the index
        // for the point that generates the largest extent in the direction
        // perpendicular to the line through the points corresponding to
        // extreme[0] and extreme[1].  The triangle formed by the points
        // V[extreme[0]], V[extreme[1]], and V[extreme[2]] is clockwise or
        // counterclockwise, the condition stored in extremeCCW.
        int extreme[3];
        bool extremeCCW;
    };
}

// -----------------------------------------------------------------------
// Mathematics/Vector3.h
//
// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.08.13

#pragma once

//#include <Mathematics/Vector.h>

namespace gte
{
    // Template alias for convenience.
    template <typename Real>
    using Vector3 = Vector<3, Real>;

    // Cross, UnitCross, and DotCross have a template parameter N that should
    // be 3 or 4.  The latter case supports affine vectors in 4D (last
    // component w = 0) when you want to use 4-tuples and 4x4 matrices for
    // affine algebra.

    // Compute the cross product using the formal determinant:
    //   cross = det{{e0,e1,e2},{x0,x1,x2},{y0,y1,y2}}
    //         = (x1*y2-x2*y1, x2*y0-x0*y2, x0*y1-x1*y0)
    // where e0 = (1,0,0), e1 = (0,1,0), e2 = (0,0,1), v0 = (x0,x1,x2), and
    // v1 = (y0,y1,y2).
    template <int N, typename Real>
    Vector<N, Real> Cross(Vector<N, Real> const& v0, Vector<N, Real> const& v1)
    {
        static_assert(N == 3 || N == 4, "Dimension must be 3 or 4.");

        Vector<N, Real> result;
        result.MakeZero();
        result[0] = v0[1] * v1[2] - v0[2] * v1[1];
        result[1] = v0[2] * v1[0] - v0[0] * v1[2];
        result[2] = v0[0] * v1[1] - v0[1] * v1[0];
        return result;
    }

    // Compute the normalized cross product.
    template <int N, typename Real>
    Vector<N, Real> UnitCross(Vector<N, Real> const& v0, Vector<N, Real> const& v1, bool robust = false)
    {
        static_assert(N == 3 || N == 4, "Dimension must be 3 or 4.");
        Vector<N, Real> unitCross = Cross(v0, v1);
        Normalize(unitCross, robust);
        return unitCross;
    }

    // Compute Dot((x0,x1,x2),Cross((y0,y1,y2),(z0,z1,z2)), the triple scalar
    // product of three vectors, where v0 = (x0,x1,x2), v1 = (y0,y1,y2), and
    // v2 is (z0,z1,z2).
    template <int N, typename Real>
    Real DotCross(Vector<N, Real> const& v0, Vector<N, Real> const& v1,
        Vector<N, Real> const& v2)
    {
        static_assert(N == 3 || N == 4, "Dimension must be 3 or 4.");
        return Dot(v0, Cross(v1, v2));
    }

    // Compute a right-handed orthonormal basis for the orthogonal complement
    // of the input vectors.  The function returns the smallest length of the
    // unnormalized vectors computed during the process.  If this value is
    // nearly zero, it is possible that the inputs are linearly dependent
    // (within numerical round-off errors).  On input, numInputs must be 1 or
    // 2 and v[0] through v[numInputs-1] must be initialized.  On output, the
    // vectors v[0] through v[2] form an orthonormal set.
    template <typename Real>
    Real ComputeOrthogonalComplement(int numInputs, Vector3<Real>* v, bool robust = false)
    {
        if (numInputs == 1)
        {
            if (std::fabs(v[0][0]) > std::fabs(v[0][1]))
            {
                v[1] = { -v[0][2], (Real)0, +v[0][0] };
            }
            else
            {
                v[1] = { (Real)0, +v[0][2], -v[0][1] };
            }
            numInputs = 2;
        }

        if (numInputs == 2)
        {
            v[2] = Cross(v[0], v[1]);
            return Orthonormalize<3, Real>(3, v, robust);
        }

        return (Real)0;
    }

    // Compute the barycentric coordinates of the point P with respect to the
    // tetrahedron <V0,V1,V2,V3>, P = b0*V0 + b1*V1 + b2*V2 + b3*V3, where
    // b0 + b1 + b2 + b3 = 1.  The return value is 'true' iff {V0,V1,V2,V3} is
    // a linearly independent set.  Numerically, this is measured by
    // |det[V0 V1 V2 V3]| <= epsilon.  The values bary[] are valid only when
    // the return value is 'true' but set to zero when the return value is
    // 'false'.
    template <typename Real>
    bool ComputeBarycentrics(Vector3<Real> const& p, Vector3<Real> const& v0,
        Vector3<Real> const& v1, Vector3<Real> const& v2, Vector3<Real> const& v3,
        Real bary[4], Real epsilon = (Real)0)
    {
        // Compute the vectors relative to V3 of the tetrahedron.
        Vector3<Real> diff[4] = { v0 - v3, v1 - v3, v2 - v3, p - v3 };

        Real det = DotCross(diff[0], diff[1], diff[2]);
        if (det < -epsilon || det > epsilon)
        {
            Real invDet = ((Real)1) / det;
            bary[0] = DotCross(diff[3], diff[1], diff[2]) * invDet;
            bary[1] = DotCross(diff[3], diff[2], diff[0]) * invDet;
            bary[2] = DotCross(diff[3], diff[0], diff[1]) * invDet;
            bary[3] = (Real)1 - bary[0] - bary[1] - bary[2];
            return true;
        }

        for (int i = 0; i < 4; ++i)
        {
            bary[i] = (Real)0;
        }
        return false;
    }

    // Get intrinsic information about the input array of vectors.  The return
    // value is 'true' iff the inputs are valid (numVectors > 0, v is not
    // null, and epsilon >= 0), in which case the class members are valid.
    template <typename Real>
    class IntrinsicsVector3
    {
    public:
        // The constructor sets the class members based on the input set.
        IntrinsicsVector3(int numVectors, Vector3<Real> const* v, Real inEpsilon)
            :
            epsilon(inEpsilon),
            dimension(0),
            maxRange((Real)0),
            origin({ (Real)0, (Real)0, (Real)0 }),
            extremeCCW(false)
        {
            min[0] = (Real)0;
            min[1] = (Real)0;
            min[2] = (Real)0;
            direction[0] = { (Real)0, (Real)0, (Real)0 };
            direction[1] = { (Real)0, (Real)0, (Real)0 };
            direction[2] = { (Real)0, (Real)0, (Real)0 };
            extreme[0] = 0;
            extreme[1] = 0;
            extreme[2] = 0;
            extreme[3] = 0;

            if (numVectors > 0 && v && epsilon >= (Real)0)
            {
                // Compute the axis-aligned bounding box for the input vectors.
                // Keep track of the indices into 'vectors' for the current
                // min and max.
                int j, indexMin[3], indexMax[3];
                for (j = 0; j < 3; ++j)
                {
                    min[j] = v[0][j];
                    max[j] = min[j];
                    indexMin[j] = 0;
                    indexMax[j] = 0;
                }

                int i;
                for (i = 1; i < numVectors; ++i)
                {
                    for (j = 0; j < 3; ++j)
                    {
                        if (v[i][j] < min[j])
                        {
                            min[j] = v[i][j];
                            indexMin[j] = i;
                        }
                        else if (v[i][j] > max[j])
                        {
                            max[j] = v[i][j];
                            indexMax[j] = i;
                        }
                    }
                }

                // Determine the maximum range for the bounding box.
                maxRange = max[0] - min[0];
                extreme[0] = indexMin[0];
                extreme[1] = indexMax[0];
                Real range = max[1] - min[1];
                if (range > maxRange)
                {
                    maxRange = range;
                    extreme[0] = indexMin[1];
                    extreme[1] = indexMax[1];
                }
                range = max[2] - min[2];
                if (range > maxRange)
                {
                    maxRange = range;
                    extreme[0] = indexMin[2];
                    extreme[1] = indexMax[2];
                }

                // The origin is either the vector of minimum x0-value, vector
                // of minimum x1-value, or vector of minimum x2-value.
                origin = v[extreme[0]];

                // Test whether the vector set is (nearly) a vector.
                if (maxRange <= epsilon)
                {
                    dimension = 0;
                    for (j = 0; j < 3; ++j)
                    {
                        extreme[j + 1] = extreme[0];
                    }
                    return;
                }

                // Test whether the vector set is (nearly) a line segment.  We
                // need {direction[2],direction[3]} to span the orthogonal
                // complement of direction[0].
                direction[0] = v[extreme[1]] - origin;
                Normalize(direction[0], false);
                if (std::fabs(direction[0][0]) > std::fabs(direction[0][1]))
                {
                    direction[1][0] = -direction[0][2];
                    direction[1][1] = (Real)0;
                    direction[1][2] = +direction[0][0];
                }
                else
                {
                    direction[1][0] = (Real)0;
                    direction[1][1] = +direction[0][2];
                    direction[1][2] = -direction[0][1];
                }
                Normalize(direction[1], false);
                direction[2] = Cross(direction[0], direction[1]);

                // Compute the maximum distance of the points from the line
                // origin + t * direction[0].
                Real maxDistance = (Real)0;
                Real distance, dot;
                extreme[2] = extreme[0];
                for (i = 0; i < numVectors; ++i)
                {
                    Vector3<Real> diff = v[i] - origin;
                    dot = Dot(direction[0], diff);
                    Vector3<Real> proj = diff - dot * direction[0];
                    distance = Length(proj, false);
                    if (distance > maxDistance)
                    {
                        maxDistance = distance;
                        extreme[2] = i;
                    }
                }

                if (maxDistance <= epsilon * maxRange)
                {
                    // The points are (nearly) on the line
                    // origin + t * direction[0].
                    dimension = 1;
                    extreme[2] = extreme[1];
                    extreme[3] = extreme[1];
                    return;
                }

                // Test whether the vector set is (nearly) a planar polygon.
                // The point v[extreme[2]] is farthest from the line:
                // origin + t * direction[0].  The vector
                // v[extreme[2]] - origin is not necessarily perpendicular to
                // direction[0], so project out the direction[0] component so
                // that the result is perpendicular to direction[0].
                direction[1] = v[extreme[2]] - origin;
                dot = Dot(direction[0], direction[1]);
                direction[1] -= dot * direction[0];
                Normalize(direction[1], false);

                // We need direction[2] to span the orthogonal complement of
                // {direction[0],direction[1]}.
                direction[2] = Cross(direction[0], direction[1]);

                // Compute the maximum distance of the points from the plane
                // origin+t0 * direction[0] + t1 * direction[1].
                maxDistance = (Real)0;
                Real maxSign = (Real)0;
                extreme[3] = extreme[0];
                for (i = 0; i < numVectors; ++i)
                {
                    Vector3<Real> diff = v[i] - origin;
                    distance = Dot(direction[2], diff);
                    Real sign = (distance > (Real)0 ? (Real)1 :
                        (distance < (Real)0 ? (Real)-1 : (Real)0));
                    distance = std::fabs(distance);
                    if (distance > maxDistance)
                    {
                        maxDistance = distance;
                        maxSign = sign;
                        extreme[3] = i;
                    }
                }

                if (maxDistance <= epsilon * maxRange)
                {
                    // The points are (nearly) on the plane
                    // origin + t0 * direction[0] + t1 * direction[1].
                    dimension = 2;
                    extreme[3] = extreme[2];
                    return;
                }

                dimension = 3;
                extremeCCW = (maxSign > (Real)0);
                return;
            }
        }

        // A nonnegative tolerance that is used to determine the intrinsic
        // dimension of the set.
        Real epsilon;

        // The intrinsic dimension of the input set, computed based on the
        // nonnegative tolerance mEpsilon.
        int dimension;

        // Axis-aligned bounding box of the input set.  The maximum range is
        // the larger of max[0]-min[0], max[1]-min[1], and max[2]-min[2].
        Real min[3], max[3];
        Real maxRange;

        // Coordinate system.  The origin is valid for any dimension d.  The
        // unit-length direction vector is valid only for 0 <= i < d.  The
        // extreme index is relative to the array of input points, and is also
        // valid only for 0 <= i < d.  If d = 0, all points are effectively
        // the same, but the use of an epsilon may lead to an extreme index
        // that is not zero.  If d = 1, all points effectively lie on a line
        // segment.  If d = 2, all points effectively line on a plane.  If
        // d = 3, the points are not coplanar.
        Vector3<Real> origin;
        Vector3<Real> direction[3];

        // The indices that define the maximum dimensional extents.  The
        // values extreme[0] and extreme[1] are the indices for the points
        // that define the largest extent in one of the coordinate axis
        // directions.  If the dimension is 2, then extreme[2] is the index
        // for the point that generates the largest extent in the direction
        // perpendicular to the line through the points corresponding to
        // extreme[0] and extreme[1].  Furthermore, if the dimension is 3,
        // then extreme[3] is the index for the point that generates the
        // largest extent in the direction perpendicular to the triangle
        // defined by the other extreme points.  The tetrahedron formed by the
        // points V[extreme[0]], V[extreme[1]], V[extreme[2]], and
        // V[extreme[3]] is clockwise or counterclockwise, the condition
        // stored in extremeCCW.
        int extreme[4];
        bool extremeCCW;
    };
}

// -----------------------------------------------------------------------
// Mathematics/Vector4.h
//
// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.08.13

#pragma once

//#include <Mathematics/Vector3.h>

namespace gte
{
    // Template alias for convenience.
    template <typename Real>
    using Vector4 = Vector<4, Real>;

    // In Vector3.h, the Vector3 Cross, UnitCross, and DotCross have a
    // template parameter N that should be 3 or 4.  The latter case supports
    // affine vectors in 4D (last component w = 0) when you want to use
    // 4-tuples and 4x4 matrices for affine algebra.  Thus, you may use those
    // template functions for Vector4.

    // Compute the hypercross product using the formal determinant:
    //   hcross = det{{e0,e1,e2,e3},{x0,x1,x2,x3},{y0,y1,y2,y3},{z0,z1,z2,z3}}
    // where e0 = (1,0,0,0), e1 = (0,1,0,0), e2 = (0,0,1,0), e3 = (0,0,0,1),
    // v0 = (x0,x1,x2,x3), v1 = (y0,y1,y2,y3), and v2 = (z0,z1,z2,z3).
    template <typename Real>
    Vector4<Real> HyperCross(Vector4<Real> const& v0, Vector4<Real> const& v1, Vector4<Real> const& v2)
    {
        Real m01 = v0[0] * v1[1] - v0[1] * v1[0];  // x0*y1 - y0*x1
        Real m02 = v0[0] * v1[2] - v0[2] * v1[0];  // x0*z1 - z0*x1
        Real m03 = v0[0] * v1[3] - v0[3] * v1[0];  // x0*w1 - w0*x1
        Real m12 = v0[1] * v1[2] - v0[2] * v1[1];  // y0*z1 - z0*y1
        Real m13 = v0[1] * v1[3] - v0[3] * v1[1];  // y0*w1 - w0*y1
        Real m23 = v0[2] * v1[3] - v0[3] * v1[2];  // z0*w1 - w0*z1
        return Vector4<Real>
        {
            +m23 * v2[1] - m13 * v2[2] + m12 * v2[3],  // +m23*y2 - m13*z2 + m12*w2
                -m23 * v2[0] + m03 * v2[2] - m02 * v2[3],  // -m23*x2 + m03*z2 - m02*w2
                +m13 * v2[0] - m03 * v2[1] + m01 * v2[3],  // +m13*x2 - m03*y2 + m01*w2
                -m12 * v2[0] + m02 * v2[1] - m01 * v2[2]   // -m12*x2 + m02*y2 - m01*z2
        };
    }

    // Compute the normalized hypercross product.
    template <typename Real>
    Vector4<Real> UnitHyperCross(Vector4<Real> const& v0,
        Vector4<Real> const& v1, Vector4<Real> const& v2, bool robust = false)
    {
        Vector4<Real> unitHyperCross = HyperCross(v0, v1, v2);
        Normalize(unitHyperCross, robust);
        return unitHyperCross;
    }

    // Compute Dot(HyperCross((x0,x1,x2,x3),(y0,y1,y2,y3),(z0,z1,z2,z3)),
    // (w0,w1,w2,w3)), where v0 = (x0,x1,x2,x3), v1 = (y0,y1,y2,y3),
    // v2 = (z0,z1,z2,z3), and v3 = (w0,w1,w2,w3).
    template <typename Real>
    Real DotHyperCross(Vector4<Real> const& v0, Vector4<Real> const& v1,
        Vector4<Real> const& v2, Vector4<Real> const& v3)
    {
        return Dot(HyperCross(v0, v1, v2), v3);
    }

    // Compute a right-handed orthonormal basis for the orthogonal complement
    // of the input vectors.  The function returns the smallest length of the
    // unnormalized vectors computed during the process.  If this value is
    // nearly zero, it is possible that the inputs are linearly dependent
    // (within numerical round-off errors).  On input, numInputs must be 1, 2
    // or 3, and v[0] through v[numInputs-1] must be initialized.  On output,
    // the vectors v[0] through v[3] form an orthonormal set.
    template <typename Real>
    Real ComputeOrthogonalComplement(int numInputs, Vector4<Real>* v, bool robust = false)
    {
        if (numInputs == 1)
        {
            int maxIndex = 0;
            Real maxAbsValue = std::fabs(v[0][0]);
            for (int i = 1; i < 4; ++i)
            {
                Real absValue = std::fabs(v[0][i]);
                if (absValue > maxAbsValue)
                {
                    maxIndex = i;
                    maxAbsValue = absValue;
                }
            }

            if (maxIndex < 2)
            {
                v[1][0] = -v[0][1];
                v[1][1] = +v[0][0];
                v[1][2] = (Real)0;
                v[1][3] = (Real)0;
            }
            else if (maxIndex == 3)
            {
                // Generally, you can skip this clause and swap the last two
                // components.  However, by swapping 2 and 3 in this case, we
                // allow the function to work properly when the inputs are 3D
                // vectors represented as 4D affine vectors (w = 0).
                v[1][0] = (Real)0;
                v[1][1] = +v[0][2];
                v[1][2] = -v[0][1];
                v[1][3] = (Real)0;
            }
            else
            {
                v[1][0] = (Real)0;
                v[1][1] = (Real)0;
                v[1][2] = -v[0][3];
                v[1][3] = +v[0][2];
            }

            numInputs = 2;
        }

        if (numInputs == 2)
        {
            Real det[6] =
            {
                v[0][0] * v[1][1] - v[1][0] * v[0][1],
                v[0][0] * v[1][2] - v[1][0] * v[0][2],
                v[0][0] * v[1][3] - v[1][0] * v[0][3],
                v[0][1] * v[1][2] - v[1][1] * v[0][2],
                v[0][1] * v[1][3] - v[1][1] * v[0][3],
                v[0][2] * v[1][3] - v[1][2] * v[0][3]
            };

            int maxIndex = 0;
            Real maxAbsValue = std::fabs(det[0]);
            for (int i = 1; i < 6; ++i)
            {
                Real absValue = std::fabs(det[i]);
                if (absValue > maxAbsValue)
                {
                    maxIndex = i;
                    maxAbsValue = absValue;
                }
            }

            if (maxIndex == 0)
            {
                v[2] = { -det[4], +det[2], (Real)0, -det[0] };
            }
            else if (maxIndex <= 2)
            {
                v[2] = { +det[5], (Real)0, -det[2], +det[1] };
            }
            else
            {
                v[2] = { (Real)0, -det[5], +det[4], -det[3] };
            }

            numInputs = 3;
        }

        if (numInputs == 3)
        {
            v[3] = HyperCross(v[0], v[1], v[2]);
            return Orthonormalize<4, Real>(4, v, robust);
        }

        return (Real)0;
    }
}


// -----------------------------------------------------------------------
// Mathematics/ParemetricCurve.h
//
// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.08.13

#pragma once

//#include <Mathematics/Integration.h>
//#include <Mathematics/RootsBisection.h>
//#include <Mathematics/Vector.h>

namespace gte
{
    template <int N, typename Real>
    class ParametricCurve
    {
    protected:
        // Abstract base class for a parameterized curve X(t), where t is the
        // parameter in [tmin,tmax] and X is an N-tuple position.  The first
        // constructor is for single-segment curves. The second constructor is
        // for multiple-segment curves. The times must be strictly increasing.
        ParametricCurve(Real tmin, Real tmax)
            :
            mTime(2),
            mSegmentLength(1),
            mAccumulatedLength(1),
            mRombergOrder(DEFAULT_ROMBERG_ORDER),
            mMaxBisections(DEFAULT_MAX_BISECTIONS),
            mConstructed(false)
        {
            mTime[0] = tmin;
            mTime[1] = tmax;
            mSegmentLength[0] = (Real)0;
            mAccumulatedLength[0] = (Real)0;
        }

        ParametricCurve(int numSegments, Real const* times)
            :
            mTime(numSegments + 1),
            mSegmentLength(numSegments),
            mAccumulatedLength(numSegments),
            mRombergOrder(DEFAULT_ROMBERG_ORDER),
            mMaxBisections(DEFAULT_MAX_BISECTIONS),
            mConstructed(false)
        {
            std::copy(times, times + numSegments + 1, mTime.begin());
            mSegmentLength[0] = (Real)0;
            mAccumulatedLength[0] = (Real)0;
        }

    public:
        virtual ~ParametricCurve()
        {
        }

        // To validate construction, create an object as shown:
        //     DerivedClassCurve<N, Real> curve(parameters);
        //     if (!curve) { <constructor failed, handle accordingly>; }
        inline operator bool() const
        {
            return mConstructed;
        }

        // Member access.
        inline Real GetTMin() const
        {
            return mTime.front();
        }

        inline Real GetTMax() const
        {
            return mTime.back();
        }

        inline int GetNumSegments() const
        {
            return static_cast<int>(mSegmentLength.size());
        }

        inline Real const* GetTimes() const
        {
            return &mTime[0];
        }

        // This function applies only when the first constructor is used (two
        // times rather than a sequence of three or more times).
        void SetTimeInterval(Real tmin, Real tmax)
        {
            if (mTime.size() == 2)
            {
                mTime[0] = tmin;
                mTime[1] = tmax;
            }
        }

        // Parameters used in GetLength(...), GetTotalLength() and
        // GetTime(...).

        // The default value is 8.
        inline void SetRombergOrder(int order)
        {
            mRombergOrder = std::max(order, 1);
        }

        // The default value is 1024.
        inline void SetMaxBisections(unsigned int maxBisections)
        {
            mMaxBisections = std::max(maxBisections, 1u);
        }

        // Evaluation of the curve.  The function supports derivative
        // calculation through order 3; that is, order <= 3 is required.  If
        // you want/ only the position, pass in order of 0.  If you want the
        // position and first derivative, pass in order of 1, and so on.  The
        // output array 'jet' must have enough storage to support the maximum
        // order.  The values are ordered as: position, first derivative,
        // second derivative, third derivative.
        enum { SUP_ORDER = 4 };
        virtual void Evaluate(Real t, unsigned int order, Vector<N, Real>* jet) const = 0;

        void Evaluate(Real t, unsigned int order, Real* values) const
        {
            Evaluate(t, order, reinterpret_cast<Vector<N, Real>*>(values));
        }

        // Differential geometric quantities.
        Vector<N, Real> GetPosition(Real t) const
        {
            Vector<N, Real> position;
            Evaluate(t, 0, &position);
            return position;
        }

        Vector<N, Real> GetTangent(Real t) const
        {
            std::array<Vector<N, Real>, 2> jet;  // (position, tangent)
            Evaluate(t, 1, jet.data());
            Normalize(jet[1]);
            return jet[1];
        }

        Real GetSpeed(Real t) const
        {
            std::array<Vector<N, Real>, 2> jet;  // (position, tangent)
            Evaluate(t, 1, jet.data());
            return Length(jet[1]);
        }

        Real GetLength(Real t0, Real t1) const
        {
            std::function<Real(Real)> speed = [this](Real t)
            {
                return GetSpeed(t);
            };

            if (mSegmentLength[0] == (Real)0)
            {
                // Lazy initialization of lengths of segments.
                int const numSegments = static_cast<int>(mSegmentLength.size());
                Real accumulated = (Real)0;
                for (int i = 0; i < numSegments; ++i)
                {
                    mSegmentLength[i] = Integration<Real>::Romberg(mRombergOrder,
                        mTime[i], mTime[i + 1], speed);
                    accumulated += mSegmentLength[i];
                    mAccumulatedLength[i] = accumulated;
                }
            }

            t0 = std::max(t0, GetTMin());
            t1 = std::min(t1, GetTMax());
            auto iter0 = std::lower_bound(mTime.begin(), mTime.end(), t0);
            int index0 = static_cast<int>(iter0 - mTime.begin());
            auto iter1 = std::lower_bound(mTime.begin(), mTime.end(), t1);
            int index1 = static_cast<int>(iter1 - mTime.begin());

            Real length;
            if (index0 < index1)
            {
                length = (Real)0;
                if (t0 < *iter0)
                {
                    length += Integration<Real>::Romberg(mRombergOrder, t0,
                        mTime[index0], speed);
                }

                int isup;
                if (t1 < *iter1)
                {
                    length += Integration<Real>::Romberg(mRombergOrder,
                        mTime[index1 - 1], t1, speed);
                    isup = index1 - 1;
                }
                else
                {
                    isup = index1;
                }
                for (int i = index0; i < isup; ++i)
                {
                    length += mSegmentLength[i];
                }
            }
            else
            {
                length = Integration<Real>::Romberg(mRombergOrder, t0, t1, speed);
            }
            return length;
        }

        Real GetTotalLength() const
        {
            if (mAccumulatedLength.back() == (Real)0)
            {
                // Lazy evaluation of the accumulated length array.
                return GetLength(mTime.front(), mTime.back());
            }

            return mAccumulatedLength.back();
        }

        // Inverse mapping of s = Length(t) given by t = Length^{-1}(s).  The
        // inverse length function generally cannot be written in closed form,
        // in which case it is not directly computable.  Instead, we can
        // specify s and estimate the root t for F(t) = Length(t) - s.  The
        // derivative is F'(t) = Speed(t) >= 0, so F(t) is nondecreasing.  To
        // be robust, we use bisection to locate the root, although it is
        // possible to use a hybrid of Newton's method and bisection.  For
        // details, see the document
        // https://www.geometrictools.com/Documentation/MovingAlongCurveSpecifiedSpeed.pdf
        Real GetTime(Real length) const
        {
            if (length > (Real)0)
            {
                if (length < GetTotalLength())
                {
                    std::function<Real(Real)> F = [this, &length](Real t)
                    {
                        return Integration<Real>::Romberg(mRombergOrder,
                            mTime.front(), t, [this](Real z) { return GetSpeed(z); })
                            - length;
                    };

                    // We know that F(tmin) < 0 and F(tmax) > 0, which allows us to
                    // use bisection.  Rather than bisect the entire interval, let's
                    // narrow it down with a reasonable initial guess.
                    Real ratio = length / GetTotalLength();
                    Real omratio = (Real)1 - ratio;
                    Real tmid = omratio * mTime.front() + ratio * mTime.back();
                    Real fmid = F(tmid);
                    if (fmid > (Real)0)
                    {
                        RootsBisection<Real>::Find(F, mTime.front(), tmid, (Real)-1,
                            (Real)1, mMaxBisections, tmid);
                    }
                    else if (fmid < (Real)0)
                    {
                        RootsBisection<Real>::Find(F, tmid, mTime.back(), (Real)-1,
                            (Real)1, mMaxBisections, tmid);
                    }
                    return tmid;
                }
                else
                {
                    return mTime.back();
                }
            }
            else
            {
                return mTime.front();
            }
        }

        // Compute a subset of curve points according to the specified attribute.
        // The input 'numPoints' must be two or larger.
        void SubdivideByTime(int numPoints, Vector<N, Real>* points) const
        {
            Real delta = (mTime.back() - mTime.front()) / (Real)(numPoints - 1);
            for (int i = 0; i < numPoints; ++i)
            {
                Real t = mTime.front() + delta * i;
                points[i] = GetPosition(t);
            }
        }

        void SubdivideByLength(int numPoints, Vector<N, Real>* points) const
        {
            Real delta = GetTotalLength() / (Real)(numPoints - 1);
            for (int i = 0; i < numPoints; ++i)
            {
                Real length = delta * i;
                Real t = GetTime(length);
                points[i] = GetPosition(t);
            }
        }

    protected:
        enum
        {
            DEFAULT_ROMBERG_ORDER = 8,
            DEFAULT_MAX_BISECTIONS = 1024
        };

        std::vector<Real> mTime;
        mutable std::vector<Real> mSegmentLength;
        mutable std::vector<Real> mAccumulatedLength;
        int mRombergOrder;
        unsigned int mMaxBisections;
        bool mConstructed;
    };
}

// -----------------------------------------------------------------------
// Mathematics/BSplineCurve.h
//
// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.08.13

#pragma once

//#include <Mathematics/BasisFunction.h>
//#include <Mathematics/ParametricCurve.h>

namespace gte
{
    template <int N, typename Real>
    class BSplineCurve : public ParametricCurve<N, Real>
    {
    public:
        // Construction.  If the input controls is non-null, a copy is made of
        // the controls.  To defer setting the control points, pass a null
        // pointer and later access the control points via GetControls() or
        // SetControl() member functions.  The domain is t in [t[d],t[n]],
        // where t[d] and t[n] are knots with d the degree and n the number of
        // control points.
        BSplineCurve(BasisFunctionInput<Real> const& input, Vector<N, Real> const* controls)
            :
            ParametricCurve<N, Real>((Real)0, (Real)1),
            mBasisFunction(input)
        {
            // The mBasisFunction stores the domain but so does
            // ParametricCurve.
            this->mTime.front() = mBasisFunction.GetMinDomain();
            this->mTime.back() = mBasisFunction.GetMaxDomain();

            // The replication of control points for periodic splines is
            // avoided by wrapping the i-loop index in Evaluate.
            mControls.resize(input.numControls);
            if (controls)
            {
                std::copy(controls, controls + input.numControls, mControls.begin());
            }
            else
            {
                Vector<N, Real> zero{ (Real)0 };
                std::fill(mControls.begin(), mControls.end(), zero);
            }
            this->mConstructed = true;
        }

        // Member access.
        inline BasisFunction<Real> const& GetBasisFunction() const
        {
            return mBasisFunction;
        }

        inline int GetNumControls() const
        {
            return static_cast<int>(mControls.size());
        }

        inline Vector<N, Real> const* GetControls() const
        {
            return mControls.data();
        }

        inline Vector<N, Real>* GetControls()
        {
            return mControls.data();
        }

        void SetControl(int i, Vector<N, Real> const& control)
        {
            if (0 <= i && i < GetNumControls())
            {
                mControls[i] = control;
            }
        }

        Vector<N, Real> const& GetControl(int i) const
        {
            if (0 <= i && i < GetNumControls())
            {
                return mControls[i];
            }
            else
            {
                return mControls[0];
            }
        }

        // Evaluation of the curve.  The function supports derivative
        // calculation through order 3; that is, order <= 3 is required.  If
        // you want/ only the position, pass in order of 0.  If you want the
        // position and first derivative, pass in order of 1, and so on.  The
        // output array 'jet' must have enough storage to support the maximum
        // order.  The values are ordered as: position, first derivative,
        // second derivative, third derivative.
        virtual void Evaluate(Real t, unsigned int order, Vector<N, Real>* jet) const override
        {
            unsigned int const supOrder = ParametricCurve<N, Real>::SUP_ORDER;
            if (!this->mConstructed || order >= supOrder)
            {
                // Return a zero-valued jet for invalid state.
                for (unsigned int i = 0; i < supOrder; ++i)
                {
                    jet[i].MakeZero();
                }
                return;
            }

            int imin, imax;
            mBasisFunction.Evaluate(t, order, imin, imax);

            // Compute position.
            jet[0] = Compute(0, imin, imax);
            if (order >= 1)
            {
                // Compute first derivative.
                jet[1] = Compute(1, imin, imax);
                if (order >= 2)
                {
                    // Compute second derivative.
                    jet[2] = Compute(2, imin, imax);
                    if (order == 3)
                    {
                        jet[3] = Compute(3, imin, imax);
                    }
                }
            }
        }

    private:
        // Support for Evaluate(...).
        Vector<N, Real> Compute(unsigned int order, int imin, int imax) const
        {
            // The j-index introduces a tiny amount of overhead in order to handle
            // both aperiodic and periodic splines.  For aperiodic splines, j = i
            // always.

            int numControls = GetNumControls();
            Vector<N, Real> result;
            result.MakeZero();
            for (int i = imin; i <= imax; ++i)
            {
                Real tmp = mBasisFunction.GetValue(order, i);
                int j = (i >= numControls ? i - numControls : i);
                result += tmp * mControls[j];
            }
            return result;
        }

        BasisFunction<Real> mBasisFunction;
        std::vector<Vector<N, Real>> mControls;
    };
}
