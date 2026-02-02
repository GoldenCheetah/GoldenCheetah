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
(const QList<CalendarEntry> &entries) const
{
    struct TimedEntry {
        int index;
        qint64 startSecs;
        qint64 endSecs;
    };
    QList<TimedEntry> normalized;

    for (int i = 0; i < entries.size(); ++i) {
        const CalendarEntry &entry = entries[i];
        qint64 startSecs = entry.start.hour() * 3600 + entry.start.minute() * 60 + entry.start.second();
        QTime endTime = entry.start.addSecs(entry.durationSecs);
        qint64 endSecs = endTime.hour() * 3600 + endTime.minute() * 60 + endTime.second();
        if (endTime < entry.start) {
            endSecs += 24 * 3600;
        }
        normalized.append({ i, startSecs, endSecs });
    }

    std::sort(normalized.begin(), normalized.end(), [](const TimedEntry &a, const TimedEntry &b) {
        return a.startSecs < b.startSecs;
    });

    QList<QList<int>> clusters;
    QList<int> currentCluster;
    qint64 clusterEnd = -1;

    for (const TimedEntry &t : normalized) {
        if (currentCluster.isEmpty()) {
            currentCluster.append(t.index);
            clusterEnd = t.endSecs;
        } else {
            if (t.startSecs < clusterEnd) {
                currentCluster.append(t.index);
                if (t.endSecs > clusterEnd) {
                    clusterEnd = t.endSecs;
                }
            } else {
                clusters.append(currentCluster);
                currentCluster = { t.index };
                clusterEnd = t.endSecs;
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
(const QList<int> &cluster, const QList<CalendarEntry> &entries) const
{
    QList<qint64> columnEndTimes;
    QMap<int, int> entryColumns;

    for (int entryIdx : cluster) {
        const CalendarEntry &entry = entries[entryIdx];
        qint64 startSecs = entry.start.hour() * 3600 + entry.start.minute() * 60 + entry.start.second();
        QTime endTime = entry.start.addSecs(entry.durationSecs);
        qint64 endSecs = endTime.hour() * 3600 + endTime.minute() * 60 + endTime.second();
        if (endTime < entry.start) {
            endSecs += 24 * 3600;
        }
        bool found = false;
        for (int colIdx = 0; colIdx < columnEndTimes.size(); ++colIdx) {
            if (columnEndTimes[colIdx] <= startSecs) {
                columnEndTimes[colIdx] = endSecs;
                entryColumns[entryIdx] = colIdx;
                found = true;
                break;
            }
        }
        if (! found) {
            columnEndTimes << endSecs;
            entryColumns[entryIdx] = columnEndTimes.size() - 1;
        }
    }

    QList<CalendarEntryLayout> result;
    for (int entryIdx : cluster) {
        result << CalendarEntryLayout({
            entryIdx,
            entryColumns[entryIdx],
            static_cast<int>(columnEndTimes.size())
        });
    }

    return result;
}
