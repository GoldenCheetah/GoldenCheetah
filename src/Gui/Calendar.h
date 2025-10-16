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
#include <QPushButton>
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
#if QT_VERSION < 0x060000
    void paintCell(QPainter *painter, const QRect &rect, const QDate &date) const override;
#else
    void paintCell(QPainter *painter, const QRect &rect, QDate date) const override;
#endif

private:
    QHash<QDate, QList<CalendarEntry>> activityEntries;
    QHash<QDate, QList<CalendarEntry>> headlineEntries;

    void drawEntries(QPainter *painter, const QList<CalendarEntry> &entries, QPolygon polygon, int maxEntries, int shiftX) const;
};


enum class CalendarDayTableType {
    Day,
    Week
};


class CalendarDayTable : public QTableWidget {
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
    void limitDateRange(const DateRange &dr);
    void setFirstDayOfWeek(Qt::DayOfWeek firstDayOfWeek);
    void setStartHour(int hour);
    void setEndHour(int hour);

signals:
    void dayClicked(const CalendarDay &day, const QTime &time);
    void dayRightClicked(const CalendarDay &day, const QTime &time);
    void entryClicked(const CalendarDay &day, int entryIdx);
    void entryRightClicked(const CalendarDay &day, int entryIdx);
    void entryMoved(const CalendarEntry &activity, const QDate &srcDay, const QDate &destDay, const QTime &destTime);
    void dayChanged(const QDate &date);
    void showInTrainMode(const CalendarEntry &activity);
    void viewActivity(const CalendarEntry &activity);
    void addActivity(bool plan, const QDate &day, const QTime &time);
    void delActivity(const CalendarEntry &activity);

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
#if QT_VERSION < 0x060000
    QAbstractItemDelegate *itemDelegateForIndex(const QModelIndex &index) const;
#endif

private slots:
    void showContextMenu(const QPoint &pos);

private:
    Qt::DayOfWeek firstDayOfWeek = Qt::Monday;
    QDate date;
    DateRange dr;
    CalendarDayTableType type;
    int defaultStartHour = 8;
    int defaultEndHour = 21;

    QTimer dragTimer;
    QPoint pressedPos;
    QModelIndex pressedIndex;
    bool isDraggable = false;
    TimeScaleData timeScaleData;

    void setDropIndicator(int y, BlockIndicator block);
};


class CalendarMonthTable : public QTableWidget {
    Q_OBJECT

public:
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
    void limitDateRange(const DateRange &dr, bool allowKeepMonth = false);
    void setFirstDayOfWeek(Qt::DayOfWeek firstDayOfWeek);

signals:
    void daySelected(const CalendarDay &day);
    void dayClicked(const CalendarDay &day);
    void dayDblClicked(const CalendarDay &day);
    void moreClicked(const CalendarDay &day);
    void moreDblClicked(const CalendarDay &day);
    void dayRightClicked(const CalendarDay &day);
    void entryClicked(const CalendarDay &day, int entryIdx);
    void entryDblClicked(const CalendarDay &day, int entryIdx);
    void entryRightClicked(const CalendarDay &day, int entryIdx);
    void entryMoved(const CalendarEntry &activity, const QDate &srcDay, const QDate &destDay, const QTime &destTime);
    void summaryClicked(const QModelIndex &index);
    void summaryDblClicked(const QModelIndex &index);
    void summaryRightClicked(const QModelIndex &index);
    void monthChanged(const QDate &month, const QDate &firstVisible, const QDate &lastVisible);
    void showInTrainMode(const CalendarEntry &activity);
    void viewActivity(const CalendarEntry &activity);
    void addActivity(bool plan, const QDate &day, const QTime &time);
    void delActivity(const CalendarEntry &activity);
    void repeatSchedule(const QDate &day);
    void insertRestday(const QDate &day);
    void delRestday(const QDate &day);

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
#if QT_VERSION < 0x060000
    QAbstractItemDelegate *itemDelegateForIndex(const QModelIndex &index) const;
#endif

private slots:
    void showContextMenu(const QPoint &pos);

private:
    Qt::DayOfWeek firstDayOfWeek = Qt::Monday;
    QDate firstOfMonth;
    QDate startDate; // first visible date
    QDate endDate; // last visible date
    QDate currentDate; // currently selected date
    DateRange dr;

    QTimer dragTimer;
    QPoint pressedPos;
    QModelIndex pressedIndex;
    bool isDraggable = false;
};


enum class CalendarView {
    Day = 0,
    Week = 1,
    Month = 2
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
    void limitDateRange(const DateRange &dr);
    QDate firstVisibleDay() const;
    QDate lastVisibleDay() const;
    QDate selectedDate() const;
    void updateMeasures();

signals:
    void showInTrainMode(const CalendarEntry &activity);
    void viewActivity(const CalendarEntry &activity);
    void addActivity(bool plan, const QDate &day, const QTime &time);
    void delActivity(const CalendarEntry &activity);
    void entryMoved(const CalendarEntry &activity, const QDate &srcDay, const QDate &destDay, const QTime &destTime);
    void dayChanged(const QDate &date);

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
    void limitDateRange(const DateRange &dr);
    QDate firstVisibleDay() const;
    QDate firstVisibleDay(const QDate &date) const;
    QDate lastVisibleDay() const;
    QDate lastVisibleDay(const QDate &date) const;
    QDate selectedDate() const;

signals:
    void showInTrainMode(const CalendarEntry &activity);
    void viewActivity(const CalendarEntry &activity);
    void addActivity(bool plan, const QDate &day, const QTime &time);
    void delActivity(const CalendarEntry &activity);
    void entryMoved(const CalendarEntry &activity, const QDate &srcDay, const QDate &destDay, const QTime &destTime);
    void dayChanged(const QDate &date);

private:
    CalendarDayTable *weekTable;
};


class Calendar : public QWidget {
    Q_OBJECT

public:
    explicit Calendar(const QDate &dateInMonth, Qt::DayOfWeek firstDayOfWeek = Qt::Monday, Measures * const athleteMeasures = nullptr, QWidget *parent = nullptr);

    void setDate(const QDate &dateInMonth, bool allowKeepMonth = false);
    void fillEntries(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QList<CalendarSummary> &summaries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries);
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
    void activateDateRange(const DateRange &dr, bool allowKeepMonth = false);
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
    void daySelected(const QDate &date);
    void dayClicked(const QDate &date);
    void summaryClicked(const QDate &date);
    void dayChanged(const QDate &date);
    void monthChanged(const QDate &month, const QDate &firstVisible, const QDate &lastVisible);
    void dateRangeActivated(const QString &name);
    void showInTrainMode(const CalendarEntry &activity);
    void viewActivity(const CalendarEntry &activity);
    void addActivity(bool plan, const QDate &day, const QTime &time);
    void delActivity(const CalendarEntry &activity);
    void repeatSchedule(const QDate &day);
    void moveActivity(const CalendarEntry &activity, const QDate &srcDay, const QDate &destDay, const QTime &destTime);
    void insertRestday(const QDate &day);
    void delRestday(const QDate &day);

private:
    QAction *prevAction;
    QAction *nextAction;
    QAction *todayAction;
    QAction *dayAction;
    QAction *weekAction;
    QAction *monthAction;
    QPushButton *dateNavigator;
    QMenu *dateMenu;
    QStackedWidget *viewStack;
    CalendarDayView *dayView;
    CalendarWeekView *weekView;
    CalendarMonthTable *monthView;
    DateRange dateRange;
    Qt::DayOfWeek firstDayOfWeek = Qt::Monday;

    void setNavButtonState();
    void updateDateLabel();

private slots:
    void populateDateMenu();
};

#endif
