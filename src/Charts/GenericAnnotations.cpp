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

#include "GenericAnnotations.h"
#include "GenericPlot.h"

//
// Controller (its the thing attached to the chart's scene
//
GenericAnnotationController::GenericAnnotationController(GenericPlot *plot) : plot(plot) {
    plot->qchart->scene()->addItem(this);
}

void GenericAnnotationController::paint(QPainter*p, const QStyleOptionGraphicsItem *o, QWidget*w) {
    foreach(GenericAnnotation *annotation, annotations)
        annotation->paint(p, o, w);
}

// add remove annotations
void GenericAnnotationController::addAnnotation(GenericAnnotation* item) { annotations.append(item); }
void GenericAnnotationController::removeAnnotation(GenericAnnotation *item) { annotations.removeOne(item); }

// always the entire plotarea
QRectF GenericAnnotationController::boundingRect() const { return plot->qchart->plotArea(); }

//
// line painter
//
GenericLines::GenericLines(GenericAnnotationController *controller) : curve(NULL), controller(controller) {}
GenericLines::~GenericLines() {}

void
GenericLines::paint(QPainter*painter, const QStyleOptionGraphicsItem *, QWidget*)
{
    if (curve == NULL) return;

    QPen pen(Qt::lightGray);
    painter->setPen(pen);

    painter->setClipRect(controller->plot->qchart->plotArea());

    foreach(QLineF line, lines) {
        QPointF from = controller->plot->qchart->mapToPosition(line.p1(), curve);
        QPointF to = controller->plot->qchart->mapToPosition(line.p2(), curve);

        painter->drawLine(from, to);
    }
}
