/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#include "WorkoutWidgetItems.h"
#include "Context.h"
#include "Athlete.h"
#include "Zones.h"
#include "Colors.h"

#include <QDebug>

static int POWERSCALEWIDTH = 5;
static int WBALSCALEWIDTH = 5;
static bool GRIDLINES = true;
static int SPACING = 4;

WWPowerScale::WWPowerScale(WorkoutWidget *w, Context *c) : WorkoutWidgetItem(w), context(c)
{
    w->addItem(this);
}

void
WWPowerScale::paint(QPainter *painter)
{
    int rnum = -1;

    // CP etc are not available so draw nothing
    if (context->athlete->zones(false) == NULL || (rnum = context->athlete->zones(false)->whichRange(QDate::currentDate())) == -1) return;

    // lets get the zones, CP and PMAX
    int CP = context->athlete->zones(false)->getCP(rnum);
    int Pmax = context->athlete->zones(false)->getPmax(rnum);
    int numZones = context->athlete->zones(false)->numZones(rnum);

    QFontMetrics fontMetrics(workoutWidget()->markerFont);
    QFontMetrics bfontMetrics(workoutWidget()->bigFont);

    for(int i=0; i<numZones; i++) {

        // zoneinfo here
        QString name, description;
        int low, high;

        // get zoneinfo  for a given range and zone
        context->athlete->zones(false)->zoneInfo(rnum, i, name, description, low, high);

        // draw coordinates
        int ylow=workoutWidget()->transform(0,low).y();
        int yhigh=workoutWidget()->transform(0,high).y();

        // stay within left block
        if (yhigh < workoutWidget()->canvas().y()) yhigh = workoutWidget()->canvas().y();
        int ymid = (yhigh + ylow) / 2;

        // bounding rect for zone color
        QRect bound(QPoint(workoutWidget()->left().topRight().x()-POWERSCALEWIDTH, yhigh), 
                    QPoint(workoutWidget()->left().topRight().x(), ylow));

        // draw rect
        painter->fillRect(bound, QBrush(zoneColor(i,numZones)));

        // HIGH % LABELS
        if (i<(numZones-1)) {

            // percent labels for high only - but skip the last zone its off to infinity..
            QString label = QString("%1%").arg(int(double(high) / double(CP) * 100.00f));
            QRect textBound = fontMetrics.boundingRect(label);
            painter->setFont(workoutWidget()->markerFont);
            painter->setPen(workoutWidget()->markerPen);

            painter->drawText(QPoint(workoutWidget()->left().right()-SPACING-textBound.width()-POWERSCALEWIDTH,
                                        yhigh+(fontMetrics.ascent()/2)), // we use ascent not height to line up numbers
                                        label);
        }

        // if we have gridlines enabled and don't paint on canvas border
        if (GRIDLINES && yhigh > workoutWidget()->canvas().y()) {

            QColor color(zoneColor(i,numZones));
            color.setAlpha(128);
            painter->setPen(QPen(color));

            // zone high
            painter->drawLine(QPoint(workoutWidget()->canvas().x(), yhigh), 
                             QPoint(workoutWidget()->canvas().x()+workoutWidget()->canvas().width(), yhigh));

            // ZONE LABELS MID CANVAS AND FEINT
            QRect boundText = bfontMetrics.boundingRect(name);
            painter->setFont(workoutWidget()->bigFont);

            painter->drawText(QPoint(workoutWidget()->canvas().center().x() - (boundText.width()/2),
                                    ymid+(bfontMetrics.ascent()/2)), // we use ascent not height to line up numbers
                                    name);
        }
    }
}

WWWBalScale::WWWBalScale(WorkoutWidget *w, Context *c) : WorkoutWidgetItem(w), context(c)
{
    w->addItem(this);
}

void
WWWBalScale::paint(QPainter *painter)
{
    int rnum = -1;

    // CP etc are not available so draw nothing
    if (context->athlete->zones(false) == NULL || (rnum = context->athlete->zones(false)->whichRange(QDate::currentDate())) == -1) return;

    // lets get the zones, CP and PMAX
    int WPRIME = context->athlete->zones(false)->getWprime(rnum);

    QFontMetrics fontMetrics(workoutWidget()->markerFont);
    QFontMetrics bfontMetrics(workoutWidget()->bigFont);

    // wprime zones are simply 25% 50% 75% 100%
    for(int i=0; i<4; i++) {

        // scale height
        int height = workoutWidget()->canvas().height();

        // bounding rect for zone color
        QPointF tl = workoutWidget()->right().topLeft();
        QRect bound(QPoint(tl.x(), tl.y() + (i * (height/4))), 
                    QPoint(tl.x()+WBALSCALEWIDTH, tl.y() + ((i+1) * (height/4))));

        // draw rect
        QColor wbal = GColor(CWBAL);
        wbal.setAlpha((255/4) * (i+1));
        painter->fillRect(bound, QBrush(wbal));

        // HIGH LABEL % (100,75,50,25)

        // percent labels for high only - but skip the last zone its off to infinity..
        QString label = QString("%1%").arg(100-(i*25));
        QRect textBound = fontMetrics.boundingRect(label);
        painter->setFont(workoutWidget()->markerFont);
        painter->setPen(workoutWidget()->markerPen);

        painter->drawText(QPoint(tl.x()+SPACING+POWERSCALEWIDTH,
                                 tl.y() + (i * (height/4)) +(fontMetrics.ascent()/2)),
                                 label);

        // absolute labels for high WPRIME - (i*WPRIME/4)
        label = QString("%1 kJ").arg(double((WPRIME - (i*(WPRIME/4)))/1000.00f), 0, 'f', 1);
        textBound = fontMetrics.boundingRect(label);

        painter->drawText(QPoint(tl.x()-SPACING-textBound.width(),
                                 tl.y() + (i * (height/4)) +(fontMetrics.ascent()/2)),
                                 label);

    }
}
void
WWPoint::paint(QPainter *painter)
{
    painter->setPen(Qt::NoPen);

    // transform
    QPoint center = workoutWidget()->transform(x,y);

    // highlight hovered
    if (hover) {
        painter->setBrush(Qt::gray); 
        painter->drawEllipse(QPointF(center.x(), center.y()), 10.0f, 10.0f);
    }

    // selected!
    if (selecting || selected) {

        // selected change color
        painter->setBrush(Qt::red); 
        painter->drawEllipse(QPointF(center.x(), center.y()), 3.0f, 3.0f);

    } else {

        // draw point
        painter->setBrush(GColor(CPOWER));
        painter->drawEllipse(QPointF(center.x(), center.y()), 3.0f, 3.0f);
    }

    // set bound so we can be moused over etc
    bound = QRectF(QPointF(center.x()-3.0f, center.y()-3.0f),QPointF(center.x()+3.0f, center.y()+3.0f));
}

void
WWLine::paint(QPainter *painter)
{
    // thin ?
    QPen linePen(GColor(CPOWER));
    linePen.setWidth(1);
    painter->setPen(linePen);

    QPoint origin = workoutWidget()->transform(0,0);
    QPainterPath path(QPointF(origin.x(), origin.y()));

    // join the dots and make a path as you go
    QPointF last(0,0);

    foreach(WWPoint *p, workoutWidget()->points()) {

        // might be better to always use float?
        QPoint center = workoutWidget()->transform(p->x,p->y);
        QPointF dot(center.x(), center.y());

        // path...
        if (last.x() && last.y()) painter->drawLine(last, dot);
        path.lineTo(dot);

        // moving on
        last = dot;
    }

    // fill beneath
    if (last.x() && last.y()) {

        // drop down to baseline then back to origin
        path.lineTo(QPointF(last.x(), origin.y()));
        path.lineTo(origin);

        // now fill
        painter->setPen(Qt::NoPen);

            QColor brush_color1 = QColor(Qt::blue);
            brush_color1.setAlpha(200);
            QColor brush_color2 = QColor(Qt::blue);
            brush_color2.setAlpha(64);

            QLinearGradient linearGradient(0, 0, 0, workoutWidget()->transform(0,0).y());
            linearGradient.setColorAt(0.0, brush_color1);
            linearGradient.setColorAt(1.0, brush_color2);
            linearGradient.setSpread(QGradient::PadSpread);

            painter->fillPath(path, QBrush(linearGradient));
    }
}

void
WWRect::paint(QPainter *painter)
{
    QPointF onRect = workoutWidget()->onRect;
    QPointF atRect = workoutWidget()->atRect;

    // draw a selection rectangle
    if (onRect != QPointF(-1,-1) && atRect != QPointF(-1,-1) && onRect != atRect) {

        // thin ?
        QPen linePen(GColor(CPLOTMARKER));
        linePen.setWidth(1);
        painter->setPen(linePen);

        painter->drawRect(QRectF(onRect,atRect));
    }
}

// block cursos
void
WWBlockCursor::paint(QPainter *painter)
{
    // is the block cursor valid?
    if (workoutWidget()->cursorBlock != QPainterPath()) {

        QColor brush_color1 = GColor(CPLOTMARKER);
        brush_color1.setAlpha(240);
        QColor brush_color2 = GColor(CPLOTMARKER);
        brush_color2.setAlpha(128);

        QLinearGradient linearGradient(0, 0, 0, workoutWidget()->transform(0,0).y());
        linearGradient.setColorAt(0.0, brush_color1);
        linearGradient.setColorAt(1.0, brush_color2);
        linearGradient.setSpread(QGradient::PadSpread);

        painter->fillPath(workoutWidget()->cursorBlock, QBrush(linearGradient));
    }
}

//W'bal curve paint
void
WWWBLine::paint(QPainter *painter)
{
    int rnum = -1;

    // CP etc are not available so draw nothing
    if (context->athlete->zones(false) == NULL || (rnum = context->athlete->zones(false)->whichRange(QDate::currentDate())) == -1) return;

    // lets get the zones, CP and PMAX
    int WPRIME = context->athlete->zones(false)->getWprime(rnum);

    // thin ?
    QPen linePen(GColor(CWBAL));
    linePen.setWidth(1);
    painter->setPen(linePen);

    // top left origin
    QPointF tl = workoutWidget()->canvas().topLeft();

    // pixels per WPRIME value
    double ratio = workoutWidget()->canvas().height() / WPRIME;

    // join the dots 
    QPointF last(tl.x(),tl.y());

    // run through the wpBal values...
    int secs=0;
    foreach(double y, workoutWidget()->wprime().ydata()) {

        // next second
        secs++;

        // this dot...
        if (y < 0) y=0;

        // x and y pixel location
        double px = workoutWidget()->transform(secs,0).x();
        double py = tl.y() + ((WPRIME-y) * ratio);

        QPointF dot(px,py);
        painter->drawLine(last, dot);
        last = dot;
    }
}

//
// COMMANDS
//

// Create a new point
CreatePointCommand::CreatePointCommand(WorkoutWidget *w, double x, double y, int index)
 : WorkoutWidgetCommand(w), x(x), y(y), index(index) { }

// undo create point
void
CreatePointCommand::undo()
{
    // wipe it from the array
    WWPoint *p = NULL;
    if (index >= 0) p = workoutWidget()->points().takeAt(index);
    else p = workoutWidget()->points().takeAt(workoutWidget()->points().count()-1);
    delete p;
}

// do it again
void
CreatePointCommand::redo()
{
    // create a new WWPoint
    WWPoint *p = new WWPoint(workoutWidget(), x,y, false);

    // -1 means append
    if (index < 0)
        workoutWidget()->points().append(p);
    else
        workoutWidget()->points().insert(index, p);
}

MovePointsCommand::MovePointsCommand(WorkoutWidget *w, QList<PointMemento> before, QList<PointMemento> after)
 : WorkoutWidgetCommand(w), before(before), after(after) { }

void
MovePointsCommand::undo()
{
    foreach(PointMemento m, before) {
        WWPoint *p = workoutWidget()->points()[m.index];
        p->x = m.x;
        p->y = m.y;
    }
}

void
MovePointsCommand::redo()
{
    foreach(PointMemento m, after) {
        WWPoint *p = workoutWidget()->points()[m.index];
        p->x = m.x;
        p->y = m.y;
    }
}

MovePointCommand::MovePointCommand(WorkoutWidget *w, QPointF before, QPointF after, int index)
 : WorkoutWidgetCommand(w), before(before), after(after), index(index) { }

void
MovePointCommand::undo()
{
    WWPoint *p = workoutWidget()->points()[index];

    p->x = before.x();
    p->y = before.y();
}

void
MovePointCommand::redo()
{
    WWPoint *p = workoutWidget()->points()[index];

    p->x = after.x();
    p->y = after.y();
}

ScaleCommand::ScaleCommand(WorkoutWidget *w, double up, double down, bool scaleup)
  : WorkoutWidgetCommand(w), up(up), down(down), scaleup(scaleup) { }

void
ScaleCommand::undo()
{
    double factor = scaleup ? down : up;
    foreach(WWPoint *p, workoutWidget()->points())
        p->y *= factor;
}

void
ScaleCommand::redo()
{
    double factor = scaleup ? up : down;
    foreach(WWPoint *p, workoutWidget()->points())
        p->y *= factor;
}

DeleteWPointsCommand::DeleteWPointsCommand(WorkoutWidget*w, QList<PointMemento>points)
  : WorkoutWidgetCommand(w), points(points) { }

void 
DeleteWPointsCommand::redo()
{
    // delete backward
    for (int i=points.count()-1; i>=0; i--) {
        PointMemento m = points[i];
        WWPoint *rm = workoutWidget()->points().takeAt(m.index);
        delete rm;
    }
}

void
DeleteWPointsCommand::undo()
{
    // add forward
    foreach(PointMemento m, points) {
        WWPoint *add = new WWPoint(workoutWidget(), m.x, m.y, false);
        workoutWidget()->points().insert(m.index, add);
    }
}
