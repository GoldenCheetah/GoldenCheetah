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
#include "Context.h"


IntervalTreeView::IntervalTreeView(Context *context) : context(context)
{
    setDragDropMode(QAbstractItemView::InternalMove);
    setDragDropOverwriteMode(true);
    setDropIndicatorShown(true);
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    invisibleRootItem()->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

void
IntervalTreeView::dropEvent(QDropEvent* event)
{
    IntervalItem* item1 = (IntervalItem *)itemAt(event->pos());
    QTreeWidget::dropEvent(event);
    IntervalItem* item2 = (IntervalItem *)itemAt(event->pos());

    if (item1==topLevelItem(0) || item1 != item2)
        QTreeWidget::itemChanged(item2, 0);
}
