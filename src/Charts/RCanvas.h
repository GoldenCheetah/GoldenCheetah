/*
 * Copyright (c) 2016 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_RCanvas_h
#define _GC_RCanvas_h 1

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QPolygonF>
#include <QVector>
#include <QWheelEvent>

class Context;

// the chart
class RCanvas : public QGraphicsView {

    Q_OBJECT

    public:
        RCanvas(Context *, QWidget *parent);

    public slots:
        void configChanged(qint32);

        // graphic events
        void newPage();
        void circle(double x, double y, double r, QPen, QBrush);
        void line(double x1, double y1, double x2, double y2, QPen);
        void polyline(int n, double *x, double *y, QPen);
        void polygon(int n, double *x, double *y, QPen, QBrush);
        void rectangle(double x0, double y0, double x1, double y1,QPen,QBrush);
        void text(double x, double y, QString s, double rot, double hadj, QPen p, QFont f);

    protected:
        QGraphicsScene *scene;
        void wheelEvent(QWheelEvent *event);

    private:
        QWidget *parent;
        Context *context;
};


#endif
