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

// utils for mucking about with qbastractaxis
static double qtchartaxismin(QAbstractAxis *axis)
{
    double returning;

    switch (axis->type()) {
    case QAbstractAxis::AxisTypeValue: returning = static_cast<QValueAxis*>(axis)->min(); break;
    case QAbstractAxis::AxisTypeDateTime: returning = static_cast<QDateTimeAxis*>(axis)->min().toMSecsSinceEpoch(); break;
    case QAbstractAxis::AxisTypeLogValue: returning = static_cast<QLogValueAxis*>(axis)->min(); break;
    default: returning=-9999;
    }

    return returning;
}

static double qtchartaxismax(QAbstractAxis *axis)
{
    double returning;

    switch (axis->type()) {
    case QAbstractAxis::AxisTypeValue: returning = static_cast<QValueAxis*>(axis)->max(); break;
    case QAbstractAxis::AxisTypeDateTime: returning = static_cast<QDateTimeAxis*>(axis)->max().toMSecsSinceEpoch(); break;
    case QAbstractAxis::AxisTypeLogValue: returning = static_cast<QLogValueAxis*>(axis)->max(); break;
    default: returning=9999;
    }

    return returning;
}
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
// lines painter
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


//
// straight line with text painter
//
StraightLine::StraightLine(GenericAnnotationController *controller) : curve(NULL), controller(controller) {}
StraightLine::~StraightLine() {}

void
StraightLine::paint(QPainter*painter, const QStyleOptionGraphicsItem *, QWidget*)
{
    if (curve == NULL) return;

    QPen pen(Qt::lightGray);
    pen.setStyle(style);
    painter->setPen(pen);

    painter->setClipRect(controller->plot->qchart->plotArea());

    double min=0, max=0;
    // get min/max from the other axis
    foreach(QAbstractAxis *axis, curve->attachedAxes()) {
        if (axis->orientation() == orientation) {
            // set min and max
            min = qtchartaxismin(axis);
            max = qtchartaxismax(axis);
        }
    }

    // draw a line from min to max at value on our axis
    QPointF p1, p2;
    if (orientation == Qt::Vertical) {
        p1 = QPointF(value,min);
        p2 = QPointF(value,max);
    } else {
        p1 = QPointF(min, value);
        p2 = QPointF(max, value);
    }

    QPointF from = controller->plot->qchart->mapToPosition(p1, curve);
    QPointF to = controller->plot->qchart->mapToPosition(p2, curve);


    QFont font; // default font size
    painter->setFont(font);
    QFontMetrics fm(font);
    double height = fm.boundingRect(text).height() + 4;

    painter->drawText(orientation == Qt::Horizontal ? from + QPointF(4,-4) : to + QPointF(4, height), text);

    pen.setColor(GColor(CPLOTMARKER));
    painter->setPen(pen);
    painter->drawLine(from, to);
}