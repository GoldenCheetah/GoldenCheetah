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
#include "AbstractView.h"

#include <QGLWidget>

//
//
// NOTE:                       X/Y CO-ORDINATES
//
//         R co-ordinates place the origin in the bottom-left as one would
//         expect when plotting. QGraphicsScene places the origin in the
//         top-left as one would expect when painting.
//
//         As a result the y co-ordinates need to be transformed from R
//         to painter co-ordinates -- we DO NOT do this with a global
//         world co-ordinate transformation -- this is because whilst it
//         flips co-ordinates it also flips text (!)
//
//         We transform y co-ordinates in the primitive functions that
//         draw circles, lines etc.
//
//         We also need to adjust text co-ordinates since they use painting
//         conventions (top-left of text) but don't use painting co-ordinates
//
//

RCanvas::RCanvas(Context *context, QWidget *parent) : QGraphicsView(parent), context(context)
{
    // no frame, its ugly
    setFrameStyle(QFrame::NoFrame);

#ifdef Q_OS_LINUX // mac and windows both have issues. sigh.
    // Enabled on Linux depending on Open GL version
    if (QGLFormat::openGLVersionFlags().testFlag(QGLFormat::OpenGL_Version_2_0)) {
        setViewport(new QGLWidget( QGLFormat(QGL::SampleBuffers)));
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    }
#endif

    // allow to click and drag
    setDragMode(QGraphicsView::ScrollHandDrag);

    // add a scene
    scene = new QGraphicsScene(this);
    this->setScene(scene);

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
    // set the background
    setBackgroundBrush(QBrush(GColor(CPLOTBACKGROUND)));

    // set background etc to the prevailing defaults
    QPalette p = palette();
    p.setColor(QPalette::Base, GColor(CPLOTBACKGROUND));
    p.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    setPalette(p);
    setStyleSheet(AbstractView::ourStyleSheet());
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
    y =-y;

    QGraphicsEllipseItem *c=new QGraphicsEllipseItem(x-r,y-r,r+r,r+r);
    c->setPen(p);
    c->setBrush(b);
    scene->addItem(c);
}

void
RCanvas::line(double x1, double y1, double x2, double y2, QPen p)
{
    y1 =-y1;
    y2 =-y2;


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

        double x1=x[i], y1=y[i]; // no transform since done by line !
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
    for(int i=0; i<n;i++)  points[i] = QPointF(x[i],-y[i]);

    QGraphicsPolygonItem *g= new QGraphicsPolygonItem(QPolygonF(points),0);
    g->setPen(p);
    g->setBrush(b);
    scene->addItem(g);
}

void
RCanvas::rectangle(double x0, double y0, double x1, double y1,QPen p, QBrush b)
{
    QGraphicsRectItem *r=new QGraphicsRectItem(x0,-y0,x1-x0,-(y1-y0));
    r->setPen(p);
    r->setBrush(b);
    scene->addItem(r);
}

void
RCanvas::text(double x, double y, QString s, double rot, double , QPen p, QFont f)
{
    // add height too
    QFontMetrics fm(f);

    QGraphicsTextItem *t = new QGraphicsTextItem(s);
    t->setFont(f);
    t->setPos(x,-y-fm.height());
    t->setRotation(rot);
    t->setDefaultTextColor(p.color());

    scene->addItem(t);

}
