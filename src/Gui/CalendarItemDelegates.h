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

    int minuteToY(int minute, int rectTop, int rectHeight) const;
    int minuteToY(int minute, const QRect& rect) const;
    int minuteToY(const QTime &time, const QRect& rect) const;

    QTime timeFromY(int y, const QRect &rect, int snap = 15) const;

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


class CalendarDayViewDayDelegate : public QStyledItemDelegate {
public:
    explicit CalendarDayViewDayDelegate(TimeScaleData const * const timeScaleData, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;

    int hitTestEntry(const QModelIndex &index, const QPoint &pos) const;

private:
    TimeScaleData const * const timeScaleData;
    mutable QHash<QModelIndex, QList<QRect>> entryRects;
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


class CalendarDayDelegate : public QStyledItemDelegate {
public:
    explicit CalendarDayDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;

    int hitTestEntry(const QModelIndex &index, const QPoint &pos) const;
    int hitTestHeadlineEntry(const QModelIndex &index, const QPoint &pos) const;
    bool hitTestMore(const QModelIndex &index, const QPoint &pos) const;

private:
    mutable QHash<QModelIndex, QList<QRect>> entryRects;
    mutable QHash<QModelIndex, QList<QRect>> headlineEntryRects;
    mutable QHash<QModelIndex, QRect> moreRects;
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
