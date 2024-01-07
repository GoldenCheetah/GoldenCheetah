/* Authors: Greg Hamerly and Jonathan Drake
 * Feedback: hamerly@cs.baylor.edu
 * See: http://cs.baylor.edu/~hamerly/software/kmeans.php
 * Copyright 2014
 */

#include "kmeans_dataset.h"
// #include <iostream>
#include <iomanip>
#include <cassert>
#include <cstring>

// print the dataset to standard output (cout), using formatting to keep the
// data in matrix format
void Dataset::print(std::ostream &out) const {
    //std::ostream &out = std::cout;
    out.precision(6);
    int ndx = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < d; ++j) {
            out << std::setw(13) << data[ndx++] << " ";
        }
        out << std::endl;
    }
}

// returns a (modifiable) reference to the value in dimension "dim" from record
// "ndx"
double &Dataset::operator()(int ndx, int dim) {
#   ifdef DEBUG
    assert(ndx < n); 
    assert(dim < d); 
#   endif
    return data[ndx * d + dim];
}

// returns a (const) reference to the value in dimension "dim" from record "ndx"
const double &Dataset::operator()(int ndx, int dim) const {
#   ifdef DEBUG
    assert(ndx < n); 
    assert(dim < d); 
#   endif
    return data[ndx * d + dim];
}

// fill the entire dataset with value. Does NOT update sumDataSquared.
void Dataset::fill(double value) {
    for (int i = 0; i < nd; ++i) {
        data[i] = value;
    }
}

// copy constructor -- makes a deep copy of everything in x
Dataset::Dataset(Dataset const &x) {
    n = d = nd = 0;
    data = sumDataSquared = NULL;
    *this = x;
}

// operator= is the standard deep-copy assignment operator, which
// returns a const reference to *this.
Dataset const &Dataset::operator=(Dataset const &x) {
    if (this != &x) {

        // reallocate sumDataSquared and data as necessary
        if (n != x.n) {
            delete [] sumDataSquared;
            sumDataSquared = x.sumDataSquared ? new double[x.n] : NULL;
        }

        if (nd != x.nd) {
            delete [] data;
            data = x.data ? new double[x.nd] : NULL;
        }

        // reflect the new sizes
        n = x.n;
        d = x.d;
        nd = x.nd;

        // copy data as appropriate
        if (x.sumDataSquared) {
            memcpy(sumDataSquared, x.sumDataSquared, x.n * sizeof(double));
        }

        if (x.data) {
            memcpy(data, x.data, x.nd * sizeof(double));
        }

    }

    // return a reference for chaining assignments
    return *this;
}

