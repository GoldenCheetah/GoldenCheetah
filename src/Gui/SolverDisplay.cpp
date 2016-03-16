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
    points.resize(4);
}

void
SolverDisplay::addPoint(SolverPoint p)
{
    if (p.z > 100) points[0] << p;
    else if (p.z > 15) points[1] << p;
    else if (p.z > 5)  points[2] << p;
    else  points[3] << p;

    if (count++%1000 == 0) repaint();
}

void
SolverDisplay::reset()
{
    for(int i=0; i<4; i++) points[i].clear();
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

    // paint a dot for each x and y based upon
    // a scale for the size of the chart
    double xratio = double(geometry().width()) / double(constraints.cpto - constraints.cpf);
    double yratio = double(geometry().height()) / double(constraints.wto - constraints.wf);

    double my=0;
    double sy=50000;
    for (int i=0; i<4; i++) {
    foreach(SolverPoint p, points[i]) {

        double px = (p.x - constraints.cpf) * xratio;
        double py = geometry().height() - (p.y - constraints.wf) * yratio;
        int size = 1;
        QColor bcolor;

        switch(i) {
        default:
        case 0:  { bcolor = QColor(0xed,0xf8,0xe9); size = 1; } break;
        case 1:  { bcolor = QColor(0xba,0xe4,0xb3); size = 2; } break;
        case 2:  { bcolor = QColor(0x74,0xc4,0x76); size = 3; } break;
        case 3:  { bcolor = QColor(0x23,0x8b,0x45); size = 5; } break;
        }

        painter.setBrush(bcolor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(px,py), size,size);

        if (p.y > my) my=p.y;
        if (p.y < sy) sy=p.y;
    }
    }
    painter.restore();
    //qDebug()<<"max W'="<<my<<"small W'="<<sy;
}
