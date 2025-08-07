#ifndef CALENDARITEMDELEGATES_H
#define CALENDARITEMDELEGATES_H

#include <QStyledItemDelegate>


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
    explicit CalendarSummaryDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;
};

#endif
