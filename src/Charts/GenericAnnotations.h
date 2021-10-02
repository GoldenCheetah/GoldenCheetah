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

#ifndef _GC_GenericAnnotation_h
#define _GC_GenericAnnotation_h 1

#include <QPointF>
#include <QtCharts>
#include <QGraphicsItem>

class GenericPlot;

// base class for a chart annotation- you have one job
class GenericAnnotation
{
    public:
        // annotation controller calls this to paint
        virtual void paint(QPainter*, const QStyleOptionGraphicsItem *, QWidget*)=0;
};

// gets attached to the chart and is used to register
// annotations, and calls them to paint on the plot
class GenericAnnotationController : public QGraphicsItem
{
    Q_INTERFACES(QGraphicsItem)

    public:

        GenericAnnotationController(GenericPlot *plot);
        ~GenericAnnotationController() {}

        void paint(QPainter*p, const QStyleOptionGraphicsItem *o, QWidget*w);

        // add remove annotations
        void addAnnotation(GenericAnnotation* item);
        void removeAnnotation(GenericAnnotation *item);

        // always the entire plotarea
        QRectF boundingRect() const;

        // make it available to annotations
        GenericPlot *plot;

        QList<GenericAnnotation *> annotations;
};

// for painting lines on a chart- used by voronoi
// could extend to painting any shapes on a chart
class GenericLines : public GenericAnnotation
{
    public:

        GenericLines(GenericAnnotationController *);
        ~GenericLines();

        void setCurve(QAbstractSeries *curve) { this->curve=curve; }
        void setLines(QList<QLineF> lines) { this->lines = lines; controller->update(); }

        void paint(QPainter*, const QStyleOptionGraphicsItem *, QWidget*);

    private:

        // lines to draw
        QList<QLineF> lines;
        QAbstractSeries *curve;

        GenericAnnotationController *controller;
};

#endif
