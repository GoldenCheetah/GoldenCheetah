/*
 * Copyright (c) 2021 Mark Liversedge (liversedge@gmail.com)
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

#include "FastKmeans.h"

#include "kmeans_general_functions.h"

FastKmeans::FastKmeans() : kmeans(NULL), data(NULL), assignments_(NULL), centers_(NULL), length_(-1), k_(-1)  {}
FastKmeans::~FastKmeans()
{
    if (data) delete data;
    if (kmeans) delete kmeans;
    if (centers_) delete centers_;
    if (assignments_) delete [] assignments_;
}

// all dimensions are resized to the largest
// and filled with zeroes, but really the caller
// should make sure they match
void
FastKmeans::addDimension(QVector<double> &data)
{
    // take a copy, and init for first dimension
    dimension.append(data);
    int index = dimension.count() - 1;

    // first init length, no resizing needed
    if (dimension.count() == 1) length_=data.length();
    else {
        if (data.length() > length_) {

            // if longer, we need to resize everyone else
            for(int i=0; i<index; i++) {
                dimension[i].resize(data.length());
                for(int j=length_; j<data.length(); j++)
                    dimension[i][j]=0;
            }
            length_ = data.length();

        } else if (data.length() < length_) {

            // if shorter we need to resize ours
            dimension[index].resize(length_);
            for(int j=length_; j<data.length(); j++)
                dimension[index][j]=0;
        }
    }
}

// find centers and assignments for k clusters
bool
FastKmeans::run(int k)
{
    // no data, or dimensions
    if (k <2 || length_ <= 0 || dimension.count() <= 0) return false;

    // set number if clusters we looked for
    k_ = k;

    // if we have old data, delete it
    if (data) delete data;
    if (kmeans) delete kmeans;
    if (centers_) delete centers_;
    if (assignments_) delete [] assignments_;

    // lets get a new one
    kmeans = new HamerlyKmeans();
    data = new Dataset(length(), dim());

    // now fill the data set with our data
    int index=0;
    for(int i=0; i<length(); i++)
        for(int j=0; j<dim(); j++)
            data->data[index++] = dimension[j][i];

    // initialise centers
    centers_ = init_centers_kmeanspp_v2(*data, k_);

    // initialise assignments
    assignments_ = new unsigned short[length()];

    // setup
    kmeans_assign(*data, *centers_, assignments_);
    kmeans->initialize(data, k, assignments_, 1);

    // run the algorithm, max out at 10,000 iterations
    // it returns true or false if it succeeded
    return kmeans->run(10000);
}

// get centers (k x dimensions)
QVector<double>
FastKmeans::centers()
{
    QVector<double> returning;

    if (kmeans == NULL) return returning;

    Dataset const *finalcenters = kmeans->getCenters();

    // lets reorganise them to d1,d1,d1,d2,d2,d2,d2,d3,d3,d3
    // from d1,d2,d3,d1,d2,d3,d1,d2,d3
    for(int d=0; d<dim(); d++)
       for(int n=0; n<k(); n++)
            returning << finalcenters->data[(n * dim()) + d];

    return returning;
}

// get assignments - n indexes
QVector<double>
FastKmeans::assignments()
{
    QVector<double> returning;

    if (kmeans == NULL) return returning;

    Dataset const *finalcenters = kmeans->getCenters();
    kmeans_assign(*data, *finalcenters, assignments_);

    // lets reorganise and convert to doubles (datafilter likes these)
    for (int i = 0; i < data->n; ++i) returning << assignments_[i];

    return returning;
}

