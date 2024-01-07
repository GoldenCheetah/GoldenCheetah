#ifndef ORIGINAL_SPACE_KMEANS_H
#define ORIGINAL_SPACE_KMEANS_H

/* Authors: Greg Hamerly and Jonathan Drake
 * Feedback: hamerly@cs.baylor.edu
 * See: http://cs.baylor.edu/~hamerly/software/kmeans.php
 * Copyright 2014
 *
 * OriginalSpaceKmeans is a base class for other algorithms that operate in the
 * same space as the data being clustered (as opposed to kernelized k-means
 * algorithms, which operate in kernel space).
 */

#include "kmeans.h"

/* Cluster with the cluster centers living in the original space (with the
 * data). This is as opposed to a kernelized version of k-means, where the
 * center points might not be explicitly represented. This is also an abstract
 * class.
 */
class OriginalSpaceKmeans : public Kmeans {
    public:
        OriginalSpaceKmeans();
        virtual ~OriginalSpaceKmeans() { free(); }
        virtual void free(); 
        virtual void initialize(Dataset const *aX, unsigned short aK, unsigned short *initialAssignment, int aNumThreads);

        virtual double pointPointInnerProduct(int x1ndx, int x2ndx) const;
        virtual double pointCenterInnerProduct(int xndx, unsigned short cndx) const;
        virtual double centerCenterInnerProduct(unsigned short c1ndx, unsigned short c2ndx) const;

        virtual Dataset const *getCenters() const { return centers; }

    protected:
        // Move the centers to the average of their current assigned points,
        // compute the distance moved by each center, and return the index of
        // the furthest-moving center.
        int move_centers();

        virtual void changeAssignment(int xIndex, int closestCluster, int threadId);

        // The set of centers we are operating on.
        Dataset *centers;
    
        // sumNewCenters and centerCount provide sufficient statistics to
        // quickly calculate the changing locations of the centers. Whenever a
        // point changes cluster membership, we subtract (add) it from (to) the
        // row in sumNewCenters associated with its old (new) cluster. We also
        // decrement (increment) centerCount for the old (new) cluster.
        Dataset **sumNewCenters;

};

#endif
