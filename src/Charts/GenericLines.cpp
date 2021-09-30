/*
 * Copyright (c) 2021 Mark Liversedge (liversedge@gmail.com)
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

#include <QChart>
#include "GenericLines.h"

GenericLines::GenericLines(GenericPlot *plot) : QGraphicsItem(NULL), plot(plot), curve(NULL)
{
}

GenericLines::~GenericLines() {}

void
GenericLines::paint(QPainter*painter, const QStyleOptionGraphicsItem *, QWidget*)
{
    if (curve == NULL) return;

    QPen pen(Qt::lightGray);
    painter->setPen(pen);

    painter->setClipRect(plot->qchart->plotArea());

    foreach(QLineF line, lines) {
        QPointF from = plot->qchart->mapToPosition(line.p1(), curve);
        QPointF to = plot->qchart->mapToPosition(line.p2(), curve);

        painter->drawLine(from, to);
    }
}

void
GenericLines::prepare()
{
    prepareGeometryChange();
}

QRectF GenericLines::boundingRect() const
{
    // we cover the entire plot area generally
    return plot->qchart->plotArea();
}
