#ifndef DATASET_H
#define DATASET_H

/* Authors: Greg Hamerly and Jonathan Drake
 * Feedback: hamerly@cs.baylor.edu
 * See: http://cs.baylor.edu/~hamerly/software/kmeans.php
 * Copyright 2014
 *
 * A Dataset class represents a collection of multidimensional records, as is
 * typical in metric machine learning. Every record has the same number of
 * dimensions (values), and every value must be numeric. Undefined values are
 * not allowed.
 *
 * This particular implementation keeps all the data in a 1-dimensional array,
 * and also optionally keeps extra storage for the sum of the squared values of
 * each record. However, the Dataset class does NOT automatically populate or
 * update the sumDataSquared values.
 */

#include <cstddef>
#include <iostream>

class Dataset {
    public:
        // default constructor -- constructs a completely empty dataset with no
        // records
        Dataset() : n(0), d(0), nd(0), data(NULL), sumDataSquared(NULL) {}

        // construct a dataset of a particular size, and determine whether to
        // keep the sumDataSquared
        Dataset(int aN, int aD, bool keepSDS = false) : n(aN), d(aD), nd(n * d), 
                                      data(new double[nd]),
                                      sumDataSquared(keepSDS ?  new double[n] : NULL) {}

        // copy constructor -- makes a deep copy of everything in x
        Dataset(Dataset const &x);

        // destroys the dataset safely
        ~Dataset() {
            n = d = nd = 0; 
            double *dp = data, *sdsp = sumDataSquared;
            data = sumDataSquared = NULL;
            delete [] dp;
            delete [] sdsp;
        }

        // operator= is the standard deep-copy assignment operator, which
        // returns a const reference to *this.
        Dataset const &operator=(Dataset const &x);

        // allows modification of the record ndx and dimension dim
        double &operator()(int ndx, int dim);

        // allows const access to record ndx and dimension dim
        const double &operator()(int ndx, int dim) const;

        // fill the entire dataset with value. Does NOT update sumDataSquared.
        void fill(double value);

        // print the dataset to standard output (cout), using formatting to keep the
        // data in matrix format
        void print(std::ostream &out = std::cout) const;

        // n represents the number of records
        // d represents the dimension
        // nd is a shortcut for the value n * d
        int n, d, nd;

        // data is an array of length n*d that stores all of the records in
        // record-major (row-major) order. Thus data[0]...data[d-1] are the
        // values associated with the first record.
        double *data;

        // sumDataSquared is an (optional) sum of squared values for every
        // record. Thus, 
        //  sumDataSquared[0] = data[0]^2 + data[1]^2 + ... + data[d-1]^2
        //  sumDataSquared[1] = data[d]^2 + data[d+1]^2 + ... + data[2*d-1]^2
        // and so on. Note that this is the *intended* use of the sumDataSquared
        // field, but that the Dataset class does NOT automatically populate or
        // update the values in sumDataSquared.
        double *sumDataSquared;
};

#endif

