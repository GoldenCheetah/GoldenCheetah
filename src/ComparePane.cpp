/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#include "ComparePane.h"

ComparePane::ComparePane(QWidget *parent, CompareMode mode) : mode_(mode), QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    setAcceptDrops(true);
    setAutoFillBackground(true);
    QPalette pal;
    pal.setBrush(QPalette::Active, QPalette::Window, Qt::white);
    pal.setBrush(QPalette::Inactive, QPalette::Window, Qt::white);
    setPalette(pal);
}


void 
ComparePane::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->formats().contains("application/x-gc-intervals") ||
        event->mimeData()->formats().contains("application/x-gc-seasons")) {
        event->acceptProposedAction();
    }
}

void 
ComparePane::dragLeaveEvent(QDragLeaveEvent *event)
{
    // we might consider hiding on this?
}

void
ComparePane::dropEvent(QDropEvent *event)
{
    qDebug()<<"compare pane: dropped:"<<event->mimeData()->formats();

    // set action to copy and accept that so the source data
    // is left intact and not wiped or removed
    event->setDropAction(Qt::CopyAction);
    event->accept();

    // here we can unpack and add etc...
}
