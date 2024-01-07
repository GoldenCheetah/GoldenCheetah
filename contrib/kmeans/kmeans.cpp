/* Authors: Greg Hamerly and Jonathan Drake
 * Feedback: hamerly@cs.baylor.edu
 * See: http://cs.baylor.edu/~hamerly/software/kmeans.php
 * Copyright 2014
 */

#include "kmeans.h"
#include "kmeans_general_functions.h"
#include <cassert>
#include <cmath>

Kmeans::Kmeans() : x(NULL), n(0), k(0), d(0), numThreads(0), converged(false),
    clusterSize(NULL), centerMovement(NULL), assignment(NULL) {
    #ifdef COUNT_DISTANCES
    numDistances = 0;
    #endif
}

void Kmeans::free() {
    delete [] centerMovement;
    for (int t = 0; t < numThreads; ++t) {
        delete [] clusterSize[t];
    }
    delete [] clusterSize;
    centerMovement = NULL;
    clusterSize = NULL;
    assignment = NULL;
    n = k = d = numThreads = 0;
}



void Kmeans::initialize(Dataset const *aX, unsigned short aK, unsigned short *initialAssignment, int aNumThreads) {
    free();

    converged = false;
    x = aX;
    n = x->n;
    d = x->d;
    k = aK;

    #ifdef USE_THREADS
    numThreads = aNumThreads;
    pthread_barrier_init(&barrier, NULL, numThreads);
    #else
    numThreads = 1;
    #endif

    assignment = initialAssignment;
    centerMovement = new double[k];
    clusterSize = new int *[numThreads];
    for (int t = 0; t < numThreads; ++t) {
        clusterSize[t] = new int[k];
        std::fill(clusterSize[t], clusterSize[t] + k, 0);
        for (int i = start(t); i < end(t); ++i) {
            assert(assignment[i] < k);
            ++clusterSize[t][assignment[i]];
        }
    }


    #ifdef COUNT_DISTANCES
    numDistances = 0;
    #endif
}

void Kmeans::changeAssignment(int xIndex, int closestCluster, int threadId) {
    --clusterSize[threadId][assignment[xIndex]];
    ++clusterSize[threadId][closestCluster];
    assignment[xIndex] = closestCluster;
}

#ifdef USE_THREADS
struct ThreadInfo {
    public:
        ThreadInfo() : km(NULL), threadId(0), pthread_id(0) {}
        Kmeans *km;
        int threadId;
        pthread_t pthread_id;
        int numIterations;
        int maxIterations;
};
#endif

void *Kmeans::runner(void *args) {
    #ifdef USE_THREADS
    ThreadInfo *ti = (ThreadInfo *)args;
    ti->numIterations = ti->km->runThread(ti->threadId, ti->maxIterations);
    #endif
    return NULL;
}

int Kmeans::run(int maxIterations) {
    int iterations = 0;
    #ifdef USE_THREADS
    {
        ThreadInfo *info = new ThreadInfo[numThreads];
        for (int t = 0; t < numThreads; ++t) {
            info[t].km = this;
            info[t].threadId = t;
            info[t].maxIterations = maxIterations;
            pthread_create(&info[t].pthread_id, NULL, Kmeans::runner, &info[t]);
        }
        // wait for everything to finish...
        for (int t = 0; t < numThreads; ++t) {
            pthread_join(info[t].pthread_id, NULL);
        }
        iterations = info[0].numIterations;
        delete [] info;
    }
    #else
    {
        iterations = runThread(0, maxIterations);
    }
    #endif

    return iterations;
}

double Kmeans::getSSE() const {
    double sse = 0.0;
    for (int i = 0; i < n; ++i) {
        sse += pointCenterDist2(i, assignment[i]);
    }
    return sse;
}

void Kmeans::verifyAssignment(int iteration, int startNdx, int endNdx) const {
    #ifdef VERIFY_ASSIGNMENTS
    for (int i = startNdx; i < endNdx; ++i) {
        // keep track of the squared distance and identity of the closest-seen
        // cluster (so far)
        int closest = assignment[i];
        double closest_dist2 = pointCenterDist2(i, closest);
        double original_closest_dist2 = closest_dist2;
        // look at all centers
        for (int j = 0; j < k; ++j) {
            if (j == closest) {
                continue;
            }
            double d2 = pointCenterDist2(i, j);

            // determine if we found a closer center
            if (d2 < closest_dist2) {
                closest = j;
                closest_dist2 = d2;
            }
        }

        // if we have found a discrepancy, then print out information and crash
        // the program
        if (closest != assignment[i]) {
            std::cerr << "assignment error:" << std::endl;
            std::cerr << "iteration             = " << iteration              << std::endl;
            std::cerr << "point index           = " << i                      << std::endl;
            std::cerr << "closest center        = " << closest                << std::endl;
            std::cerr << "closest center dist2  = " << closest_dist2          << std::endl;
            std::cerr << "assigned center       = " << assignment[i]          << std::endl;
            std::cerr << "assigned center dist2 = " << original_closest_dist2 << std::endl;
            assert(false);
        }
    }
    #endif
}

