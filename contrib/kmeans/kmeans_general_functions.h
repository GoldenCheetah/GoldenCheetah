#ifndef GENERAL_KMEANS_FUNCTIONS_H
#define GENERAL_KMEANS_FUNCTIONS_H

/* Authors: Greg Hamerly and Jonathan Drake
 * Feedback: hamerly@cs.baylor.edu
 * See: http://cs.baylor.edu/~hamerly/software/kmeans.php
 * Copyright 2014
 *
 * Generally useful functions.
 */


#include <iostream>
#include <string>
#include "kmeans_dataset.h"

/* Add together two vectors, and put the result in the first argument.
 * Calculates a = a + b
 *
 * Parameters:
 *  a -- vector to add, and the result of the operation
 *  b -- vector to add to a
 *  d -- the dimension
 * Return value: none
 */
void addVectors(double *a, double const *b, int d);

/* Subtract two vectors, and put the result in the first argument. Calculates 
 * a = a - b
 *
 * Parameters:
 *  a -- vector to subtract from, and the result of the operation
 *  b -- vector to subtract
 *  d -- the dimension
 * Return value: none
 */
void subVectors(double *a, double const *b, int d);

/* Initialize the centers randomly. Choose random records from x as the initial
 * values for the centers. Assumes that c uses the sumDataSquared field.
 *
 * Parameters:
 *  x -- records that are being clustered (n * d)
 *  c -- centers to be initialized. Should be pre-allocated with the number of
 *       centers desired, and dimension.
 * Return value: none
 */
Dataset *init_centers(Dataset const &x, unsigned short k);

/* Initialize the centers randomly using K-means++.
 *
 * Parameters:
 *  x -- records that are being clustered (n * d)
 *  c -- centers to be initialized. Should be pre-allocated with the number of
 *       centers desired, and dimension.
 * Return value: none
 */
Dataset *init_centers_kmeanspp(Dataset const &x, unsigned short k);
Dataset *init_centers_kmeanspp_v2(Dataset const &x, unsigned short k);

/* Print an array (templated). Convenience function.
 *
 * Parameters:
 *  arr -- the array to print
 *  length -- the length of the array
 *  separator -- the string to put between each pair of printed elements
 * Return value: none
 */
template <class T>
void printArray(T const *arr, int length, std::string separator) {
    for (int i = 0; i < length; ++i) {
        if (i > 0) {
            std::cout << separator;
        }
        std::cout << arr[i];
    }
}

void centerDataset(Dataset *x);

void assign(Dataset const &x, Dataset const &c, unsigned short *assignment);

#endif
