#ifndef CALENDARDATA_H
#define CALENDARDATA_H

#include <QDate>
#include <QTime>
#include <QList>
#include <QHash>

#define ENTRY_TYPE_ACTIVITY 0
#define ENTRY_TYPE_PLANNED_ACTIVITY 1
#define ENTRY_TYPE_EVENT 10
#define ENTRY_TYPE_PHASE 11
#define ENTRY_TYPE_OTHER 99


struct CalendarEvent {
    QString name;
    QDate date;
};

struct CalendarPhase {
    QString name;
    QDate start;
    QDate end;
};

struct CalendarEntry {
    QString name;
    QString iconFile;
    QColor color;
    QString reference;
    QTime start;
    int durationSecs = 0;
    int type = 0;
    bool isRelocatable = false;
    QDate spanStart = QDate();
    QDate spanEnd = QDate();
};

struct CalendarDay {
    QDate date;
    bool isDimmed;
    QList<CalendarEntry> entries = QList<CalendarEntry>();
    QList<CalendarEntry> headlineEntries = QList<CalendarEntry>();
};

struct CalendarWeeklySummary {
    QDate firstDayOfWeek;
    QList<CalendarEntry> entries;
    QHash<int, int> entriesByType;
};

Q_DECLARE_METATYPE(CalendarEntry)
Q_DECLARE_METATYPE(CalendarDay)
Q_DECLARE_METATYPE(CalendarWeeklySummary)

#endif
