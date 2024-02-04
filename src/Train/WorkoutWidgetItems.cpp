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

#define POWERSCALEWIDTH (5 *dpiXFactor)
#define WBALSCALEWIDTH (5 *dpiXFactor)
static bool GRIDLINES = true;
#define SPACING (4 *dpiXFactor)

static int MINTOOLHEIGHT = 350; // don't do lots of decoration on "mini" view

WWPowerScale::WWPowerScale(WorkoutWidget *w, Context *c) : WorkoutWidgetItem(w), context(c)
{
    w->addItem(this);
}

void
WWPowerScale::paint(QPainter *painter)
{
    // if too small paint nothing
    if (workoutWidget()->height() < MINTOOLHEIGHT) return;

    // fill transparent to deminish "over painted" curves
    // that are shown in borders to give sense of scroll
    QColor deminish(GColor(GCol::TRAINPLOTBACKGROUND));
    deminish.setAlpha(175);
    painter->fillRect(workoutWidget()->left(), deminish);

    int rnum = -1;

    // CP etc are not available so draw nothing
    if (context->athlete->zones("Bike") == NULL || (rnum = context->athlete->zones("Bike")->whichRange(QDate::currentDate())) == -1) return;

    // lets get the zones, CP and PMAX
    int CP = context->athlete->zones("Bike")->getCP(rnum);
    int Pmax = context->athlete->zones("Bike")->getPmax(rnum);
    Q_UNUSED(Pmax); // for now ........
    int numZones = context->athlete->zones("Bike")->numZones(rnum);

    QFontMetrics fontMetrics(workoutWidget()->markerFont);
    QFontMetrics bfontMetrics(workoutWidget()->bigFont);

    for(int i=0; i<numZones; i++) {

        // zoneinfo here
        QString name, description;
        int low, high;

        // get zoneinfo  for a given range and zone
        context->athlete->zones("Bike")->zoneInfo(rnum, i, name, description, low, high);

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

    // CP !
    QPen cppen(GColor(GCol::PLOTMARKER));
    cppen.setStyle(Qt::DashLine);
    painter->setPen(cppen);

    double CPy = workoutWidget()->transform(0, CP).y();

    // zone high
    painter->drawLine(QPoint(workoutWidget()->canvas().x(), CPy),
                      QPoint(workoutWidget()->canvas().x()+workoutWidget()->canvas().width(), CPy));

}

WWWBalScale::WWWBalScale(WorkoutWidget *w, Context *c) : WorkoutWidgetItem(w), context(c)
{
    w->addItem(this);
}

void
WWWBalScale::paint(QPainter *painter)
{
    // if too small paint nothing
    if (workoutWidget()->height() < MINTOOLHEIGHT) return;

    // fill transparent to deminish "over painted" curves
    // that are shown in borders to give sense of scroll
    QColor deminish(GColor(GCol::TRAINPLOTBACKGROUND));
    deminish.setAlpha(175);
    painter->fillRect(workoutWidget()->right(), deminish);

    int rnum = -1;

    // CP etc are not available so draw nothing
    if (context->athlete->zones("Bike") == NULL || (rnum = context->athlete->zones("Bike")->whichRange(QDate::currentDate())) == -1) return;

    // lets get the zones, CP and PMAX
    int WPRIME = context->athlete->zones("Bike")->getWprime(rnum);

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
        QColor wbal = GColor(GCol::WBAL);
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

// the warning bar at bottom for TTE efforts
void
WWTTE::paint(QPainter *painter)
{
    // don't show when recording
    if (workoutWidget()->recording()) return;

    QRectF gap = workoutWidget()->bottomgap();

    // if no efforts/TTE found then its green by default
    // but just way too colorful for my eyes
    //XXX QRectF all(gap.left(), gap.top(), gap.width(), 2);
    //XXX painter->fillRect(all, QBrush(Qt::green));

    foreach(WWEffort m, workoutWidget()->efforts) {

        // top left of rectangle
        QPoint tl = workoutWidget()->transform(m.start, 0);
        tl.setY(gap.top());

        QPoint br = workoutWidget()->transform(m.start + m.duration, 0);
        br.setY(gap.top()+2); // thin line

        // paint a red rectange
        if (m.quality >= 1.00f)
            painter->fillRect(QRect(tl,br), QBrush(Qt::red));
        else
            painter->fillRect(QRect(tl,br), QBrush(QColor(255,152,0)));
    }
}

// lap markers
void
WWLap::paint(QPainter *painter)
{
    if (workoutWidget()->laps().count() == 0) return;

    QRectF top = workoutWidget()->top();

    QFontMetrics fontMetrics(workoutWidget()->bigFont);
    painter->setFont(workoutWidget()->bigFont);
    painter->setPen(GColor(GCol::PLOTMARKER));

    for(int i=0; i<workoutWidget()->laps().count(); i++) {

        QString name = QString("%1").arg(workoutWidget()->laps()[i].LapNum);

        // where to paint?
        QPointF here = workoutWidget()->transform(workoutWidget()->laps()[i].x/1000.0f, 0);
        here.setY(top.top() + fontMetrics.height());

        // end ..
        int end = workoutWidget()->maxWX();
        if (i < workoutWidget()->laps().count()-1) end=workoutWidget()->laps()[i+1].x/1000.f;
        double endx = workoutWidget()->transform(end,0).x();

        // red rectangle for selected intervals
        if (workoutWidget()->laps()[i].selected) {
            QColor color(255,0,0,64); // lighten whatever it is
            painter->fillRect(QRectF(here.x(), top.top(), endx - here.x(), 
                                    top.height()+workoutWidget()->canvas().height()), 
                                    QBrush(color));
        }

        // hover over show full length markers
        painter->drawLine(QPointF(here.x(), top.top()), QPointF(here.x(), top.bottom()));

        // and the text
        painter->drawText(here, name);
    }
}

// a dot
void
WWPoint::paint(QPainter *painter)
{
    // transform
    QPoint center = workoutWidget()->transform(x,y);

    // don't show when recording, or not on main canvas
    if (!workoutWidget()->canvas().contains(center) || workoutWidget()->recording()) return;

    // if too small paint nothing
    if (workoutWidget()->height() < MINTOOLHEIGHT) return;

    painter->setPen(Qt::NoPen);

    // highlight hovered
    if (hover) {
        painter->setBrush(Qt::gray);
        painter->drawEllipse(QPointF(center.x(), center.y()), 10.0f*dpiXFactor, 10.0f*dpiXFactor);
    }

    // selected!
    if (selecting || selected) {

        // selected change color
        painter->setBrush(Qt::red);
        painter->drawEllipse(QPointF(center.x(), center.y()), 3.0f*dpiXFactor, 3.0f*dpiXFactor);

    } else {

        // draw point
        painter->setBrush(GColor(GCol::POWER));
        painter->drawEllipse(QPointF(center.x(), center.y()), 3.0f*dpiXFactor, 3.0f*dpiXFactor);
    }

    // set bound so we can be moused over etc
    bound = QRectF(QPointF(center.x()-(3.0f*dpiXFactor), center.y()-(3.0f*dpiXFactor)),QPointF(center.x()+(3.0f*dpiXFactor), center.y()+(3.0f*dpiXFactor)));
}

void
WWLine::paint(QPainter *painter)
{
    // thin ?
    QPen linePen(workoutWidget()->recording() ? GColor(GCol::TPOWER) : GColor(GCol::POWER));
    linePen.setWidth(1 *dpiXFactor);
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

            QColor brush_color1 = QColor(GColor(GCol::TPOWER));
            brush_color1.setAlpha(240);
            QColor brush_color2 = QColor(GColor(GCol::TPOWER));
            brush_color2.setAlpha(200);

            QLinearGradient linearGradient(0, 0, 0, workoutWidget()->transform(0,0).y());
            linearGradient.setColorAt(0.0, brush_color1);
            linearGradient.setColorAt(1.0, brush_color2);
            linearGradient.setSpread(QGradient::PadSpread);

            painter->fillPath(path, QBrush(linearGradient));
    }
}

void
WWTelemetry::paint(QPainter *painter)
{
    // only when recording
    if (!workoutWidget()->recording()) return;

    // Draw POWER
    if (workoutWidget()->shouldPlotPwr())
    {
        updateAvg(workoutWidget()->watts, workoutWidget()->pwrAvg, workoutWidget()->pwrPlotAvgLength());
        paintSampleList(painter, GColor(GCol::POWER), workoutWidget()->pwrAvg, WorkoutWidget::POWER);
    }

    // Draw HR
    if (workoutWidget()->shouldPlotHr())
    {
        updateAvg(workoutWidget()->hr, workoutWidget()->hrAvg, workoutWidget()->hrPlotAvgLength());
        paintSampleList(painter, GColor(GCol::HEARTRATE), workoutWidget()->hrAvg, WorkoutWidget::HEARTRATE);
    }
    // Draw Speed
    if (workoutWidget()->shouldPlotSpeed())
    {
        updateAvg(workoutWidget()->speed, workoutWidget()->speedAvg, workoutWidget()->speedPlotAvgLength());
        paintSampleList(painter, GColor(GCol::SPEED), workoutWidget()->speedAvg, WorkoutWidget::SPEED);
    }
    // Draw Cadence
    if (workoutWidget()->shouldPlotCadence())
    {
        updateAvg(workoutWidget()->cadence, workoutWidget()->cadenceAvg, workoutWidget()->cadencePlotAvgLength());
        paintSampleList(painter, GColor(GCol::CADENCE), workoutWidget()->cadenceAvg, WorkoutWidget::CADENCE);
    }
    // Draw VO2
    if (workoutWidget()->shouldPlotVo2())
    {
        updateAvg(workoutWidget()->vo2, workoutWidget()->vo2Avg, workoutWidget()->vo2PlotAvgLength());
        paintSampleList(painter, GColor(GCol::VO2), workoutWidget()->vo2Avg, WorkoutWidget::VO2);
    }
    // Draw Ventilation
    if (workoutWidget()->shouldPlotVentilation())
    {
        updateAvg(workoutWidget()->ventilation, workoutWidget()->ventilationAvg, workoutWidget()->ventilationPlotAvgLength());
        paintSampleList(painter, GColor(GCol::VENTILATION), workoutWidget()->ventilationAvg, WorkoutWidget::VENTILATION);
    }

    //
    // W'bal last, if not zones return
    //
    if (workoutWidget()->shouldPlotWbal())
    {
        // lets get the zones, CP and PMAX, if none we're done
        int rnum = -1;

        // CP etc are not available so draw nothing
        if (context->athlete->zones("Bike") == NULL ||
           (rnum = context->athlete->zones("Bike")->whichRange(QDate::currentDate())) == -1) return;

        // lets get the zones, CP and PMAX
        int WPRIME = context->athlete->zones("Bike")->getWprime(rnum);

        // full color
        QColor color = GColor(GCol::WBAL);
        QPen wlinePen(color);
        wlinePen.setWidth(1 *dpiXFactor);
        painter->setPen(wlinePen);

        // top left origin
        QPointF tl = workoutWidget()->canvas().topLeft();

        // pixels per WPRIME value
        double ratio = workoutWidget()->canvas().height() / WPRIME;

        // join the dots
        QPointF last = QPointF(tl.x(),tl.y());

        // run through the wpBal values...
        int index = 0;
        foreach(int b, workoutWidget()->wbal) {
            // this dot...
            if (b < 0) b=0;

            // x and y pixel location
            double now = workoutWidget()->sampleTimes.at(index) / 1000.0f;
            double px = workoutWidget()->transform(now, 0).x();
            double py = tl.y() + ((WPRIME-b) * ratio);

            QPointF dot(px,py);
            painter->drawLine(last, dot);
            last = dot;

            // next sample
            index++;
        }
    }
}

void
WWRect::paint(QPainter *painter)
{
    // don't show when recording
    if (workoutWidget()->recording()) return;

    QPointF onRect = workoutWidget()->onRect;
    QPointF atRect = workoutWidget()->atRect;

    // draw a selection rectangle
    if (onRect != QPointF(-1,-1) && atRect != QPointF(-1,-1) && onRect != atRect) {

        // thin ?
        QPen linePen(GColor(GCol::PLOTMARKER));
        linePen.setWidth(1 *dpiXFactor);
        painter->setPen(linePen);

        painter->drawRect(QRectF(onRect,atRect));
    }
}

// block cursos
void
WWBlockCursor::paint(QPainter *painter)
{
    // don't show when recording
    if (workoutWidget()->recording()) return;

    // if were in a selection block don't draw a cursos
    if (workoutWidget()->selectionBlock.contains(workoutWidget()->mapFromGlobal(QCursor::pos()))) return;

    // are we busy resizing and stuff and is the block cursor valid?
    if (workoutWidget()->state == WorkoutWidget::none &&
        workoutWidget()->cursorBlock != QPainterPath()) {

        QColor darken = Qt::black;
        darken.setAlpha(125);
        painter->fillPath(workoutWidget()->cursorBlock, QBrush(darken));

        // cursor block duration text
        QFontMetrics fontMetrics(workoutWidget()->bigFont);
        QRect textBound = fontMetrics.boundingRect(workoutWidget()->cursorBlockText);
        painter->setFont(workoutWidget()->bigFont);
        painter->setPen(GColor(GCol::PLOTMARKER));

        QPointF where(workoutWidget()->cursorBlock.boundingRect().center().x()-(textBound.width()/2),
                      workoutWidget()->cursorBlock.boundingRect().bottom()-10); //XXX 10 is hardcoded space from bottom

        painter->drawText(where, workoutWidget()->cursorBlockText);

        QRect textBound2 = fontMetrics.boundingRect(workoutWidget()->cursorBlockText2);
        QPointF where2(workoutWidget()->cursorBlock.boundingRect().center().x()-(textBound2.width()/2),
                      where.y()-textBound.height());  //XXX 4 is hardcoded space between labels

        painter->drawText(where2, workoutWidget()->cursorBlockText2);

    }
}

void
WWBlockSelection::paint(QPainter *painter)
{
    // don't show when recording
    if (workoutWidget()->recording()) return;

    // draw the block selection
    if (workoutWidget()->selectionBlock == QPainterPath()) return;

    // set pen
    painter->setPen(GColor(GCol::PLOTMARKER));

    // now draw the path
    painter->drawPath(workoutWidget()->selectionBlock);

    // and fill it
    QColor darken = Qt::red;
    darken.setAlpha(125);
    painter->fillPath(workoutWidget()->selectionBlock, QBrush(darken));

    // cursor block duration text
    QFontMetrics fontMetrics(workoutWidget()->bigFont);
    QRect textBound = fontMetrics.boundingRect(workoutWidget()->selectionBlockText);
    painter->setFont(workoutWidget()->bigFont);
    painter->setPen(GColor(GCol::PLOTMARKER));

    QPointF where(workoutWidget()->selectionBlock.boundingRect().center().x()-(textBound.width()/2),
                  workoutWidget()->selectionBlock.boundingRect().bottom()-10); //XXX 10 is hardcoded space from bottom

    painter->drawText(where, workoutWidget()->selectionBlockText);

    QRect textBound2 = fontMetrics.boundingRect(workoutWidget()->selectionBlockText2);
    QPointF where2(workoutWidget()->selectionBlock.boundingRect().center().x()-(textBound2.width()/2),
                  where.y()-textBound.height());  //XXX 4 is hardcoded space between labels

    painter->drawText(where2, workoutWidget()->selectionBlockText2);
}

// locate me on the parent widget in paint coordinates
QRectF
WWBlockSelection::bounding()
{
    return QRectF();
}

//W'bal curve paint
void
WWWBLine::paint(QPainter *painter)
{
    int rnum = -1;

    // CP etc are not available so draw nothing
    if (context->athlete->zones("Bike") == NULL || (rnum = context->athlete->zones("Bike")->whichRange(QDate::currentDate())) == -1) return;

    // lets get the zones, CP and PMAX
    int WPRIME = context->athlete->zones("Bike")->getWprime(rnum);

    // should be translucent if recording, as will be "overwritten"
    // by the actual W'bal value
    QColor color = GColor(GCol::WBAL);
    if (workoutWidget()->recording()) color.setAlpha(64);

    // set pen
    QPen linePen(color);
    linePen.setWidth(1 *dpiXFactor);
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

//MMP Curve
void
WWMMPCurve::paint(QPainter *painter)
{
    // don't show when recording
    if (workoutWidget()->recording()) return;

    // thin ?
    QPen linePen(GColor(GCol::CP));
    linePen.setWidth(1);
    painter->setPen(linePen);

    // join the dots
    QPointF last(-1,-1);

    // run through the wpBal values...
    int secs=0;
    foreach(int watts, workoutWidget()->mmpArray) {

        // skip zero
        if (watts == 0) { secs++; continue; }

        // x and y pixel location
        QPointF point = workoutWidget()->transform(secs,watts);

        // use a log scale?
        if (workoutWidget()->logScale()) point.setX(workoutWidget()->logX(secs));

        if (last.x() >= 0) painter->drawLine(last, point);

        // move on
        last = point;
        secs++;

    }
}

void
WWSmartGuide::paint(QPainter *painter)
{
    // don't show when recording
    if (workoutWidget()->recording()) return;

    // if too small paint nothing
    if (workoutWidget()->height() < MINTOOLHEIGHT) return;

    QPointF tl(-1,-1);
    QPointF br(-1,-1);
    int selected=0;

    //
    // FIND SCOPE TO MARK GUDIES FOR
    //
    // Currently we add guides when items are selected
    // or you are dragging a block or point around
    //

    // If not dragging points or blocks just mark any selected items
    if (workoutWidget()->state == WorkoutWidget::none) {

        // get the boundary of the current selection
        foreach(WWPoint *p, workoutWidget()->points()) {

            // don't show guides whilst selecting, too noisy
            if (!p->selected) continue;

            selected++;

            // top left
            if (tl.x() == -1 || tl.x() > p->x) tl.setX(p->x);
            if (tl.y() == -1 || tl.y() < p->y) tl.setY(p->y);

            // bottom right
            if (br.x() == -1 || br.x() < p->x) br.setX(p->x);
            if (br.y() == -1 || br.y() > p->y) br.setY(p->y);
        }
    }

    // if dragging a block mark it
    if (workoutWidget()->state == WorkoutWidget::dragblock) {

        foreach(PointMemento m, workoutWidget()->cr8block) {

            // how many points
            selected ++;

            // top left
            if (tl.x() == -1 || tl.x() > m.x) tl.setX(m.x);
            if (tl.y() == -1 || tl.y() < m.y) tl.setY(m.y);

            // bottom right
            if (br.x() == -1 || br.x() < m.x) br.setX(m.x);
            if (br.y() == -1 || br.y() > m.y) br.setY(m.y);
        }
    }

    // if dragging a point mark that
    if (workoutWidget()->state == WorkoutWidget::drag && workoutWidget()->dragging) {

        // how many points
        selected=1;

        tl.setX(workoutWidget()->dragging->x);
        br.setX(workoutWidget()->dragging->x);
        tl.setY(workoutWidget()->dragging->y);
        br.setY(workoutWidget()->dragging->y);
    }

    // set the boundary
    QRectF boundary (workoutWidget()->transform(tl.x(), tl.y()),
                     workoutWidget()->transform(br.x(), br.y()));


    QFontMetrics fontMetrics(workoutWidget()->markerFont);
    painter->setFont(workoutWidget()->markerFont);

    //
    // X-AXIS GUIDES
    //
    if (selected > 0) {

        // for now just paint the boundary tics on the x-axis
        QPen linePen(GColor(GCol::PLOTMARKER));
        linePen.setWidthF(0);
        painter->setPen(linePen);

        QRectF bottom = workoutWidget()->bottom();

        // top line width indicators
        painter->drawLine(boundary.bottomLeft().x(), workoutWidget()->bottom().y(),
                          boundary.bottomLeft().x(), workoutWidget()->bottom().y()+bottom.height());

        painter->drawLine(boundary.bottomRight().x(), bottom.y(),
                          boundary.bottomRight().x(), bottom.y()+bottom.height());

        // now the text - but only if we have a gap!
        if (br.x() > tl.x()) {

            // paint time at very bottom of bottom in the middle
            // of the elongated tic marks
            QString text = time_to_string(br.x()-tl.x());
            QRect bound = fontMetrics.boundingRect(text);
            QPoint here(boundary.center().x()-(bound.width()/2), bottom.bottom());

            painter->drawText(here, text);
        }

        // find next and previous points, so we can mark them too
        // this is useful when shifting left and right
        int prev=-1, next=-1;
        for(int i=0; i < workoutWidget()->points().count(); i++) {
            WWPoint *p = workoutWidget()->points()[i];
            if (p->x < tl.x()) prev=i; // to left
            if (p->x > br.x()) {
                next=i;
                break;
            }
        }

        // in seconds
        int left = prev >= 0 ? workoutWidget()->points()[prev]->x : 0;
        int right = next >= 0 ? workoutWidget()->points()[next]->x : workoutWidget()->maxWX();

        // in plot coordinates
        QPointF leftpx = workoutWidget()->transform(left,0);
        QPointF rightpx = workoutWidget()->transform(right,0);
        leftpx.setY(bottom.y());
        rightpx.setY(bottom.y());

        // left indicator
        painter->drawLine(leftpx, leftpx + QPointF(0, bottom.height()));
        painter->drawLine(rightpx, rightpx + QPointF(0, bottom.height()));

        // now the left text - but only if we have a gap!
        if (left < tl.x()) {

            // paint time at very bottom of bottom in the middle
            // of the elongated tic marks
            QString text = time_to_string(tl.x() - left);
            QRect bound = fontMetrics.boundingRect(text);
            QPointF here(leftpx.x() + ((boundary.left() - leftpx.x())/2) - (bound.width()/2), bottom.bottom());

            painter->drawText(here, text);
        }

        // now the right text - but only if we have a gap!
        if (right > br.x()) {

            // paint time at very bottom of bottom in the middle
            // of the elongated tic marks
            QString text = time_to_string(right - br.x());
            QRect bound = fontMetrics.boundingRect(text);
            QPointF here(boundary.right() + ((rightpx.x()-boundary.right())/2) - (bound.width()/2), bottom.bottom());
            painter->drawText(here, text);
        }

#if 0 // doesn't add much
        // side line width indicators
        painter->drawLine(boundary.topLeft()-QPointF(10,0), boundary.topLeft()-QPointF(30,0));
        painter->drawLine(boundary.bottomLeft()-QPointF(10,0), boundary.bottomLeft()-QPointF(30,0));
        painter->drawLine(boundary.bottomLeft()-QPointF(20,0), boundary.topLeft()-QPointF(20,0));
#endif
    }

    //
    // Y-AXIS GUIDES
    //
    if (selected > 0) {

        // for now just paint the boundary tics on the y-axis
        QPen linePen(GColor(GCol::PLOTMARKER));
        linePen.setWidthF(0);
        painter->setPen(linePen);

        // top line width indicators
        painter->drawLine(workoutWidget()->left().left(), boundary.topRight().y(),
                          workoutWidget()->left().right(), boundary.topRight().y());

        painter->drawLine(workoutWidget()->left().left(), boundary.bottomLeft().y(),
                          workoutWidget()->left().right(), boundary.bottomLeft().y());


        // now the texts
        // paint the bottom value
        QString text = QString("%1w").arg(double(tl.y()), 0, 'f', 0);
        QRect bound = fontMetrics.boundingRect(text);
        painter->drawText(workoutWidget()->left().left(), boundary.top()-SPACING, text);
        if (br.y() < tl.y()) {

            // paint the bottom value UNDERNEATH the line, just in case they are really close!
            text = QString("%1w").arg(double(br.y()), 0, 'f', 0);
            bound = fontMetrics.boundingRect(text);
            painter->drawText(workoutWidget()->left().left(), boundary.bottom()+SPACING+fontMetrics.ascent(), text);
        }
    }
}

void
WWNow::paint(QPainter *painter)
{
    // only when recording
    if (!workoutWidget()->recording()) return;

    // get now
    int px = workoutWidget()->transform(context->getNow()/1000.0f,0).x();

    QPen linePen(GColor(GCol::PLOTMARKER));
    linePen.setWidthF(2 *dpiXFactor);
    painter->setPen(linePen);

    // horizontal bar
    painter->drawLine(px, workoutWidget()->canvas().top(), px, workoutWidget()->canvas().bottom());
                      
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

// create a block
CreateBlockCommand::CreateBlockCommand(WorkoutWidget *w, QList<PointMemento>&points)
   : WorkoutWidgetCommand(w), points(points) { }

// undo create block
void
CreateBlockCommand::undo()
{
    // delete the points in reverse
    for (int i=points.count()-1; i>=0; i--) {
        WWPoint *p = NULL;
        if (points[i].index >= 0) p = workoutWidget()->points().takeAt(points[i].index);
        else p = workoutWidget()->points().takeAt(workoutWidget()->points().count()-1);
        delete p;
    }
}

// do it again
void
CreateBlockCommand::redo()
{
    // add the points forward
    foreach(PointMemento m, points) {

        // create a new WWPoint
        WWPoint *p = new WWPoint(workoutWidget(), m.x,m.y, false);

        // -1 means append
        if (m.index < 0)
            workoutWidget()->points().append(p);
        else
            workoutWidget()->points().insert(m.index, p);
        }
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

CutCommand::CutCommand(WorkoutWidget*w, QList<PointMemento>copyIndexes, QList<PointMemento> deleteIndexes, double shift)
  : WorkoutWidgetCommand(w), copyIndexes(copyIndexes), deleteIndexes(deleteIndexes), shift(shift) { }

void
CutCommand::redo()
{
    // delete again
    int last = -1;
    foreach (PointMemento m, deleteIndexes) {
        WWPoint *remove = workoutWidget()->points().takeAt(m.index);
        delete remove;
        last = m.index;
    }

    // shift in
    for (int i=last; i<workoutWidget()->points().count(); i++) {
        workoutWidget()->points()[i]->x -= shift;
    }

    // update clipboard
    workoutWidget()->setClipboard(copyIndexes);
}

void
CutCommand::undo()
{
    // undelete
    int last=-1;
    // re-add backwards
    for (int i=deleteIndexes.count()-1; i >=0; i--) {

        PointMemento m = deleteIndexes[i];
        WWPoint *add = new WWPoint(workoutWidget(), m.x, m.y, false);
        workoutWidget()->points().insert(m.index, add);
        last = m.index;
    }

    // shift out
    for (int i=last+1; i<workoutWidget()->points().count(); i++) {
        workoutWidget()->points()[i]->x += shift;
    }
}

PasteCommand::PasteCommand(WorkoutWidget*w, int here, double offset, double shift, QList<PointMemento> points)
  : WorkoutWidgetCommand(w), here(here), offset(offset), shift(shift), points(points) {}


void PasteCommand::undo()
{
    // remove the added points
    for(int i=points.count(); i>0; i--) {
        WWPoint *take = workoutWidget()->points().takeAt(here == -1 ? (workoutWidget()->points().count()-1) : here);
        delete take;
    }

    // unshift
    if (here != -1) {
        for(int i=here; i<workoutWidget()->points().count(); i++) {
            workoutWidget()->points()[i]->x -= shift;
        }
    }

    // reduce maxWX if needed, never less than an hour
    if (workoutWidget()->points().count() && workoutWidget()->points().last()->x < workoutWidget()->maxWX() &&
        workoutWidget()->maxWX()>3600) {
        if (workoutWidget()->points().last()->x<3600)
            workoutWidget()->setMaxWX(3600);
        else
            workoutWidget()->setMaxWX(workoutWidget()->points().last()->x);
    }
}

void PasteCommand::redo()
{
    // if its the last point append!
    if (here != -1) {
        for(int i=here; i<workoutWidget()->points().count(); i++) {
            workoutWidget()->points()[i]->x += shift;
        }
    }

    // here is either the index to append after
    // or we add to the end of the workout
    foreach(PointMemento m, points) {

        if (here == -1) {
            new WWPoint(workoutWidget(), m.x+offset, m.y);
        } else {

            WWPoint *add = new WWPoint(workoutWidget(), m.x+offset, m.y, false);
            workoutWidget()->points().insert(here + m.index, add);
        }
    }

    // increase maxWX if needed?
    if (workoutWidget()->points().count() && workoutWidget()->points().last()->x > workoutWidget()->maxWX())
        workoutWidget()->setMaxWX(workoutWidget()->points().last()->x);

}

QWKCommand::QWKCommand(WorkoutWidget *w, QString before, QString after)
  : WorkoutWidgetCommand(w), before(before), after(after) {}

void
QWKCommand::redo()
{
    workoutWidget()->apply(after);
}

void
QWKCommand::undo()
{
    workoutWidget()->apply(before);
}
