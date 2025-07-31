#ifndef CALENDAR_H
#define CALENDAR_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QDate>
#include <QStringList>
#include <QTimer>
#include <QApplication>

#include "CalendarData.h"
#include "TimeUtils.h"


class CalendarMonthTable : public QTableWidget {
    Q_OBJECT

public:
    explicit CalendarMonthTable(Qt::DayOfWeek firstDayOfWeek = Qt::Monday, QWidget *parent = nullptr);
    explicit CalendarMonthTable(const QDate &dateInMonth, Qt::DayOfWeek firstDayOfWeek = Qt::Monday, QWidget *parent = nullptr);

    bool selectDay(const QDate &day);
    bool setMonth(const QDate &dateInMonth, bool allowKeepMonth = false);
    bool addMonths(int months);
    bool addYears(int years);
    bool canAddMonths(int months) const;
    bool canAddYears(int years) const;
    bool isInDateRange(const QDate &date) const;
    void fillEntries(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries);
    QDate firstOfCurrentMonth() const;
    QDate firstVisibleDay() const;
    QDate lastVisibleDay() const;
    QDate selectedDate() const;
    void limitDateRange(const DateRange &dr, bool allowKeepMonth = false);
    void setFirstDayOfWeek(Qt::DayOfWeek firstDayOfWeek);

signals:
    void dayClicked(const CalendarDay &day);
    void dayDblClicked(const CalendarDay &day);
    void moreClicked(const CalendarDay &day);
    void moreDblClicked(const CalendarDay &day);
    void dayRightClicked(const CalendarDay &day);
    void entryClicked(const CalendarDay &day, int entryIdx);
    void entryDblClicked(const CalendarDay &day, int entryIdx);
    void entryRightClicked(const CalendarDay &day, int entryIdx);
    void entryMoved(const CalendarEntry &activity, const QDate &srcDay, const QDate &destDay);
    void summaryClicked(const QModelIndex &index);
    void summaryDblClicked(const QModelIndex &index);
    void summaryRightClicked(const QModelIndex &index);
    void monthChanged(const QDate &month, const QDate &firstVisible, const QDate &lastVisible);
    void viewActivity(const CalendarEntry &activity);
    void addActivity(bool plan, const QDate &day, const QTime &time);
    void delActivity(const CalendarEntry &activity);
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
    int findEntry(const QModelIndex &index, const QPoint &pos) const;

    Qt::DayOfWeek firstDayOfWeek = Qt::Monday;
    QDate firstOfMonth;
    QDate startDate; // first visible date
    QDate endDate; // last visible date
    DateRange dr;

    QTimer dragTimer;
    QPoint pressedPos;
    QModelIndex pressedIndex;
    bool isDraggable = false;
};


class Calendar : public QWidget {
    Q_OBJECT

public:
    explicit Calendar(Qt::DayOfWeek firstDayOfWeek = Qt::Monday, QWidget *parent = nullptr);
    explicit Calendar(const QDate &dateInMonth, Qt::DayOfWeek firstDayOfWeek = Qt::Monday, QWidget *parent = nullptr);

    void setMonth(const QDate &dateInMonth, bool allowKeepMonth = false);
    void fillEntries(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries);
    QDate firstOfCurrentMonth() const;
    QDate firstVisibleDay() const;
    QDate lastVisibleDay() const;
    QDate selectedDate() const;

public slots:
    void activateDateRange(const DateRange &dr, bool allowKeepMonth = false);
    void setFirstDayOfWeek(Qt::DayOfWeek firstDayOfWeek);
    void setSummaryMonthVisible(bool visible);

signals:
    void dayClicked(const QDate &date);
    void summaryClicked(const QDate &date);
    void monthChanged(const QDate &month, const QDate &firstVisible, const QDate &lastVisible);
    void dateRangeActivated(const QString &name);
    void viewActivity(const CalendarEntry &activity);
    void addActivity(bool plan, const QDate &day, const QTime &time);
    void delActivity(const CalendarEntry &activity);
    void moveActivity(const CalendarEntry &activity, const QDate &srcDay, const QDate &destDay);
    void insertRestday(const QDate &day);
    void delRestday(const QDate &day);

private:
    QPushButton *prevYButton;
    QPushButton *prevMButton;
    QPushButton *nextMButton;
    QPushButton *nextYButton;
    QPushButton *todayButton;
    QLabel *dateLabel;
    CalendarMonthTable *monthTable;
    QHash<QDate, QList<CalendarEntry>> headlineEntries;

    void setNavButtonState();
};

#endif
