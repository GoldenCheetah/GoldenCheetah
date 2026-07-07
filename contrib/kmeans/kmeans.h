#ifndef KMEANS_H
#define KMEANS_H

/* Authors: Greg Hamerly and Jonathan Drake
 * Feedback: hamerly@cs.baylor.edu
 * See: http://cs.baylor.edu/~hamerly/software/kmeans.php
 * Copyright 2014
 *
 * Kmeans is an abstract base class for algorithms which implement Lloyd's
 * k-means algorithm. Subclasses provide functionality in the "runThread()"
 * method.
 */

#include "kmeans_dataset.h"
#include <limits>
#include <string>
#ifdef USE_THREADS
    #include <pthread.h>
#endif

class Kmeans {
    public:
        // Construct a K-means object to operate on the given dataset
        Kmeans();
        virtual ~Kmeans() { free(); }

        // This method kicks off the threads that do the clustering and run
        // until convergence (or until reaching maxIterations). It returns the
        // number of iterations performed.
        int run(int aMaxIterations = std::numeric_limits<int>::max());
    
        // Get the cluster assignment for the given point index.
        int getAssignment(int xIndex) const { return assignment[xIndex]; }

        // Initialize the algorithm at the beginning of the run(), with the
        // given data and initial assignment. The parameter initialAssignment
        // will be modified by this algorithm and will at the end contain the
        // final assignment of clusters.
        virtual void initialize(Dataset const *aX, unsigned short aK, unsigned short *initialAssignment, int aNumThreads);

        // Free all memory being used by the object.
        virtual void free();

        // This method verifies that the current assignment is correct, by
        // checking every point-center distance. For debugging.
        virtual void verifyAssignment(int iteration, int startNdx, int endNdx) const;

        // Compute the sum of squared errors for the data on the centers (not
        // designed to be fast).
        virtual double getSSE() const;

        // Get the name of this clustering algorithm (to be overridden by
        // subclasses).
        virtual std::string getName() const = 0;

        // Virtual methods for computing inner products (depending on the kernel
        // being used, e.g.). For vanilla k-means these will be the standard dot
        // product; for more exotic applications these will be other kernel
        // functions.
        virtual double pointPointInnerProduct(int x1, int x2) const = 0;
        virtual double pointCenterInnerProduct(int xndx, unsigned short cndx) const = 0;
        virtual double centerCenterInnerProduct(unsigned short c1, unsigned short c2) const = 0;

        // Use the inner products to compute squared distances between a point
        // and center.
        virtual double pointCenterDist2(int x1, unsigned short cndx) const {
            #ifdef COUNT_DISTANCES
            ++numDistances;
            #endif
            return pointPointInnerProduct(x1, x1) - 2 * pointCenterInnerProduct(x1, cndx) + centerCenterInnerProduct(cndx, cndx);
        }

        // Use the inner products to compute squared distances between two
        // centers.
        virtual double centerCenterDist2(unsigned short c1, unsigned short c2) const {
            #ifdef COUNT_DISTANCES
            ++numDistances;
            #endif
            return centerCenterInnerProduct(c1, c1) - 2 * centerCenterInnerProduct(c1, c2) + centerCenterInnerProduct(c2, c2);
        }

        #ifdef COUNT_DISTANCES
            #ifdef USE_THREADS
            // Note: numDistances is NOT thread-safe, but it is not meant to be
            // enabled in performant code.
            #error Counting distances and using multiple threads is not supported.
            #endif
        mutable long long numDistances;
        #endif

        virtual Dataset const *getCenters() const { return NULL; }

    protected:
        // The dataset to cluster.
        const Dataset *x;

        // Local copies for convenience.
        int n, k, d;

        // Pthread primitives for multithreading.
        int numThreads;
        #ifdef USE_THREADS
        pthread_barrier_t barrier;
        #endif

        // To communicate (to all threads) that we have converged.
        bool converged;

        // Keep track of how many points are in each cluster, divided over each
        // thread.
        int **clusterSize;

        // centerMovement is computed in move_centers() and used to detect
        // convergence (if max(centerMovement) == 0.0) and update point-center
        // distance bounds (in subclasses that use them).
        double *centerMovement;

        // For each point in x, keep which cluster it is assigned to. By using a
        // short, we assume a limited number of clusters (fewer than 2^16).
        unsigned short *assignment;


        // This is where each thread does its work.
        virtual int runThread(int threadId, int maxIterations) = 0;

        // Static entry method for pthread_create(). 
        static void *runner(void *args);

        // Assign point at xIndex to cluster newCluster, working within thread threadId.
        virtual void changeAssignment(int xIndex, int newCluster, int threadId);

        // Over what range in [0, n) does this thread have ownership of the
        // points? end() returns one past the last owned point.
        int start(int threadId) const { return n * threadId / numThreads; }
        int end(int threadId) const { return start(threadId + 1); }
        int whichThread(int index) const { return index * numThreads / n; }

        // Convenience method for causing all threads to synchronize.
        void synchronizeAllThreads() {
            #ifdef USE_THREADS
            pthread_barrier_wait(&barrier);
            #endif
        }
};

#endif

