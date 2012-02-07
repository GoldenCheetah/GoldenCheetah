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

RunningAverage::RunningAverage() {}

double RunningAverage::average() const {
    if (_values.count() == 0)
        return 0.0;
    double sum = 0.0;
    QListIterator<Item> it(_values);
    while(it.hasNext())
        sum += it.next().value;

    return sum / _values.count();
}

void RunningAverage::add(double value) {
    QTime now = QTime::currentTime();
    QTime oneSecondAgo = now.addSecs(-1);
    QMutableListIterator<Item> it(_values);
    while(it.hasNext() && it.next().time < oneSecondAgo)
        it.remove();

    Item newItem(now, value);
    _values.append(Item(now, value));
}

void RunningAverage::reset() {
    _values.clear();
}

