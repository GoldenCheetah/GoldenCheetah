/*
 * Copyright (c) 2020 Eric Christoffersen (impolexg@outlook.com)
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

#include <array>
#include <iostream>
#include <string>
#include <sstream>

#include <QtDebug>

#include "PolynomialRegression.h"
#include "MultiRegressionizer.h"
#include "Integrator.h"

// In the future all toolchains will support constexpr and we can turn this on...
// Benefit is the entire init of maker method array is constructed at compile time
// and can be constructed into the image as readonly.
#if defined(USING_CXX_17_OR_HIGHER)
#define CONSTEXPR constexpr
#define CONSTEXPR_VAR constexpr
#else
#define CONSTEXPR
#define CONSTEXPR_VAR static const
#endif

// Utility to emit polynomial as string. This layout is tied to the parsing in
// realtimecontroller.cpp:void VirtualPowerTrainerManager::GetVirtualPowerTrainerAsString(int idx, QString& s)

template <typename T_String, typename T_fptype>
void T_AppendVirtualPowerDescriptionString(T_String &s, const char* tla, size_t arrSize, size_t numSize, const T_fptype *arr, T_fptype scale) {

    std::ostringstream accum;
    accum.precision(12);

    // tla,arrSize,numSize|coefs,...|scale

    accum << tla << ", " << arrSize << "," << numSize << "|";
    for (int i = 0; i <= (int)(arrSize - 1); i++) {
        accum << arr[i];
        if (i != (int)(arrSize - 1)) accum << ",";
    }
    accum << "|" << scale;

    s.append(accum.str());
}

// Structure to serve symplectic integrator across polyfits.
// Polyfit is 2d map so doesn't use v.
template <typename T_poly> class PolyPrivateMotionStatePair
{
    typedef typename T_poly::value_type T_fptype;

    const T_poly* m_fit;

    T_fptype      m_t0;
    T_fptype      m_t1;

public:

    PolyPrivateMotionStatePair(const T_poly* pfit, T_fptype t0, T_fptype t1) :
        m_fit(pfit), m_t0(t0), m_t1(t1)
    {}

    T_fptype T0() const { return m_t0; }
    T_fptype T1() const { return m_t1; }
    T_fptype DT() const { return T1() - T0(); }

    T_fptype CalcV(T_fptype v, T_fptype t) const {
        Q_UNUSED(v);
        return m_fit->Fit(t);
    }

    T_fptype dVdT(T_fptype v, T_fptype t) const {
        Q_UNUSED(v);
        return m_fit->Slope(t);
    }
};

template <typename T, typename T_inittype>
struct FractionalPolynomialFitter : virtual public T {
    typedef typename T::value_type T_fptype;

    std::array<T_fptype, 3> arr;
    T_fptype scale;

    static T* Make(const T_inittype& v, const typename T_inittype::value_type& scale) {
        return new FractionalPolynomialFitter(v, scale);
    }

    FractionalPolynomialFitter(const T_inittype& n, const typename T_inittype::value_type& s) : arr(), scale(s) {
        for (size_t i = 0; i < n.size(); i++) arr[i] = n[i];
    }

    ~FractionalPolynomialFitter() {}

    T_fptype Fit(T_fptype v) const {
        // Scale v, for example mph -> kph
        v = v * scale;

        // v^X * Y + Z
        return (pow(v, arr[0]) * arr[1]) + arr[2];
    }

    T_fptype Slope(T_fptype v) const {
        // Scale v, for example mph -> kph
        v = v * scale;

        return (v * pow(v, arr[0] - 1));
    }

    T_fptype Integrate(T_fptype from, T_fptype to, T_fptype &endPoint) const {
        typedef PolyPrivateMotionStatePair<T> MotionStatePair;
        typedef Integrator<MotionStatePair> Integrator;

        IntegrateResult r = Integrator::I(MotionStatePair(this, from, to), this->Fit(from));

        endPoint = r.endPoint();
        return r.sum();
    }

    T_fptype Integrate(T_fptype from, T_fptype to) const {
        T_fptype unused;
        return Integrate(from, to, unused);
    }

    void append(std::string& s) const {
        T_AppendVirtualPowerDescriptionString(s, "FPR", 3, 0, &(arr[0]), scale);
    }
};

template <size_t T_size, size_t T_num, typename T, typename T_inittype>
struct RationalFitter : virtual public T {
    typedef typename T::value_type T_fptype;

    std::array<T_fptype, T_size> arr;
    T_fptype scale;

    static T* Make(const T_inittype& v, const T_inittype& v2, const typename T_inittype::value_type& scale) {
        return new RationalFitter(v, v2, scale);
    }

    RationalFitter(const T_inittype& n, const T_inittype& d, const typename T_inittype::value_type& s) : arr(), scale(s) {
        // Populate numerator coefficients.
        for (size_t i = 0; i < T_num; i++) arr[i] = n[i];

        // Denominator may have no coefficients (implicit 1.)
        size_t i = T_num;
        while (i < T_size) {
            arr[i] = d[i - T_num];
            i++;
        }
    }

    ~RationalFitter() {}

    // Compute Fit.
    // Because fitter has templated size the compiler is able to fully unroll the fit
    // calculation. This function also serves all polynomial evaluation since denominator
    // has implicit 1 which allows zero sized denominator evaluation to be optimized away.
    T_fptype Fit(T_fptype v) const {

        // Scale v, for example mph -> kph
        v = v * scale;

        // Determine numerator value.
        T_fptype n = 0.;
        T_fptype p = 1.; // power starts with x^0 (== 1)
        for (size_t i = 0; i < T_num; i++) {
            n += (arr[i] * p);
            p *= v;
        }

        // If no denominator coeficients then implicit divide 1. Done-ski.
        if (T_size == T_num) return n;

        // Determine denominator value.

        // This implementation of rational polynomials has an implicit 1 for denominator
        // coefficient. It is impossible to not have a 1 for first coeficient. The first
        // coefficient for denominator of rational polynomial is for x^1.
        T_fptype d = 1.; // d starts at 1

        // This compile time check avoids compiler warning about unused value.
        if (T_num < T_size) {
            p = v;           // loop starts with p = x^1
            size_t i = T_num;
            while (i < T_size) {
                d += (arr[i] * p);
                p *= v;
                i++;
            }
        }

        T_fptype r = n / d;

        return r;
    }

    // Slope of rational polynomial using quotiant rule
    T_fptype Slope(T_fptype v) const {
        // Scale v, for example mph -> kph
        v = v * scale;

        // Compute Numerator Slope
        static const T_fptype s_zero = 0.;
        static const T_fptype s_one = 1.;
        T_fptype numerSlope = s_zero;
        T_fptype p = s_one;
        T_fptype count = s_one;
        if (T_num > 1)
            for (size_t i = 1; i < T_num; i++, count = count + 1, p = p * v)
                if (!IsZero2(arr[i], s_one))
                    numerSlope = numerSlope + arr[i] * p * count;

        // If no denominator coeficients then implicit divide 1. Done-ski.
        if (T_size == T_num) return numerSlope;

        // Compute Denominator Slope
        T_fptype denomSlope = s_zero; // d starts at 0
        p = s_one;
        count = s_one;
        for (size_t i = T_num; i < T_size; i++, count = count + 1, p = p * v)
            denomSlope = denomSlope + (arr[i] * p * count);

        // Determine numerator fit value.
        T_fptype numerFit = s_zero;
        p = s_one; // power starts with x^0 (== 1)
        for (size_t i = 0; i < T_num; i++, p = p * v)
            numerFit = numerFit + (arr[i] * p);

        // Determine denominator fit value
        T_fptype denomFit = s_one; // d starts at 1
        p = v;                     // loop starts with p = x^1
        for (size_t i = T_num; i < T_size; i++, p = p * v)
            denomFit = denomFit + (arr[i] * p);

        // Quotiant Rule:
        T_fptype r = ((numerSlope * denomFit) - (numerFit * denomSlope)) / (denomFit * denomFit);

        return r;
    }

    T_fptype Integrate(T_fptype from, T_fptype to, T_fptype &endPoint) const {
        typedef PolyPrivateMotionStatePair<T> MotionStatePair;
        typedef Integrator<MotionStatePair> Integrator;

        IntegrateResult r = Integrator::I(MotionStatePair(this, from, to), this->Fit(from));

        endPoint = r.endPoint();
        return r.sum();
    }

    T_fptype Integrate(T_fptype from, T_fptype to) const {
        T_fptype unused;
        return Integrate(from, to, unused);
    }

    void append(std::string& s) const {
        T_AppendVirtualPowerDescriptionString(s, "RPR", T_size, T_num, &(arr[0]), scale);
    }
};

template <size_t T_maxSize, size_t T_maxDen, typename T, typename T_inittype, size_t T_valueNumber>
struct RationalFitterGenerator {
    CONSTEXPR static void setMaker(std::array<T * (*)(const T_inittype&, const T_inittype&, const typename T_inittype::value_type&), T_maxSize>& p) {
        CONSTEXPR_VAR size_t s_num = (T_valueNumber % T_maxDen) + 1;
        CONSTEXPR_VAR size_t s_den = T_valueNumber / T_maxDen;
        p[T_valueNumber] = RationalFitter<s_num + s_den, s_num, T, T_inittype>::Make;
        RationalFitterGenerator<T_maxSize, T_maxDen, T, T_inittype, T_valueNumber + 1>::setMaker(p);
    }
};

template <size_t T_maxSize, size_t T_maxDen, typename T, typename T_inittype>
struct RationalFitterGenerator<T_maxSize, T_maxDen, T, T_inittype, T_maxSize> {
    CONSTEXPR static void setMaker(std::array<T * (*)(const T_inittype&, const T_inittype&, const typename T_inittype::value_type&), T_maxSize>& p) { (void)(p); }
};

template <size_t T_maxNum, size_t T_maxDen, typename T, typename T_inittype>
struct T_PolyFitGenerator {

    // Value number from <numeratorCount, denominatorCount> to a unique combination of the two,
    // for example:
    //
    // if num,den max both 6...
    //
    // 1,0 = 0
    // 2,0 = 1
    // 6,0 = 5
    // 1,1 = 6*1 + 0 == 6
    // 1,2 = 6*2 + 1 == 13
    // 6,5 = 6*5 + 5 == 35
    // 6,6 = 6*6 + 5 == 41
    //
    // The implicit 1 is there so the linear system to solve for rational polynomial least squares doesnt need to worry about divide by zero.

    std::array<T * (*)(const T_inittype&, const T_inittype&, const typename T_inittype::value_type&), T_maxNum * (T_maxDen + 1)> arr;

    CONSTEXPR T_PolyFitGenerator() : arr() {
        RationalFitterGenerator<T_maxNum * (T_maxDen + 1), T_maxDen, T, T_inittype, 0>::setMaker(arr);
    }

    // Generate Rational Polynomial Fitter
    CONSTEXPR T* GetRationalPolyFit(const T_inittype& n, const T_inittype& d, const typename T_inittype::value_type& scale = 1.) const {
        return arr[(n.size() - 1) + (T_maxDen * (d.size()))](n, d, scale);
    }

    // Generate Polynomial Fitter
    CONSTEXPR T* GetPolyFit(const T_inittype& n, const typename T_inittype::value_type& scale = 1.) const {
        static const T_inittype z;
        return arr[n.size() - 1](n, z, scale);
    }

    // Generate Fractional Polynomial Fitter (v^X*Y)+Z
    CONSTEXPR T* GetFractionalPolyFit(const T_inittype& v, const typename T_inittype::value_type& scale = 1.) const {
        return FractionalPolynomialFitter<T, T_inittype>::Make(v, scale);
    }
};

// File static global. Compiler optimizes it so array of maker methods can live entirely in image memory -
// populated by the loader.
CONSTEXPR static const T_PolyFitGenerator<7, 7, PolyFit<double>, std::vector<double>> s_PolyFitGenerator;

// Factory accessed via static methods to avoid exposing templates into tender world...

PolyFit<double>* PolyFitGenerator::GetRationalPolyFit  (const std::vector<double>& numeratorCoefs, const std::vector<double>& denominatorCoefs, const double &scale) {
    return s_PolyFitGenerator.GetRationalPolyFit(numeratorCoefs, denominatorCoefs, scale);
}

PolyFit<double>* PolyFitGenerator::GetPolyFit          (const std::vector<double>& coefs, const double& scale) {
    return s_PolyFitGenerator.GetPolyFit(coefs, scale);
}

PolyFit<double>* PolyFitGenerator::GetFractionalPolyFit(const std::vector<double>& coefs, const double& scale) {
    return s_PolyFitGenerator.GetFractionalPolyFit(coefs, scale);
}

bool printAndTestAgainstExpected(PolyFit<double>* poly, std::vector<std::array<double, 3>>& expected) {
    static const double s_CloseEnough = 0.0000001;
    bool sawFailure = false;

    for (int i = 0; i < expected.size(); i++) {
        double fit = poly->Fit(expected[i][0]);
        double slope = poly->Slope(expected[i][0]);
        std::cout << "{" << i << " , " << fit << " , " << slope << "}" << std::endl;
        double delta = fit - expected[i][1];
        if (fabs(delta) > s_CloseEnough) {
            std::cout << "i:" << i << ": wrong fit, got:" << fit << " expected " << expected[i][1] << std::endl;
            sawFailure = true;
        }
        delta = slope - expected[i][2];
        if (fabs(delta) > s_CloseEnough) {
            std::cout << "i:" << i << ": wrong slope, got:" << slope << " expected " << expected[i][2] << std::endl;
            sawFailure = true;
        }
    }
    if (sawFailure) std::cout << "Failed!" << std::endl;
    else std::cout << "Passed." << std::endl;

    return sawFailure;
}

void PolynomialRegressionTest(void) {

    // Test slope capability of fitted poly.
    // Each triple is v, watts at v, power slope at v
    // The immediates were computed offline, now compare
    // test values against fit and slope.

    {
        std::cout << "POLYFIT y = x ^ 2" << std::endl;
        std::vector<double> coefs = { 0,0,1 };
        std::vector<std::array<double, 3>> expected = {
            { 0 , 0  , 0  },
            { 1 , 1  , 2  },
            { 2 , 4  , 4  },
            { 3 , 9  , 6  },
            { 4 , 16 , 8  },
            { 5 , 25 , 10 },
            { 6 , 36 , 12 },
            { 7 , 49 , 14 },
            { 8 , 64 , 16 },
            { 9 , 81 , 18 }
        };

        PolyFit<double>* poly = PolyFitGenerator::GetPolyFit(coefs);
        printAndTestAgainstExpected(poly, expected);
    }

    {
        std::cout << "POLYFIT y = 2 * x ^ 2" << std::endl;
        std::vector<double> coefs = { 0,0,2 };
        std::vector<std::array<double, 3>> expected = {
            { 0 , 0   , 0  },
            { 1 , 2   , 4  },
            { 2 , 8   , 8  },
            { 3 , 18  , 12 },
            { 4 , 32  , 16 },
            { 5 , 50  , 20 },
            { 6 , 72  , 24 },
            { 7 , 98  , 28 },
            { 8 , 128 , 32 },
            { 9 , 162 , 36 } };

        PolyFit<double>* poly = PolyFitGenerator::GetPolyFit(coefs);
        printAndTestAgainstExpected(poly, expected);
    }

    {
        std::cout << "POLYFIT y = 4 * x ^ 5 + 3 x ^ 2 + 2 x + 7" << std::endl;
        std::vector<double> coefs = { 7,2,3,0,4 };
        std::vector<std::array<double, 3>> expected = {
            {0 , 7     , 2     },
            {1 , 16    , 24    },
            {2 , 87    , 142   },
            {3 , 364   , 452   },
            {4 , 1087  , 1050  },
            {5 , 2592  , 2032  },
            {6 , 5311  , 3494  },
            {7 , 9772  , 5532  },
            {8 , 16599 , 8242  },
            {9 , 26512 , 11720 }
        };

        PolyFit<double>* poly = PolyFitGenerator::GetPolyFit(coefs);
        printAndTestAgainstExpected(poly, expected);
    }

    {
        std::cout << "FRACTIONAL FIT y = x ^ 2" << std::endl;
        std::vector<double> coefs = { 2,1,0 };
        std::vector<std::array<double, 3>> expected = {
            { 0 , 0   , 0  },
            { 1 , 1   , 2  },
            { 2 , 4   , 4  },
            { 3 , 9   ,  6 },
            { 4 , 16  , 8 },
            { 5 , 25  , 10 },
            { 6 , 36  , 12 },
            { 7 , 49  , 14 },
            { 8 , 64  , 16 },
            { 9 , 81  , 18 } };

        PolyFit<double>* poly = PolyFitGenerator::GetFractionalPolyFit(coefs);
        printAndTestAgainstExpected(poly, expected);
    }

    {
        std::cout << "RATIONAL FIT y = x ^ 2" << std::endl;
        std::vector<double> coefs = { 0,0,1 };
        std::vector<double> denCoefs = { 0 };
        std::vector<std::array<double, 3>> expected = {
            { 0 , 0   , 0  },
            { 1 , 1   , 2  },
            { 2 , 4   , 4  },
            { 3 , 9  ,  6 },
            { 4 , 16  , 8 },
            { 5 , 25  , 10 },
            { 6 , 36  , 12 },
            { 7 , 49  , 14 },
            { 8 , 64 , 16 },
            { 9 , 81 , 18 } };

        PolyFit<double>* poly = PolyFitGenerator::GetRationalPolyFit(coefs, denCoefs);
        printAndTestAgainstExpected(poly, expected);
    }

    {
        std::cout << "RATIONAL FIT y = (x ^ 2) / (1 + (x ^ 3))" << std::endl;
        std::vector<double> coefs = { 0,0,1 };
        std::vector<double> denCoefs = { 0,0,1 };
        std::vector<std::array<double, 3>> expected = {
            {0	, 0	          ,  0             },
            {1	, 0.5	      ,  0.25          },
            {2	, 0.4444444444 , -0.1481481481 },
            {3	, 0.3214285714 , -0.09566326531},
            {4	, 0.2461538462 , -0.05869822485},
            {5	, 0.1984126984 , -0.03873771731},
            {6	, 0.1658986175 , -0.02726751471},
            {7	, 0.1424418605 , -0.02017137642},
            {8	, 0.1247563353 , -0.01550334576},
            {9	, 0.1109589041 , -0.01227810096}
        };

        PolyFit<double>* poly = PolyFitGenerator::GetRationalPolyFit(coefs, denCoefs);
        printAndTestAgainstExpected(poly, expected);
    }

    {
        std::cout << "RATIONAL FIT y = (1 + (x ^ 2)) / (1 + (x ^ 2))" << std::endl;
        std::vector<double> coefs = { 1,0,1 };
        std::vector<double> denCoefs = { 0,1 };
        std::vector<std::array<double, 3>> expected = {
            {0 , 1 , 0},
            {1 , 1 , 0},
            {2 , 1 , 0},
            {3 , 1 , 0},
            {4 , 1 , 0},
            {5 , 1 , 0},
            {6 , 1 , 0},
            {7 , 1 , 0},
            {8 , 1 , 0},
            {9 , 1 , 0}
        };

        PolyFit<double>* poly = PolyFitGenerator::GetRationalPolyFit(coefs, denCoefs);
        printAndTestAgainstExpected(poly, expected);
    }

    {
        std::cout << "RATIONAL FIT y = (x ^ 2) / (1 + (x ^ 2))" << std::endl;
        std::vector<double> coefs = { 0,0,1 };
        std::vector<double> denCoefs = { 0,1 };
        std::vector<std::array<double, 3>> expected = {
            {0 , 0        , 0}         ,
            {1 , 0.5      , 0.5}       ,
            {2 , 0.8      , 0.16}      ,
            {3 , 0.9      , 0.06}      ,
            {4 , 0.941176 , 0.0276817} ,
            {5 , 0.961538 , 0.0147929} ,
            {6 , 0.972973 , 0.00876552},
            {7 , 0.98     , 0.0056}    ,
            {8 , 0.984615 , 0.00378698},
            {9 , 0.987805 , 0.00267698}
        };

        PolyFit<double>* poly = PolyFitGenerator::GetRationalPolyFit(coefs, denCoefs);
        printAndTestAgainstExpected(poly, expected);
    }

    // Some struct that matches data layout. Main thing is that
    // 'time' and 'speed' fields exist and line up with the data.
    struct SpindownData {
        double time, cad, hr, distance, speed, nm, watts;
    };

    static const SpindownData Kinetic0[] =
    {
        { 43    ,74, 124    ,0.218709   ,35.9748    ,0  ,322 },
        { 44	,0	,126	,0.227924	,31.3092	,0	,0 },
        { 45	,0	,127	,0.235807	,25.5384	,0	,136 },
        { 46	,0	,127	,0.242162	,21.1068	,0	,84 },
        { 47	,0	,128	,0.247444	,17.6184	,0	,52 },
        { 48	,0	,129	,0.251868	,14.7996	,0	,30 },
        { 49	,0	,129	,0.255584	,12.4308	,0	,15 },
        { 50	,0	,129	,0.258855	,11.3364	,0	,4 },
        { 51	,0	,129	,0.261546	,8.5896	    ,0	,0 },
        { 52	,0	,129	,0.263682	,7.092	    ,0	,0 },
        { 53	,0	,129	,0.265421	,5.706	    ,0	,0 },
        { 54	,0	,129	,0.266055	,0	        ,0	,0 }
    };

    static const SpindownData Kinetic1[] =
    {
        { 96	,0	    ,141	,0.516778	,29.9448	,0	,123 },
        { 97	,0	    ,142	,0.523957	,22.392	    ,0	,77 },
        { 98	,0	    ,143	,0.52977	,18.7308	,0	,48 },
        { 99	,0	    ,143	,0.534643	,15.7608	,0	,28 },
        { 100	,0	    ,144	,0.538603	,13.2516	,0	,14 },
        { 101	,0	    ,144	,0.541942	,11.1996	,0	,3 },
        { 102	,0	    ,144	,0.544747	,9.3672	    ,0	,0 },
        { 103	,0	    ,145	,0.547091	,7.8192	    ,0	,0 },
        { 104	,0	    ,144	,0.549033	,6.4404	    ,0	,0 },
        { 105	,0	    ,144	,0.550611	,5.1732	    ,0	,0 },
        { 106	,0	    ,144	,0.551186	,0	        ,0	,0 }
    };

    // For convenience push references to above static data into arrays.
    std::vector<const SpindownData*> msdata;
    std::vector<size_t> msdatasize;
    
    msdata.push_back(Kinetic0); msdatasize.push_back(sizeof(Kinetic0) / sizeof(Kinetic0[0]));
    msdata.push_back(Kinetic1); msdatasize.push_back(sizeof(Kinetic1) / sizeof(Kinetic0[1]));

    // Spindown fitter: desired fit epsilon is 1kph, max order for match is 6.
    // 6 is almost certainly too high but this is for test so why not.
    SpindownToPolyFit < SpindownData, XYVector<double>> ms_stp(1, 6);
    
    // Array to hold fit variance (computed when each array is pushed.)
    std::vector<double> ms_variances;

    // Push data referenced by arrays into the spindown fitter.
    for (size_t i = 0; i < msdata.size(); i++) {
        ms_variances.push_back(
            ms_stp.Push(msdata[i], (unsigned)msdatasize[i]));
    }

    std::cout << "------------- SpindownTest ------------" << std::endl;
    std::cout << "Accumulated Power Fit" << std::endl;
    PolyFit<double> *pf = ms_stp.AsPolyFit();
    for (double d = 0.; d <= 55; d += 5) {
        double bfd = d;
        std::cout << d << "\t\t" << pf->Fit(bfd) << std::endl;
    }

    // Delete polyfit
    delete pf;
}
