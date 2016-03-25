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

#include "SolverDisplay.h"

SolverDisplay::SolverDisplay(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    points.resize(5);
    count=0;
}

void
SolverDisplay::addPoint(SolverPoint p)
{
    if (p.z > 100) ;//points[0] << p; // disabled since we don't plot
    else if (p.z > 50) points[1] << p;
    else if (p.z > 5) points[2] << p;
    else if (p.z > 1)  points[3] << p;
    else  points[4] << p;

    if (count++%1000 == 0) {
        //qDebug()<<"HQS="<<points[3].count();
        repaint();
    }
}

void
SolverDisplay::reset()
{
    for(int i=0; i<5; i++) points[i].clear();
    count=0;
    repaint();
}

void
SolverDisplay::resizeEvent(QResizeEvent*p)
{
    QWidget::resizeEvent(p);
    repaint();
}


void
SolverDisplay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.save();

    // use antialiasing when drawing
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(QRectF(0,0, geometry().width(), geometry().height()), QBrush(Qt::white));

    QPen pen(Qt::darkGray);
    pen.setWidthF(1.0f);
    painter.setPen(pen);
    painter.drawRect(QRectF(0,0, geometry().width(), geometry().height()));

    // paint a dot for each x and y based upon
    // a scale for the size of the chart
    double xratio = double(geometry().width()) / double(constraints.cpto - constraints.cpf);
    double yratio = double(geometry().height()) / double(constraints.wto - constraints.wf);

    double my=0;
    double sy=50000;
    for (int i=1; i<5; i++) {
    foreach(SolverPoint p, points[i]) {

        double px = (p.x - constraints.cpf) * xratio;
        double py = geometry().height() - (p.y - constraints.wf) * yratio;
        int size = 1;
        QColor bcolor;

        switch(i) {
        default:
        case 0:  { bcolor = QColor(0xed,0xf8,0xe9); size = 1; } break;
        case 1:  { bcolor = QColor(0xed,0xf8,0xe9); size = 1; } break;
        case 2:  { bcolor = QColor(0xba,0xe4,0xb3); size = 2; } break;
        case 3:  { bcolor = QColor(0x74,0xc4,0x76); size = 2; } break;
        case 4:  {
                    if (p.t < 40) bcolor=QColor(255,0,0); // diff low
                    else if (p.t < 60) bcolor = QColor(0x23,0x8b,0x45); // diff mid
                    else if (p.t < 300) bcolor = QColor(0,0,255); // diff high
                    else if (p.t < 433)  bcolor=QColor(255,0,0); // integral low
                    else if (p.t < 566) bcolor = QColor(0x23,0x8b,0x45); // integral mid
                    else if (p.t <= 700) bcolor = QColor(0,0,255); // integral high

                    size=3;
                 }
                 break;
        }

        painter.setBrush(bcolor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(px,py), size,size);

        if (p.y > my) my=p.y;
        if (p.y < sy) sy=p.y;
    }
    }

    // configured zone, with 10% added for good measure
    QRectF c;

    // now convert the points of the rectangle to pixels
    c.setX(((double(constraints.ccpf)*0.9f) - constraints.cpf) * xratio);
    c.setY(geometry().height() - ((double(constraints.cwto)*1.1f) - constraints.wf) * yratio);

    // width
    c.setWidth(((double(constraints.ccpto)*1.1f)-(double(constraints.ccpf)*0.9f)) * xratio);
    c.setHeight(((double(constraints.cwto) *1.1f) - (double(constraints.cwf)*0.9f)) * yratio);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(Qt::red);
    painter.drawRect(c);

    // mouse position
    if (underMouse() && count > 0) {

        // where is the mouse?
        QPoint p = mapFromGlobal(QCursor::pos());

        double cp = (double(p.x()) / xratio) + constraints.cpf;
        double w = ((double(height()-p.y()) / yratio) + constraints.wf) / 1000.0f;

        // W' and CP co-ordinates in top right
        QString label = QString("CP %1 W' %2").arg(cp,0,'f',0).arg(w,0,'f',1);

        // top right and 5px gap
        QFont font;
        QFontMetrics fm(font);
        QRect t = fm.boundingRect(label);

        QRect s;
        s.setY(geometry().height() - (t.height()+5));
        s.setX(5);
        s.setWidth(width()-10);
        s.setHeight(t.height());

        painter.setFont(font);
        painter.setPen(QPen(Qt::darkGray));
        painter.drawText(s,label);
    }

    painter.restore();
    //qDebug()<<"max W'="<<my<<"small W'="<<sy;
}
