/* Authors: Greg Hamerly and Jonathan Drake
 * Feedback: hamerly@cs.baylor.edu
 * See: http://cs.baylor.edu/~hamerly/software/kmeans.php
 * Copyright 2014
 */

#include "original_space_kmeans.h"
#include "kmeans_general_functions.h"
#include <cmath>
#include <cassert>
#include <numeric>

OriginalSpaceKmeans::OriginalSpaceKmeans() : centers(NULL), sumNewCenters(NULL) { }

void OriginalSpaceKmeans::free() {
    for (int t = 0; t < numThreads; ++t) {
        delete sumNewCenters[t];
    }
    Kmeans::free();
    delete centers;
    delete [] sumNewCenters;
    centers = NULL;
    sumNewCenters = NULL;
}

/* This method moves the newCenters to their new locations, based on the
 * sufficient statistics in sumNewCenters. It also computes the centerMovement
 * and the center that moved the furthest.
 *
 * Parameters: none
 *
 * Return value: index of the furthest-moving centers
 */
int OriginalSpaceKmeans::move_centers() {
    int furthestMovingCenter = 0;
    for (int j = 0; j < k; ++j) {
        centerMovement[j] = 0.0;
        int totalClusterSize = 0;
        for (int t = 0; t < numThreads; ++t) {
            totalClusterSize += clusterSize[t][j];
        }
        if (totalClusterSize > 0) {
            for (int dim = 0; dim < d; ++dim) {
                double z = 0.0;
                for (int t = 0; t < numThreads; ++t) {
                    z += (*sumNewCenters[t])(j,dim);
                }
                z /= totalClusterSize;
                centerMovement[j] += (z - (*centers)(j, dim)) * (z - (*centers)(j, dim));
                (*centers)(j, dim) = z;
            }
        }
        centerMovement[j] = sqrt(centerMovement[j]);

        if (centerMovement[furthestMovingCenter] < centerMovement[j]) {
            furthestMovingCenter = j;
        }
    }

    #ifdef COUNT_DISTANCES
    numDistances += k;
    #endif

    return furthestMovingCenter;
}

void OriginalSpaceKmeans::initialize(Dataset const *aX, unsigned short aK, unsigned short *initialAssignment, int aNumThreads) {
    Kmeans::initialize(aX, aK, initialAssignment, aNumThreads);

    centers = new Dataset(k, d);
    sumNewCenters = new Dataset *[numThreads];
    centers->fill(0.0);

    for (int t = 0; t < numThreads; ++t) {
        sumNewCenters[t] = new Dataset(k, d, false);
        sumNewCenters[t]->fill(0.0);
        for (int i = start(t); i < end(t); ++i) {
            addVectors(sumNewCenters[t]->data + assignment[i] * d, x->data + i * d, d);
        }
    }

    // put the centers at their initial locations, based on clusterSize and
    // sumNewCenters
    move_centers();
}

void OriginalSpaceKmeans::changeAssignment(int xIndex, int closestCluster, int threadId) {
    unsigned short oldAssignment = assignment[xIndex];
    Kmeans::changeAssignment(xIndex, closestCluster, threadId);
    double *xp = x->data + xIndex * d;
    subVectors(sumNewCenters[threadId]->data +  oldAssignment * d, xp, d);
    addVectors(sumNewCenters[threadId]->data + closestCluster * d, xp, d);
}

double OriginalSpaceKmeans::pointPointInnerProduct(int x1, int x2) const {
    return std::inner_product(x->data + x1 * d, x->data + (x1 + 1) * d, x->data + x2 * d, 0.0);
}

double OriginalSpaceKmeans::pointCenterInnerProduct(int xndx, unsigned short cndx) const {
    return std::inner_product(x->data + xndx * d, x->data + (xndx + 1) * d, centers->data + cndx * d, 0.0);
}

double OriginalSpaceKmeans::centerCenterInnerProduct(unsigned short c1, unsigned short c2) const {
    return std::inner_product(centers->data + c1 * d, centers->data + (c1 + 1) * d, centers->data + c2 * d, 0.0);
}

