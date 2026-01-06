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

#ifndef CALENDARDATA_H
#define CALENDARDATA_H

#include <QMetaType>
#include <QDate>
#include <QTime>
#include <QList>
#include <QHash>
#include <QMap>
#include <QColor>
#include <utility>

#define ENTRY_TYPE_ACTIVITY 0
#define ENTRY_TYPE_PLANNED_ACTIVITY 1
#define ENTRY_TYPE_EVENT 10
#define ENTRY_TYPE_PHASE 11
#define ENTRY_TYPE_OTHER 99


struct AgendaEntry {
    QString primary;
    QStringList secondaryValues;
    QStringList secondaryLabels;
    QString tertiary;
    QString iconFile;
    QColor color;
    QString reference;
    QTime start;
    int type = 0;
    bool hasTrainMode = false;
    QDate spanStart = QDate();
    QDate spanEnd = QDate();
};


struct CalendarEntry {
    QString primary;
    QString secondary;
    QString secondaryMetric;
    QString tertiary;
    QString iconFile;
    QColor color;
    QString reference;
    QTime start;
    int durationSecs = 0;
    int type = 0;
    bool isRelocatable = false;
    bool hasTrainMode = false;
    bool dirty = false;
    QDate spanStart = QDate();
    QDate spanEnd = QDate();

    QString linkedReference = QString();
    QString linkedPrimary = QString();
    QDateTime linkedStartDT = QDateTime();
};

struct CalendarEntryLayout {
    int entryIdx;
    int columnIndex;
    int columnCount;
};

enum class DayDimLevel {
    None,
    Full,
    Partial
};

struct CalendarDay {
    QDate date;
    DayDimLevel isDimmed;
    QList<CalendarEntry> entries = QList<CalendarEntry>();
    QList<CalendarEntry> headlineEntries = QList<CalendarEntry>();
};

struct CalendarSummary {
    QList<std::pair<QString, QString>> keyValues;
};

Q_DECLARE_METATYPE(AgendaEntry)
Q_DECLARE_METATYPE(CalendarEntry)
Q_DECLARE_METATYPE(CalendarEntryLayout)
Q_DECLARE_METATYPE(CalendarDay)
Q_DECLARE_METATYPE(CalendarSummary)


class CalendarEntryLayouter {
public:
    explicit CalendarEntryLayouter();

    QList<CalendarEntryLayout> layout(const QList<CalendarEntry> &entries);

private:
    QList<QList<int>> groupOverlapping(const QList<CalendarEntry> &entries) const;
    QList<CalendarEntryLayout> assignColumns(const QList<int> &cluster, const QList<CalendarEntry> &entries) const;
};

#endif
