/*
 * Copyright (c) 2012 Ilja Booij (ibooij@gmail.com)
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
#ifndef RUNNINGGAVERAGE_H
#define RUNNINGAVERAGE_H

#include <QVector>

/*!
 * Simple Running Average class. Takes the average of a given number of
 * values, which are added using the \c add(double) method.
 */
class RunningAverage {
public:
    /*!
      * Creates a new running average wich takes the average of the last
      * \a size values.
      */
    RunningAverage(int size);

    /*!
      * Get the current value of the Running Average
      */
    double average() const;

    /*!
     * Add a new \a value to the \c RunningAverage.
     */
    void add(double value);

    /*!
     * Reset all values to zero
     */
    void reset();

private:
    const int _size;
    QVector<double> _values;
    double _sum;
    int _index;
};

#endif // RUNNINGAVERAGE_H
