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

#ifndef CALENDARITEMDELEGATES_H
#define CALENDARITEMDELEGATES_H

#include <QStyledItemDelegate>


class HitTester {
public:
    explicit HitTester();

    void clear(const QModelIndex &index);
    void resize(const QModelIndex &index, qsizetype size);
    void append(const QModelIndex &index, const QRect &rect);
    bool set(const QModelIndex &index, qsizetype i, const QRect &rect);
    int hitTest(const QModelIndex &index, const QPoint &pos) const;

private:
    QHash<QModelIndex, QList<QRect>> rects;
};


enum class BlockIndicator {
    NoBlock = 0,
    AllBlock = 1,
    BlockBeforeNow = 2
};


class TimeScaleData {
public:
    explicit TimeScaleData();

    void setFirstMinute(int minute);
    int firstMinute() const;

    void setLastMinute(int minute);
    int lastMinute() const;

    void setStepSize(int step);
    int stepSize() const;

    double pixelsPerMinute(int availableHeight) const;
    double pixelsPerMinute(const QRect& rect) const;

    int minuteToYInTable(int minute, int rectTop, int rectHeight) const;
    int minuteToYInTable(int minute, const QRect& rect) const;
    int minuteToYInTable(const QTime &time, const QRect& rect) const;

    QTime timeFromYInTable(int y, const QRect &rect, int snap = 15) const;

private:
    int _firstMinute = 8 * 60;
    int _lastMinute = 21 * 60;
    int _stepSize = 60;
};


// Workaround for Qt5 not having QAbstractItemView::itemDelegateForIndex (Qt6 only)
class ColumnDelegatingItemDelegate : public QStyledItemDelegate {
public:
    explicit ColumnDelegatingItemDelegate(QList<QStyledItemDelegate*> delegates, QObject *parent = nullptr);

    QStyledItemDelegate *getDelegate(int idx) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QList<QStyledItemDelegate*> delegates;
};


class CalendarDetailedDayDelegate : public QStyledItemDelegate {
public:
    explicit CalendarDetailedDayDelegate(TimeScaleData const * const timeScaleData, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;

    mutable HitTester entryTester;

private:
    TimeScaleData const * const timeScaleData;
    void drawWrappingText(QPainter &painter, const QRect &rect, const QString &text) const;
};


class CalendarHeadlineDelegate : public QStyledItemDelegate {
public:
    explicit CalendarHeadlineDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    mutable HitTester headlineTester;

private:
    mutable int heightHint = 0;
};


class CalendarTimeScaleDelegate : public QStyledItemDelegate {
public:
    explicit CalendarTimeScaleDelegate(TimeScaleData *timeScaleData, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    TimeScaleData *timeScaleData;

    int selectNiceInterval(int minInterval) const;
};


class CalendarCompactDayDelegate : public QStyledItemDelegate {
public:
    explicit CalendarCompactDayDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;

    mutable HitTester headlineTester;
    mutable HitTester entryTester;
    mutable HitTester moreTester;
};


class CalendarSummaryDelegate : public QStyledItemDelegate {
public:
    explicit CalendarSummaryDelegate(int horMargin = 4, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    mutable QHash<QModelIndex, bool> indexHasToolTip;
    const int horMargin;
    const int vertMargin = 4;
    const int lineSpacing = 2;
};

#endif
