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

#ifndef _GC_PolynomialRegression_h
#define  _GC_PolynomialRegression_h

#include <vector>

template <typename T_fptype>
struct PolyFit {
    typedef T_fptype value_type;
    virtual ~PolyFit() = 0;
    virtual value_type Fit(value_type v) const = 0;
    virtual value_type Slope(value_type v) const = 0;
    virtual value_type Integrate(value_type from, value_type to) const = 0;
    virtual void append(std::string& s) const = 0;
};

template <typename T_fptype> PolyFit<T_fptype>::~PolyFit() {}

struct PolyFitGenerator {
    // Following methods return heap allocated objects. delete when finished with them.
    static PolyFit<double>* GetPolyFit          (const std::vector<double>& coefs, const double& scale = 1.);
    static PolyFit<double>* GetRationalPolyFit  (const std::vector<double>& numeratorCoefs, const std::vector<double>& denominatorCoefs, const double &scale = 1.);
    static PolyFit<double>* GetFractionalPolyFit(const std::vector<double>& coefs, const double& scale = 1.);
};
#endif
