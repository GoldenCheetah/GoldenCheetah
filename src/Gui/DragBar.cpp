/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#include "DragBar.h"
#include <QDebug>

DragBar::DragBar(QWidget*parent) : QTabBar(parent)
{
    setAcceptDrops(true);
}

void
DragBar::dragEnterEvent(QDragEnterEvent*event)
{
    if (tabAt(event->position().toPoint()) != -1 && tabAt(event->position().toPoint()) != currentIndex()) {
        setCurrentIndex(tabAt(event->position().toPoint()));
    }
}

void
DragBar::dragLeaveEvent(QDragLeaveEvent*) {}

void
DragBar::dragMoveEvent(QDragMoveEvent *event) // we don't get these without accepting dragevent
{                                             // and that would be a bad idea
    if (tabAt(event->position().toPoint()) != -1 && tabAt(event->position().toPoint()) != currentIndex()) {
        setCurrentIndex(tabAt(event->position().toPoint()));
    }
}

void
DragBar::dropEvent(QDropEvent *) {}
