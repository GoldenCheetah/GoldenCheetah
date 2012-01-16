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
#include "RunningAverage.h"

RunningAverage::RunningAverage(int size): _size(size), _values(size, 0.0), _sum(0.0), _index(0) {}

double RunningAverage::average() const {
    return _sum / _size;
}

void RunningAverage::add(double value) {
    _sum -= _values[_index];
    _values[_index] = value;
    _sum += value;
    _index = (_index + 1) % _size;
}

void RunningAverage::reset() {
    _sum = 0.0;
    _values.fill(0.0);
}

