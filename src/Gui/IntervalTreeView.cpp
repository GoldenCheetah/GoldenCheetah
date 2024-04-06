/*
 * Copyright (c) 2012 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#include "IntervalTreeView.h"
#include "IntervalItem.h"
#include "RideItem.h"
#include "RideFile.h"
#include "Context.h"
#include "Settings.h"
#include "Colors.h"
#include <QStyle>
#include <QStyleFactory>
#include <QScrollBar>
#include <QStyledItemDelegate>


IntervalTreeView::IntervalTreeView(Context *context) : context(context)
{
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setDragDropOverwriteMode(false);
    setDropIndicatorShown(true);
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
#ifdef Q_OS_WIN
    QStyle *cde = QStyleFactory::create(OS_STYLE);
    verticalScrollBar()->setStyle(cde);
#endif
    setStyleSheet("QTreeView::item:hover { background: lightGray; }");
    setMouseTracking(true);
    invisibleRootItem()->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    // we want the color rectangle
    setItemDelegate(new IntervalColorDelegate(this));

    connect(this, SIGNAL(itemEntered(QTreeWidgetItem*,int)), this, SLOT(mouseHover(QTreeWidgetItem*,int)));
}

void
IntervalTreeView::mouseHover(QTreeWidgetItem *item, int)
{
    QVariant v = item->data(0, Qt::UserRole);
    IntervalItem *hover = static_cast<IntervalItem*>(v.value<void*>());

    // NULL is a tree, non-NULL is a node
    if (hover) context->notifyIntervalHover(hover);
}

void
IntervalTreeView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->source() != this || event->dropAction() != Qt::MoveAction) return;

    QAbstractItemView::dragEnterEvent(event);
}

void
IntervalTreeView::dropEvent(QDropEvent* event)
{
    QTreeWidgetItem* target = (QTreeWidgetItem *)itemAt(event->pos());
    QTreeWidgetItem* parent = target->parent();

    QList<IntervalItem*> intervals =  context->rideItem()->intervals();
    QList<IntervalItem*> userIntervals = context->rideItem()->intervals(RideFileInterval::USER);

    int indexTo = intervals.indexOf(userIntervals.at(parent->indexOfChild(target)));
    int offsetFrom = 0;
    int offsetTo = 0;

    bool change = false;
    foreach (QTreeWidgetItem *p, selectedItems()) {
        if (p->parent() == parent) {
            int indexFrom = intervals.indexOf(userIntervals.at(parent->indexOfChild(p)));

            context->rideItem()->moveInterval(indexFrom+offsetFrom,indexTo+offsetTo);
            change = true;

            if (indexFrom<indexTo)
                offsetFrom--;
            else
                offsetTo++;
        }
    }

    if (change) {
        context->notifyIntervalsUpdate(context->rideItem());
        context->rideItem()->setDirty(true);
    }

    // We don't need or want to finish the dropEvent
    //QTreeWidget::dropEvent(event);
}

QStringList 
IntervalTreeView::mimeTypes() const
{
    QStringList returning;
    returning << "application/x-gc-intervals";

    return returning;
}

QMimeData *
IntervalTreeView::mimeData
#if QT_VERSION < 0x060000
(const QList<QTreeWidgetItem *> items) const
#else
(const QList<QTreeWidgetItem *> &items) const
#endif
{
    QMimeData *returning = new QMimeData;

    // we need to pack into a byte array
    QByteArray rawData;
    QDataStream stream(&rawData, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_6);

    // pack data 
    stream << (quint64)(context); // where did this come from?
    stream << (int)items.count();
    foreach (QTreeWidgetItem *p, items) {

        // convert to one of ours
        QVariant v = p->data(0, Qt::UserRole);
        IntervalItem *interval = static_cast<IntervalItem*>(v.value<void*>());

        // drag and drop tree !?
        if (interval == NULL) return returning;

        RideItem *ride = interval->rideItem();

        // serialize
        stream << interval->name; // name
        stream << (quint64)(ride);
        stream << (quint64)interval->start << (quint64)interval->stop; // start and stop in secs
        stream << (quint64)interval->startKM << (quint64)interval->stopKM; // start and stop km
        stream << (quint64)interval->displaySequence;
        stream << interval->route;

    }

    // and return as mime data
    returning->setData("application/x-gc-intervals", rawData);
    return returning;
}

void 
IntervalColorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const 
{
    //QStyledItemDelegate::paint(painter, option, index);

    painter->save();

    // Only do this on items !
    if(index.isValid() && index.parent().isValid()) {

        // extract the state of item
        bool hover = option.state & QStyle::State_MouseOver;
        bool selected = option.state & QStyle::State_Selected;

        QTreeWidgetItem *item = tree->itemFromIndexPublic(index);
        QVariant v =  item->data(0, Qt::UserRole+1);
        QColor color = v.value<QColor>();

        // indicate color of interval in charts
        if (!selected && !hover) {

            // 7 pix wide mark on rhs
            QRect high(option.rect.x()+option.rect.width() - (7*dpiXFactor),
                       option.rect.y(), (7*dpiXFactor), tree->rowHeightPublic(index));

            // use the interval colour
            painter->fillRect(high, color);

        }

        // is it a performance test ?
        QVariant t = item->data(0, Qt::UserRole+2);
        const_cast<QStyleOptionViewItem&>(option).font.setBold(t.toInt() ? true : false);
    }
    QStyledItemDelegate::paint(painter, option, index);
    painter->restore();
}
