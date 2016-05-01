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

#include "RCanvas.h"

#include "Context.h"
#include "Colors.h"
#include "TabView.h"

#include <QGLWidget>

RCanvas::RCanvas(Context *context, QWidget *parent) : QGraphicsView(parent), context(context)
{
    // no frame, its ugly
    setFrameStyle(QFrame::NoFrame);

    // use OpenGL rendering for better performance -- disabled till configurable in settings
#ifndef WIN32
    setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers | QGL::DirectRendering)));
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
#endif

    // add a scene
    scene = new QGraphicsScene(this);
    this->setScene(scene);

    // flip horizontal as y-co-ordinates place the
    // origin in the bottom left, not screen co-ords
    // of top left. x axis is fine though.
    scale(1,-1);

    // turn on antialiasing too
    setRenderHint(QPainter::Antialiasing, true);

    // set colors etc
    configChanged(CONFIG_APPEARANCE);

    // connect
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
}

void
RCanvas::configChanged(qint32)
{
    // set background etc to the prevailing defaults
    QPalette p = palette();
    p.setColor(QPalette::Base, GColor(CPLOTBACKGROUND));
    p.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    setPalette(p);
    setStyleSheet(TabView::ourStyleSheet());
}

void
RCanvas::wheelEvent(QWheelEvent *event){

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    // Scale the view / do the zoom
    double scaleFactor = 1.15;
    if(event->delta() > 0) {
        // Zoom in
        scale(scaleFactor, scaleFactor);

    } else {
        // Zooming out
         scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
}

void
RCanvas::newPage()
{
    scene->clear();
}

void
RCanvas::circle(double x, double y, double r, QPen p, QBrush b)
{
    QGraphicsEllipseItem *c=new QGraphicsEllipseItem(x,y,r+r,r+r);
    c->setPen(p);
    c->setBrush(b);
    scene->addItem(c);
}

void
RCanvas::line(double x1, double y1, double x2, double y2, QPen p)
{
    QGraphicsLineItem *l=new QGraphicsLineItem(x1,y1,x2,y2);
    l->setPen(p);
    scene->addItem(l);
}

void
RCanvas::polyline(int n, double *x, double *y, QPen p)
{
    // origin
    double x0=x[0], y0=y[0];

    // create a line per path
    for(int i=1; i<n;i++) {

        double x1=x[i], y1=y[i];
        line(x0,y0,x1,y1,p);

        x0=x1;
        y0=y1;
    }
}

void
RCanvas::polygon(int n, double *x, double *y, QPen p, QBrush b)
{
    QVector<QPointF> points(n);

    // create a line per path
    for(int i=0; i<n;i++)  points[i] = QPointF(x[i],y[i]);

    QGraphicsPolygonItem *g= new QGraphicsPolygonItem(QPolygonF(points),0);
    g->setPen(p);
    g->setBrush(b);
    scene->addItem(g);
}

void
RCanvas::rectangle(double x0, double y0, double x1, double y1,QPen p, QBrush b)
{
    QGraphicsRectItem *r=new QGraphicsRectItem(x0,y0,x1-x0,y1-y0);
    r->setPen(p);
    r->setBrush(b);
    scene->addItem(r);
}

void
RCanvas::text(double x, double y, QString s, double rot, double hadj, QPen p, QFont f)
{
    QGraphicsTextItem *t = new QGraphicsTextItem(s);
    t->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    t->setFont(f);
    t->setPos(0,0);
    t->setRotation(rot);
    t->setDefaultTextColor(p.color());

    // add to group
    QList<QGraphicsItem*> texts;
    texts<<t;
    QGraphicsItemGroup *grp = scene->createItemGroup(texts);

    // add height too
    QFontMetrics fm(f);
    grp->setPos(x,y+fm.height());
}
