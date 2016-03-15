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
}

void
SolverDisplay::addPoint(SolverPoint p)
{
    points << p;

    if (points.count()%1000 == 0) repaint();
}

void
SolverDisplay::reset()
{
    points.clear();
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
    foreach(SolverPoint p, points) {

        double px = (p.x - constraints.cpf) * xratio;
        double py = geometry().height() - (p.y - constraints.wf) * yratio;

        QColor bcolor(127,127,127,64);
        painter.setBrush(bcolor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(px,py), 2,2);

        if (p.y > my) my=p.y;
        if (p.y < sy) sy=p.y;
    }
    painter.restore();
    //qDebug()<<"max W'="<<my<<"small W'="<<sy;
}
