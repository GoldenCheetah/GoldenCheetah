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
    setDragDropOverwriteMode(false);
    setDropIndicatorShown(true);
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    invisibleRootItem()->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

void
IntervalTreeView::dropEvent(QDropEvent* event)
{
qDebug()<<"interval tree drop event!!"<<event->mimeData()->urls();

    IntervalItem* item1 = (IntervalItem *)itemAt(event->pos());
    QTreeWidget::dropEvent(event);
    IntervalItem* item2 = (IntervalItem *)itemAt(event->pos());

    if (item1==topLevelItem(0) || item1 != item2)
        QTreeWidget::itemChanged(item2, 0);
}

QStringList 
IntervalTreeView::mimeTypes() const
{
    QStringList returning;
    returning << "application/x-gc-intervals";

    return returning;
}

QMimeData *
IntervalTreeView::mimeData (const QList<QTreeWidgetItem *> items) const
{
    QMimeData *returning = new QMimeData;

    // we need to pack into a byte array
    QByteArray rawData;
    QDataStream stream(&rawData, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_6);

    // pack data 
    stream << (quint64)(context); // where did this come from?
    stream << items.count();
    foreach (QTreeWidgetItem *p, items) {

        // convert to one of ours
        IntervalItem *i = static_cast<IntervalItem*>(p);

        // serialize
        stream << p->text(0); // name
        stream << (quint64)(i->ride);
        stream << i->start << i->stop; // start and stop in secs
        stream << i->startKM << i->stopKM; // start and stop km
        stream << i->displaySequence;

    }

    // and return as mime data
    returning->setData("application/x-gc-intervals", rawData);
    return returning;
}
