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

#include "hamerly_kmeans.h"
#include "kmeans_dataset.h"
#include <QVector>
#include <QList>

#ifndef _GC_FastKmeans_h
#define _GC_FastKmeans_h 1

class FastKmeans
{
    public:

        // since we are a wrapper around algorithms we need
        // to initialise and cleanup subordinates, especially
        // since they typically use non-QT containers
        FastKmeans();
        ~FastKmeans();

        // all dimensions are resized to the largest
        // and filled with zeroes, but really the caller
        // should make sure they match
        void addDimension(QVector<double> &data);

        // find centers and assignments for k clusters
        bool run(int k);

        // get centers (k x dimensions)
        QVector<double> centers();

        // get assignments - n indexes in datafilter order (use in overview tables etc)
        QVector<double> assignments();

        int length() const { return length_; } ;   // number of points
        int dim() const { return dimension.count(); }      // number of dimensions to a point
        int k() const { return k_; }        // number of clusters used

    private:

        HamerlyKmeans *kmeans; // the algorithm we use
        Dataset *data;
        unsigned short *assignments_; // updated with the cluster assignments
        Dataset *centers_;

        QList<QVector<double> > dimension;

        int length_; // updated as we add dimensions, but really should be the same
        int k_;       // updated when we run
};

#endif
