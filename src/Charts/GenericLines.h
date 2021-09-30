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

#ifndef _GC_GenericLines_h
#define _GC_GenericLines_h 1

#include <QPointF>
#include <QtCharts>
#include <QGraphicsItem>

#include "GenericPlot.h"

// for painting lines on a chart
class GenericLines : public QGraphicsItem
{

    Q_INTERFACES(QGraphicsItem)

    public:

        GenericLines(GenericPlot *);
        ~GenericLines();

        void setCurve(QAbstractSeries *curve) { this->curve=curve; }
        void setLines(QList<QLineF> lines) { this->lines = lines; update(); }

        void paint(QPainter*, const QStyleOptionGraphicsItem *, QWidget*);
        QRectF boundingRect() const;

        void prepare();
    private:

        // lines to draw
        QList<QLineF> lines;
        GenericPlot *plot;

        QAbstractSeries *curve;

};

#endif
