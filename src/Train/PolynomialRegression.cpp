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
#include "PolynomialRegression.h"
#include "MultiRegressionizer.h"

template <typename T, typename T_inittype>
struct FractionalPolynomialFitter : public T {
    typename typedef T::value_type T_fptype;

    std::array<T_fptype, 3> arr;
    T_fptype scale;

    static T* Make(const T_inittype& v, typename const T_inittype::value_type& scale) {
        return new FractionalPolynomialFitter(v, scale);
    }

    FractionalPolynomialFitter(const T_inittype& n, typename const T_inittype::value_type& s) : arr(), scale(s) {
        for (size_t i = 0; i < n.size(); i++) arr[i] = n[i];
    }

    T_fptype Fit(T_fptype v) {
        // Scale v, for example mph -> kph
        v = v * scale;

        // v^X * Y + Z
        return (pow(v, arr[0]) * arr[1]) + arr[2];
    }
};

template <size_t T_size, size_t T_num, typename T, typename T_inittype>
struct RationalFitter : public T {
    typename typedef T::value_type T_fptype;

    std::array<T_fptype, T_size> arr;
    T_fptype scale;

    static T* Make(const T_inittype& v, const T_inittype& v2, typename const T_inittype::value_type& scale) {
        return new RationalFitter(v, v2, scale);
    }

    RationalFitter(const T_inittype& n, const T_inittype& d, typename const T_inittype::value_type& s) : arr(), scale(s) {
        // Populate numerator coefficients.
        for (size_t i = 0; i < T_num; i++) arr[i] = n[i];

        // Denominator may have no coefficients (implicit 1.)
        size_t i = T_num;
        while (i < T_size) {
            arr[i] = d[i - T_num];
            i++;
        }
    }

    // Compute Fit.
    // Because fitter has templated size the compiler is able to fully unroll the fit
    // calculation. This function also serves all polynomial evaluation since denominator
    // has implicit 1 which allows zero sized denominator evaluation to be optimized away.
    T_fptype Fit(T_fptype v) {

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
};

template <size_t T_maxSize, size_t T_maxDen, typename T, typename T_inittype, size_t T_valueNumber>
struct RationalFitterGenerator {
    static void setMaker(std::array<T * (*)(const T_inittype&, const T_inittype&, typename const T_inittype::value_type&), T_maxSize>& p) {
        static const size_t s_num = (T_valueNumber % T_maxDen) + 1;
        static const size_t s_den = T_valueNumber / T_maxDen;
        p[T_valueNumber] = RationalFitter<s_num + s_den, s_num, T, T_inittype>::Make;
        RationalFitterGenerator<T_maxSize, T_maxDen, T, T_inittype, T_valueNumber + 1>::setMaker(p);
    }
};

template <size_t T_maxSize, size_t T_maxDen, typename T, typename T_inittype>
struct RationalFitterGenerator<T_maxSize, T_maxDen, T, T_inittype, T_maxSize> {
    static void setMaker(std::array<T * (*)(const T_inittype&, const T_inittype&, typename const T_inittype::value_type&), T_maxSize>& p) { p; }
};

template <size_t T_maxNum, size_t T_maxDen, typename T, typename T_inittype>
struct T_PolyFitGenerator {

    // Value number from <numeratorCount, denominatorCount> to a unique combination of the two,
    // for example:
    //
    // num,den max both 6...
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

    std::array<T * (*)(const T_inittype&, const T_inittype&, typename const T_inittype::value_type&), T_maxNum * (T_maxDen + 1)> arr;

    T_PolyFitGenerator() : arr() {
        RationalFitterGenerator<T_maxNum * (T_maxDen + 1), T_maxDen, T, T_inittype, 0>::setMaker(arr);
    }

    // Generate Rational Polynomial Fitter
    T* GetRationalPolyFit(const T_inittype& n, const T_inittype& d, typename const T_inittype::value_type& scale = 1.) const {
        return arr[(n.size() - 1) + (T_maxDen * (d.size()))](n, d, scale);
    }

    // Generate Polynomial Fitter
    T* GetPolyFit(const T_inittype& n, typename const T_inittype::value_type& scale = 1.) const {
        static const T_inittype z;
        return arr[n.size() - 1](n, z, scale);
    }

    // Generate Fractional Polynomial Fitter (v^X*Y)+Z
    T* GetFractionalPolyFit(const T_inittype& v, typename const T_inittype::value_type& scale = 1.) const {
        return FractionalPolynomialFitter<T, T_inittype>::Make(v, scale);
    }
};

// File static global. Compiler optimizes it so array of maker methods can live entirely in image memory -
// populated by the loader.
static const T_PolyFitGenerator<6, 6, PolyFit<double>, std::vector<double>> s_PolyFitGenerator;

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
