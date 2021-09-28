/* Authors: Greg Hamerly and Jonathan Drake
 * Feedback: hamerly@cs.baylor.edu
 * See: http://cs.baylor.edu/~hamerly/software/kmeans.php
 * Copyright 2014
 */

#include "kmeans_dataset.h"
#include "kmeans.h"
#include "kmeans_general_functions.h"
#include <cassert>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstring>
#include <cstdio>
#include <unistd.h>

void addVectors(double *a, double const *b, int d) {
    double const *end = a + d;
    while (a < end) {
        *(a++) += *(b++);
    }
}

void subVectors(double *a, double const *b, int d) {
    double const *end = a + d;
    while (a < end) {
        *(a++) -= *(b++);
    }
}

double distance2silent(double const *a, double const *b, int d) {
    double d2 = 0.0, diff;
    double const *end = a + d; // one past the last valid entry in a
    while (a < end) {
        diff = *(a++) - *(b++);
        d2 += diff * diff;
    }
    return d2;
}


void centerDataset(Dataset *x) {
    double *xCentroid = new double[x->d];

    for (int d = 0; d < x->d; ++d) {
        xCentroid[d] = 0.0;
    }

    for (int i = 0; i < x->n; ++i) {
        addVectors(xCentroid, x->data + i * x->d, x->d);
    }

    // compute average (divide by n)
    for (int d = 0; d < x->d; ++d) {
        xCentroid[d] /= x->n;
    }
    
    // re-center the dataset
    const double *xEnd = x->data + x->n * x->d;
    for (double *xp = x->data; xp != xEnd; xp += x->d) {
        subVectors(xp, xCentroid, x->d);
    }

    delete [] xCentroid;
}

Dataset *init_centers(Dataset const &x, unsigned short k) {
    int *chosen_pts = new int[k];
    Dataset *c = new Dataset(k, x.d);
    for (int i = 0; i < k; ++i) {
        bool acceptable = true;
        do {
            acceptable = true;
            chosen_pts[i] = rand() % x.n;
            for (int j = 0; j < i; ++j) {
                if (chosen_pts[i] == chosen_pts[j]) {
                    acceptable = false;
                    break;
                }
            }
        } while (! acceptable);
        double *cdp = c->data + i * x.d;
        memcpy(cdp, x.data + chosen_pts[i] * x.d, sizeof(double) * x.d);
        if (c->sumDataSquared) {
            c->sumDataSquared[i] = std::inner_product(cdp, cdp + x.d, cdp, 0.0);
        }
    }

    delete [] chosen_pts;

    return c;
}


Dataset *init_centers_kmeanspp(Dataset const &x, unsigned short k) {
    int *chosen_pts = new int[k];
    std::pair<double, int> *dist2 = new std::pair<double, int>[x.n];
    double *distribution = new double[x.n];

    // initialize dist2
    for (int i = 0; i < x.n; ++i) {
        dist2[i].first = std::numeric_limits<double>::max();
        dist2[i].second = i;
    }

    // choose the first point randomly
    int ndx = 1;
    chosen_pts[ndx - 1] = rand() % x.n;

    while (ndx < k) {
        double sum_distribution = 0.0;
        // look for the point that is furthest from any center
        for (int i = 0; i < x.n; ++i) {
            int example = dist2[i].second;
            double d2 = 0.0, diff;
            for (int j = 0; j < x.d; ++j) {
                diff = x(example,j) - x(chosen_pts[ndx - 1],j);
                d2 += diff * diff;
            }
            if (d2 < dist2[i].first) {
                dist2[i].first = d2;
            }

            sum_distribution += dist2[i].first;
        }

        // sort the examples by their distance from centers
        sort(dist2, dist2 + x.n);

        // turn distribution into a CDF
        distribution[0] = dist2[0].first / sum_distribution;
        for (int i = 1; i < x.n; ++i) {
            distribution[i] = distribution[i - 1] + dist2[i].first / sum_distribution;
        }

        // choose a random interval according to the new distribution
        double r = (double)rand() / (double)RAND_MAX;
        double *new_center_ptr = std::lower_bound(distribution, distribution + x.n, r);
        int distribution_ndx = (int)(new_center_ptr - distribution);
        chosen_pts[ndx] = dist2[distribution_ndx].second;
        /*
        cout << "chose " << distribution_ndx << " which is actually " 
             << chosen_pts[ndx] << " with distance " 
             << dist2[distribution_ndx].first << std::endl;
             */

        ++ndx;
    }

    Dataset *c = new Dataset(k, x.d);

    for (int i = 0; i < k; ++i) {
        double *cdp = c->data + i * x.d;
        memcpy(cdp, x.data + chosen_pts[i] * x.d, sizeof(double) * x.d);
        if (c->sumDataSquared) {
            c->sumDataSquared[i] = std::inner_product(cdp, cdp + x.d, cdp, 0.0);
        }
    }

    delete [] chosen_pts;
    delete [] dist2;
    delete [] distribution;

    return c;
}


Dataset *init_centers_kmeanspp_v2(Dataset const &x, unsigned short k) {
    int *chosen_pts = new int[k];
    std::pair<double, int> *dist2 = new std::pair<double, int>[x.n];

    // initialize dist2
    for (int i = 0; i < x.n; ++i) {
        dist2[i].first = std::numeric_limits<double>::max();
        dist2[i].second = i;
    }

    // choose the first point randomly
    int ndx = 1;
    chosen_pts[ndx - 1] = rand() % x.n;

    while (ndx < k) {
        double sum_distribution = 0.0;
        // look for the point that is furthest from any center
        double max_dist = 0.0;
        for (int i = 0; i < x.n; ++i) {
            int example = dist2[i].second;
            double d2 = 0.0, diff;
            for (int j = 0; j < x.d; ++j) {
                diff = x(example,j) - x(chosen_pts[ndx - 1],j);
                d2 += diff * diff;
            }
            if (d2 < dist2[i].first) {
                dist2[i].first = d2;
            }

            if (dist2[i].first > max_dist) {
                max_dist = dist2[i].first;
            }

            sum_distribution += dist2[i].first;
        }

        bool unique = true;

        do {
            // choose a random interval according to the new distribution
            double r = sum_distribution * (double)rand() / (double)RAND_MAX;
            double sum_cdf = dist2[0].first;
            int cdf_ndx = 0;
            while (sum_cdf < r) {
                sum_cdf += dist2[++cdf_ndx].first;
            }
            chosen_pts[ndx] = cdf_ndx;

            for (int i = 0; i < ndx; ++i) {
                unique = unique && (chosen_pts[ndx] != chosen_pts[i]);
            }
        } while (! unique);

        ++ndx;
    }

    Dataset *c = new Dataset(k, x.d);
    for (int i = 0; i < c->n; ++i) {
        double *cdp = c->data + i * x.d;
        memcpy(cdp, x.data + chosen_pts[i] * x.d, sizeof(double) * x.d);
        if (c->sumDataSquared) {
            c->sumDataSquared[i] = std::inner_product(cdp, cdp + x.d, cdp, 0.0);
        }
    }

    delete [] chosen_pts;
    delete [] dist2;

    return c;
}


/**
 * in MB
 */
double getMemoryUsage() {
    char buf[30];
    snprintf(buf, 30, "/proc/%u/statm", (unsigned)getpid());
    FILE* pf = fopen(buf, "r");
    unsigned int totalProgramSizeInPages = 0;
    unsigned int residentSetSizeInPages = 0;
    if (pf) {
        int numScanned = fscanf(pf, "%u %u" /* %u %u %u %u %u"*/, &totalProgramSizeInPages, &residentSetSizeInPages);
        if (numScanned != 2) {
            return 0.0;
        }
    }
    
    fclose(pf);
    pf = NULL;
    
    double sizeInKilobytes = residentSetSizeInPages * 4.0; // assumes 4096 byte page
    // getconf PAGESIZE
    
    return sizeInKilobytes;
}


void assign(Dataset const &x, Dataset const &c, unsigned short *assignment) {
    for (int i = 0; i < x.n; ++i) {
        double shortestDist2 = std::numeric_limits<double>::max();
        int closest = 0;
        for (int j = 0; j < c.n; ++j) {
            double d2 = 0.0, *a = x.data + i * x.d, *b = c.data + j * x.d;
            for (; a != x.data + (i + 1) * x.d; ++a, ++b) {
                d2 += (*a - *b) * (*a - *b);
            }
            if (d2 < shortestDist2) {
                shortestDist2 = d2;
                closest = j;
            }
        }
        assignment[i] = closest;
    }
}

rusage get_time() {
    rusage now;
    getrusage(RUSAGE_SELF, &now);
    return now;
}


double get_wall_time(){
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

int timeval_subtract(timeval *result, timeval *x, timeval *y) {
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.  tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result
     * is negative. */
    return x->tv_sec < y->tv_sec;
}

double elapsed_time(rusage *start) {
    rusage now;
    timeval diff;
    getrusage(RUSAGE_SELF, &now);
    timeval_subtract(&diff, &now.ru_utime, &start->ru_utime);

    return (double)diff.tv_sec + (double)diff.tv_usec / 1e6;
}
