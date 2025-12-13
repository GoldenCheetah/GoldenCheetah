/*
 * Copyright (c) 2025 Magnus Gille (mgille@gmail.com)
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

#include <iterator>
#include <QPointF>
#include <QXYSeries>

// Minimal iterator to traverse QXYSeries points without copying
// Required to use std::lower_bound on QXYSeries data in-place
class SeriesIterator : public std::iterator<std::random_access_iterator_tag, QPointF> {
public:
    SeriesIterator(QXYSeries *series, int index) : m_series(series), m_index(index) {}

    QPointF operator*() const { return m_series->at(m_index); }
    SeriesIterator& operator++() { m_index++; return *this; }
    SeriesIterator operator++(int) { SeriesIterator tmp = *this; m_index++; return tmp; }
    SeriesIterator& operator--() { m_index--; return *this; }
    SeriesIterator operator--(int) { SeriesIterator tmp = *this; m_index--; return tmp; }
    SeriesIterator& operator+=(int n) { m_index += n; return *this; }
    SeriesIterator operator+(int n) const { return SeriesIterator(m_series, m_index + n); }
    SeriesIterator& operator-=(int n) { m_index -= n; return *this; }
    SeriesIterator operator-(int n) const { return SeriesIterator(m_series, m_index - n); }
    difference_type operator-(const SeriesIterator& other) const { return m_index - other.m_index; }
    bool operator==(const SeriesIterator& other) const { return m_index == other.m_index; }
    bool operator!=(const SeriesIterator& other) const { return m_index != other.m_index; }
    bool operator<(const SeriesIterator& other) const { return m_index < other.m_index; }

private:
    QXYSeries *m_series;
    int m_index;
};

