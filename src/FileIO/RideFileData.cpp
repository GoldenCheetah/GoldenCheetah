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

#include "RideFileData.h"

RideFileData::RideFileData(const RideFile *source)
    : samples_(0)
{
    if (!source) return;

    const QVector<RideFilePoint*> &points = source->dataPoints();
    samples_ = points.size();

    for (auto &col : columns_) col.resize(samples_);
    intervalColumn_.resize(samples_);

    for (int i = 0; i < samples_; ++i) {
        const RideFilePoint *p = points[i];
        if (!p) continue;

        for (std::size_t s = 0; s < kColumnCount; ++s) {
            const auto series = static_cast<RideFile::SeriesType>(s);
            columns_[s][i] = p->value(series);
        }
        intervalColumn_[i] = p->interval;
    }
}

const QVector<double> &
RideFileData::series(RideFile::SeriesType s) const
{
    const auto idx = static_cast<std::size_t>(s);
    Q_ASSERT(idx < kColumnCount);
    return columns_[idx];
}
