/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#include "GcToolBar.h"
#include "Colors.h"

GcToolBar::GcToolBar(QWidget *parent) : QWidget(parent)
{
#ifdef WIN32
    setFixedHeight(40);
#else
    setFixedHeight(50);
#endif
    setContentsMargins(0,0,0,0);
    layout = new QHBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(0,0,0,0);
    installEventFilter(this);
}

void
GcToolBar::addStretch()
{
    layout->addStretch();
}

void
GcToolBar::addWidget(QWidget *x) // add a widget that doesn't toggle selection
{
    layout->addWidget(x);
    x->installEventFilter(this);
}

void
GcToolBar::paintEvent (QPaintEvent *event)
{
    // paint the darn thing!
    paintBackground(event);
}

void
GcToolBar::paintBackground(QPaintEvent *)
{
    QPainter painter(this);

    // get the widget area
    QRect all(0,0,width(),height());

    // fill with a linear gradient
    QLinearGradient linearGradient = GCColor::linearGradient(23, isActiveWindow());
    
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, linearGradient);

    if (!GCColor::isFlat()) {
        // paint the bottom lines
        QPen black(QColor(100,100,100));
        painter.setPen(black);
        painter.drawLine(0,height()-1, width()-1, height()-1);

#ifndef Q_OS_WIN32 // never on windows.
        QPen gray(QColor(230,230,230));
        painter.setPen(gray);
        painter.drawLine(0,0, width()-1, 0);
#endif
    }
}
