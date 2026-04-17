/*
 * Copyright (c) 2026 GoldenCheetah contributors
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
 */

#ifndef _RideFileData_h
#define _RideFileData_h

#include "RideFile.h"

#include <QVector>
#include <array>

// Columnar (struct-of-arrays) view of a RideFile's samples.
//
// Phase 1 scaffolding for the RideFile refactor: lets metric code scan a
// single series as a contiguous double array instead of walking
// QVector<RideFilePoint*> and dereferencing a pointer per sample.
//
// Phase 1 is read-only and built explicitly by the caller. Later phases
// will add lazy caching on RideFile and migrate hot metric loops.
class RideFileData
{
public:
    explicit RideFileData(const RideFile *source);

    int samples() const { return samples_; }

    // Contiguous column for a single series. Guaranteed length == samples().
    const QVector<double> &series(RideFile::SeriesType s) const;

    // Interval column. Stored separately because RideFilePoint::interval is int.
    const QVector<int> &intervals() const { return intervalColumn_; }

private:
    static constexpr std::size_t kColumnCount =
        static_cast<std::size_t>(RideFile::none);

    int samples_;
    std::array<QVector<double>, kColumnCount> columns_;
    QVector<int> intervalColumn_;
};

#endif // _RideFileData_h
