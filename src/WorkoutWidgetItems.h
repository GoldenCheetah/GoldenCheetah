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

#ifndef _GC_WorkoutWidgetItems_h
#define _GC_WorkoutWidgetItems_h 1

#include "GoldenCheetah.h"

#include "WorkoutWidget.h" // also declares workoutwidgetitem

#include "Settings.h"
#include "Units.h"
#include "Colors.h"

#include <QWidget>
#include <QRect>

// types
#define GCWW_SCALE  1
#define GCWW_POINT  2
#define GCWW_LINE   3
#define GCWW_WSCALE 4 //W'bal

//
// ITEMS
//
class WWPowerScale : public WorkoutWidgetItem {

    public:

        WWPowerScale(WorkoutWidget *w, Context *c);

        // Reimplement in children
        int type() { return GCWW_SCALE; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return workoutWidget()->left(); }

    private:

        Context *context; // for athlete zones etc
};

class WWBalScale : public WorkoutWidgetItem {

    public:

        WWBalScale(WorkoutWidget *w, Context *c);

        // Reimplement in children
        int type() { return GCWW_WSCALE; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return workoutWidget()->right(); }

    private:

        Context *context; // for athlete zones etc
};

// is a point, can be manipulated ...
class WWPoint : public WorkoutWidgetItem {

    public:

        WWPoint(WorkoutWidget *w, double x, double y, bool append=true) : 
                WorkoutWidgetItem(w), hover(false), selected(false), x(x), y(y) { if (append) w->addPoint(this); }

        // Reimplement in children
        int type() { return GCWW_POINT; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return bound; }
        bool hover;
        bool selected;

        double x, y;

    private:
        QRectF bound; // set when we paint
};

// draws a line between all the points
class WWLine : public WorkoutWidgetItem {

    public:

        WWLine(WorkoutWidget *w) : WorkoutWidgetItem(w) { w->addItem(this); }

        // Reimplement in children
        int type() { return GCWW_LINE; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return QRectF(); }

    private:

};

//
// COMMANDS
//
class CreatePointCommand : public WorkoutWidgetCommand
{
    public:
        CreatePointCommand(WorkoutWidget *w, double x, double y, int index);
        void redo();
        void undo();

    private:

        //state info
        double x,y;
        int index;
};

class MovePointCommand : public WorkoutWidgetCommand
{
    public:
            MovePointCommand(WorkoutWidget *w, QPointF before, QPointF after, int index);
        void redo();
        void undo();

    private:

        //state info
        QPointF before, after;
        int index;
};

class ScaleCommand : public WorkoutWidgetCommand
{
    public:
        ScaleCommand(WorkoutWidget *w, double up, double down, bool scaleup);

        void redo();
        void undo();

    private:
        double up, down;
        bool scaleup;
};
#endif // _GC_WorkoutWidgetItems_h
