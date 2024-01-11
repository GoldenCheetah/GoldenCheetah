#ifndef TRIANGLE_INEQUALITY_BASE_KMEANS_H
#define TRIANGLE_INEQUALITY_BASE_KMEANS_H

/* Authors: Greg Hamerly and Jonathan Drake
 * Feedback: hamerly@cs.baylor.edu
 * See: http://cs.baylor.edu/~hamerly/software/kmeans.php
 * Copyright 2014
 *
 * This class is an abstract base class for several other algorithms that use
 * upper & lower bounds to avoid distance calculations in k-means.
 */

#include "original_space_kmeans.h"

class TriangleInequalityBaseKmeans : public OriginalSpaceKmeans {
    public:
        TriangleInequalityBaseKmeans() : numLowerBounds(0), s(NULL), upper(NULL), lower(NULL) {}
        virtual ~TriangleInequalityBaseKmeans() { free(); }

        virtual void initialize(Dataset const *aX, unsigned short aK, unsigned short *initialAssignment, int aNumThreads);
        virtual void free();

    protected:
        void update_s(int threadId);

        // The number of lower bounds being used by this algorithm.
        int numLowerBounds;

        // Half the distance between each center and its closest other center.
        double *s;

        // One upper bound for each point on the distance between that point and
        // its assigned (closest) center.
        double *upper;

        // Lower bound(s) for each point on the distance between that point and
        // the centers being tracked for lower bounds, which may be 1 to k.
        // Actual size is n * numLowerBounds.
        double *lower;
};

#endif

