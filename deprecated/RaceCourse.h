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

#ifndef _GC_RaceCourse_h
#define _GC_RaceCourse_h 1
#include "GoldenCheetah.h"

#include <QtGui>

#include "TrainTool.h"


class RaceCourse : public QGraphicsScene
{
    Q_OBJECT
    G_OBJECT


    public:

        RaceCourse(QWidget *, TrainTool *);

    public slots:

    protected:
        TrainTool *trainTool;
};

class Cheetah : public QGraphicsItem
{
    public:
        Cheetah(QColor, QGraphicsScene *);
        void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget*);
        QRectF boundingRect() const;
        void setPos(int, int);

    private:
        QImage icon;
        QColor color;
        int x,y; // position on graphics scene
};

#endif // _GC_RaceCourse_h
