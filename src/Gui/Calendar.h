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

#ifndef CALENDAR_H
#define CALENDAR_H

#include <QWidget>
#include <QStackedWidget>
#include <QTableWidget>
#include <QCalendarWidget>
#include <QToolBar>
#include <QMenu>
#include <QLabel>
#include <QComboBox>
#include <QDate>
#include <QStringList>
#include <QTimer>
#include <QApplication>

#include "CalendarItemDelegates.h"
#include "CalendarData.h"
#include "TimeUtils.h"
#include "Measures.h"


class CalendarOverview : public QCalendarWidget {
    Q_OBJECT

public:
    explicit CalendarOverview(QWidget *parent = nullptr);

    QDate firstVisibleDay() const;
    QDate lastVisibleDay() const;
    void limitDateRange(const DateRange &dr);
    void fillEntries(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries);

protected:
    void paintCell(QPainter *painter, const QRect &rect, QDate date) const override;

private:
    QHash<QDate, QList<CalendarEntry>> activityEntries;
    QHash<QDate, QList<CalendarEntry>> headlineEntries;

    void drawEntries(QPainter *painter, const QList<CalendarEntry> &entries, QPolygon polygon, int maxEntries, int shiftX) const;
};


class CalendarBaseTable : public QTableWidget {
    Q_OBJECT

public:
    explicit CalendarBaseTable(QWidget *parent = nullptr);

signals:
    void showInTrainMode(CalendarEntry ctivity);
    void filterSimilar(CalendarEntry activity);
    void linkActivity(CalendarEntry activity, bool autoLink);
    void unlinkActivity(CalendarEntry activity);
    void viewActivity(CalendarEntry activity);
    void copyPlannedActivity(CalendarEntry activity);
    void pastePlannedActivity(QDate day, QTime time);
    void viewLinkedActivity(CalendarEntry activity);
    void addActivity(bool plan, QDate day, QTime time);
    void delActivity(CalendarEntry activity);
    void saveChanges(CalendarEntry activity);
    void discardChanges(CalendarEntry activity);
    void repeatSchedule(QDate day);
    void insertRestday(QDate day);
    void delRestday(QDate day);
    void addEvent(QDate date);
    void editEvent(CalendarEntry entry);
    void delEvent(CalendarEntry entry);
    void addPhase(QDate date);
    void editPhase(CalendarEntry entry);
    void delPhase(CalendarEntry entry);

protected:
    QMenu *buildContextMenu(const CalendarDay &day, CalendarEntry const * const entryPtr, const QTime &time, bool canHavePhasesEvents);
};


enum class CalendarDayTableType {
    Day,
    Week
};


class CalendarDayTable : public CalendarBaseTable {
    Q_OBJECT

public:
    explicit CalendarDayTable(const QDate &date, CalendarDayTableType type = CalendarDayTableType::Day, Qt::DayOfWeek firstDayOfWeek = Qt::Monday, QWidget *parent = nullptr);

    bool setDay(const QDate &date);
    QDate firstVisibleDay() const;
    QDate firstVisibleDay(const QDate &date) const;
    QDate lastVisibleDay() const;
    QDate lastVisibleDay(const QDate &date) const;
    QDate selectedDate() const;
    bool isInDateRange(const QDate &date) const;
    void fillEntries(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QList<CalendarSummary> &summaries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries);
    void limitDateRange(const DateRange &dr, bool canHavePhasesOrEvents = false);
    void setFirstDayOfWeek(Qt::DayOfWeek firstDayOfWeek);
    void setStartHour(int hour);
    void setEndHour(int hour);

signals:
    void dayClicked(CalendarDay day, QTime time);
    void dayRightClicked(CalendarDay day, QTime time);
    void entryClicked(CalendarDay day, int entryIdx);
    void entryRightClicked(CalendarDay day, int entryIdx);
    void entryMoved(CalendarEntry activity, QDate srcDay, QDate destDay, QTime destTime);
    void dayChanged(QDate date);

protected:
    void changeEvent(QEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void showContextMenu(const QPoint &pos);

private:
    Qt::DayOfWeek firstDayOfWeek = Qt::Monday;
    QDate date;
    DateRange dr;
    bool canHavePhasesOrEvents = false;
    CalendarDayTableType type;
    int defaultStartHour = 8;
    int defaultEndHour = 21;

    QTimer dragTimer;
    QPoint pressedPos;
    QModelIndex pressedIndex;
    bool isDraggable = false;
    TimeScaleData timeScaleData;

    void setDropIndicator(int y, BlockIndicator block);
    QMenu *makeHeaderMenu(const QModelIndex &index, const QPoint &pos);
    QMenu *makeActivityMenu(const QModelIndex &index, const QPoint &pos);
    void setRelated(const QString &linkedReference);
    void clearRelated();
};


class CalendarMonthTable : public CalendarBaseTable {
    Q_OBJECT

public:
    enum CalendarDayTableRoles {
        DateRole = Qt::UserRole + 1000 // [QDate] Date of cell
    };

    explicit CalendarMonthTable(Qt::DayOfWeek firstDayOfWeek = Qt::Monday, QWidget *parent = nullptr);
    explicit CalendarMonthTable(const QDate &dateInMonth, Qt::DayOfWeek firstDayOfWeek = Qt::Monday, QWidget *parent = nullptr);

    bool selectDay(const QDate &day);
    bool setMonth(const QDate &dateInMonth, bool allowKeepMonth = false);
    bool isInDateRange(const QDate &date) const;
    void fillEntries(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QList<CalendarSummary> &summaries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries);
    QDate firstOfCurrentMonth() const;
    QDate firstVisibleDay() const;
    QDate lastVisibleDay() const;
    QDate selectedDate() const;
    void limitDateRange(const DateRange &dr, bool allowKeepMonth = false, bool canHavePhasesOrEvents = false);
    void setFirstDayOfWeek(Qt::DayOfWeek firstDayOfWeek);

signals:
    void daySelected(CalendarDay day);
    void dayClicked(CalendarDay day);
    void dayDblClicked(CalendarDay day);
    void moreClicked(CalendarDay day);
    void moreDblClicked(CalendarDay day);
    void dayRightClicked(CalendarDay day);
    void entryClicked(CalendarDay day, int entryIdx);
    void entryDblClicked(CalendarDay day, int entryIdx);
    void entryRightClicked(CalendarDay day, int entryIdx);
    void entryMoved(CalendarEntry activity, QDate srcDay, QDate destDay, QTime destTime);
    void summaryClicked(QModelIndex index);
    void summaryDblClicked(QModelIndex index);
    void summaryRightClicked(QModelIndex index);
    void monthChanged(QDate month, QDate firstVisible, QDate lastVisible);

protected:
    void changeEvent(QEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void showContextMenu(const QPoint &pos);

private:
    Qt::DayOfWeek firstDayOfWeek = Qt::Monday;
    QDate firstOfMonth;
    QDate startDate; // first visible date
    QDate endDate; // last visible date
    QDate currentDate; // currently selected date
    DateRange dr;
    bool canHavePhasesOrEvents = false;

    QTimer dragTimer;
    QPoint pressedPos;
    QModelIndex pressedIndex;
    bool isDraggable = false;

    void setRelated(const QString &linkedReference);
    void clearRelated();
};


enum class CalendarView {
    Day = 0,
    Week = 1,
    Month = 2,
};


class CalendarDayView : public QWidget {
    Q_OBJECT

public:
    explicit CalendarDayView(const QDate &date, Measures * const athleteMeasures = nullptr, QWidget *parent = nullptr);

    bool setDay(const QDate &date);
    void setFirstDayOfWeek(Qt::DayOfWeek firstDayOfWeek);
    void setStartHour(int hour);
    void setEndHour(int hour);
    void setSummaryVisible(bool visible);
    void fillEntries(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QList<CalendarSummary> &summaries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries);
    void limitDateRange(const DateRange &dr, bool canHavePhasesOrEvents = false);
    QDate firstVisibleDay() const;
    QDate lastVisibleDay() const;
    QDate selectedDate() const;
    void updateMeasures();

signals:
    void entryMoved(CalendarEntry activity, QDate srcDay, QDate destDay, QTime destTime);
    void dayChanged(QDate date);

    void showInTrainMode(CalendarEntry activity);
    void filterSimilar(CalendarEntry activity);
    void linkActivity(CalendarEntry activity, bool autoLink);
    void unlinkActivity(CalendarEntry activity);
    void viewActivity(CalendarEntry activity);
    void viewLinkedActivity(CalendarEntry activity);
    void copyPlannedActivity(CalendarEntry activity);
    void pastePlannedActivity(QDate day, QTime time);
    void addActivity(bool plan, QDate day, QTime time);
    void delActivity(CalendarEntry activity);
    void saveChanges(CalendarEntry activity);
    void discardChanges(CalendarEntry activity);
    void repeatSchedule(QDate day);
    void insertRestday(QDate day);
    void delRestday(QDate day);
    void addEvent(QDate date);
    void editEvent(CalendarEntry entry);
    void delEvent(CalendarEntry entry);
    void addPhase(QDate date);
    void editPhase(CalendarEntry entry);
    void delPhase(CalendarEntry entry);

private:
    Measures * const athleteMeasures;
    CalendarOverview *dayDateSelector;
    QTabWidget *measureTabs;
    CalendarDayTable *dayTable;

    bool measureDialog(const QDateTime &when, MeasuresGroup * const measuresGroup, bool update);
    void updateMeasures(const QDate &date);
};


class CalendarWeekView : public QWidget {
    Q_OBJECT

public:
    explicit CalendarWeekView(const QDate &date, QWidget *parent = nullptr);

    bool setDay(const QDate &date);
    void setFirstDayOfWeek(Qt::DayOfWeek firstDayOfWeek);
    void setStartHour(int hour);
    void setEndHour(int hour);
    void setSummaryVisible(bool visible);
    void fillEntries(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QList<CalendarSummary> &summaries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries);
    void limitDateRange(const DateRange &dr, bool canHavePhasesOrEvents = false);
    QDate firstVisibleDay() const;
    QDate firstVisibleDay(const QDate &date) const;
    QDate lastVisibleDay() const;
    QDate lastVisibleDay(const QDate &date) const;
    QDate selectedDate() const;

signals:
    void entryMoved(CalendarEntry activity, QDate srcDay, QDate destDay, QTime destTime);
    void dayChanged(QDate date);

    void showInTrainMode(CalendarEntry activity);
    void filterSimilar(CalendarEntry activity);
    void linkActivity(CalendarEntry activity, bool autoLink);
    void unlinkActivity(CalendarEntry activity);
    void viewActivity(CalendarEntry activity);
    void viewLinkedActivity(CalendarEntry activity);
    void copyPlannedActivity(CalendarEntry activity);
    void pastePlannedActivity(QDate day, QTime time);
    void addActivity(bool plan, QDate day, QTime time);
    void delActivity(CalendarEntry activity);
    void saveChanges(CalendarEntry activity);
    void discardChanges(CalendarEntry activity);
    void repeatSchedule(QDate day);
    void insertRestday(QDate day);
    void delRestday(QDate day);
    void addEvent(QDate date);
    void editEvent(CalendarEntry entry);
    void delEvent(CalendarEntry entry);
    void addPhase(QDate date);
    void editPhase(CalendarEntry entry);
    void delPhase(CalendarEntry entry);

private:
    CalendarDayTable *weekTable;
};


class Calendar : public QWidget {
    Q_OBJECT

public:
    explicit Calendar(const QDate &dateInMonth, Qt::DayOfWeek firstDayOfWeek = Qt::Monday, Measures * const athleteMeasures = nullptr, QWidget *parent = nullptr);

    void setDate(const QDate &dateInMonth, bool allowKeepMonth = false);
    void fillEntries(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QList<CalendarSummary> &summaries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries, bool isFiltered);
    QDate firstOfCurrentMonth() const;
    QDate firstVisibleDay() const;
    QDate lastVisibleDay() const;
    QDate selectedDate() const;
    CalendarView currentView() const;

    bool goNext(int amount);
    QDate fitToMonth(const QDate &date, bool preferToday = true) const;
    bool canGoNext(int amount) const;
    bool isInDateRange(const QDate &date) const;
    int weekNumber(const QDate &date) const;

public slots:
    void setView(CalendarView view);
    void activateDateRange(const DateRange &dr, bool allowKeepMonth = false, bool canHavePhasesOrEvents = false);
    void setFirstDayOfWeek(Qt::DayOfWeek firstDayOfWeek);
    void setStartHour(int hour);
    void setEndHour(int hour);
    void setSummaryDayVisible(bool visible);
    void setSummaryWeekVisible(bool visible);
    void setSummaryMonthVisible(bool visible);
    void applyNavIcons();
    void updateMeasures();

signals:
    void viewChanged(CalendarView newView, CalendarView oldView);
    void daySelected(QDate date);
    void dayClicked(QDate date);
    void summaryClicked(QDate date);
    void dayChanged(QDate date);
    void monthChanged(QDate month, QDate firstVisible, QDate lastVisible);
    void dateRangeActivated(QString name);
    void moveActivity(CalendarEntry activity, QDate srcDay, QDate destDay, QTime destTime);

    void showInTrainMode(CalendarEntry activity);
    void filterSimilar(CalendarEntry activity);
    void linkActivity(CalendarEntry activity, bool autoLink);
    void unlinkActivity(CalendarEntry activity);
    void viewActivity(CalendarEntry activity);
    void viewLinkedActivity(CalendarEntry activity);
    void copyPlannedActivity(CalendarEntry activity);
    void pastePlannedActivity(QDate day, QTime time);
    void addActivity(bool plan, QDate day, QTime time);
    void delActivity(CalendarEntry activity);
    void saveChanges(CalendarEntry activity);
    void discardChanges(CalendarEntry activity);
    void repeatSchedule(QDate day);
    void insertRestday(QDate day);
    void delRestday(QDate day);
    void addEvent(QDate date);
    void editEvent(CalendarEntry entry);
    void delEvent(CalendarEntry entry);
    void addPhase(QDate date);
    void editPhase(CalendarEntry entry);
    void delPhase(CalendarEntry entry);

private:
    QToolBar *toolbar;
    QAction *prevAction;
    QAction *nextAction;
    QAction *todayAction;
    QAction *separator;
    QAction *dayAction;
    QAction *weekAction;
    QAction *monthAction;
    QToolButton *dateNavigator;
    QAction *dateNavigatorAction;
    QMenu *dateMenu;
    QLabel *seasonLabel;
    QAction *seasonLabelAction;
    QAction *filterSpacerAction;
    QLabel *filterLabel;
    QAction *filterLabelAction;
    QStackedWidget *viewStack;
    CalendarDayView *dayView;
    CalendarWeekView *weekView;
    CalendarMonthTable *monthView;
    DateRange dateRange;
    Qt::DayOfWeek firstDayOfWeek = Qt::Monday;

    void setNavButtonState();
    void updateHeader();

private slots:
    void populateDateMenu();
};

#endif
