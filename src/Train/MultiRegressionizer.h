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

#ifndef _GC_MultiRegressionizer_h
#define  _GC_MultiRegressionizer_h

#include <vector>
#include <algorithm>
#include <array>
#include <cmath>
#include "BlinnSolver.h"
#include "PolynomialRegression.h"

//#define CONFIG_PRINTING

template<typename T> struct XYPair {
    typedef T value_type;

    T x, y;
};

template <typename T>
struct XYGetter {
    typedef T value_type;

    virtual T X(size_t) const = 0;
    virtual T Y(size_t) const = 0;
};

template<typename T> struct XYVector : public XYGetter<T>, public std::vector<XYPair<T>> {
    typedef struct XYPair<T> value_type;

    T X(size_t idx) const { return this->operator[](idx).x; }
    T Y(size_t idx) const { return this->operator[](idx).y; }
};

template <typename T_fptype>
struct T_Matrix
{
    std::vector<std::vector<T_fptype> > data;

    // Use [] to index into matrix row
    std::vector<T_fptype>& operator[](size_t i) { return data[i]; }

    // Declaring the length of the matrix, not including results if matrix is augmented.
    size_t N;

    T_Matrix(size_t dimensions, std::vector<std::vector<T_fptype> >& temp)
    {
        // switching the passed value of dimensions from the user to N, so it can be used throughout the struct
        N = dimensions;
        data = temp;
    }

    void print(void) {
#if defined(CONFIG_PRINTING)
        for (size_t i = 0; i < data.size(); i++) {
            for (size_t j = 0; j < data[i].size(); j++) {
                std::cout << data[i][j] << "\t";
            }
            std::cout << std::endl;
        }
#endif
    }
};

// Generic Polynomial Based on Vector.
template <typename T>
struct T_Polynomial : public std::vector<T> {
    typedef T T_fptype;

    T_fptype Fit(T_fptype v) const {
        static const T_fptype s_zero = 0.;
        static const T_fptype s_one = 1.;
        T_fptype ret = s_zero;
        T_fptype vpow = s_one;
        for (auto f : *this) {
            if (!IsZero2(f, s_one))
                ret += f * vpow;
            vpow *= v;
        }
        return ret;
    }

    T_fptype Slope(T_fptype v) const {
        static const T_fptype s_zero = 0.;
        static const T_fptype s_one = 1.;
        T_fptype ret = s_zero;
        T_fptype vpow = s_one;
        T_fptype count = s_one;
        for (size_t i = 1; i < this->size(); i++, count = count + 1) {
            if (!IsZero2((*this)[i], s_one)) {
                ret += (*this)[i] * vpow * count;
            }
            vpow *= v;
        }
        return ret;
    }
};

template <typename T>
class T_RegressionizerBase {
    typedef typename T::value_type::value_type T_fptype;
public:
    virtual T_fptype Slope(T_fptype) const = 0;
    virtual T_fptype Fit(T_fptype)   const = 0;
    virtual T_fptype StdDev()        const = 0;
    virtual T_fptype Order()         const = 0;
    virtual bool     Valid()         const = 0;
    virtual void     PrintConfig()   const = 0;
    virtual PolyFit<T_fptype>* AsPolyFit() const = 0;
};

template <typename T>
class T_PolyRegressionizer : public T_RegressionizerBase<T> {
    typedef typename T::value_type::value_type T_fptype;

    T_Polynomial<T_fptype> coefs;
    T_fptype stddev;
    T_fptype epsilon;
    unsigned maxOrder;
    unsigned order;

    // Populate matrix for polynomial least square fit, then solve via
    // row reduction. By magic the coefficients
    bool ComputeCoefs(
        const T& xy,
        const unsigned& order)
    {
        static const T_fptype s_one = 1.;

        if (xy.size() <= 0) return false;

        int N = (int)xy.size();
        int n = order;
        int np1 = n + 1;
        int np2 = n + 2;
        int tnp1 = 2 * n + 1;
        T_fptype tmp;

        std::vector<T_fptype> M(tnp1 + N);

        // Avoid pow, extra space in array holds accumulated powers of x[j]
        for (int j = 0; j < N; j++) {
            M[tnp1 + j] = s_one;
        }

        M[0] = (T_fptype)(int)N;
        for (int i = 1; i < tnp1; ++i) {
            T_fptype sum = 0;
            for (int j = 0; j < N; j++) {
                T_fptype v = M[tnp1 + j] * xy.X(j);
                M[tnp1 + j] = v;
                sum += v;
            }
            M[i] = sum;
        }

        // Matrix B: populated then row reduced.
        std::vector<std::vector<T_fptype> > B(np1, std::vector<T_fptype>(np2, 0));

        for (int i = 0; i <= n; ++i)
            for (int j = 0; j <= n; ++j)
                B[i][j] = M[i + j];

        std::vector<T_fptype> Y(np1 + N);

        for (int j = 0; j < N; j++) {
            Y[0] += xy[j].y;
            Y[np1 + j] = s_one;
        }

        for (int i = 1; i < np1; ++i) {
            T_fptype sum = 0;
            for (int j = 0; j < N; j++) {
                XYPair<T_fptype> t = xy[j];
                T_fptype v = Y[np1 + j] * t.x;
                Y[np1 + j] = v;
                sum += v * t.y;
            }
            Y[i] = sum;
        }

        for (int i = 0; i <= n; ++i)
            B[i][np1] = Y[i];

        n += 1;
        int nm1 = n - 1;

        // Pivot
        for (int i = 0; i < n; ++i)
            for (int k = i + 1; k < n; ++k)
                if (B[i][i] < B[k][i])
                    for (int j = 0; j <= n; ++j) {
                        tmp = B[i][j];
                        B[i][j] = B[k][j];
                        B[k][j] = tmp;
                    }

        // row reduction
        for (int i = 0; i < nm1; ++i)
            for (int k = i + 1; k < n; ++k) {
                T_fptype t = B[k][i] / B[i][i];
                for (int j = 0; j <= n; ++j)
                    B[k][j] -= t * B[i][j];
            }

        // back sub onto result vector.
        coefs.clear();
        coefs.resize(np1);

        for (int i = nm1; i >= 0; --i) {
            coefs[i] = B[i][n];
            for (int j = 0; j < n; ++j)
                if (j != i)
                    coefs[i] -= B[i][j] * coefs[j];

            coefs[i] /= B[i][i];
        }

        return true;
    }

public:

    bool Build(const T& xy, double e) {
        static const T_fptype s_zero = 0.;
        static const T_fptype s_minusOne = -1.;

        epsilon = e;

        // Build integer poly regression. Use whichever is best.
        // Iterate over orders, find first order that has great fit
        for (unsigned i = 0; i <= maxOrder; i++) {
            order = i;

            // If fit cannot be found then fail.
            if (!ComputeCoefs(xy, order)) {
                stddev = s_minusOne;
                break;
            }

            // Compute StdDev
            T_fptype sumSquares = s_zero;
            for (auto elem : xy) {
                T_fptype delta = elem.y - Fit(elem.x);
                sumSquares += delta * delta;
            }
            T_fptype N = (unsigned)xy.size();
            stddev = sqrt(sumSquares / N);

            // Try higher and higher order until fit is accurate 'enough'.
            if (stddev < epsilon)
                break;
        }

        return Valid();
    }

    T_PolyRegressionizer(T_fptype e = 0.1, unsigned mo = 3) : stddev(0.), epsilon(e), maxOrder(mo), order(0) {}

    virtual T_fptype Fit(T_fptype kph)   const { return coefs.Fit(kph); }
    virtual T_fptype Slope(T_fptype kph) const { return coefs.Slope(kph); }
    virtual T_fptype StdDev()            const { return stddev; }
    virtual T_fptype Order()             const { return (T_fptype)(int)coefs.size() - 1; }
    virtual bool     Valid()             const { static const T_fptype s_zero = 0.; return stddev >= s_zero; }
    virtual void     PrintConfig()       const {
#if defined(CONFIG_PRINTING)
        std::cout << "Poly Regression Curve, order<" << coefs.size() << ">, stddev: " << StdDev() << std::endl;
#endif
    }
    virtual PolyFit<T_fptype>* AsPolyFit() const {
        return PolyFitGenerator::GetPolyFit(coefs);
    }
};

template <typename T> class T_FractionalPolyRegressionizer : public T_RegressionizerBase<T> {

    typedef typename T::value_type::value_type T_fptype;

    T_fptype m_X, m_Y, m_Z, m_stddev, m_epsilon;
    unsigned maxOrder;

    // Finds a fit X, Y, Z for:
    //
    // w = v ^ X * Y + Z
    //
    // Returns stddev if success, else returns negative value.
    // Fit will fail if any negative x.
    T_fptype ComputeCoefs(const T& xy) {
        static const T_fptype s_zero = 0.;
        static const T_fptype s_one = 1.;
        static const T_fptype s_minusOne = -1.;
        static const T_fptype s_minusTwo = -2.;
        static const T_fptype s_half = 0.5;

        if (xy.size() == 0) {
            T_fptype  ret = -1;
            return ret;
        }

        std::vector<T_fptype> y;
        for (auto v : xy) {
            if (v.x < s_zero) {
                return s_minusOne;
            }
            y.push_back(v.y);
        }

        T_fptype sumSquares = s_minusOne;

        // bias all y by first element - that is Z.
        m_Z = y[0];

        size_t e = y.size() - 1;

        if (m_Z != s_zero) for (auto &i : y) i -= m_Z;

        // Start with X = 1., a line from first to last point.
        m_X = s_one;

        T_fptype delta = s_half;

        std::vector<T_fptype> results(e+1);

        bool firstLoop = true;

        do {
            T_fptype xv = xy.X(e);
            T_fptype yv = y[e];
            if (xv == 0.) m_Y = s_one;
            else if (yv == 0.) m_Y = s_one;
            else m_Y = yv / pow(xv, m_X);

            T_fptype newSumSquares = 0;
            for (size_t i = 0; i <= e; i++) {
                xv = xy.X(i);
                yv = xy.Y(i);
                if (xv == s_zero) results[i] = s_zero;
                else results[i] = pow(xv, m_X) * m_Y;
                T_fptype deltat = results[i] - yv;
                newSumSquares += (deltat * deltat);
            }

            // Its good if sum of squares decreases.
            bool good = firstLoop || newSumSquares < sumSquares;
            sumSquares = newSumSquares;

            if (!good)
                delta /= s_minusTwo;

            // Stop when step gets real small.
            if (fabs(delta) <= m_epsilon)
                break;

            m_X += delta;

            if (fabs(m_X) > (int)maxOrder)
                break;

            firstLoop = false;
        } while (true);

        T_fptype N = (unsigned)xy.size();
        T_fptype variance = sumSquares / N;
        T_fptype stddev = sqrt(variance);

        return stddev;
    }

public:

    T_FractionalPolyRegressionizer(T_fptype e = 0.000001, unsigned mo = 3) {
        static const T_fptype s_zero = 0.;
        static const T_fptype s_minusOne = -1.;
        maxOrder = mo; m_X = s_zero; m_Y = s_zero; m_Z = s_zero; m_stddev = s_minusOne; m_epsilon = e;
    }
    
    bool Build(const XYVector<T_fptype>& xy, double epsilon) {
        Q_UNUSED(epsilon) // This function has no ability to honor epsilon
        m_stddev = ComputeCoefs(xy);
        return Valid();
    }

    virtual T_fptype Fit(T_fptype kph)   const { static const T_fptype s_zero = 0.; return (kph < s_zero) ? s_zero : (pow(kph, m_X) * m_Y + m_Z); }
    virtual T_fptype Slope(T_fptype kph) const { static const T_fptype s_zero = 0.; return (kph < s_zero) ? s_zero : (kph * pow(kph, m_X - 1)); }
    virtual T_fptype StdDev()            const { return m_stddev; }
    virtual T_fptype Order()             const { return m_X; }
    virtual bool     Valid()             const { static const T_fptype s_zero = 0.; return StdDev() >= s_zero; }
    virtual void     PrintConfig()       const {
#if defined(CONFIG_PRINTING)
        std::cout << "Fractional Poly Regression Curve, x^" << m_X << "*" << m_Y << "+" << m_Z << ", stddev: " << StdDev() << std::endl;
#endif
    }
    virtual PolyFit<T_fptype>* AsPolyFit() const {
        return PolyFitGenerator::GetFractionalPolyFit({ m_X, m_Y, m_Z });
    }
};

// Regression Fit using Rational Polynomial.
// Clearest explanation I found was this:
// https://math.stackexchange.com/questions/924482/least-squares-regression-matrix-for-rational-functions
// Uses gaussian elimination to solve.
template <typename T>
class T_RationalPolyRegressionizer : public T_RegressionizerBase<T> {
    typedef typename T::value_type::value_type T_fptype;

    T_Polynomial<T_fptype> m_num, m_den;

    T_fptype stddev;
    T_fptype epsilon;

    unsigned maxOrder;

    // This is a quick power for simple integer exponent. This is much more
    // efficient than standard pow, inlines into simple multiply.
    static T_fptype T_pow(T_fptype x, int iPow) {
        static const T_fptype s_one = 1.;
        if (iPow == 0)
            return s_one;

        T_fptype p = T_fptype(x);
        T_fptype pr = p;
        for (T_fptype ip = 1; ip < iPow; ip = ip + 1)
            pr = pr * p;

        return pr;
    }

    static T_fptype T_fabs(T_fptype t) {
        static const T_fptype s_zero = 0.;
        return (t < s_zero) ? -(t) : (t);
    }

    static void T_GaussElimination(T_Matrix<T_fptype>& M, std::vector<T_fptype>& XVec) {
        static const T_fptype s_zero = 0.;

        size_t n = M.data.size();
        XVec.clear();
        XVec.resize(n);

        for (size_t i = 0; i < n; i++) {
            // Search for maximum in this column
            T_fptype maxelement = T_fabs(M[i][i]);
            size_t maxRow = i;
            for (size_t k = i + 1; k < n; k++) {
                T_fptype pos = T_fabs(M[k][i]);
                if (pos > maxelement) {
                    maxelement = pos;
                    maxRow = k;
                }
            }
            // Swap maximum row with current row (column by column)
            if (maxRow != i) {
                for (size_t k = i; k < n + 1; k++) {
                    T_fptype tmp = M[maxRow][k];
                    M[maxRow][k] = M[i][k];
                    M[i][k] = tmp;
                }
            }
            // Make all rows below this one 0 in current column
            for (size_t k = i + 1; k < n; k++) {
                T_fptype c = M[k][i] / M[i][i];
                for (size_t j = i; j < n + 1; j++) {
                    if (i == j) {
                        M[k][j] = s_zero;
                    }
                    else {
                        M[k][j] = M[k][j] - (c * M[i][j]);
                    }
                }
            }
        }

        for (int i = (int)(n - 1); i >= 0; i--) {
            if (M[i][i] != T_fptype(0)) {
                XVec[i] = M[i][n] / M[i][i];
                for (int k = (int)(i - 1); k >= 0; k--) {
                    M[k][n] = M[k][n] - (M[k][i] * XVec[i]);
                }
            }
        }
    }

    // Build augmented matrix for computing best fit rational polynomial
    bool BuildRationalPolynomialMatrix(std::vector<std::vector<T_fptype> >& X, std::vector<T_fptype>& x, std::vector<T_fptype>& y, size_t orderN, size_t orderM) {

        static const T_fptype s_zero = 0.;

        orderM = std::min<size_t>(orderM, x.size() + 1);
        orderN = std::min<size_t>(orderN, x.size() + 1);

        size_t dim = orderN + orderM + 1;

        X.resize(dim);
        for (auto& i : X) {
            i.resize(dim + 1); // '+ 1': make room for results column
        }

        // Top left square matrix srows: n+1, scol: n+1

        // Sum(j) x(j)^(srow + scol)

        // Compute values above diagonal
        for (size_t i = 0; i <= orderN; i++) {         // i = row
            for (size_t j = i; j <= orderN; j++) {     // j = column
                T_fptype v = s_zero;
                for (auto xn : x)
                    v += T_pow(T_fptype(xn), (int)(i + j));

                X[i][j] = v;
            }
        }

        // Copy upper diagonal values to below diagonal
        for (size_t i = 1; i <= orderN; i++) {
            for (size_t j = 0; j < i; j++) {
                X[i][j] = X[j][i];
            }
        }

        // Bottom left rows, srows: m, scol: n + 1

        // Sum(j) y(j) * x(j)^(srow + scol + 1)

        for (size_t i = 0; i < orderM; i++) {         // i = sub row
            for (size_t j = 0; j < orderN + 1; j++) { // j = sub column

                size_t aI = i + orderN + 1;  // target matrix row idx
                size_t aJ = j;               // target matrix col idx

                // If possible copy duplicate from previous row.
                if (i > 0 && j < orderN) {
                    X[aI][aJ] = X[aI - 1][aJ + 1];
                } else {
                    T_fptype v = s_zero;
                    int iPow = (int)(i + j + 1);
                    for (size_t n = 0; n < x.size(); n++) {
                        v += T_fptype(y[n]) * T_pow(T_fptype(x[n]), iPow);
                    }

                    X[aI][aJ] = v;
                }
            }
        }

        // Right Hand Columns row: n+1, col: m

        // - sum(j) y(j) * x(j)^(srow + scol + 1)

        for (size_t i = 0; i < orderN + 1; i++) {    // i = sub row
            for (size_t j = 0; j < orderM; j++) {    // j = sub col

                size_t aI = i;                       // target matrix row idx
                size_t aJ = j + orderN + 1;          // target matrix col idx

                // If possible copy duplicate from previous column.
                if (j > 0 && i < orderN) {
                    X[aI][aJ] = X[aI + 1][aJ - 1];
                } else {
                    T_fptype v = s_zero;
                    int iPow = (int)(i + j + 1);
                    for (size_t n = 0; n < x.size(); n++) {
                        v += T_fptype(y[n]) * T_pow(T_fptype(x[n]), iPow);
                    }

                    v = -v;

                    X[aI][aJ] = v;
                }
            }
        }

        // Bottom right square, srow: m, scol: m

        // sum(j) y(j)^2 * x(j)^(srow + scol + 2)

        for (size_t i = 0; i < orderM; i++) {
            for (size_t j = 0; j < orderM; j++) {

                size_t aI = i + orderN + 1;
                size_t aJ = j + orderN + 1;

                if (i > 0 && j < i) {
                    X[aI][aJ] = X[aI - 1][aJ + 1];
                } else {
                    int iPow = (int)(i + j + 2);
                    T_fptype v = s_zero;
                    for (size_t n = 0; n < x.size(); n++) {
                        v += T_pow(T_fptype(y[n]), 2) * T_pow(T_fptype(x[n]), iPow);
                    }

                    v = -v;

                    X[aI][aJ] = v;
                }
            }
        }

        // Populate result coef column:

        // Top part of column vector
        for (size_t i = 0; i < orderN + 1; i++) {
            T_fptype sum = s_zero;
            for (size_t j = 0; j < y.size(); j++) {
                sum += y[j] * T_pow(x[j], (int)i);
            }
            X[i][dim] = sum;
        }
        // Bottom part of column vector
        for (size_t i = 0; i < orderM; i++) {
            T_fptype sum = s_zero;
            for (size_t j = 0; j < y.size(); j++) {
                sum += T_pow(y[j], 2) * T_pow(x[j], (int)(i + 1));
            }
            X[i + orderN + 1][dim] = sum;
        }

        return true;
    }

public:

    T_RationalPolyRegressionizer(T_fptype e = 0.1, unsigned mo = 3) : stddev(-1.), epsilon(e), maxOrder(mo) {}

    // Build computes poly fit for data, then divides data by poly fitted values, then finds poly fit for those residuals.
    bool Build(const T& xy, double e) {
        epsilon = e;

        static const T_fptype s_zero = 0.;
        static const T_fptype s_minusOne = -1.;

        T_fptype bestStdDev = s_minusOne;
        T_Polynomial<T_fptype> bestNum, bestDen;

        std::vector<T_fptype> x, y;
        for (size_t i = 0; i < xy.size(); i++) {
            x.push_back(xy.X(i));
            y.push_back(xy.Y(i));
        }

        // Search for best fit poly within order.
        for (int m = 0; m <= (int)maxOrder; m++) {
            for (int n = 0; n <= (int)maxOrder; n++) {
                m_num.clear();
                m_den.clear();

                std::vector<std::vector<T_fptype>> X;

                BuildRationalPolynomialMatrix(X, x, y, n, m);

                std::vector<T_fptype> adr;
                T_Matrix<T_fptype> TM(X.size(), X);

                T_GaussElimination(TM, adr);

                m_den.push_back(1.);
                for (size_t i = 0; i < adr.size(); i++) {
                    if (i <= n) m_num.push_back(adr[i]);
                    else m_den.push_back(adr[i]);
                }

                T_fptype sumSquares = s_zero;
                T_fptype maxY = 0.;
                T_fptype minY = 0.;
                T_fptype minX = 0.;
                T_fptype maxX = 0.;
                for (auto i : xy) {
                    T_fptype fitVal = Fit(i.x);

                    minX = std::min<T_fptype>(minX, i.x);
                    maxX = std::max<T_fptype>(maxX, i.x);
                    minY = std::min<T_fptype>(minY, fitVal);
                    maxY = std::max<T_fptype>(maxY, fitVal);

                    T_fptype delta = i.y - fitVal;
                    sumSquares += delta * delta;
                }

                // Now test for asymptotes across finer range of x.
                // If we find asymptotes then this is not a good candidate,
                // set stddev artificially high and move on.
                static bool fail = false;
                fail = false;
                static const T_fptype s_hundo = 100.;
                T_fptype rangeY = (maxY - minY);
                T_fptype delta = (maxX - minX) / s_hundo;
                T_fptype tripleRangeY = rangeY + rangeY + rangeY;
                T_fptype upperLimitY = maxY + (tripleRangeY);
                T_fptype lowerLimitY = minY - (tripleRangeY);
                for (T_fptype i = minX; i < (maxX + maxX); i+= delta) {
                    T_fptype fitVal = Fit(i);
                    if (fitVal > upperLimitY || fitVal < lowerLimitY) {
                        fail = true;
                        break;
                    }
                }

                if (fail) {
                    stddev = s_hundo * s_hundo;
                } else {
                    T_fptype N = (unsigned)xy.size();
                    T_fptype variance = sumSquares / N;
                    stddev = sqrt(variance);
                }

                // Now to determine 'best'.
                // If both are already below epsilon then take the one with lower order.
                bool isNewBest = false;
                if (bestStdDev < 0) isNewBest = true;
                else if (stddev < bestStdDev) {
                    if (bestStdDev > epsilon)
                        isNewBest = true;
                    else {
                        size_t bestMax = std::max(bestNum.size(), bestDen.size());
                        size_t curMax = std::max(m_num.size(), m_den.size());
                        if (curMax < bestMax)
                            isNewBest = true;
                        if (curMax == bestMax) {
                            size_t bestSum = bestNum.size() + bestDen.size();
                            size_t curSum = m_num.size() + m_den.size();
                            if (curSum < bestSum)
                                isNewBest = true;
                        }
                    }
                }

                if (isNewBest) {
                    bestStdDev = stddev;
                    bestNum = m_num;
                    bestDen = m_den;
                }

            }  // numerator order iteration
        } // denominator order iteration

        m_num = bestNum;
        m_den = bestDen;
        stddev = bestStdDev;

        return Valid();
    }

    virtual T_fptype Fit(T_fptype kph) const {
        T_fptype n = m_num.Fit(kph);
        T_fptype d = m_den.Fit(kph);
        T_fptype v = n / d;
        return v;
    }

    virtual T_fptype Slope(T_fptype v) const {
        T_fptype N = m_num.Fit(v);
        T_fptype D = m_den.Fit(v);

        T_fptype dNdV = m_num.Slope(v);
        T_fptype dDdV = m_den.Slope(v);

        return ((dNdV * D) - (N * dDdV)) / (D * D);
    }

    virtual T_fptype StdDev()      const { return stddev; }
    virtual T_fptype Order()       const { return (T_fptype)std::max<int>((int)m_num.size() - 1, (int)m_den.size() - 1); }
    virtual bool     Valid()       const { static const T_fptype s_zero = 0.; return stddev >= s_zero; }
    virtual void     PrintConfig() const {
#if defined(CONFIG_PRINTING)
        std::cout << "Rational Poly Regression Curve, order<" << m_num.size() << "," << m_den.size() << ">, stddev: " << StdDev() << std::endl;
#endif
    }

    virtual PolyFit<T_fptype>* AsPolyFit() const {
        // Create denominator vector without leading 1.
        std::vector<T_fptype> den;
        for (size_t i = 1; i < m_den.size(); i++) {
            den.push_back(m_den[i]);
        }
        return PolyFitGenerator::GetRationalPolyFit(m_num, den, 1.);
    }
};

template <typename T>
class T_MultiRegressionizer {

    typedef typename T::value_type::value_type T_fptype;

    T                                         m_xy;
    mutable T_PolyRegressionizer<T>           m_pr;
    mutable T_FractionalPolyRegressionizer<T> m_fpr;
    mutable T_RationalPolyRegressionizer<T>   m_rpr;

    const mutable T_RegressionizerBase<T>*    m_best;
    mutable bool                              m_isFinalized;

    const double                              m_epsilon;

    const T_RegressionizerBase<T>* CompareAndReturnBest(const T_RegressionizerBase<T>* a, const T_RegressionizerBase<T>* b) const {

        // Prefer a choice that exists.
        int c = (a && a->Valid()) + (2 * (b && b->Valid()));
        switch (c) {
        case 0: return NULL;
        case 1: return a;
        case 2: return b;
        }

        // Prefer a choice that has stddev below epsilon
        int v = (a->StdDev() <= m_epsilon) + (2 * (b->StdDev() <= m_epsilon));
        switch (v) {
        case 0: return (a->StdDev() <= b->StdDev()) ? a : b;
        case 1: return a;
        case 2: return b;
        }

        // Both valid and stddev less than epsilon, choose model with lower order
        return (a->Order() <= b->Order()) ? a : b;
    }

    bool Build() const {
        if (!m_isFinalized) {
            m_fpr.Build(m_xy, m_epsilon);
            m_pr.Build(m_xy,  m_epsilon);
            m_rpr.Build(m_xy, m_epsilon);

            // Select best fit
            m_best = CompareAndReturnBest(&m_fpr, &m_pr);
            m_best = CompareAndReturnBest(m_best, &m_rpr);

            m_isFinalized = (m_best != NULL);
        }

        return m_isFinalized;
    }

public:

    typedef T value_type;

    T_MultiRegressionizer(double e = 0.1, unsigned mo = 3) : 
        m_pr(e, mo), m_fpr(e, mo), m_rpr(e, mo), m_best(NULL), m_isFinalized(false), m_epsilon(e) {}

    void Set(const T& p)            { m_isFinalized = false; m_xy = p; }
    void Push(XYPair<T_fptype> val) { m_isFinalized = false; m_xy.push_back(val); }
    void Clear()                    { m_isFinalized = false; m_xy.clear(); }

    T_fptype Fit(T_fptype kph) const {
        static const T_fptype s_zero = 0.;

        T_fptype ret = s_zero;

        if (Build())
            ret = m_best->Fit(kph);

        return ret;
    }

    T_fptype Slope(T_fptype kph) const {
        static const T_fptype s_zero = 0.;

        T_fptype ret = s_zero;

        if (Build())
            ret = m_best->Slope(kph);

        return ret;
    }

    T_fptype XYToYDYDT(T_MultiRegressionizer<T>& ydydt) const {
        for (auto i : m_xy)
            ydydt.Push({ i.y, Slope(i.x) });

        return StdDev();
    }

    T_fptype StdDev() const {
        T_fptype ret = -1.;

        if (!Build()) return ret;

        return m_best->StdDev();
    }

    T_fptype Order() const {
        if (!Build()) return -1;
        return m_best->Order();
    }

    void PrintConfig() {
#if defined(CONFIG_PRINTING)
        if (!Build()) std::cout << "Failed!" << std::endl;
        else m_best->PrintConfig();
#endif
    }

    void Print() {
#if defined(CONFIG_PRINTING)
        if (!Build()) std::cout << "Print() failed." << std::endl;
        else {
            for (int i = 0; i < m_xy.size(); i++) {
                std::cout << m_xy.X(i) << "\t\t" << m_xy.Y(i) << std::endl;
            }
        }
#endif
    }

    bool Valid() {
        return Build();
    }

    PolyFit<T_fptype>* AsPolyFit() {
        if (!Valid()) return NULL;

        return m_best->AsPolyFit();
    }
};

// Gather data from spindown array (type C) into reversed array of xy pairs. Time
// is reset to start at 0. This yields xy pairs where speed increases, the slope
// of the speed increase is the deceleration applied by the trainer at that speed.
template <typename T_fptype, typename C, typename T>
void T_SpindownArrayToTimeKph(const C* p, unsigned count, T& xy) {

    T_fptype time0 = p[0].time;
    T_fptype time = p[count - 1].time;
    T_fptype totalTime = time - time0;
    for (int i = count - 1; i >= 0; i--) {
        time = p[i].time;
        T_fptype speed = p[i].speed;
        xy.Push({ totalTime - (time - time0), speed });
    }
}

// Accumulate spindown data into provided xy pair vector.
//
// Fit original time/speed samples to polynomial 'speedFit'.
// Compute new curve 'accFit', the derivative of 'speedFit'.
// 'accFit' is the curve of speed to user deceleration.
// Those pairs are datapoints without context and can be combined with data
// from other spindowns to create a multiple-spindown fit. (See: SpindownToPolyFit)
// Points are accumulated onto 'resultsXY' which is the list of pairs of <speed, deceleration>.
template <typename T_fptype, typename C, typename T>
T_fptype T_SpeedTimeToAccKph_Accumulate(const C* p, unsigned count, T& resultsXY, unsigned maxOrder = 3, double fitepsilon = 10.) {

    // Max order only applies to initial fit to raw user data. After we have a fit
    // theres not much point ignoring the fit's stddevs.
    T_MultiRegressionizer<XYVector<T_fptype>> speedFit(fitepsilon, maxOrder);
    T_SpindownArrayToTimeKph<T_fptype, C>(p, count, speedFit);

    // Fitting acceleration into a speed power curve. Since this is a fit of a
    // fitted function we should expect a near perfect fit for its slope and with
    // a lesser order.
    T_MultiRegressionizer<XYVector<T_fptype>> accFit(0.1, maxOrder);

    // Converts slope graph of speedfit into a new polynomial of change in
    // speed with respect to speed. This is decel per speed. The stddev
    // returned here is how well speedFit was able to model the original data.
    T_fptype stddev = speedFit.XYToYDYDT(accFit);

    // Now resample original speeds against accFit to produce speed/acc pairs, then
    // push those pairs onto result array.

    // Use the speeds from incoming points since they are the least interpolated.
    for (size_t i = 0; i < count; i++) {
        resultsXY.push_back({ p[i].speed, accFit.Fit(p[i].speed) });
    }

    return stddev;
}

// Class to fit multiple spindowns into a single speed/deceleration curve.
template <typename C, typename T>
class SpindownToPolyFit {

    typedef typename T::value_type::value_type T_fptype;

    T_MultiRegressionizer<T> m_multifit;
    T m_resultsXY;

    unsigned m_maxOrder;

    bool m_built;

    bool Build(void) {
        if (!m_built) {
            // sort accumulated speed/acceleration results
            std::sort(m_resultsXY.begin(), m_resultsXY.end(),
                [](const XYPair<T_fptype>& n1, const XYPair<T_fptype>& n2)->bool {
                return n1.x < n2.x;
            });

            // Clear and re-fit speed/acceleration results.
            m_multifit.Clear();
            for (auto i : m_resultsXY) {
                m_multifit.Push(i);
            }

            m_built = true;
        }

        return m_built;
    }

public:

    // Epsilon is the stddev goal for fitting spindown data. Fitter will go up to maxOrder
    // to achieve the goal. Higher order functions are unstable and do wacky things, so
    // often its better to tolerate a looser data fit by lowering desired stddev than to
    // raise the maxOrder.
    SpindownToPolyFit(double epsilon, unsigned maxOrder) :  m_multifit(epsilon, maxOrder), m_maxOrder(maxOrder), m_built(false) {}

    // Process spindown graph into speed/deceleration datapoints and accumulate.
    T_fptype Push(const C* p, unsigned count) {
        m_built = false;
        return T_SpeedTimeToAccKph_Accumulate<T_fptype, C, T> (p, count, m_resultsXY, m_maxOrder);
    }

    T_fptype StdDev() const {
        static const T_fptype s_zero = 0.;
        if (!Build()) return s_zero;
        return m_multifit->StdDev();
    }

    // Fit speed against current determined speed/power graph.
    T_fptype Fit(T_fptype v) {
        // Return zero on failure?
        static const T_fptype s_zero = 0.;
        if (!Build()) return s_zero;

        return m_multifit.Fit(v);
    }

    // Convert current speed/power graph to polyfit for use outside this class.
    // Caller must call delete upon it when finished.
    PolyFit<T_fptype>* AsPolyFit() {
        if (!Build()) return NULL;
        
        return m_multifit.AsPolyFit();
    }
};

#endif // _GC_MultiRegressionizer_h
