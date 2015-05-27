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

#ifndef _GC_IntervalTreeView_h
#define _GC_IntervalTreeView_h 1
#include "GoldenCheetah.h"
#include "IntervalItem.h"
#include "Colors.h"

#include <QtGui>
#include <QTreeWidget>
#include <QStyle>
#include <QStyledItemDelegate>

class Context;

class IntervalTreeView : public QTreeWidget
{
    Q_OBJECT;

    public:
        IntervalTreeView(Context *context);

        // for drag/drop
        QStringList mimeTypes () const;
        QMimeData * mimeData ( const QList<QTreeWidgetItem *> items ) const;

        // access protected members .. why do the Trolls do this to us?
        QTreeWidgetItem *itemFromIndexPublic(QModelIndex index) { return itemFromIndex(index); }
        int rowHeightPublic(QModelIndex index) { return rowHeight(index); }

    private slots:
        void mouseHover(QTreeWidgetItem *item, int column);

    protected:
        Context *context;

        void dropEvent(QDropEvent* event);

};

// style the item to show the color like we see in the ride navigator
class IntervalColorDelegate : public QStyledItemDelegate 
{
    Q_OBJECT
public:

    explicit IntervalColorDelegate(IntervalTreeView *parent = 0) : QStyledItemDelegate(parent), tree(parent) { }
    IntervalTreeView *tree;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const {
        QStyledItemDelegate::paint(painter, option, index);

        painter->save();

        // Only do this on items !
        if(index.isValid() && index.parent().isValid()) {

            // state of item
            bool hover = option.state & QStyle::State_MouseOver;
            bool selected = option.state & QStyle::State_Selected;
            bool focus = option.state & QStyle::State_HasFocus;

            // get the interval item a bit convoluted !
            QTreeWidgetItem *item = tree->itemFromIndexPublic(index);
            QVariant v =  item->data(0, Qt::UserRole);
            IntervalItem *interval = v.isValid() ? static_cast<IntervalItem*>(v.value<void*>()) : NULL;

            if (!selected && !hover) {
                QRect high(option.rect.x()+option.rect.width() - 7, option.rect.y(), 7, tree->rowHeightPublic(index));
                painter->fillRect(high, interval->color);
                //qDebug()<<"interval paint event" << hover << selected << focus <<interval->name<<interval->type<<interval->color;
            }
        }
        painter->restore();
    }
};
#endif // _GC_IntervalTreeView_h
