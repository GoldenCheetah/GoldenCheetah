/*
 * Copyright (c) 2025 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include <CalendarData.h>


CalendarEntryLayouter::CalendarEntryLayouter
()
{
}


QList<CalendarEntryLayout>
CalendarEntryLayouter::layout
(const QList<CalendarEntry> &entries)
{
    QList<QList<int>> clusters = groupOverlapping(entries);
    QList<CalendarEntryLayout> layouted;
    for (const auto &cluster : clusters) {
        layouted += assignColumns(cluster, entries);
    }
    return layouted;
}


QList<QList<int>>
CalendarEntryLayouter::groupOverlapping
(const QList<CalendarEntry> &entries)
{
    QList<QList<int>> clusters;
    QList<int> currentCluster;
    QTime clusterEnd;

    for (int entryIdx = 0; entryIdx < entries.count(); ++entryIdx) {
        const CalendarEntry &entry = entries[entryIdx];
        QTime end = entry.start.addSecs(entry.durationSecs);

        if (currentCluster.isEmpty()) {
            currentCluster.append(entryIdx);
            clusterEnd = end;
        } else {
            if (entry.start < clusterEnd) {
                currentCluster.append(entryIdx);
                if (end > clusterEnd) {
                    clusterEnd = end;
                }
            } else {
                clusters.append(currentCluster);
                currentCluster = { entryIdx };
                clusterEnd = end;
            }
        }
    }
    if (! currentCluster.isEmpty()) {
        clusters.append(currentCluster);
    }

    return clusters;
}


QList<CalendarEntryLayout>
CalendarEntryLayouter::assignColumns
(const QList<int> &cluster, const QList<CalendarEntry> &entries)
{
    QList<QTime> columns;
    QMap<int, int> entryColumns;

    for (int entryIdx : cluster) {
        const CalendarEntry &entry = entries[entryIdx];
        QTime start = entry.start;
        QTime end = start.addSecs(entry.durationSecs);
        bool found = false;
        for (int colIdx = 0; colIdx < columns.count(); ++colIdx) {
            if (columns[colIdx] <= start) {
                columns[colIdx] = end;
                entryColumns[entryIdx] = colIdx;
                found = true;
                break;
            }
        }
        if (! found) {
            columns << end;
            entryColumns[entryIdx] = columns.count() - 1;
        }
    }

    QList<CalendarEntryLayout> result;
    for (int entryIdx : cluster) {
        result << CalendarEntryLayout({
            entryIdx,
            entryColumns[entryIdx],
            static_cast<int>(columns.count())
        });
    }
    return result;
}
