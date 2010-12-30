/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "RaceCourse.h"
#include <QtGui>
#include <QImage>

Cheetah::Cheetah(QColor color, QGraphicsScene *scene) : QGraphicsItem(0, scene), color(color), x(0), y(0)
{
    QImage png;
    png.load(":/images/cheetah.png");

    // convert to a colormapped icon which is nice and small for the display
    icon = png.convertToFormat(QImage::Format_Indexed8).scaledToHeight(500);
    icon.setColor(0, color.rgb());
}

void
Cheetah::setPos(int x, int y)
{
    this->x = x;
    this->y = y;
}

void
Cheetah::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget*)
{
    painter->drawImage(x,y,icon);
}

QRectF
Cheetah::boundingRect() const
{
    return icon.rect();
}

RaceCourse::RaceCourse(QWidget *parent, TrainTool *trainTool) : QGraphicsScene(parent), trainTool(trainTool)
{
    // and a course
    QGraphicsLineItem *lane0 = new QGraphicsLineItem (0, 0000, 100000, 0000, NULL, this);
    QGraphicsLineItem *lane1 = new QGraphicsLineItem (0, 1000, 100000, 1000, NULL, this);
    QGraphicsLineItem *lane2 = new QGraphicsLineItem (0, 2000, 100000, 2000, NULL, this);
    QGraphicsLineItem *lane3 = new QGraphicsLineItem (0, 3000, 100000, 3000, NULL, this);
    QGraphicsLineItem *lane4 = new QGraphicsLineItem (0, 4000, 100000, 4000, NULL, this);
    QGraphicsLineItem *lane5 = new QGraphicsLineItem (0, 5000, 100000, 5000, NULL, this);
    QGraphicsLineItem *lane6 = new QGraphicsLineItem (0, 6000, 100000, 6000, NULL, this);
    QGraphicsLineItem *lane7 = new QGraphicsLineItem (0, 7000, 100000, 7000, NULL, this);
    QGraphicsLineItem *lane8 = new QGraphicsLineItem (0, 8000, 100000, 8000, NULL, this);

    // the 1km markers
    QPen penk;
    QPen pen200m;

    penk.setColor(Qt::red);
    penk.setWidth(4);
    pen200m.setColor(Qt::darkGray);
    pen200m.setStyle(Qt::DashLine);
    for (int i=0; i<= 100000; i+= 1000) {
        QGraphicsLineItem *marker = new QGraphicsLineItem (i, 0, i, 8000, NULL, this);
        if (i%5000 == 0)
            marker->setPen(penk);
        else
            marker->setPen(pen200m);
    }

    Cheetah *one = new Cheetah(QColor(Qt::red), this);
    one->setPos(0,0);
    Cheetah *two = new Cheetah(QColor(Qt::green), this);
    two->setPos(0,1000);
    Cheetah *three = new Cheetah(QColor(Qt::blue), this);
    three->setPos(0,2000);
    Cheetah *four = new Cheetah(QColor(Qt::yellow), this);
    four->setPos(0, 3000);
    Cheetah *five = new Cheetah(QColor(Qt::gray), this);
    five->setPos(0, 4000);
    Cheetah *six = new Cheetah(QColor(Qt::black), this);
    six->setPos(0, 5000);
    Cheetah *seven = new Cheetah(QColor(Qt::darkRed), this);
    seven->setPos(0, 6000);
    Cheetah *eight = new Cheetah(QColor(Qt::darkGreen), this);
    eight->setPos(0, 7000);
}
