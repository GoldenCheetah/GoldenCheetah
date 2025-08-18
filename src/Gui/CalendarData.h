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
    QString primary;
    QString secondary;
    QString secondaryMetric;
    QString iconFile;
    QColor color;
    QString reference;
    QTime start;
    int durationSecs = 0;
    int type = 0;
    bool isRelocatable = false;
    bool hasTrainMode = false;
    QDate spanStart = QDate();
    QDate spanEnd = QDate();
};

struct CalendarDay {
    QDate date;
    bool isDimmed;
    QList<CalendarEntry> entries = QList<CalendarEntry>();
    QList<CalendarEntry> headlineEntries = QList<CalendarEntry>();
};

struct CalendarSummary {
    QList<std::pair<QString, QString>> keyValues;
};

Q_DECLARE_METATYPE(CalendarEntry)
Q_DECLARE_METATYPE(CalendarDay)
Q_DECLARE_METATYPE(CalendarSummary)

#endif
